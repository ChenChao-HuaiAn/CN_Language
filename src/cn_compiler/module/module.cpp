// CN语言编译器模块系统实现（Task 3.6，规格书08 模块系统）
// 实现要点：
//   1. readSourceFile：UTF-8 读取（去 BOM），与 cn_main.cpp 同逻辑（模块层独立提供）
//   2. parseSourceText：词法 + 语法 + 导入依赖收集（模块名去重）
//   3. ModuleGraph：依赖图 + DFS 拓扑排序（三色标记检测循环依赖）
//   4. mergeModules：公开符号合并 + 跨模块类型重名冲突检测
//      所有权转移：源 AST 声明节点 release() 移交目标 Program（避免 double-free）
//   5. 入口判定：ModuleUnit::isEntryUnit（命令行显式入口标记，簇⑥ 根治 2026-09-05）；
//      isEntryModule（模块名 == 主）为文件名约定查询保留，非入口语义依据
// 单文件 <=1000 行、单函数 <=100 行约束。
#include "cn_compiler/module/module.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>  // MultiByteToWideChar：UTF-8 路径 -> UTF-16 宽路径（_wfopen）
#endif

#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/parser.hpp"

namespace cn_compiler {
namespace module {

// ==================== 源文件读取 ====================

// 读取 UTF-8 源文件（自动去除 UTF-8 BOM，兼容记事本保存的源码）
// 注意：源码内导入语句中的中文模块名（如 数学.cn）是 UTF-8 字节，而 Windows
//   std::ifstream 窄字符路径按 ANSI 代码页解释，找不到 UTF-8 文件名。
//   实现：MultiByteToWideChar 将 UTF-8 路径转 UTF-16 宽路径，_wfopen_s 打开
//   （Task 3.6 集成验证：std::filesystem::u8path 在 MSVC 对中文路径挂起，弃用）。
bool readSourceFile(const std::string& path, std::string& content, std::string& error) {
#ifdef _WIN32
    // 路径编码兼容：
    //   1. 入口文件路径来自命令行（cmd 按 ANSI 代码页 GBK 传入）——窄字符直接打开
    //   2. 依赖模块路径由源码内 UTF-8 模块名拼成——MultiByteToWideChar(CP_UTF8) 转宽路径
    //   先试窄字符（GBK 入口路径），失败再试 UTF-8 转换（UTF-8 依赖路径）。
    auto readNarrow = [&](const std::string& p, std::string& out) -> bool {
        FILE* fp = nullptr;
        if (fopen_s(&fp, p.c_str(), "rb") != 0 || fp == nullptr) return false;
        std::fseek(fp, 0, SEEK_END);
        const long size = std::ftell(fp);
        std::fseek(fp, 0, SEEK_SET);
        out.clear();
        if (size > 0) {
            out.resize(static_cast<std::size_t>(size));
            out.resize(std::fread(&out[0], 1, static_cast<std::size_t>(size), fp));
        }
        std::fclose(fp);
        return true;
    };
    if (readNarrow(path, content)) {
        // 成功：窄字符（GBK/ASCII）路径
    } else {
        // 窄字符失败：尝试 UTF-8 路径 -> UTF-16 宽路径（依赖模块中文名）
        const int wideLen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
        if (wideLen <= 0) {
            error = "无法打开源文件: " + path;
            return false;
        }
        std::vector<wchar_t> widePath(static_cast<std::size_t>(wideLen));
        MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, widePath.data(), wideLen);
        FILE* fp = nullptr;
        if (_wfopen_s(&fp, widePath.data(), L"rb") != 0 || fp == nullptr) {
            error = "无法打开源文件: " + path;
            return false;
        }
        std::string buf;
        std::fseek(fp, 0, SEEK_END);
        const long size = std::ftell(fp);
        std::fseek(fp, 0, SEEK_SET);
        if (size > 0) {
            buf.resize(static_cast<std::size_t>(size));
            buf.resize(std::fread(&buf[0], 1, static_cast<std::size_t>(size), fp));
        }
        std::fclose(fp);
        content = std::move(buf);
    }
#else
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "无法打开源文件: " + path;
        return false;
    }
    std::ostringstream buf;
    buf << in.rdbuf();
    content = buf.str();
#endif
    // 去除 UTF-8 BOM（EF BB BF）
    if (content.size() >= 3 && content.compare(0, 3, "\xEF\xBB\xBF") == 0) {
        content = content.substr(3);
    }
    return true;
}

// ==================== 单模块解析 ====================

// 从源码文本解析模块（词法 + 语法 + 导入收集）
// 返回解析是否成功（失败时 diags 已记录词法/语法错误）
bool parseSourceText(const std::string& source, const std::string& filePath,
                     const std::string& moduleName, std::unique_ptr<Program>& ast,
                     std::vector<std::string>& imports, Diagnostics& diags,
                     const std::unordered_set<std::string>& macros) {
    // Task 6.6：命令行注入宏参与条件编译判定（#如果定义 宏名）
    Lexer lexer(source, filePath, diags, macros);
    auto tokens = lexer.tokenize();
    if (diags.hasErrors()) return false;

    Parser parser(diags);
    ast = parser.parse(tokens);
    if (diags.hasErrors()) return false;

    // 收集导入依赖（模块名去重）。
    // 第 4 层（v2.0 决策6，P1-2）：目录层级——依赖模块名取 :: 路径**前缀层级**：
    //   - 导入 网络::传输控制  -> 依赖 网络（加载 网络.cn 或 网络/ 目录树）
    //   - 导入 网络::传输控制::发送 -> 依赖 网络（首段模块树根）
    //   - 导入 网络协议::HTTP::请求 -> 依赖 网络协议（crate/模块树首段）
    //   模块声明（isModuleDecl，模块 网络）同样取首段加载网络.cn。
    //   依赖收集仍以首段为模块名（driver 递归加载 网络.cn 后，其内部再
    //   通过子模块声明加载 网络/传输控制.cn——目录层级由 driver 展开）。
    for (const auto& imp : ast->imports) {
        std::string dep = imp->importPath;
        const std::size_t sep = dep.find("::");
        if (sep != std::string::npos) dep = dep.substr(0, sep);
        // 模块声明（挂载）恒登记：树结构声明而非依赖引用。旧 dep != moduleName
        //   自引用过滤对目录包根误伤——包根 moduleName=包目录名（代码生成/包.cn
        //   -> 代码生成）后，同名成员挂载（模块 代码生成;）被丢弃 -> 挂载循环
        //   不可见 -> 成员文件永不加载（2026-09-08 千行拆分轮 探针实证）。
        if (imp->isModuleDecl) {
            if (!dep.empty() &&
                std::find(imports.begin(), imports.end(), dep) == imports.end()) {
                imports.push_back(dep);
            }
            continue;
        }
        if (!dep.empty() && dep != moduleName &&
            std::find(imports.begin(), imports.end(), dep) == imports.end()) {
            imports.push_back(dep);
        }
    }
    return true;
}

// ==================== 模块依赖图 ====================

// 添加模块（同模块名去重：重复添加保留首个）
bool ModuleGraph::addModule(std::unique_ptr<ModuleUnit> unit) {
    if (unit == nullptr) return false;
    const std::string& name = unit->moduleName;
    if (units_.find(name) != units_.end()) return false;  // 已存在
    // 路径索引登记（findByPath O(1) 查询；unordered_map 节点地址稳定，
    //   unique_ptr 迁移不影响 ModuleUnit* 有效性）
    pathIndex_[unit->filePath] = unit.get();
    units_[name] = std::move(unit);
    return true;
}

// 查找模块（未找到返回 nullptr）
ModuleUnit* ModuleGraph::findModule(const std::string& moduleName) {
    auto it = units_.find(moduleName);
    return (it == units_.end()) ? nullptr : it->second.get();
}

// 按源文件路径查找模块（未找到返回 nullptr；挂账1 根治 2026-09-08）
ModuleUnit* ModuleGraph::findByPath(const std::string& filePath) {
    auto it = pathIndex_.find(filePath);
    return (it == pathIndex_.end()) ? nullptr : it->second;
}

// DFS 拓扑排序辅助：三色标记（0=未访问 1=访问中 2=已完成）
namespace {

// 递归 DFS：从 name 出发沿导入边遍历，被依赖者先压入 ordered
// 返回 false 表示检测到循环依赖（error 记录环路径）
bool dfsTopo(const std::unordered_map<std::string, std::unique_ptr<ModuleUnit>>& units,
             const std::string& name, std::unordered_map<std::string, int>& color,
             std::vector<ModuleUnit*>& ordered, std::vector<std::string>& path,
             std::string& error) {
    auto it = units.find(name);
    if (it == units.end()) return true;  // 依赖模块未加载（合并时忽略）
    const int c = color[name];
    if (c == 2) return true;             // 已完成
    if (c == 1) {
        // 循环依赖：path 中 name 之后的段即为环
        error = "模块循环依赖: ";
        bool seen = false;
        for (const auto& p : path) {
            if (p == name) seen = true;
            if (seen) error += p + " -> ";
        }
        error += name;
        return false;
    }
    color[name] = 1;
    path.push_back(name);
    // 依赖源：统一从 AST 的导入声明收集（解析期已填充 Program::imports），
    //   避免 ModuleUnit.imports 缓存字段与 AST 不同步导致拓扑顺序错误。
    if (it->second->ast != nullptr) {
        for (const auto& imp : it->second->ast->imports) {
            // 取导入路径首段为模块名（如 数学::平方根 -> 数学；v2.0 :: 分隔）
            std::string dep = imp->importPath;
            const std::size_t sep = dep.find("::");
            if (sep != std::string::npos) dep = dep.substr(0, sep);
            // 自包导入边省略（挂账1 根治，2026-09-08；rustc crate 模型同构）：
            //   目录包成员 导入 自包名（IR布局 导入 IR）是包内符号引用（合并
            //   期全局池可见）而非跨包依赖——与包根聚合边（IR -> IR布局）不构
            //   成拓扑环。结构归属（pkgRoot）判定而非模块名前缀文本推断：
            //   单文件入口形态成员名无包前缀（历史形态文件主干），前缀判定
            //   漏洞曾致假环（IR -> IR布局 -> IR，2026-09-08 单文件入口实测）。
            if (!dep.empty() && dep != name && dep != it->second->pkgRoot) {
                if (!dfsTopo(units, dep, color, ordered, path, error)) return false;
            }
        }
    }
    path.pop_back();
    color[name] = 2;
    ordered.push_back(it->second.get());
    return true;
}

} // namespace

// 按依赖拓扑排序：ordered 输出 [被依赖者..., 入口]；false=循环依赖
bool ModuleGraph::topoSort(std::vector<ModuleUnit*>& ordered, std::string& error) const {
    ordered.clear();
    std::unordered_map<std::string, int> color;
    std::vector<std::string> path;
    // 按模块名排序保证确定性（避免依赖 unordered_map 遍历顺序）
    std::vector<std::string> names;
    names.reserve(units_.size());
    for (const auto& kv : units_) names.push_back(kv.first);
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        if (!dfsTopo(units_, name, color, ordered, path, error)) return false;
    }
    return true;
}

// ==================== AST 合并 ====================

namespace {

// ---- 函数体引用的函数名收集（缺陷4 修复：私有依赖闭包） ----
// 前置声明（collectExprFuncRefs 的 LambdaExpr 分支会调用，定义在下文）
void collectLambdaBodyRefs(BlockStmt* body, std::unordered_set<std::string>& out);

// 被导入模块的公开函数体内可能直接调用本模块私有函数（如 公开计算 调用 内部辅助）。
// 私有函数不跨模块合并，导致公开函数体找不到符号报"未声明"。
// 本收集器遍历表达式/语句，找出直接函数调用（CallExpr.callee 为 IdentifierExpr）的名字，
// 供 mergeModuleDecls 做闭包合并（把被公开函数引用的私有函数一并带入）。
void collectExprFuncRefs(Expr* e, std::unordered_set<std::string>& out) {
    if (e == nullptr) return;
    switch (e->getType()) {
        case NodeType::CallExpr: {
            CallExpr* call = static_cast<CallExpr*>(e);
            if (call->callee != nullptr &&
                call->callee->getType() == NodeType::IdentifierExpr) {
                out.insert(static_cast<IdentifierExpr*>(call->callee.get())->name);
            }
            // 递归：callee 可能为成员调用（object 含函数调用）与实参
            collectExprFuncRefs(call->callee.get(), out);
            for (auto& a : call->arguments) collectExprFuncRefs(a.get(), out);
            return;
        }
        case NodeType::BinaryExpr: {
            BinaryExpr* b = static_cast<BinaryExpr*>(e);
            collectExprFuncRefs(b->left.get(), out);
            collectExprFuncRefs(b->right.get(), out);
            return;
        }
        case NodeType::UnaryExpr:
            collectExprFuncRefs(static_cast<UnaryExpr*>(e)->operand.get(), out);
            return;
        case NodeType::AssignmentExpr: {
            AssignmentExpr* a = static_cast<AssignmentExpr*>(e);
            collectExprFuncRefs(a->target.get(), out);
            collectExprFuncRefs(a->value.get(), out);
            return;
        }
        case NodeType::MemberExpr:
            collectExprFuncRefs(static_cast<MemberExpr*>(e)->object.get(), out);
            return;
        case NodeType::IndexExpr: {
            IndexExpr* ix = static_cast<IndexExpr*>(e);
            collectExprFuncRefs(ix->object.get(), out);
            collectExprFuncRefs(ix->index.get(), out);
            return;
        }
        case NodeType::InitListExpr:
            for (auto& el : static_cast<InitListExpr*>(e)->elements) {
                collectExprFuncRefs(el.get(), out);
            }
            return;
        case NodeType::StructInitExpr:
            for (auto& f : static_cast<StructInitExpr*>(e)->fields) {
                collectExprFuncRefs(f.second.get(), out);
            }
            return;
        case NodeType::TernaryExpr: {
            TernaryExpr* t = static_cast<TernaryExpr*>(e);
            collectExprFuncRefs(t->condition.get(), out);
            collectExprFuncRefs(t->trueValue.get(), out);
            collectExprFuncRefs(t->falseValue.get(), out);
            return;
        }
        case NodeType::CastExpr:
            collectExprFuncRefs(static_cast<CastExpr*>(e)->operand.get(), out);
            return;
        case NodeType::LambdaExpr:
            collectLambdaBodyRefs(static_cast<LambdaExpr*>(e)->body.get(), out);
            return;
        default:
            return;  // 字面量/标识符/自身/父类 等无嵌套
    }
}

// 收集一条语句中的函数引用
void collectStmtFuncRefs(Stmt* s, std::unordered_set<std::string>& out);

void collectLambdaBodyRefs(BlockStmt* body, std::unordered_set<std::string>& out) {
    if (body == nullptr) return;
    for (auto& s : body->statements) collectStmtFuncRefs(s.get(), out);
}

void collectStmtFuncRefs(Stmt* s, std::unordered_set<std::string>& out) {
    if (s == nullptr) return;
    switch (s->getType()) {
        case NodeType::ExprStmt:
            collectExprFuncRefs(static_cast<ExprStmt*>(s)->expr.get(), out);
            return;
        case NodeType::VarDecl: {
            VarDecl* v = static_cast<VarDecl*>(s);
            if (v->initializer != nullptr) {
                collectExprFuncRefs(v->initializer.get(), out);
            }
            return;
        }
        case NodeType::BlockStmt:
            for (auto& sub : static_cast<BlockStmt*>(s)->statements) {
                collectStmtFuncRefs(sub.get(), out);
            }
            return;
        case NodeType::IfStmt: {
            IfStmt* f = static_cast<IfStmt*>(s);
            collectExprFuncRefs(f->condition.get(), out);
            if (f->thenBranch != nullptr) {
                for (auto& sub : f->thenBranch->statements) {
                    collectStmtFuncRefs(sub.get(), out);
                }
            }
            if (f->elseBranch != nullptr) collectStmtFuncRefs(f->elseBranch.get(), out);
            return;
        }
        case NodeType::WhileStmt: {
            WhileStmt* w = static_cast<WhileStmt*>(s);
            collectExprFuncRefs(w->condition.get(), out);
            if (w->body != nullptr) {
                for (auto& sub : w->body->statements) {
                    collectStmtFuncRefs(sub.get(), out);
                }
            }
            return;
        }
        case NodeType::ForStmt: {
            ForStmt* fr = static_cast<ForStmt*>(s);
            if (fr->init != nullptr) collectStmtFuncRefs(fr->init.get(), out);
            collectExprFuncRefs(fr->condition.get(), out);
            collectExprFuncRefs(fr->update.get(), out);
            if (fr->body != nullptr) {
                for (auto& sub : fr->body->statements) {
                    collectStmtFuncRefs(sub.get(), out);
                }
            }
            return;
        }
        case NodeType::ReturnStmt:
            if (static_cast<ReturnStmt*>(s)->value != nullptr) {
                collectExprFuncRefs(static_cast<ReturnStmt*>(s)->value.get(), out);
            }
            return;
        case NodeType::SwitchStmt: {
            SwitchStmt* sw = static_cast<SwitchStmt*>(s);
            collectExprFuncRefs(sw->condition.get(), out);
            for (auto& c : sw->cases) {
                for (auto& sub : c->statements) collectStmtFuncRefs(sub.get(), out);
            }
            if (sw->defaultCase != nullptr) {
                for (auto& sub : sw->defaultCase->statements) {
                    collectStmtFuncRefs(sub.get(), out);
                }
            }
            return;
        }
        default:
            return;  // 中断/继续/情况/默认 无嵌套表达式
    }
}

// 按模块（crate）分桶的类型去重表：模块名 -> 已合并类型名集合。
// v2.0 crate 隔离（决策4）：跨模块同名类型允许（crate 各自命名空间），
//   仅同一模块内重名报错。取代 v1.0 的全局 seenTypes 去重。
using CrateTypeBuckets = std::unordered_map<std::string, std::unordered_set<std::string>>;

// 合并一个模块的声明到 out（所有权 release 转移）。
// 入口模块（entryModule=true）合并全部声明；被导入模块仅合并公开声明（私有不跨模块）。
// 返回 false 表示存在同模块类型重名冲突（diags 已记录；冲突声明跳过合并）。
bool mergeModuleDecls(ModuleUnit& unit, Program* out, bool entryModule, bool singleModule,
                      CrateTypeBuckets& typeBuckets, Diagnostics& diags) {
    bool ok = true;
    // 声明是否跨模块可见：入口模块全部可见；被导入模块仅公开可见
    auto visible = [entryModule](AccessSpecifier a) {
        return entryModule || a == AccessSpecifier::Public;
    };
    // 本模块的类型分桶（crate 隔离：同名类型跨模块互不冲突）
    std::unordered_set<std::string>& seenTypes = typeBuckets[unit.moduleName];
    // 第 4 层（P2-6）：单文件场景（无导入，singleModule=true）不启用 crate 前缀——
    //   单文件模块名=文件主干（如 函数.cn -> 函数），加前缀会破坏既有链接符号
    //   （codegen 对 主->cn_main、打印->__cn_print_* 的映射基于纯名）。仅多模块
    //   （含导入）才写入 moduleName，供 IR 层加 模块名$ 链接前缀。
    const std::string crateName = singleModule ? "" : unit.moduleName;

    // ---- 导入声明：所有权转移（所有模块的导入都保留到 merged Program）----
    // 语义层 visitProgram 从 Program::imports 收集 importedModules_（限定调用
    //   "模块.函数" 重写识别用）。被导入模块内部的限定调用同样需要重写，
    //   故全部模块的 ImportDecl 一并合并（合并阶段不校验符号，仅记录模块名）。
    for (auto& imp : unit.ast->imports) {
        // plans/018 P6b 工作流2：写入来源模块归属（「导入与本地定义同名」
        //   E0255 检查按归属文件的本模块定义比对——合并后声明 moduleName
        //   == ownerModule 的项即该文件的本地定义）
        imp->ownerModule = unit.moduleName;
        out->imports.push_back(std::unique_ptr<ImportDecl>(imp.release()));
    }
    // ---- 函数（含重载）：所有权转移；重复定义检测交给语义层 registerFunction
    //      （其已处理"原型+定义"组合与重载）。v2.0 crate 隔离：跨模块同名函数
    //      允许（各模块独立命名空间），仅同模块内重名报错——语义层 registerFunction
    //      按 sigKey 分模块判定（moduleName 已写入 FunctionDecl）。----
    // 缺陷4 修复（私有依赖闭包）：被导入模块的公开函数体内可能直接调用本模块
    //   私有函数。私有函数不跨模块可见，但公开函数体需要它才能编译——须把
    //   被公开函数（及其闭包链）引用的私有函数一并合并（它们对入口不可见，仅
    //   作为内部实现存在）。采用两阶段：先合并公开函数，再按"公开函数引用集"
    //   迭代补齐私有函数闭包。
    if (!entryModule) {
        // 1) 收集本模块全部私有函数名（含其 sigKey 复用函数名）
        std::unordered_set<std::string> privateFuncs;
        for (const auto& f : unit.ast->declarations) {
            if (f->access != AccessSpecifier::Public) privateFuncs.insert(f->name);
        }
        // 2) 从公开函数体收集被引用的函数名（种子）
        std::unordered_set<std::string> needed;
        for (const auto& f : unit.ast->declarations) {
            if (f->access != AccessSpecifier::Public || f->body == nullptr) continue;
            for (const auto& s : f->body->statements) {
                collectStmtFuncRefs(s.get(), needed);
            }
        }
        // 3) 迭代闭包：只要 needed 里出现了本模块私有函数，就继续收集其引用
        bool changed = true;
        while (changed) {
            changed = false;
            std::unordered_set<std::string> added;
            for (const auto& name : needed) {
                if (privateFuncs.count(name) == 0) continue;
                // 找到该私有函数定义，收集其体引用
                for (const auto& f : unit.ast->declarations) {
                    if (f->name == name && f->access != AccessSpecifier::Public &&
                        f->body != nullptr) {
                        for (const auto& s : f->body->statements) {
                            collectStmtFuncRefs(s.get(), added);
                        }
                    }
                }
            }
            for (const auto& name : added) {
                if (needed.insert(name).second) changed = true;
            }
        }
        // 4) 合并：公开函数 + 被闭包引用的私有函数（写入 crate 域 moduleName）
        for (auto& f : unit.ast->declarations) {
            const bool isPublic = (f->access == AccessSpecifier::Public);
            const bool inClosure = (needed.count(f->name) > 0);
            if (!isPublic) {
                // 609-a（T100·170）：私有符号登记（不合并·仅存在性登记——
                // 语义层 591-a 导入位私有拒绝查全集；含闭包私有=仍不可直接导入）
                out->modulePrivateSymbols.emplace_back(unit.moduleName, f->name);
                if (!inClosure) continue;  // 私有且未被引用：不合并
            }
            f->moduleName = crateName;              // crate 分桶/链接前缀（第 4 层）
            out->declarations.push_back(std::unique_ptr<FunctionDecl>(f.release()));
        }
    } else {
        for (auto& f : unit.ast->declarations) {
            f->moduleName = crateName;  // crate 分桶/链接前缀（第 4 层）
            out->declarations.push_back(std::unique_ptr<FunctionDecl>(f.release()));
        }
    }
    // ---- 顶层常量/静态（第 4 层，v2.0 决策8/9，P1-4/P3-8）----
    // crate 级常量/静态变量：入口模块全部合并；被导入模块仅合并公开的。
    //   模块级私有常量不跨模块（与函数/类可见性规则一致）。
    for (auto& g : unit.ast->globals) {
        if (!visible(g->access)) {
            // 609-a（T100·170）：私有常量/静态存在性登记（同函数面）
            out->modulePrivateSymbols.emplace_back(unit.moduleName, g->name);
            continue;
        }
        if (entryModule) {
            g->moduleName = crateName;
            out->globals.push_back(std::unique_ptr<VarDecl>(g.release()));
        } else if (g->access == AccessSpecifier::Public) {
            g->moduleName = crateName;
            out->globals.push_back(std::unique_ptr<VarDecl>(g.release()));
        }
    }
    // ---- 结构体/联合体 ----
    for (auto& s : unit.ast->structs) {
        if (!visible(s->access)) {
            // 609-a（T100·170）：私有类型存在性登记（同函数面）
            out->modulePrivateSymbols.emplace_back(unit.moduleName, s->name);
            continue;
        }
        if (!seenTypes.insert(s->name).second) {
            diags.report(DiagnosticLevel::Error, s->location,
                         "模块 '" + unit.moduleName + "' 内重复声明类型 '" + s->name + "'");
            ok = false;
            continue;  // 冲突：跳过合并（源 AST 仍持有所有权，正常释放）
        }
        s->moduleName = crateName;  // crate 域（第 4 层）
        out->structs.push_back(std::unique_ptr<StructDecl>(s.release()));
    }
    // ---- 枚举 ----
    for (auto& e : unit.ast->enums) {
        if (!visible(e->access)) {
            // 609-a（T100·170）：私有枚举存在性登记（同函数面）
            out->modulePrivateSymbols.emplace_back(unit.moduleName, e->name);
            continue;
        }
        if (!seenTypes.insert(e->name).second) {
            diags.report(DiagnosticLevel::Error, e->location,
                         "模块 '" + unit.moduleName + "' 内重复声明类型 '" + e->name + "'");
            ok = false;
            continue;
        }
        e->moduleName = crateName;  // crate 域（第 4 层）
        out->enums.push_back(std::unique_ptr<EnumDecl>(e.release()));
    }
    // ---- 类 ----
    for (auto& c : unit.ast->classes) {
        if (!visible(c->access)) {
            // 609-a（T100·170）：私有类存在性登记（同函数面）
            out->modulePrivateSymbols.emplace_back(unit.moduleName, c->name);
            continue;
        }
        if (!seenTypes.insert(c->name).second) {
            diags.report(DiagnosticLevel::Error, c->location,
                         "模块 '" + unit.moduleName + "' 内重复声明类型 '" + c->name + "'");
            ok = false;
            continue;
        }
        c->moduleName = crateName;  // crate 域（第 4 层；可见性交集检查依据）
        out->classes.push_back(std::unique_ptr<ClassDecl>(c.release()));
    }
    // ---- 接口 ----
    for (auto& i : unit.ast->interfaces) {
        if (!visible(i->access)) {
            // 609-a（T100·170）：私有接口存在性登记（同函数面）
            out->modulePrivateSymbols.emplace_back(unit.moduleName, i->name);
            continue;
        }
        if (!seenTypes.insert(i->name).second) {
            diags.report(DiagnosticLevel::Error, i->location,
                         "模块 '" + unit.moduleName + "' 内重复声明类型 '" + i->name + "'");
            ok = false;
            continue;
        }
        i->moduleName = crateName;  // crate 域（第 4 层）
        out->interfaces.push_back(std::unique_ptr<InterfaceDecl>(i.release()));
    }
    // ---- 泛型声明（Task 3.8，E2E 26 修复；Task 6.1 泛型跨模块打通）----
    // 入口模块泛型声明全部合并（registerGenerics 依赖）；被导入模块合并 公开 泛型
    //   （泛型声明内嵌 innerClass/innerFunc 自带模块级 access——公开: 标签后的
    //   泛型声明跨模块可见；私有 泛型不跨模块，与函数/类可见性规则一致）。
    //   Task 6.1：stdlib/核心.cn（交换/最小/最大）与 容器.cn（向量/链表/栈/队列）
    //   为被导入标准库模块，其公开泛型须跨模块合并才能实例化使用。
    // 类型重名检测：泛型类/函数名加入本模块分桶（与普通类/函数冲突检测，crate 隔离）。
    for (auto& g : unit.ast->generics) {
        const bool genPublic = (g->innerClass != nullptr)
                                   ? (g->innerClass->access == AccessSpecifier::Public)
                                   : (g->innerFunc != nullptr &&
                                      g->innerFunc->access == AccessSpecifier::Public);
        if (!entryModule && !genPublic) {
            // 609-a（T100·170）：私有泛型存在性登记（同函数面；符号名在其内层声明）
            std::string gpname;
            if (g->innerClass != nullptr) gpname = g->innerClass->name;
            else if (g->innerFunc != nullptr) gpname = g->innerFunc->name;
            if (!gpname.empty())
                out->modulePrivateSymbols.emplace_back(unit.moduleName, gpname);
            continue;  // 私有泛型不跨模块
        }
        std::string gname;
        if (g->innerClass != nullptr) gname = g->innerClass->name;
        else if (g->innerFunc != nullptr) gname = g->innerFunc->name;
        if (!gname.empty() && !seenTypes.insert(gname).second) {
            diags.report(DiagnosticLevel::Error, g->location,
                         "模块 '" + unit.moduleName + "' 内重复声明类型 '" + gname + "'");
            ok = false;
            continue;
        }
        g->moduleName = crateName;  // crate 域（第 4 层）
        out->generics.push_back(std::unique_ptr<GenericDecl>(g.release()));
    }
    return ok;
}

} // namespace

// 合并多个模块 AST 为单一 Program（Task 3.6；第 4 层 crate 分桶）
// ordered 为拓扑排序结果（被依赖者在前）。
// 入口模块判定 = ModuleUnit::isEntryUnit（簇⑥ 根治，2026-09-05 方案A：命令行
//   显式入口标记，loadModuleTree 根调用写入）：入口全部声明（含私有）保留；
//   被导入模块（递归加载的依赖）仅合并公开声明 + 公开函数闭包引用的私有函数。
//   取代旧「模块名==主 + 单模块特判」双轨判定——两者均会把含导入的非主.cn
//   命令行入口误判为被导入模块（E2E 26 单文件形态 / 簇⑥ 组件自检形态两度发作）。
// v2.0 crate 隔离：类型重名按模块分桶（跨模块同名允许），函数重复定义
//   由语义层 registerFunction 按 moduleName 分模块判定（同模块重名才报错）。
bool mergeModules(const std::vector<ModuleUnit*>& ordered, Program* out, Diagnostics& diags) {
    CrateTypeBuckets typeBuckets;  // 模块名 -> 已合并类型名集合（crate 分桶）
    bool ok = true;
    // singleModule 仅用于 mergeModuleDecls 的 crateName 前缀决策（单文件不加
    //   模块名链接前缀）——不再参与入口判定（isEntryUnit 已统一覆盖单文件形态）。
    const bool singleModule = (ordered.size() == 1);
    for (ModuleUnit* unit : ordered) {
        if (unit == nullptr || unit->ast == nullptr) continue;
        const bool isEntry = unit->isEntryUnit;
        if (!mergeModuleDecls(*unit, out, isEntry, singleModule, typeBuckets, diags)) ok = false;
    }
    return ok;
}

// ==================== 入口识别 ====================

// 模块名 == 主（即文件名 主.cn，规范08-四 的文件名约定查询）。
// 注意：合并阶段入口判定已由 isEntryUnit 承担（簇⑥ 根治）——本函数仅供
//   文件名约定查询（单测覆盖），不再是入口语义依据。
bool isEntryModule(const ModuleUnit& unit) {
    return unit.moduleName == "主";
}

} // namespace module
} // namespace cn_compiler
