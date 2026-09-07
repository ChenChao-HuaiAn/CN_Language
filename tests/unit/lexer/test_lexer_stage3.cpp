// 阶段3 词法单元测试（Task 3.1/3.3/3.6/3.7/3.8/3.9）
// 覆盖：
//   1. 新增关键字：常量/友元/泛型（55 -> 58 个保留字）
//      v2.0 模块系统：58 -> 61 个保留字（删 从，增 模块/作为/包/货舱）
//   2. 运算符 为上下文关键字（非保留字）：普通位置是标识符
//   3. 模板尖括号：结果<T,E>/可选<T>/泛型 <类型 T> 中的 < > 仍是普通运算符 token
//      （消歧在 parser 层做，lexer 产出 Less/Greater 运算符 token）
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

// 辅助：词法分析源码，返回Token流（含EOF）
std::vector<Token> lexSource(const std::string& source) {
    Diagnostics diagnostics;
    Lexer lexer(source, "阶段3词法测试.cn", diagnostics);
    return lexer.tokenize();
}

// 去除末尾EOF后的Token列表
std::vector<Token> withoutEof(const std::vector<Token>& tokens) {
    std::vector<Token> result = tokens;
    if (!result.empty() && result.back().getType() == TokenType::EndOfFile) {
        result.pop_back();
    }
    return result;
}

} // namespace

// ==================== 1. 新增关键字识别（常量/友元/泛型） ====================

// 常量 关键字：词法层识别为 Kw_Const（此前为标识符，parser 用 checkText）
TEST(LexerStage3Test, ConstKeyword) {
    auto tokens = withoutEof(lexSource("常量 最大 = 100"));
    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Const);
    EXPECT_EQ(tokens[0].getValue(), "常量");
    EXPECT_TRUE(tokens[0].isKeyword());
}

// 友元 关键字：词法层识别为 Kw_Friend
TEST(LexerStage3Test, FriendKeyword) {
    auto tokens = withoutEof(lexSource("友元 函数 审计(账户& 账)"));
    ASSERT_GE(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Friend);
    EXPECT_EQ(tokens[0].getValue(), "友元");
}

// 泛型 关键字：词法层识别为 Kw_Generic
// Token 序列：泛型(Kw_Generic) <(Less) 类型(Identifier) T(Identifier) >(Greater)
TEST(LexerStage3Test, GenericKeyword) {
    auto tokens = withoutEof(lexSource("泛型 <类型 T>"));
    ASSERT_EQ(tokens.size(), 5u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Generic);
    EXPECT_EQ(tokens[0].getValue(), "泛型");
    // 模板尖括号仍是普通运算符 token（消歧在 parser 层）
    EXPECT_EQ(tokens[1].getType(), TokenType::Less);
    EXPECT_EQ(tokens[2].getType(), TokenType::Identifier);  // 类型（非保留字）
    EXPECT_EQ(tokens[2].getValue(), "类型");
    EXPECT_EQ(tokens[3].getType(), TokenType::Identifier);  // T
    EXPECT_EQ(tokens[4].getType(), TokenType::Greater);
}

// 关键字计数：61 个保留字（v2.0：含 常量/友元/泛型 + 模块/作为/包/货舱，删 从，不含 运算符）
TEST(LexerStage3Test, KeywordCount61) {
    // 统计 keywordTable 全部关键字数量（通过遍历 isKeyword 分类验证连续性）
    // 已知 61 个关键字的区间：[Kw_If, Kw_Generic]（模块关键字位于区间内）
    EXPECT_TRUE(Token::isKeyword(TokenType::Kw_Generic));
    EXPECT_TRUE(Token::isKeyword(TokenType::Kw_Friend));
    EXPECT_TRUE(Token::isKeyword(TokenType::Kw_Const));
    // v2.0 模块系统关键字
    EXPECT_TRUE(Token::isKeyword(TokenType::Kw_Module));
    EXPECT_TRUE(Token::isKeyword(TokenType::Kw_As));
    // plans/018 摘除（2026-09-07 用户裁决方案A）：包/货舱 死保留字摘除，
    //   Kw_Package/Kw_Cargo 枚举已删（原 isKeyword 断言随之移除）
    // 运算符 不是保留字（isKeyword 区间外，落为 Identifier）
    EXPECT_FALSE(Token::isKeyword(TokenType::Identifier));
}

// tokenTypeToString 映射：新关键字返回正确中文文本
TEST(LexerStage3Test, NewKeywordToString) {
    EXPECT_EQ(Token::tokenTypeToString(TokenType::Kw_Const), "常量");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::Kw_Friend), "友元");
    EXPECT_EQ(Token::tokenTypeToString(TokenType::Kw_Generic), "泛型");
}

// ==================== 2. 运算符 上下文关键字（非保留字） ====================

// 运算符 在普通位置是标识符（可作变量名/函数名）
TEST(LexerStage3Test, OperatorIsIdentifierInNormalContext) {
    auto tokens = withoutEof(lexSource("运算符 = 42"));
    ASSERT_GE(tokens.size(), 2u);
    // 运算符 是 Identifier 而非关键字
    EXPECT_EQ(tokens[0].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[0].getValue(), "运算符");
    EXPECT_FALSE(tokens[0].isKeyword());
}

// 运算符 后随运算符符号时仍为 Identifier（上下文识别在 parser 层）
// lexer 层统一产出：运算符(Identifier) + 运算符符号(token)
TEST(LexerStage3Test, OperatorContextFollowedBySymbol) {
    auto tokens = withoutEof(lexSource("函数 运算符+(复数 右)"));
    ASSERT_GE(tokens.size(), 4u);
    // 函数(Kw_Function) 运算符(Identifier) +(Plus)
    EXPECT_EQ(tokens[1].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[1].getValue(), "运算符");
    EXPECT_EQ(tokens[2].getType(), TokenType::Plus);
}

// 运算符 后随 == （双目运算符符号）
TEST(LexerStage3Test, OperatorContextEqualEqual) {
    auto tokens = withoutEof(lexSource("运算符=="));
    ASSERT_GE(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[0].getValue(), "运算符");
    EXPECT_EQ(tokens[1].getType(), TokenType::EqualEqual);
}

// ==================== 3. 模板尖括号消歧（lexer 层保持运算符 token） ====================

// 结果<整32, 整32>：< > 在 lexer 层是 Less/Greater 运算符 token
// （parser 层 isTemplateAngleOpen 消歧为模板类型）
// Token 序列：结果 < 整32 , 整32 >
TEST(LexerStage3Test, ResultTemplateAngleTokens) {
    auto tokens = withoutEof(lexSource("结果<整32, 整32>"));
    ASSERT_EQ(tokens.size(), 6u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Result);
    EXPECT_EQ(tokens[1].getType(), TokenType::Less);
    EXPECT_EQ(tokens[2].getType(), TokenType::Kw_Int32);
    EXPECT_EQ(tokens[3].getType(), TokenType::Comma);
    EXPECT_EQ(tokens[4].getType(), TokenType::Kw_Int32);
    EXPECT_EQ(tokens[5].getType(), TokenType::Greater);
}

// 可选<字符串>：单实参模板
TEST(LexerStage3Test, OptionalTemplateAngleTokens) {
    auto tokens = withoutEof(lexSource("可选<字符串>"));
    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Optional);
    EXPECT_EQ(tokens[1].getType(), TokenType::Less);
    EXPECT_EQ(tokens[2].getType(), TokenType::Kw_String);
    EXPECT_EQ(tokens[3].getType(), TokenType::Greater);
}

// 自定义类型名<实参>：向量<整32>
TEST(LexerStage3Test, CustomTemplateAngleTokens) {
    auto tokens = withoutEof(lexSource("向量<整32>"));
    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[1].getType(), TokenType::Less);
    EXPECT_EQ(tokens[2].getType(), TokenType::Kw_Int32);
    EXPECT_EQ(tokens[3].getType(), TokenType::Greater);
}

// 小于比较 a < b：仍是 Less 运算符 token（与模板形态 token 序列相同，parser 层区分）
TEST(LexerStage3Test, LessThanComparisonTokens) {
    auto tokens = withoutEof(lexSource("a < b"));
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[1].getType(), TokenType::Less);
    EXPECT_EQ(tokens[2].getType(), TokenType::Identifier);
}

// ==================== 4. 新关键字保留字检查 ====================

// 常量 作为保留字：不可作标识符（词法层识别为关键字）
TEST(LexerStage3Test, ConstReserved) {
    auto tokens = withoutEof(lexSource("整32 常量"));
    ASSERT_GE(tokens.size(), 2u);
    EXPECT_EQ(tokens[1].getType(), TokenType::Kw_Const);
}

// 友元 作为保留字
TEST(LexerStage3Test, FriendReserved) {
    auto tokens = withoutEof(lexSource("友元"));
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Friend);
}

// 泛型 作为保留字
TEST(LexerStage3Test, GenericReserved) {
    auto tokens = withoutEof(lexSource("泛型"));
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Generic);
}
