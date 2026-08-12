// Token实现：TokenType转字符串映射与分类方法实现（Task 1.1，Task 1.1a 补充自增/自减/问号）
#include <unordered_map>

#include "cn_compiler/lexer/token.hpp"

namespace cn_compiler {

namespace {

// TokenType -> 中文文本的静态映射表（调试输出/错误报告用）
const std::unordered_map<TokenType, std::string>& tokenTypeToStringMap() {
    static const std::unordered_map<TokenType, std::string> kMap = {
        // ---- 控制流关键字(10) ----
        {TokenType::Kw_If, "如果"},
        {TokenType::Kw_Else, "否则"},
        {TokenType::Kw_While, "当"},
        {TokenType::Kw_For, "循环"},
        {TokenType::Kw_Return, "返回"},
        {TokenType::Kw_Break, "中断"},
        {TokenType::Kw_Continue, "继续"},
        {TokenType::Kw_Switch, "选择"},
        {TokenType::Kw_Case, "情况"},
        {TokenType::Kw_Default, "默认"},
        // ---- 类型关键字(21) ----
        {TokenType::Kw_Int, "整数"},
        {TokenType::Kw_Double, "小数"},
        {TokenType::Kw_Int8, "整8"},
        {TokenType::Kw_Int16, "整16"},
        {TokenType::Kw_Int32, "整32"},
        {TokenType::Kw_Int64, "整64"},
        {TokenType::Kw_Int128, "整128"},
        {TokenType::Kw_UInt8, "正8"},
        {TokenType::Kw_UInt16, "正16"},
        {TokenType::Kw_UInt32, "正32"},
        {TokenType::Kw_UInt64, "正64"},
        {TokenType::Kw_UInt128, "正128"},
        {TokenType::Kw_Float32, "浮32"},
        {TokenType::Kw_Float64, "浮64"},
        {TokenType::Kw_Bool, "布尔"},
        {TokenType::Kw_Char, "字符"},
        {TokenType::Kw_String, "字符串"},
        {TokenType::Kw_Void, "空类型"},
        {TokenType::Kw_Struct, "结构体"},
        {TokenType::Kw_Union, "联合体"},
        {TokenType::Kw_Enum, "枚举"},
        // ---- 声明关键字(7) ----
        {TokenType::Kw_Function, "函数"},
        {TokenType::Kw_Var, "变量"},
        {TokenType::Kw_Import, "导入"},
        {TokenType::Kw_From, "从"},
        {TokenType::Kw_Public, "公开"},
        {TokenType::Kw_Private, "私有"},
        {TokenType::Kw_Static, "静态"},
        // ---- 常量关键字(3) ----
        {TokenType::Kw_True, "真"},
        {TokenType::Kw_False, "假"},
        {TokenType::Kw_None, "无"},
        // ---- OOP关键字(9) ----
        {TokenType::Kw_Class, "类"},
        {TokenType::Kw_Interface, "接口"},
        {TokenType::Kw_Protected, "保护"},
        {TokenType::Kw_Virtual, "虚拟"},
        {TokenType::Kw_Override, "重写"},
        {TokenType::Kw_Abstract, "抽象"},
        {TokenType::Kw_Implements, "实现"},
        {TokenType::Kw_Self, "自身"},
        {TokenType::Kw_Super, "父类"},
        // ---- 错误处理关键字(2) ----
        {TokenType::Kw_Result, "结果"},
        {TokenType::Kw_Optional, "可选"},
        // ---- 字面量前缀关键字(2) ----
        {TokenType::Kw_Raw, "原始"},
        {TokenType::Kw_MultiLine, "多行"},
        // ---- 标识符与字面量 ----
        {TokenType::Identifier, "标识符"},
        {TokenType::IntegerLiteral, "整数字面量"},
        {TokenType::FloatLiteral, "浮点字面量"},
        {TokenType::StringLiteral, "字符串字面量"},
        {TokenType::CharLiteral, "字符字面量"},
        // ---- 算术运算符(7) ----
        {TokenType::PlusPlus, "++"},
        {TokenType::MinusMinus, "--"},
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
        // ---- 分隔符(8) ----
        {TokenType::LeftParen, "("},
        {TokenType::RightParen, ")"},
        {TokenType::LeftBrace, "{"},
        {TokenType::RightBrace, "}"},
        {TokenType::Semicolon, ";"},
        {TokenType::Comma, ","},
        {TokenType::Colon, ":"},
        {TokenType::Question, "?"},
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

// 判断是否为关键字：枚举值落在关键字区间 [Kw_If, Kw_MultiLine]
bool Token::isKeyword(TokenType type) {
    return type >= TokenType::Kw_If && type <= TokenType::Kw_MultiLine;
}

// 判断是否为字面量：整数/浮点/字符串/字符四类
bool Token::isLiteral(TokenType type) {
    return type == TokenType::IntegerLiteral ||
           type == TokenType::FloatLiteral ||
           type == TokenType::StringLiteral ||
           type == TokenType::CharLiteral;
}

// 判断是否为运算符：从 PlusPlus 到 RightBracket 的连续区间
// （含方括号[ ]：下标运算符；分隔符中的括号/分号/逗号/冒号/问号不在此区间）
bool Token::isOperator(TokenType type) {
    return type >= TokenType::PlusPlus && type <= TokenType::RightBracket;
}

// 判断是否为分隔符：括号/花括号/分号/逗号/冒号/问号，以及双角色的方括号
bool Token::isDelimiter(TokenType type) {
    return type == TokenType::LeftParen || type == TokenType::RightParen ||
           type == TokenType::LeftBrace || type == TokenType::RightBrace ||
           type == TokenType::Semicolon || type == TokenType::Comma ||
           type == TokenType::Colon || type == TokenType::Question ||
           type == TokenType::LeftBracket || type == TokenType::RightBracket;
}

} // namespace cn_compiler
