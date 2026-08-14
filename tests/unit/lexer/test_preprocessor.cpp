// 条件编译预处理器单元测试（Task 6.6，规格书阶段六）
// 覆盖：#定义/#如果定义（括号与空白两种形式）/#否则/#结束如果、
//       命令行 -D 宏注入、嵌套条件编译、未定义宏分支裁剪、
//       未激活分支不产生语法错误（保留行号）、错误指令诊断
#include <gtest/gtest.h>
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/lexer/token.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Token;
using cn_compiler::TokenType;

namespace {

// 辅助：带宏集合分析源码，返回 Token 类型序列（不含 EOF）
std::vector<TokenType> typesOf(const std::string& source,
                               const std::unordered_set<std::string>& macros = {}) {
    Diagnostics diagnostics;
    Lexer lexer(source, "测试.cn", diagnostics, macros);
    auto tokens = lexer.tokenize();
    std::vector<TokenType> result;
    for (const auto& t : tokens) {
        if (t.getType() != TokenType::EndOfFile) result.push_back(t.getType());
    }
    return result;
}

// 辅助：带宏集合分析源码，返回诊断引擎（用于错误测试）
Diagnostics diagsOf(const std::string& source,
                    const std::unordered_set<std::string>& macros = {}) {
    Diagnostics diagnostics;
    Lexer lexer(source, "测试.cn", diagnostics, macros);
    lexer.tokenize();
    return diagnostics;
}

} // namespace

// ==================== 1. #如果定义 分支选择 ====================

// 定义了宏 -> 编译 #如果定义 分支；未定义 -> 走 #否则 分支
TEST(PreprocessorTest, IfDefElseSelection) {
    const std::string src =
        "#如果定义(调试)\n"
        "变量 a = 1\n"
        "#否则\n"
        "变量 b = 2\n"
        "#结束如果\n";
    // 未注入宏：b 分支（变量/Kw_Var, b/Identifier, =, 2）
    auto t1 = typesOf(src);
    ASSERT_EQ(t1.size(), 4u);
    EXPECT_EQ(t1[0], TokenType::Kw_Var);
    EXPECT_EQ(t1[1], TokenType::Identifier);
    EXPECT_EQ(t1[2], TokenType::Equal);
    EXPECT_EQ(t1[3], TokenType::IntegerLiteral);

    // 注入宏 调试：a 分支（变量/Kw_Var, a/Identifier, =, 1）
    auto t2 = typesOf(src, {"调试"});
    ASSERT_EQ(t2.size(), 4u);
    EXPECT_EQ(t2[1], TokenType::Identifier);
    EXPECT_EQ(t2[3], TokenType::IntegerLiteral);
}

// 无 #否则：未定义宏时分支整体裁剪
TEST(PreprocessorTest, IfDefWithoutElse) {
    const std::string src =
        "#如果定义 调试\n"
        "变量 a = 1\n"
        "#结束如果\n";
    // 未定义 -> 裁剪，只剩 EOF
    auto t1 = typesOf(src);
    EXPECT_EQ(t1.size(), 0u);
    // 已定义 -> 保留
    auto t2 = typesOf(src, {"调试"});
    ASSERT_EQ(t2.size(), 4u);
}

// 括号形式与空白分隔形式等价
TEST(PreprocessorTest, IfDefParenAndSpaceForms) {
    auto paren = typesOf("#如果定义(调试)\n变量 x = 1\n#结束如果\n", {"调试"});
    auto space = typesOf("#如果定义 调试\n变量 x = 1\n#结束如果\n", {"调试"});
    ASSERT_EQ(paren.size(), 4u);
    ASSERT_EQ(space.size(), 4u);
    EXPECT_EQ(paren[0], TokenType::Kw_Var);
    EXPECT_EQ(space[0], TokenType::Kw_Var);
}

// ==================== 2. #定义 宏 ====================

// 源码内 #定义 后 #如果定义 命中（C 语义：宏在指令行之后可见）
TEST(PreprocessorTest, DefineThenIfDef) {
    const std::string src =
        "#定义 调试模式\n"
        "#如果定义 调试模式\n"
        "变量 x = 1\n"
        "#否则\n"
        "变量 y = 2\n"
        "#结束如果\n";
    auto t = typesOf(src);
    ASSERT_EQ(t.size(), 4u);
    EXPECT_EQ(t[1], TokenType::Identifier);  // x 分支
}

// 父层不活跃时 #定义 不生效（C 语义）
TEST(PreprocessorTest, DefineInInactiveBranchIgnored) {
    const std::string src =
        "#如果定义 不存在\n"
        "#定义 内部宏\n"
        "#结束如果\n"
        "#如果定义 内部宏\n"
        "变量 x = 1\n"
        "#结束如果\n";
    // 内部宏 未被定义 -> 后续 #如果定义 不命中，x 分支裁剪
    auto t = typesOf(src);
    EXPECT_EQ(t.size(), 0u);
}

// ==================== 3. 嵌套条件编译 ====================

TEST(PreprocessorTest, NestedConditionals) {
    const std::string src =
        "#如果定义 甲\n"
        "变量 a = 1\n"
        "#如果定义 乙\n"
        "变量 b = 2\n"
        "#结束如果\n"
        "#否则\n"
        "变量 c = 3\n"
        "#结束如果\n";
    // 注入 甲：外层真 -> a；内层 乙 未定义 -> 裁剪；否则分支不取
    auto t = typesOf(src, {"甲"});
    ASSERT_EQ(t.size(), 4u);
    EXPECT_EQ(t[1], TokenType::Identifier);  // a

    // 注入 甲+乙：a 与 b 都编译
    auto t2 = typesOf(src, {"甲", "乙"});
    ASSERT_EQ(t2.size(), 8u);

    // 都不注入：外层假 -> c 分支
    auto t3 = typesOf(src);
    ASSERT_EQ(t3.size(), 4u);
    EXPECT_EQ(t3[1], TokenType::Identifier);  // c
}

// ==================== 4. 行号保留 ====================

// 未激活分支裁剪后行号不偏移：第4行 Token 位置为行4
TEST(PreprocessorTest, LineNumbersPreserved) {
    const std::string src =
        "#如果定义 调试\n"
        "变量 a = 1\n"
        "#否则\n"
        "变量 b = 2\n"
        "#结束如果\n";
    Diagnostics diagnostics;
    Lexer lexer(src, "测试.cn", diagnostics);
    auto tokens = lexer.tokenize();
    // b 分支：变量 在第4行
    bool found = false;
    for (const auto& t : tokens) {
        if (t.getType() == TokenType::Kw_Var) {
            EXPECT_EQ(t.getLocation().getLine(), 4);
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// ==================== 5. 错误诊断 ====================

// 缺少 #结束如果
TEST(PreprocessorTest, MissingEndIf) {
    const std::string src = "#如果定义 调试\n变量 x = 1\n";
    auto d = diagsOf(src);
    EXPECT_TRUE(d.hasErrors());
}

// #否则 前缺少 #如果定义
TEST(PreprocessorTest, ElseWithoutIf) {
    const std::string src = "#否则\n变量 x = 1\n";
    auto d = diagsOf(src);
    EXPECT_TRUE(d.hasErrors());
}

// 未知预处理指令
TEST(PreprocessorTest, UnknownDirective) {
    const std::string src = "#未知指令\n变量 x = 1\n";
    auto d = diagsOf(src);
    EXPECT_TRUE(d.hasErrors());
}

// #如果定义 缺少宏名
TEST(PreprocessorTest, IfDefMissingMacroName) {
    const std::string src = "#如果定义\n变量 x = 1\n#结束如果\n";
    auto d = diagsOf(src);
    EXPECT_TRUE(d.hasErrors());
}

// #如果定义( 括号未闭合
TEST(PreprocessorTest, IfDefUnclosedParen) {
    const std::string src = "#如果定义(调试\n变量 x = 1\n#结束如果\n";
    auto d = diagsOf(src);
    EXPECT_TRUE(d.hasErrors());
}

// ==================== 6. 与普通代码共存 ====================

// 预处理指令行不干扰普通代码；# 不在行首视为普通未知字符？——
// 设计中指令行要求 '#' 在行首（允许前导空白），普通代码行内的 '#' 不触发
TEST(PreprocessorTest, DirectiveMustBeLineStart) {
    const std::string src =
        "变量 x = 1\n"
        "#如果定义 调试\n"
        "变量 y = 2\n"
        "#结束如果\n";
    // 未注入宏：x 分支保留，y 分支裁剪
    auto t = typesOf(src);
    ASSERT_EQ(t.size(), 4u);
    EXPECT_EQ(t[1], TokenType::Identifier);  // x
}
