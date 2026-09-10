// 词法分析器实现：UTF-8字符处理、关键字匹配、字面量与运算符识别（Task 1.2）
// 覆盖：59个中文关键字（v2.0 含模块系统关键字；plans/018 摘除 包/货舱 死保留字）、整数/浮点/字符串/字符字面量（含原始/多行前缀）、
//       39个运算符（含++/--）、9个分隔符（含 ::）、注释（块注释嵌套）、错误诊断
#include <cstddef>
#include <string>
#include <unordered_map>

#include "cn_compiler/lexer/lexer.hpp"

namespace cn_compiler {

namespace {

// 从 pos 处解码一个UTF-8字符，返回Unicode码点，len 输出该字符的字节长度
char32_t decodeUtf8(const std::string& s, std::size_t pos, int& len) {
    unsigned char first = static_cast<unsigned char>(s[pos]);
    len = 1;
    char32_t cp = first;
    if (first >= 0xF0) {
        len = 4;
        cp = first & 0x07;
    } else if (first >= 0xE0) {
        len = 3;
        cp = first & 0x0F;
    } else if (first >= 0xC0) {
        len = 2;
        cp = first & 0x1F;
    }
    for (int i = 1; i < len && pos + static_cast<std::size_t>(i) < s.size(); i++) {
        cp = (cp << 6) | (static_cast<unsigned char>(s[pos + static_cast<std::size_t>(i)]) & 0x3F);
    }
    return cp;
}

// ASCII数字判断（避免 cctype 对 char32_t 的不安全转换）
bool isAsciiDigit(char32_t c) { return c >= U'0' && c <= U'9'; }

// ASCII字母判断
bool isAsciiAlpha(char32_t c) {
    return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z');
}

// 61个关键字 -> TokenType 映射表（v2.0）
// 控制流(10)：如果/否则/当/循环/返回/中断/继续/选择/情况/默认
// 类型(21)：整数/小数/整8~整128/正8~正128/浮32/浮64/布尔/字符/字符串/空类型/结构体/联合体/枚举
// 声明(9)：函数/变量/导入/公开/私有/静态/自动 + 模块系统(2)：模块/作为
// 常量(4)：真/假/无/常量
// OOP(10)：类/接口/保护/虚拟/重写/抽象/实现/自身/父类/友元
// 错误处理(2)：结果/可选
// 字面量前缀(2)：原始/多行
// 泛型(1)：泛型
// 注：运算符 为上下文关键字（非保留字），不在此表，仅当后随运算符符号且处于
//     类/结构体函数定义上下文时由 parser 识别（Task 3.7）
const std::unordered_map<std::string, TokenType>& keywordTable() {
    static const std::unordered_map<std::string, TokenType> kTable = {
        // ---- 控制流(10) ----
        {"如果", TokenType::Kw_If}, {"否则", TokenType::Kw_Else},
        {"当", TokenType::Kw_While}, {"循环", TokenType::Kw_For},
        {"返回", TokenType::Kw_Return}, {"中断", TokenType::Kw_Break},
        {"继续", TokenType::Kw_Continue}, {"选择", TokenType::Kw_Switch},
        {"情况", TokenType::Kw_Case}, {"默认", TokenType::Kw_Default},
        // ---- 类型(21) ----
        {"整数", TokenType::Kw_Int}, {"小数", TokenType::Kw_Double},
        {"整8", TokenType::Kw_Int8}, {"整16", TokenType::Kw_Int16},
        {"整32", TokenType::Kw_Int32}, {"整64", TokenType::Kw_Int64},
        {"整128", TokenType::Kw_Int128},
        {"正8", TokenType::Kw_UInt8}, {"正16", TokenType::Kw_UInt16},
        {"正32", TokenType::Kw_UInt32}, {"正64", TokenType::Kw_UInt64},
        {"正128", TokenType::Kw_UInt128},
        {"浮32", TokenType::Kw_Float32}, {"浮64", TokenType::Kw_Float64},
        {"布尔", TokenType::Kw_Bool}, {"字符", TokenType::Kw_Char},
        {"字符串", TokenType::Kw_String}, {"空类型", TokenType::Kw_Void},
        {"结构体", TokenType::Kw_Struct}, {"联合体", TokenType::Kw_Union},
        {"枚举", TokenType::Kw_Enum},
        // ---- 声明(9) ----
        {"函数", TokenType::Kw_Function}, {"变量", TokenType::Kw_Var},
        {"导入", TokenType::Kw_Import},
        {"公开", TokenType::Kw_Public}, {"私有", TokenType::Kw_Private},
        {"静态", TokenType::Kw_Static}, {"自动", TokenType::Kw_Auto},
        // ---- 模块系统关键字(4，v2.0 新增) ----
        {"模块", TokenType::Kw_Module}, {"作为", TokenType::Kw_As},
        // ---- 常量(4) ----
        {"真", TokenType::Kw_True}, {"假", TokenType::Kw_False},
        {"无", TokenType::Kw_None}, {"常量", TokenType::Kw_Const},
        // ---- 安全区边界(1，plans/019 阶段4) ----
        {"不安全", TokenType::Kw_Unsafe},
        // ---- OOP(10) ----
        {"类", TokenType::Kw_Class}, {"接口", TokenType::Kw_Interface},
        {"保护", TokenType::Kw_Protected}, {"虚拟", TokenType::Kw_Virtual},
        {"重写", TokenType::Kw_Override}, {"抽象", TokenType::Kw_Abstract},
        {"实现", TokenType::Kw_Implements}, {"自身", TokenType::Kw_Self},
        {"父类", TokenType::Kw_Super}, {"友元", TokenType::Kw_Friend},
        // ---- 错误处理(2) ----
        {"结果", TokenType::Kw_Result}, {"可选", TokenType::Kw_Optional},
        // ---- 字面量前缀(2) ----
        {"原始", TokenType::Kw_Raw}, {"多行", TokenType::Kw_MultiLine},
        // ---- 泛型(1) ----
        {"泛型", TokenType::Kw_Generic},
    };
    return kTable;
}

} // namespace

// 查找关键字：命中返回true并设置类型
bool Lexer::lookupKeyword(const std::string& text, TokenType& type) {
    const auto& table = keywordTable();
    const auto it = table.find(text);
    if (it != table.end()) {
        type = it->second;
        return true;
    }
    return false;
}

// 报告一条词法错误到诊断引擎
void Lexer::reportError(const SourceLocation& loc, const std::string& message) {
    diagnostics_.report(DiagnosticLevel::Error, loc, message);
}

// 是否到达源码末尾
bool Lexer::isAtEnd() const { return pos_ >= source_.size(); }

// 前进一个UTF-8字符，返回其Unicode码点（\n换行时更新行号/列号）
char32_t Lexer::advance() {
    if (isAtEnd()) return 0;
    int len = 1;
    char32_t cp = decodeUtf8(source_, pos_, len);
    pos_ += static_cast<std::size_t>(len);
    if (cp == U'\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return cp;
}

// 查看当前字符（不前进）
char32_t Lexer::peek() const {
    if (isAtEnd()) return 0;
    int len = 0;
    return decodeUtf8(source_, pos_, len);
}

// 查看向后第n个字符（不前进，n>=1；越界返回0）
// 注意：必须跳过当前位置再前进n个，peekNext(1) 返回下一个字符
char32_t Lexer::peekNext(int n) const {
    std::size_t p = pos_;
    char32_t cp = 0;
    // 共前进 n+1 个字符：第1次跳过当前位置，后n次定位到目标字符
    for (int i = 0; i <= n; i++) {
        if (p >= source_.size()) return 0;
        int len = 1;
        cp = decodeUtf8(source_, p, len);
        p += static_cast<std::size_t>(len);
    }
    return cp;
}

// 返回当前位置字符的UTF-8字节串（不前进；越界返回空串）
std::string Lexer::currentUtf8() const {
    if (isAtEnd()) return "";
    int len = 1;
    decodeUtf8(source_, pos_, len);
    return source_.substr(pos_, static_cast<std::size_t>(len));
}

// 当前位置（文件名/行/列）
SourceLocation Lexer::currentLocation() const {
    return SourceLocation(fileName_, line_, column_);
}

// 跳过空白与注释：//单行注释、/* */块注释（支持嵌套），循环直到无空白/注释
void Lexer::skipWhitespaceAndComments() {
    bool progressed = true;
    while (progressed) {
        progressed = false;
        // 空白（换行由 advance 更新行号）
        while (!isAtEnd()) {
            char32_t c = peek();
            if (c == U' ' || c == U'\t' || c == U'\r' || c == U'\n') {
                advance();
                progressed = true;
            } else {
                break;
            }
        }
        // 单行注释 // 到行尾
        if (peek() == U'/' && peekNext(1) == U'/') {
            advance();
            advance();
            while (!isAtEnd() && peek() != U'\n') advance();
            progressed = true;
        }
        // 块注释 /* */（支持嵌套）
        if (peek() == U'/' && peekNext(1) == U'*') {
            SourceLocation loc = currentLocation();
            advance();
            advance();
            int depth = 1;
            while (!isAtEnd() && depth > 0) {
                if (peek() == U'/' && peekNext(1) == U'*') {
                    advance();
                    advance();
                    depth++;
                } else if (peek() == U'*' && peekNext(1) == U'/') {
                    advance();
                    advance();
                    depth--;
                } else {
                    advance();
                }
            }
            if (depth > 0) reportError(loc, "未闭合的块注释");
            progressed = true;
        }
    }
}

// 读取标识符或关键字（UTF-8中文/ASCII字母/数字/下划线连续收集，最长匹配）
// 特殊处理：原始/多行前缀后紧跟引号时转入带前缀字符串读取
Token Lexer::readIdentifierOrKeyword() {
    SourceLocation loc = currentLocation();
    std::string text;
    while (!isAtEnd()) {
        char32_t c = peek();
        const bool isIdentifierChar = isAsciiAlpha(c) || isAsciiDigit(c) ||
                                      c == U'_' || c >= 0x80;
        if (!isIdentifierChar) break;
        text += currentUtf8();
        advance();
    }
    TokenType type;
    if (lookupKeyword(text, type)) {
        // 原始/多行字符串前缀：关键字后紧跟引号才是字符串字面量
        if ((type == TokenType::Kw_Raw || type == TokenType::Kw_MultiLine) &&
            peek() == U'"') {
            return readPrefixedString(text, loc);
        }
        return Token(type, text, loc);
    }
    // 组合前缀：原始多行 / 多行原始（连续无空白）后紧跟引号 → 字符串字面量
    if (peek() == U'"' && (text == "原始多行" || text == "多行原始")) {
        return readPrefixedString(text, loc);
    }
    return Token(TokenType::Identifier, text, loc);
}

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
                // 转义序列：原样收集两字符（\ + 转义字符）
                text += currentUtf8();
                advance();
                if (isAtEnd()) break;
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

// 收集整数/浮点后缀：f/L/LL/U/UL/ULL（规格书4.3节）
// 注意：不在数字收集阶段处理a-f，避免与十六进制数字冲突
void Lexer::collectNumberSuffix(std::string& text) {
    while (!isAtEnd()) {
        const char32_t c = peek();
        if (c == U'f' || c == U'F' || c == U'l' || c == U'L' ||
            c == U'u' || c == U'U') {
            text += currentUtf8();
            advance();
        } else {
            break;
        }
    }
}

// 读取数字字面量：十进制/十六进制(0x)/二进制(0b)/八进制(0o)/浮点（小数与科学计数法）
Token Lexer::readNumber() {
    SourceLocation loc = currentLocation();
    std::string text;
    bool isFloat = false;

    // ---- 前缀进制 ----
    if (peek() == U'0' && (peekNext(1) == U'x' || peekNext(1) == U'X')) {
        text += currentUtf8();
        advance();
        text += currentUtf8();
        advance();
        while (!isAtEnd()) {
            const char32_t c = peek();
            if (isAsciiDigit(c) || (c >= U'a' && c <= U'f') || (c >= U'A' && c <= U'F')) {
                text += currentUtf8();
                advance();
            } else {
                break;
            }
        }
        collectNumberSuffix(text);
        return Token(TokenType::IntegerLiteral, text, loc);
    }
    if (peek() == U'0' && (peekNext(1) == U'b' || peekNext(1) == U'B')) {
        text += currentUtf8();
        advance();
        text += currentUtf8();
        advance();
        while (!isAtEnd()) {
            const char32_t c = peek();
            if (c == U'0' || c == U'1') {
                text += currentUtf8();
                advance();
            } else {
                break;
            }
        }
        collectNumberSuffix(text);
        return Token(TokenType::IntegerLiteral, text, loc);
    }
    if (peek() == U'0' && (peekNext(1) == U'o' || peekNext(1) == U'O')) {
        text += currentUtf8();
        advance();
        text += currentUtf8();
        advance();
        while (!isAtEnd()) {
            const char32_t c = peek();
            if (c >= U'0' && c <= U'7') {
                text += currentUtf8();
                advance();
            } else {
                break;
            }
        }
        collectNumberSuffix(text);
        return Token(TokenType::IntegerLiteral, text, loc);
    }

    // ---- 十进制整数部分 ----
    while (!isAtEnd()) {
        const char32_t c = peek();
        if (isAsciiDigit(c)) {
            text += currentUtf8();
            advance();
        } else {
            break;
        }
    }
    // ---- 浮点：小数点后必须跟数字 ----
    if (peek() == U'.' && isAsciiDigit(peekNext(1))) {
        isFloat = true;
        text += currentUtf8();
        advance(); // 消费 .
        while (!isAtEnd() && isAsciiDigit(peek())) {
            text += currentUtf8();
            advance();
        }
    }
    // ---- 浮点：科学计数法 e/E 后跟数字或符号 ----
    if (peek() == U'e' || peek() == U'E') {
        const char32_t next = peekNext(1);
        if (isAsciiDigit(next) || next == U'+' || next == U'-') {
            isFloat = true;
            text += currentUtf8();
            advance(); // 消费 e/E
            if (peek() == U'+' || peek() == U'-') {
                text += currentUtf8();
                advance();
            }
            while (!isAtEnd() && isAsciiDigit(peek())) {
                text += currentUtf8();
                advance();
            }
        }
    }
    collectNumberSuffix(text);
    return Token(isFloat ? TokenType::FloatLiteral : TokenType::IntegerLiteral, text, loc);
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
            // Unicode转义 \u{XXXX}
            text += currentUtf8();
            advance(); // 消费 u
            if (peek() == U'{') {
                text += currentUtf8();
                advance(); // 消费 {
            }
            while (!isAtEnd() && peek() != U'}') {
                text += currentUtf8();
                advance();
            }
            if (peek() == U'}') {
                text += currentUtf8();
                advance();
            } else {
                reportError(loc, "无效的Unicode转义：缺少 }");
            }
        } else if (esc == U'n' || esc == U't' || esc == U'r' || esc == U'0' ||
                   esc == U'\\' || esc == U'\'' || esc == U'"') {
            text += currentUtf8();
            advance();
        } else {
            reportError(loc, "无效的转义序列");
            text += currentUtf8();
            advance();
        }
    } else {
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

// 读取运算符或分隔符：三字符(<<= >>=) > 双字符(++ -- += -= ...) > 单字符（贪婪最长匹配）
Token Lexer::readOperatorOrDelimiter() {
    SourceLocation loc = currentLocation();
    const char32_t c = peek();

    // ---- 三字符运算符：<<= >>= ----
    if ((c == U'<' || c == U'>') && peekNext(1) == c && peekNext(2) == U'=') {
        const bool isShiftLeft = (c == U'<');
        advance();
        advance();
        advance();
        return Token(isShiftLeft ? TokenType::LessLessEqual : TokenType::GreaterGreaterEqual,
                     isShiftLeft ? "<<=" : ">>=", loc);
    }

    // ---- 双字符运算符（贪婪匹配） ----
    switch (c) {
        case U'+':
            if (peekNext(1) == U'+') { advance(); advance(); return Token(TokenType::PlusPlus, "++", loc); }
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::PlusEqual, "+=", loc); }
            break;
        case U'-':
            if (peekNext(1) == U'-') { advance(); advance(); return Token(TokenType::MinusMinus, "--", loc); }
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::MinusEqual, "-=", loc); }
            if (peekNext(1) == U'>') { advance(); advance(); return Token(TokenType::Arrow, "->", loc); }
            break;
        case U'*':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::StarEqual, "*=", loc); }
            break;
        case U'/':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::SlashEqual, "/=", loc); }
            break;
        case U'%':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::PercentEqual, "%=", loc); }
            break;
        case U'=':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::EqualEqual, "==", loc); }
            break;
        case U'!':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::BangEqual, "!=", loc); }
            break;
        case U':':
            // 模块路径分隔符 ::（v2.0）：贪婪匹配优先于单字符 Colon
            if (peekNext(1) == U':') { advance(); advance(); return Token(TokenType::ColonColon, "::", loc); }
            break;
        case U'<':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::LessEqual, "<=", loc); }
            if (peekNext(1) == U'<') { advance(); advance(); return Token(TokenType::LessLess, "<<", loc); }
            break;
        case U'>':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::GreaterEqual, ">=", loc); }
            if (peekNext(1) == U'>') { advance(); advance(); return Token(TokenType::GreaterGreater, ">>", loc); }
            break;
        case U'&':
            if (peekNext(1) == U'&') { advance(); advance(); return Token(TokenType::AndAnd, "&&", loc); }
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::AmpEqual, "&=", loc); }
            break;
        case U'|':
            if (peekNext(1) == U'|') { advance(); advance(); return Token(TokenType::OrOr, "||", loc); }
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::PipeEqual, "|=", loc); }
            break;
        case U'^':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::CaretEqual, "^=", loc); }
            break;
        default:
            break;
    }

    // ---- 单字符运算符/分隔符 ----
    switch (c) {
        case U'+': advance(); return Token(TokenType::Plus, "+", loc);
        case U'-': advance(); return Token(TokenType::Minus, "-", loc);
        case U'*': advance(); return Token(TokenType::Star, "*", loc);
        case U'/': advance(); return Token(TokenType::Slash, "/", loc);
        case U'%': advance(); return Token(TokenType::Percent, "%", loc);
        case U'=': advance(); return Token(TokenType::Equal, "=", loc);
        case U'<': advance(); return Token(TokenType::Less, "<", loc);
        case U'>': advance(); return Token(TokenType::Greater, ">", loc);
        case U'!': advance(); return Token(TokenType::Bang, "!", loc);
        case U'&': advance(); return Token(TokenType::Amp, "&", loc);
        case U'|': advance(); return Token(TokenType::Pipe, "|", loc);
        case U'^': advance(); return Token(TokenType::Caret, "^", loc);
        case U'~': advance(); return Token(TokenType::Tilde, "~", loc);
        case U'(': advance(); return Token(TokenType::LeftParen, "(", loc);
        case U')': advance(); return Token(TokenType::RightParen, ")", loc);
        case U'{': advance(); return Token(TokenType::LeftBrace, "{", loc);
        case U'}': advance(); return Token(TokenType::RightBrace, "}", loc);
        case U'[': advance(); return Token(TokenType::LeftBracket, "[", loc);
        case U']': advance(); return Token(TokenType::RightBracket, "]", loc);
        case U';': advance(); return Token(TokenType::Semicolon, ";", loc);
        case U',': advance(); return Token(TokenType::Comma, ",", loc);
        case U'.': advance(); return Token(TokenType::Dot, ".", loc);
        case U':': advance(); return Token(TokenType::Colon, ":", loc);  // 单冒号（:: 已在双字符分支处理）
        case U'?': advance(); return Token(TokenType::Question, "?", loc);
        default: {
            // 未知字符：报告诊断并跳过（错误恢复）
            const std::string bad = currentUtf8();
            reportError(loc, "非法字符: " + bad);
            advance();
            return Token(TokenType::Unknown, bad, loc);
        }
    }
}

// 主入口：分析源码返回Token流（末尾含文件结束Token）
// Task 6.6 条件编译：先经 Preprocessor 裁剪（保留行号），再切分 Token。
//   裁剪后的非激活行替换为空白，行号/列号与原始源码一致，诊断定位不偏移。
std::vector<Token> Lexer::tokenize() {
    // 预处理：条件编译指令裁剪（#定义/#如果定义/#否则/#结束如果）
    Preprocessor preprocessor(source_, fileName_, diagnostics_, macros_);
    source_ = preprocessor.process();
    pos_ = 0;
    line_ = 1;
    column_ = 1;

    std::vector<Token> tokens;
    while (true) {
        skipWhitespaceAndComments();
        if (isAtEnd()) {
            tokens.push_back(Token(TokenType::EndOfFile, "", currentLocation()));
            break;
        }
        const char32_t c = peek();
        // 标识符/关键字（含中文）
        if (isAsciiAlpha(c) || c == U'_' || c >= 0x80) {
            tokens.push_back(readIdentifierOrKeyword());
            continue;
        }
        // 数字字面量
        if (isAsciiDigit(c)) {
            tokens.push_back(readNumber());
            continue;
        }
        // 字符串字面量
        if (c == U'"') {
            SourceLocation loc = currentLocation();
            tokens.push_back(Token(TokenType::StringLiteral, readStringBody(false, false), loc));
            continue;
        }
        // 字符字面量
        if (c == U'\'') {
            tokens.push_back(readChar());
            continue;
        }
        // 运算符/分隔符/未知字符
        tokens.push_back(readOperatorOrDelimiter());
    }
    return tokens;
}

} // namespace cn_compiler
