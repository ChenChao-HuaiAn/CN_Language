// CN语言优化器：复写传播 Pass 实现（Task 完善C 优化器增强）
// 实现要点：
//   1. 槽值表：唯一内部名 -> 最近存储值（常量 IRValue 或寄存器 IRValue）
//   2. 遍历块内指令：
//      - Store（extra=变量名, operand[0]=值）：记录槽值表
//      - Load（operand[0]=变量名, result 为寄存器）：命中槽值表 ->
//        登记 结果寄存器 -> 存储值 的替换映射
//      - 可能写任意内存的指令（StorePtr/LoadPtr/Call/CallIndirect/
//        FieldAddr/AddrOf）：清空槽值表（保守）
//   3. 替换延迟到"应用替换"步骤（replaceUses 处理全部引用点）
#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/opt/copy_propagation.hpp"
#include "cn_compiler/opt/opt_common.hpp"

namespace cn_compiler {
namespace opt {

// 沿寄存器替换链解析最终寄存器 id（%v3->%v1->%v0 解析为 %v0）
int CopyPropagationPass::resolveReg(int id, const RegRewriteMap& regRewrite) {
    int current = id;
    for (int depth = 0; depth < 16; ++depth) {  // 防环保护
        auto it = regRewrite.find(current);
        if (it == regRewrite.end()) return current;
        current = it->second;
    }
    return current;
}

// 是否可能写入任意内存的指令（清空槽值表）
bool CopyPropagationPass::mayAliasWrite(ir::Opcode op) {
    switch (op) {
        case ir::Opcode::StorePtr:
        case ir::Opcode::LoadPtr:      // 指针加载可能触发空指针检查（副作用）
        case ir::Opcode::Call:
        case ir::Opcode::CallIndirect:
        case ir::Opcode::FieldAddr:    // 隐含空指针检查
        case ir::Opcode::AddrOf:       // 地址逃逸（指针可能别名）
            return true;
        default:
            return false;
    }
}

// 是否结构体/数组多槽变量（按 varSlots 判断；保守跳过）
bool CopyPropagationPass::isMultiSlot(
    const std::string& uniqueName,
    const std::unordered_map<std::string, int>& varSlots) {
    auto it = varSlots.find(uniqueName);
    if (it != varSlots.end() && it->second > 1) return true;
    return false;
}

// 遍历模块全部函数的全部基本块，块内转发 Store->Load
bool CopyPropagationPass::run(ir::IRModule& module) {
    bool changed = false;
    for (auto& fn : module.functions) {
        for (auto& block : fn.blocks) {
            // 槽值表：唯一内部名 -> 最近存储值
            std::unordered_map<std::string, ir::IRValue> slotValues;
            // 寄存器替换表：Load 结果寄存器 -> 存储值寄存器/常量
            RegRewriteMap regRewrite;
            ConstRewriteMap constRewrite;
            for (auto& inst : block->instructions) {
                // 第一步：应用已收集的替换到本指令的引用点
                //   （Load 命中槽值表登记替换后，后续指令（含 Store 的值、
                //    后续 Load 前的任何引用）都须同步替换）
                if (replaceUses(inst, regRewrite, constRewrite)) changed = true;
                // 第二步：按指令类别更新槽值表/登记新替换
                switch (inst.opcode) {
                    case ir::Opcode::Store: {
                        // Store：operand[0]=值寄存器/常量, extra=变量名（唯一内部名）
                        if (inst.extra.empty()) break;
                        if (isMultiSlot(inst.extra, fn.varSlots)) break;  // 多槽保守
                        if (inst.operands.empty()) break;
                        const ir::IRValue& value = inst.operands[0];
                        // 值若是寄存器且已被替换 -> 解析最终寄存器
                        if (!value.isConstant && value.id >= 0) {
                            slotValues[inst.extra] = ir::IRValue::reg(
                                resolveReg(value.id, regRewrite), value.type);
                        } else {
                            slotValues[inst.extra] = value;
                        }
                        break;
                    }
                    case ir::Opcode::Load: {
                        // Load：operand[0]=变量名（唯一内部名），result=寄存器
                        if (inst.operands.empty() || inst.result.id < 0) break;
                        const std::string& name = inst.operands[0].extra;
                        if (name.empty() || inst.operands[0].id >= 0) break;
                        auto it = slotValues.find(name);
                        if (it == slotValues.end()) break;
                        const ir::IRValue& stored = it->second;
                        // 类型匹配检查（Load 结果类型与存储值类型应一致）
                        if (!stored.type.empty() && !inst.type.empty() &&
                            stored.type != inst.type) {
                            break;  // 类型不一致：不转发（可能经 Cast 窄化）
                        }
                        // 登记替换（不设 changed：实际引用替换由 replaceUses 报告，
                        //   保证 fixpoint 收敛——重复运行时 Load 结果已无引用
                        //   则不报告修改）
                        if (stored.isConstant) {
                            constRewrite[inst.result.id] = stored;
                        } else if (stored.id >= 0) {
                            regRewrite[inst.result.id] = stored.id;
                        }
                        break;
                    }
                    default:
                        // 可能写任意内存的指令：清空槽值表（保守）
                        if (mayAliasWrite(inst.opcode)) {
                            slotValues.clear();
                        }
                        break;
                }
            }
            // 块内最后一条指令后的替换：无需再应用（块边界外不传播）
        }
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
