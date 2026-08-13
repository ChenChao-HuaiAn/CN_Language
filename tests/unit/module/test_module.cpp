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

// 便捷：取 Program 中的第一个函数声明
cn_compiler::FunctionDecl* firstFunc(Program* p) {
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

// 导入 数学.平方根：importPath=数学.平方根，fromImport=false
TEST(ModuleTest, ParseImportWholePath) {
    Diagnostics diags;
    auto unit = makeUnit("导入 数学.平方根\n函数 主() -> 整32 { 返回 0 }", "主.cn", diags);
    EXPECT_FALSE(diags.hasErrors());
    ASSERT_NE(unit->ast, nullptr);
    ASSERT_EQ(unit->ast->imports.size(), 1u);
    EXPECT_EQ(unit->ast->imports[0]->importPath, "数学.平方根");
    EXPECT_FALSE(unit->ast->imports[0]->fromImport);
    // 导入依赖：首段模块名
    ASSERT_EQ(unit->imports.size(), 1u);
    EXPECT_EQ(unit->imports[0], "数学");
}

// 从 数学 导入 正弦, 余弦：fromImport=true，names=[正弦, 余弦]
TEST(ModuleTest, ParseFromImportNames) {
    Diagnostics diags;
    auto unit = makeUnit("从 数学 导入 正弦, 余弦\n函数 主() -> 整32 { 返回 0 }", "主.cn", diags);
    EXPECT_FALSE(diags.hasErrors());
    ASSERT_NE(unit->ast, nullptr);
    ASSERT_EQ(unit->ast->imports.size(), 1u);
    EXPECT_EQ(unit->ast->imports[0]->importPath, "数学");
    EXPECT_TRUE(unit->ast->imports[0]->fromImport);
    ASSERT_EQ(unit->ast->imports[0]->names.size(), 2u);
    EXPECT_EQ(unit->ast->imports[0]->names[0], "正弦");
    EXPECT_EQ(unit->ast->imports[0]->names[1], "余弦");
    ASSERT_EQ(unit->imports.size(), 1u);
    EXPECT_EQ(unit->imports[0], "数学");
}

// 嵌套路径导入：导入 网络协议.HTTP.请求 -> 模块名取首段 网络协议
TEST(ModuleTest, ParseNestedPathFirstSegment) {
    Diagnostics diags;
    auto unit = makeUnit("导入 网络协议.HTTP.请求\n函数 主() -> 整32 { 返回 0 }", "主.cn", diags);
    EXPECT_FALSE(diags.hasErrors());
    ASSERT_EQ(unit->imports.size(), 1u);
    EXPECT_EQ(unit->imports[0], "网络协议");
}

// 导入依赖去重：重复导入同一模块只记一次
TEST(ModuleTest, ImportDedup) {
    Diagnostics diags;
    auto unit = makeUnit(
        "导入 数学.平方根\n从 数学 导入 正弦\n函数 主() -> 整32 { 返回 0 }", "主.cn", diags);
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

// 默认可见性：无标签时顶层声明为 Public
TEST(ModuleTest, ModuleDefaultPublic) {
    Diagnostics diags;
    auto unit = makeUnit("函数 默认函数() -> 整32 { 返回 0 }", "数学.cn", diags);
    EXPECT_FALSE(diags.hasErrors());
    ASSERT_EQ(unit->ast->declarations.size(), 1u);
    EXPECT_EQ(unit->ast->declarations[0]->access, AccessSpecifier::Public);
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

// 跨模块类型重名（结构体）报错
TEST(ModuleTest, MergeTypeConflict) {
    Diagnostics diags;
    ModuleGraph graph;
    graph.addModule(makeUnit("结构体 点 { 整32 x }", "甲.cn", diags));
    graph.addModule(makeUnit("导入 甲\n结构体 点 { 整32 y }", "主.cn", diags));

    std::vector<ModuleUnit*> ordered;
    std::string error;
    ASSERT_TRUE(graph.topoSort(ordered, error)) << error;
    Program merged;
    EXPECT_FALSE(mergeModules(ordered, &merged, diags));
    EXPECT_GT(diags.getErrorCount(), 0);
}

// 跨模块类型重名（入口模块私有类 与 导入模块公开类）报错：
//   两者合并到同一 Program 后语义层类型名冲突（类型表全局唯一）。
TEST(ModuleTest, MergeClassConflictPrivateSkipped) {
    Diagnostics diags;
    ModuleGraph graph;
    graph.addModule(makeUnit("类 动物 { }", "甲.cn", diags));
    graph.addModule(makeUnit(
        "私有:\n"
        "类 动物 { }\n"
        "函数 主() -> 整32 { 返回 0 }\n",
        "主.cn", diags));

    std::vector<ModuleUnit*> ordered;
    std::string error;
    ASSERT_TRUE(graph.topoSort(ordered, error)) << error;
    Program merged;
    // 入口模块私有类 与 导入模块公开类 重名：合并阶段检测到类型冲突 -> 报错
    EXPECT_FALSE(mergeModules(ordered, &merged, diags));
    EXPECT_GT(diags.getErrorCount(), 0);
}

// 导入声明合并到 Program（供语义层收集 importedModules_ 识别限定调用）
TEST(ModuleTest, MergeImportsMerged) {
    Diagnostics diags;
    ModuleGraph graph;
    graph.addModule(makeUnit("函数 工具函数() -> 整32 { 返回 1 }", "工具.cn", diags));
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

// 模块限定调用 数学.平方根(16.0) 经语义分析重写为直接调用并类型检查通过
TEST(ModuleTest, SemanticQualifiedCallRewrite) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 平方根(浮64 x) -> 浮64 { 返回 x }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "导入 数学.平方根\n"
        "函数 主() -> 整32 {\n"
        "    变量 结果 = 数学.平方根(16.0)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    EXPECT_TRUE(r.ok) << r.messages;
}

// 从...导入 限定调用：从 数学 导入 正弦, 余弦
TEST(ModuleTest, SemanticFromImportQualifiedCall) {
    std::vector<std::unique_ptr<ModuleUnit>> units;
    Diagnostics diags1, diags2;
    units.push_back(makeUnit(
        "公开:\n"
        "函数 正弦(浮64 x) -> 浮64 { 返回 x }\n"
        "函数 余弦(浮64 x) -> 浮64 { 返回 x }\n",
        "数学.cn", diags1));
    units.push_back(makeUnit(
        "从 数学 导入 正弦, 余弦\n"
        "函数 主() -> 整32 {\n"
        "    变量 结果 = 数学.正弦(1.0) + 数学.余弦(2.0)\n"
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
        "    变量 数值 = 数学.内部辅助(10)\n"
        "    返回 0\n"
        "}\n",
        "主.cn", diags2));
    auto r = analyzeModules(std::move(units));
    // 私有函数不跨模块合并 -> 数学.内部辅助 无法重写为直接调用 -> 报错
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
