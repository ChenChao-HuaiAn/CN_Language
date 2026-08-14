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
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/opt/cfg.hpp"
#include "cn_compiler/opt/licm.hpp"
#include "cn_compiler/opt/opt_common.hpp"

namespace cn_compiler {
namespace opt {

// 收集循环体内定义的寄存器集合与写入的槽集合
void LICMPass::collectBodyDefs(
    const ir::IRFunction& fn, const std::vector<int>& bodyIndexes,
    std::unordered_set<int>& defRegs,
    std::unordered_set<std::string>& writtenSlots) {
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

// 单函数 LICM：识别循环 + 外提不变量
bool LICMPass::runFunction(ir::IRFunction& fn) {
    if (fn.blocks.size() < 2) return false;
    DomTree dom;
    dom.rebuild(fn);
    const std::vector<DomTree::Loop> loops = dom.findNaturalLoops();
    if (loops.empty()) return false;
    bool changed = false;

    // 对每个循环：确定 preheader（header 的循环外前驱）
    for (const auto& loop : loops) {
        const int header = loop.header;
        std::vector<int> preheaders;
        for (const int p : dom.predecessors(header)) {
            // 循环外前驱 = 不在循环体块集合中
            bool inBody = false;
            for (const int b : loop.body) {
                if (b == p) { inBody = true; break; }
            }
            if (!inBody) preheaders.push_back(p);
        }
        if (preheaders.empty()) continue;  // 无 preheader：跳过（保守）
        // preheader 选择第一个循环外前驱（多入口循环保守处理）
        const int preheader = preheaders[0];
        ir::IRBlock* preBlock = fn.blocks[static_cast<std::size_t>(preheader)].get();

        // 1. 收集循环体定义寄存器与写入槽
        std::unordered_set<int> defRegs;
        std::unordered_set<std::string> writtenSlots;
        collectBodyDefs(fn, loop.body, defRegs, writtenSlots);

        // 2. 迭代计算"不变寄存器"集合（不动点）：
        //    初始 = 循环外定义的寄存器；每轮把"操作数全为常量/不变寄存器/
        //    未写槽"的纯运算结果加入，直到无新增
        std::unordered_set<int> invariantRegs;
        for (const int r : defRegs) {
            // 循环外定义的寄存器（体内 Load 的定义在 defRegs 中——
            // 需按"其指令本身是否不变"判定，故从空集开始迭代）
            (void)r;
        }
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
    bool changed = false;
    for (auto& fn : module.functions) {
        if (runFunction(fn)) changed = true;
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
