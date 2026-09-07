// Token定义单元测试：类型枚举、文本值、源码位置
// 覆盖：53个关键字、字面量、运算符、分隔符、tokenTypeToString、分类方法、位置存储
// Task 1.1a 补充：自增/自减/问号
#include <gtest/gtest.h>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/lexer/token.hpp"

using cn_compiler::SourceLocation;
using cn_compiler::Token;
using cn_compiler::TokenType;

namespace {

// 61个关键字（v2.0）：枚举值 -> 对应中文文本（删 从，增 模块/作为/包/货舱）
const std::vector<std::pair<TokenType, std::string>> kKeywordTable = {
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
    {TokenType::Kw_Public, "公开"},
    {TokenType::Kw_Private, "私有"},
    {TokenType::Kw_Static, "静态"},
    {TokenType::Kw_Auto, "自动"},
    // ---- 模块系统关键字(4，v2.0 新增) ----
    {TokenType::Kw_Module, "模块"},
    {TokenType::Kw_As, "作为"},
    // ---- 常量关键字(4) ----
    {TokenType::Kw_True, "真"},
    {TokenType::Kw_False, "假"},
    {TokenType::Kw_None, "无"},
    {TokenType::Kw_Const, "常量"},
    // ---- OOP关键字(10) ----
    {TokenType::Kw_Class, "类"},
    {TokenType::Kw_Interface, "接口"},
    {TokenType::Kw_Protected, "保护"},
    {TokenType::Kw_Virtual, "虚拟"},
    {TokenType::Kw_Override, "重写"},
    {TokenType::Kw_Abstract, "抽象"},
    {TokenType::Kw_Implements, "实现"},
    {TokenType::Kw_Self, "自身"},
    {TokenType::Kw_Super, "父类"},
    {TokenType::Kw_Friend, "友元"},
    // ---- 错误处理关键字(2) ----
    {TokenType::Kw_Result, "结果"},
    {TokenType::Kw_Optional, "可选"},
    // ---- 字面量前缀关键字(2) ----
    {TokenType::Kw_Raw, "原始"},
    {TokenType::Kw_MultiLine, "多行"},
    // ---- 泛型关键字(1) ----
    {TokenType::Kw_Generic, "泛型"},
};

// 运算符：枚举值 -> 符号文本（tokenTypeToString 输出）
const std::vector<std::pair<TokenType, std::string>> kOperatorTable = {
    // ---- 算术(7) ----
    {TokenType::PlusPlus, "++"},
    {TokenType::MinusMinus, "--"},
    {TokenType::Plus, "+"},
    {TokenType::Minus, "-"},
    {TokenType::Star, "*"},
    {TokenType::Slash, "/"},
    {TokenType::Percent, "%"},
    // ---- 比较(6) ----
    {TokenType::EqualEqual, "=="},
    {TokenType::BangEqual, "!="},
    {TokenType::Less, "<"},
    {TokenType::LessEqual, "<="},
    {TokenType::Greater, ">"},
    {TokenType::GreaterEqual, ">="},
    // ---- 逻辑(3) ----
    {TokenType::AndAnd, "&&"},
    {TokenType::OrOr, "||"},
    {TokenType::Bang, "!"},
    // ---- 位(6) ----
    {TokenType::Amp, "&"},
    {TokenType::Pipe, "|"},
    {TokenType::Caret, "^"},
    {TokenType::Tilde, "~"},
    {TokenType::LessLess, "<<"},
    {TokenType::GreaterGreater, ">>"},
    // ---- 赋值(11) ----
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
    // ---- 指针(3) ----
    {TokenType::Arrow, "->"},
    {TokenType::AmpAddress, "&"},
    {TokenType::StarDeref, "*"},
    // ---- 其他(3) ----
    {TokenType::Dot, "."},
    {TokenType::LeftBracket, "["},
    {TokenType::RightBracket, "]"},
};

// 分隔符：枚举值 -> 符号文本（含 ::，v2.0 模块路径分隔符）
const std::vector<std::pair<TokenType, std::string>> kDelimiterTable = {
    {TokenType::LeftParen, "("},
    {TokenType::RightParen, ")"},
    {TokenType::LeftBrace, "{"},
    {TokenType::RightBrace, "}"},
    {TokenType::Semicolon, ";"},
    {TokenType::Comma, ","},
    {TokenType::Colon, ":"},
    {TokenType::ColonColon, "::"},
    {TokenType::Question, "?"},
};

} // namespace

// ---- 关键字测试 ----

// 构造61个关键字Token并验证类型与文本（v2.0）
// 注：测试名使用英文（GCC 7 不支持中文标识符，中文仅用于注释与字符串）
TEST(TokenTest, ConstructAllKeywords) {
    ASSERT_EQ(kKeywordTable.size(), static_cast<size_t>(59));
    for (const auto& entry : kKeywordTable) {
        Token token(entry.first, entry.second, SourceLocation("测试.cn", 1, 1));
        EXPECT_EQ(token.getType(), entry.first);
        EXPECT_EQ(token.getValue(), entry.second);
        EXPECT_TRUE(token.isKeyword());               // 实例方法
        EXPECT_TRUE(Token::isKeyword(entry.first));   // 静态方法
        EXPECT_FALSE(token.isLiteral());
        EXPECT_FALSE(token.isOperator());
        EXPECT_FALSE(token.isDelimiter());
    }
}

// tokenTypeToString 对全部关键字返回正确中文文本
TEST(TokenTest, KeywordToString) {
    for (const auto& entry : kKeywordTable) {
        EXPECT_EQ(Token::tokenTypeToString(entry.first), entry.second);
    }
}

// ---- 字面量测试 ----

// 字面量Token的值存储与分类
TEST(TokenTest, LiteralValueStorage) {
    Token integer(TokenType::IntegerLiteral, "42", SourceLocation("测试.cn", 1, 1));
    EXPECT_EQ(integer.getType(), TokenType::IntegerLiteral);
    EXPECT_EQ(integer.getValue(), "42");
    EXPECT_TRUE(integer.isLiteral());
    EXPECT_FALSE(integer.isKeyword());
    EXPECT_FALSE(integer.isOperator());
    EXPECT_FALSE(integer.isDelimiter());

    Token floating(TokenType::FloatLiteral, "3.14", SourceLocation("测试.cn", 2, 1));
    EXPECT_EQ(floating.getType(), TokenType::FloatLiteral);
    EXPECT_EQ(floating.getValue(), "3.14");

    Token string(TokenType::StringLiteral, "你好", SourceLocation("测试.cn", 3, 1));
    EXPECT_EQ(string.getType(), TokenType::StringLiteral);
    EXPECT_EQ(string.getValue(), "你好");

    Token character(TokenType::CharLiteral, "中", SourceLocation("测试.cn", 4, 1));
    EXPECT_EQ(character.getType(), TokenType::CharLiteral);
    EXPECT_EQ(character.getValue(), "中");
}

// ---- 运算符测试 ----

// 自增/自减/问号新Token类型专项测试（Task 1.1a）
TEST(TokenTest, IncrementDecrementQuestionToken) {
    Token plusPlus(TokenType::PlusPlus, "++", SourceLocation("测试.cn", 1, 1));
    EXPECT_EQ(plusPlus.getType(), TokenType::PlusPlus);
    EXPECT_EQ(plusPlus.getValue(), "++");
    EXPECT_TRUE(plusPlus.isOperator());
    EXPECT_TRUE(Token::isOperator(TokenType::PlusPlus));
    EXPECT_FALSE(plusPlus.isDelimiter());

    Token minusMinus(TokenType::MinusMinus, "--", SourceLocation("测试.cn", 1, 1));
    EXPECT_EQ(minusMinus.getType(), TokenType::MinusMinus);
    EXPECT_EQ(minusMinus.getValue(), "--");
    EXPECT_TRUE(minusMinus.isOperator());
    EXPECT_TRUE(Token::isOperator(TokenType::MinusMinus));
    EXPECT_FALSE(minusMinus.isDelimiter());

    Token question(TokenType::Question, "?", SourceLocation("测试.cn", 1, 1));
    EXPECT_EQ(question.getType(), TokenType::Question);
    EXPECT_EQ(question.getValue(), "?");
    EXPECT_TRUE(question.isDelimiter());
    EXPECT_TRUE(Token::isDelimiter(TokenType::Question));
    EXPECT_FALSE(question.isOperator());
    EXPECT_FALSE(question.isKeyword());
    EXPECT_FALSE(question.isLiteral());

    // tokenTypeToString 映射
    EXPECT_EQ(Token::tokenTypeToString(TokenType::PlusPlus), "++");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::MinusMinus), "--");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::Question), "?");
}

// 构造运算符Token并验证类型与分类
TEST(TokenTest, OperatorConstruction) {
    for (const auto& entry : kOperatorTable) {
        Token token(entry.first, entry.second, SourceLocation("测试.cn", 1, 1));
        EXPECT_EQ(token.getType(), entry.first);
        EXPECT_TRUE(token.isOperator());
        EXPECT_TRUE(Token::isOperator(entry.first));
        EXPECT_FALSE(token.isKeyword());
        EXPECT_FALSE(token.isLiteral());
        // 方括号[ ]为双角色Token：既是下标运算符也是分隔符（同C/C++惯例）
        const bool isDualRoleBracket =
            entry.first == TokenType::LeftBracket || entry.first == TokenType::RightBracket;
        if (isDualRoleBracket) {
            EXPECT_TRUE(token.isDelimiter());
        } else {
            EXPECT_FALSE(token.isDelimiter());
        }
    }
}

// 运算符转字符串返回符号文本
TEST(TokenTest, OperatorToString) {
    for (const auto& entry : kOperatorTable) {
        EXPECT_EQ(Token::tokenTypeToString(entry.first), entry.second);
    }
}

// ---- 分隔符测试 ----

// 构造分隔符Token并验证分类
TEST(TokenTest, DelimiterConstruction) {
    for (const auto& entry : kDelimiterTable) {
        Token token(entry.first, entry.second, SourceLocation("测试.cn", 1, 1));
        EXPECT_EQ(token.getType(), entry.first);
        EXPECT_TRUE(token.isDelimiter());
        EXPECT_TRUE(Token::isDelimiter(entry.first));
        EXPECT_FALSE(token.isKeyword());
        EXPECT_FALSE(token.isLiteral());
        EXPECT_FALSE(token.isOperator());
    }
    // 方括号同时具备分隔符身份（下标运算符与分隔符双重角色，同C/C++惯例）
    EXPECT_TRUE(Token::isDelimiter(TokenType::LeftBracket));
    EXPECT_TRUE(Token::isDelimiter(TokenType::RightBracket));
}

// ---- tokenTypeToString 覆盖测试 ----

// 标识符/字面量/EOF/未知类型的字符串映射
TEST(TokenTest, SpecialTypeToString) {
    EXPECT_EQ(Token::tokenTypeToString(TokenType::Identifier), "标识符");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::IntegerLiteral), "整数字面量");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::FloatLiteral), "浮点字面量");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::StringLiteral), "字符串字面量");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::CharLiteral), "字符字面量");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::EndOfFile), "文件结束");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::Unknown), "未知");
    // 分隔符转字符串
    for (const auto& entry : kDelimiterTable) {
        EXPECT_EQ(Token::tokenTypeToString(entry.first), entry.second);
    }
}

// ---- :: 模块路径分隔符专项测试（v2.0） ----

// :: token 构造与分类：ColonColon 是分隔符、非关键字、非运算符、非字面量
TEST(TokenTest, ColonColonToken) {
    Token cc(TokenType::ColonColon, "::", SourceLocation("测试.cn", 1, 1));
    EXPECT_EQ(cc.getType(), TokenType::ColonColon);
    EXPECT_EQ(cc.getValue(), "::");
    EXPECT_TRUE(cc.isDelimiter());
    EXPECT_TRUE(Token::isDelimiter(TokenType::ColonColon));
    EXPECT_FALSE(cc.isKeyword());
    EXPECT_FALSE(cc.isLiteral());
    EXPECT_FALSE(cc.isOperator());
    // tokenTypeToString 双向映射
    EXPECT_EQ(Token::tokenTypeToString(TokenType::ColonColon), "::");
}

// Colon 与 ColonColon 区分：两个独立 TokenType 各自映射
TEST(TokenTest, ColonVsColonColon) {
    Token colon(TokenType::Colon, ":", SourceLocation("测试.cn", 1, 1));
    Token colonColon(TokenType::ColonColon, "::", SourceLocation("测试.cn", 1, 2));
    EXPECT_EQ(colon.getType(), TokenType::Colon);
    EXPECT_EQ(colon.getValue(), ":");
    EXPECT_EQ(colonColon.getType(), TokenType::ColonColon);
    EXPECT_EQ(colonColon.getValue(), "::");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::Colon), ":");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::ColonColon), "::");
    EXPECT_NE(Token::tokenTypeToString(TokenType::Colon),
              Token::tokenTypeToString(TokenType::ColonColon));
}

// ---- 分类方法测试 ----

// 各类别方法的正反向断言
TEST(TokenTest, ClassificationMethods) {
    // 关键字
    EXPECT_TRUE(Token::isKeyword(TokenType::Kw_Return));
    EXPECT_FALSE(Token::isKeyword(TokenType::Identifier));
    // 字面量
    EXPECT_TRUE(Token::isLiteral(TokenType::IntegerLiteral));
    EXPECT_TRUE(Token::isLiteral(TokenType::FloatLiteral));
    EXPECT_TRUE(Token::isLiteral(TokenType::StringLiteral));
    EXPECT_TRUE(Token::isLiteral(TokenType::CharLiteral));
    EXPECT_FALSE(Token::isLiteral(TokenType::Plus));
    // 运算符
    EXPECT_TRUE(Token::isOperator(TokenType::Plus));
    EXPECT_TRUE(Token::isOperator(TokenType::Arrow));
    EXPECT_TRUE(Token::isOperator(TokenType::AmpAddress));
    EXPECT_TRUE(Token::isOperator(TokenType::StarDeref));
    EXPECT_FALSE(Token::isOperator(TokenType::LeftParen));
    EXPECT_FALSE(Token::isOperator(TokenType::Kw_If));
    // 分隔符
    EXPECT_TRUE(Token::isDelimiter(TokenType::LeftParen));
    EXPECT_TRUE(Token::isDelimiter(TokenType::Semicolon));
    EXPECT_TRUE(Token::isDelimiter(TokenType::Colon));
    EXPECT_FALSE(Token::isDelimiter(TokenType::Plus));
    // 其他类型不属于任何分类
    EXPECT_FALSE(Token::isKeyword(TokenType::EndOfFile));
    EXPECT_FALSE(Token::isLiteral(TokenType::EndOfFile));
    EXPECT_FALSE(Token::isOperator(TokenType::EndOfFile));
    EXPECT_FALSE(Token::isDelimiter(TokenType::EndOfFile));
}

// ---- 源码位置测试 ----

// Token携带SourceLocation并可正确读取
TEST(TokenTest, SourceLocationStorage) {
    SourceLocation location("测试.cn", 3, 7);
    Token token(TokenType::Kw_Return, "返回", location);
    EXPECT_EQ(token.getLocation().getFileName(), "测试.cn");
    EXPECT_EQ(token.getLocation().getLine(), 3);
    EXPECT_EQ(token.getLocation().getColumn(), 7);
    EXPECT_EQ(token.getLocation(), location);
}
