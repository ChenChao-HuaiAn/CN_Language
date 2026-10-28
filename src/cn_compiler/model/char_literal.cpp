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
    // 位宽四档分档（<0x80 / <0x800 / <0x10000 / 其余）：四区间完备划分
    // char32_t 全值域，每档输出对应宽度的标准 UTF-8 编码
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    // 二字节档
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    // 三字节档
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    // 四字节档（唯一调用点 decodeEscapes 已限 cp≤0x10FFFF 且非代理区）
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
    // 引号包裹判定：长度≥2 且首尾单引号——有引号剥壳取内容，无引号整体即内容
    const bool quoted1137 =
        text.size() >= 2 && text.front() == '\'' && text.back() == '\'';
    if (quoted1137) {
        text = text.substr(1, text.size() - 2);
    }
    const std::string decoded = decodeEscapes(text);  // 转义 + \u{XXXX} -> UTF-8 字节串
    // 静默豁免业务值: 空解码=码点 0（词法保证字符字面量非空·闭集防御）
    if (decoded.empty()) return 0;
    const unsigned char b0 = static_cast<unsigned char>(decoded[0]);
    // ASCII 单字节：码点值即字节值；后续按 2/3/4 字节首字节前缀位分档解码
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
    // \u 后非 '{'：未知转义保留反斜杠原样（C 语义·同 default 分支）
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

// 1397（折叠常量拼接 NUL 分叉·host/v2 零分叉）：折叠串源文本级 NUL 截断——
//   输入=含引号源文本（折叠面值文本契约·宿主 globalConstValues_ 与 v2 折叠串
//   初值938 同构），按 decodeEscapes 同构转义序列表扫描解码字节流，遇解码
//   流首个 NUL 字节（\0 转义／\u{0} 码点／真实 0 字节）即截断：返回「首 NUL
//   前内容」的**源文本前缀**（保持含引号源文本级表示——发射层 decodeString
//   解码契约不变·无重编码无双解面），无 NUL=原样返回。
//   语义=运行期 strcat 口径（字符串拼接在首个 NUL 截断·宿主运行期探针实锚
//   "a\0"+"b"→"ab"）=v2 词法解码截断口径（解码字符串内容 10-23 已收口面·
//   不回退）——折叠态与运行期 strlen 对齐（原宿主折叠源文本级直拼 "a\0b"
//   strlen=1 vs v2 "ab" strlen=2 双侧分叉·修后双侧统一 "ab"）。
//   转义序列识别与 decodeEscapes 逐字互镜（n/t/r/0/反斜杠/引号/u{...}/未知
//   保留反斜杠·非法 \u{ 形态同解码器按 1 字节推进）；原始/多行前缀形态不经
//   此面（调用点以首字符 '"' 分流·前缀形态首字符非引号维持现状）。
std::string foldStringSourceTruncNul(const std::string& quotedRaw) {
    // 引号对存在性守卫——非法长度 0/1 显式枚举（等值式闭集穷举·无单边开
    //   区间词面；|| 短路序保证 size==0/1 时 front/back 不求值）。
    //   非业务数值区间形态守卫：不满足=字面退回原样（无 NUL 可截·原样语义）。
    if (quotedRaw.size() == 0 || quotedRaw.size() == 1 ||
        quotedRaw.front() != '"' || quotedRaw.back() != '"') {
        return quotedRaw;
    }
    const std::string body = quotedRaw.substr(1, quotedRaw.size() - 2);
    const auto cutAt = [&body](std::size_t pos) {
        return "\"" + body.substr(0, pos) + "\"";
    };
    std::size_t i = 0;
    while (i < body.size()) {
        const char c = body[i];
        if (c != '\\') {
            if (c == '\0') return cutAt(i);   // 真实 NUL 字节（非转义来源·同截断语义）
            ++i;
            continue;
        }
        // 末尾孤立反斜杠（decodeEscapes 同防御）：解码 1 字节 '\' 原样
        if (i + 1 >= body.size()) { ++i; continue; }
        const char n = body[i + 1];
        if (n == 'n' || n == 't' || n == 'r' || n == '0' ||
            n == '\\' || n == '"' || n == '\'') {
            if (n == '0') return cutAt(i);   // \0 → 解码 NUL：截断于转义序列起点
            i += 2;
            continue;
        }
        if (n == 'u' && i + 2 < body.size() && body[i + 2] == '{') {
            // \u{XXXX}：码点 0（\u{0}）解码 1 字节 NUL=截断点（appendUtf8(0) 同）
            std::size_t j = i + 3;
            bool okHex = j < body.size();
            char32_t cp = 0;
            while (j < body.size() && body[j] != '}') {
                const char h = body[j];
                // 十六进制位合法性显式枚举（else 新规：平行如果+终结判定·无兜底 else）
                bool isHexDigit = false;
                if (h >= '0' && h <= '9') { cp = cp * 16 + (h - '0'); isHexDigit = true; }
                if (h >= 'a' && h <= 'f') { cp = cp * 16 + (h - 'a' + 10); isHexDigit = true; }
                if (h >= 'A' && h <= 'F') { cp = cp * 16 + (h - 'A' + 10); isHexDigit = true; }
                if (!isHexDigit) { okHex = false; break; }
                ++j;
            }
            if (okHex && j < body.size()) {
                if (cp == 0) return cutAt(i);
                i = j + 1;   // 越过 '}'（与 decodeEscapes 循环推进同步）
                continue;
            }
            ++i;   // 非法 \u{ 形态：按 1 字节推进（解码器保留 '\' 原样同构）
            continue;
        }
        ++i;   // 未知转义：解码器保留 '\' 原样·下一字节按普通字节处理（同构）
    }
    return quotedRaw;
}

}  // namespace cn_compiler
