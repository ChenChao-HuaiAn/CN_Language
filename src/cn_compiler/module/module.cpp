// CN语言编译器模块系统实现（Task 3.6，规格书08 模块系统）
// 实现要点：
//   1. readSourceFile：UTF-8 读取（去 BOM），与 cn_main.cpp 同逻辑（模块层独立提供）
//   2. parseSourceText：词法 + 语法 + 导入依赖收集（模块名去重）
//   3. ModuleGraph：依赖图 + DFS 拓扑排序（三色标记检测循环依赖）
//   4. mergeModules：公开符号合并 + 跨模块类型重名冲突检测
//      所有权转移：源 AST 声明节点 release() 移交目标 Program（避免 double-free）
//   5. isEntryModule：入口识别（模块名 == 主，即 主.cn）
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

    // 收集导入依赖（模块名去重；importPath 取首个 :: 前段为模块名，
    //   如 数学::平方根 -> 数学；网络协议::HTTP::请求 -> 网络协议；v2.0 :: 分隔）
    for (const auto& imp : ast->imports) {
        std::string dep = imp->importPath;
        const std::size_t sep = dep.find("::");
        if (sep != std::string::npos) dep = dep.substr(0, sep);
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
    units_[name] = std::move(unit);
    return true;
}

// 查找模块（未找到返回 nullptr）
ModuleUnit* ModuleGraph::findModule(const std::string& moduleName) {
    auto it = units_.find(moduleName);
    return (it == units_.end()) ? nullptr : it->second.get();
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
            if (!dep.empty() && dep != name) {
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

// 合并一个模块的声明到 out（所有权 release 转移）。
// 入口模块（entryModule=true）合并全部声明；被导入模块仅合并公开声明（私有不跨模块）。
// 返回 false 表示存在跨模块类型重名冲突（diags 已记录；冲突声明跳过合并）。
bool mergeModuleDecls(ModuleUnit& unit, Program* out, bool entryModule,
                      std::unordered_set<std::string>& seenTypes, Diagnostics& diags) {
    bool ok = true;
    // 声明是否跨模块可见：入口模块全部可见；被导入模块仅公开可见
    auto visible = [entryModule](AccessSpecifier a) {
        return entryModule || a == AccessSpecifier::Public;
    };

    // ---- 导入声明：所有权转移（所有模块的导入都保留到 merged Program）----
    // 语义层 visitProgram 从 Program::imports 收集 importedModules_（限定调用
    //   "模块.函数" 重写识别用）。被导入模块内部的限定调用同样需要重写，
    //   故全部模块的 ImportDecl 一并合并（合并阶段不校验符号，仅记录模块名）。
    for (auto& imp : unit.ast->imports) {
        out->imports.push_back(std::unique_ptr<ImportDecl>(imp.release()));
    }
    // ---- 函数（含重载）：所有权转移；重复定义检测交给语义层 registerFunction
    //      （其已处理"原型+定义"组合与重载；跨模块同签名重名会在语义层报重复定义）----
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
        // 4) 合并：公开函数 + 被闭包引用的私有函数
        for (auto& f : unit.ast->declarations) {
            const bool isPublic = (f->access == AccessSpecifier::Public);
            const bool inClosure = (needed.count(f->name) > 0);
            if (!isPublic && !inClosure) continue;  // 私有且未被引用：不合并
            out->declarations.push_back(std::unique_ptr<FunctionDecl>(f.release()));
        }
    } else {
        for (auto& f : unit.ast->declarations) {
            out->declarations.push_back(std::unique_ptr<FunctionDecl>(f.release()));
        }
    }
    // ---- 结构体/联合体 ----
    for (auto& s : unit.ast->structs) {
        if (!visible(s->access)) continue;
        if (!seenTypes.insert(s->name).second) {
            diags.report(DiagnosticLevel::Error, s->location,
                         "跨模块重复声明类型 '" + s->name + "'（模块 " + unit.moduleName + "）");
            ok = false;
            continue;  // 冲突：跳过合并（源 AST 仍持有所有权，正常释放）
        }
        out->structs.push_back(std::unique_ptr<StructDecl>(s.release()));
    }
    // ---- 枚举 ----
    for (auto& e : unit.ast->enums) {
        if (!visible(e->access)) continue;
        if (!seenTypes.insert(e->name).second) {
            diags.report(DiagnosticLevel::Error, e->location,
                         "跨模块重复声明类型 '" + e->name + "'（模块 " + unit.moduleName + "）");
            ok = false;
            continue;
        }
        out->enums.push_back(std::unique_ptr<EnumDecl>(e.release()));
    }
    // ---- 类 ----
    for (auto& c : unit.ast->classes) {
        if (!visible(c->access)) continue;
        if (!seenTypes.insert(c->name).second) {
            diags.report(DiagnosticLevel::Error, c->location,
                         "跨模块重复声明类型 '" + c->name + "'（模块 " + unit.moduleName + "）");
            ok = false;
            continue;
        }
        out->classes.push_back(std::unique_ptr<ClassDecl>(c.release()));
    }
    // ---- 接口 ----
    for (auto& i : unit.ast->interfaces) {
        if (!visible(i->access)) continue;
        if (!seenTypes.insert(i->name).second) {
            diags.report(DiagnosticLevel::Error, i->location,
                         "跨模块重复声明类型 '" + i->name + "'（模块 " + unit.moduleName + "）");
            ok = false;
            continue;
        }
        out->interfaces.push_back(std::unique_ptr<InterfaceDecl>(i.release()));
    }
    // ---- 泛型声明（Task 3.8，E2E 26 修复；Task 6.1 泛型跨模块打通）----
    // 入口模块泛型声明全部合并（registerGenerics 依赖）；被导入模块合并 公开 泛型
    //   （泛型声明内嵌 innerClass/innerFunc 自带模块级 access——公开: 标签后的
    //   泛型声明跨模块可见；私有 泛型不跨模块，与函数/类可见性规则一致）。
    //   Task 6.1：stdlib/核心.cn（交换/最小/最大）与 容器.cn（向量/链表/栈/队列）
    //   为被导入标准库模块，其公开泛型须跨模块合并才能实例化使用。
    // 类型重名检测：泛型类/函数名加入 seenTypes（与普通类/函数冲突检测）。
    for (auto& g : unit.ast->generics) {
        const bool genPublic = (g->innerClass != nullptr)
                                   ? (g->innerClass->access == AccessSpecifier::Public)
                                   : (g->innerFunc != nullptr &&
                                      g->innerFunc->access == AccessSpecifier::Public);
        if (!entryModule && !genPublic) continue;  // 私有泛型不跨模块
        std::string gname;
        if (g->innerClass != nullptr) gname = g->innerClass->name;
        else if (g->innerFunc != nullptr) gname = g->innerFunc->name;
        if (!gname.empty() && !seenTypes.insert(gname).second) {
            diags.report(DiagnosticLevel::Error, g->location,
                         "跨模块重复声明类型 '" + gname + "'（模块 " + unit.moduleName + "）");
            ok = false;
            continue;
        }
        out->generics.push_back(std::unique_ptr<GenericDecl>(g.release()));
    }
    return ok;
}

} // namespace

// 合并多个模块 AST 为单一 Program（Task 3.6）
// ordered 为拓扑排序结果（被依赖者在前）。
// 入口模块判定 = isEntryModule（模块名 == 主，即 主.cn，规范08-四）：
//   其全部声明（含私有）保留；被导入模块（非入口）仅合并公开声明。
bool mergeModules(const std::vector<ModuleUnit*>& ordered, Program* out, Diagnostics& diags) {
    std::unordered_set<std::string> seenTypes;  // 已合并类型名（结构体/枚举/类/接口）
    bool ok = true;
    // 单模块场景（无导入）：唯一模块即入口（E2E 26 修复——单文件用例模块名
    //   非 主（如 泛型模板.cn），isEntryModule 判 false 导致泛型/私有被过滤，
    //   单测走 Parser.parse 无此问题，runModulePipeline 必经 mergeModules）。
    const bool singleModule = (ordered.size() == 1);
    for (ModuleUnit* unit : ordered) {
        if (unit == nullptr || unit->ast == nullptr) continue;
        const bool isEntry = singleModule || isEntryModule(*unit);
        if (!mergeModuleDecls(*unit, out, isEntry, seenTypes, diags)) ok = false;
    }
    return ok;
}

// ==================== 入口识别 ====================

// 该模块是否为程序入口：模块名 == 主（即文件名 主.cn，不含扩展名）
bool isEntryModule(const ModuleUnit& unit) {
    return unit.moduleName == "主";
}

} // namespace module
} // namespace cn_compiler
