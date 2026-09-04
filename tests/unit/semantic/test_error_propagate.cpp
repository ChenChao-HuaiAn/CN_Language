// 错误传播运算符 ? 语义单元测试（C-1，2026-08）
// 覆盖：结果<T,E>/可选<T> 操作数 ? 取值通过、非结果/可选操作数报错、
//   非 结果/可选 返回函数内 ? 报错、值类型精确一致校验（T 不匹配报错）、
//   错误类型可转换校验（E 不兼容报错）、? 在调用实参位置、? 后继续二元运算
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
using cn_compiler::SemanticAnalyzer;

namespace {

struct SemanticResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;
};

SemanticResult analyzeSource(const std::string& source) {
    SemanticResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "错误传播测试.cn", diagnostics);
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

// 结果<T,E> ? 正常传播（赋值位置）：语义通过
TEST(ErrorPropagateTest, ResultPropagateOk) {
    auto r = analyzeSource(R"CN(
函数 安全除法(整32 a, 整32 b) -> 结果<整32, 整32> {
    如果 b == 0 {
        返回 错误(1);
    }
    返回 正常(a / b);
}
函数 外层(整32 a, 整32 b) -> 结果<整32, 整32> {
    整32 商 = 安全除法(a, b)?;
    返回 正常(商);
}
函数 主() -> 整32 {
    结果<整32, 整32> r = 外层(10, 2);
    如果 r.正常 {
        返回 0;
    }
    返回 1;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}

// 可选<T> ? 传播：语义通过
TEST(ErrorPropagateTest, OptionalPropagateOk) {
    auto r = analyzeSource(R"CN(
函数 查找(整32 编号) -> 可选<整32> {
    如果 编号 == 0 {
        返回 无;
    }
    返回 某些(编号);
}
函数 外层(整32 编号) -> 可选<整32> {
    整32 v = 查找(编号)?;
    返回 某些(v + 1);
}
函数 主() -> 整32 {
    可选<整32> r = 外层(1);
    如果 r.有值 {
        返回 r.值;
    }
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}

// ? 在函数调用实参位置 + 继续二元运算（后缀优先级）：语义通过
TEST(ErrorPropagateTest, PropagateInArgumentAndBinaryOk) {
    auto r = analyzeSource(R"CN(
函数 读数() -> 结果<整32, 整32> {
    返回 正常(10);
}
函数 外层() -> 结果<整32, 整32> {
    打印("值: ", 读数()? + 1);
    返回 正常(0);
}
函数 主() -> 整32 {
    结果<整32, 整32> r = 外层();
    如果 r.正常 {
        返回 0;
    }
    返回 1;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}

// 非 结果/可选 操作数使用 ?：报错
TEST(ErrorPropagateTest, NonResultOperandError) {
    auto r = analyzeSource(R"CN(
函数 外层() -> 结果<整32, 整32> {
    整32 x = 5;
    整32 y = x?;
    返回 正常(y);
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("错误传播要求 结果<T,E>/可选<T>"));
}

// 返回类型非 结果/可选 的函数内使用 ?：报错
TEST(ErrorPropagateTest, NonResultFunctionError) {
    auto r = analyzeSource(R"CN(
函数 读数() -> 结果<整32, 整32> {
    返回 正常(10);
}
函数 主() -> 整32 {
    整32 x = 读数()?;
    返回 x;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("要求当前函数返回 结果<"));
}

// 值类型不一致（结果<整32,E> 传播到 结果<整64,E>）：报错
TEST(ErrorPropagateTest, ValueTypeMismatchError) {
    auto r = analyzeSource(R"CN(
函数 读数() -> 结果<整32, 整32> {
    返回 正常(10);
}
函数 外层() -> 结果<整64, 整32> {
    整64 x = 读数()?;
    返回 正常(x);
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("值类型"));
}

// 错误类型不可转换（结果<T,字符串> 传播到 结果<T,整32>）：报错
TEST(ErrorPropagateTest, ErrorTypeMismatchError) {
    auto r = analyzeSource(R"CN(
函数 读数() -> 结果<整32, 字符串> {
    返回 错误("失败");
}
函数 外层() -> 结果<整32, 整32> {
    整32 x = 读数()?;
    返回 正常(x);
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("错误类型"));
}

// 错误码整型可转换传播（整32 -> 整64）：语义通过
TEST(ErrorPropagateTest, ErrorTypeConvertibleOk) {
    auto r = analyzeSource(R"CN(
函数 读数() -> 结果<整32, 整32> {
    返回 错误(1);
}
函数 外层() -> 结果<整32, 整64> {
    整32 x = 读数()?;
    返回 正常(x);
}
函数 主() -> 整32 {
    结果<整32, 整64> r = 外层();
    如果 r.正常 {
        返回 0;
    }
    返回 1;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}
