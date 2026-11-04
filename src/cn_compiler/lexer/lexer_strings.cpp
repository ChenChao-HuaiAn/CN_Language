// 词法分析器·字符串/字符字面量识别：带前缀字符串（原始/多行组合）、字符串体
// （单引号/三引号多行）、\u{} 转义三态校验、字符字面量（转义序列）——自
// lexer.cpp 纯搬家拆分（该文件超 900 行警戒线贴 1000 硬红线·先拆再加纪律；
// 零行为变更，函数体逐字未动）。
#include <cstddef>
#include <string>

#include "cn_compiler/lexer/lexer.hpp"

namespace cn_compiler {

// 读取带前缀的字符串字面量：支持 原始 / 多行 / 原始多行 / 多行原始 组合
// 字符串Token的value保留完整原始文本（含前缀与引号），供语法分析器统一解码
// prefixText 已由调用方收集完整前缀（含组合前缀，如"原始多行"），find 判断即可
Token Lexer::readPrefixedString(const std::string& prefixText, const SourceLocation& loc) {
    const bool raw = (prefixText.find("原始") != std::string::npos);
    const bool multiLine = (prefixText.find("多行") != std::string::npos);
    if (peek() != U'"') {
        reportError(loc, "字符串前缀后必须紧跟引号");
        return Token(TokenType::StringLiteral, "", loc);
    }
    return Token(TokenType::StringLiteral, prefixText + readStringBody(multiLine, raw), loc);
}

// 读取字符串体：返回含引号的原始文本
// multiLine=true 使用三引号"""..."""（可跨行）；raw=true 不处理转义
std::string Lexer::readStringBody(bool multiLine, bool raw) {
    // 多行前缀但实际只有单引号时，降级为普通字符串
    const bool isTripleQuote = peek() == U'"' && peekNext(1) == U'"' && peekNext(2) == U'"';
    if (multiLine && !isTripleQuote) multiLine = false;

    std::string text;
    if (multiLine) {
        // 消费三引号 """
        for (int i = 0; i < 3 && peek() == U'"'; i++) {
            text += currentUtf8();
            advance();
        }
        // 读取到下一个 """ 为止（可跨行，\n由 advance 更新行号）
        while (!isAtEnd()) {
            if (peek() == U'"' && peekNext(1) == U'"' && peekNext(2) == U'"') {
                text += "\"\"\"";
                advance();
                advance();
                advance();
                return text;
            }
            if (peek() == U'\\' && !raw) {
                text += currentUtf8();
                advance();
                if (isAtEnd()) break;
                // 282-a T5 多行段：\u{ 结构感知+三态校验（对齐普通段 278-a
                //   同款——原宽松 +1 字符收集不识别结构；raw 多行不处理转义
                //   保持 raw 语义）
                if (peek() == U'u') {
                    text += currentUtf8();
                    advance();
                    if (peek() != U'{') {
                        reportError(currentLocation(), "无效的Unicode转义：缺少 {");
                        text += currentUtf8();
                        advance();
                        continue;
                    }
                    text += currentUtf8();
                    advance();
                    const std::size_t hexStart = text.size();
                    while (!isAtEnd() && peek() != U'}') {
                        text += currentUtf8();
                        advance();
                    }
                    const std::string hexBody = text.substr(hexStart);
                    if (peek() == U'}') {
                        text += currentUtf8();
                        advance();
                    } else {
                        reportError(currentLocation(), "无效的Unicode转义：缺少 }");
                    }
                    validateUnicodeEscape(hexBody, currentLocation());
                    continue;
                }
                text += currentUtf8();
                advance();
                continue;
            }
            text += currentUtf8();
            advance();
        }
        reportError(currentLocation(), "未闭合的多行字符串");
        return text;
    }

    // 普通或原始字符串：单引号包裹
    text += currentUtf8();
    advance(); // 消费开引号 "
    while (!isAtEnd()) {
        char32_t c = peek();
        if (c == U'"') {
            text += currentUtf8();
            advance();
            return text;
        }
        if (c == U'\n') {
            reportError(currentLocation(), "字符串字面量不能跨行");
            return text;
        }
        if (c == U'\\' && !raw) {
            text += currentUtf8();
            advance();
            if (isAtEnd()) break;
            // 278-a T5：\u{ 结构感知+三态校验（原宽松 +1 字符收集不识别结构）
            if (peek() == U'u') {
                text += currentUtf8();
                advance();
                if (peek() == U'{') {
                    text += currentUtf8();
                    advance();
                    const std::size_t hexStart = text.size();
                    while (!isAtEnd() && peek() != U'}') {
                        text += currentUtf8();
                        advance();
                    }
                    const std::string hexBody = text.substr(hexStart);
                    if (peek() == U'}') {
                        text += currentUtf8();
                        advance();
                    } else {
                        reportError(currentLocation(), "无效的Unicode转义：缺少 }");
                    }
                    validateUnicodeEscape(hexBody, currentLocation());
                    continue;
                }
                reportError(currentLocation(), "无效的Unicode转义：缺少 {");
                text += currentUtf8();
                advance();
                continue;
            }
            text += currentUtf8();
            advance();
            continue;
        }
        text += currentUtf8();
        advance();
    }
    reportError(currentLocation(), "未闭合的字符串字面量");
    return text;
}

// 278-a T5：\u{...} 十六进制体三态校验（空转义/超码点>U+10FFFF/代理区
//   U+D800~U+DFFF）+非法十六进制字符——四态均 reportError（词法期拒绝，
//   消除 decodeEscapes 生成非法 UTF-8 字节的面）。
bool Lexer::validateUnicodeEscape(const std::string& hexBody, const SourceLocation& loc) {
    if (hexBody.empty()) {  // 静默豁免上游已诊断: 空转义已 reportError 后显式 false 哨兵·非静默默认值（lexer.cpp 拆分迁入）
        reportError(loc, "无效的Unicode转义：\\u{} 空转义（须为 Unicode 码点）");
        return false;
    }
    char32_t cp = 0;
    for (char h : hexBody) {
        cp *= 16;
        if (h >= '0' && h <= '9') cp += h - '0';
        else if (h >= 'a' && h <= 'f') cp += h - 'a' + 10;
        else if (h >= 'A' && h <= 'F') cp += h - 'A' + 10;
        else {
            reportError(loc, "无效的Unicode转义：非十六进制字符");
            return false;  // 静默豁免上游已诊断: reportError 已报「非十六进制字符」false=词法错误通道非静默默认（2051 rebase registry 并集随迁）
        }
    }
    if (cp > 0x10FFFF) {  // 静默豁免闭集穷举: U+10FFFF=Unicode 规范上界常量判定·非业务区间单边比较（lexer.cpp 拆分迁入）
        reportError(loc, "无效的Unicode转义：码点超过 U+10FFFF 上限");
        return false;
    }
    if (cp >= 0xD800 && cp <= 0xDFFF) {
        reportError(loc, "无效的Unicode转义：代理区码点（U+D800~U+DFFF）不可直接使用");
        return false;
    }
    return true;
}

// 读取字符字面量：'X'、'\n'、'\t'、'\0'、'\\'、'\''、'\"'、'\u{XXXX}'
Token Lexer::readChar() {
    SourceLocation loc = currentLocation();
    advance(); // 消费开引号（单引号）
    std::string text = "'";
    if (isAtEnd()) {
        reportError(loc, "未闭合的字符字面量");
        return Token(TokenType::CharLiteral, text, loc);
    }
    if (peek() == U'\\') {
        // 转义序列
        text += currentUtf8();
        advance(); // 消费反斜杠
        if (isAtEnd()) {
            reportError(loc, "未闭合的字符字面量");
            return Token(TokenType::CharLiteral, text, loc);
        }
        const char32_t esc = peek();
        if (esc == U'u') {
            // Unicode转义 \u{XXXX}——278-a T5：结构严格化（缺 { 拒绝）+
            //   收集 hex 体后三态校验（空/超限/代理区）
            text += currentUtf8();
            advance(); // 消费 u
            if (peek() != U'{') {
                reportError(loc, "无效的Unicode转义：缺少 {");
            }
            // 静默豁免业务值: 缺 { 已在 if 分支报错·else=合法收集路径（isAtEnd/peek 守卫先行·currentUtf8 越界空串=推进语义非失败通道）
            else {
                text += currentUtf8();
                advance(); // 消费 {
                const std::size_t hexStart = text.size();
                while (!isAtEnd() && peek() != U'}') {
                    text += currentUtf8();
                    advance();
                }
                const std::string hexBody = text.substr(hexStart);
                if (peek() == U'}') {
                    text += currentUtf8();
                    advance();
                } else {
                    reportError(loc, "无效的Unicode转义：缺少 }");
                }
                validateUnicodeEscape(hexBody, loc);
            }
        } else if (esc == U'n' || esc == U't' || esc == U'r' || esc == U'0' ||
                   esc == U'\\' || esc == U'\'' || esc == U'"') {
            text += currentUtf8();
            advance();
        } else {  // 静默豁免上游已诊断: 非法转义已 reportError 后按原样收集·非静默默认（lexer.cpp 拆分迁入）
            reportError(loc, "无效的转义序列");
            text += currentUtf8();
            advance();
        }
    } else {  // 静默豁免业务值: 普通字符收集默认分支·非失败通道（1360 批4 同口径·lexer.cpp 拆分迁入）
        // 普通字符（含多字节中文）
        text += currentUtf8();
        advance();
    }
    // 闭引号
    if (peek() == U'\'') {
        text += currentUtf8();
        advance();
    } else {
        reportError(loc, "未闭合的字符字面量");
    }
    return Token(TokenType::CharLiteral, text, loc);
}

} // namespace cn_compiler
