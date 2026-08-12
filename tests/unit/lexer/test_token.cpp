// Token定义单元测试：类型枚举、文本值、源码位置
// 覆盖：53个关键字、字面量、运算符、分隔符、tokenTypeToString、分类方法、位置存储
#include <gtest/gtest.h>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/lexer/token.hpp"

using cn_compiler::SourceLocation;
using cn_compiler::Token;
using cn_compiler::TokenType;

namespace {

// 53个关键字：枚举值 -> 对应中文文本
const std::vector<std::pair<TokenType, std::string>> kKeywordTable = {
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
};

// 运算符：枚举值 -> 符号文本（tokenTypeToString 输出）
const std::vector<std::pair<TokenType, std::string>> kOperatorTable = {
    // ---- 算术(5) ----
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

// 分隔符：枚举值 -> 符号文本
const std::vector<std::pair<TokenType, std::string>> kDelimiterTable = {
    {TokenType::LeftParen, "("},
    {TokenType::RightParen, ")"},
    {TokenType::LeftBrace, "{"},
    {TokenType::RightBrace, "}"},
    {TokenType::Semicolon, ";"},
    {TokenType::Comma, ","},
    {TokenType::Colon, ":"},
};

} // namespace

// ---- 关键字测试 ----

// 构造53个关键字Token并验证类型与文本
TEST(TokenTest, 构造53个关键字) {
    ASSERT_EQ(kKeywordTable.size(), static_cast<size_t>(53));
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
TEST(TokenTest, 关键字转字符串) {
    for (const auto& entry : kKeywordTable) {
        EXPECT_EQ(Token::tokenTypeToString(entry.first), entry.second);
    }
}

// ---- 字面量测试 ----

// 字面量Token的值存储与分类
TEST(TokenTest, 字面量值存储) {
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

// 构造运算符Token并验证类型与分类
TEST(TokenTest, 运算符构造与分类) {
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
TEST(TokenTest, 运算符转字符串) {
    for (const auto& entry : kOperatorTable) {
        EXPECT_EQ(Token::tokenTypeToString(entry.first), entry.second);
    }
}

// ---- 分隔符测试 ----

// 构造分隔符Token并验证分类
TEST(TokenTest, 分隔符构造与分类) {
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
TEST(TokenTest, 特殊类型转字符串) {
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

// ---- 分类方法测试 ----

// 各类别方法的正反向断言
TEST(TokenTest, 分类方法) {
    // 关键字
    EXPECT_TRUE(Token::isKeyword(TokenType::关键字返回));
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
    EXPECT_FALSE(Token::isOperator(TokenType::关键字如果));
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
TEST(TokenTest, 源码位置存储) {
    SourceLocation location("测试.cn", 3, 7);
    Token token(TokenType::关键字返回, "返回", location);
    EXPECT_EQ(token.getLocation().getFileName(), "测试.cn");
    EXPECT_EQ(token.getLocation().getLine(), 3);
    EXPECT_EQ(token.getLocation().getColumn(), 7);
    EXPECT_EQ(token.getLocation(), location);
}
