// 语义分析器单元测试（Task 1.5）
// 覆盖：符号表管理（变量声明/查找/作用域）、类型检查（二元/一元/赋值）、
//       函数调用检查（参数匹配）、错误检测（未声明/重复声明/类型不匹配/非循环中跳出）、
//       返回值检查、混合代码语义分析
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
    Lexer lexer(source, "语义测试.cn", diagnostics);
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

// ==================== 符号表管理 ====================

// 变量声明 + 引用查找（同一函数体内）
TEST(SemanticTest, VarDeclAndReference) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    变量 x = 10
    返回 x
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 类型前置声明 + 初始化
TEST(SemanticTest, VarDeclExplicitType) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 x = 10
    浮64 y = 3.14
    返回 x
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 内层作用域遮蔽外层变量（查找从内到外）
TEST(SemanticTest, ScopeShadowing) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    变量 x = 1
    如果 (真) {
        变量 x = 2
        返回 x
    }
    返回 x
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 代码块子作用域：块内声明块外不可见
TEST(SemanticTest, ScopeExitVarInvisible) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    如果 (真) {
        变量 x = 2
    }
    返回 x
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// ==================== 类型检查 ====================

// 整型运算 + 浮点运算推导
TEST(SemanticTest, ArithmeticTypeInference) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    变量 a = 1 + 2
    变量 b = 3.5 * 2
    返回 a
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 取余要求整数
TEST(SemanticTest, ModuloRequiresInteger) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    变量 x = 7 % 2
    返回 x
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 比较运算返回布尔
TEST(SemanticTest, ComparisonReturnsBool) {
    auto r = analyzeSource(R"CN(
函数 主() -> 布尔 {
    返回 1 < 2
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 逻辑运算要求布尔操作数
TEST(SemanticTest, LogicalOpBoolOperands) {
    auto r = analyzeSource(R"CN(
函数 主() -> 布尔 {
    返回 真 && 假
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 一元负号要求数值
TEST(SemanticTest, UnaryMinusNumeric) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    返回 -5
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 逻辑非要求布尔（对整数取非报错）
TEST(SemanticTest, LogicalNotRequiresBool) {
    auto r = analyzeSource(R"CN(
函数 主() -> 布尔 {
    返回 !1
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 条件必须为布尔（整数条件报错）
TEST(SemanticTest, ConditionMustBeBool) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    如果 (1) {
        返回 0
    }
    返回 1
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 赋值类型不匹配报错
TEST(SemanticTest, AssignmentTypeMismatch) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    变量 x = 1
    x = "字符串"
    返回 x
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 声明初始值类型不匹配报错
TEST(SemanticTest, InitTypeMismatch) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 x = "字符串"
    返回 x
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 宽化转换合法（整32 -> 整64）
TEST(SemanticTest, IntegerWidening) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整64 {
    整64 x = 10
    返回 x
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// ==================== 函数调用检查 ====================

// 函数调用参数数量匹配
TEST(SemanticTest, CallArgsMatch) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 {
    返回 a + b
}
函数 主() -> 整32 {
    返回 加(1, 2)
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 函数调用参数数量不匹配报错
TEST(SemanticTest, CallArgCountMismatch) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 {
    返回 a + b
}
函数 主() -> 整32 {
    返回 加(1)
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 函数调用参数类型不匹配报错
TEST(SemanticTest, CallArgTypeMismatch) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 {
    返回 a + b
}
函数 主() -> 整32 {
    返回 加(1, "字符串")
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 前向调用（先调用后定义）合法
TEST(SemanticTest, ForwardCall) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    返回 平方(3)
}
函数 平方(整32 x) -> 整32 {
    返回 x * x
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// ==================== 错误检测 ====================

// 未声明的变量
TEST(SemanticTest, UndeclaredVariable) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    返回 不存在
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 未声明的函数
TEST(SemanticTest, UndeclaredFunction) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    返回 不存在的函数(1)
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 同作用域重复声明变量
TEST(SemanticTest, DuplicateVariable) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    变量 x = 1
    变量 x = 2
    返回 x
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 重复定义函数
TEST(SemanticTest, DuplicateFunction) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    返回 0
}
函数 主() -> 整32 {
    返回 1
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 非循环中使用中断
TEST(SemanticTest, BreakOutsideLoop) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    中断
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 非循环中使用继续
TEST(SemanticTest, ContinueOutsideLoop) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    继续
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 循环中使用中断/继续合法
TEST(SemanticTest, BreakContinueInLoop) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    当 (真) {
        中断
    }
    循环 (变量 i = 0; i < 10; i = i + 1) {
        如果 (i == 3) {
            继续
        }
        中断
    }
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// ==================== 返回值检查 ====================

// 正确返回值
TEST(SemanticTest, CorrectReturnValue) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 {
    返回 a + b
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 返回类型不匹配报错
TEST(SemanticTest, ReturnTypeMismatch) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    返回 "字符串"
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 有返回类型但缺少返回语句报错
TEST(SemanticTest, MissingReturnStatement) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    变量 x = 1
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 空类型函数返回值报错
TEST(SemanticTest, VoidFunctionReturnsValue) {
    auto r = analyzeSource(R"CN(
函数 无返回值() {
    返回 1
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 空类型函数无返回值合法
TEST(SemanticTest, VoidFunctionNoReturn) {
    auto r = analyzeSource(R"CN(
函数 无返回值() {
    返回
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// ==================== 混合代码 ====================

// 综合：函数调用+控制流+变量+返回值
TEST(SemanticTest, MixedCode) {
    auto r = analyzeSource(R"CN(
函数 阶乘(整32 n) -> 整32 {
    如果 (n <= 1) {
        返回 1
    }
    返回 n * 阶乘(n - 1)
}
函数 主() -> 整32 {
    变量 答案 = 阶乘(5)
    循环 (变量 i = 0; i < 10; i = i + 1) {
        如果 (i == 5) {
            中断
        }
    }
    返回 答案
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 递归函数合法
TEST(SemanticTest, RecursiveFunction) {
    auto r = analyzeSource(R"CN(
函数 斐波那契(整32 n) -> 整32 {
    如果 (n < 2) {
        返回 n
    }
    返回 斐波那契(n - 1) + 斐波那契(n - 2)
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 自增自减（数值操作数）
TEST(SemanticTest, IncrementDecrement) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    变量 i = 0
    i++
    变量 x = i--
    返回 x
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}
