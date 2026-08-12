// 类型系统完善单元测试（Task 2.3）
// 覆盖：20类型识别、字面量后缀解析（f/L/LL/U/UL/ULL）、隐式转换链、
//       位运算/移位类型检查、i128 字面量与运算、8/16位整数类型
// 测试方式：通过 Lexer + Parser 得到真实AST，再交给 SemanticAnalyzer 分析（全链路）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::SemanticAnalyzer;
using cn_compiler::types::canonical;
using cn_compiler::types::canConvert;
using cn_compiler::types::commonNumericType;
using cn_compiler::types::isFloat;
using cn_compiler::types::isInteger;
using cn_compiler::types::literalTypeOf;

namespace {

// 辅助：解析+语义分析源码，返回是否成功与诊断引擎
struct SemanticResult {
    bool ok = false;         // 语义分析是否成功
    int errorCount = 0;      // 错误数量
    std::string messages;    // 全部诊断消息（拼接）
};

SemanticResult analyzeSource(const std::string& source) {
    SemanticResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "类型测试.cn", diagnostics);
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

// ==================== 类型别名与分类（type_system 子模块） ====================

// 别名规范化：整数 -> 整32、小数 -> 浮64
TEST(TypeSystemTest, CanonicalAlias) {
    EXPECT_EQ(canonical("整数"), "整32");
    EXPECT_EQ(canonical("小数"), "浮64");
    EXPECT_EQ(canonical("整64"), "整64");
    EXPECT_EQ(canonical("浮32"), "浮32");
}

// 整数类型识别（含别名）
TEST(TypeSystemTest, IntegerClassification) {
    EXPECT_TRUE(isInteger("整8"));
    EXPECT_TRUE(isInteger("整16"));
    EXPECT_TRUE(isInteger("整32"));
    EXPECT_TRUE(isInteger("整64"));
    EXPECT_TRUE(isInteger("整128"));
    EXPECT_TRUE(isInteger("正8"));
    EXPECT_TRUE(isInteger("正128"));
    EXPECT_TRUE(isInteger("整数"));  // 别名
    EXPECT_FALSE(isInteger("浮32"));
    EXPECT_FALSE(isInteger("字符"));
}

// 浮点类型识别（含别名）
TEST(TypeSystemTest, FloatClassification) {
    EXPECT_TRUE(isFloat("浮32"));
    EXPECT_TRUE(isFloat("浮64"));
    EXPECT_TRUE(isFloat("小数"));  // 别名
    EXPECT_FALSE(isFloat("整32"));
}

// ==================== 字面量后缀解析（规格书4.3） ====================

// 整数后缀：无后缀->整32、f->浮32、L->整64、LL->整128、U->正32、UL->正64、ULL->正128
TEST(TypeSystemTest, LiteralSuffixInteger) {
    EXPECT_EQ(literalTypeOf("42", false), "整32");
    EXPECT_EQ(literalTypeOf("42f", false), "浮32");
    EXPECT_EQ(literalTypeOf("42L", false), "整64");
    EXPECT_EQ(literalTypeOf("42LL", false), "整128");
    EXPECT_EQ(literalTypeOf("42U", false), "正32");
    EXPECT_EQ(literalTypeOf("42UL", false), "正64");
    EXPECT_EQ(literalTypeOf("42ULL", false), "正128");
}

// 浮点后缀：无后缀->浮64、f->浮32
TEST(TypeSystemTest, LiteralSuffixFloat) {
    EXPECT_EQ(literalTypeOf("3.14", true), "浮64");
    EXPECT_EQ(literalTypeOf("3.14f", true), "浮32");
    EXPECT_EQ(literalTypeOf("3.14F", true), "浮32");
}

// ==================== 隐式转换链（规格书3.7） ====================

// 相同类型可转；字符↔整数互通
TEST(TypeSystemTest, ImplicitConversionCharInt) {
    EXPECT_TRUE(canConvert("整32", "整32"));
    EXPECT_TRUE(canConvert("字符", "整32"));
    EXPECT_TRUE(canConvert("整32", "字符"));
    EXPECT_TRUE(canConvert("字符", "整64"));
}

// 整型宽化：小位宽 -> 大位宽（同符号方向）
TEST(TypeSystemTest, ImplicitConversionIntWiden) {
    EXPECT_TRUE(canConvert("整8", "整32"));
    EXPECT_TRUE(canConvert("整32", "整64"));
    EXPECT_TRUE(canConvert("整64", "整128"));
    EXPECT_TRUE(canConvert("正8", "正64"));
    // 同秩跨符号允许隐式（位模式一致，仅解释不同；正64 传 整64 参数等场景）
    EXPECT_TRUE(canConvert("正32", "整32"));
    EXPECT_TRUE(canConvert("整32", "正32"));
    // 降位宽不允许（变量/表达式；字面量窄化在变量声明处单独处理）
    EXPECT_FALSE(canConvert("整64", "整32"));
}

// 浮点宽化：浮32 -> 浮64；整数 -> 浮点；浮点不可隐式转整数
TEST(TypeSystemTest, ImplicitConversionFloat) {
    EXPECT_TRUE(canConvert("浮32", "浮64"));
    EXPECT_TRUE(canConvert("整32", "浮64"));
    EXPECT_TRUE(canConvert("整8", "浮32"));
    EXPECT_FALSE(canConvert("浮64", "整32"));
    EXPECT_FALSE(canConvert("浮64", "浮32"));
}

// 数值运算结果类型：整型取秩高者、含浮点取浮点
TEST(TypeSystemTest, CommonNumericType) {
    EXPECT_EQ(commonNumericType("整8", "整32"), "整32");
    EXPECT_EQ(commonNumericType("整32", "整64"), "整64");
    EXPECT_EQ(commonNumericType("整64", "整128"), "整128");
    EXPECT_EQ(commonNumericType("整32", "浮32"), "浮32");
    EXPECT_EQ(commonNumericType("整32", "浮64"), "浮64");
    EXPECT_EQ(commonNumericType("浮32", "浮64"), "浮64");
}

// ==================== 全链路语义测试 ====================

// 20类型声明与初始化（含后缀字面量）
TEST(TypeSystemSemanticTest, AllWidthTypesDecl) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整8 a = 10
    整16 b = 100
    整32 c = 1000
    整64 d = 100000L
    整128 e = 100000LL
    正8 f = 200
    正16 g = 300
    正32 h = 400U
    正64 i = 500UL
    正128 j = 600ULL
    浮32 k = 1.5f
    浮64 l = 2.5
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 隐式转换赋值（整32 -> 整64、浮32 -> 浮64、整 -> 浮）
TEST(TypeSystemSemanticTest, ImplicitConversionAssign) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 a = 100
    整64 b = a
    浮32 f = 1.5f
    浮64 g = f
    浮64 h = a
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 位运算与移位（整型操作数，结果类型推导）
TEST(TypeSystemSemanticTest, BitwiseAndShift) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 a = 0b1100
    整32 b = 0b1010
    整32 c = a & b
    整32 d = a | b
    整32 e = a ^ b
    整32 f = ~a
    整32 g = a << 2
    整32 h = a >> 1
    返回 c
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 位运算要求整数操作数（浮点报错）
TEST(TypeSystemSemanticTest, BitwiseOnFloatError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    浮64 a = 1.5
    浮64 b = 2.5
    浮64 c = a & b
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// i128 字面量与基本运算（加/减/比较）
TEST(TypeSystemSemanticTest, Int128BasicOps) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整128 a = 1000000LL
    整128 b = 2000000LL
    整128 c = a + b
    整128 d = b - a
    布尔 e = (c == 3000000LL)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 8/16位整数运算（结果类型应为公共类型）
TEST(TypeSystemSemanticTest, SmallIntArithmetic) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整8 a = 10
    整8 b = 20
    整32 c = a + b
    正8 d = 100
    正16 e = 200
    正32 f = d + e
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 十六进制/二进制/八进制字面量（词法+解析+语义全链路）
TEST(TypeSystemSemanticTest, HexBinOctLiterals) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 a = 0xFF
    整32 b = 0b1010
    整32 c = 0o777
    返回 a
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 浮点算术（SSE 全链路类型检查）
TEST(TypeSystemSemanticTest, FloatArithmetic) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    浮64 a = 1.5 + 2.25
    浮64 b = 5.0 * 2.0
    浮64 c = 7.5 / 2.0
    浮64 d = 10.0 - 3.5
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 浮点参数传递与返回（函数调用链）
TEST(TypeSystemSemanticTest, FloatFunctionCall) {
    auto r = analyzeSource(R"CN(
函数 加浮(浮64 a, 浮64 b) -> 浮64 {
    返回 a + b
}

函数 主() -> 整32 {
    浮64 r = 加浮(1.5, 2.5)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}
