// Token实现：TokenType转字符串映射与分类方法实现（Task 1.1）
#include <unordered_map>

#include "cn_compiler/lexer/token.hpp"

namespace cn_compiler {

namespace {

// TokenType -> 字符串的静态映射表（调试输出/错误报告用）
const std::unordered_map<TokenType, std::string>& tokenTypeToStringMap() {
    static const std::unordered_map<TokenType, std::string> kMap = {
        // ---- 控制流关键字(10) ----
        {TokenType::关键字如果, "如果"},
        {TokenType::关键字否则, "否则"},
        {TokenType::关键字当, "当"},
        {TokenType::关键字循环, "循环"},
        {TokenType::关键字返回, "返回"},
        {TokenType::关键字中断, "中断"},
        {TokenType::关键字继续, "继续"},
        {TokenType::关键字选择, "选择"},
        {TokenType::关键字情况, "情况"},
        {TokenType::关键字默认, "默认"},
        // ---- 类型关键字(20) ----
        {TokenType::关键字整数, "整数"},
        {TokenType::关键字小数, "小数"},
        {TokenType::关键字整8, "整8"},
        {TokenType::关键字整16, "整16"},
        {TokenType::关键字整32, "整32"},
        {TokenType::关键字整64, "整64"},
        {TokenType::关键字整128, "整128"},
        {TokenType::关键字正8, "正8"},
        {TokenType::关键字正16, "正16"},
        {TokenType::关键字正32, "正32"},
        {TokenType::关键字正64, "正64"},
        {TokenType::关键字正128, "正128"},
        {TokenType::关键字浮32, "浮32"},
        {TokenType::关键字浮64, "浮64"},
        {TokenType::关键字布尔, "布尔"},
        {TokenType::关键字字符, "字符"},
        {TokenType::关键字字符串, "字符串"},
        {TokenType::关键字空类型, "空类型"},
        {TokenType::关键字结构体, "结构体"},
        {TokenType::关键字枚举, "枚举"},
        // ---- 声明关键字(7) ----
        {TokenType::关键字函数, "函数"},
        {TokenType::关键字变量, "变量"},
        {TokenType::关键字导入, "导入"},
        {TokenType::关键字从, "从"},
        {TokenType::关键字公开, "公开"},
        {TokenType::关键字私有, "私有"},
        {TokenType::关键字静态, "静态"},
        // ---- 常量关键字(3) ----
        {TokenType::关键字真, "真"},
        {TokenType::关键字假, "假"},
        {TokenType::关键字无, "无"},
        // ---- OOP关键字(9) ----
        {TokenType::关键字类, "类"},
        {TokenType::关键字接口, "接口"},
        {TokenType::关键字保护, "保护"},
        {TokenType::关键字虚拟, "虚拟"},
        {TokenType::关键字重写, "重写"},
        {TokenType::关键字抽象, "抽象"},
        {TokenType::关键字实现, "实现"},
        {TokenType::关键字自身, "自身"},
        {TokenType::关键字父类, "父类"},
        // ---- 错误处理关键字(2) ----
        {TokenType::关键字结果, "结果"},
        {TokenType::关键字可选, "可选"},
        // ---- 字面量前缀关键字(2) ----
        {TokenType::关键字原始, "原始"},
        {TokenType::关键字多行, "多行"},
        // ---- 标识符与字面量 ----
        {TokenType::Identifier, "标识符"},
        {TokenType::IntegerLiteral, "整数字面量"},
        {TokenType::FloatLiteral, "浮点字面量"},
        {TokenType::StringLiteral, "字符串字面量"},
        {TokenType::CharLiteral, "字符字面量"},
        // ---- 算术运算符(5) ----
        {TokenType::Plus, "+"},
        {TokenType::Minus, "-"},
        {TokenType::Star, "*"},
        {TokenType::Slash, "/"},
        {TokenType::Percent, "%"},
        // ---- 比较运算符(6) ----
        {TokenType::EqualEqual, "=="},
        {TokenType::BangEqual, "!="},
        {TokenType::Less, "<"},
        {TokenType::LessEqual, "<="},
        {TokenType::Greater, ">"},
        {TokenType::GreaterEqual, ">="},
        // ---- 逻辑运算符(3) ----
        {TokenType::AndAnd, "&&"},
        {TokenType::OrOr, "||"},
        {TokenType::Bang, "!"},
        // ---- 位运算符(6) ----
        {TokenType::Amp, "&"},
        {TokenType::Pipe, "|"},
        {TokenType::Caret, "^"},
        {TokenType::Tilde, "~"},
        {TokenType::LessLess, "<<"},
        {TokenType::GreaterGreater, ">>"},
        // ---- 赋值运算符(11) ----
        {TokenType::Equal, "="},
        {TokenType::PlusEqual, "+="},
        {TokenType::MinusEqual, "-="},
        {TokenType::StarEqual, "*="},
        {TokenType::SlashEqual, "/="},
        {TokenType::PercentEqual, "%="},
        {TokenType::AmpEqual, "&="},
        {TokenType::PipeEqual, "|="},
        {TokenType::CaretEqual, "^="},
        {TokenType::LessLessEqual, "<<="},
        {TokenType::GreaterGreaterEqual, ">>="},
        // ---- 指针运算符(3) ----
        {TokenType::Arrow, "->"},
        {TokenType::AmpAddress, "&"},
        {TokenType::StarDeref, "*"},
        // ---- 其他运算符(3) ----
        {TokenType::Dot, "."},
        {TokenType::LeftBracket, "["},
        {TokenType::RightBracket, "]"},
        // ---- 分隔符(7) ----
        {TokenType::LeftParen, "("},
        {TokenType::RightParen, ")"},
        {TokenType::LeftBrace, "{"},
        {TokenType::RightBrace, "}"},
        {TokenType::Semicolon, ";"},
        {TokenType::Comma, ","},
        {TokenType::Colon, ":"},
        // ---- 其他 ----
        {TokenType::EndOfFile, "文件结束"},
        {TokenType::Unknown, "未知"},
    };
    return kMap;
}

} // namespace

// TokenType转字符串：优先查映射表，查不到返回"未知TokenType"
const std::string& Token::tokenTypeToString(TokenType type) {
    static const std::string kUnknown = "未知TokenType";
    const auto& map = tokenTypeToStringMap();
    const auto it = map.find(type);
    return it != map.end() ? it->second : kUnknown;
}

// 判断是否为关键字：枚举值落在关键字区间 [关键字如果, 关键字多行]
bool Token::isKeyword(TokenType type) {
    return type >= TokenType::关键字如果 && type <= TokenType::关键字多行;
}

// 判断是否为字面量：整数/浮点/字符串/字符四类
bool Token::isLiteral(TokenType type) {
    return type == TokenType::IntegerLiteral ||
           type == TokenType::FloatLiteral ||
           type == TokenType::StringLiteral ||
           type == TokenType::CharLiteral;
}

// 判断是否为运算符：从 Plus 到 RightBracket 的连续区间
// （含方括号[ ]：下标运算符；分隔符中的括号/分号/逗号/冒号不在此区间）
bool Token::isOperator(TokenType type) {
    return type >= TokenType::Plus && type <= TokenType::RightBracket;
}

// 判断是否为分隔符：括号/花括号/分号/逗号/冒号，以及双角色的方括号
bool Token::isDelimiter(TokenType type) {
    return type == TokenType::LeftParen || type == TokenType::RightParen ||
           type == TokenType::LeftBrace || type == TokenType::RightBrace ||
           type == TokenType::Semicolon || type == TokenType::Comma ||
           type == TokenType::Colon ||
           type == TokenType::LeftBracket || type == TokenType::RightBracket;
}

} // namespace cn_compiler
