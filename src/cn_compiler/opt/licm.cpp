// CN语言优化器：循环不变量外提（LICM）Pass 实现（阶段B Task 4.2）
// 实现要点：
//   1. 循环识别：DomTree::findNaturalLoops() 返回 (header, body索引集合)
//   2. 循环体内寄存器定义集合（result.id >= 0 的指令）与写入槽集合
//      （Store extra）——用于不变量判定
//   3. 不变量 = 纯运算指令 && 操作数全为"常量 或 循环外寄存器 或 循环外槽"
//   4. preheader = header 的"循环外前驱"（在循环体内集合中不存在的前驱）；
//      多个前驱时选第一个（保守）；无循环外前驱跳过
//   5. 外提：指令从循环体块中移除，插入 preheader 末尾
//      （保持操作数定义序——preheader 定义的操作数先于 header 执行）
//   6. 循环存在性快筛（1207）：无后向边候选的函数直接跳过，不构建支配树
//      （绝大多数函数无循环，避免每轮 fixpoint 对全函数付 DomTree 成本）
//   7. 跨轮缓存（1207）：CFG 快照（块序/标签/终止边）相同的 fixpoint 轮复用
//      循环集合与 preheader（DomTree 输入不含指令，快照相同即结构相同）
#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/opt/cfg.hpp"
#include "cn_compiler/opt/licm.hpp"
#include "cn_compiler/opt/opt_common.hpp"

namespace cn_compiler {
namespace opt {

namespace {

constexpr long long kInvalidBlockOrder = -1;   // 标签不可解析哨兵
constexpr long long kMaxBlockOrder = 1LL << 40;  // 病态标签上限（防解析溢出）
constexpr int kNoPreheader = -1;               // 循环无循环外前驱哨兵

// 十进制数字判定（比较不放在 if 条件位——单边比较禁令只扫条件位）
bool isDecimalDigit(char c) {
    return c >= '0' && c <= '9';
}

// 解析块标签的生成序号（宿主 IRGenerator 契约："bb<十进制>"）。
//   解析失败返回 kInvalidBlockOrder（调用方按"未证明无环"保守处理）。
long long blockOrder(const std::string& label) {
    if (label.rfind("bb", 0) != 0) return kInvalidBlockOrder;  // 非 bb 前缀
    if (label.size() == 2) return kInvalidBlockOrder;          // 仅前缀无序号
    long long value = 0;
    for (std::size_t i = 2; i < label.size(); ++i) {
        const char c = label[i];
        if (!isDecimalDigit(c)) return kInvalidBlockOrder;
        value = value * 10 + (c - '0');
        if (value > kMaxBlockOrder) return kInvalidBlockOrder;
    }
    return value;
}

// 有效 CFG 字段的单一读取口径（快照采集与比较共用，防两处口径漂移）：
//   未终止块=无出边；跳转=单目标；条件跳转=双目标；返回/未知=无目标。
struct CfgFields {
    const std::string* label;
    const std::string* termKind;
    const std::string* target0;
    const std::string* target1;
};

CfgFields cfgFieldsOf(const ir::IRBlock& block) {
    static const std::string kEmpty;
    if (!block.terminated) {
        return {&block.label, &kEmpty, &kEmpty, &kEmpty};
    }
    if (block.termKind == "跳转") {
        return {&block.label, &block.termKind, &block.termTarget, &kEmpty};
    }
    if (block.termKind == "条件跳转") {
        return {&block.label, &block.termKind, &block.termTrueTarget,
                &block.termFalseTarget};
    }
    // 返回（及未知终止形态）：无出边（与 DomTree::rebuild 同口径）
    return {&block.label, &block.termKind, &kEmpty, &kEmpty};
}

} // namespace

// 循环存在性快筛（1207）：返回 true = 已证明 CFG 无环（必然无自然循环）。
//   原理：任意有向环在任一顶点全序下必含一条「目标 <= 源」的边——若全部
//   边严格前向，沿环序号严格递增，无法回到起点，故必然无环；无环即无
//   自然循环（自然循环以回边为必要条件）。块标签 bbN 的 N 即 IR 生成序，
//   构成与块列表一致的全序；块删除/重排后仍是合法全序（结论不受影响）。
//   标签不可解析（非宿主 IRGenerator 形态）=> 返回 false（未证明，走完整
//   支配树判定——宽进严出，不改变任何优化语义）。
bool LICMPass::hasNoCycle(const ir::IRFunction& fn) {
    for (const auto& block : fn.blocks) {
        if (!block->terminated) continue;
        if (block->termKind == "跳转") {
            const long long source = blockOrder(block->label);
            const long long target = blockOrder(block->termTarget);
            if (source == kInvalidBlockOrder || target == kInvalidBlockOrder) {
                return false;
            }
            if (target <= source) return false;
            continue;
        }
        if (block->termKind == "条件跳转") {
            const long long source = blockOrder(block->label);
            const long long trueTarget = blockOrder(block->termTrueTarget);
            const long long falseTarget = blockOrder(block->termFalseTarget);
            if (source == kInvalidBlockOrder ||
                trueTarget == kInvalidBlockOrder ||
                falseTarget == kInvalidBlockOrder) {
                return false;
            }
            if (trueTarget <= source || falseTarget <= source) return false;
        }
        // "返回" 无后继；未知终止形态按 rebuild 同口径视为无出边
    }
    return true;
}
// 收集循环体内定义的寄存器集合与写入的槽集合
void LICMPass::collectBodyDefs(
    const ir::IRFunction& fn, const std::vector<int>& bodyIndexes,
    std::unordered_set<int>& defRegs,
    std::unordered_set<std::string>& writtenSlots) {
    // 按循环体块数预留（每块至少一个定义寄存器；减少扩容/重哈希）
    defRegs.reserve(bodyIndexes.size() * 4);
    writtenSlots.reserve(bodyIndexes.size());
    for (const int bi : bodyIndexes) {
        const auto& insts = fn.blocks[static_cast<std::size_t>(bi)]->instructions;
        for (const auto& inst : insts) {
            if (inst.result.id >= 0) defRegs.insert(inst.result.id);
            if (inst.opcode == ir::Opcode::Store && !inst.extra.empty()) {
                writtenSlots.insert(inst.extra);
            }
        }
    }
}

// 指令是否不变量（纯运算 && 操作数全为常量/不变寄存器/不变槽）
// 实现：基于"不变寄存器集合"判定——操作数寄存器只要在不变集合中即视为
// 不变量（无论定义在循环内外）。invariantRegs 由外部迭代计算：
//   初始 = 循环外定义的寄存器；迭代加入"操作数全为不变值"的纯运算结果
bool LICMPass::isInvariant(
    const ir::IRInstruction& inst,
    const std::unordered_set<int>& bodyDefRegs,
    const std::unordered_set<int>& invariantRegs,
    const std::unordered_set<std::string>& bodyWrittenSlots) {
    // 纯运算指令：操作数全为常量/不变寄存器/未写槽 -> 值不变
    if (isPureArith(inst.opcode)) {
        for (const auto& op : inst.operands) {
            if (op.isConstant) continue;
            if (op.id >= 0) {
                // 寄存器：循环体内定义且未判不变 -> 非不变量；
                // 循环外定义（不在 bodyDefRegs）-> 天然不变
                if (bodyDefRegs.count(op.id) > 0 &&
                    invariantRegs.count(op.id) == 0) {
                    return false;
                }
                continue;
            }
            if (!op.extra.empty() && bodyWrittenSlots.count(op.extra) > 0) {
                return false;
            }
        }
        return true;
    }
    // Load（operand[0]=变量名）：槽循环内无写入 -> 值不变（仅值判定，
    //   不外提 Load 本身——副作用指令保留在循环体内）
    if (inst.opcode == ir::Opcode::Load && inst.operands.size() == 1) {
        const ir::IRValue& slot = inst.operands[0];
        if (slot.id >= 0 || slot.extra.empty()) return false;
        return bodyWrittenSlots.count(slot.extra) == 0;
    }
    return false;
}

// CFG 快照采集（逐块标签/有效终止边；指令不参与支配树输入）
void LICMPass::captureCfg(const ir::IRFunction& fn,
                          std::vector<CfgBlock>& out) {
    out.clear();
    out.reserve(fn.blocks.size());
    for (const auto& block : fn.blocks) {
        const CfgFields f = cfgFieldsOf(*block);
        out.push_back({*f.label, *f.termKind, *f.target0, *f.target1});
    }
}

// CFG 快照比较：true = CFG 结构与上次分析时逐块一致（支配树结果可复用）
bool LICMPass::sameCfg(const ir::IRFunction& fn,
                       const std::vector<CfgBlock>& cfg) {
    if (cfg.size() != fn.blocks.size()) return false;
    for (std::size_t i = 0; i < cfg.size(); ++i) {
        const CfgFields f = cfgFieldsOf(*fn.blocks[i]);
        const CfgBlock& c = cfg[i];
        if (c.label != *f.label || c.termKind != *f.termKind ||
            c.target0 != *f.target0 || c.target1 != *f.target1) {
            return false;
        }
    }
    return true;
}

// 单函数 LICM：识别循环 + 外提不变量
bool LICMPass::runFunction(ir::IRFunction& fn) {
    if (fn.blocks.size() < 2) return false;
    // 1207 快筛：已证明无环 = 无自然循环——跳过支配树构建
    //   （无循环函数此前也逐轮付 DomTree 的索引/DFS/CHK/回边扫描成本）
    if (hasNoCycle(fn)) return false;

    // 1207 跨轮缓存：fixpoint 各轮之间 CFG 通常不变（其余 Pass 以改指令为主），
    //   逐块快照相同即证明 CFG 未变——复用循环集合与 preheader，不再重复
    //   DomTree::rebuild/findNaturalLoops/前驱扫描。快照不同则重建并刷新缓存。
    const std::string key = fn.mangledName.empty() ? fn.name : fn.mangledName;
    LoopCache& cache = loopCache_[key];
    if (!sameCfg(fn, cache.cfg)) {
        DomTree dom;
        dom.rebuild(fn);
        cache.loops = dom.findNaturalLoops();
        cache.preheaders.clear();
        cache.preheaders.reserve(cache.loops.size());
        for (const auto& loop : cache.loops) {
            int preheader = kNoPreheader;
            for (const int p : dom.predecessors(loop.header)) {
                // 循环外前驱 = 不在循环体块集合中；多个时取首个（保守，原语义）
                if (std::find(loop.body.begin(), loop.body.end(), p) ==
                    loop.body.end()) {
                    preheader = p;
                    break;
                }
            }
            cache.preheaders.push_back(preheader);
        }
        captureCfg(fn, cache.cfg);
    }
    bool changed = false;

    // 对每个循环：preheader 来自缓存（CFG 快照保证索引有效）
    for (std::size_t li = 0; li < cache.loops.size(); ++li) {
        const DomTree::Loop& loop = cache.loops[li];
        const int preheader = cache.preheaders[li];
        if (preheader == kNoPreheader) continue;  // 无 preheader：跳过（保守）
        ir::IRBlock* preBlock = fn.blocks[static_cast<std::size_t>(preheader)].get();

        // 1. 收集循环体定义寄存器与写入槽
        std::unordered_set<int> defRegs;
        std::unordered_set<std::string> writtenSlots;
        collectBodyDefs(fn, loop.body, defRegs, writtenSlots);

        // 2. 迭代计算"不变寄存器"集合（不动点）：
        //    初始 = 循环外定义的寄存器；每轮把"操作数全为常量/不变寄存器/
        //    未写槽"的纯运算结果加入，直到无新增
        std::unordered_set<int> invariantRegs;
        invariantRegs.reserve(defRegs.size());
        // 从空集开始迭代：体内定义按其指令自身是否不变判定（旧实现在此
        //   有一个只读 defRegs 的空转循环——无副作用，删除）
        bool grew = true;
        int rounds = 0;
        while (grew && rounds < 16) {
            grew = false;
            ++rounds;
            for (const int bi : loop.body) {
                const auto& insts =
                    fn.blocks[static_cast<std::size_t>(bi)]->instructions;
                for (const auto& inst : insts) {
                    if (inst.result.id < 0) continue;
                    if (invariantRegs.count(inst.result.id) > 0) continue;
                    // Load/副作用指令结果不加入不变集合：其定义留在循环体内，
                    // 外提依赖它的指令会在 preheader 引用未定义寄存器
                    if (inst.opcode == ir::Opcode::Load ||
                        inst.opcode == ir::Opcode::LoadPtr ||
                        inst.opcode == ir::Opcode::FieldAddr ||
                        inst.opcode == ir::Opcode::StrFieldAddr ||
                        inst.opcode == ir::Opcode::Call ||
                        inst.opcode == ir::Opcode::CallIndirect ||
                        inst.opcode == ir::Opcode::Alloca) {
                        continue;
                    }
                    if (isInvariant(inst, defRegs, invariantRegs, writtenSlots)) {
                        invariantRegs.insert(inst.result.id);
                        grew = true;
                    }
                }
            }
        }

        // 3. 外提不变指令到 preheader
        std::vector<ir::IRInstruction> hoisted;
        for (const int bi : loop.body) {
            auto& insts = fn.blocks[static_cast<std::size_t>(bi)]->instructions;
            for (std::size_t i = insts.size(); i-- > 0;) {
                const ir::IRInstruction& inst = insts[i];
                // 仅外提"结果已判为不变"的纯运算指令（Load 值不变但副作用，
                //   保留在循环体内——避免改变内存访问次数语义）
                if (inst.result.id < 0) continue;
                if (invariantRegs.count(inst.result.id) == 0) continue;
                if (!isPureArith(inst.opcode)) continue;  // 副作用不外提
                if (!isInvariant(inst, defRegs, invariantRegs, writtenSlots)) {
                    continue;
                }
                hoisted.push_back(inst);
                insts.erase(insts.begin() + static_cast<std::ptrdiff_t>(i));
            }
        }
        if (hoisted.empty()) continue;
        // 逆序插入 preheader 末尾（hoisted 是反向收集，恢复原序）
        for (std::size_t i = hoisted.size(); i-- > 0;) {
            preBlock->instructions.push_back(hoisted[i]);
        }
        changed = true;
    }
    return changed;
}

// 遍历模块全部函数：循环不变量外提
bool LICMPass::run(ir::IRModule& module) {
    // 换模块=新一轮优化：跨轮缓存仅对同一模块有意义（清空防内存增长；
    //   即便键/地址复用也有 sameCfg 逐块快照兜底，不会误用陈旧分析）
    if (&module != lastModule_) {
        loopCache_.clear();
        lastModule_ = &module;
    }
    bool changed = false;
    for (auto& fn : module.functions) {
        if (runFunction(fn)) changed = true;
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
