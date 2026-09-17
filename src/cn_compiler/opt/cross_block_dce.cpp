// CN语言优化器：跨块 DCE 增强 Pass 实现（Task 完善C 优化器增强）
// 实现要点：
//   1. foldConstantBranch：条件跳转块的最后一条指令 operands 尾部挂条件
//      常量（ConstFold 尾部传播产物，形如 [常量, 常量]）：
//      - 条件为"真"/"1" -> 折叠为无条件跳转到 termTrueTarget
//      - 条件为"假"/"0" -> 折叠为无条件跳转到 termFalseTarget
//   2. collectReachable：入口（blocks[0]）BFS，沿"跳转"/"条件跳转"目标遍历
//   3. removeUnreachableBlocks：删除 label 不在可达集合的块
//      （label 字符串互引，删除后无需修正索引）
#include <algorithm>
#include <queue>
#include <string>
#include <unordered_set>
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

// 收集函数内可达块 label 集合（从入口 BFS，沿跳转目标遍历）
std::unordered_set<std::string> CrossBlockDCEPass::collectReachable(
    const ir::IRFunction& fn) {
    std::unordered_set<std::string> reachable;
    if (fn.blocks.empty()) return reachable;
    std::queue<std::string> pending;
    // 入口块 = blocks[0]（IRGenerator 保证第一个块为函数入口）
    const std::string entry = fn.blocks[0]->label;
    pending.push(entry);
    reachable.insert(entry);
    while (!pending.empty()) {
        const std::string label = pending.front();
        pending.pop();
        // 找到对应块，收集其跳转目标
        for (const auto& block : fn.blocks) {
            if (block->label != label) continue;
            if (block->terminated) {
                if (block->termKind == "跳转" && !block->termTarget.empty()) {
                    if (reachable.insert(block->termTarget).second) {
                        pending.push(block->termTarget);
                    }
                } else if (block->termKind == "条件跳转") {
                    if (!block->termTrueTarget.empty() &&
                        reachable.insert(block->termTrueTarget).second) {
                        pending.push(block->termTrueTarget);
                    }
                    if (!block->termFalseTarget.empty() &&
                        reachable.insert(block->termFalseTarget).second) {
                        pending.push(block->termFalseTarget);
                    }
                }
            }
            break;
        }
    }
    return reachable;
}

// 删除不可达块（返回是否删除）；label 字符串互引，删除后无需修正索引
bool CrossBlockDCEPass::removeUnreachableBlocks(
    ir::IRFunction& fn, const std::unordered_set<std::string>& reachable) {
    bool removed = false;
    auto& blocks = fn.blocks;
    for (std::size_t i = blocks.size(); i-- > 0;) {
        const std::string& label = blocks[i]->label;
        if (reachable.find(label) != reachable.end()) continue;
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
        const std::unordered_set<std::string> reachable = collectReachable(fn);
        if (removeUnreachableBlocks(fn, reachable)) changed = true;
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
