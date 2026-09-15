// 指针/数组语义单元测试（Task 2.4）
// 覆盖：指针类型检查（整32* 与 整64* 不互通）、数组退化（整32[5] -> 整32*）、
//       指针算术类型（p+1 仍为指针）、空指针（无 字面量/比较）、空类型* 双向转换、
//       下标类型检查（非整型下标报错）、取地址/解引用类型推导、-> 占位报错
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
#include "cn_compiler/types/type_system.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::SemanticAnalyzer;
using cn_compiler::types::arrayElemOf;
using cn_compiler::types::arrayLenOf;
using cn_compiler::types::canonical;
using cn_compiler::types::canConvert;
using cn_compiler::types::isArray;
using cn_compiler::types::isPointer;
using cn_compiler::types::pointeeOf;
using cn_compiler::types::typeSize;

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
    Lexer lexer(source, "指针测试.cn", diagnostics);
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

// ==================== 复合类型工具（type_system） ====================

// 递归规范化：整数* -> 整32*、整数[5] -> 整32[5]
TEST(PointerSemanticTest, CanonicalComposite) {
    EXPECT_EQ(canonical("整数*"), "整32*");
    EXPECT_EQ(canonical("整数[5]"), "整32[5]");
    EXPECT_EQ(canonical("小数*"), "浮64*");
}

// isPointer / isArray / pointeeOf / arrayElemOf / arrayLenOf
TEST(PointerSemanticTest, CompositeTools) {
    EXPECT_TRUE(isPointer("整32*"));
    EXPECT_FALSE(isPointer("整32[5]"));
    EXPECT_TRUE(isArray("整32[5]"));
    EXPECT_FALSE(isArray("整32"));
    EXPECT_EQ(pointeeOf("整32*"), "整32");
    EXPECT_EQ(arrayElemOf("整32[5]"), "整32");
    EXPECT_EQ(arrayLenOf("整32[5]"), 5);
    EXPECT_EQ(arrayLenOf("整32"), -1);
}

// typeSize：基本类型字节大小
TEST(PointerSemanticTest, TypeSize) {
    EXPECT_EQ(typeSize("整8"), 1);
    EXPECT_EQ(typeSize("整16"), 2);
    EXPECT_EQ(typeSize("整32"), 4);
    EXPECT_EQ(typeSize("整64"), 8);
    EXPECT_EQ(typeSize("浮64"), 8);
    EXPECT_EQ(typeSize("布尔"), 1);
    EXPECT_EQ(typeSize("整32*"), 8);  // 指针恒8字节
}

// ==================== 隐式转换 ====================

// 空类型* <-> 任意指针（规格书3.7）
TEST(PointerSemanticTest, VoidPtrConvert) {
    EXPECT_TRUE(canConvert("空类型*", "整32*"));
    EXPECT_TRUE(canConvert("整32*", "空类型*"));
    EXPECT_TRUE(canConvert("空类型*", "浮64*"));
}

// 数组退化：整32[5] -> 整32*
TEST(PointerSemanticTest, ArrayDecay) {
    EXPECT_TRUE(canConvert("整32[5]", "整32*"));
    EXPECT_FALSE(canConvert("整32[5]", "整64*"));
}

// 不同指针类型不互通（整32* -> 整64* 不允许）
TEST(PointerSemanticTest, PointerStrict) {
    EXPECT_FALSE(canConvert("整32*", "整64*"));
    EXPECT_FALSE(canConvert("整64*", "整32*"));
    EXPECT_TRUE(canConvert("整32*", "整32*"));
}

// ==================== 语义检查 ====================

// 指针声明/取地址/解引用全链路
TEST(PointerSemanticTest, PointerFullChain) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32 x = 42;\n"
        "  整32* p = &x;\n"
        "  整32 y = *p;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 指针算术：p + 1 结果仍为指针；p - 1；指针自增
TEST(PointerSemanticTest, PointerArith) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32[3] 数据 = { 1, 2, 3 };\n"
        "  整32* p = &数据[0];\n"
        "  整32* q = p + 1;\n"
        "  整32* r = p - 1;\n"
        "  p++;\n"
        "  p--;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 数组下标读写 + 变量下标
TEST(PointerSemanticTest, ArrayIndex) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32[5] 数据 = { 1, 2, 3, 4, 5 };\n"
        "  整32 i = 2;\n"
        "  整32 v = 数据[i];\n"
        "  数据[0] = 100;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 数组初始化列表：元素类型不匹配报错
TEST(PointerSemanticTest, InitListTypeMismatch) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32[3] 数据 = { 1, 2.5, 3 };\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_FALSE(r.ok);
}

// 数组初始化超长报错
TEST(PointerSemanticTest, InitListTooLong) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32[2] 数据 = { 1, 2, 3 };\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_FALSE(r.ok);
}

// 空指针：无 赋给指针、与指针比较
TEST(PointerSemanticTest, NullPointer) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32* p = 无;\n"
        "  如果 (p == 无) {\n"
        "    整32 x = 1;\n"
        "  }\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 空类型* 双向转换赋值
TEST(PointerSemanticTest, VoidPtrAssign) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32* p = 无;\n"
        "  空类型* vp = p;\n"
        "  整32* q = vp;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 数组名退化：整32[5] 变量赋给 整32*
TEST(PointerSemanticTest, ArrayDecaySemantic) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32[5] 数据 = { 1, 2, 3, 4, 5 };\n"
        "  整32* p = 数据;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 解引用非指针报错
TEST(PointerSemanticTest, DerefNonPointer) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32 x = 42;\n"
        "  整32 y = *x;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_FALSE(r.ok);
}

// 取地址非常量左值报错（&(x + 1)）
TEST(PointerSemanticTest, AddressOfNonLvalue) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32 x = 1;\n"
        "  整32 y = 2;\n"
        "  整32* p = &(x + y);\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_FALSE(r.ok);
}

// 指针与整数比较报错（规格书3.7：指针与整数禁止隐式转换）
TEST(PointerSemanticTest, PointerIntCompare) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32* p = 无;\n"
        "  如果 p == 1 {\n"
        "  }\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_FALSE(r.ok);
}

// 下标对象非数组/指针报错
TEST(PointerSemanticTest, IndexNonArray) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32 x = 5;\n"
        "  整32 y = x[0];\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_FALSE(r.ok);
}

// 下标非整型报错
TEST(PointerSemanticTest, IndexNonInteger) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32[3] 数据 = { 1, 2, 3 };\n"
        "  浮64 f = 1.5;\n"
        "  整32 y = 数据[f];\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_FALSE(r.ok);
}

// -> 成员访问：非结构体指针报错（Task 2.7 结构体语义接通后，整32* -> 字段 报类型错误）
TEST(PointerSemanticTest, ArrowOnNonStructPointer) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32* p = 无;\n"
        "  整32 y = p->字段;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("不是结构体"), std::string::npos);
}

// 指针返回类型与参数
TEST(PointerSemanticTest, PointerParamReturn) {
    SemanticResult r = analyzeSource(
        "不安全 函数 取首元素(整32* p) -> 整32 {\n"
        "  返回 *p;\n"
        "}\n"
        "不安全 函数 主() -> 整32 {\n"
        "  整32[3] 数据 = { 1, 2, 3 };\n"
        "  整32 v = 取首元素(&数据[0]);\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// ==================== 188-a（D6·plans/023 B11 变量常量传播） ====================
//   判定=无字面量种子 + 全函数无其他写入 + 传播闭包 + 取地址/引用实参失格。
//   使用点三操作面（解引用/成员访问/下标访问）函数收尾统一判定报硬错误。

// 变量形态解引用：`整64* p = 无; *p` -> 编译期硬错误（原判据只认 `*无` 字面量）
TEST(PointerSemanticTest, NullConstPropagationDeref) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整64* p = 无;\n"
        "  整64 v = *p;\n"
        "  返回 整32(v);\n"
        "}\n");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("编译期常量空指针解引用"), std::string::npos);
}

// 判空比较合法：`如果 (p == 无)` 不触发（Task 6.2 判空惯用法）
TEST(PointerSemanticTest, NullConstPropagationGuardOk) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整64* p = 无;\n"
        "  如果 (p == 无) {\n"
        "    返回 1;\n"
        "  }\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 非空写入即失格：分支内 取地址 赋值后解引用（保守判定——不报）
TEST(PointerSemanticTest, NullConstPropagationDisqualifiedByWrite) {
    SemanticResult r = analyzeSource(
        "静态 整64 甲 = 5;\n"
        "不安全 函数 主(布尔 c) -> 整32 {\n"
        "  整64* p = 无;\n"
        "  如果 (c) {\n"
        "    p = &甲;\n"
        "  }\n"
        "  整64 v = *p;\n"
        "  返回 整32(v);\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 取地址失格：别名可改写 -> 不报（保守，防假阳性）
TEST(PointerSemanticTest, NullConstPropagationDisqualifiedByEscape) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整64* p = 无;\n"
        "  整64** pp = &p;\n"
        "  整64 v = *p;\n"
        "  返回 整32(v);\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 传播闭包：`整64* q = p;`（p 恒空）-> q 亦恒空 -> 解引用硬错误
TEST(PointerSemanticTest, NullConstPropagationEdge) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整64* p = 无;\n"
        "  整64* q = p;\n"
        "  整64 v = *q;\n"
        "  返回 整32(v);\n"
        "}\n");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("编译期常量空指针解引用"), std::string::npos);
}

// 成员访问面：`点* q = 无; q.x`
TEST(PointerSemanticTest, NullConstPropagationMember) {
    SemanticResult r = analyzeSource(
        "结构体 点 { 整64 x; }\n"
        "不安全 函数 主() -> 整32 {\n"
        "  点* q = 无;\n"
        "  整64 v = q.x;\n"
        "  返回 整32(v);\n"
        "}\n");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("编译期常量空指针成员访问"), std::string::npos);
}

// 下标访问面：`整64* p = 无; p[0]`
TEST(PointerSemanticTest, NullConstPropagationIndex) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整64* p = 无;\n"
        "  整64 v = p[0];\n"
        "  返回 整32(v);\n"
        "}\n");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("编译期常量空指针下标访问"), std::string::npos);
}

// 字面量面补齐：`无.字段` / `无[0]` 直接报 B11（原为间接类型错误）
TEST(PointerSemanticTest, NullConstLiteralMemberAndIndex) {
    SemanticResult r1 = analyzeSource(
        "结构体 点 { 整64 x; }\n"
        "不安全 函数 主() -> 整32 {\n"
        "  整64 v = 无.x;\n"
        "  返回 整32(v);\n"
        "}\n");
    EXPECT_FALSE(r1.ok);
    EXPECT_NE(r1.messages.find("编译期常量空指针成员访问"), std::string::npos);
    SemanticResult r2 = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整64 v = 无[0];\n"
        "  返回 整32(v);\n"
        "}\n");
    EXPECT_FALSE(r2.ok);
    EXPECT_NE(r2.messages.find("编译期常量空指针下标访问"), std::string::npos);
}
