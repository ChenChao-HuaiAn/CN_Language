// 词法分析器单元测试（Task 1.2）
// 覆盖：基本Token识别、53个关键字、运算符（含++/--）、分隔符、注释、字符串转义、
//       原始/多行字符串、错误处理、位置追踪、混合代码
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

// 辅助函数：分析源码并返回诊断引擎（用于错误测试）
Diagnostics analyzeWithDiagnostics(const std::string& source) {
    Diagnostics diagnostics;
    Lexer lexer(source, "测试.cn", diagnostics);
    lexer.tokenize();
    return diagnostics;
}

// 去除末尾EOF后的Token列表（便于断言长度）
std::vector<Token> withoutEof(const std::vector<Token>& tokens) {
    std::vector<Token> result = tokens;
    if (!result.empty() && result.back().getType() == TokenType::EndOfFile) {
        result.pop_back();
    }
    return result;
}

} // namespace

// ==================== 1. 基本Token识别 ====================

// 整数/浮点/字符串/字符/标识符基本识别
TEST(LexerTest, BasicTokenRecognition) {
    // 8个token：变量 x = 42 3.14 "你好" 'A' 名称
    auto tokens = withoutEof(analyze("变量 x = 42 3.14 \"你好\" 'A' 名称"));
    ASSERT_EQ(tokens.size(), 8u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Var);
    EXPECT_EQ(tokens[1].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[1].getValue(), "x");
    EXPECT_EQ(tokens[2].getType(), TokenType::Equal);
    EXPECT_EQ(tokens[3].getType(), TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[3].getValue(), "42");
    EXPECT_EQ(tokens[4].getType(), TokenType::FloatLiteral);
    EXPECT_EQ(tokens[4].getValue(), "3.14");
    EXPECT_EQ(tokens[5].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[5].getValue(), "\"你好\"");
    EXPECT_EQ(tokens[6].getType(), TokenType::CharLiteral);
    EXPECT_EQ(tokens[6].getValue(), "'A'");
    EXPECT_EQ(tokens[7].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[7].getValue(), "名称");
}

// 空源码只产生EOF
TEST(LexerTest, EmptySource) {
    auto tokens = analyze("");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].getType(), TokenType::EndOfFile);
}

// ==================== 2. 关键字识别（53个全部） ====================

// 全部53个关键字：文本 -> 类型
TEST(LexerTest, All53Keywords) {
    const std::vector<std::pair<std::string, TokenType>> kKeywords = {
        // 控制流(10)
        {"如果", TokenType::Kw_If}, {"否则", TokenType::Kw_Else},
        {"当", TokenType::Kw_While}, {"循环", TokenType::Kw_For},
        {"返回", TokenType::Kw_Return}, {"中断", TokenType::Kw_Break},
        {"继续", TokenType::Kw_Continue}, {"选择", TokenType::Kw_Switch},
        {"情况", TokenType::Kw_Case}, {"默认", TokenType::Kw_Default},
        // 类型(20)
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
        {"结构体", TokenType::Kw_Struct}, {"枚举", TokenType::Kw_Enum},
        // 声明(7)
        {"函数", TokenType::Kw_Function}, {"变量", TokenType::Kw_Var},
        {"导入", TokenType::Kw_Import}, {"从", TokenType::Kw_From},
        {"公开", TokenType::Kw_Public}, {"私有", TokenType::Kw_Private},
        {"静态", TokenType::Kw_Static},
        // 常量(3)
        {"真", TokenType::Kw_True}, {"假", TokenType::Kw_False},
        {"无", TokenType::Kw_None},
        // OOP(9)
        {"类", TokenType::Kw_Class}, {"接口", TokenType::Kw_Interface},
        {"保护", TokenType::Kw_Protected}, {"虚拟", TokenType::Kw_Virtual},
        {"重写", TokenType::Kw_Override}, {"抽象", TokenType::Kw_Abstract},
        {"实现", TokenType::Kw_Implements}, {"自身", TokenType::Kw_Self},
        {"父类", TokenType::Kw_Super},
        // 错误处理(2)
        {"结果", TokenType::Kw_Result}, {"可选", TokenType::Kw_Optional},
        // 字面量前缀(2)
        {"原始", TokenType::Kw_Raw}, {"多行", TokenType::Kw_MultiLine},
    };
    ASSERT_EQ(kKeywords.size(), static_cast<size_t>(53));
    std::string source;
    for (const auto& entry : kKeywords) {
        source += entry.first + " ";
    }
    auto tokens = withoutEof(analyze(source));
    ASSERT_EQ(tokens.size(), static_cast<size_t>(53));
    for (size_t i = 0; i < kKeywords.size(); i++) {
        EXPECT_EQ(tokens[i].getType(), kKeywords[i].second) << "关键字: " << kKeywords[i].first;
        EXPECT_EQ(tokens[i].getValue(), kKeywords[i].first);
    }
}

// 关键字最长匹配：整32 是一个关键字，而非 整3 + 2
TEST(LexerTest, KeywordLongestMatch) {
    auto tokens = withoutEof(analyze("整32 x"));
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Int32);
    EXPECT_EQ(tokens[1].getType(), TokenType::Identifier);
}

// 关键字后必须分隔：如果x 是标识符而非关键字
TEST(LexerTest, KeywordNeedsSeparator) {
    auto tokens = withoutEof(analyze("如果x"));
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[0].getValue(), "如果x");
}

// ==================== 3. 运算符识别（含++/--） ====================

// 自增/自减：i++ / i-- / ++i / --i
TEST(LexerTest, IncrementDecrement) {
    auto tokens = withoutEof(analyze("i++ j-- ++k --m"));
    ASSERT_EQ(tokens.size(), 8u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[1].getType(), TokenType::PlusPlus);
    EXPECT_EQ(tokens[1].getValue(), "++");
    EXPECT_EQ(tokens[2].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[3].getType(), TokenType::MinusMinus);
    EXPECT_EQ(tokens[3].getValue(), "--");
    EXPECT_EQ(tokens[4].getType(), TokenType::PlusPlus);
    EXPECT_EQ(tokens[5].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[6].getType(), TokenType::MinusMinus);
    EXPECT_EQ(tokens[7].getType(), TokenType::Identifier);
}

// 所有双字符运算符：== != <= >= && || -> += -= *= /= %= &= |= ^= <<= >>= << >>
TEST(LexerTest, TwoCharOperators) {
    auto tokens = withoutEof(analyze(
        "a == b != c <= d >= e && f || g -> h "
        "i += j -= k *= l /= m %= n &= o |= p ^= q <<= r >>= s << t >> u"));
    // 40个token：21个标识符 + 19个运算符
    ASSERT_EQ(tokens.size(), 40u);
    // 运算符类型序列（按出现顺序，跳过标识符）
    const std::vector<TokenType> expectedOps = {
        TokenType::EqualEqual, TokenType::BangEqual, TokenType::LessEqual,
        TokenType::GreaterEqual, TokenType::AndAnd, TokenType::OrOr,
        TokenType::Arrow, TokenType::PlusEqual, TokenType::MinusEqual,
        TokenType::StarEqual, TokenType::SlashEqual, TokenType::PercentEqual,
        TokenType::AmpEqual, TokenType::PipeEqual, TokenType::CaretEqual,
        TokenType::LessLessEqual, TokenType::GreaterGreaterEqual,
        TokenType::LessLess, TokenType::GreaterGreater,
    };
    size_t opIndex = 0;
    for (const auto& token : tokens) {
        if (token.isOperator()) {
            ASSERT_LT(opIndex, expectedOps.size());
            EXPECT_EQ(token.getType(), expectedOps[opIndex]) << "运算符下标: " << opIndex;
            opIndex++;
        }
    }
    EXPECT_EQ(opIndex, expectedOps.size());
}

// 所有单字符运算符与分隔符
TEST(LexerTest, SingleCharOperatorsAndDelimiters) {
    auto tokens = withoutEof(analyze(
        "+ - * / % = < > ! & | ^ ~ ( ) { } [ ] ; , . : ?"));
    // 24个单字符（13运算符 + 8分隔符 + 3个?等）
    ASSERT_EQ(tokens.size(), 24u);
    const std::vector<TokenType> expected = {
        TokenType::Plus, TokenType::Minus, TokenType::Star, TokenType::Slash,
        TokenType::Percent, TokenType::Equal, TokenType::Less, TokenType::Greater,
        TokenType::Bang, TokenType::Amp, TokenType::Pipe, TokenType::Caret,
        TokenType::Tilde, TokenType::LeftParen, TokenType::RightParen,
        TokenType::LeftBrace, TokenType::RightBrace, TokenType::LeftBracket,
        TokenType::RightBracket, TokenType::Semicolon, TokenType::Comma,
        TokenType::Dot, TokenType::Colon, TokenType::Question,
    };
    ASSERT_EQ(tokens.size(), expected.size());
    for (size_t i = 0; i < tokens.size(); i++) {
        EXPECT_EQ(tokens[i].getType(), expected[i]) << "下标: " << i << " 值: " << tokens[i].getValue();
    }
}

// ==================== 4. 分隔符识别 ====================

// 分隔符完整覆盖（含?）
TEST(LexerTest, DelimiterRecognition) {
    auto tokens = withoutEof(analyze("( ) { } [ ] ; , . : ?"));
    ASSERT_EQ(tokens.size(), 11u);
    EXPECT_TRUE(tokens[0].isDelimiter());
    EXPECT_TRUE(tokens[10].isDelimiter());
    EXPECT_EQ(tokens[10].getType(), TokenType::Question);
}

// ==================== 5. 注释处理 ====================

// 行注释与块注释剥离
TEST(LexerTest, CommentStrip) {
    // 12个token：整数 a = 1 整数 b = 2 整数 c = 3（注释已剥离）
    auto tokens = withoutEof(analyze(
        "整数 a = 1 // 单行注释\n整数 b = 2 /* 块注释 */ 整数 c = 3"));
    ASSERT_EQ(tokens.size(), 12u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Int);
    EXPECT_EQ(tokens[3].getType(), TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[4].getType(), TokenType::Kw_Int);
    EXPECT_EQ(tokens[7].getType(), TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[8].getType(), TokenType::Kw_Int);
    EXPECT_EQ(tokens[11].getType(), TokenType::IntegerLiteral);
}

// 嵌套块注释
TEST(LexerTest, NestedBlockComment) {
    // 4个token：整数 a = 1（嵌套注释整体剥离）
    auto tokens = withoutEof(analyze("整数 a /* 外层 /* 内层 */ 继续 */ = 1"));
    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Int);
    EXPECT_EQ(tokens[1].getType(), TokenType::Identifier);
    EXPECT_EQ(tokens[2].getType(), TokenType::Equal);
    EXPECT_EQ(tokens[3].getType(), TokenType::IntegerLiteral);
}

// ==================== 6. 字符串转义处理 ====================

// 字符串转义：\n \t \r \" \\ \uXXXX
// 注意：MSVC 不支持 raw string 中的 \" 序列（C2017非法转义），改用普通字符串字面量
TEST(LexerTest, StringEscapes) {
    auto tokens = withoutEof(analyze("\"行1\\n行2\\t\\\"引号\\\"\\\\结束\""));
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].getValue(), "\"行1\\n行2\\t\\\"引号\\\"\\\\结束\"");
}

// 字符转义：\n \t \0 \u{4E2D}
TEST(LexerTest, CharEscapes) {
    auto tokens = withoutEof(analyze("'\\n' '\\t' '\\0' '\\u{4E2D}'"));
    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokens[0].getType(), TokenType::CharLiteral);
    EXPECT_EQ(tokens[1].getType(), TokenType::CharLiteral);
    EXPECT_EQ(tokens[2].getType(), TokenType::CharLiteral);
    EXPECT_EQ(tokens[3].getType(), TokenType::CharLiteral);
    EXPECT_EQ(tokens[3].getValue(), "'\\u{4E2D}'");
}

// 中文字符字面量
TEST(LexerTest, ChineseCharLiteral) {
    auto tokens = withoutEof(analyze("'中'"));
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].getType(), TokenType::CharLiteral);
    EXPECT_EQ(tokens[0].getValue(), "'中'");
}

// ==================== 7. 原始字符串与多行字符串 ====================

// 原始字符串：反斜杠不需转义
TEST(LexerTest, RawString) {
    auto tokens = withoutEof(analyze("原始\"^\\d+\\.\\d+$\""));
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].getValue(), "原始\"^\\d+\\.\\d+$\"");
}

// 多行字符串：三引号包裹，可跨行
TEST(LexerTest, MultiLineString) {
    auto tokens = withoutEof(analyze("多行\"\"\"第一行\\n第二行\"\"\""));
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].getValue(), "多行\"\"\"第一行\\n第二行\"\"\"");
}

// 组合前缀：原始多行
TEST(LexerTest, RawMultiLineString) {
    auto tokens = withoutEof(analyze("原始多行\"\"\"第一行\n第二行\"\"\""));
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].getType(), TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].getValue(), "原始多行\"\"\"第一行\n第二行\"\"\"");
}

// 原始/多行作为独立关键字（后不跟引号）
TEST(LexerTest, RawMultiLineAsKeyword) {
    auto tokens = withoutEof(analyze("原始 多行"));
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].getType(), TokenType::Kw_Raw);
    EXPECT_EQ(tokens[1].getType(), TokenType::Kw_MultiLine);
}

// ==================== 8. 错误处理 ====================

// 非法字符报告诊断
TEST(LexerTest, IllegalCharDiagnostic) {
    Diagnostics d = analyzeWithDiagnostics("a @ b");
    EXPECT_GT(d.getErrorCount(), 0);
    EXPECT_TRUE(d.hasErrors());
}

// 未闭合字符串报告诊断
TEST(LexerTest, UnclosedStringDiagnostic) {
    Diagnostics d = analyzeWithDiagnostics("\"未闭合");
    EXPECT_GT(d.getErrorCount(), 0);
}

// 未闭合字符报告诊断
TEST(LexerTest, UnclosedCharDiagnostic) {
    Diagnostics d = analyzeWithDiagnostics("'x");
    EXPECT_GT(d.getErrorCount(), 0);
}

// 无效转义序列报告诊断
TEST(LexerTest, InvalidEscapeDiagnostic) {
    Diagnostics d = analyzeWithDiagnostics("'\\q'");
    EXPECT_GT(d.getErrorCount(), 0);
}

// 未闭合块注释报告诊断
TEST(LexerTest, UnclosedBlockCommentDiagnostic) {
    Diagnostics d = analyzeWithDiagnostics("/* 未闭合");
    EXPECT_GT(d.getErrorCount(), 0);
}

// 字符串跨行报告诊断
TEST(LexerTest, StringNewlineDiagnostic) {
    Diagnostics d = analyzeWithDiagnostics("\"第一行\n第二行\"");
    EXPECT_GT(d.getErrorCount(), 0);
}

// 正常代码无诊断
TEST(LexerTest, NormalCodeNoDiagnostic) {
    Diagnostics d = analyzeWithDiagnostics("整32 x = 1; 返回 x + 2");
    EXPECT_EQ(d.getErrorCount(), 0);
    EXPECT_EQ(d.getWarningCount(), 0);
}

// ==================== 9. 位置追踪 ====================

// 行号/列号正确性（多行源码）
TEST(LexerTest, LocationTracking) {
    auto tokens = analyze("整32 a = 1;\n字符串 b = \"你好\"");
    // 10个token：整32 a = 1 ; 字符串 b = "你好" EOF
    ASSERT_EQ(tokens.size(), 10u);
    // 第1行：整32(1,1) a(1,5) =(1,7) 1(1,9) ;(1,10)
    EXPECT_EQ(tokens[0].getLocation().getLine(), 1);
    EXPECT_EQ(tokens[0].getLocation().getColumn(), 1);
    EXPECT_EQ(tokens[4].getLocation().getLine(), 1);
    EXPECT_EQ(tokens[4].getLocation().getColumn(), 10);
    // 第2行：字符串(2,1) b(2,5) =(2,7) "你好"(2,9)
    EXPECT_EQ(tokens[5].getLocation().getLine(), 2);
    EXPECT_EQ(tokens[5].getLocation().getColumn(), 1);
    EXPECT_EQ(tokens[8].getLocation().getLine(), 2);
    EXPECT_EQ(tokens[8].getLocation().getColumn(), 9);
}

// ==================== 10. 混合代码测试 ====================

// 类似真实CN代码片段：函数定义+循环+自增自减
TEST(LexerTest, MixedCode) {
    const std::string source =
        "函数 求和(整32 n) -> 整32 {\n"
        "    整32 总和 = 0\n"
        "    循环 (整32 i = 0; i < n; i++) {\n"
        "        总和 += i\n"
        "    }\n"
        "    返回 总和\n"
        "}";
    auto tokens = withoutEof(analyze(source));
    // 32个token（按源码逐行精确计数，无多余分号）
    const std::vector<TokenType> expected = {
        // 函数 求和(整32 n) -> 整32 {
        TokenType::Kw_Function, TokenType::Identifier, TokenType::LeftParen,
        TokenType::Kw_Int32, TokenType::Identifier, TokenType::RightParen,
        TokenType::Arrow, TokenType::Kw_Int32, TokenType::LeftBrace,
        // 整32 总和 = 0
        TokenType::Kw_Int32, TokenType::Identifier, TokenType::Equal,
        TokenType::IntegerLiteral,
        // 循环 (整32 i = 0; i < n; i++) {
        TokenType::Kw_For, TokenType::LeftParen,
        TokenType::Kw_Int32, TokenType::Identifier, TokenType::Equal,
        TokenType::IntegerLiteral, TokenType::Semicolon,
        TokenType::Identifier, TokenType::Less, TokenType::Identifier,
        TokenType::Semicolon,
        TokenType::Identifier, TokenType::PlusPlus, TokenType::RightParen,
        TokenType::LeftBrace,
        // 总和 += i
        TokenType::Identifier, TokenType::PlusEqual, TokenType::Identifier,
        // }
        TokenType::RightBrace,
        // 返回 总和
        TokenType::Kw_Return, TokenType::Identifier,
        // }
        TokenType::RightBrace,
    };
    ASSERT_EQ(tokens.size(), expected.size());
    for (size_t i = 0; i < tokens.size(); i++) {
        EXPECT_EQ(tokens[i].getType(), expected[i]) << "下标: " << i << " 值: " << tokens[i].getValue();
    }
}
