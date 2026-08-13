// CN语言优化器：共享工具（Task 完善C 优化器增强）
// 提供各 Pass 共用的"引用点替换"辅助：
//   1. 寄存器替换（id -> id）：无条件安全——SSA 形式下被替换寄存器
//      的定义在被引用之前，替换为更早定义的寄存器不破坏支配关系
//   2. 常量替换（id -> 常量）：仅向"可传播指令"传播 + 白名单过滤
//      （浮点/i128 不传播，MASM 无浮点立即数；类型须匹配）
// 设计原则（规格书12.4 优化正确性）：安全第一，保守为要。
#pragma once

#include <string>
#include <unordered_map>

#include "cn_compiler/ir/ir.hpp"

namespace cn_compiler {
namespace opt {

// 是否纯运算指令（无副作用，结果可安全替换/删除）：
//   代数简化/CSE/复写传播仅对这类指令建立"模式/替换"关系
inline bool isPureArith(ir::Opcode op) {
    switch (op) {
        case ir::Opcode::Add: case ir::Opcode::Sub:
        case ir::Opcode::Mul: case ir::Opcode::Div: case ir::Opcode::Mod:
        case ir::Opcode::BitAnd: case ir::Opcode::BitOr: case ir::Opcode::BitXor:
        case ir::Opcode::Shl: case ir::Opcode::Shr:
        case ir::Opcode::Eq: case ir::Opcode::Ne:
        case ir::Opcode::Lt: case ir::Opcode::Le: case ir::Opcode::Gt: case ir::Opcode::Ge:
        case ir::Opcode::And: case ir::Opcode::Or: case ir::Opcode::Not:
        case ir::Opcode::Cast:
            return true;
        default:
            return false;
    }
}

// 常量传播白名单：排除浮点（MASM 无浮点立即数）与 i128（双槽，无单一立即数），
//   且常量类型必须与操作数类型匹配（i1 布尔兼容整型）
inline bool constCanPropagate(const ir::IRValue& cv, const ir::IRValue& op) {
    if (cv.isConstant &&
        (cv.type == "f32" || cv.type == "f64" ||
         cv.type == "i128" || cv.type == "u128")) {
        return false;
    }
    return cv.type == op.type || cv.type == "i1";
}

// 常量文本规范化：0b1100/0xFF/077 -> 十进制（codegen 对常量直接输出文本，
//   MASM 不支持 0b/0x/0o 前缀）；解析失败返回原文本
inline std::string normalizeIntText(const std::string& text) {
    if (text.size() > 2 && text[0] == '0' &&
        (text[1] == 'x' || text[1] == 'X' || text[1] == 'b' ||
         text[1] == 'B' || text[1] == 'o' || text[1] == 'O')) {
        const int base = (text[1] == 'x' || text[1] == 'X') ? 16
                         : (text[1] == 'b' || text[1] == 'B') ? 2 : 8;
        try {
            const unsigned long long raw = std::stoull(text.substr(2), nullptr, base);
            return std::to_string(raw);
        } catch (...) {
            return text;
        }
    }
    return text;
}

// 是否整型 IR 类型（i8~u64；i128 双槽特殊处理，不参与代数简化/CSE）
inline bool isIntType(const std::string& type) {
    return type == "i8" || type == "u8" || type == "i16" || type == "u16" ||
           type == "i32" || type == "u32" || type == "i64" || type == "u64";
}

// 是否浮点 IR 类型
inline bool isFloatType(const std::string& type) {
    return type == "f32" || type == "f64";
}

// 整型位宽（位）：i8=8, i16=16, i32=32, i64=64, u8=8, ...；未知返回 64
inline int intBitWidth(const std::string& type) {
    if (type == "i8" || type == "u8") return 8;
    if (type == "i16" || type == "u16") return 16;
    if (type == "i32" || type == "u32") return 32;
    return 64;
}

// 寄存器替换表：寄存器 id -> 替换寄存器 id（无条件替换，所有指令操作数含尾部）
using RegRewriteMap = std::unordered_map<int, int>;
// 常量替换表：寄存器 id -> 常量值（仅可传播指令 + 白名单）
using ConstRewriteMap = std::unordered_map<int, ir::IRValue>;

// 替换单条指令的引用点（操作数，含 Branch 条件尾部操作数）：
//   1. 寄存器替换：无条件（SSA 安全）
//   2. 常量替换：仅对 isPureArith 指令或 Call/CallIndirect 实参，
//      且通过 constCanPropagate 白名单（整型常量传播到 Call 实参安全：
//      emitCall 生成 mov rcx, 5；浮点/i128 除外）
//   注意：不替换 Load 的变量名操作数（id=-1）；Store 值操作数若为寄存器
//   则被无条件替换（从新寄存器槽读，值相同，安全）；常量不替换到 Store 值
//   （codegen 小位宽 Store 生成 movsx eax, byte ptr 常量 -> A2070，保守保持寄存器）
inline bool replaceUses(ir::IRInstruction& inst,
                        const RegRewriteMap& regRewrite,
                        const ConstRewriteMap& constRewrite) {
    bool changed = false;
    for (auto& op : inst.operands) {
        if (op.isConstant || op.id < 0) continue;
        const auto rit = regRewrite.find(op.id);
        if (rit != regRewrite.end()) {
            op.id = rit->second;  // 寄存器替换（保持原类型）
            changed = true;
            continue;
        }
        const auto cit = constRewrite.find(op.id);
        if (cit == constRewrite.end()) continue;
        const ir::IRValue& cv = cit->second;
        // 常量替换范围：纯运算指令 + Call/CallIndirect 实参（整型安全）
        if (!isPureArith(inst.opcode) &&
            inst.opcode != ir::Opcode::Call &&
            inst.opcode != ir::Opcode::CallIndirect) {
            continue;
        }
        if (!constCanPropagate(cv, op)) continue;
        ir::IRValue normalized = cv;
        normalized.extra = normalizeIntText(cv.extra);
        op = normalized;
        changed = true;
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
