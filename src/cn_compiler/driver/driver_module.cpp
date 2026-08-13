// CN语言编译器驱动模块：多文件编译流水线（Task 3.6 模块系统）
// 职责：
//   1. 以入口文件（主.cn 或含 函数 主 的文件）为根，递归加载 导入/从...导入 依赖模块
//   2. 依赖模块在同目录解析：模块名 数学 -> 数学.cn（. 分隔路径取首段）
//   3. 拓扑排序（被依赖者在前）后合并 AST 为单一 Program
//   4. 复用 语义->IR->优化->代码生成 公共流水线（与 runPipeline 阶段一致）
// 与既有单文件 CLI 兼容：单文件编译仍走 runPipeline；多文件仅当入口含导入时启用。
// 单文件 <=1000 行、单函数 <=100 行约束。
#include "cn_compiler/driver/driver.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/module/module.hpp"
#include "cn_compiler/opt/pass_manager.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {
namespace driver {

namespace {

// 提取文件主干名（去除目录与扩展名）：tests/主.cn -> 主
std::string pathStem(const std::string& path) {
    std::size_t slash = path.find_last_of("/\\");
    std::string base = (slash == std::string::npos) ? path : path.substr(slash + 1);
    std::size_t dot = base.find_last_of('.');
    return (dot == std::string::npos) ? base : base.substr(0, dot);
}

// 提取文件所在目录（含末尾分隔符）；无目录返回空串
std::string pathDir(const std::string& path) {
    std::size_t slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? "" : path.substr(0, slash + 1);
}

// 递归加载模块及其依赖：已加载模块名去重（重复导入只加载一次）
// 依赖模块路径 = 入口所在目录 + 模块名 + ".cn"（文件即模块，同目录平铺）
// 返回 false 表示加载/解析失败（diags 已输出或 error 已写入）
bool loadModuleTree(const std::string& filePath, const std::string& dir,
                    module::ModuleGraph& graph, std::string& error) {
    const std::string stem = pathStem(filePath);
    if (graph.findModule(stem) != nullptr) return true;  // 已加载：去重

    std::string source;
    if (!module::readSourceFile(filePath, source, error)) return false;

    Diagnostics diags;  // 词法/语法错误本地收集
    auto unit = std::make_unique<module::ModuleUnit>();
    unit->filePath = filePath;
    unit->moduleName = stem;
    if (!module::parseSourceText(source, filePath, stem, unit->ast, unit->imports, diags)) {
        std::cerr << diags.format();
        error = "模块 '" + stem + "' 解析失败";
        return false;
    }
    graph.addModule(std::move(unit));

    // 递归加载依赖模块（同目录下 模块名.cn）
    module::ModuleUnit* cur = graph.findModule(stem);
    for (const auto& dep : cur->imports) {
        const std::string depFile = dir + dep + ".cn";
        if (!loadModuleTree(depFile, dir, graph, error)) return false;
    }
    return true;
}

} // namespace

// 多文件编译流水线（Task 3.6 模块系统）：
//   入口文件 -> 递归加载依赖 -> 拓扑排序 -> AST 合并 -> 语义 -> IR -> 代码生成
int runModulePipeline(const std::string& entryFile, const DriverOptions& options,
                      PipelineOutput& output) {
    Diagnostics diagnostics;

    // 1. 加载模块树：入口 + 依赖（依赖与入口同目录平铺解析）
    module::ModuleGraph graph;
    std::string error;
    const std::string dir = pathDir(entryFile);
    if (!loadModuleTree(entryFile, dir, graph, error)) {
        if (!error.empty()) std::cerr << "错误: " << error << "\n";
        return 1;
    }

    // 2. 拓扑排序（被依赖者在前，入口最后；循环依赖报错）
    std::vector<module::ModuleUnit*> ordered;
    if (!graph.topoSort(ordered, error)) {
        std::cerr << "错误: " << error << "\n";
        return 1;
    }

    // 3. 合并 AST 为单一 Program（被导入模块仅公开声明；入口模块全部声明）
    auto program = std::make_unique<Program>();
    if (!module::mergeModules(ordered, program.get(), diagnostics)) {
        std::cerr << diagnostics.format();
        return 1;
    }

    // 4. 语义分析（符号表/类型检查/类解析/错误码传播）
    SemanticAnalyzer semantic(diagnostics);
    if (!semantic.analyze(program.get())) {
        std::cerr << diagnostics.format();
        return 1;
    }

    // 5. IR 生成（绑定语义引用：类布局/虚表/结构体布局查询）
    IRGenerator irGen(diagnostics, &semantic);
    output.module = irGen.generate(program.get());
    output.hasModule = true;
    if (diagnostics.hasErrors()) {
        std::cerr << diagnostics.format();
        return 1;
    }

    // 5.5 优化阶段（与 runPipeline 一致：-O1~O3 Pass 流水线）
    if (options.optLevel > 0) {
        opt::runOptLevel(output.module, options.optLevel);
    }

    // 6. 代码生成（X64 MASM 汇编文本）
    X64CodeGenerator codegen(diagnostics, &semantic);
    output.asmText = codegen.generateAssembly(output.module);
    if (diagnostics.hasErrors()) {
        std::cerr << diagnostics.format();
        return 1;
    }
    return 0;
}

} // namespace driver
} // namespace cn_compiler
