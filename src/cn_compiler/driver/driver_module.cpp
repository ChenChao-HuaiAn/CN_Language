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
//   外部模块（stdlib/依赖目录，文件路径不在 entryDir 下）：取文件名主干为模块名
//   （stdlib/核心.cn -> 核心），moduleDir 为空（子模块从该文件目录加载）。
// 第 5 层（v2.0 规格书09）依赖查找顺序：
//   1. 入口同目录：候选1 = 当前模块文件目录 + 依赖名.cn；候选2 = 当前模块树目录
//   2. 货舱.toml [依赖]：声明依赖包的解析路径——
//      - 版本 = "内置" -> 编译器 stdlib 目录（options.stdlibDir）+ 依赖名.cn
//      - 其他版本    -> 货舱目录/依赖/<名称>/<名称>.cn（或 /包.cn，库 crate 根）
//   3. stdlib 兜底：options.stdlibDir + 依赖名.cn（未声明依赖但 stdlib 存在，如 核心）
// 未找到 -> 报「无法打开源文件」（与 v1.0 一致）
// 返回 false 表示加载/解析失败（diags 已输出或 error 已写入）
// isRootCall（簇⑥ 根治，2026-09-05）：仅命令行入口的首次调用为 true（递归
//   加载依赖恒 false）——写入 unit->isEntryUnit，mergeModules 据此判定入口
//   （命令行文件全部声明保留；仅被导入依赖按可见性过滤）。
// pkgPrefix（挂账1 根治，2026-09-08）：目录包成员的模块名包前缀——项目内
//   目录包根挂载成员时传包根名（IR布局.cn -> IR::IR布局），与整体构建形态
//   （相对入口目录路径自然带前缀）对齐，模块名只由包目录结构决定、不随
//   入口目录漂移；根治单文件入口形态成员与包根同名撞车（代码生成/包.cn 挂
//   模块 代码生成;，主干名=包根目录名，findModule 去重曾致成员永不加载）。
//   货舱依赖目录包根（候选3）不传——成员保持文件主干（既有符号面零变化）。
// cargoDepRoot（挂账1 根治，2026-09-08）：本次加载经由候选3 货舱 [依赖] 声明
//   路径——命中 包.cn（isPkgEntry）时写入 cargoPkgRoot 标记，其挂载成员模块
//   名保持文件主干（依赖包成员历史 bare 形态，E2E 47 等符号面零变化）。
bool loadModuleTree(const std::string& filePath, const std::string& dir,
                    const std::string& entryDir,
                    module::ModuleGraph& graph, std::string& error,
                    const std::unordered_set<std::string>& macros,
                    const DriverOptions& options, bool isRootCall = false,
                    const std::string& pkgPrefix = "",
                    bool cargoDepRoot = false) {
    // 源文件路径去重（挂账1 根治，2026-09-08；v2 编译文件 按 路径ID 已加载
    //   去重同构）：同一文件经不同链路加载（包根挂载链 vs 命令行入口/依赖
    //   候选链）模块名可能不同（各形态命名差异）——按路径去重防同文件双
    //   单元重复解析/重复合并（包上下文恢复场景：入口先载、包根后聚合）。
    if (graph.findByPath(filePath) != nullptr) return true;  // 已加载：去重
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
    // 外部模块（stdlib/依赖目录，不在入口 crate 模块树内）：模块名 = 文件名主干。
    //   stdlib/核心.cn -> 核心；依赖/网络库/网络.cn -> 网络（跨 crate 按名匹配）。
    // 判定为外部的条件：
    //   1. 文件不在入口目录下（relPart == filePath，如显式绝对路径）
    //   2. 文件在货舱依赖目录（relPart 以 "依赖/" 开头）——即使以 entryDir 前缀
    //      开头（tests/e2e/47_package_cargo/依赖/...），也不属于入口 crate 模块树，
    //      否则 moduleDir 会与 depRoot 重复拼接（第 5 层实测 bug）。
    //   3. 文件在编译器 stdlib 目录（relPart 以 "stdlib/" 开头）——同上。
    bool externalModule = false;
    const bool inCargoDeps = (relPart.rfind("依赖/", 0) == 0);
    const bool inStdlib = (relPart.rfind("stdlib/", 0) == 0);
    // 库 crate 包入口（<目录>/包.cn）：视为外部模块——其 moduleDir 为空，
    //   子模块依赖从文件所在目录解析（否则 dir+moduleDir 重复拼接包目录，
    //   <入口>/CN语言编译器//CN语言编译器/词法分析.cn 实测）。
    const bool isPkgEntry =
        (relPart.size() > 7 &&
         relPart.rfind("/包.cn") == relPart.size() - 7);  // "/包.cn" UTF-8 7 字节
    if (isPkgEntry) {
        // 目录包根（<目录>/包.cn）：模块名 = 包目录名（词法/包.cn -> 词法）。
        //   旧实现 pathStem（"包"）使全部目录包根撞名——graph.findModule("包")
        //   去重把第二个及以后的目录包整树静默跳过（成员模块不加载、公开符号
        //   不合并；2026-09-08 千行拆分轮 探针实证）。模块名=目录名与成员模块
        //   名前缀（词法::词法分析）同源，与候选2bPkg 探测键（dep 首段）一致。
        std::string dirPart = relPart.substr(0, relPart.size() - 7);
        while (!dirPart.empty() && (dirPart.back() == '/' || dirPart.back() == '\\')) {
            dirPart.pop_back();
        }
        const std::size_t lastSep = dirPart.find_last_of("/\\");
        if (lastSep != std::string::npos) dirPart = dirPart.substr(lastSep + 1);
        moduleName = dirPart;
        externalModule = true;
    } else if (relPart == filePath || inCargoDeps || inStdlib) {
        // 仅当路径含目录时视为外部（纯文件名如 主.cn 保持原逻辑）
        const std::size_t slash = moduleName.find("::");
        if (slash != std::string::npos) {
            moduleName = pathStem(filePath);
            externalModule = true;
        }
    }
    // 包前缀覆盖（挂账1 根治，2026-09-08）：目录包根挂载成员的模块名 =
    //   包根名::相对包目录路径主干（IR布局.cn -> IR::IR布局）——与整体构建
    //   形态（相对入口目录路径自然带前缀）逐字节对齐，成员命名只由包目录
    //   结构决定、不随入口目录漂移。货舱依赖包成员（pkgPrefix 空）保持
    //   文件主干历史形态。注意：放在外部判定之后（moduleDir 语义不受影响
    //   ——包成员 moduleDir 经下方尾匹配置空，两形态一致）。
    if (!pkgPrefix.empty()) {
        std::string relToPkg = filePath;
        if (relToPkg.size() > dir.size() &&
            relToPkg.compare(0, dir.size(), dir) == 0) {
            relToPkg = filePath.substr(dir.size());
        }
        const std::size_t dotPkg = relToPkg.find_last_of('.');
        if (dotPkg != std::string::npos) relToPkg = relToPkg.substr(0, dotPkg);
        for (std::size_t pos = relToPkg.find_first_of("/\\"); pos != std::string::npos;
             pos = relToPkg.find_first_of("/\\", pos + 1)) {
            relToPkg.replace(pos, 1, "::");
        }
        moduleName = pkgPrefix + "::" + relToPkg;
    }
    if (graph.findModule(moduleName) != nullptr) return true;  // 已加载：去重

    std::string source;
    if (!module::readSourceFile(filePath, source, error)) return false;

    Diagnostics diags;  // 词法/语法错误本地收集
    auto unit = std::make_unique<module::ModuleUnit>();
    unit->filePath = filePath;
    unit->moduleName = moduleName;
    unit->isEntryUnit = isRootCall;  // 簇⑥ 根治：命令行入口标记（递归依赖恒 false）
    unit->pkgRoot = pkgPrefix;  // 目录包归属（rustc crate 结构关系；非包成员为空）
    unit->cargoPkgRoot = isPkgEntry && cargoDepRoot;  // 货舱依赖包根标记（挂载成员命名 bare）
    // 模块目录前缀（相对入口；网络/传输控制.cn -> 网络/），子模块从该目录加载。
    // 外部模块（stdlib/依赖目录）moduleDir 为空——子模块从文件所在目录加载。
    unit->moduleDir = externalModule ? "" : pathDir(relPart);
    // 阶段7 修复：模块实际目录 dir 已含 relDir（包目录加载链：包.cn 子模块
    //   经包目录 dir 加载，relPart 相对 entryDir 仍带包前缀）——置空防重复拼接
    //   （<入口>/CN语言编译器//CN语言编译器/词法分析.cn 实测）。
    {
        const std::string relDir = pathDir(relPart);
        if (!relDir.empty() && dir.size() >= relDir.size() &&
            dir.compare(dir.size() - relDir.size(), relDir.size(), relDir) == 0) {
            unit->moduleDir = "";
        }
    }
    if (!module::parseSourceText(source, filePath, moduleName, unit->ast, unit->imports,
                                 diags, macros)) {
        std::cerr << diags.format();
        error = "模块 '" + moduleName + "' 解析失败";
        return false;
    }
    graph.addModule(std::move(unit));

    // 递归加载依赖模块（目录层级：候选1 当前文件目录 / 候选2 当前模块树目录）
    module::ModuleUnit* cur = graph.findModule(moduleName);
    // plans/018 P6b 工作流2（定位收紧，规格08-二）：收集本文件的 模块 声明
    //   挂载清单——依赖环对挂载类依赖收紧解析（仅 crate 根同级聚合 / 父模块
    //   子目录挂载两种合法形态，其余一律定位诊断）。
    for (const auto& imp : cur->ast->imports) {
        if (imp->isModuleDecl) cur->moduleMounts.insert(imp->importPath);
    }
    const std::string depBase = dir + cur->moduleDir;
    // 当前模块名最后段（网络::传输控制 -> 传输控制；网络 -> 网络）
    std::string lastSeg = moduleName;
    const std::size_t lastColon = lastSeg.rfind("::");
    if (lastColon != std::string::npos) lastSeg = lastSeg.substr(lastColon + 2);
    for (const auto& dep : cur->imports) {
        // 自包/包祖先导入（目录包成员 导入 自包名，如 IR::IR生成 导入 IR）：
        //   本包符号经合并阶段全局可见，无需加载；依赖边必须省略——否则
        //   「包根聚合边（IR -> IR布局）」与「成员导入边（IR布局 -> IR）」
        //   构成拓扑环（topoSort 报 模块循环依赖，2026-09-08 千行拆分轮实证）。
        //   判定双通道（挂账1 根治 2026-09-08）：①模块名前缀（整体形态
        //   IR::IR布局）②结构归属 pkgRoot（单文件入口形态成员名无前缀，
        //   前缀判定曾漏判致假环 + 多余路径探测）。rustc 同构：crate 成员
        //   引用本 crate 符号无需跨 crate 加载。仅限非挂载依赖：包根的
        //   同名成员挂载（代码生成/包.cn 声明 模块 代码生成;，dep==包根
        //   本身 moduleName）必须放行到挂载分支。
        if (cur->moduleMounts.count(dep) == 0 &&
            (dep == cur->moduleName ||
             cur->moduleName.rfind(std::string(dep) + "::", 0) == 0 ||
             dep == cur->pkgRoot)) {
            continue;
        }
        // A-5（子目录模块无法导入父目录 根治）：依赖模块已在图中（父模块是
        //   当前模块名 网络::传输控制 的前缀层级 网络，加载链祖先）——直接跳过
        //   候选路径探测（父目录文件不在子模块目录下，探测必失败报
        //   "无法打开源文件"）。符号经合并阶段全局可见（依赖主导入链全局合并），
        //   语义层 use 导入表按 导入 网络 正常登记。
        //   前置去重仅适用于非挂载依赖（2026-09-08 探针实证）：目录包根声明
        //   同名成员挂载时 findModule 命中包根自身 -> 挂载被跳过 -> 成员永不
        //   加载；挂载重复防护由 loadModuleTree 内部 findModule 去重兜底（幂等）。
        if (cur->moduleMounts.count(dep) == 0 && graph.findModule(dep) != nullptr) {
            continue;
        }
        // :: 分隔的依赖路径（网络::传输控制）转为目录层级（网络/传输控制.cn）
        std::string relPath = dep;
        for (std::size_t pos = relPath.find("::"); pos != std::string::npos;
             pos = relPath.find("::", pos + 1)) {
            relPath.replace(pos, 2, "/");
        }
        // ---- plans/018 P6b 工作流2：模块 声明挂载收紧解析（规格08-二）----
        //   模块 声明仅限两种树结构位置，物理路径必须与之对应：
        //   ① crate 根（命令行入口 / 包.cn）成员聚合 -> 同级 <S>.cn（候选1 形态，
        //      52 主.cn 挂 图书、47 包.cn 挂 网络）；
        //   ② 父模块挂子模块 -> 本模块子目录 <lastSeg>/<S>.cn（候选2 形态，
        //      网络库.cn 挂 网络库/内部工具.cn、网络.cn 挂 网络/传输控制.cn）。
        //   其余一切命中路径（兄弟文件/上溯/stdlib/货舱依赖）=「普通文件的模块
        //   路径由文件名+目录唯一决定」违背，报定位诊断（取代误导性的
        //   「无法打开源文件」——路径本身就不该被探测）。已加载去重检查在先：
        //   已在图中的挂载名不重复校验（首探测点已定去留）。
        if (cur->moduleMounts.count(dep) > 0) {
            const std::string baseName =
                cur->filePath.substr(cur->filePath.find_last_of("/\\") + 1);
            const bool isRootUnit = cur->isEntryUnit || baseName == "包.cn";
            std::string mountCand;
            std::string mountDir;
            if (isRootUnit) {
                mountCand = depBase + relPath + ".cn";
                mountDir = depBase;
            } else {
                mountCand = depBase + lastSeg + "/" + relPath + ".cn";
                mountDir = dir + cur->moduleDir + lastSeg + "/";
            }
            std::string mountSrc;
            if (!tryReadSource(mountCand, mountSrc)) {
                error = "模块声明 '" + dep + "' 非当前模块 '" + moduleName +
                        "' 的子模块（模块 声明仅限 包根成员聚合/父模块挂子模块，"
                        "规格08-二）";
                return false;
            }
            // 成员模块名包前缀（挂账1 根治 2026-09-08；v2 加载挂载声明 模块
            //   路径规则对齐）：项目内目录包根（包.cn 且非货舱依赖包）挂载
            //   成员 -> 包根名前缀（IR布局 -> IR::IR布局，整体/单文件两形态
            //   对齐+成员与包根同名撞车根治）；父挂子（普通文件挂子模块）->
            //   本模块完整名前缀（网络.cn 挂 传输控制 -> 网络::传输控制，
            //   v2 本模块路径::挂载名同构）；crate 根主.cn 同级挂载（52 挂
            //   图书）与货舱依赖包成员 -> 无前缀（历史 bare 形态）。
            std::string memberPrefix;
            if (!cur->cargoPkgRoot && !(isRootUnit && baseName != "包.cn")) {
                memberPrefix = cur->moduleName;
            }
            if (!loadModuleTree(mountCand, mountDir, entryDir, graph, error, macros,
                                options, /*isRootCall=*/false, memberPrefix)) {
                return false;
            }
            continue;
        }
        // 依赖首段作为包名（候选2b 目录包 与 候选3 货舱依赖 共用）
        std::string pkgName = dep;
        const std::size_t pkgSep = pkgName.find("::");
        if (pkgSep != std::string::npos) pkgName = pkgName.substr(0, pkgSep);
        // ---- 候选1：当前模块文件目录 + 依赖名.cn（普通依赖，主 导入 数学 -> 数学.cn）
        const std::string cand1 = depBase + relPath + ".cn";
        std::string srcBuf;
        if (tryReadSource(cand1, srcBuf)) {
            if (!loadModuleTree(cand1, depBase, entryDir, graph, error, macros, options)) {
                return false;
            }
            continue;
        }
        // ---- 候选2：当前模块树目录（网络.cn 内 模块 传输控制 -> 网络/传输控制.cn）
        const std::string cand2 = depBase + lastSeg + "/" + relPath + ".cn";
        if (tryReadSource(cand2, srcBuf)) {
            if (!loadModuleTree(cand2, dir + cur->moduleDir + lastSeg + "/", entryDir,
                                graph, error, macros, options)) {
                return false;
            }
            continue;
        }
        // ---- 候选2b（阶段7 自举模块，2026-08）：入口目录逐级上溯查找
        //   <目录>/<relPath>.cn——自举组件约定目录 CN语言编译器/（导入
        //   CN语言编译器::词法分析 -> relPath=CN语言编译器/词法分析 ->
        //   上溯命中 <项目根>/CN语言编译器/词法分析.cn）。E2E 用例入口
        //   （tests/e2e/NN_x/主.cn）经 3~4 级上溯命中项目根。
        {
            // entryDir 可能为相对路径（E2E 从项目根调用）：逐级上溯到空前缀
            //   （空前缀 = 相对 cwd，即项目根）。tests/e2e/NN_x -> tests/e2e ->
            //   tests -> 空 -> <cwd>/CN语言编译器/词法分析.cn 命中。
            // 起点=当前文件目录（dir+cur->moduleDir）而非 entryDir——包成员
            //   引用兄弟包（阶段目录包形态：IR/IR签名.cn 导入 词法/包.cn）
            //   时必须从成员文件目录向上查找；上溯链祖先段自然覆盖 entryDir
            //   （2026-09-08 千行拆分轮探针实证：旧起点下兄弟包导入不可达）。
            std::string upDir = depBase;
            while (!upDir.empty() && (upDir.back() == '/' || upDir.back() == '\\')) {
                upDir.pop_back();  // 去尾斜杠（pathDir 带尾），防 cand2bPkg 双斜杠
            }
            bool found = false;
            while (true) {
                const std::string cand2b =
                    (upDir.empty() ? "" : upDir + "/") + relPath + ".cn";
                if (tryReadSource(cand2b, srcBuf)) {
                    if (!loadModuleTree(cand2b, upDir.empty() ? "" : upDir + "/",
                                        entryDir, graph, error, macros, options)) {
                        return false;
                    }
                    found = true;
                    break;
                }
                // 目录包形态：<目录>/<首段>/包.cn（库 crate 入口；阶段7 自举
                //   组件 项目根/CN语言编译器/包.cn —— 包内 导入 词法分析 等子模块）
                const std::string cand2bPkg =
                    (upDir.empty() ? "" : upDir + "/") + pkgName + "/包.cn";
                if (tryReadSource(cand2bPkg, srcBuf)) {
                    // 包内子模块从包目录加载（cwd 级命中时 dir = <包名>/）
                    const std::string pkgDir = upDir.empty() ? pkgName + "/"
                                                             : upDir + "/" + pkgName + "/";
                    if (!loadModuleTree(cand2bPkg, pkgDir, entryDir,
                                        graph, error, macros, options)) {
                        return false;
                    }
                    found = true;
                    break;
                }
                if (upDir.empty()) break;
                const std::size_t sep = upDir.find_last_of("/\\");
                if (sep == std::string::npos) {
                    upDir = "";  // 末级单段目录（如 tests）：上溯到空（cwd）
                    continue;
                }
                upDir = upDir.substr(0, sep);
            }
            if (found) continue;
        }
        // ---- 候选3：货舱.toml [依赖] 声明路径（第 5 层，规格书09 六 依赖解析流程）
        // 依赖首段（网络协议::HTTP -> 网络协议）作为包名查 [依赖]（pkgName 已提前计算）
        if (options.hasCargoConfig) {
            const driver::CargoDependency* cargoDep =
                options.cargoConfig.findDependency(pkgName);
            if (cargoDep != nullptr) {
                if (cargoDep->version == "内置") {
                    // 内置依赖：编译器 stdlib 目录
                    if (!options.stdlibDir.empty()) {
                        const std::string candBuiltin = options.stdlibDir + "/" + pkgName + ".cn";
                        if (tryReadSource(candBuiltin, srcBuf)) {
                            if (!loadModuleTree(candBuiltin, options.stdlibDir + "/", entryDir,
                                                graph, error, macros, options)) {
                                return false;
                            }
                            continue;
                        }
                    }
                } else {
                    // 本地依赖：货舱目录/依赖/<名称>/ 子目录（库 crate：<名称>.cn 或 包.cn）
                    const std::string depRoot = options.cargoDir + "依赖/" + pkgName + "/";
                    const std::string candLocal1 = depRoot + pkgName + ".cn";
                    if (tryReadSource(candLocal1, srcBuf)) {
                        if (!loadModuleTree(candLocal1, depRoot, entryDir,
                                            graph, error, macros, options)) {
                            return false;
                        }
                        continue;
                    }
                    const std::string candLocal2 = depRoot + "包.cn";
                    if (tryReadSource(candLocal2, srcBuf)) {
                        if (!loadModuleTree(candLocal2, depRoot, entryDir,
                                            graph, error, macros, options,
                                            /*isRootCall=*/false, /*pkgPrefix=*/"",
                                            /*cargoDepRoot=*/true)) {
                            return false;
                        }
                        continue;
                    }
                }
            }
        }
        // ---- 候选4：编译器 stdlib 兜底（未声明依赖但 stdlib 存在，如 核心/容器）
        if (!options.stdlibDir.empty()) {
            const std::string candStdlib = options.stdlibDir + "/" + pkgName + ".cn";
            if (tryReadSource(candStdlib, srcBuf)) {
                if (!loadModuleTree(candStdlib, options.stdlibDir + "/", entryDir,
                                    graph, error, macros, options)) {
                    return false;
                }
                continue;
            }
        }
        // 全部候选不存在：依赖模块缺失（保持与 v1.0 一致：加载失败报错）
        error = "无法打开源文件: " + cand1;
        return false;
    }
    return true;
}

// 包上下文恢复（挂账1 根治，2026-09-08）：单文件入口若是项目内目录包成员——
//   所在目录存在 包.cn 且其声明了入口文件主干名的 模块 挂载——则加载包根聚合
//   （兄弟成员补入依赖闭包）。背景：包内兄弟符号经合并阶段全局池解析（无显式
//   导入），命令行入口绕过包根挂载链单独加载时兄弟文件不在依赖闭包——
//   「未声明函数」错误族（2026-09-08 第三十七轮挂账：语义检查.cn 入口报
//   未声明函数: 取语义诊断 等 137 错实证）。
//   时序：入口先载（isEntryUnit 标记成立）→ 包根后聚合——包根挂载入口同
//   文件经 loadModuleTree 路径去重跳过（findByPath，同文件不双载、入口标记
//   不被挂载链覆盖）。
//   成员判定精确：包根未声明入口主干挂载（入口只是恰好位于包目录的非成员
//   文件）不触发恢复，防误载无关包符号；入口自身即 包.cn 时主干=「包」不与
//   任何挂载名相同，天然跳过。返回 false=触发恢复但包根加载失败（error 已写）。
bool restorePackageContext(const std::string& entryFile,
                           module::ModuleGraph& graph, std::string& error,
                           const std::unordered_set<std::string>& macros,
                           const DriverOptions& options) {
    const std::string dir = pathDir(entryFile);
    const std::string pkgCand = dir + "包.cn";
    std::string pkgSrc;
    if (!tryReadSource(pkgCand, pkgSrc)) return true;  // 所在目录无包根：非包成员
    std::unique_ptr<Program> pkgAst;
    std::vector<std::string> pkgImports;
    Diagnostics pkgDiags;
    if (!module::parseSourceText(pkgSrc, pkgCand, pathStem(pkgCand), pkgAst,
                                 pkgImports, pkgDiags, macros)) {
        return true;  // 包根本身解析失败：交由其正式加载链路报错
    }
    const std::string stem = pathStem(entryFile);
    bool isMember = false;
    for (const auto& imp : pkgAst->imports) {
        if (imp->isModuleDecl && imp->importPath == stem) {
            isMember = true;
            break;
        }
    }
    if (!isMember) return true;  // 包根未挂载入口主干：非包成员文件
    return loadModuleTree(pkgCand, dir, dir, graph, error, macros, options);
}

} // namespace

// 多文件编译流水线（Task 3.6 模块系统）：
//   入口文件 -> 递归加载依赖 -> 拓扑排序 -> AST 合并 -> 语义 -> IR -> 代码生成
int runModulePipeline(const std::string& entryFile, const DriverOptions& options,
                      PipelineOutput& output) {
    Diagnostics diagnostics;

    // 1. 加载模块树：入口 + 依赖
    //    第 5 层：依赖查找按 options 中货舱.toml 配置 + stdlib 目录扩展
    //    （入口同目录优先；货舱 [依赖] 次之；stdlib 兜底）
    //    簇⑥ 根治：根调用 isRootCall=true 写入命令行入口的 isEntryUnit 标记
    module::ModuleGraph graph;
    std::string error;
    const std::string dir = pathDir(entryFile);
    if (!loadModuleTree(entryFile, dir, dir, graph, error, options.macros, options,
                        /*isRootCall=*/true)) {
        if (!error.empty()) std::cerr << "错误: " << error << "\n";
        return 1;
    }

    // 1.5 包上下文恢复（挂账1 根治，2026-09-08）：入口是项目内目录包成员时
    //     聚合包根（兄弟成员补入依赖闭包；入口文件经路径去重不双载）
    if (!restorePackageContext(entryFile, graph, error, options.macros, options)) {
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
    // plans/018 呈报一B（2026-09-07 用户终裁）：注入已加载模块名清单——
    //   依赖图内全部模块名（含入口/子模块/外部文件主干）+ 货舱 [依赖] 包名
    //   （crate 根名如 工具库 不产生声明，限定调用 工具库::模块::符号 按
    //   「模块已加载」放行须依赖本清单；P1-1 废止的加载判定数据源）。
    for (module::ModuleUnit* u : ordered) {
        if (u != nullptr) program->loadedModules.push_back(u->moduleName);
    }
    for (const auto& dep : options.cargoConfig.deps) {
        program->loadedModules.push_back(dep.name);
    }

    // 3.5（241-a 立 D13·242-a 根治）：入口主函数缺失编译期诊断——
    //   合并后的入口模块顶层若没有 主 函数，编译期即报错（原静默到链接期
    //   才爆 LNK2019 cn_main）。入口模块=拓扑排序最后一个（入口最后）；
    //   库模块无 主 不参与判定。
    {
        // 注意：mergeModules 以 release() 摘除各 unit 的声明（原 unit->ast 数组
        //   留空壳）——入口判定必须遍历合并后的 program，不能回看 unit->ast。
        const bool requireMain = options.requireEntryMain;  // 242-a：check 库组件不强制主
        bool hasMain = !requireMain;
        for (auto& d : program->declarations) {
            if (d != nullptr && d->name == "主") { hasMain = true; break; }
        }
        if (!hasMain) {
            std::string entryName = "主";
            for (module::ModuleUnit* u : ordered) {
                if (u != nullptr && u->isEntryUnit) { entryName = u->moduleName; break; }
            }
            diagnostics.report(DiagnosticLevel::Error, program->location,
                               "入口模块 '" + entryName +
                                   "' 未定义入口函数 主（程序入口必须为 函数 主()）");
            std::cerr << diagnostics.format();
            return 1;
        }
        // 243-a（D17 防线）：主 返回 结果/可选/聚合类型——运行时入口 ABI 仅支持
        //   整32 返回（runtime entry 期望 int，聚合返回走 sret 隐藏指针 → 调用
        //   约定错位 → 退出路径段错误 0xC0000005）。退出码语义（正常→值/错误→
        //   错误码）待规范定义（呈报 plans/021 §3-D17），在此之前编译期拒绝
        //   （把必然崩溃的程序挡在编译期——诚实防线非最终形态）。
        for (auto& d : program->declarations) {
            if (d != nullptr && d->name == "主" && !d->returnType.empty() &&
                d->returnType != "整32") {
                diagnostics.report(DiagnosticLevel::Error, d->location,
                                   "入口函数 主 的返回类型 '" + d->returnType +
                                       "' 暂不支持（运行时入口 ABI 仅支持 整32；"
                                       "结果 类型退出码语义待规范定义——见 plans/021 D17）");
                std::cerr << diagnostics.format();
                return 1;
            }
        }
    }

    // 4. 语义分析（符号表/类型检查/类解析/错误码传播）
    SemanticAnalyzer semantic(diagnostics);
    // 239-a：内建编译期常量 调试模式 取值（--发布=假）
    semantic.setBuiltinReleaseMode(options.releaseMode);
    if (!semantic.analyze(program.get())) {
        std::cerr << diagnostics.format();
        return 1;
    }
    // plans/019 阶段4（2026-09-10）：观察期警告可见性——仅警告无错误时同样
    //   输出（安全区边界警告原被 hasErrors 短路吞掉；模块主管线=check/build
    //   实际路径）
    if (!diagnostics.hasErrors() && diagnostics.getWarningCount() > 0) {
        std::cerr << diagnostics.format();
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
