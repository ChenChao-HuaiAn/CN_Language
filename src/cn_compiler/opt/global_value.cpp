// CN语言优化器：全局值传播 Pass 实现（Task 完善C 优化器增强）
// 实现要点：
//   1. 常量表：唯一内部名 -> 常量值（仅整型/布尔常量，浮点/i128 除外）
//   2. 沿函数基本块线性序扫描（blocks 顺序近似源码顺序）：
//      - Store 常量到槽 -> 记录常量表
//      - Load 同槽命中 -> 登记 结果寄存器 -> 常量 替换
//      - Store 寄存器值/StorePtr/Call 等 -> 清空常量表（值不确定）
//   3. 替换延迟到 replaceUses（仅纯运算指令/Call 实参传播 + 白名单）
//   4. 保守：跨块仅线性序，不做真正数据流（无前驱/多前驱交集）
#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/opt/global_value.hpp"
#include "cn_compiler/opt/opt_common.hpp"

namespace cn_compiler {
namespace opt {

// 是否可能写入任意内存的指令（清空常量表）
bool GlobalValuePass::mayAliasWrite(ir::Opcode op) {
    switch (op) {
        case ir::Opcode::StorePtr:
        case ir::Opcode::LoadPtr:
        case ir::Opcode::Call:
        case ir::Opcode::CallIndirect:
        case ir::Opcode::FieldAddr:
        case ir::Opcode::AddrOf:
            return true;
        default:
            return false;
    }
}

// 是否多槽变量（结构体/数组，保守跳过）
bool GlobalValuePass::isMultiSlot(
    const std::string& uniqueName,
    const std::unordered_map<std::string, int>& varSlots) {
    auto it = varSlots.find(uniqueName);
    if (it != varSlots.end() && it->second > 1) return true;
    return false;
}

// 遍历模块全部函数：线性扫描常量 Store->Load 转发
bool GlobalValuePass::run(ir::IRModule& module) {
    bool changed = false;
    for (auto& fn : module.functions) {
        // 304-a T26 根治（B11 波次1）：汇合块（多前驱）感知——
        //   线性序扫描的常量表必须在汇合块入口失效：汇合块内槽的可见定义
        //   =各前驱的 phi 值，「后块 Store 覆盖前块记录」在此无顺序语义依据。
        //   事故形态（s2609179001 实弹）：bb1（乙路径）Store 寄存器→擦除记录，
        //   bb2（短路路径）Store 常量真→覆盖记录，bb3（汇合）Load→错误折叠为真
        //   →布尔短路值位 -O3 常量化（O0/O2 无本 pass 故免疫·级别分叉铁证）。
        //   同族面=整型/布尔一切「分支汇合后读槽」形态（B9 扩散矩阵见 E2E 410）。
        std::unordered_map<std::string, int> indegree;
        for (const auto& block : fn.blocks) {
            if (!block->terminated) continue;
            if (block->termKind == "跳转") {
                ++indegree[block->termTarget];
            } else if (block->termKind == "条件跳转") {
                ++indegree[block->termTrueTarget];
                ++indegree[block->termFalseTarget];
            }
        }
        // 常量表：唯一内部名 -> 常量值（函数级，跨块线性序维护）
        std::unordered_map<std::string, ir::IRValue> constants;
        // 常量替换表：Load 结果寄存器 -> 常量
        ConstRewriteMap constRewrite;
        for (auto& block : fn.blocks) {
            // 汇合块（≥2 前驱）：线性序常量表失效——保守清空（块内 Store→Load
            //   转发从本块头重新记录，仍安全可用）
            auto indeg = indegree.find(block->label);
            if (indeg != indegree.end() && indeg->second >= 2) {
                constants.clear();
            }
            // 283-a T12 字段化：块终止条件寄存器同步替换（在指令遍历前应用）
            if (replaceTermCondition(*block, RegRewriteMap(), constRewrite)) {
                changed = true;
            }
            for (auto& inst : block->instructions) {
                // 第一步：应用已收集的替换
                if (replaceUses(inst, RegRewriteMap(), constRewrite)) changed = true;
                // 第二步：维护常量表/登记新替换
                switch (inst.opcode) {
                    case ir::Opcode::Store: {
                        if (inst.extra.empty() || inst.operands.empty()) break;
                        if (isMultiSlot(inst.extra, fn.varSlots)) break;
                        const ir::IRValue& value = inst.operands[0];
                        if (value.isConstant) {
                            // 常量存储：仅整型/布尔记录（浮点/i128 不传播）
                            if (isIntType(value.type) || value.type == "i1") {
                                constants[inst.extra] = value;
                            } else {
                                constants.erase(inst.extra);
                            }
                        } else {
                            // 寄存器存储：值不确定 -> 清空
                            constants.erase(inst.extra);
                        }
                        break;
                    }
                    case ir::Opcode::Load: {
                        if (inst.operands.empty() || inst.result.id < 0) break;
                        const std::string& name = inst.operands[0].extra;
                        if (name.empty() || inst.operands[0].id >= 0) break;
                        auto it = constants.find(name);
                        if (it == constants.end()) break;
                        const ir::IRValue& cv = it->second;
                        if (!cv.type.empty() && !inst.type.empty() &&
                            cv.type != inst.type && cv.type != "i1") {
                            break;  // 类型不匹配：不传播
                        }
                        // 登记替换（不设 changed：实际引用替换由 replaceUses 报告，
                        //   保证 fixpoint 收敛——重复运行时 Load 结果已无引用
                        //   则不报告修改）
                        constRewrite[inst.result.id] = cv;
                        break;
                    }
                    default:
                        // 可能写任意内存：清空常量表
                        if (mayAliasWrite(inst.opcode)) {
                            constants.clear();
                        }
                        break;
                }
            }
        }
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
