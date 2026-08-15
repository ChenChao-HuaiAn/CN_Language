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
#include <unordered_set>
#include <vector>

#include "cn_compiler/codegen/backend_factory.hpp"
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

// 安静尝试读取文件（不写 error）：探测候选路径是否存在
bool tryReadSource(const std::string& path, std::string& out) {
    std::string err;
    return module::readSourceFile(path, out, err);
}

// 递归加载模块及其依赖：已加载模块名去重（重复导入只加载一次）
// 第 4 层（v2.0 决策6，P1-2）：目录层级——模块名 = 文件相对入口目录的路径：
//   - 入口：网络/传输控制.cn -> 模块名 网络::传输控制（:: 对应 / 目录）
//   - 依赖：导入 数学::平方根 首段 数学 -> 数学.cn（当前模块文件目录）
//   - 子模块：网络.cn 内 模块 传输控制 -> 网络/传输控制.cn（当前模块树目录）
// 模块名规范化：始终基于入口目录 entryDir（保证子模块名含父模块前缀，
//   如 网络/传输控制.cn -> 网络::传输控制，而非仅 传输控制）。
// 搜索顺序（每依赖两候选路径）：
//   候选1 = 当前模块文件目录 + 依赖名.cn（普通依赖，如 主 导入 数学）
//   候选2 = 当前模块文件目录 + 当前模块名最后段 + "/" + 依赖名.cn
//           （子模块声明：网络.cn 内 模块 传输控制 -> 网络/传输控制.cn）
// 返回 false 表示加载/解析失败（diags 已输出或 error 已写入）
bool loadModuleTree(const std::string& filePath, const std::string& dir,
                    const std::string& entryDir,
                    module::ModuleGraph& graph, std::string& error,
                    const std::unordered_set<std::string>& macros) {
    // 模块名 = 文件相对**入口目录**的路径主干（网络/传输控制.cn -> 网络::传输控制）
    std::string relPart;
    if (filePath.size() > entryDir.size() && filePath.compare(0, entryDir.size(), entryDir) == 0) {
        relPart = filePath.substr(entryDir.size());
    } else {
        relPart = filePath;
    }
    // 模块名 = 相对入口目录的完整路径（去扩展名，斜杠 -> ::）。
    // 注意：不能用 pathStem（会去掉目录——net/transport.cn 只取 transport），
    //   必须保留 net/ 前缀：net/transport.cn -> net::transport（目录层级 P1-2）。
    std::string moduleName = relPart;
    const std::size_t dotPos = moduleName.find_last_of('.');
    if (dotPos != std::string::npos) moduleName = moduleName.substr(0, dotPos);
    for (std::size_t pos = moduleName.find_first_of("/\\"); pos != std::string::npos;
         pos = moduleName.find_first_of("/\\", pos + 1)) {
        moduleName.replace(pos, 1, "::");
    }
    if (graph.findModule(moduleName) != nullptr) return true;  // 已加载：去重

    std::string source;
    if (!module::readSourceFile(filePath, source, error)) return false;

    Diagnostics diags;  // 词法/语法错误本地收集
    auto unit = std::make_unique<module::ModuleUnit>();
    unit->filePath = filePath;
    unit->moduleName = moduleName;
    // 模块目录前缀（相对入口；网络/传输控制.cn -> 网络/），子模块从该目录加载
    unit->moduleDir = pathDir(relPart);
    if (!module::parseSourceText(source, filePath, moduleName, unit->ast, unit->imports,
                                 diags, macros)) {
        std::cerr << diags.format();
        error = "模块 '" + moduleName + "' 解析失败";
        return false;
    }
    graph.addModule(std::move(unit));

    // 递归加载依赖模块（目录层级：候选1 当前文件目录 / 候选2 当前模块树目录）
    module::ModuleUnit* cur = graph.findModule(moduleName);
    const std::string depBase = dir + cur->moduleDir;
    // 当前模块名最后段（网络::传输控制 -> 传输控制；网络 -> 网络）
    std::string lastSeg = moduleName;
    const std::size_t lastColon = lastSeg.rfind("::");
    if (lastColon != std::string::npos) lastSeg = lastSeg.substr(lastColon + 2);
    for (const auto& dep : cur->imports) {
        // :: 分隔的依赖路径（网络::传输控制）转为目录层级（网络/传输控制.cn）
        std::string relPath = dep;
        for (std::size_t pos = relPath.find("::"); pos != std::string::npos;
             pos = relPath.find("::", pos + 1)) {
            relPath.replace(pos, 2, "/");
        }
        // 候选1：当前模块文件目录 + 依赖名.cn（普通依赖，主 导入 数学 -> 数学.cn）
        const std::string cand1 = depBase + relPath + ".cn";
        std::string srcBuf;
        if (tryReadSource(cand1, srcBuf)) {
            if (!loadModuleTree(cand1, depBase, entryDir, graph, error, macros)) return false;
            continue;
        }
        // 候选2：当前模块树目录（网络.cn 内 模块 传输控制 -> 网络/传输控制.cn）
        //   当前模块文件目录 + 当前模块名最后段 + "/" + 依赖名.cn
        const std::string cand2 = depBase + lastSeg + "/" + relPath + ".cn";
        if (tryReadSource(cand2, srcBuf)) {
            if (!loadModuleTree(cand2, dir + cur->moduleDir + lastSeg + "/", entryDir,
                                graph, error, macros)) {
                return false;
            }
            continue;
        }
        // 两个候选均不存在：依赖模块缺失（保持与 v1.0 一致：加载失败报错）
        error = "无法打开源文件: " + cand1;
        return false;
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
    if (!loadModuleTree(entryFile, dir, dir, graph, error, options.macros)) {
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

    // 6. 代码生成（按目标平台分发后端：win-x64 -> MASM / linux-arm64 -> GAS）
    std::unique_ptr<Backend> backend = createBackend(
        options.target, diagnostics, &semantic,
        options.optLevel, options.useRegAlloc, options.debugInfo);
    if (!backend) {
        std::cerr << diagnostics.format();
        return 1;
    }
    output.asmText = backend->generateAssembly(output.module);
    if (diagnostics.hasErrors()) {
        std::cerr << diagnostics.format();
        return 1;
    }
    return 0;
}

} // namespace driver
} // namespace cn_compiler
