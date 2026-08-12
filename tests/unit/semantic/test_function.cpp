// 函数完整支持语义单元测试（Task 2.2）
// 覆盖：函数原型声明（前向引用/签名一致性）、递归、函数指针（声明/赋值/调用/参数）
// 测试方式：Lexer + Parser + SemanticAnalyzer 全链路（非Mock）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::SemanticAnalyzer;

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
    Lexer lexer(source, "函数语义测试.cn", diagnostics);
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

// ==================== 函数原型声明（前向引用） ====================

// 前向引用：先调用后定义（原型声明在前，定义在后）合法
TEST(SemanticFunctionTest, ForwardReferenceOk) {
    auto r = analyzeSource(R"CN(
函数 计算(整32 n) -> 整32

函数 使用() -> 整32 {
    返回 计算(10)
}

函数 计算(整32 n) -> 整32 {
    返回 n * 2
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 原型声明+定义签名一致合法（参数类型/返回类型相同）
TEST(SemanticFunctionTest, PrototypeAndDefMatchOk) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 原型声明+定义签名不一致报错（返回类型不同）
TEST(SemanticFunctionTest, PrototypeReturnMismatchError) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32
函数 加(整32 a, 整32 b) -> 整64 { 返回 a + b }
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 原型声明+定义签名不一致报错（参数类型不同）
TEST(SemanticFunctionTest, PrototypeParamMismatchError) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32
函数 加(浮64 a, 整32 b) -> 整32 { 返回 0 }
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 重复定义（两个函数体）报错
TEST(SemanticFunctionTest, DuplicateDefinitionError) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 加(整32 a, 整32 b) -> 整32 { 返回 a - b }
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 纯原型声明（无定义）合法：声明本身不报错（链接期缺失检测）
TEST(SemanticFunctionTest, PrototypeOnlyOk) {
    auto r = analyzeSource(R"CN(
函数 外部函数(整32 n) -> 整32
)CN");
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.errorCount, 0);
}

// ==================== 递归调用 ====================

// 递归：函数体内调用自身合法
TEST(SemanticFunctionTest, RecursionOk) {
    auto r = analyzeSource(R"CN(
函数 阶乘(整32 n) -> 整32 {
    如果 (n <= 1) { 返回 1 }
    返回 n * 阶乘(n - 1)
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 互递归：两个函数相互调用合法（前向符号注册）
TEST(SemanticFunctionTest, MutualRecursionOk) {
    auto r = analyzeSource(R"CN(
函数 偶数(整32 n) -> 布尔 {
    如果 (n == 0) { 返回 真 }
    返回 奇数(n - 1)
}
函数 奇数(整32 n) -> 布尔 {
    如果 (n == 0) { 返回 假 }
    返回 偶数(n - 1)
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// ==================== 函数指针：声明/赋值/调用 ====================

// 函数指针声明+赋值+调用合法（规格书5.8 C风格）
TEST(SemanticFunctionTest, FuncPtrDeclAssignCallOk) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 主() -> 整32 {
    整32(*回调)(整32, 整32)
    回调 = 加
    整32 r = 回调(10, 20)
    返回 r
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 函数指针赋值类型不匹配报错（返回类型不同）
TEST(SemanticFunctionTest, FuncPtrAssignReturnMismatchError) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 主() -> 整32 {
    整64(*回调)(整32, 整32)
    回调 = 加
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 函数指针赋值类型不匹配报错（参数类型不同）
TEST(SemanticFunctionTest, FuncPtrAssignParamMismatchError) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 主() -> 整32 {
    整32(*回调)(浮64, 整32)
    回调 = 加
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 函数指针调用参数数量错误报错
TEST(SemanticFunctionTest, FuncPtrCallArgCountError) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 主() -> 整32 {
    整32(*回调)(整32, 整32)
    回调 = 加
    整32 结果 = 回调(10)
    返回 结果
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 函数指针调用参数类型错误报错
TEST(SemanticFunctionTest, FuncPtrCallArgTypeError) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 主() -> 整32 {
    整32(*回调)(整32, 整32)
    回调 = 加
    整32 结果 = 回调("字符串", 20)
    返回 结果
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 函数指针作为函数参数（回调模式）
TEST(SemanticFunctionTest, FuncPtrAsParamOk) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 执行(整32(*func)(整32, 整32), 整32 x, 整32 y) -> 整32 {
    返回 func(x, y)
}
函数 主() -> 整32 {
    整32(*回调)(整32, 整32)
    回调 = 加
    返回 执行(回调, 10, 20)
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 函数指针作实参类型不匹配报错
TEST(SemanticFunctionTest, FuncPtrArgMismatchError) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 执行(整32(*func)(整32, 整32), 整32 x, 整32 y) -> 整32 {
    返回 func(x, y)
}
函数 主() -> 整32 {
    整64(*回调)(整32, 整32)
    回调 = 加
    返回 执行(回调, 10, 20)
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 未声明函数名赋值报错
TEST(SemanticFunctionTest, FuncPtrAssignUndefinedError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32(*回调)(整32, 整32)
    回调 = 不存在的函数
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 未赋值函数指针调用：语义层通过（静态无法跟踪赋值），运行期行为由后续Task/运行期处理
// 这里验证调用语法与类型检查合法（空指针运行期崩溃属于运行期错误，非编译期）
TEST(SemanticFunctionTest, FuncPtrCallUninitializedCompiles) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32(*回调)(整32, 整32)
    整32 r = 回调(10, 20)
    返回 r
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}
