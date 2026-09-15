// i128 类型系统单元测试（Task 完善A：规格书10.5）
// 覆盖：isI128 识别、textExceedsInt64 边界、splitI128Text 全范围拆分、
//       i128 字面量解析、i128 运算/比较语义
// 测试方式：Lexer + Parser + SemanticAnalyzer 全链路 + 类型系统纯函数
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/types/type_system.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::SemanticAnalyzer;
using cn_compiler::types::canonical;
using cn_compiler::types::isI128;
using cn_compiler::types::splitI128Text;
using cn_compiler::types::textExceedsInt64;

namespace {

struct SemanticResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;
};

SemanticResult analyzeSource(const std::string& source) {
    SemanticResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "i128语义测试.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    SemanticAnalyzer analyzer(diagnostics);
    result.ok = analyzer.analyze(program.get());
    result.errorCount = diagnostics.getErrorCount();
    for (const auto& d : diagnostics.getAll()) {
        result.messages += d.message + "\n";
    }
    return result;
}

} // namespace

// ==================== isI128 类型识别 ====================

TEST(I128SemanticTest, IsI128Recognizes) {
    EXPECT_TRUE(isI128(canonical("整128")));
    EXPECT_TRUE(isI128(canonical("正128")));
    EXPECT_FALSE(isI128(canonical("整64")));
    EXPECT_FALSE(isI128(canonical("整32")));
    EXPECT_FALSE(isI128(canonical("浮64")));
}

// ==================== textExceedsInt64 边界 ====================

TEST(I128SemanticTest, TextExceedsInt64Boundary) {
    EXPECT_FALSE(textExceedsInt64("9223372036854775807"));    // 2^63-1 不超
    EXPECT_TRUE(textExceedsInt64("9223372036854775808"));     // 2^63 超
    EXPECT_TRUE(textExceedsInt64("18446744073709551615"));    // 2^64-1 超
    EXPECT_TRUE(textExceedsInt64("170141183460469231731687303715884105727")); // 2^127-1
    EXPECT_FALSE(textExceedsInt64("0"));
    EXPECT_FALSE(textExceedsInt64("42"));
}

// ==================== splitI128Text 拆分 ====================

// 小值：低64位即值，高64位为0
TEST(I128SemanticTest, SplitI128SmallValue) {
    const std::string s = splitI128Text("42");
    ASSERT_FALSE(s.empty());
    // 格式 "LO:HI"（十六进制，%llX 大写）
    const std::size_t colon = s.find(':');
    ASSERT_NE(colon, std::string::npos);
    EXPECT_EQ(s.substr(0, colon), "2A");          // 42 = 0x2A
    EXPECT_EQ(s.substr(colon + 1), "0");           // 高64位 0
}

// 2^64 边界：低64位0，高64位1
TEST(I128SemanticTest, SplitI128TwoPow64) {
    const std::string s = splitI128Text("18446744073709551616");  // 2^64
    ASSERT_FALSE(s.empty());
    const std::size_t colon = s.find(':');
    ASSERT_NE(colon, std::string::npos);
    EXPECT_EQ(s.substr(0, colon), "0");            // 低64位 0
    EXPECT_EQ(s.substr(colon + 1), "1");           // 高64位 1
}

// 最大边界 2^127-1
TEST(I128SemanticTest, SplitI128Max) {
    const std::string s = splitI128Text("170141183460469231731687303715884105727");
    ASSERT_FALSE(s.empty());
    const std::size_t colon = s.find(':');
    ASSERT_NE(colon, std::string::npos);
    EXPECT_EQ(s.substr(0, colon), "FFFFFFFFFFFFFFFF");     // 低64位全1
    EXPECT_EQ(s.substr(colon + 1), "7FFFFFFFFFFFFFFF");    // 高64位 0x7FFF...
}

// ==================== i128 字面量语义 ====================

// 超 int64 整数字面量应合法（自动提升 i128）
TEST(I128SemanticTest, Int128LiteralLargeAccepted) {
    const std::string src = R"(
函数 主() -> 整32 {
    整128 大 = 170141183460469231731687303715884105727;
    返回 0;
}
)";
    SemanticResult r = analyzeSource(src);
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 超 2^127-1 的字面量应报错（越界）
TEST(I128SemanticTest, Int128LiteralOverflowRejected) {
    const std::string src = R"(
函数 主() -> 整32 {
    整128 大 = 170141183460469231731687303715884105728;
    返回 0;
}
)";
    SemanticResult r = analyzeSource(src);
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// i128 加法运算语义合法
TEST(I128SemanticTest, Int128AddOps) {
    const std::string src = R"(
函数 主() -> 整32 {
    整128 a = 9223372036854775808;
    整128 b = 9223372036854775809;
    整128 c = a + b;
    返回 0;
}
)";
    SemanticResult r = analyzeSource(src);
    EXPECT_TRUE(r.ok) << r.messages;
}
