// CN语言优化器：跨块 DCE 增强 Pass 实现（Task 完善C 优化器增强）
// 实现要点：
//   1. foldConstantBranch：条件跳转块的最后一条指令 operands 尾部挂条件
//      常量（ConstFold 尾部传播产物，形如 [常量, 常量]）：
//      - 条件为"真"/"1" -> 折叠为无条件跳转到 termTrueTarget
//      - 条件为"假"/"0" -> 折叠为无条件跳转到 termFalseTarget
//   2. collectReachable：入口（blocks[0]）BFS，沿"跳转"/"条件跳转"目标遍历
//      （label->索引映射 + 索引队列，O(块数+边数)；旧实现逐 label 线性扫块）
//   3. removeUnreachableBlocks：删除可达位图中未标记的块
//      （从尾部向前删除，剩余块索引不变，label 互引无需修正）
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/opt/cross_block_dce.hpp"

namespace cn_compiler {
namespace opt {

namespace {

// 布尔常量文本 -> 值（"真"/"1" -> true；"假"/"0" -> false）；失败返回 false
bool parseBoolText(const std::string& text, bool& out) {
    if (text == "真" || text == "1") { out = true; return true; }
    if (text == "假" || text == "0") { out = false; return true; }
    return false;
}

} // namespace

// 将条件跳转折叠为无条件跳转（条件常量"真"->真目标，"假"->假目标）
// 条件取 block.termCondition（280-a T12 字段化，不再寄生于块尾指令）
bool CrossBlockDCEPass::foldConstantBranch(ir::IRBlock& block) {
    if (block.termKind != "条件跳转") return false;
    // 条件为常量文本（"真"/"假"/"1"/"0"）时可折叠；寄存器条件不折叠
    const std::string& cond = block.termCondition;
    if (cond.empty() || (cond.size() > 2 && cond[0] == '%' && cond[1] == 'v')) {
        return false;
    }
    bool value = false;
    if (!parseBoolText(cond, value)) return false;
    // 折叠：条件跳转 -> 无条件跳转（目标由条件常量决定）
    block.termKind = "跳转";
    block.termTarget = value ? block.termTrueTarget : block.termFalseTarget;
    block.termTrueTarget.clear();
    block.termFalseTarget.clear();
    block.termCondition.clear();
    return true;
}

// 收集函数内可达块集合（从入口 BFS，沿跳转目标遍历）。
//   旧实现按 label 逐个线性扫描全部块（O(块数²) 字符串比较）；现先建
//   label->索引映射，BFS 直接按下标访问（O(块数+边数)）。
//   label 唯一性=IRGenerator 契约；重复 label 取首个（与旧实现首个命中一致）。
std::vector<char> CrossBlockDCEPass::collectReachable(
    const ir::IRFunction& fn,
    const std::unordered_map<std::string, int>& labelIndex) {
    std::vector<char> reachable(fn.blocks.size(), 0);
    if (fn.blocks.empty()) return reachable;
    // 待处理块索引队列（vector 作队列：head 指针推进，避免 deque 分配）
    std::vector<int> pending;
    pending.reserve(fn.blocks.size());
    // 入口块 = blocks[0]（IRGenerator 保证第一个块为函数入口）
    pending.push_back(0);
    reachable[0] = 1;
    for (std::size_t head = 0; head < pending.size(); ++head) {
        const ir::IRBlock& block = *fn.blocks[static_cast<std::size_t>(pending[head])];
        if (!block.terminated) continue;
        // 目标 label 入队（未能映射到块的目标忽略——旧实现同样无块可遍历）
        const auto enqueue = [&](const std::string& target) {
            if (target.empty()) return;
            const auto it = labelIndex.find(target);
            if (it == labelIndex.end()) return;
            const int t = it->second;
            if (reachable[static_cast<std::size_t>(t)]) return;
            reachable[static_cast<std::size_t>(t)] = 1;
            pending.push_back(t);
        };
        if (block.termKind == "跳转") {
            enqueue(block.termTarget);
        } else if (block.termKind == "条件跳转") {
            enqueue(block.termTrueTarget);
            enqueue(block.termFalseTarget);
        }
    }
    return reachable;
}

// 删除不可达块（返回是否删除）；reachable 按块索引标记。
//   从尾部向前删除：删除位置之后的元素已处理完，剩余块索引不变，
//   label 字符串互引无需修正。
bool CrossBlockDCEPass::removeUnreachableBlocks(
    ir::IRFunction& fn, const std::vector<char>& reachable) {
    bool removed = false;
    auto& blocks = fn.blocks;
    for (std::size_t i = blocks.size(); i-- > 0;) {
        if (reachable[i]) continue;
        blocks.erase(blocks.begin() + static_cast<std::ptrdiff_t>(i));
        removed = true;
    }
    return removed;
}

// 遍历模块全部函数：折叠常量条件跳转 + 删除不可达块
bool CrossBlockDCEPass::run(ir::IRModule& module) {
    bool changed = false;
    for (auto& fn : module.functions) {
        // 第一步：折叠常量条件跳转（恒真/恒假 -> 无条件跳转）
        for (auto& block : fn.blocks) {
            if (foldConstantBranch(*block)) changed = true;
        }
        // 第二步：可达性分析 + 删除不可达块
        // 单块函数无可删（入口自身必可达）：跳过索引映射构建，省哈希/分配
        if (fn.blocks.empty() || fn.blocks.size() == 1) continue;
        // label->索引映射（每函数建一次；与 cfg.DomTree::rebuild 的索引映射
        // 同构，第二处重复——待归纳，见质量纪律二）
        std::unordered_map<std::string, int> labelIndex;
        labelIndex.reserve(fn.blocks.size() * 2);
        for (std::size_t i = 0; i < fn.blocks.size(); ++i) {
            labelIndex.emplace(fn.blocks[i]->label, static_cast<int>(i));
        }
        const std::vector<char> reachable = collectReachable(fn, labelIndex);
        if (removeUnreachableBlocks(fn, reachable)) changed = true;
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
