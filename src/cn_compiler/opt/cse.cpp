// CN语言优化器：块内公共子表达式消除 Pass 实现（Task 完善C 优化器增强）
// 实现要点：
//   1. patternOf 生成纯运算指令的模式键（操作码+类型+操作数身份），
//      相同键 = 相同输入（同寄存器/同常量文本）-> 结果可复用
//   2. 纯运算模式表跨副作用指令存活（无副作用，结果恒定）
//   3. Load 表独立维护：变量名 -> Load 结果寄存器；遇写内存清空
//   4. 替换复用 opt_common 的 replaceUses（寄存器替换无条件安全）
#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/opt/cse.hpp"
#include "cn_compiler/opt/opt_common.hpp"

namespace cn_compiler {
namespace opt {

namespace {

// 操作数身份：常量用文本+类型，寄存器用 id+类型，变量名用名+类型
std::string operandKey(const ir::IRValue& op) {
    std::string key;
    if (op.isConstant) {
        key = "c:" + op.type + ":" + normalizeIntText(op.extra);
    } else if (op.id >= 0) {
        key = "r:" + op.type + ":" + std::to_string(op.id);
    } else {
        key = "v:" + op.type + ":" + op.extra;  // 变量引用（按名）
    }
    return key;
}

} // namespace

// 计算纯运算指令的模式键（操作码+类型+操作数身份）；非纯运算返回空串
std::string CSEPass::patternOf(const ir::IRInstruction& inst) {
    // 常量指令（ConstInt/ConstFloat/ConstBool）不 CSE（常量折叠已处理）
    if (inst.opcode == ir::Opcode::ConstInt ||
        inst.opcode == ir::Opcode::ConstFloat ||
        inst.opcode == ir::Opcode::ConstBool) {
        return "";
    }
    // 仅纯运算指令可 CSE（含 Cast 一元）
    if (!isPureArith(inst.opcode)) return "";
    // 浮点指令：仅 allowFloat_ 启用时入表（-O2 以上）
    if (!allowFloat_ && isFloatType(inst.type)) return "";
    std::string key = std::to_string(static_cast<int>(inst.opcode)) + "|" + inst.type;
    for (const auto& op : inst.operands) {
        key += "|" + operandKey(op);
    }
    return key;
}

// 是否可能修改任意内存的指令（清空 Load 表）
bool CSEPass::mayWriteMemory(ir::Opcode op) {
    switch (op) {
        case ir::Opcode::Store:
        case ir::Opcode::StorePtr:
        case ir::Opcode::Call:
        case ir::Opcode::CallIndirect:
        case ir::Opcode::LoadPtr:    // 指针加载可能触发空指针检查（副作用）
        case ir::Opcode::FieldAddr:  // 隐含空指针检查
        case ir::Opcode::AddrOf:     // 地址逃逸
            return true;
        default:
            return false;
    }
}

// 遍历模块全部函数的全部基本块，消除块内公共子表达式
bool CSEPass::run(ir::IRModule& module) {
    bool changed = false;
    for (auto& fn : module.functions) {
        for (auto& block : fn.blocks) {
            // 纯运算模式表：模式键 -> 第一个结果寄存器 id
            std::unordered_map<std::string, int> patterns;
            // Load 表：变量名（唯一内部名）-> Load 结果寄存器 id
            std::unordered_map<std::string, int> loadResults;
            // 寄存器替换表：重复指令结果 -> 第一个结果
            RegRewriteMap regRewrite;
            for (auto& inst : block->instructions) {
                // 第一步：应用已收集的替换（本指令引用点）
                if (replaceUses(inst, regRewrite, ConstRewriteMap())) changed = true;
                // 第二步：按指令类别维护表
                const std::string pattern = patternOf(inst);
                if (!pattern.empty()) {
                    // 纯运算：查表命中 -> 登记替换；未命中 -> 登记
                    auto it = patterns.find(pattern);
                    if (it != patterns.end() && inst.result.id >= 0) {
                        regRewrite[inst.result.id] = it->second;
                        changed = true;
                    } else if (inst.result.id >= 0) {
                        patterns[pattern] = inst.result.id;
                    }
                } else if (inst.opcode == ir::Opcode::Load) {
                    // Load：operand[0]=变量名（唯一内部名），result=寄存器
                    if (inst.operands.empty() || inst.result.id < 0) continue;
                    const std::string& name = inst.operands[0].extra;
                    if (name.empty() || inst.operands[0].id >= 0) continue;
                    auto it = loadResults.find(name);
                    if (it != loadResults.end()) {
                        // 同槽 Load 命中（两者间无写内存）-> 复用
                        regRewrite[inst.result.id] = it->second;
                        changed = true;
                    } else {
                        loadResults[name] = inst.result.id;
                    }
                } else if (mayWriteMemory(inst.opcode)) {
                    // 写内存指令：清空 Load 表（内存可能被修改，保守）
                    loadResults.clear();
                }
            }
        }
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
