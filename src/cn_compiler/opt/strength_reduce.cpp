// CN语言优化器：强度削减 Pass 实现（阶段B Task 4.2）
// 实现要点：
//   1. Mul x, 2^n -> Shl x, n（整型；i1 布尔不可移，跳过）
//   2. Div x, 2^n -> Shr x, n（仅无符号/正类型 u8~u64：
//      逻辑右移 = 无符号除法；有符号右移是向下取整，与 C++ 除法
//      （向零取整）语义不符，保守不削减）
//   3. 常量解析：normalizeIntText 规范化后 stoull（防御异常）
//   4. 幂等：削减后 opcode 变为 Shl/Shr，不再匹配 Mul/Div
#include <string>

#include "cn_compiler/opt/opt_common.hpp"
#include "cn_compiler/opt/strength_reduce.hpp"

namespace cn_compiler {
namespace opt {

// 解析整型常量文本为十进制（支持 0x/0b/0o 前缀），失败返回 false
bool StrengthReducePass::parseIntText(const std::string& text,
                                      unsigned long long& out) {
    const std::string norm = normalizeIntText(text);
    try {
        out = std::stoull(norm);
        return true;
    } catch (...) {
        return false;
    }
}

// 计算 2^n 的 n（value 为 2 的幂），非 2 的幂返回 -1
int StrengthReducePass::powerOfTwo(unsigned long long value) {
    if (value == 0) return -1;
    int n = 0;
    while ((value & 1ULL) == 0) {
        value >>= 1ULL;
        ++n;
    }
    return (value == 1ULL) ? n : -1;
}

// 削减单条 Mul/Div 指令（原地改 opcode/操作数），成功返回 true
// 形式：Mul reg, const 或 Mul const, reg；Div reg, const
bool StrengthReducePass::reduceInstruction(ir::IRInstruction& inst) {
    // 仅整型可削减（浮点/i128/i1 跳过）
    if (!isIntType(inst.type)) return false;
    if (inst.operands.size() < 2) return false;

    if (inst.opcode == ir::Opcode::Mul) {
        // 找常量操作数与寄存器操作数
        const ir::IRValue& a = inst.operands[0];
        const ir::IRValue& b = inst.operands[1];
        const ir::IRValue* reg = nullptr;
        const ir::IRValue* cnst = nullptr;
        if (a.isConstant && !b.isConstant) { cnst = &a; reg = &b; }
        else if (!a.isConstant && b.isConstant) { cnst = &b; reg = &a; }
        else return false;  // 双常量由 ConstFold 处理；双寄存器不可削减
        if (reg == nullptr || reg->id < 0) return false;
        unsigned long long val = 0;
        if (!parseIntText(cnst->extra, val)) return false;
        const int n = powerOfTwo(val);
        if (n < 0 || n > 63) return false;
        // Mul x, 2^n -> Shl x, n
        inst.opcode = ir::Opcode::Shl;
        inst.operands = {*reg, ir::IRValue::constant(std::to_string(n), "i32")};
        return true;
    }

    if (inst.opcode == ir::Opcode::Div) {
        // Div 仅支持无符号/正类型（u8~u64）：逻辑右移 = 无符号除法
        if (inst.type != "u8" && inst.type != "u16" &&
            inst.type != "u32" && inst.type != "u64") {
            return false;  // 有符号除法的右移语义不符，保守跳过
        }
        const ir::IRValue& a = inst.operands[0];
        const ir::IRValue& b = inst.operands[1];
        if (b.isConstant && a.id >= 0) {
            unsigned long long val = 0;
            if (!parseIntText(b.extra, val)) return false;
            const int n = powerOfTwo(val);
            if (n < 0 || n > 63) return false;
            // Div x, 2^n -> Shr x, n（逻辑右移）
            inst.opcode = ir::Opcode::Shr;
            inst.operands = {a, ir::IRValue::constant(std::to_string(n), "i32")};
            return true;
        }
    }
    return false;
}

// 遍历模块全部函数：乘/除 2 的幂 -> 移位
bool StrengthReducePass::run(ir::IRModule& module) {
    bool changed = false;
    for (auto& fn : module.functions) {
        for (auto& block : fn.blocks) {
            for (auto& inst : block->instructions) {
                if (reduceInstruction(inst)) changed = true;
            }
        }
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
