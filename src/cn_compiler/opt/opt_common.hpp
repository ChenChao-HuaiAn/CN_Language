// CN语言优化器：共享工具（Task 完善C 优化器增强）
// 提供各 Pass 共用的"引用点替换"辅助：
//   1. 寄存器替换（id -> id）：无条件安全——SSA 形式下被替换寄存器
//      的定义在被引用之前，替换为更早定义的寄存器不破坏支配关系
//   2. 常量替换（id -> 常量）：仅向"可传播指令"传播 + 白名单过滤
//      （浮点/i128 不传播，MASM 无浮点立即数；类型须匹配）
// 设计原则（规格书12.4 优化正确性）：安全第一，保守为要。
#pragma once

#include <cerrno>
#include <climits>
#include <cstdlib>
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

// 常量文本是否带 0x/0b/0o 进制前缀（需归一化为十进制；
//   十进制文本是恒等变换，调用方可跳过归一化省一次字符串拷贝）
inline bool hasRadixPrefix(const std::string& text) {
    return text.size() > 2 && text[0] == '0' &&
           (text[1] == 'x' || text[1] == 'X' || text[1] == 'b' ||
            text[1] == 'B' || text[1] == 'o' || text[1] == 'O');
}

// 常量文本规范化：0b1100/0xFF/077 -> 十进制（codegen 对常量直接输出文本，
//   MASM 不支持 0b/0x/0o 前缀）；解析失败返回原文本
inline std::string normalizeIntText(const std::string& text) {
    if (hasRadixPrefix(text)) {
        const int base = (text[1] == 'x' || text[1] == 'X') ? 16
                         : (text[1] == 'b' || text[1] == 'B') ? 2 : 8;
        try {
            // 直接按 c_str()+2 解析，避免 substr 临时 string 分配
            const unsigned long long raw = std::stoull(text.c_str() + 2, nullptr, base);
            return std::to_string(raw);
        } catch (...) {
            return text;
        }
    }
    return text;
}

// 是否整型 IR 类型（i8~u64；i128 双槽特殊处理，不参与代数简化/CSE）
inline bool isIntType(const std::string& type) {
    // 注：i128/u128 **不在此列**——各 pass 的 64 位解析对它不适用（单测
    //   I128AddZeroSkipped/I128MulNotReduced 锁定既有保守行为）；i128 的
    //   常量折叠由 ConstFold 内部单独判据承担（D9·228-a）。
    //   按长度+字符判定（等价于 8 个字符串比较，免 memcmp 调用开销）。
    const std::size_t n = type.size();
    if (n == 2) {
        if (type[0] != 'i' && type[0] != 'u') return false;
        return type[1] == '8';
    }
    if (n == 3) {
        if (type[0] != 'i' && type[0] != 'u') return false;
        const char d1 = type[1];
        const char d2 = type[2];
        return (d1 == '1' && d2 == '6') || (d1 == '3' && d2 == '2') ||
               (d1 == '6' && d2 == '4');
    }
    return false;
}

// 是否浮点 IR 类型（f32/f64；按长度+字符判定，等价且免 memcmp）
inline bool isFloatType(const std::string& type) {
    if (type.size() != 3 || type[0] != 'f') return false;
    return (type[1] == '3' && type[2] == '2') ||
           (type[1] == '6' && type[2] == '4');
}

// 整型位宽（位）：i8=8, i16=16, i32=32, i64=64, u8=8, ...；未知返回 64
inline int intBitWidth(const std::string& type) {
    const std::size_t n = type.size();
    if (n == 2) {
        const bool matched = (type[0] == 'i' || type[0] == 'u') && type[1] == '8';
        return matched ? 8 : 64;
    }
    if (n == 3 && (type[0] == 'i' || type[0] == 'u')) {
        if (type[1] == '1' && type[2] == '6') return 16;
        if (type[1] == '3' && type[2] == '2') return 32;
    }
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
    // 两表皆空 = 无任何可替换项（多数块/轮次）：省去全操作数遍历与查表
    if (!regRewrite.empty() || !constRewrite.empty()) {
        for (auto& op : inst.operands) {
            if (op.isConstant || op.id < 0) continue;
            const auto rit = regRewrite.find(op.id);
            if (rit != regRewrite.end()) {
                op.id = rit->second;  // 寄存器替换（保持原类型）
                changed = true;
                continue;
            }
            // 常量替换范围：纯运算指令 + Call/CallIndirect 实参（整型安全）。
            //   不可传播指令直接跳过——不再为它们白查常量表。
            if (!isPureArith(inst.opcode) &&
                inst.opcode != ir::Opcode::Call &&
                inst.opcode != ir::Opcode::CallIndirect) {
                continue;
            }
            const auto cit = constRewrite.find(op.id);
            if (cit == constRewrite.end()) continue;
            const ir::IRValue& cv = cit->second;
            if (!constCanPropagate(cv, op)) continue;
            // 直接赋值再就地归一化（旧实现多一次 IRValue 拷贝）；
            //   十进制文本归一化是恒等变换，跳过省一次字符串拷贝。
            op = cv;
            if (hasRadixPrefix(op.extra)) op.extra = normalizeIntText(op.extra);
            changed = true;
        }
    }
    return changed;
}

// 解析 "%vN" 文本（N 为 int）到寄存器 id。
//   快速路径 strtol（无异常/无 substr 分配）；语义与旧 std::stoi(c.substr(2))
//   一致：接受前导空白与正负号、尾随非数字截止；无数字/溢出=false。
inline bool parseRegText(const std::string& c, int& id) {
    if (c.size() <= 2 || c[0] != '%' || c[1] != 'v') return false;
    const char* const start = c.c_str() + 2;
    char* end = nullptr;
    errno = 0;
    const long value = std::strtol(start, &end, 10);
    if (end == start) return false;                       // 无数字（stoi 抛 invalid）
    if (errno == ERANGE || value < INT_MIN || value > INT_MAX) return false;  // 溢出
    id = static_cast<int>(value);
    return true;
}

// 280-a T12 字段化连带：块终止条件跳转的条件寄存器同步替换。
//   条件已显式存 termCondition（不再寄生于块尾指令 operands），
//   replaceUses 的指令级遍历覆盖不到——各持 RegRewriteMap 的 Pass
//   （algebraic_simplify/copy_propagation/global_value）逐块调用本函数。
//   寄存器替换无条件（SSA 安全）；常量替换经 constCanPropagate 白名单
//   （条件为 i1，"真"/"假" 文本可传播）。
inline bool replaceTermCondition(ir::IRBlock& block,
                                 const RegRewriteMap& regRewrite,
                                 const ConstRewriteMap& constRewrite) {
    if (!block.terminated || block.termKind != "条件跳转") return false;
    // 两表皆空 = 无替换项（多数块/轮次）：省去条件文本解析
    if (!regRewrite.empty() || !constRewrite.empty()) {
        int id = 0;
        // 解析失败（IR 契约外文本）：不替换（与旧 stoi 异常返回 false 等价）
        if (parseRegText(block.termCondition, id)) {
            const auto rit = regRewrite.find(id);
            if (rit != regRewrite.end()) {
                block.termCondition = "%v" + std::to_string(rit->second);
                return true;
            }
            const auto cit = constRewrite.find(id);
            if (cit != constRewrite.end() && cit->second.isConstant) {
                ir::IRValue cond = cit->second;
                if (constCanPropagate(cond, ir::IRValue::reg(id, "i1"))) {
                    block.termCondition = cond.extra;  // "真"/"假"/数值文本
                    return true;
                }
            }
        }
    }
    return false;
}

} // namespace opt
} // namespace cn_compiler
