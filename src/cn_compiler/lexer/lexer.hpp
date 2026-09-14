// 词法分析器：UTF-8源码 -> Token流（Task 1.2）
// 关键设计：中文关键字最长匹配，关键字后必须有空格或符号分隔
#pragma once
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <unordered_set>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/common/source_location.hpp"
#include "cn_compiler/lexer/preprocessor.hpp"
#include "cn_compiler/lexer/token.hpp"

namespace cn_compiler {

// 词法分析器：将UTF-8编码的CN源码切分为Token流
// 覆盖：53个中文关键字、4种字面量（整/浮/字符串/字符，含原始/多行前缀）、
//       39个运算符（含++/--自增自减）、8个分隔符（含?）、注释（块注释嵌套）、错误诊断
class Lexer {
public:
    // 构造函数：绑定源码全文、源文件名、诊断引擎引用
    // 可选：命令行注入宏集合（-D 宏名，条件编译 #如果定义 判定用；Task 6.6）
    Lexer(const std::string& source, std::string fileName, Diagnostics& diagnostics,
          const std::unordered_set<std::string>& macros = {})
        : source_(source), fileName_(std::move(fileName)), diagnostics_(diagnostics),
          macros_(macros) {}

    // 主入口：分析源码返回Token流（末尾含文件结束Token）
    // 处理流程：预处理（条件编译裁剪，保留行号）-> 词法切分
    std::vector<Token> tokenize();

private:
    // ---- UTF-8与位置追踪 ----
    bool isAtEnd() const;                    // 是否到达源码末尾
    char32_t advance();                      // 前进一个UTF-8字符，返回其Unicode码点（更新行/列）
    char32_t peek() const;                   // 查看当前字符（不前进）
    char32_t peekNext(int n) const;          // 查看向后第n个字符（不前进，n>=1）
    std::string currentUtf8() const;         // 返回当前位置字符的UTF-8字节串（不前进）
    SourceLocation currentLocation() const;  // 当前行列位置

    // ---- 读取辅助 ----
    void skipWhitespaceAndComments();        // 跳过空白与注释（//单行、/* */块注释嵌套）
    Token readIdentifierOrKeyword();         // 读取标识符或关键字（含原始/多行前缀处理）
    Token readPrefixedString(const std::string& prefixText, const SourceLocation& loc); // 读取带前缀字符串
    std::string readStringBody(bool multiLine, bool raw); // 读取字符串体（返回含引号原文）
    Token readNumber();                      // 读取数字（十进制/十六进制/二进制/八进制/浮点）
    void collectNumberSuffix(std::string& text); // 收集整数/浮点后缀 f/L/LL/U/UL/ULL
    Token readChar();                        // 读取字符字面量（含转义与\u{}）
    Token readOperatorOrDelimiter();         // 读取运算符或分隔符（贪婪最长匹配）

    // ---- 工具 ----
    static bool lookupKeyword(const std::string& text, TokenType& type); // 查找46个保留字
    // 前缀误用探测（162-a 非保留化）：跳过空格/制表后紧跟引号（诊断用，不消费）
    bool prefixAbuseNextQuote() const;
    void reportError(const SourceLocation& loc, const std::string& message); // 报告词法错误

    std::string source_;       // 源码全文（预处理裁剪后）
    std::string fileName_;     // 源文件名
    std::size_t pos_ = 0;      // 当前字节偏移
    int line_ = 1;             // 当前行号（从1开始）
    int column_ = 1;           // 当前列号（从1开始，按字符计数）
    Diagnostics& diagnostics_; // 诊断引擎引用
    std::unordered_set<std::string> macros_; // 命令行注入宏（值拷贝，防默认参数临时对象悬垂）
};

} // namespace cn_compiler
