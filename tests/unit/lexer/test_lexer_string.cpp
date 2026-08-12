// 词法分析器字符串字面量单元测试（Task 2.5）
// 覆盖：普通字符串、转义序列、原始字符串（不转义）、多行字符串、原始多行组合
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/lexer/token.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Token;
using cn_compiler::TokenType;

namespace {

// 辅助函数：分析源码并返回Token流（含EOF）
std::vector<Token> analyze(const std::string& source) {
    Diagnostics diagnostics;
    Lexer lexer(source, "测试.cn", diagnostics);
    return lexer.tokenize();
}

// 辅助函数：返回带诊断的token流（用于错误测试）
std::vector<Token> analyzeWithDiagnostics(const std::string& source,
                                          Diagnostics& diagnostics) {
    Lexer lexer(source, "测试.cn", diagnostics);
    return lexer.tokenize();
}

// 提取第 index 个字符串字面量 token 的原始文本
std::string stringTokenValue(const std::vector<Token>& tokens, std::size_t index) {
    std::size_t found = 0;
    for (const auto& token : tokens) {
        if (token.getType() == TokenType::StringLiteral) {
            if (found == index) return token.getValue();
            ++found;
        }
    }
    return "";
}

} // namespace

// ==================== 1. 普通字符串 ====================

// 普通字符串：保留完整引号文本（供IR层解码）
TEST(LexerStringTest, NormalString) {
    auto tokens = analyze("\"你好\"");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].getValue(), "\"你好\"");
}

// 普通字符串：转义序列原样保留（\n \t \\ \"），IR层解码
TEST(LexerStringTest, NormalStringWithEscapes) {
    auto tokens = analyze("\"a\\nb\\tc\\\\d\\\"e\"");
    ASSERT_EQ(tokens[0].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].getValue(), "\"a\\nb\\tc\\\\d\\\"e\"");
}

// 普通字符串：Unicode转义 \u{XXXX} 保留原文
TEST(LexerStringTest, NormalStringUnicodeEscape) {
    auto tokens = analyze("\"\\u{4E2D}\"");
    ASSERT_EQ(tokens[0].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].getValue(), "\"\\u{4E2D}\"");
}

// 空字符串
TEST(LexerStringTest, EmptyString) {
    auto tokens = analyze("\"\"");
    ASSERT_EQ(tokens[0].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].getValue(), "\"\"");
}

// ==================== 2. 原始字符串 ====================

// 原始字符串：前缀保留在值中，反斜杠不转义（所见即所得）
TEST(LexerStringTest, RawString) {
    auto tokens = analyze("原始\"^\\d+\\.\\d+$\"");
    ASSERT_EQ(tokens[0].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].getValue(), "原始\"^\\d+\\.\\d+$\"");
}

// 原始字符串：Windows路径反斜杠不转义
TEST(LexerStringTest, RawStringPath) {
    auto tokens = analyze("原始\"C:\\dir\\file\"");
    ASSERT_EQ(tokens[0].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].getValue(), "原始\"C:\\dir\\file\"");
}

// ==================== 3. 多行字符串 ====================

// 多行字符串：三引号包裹，可跨行，前缀保留
TEST(LexerStringTest, MultiLineString) {
    auto tokens = analyze("多行\"\"\"第一行\n第二行\"\"\"");
    ASSERT_EQ(tokens[0].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].getValue(), "多行\"\"\"第一行\n第二行\"\"\"");
}

// 多行字符串：内部转义序列保留（\n \t），IR层解码
TEST(LexerStringTest, MultiLineStringEscapes) {
    auto tokens = analyze("多行\"\"\"a\\nb\\t\"\"\"");
    ASSERT_EQ(tokens[0].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].getValue(), "多行\"\"\"a\\nb\\t\"\"\"");
}

// ==================== 4. 原始多行组合 ====================

// 原始多行组合：反斜杠不转义 + 可跨行
TEST(LexerStringTest, RawMultiLineString) {
    auto tokens = analyze("原始多行\"\"\"A\\nB\nC\"\"\"");
    ASSERT_EQ(tokens[0].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].getValue(), "原始多行\"\"\"A\\nB\nC\"\"\"");
}

// 多行原始组合（顺序可交换）
TEST(LexerStringTest, MultiLineRawString) {
    auto tokens = analyze("多行原始\"\"\"X\\tY\"\"\"");
    ASSERT_EQ(tokens[0].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].getValue(), "多行原始\"\"\"X\\tY\"\"\"");
}

// ==================== 5. 错误处理 ====================

// 未闭合字符串：报错但tokenize不崩溃
TEST(LexerStringTest, UnterminatedString) {
    Diagnostics diagnostics;
    auto tokens = analyzeWithDiagnostics("\"未闭合", diagnostics);
    EXPECT_TRUE(diagnostics.hasErrors());
    EXPECT_FALSE(tokens.empty());
}

// 字符串不能跨行（普通/原始形式）
TEST(LexerStringTest, StringCannotSpanLines) {
    Diagnostics diagnostics;
    auto tokens = analyzeWithDiagnostics("\"第一行\n第二行\"", diagnostics);
    EXPECT_TRUE(diagnostics.hasErrors());
    EXPECT_FALSE(tokens.empty());
}

// ==================== 6. 组合场景 ====================

// 混合代码中多个字符串字面量
TEST(LexerStringTest, MixedStrings) {
    auto tokens = analyze("打印行(\"普通\") 打印行(原始\"原始\")");
    ASSERT_EQ(stringTokenValue(tokens, 0), "\"普通\"");
    ASSERT_EQ(stringTokenValue(tokens, 1), "原始\"原始\"");
}
