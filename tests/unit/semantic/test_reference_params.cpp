// 引用参数语义单元测试（A-1 修复，2026-08）
// 覆盖：引用参数注册（签名保留 &，与按值参数区分）、实参自动取地址（左值校验）、
//   引用参数禁止默认值、引用返回类型（T&，P3-18 补完）、引用变量声明、
//   泛型 T& 引用参数、值/引用同形参重载调用歧义（文档化边界）
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
    Lexer lexer(source, "引用参数测试.cn", diagnostics);
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

// 引用参数基本形态：整32& 参数 + 左值实参调用 -> 语义通过
TEST(RefParamTest, BasicSwapOk) {
    auto r = analyzeSource(R"CN(
函数 交换(整32& a, 整32& b) -> 空类型 {
    整32 临时 = a
    a = b
    b = 临时
}
函数 主() -> 整32 {
    整32 x = 1
    整32 y = 2
    交换(x, y)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 引用参数与按值参数可共存定义（签名 key 保留 &，重载注册不冲突）
TEST(RefParamTest, OverloadValueVsRefCoexist) {
    auto r = analyzeSource(R"CN(
函数 双倍(整32 值) -> 整32 {
    返回 值 * 2
}
函数 双倍(整32& 值) -> 整32 {
    返回 值 * 2
}
函数 主() -> 整32 {
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 值/引用同形参调用歧义：两版本转换等级相同 -> 报歧义（文档化边界，
//   用户可用指针/显式转换消除；与 C++ 左值偏好规则不同，见报告 A-1）
TEST(RefParamTest, ValueVsRefCallAmbiguous) {
    auto r = analyzeSource(R"CN(
函数 双倍(整32 值) -> 整32 {
    返回 值 * 2
}
函数 双倍(整32& 值) -> 整32 {
    返回 值 * 2
}
函数 主() -> 整32 {
    整32 x = 5
    返回 双倍(x)
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("歧义"), std::string::npos) << r.messages;
}

// 非左值实参（字面量）传给引用参数 -> 报错
TEST(RefParamTest, NonLvalueArgError) {
    auto r = analyzeSource(R"CN(
函数 交换(整32& a, 整32& b) -> 空类型 {
    整32 临时 = a
    a = b
    b = 临时
}
函数 主() -> 整32 {
    交换(1, 2)
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("左值"), std::string::npos) << r.messages;
}

// 引用参数禁止默认值（引用须绑定调用方左值）
TEST(RefParamTest, RefDefaultValueError) {
    auto r = analyzeSource(R"CN(
函数 问候(整32& 值 = 1) -> 空类型 {
    值 += 1
}
函数 主() -> 整32 {
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("引用参数不能有默认值"), std::string::npos) << r.messages;
}

// P3-18 补完：引用返回全局/静态对象（存活) -> 语义通过
TEST(RefParamTest, RefReturnGlobalOk) {
    auto r = analyzeSource(R"CN(
静态 整32 全局值 = 42
函数 取引用() -> 整32& {
    返回 全局值
}
函数 主() -> 整32 {
    整32& r = 取引用()
    取引用() = 9
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// P3-18 补完：引用返回本函数局部变量地址（悬垂引用）-> 报错
TEST(RefParamTest, RefReturnLocalError) {
    auto r = analyzeSource(R"CN(
函数 坏() -> 整32& {
    整32 局部 = 5
    返回 局部
}
函数 主() -> 整32 {
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("局部变量"), std::string::npos) << r.messages;
}

// P3-18 补完：引用返回本函数按值参数地址（随栈帧消亡）-> 报错
TEST(RefParamTest, RefReturnByValueParamError) {
    auto r = analyzeSource(R"CN(
函数 坏(整32 v) -> 整32& {
    返回 v
}
函数 主() -> 整32 {
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("局部变量"), std::string::npos) << r.messages;
}

// P3-18 补完：引用返回引用参数（指向调用方存储，存活）-> 通过
TEST(RefParamTest, RefReturnRefParamOk) {
    auto r = analyzeSource(R"CN(
函数 传回(整32& x) -> 整32& {
    返回 x
}
函数 主() -> 整32 {
    整32 值 = 100
    整32& 别名 = 传回(值)
    传回(值) = 300
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// P3-18 补完：引用返回非左值（字面量）-> 报错
TEST(RefParamTest, RefReturnNonLvalueError) {
    auto r = analyzeSource(R"CN(
函数 坏() -> 整32& {
    返回 42
}
函数 主() -> 整32 {
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("左值"), std::string::npos) << r.messages;
}

// P3-18：引用变量声明（变量 整32& r = x，x 为左值变量）-> 通过（此前报"暂不支持"）
TEST(RefParamTest, RefVarDeclOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 x = 1
    整32& r = x
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// P3-18：引用变量须绑定左值变量（右值初始化器 -> 报错）
TEST(RefParamTest, RefVarDeclRvalueError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32& r = 42
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("须绑定左值"), std::string::npos) << r.messages;
}

// 泛型引用参数（T&）声明与调用 -> 语义通过
TEST(RefParamTest, GenericRefParamOk) {
    auto r = analyzeSource(R"CN(
泛型 <类型 T>
函数 引用交换(T& a, T& b) -> 空类型 {
    T 临时 = a
    a = b
    b = 临时
}
函数 主() -> 整32 {
    整64 p = 100
    整64 q = 200
    引用交换<整64>(p, q)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 结构体引用参数（账户&）：成员访问经引用 -> 语义通过（友元授权场景）
TEST(RefParamTest, StructRefParamOk) {
    auto r = analyzeSource(R"CN(
结构体 点 {
    整32 x
    整32 y
}
函数 移动(点& p, 整32 dx) -> 空类型 {
    p.x += dx
    p.y += dx
}
函数 主() -> 整32 {
    点 原点 = 点{ x = 1, y = 2 }
    移动(原点, 10)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 数组元素引用实参（表[0] 传给 整32&）-> 语义通过
TEST(RefParamTest, ArrayElementRefArgOk) {
    auto r = analyzeSource(R"CN(
函数 加一(整32& 值) -> 空类型 {
    值 += 1
}
函数 主() -> 整32 {
    整32[3] 表 = { 5, 6, 7 }
    加一(表[0])
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}
