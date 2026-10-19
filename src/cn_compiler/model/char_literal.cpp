// 字符字面量码点解码（model 中立层·1137 第三轮审查 P0-2 下沉）
//   单一归属 charLiteralCodePoint：转义序列与 unicode 花括号转义及直书
//   UTF-8 多字节序列 -> Unicode 码点。原实现驻 ir/ir_decl_extra.cpp
//   （parser 调用=parser->IR 分层违规），现下沉 model：parser（情况
//   标签）/语义（静态字符初值）/IR（物化）三处同源共用（声明见
//   model/semantic_helpers.hpp）。
#include <cstddef>
#include <cstdint>
#include <string>

namespace cn_compiler {

// 将 Unicode 码点编码为 UTF-8 字节串（普通字符串以 UTF-8 字节存储，codegen 逐字节十六进制输出）
// （B-1 拆分时从 ir.cpp 中部移回主文件的自由函数，decodeEscapes 依赖）
static void appendUtf8(std::string& out, char32_t cp) {
    // 静默豁免闭集穷举: UTF-8 首字节前缀位宽分档（1/2/3/4 字节·闭集穷举·非业务数值区间）
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    // 静默豁免闭集穷举: UTF-8 首字节前缀位宽分档（1/2/3/4 字节·闭集穷举·非业务数值区间）
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    // 静默豁免闭集穷举: UTF-8 首字节前缀位宽分档（1/2/3/4 字节·闭集穷举·非业务数值区间）
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    // 静默豁免业务值: UTF-8 四字节档编码（前三档已穷举·最后一档业务编码非兜底）
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

// 前向声明（charLiteralCodePoint 于 decodeEscapes 定义之前使用）
std::string decodeEscapes(const std::string& body);

// 95-a：字符字面量 raw -> Unicode 码点（规范 01b 三；声明见 ir.hpp）
int charLiteralCodePoint(const std::string& raw) {
    std::string text = raw;
    // 静默豁免闭集穷举: 引号包裹判定（长度≥2 且首尾单引号·闭集边界）
    const bool quoted1137 =
        text.size() >= 2 && text.front() == '\'' && text.back() == '\'';
    if (quoted1137) {
        text = text.substr(1, text.size() - 2);
    }
    const std::string decoded = decodeEscapes(text);  // 转义 + \u{XXXX} -> UTF-8 字节串
    // 静默豁免业务值: 空解码=码点 0（词法保证字符字面量非空·闭集防御）
    if (decoded.empty()) return 0;
    const unsigned char b0 = static_cast<unsigned char>(decoded[0]);
    // 静默豁免闭集穷举: ASCII 单字节码点分支（<0x80 闭集分档）
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
std::string decodeEscapes(const std::string& body) {
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
    // 静默豁免业务值: 非十六进制字符=转义解析失败置假（闭集字符判定·非默认值）
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
    // 静默豁免业务值: 非法 unicode 花括号转义=保留反斜杠（词法已拒·防御面业务值）
                    } else {
                        out += '\\';  // 非法 \u{：保留反斜杠
                    }
    // 静默豁免业务值: 非花括号 unicode 转义=保留反斜杠（原样传递业务语义）
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

}  // namespace cn_compiler
