// CN-IR生成器实现：字符串解码 + 类型映射族（350 重构E2 自 ir.cpp 纯机械搬移）
// 职责（函数体逐字搬移·零语义变化；成员声明保持在 ir.hpp 原位）：
//   appendUtf8/decodeEscapes/charLiteralCodePoint —— 字符串/字符字面量解码
//   mapType/mapBinaryOp/isCompoundAssignOp/baseOpOfCompound/widenCompoundRhs
//                                                    —— 类型与运算符映射
//   decodeString —— 字符串字面量前缀识别+引号剥离+转义解码
#include <cstdint>
#include <string>
#include <utility>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// ==================== 字符串字面量解码（Task 2.5 完整实现） ====================

// 将 Unicode 码点编码为 UTF-8 字节串（普通字符串以 UTF-8 字节存储，codegen 逐字节十六进制输出）
// （B-1 拆分时从 ir.cpp 中部移回主文件的自由函数，decodeEscapes 依赖）
static void appendUtf8(std::string& out, char32_t cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

// 前向声明（95-a：charLiteralCodePoint 于 decodeEscapes 定义之前使用）
static std::string decodeEscapes(const std::string& body);

// 95-a：字符字面量 raw -> Unicode 码点（规范 01b 三；声明见 ir.hpp）
int charLiteralCodePoint(const std::string& raw) {
    std::string text = raw;
    if (text.size() >= 2 && text.front() == '\'' && text.back() == '\'') {
        text = text.substr(1, text.size() - 2);
    }
    const std::string decoded = decodeEscapes(text);  // 转义 + \u{XXXX} -> UTF-8 字节串
    if (decoded.empty()) return 0;
    const unsigned char b0 = static_cast<unsigned char>(decoded[0]);
    if (b0 < 0x80) return static_cast<int>(b0);
    auto cont = [&decoded](std::size_t i) -> int {
        return (i < decoded.size())
                   ? (static_cast<unsigned char>(decoded[i]) & 0x3F) : 0;
    };
    if ((b0 & 0xE0) == 0xC0) {
        return ((b0 & 0x1F) << 6) | cont(1);
    }
    if ((b0 & 0xF0) == 0xE0) {
        return ((b0 & 0x0F) << 12) | (cont(1) << 6) | cont(2);
    }
    if ((b0 & 0xF8) == 0xF0) {
        return ((b0 & 0x07) << 18) | (cont(1) << 12) | (cont(2) << 6) | cont(3);
    }
    return static_cast<int>(b0);
}

// 解码转义序列（\n \t \r \0 \\ \" \' \u{XXXX}），返回解码后的字符串
// （B-1 拆分时从 ir.cpp 中部移回主文件的自由函数，decodeString 依赖）
static std::string decodeEscapes(const std::string& body) {
    std::string out;
    for (std::size_t i = 0; i < body.size(); ++i) {
        const char c = body[i];
        if (c != '\\') {
            out += c;
            continue;
        }
        if (i + 1 >= body.size()) {
            out += c;  // 末尾孤立反斜杠：原样保留（词法层已保证闭合，防御）
            break;
        }
        const char n = body[i + 1];
        switch (n) {
            case 'n': out += '\n'; i += 1; break;
            case 't': out += '\t'; i += 1; break;
            case 'r': out += '\r'; i += 1; break;
            case '0': out += '\0'; i += 1; break;
            case '\\': out += '\\'; i += 1; break;
            case '"': out += '"'; i += 1; break;
            case '\'': out += '\''; i += 1; break;
            case 'u': {
                // \u{XXXX}：Unicode 码点转义（规格书4.3）
                if (i + 2 < body.size() && body[i + 2] == '{') {
                    std::size_t j = i + 3;
                    const std::size_t hexStart = j;
                    while (j < body.size() && body[j] != '}') ++j;
                    if (j < body.size() && j > hexStart) {
                        const std::string hex = body.substr(hexStart, j - hexStart);
                        char32_t cp = 0;
                        bool ok = true;
                        for (char h : hex) {
                            cp *= 16;
                            if (h >= '0' && h <= '9') cp += h - '0';
                            else if (h >= 'a' && h <= 'f') cp += h - 'a' + 10;
                            else if (h >= 'A' && h <= 'F') cp += h - 'A' + 10;
                            else { ok = false; break; }
                        }
                        if (ok) {
                            // 278-a T5 防御：超码点/代理区不 append（词法层
                            //   validateUnicodeEscape 已拒·此处防御非法 UTF-8
                            //   字节生成面——字符/字符串两消费面共用本函数）
                            if (cp <= 0x10FFFF && !(cp >= 0xD800 && cp <= 0xDFFF)) {
                                appendUtf8(out, cp);
                            }
                        }
                        i = j;  // 跳到 '}'（循环 ++i 后到达 '}' 之后）
                    } else {
                        out += '\\';  // 非法 \u{：保留反斜杠
                    }
                } else {
                    out += '\\';
                }
                break;
            }
            default: out += c; break;  // 未知转义（如 \d）：保留反斜杠原样（C 语义）
        }
    }
    return out;
}

// ==================== 类型映射 ====================

// 源码类型 -> IR类型映射（规格书7.5类型表示）
std::string IRGenerator::mapType(const std::string& type) {
    // Task 6.1（泛型函数实例化）：类型参数 T/U 先替换为实参类型
    //   （最小<整32> 函数体内 返回 T / 局部 T 变量须映射为整32 而非 ptr 兜底）
    if (!genericTypeParams_.empty()) {
        const std::string subst = substGenericType(type);
        if (subst != type) return mapType(subst);
    }
    if (type == "整8") return "i8";
    if (type == "整16") return "i16";
    if (type == "整32" || type == "整数") return "i32";
    if (type == "整64") return "i64";
    if (type == "整128") return "i128";
    if (type == "正8") return "u8";
    if (type == "正16") return "u16";
    if (type == "正32") return "u32";
    if (type == "正64") return "u64";
    if (type == "正128") return "u128";
    if (type == "浮32") return "f32";
    if (type == "浮64" || type == "小数") return "f64";
    if (type == "布尔") return "i1";
    if (type == "字符") return "i32";
    if (type == "字符串") return "ptr";
    if (type == "空类型") return "void";
    if (!type.empty() && type.back() == '*') return "ptr";
    // Task 2.4：数组类型（整32[10]）在IR层按元素类型处理（栈上连续分配，
    // 数组名引用为元素指针；Alloca 元素大小按元素类型登记）
    if (types::isArray(type)) return mapType(types::arrayElemOf(type));
    // Task 2.2：函数指针类型（函数指针<返回>(参数,...)）按指针处理
    if (!type.empty() && type.rfind("函数指针<", 0) == 0) return "ptr";
    // Task 2.7：枚举类型按整32处理（与语义层 typeSizeOf 一致）
    if (semantic_ != nullptr && semantic_->isEnumType(type)) return "i32";
    if (!type.empty() && type != "未知") return "ptr";  // 自定义类型按指针处理
    return "void";
}
bool IRGenerator::mapBinaryOp(Operator op, bool isFloat, ir::Opcode& out) {
    (void)isFloat;  // 浮点/整型共用Opcode，由 codegen 按类型分派（Task 2.3）
    switch (op) {
        // ---- 算术（整型与浮点共用Opcode，codegen按类型分派SSE/整型指令） ----
        case Operator::Add: out = ir::Opcode::Add; return true;
        case Operator::Subtract: out = ir::Opcode::Sub; return true;
        case Operator::Multiply: out = ir::Opcode::Mul; return true;
        case Operator::Divide: out = ir::Opcode::Div; return true;
        case Operator::Modulo: out = ir::Opcode::Mod; return true;
        // ---- 比较（结果为布尔） ----
        case Operator::EqualEqual: out = ir::Opcode::Eq; return true;
        case Operator::BangEqual: out = ir::Opcode::Ne; return true;
        case Operator::Less: out = ir::Opcode::Lt; return true;
        case Operator::LessEqual: out = ir::Opcode::Le; return true;
        case Operator::Greater: out = ir::Opcode::Gt; return true;
        case Operator::GreaterEqual: out = ir::Opcode::Ge; return true;
        // ---- 逻辑（布尔操作数） ----
        case Operator::AndAnd: out = ir::Opcode::And; return true;
        case Operator::OrOr: out = ir::Opcode::Or; return true;
        // ---- 位运算与移位（Task 2.3 新增，整型专用；浮点不允许位运算） ----
        case Operator::Amp: out = ir::Opcode::BitAnd; return true;
        case Operator::Pipe: out = ir::Opcode::BitOr; return true;
        case Operator::Caret: out = ir::Opcode::BitXor; return true;
        case Operator::LessLess: out = ir::Opcode::Shl; return true;
        case Operator::GreaterGreater: out = ir::Opcode::Shr; return true;
        default: return false;
    }
}
bool IRGenerator::isCompoundAssignOp(Operator op) {
    switch (op) {
        case Operator::PlusAssign: case Operator::MinusAssign:
        case Operator::StarAssign: case Operator::SlashAssign:
        case Operator::PercentAssign:
        case Operator::AmpAssign: case Operator::PipeAssign:
        case Operator::CaretAssign: case Operator::LessLessAssign:
        case Operator::GreaterGreaterAssign:
            return true;
        default:
            return false;
    }
}
Operator IRGenerator::baseOpOfCompound(Operator op) {
    switch (op) {
        case Operator::PlusAssign: return Operator::Add;
        case Operator::MinusAssign: return Operator::Subtract;
        case Operator::StarAssign: return Operator::Multiply;
        case Operator::SlashAssign: return Operator::Divide;
        case Operator::PercentAssign: return Operator::Modulo;
        case Operator::AmpAssign: return Operator::Amp;
        case Operator::PipeAssign: return Operator::Pipe;
        case Operator::CaretAssign: return Operator::Caret;
        case Operator::LessLessAssign: return Operator::LessLess;
        case Operator::GreaterGreaterAssign: return Operator::GreaterGreater;
        default: return Operator::Assign;
    }
}

// 复合赋值右值宽化（331-a·T51 单一归属）——见 ir.hpp 声明注释。
ir::IRValue IRGenerator::widenCompoundRhs(const ir::IRValue& rhs,
                                          const std::string& targetType,
                                          const SourceLocation& loc) {
    if (targetType != "i128" && targetType != "u128") return rhs;
    if (rhs.type == targetType) return rhs;
    return emitResult(ir::Opcode::Cast, {rhs}, targetType, "", loc);
}
std::string IRGenerator::decodeString(const std::string& raw) {
    // 识别前缀（组合前缀 原始多行 / 多行原始 优先）。
    // 注意：中文前缀为 UTF-8 多字节，偏移必须用字节数（substr 按字节切割）：
    //       "原始"=6字节、"多行"=6字节、"原始多行"=12字节
    bool rawMode = false;
    bool multiLine = false;
    std::size_t start = 0;
    if (raw.rfind("原始多行", 0) == 0 || raw.rfind("多行原始", 0) == 0) {
        rawMode = true;
        multiLine = true;
        start = 12;
    } else if (raw.rfind("原始", 0) == 0) {
        rawMode = true;
        start = 6;
    } else if (raw.rfind("多行", 0) == 0) {
        multiLine = true;
        start = 6;
    }
    // 剥离引号：多行用三引号 """..."""；普通/原始用单引号 "..."（可带前缀）
    std::string body;
    if (multiLine) {
        if (raw.size() >= start + 6 && raw.compare(start, 3, "\"\"\"") == 0 &&
            raw.compare(raw.size() - 3, 3, "\"\"\"") == 0) {
            body = raw.substr(start + 3, raw.size() - start - 6);
        } else {
            body = raw.substr(start);  // 防御：非法形态保留原文
        }
    } else {
        if (raw.size() >= start + 2 && raw[start] == '"' && raw.back() == '"') {
            body = raw.substr(start + 1, raw.size() - start - 2);
        } else {
            body = raw.substr(start);
        }
    }
    // 原始形态：不处理转义（所见即所得）
    if (rawMode) return body;
    // 普通/多行：处理转义序列
    return decodeEscapes(body);
}

} // namespace cn_compiler
