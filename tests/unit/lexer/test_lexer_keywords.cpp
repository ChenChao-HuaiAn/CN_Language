// 关键字词法单元测试（Task 2.7 新增"联合体"）
// 覆盖：联合体/结构体/枚举关键字识别为对应 TokenType；联合体作为保留字不可作标识符
// 测试方式：通过 Lexer 得到真实 Token 流（全链路，非Mock）
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

// 辅助：词法分析源码，返回Token流
std::vector<Token> lexSource(const std::string& source) {
    Diagnostics diagnostics;
    Lexer lexer(source, "关键字测试.cn", diagnostics);
    return lexer.tokenize();
}

} // namespace

// 联合体关键字识别（Task 2.7 新关键字）
TEST(LexerKeywordTest, UnionKeyword) {
    auto tokens = lexSource("联合体 数值 { 整32 值 }");
    ASSERT_GE(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Union);
    EXPECT_EQ(tokens[0].getValue(), "联合体");
    // 联合体名（标识符）
    EXPECT_EQ(tokens[1].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[1].getValue(), "数值");
}

// 结构体/枚举关键字识别（已有关键字回归）
// Token 序列：结构体(Kw_Struct) 点(Identifier) {(LeftBrace) 整32(Kw_Int32) x(Identifier)
//             }(RightBrace) 枚举(Kw_Enum) 颜色(Identifier) {(LeftBrace) 红(Identifier)
TEST(LexerKeywordTest, StructAndEnumKeywords) {
    auto tokens = lexSource("结构体 点 { 整32 x } 枚举 颜色 { 红 }");
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Struct);
    EXPECT_EQ(tokens[1].getType(), TokenType::Identifier);  // 点
    EXPECT_EQ(tokens[2].getType(), TokenType::LeftBrace);
    EXPECT_EQ(tokens[6].getType(), TokenType::Kw_Enum);
    EXPECT_EQ(tokens[7].getType(), TokenType::Identifier);  // 颜色
}

// 关键字计数：联合体加入后关键字区间仍连续（isKeyword 判断）
TEST(LexerKeywordTest, UnionIsKeyword) {
    auto tokens = lexSource("联合体");
    ASSERT_FALSE(tokens.empty());
    EXPECT_TRUE(tokens[0].isKeyword());
}

// 联合体保留字：不能作为普通标识符变量名（词法层识别为关键字）
TEST(LexerKeywordTest, UnionReserved) {
    auto tokens = lexSource("整32 联合体");
    // 第二个token应为关键字而非标识符
    ASSERT_GE(tokens.size(), 2u);
    EXPECT_EQ(tokens[1].getType(), TokenType::Kw_Union);
}

// 关键字映射表文本（tokenTypeToString）
TEST(LexerKeywordTest, UnionToString) {
    EXPECT_EQ(Token::tokenTypeToString(TokenType::Kw_Union), "联合体");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::Kw_Struct), "结构体");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::Kw_Enum), "枚举");
}

// ==================== v2.0 模块系统关键字 ====================

// 模块 关键字识别（v2.0 新增）：模块 网络
TEST(LexerKeywordTest, ModuleKeyword) {
    auto tokens = lexSource("模块 网络");
    ASSERT_GE(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Module);
    EXPECT_EQ(tokens[0].getValue(), "模块");
    EXPECT_TRUE(tokens[0].isKeyword());
    EXPECT_EQ(tokens[1].getType(), TokenType::Identifier);  // 网络
}

// 作为 关键字识别（v2.0 新增）：导入重命名
TEST(LexerKeywordTest, AsKeyword) {
    auto tokens = lexSource("作为 别名");
    ASSERT_GE(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_As);
    EXPECT_EQ(tokens[0].getValue(), "作为");
    EXPECT_EQ(tokens[1].getType(), TokenType::Identifier);
}

// 包 已摘除关键字（plans/018，2026-09-07 用户裁决方案A）：普通位置是标识符
//   （死保留字摘除——包/货舱 在 CN 源码语法中无语法位置，包.cn/货舱.toml 是
//   文件名约定；Rust 对照 cargo 非保留字；恢复标识符自由）
TEST(LexerKeywordTest, PackageRemovedIsIdentifier) {
    auto tokens = lexSource("包 名");
    ASSERT_GE(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[0].getValue(), "包");
    EXPECT_FALSE(tokens[0].isKeyword());
    EXPECT_EQ(tokens[1].getType(), TokenType::Identifier);
}

// 货舱 已摘除关键字（plans/018，2026-09-07 用户裁决方案A）：普通位置是标识符
TEST(LexerKeywordTest, CargoRemovedIsIdentifier) {
    auto tokens = lexSource("货舱");
    ASSERT_FALSE(tokens.empty());
    EXPECT_EQ(tokens[0].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[0].getValue(), "货舱");
    EXPECT_FALSE(tokens[0].isKeyword());
}

// 从 已删除关键字：普通位置是标识符（v2.0）
TEST(LexerKeywordTest, FromRemovedIsIdentifier) {
    auto tokens = lexSource("从");
    ASSERT_FALSE(tokens.empty());
    EXPECT_EQ(tokens[0].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[0].getValue(), "从");
    EXPECT_FALSE(tokens[0].isKeyword());
}

// 导入 仍是关键字（v2.0 保留）
TEST(LexerKeywordTest, ImportStillKeyword) {
    auto tokens = lexSource("导入 数学");
    ASSERT_GE(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Import);
    EXPECT_EQ(tokens[0].getValue(), "导入");
    EXPECT_TRUE(tokens[0].isKeyword());
}

// 模块系统关键字映射表文本（tokenTypeToString）
TEST(LexerKeywordTest, ModuleSystemKeywordToString) {
    EXPECT_EQ(Token::tokenTypeToString(TokenType::Kw_Module), "模块");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::Kw_As), "作为");
    // plans/018 摘除：Kw_Package/Kw_Cargo 枚举已删（包/货舱 死保留字）
}
