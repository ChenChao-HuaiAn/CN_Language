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
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "cn_compiler/opt/const_fold.hpp"
#include "cn_compiler/opt/opt_common.hpp"

namespace cn_compiler {
namespace opt {

namespace {

// 整型常量文本 -> int64 值（支持负号与各进制），失败返回 false。
//   十进制快路径 std::from_chars（0x/0b/0o 先走 opt_common 归一化）；
//   非法形态回退 std::stoll 保持旧语义（前导空白/+号/尾随垃圾截止）。
bool parseInt64(const std::string& textRaw, std::int64_t& out) {
    if (!hasRadixPrefix(textRaw)) {
        const char* const begin = textRaw.c_str();
        std::int64_t value = 0;
        const auto result =
            std::from_chars(begin, begin + textRaw.size(), value);
        if (result.ec == std::errc()) {
            out = value;
            return true;
        }
        if (result.ec == std::errc::result_out_of_range) return false;
        // invalid_argument：空串/空白/正号等——回退 stoll（旧行为）
    }
    const std::string text = normalizeIntText(textRaw);
    try {
        out = std::stoll(text, nullptr, 10);
        return true;
    } catch (...) {
        return false;
    }
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

// 浮点常量文本 -> float32（经 double 正确舍入单次转换）。
//   折叠链中间文本（%.9g）对 float32 往返无损：FLT_DECIMAL_DIG=9，文本量化
//   相对偏差 < 5e-10，远小于 float32 半 ulp（相对 >= 2.9e-8），cast float
//   舍回原位模式——f32 域折叠全程以本函数读回操作数，禁直接用 double 值运算。
bool textToF32(const std::string& text, float& out) {
    double d = 0.0;
    // 静默豁免业务值: 文本不可解析=确权「不可折叠」返回 假，上层放弃折叠保留
    //   运行期原指令求值（产物语义=未优化·无默认值无错值）——继承冻结基线同语义
    //   旧锚 parseDouble 失败链放行面（1187 折叠放弃通道同构先例）
    if (!parseDouble(text, d)) return false;
    out = static_cast<float>(d);
    return true;
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

// 常量传播目标指令（与旧 27 项 || 长链等价；switch 分派开销更低）。
//   注意：Store 不在列（codegen 依赖操作数为寄存器/槽，见 run 内注意5）。
bool isConstPropagateTarget(ir::Opcode op) {
    switch (op) {
        case ir::Opcode::Add: case ir::Opcode::Sub: case ir::Opcode::Mul:
        case ir::Opcode::Div: case ir::Opcode::Mod:
        case ir::Opcode::BitAnd: case ir::Opcode::BitOr: case ir::Opcode::BitXor:
        case ir::Opcode::Shl: case ir::Opcode::Shr:
        case ir::Opcode::Eq: case ir::Opcode::Ne:
        case ir::Opcode::Lt: case ir::Opcode::Le: case ir::Opcode::Gt: case ir::Opcode::Ge:
        case ir::Opcode::And: case ir::Opcode::Or: case ir::Opcode::Not:
        case ir::Opcode::Cast:
        case ir::Opcode::Load: case ir::Opcode::LoadPtr:
        case ir::Opcode::StorePtr:
        case ir::Opcode::Call: case ir::Opcode::CallIndirect:
        case ir::Opcode::FieldAddr: case ir::Opcode::StrFieldAddr:
        case ir::Opcode::AddrOf:
            return true;
        default:
            return false;
    }
}

// 是否可折叠操作码（foldResolved 的 case 列表；其余指令调用必然返回 false，
//   run 主循环据此跳过 resolveOperands 的向量/IRValue 拷贝）。
bool isFoldableOpcode(ir::Opcode op) {
    switch (op) {
        case ir::Opcode::Add: case ir::Opcode::Sub: case ir::Opcode::Mul:
        case ir::Opcode::Div: case ir::Opcode::Mod:
        case ir::Opcode::WrapAdd: case ir::Opcode::WrapSub: case ir::Opcode::WrapMul:
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
        case ir::Opcode::WrapAdd: case ir::Opcode::WrapSub: case ir::Opcode::WrapMul:
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
    out.reserve(count);
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

// 任务 474：Add/Sub 常量折叠溢出预演（位模式运算，不依赖 __builtin——MSVC 兼容）。
//   a/b 为类型域内位模式；true=溢出（不折叠，保留运行期错误码 8 通路——与
//   除零不折叠同策略：折叠产出回绕值会绕过运行期检查=改语义的优化）。
static bool addSubOvf474(ir::Opcode op, std::uint64_t a, std::uint64_t b, int bits,
                         bool isUnsigned) {
    // r 必须按类型位宽截断（验收返工：a+b 在 uint64 域无截断——u8 的
    //   200+200=400 < 200 恒假=漏检；截断后 144 < 200=进位检出）
    const std::uint64_t mask474 = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
    const std::uint64_t r =
        ((op == ir::Opcode::Add) ? (a + b) : (a - b)) & mask474;
    if (isUnsigned) {
        return (op == ir::Opcode::Add) ? (r < a) : (a < b);  // 进位/借位
    }
    // 有符号溢出（标志位技巧，与 x86 add/sub OF 同构）
    const std::uint64_t sameAdd = ~(a ^ b) & (a ^ r);   // add: 结果符号 ≠ 操作数公共符号
    const std::uint64_t diffSub = (a ^ b) & (a ^ r);    // sub: 操作数异号且结果与被减数异号
    return (((op == ir::Opcode::Add) ? sameAdd : diffSub) >> (bits - 1)) & 1;
}

bool ConstFoldPass::foldIntBinary(ir::Opcode op, const std::string& aText,
                                  const std::string& bText, const std::string& type,
                                  std::string& out) {
    if (!isIntType(type)) return false;
    std::uint64_t a = 0, b = 0;
    if (!parseConstInt(aText, type, a) || !parseConstInt(bText, type, b)) return false;
    const int bits = intBitWidth(type);
    const std::uint64_t mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
    const bool isUnsignedT = (type.size() >= 2 && type[0] == 'u');

    switch (op) {
        case ir::Opcode::Add:
        case ir::Opcode::Sub: {
            // 任务 474：可溢出算术常量折叠先预演——溢出不折叠（保留运行期
            //   错误码 8 确定性失败通路；Wrap* 豁免面照常回绕折叠）
            if (addSubOvf474(op, a, b, bits, isUnsignedT)) return false;
            out = formatSigned(((op == ir::Opcode::Add) ? (a + b) : (a - b)) & mask,
                               type);
            return true;
        }
        case ir::Opcode::WrapAdd:  // 任务 474：回绕加=按类型位宽掩码回绕（免检面）
            out = formatSigned((a + b) & mask, type); return true;
        case ir::Opcode::WrapSub:
            out = formatSigned((a - b) & mask, type); return true;
        case ir::Opcode::WrapMul:
            out = formatSigned((a * b) & mask, type); return true;
        case ir::Opcode::Mul: {
            // 任务 474：乘法溢出不折叠（同 Add/Sub 策略）。64 位乘积无法在
            //   宿主可移植域内安全判定（__builtin_*_overflow MSVC 不可用/
            //   __int128 MSVC x64 不可用）——i64/u64 一律不折叠（保守保留
            //   运行期面）；窄/32 位全积在 int64/uint64 内，域预演安全。
            if (bits >= 64) return false;
            const std::int64_t sa = signExtend(a, bits, type);
            const std::int64_t sb = signExtend(b, bits, type);
            const std::int64_t product = sa * sb;   // |积| < 2^62 域内安全
            if (isUnsignedT) {
                const std::uint64_t maxV = mask;
                if (static_cast<std::uint64_t>(product) > maxV) return false;
            } else {
                const std::int64_t lo = -(static_cast<std::int64_t>(1) << (bits - 1));
                const std::int64_t hi = (static_cast<std::int64_t>(1) << (bits - 1)) - 1;
                if (product < lo || product > hi) return false;
            }
            out = formatSigned(static_cast<std::uint64_t>(product) & mask, type);
            return true;
        }
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
                // 任务 474：INT_MIN/-1 除法折叠=宿主 UB 面（结果超域经位域
                //   归一化静默回绕）——不折叠保留运行期错误码 8 通路
                const std::int64_t lo474 =
                    -(static_cast<std::int64_t>(1) << (bits - 1));
                if (sb == -1 && sa == lo474) return false;
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
                // 任务 474：INT_MIN % -1 同为除法溢出形态——不折叠
                const std::int64_t lo474 =
                    -(static_cast<std::int64_t>(1) << (bits - 1));
                if (sb == -1 && sa == lo474) return false;
                out = std::to_string(sa % sb);
            }
            return true;
        }
        case ir::Opcode::BitAnd: out = formatSigned(a & b, type); return true;
        case ir::Opcode::BitOr: out = formatSigned(a | b, type); return true;
        case ir::Opcode::BitXor: out = formatSigned(a ^ b, type); return true;
        case ir::Opcode::Shl:
            // 任务 474：移位量超宽不再折叠掩码（原 2026-09-09 规范化废止）——
            //   b >= 位宽（或负）语义=错误码 8『溢出错误』运行期确定性失败，
            //   此处不折叠以保留指令走运行期检查（语义层字面量面已编译期硬错误）
            if (b >= static_cast<std::uint64_t>(bits)) return false;
            out = formatSigned((a << b) & mask, type);
            return true;
        case ir::Opcode::Shr: {
            // 有符号算术右移（无符号逻辑右移由类型决定）
            // 任务 474：超宽移位量同样不折叠（保留运行期错误码 8 通路）
            if (b >= static_cast<std::uint64_t>(bits)) return false;
            const bool isUnsigned = (type.size() >= 2 && type[0] == 'u');
            const int shift = static_cast<int>(b);
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
    if (type == "f32") {
        // 浮32 折叠全程 float32 域——与运行期 addss/subss/mulss/divss 同语义
        //   （001：编译期常量折叠与三后端运行期发射同语义）。禁在 double 域
        //   运算后截断：折叠链中间文本（%.9g）在 double 域二次运算时量化偏差
        //   被减法放大——0.1f+0.1f-0.2f 曾折出 3e-9 假差（运行期 float32 域
        //   位模式 0x3E4CCCCD-0x3E4CCCCD 精确=0），假差经 ×1e9 折成 3.0 整链
        //   假值。float32 域运算后 %.9g 文本往返仍无损（见 textToF32）。
        float a = 0.0f, b = 0.0f;
        // 静默豁免业务值: 文本不可解析=确权「不可折叠」返回 假，上层放弃折叠保留
        //   运行期原指令求值（无默认值无错值）——继承冻结基线同语义旧锚
        //   parseDouble 失败链放行面（1187 折叠放弃通道同构先例）
        if (!textToF32(aText, a) || !textToF32(bText, b)) return false;
        float result = 0.0f;
        switch (op) {
            case ir::Opcode::Add: result = a + b; break;
            case ir::Opcode::Sub: result = a - b; break;
            case ir::Opcode::Mul: result = a * b; break;
            case ir::Opcode::Div:
                // 浮点除零不折叠（保留 IEEE 语义/运行期行为）
                if (b == 0.0f) return false;
                result = a / b;
                break;
            default:
                return false;
        }
        out = formatDouble(static_cast<double>(result), type);
        return true;
    }
    // 浮64：double 域运算（与运行期 addsd/subss 族同语义）
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
        // 浮32 目标单次正确舍入（与运行期 cvtsi2ss 同语义）——禁经 double
        //   中转双重舍入（int64 大值落在 double/float 舍入边界间时差 1 ulp）。
        //   浮点目标闭集 {f32, f64}，三目显式枚举（else 降格纪律）
        const double promoted = (toType == "f32")
            ? static_cast<double>(static_cast<float>(value))
            : static_cast<double>(value);
        out = formatDouble(promoted, toType);
        isFloat = true;
        return true;
    }
    // 浮 -> 整（528 饱和折叠·475 甲案 Rust as 对齐）：NaN→0·超域→MIN/MAX
    //   （无符号负超域→0）·域内向零截断——与 runtime __cn_fXX_to_YY 饱和辅助
    //   族/三后端运行期发射同语义（原「域内折叠+掩码截断」编译期出模回绕值，
    //   整32(1.0e18) 折叠 -1486618624 与饱和立法分叉）。判据边界全取浮点精确
    //   可表示值（窄宽=MIN/MAX 本身；i64/u64=±2^63/2^64 幂界·灰区无可表示值
    //   宽判据与截断同值）。128 目标本层不折叠（isIntType 不含——保守留运行期，
    //   运行期饱和结果一致）。
    if (isFloatType(fromType) && isIntType(toType)) {
        double d = 0.0;
        if (!parseDouble(fromText, d)) return false;
        // 浮32 源先按 f32 精度截断再判界/截断（与下方 f32->f64 审查修复同构）
        //   ——%.9g 折叠文本在 double 域直接判界时量化偏差可跨整数边界
        //   （2^30f 的 %.9g 文本读回差 4，饱和/截断结果错位）
        if (fromType == "f32") {
            d = static_cast<double>(static_cast<float>(d));
        }
        const int bits = intBitWidth(toType);
        if (d != d) {  // NaN → 0
            out = "0";
            isFloat = false;
            return true;
        }
        if (toType[0] == 'u') {  // 无符号族：负超域（含 -inf）→0
            const std::uint64_t umax =
                (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
            const double hiF = (bits >= 64)
                ? 18446744073709551616.0                       // 2^64
                : static_cast<double>(umax) + 1.0;             // MAX+1 可精确
            const std::uint64_t uv = (d <= -1.0) ? 0ULL
                : (d >= hiF) ? umax
                : static_cast<std::uint64_t>(d);
            out = formatSigned(uv, toType);
            isFloat = false;
            return true;
        }
        const std::int64_t lo = -(1LL << (bits - 1));
        const std::int64_t hi = (bits >= 63)
            ? 9223372036854775807LL
            : ((1LL << (bits - 1)) - 1);
        // 有符号族：hi 不可精确表示（bits=64）时上界判据用 2^63
        const double hiF = (bits >= 63)
            ? 9223372036854775808.0
            : static_cast<double>(hi);
        const std::int64_t value = (d >= hiF) ? hi
            : (d <= static_cast<double>(lo)) ? lo
            : static_cast<std::int64_t>(d);
        out = std::to_string(value);
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

// 浮点比较（f32=float32 域读回/f64=double 域——与运行期 ucomiss/ucomisd 同语义）。
//   禁一律 double 域比较：折叠链中间文本（%.9g）在 double 域比较时量化偏差
//   可翻转相等判定（0.2f==0.2f 曾折出假「真」——两文本 double 域差 2e-11，
//   float32 域同位模式必等）
bool ConstFoldPass::compareFloat(ir::Opcode op, const std::string& aText,
                                 const std::string& bText, const std::string& type,
                                 bool& result) {
    if (type == "f32") {
        float a = 0.0f, b = 0.0f;
        // 静默豁免业务值: 文本不可解析=确权「不可折叠」返回 假，上层放弃折叠保留
        //   运行期原指令求值（无默认值无错值）——继承冻结基线同语义旧锚
        //   parseDouble 失败链放行面（1187 折叠放弃通道同构先例）
        if (!textToF32(aText, a) || !textToF32(bText, b)) return false;
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
    // 引用而非拷贝（旧实现每条可折叠指令复制类型字符串）
    static const std::string kEmptyType;
    const std::string& opType = out.empty() ? kEmptyType : out[0].type;
    const std::string& resultType = inst.type;

    switch (inst.opcode) {
        // ---- 算术（整型/浮点；Wrap* 回绕折叠同面·任务 474）----
        case ir::Opcode::Add:
        case ir::Opcode::Sub:
        case ir::Opcode::Mul:
        case ir::Opcode::Div:
        case ir::Opcode::Mod:
        case ir::Opcode::WrapAdd:
        case ir::Opcode::WrapSub:
        case ir::Opcode::WrapMul: {
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
                                ? compareFloat(inst.opcode, out[0].extra, out[1].extra,
                                               opType, result)
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
                //   switch 判定（旧实现为 27 项 || 长链，每指令比较开销大）
                const bool isPureOp = isConstPropagateTarget(inst.opcode);
                // 操作数数量与可折叠性按 opcode 一次判定（旧实现每指令两次）
                const std::size_t opCount = operandCount(inst.opcode);
                const bool foldable = isFoldableOpcode(inst.opcode);
                // 注意7：Branch 条件（尾部操作数）必须传播——条件跳转块的
                //        最后一条指令被折叠为 ConstBool 后，尾部条件寄存器若不替换，
                //        codegen emitTerminator 读 last.operands.back() 得到槽垃圾 -> 崩溃。
                //        因此尾部操作数无条件传播（条件是 i1 布尔，安全）。
                bool replaced = false;
                if (isPureOp) {
                    for (std::size_t i = 0; i < opCount && i < inst.operands.size(); ++i) {
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
                                //  MASM 不支持 0b/0x 前缀）；十进制恒等跳过拷贝
                                op = cv;
                                if (hasRadixPrefix(op.extra)) {
                                    op.extra = normalizeIntText(op.extra);
                                }
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
                for (std::size_t i = opCount;
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
                // 第二步：常量折叠（操作数已替换为常量/命中常量表）。
                //   仅可折叠操作码进入（不可折叠指令跳过操作数解析与向量拷贝）。
                if (foldable && foldInstruction(inst, consts)) changed = true;
                // 折叠后指令已变为常量指令 -> updateConstTable 登记新常量；
                // 副作用指令 -> 清空映射（内存可能被修改，保守）
                updateConstTable(inst, consts);
            }
            // 283-a T12 字段化：块终止条件寄存器命中常量表 -> 改写为常量文本
            //   （旧契约把条件挂在块尾指令 operands，由尾部传播机制替换；
            //    字段化后由本段承接，供 cross_block_dce foldConstantBranch
            //    折叠恒真/恒假分支）
            if (block->terminated && block->termKind == "条件跳转") {
                int condReg = 0;
                // 解析失败（IR 契约外文本）：不传播（与旧 stoi 异常忽略等价）
                if (parseRegText(block->termCondition, condReg)) {
                    auto it = consts.find(condReg);
                    if (it != consts.end() && it->second.isConstant &&
                        it->second.type != "f32" && it->second.type != "f64" &&
                        it->second.type != "i128" && it->second.type != "u128") {
                        block->termCondition = it->second.extra;
                        changed = true;
                    }
                }
            }
        }
    }
    return changed;
}

// ==================== 位域归一化（D31 方案C①·258-a：优化链出口保证） ====================

// 单个整型常量文本按类型位宽归一化（-O3 优化链各 pass 折叠/强度削减产物可能出现
// 「数值超出其类型位宽」的文本——如传播链把整64 域数值带进 整32 常量）。
// 归一化=值按类型位宽掩码截断后按类型符号性格式化回文本（Rust wrapping 同构，
// 与运行期回绕语义一致）；域内文本是恒等变换（产物不变，锚定链零影响）。
// 可解析性失败（浮点/布尔/ptr 标签文本）原样保留返回 false；
// i128/u128 split 文本不在范围（语义层生成时保证域内+后端全走 64 位成对装载）。
bool ConstFoldPass::normalizeConstText(const std::string& text, const std::string& type,
                                       std::string& out) {
    if (!isIntType(type)) return false;
    std::uint64_t raw = 0;
    if (!parseConstInt(text, type, raw)) return false;
    const std::string normalized = formatSigned(raw, type);
    if (normalized == text) return false;
    out = normalized;
    return true;
}

// 模块级归一化：ConstInt 指令文本与全部操作数位置的内联整型常量逐一点名检查。
// 由 runOptLevel 在 fixpoint 收敛后调用（见 pass_manager.cpp），任何优化 pass
// 的常量产物在出口统一收口——到达后端的整型常量必在类型域内。
void ConstFoldPass::normalizeModuleConstWidths(ir::IRModule& module) {
    for (auto& fn : module.functions) {
        for (auto& block : fn.blocks) {
            for (auto& inst : block->instructions) {
                for (auto& op : inst.operands) {
                    if (!op.isConstant) continue;
                    std::string out;
                    if (normalizeConstText(op.extra, op.type, out)) op.extra = out;
                }
                // ConstInt 指令：extra 与操作数[0] 常量同源，以归一化后的操作数为准
                if (inst.opcode == ir::Opcode::ConstInt && !inst.operands.empty() &&
                    inst.operands[0].isConstant) {
                    inst.extra = inst.operands[0].extra;
                }
            }
        }
    }
}

} // namespace opt
} // namespace cn_compiler