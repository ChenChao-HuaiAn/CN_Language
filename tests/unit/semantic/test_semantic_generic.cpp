// 阶段3 语义分析器 泛型 单元测试（Task 3.8，规格书06-十三）
// 覆盖：泛型声明注册、泛型类单态化（类型名<实参> 实例化）、
//       泛型函数单态化、接口约束（泛型 <类型 T : 接口> 编译期检查）
// 测试方式：通过 Lexer + Parser 得到真实AST，再交给 SemanticAnalyzer 分析（全链路）
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

struct SemanticResult {
    bool ok = false;         // 语义分析是否成功
    int errorCount = 0;      // 错误数量
    std::string messages;    // 全部诊断消息（拼接）
};

SemanticResult analyzeSource(const std::string& source) {
    SemanticResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "语义泛型测试.cn", diagnostics);
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

// ==================== 泛型声明注册（Task 3.8） ====================

// 泛型类声明：合法
TEST(SemanticGenericTest, GenericClassDecl) {
    auto r = analyzeSource(R"CN(
泛型 <类型 T>
类 向量 {
私有:
    T* 数据;
    整64 长度;
公开:
    函数 向量(整64 容量) {
        数据 = 无;
        长度 = 0;
    }
    函数 推入(T 值) -> 空类型 {
        长度++;
    }
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 泛型函数声明：合法
TEST(SemanticGenericTest, GenericFunctionDecl) {
    auto r = analyzeSource(R"CN(
泛型 <类型 T>
函数 交换(T 值) -> T {
    返回 值;
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// ==================== 泛型类单态化（Task 3.8） ====================

// 泛型类实例化：盒子<整32> 声明变量 -> 生成单态化副本（合法）
TEST(SemanticGenericTest, GenericClassInstantiation) {
    auto r = analyzeSource(R"CN(
泛型 <类型 T>
类 盒子 {
公开:
    T 内容;
    函数 盒子(T 初始) {
        内容 = 初始;
    }
    函数 获取() -> T {
        返回 内容;
    }
}
函数 主() -> 整32 {
    // H7 语义补完（2026-08-25）：类仅有带参构造时裸声明（类名 变量）无默认构造，
    // 编译报错（与 C++ 语义一致）；此处用构造调用初始化（正确构造模式）。
    盒子<整32> 整数盒子 = 盒子<整32>(10);
    整32 值 = 整数盒子.获取();
    返回 值;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 泛型类实例化参数个数错误：报错
TEST(SemanticGenericTest, GenericArgCountMismatch) {
    auto r = analyzeSource(R"CN(
泛型 <类型 T>
类 盒子 {
公开:
    T 内容;
}
函数 主() -> 整32 {
    盒子<整32, 整64> 错误盒子;
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// ==================== 泛型函数注册（Task 3.8） ====================

// 泛型函数声明注册：合法（实例化由 IR/codegen 层展开）
TEST(SemanticGenericTest, GenericFunctionDeclared) {
    auto r = analyzeSource(R"CN(
泛型 <类型 T>
函数 恒等(T 值) -> T {
    返回 值;
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// ==================== 接口约束（Task 3.8） ====================

// 接口约束满足：实参类实现约束接口 -> 合法
// （泛型函数 名<类型>(实参) 调用形式由 IR/codegen 层展开；此处验证约束声明合法）
TEST(SemanticGenericTest, GenericConstraintSatisfied) {
    auto r = analyzeSource(R"CN(
接口 可命名 {
    虚拟 函数 获取名字() -> 字符串
}
类 人 : 可命名 {
公开:
    重写 函数 获取名字() -> 字符串 {
        返回 "人";
    }
}
泛型 <类型 T : 可命名>
函数 打印名字(T 实体) -> 空类型 {
    打印行(实体.获取名字());
}
函数 主() -> 整32 {
    人 张三 = 人();
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 接口约束不满足：实参类未实现约束接口 -> 报错
TEST(SemanticGenericTest, GenericConstraintViolated) {
    auto r = analyzeSource(R"CN(
接口 可命名 {
    虚拟 函数 获取名字() -> 字符串
}
类 数字 {
公开:
    整32 值;
}
泛型 <类型 T : 可命名>
函数 打印名字(T 实体) -> 空类型 {
    打印行(实体.获取名字());
}
函数 主() -> 整32 {
    数字 n = 数字();
    打印名字<数字>(n);
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(r.messages.find("未实现接口"), std::string::npos);
}
