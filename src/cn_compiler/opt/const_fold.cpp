// CN语言优化器：常量折叠 Pass 实现（Task 2.6）
// 实现要点：
//   1. 块内常量跟踪（局部常量传播+折叠）：维护 regId -> 常量映射。
//      IR生成器将字面量生成为独立 ConstInt 指令 + 寄存器引用
//      （2+3 -> ConstInt 2 -> %v0; ConstInt 3 -> %v1; Add %v0 %v1），
//      因此折叠必须在块内把寄存器解析回常量（本 Pass 的核心机制）
//   2. 整型运算统一按 uint64_t 无符号运算（避免有符号溢出UB），
//      再按目标位宽截断（i8/i16/i32/i64/u8/u16/u32/u64），
//      负数以有符号解读输出（-1 -> "-1" 而非 "18446744073709551615"）
//   3. 除零/取余零不折叠（保留运行期错误码语义）
//   4. Branch 条件寄存器挂在块内最后一条指令 operands 尾部：
//      折叠时保留尾部条件操作数（只替换前半段操作数），
//      否则 codegen 读不到分支条件（emitTerminator 取 last.operands.back()）
//   5. 副作用指令（Load/Store/StorePtr/LoadPtr/Call/CallIndirect/FieldAddr）
//      后清空常量表（内存可能被修改，保守）
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "cn_compiler/opt/const_fold.hpp"

namespace cn_compiler {
namespace opt {

namespace {

// 常量文本规范化：解析 10/16/2/8 进制（IR生成器保留原始字面量文本，
// 如 0b1100/0xFF/077），返回十进制字符串；解析失败返回原文本
std::string normalizeIntText(const std::string& text) {
    if (text.size() > 2 && text[0] == '0' &&
        (text[1] == 'x' || text[1] == 'X' || text[1] == 'b' ||
         text[1] == 'B' || text[1] == 'o' || text[1] == 'O')) {
        const int base = (text[1] == 'x' || text[1] == 'X') ? 16
                         : (text[1] == 'b' || text[1] == 'B') ? 2 : 8;
        try {
            const unsigned long long raw = std::stoull(text.substr(2), nullptr, base);
            return std::to_string(raw);
        } catch (...) {
            return text;  // 解析失败保持原样
        }
    }
    return text;
}

// 整型常量文本 -> int64 值（支持负号与各进制），失败返回 false
bool parseInt64(const std::string& textRaw, std::int64_t& out) {
    const std::string text = normalizeIntText(textRaw);
    try {
        out = std::stoll(text, nullptr, 10);
        return true;
    } catch (...) {
        return false;
    }
}

// 整型位宽（位）：i8=8, i16=16, i32=32, i64=64, u8=8, ...；未知返回 64
int intBitWidth(const std::string& type) {
    if (type == "i8" || type == "u8") return 8;
    if (type == "i16" || type == "u16") return 16;
    if (type == "i32" || type == "u32") return 32;
    return 64;  // i64/u64 及未知类型按64位
}

// 是否整型 IR 类型（i8~u64；i128 暂不折叠）
bool isIntType(const std::string& type) {
    return type == "i8" || type == "u8" || type == "i16" || type == "u16" ||
           type == "i32" || type == "u32" || type == "i64" || type == "u64";
}

// 是否浮点 IR 类型
bool isFloatType(const std::string& type) {
    return type == "f32" || type == "f64";
}

// 是否布尔 IR 类型
bool isBoolType(const std::string& type) {
    return type == "i1";
}

// 常量文本 -> 位模式（按类型位宽截断）
bool parseConstInt(const std::string& text, const std::string& type, std::uint64_t& raw) {
    std::int64_t value = 0;
    if (!parseInt64(text, value)) return false;
    const int bits = intBitWidth(type);
    const std::uint64_t mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
    raw = static_cast<std::uint64_t>(value) & mask;
    return true;
}

// 位模式按类型有符号解读输出（负数输出负号十进制；无符号直接输出）
std::string formatSigned(std::uint64_t raw, const std::string& type) {
    const int bits = intBitWidth(type);
    if (bits >= 64) {
        return std::to_string(static_cast<std::int64_t>(raw));
    }
    const bool isUnsigned = (type.size() >= 2 && type[0] == 'u');
    if (isUnsigned) return std::to_string(raw);
    const std::uint64_t signBit = 1ULL << (bits - 1);
    if (raw & signBit) {
        const std::int64_t value = static_cast<std::int64_t>(raw) -
                                   (static_cast<std::int64_t>(1) << bits);
        return std::to_string(value);
    }
    return std::to_string(raw);
}

// 布尔常量文本（"真"/"假"/"1"/"0"）-> 值
bool parseBool(const std::string& text, bool& out) {
    if (text == "真" || text == "1") { out = true; return true; }
    if (text == "假" || text == "0") { out = false; return true; }
    return false;
}

// 浮点常量文本 -> double（codegen 用 std::stod 解析，折叠产物须能被 stod 读回）
bool parseDouble(const std::string& text, double& out) {
    try {
        out = std::stod(text);
        return true;
    } catch (...) {
        return false;
    }
}

// 有符号按位宽解读（-1 的 i8 位模式 0xFF -> -1）
std::int64_t signExtend(std::uint64_t raw, int bits, const std::string& type) {
    const bool isUnsigned = (type.size() >= 2 && type[0] == 'u');
    if (isUnsigned || bits >= 64) return static_cast<std::int64_t>(raw);
    const std::uint64_t signBit = 1ULL << (bits - 1);
    if (raw & signBit) {
        return static_cast<std::int64_t>(raw) - (std::int64_t(1) << bits);
    }
    return static_cast<std::int64_t>(raw);
}

// 浮点结果格式化（f32 截断 + %.9g；f64 %.17g 保证 stod 往返）
std::string formatDouble(double value, const std::string& type) {
    char buf[32];
    if (type == "f32") {
        const float f = static_cast<float>(value);
        std::snprintf(buf, sizeof(buf), "%.9g", static_cast<double>(f));
    } else {
        std::snprintf(buf, sizeof(buf), "%.17g", value);
    }
    return buf;
}

// 将折叠结果写回指令（opcode/extra/type/operands），保留尾部操作数
void replaceWithConstant(ir::IRInstruction& inst, const std::string& value,
                         const std::string& resultType, bool isFloat,
                         const std::vector<ir::IRValue>& tail) {
    inst.opcode = isFloat ? ir::Opcode::ConstFloat : ir::Opcode::ConstInt;
    inst.extra = value;
    inst.type = resultType;
    inst.operands.clear();
    inst.operands.push_back(ir::IRValue::constant(value, resultType));
    for (const auto& t : tail) inst.operands.push_back(t);
}

} // namespace

// ==================== 操作数数量与常量表维护 ====================

// 指令的实际操作数数量（按 opcode 固定；Branch 条件寄存器追加在尾部，不计入）
std::size_t ConstFoldPass::operandCount(ir::Opcode op) {
    switch (op) {
        case ir::Opcode::Store: return 1;          // operand[0]=值
        case ir::Opcode::StorePtr: return 2;       // operand[0]=目标地址, operand[1]=值
        case ir::Opcode::Call: return 2;           // 前两个为实参（最多折叠两个实参）
        case ir::Opcode::Not: return 1;            // 一元
        case ir::Opcode::Cast: return 1;           // 一元
        case ir::Opcode::Phi: return 0;
        default: return 2;                         // 二元运算/比较/逻辑/移位
    }
}

// 更新常量表：常量指令记录映射；有副作用/结果非确定的指令清除映射
void ConstFoldPass::updateConstTable(const ir::IRInstruction& inst, ConstTable& consts) {
    if (inst.result.id < 0) return;
    switch (inst.opcode) {
        case ir::Opcode::ConstInt:
        case ir::Opcode::ConstFloat:
        case ir::Opcode::ConstBool:
            consts[inst.result.id] = ir::IRValue::constant(inst.extra, inst.type);
            break;
        case ir::Opcode::Add: case ir::Opcode::Sub:
        case ir::Opcode::Mul: case ir::Opcode::Div: case ir::Opcode::Mod:
        case ir::Opcode::BitAnd: case ir::Opcode::BitOr: case ir::Opcode::BitXor:
        case ir::Opcode::Shl: case ir::Opcode::Shr:
        case ir::Opcode::Eq: case ir::Opcode::Ne:
        case ir::Opcode::Lt: case ir::Opcode::Le: case ir::Opcode::Gt: case ir::Opcode::Ge:
        case ir::Opcode::And: case ir::Opcode::Or: case ir::Opcode::Not:
        case ir::Opcode::Cast:
            // 纯运算指令：折叠后上方常量分支会登记；未折叠时结果非确定 -> 清旧映射
            consts.erase(inst.result.id);
            break;
        default:
            // Load/Store/StorePtr/LoadPtr/Alloca/AddrOf/FieldAddr/Call/
            // CallIndirect/FuncAddr/Jump/Branch/Return/Phi/ConstString：
            // 有副作用或结果非确定 -> 清除映射（保守）
            consts.erase(inst.result.id);
            break;
    }
}

// 解析操作数：常量直用；寄存器命中常量表 -> 视为常量；否则失败
bool ConstFoldPass::resolveOperands(const ir::IRInstruction& inst,
                                    const ConstTable& consts,
                                    std::vector<ir::IRValue>& out,
                                    std::vector<ir::IRValue>& tail) {
    const std::size_t count = operandCount(inst.opcode);
    for (std::size_t i = 0; i < count && i < inst.operands.size(); ++i) {
        const ir::IRValue& op = inst.operands[i];
        if (op.isConstant) {
            out.push_back(op);
        } else if (op.id >= 0) {
            auto it = consts.find(op.id);
            if (it == consts.end()) return false;  // 寄存器未跟踪到常量
            out.push_back(it->second);
        } else {
            return false;  // 变量引用（按名）不可折叠
        }
    }
    // 尾部追加操作数（Branch 条件寄存器等）保留，不参与折叠判断
    if (count < inst.operands.size()) {
        tail.assign(inst.operands.begin() + static_cast<std::ptrdiff_t>(count),
                    inst.operands.end());
    }
    return true;
}

// ==================== 整型二元运算折叠 ====================

bool ConstFoldPass::foldIntBinary(ir::Opcode op, const std::string& aText,
                                  const std::string& bText, const std::string& type,
                                  std::string& out) {
    if (!isIntType(type)) return false;
    std::uint64_t a = 0, b = 0;
    if (!parseConstInt(aText, type, a) || !parseConstInt(bText, type, b)) return false;
    const int bits = intBitWidth(type);
    const std::uint64_t mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);

    switch (op) {
        case ir::Opcode::Add: out = formatSigned((a + b) & mask, type); return true;
        case ir::Opcode::Sub: out = formatSigned((a - b) & mask, type); return true;
        case ir::Opcode::Mul: out = formatSigned((a * b) & mask, type); return true;
        case ir::Opcode::Div: {
            // 除零不折叠（保留运行期错误语义）
            if (b == 0) return false;
            // 修复9（无符号除法）：正N 按无符号除法（div），
            //   原实现 signExtend 按有符号——4000000000 / 2 折叠为 -147483648 与运行期 div 不一致
            const bool isUnsigned = (type.size() >= 2 && type[0] == 'u');
            if (isUnsigned) {
                out = std::to_string(a / b);
            } else {
                const std::int64_t sa = signExtend(a, bits, type);
                const std::int64_t sb = signExtend(b, bits, type);
                if (sb == 0) return false;
                out = std::to_string(sa / sb);
            }
            return true;
        }
        case ir::Opcode::Mod: {
            if (b == 0) return false;
            const bool isUnsigned = (type.size() >= 2 && type[0] == 'u');
            if (isUnsigned) {
                out = std::to_string(a % b);
            } else {
                const std::int64_t sa = signExtend(a, bits, type);
                const std::int64_t sb = signExtend(b, bits, type);
                if (sb == 0) return false;
                out = std::to_string(sa % sb);
            }
            return true;
        }
        case ir::Opcode::BitAnd: out = formatSigned(a & b, type); return true;
        case ir::Opcode::BitOr: out = formatSigned(a | b, type); return true;
        case ir::Opcode::BitXor: out = formatSigned(a ^ b, type); return true;
        case ir::Opcode::Shl:
            // 移位量按位宽截断（x86 语义）
            out = formatSigned((a << (b & (bits - 1))) & mask, type);
            return true;
        case ir::Opcode::Shr: {
            // 有符号算术右移（无符号逻辑右移由类型决定）
            const bool isUnsigned = (type.size() >= 2 && type[0] == 'u');
            const int shift = static_cast<int>(b & (bits - 1));
            if (isUnsigned) {
                out = std::to_string((a >> shift) & mask);
            } else {
                const std::int64_t sa = signExtend(a, bits, type);
                out = std::to_string(sa >> shift);
            }
            return true;
        }
        default:
            return false;
    }
}

// ==================== 逻辑运算折叠 ====================

bool ConstFoldPass::foldLogic(ir::Opcode op, const std::string& aText,
                              const std::string& bText, std::string& out) {
    bool a = false, b = false;
    if (!parseBool(aText, a)) return false;
    if (op != ir::Opcode::Not && !parseBool(bText, b)) return false;
    switch (op) {
        case ir::Opcode::And: out = (a && b) ? "真" : "假"; return true;
        case ir::Opcode::Or: out = (a || b) ? "真" : "假"; return true;
        case ir::Opcode::Not: out = (!a) ? "真" : "假"; return true;
        default: return false;
    }
}

// ==================== 浮点二元运算折叠 ====================

bool ConstFoldPass::foldFloatBinary(ir::Opcode op, const std::string& aText,
                                    const std::string& bText, const std::string& type,
                                    std::string& out) {
    if (!isFloatType(type)) return false;
    double a = 0.0, b = 0.0;
    if (!parseDouble(aText, a) || !parseDouble(bText, b)) return false;
    double result = 0.0;
    switch (op) {
        case ir::Opcode::Add: result = a + b; break;
        case ir::Opcode::Sub: result = a - b; break;
        case ir::Opcode::Mul: result = a * b; break;
        case ir::Opcode::Div:
            // 浮点除零不折叠（保留 IEEE 语义/运行期行为）
            if (b == 0.0) return false;
            result = a / b;
            break;
        default:
            return false;
    }
    out = formatDouble(result, type);
    return true;
}

// ==================== 类型转换折叠 ====================

bool ConstFoldPass::foldCast(const std::string& fromType, const std::string& fromText,
                             const std::string& toType, std::string& out, bool& isFloat) {
    // 整 -> 整（位宽转换/截断/符号扩展）
    // 语义：先按源类型位宽解读为有符号值，再按目标类型位宽掩码输出
    //   （i8 -1 -> i32 -1 符号扩展；i32 300 -> i8 44 截断）
    if (isIntType(fromType) && isIntType(toType)) {
        std::uint64_t raw = 0;
        if (!parseConstInt(fromText, fromType, raw)) return false;
        const std::int64_t value = signExtend(raw, intBitWidth(fromType), fromType);
        const int bits = intBitWidth(toType);
        const std::uint64_t mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
        out = formatSigned(static_cast<std::uint64_t>(value) & mask, toType);
        isFloat = false;
        return true;
    }
    // 整 -> 浮（i32 42 -> f64 42.0）
    if (isIntType(fromType) && isFloatType(toType)) {
        std::int64_t value = 0;
        if (!parseInt64(fromText, value)) return false;
        out = formatDouble(static_cast<double>(value), toType);
        isFloat = true;
        return true;
    }
    // 浮 -> 整（截断语义，保守：仅当值在 int64 范围内）
    if (isFloatType(fromType) && isIntType(toType)) {
        double d = 0.0;
        if (!parseDouble(fromText, d)) return false;
        if (d < -9223372036854775808.0 || d > 9223372036854775807.0) return false;
        const std::int64_t value = static_cast<std::int64_t>(d);
        const int bits = intBitWidth(toType);
        const std::uint64_t mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
        out = formatSigned(static_cast<std::uint64_t>(value) & mask, toType);
        isFloat = false;
        return true;
    }
    // 浮 -> 浮（f32 -> f64 / f64 -> f32）
    if (isFloatType(fromType) && isFloatType(toType)) {
        double d = 0.0;
        if (!parseDouble(fromText, d)) return false;
        // 审查修复（方案C f32 打印 Cast 链路）：f32 -> f64 必须先按 f32 精度截断
        //   再提升——(double)(3.14f) 应为 3.1400001049041748（float 3.14 的精确值），
        //   原实现直接按 double 解析 3.14（3.1400000000000001），打印/格式化在
        //   更高精度（%g/%e 或 浮点转字符串）下输出错误；%f 6位小数恰好掩盖差异。
        if (fromType == "f32" && toType == "f64") {
            d = static_cast<double>(static_cast<float>(d));
        }
        out = formatDouble(d, toType);
        isFloat = true;
        return true;
    }
    // 整 <-> 布尔（Cast 到 i1：非零为真）
    if (isIntType(fromType) && isBoolType(toType)) {
        std::int64_t value = 0;
        if (!parseInt64(fromText, value)) return false;
        out = (value != 0) ? "真" : "假";
        isFloat = false;
        return true;
    }
    if (isBoolType(fromType) && isIntType(toType)) {
        bool b = false;
        if (!parseBool(fromText, b)) return false;
        out = b ? "1" : "0";
        isFloat = false;
        return true;
    }
    return false;
}

// ==================== 比较折叠（整型/浮点 -> 布尔） ====================

// 整型比较（按类型位宽解读；修复9：无符号类型按无符号比较）
//   原实现一律 signExtend 有符号——正32 4294967295 > 100 折叠为假，
//   与运行期（cmp + seta）不一致
bool ConstFoldPass::compareInt(ir::Opcode op, const std::string& aText,
                               const std::string& bText, const std::string& type,
                               bool& result) {
    std::uint64_t a = 0, b = 0;
    if (!parseConstInt(aText, type, a) || !parseConstInt(bText, type, b)) return false;
    const int bits = intBitWidth(type);
    const bool isUnsigned = (type.size() >= 2 && type[0] == 'u');
    if (isUnsigned) {
        // 无符号：直接按位模式比较（Eq/Ne 同值，序比较无符号）
        switch (op) {
            case ir::Opcode::Eq: result = (a == b); break;
            case ir::Opcode::Ne: result = (a != b); break;
            case ir::Opcode::Lt: result = (a < b); break;
            case ir::Opcode::Le: result = (a <= b); break;
            case ir::Opcode::Gt: result = (a > b); break;
            case ir::Opcode::Ge: result = (a >= b); break;
            default: return false;
        }
    } else {
        const std::int64_t sa = signExtend(a, bits, type);
        const std::int64_t sb = signExtend(b, bits, type);
        switch (op) {
            case ir::Opcode::Eq: result = (sa == sb); break;
            case ir::Opcode::Ne: result = (sa != sb); break;
            case ir::Opcode::Lt: result = (sa < sb); break;
            case ir::Opcode::Le: result = (sa <= sb); break;
            case ir::Opcode::Gt: result = (sa > sb); break;
            case ir::Opcode::Ge: result = (sa >= sb); break;
            default: return false;
        }
    }
    return true;
}

// 浮点比较
bool ConstFoldPass::compareFloat(ir::Opcode op, const std::string& aText,
                                 const std::string& bText, bool& result) {
    double a = 0.0, b = 0.0;
    if (!parseDouble(aText, a) || !parseDouble(bText, b)) return false;
    switch (op) {
        case ir::Opcode::Eq: result = (a == b); break;
        case ir::Opcode::Ne: result = (a != b); break;
        case ir::Opcode::Lt: result = (a < b); break;
        case ir::Opcode::Le: result = (a <= b); break;
        case ir::Opcode::Gt: result = (a > b); break;
        case ir::Opcode::Ge: result = (a >= b); break;
        default: return false;
    }
    return true;
}

// ==================== 折叠分派（按操作码） ====================

// 折叠纯运算指令（操作数已解析为常量 out），成功则替换为常量指令
bool ConstFoldPass::foldResolved(ir::IRInstruction& inst,
                                 const std::vector<ir::IRValue>& out,
                                 const std::vector<ir::IRValue>& tail) {
    const std::string opType = out.empty() ? "" : out[0].type;
    const std::string resultType = inst.type;

    switch (inst.opcode) {
        // ---- 算术（整型/浮点） ----
        case ir::Opcode::Add:
        case ir::Opcode::Sub:
        case ir::Opcode::Mul:
        case ir::Opcode::Div:
        case ir::Opcode::Mod: {
            std::string value;
            const bool isFloat = isFloatType(opType);
            if (isFloat) {
                if (!ConstFoldPass::foldFloatBinary(inst.opcode, out[0].extra,
                                                    out[1].extra, opType, value))
                    return false;
            } else {
                if (!ConstFoldPass::foldIntBinary(inst.opcode, out[0].extra,
                                                  out[1].extra, opType, value))
                    return false;
            }
            replaceWithConstant(inst, value, resultType, isFloat, tail);
            return true;
        }

        // ---- 位运算/移位 ----
        case ir::Opcode::BitAnd:
        case ir::Opcode::BitOr:
        case ir::Opcode::BitXor:
        case ir::Opcode::Shl:
        case ir::Opcode::Shr: {
            std::string value;
            if (!ConstFoldPass::foldIntBinary(inst.opcode, out[0].extra,
                                              out[1].extra, opType, value))
                return false;
            replaceWithConstant(inst, value, resultType, false, tail);
            return true;
        }

        // ---- 比较（结果为布尔） ----
        case ir::Opcode::Eq:
        case ir::Opcode::Ne:
        case ir::Opcode::Lt:
        case ir::Opcode::Le:
        case ir::Opcode::Gt:
        case ir::Opcode::Ge: {
            bool result = false;
            const bool ok = isFloatType(opType)
                                ? compareFloat(inst.opcode, out[0].extra, out[1].extra, result)
                                : compareInt(inst.opcode, out[0].extra, out[1].extra,
                                             opType, result);
            if (!ok) return false;
            const std::string text = result ? "真" : "假";
            inst.opcode = ir::Opcode::ConstBool;
            inst.extra = text;
            inst.type = "i1";
            inst.operands.clear();
            inst.operands.push_back(ir::IRValue::constant(text, "i1"));
            for (const auto& t : tail) inst.operands.push_back(t);
            return true;
        }

        // ---- 逻辑运算（i1 常量） ----
        case ir::Opcode::And:
        case ir::Opcode::Or:
        case ir::Opcode::Not: {
            const std::string aText = out.empty() ? "" : out[0].extra;
            const std::string bText = (out.size() > 1) ? out[1].extra : "";
            std::string value;
            if (!ConstFoldPass::foldLogic(inst.opcode, aText, bText, value)) return false;
            inst.opcode = ir::Opcode::ConstBool;
            inst.extra = value;
            inst.type = "i1";
            inst.operands.clear();
            inst.operands.push_back(ir::IRValue::constant(value, "i1"));
            for (const auto& t : tail) inst.operands.push_back(t);
            return true;
        }

        // ---- 类型转换（Cast） ----
        case ir::Opcode::Cast: {
            const std::string fromType = out[0].type;
            const std::string fromText = out[0].extra;
            std::string value;
            bool isFloat = false;
            if (!ConstFoldPass::foldCast(fromType, fromText, resultType, value, isFloat))
                return false;
            replaceWithConstant(inst, value, resultType, isFloat, tail);
            return true;
        }

        default:
            return false;
    }
}

// ==================== 单条指令折叠 ====================

bool ConstFoldPass::foldInstruction(ir::IRInstruction& inst, ConstTable& consts) {
    // 解析操作数（常量/常量表命中 -> 视为常量）
    std::vector<ir::IRValue> out;
    std::vector<ir::IRValue> tail;
    if (!resolveOperands(inst, consts, out, tail)) return false;
    // 尾部（Branch 条件）操作数也传播常量：命中常量表 -> 替换为常量值。
    // 关键：条件跳转块内最后一条指令（比较/逻辑）被折叠为 ConstBool 后，
    //       尾部条件寄存器 %vN 若不被替换，codegen emitTerminator 取
    //       last.operands.back() 得到寄存器 -> 槽垃圾 -> 崩溃。
    //       此处用常量表解析尾部寄存器，使折叠产物变为 [常量, 常量]。
    //       （条件为 i1，不会命中浮点常量，无需浮点过滤）
    for (auto& t : tail) {
        if (!t.isConstant && t.id >= 0) {
            auto it = consts.find(t.id);
            if (it != consts.end()) t = it->second;
        }
    }
    // 折叠纯运算指令（Load/Store/Call 等副作用指令在 foldResolved 中返回 false，
    // 不会误折叠——其 opcode 不在 foldResolved 的 case 列表中）
    return foldResolved(inst, out, tail);
}

// ==================== 模块级折叠 ====================

bool ConstFoldPass::run(ir::IRModule& module) {
    bool changed = false;
    for (auto& fn : module.functions) {
        for (auto& block : fn.blocks) {
            ConstTable consts;  // 块级常量表（块入口清空，跨块不做传播）
            for (auto& inst : block->instructions) {
                // 第一步：常量传播——把操作数中命中常量表的寄存器原地替换为常量值。
                // 注意1：折叠把指令结果替换为 ConstInt/ConstFloat/ConstBool 后仍保留结果寄存器，
                //        若后续不可折叠指令（如 ptr 加法）引用该寄存器，codegen 会从
                //        该寄存器的栈槽读垃圾值（崩溃）。因此必须同步替换所有引用点为常量。
                // 注意2：Branch 条件寄存器挂在块内最后一条指令 operands 尾部（endBranch 追加）。
                //        若该指令被折叠为 ConstBool（比较/逻辑），尾部条件仍是结果寄存器 %vN，
                //        codegen emitTerminator 取 last.operands.back() -> 槽垃圾 -> 崩溃。
                //        因此传播范围覆盖全部操作数（含尾部），命中常量表即替换为常量值。
                // 注意3：浮点常量不传播到操作数位置——codegen operandText 对 isConstant
                //        直接输出文本（如 3.5），而 MASM 不支持浮点立即数（需 @fpN 池标签），
                //        StorePtr 浮点分支会生成 movsd xmm0, qword ptr 3.5（A2050 汇编错误）。
                //        浮点常量仍走 ConstFloat 指令 + 寄存器引用（codegen 从槽加载，安全）。
                // 注意4：i128/u128 不传播——超出64位，codegen 拆双槽处理，
                //        操作数位置无法用单一立即数表示（cmp rax, 大值 -> A2084）。
                // 注意5：只向"纯运算指令"传播常量——Load/Store/StorePtr/Call/LoadPtr/
                //        FieldAddr/AddrOf 等指令的 codegen 依赖"操作数是寄存器/槽"：
                //        Store 小位宽值会生成 movsx eax, byte ptr <值>（大小前缀+立即数
                //        -> A2070 invalid instruction operands）。这些指令保持寄存器引用，
                //        由折叠替换指令本身（Store 不折叠），常量仍从槽加载，安全。
                // 注意6：常量类型必须与操作数类型匹配（i32 常量 100 不能传播给 i8 操作数
                //        ——原 IR 有 Cast 窄化；直接传播会跳过窄化）。
                const bool isPureOp =
                    inst.opcode == ir::Opcode::Add || inst.opcode == ir::Opcode::Sub ||
                    inst.opcode == ir::Opcode::Mul || inst.opcode == ir::Opcode::Div ||
                    inst.opcode == ir::Opcode::Mod ||
                    inst.opcode == ir::Opcode::BitAnd || inst.opcode == ir::Opcode::BitOr ||
                    inst.opcode == ir::Opcode::BitXor || inst.opcode == ir::Opcode::Shl ||
                    inst.opcode == ir::Opcode::Shr ||
                    inst.opcode == ir::Opcode::Eq || inst.opcode == ir::Opcode::Ne ||
                    inst.opcode == ir::Opcode::Lt || inst.opcode == ir::Opcode::Le ||
                    inst.opcode == ir::Opcode::Gt || inst.opcode == ir::Opcode::Ge ||
                    inst.opcode == ir::Opcode::And || inst.opcode == ir::Opcode::Or ||
                    inst.opcode == ir::Opcode::Not || inst.opcode == ir::Opcode::Cast ||
                    inst.opcode == ir::Opcode::Load || inst.opcode == ir::Opcode::LoadPtr ||
                    inst.opcode == ir::Opcode::StorePtr || inst.opcode == ir::Opcode::Call ||
                    inst.opcode == ir::Opcode::CallIndirect ||
                    inst.opcode == ir::Opcode::FieldAddr ||
                    inst.opcode == ir::Opcode::AddrOf;
                // 注意7：Branch 条件（尾部操作数）必须传播——条件跳转块的
                //        最后一条指令被折叠为 ConstBool 后，尾部条件寄存器若不替换，
                //        codegen emitTerminator 读 last.operands.back() 得到槽垃圾 -> 崩溃。
                //        因此尾部操作数无条件传播（条件是 i1 布尔，安全）。
                bool replaced = false;
                if (isPureOp) {
                    const std::size_t count = operandCount(inst.opcode);
                    for (std::size_t i = 0; i < count && i < inst.operands.size(); ++i) {
                        ir::IRValue& op = inst.operands[i];
                        if (!op.isConstant && op.id >= 0) {
                            auto it = consts.find(op.id);
                            if (it == consts.end()) continue;
                            const ir::IRValue& cv = it->second;
                            if (cv.type != "f32" && cv.type != "f64" &&
                                cv.type != "i128" && cv.type != "u128" &&
                                (cv.type == op.type || cv.type == "i1")) {
                                // 常量文本规范化：0b1100/0xFF/077 -> 十进制
                                // （codegen operandText 对常量直接输出文本，
                                //  MASM 不支持 0b/0x 前缀）
                                ir::IRValue normalized = cv;
                                normalized.extra = normalizeIntText(cv.extra);
                                op = normalized;  // 替换为常量值
                                replaced = true;
                            }
                        }
                    }
                }
                // 尾部操作数传播：Branch 条件（i1）必须传播（codegen emitTerminator
                // 读 last.operands.back()）。修复8：Call/CallIndirect 超出
                // operandCount 的实参（第3+参数）也会落到尾部——无条件传播浮点
                // 常量会把实参替换为 4.0 文本，emitCall 生成 `movsd xmm2, qword ptr 4.0`
                // （A2050 real or BCD number not allowed）。故尾部传播同样排除
                // 浮点/i128 常量（与主操作数传播一致），仅 i1/整型常量安全传播。
                for (std::size_t i = operandCount(inst.opcode);
                     i < inst.operands.size(); ++i) {
                    ir::IRValue& op = inst.operands[i];
                    if (!op.isConstant && op.id >= 0) {
                        auto it = consts.find(op.id);
                        if (it != consts.end()) {
                            const ir::IRValue& cv = it->second;
                            if (cv.type != "f32" && cv.type != "f64" &&
                                cv.type != "i128" && cv.type != "u128") {
                                op = cv;
                                replaced = true;
                            }
                        }
                    }
                }
                if (replaced) changed = true;
                // 第二步：常量折叠（操作数已替换为常量/命中常量表）
                if (foldInstruction(inst, consts)) changed = true;
                // 折叠后指令已变为常量指令 -> updateConstTable 登记新常量；
                // 副作用指令 -> 清空映射（内存可能被修改，保守）
                updateConstTable(inst, consts);
            }
        }
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
