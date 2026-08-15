// 对...属于 迭代语句语义单元测试（C-2，2026-08）
// 覆盖：数组迭代降级通过、类容器（大小()/元素(整64)）降级通过、
//   非数组/非类容器报错、类缺 大小()/元素() 方法报错、
//   非名称式迭代对象（函数调用）报错、循环变量作用域（循环外不可见）、
//   循环体类型错误传播、嵌套 对...属于
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
    Lexer lexer(source, "对属于测试.cn", diagnostics);
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

// 数组迭代：对 x 属于 数组 { 体 } —— 语义通过（数组形态降级）
TEST(RangeForTest, ArrayIterationOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32[3] 数据 = { 1, 2, 3 }
    整32 总和 = 0
    对 x 属于 数据 {
        总和 += x
    }
    返回 总和
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}

// 类容器迭代：大小()/元素(整64) 形态 —— 语义通过（类容器降级）
TEST(RangeForTest, ClassContainerOk) {
    auto r = analyzeSource(R"CN(
类 迷你表 {
私有:
    整32[3] 数据
公开:
    常量 函数 大小() -> 整64 {
        返回 3
    }
    函数 元素(整64 位置) -> 整32 {
        返回 数据[位置]
    }
}
函数 主() -> 整32 {
    迷你表 表
    整32 总和 = 0
    对 x 属于 表 {
        总和 += x
    }
    返回 总和
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}

// 迭代对象为成员访问（结构体字段数组）：语义通过
TEST(RangeForTest, MemberIterableOk) {
    auto r = analyzeSource(R"CN(
结构体 班级 {
    整32[3] 成绩
}
函数 主() -> 整32 {
    班级 班 = 班级{ 成绩 = { 1, 2, 3 } }
    整32 总和 = 0
    对 成绩 属于 班.成绩 {
        总和 += 成绩
    }
    返回 总和
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}

// 嵌套 对...属于（结构体数组 -> 字段数组）：语义通过
TEST(RangeForTest, NestedIterationOk) {
    auto r = analyzeSource(R"CN(
结构体 班级 {
    整32[2] 成绩
}
函数 主() -> 整32 {
    班级[2] 各班 = { 班级{ 成绩 = { 1, 2 } }, 班级{ 成绩 = { 3, 4 } } }
    整32 总和 = 0
    对 班 属于 各班 {
        对 成绩 属于 班.成绩 {
            总和 += 成绩
        }
    }
    返回 总和
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}

// 迭代非数组/非类容器（整32）：报错
TEST(RangeForTest, NonContainerError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 x = 5
    对 v 属于 x {
        打印(v)
    }
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("须为数组或类容器"));
}

// 类缺 元素(整64) 方法：报错
TEST(RangeForTest, MissingElementMethodError) {
    auto r = analyzeSource(R"CN(
类 无元素表 {
公开:
    常量 函数 大小() -> 整64 {
        返回 3
    }
}
函数 主() -> 整32 {
    无元素表 表
    对 x 属于 表 {
        打印(x)
    }
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("大小() 与 元素(整64)"));
}

// 非名称式迭代对象（函数调用）：报错
TEST(RangeForTest, NonNameIterableError) {
    auto r = analyzeSource(R"CN(
函数 取数组() -> 整32[3] {
    整32[3] 数据 = { 1, 2, 3 }
    返回 数据
}
函数 主() -> 整32 {
    对 x 属于 取数组() {
        打印(x)
    }
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("变量/自身/成员/下标"));
}

// 循环变量作用域：循环外引用 报错（未声明）
TEST(RangeForTest, LoopVarScopedError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32[3] 数据 = { 1, 2, 3 }
    对 x 属于 数据 {
        打印(x)
    }
    返回 x
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("未声明"));
}

// 循环体内类型错误正常报告（体被克隆后仍检查）
TEST(RangeForTest, BodyTypeErrorReported) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32[3] 数据 = { 1, 2, 3 }
    对 x 属于 数据 {
        字符串 错误 = x
    }
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 同作用域多个 对...属于：索引变量唯一化，不冲突
TEST(RangeForTest, MultipleLoopsSameScopeOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32[2] 甲 = { 1, 2 }
    整32[2] 乙 = { 3, 4 }
    整32 总和 = 0
    对 a 属于 甲 {
        总和 += a
    }
    对 b 属于 乙 {
        总和 += b
    }
    返回 总和
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}
