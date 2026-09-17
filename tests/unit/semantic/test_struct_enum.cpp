// 结构体/枚举/联合体语义单元测试（Task 2.7）
// 覆盖：结构体布局计算（对齐/偏移/总大小）、字段访问类型检查（. 与 ->）、
//       枚举成员值求值（自动递增/显式赋值/负数）、枚举变量声明与使用、
//       结构体初始化列表检查、嵌套结构体、循环引用检测、类型名重复声明检测
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

// 辅助：解析+语义分析源码，返回是否成功与诊断消息
struct SemanticResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;
    std::unique_ptr<Program> program;
    SemanticAnalyzer* analyzer = nullptr;  // 不拥有，仅测试内临时访问
};

SemanticResult analyzeSource(const std::string& source) {
    SemanticResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "结构体语义测试.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    result.program = parser.parse(tokens);
    // 注意：analyzer 生命周期在函数内结束，布局信息无法跨测试访问；
    // 这里通过 AST 节点（program->structs）的布局回填字段验证
    SemanticAnalyzer analyzer(diagnostics);
    result.ok = analyzer.analyze(result.program.get());
    result.errorCount = diagnostics.getErrorCount();
    result.messages = diagnostics.format();
    return result;
}

} // namespace

// 结构体布局：整32 字段对齐 4、总大小对齐
TEST(StructSemanticTest, LayoutBasic) {
    SemanticResult r = analyzeSource(
        "结构体 点 { 整32 x; 整32 y; }\n"
        "不安全 函数 主() -> 整32 { 返回 0; }\n");
    EXPECT_TRUE(r.ok) << r.messages;
    ASSERT_EQ(r.program->structs.size(), 1u);
    const auto& decl = r.program->structs[0];
    EXPECT_TRUE(decl->layoutComputed);
    EXPECT_EQ(decl->totalSize, 8);   // 2 * 4
    EXPECT_EQ(decl->align, 4);
    EXPECT_EQ(decl->fields[0].offset, 0);
    EXPECT_EQ(decl->fields[1].offset, 4);
}

// 结构体布局：混合类型（整8 + 整64）C风格对齐
TEST(StructSemanticTest, LayoutMixedAlign) {
    SemanticResult r = analyzeSource(
        "结构体 混合 { 整8 a; 整64 b; }\n"
        "不安全 函数 主() -> 整32 { 返回 0; }\n");
    EXPECT_TRUE(r.ok) << r.messages;
    const auto& decl = r.program->structs[0];
    // 整8 a 在偏移0；整64 b 对齐到8 → 偏移8；总大小16（对齐8）
    EXPECT_EQ(decl->fields[0].offset, 0);
    EXPECT_EQ(decl->fields[1].offset, 8);
    EXPECT_EQ(decl->totalSize, 16);
    EXPECT_EQ(decl->align, 8);
}

// 联合体布局：所有字段偏移0、大小=最大字段大小
TEST(StructSemanticTest, UnionLayout) {
    SemanticResult r = analyzeSource(
        "联合体 数值 { 整32 整数部分; 浮64 浮点部分; }\n"
        "不安全 函数 主() -> 整32 { 返回 0; }\n");
    EXPECT_TRUE(r.ok) << r.messages;
    const auto& decl = r.program->structs[0];
    EXPECT_TRUE(decl->isUnion);
    EXPECT_EQ(decl->fields[0].offset, 0);
    EXPECT_EQ(decl->fields[1].offset, 0);   // 共享内存
    EXPECT_EQ(decl->totalSize, 8);          // 最大字段（浮64）大小
    EXPECT_EQ(decl->align, 8);
}

// 字段访问检查：p.x 有效；p.不存在字段 报错
TEST(StructSemanticTest, FieldAccessValidAndInvalid) {
    SemanticResult r = analyzeSource(
        "结构体 点 { 整32 x; 整32 y; }\n"
        "不安全 函数 主() -> 整32 {\n"
        "  点 p = 点{ x = 1, y = 2 };\n"
        "  整32 a = p.x;\n"
        "  整32 b = p.z;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("没有成员"), std::string::npos);
}

// 经指针访问（v2.1 统一 .，自动解引用一级）：结构体指针访问字段（类型正确）
TEST(StructSemanticTest, ArrowFieldAccess) {
    SemanticResult r = analyzeSource(
        "结构体 点 { 整32 x; 整32 y; }\n"
        "不安全 函数 主() -> 整32 {\n"
        "  点 p = 点{ x = 1, y = 2 };\n"
        "  点* ptr = &p;\n"
        "  整32 a = ptr.x;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 经指针访问：非结构体指针报错（整32* . 字段）
TEST(StructSemanticTest, ArrowOnNonStruct) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32* p = 无;\n"
        "  整32 a = p.字段;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("不是结构体"), std::string::npos);
}

// . 访问：非结构体类型（整32.字段）报错
TEST(StructSemanticTest, DotOnNonStruct) {
    SemanticResult r = analyzeSource(
        "不安全 函数 主() -> 整32 {\n"
        "  整32 n = 10;\n"
        "  整32 a = n.字段;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_FALSE(r.ok);
}

// 枚举成员值求值：自动递增 + 显式赋值 + 负数
TEST(StructSemanticTest, EnumValues) {
    SemanticResult r = analyzeSource(
        "枚举 颜色 { 红, 绿, 蓝, 自定义 = 100, 之后 }\n"
        "枚举 方向 { 上 = -1, 中, 下 }\n"
        "不安全 函数 主() -> 整32 { 返回 0; }\n");
    EXPECT_TRUE(r.ok) << r.messages;
    ASSERT_EQ(r.program->enums.size(), 2u);
    const auto& colors = r.program->enums[0];
    EXPECT_EQ(colors->members[0].value, 0);    // 红=0
    EXPECT_EQ(colors->members[1].value, 1);    // 绿=1
    EXPECT_EQ(colors->members[2].value, 2);    // 蓝=2
    EXPECT_EQ(colors->members[3].value, 100);  // 自定义=100
    EXPECT_EQ(colors->members[4].value, 101);  // 之后=101（自动递增）
    const auto& dirs = r.program->enums[1];
    EXPECT_EQ(dirs->members[0].value, -1);     // 上=-1
    EXPECT_EQ(dirs->members[1].value, 0);      // 中=0
    EXPECT_EQ(dirs->members[2].value, 1);      // 下=1
}

// 枚举使用：枚举名.成员 引用 + 枚举变量声明
TEST(StructSemanticTest, EnumUse) {
    SemanticResult r = analyzeSource(
        "枚举 颜色 { 红, 绿, 蓝 }\n"
        "不安全 函数 主() -> 整32 {\n"
        "  颜色 c = 颜色.绿;\n"
        "  整32 v = 颜色.蓝;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 枚举成员不存在报错
TEST(StructSemanticTest, EnumMemberNotFound) {
    SemanticResult r = analyzeSource(
        "枚举 颜色 { 红, 绿 }\n"
        "不安全 函数 主() -> 整32 {\n"
        "  整32 v = 颜色.紫;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("没有成员"), std::string::npos);
}

// 选择语句 case 使用枚举值（Task 2.7：情况 颜色.红）
TEST(StructSemanticTest, EnumInSwitch) {
    SemanticResult r = analyzeSource(
        "枚举 颜色 { 红, 绿, 蓝 }\n"
        "不安全 函数 主() -> 整32 {\n"
        "  颜色 c = 颜色.绿;\n"
        "  选择 (c) {\n"
        "    情况 颜色.红:\n"
        "      返回 1;\n"
        "    情况 颜色.绿:\n"
        "      返回 2;\n"
        "    默认:\n"
        "      返回 0;\n"
        "  }\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 结构体初始化列表：字段类型不匹配报错
TEST(StructSemanticTest, StructInitTypeMismatch) {
    SemanticResult r = analyzeSource(
        "结构体 点 { 整32 x; }\n"
        "不安全 函数 主() -> 整32 {\n"
        "  点 p = 点{ x = 1.5 };\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("无法将"), std::string::npos);
}

// 结构体初始化列表：字段不存在报错
TEST(StructSemanticTest, StructInitUnknownField) {
    SemanticResult r = analyzeSource(
        "结构体 点 { 整32 x; }\n"
        "不安全 函数 主() -> 整32 {\n"
        "  点 p = 点{ 不存在 = 1 };\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("没有字段"), std::string::npos);
}

// 嵌套结构体初始化：值类型可隐式转换
TEST(StructSemanticTest, NestedStructInit) {
    SemanticResult r = analyzeSource(
        "结构体 点 { 整32 x; 整32 y; }\n"
        "结构体 矩形 { 点 左上 点 右下; }\n"
        "不安全 函数 主() -> 整32 {\n"
        "  矩形 r = 矩形{ 左上 = 点{ x = 1, y = 2 }, 右下 = 点{ x = 3, y = 4 } };\n"
        "  整32 a = r.左上.x;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 循环引用检测：结构体直接包含自身（无限大小）报错
TEST(StructSemanticTest, SelfReference) {
    SemanticResult r = analyzeSource(
        "结构体 节点 { 节点 下一个; }\n"
        "不安全 函数 主() -> 整32 { 返回 0; }\n");
    // T3（306-a 波次2·用户批量裁决方案甲）：值字段递归=无穷大小类型，
    //   编译期拒绝（Rust E0072 同类）——原断言「允许不完整类型字段」随
    //   语义变更作废（2026-09-17 用户批量裁决·清零波次2）。
    EXPECT_FALSE(r.ok) << r.messages;
    EXPECT_NE(r.messages.find("无穷大小"), std::string::npos);
}

// 重复类型名声明报错
TEST(StructSemanticTest, DuplicateTypeName) {
    SemanticResult r = analyzeSource(
        "结构体 点 { 整32 x; }\n"
        "结构体 点 { 整32 y; }\n"
        "不安全 函数 主() -> 整32 { 返回 0; }\n");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.messages.find("重复声明类型"), std::string::npos);
}

// 联合体共享内存：写整字段后读浮字段（语义层允许，无类型错误）
TEST(StructSemanticTest, UnionFieldAccess) {
    SemanticResult r = analyzeSource(
        "联合体 数值 { 整32 整数部分; 浮64 浮点部分; }\n"
        "不安全 函数 主() -> 整32 {\n"
        "  数值 u = 数值{ 整数部分 = 42 };\n"
        "  整32 v = u.整数部分;\n"
        "  浮64 f = u.浮点部分;\n"
        "  返回 v;\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 结构体变量无初始化器：允许（字段未初始化，Task 2.7 不做未初始化检查）
TEST(StructSemanticTest, StructVarNoInit) {
    SemanticResult r = analyzeSource(
        "结构体 点 { 整32 x; }\n"
        "不安全 函数 主() -> 整32 {\n"
        "  点 p;\n"
        "  返回 0;\n"
        "}\n");
    EXPECT_TRUE(r.ok) << r.messages;
}
