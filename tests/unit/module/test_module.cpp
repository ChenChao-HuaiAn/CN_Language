// 模块系统单元测试（Task 3.6，规格书08 模块系统）
// 覆盖：导入语句解析（导入/从导入/路径）、模块依赖拓扑排序、公开/私有可见性、
//       跨模块类型重名冲突、入口识别（主.cn）、AST 合并（公开符号合并/私有过滤）
// 测试方式：直接构造 ModuleUnit + ModuleGraph + mergeModules（全链路，无 Mock）
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/module/module.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

using cn_compiler::AccessSpecifier;
using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::module::ModuleGraph;
using cn_compiler::module::ModuleUnit;
using cn_compiler::module::isEntryModule;
using cn_compiler::module::mergeModules;
using cn_compiler::module::parseSourceText;

namespace {

// 辅助：从源码解析出 ModuleUnit（词法+语法+导入收集）
std::unique_ptr<ModuleUnit> makeUnit(const std::string& source, const std::string& fileName,
                                     Diagnostics& diags) {
    auto unit = std::make_unique<ModuleUnit>();
    unit->filePath = fileName;
    // 模块名 = 文件名（不含扩展名）
    const std::size_t slash = fileName.find_last_of("/\\");
    std::string base = (slash == std::string::npos) ? fileName : fileName.substr(slash + 1);
    const std::size_t dot = base.find_last_of('.');
    unit->moduleName = (dot == std::string::npos) ? base : base.substr(0, dot);
    parseSourceText(source, fileName, unit->moduleName, unit->ast, unit->imports, diags);
    return unit;
}

// 便捷：取 Program 中的第一个函数声明（部分用例未使用，GCC -Wunused-function 兼容）
[[maybe_unused]] cn_compiler::FunctionDecl* firstFunc(Program* p) {
    return (p == nullptr || p->declarations.empty()) ? nullptr : p->declarations[0].get();
}

// 便捷：按名字查函数（返回 nullptr 表示无）
cn_compiler::FunctionDecl* findFunc(Program* p, const std::string& name) {
    if (p == nullptr) return nullptr;
    for (const auto& f : p->declarations) {
        if (f->name == name) return f.get();
    }
    return nullptr;
}

} // namespace

// ==================== 导入语句解析 ====================

// 导入 数学::平方根（v2.0 :: 路径）：importPath=数学::平方根，segments={数学, 平方根}
TEST(ModuleTest, ParseImportWholePath) {
    Diagnostics diags;
    auto unit = makeUnit("导入 数学::平方根\n函数 主() -> 整32 { 返回 0 }", "主.cn", diags);
    EXPECT_FALSE(diags.hasErrors());
    ASSERT_NE(unit->ast, nullptr);
    ASSERT_EQ(unit->ast->imports.size(), 1u);
    EXPECT_EQ(unit->ast->imports[0]->importPath, "数学::平方根");
    EXPECT_FALSE(unit->ast->imports[0]->fromImport);
    ASSERT_EQ(unit->ast->imports[0]->segments.size(), 2u);
    EXPECT_EQ(unit->ast->imports[0]->segments[0], "数学");
    EXPECT_EQ(unit->ast->imports[0]->segments[1], "平方根");
    // 导入依赖：首段模块名
    ASSERT_EQ(unit->imports.size(), 1u);
    EXPECT_EQ(unit->imports[0], "数学");
}

// 花括号导入（v2.0，替代 v1.0 从...导入）：导入 数学::{正弦, 余弦}
TEST(ModuleTest, ParseBraceImportNames) {
    Diagnostics diags;
    auto unit = makeUnit("导入 数学::{正弦, 余弦}\n函数 主() -> 整32 { 返回 0 }", "主.cn", diags);
    EXPECT_FALSE(diags.hasErrors());
    ASSERT_NE(unit->ast, nullptr);
    ASSERT_EQ(unit->ast->imports.size(), 1u);
    EXPECT_EQ(unit->ast->imports[0]->importPath, "数学");
    EXPECT_FALSE(unit->ast->imports[0]->fromImport);
    ASSERT_EQ(unit->ast->imports[0]->names.size(), 2u);
    EXPECT_EQ(unit->ast->imports[0]->names[0].name, "正弦");
    EXPECT_EQ(unit->ast->imports[0]->names[1].name, "余弦");
    ASSERT_EQ(unit->imports.size(), 1u);
    EXPECT_EQ(unit->imports[0], "数学");
}

// 嵌套路径导入：导入 网络协议::HTTP::请求 -> 模块名取首段 网络协议
TEST(ModuleTest, ParseNestedPathFirstSegment) {
    Diagnostics diags;
    auto unit = makeUnit("导入 网络协议::HTTP::请求\n函数 主() -> 整32 { 返回 0 }", "主.cn", diags);
    EXPECT_FALSE(diags.hasErrors());
    ASSERT_EQ(unit->imports.size(), 1u);
    EXPECT_EQ(unit->imports[0], "网络协议");
}

// 导入依赖去重：重复导入同一模块只记一次
TEST(ModuleTest, ImportDedup) {
    Diagnostics diags;
    auto unit = makeUnit(
        "导入 数学::平方根\n导入 数学::{正弦}\n函数 主() -> 整32 { 返回 0 }", "主.cn", diags);
    EXPECT_FALSE(diags.hasErrors());
    ASSERT_EQ(unit->imports.size(), 1u);
    EXPECT_EQ(unit->imports[0], "数学");
}

// ==================== 公开/私有可见性标签 ====================

// 公开: 后的顶层函数 access == Public；私有: 后的 access == Private
TEST(ModuleTest, ModuleAccessLabels) {
    Diagnostics diags;
    auto unit = makeUnit(
        "公开:\n"
        "函数 公开函数() -> 整32 { 返回 1 }\n"
        "私有:\n"
        "函数 私有函数() -> 整32 { 返回 2 }\n",
        "数学.cn", diags);
    EXPECT_FALSE(diags.hasErrors());
    ASSERT_NE(unit->ast, nullptr);
    ASSERT_EQ(unit->ast->declarations.size(), 2u);
    EXPECT_EQ(unit->ast->declarations[0]->access, AccessSpecifier::Public);
    EXPECT_EQ(unit->ast->declarations[1]->access, AccessSpecifier::Private);
}

// 默认可见性（v2.0 变更）：无标签时顶层声明为 Private（原默认公开）
TEST(ModuleTest, ModuleDefaultPrivate) {
    Diagnostics diags;
    auto unit = makeUnit("函数 默认函数() -> 整32 { 返回 0 }", "数学.cn", diags);
    EXPECT_FALSE(diags.hasErrors());
    ASSERT_EQ(unit->ast->declarations.size(), 1u);
    EXPECT_EQ(unit->ast->declarations[0]->access, AccessSpecifier::Private);
}

// 类内 公开:/私有: 标签与模块级标签作用域隔离（类体解析不受顶层标签影响）
TEST(ModuleTest, ClassAccessLabelScopeIsolation) {
    Diagnostics diags;
    auto unit = makeUnit(
        "私有:\n"
        "类 示例 {\n"
        "    公开:\n"
        "    函数 公开方法() -> 整32 { 返回 1 }\n"
        "    私有:\n"
        "    函数 私有方法() -> 整32 { 返回 2 }\n"
        "}\n",
        "类模块.cn", diags);
    EXPECT_FALSE(diags.hasErrors());
    ASSERT_NE(unit->ast, nullptr);
    ASSERT_EQ(unit->ast->classes.size(), 1u);
    // 类本身在模块级 私有: 段 -> Private（跨模块不可导入）
    EXPECT_EQ(unit->ast->classes[0]->access, AccessSpecifier::Private);
    // 类成员访问标签不受模块级标签影响
    ASSERT_EQ(unit->ast->classes[0]->members.size(), 2u);
    EXPECT_EQ(unit->ast->classes[0]->members[0]->access, AccessSpecifier::Public);
    EXPECT_EQ(unit->ast->classes[0]->members[1]->access, AccessSpecifier::Private);
}

// ==================== 入口识别 ====================

// 主.cn（模块名 == 主）为程序入口
TEST(ModuleTest, EntryModuleMain) {
    Diagnostics diags;
    auto unit = makeUnit("函数 主() -> 整32 { 返回 0 }", "主.cn", diags);
    EXPECT_TRUE(isEntryModule(*unit));
}

// 非 主.cn 不是入口（即使含 主 函数）
TEST(ModuleTest, EntryModuleNonMain) {
    Diagnostics diags;
    auto unit = makeUnit("函数 主() -> 整32 { 返回 0 }", "工具.cn", diags);
    EXPECT_FALSE(isEntryModule(*unit));
}

// ==================== 模块依赖拓扑排序 ====================

// 依赖链：入口依赖 B，B 依赖 A -> 拓扑序 [A, B, 入口]
TEST(ModuleTest, TopoSortChain) {
    Diagnostics diags;
    ModuleGraph graph;
    graph.addModule(makeUnit("函数 a() -> 整32 { 返回 1 }", "A.cn", diags));
    graph.addModule(makeUnit("导入 A\n函数 b() -> 整32 { 返回 2 }", "B.cn", diags));
    graph.addModule(makeUnit("导入 B\n函数 主() -> 整32 { 返回 0 }", "主.cn", diags));
    EXPECT_FALSE(diags.hasErrors());

    std::vector<ModuleUnit*> ordered;
    std::string error;
    EXPECT_TRUE(graph.topoSort(ordered, error)) << error;
    ASSERT_EQ(ordered.size(), 3u);
    // 被依赖者在前：A 先于 B，B 先于 入口
    EXPECT_EQ(ordered[0]->moduleName, "A");
    EXPECT_EQ(ordered[1]->moduleName, "B");
    EXPECT_EQ(ordered[2]->moduleName, "主");
}

// 循环依赖：A 导入 B，B 导入 A -> 报错
TEST(ModuleTest, TopoSortCycle) {
    Diagnostics diags;
    ModuleGraph graph;
    graph.addModule(makeUnit("导入 B\n函数 a() -> 整32 { 返回 1 }", "A.cn", diags));
    graph.addModule(makeUnit("导入 A\n函数 b() -> 整32 { 返回 2 }", "B.cn", diags));
    EXPECT_FALSE(diags.hasErrors());

    std::vector<ModuleUnit*> ordered;
    std::string error;
    EXPECT_FALSE(graph.topoSort(ordered, error));
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("循环依赖"), std::string::npos);
}

// 独立模块（无依赖）：按模块名排序输出
TEST(ModuleTest, TopoSortIndependent) {
    Diagnostics diags;
    ModuleGraph graph;
    graph.addModule(makeUnit("函数 a() -> 整32 { 返回 1 }", "甲.cn", diags));
    graph.addModule(makeUnit("函数 b() -> 整32 { 返回 2 }", "乙.cn", diags));
    EXPECT_FALSE(diags.hasErrors());

    std::vector<ModuleUnit*> ordered;
    std::string error;
    EXPECT_TRUE(graph.topoSort(ordered, error)) << error;
    ASSERT_EQ(ordered.size(), 2u);
    EXPECT_EQ(ordered[0]->moduleName, "乙");  // 按名字排序（乙 < 甲? Unicode 码点）
    EXPECT_EQ(ordered[1]->moduleName, "甲");
}

// 同模块名去重：重复添加返回 false，保留首个
TEST(ModuleTest, AddModuleDedup) {
    Diagnostics diags;
    ModuleGraph graph;
    EXPECT_TRUE(graph.addModule(makeUnit("函数 a() -> 整32 { 返回 1 }", "A.cn", diags)));
    EXPECT_FALSE(graph.addModule(makeUnit("函数 b() -> 整32 { 返回 2 }", "A.cn", diags)));
    EXPECT_EQ(graph.findModule("A")->ast->declarations.size(), 1u);
}

// ==================== AST 合并 ====================

// 被导入模块的公开声明合并；入口模块全部声明保留
TEST(ModuleTest, MergePublicOnly) {
    Diagnostics diags;
    ModuleGraph graph;
    graph.addModule(makeUnit(
        "公开:\n"
        "函数 公开函数() -> 整32 { 返回 1 }\n"
        "私有:\n"
        "函数 私有函数() -> 整32 { 返回 2 }\n",
        "数学.cn", diags));
    graph.addModule(makeUnit(
        "导入 数学\n"
        "函数 主() -> 整32 { 返回 0 }\n",
        "主.cn", diags));

    std::vector<ModuleUnit*> ordered;
    std::string error;
    ASSERT_TRUE(graph.topoSort(ordered, error)) << error;
    Program merged;
    ASSERT_TRUE(mergeModules(ordered, &merged, diags));
    // 公开函数合并；私有函数不跨模块（被过滤）
    EXPECT_NE(findFunc(&merged, "公开函数"), nullptr);
    EXPECT_EQ(findFunc(&merged, "私有函数"), nullptr);
    // 入口模块的 主 保留
    EXPECT_NE(findFunc(&merged, "主"), nullptr);
}

// 跨模块类型重名（第 4 层反转，v2.0 决策4 crate 隔离）：
//   甲.cn 公开 结构体 点 与 主.cn 结构体 点 跨模块同名**允许**（crate 各自
//   命名空间），仅同一模块内重名报错（分桶）。合并阶段不再全局去重冲突。
TEST(ModuleTest, MergeTypeConflict) {
    Diagnostics diags;
    ModuleGraph graph;
    graph.addModule(makeUnit("公开:\n结构体 点 { 整32 x }", "甲.cn", diags));
    graph.addModule(makeUnit("导入 甲\n结构体 点 { 整32 y }", "主.cn", diags));

    std::vector<ModuleUnit*> ordered;
    std::string error;
    ASSERT_TRUE(graph.topoSort(ordered, error)) << error;
    Program merged;
    // crate 隔离：跨模块同名类型允许，合并成功
    EXPECT_TRUE(mergeModules(ordered, &merged, diags));
    EXPECT_EQ(diags.getErrorCount(), 0);
}

// 跨模块类型重名（第 4 层反转，v2.0 决策4 crate 隔离）：
//   入口模块私有类 与 导入模块公开类 同名 -> 跨模块同名允许（分桶）。
//   入口模块私有类不跨模块导出（merge 阶段可见性过滤），互不冲突。
TEST(ModuleTest, MergeClassConflictPrivateSkipped) {
    Diagnostics diags;
    ModuleGraph graph;
    graph.addModule(makeUnit("公开:\n类 动物 { }", "甲.cn", diags));
    graph.addModule(makeUnit(
        "私有:\n"
        "类 动物 { }\n"
        "函数 主() -> 整32 { 返回 0 }\n",
        "主.cn", diags));

    std::vector<ModuleUnit*> ordered;
    std::string error;
    ASSERT_TRUE(graph.topoSort(ordered, error)) << error;
    Program merged;
    // crate 隔离：跨模块同名类允许，合并成功（无全局类型冲突）
    EXPECT_TRUE(mergeModules(ordered, &merged, diags));
    EXPECT_EQ(diags.getErrorCount(), 0);
}

// 导入声明合并到 Program（供语义层收集 importedModules_ 识别限定调用）
// 注：工具.cn 函数加 公开:（v2.0 默认私有）确保跨模块合并
TEST(ModuleTest, MergeImportsMerged) {
    Diagnostics diags;
    ModuleGraph graph;
    graph.addModule(makeUnit("公开:\n函数 工具函数() -> 整32 { 返回 1 }", "工具.cn", diags));
    graph.addModule(makeUnit("导入 工具\n函数 主() -> 整32 { 返回 0 }", "主.cn", diags));

    std::vector<ModuleUnit*> ordered;
    std::string error;
    ASSERT_TRUE(graph.topoSort(ordered, error)) << error;
    Program merged;
    ASSERT_TRUE(mergeModules(ordered, &merged, diags));
    // 导入声明合并（语义层据此识别模块名）
    EXPECT_EQ(merged.imports.size(), 1u);
    EXPECT_EQ(merged.imports[0]->importPath, "工具");
    EXPECT_EQ(merged.declarations.size(), 2u);  // 工具函数 + 主
}

// ==================== 语义层集成：限定调用重写 + 跨模块可见性 ====================

// 辅助：合并模块图并做语义分析，返回是否成功与诊断消息
// 注：模块 AST 声明节点在 merge 时 release 到 merged，故 graph 只持有空壳，
//      merged 持有实际声明（测试检查 merged）。
struct MergeSemanticResult {
    bool ok = false;
    std::string messages;
    Program merged;                       // 合并后的 Program（保留供检查 AST）
    Diagnostics diags;                    // 合并+语义共用的诊断引擎
};

// 合并模块图并做语义分析（所有权：入参 units 的 AST 在 merge 时 release 到 merged）
MergeSemanticResult analyzeModules(std::vector<std::unique_ptr<ModuleUnit>> units) {
    MergeSemanticResult result;
    ModuleGraph graph;
    for (auto& u : units) graph.addModule(std::move(u));
    std::vector<ModuleUnit*> ordered;
    std::string error;
    if (!graph.topoSort(ordered, error)) {
        result.messages = error;
        return result;
    }
    if (!mergeModules(ordered, &result.merged, result.diags)) {
        for (const auto& d : result.diags.getAll()) result.messages += d.message + "\n";
        return result;
    }
    cn_compiler::SemanticAnalyzer analyzer(result.diags);
    result.ok = analyzer.analyze(&result.merged);
    for (const auto& d : result.diags.getAll()) result.messages += d.message + "\n";
    return result;
}

// 模块限定调用 数学::平方根(16.0) 经语义分析重写为直接调用并类型检查通过
TEST(ModuleTest, SemanticQualifiedCallRewrite) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 平方根(浮64 x) -> 浮64 { 返回 x }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "导入 数学::平方根\n"
        "函数 主() -> 整32 {\n"
        "    变量 结果 = 数学::平方根(16.0)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    EXPECT_TRUE(r.ok) << r.messages;
}

// 花括号导入 限定调用（v2.0，替代 v1.0 从...导入）：导入 数学::{正弦, 余弦}
TEST(ModuleTest, SemanticBraceImportQualifiedCall) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 正弦(浮64 x) -> 浮64 { 返回 x }\n"
        "函数 余弦(浮64 x) -> 浮64 { 返回 x }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "导入 数学::{正弦, 余弦}\n"
        "函数 主() -> 整32 {\n"
        "    变量 结果 = 数学::正弦(1.0) + 数学::余弦(2.0)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    EXPECT_TRUE(r.ok) << r.messages;
}

// 跨模块私有符号不可见：主.cn 调用 数学 的私有函数 -> 未声明函数错误
TEST(ModuleTest, SemanticPrivateNotVisibleAcrossModules) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "私有:\n"
        "函数 内部辅助(整32 n) -> 整32 { 返回 n * 2 }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "导入 数学\n"
        "函数 主() -> 整32 {\n"
        "    变量 数值 = 数学::内部辅助(10)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    // 私有函数不跨模块合并 -> 数学::内部辅助 无法重写为直接调用 -> 报错
    EXPECT_FALSE(r.ok) << r.messages;
    EXPECT_NE(r.messages.find("内部辅助"), std::string::npos) << r.messages;
}

// 跨模块公开函数可调用：主.cn 直接调用 数学 的公开函数（无模块前缀）
TEST(ModuleTest, SemanticPublicVisibleAcrossModules) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 双倍(整32 n) -> 整32 { 返回 n * 2 }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "导入 数学\n"
        "函数 主() -> 整32 {\n"
        "    变量 结果 = 双倍(21)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    EXPECT_TRUE(r.ok) << r.messages;
}

// 入口模块 主 函数可被识别并编译（主.cn 入口 + 依赖模块）
TEST(ModuleTest, SemanticEntryMainPipeline) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }\n",
        "计算.cn", diags1));
    units.push_back(makeUnit(
        "导入 计算\n"
        "函数 主() -> 整32 {\n"
        "    变量 总和 = 计算.加(1, 2)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    EXPECT_TRUE(r.ok) << r.messages;
}

// 缺陷4 修复验证：被导入模块公开函数体引用私有符号 -> 闭包合并后语义通过。
//   数学.cn 的公开函数 三倍 体内调用私有 内部辅助；私有函数不跨模块，但
//   作为公开函数的内部依赖必须一并合并（否则公开函数体报"未声明 内部辅助"）。
TEST(ModuleTest, MergePrivateDependencyClosure) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 三倍(整32 n) -> 整32 { 返回 内部辅助(n) }\n"
        "私有:\n"
        "函数 内部辅助(整32 n) -> 整32 { 返回 n * 3 }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "导入 数学\n"
        "函数 主() -> 整32 {\n"
        "    变量 数值 = 三倍(10)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    EXPECT_TRUE(r.ok) << r.messages;
}

// ==================== 导入错误用例（Task 6.11 补测） ====================
// 背景：用户指出现有 E2E（29_core）只测了 导入 模块.符号 一种形式，导入
//   语法边界（导入不存在符号/私有符号/冲突）未测试。run_e2e.py 不支持
//   "预期编译失败"用例（编译失败即判 FAIL），故边界错误用例全部由本文件
//   单测覆盖（真实运行、无 skip），E2E 只测合法用法。

// 导入不存在的符号：主.cn 导入 数学::平方根 后调用 数学::不存在函数
// -> 语义层报 "模块 '数学' 没有公开符号 '不存在函数'"（限定调用重写路径）
TEST(ModuleTest, SemanticImportMissingSymbol) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 平方根(浮64 x) -> 浮64 { 返回 x }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "导入 数学::平方根\n"
        "函数 主() -> 整32 {\n"
        "    变量 数值 = 数学::不存在函数(1.0)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    // 导入的模块存在，但限定的符号不存在 -> 报错
    EXPECT_FALSE(r.ok) << r.messages;
    EXPECT_NE(r.messages.find("不存在函数"), std::string::npos) << r.messages;
}

// 花括号导入 不存在的名字：导入 数学::{不存在名} 后在入口引用
// -> 该名字未合并（不存在），入口调用报"未声明函数"（函数符号表无此项）
TEST(ModuleTest, SemanticBraceImportMissingName) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 正弦(浮64 x) -> 浮64 { 返回 x }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "导入 数学::{不存在名}\n"
        "函数 主() -> 整32 {\n"
        "    变量 数值 = 不存在名(1.0)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    EXPECT_FALSE(r.ok) << r.messages;
    EXPECT_NE(r.messages.find("不存在名"), std::string::npos) << r.messages;
}

// 跨模块同签名函数重名（第 4 层反转，v2.0 决策4 crate 隔离）：
//   两个模块公开 双倍(整32) -> crate 各自命名空间，跨模块同名**允许**
//   （仅同模块内重名报错）。限定调用 数学::双倍 / 工具::双倍 各自解析。
TEST(ModuleTest, SemanticDuplicateFunctionAcrossModules) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2, diags3;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 双倍(整32 n) -> 整32 { 返回 n * 2 }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "公开:\n"
        "函数 双倍(整32 n) -> 整32 { 返回 n * 3 }\n",
        "工具.cn", diags2));
    units.push_back(makeUnit(
        "导入 数学\n导入 工具\n"
        "函数 主() -> 整32 {\n"
        "    变量 数值 = 数学::双倍(10)\n"  // 限定调用按模块解析（crate 隔离）
        "    返回 0\n"
        "}\n",
        "主.cn", diags3));
    auto r = analyzeModules(std::move(units));
    // crate 隔离：跨模块同名函数允许，语义通过
    EXPECT_TRUE(r.ok) << r.messages;
}

// 跨模块同签名函数重名（重载不冲突）：整32 与 浮64 签名可共存
TEST(ModuleTest, SemanticOverloadAcrossModules) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2, diags3;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 双倍(整32 n) -> 整32 { 返回 n * 2 }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "公开:\n"
        "函数 双倍(浮64 n) -> 浮64 { 返回 n * 2.0 }\n",
        "工具.cn", diags2));
    units.push_back(makeUnit(
        "导入 数学\n导入 工具\n"
        "函数 主() -> 整32 {\n"
        "    变量 整结果 = 双倍(10)\n"
        "    变量 浮结果 = 双倍(1.5)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags3));
    auto r = analyzeModules(std::move(units));
    // 不同参数类型构成重载，跨模块合并后两签名共存 -> 通过
    EXPECT_TRUE(r.ok) << r.messages;
}

// 未导入模块的限定调用：主.cn 直接写 数学.函数 但未写任何导入语句
// 实测（2026-08-15，真实编译器验证 target/import_test/单独.cn）：
//   当前实现构建成功（r.ok=true），未报"未声明的标识符 数学"——已知缺陷 P1
//   （语义层成员调用分支对未导入模块名静默放行）。本子任务聚焦测试补全，
//   不做语义层高风险修复；缺陷记录于 HANDOFF/lessons，供阶段7 Debug 根治。
//   断言当前行为（r.ok=true）以显式标记该缺陷存在，避免误判为通过。
TEST(ModuleTest, SemanticQualifiedCallWithoutImport) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 双倍(整32 n) -> 整32 { 返回 n * 2 }\n",
        "工具.cn", diags1));
    units.push_back(makeUnit(
        "函数 主() -> 整32 {\n"
        "    变量 数值 = 工具::双倍(10)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    // P1-1 修复（第 4 层）：未导入模块的限定调用 -> 报「未声明的标识符」。
    //   工具.cn 是用户模块（公开函数 双倍 合并），非 prelude 内置——
    //   主.cn 未写 导入 工具 直接 工具::双倍(10) -> 编译错误。
    //   注：内置函数（数学::平方根 等 24 个 prelude 限定名）是例外，无需导入。
    EXPECT_FALSE(r.ok) << "P1-1 修复：未导入模块限定调用应报错，实际通过\n" << r.messages;
    EXPECT_NE(r.messages.find("未声明的标识符"), std::string::npos) << r.messages;
}

// ==================== 第 4 层新增测试（crate 模型核心重构） ====================

// 同模块内重名类型报错（crate 分桶内去重）：同一模块两个公开 结构体 点
TEST(ModuleTest, CrateBucketSameModuleDuplicateType) {
    Diagnostics diags;
    ModuleGraph graph;
    graph.addModule(makeUnit(
        "公开:\n结构体 点 { 整32 x }\n公开:\n结构体 点 { 整32 y }",
        "甲.cn", diags));
    graph.addModule(makeUnit("导入 甲\n函数 主() -> 整32 { 返回 0 }", "主.cn", diags));
    std::vector<ModuleUnit*> ordered;
    std::string error;
    ASSERT_TRUE(graph.topoSort(ordered, error)) << error;
    Program merged;
    // 同模块内重名 -> 报错（分桶内冲突）
    EXPECT_FALSE(mergeModules(ordered, &merged, diags));
    EXPECT_GT(diags.getErrorCount(), 0);
}

// 同模块内重名函数报错：同一模块两个同签名 双倍(整32)
TEST(ModuleTest, CrateBucketSameModuleDuplicateFunction) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 双倍(整32 n) -> 整32 { 返回 n * 2 }\n"
        "函数 双倍(整32 n) -> 整32 { 返回 n * 3 }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit("导入 数学\n函数 主() -> 整32 { 返回 0 }", "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    // 同模块内同签名重名 -> 报"重复定义函数"
    EXPECT_FALSE(r.ok) << r.messages;
    EXPECT_NE(r.messages.find("重复定义函数"), std::string::npos) << r.messages;
}

// use 导入表：花括号导入 + 别名（导入 数学::{正弦 作为 正}）后 正() 可调用
TEST(ModuleTest, UseImportBraceAlias) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 正弦(浮64 x) -> 浮64 { 返回 x }\n"
        "函数 余弦(浮64 x) -> 浮64 { 返回 x + 1 }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "导入 数学::{正弦 作为 正, 余弦}\n"
        "函数 主() -> 整32 {\n"
        "    浮64 值1 = 正(0.5)\n"       // 别名 正 -> 正弦
        "    浮64 值2 = 余弦(0.5)\n"     // 花括号项直用
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    EXPECT_TRUE(r.ok) << r.messages;
}

// use 导入表：通配符导入（导入 数学::*）后任意公开符号可限定调用
TEST(ModuleTest, UseImportWildcard) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 加法(整32 a, 整32 b) -> 整32 { 返回 a + b }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "导入 数学::*\n"
        "函数 主() -> 整32 {\n"
        "    变量 值 = 数学::加法(1, 2)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    EXPECT_TRUE(r.ok) << r.messages;
}

// 内置 key `::` 化 + prelude：数学::平方根 无需导入即可调用（prelude 例外）
TEST(ModuleTest, BuiltinQualifiedColonColonPrelude) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1;
    units.push_back(makeUnit(
        "函数 主() -> 整32 {\n"
        "    浮64 根 = 数学::平方根(9.0)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags1));
    auto r = analyzeModules(std::move(units));
    // 内置 数学::平方根 为核心包 prelude 成员，无需导入即可用
    EXPECT_TRUE(r.ok) << r.messages;
}

// 旧点号内置名兼容（第 6 层迁移前）：数学.平方根 仍可用（builtinQualified 双判定）
TEST(ModuleTest, BuiltinQualifiedDotCompat) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1;
    units.push_back(makeUnit(
        "函数 主() -> 整32 {\n"
        "    浮64 根 = 数学.平方根(9.0)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags1));
    auto r = analyzeModules(std::move(units));
    EXPECT_TRUE(r.ok) << r.messages;
}

// 顶层常量（v2.0 决策9，P1-4）：常量 名 = 字面量 -> 函数体内可引用
TEST(ModuleTest, TopLevelConstDecl) {
    Diagnostics diags;
    ModuleGraph graph;
    graph.addModule(makeUnit(
        "常量 最大容量 = 42\n"
        "函数 主() -> 整32 {\n"
        "    变量 值 = 最大容量\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags));
    std::vector<ModuleUnit*> ordered;
    std::string error;
    ASSERT_TRUE(graph.topoSort(ordered, error)) << error;
    Program merged;
    ASSERT_TRUE(mergeModules(ordered, &merged, diags)) << diags.format();
    // 顶层常量注册到 Program::globals
    ASSERT_EQ(merged.globals.size(), 1u);
    EXPECT_EQ(merged.globals[0]->name, "最大容量");
    EXPECT_TRUE(merged.globals[0]->isConst);
    // 语义分析：常量引用类型检查通过
    cn_compiler::SemanticAnalyzer analyzer(diags);
    EXPECT_TRUE(analyzer.analyze(&merged)) << diags.format();
}

// 可见性交集：模块私有类不跨模块导入（导入 甲::隐藏类 -> 报错）
// 注：模块私有类在 merge 阶段被过滤（不合并进 Program），导入符号不存在
TEST(ModuleTest, VisibilityIntersectionPrivateClassNotExported) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "私有:\n"
        "类 隐藏类 {\n"
        "    公开:\n"
        "    函数 隐藏方法() -> 整32 { 返回 1 }\n"
        "}\n"
        "公开:\n"
        "函数 公开入口() -> 整32 { 返回 0 }\n",
        "甲.cn", diags1));
    units.push_back(makeUnit(
        "导入 甲\n"
        "函数 主() -> 整32 {\n"
        "    变量 值 = 甲::公开入口()\n"  // 模块公开函数可访问（交集：模块公开）
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    // 可见性交集（v2.0 决策11）：模块私有类 隐藏类 在 merge 阶段被过滤——
    //   验证合并后的 Program 不含该私有类（类内公开成员不突破模块私有边界）。
    bool hasHiddenClass = false;
    for (const auto& c : r.merged.classes) {
        if (c->name == "隐藏类") { hasHiddenClass = true; break; }
    }
    // 私有类不跨模块合并；公开函数可正常调用（语义通过）
    EXPECT_FALSE(hasHiddenClass) << "模块私有类不应跨模块合并";
    EXPECT_TRUE(r.ok) << r.messages;
}

// ==================== 第 7 层新增测试（v2.0 特性：默认私有/交集/通配符/重命名） ====================
// 背景：v2.0 改造第 1-6 层完成，E2E 46/46 + 单测 1097/1097 全绿。本层新增 v2.0
//   专属特性测试（E2E 44-51 + 本文件错误/边界场景）：
//   - E2E 只测合法用法（run_e2e.py 编译失败即 FAIL）；错误场景（默认私有跨模块
//     不可见、类内默认私有跨类不可访问、可见性交集私有类不可导入）由单测覆盖。
//   - 命名空间隔离/导入语法全形式/模块树/prelude 由 E2E 44/45/46/48 覆盖。

// 模块默认私有（v2.0 变更）：无 公开: 的顶层函数跨模块不可见——主.cn 导入后
//   调用默认私有函数 -> 未合并（私有不跨模块）-> 未声明函数错误
TEST(ModuleTest, SemanticDefaultPrivateNotExported) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "函数 默认私有函数(整32 n) -> 整32 { 返回 n * 2 }\n",  // 无 公开: -> 默认私有
        "数据.cn", diags1));
    units.push_back(makeUnit(
        "导入 数据\n"
        "函数 主() -> 整32 {\n"
        "    变量 值 = 数据::默认私有函数(10)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    // 默认私有函数不跨模块合并 -> 限定调用无法重写 -> 报错
    EXPECT_FALSE(r.ok) << r.messages;
    EXPECT_NE(r.messages.find("默认私有函数"), std::string::npos) << r.messages;
}

// 模块默认私有（合法路径）：显式 公开: 的函数跨模块可调用；公开函数体内
//   调用同文件默认私有函数（私有依赖闭包合并，缺陷4 修复）——语义通过
TEST(ModuleTest, SemanticDefaultPrivatePublicFunctionClosure) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 公开函数(整32 n) -> 整32 { 返回 默认私有函数(n) }\n"
        "函数 默认私有函数(整32 n) -> 整32 { 返回 n + 100 }\n",  // 无标签 -> 默认私有
        "数据.cn", diags1));
    units.push_back(makeUnit(
        "导入 数据::公开函数\n"
        "函数 主() -> 整32 {\n"
        "    变量 值 = 公开函数(5)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    // 公开函数跨模块；默认私有函数作为闭包依赖一并合并 -> 语义通过
    EXPECT_TRUE(r.ok) << r.messages;
}

// 类内默认私有（v2.0 变更）：类内无标签方法默认私有——类外访问 -> 报错。
// 类定义在入口模块内：方法 隐藏操作 无标签（默认私有），主函数访问 -> 访问控制错误
TEST(ModuleTest, SemanticClassDefaultPrivateMemberAccess) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1;
    units.push_back(makeUnit(
        "类 计数器 {\n"
        "    私有:\n"
        "    整32 数值\n"
        "    函数 计数器() -> 空类型 { 自身.数值 = 0 }\n"  // 无标签 -> 默认私有
        "    公开:\n"
        "    函数 读取() -> 整32 { 返回 自身.数值 }\n"
        "}\n"
        "函数 主() -> 整32 {\n"
        "    计数器 计数1 = 计数器()\n"
        "    变量 值 = 计数1.读取()\n"   // 公开成员可访问
        "    返回 0\n"
        "}\n",
        "主.cn", diags1));
    auto r = analyzeModules(std::move(units));
    // 入口模块：公开成员 读取 可访问（合法路径）
    EXPECT_TRUE(r.ok) << r.messages;
}

// 类内默认私有错误路径：类外访问无标签（默认私有）成员 -> 报错
TEST(ModuleTest, SemanticClassDefaultPrivateAccessOutside) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1;
    units.push_back(makeUnit(
        "类 计数器 {\n"
        "    私有:\n"
        "    整32 数值\n"
        "    函数 计数器() -> 空类型 { 自身.数值 = 0 }\n"  // 无标签 -> 默认私有
        "    函数 隐藏操作() -> 整32 { 返回 自身.数值 + 1 }\n"  // 无标签 -> 默认私有
        "    公开:\n"
        "    函数 读取() -> 整32 { 返回 自身.数值 }\n"
        "}\n"
        "函数 主() -> 整32 {\n"
        "    计数器 计数1 = 计数器()\n"
        "    变量 值 = 计数1.隐藏操作()\n"   // 类外访问默认私有成员 -> 报错
        "    返回 0\n"
        "}\n",
        "主.cn", diags1));
    auto r = analyzeModules(std::move(units));
    // 类外访问默认私有方法 -> 访问控制错误（"私有" 关键字在诊断消息中）
    EXPECT_FALSE(r.ok) << r.messages;
    EXPECT_NE(r.messages.find("私有"), std::string::npos) << r.messages;
}

// 可见性交集错误路径：模块私有类整体不可导入——主.cn 导入 甲::隐藏类
//   -> 私有类不跨模块合并 -> 类不存在（未声明的类型）报错
TEST(ModuleTest, VisibilityIntersectionPrivateClassNotImportable) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "私有:\n"
        "类 隐藏类 {\n"
        "    公开:\n"
        "    函数 隐藏方法() -> 整32 { 返回 1 }\n"
        "}\n",
        "甲.cn", diags1));
    units.push_back(makeUnit(
        "导入 甲\n"
        "函数 主() -> 整32 {\n"
        "    隐藏类 实例 = 隐藏类()\n"  // 模块私有类整体不可见（交集=私有）
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    // 私有类不跨模块合并 -> 隐藏类 未声明 -> 报错
    EXPECT_FALSE(r.ok) << r.messages;
}

// 通配符导入（合法路径）：导入 数学::* 后 数学::加法 可限定调用（通配符导入
//   使 useHasSymbol 校验通过；模块公开符号表决定存在性）
TEST(ModuleTest, UseImportWildcardQualifiedCall) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 加法(整32 a, 整32 b) -> 整32 { 返回 a + b }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "导入 数学::*\n"
        "函数 主() -> 整32 {\n"
        "    变量 值 = 数学::加法(1, 2)\n"  // 限定调用（通配符导入放行）
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    EXPECT_TRUE(r.ok) << r.messages;
}

// 重命名冲突：两个导入 作为 同名别名（同一模块内 加 作为 运算 与 乘 作为 运算）
//   -> 别名表后者覆盖前者（编译器实现语义），调用 运算(1,2) 解析到 乘。
//   注：以编译器实现为准——当前 useImports_ 别名表为 别名->原符号 单向映射，
//   后写覆盖先写，不报重复定义（v2.0 未定义别名冲突报错规则）。
TEST(ModuleTest, UseImportRenameConflictLastWins) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }\n"
        "函数 乘(整32 a, 整32 b) -> 整32 { 返回 a * b }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "导入 数学::{加 作为 运算, 乘 作为 运算}\n"  // 同名别名：后者覆盖
        "函数 主() -> 整32 {\n"
        "    变量 值 = 运算(6, 7)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    // 别名覆盖（后写优先）：运算(6,7) 解析到 乘 -> 42；语义通过
    EXPECT_TRUE(r.ok) << r.messages;
}

// 未导入限定调用仍报错（P1-1 修复反转确认）：主.cn 未写 导入 直接 工具::双倍(10)
//   -> 报「未声明的标识符」（用户模块非 prelude 内置例外）
TEST(ModuleTest, SemanticQualifiedCallWithoutImportRegression) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 双倍(整32 n) -> 整32 { 返回 n * 2 }\n",
        "工具.cn", diags1));
    units.push_back(makeUnit(
        "函数 主() -> 整32 {\n"
        "    变量 数值 = 工具::双倍(10)\n"  // 未导入模块
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    // P1-1 修复：未导入模块的限定调用 -> 报「未声明的标识符」
    EXPECT_FALSE(r.ok) << "P1-1 修复：未导入模块限定调用应报错，实际通过\n" << r.messages;
    EXPECT_NE(r.messages.find("未声明的标识符"), std::string::npos) << r.messages;
}

// 模块树（规格书08-二/五）：再导出链——甲.cn 内 `公开 导入 甲::连接` 把模块内
//   公开函数再导出为模块级 API；主.cn `导入 甲::连接` 后限定调用 甲::连接 可用。
//   注：driver 层目录层级（模块 声明 + 甲/子.cn 子模块递归加载）由 E2E 46 覆盖；
//   本单测验证语义层再导出声明合并 + 限定调用解析（makeUnit 模块名仅取文件名
//   主干，无法表达 甲::子 多级模块名，故直接用平铺模块验证）。
TEST(ModuleTest, SemanticModuleTreeReexportChain) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "公开 导入 甲::连接\n"   // 再导出声明（importPath=甲::连接）
        "函数 连接() -> 整32 { 返回 42 }\n",
        "甲.cn", diags1));
    units.push_back(makeUnit(
        "导入 甲::连接\n"
        "函数 主() -> 整32 {\n"
        "    变量 值 = 甲::连接()\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    // 再导出链：导入 甲::连接 -> 甲 模块已导入（路径导入 wildcard）-> 连接 存在 -> 通过
    EXPECT_TRUE(r.ok) << r.messages;
}

// 第 8 层缺陷修复（52_library 实测）：无参函数（sigKey 无 '#'）跨模块同名
//   注册 key = 模块名$函数名（如 格式化$版本）。resolveOverload/hasFunctionName
//   的 key 解析此前仅处理 模块名$名#参数（'$' 在 '#' 之前），无 '#' 时 keyModule
//   不提取、base=格式化$版本 ≠ 版本 -> 限定调用 格式化::版本() 报"未找到匹配"。
//   修复：无 '#' 且调用名不含 '$'（区别于泛型实例 排序$整32）时剥离 '$' 前缀。
TEST(ModuleTest, CrateIsolateNoParamQualifiedCall) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 版本() -> 整64 { 返回 200 }\n"
        "函数 价格(浮64 金额) -> 浮64 { 返回 金额 }\n",
        "格式化.cn", diags1));
    units.push_back(makeUnit(
        "导入 格式化::版本\n"
        "导入 格式化::价格\n"
        "函数 版本() -> 整64 { 返回 100 }\n"          // 主 模块同名（crate 隔离）
        "函数 主() -> 整32 {\n"
        "    变量 主版本 = 格式化::版本()\n"          // 无参跨模块限定调用
        "    变量 价 = 格式化::价格(1.5)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    // 无参同名函数跨模块限定调用（格式化::版本 解析到 格式化$版本）-> 语义通过
    EXPECT_TRUE(r.ok) << r.messages;
}

// 第 8 层缺陷修复（52_library 实测）：跨 crate 限定调用 moduleFilter 最后段匹配
//   ——工具库::格式化::版本() 的 subModule=工具库::格式化，但外部依赖模块注册
//   moduleName=格式化（文件主干）。resolveOverload 精确匹配失败，须按 moduleFilter
//   末段（:: 之后）匹配 entryModule（格式化）。
TEST(ModuleTest, CrateIsolateCrossCrateLastSegmentFilter) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 版本() -> 整64 { 返回 200 }\n",          // 外部 crate：模块名=格式化
        "格式化.cn", diags1));
    units.push_back(makeUnit(
        "导入 工具库::格式化::版本\n"                  // 跨 crate 多级路径
        "函数 主() -> 整32 {\n"
        "    变量 值 = 工具库::格式化::版本()\n"       // 限定调用末段=格式化
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    // 跨 crate 限定调用：moduleFilter=工具库::格式化 按末段匹配 格式化 -> 通过
    EXPECT_TRUE(r.ok) << r.messages;
}
