// 语义分析器实现：导入/模块声明族（350 重构E2 自 semantic_decl.cpp 纯机械搬移）
// 职责（函数体逐字搬移·零语义变化；声明保持在 semantic.hpp 原位）：
//   visitImportDecl        —— 导入/模块声明（①具名绑定/②花括号/③通配分派）
//   bindImportedSymbols    —— 尾段符号具名绑定（218-a 自 visitImportDecl 拆出）
//   joinPathSegments       —— 路径段 join（"::" 连接）
//   checkImportLocalConflicts —— plans/018 P6b 导入冲突检查（①×②/②×②）
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

void SemanticAnalyzer::visitImportDecl(ImportDecl* node) {
    if (node == nullptr || node->segments.empty()) return;
    // ---- 模块声明（模块 X）：仅建立模块树引用，不引入符号 ----
    if (node->isModuleDecl) return;
    // ---- 路径导入 / 重命名导入 / 通配符导入 ----
    // 取路径首段为模块名（跨包路径 包名::模块::符号 首段 = 包名；当前实现
    //   以首段为 crate 边界，多段路径按模块名 = 完整路径前缀解析）
    std::string moduleName = node->segments[0];
    UseImportInfo& use = useImports_[moduleName];
    if (node->wildcard) {
        // 导入 模块::*：通配符导入（模块全部公开符号，③ 通配层）
        use.wildcard = true;
        importedModules_.insert(moduleName);
        return;
    }
    if (!node->names.empty()) {
        // 导入 路径::{项1 [作为 别名], ...}：花括号导入（② 具名绑定）
        // A-5（花括号项别名跨模块同名）：记录每个导入项的来源模块**完整路径**
        //   （工具库::格式化）——纯名调用重写回原符号名后按完整路径过滤；
        //   首段（工具库）过滤在跨 crate 场景会漏掉 格式化 模块条目
        const std::string braceFullPath = joinPathSegments(node->segments);
        for (const auto& item : node->names) {
            if (item.name.empty()) continue;
            use.symbols.insert(item.name);
            if (!item.alias.empty()) {
                use.aliases[item.alias] = item.name;
                itemAliasModules_[item.alias] = braceFullPath;
            } else {
                use.aliases[item.name] = item.name;
                itemAliasModules_[item.name] = braceFullPath;
            }
        }
        importedModules_.insert(moduleName);
        return;
    }
    // ---- 路径导入（无花括号）----
    // plans/018 呈报一B（2026-09-07 用户终裁）：「导入 m::符号」= 具名绑定②
    //   （Rust use m::f 一致）——把尾段符号真搬进本文件命名空间，与本文件
    //   本地定义同名 = 编译错误（E0255，checkImportLocalConflicts）。
    //   旧实现「模块级通配（wildcard=true）」废止；限定调用资格由
    //   「模块已加载」判定（visitCallExpr，P1-1 废止），不再依赖导入。
    // 未合并单文件（ownerModule 空，runPipeline/单测直构 Program）：无模块
    //   加载概念，保持旧通配行为（与 checkImportLocalConflicts 的 owner
    //   空跳过同口径）。
    if (node->ownerModule.empty() || node->segments.size() < 2) {
        // 单段无别名 = 模块整体导入（③ 通配层）；模块重命名（导入 模块 作为
        //   别名）同层——别名绑定模块级符号（调用 别名::符号 按原模块解析）
        use.wildcard = true;
        if (!node->alias.empty()) {
            use.aliases[node->alias] = moduleName;
            // A-5（整路径重命名绑定模块级别名）：别名绑定完整路径（含子模块/包
            //   路径 甲::乙），并登记别名本身可导入（别名::符号 限定调用路径解析）
            const std::string fullPath = joinPathSegments(node->segments);
            moduleAliases_[node->alias] = fullPath;
            importedModules_.insert(node->alias);
            useImports_[node->alias].wildcard = true;
        }
        importedModules_.insert(moduleName);
        return;
    }
    // 完整路径（join 全部段）：命中已加载模块名 = 尾段是模块（如 自举组件
    //   导入 CN语言编译器::词法分析; / 父挂子 导入 网络库::内部工具;）——
    //   ③ 模块导入（Rust use a::b 绑定模块名；限定调用按加载判定放行，
    //   旧通配语义对限定调用无观察差异，保持防回归）。
    const std::string fullPath = joinPathSegments(node->segments);
    // 千行拆分轮（2026-09-08）：尾段本身=已加载模块名（v1 再导出包
    //   CN语言编译器/包.cn 的 导入 CN语言编译器::IR生成，IR生成=组件 unit 名）
    //   同样判③模块导入——旧只查完整路径，尾段被误判符号走②具名绑定，
    //   itemAliasModules_[IR生成]=首段 crate 名 -> 纯名调用 moduleFilter=
    //   首段 -> 决议过滤器拒绝真实条目（entryModule=IR生成）→「未找到
    //   匹配的函数」。Rust 对照：use a::b 按 b 的实际 def（模块/符号）分派。
    // 尾段命中收窄为「两段导入」（a::b，b=已加载 unit 名——v1 再导出包
    //   形态）：三段及以上（网络库::传输控制::连接，第 8 层跨 crate 符号
    //   导入）保持原 fullPath 判定——「连接」等函数名与无关 unit 撞名时
    //   不得误判为模块导入（47_package_cargo 回归实测）。
    const std::string& importLastSeg = node->segments.back();
    const bool twoSegModuleImport = node->segments.size() == 2 &&
                                    knownModules_.count(importLastSeg) > 0;
    if (knownModules_.count(fullPath) > 0 || twoSegModuleImport) {
        use.wildcard = true;
        if (!node->alias.empty()) {
            // 导入 甲::乙 作为 丙（甲::乙 是模块）：模块重命名（A-5）
            use.aliases[node->alias] = fullPath;
            moduleAliases_[node->alias] = fullPath;
            importedModules_.insert(node->alias);
            useImports_[node->alias].wildcard = true;
        }
        importedModules_.insert(moduleName);
        return;
    }
    // 尾段是符号：② 具名绑定（218-a 迁 bindImportedSymbols——A-5 过滤哨兵/
    //   自导入豁免随体迁移）。
    bindImportedSymbols(node, use, moduleName);
}

// 218-a（2026-09-16 第两百一十八轮，D1 行数整改）：尾段符号具名绑定——来源模块
//   完整路径=去尾段（工具库::格式化::版本 -> 工具库::格式化）——纯名调用重写按此
//   过滤（A-5 同款）；自导入（来源首段 == 归属模块）不引入绑定名（本地定义恒
//   ① 优先）；两段符号导入过滤器置空哨兵（crate 名与注册模块名不同名回归根治）。
void SemanticAnalyzer::bindImportedSymbols(ImportDecl* node, UseImportInfo& use,
                                           const std::string& moduleName) {
    std::string srcModule;
    for (std::size_t si = 0; si + 1 < node->segments.size(); ++si) {
        if (si > 0) srcModule += "::";
        srcModule += node->segments[si];
    }
    const std::string& sym = node->segments.back();
    const std::string bindName = node->alias.empty() ? sym : node->alias;
    const bool selfImport = (moduleName == node->ownerModule);
    // 591-a（T100·170 对齐 v2 sem5/规格08 5.9.3）：跨模块具名导入私有符号拒绝——
    //   模块已知 且 符号在模块声明全集 但 非公开 → Error（v2 措辞对齐）；
    //   符号不在全集=不存在 → 保持既有（使用点未声明错·不劫持）。
    //   豁免：自导入（既有）；ownerModule 空=单文件管线（无模块加载·既有同口径）。
    if (!selfImport && !node->ownerModule.empty() &&
        knownModules_.count(moduleName) > 0) {
        const auto& allIt = moduleAllSymbols_.find(moduleName);
        const bool symExists = allIt != moduleAllSymbols_.end() &&
                               allIt->second.count(sym) > 0;
        if (symExists) {
            const auto& pubIt = modulePublicSymbols_.find(moduleName);
            const bool isPublic = pubIt != modulePublicSymbols_.end() &&
                                  pubIt->second.count(sym) > 0;
            if (!isPublic) {
                diagnostics_.report(
                    DiagnosticLevel::Error, node->location,
                    "符号 '" + sym + "' 为模块 '" + moduleName +
                        "' 私有（未以 公开: 标注），不可导入（规格08 5.9.3）");
                return;
            }
        }
    }
    if (!selfImport) {
        use.symbols.insert(sym);
        use.aliases[bindName] = sym;
        itemAliasModules_[bindName] = node->segments.size() == 2 ? "" : srcModule;
    }
    importedModules_.insert(moduleName);
}

// 218-a：路径段 join（"::" 连接）——visitImportDecl 内三处同构循环的单一归属。
std::string SemanticAnalyzer::joinPathSegments(const std::vector<std::string>& segments) {
    std::string joined;
    for (std::size_t si = 0; si < segments.size(); ++si) {
        if (si > 0) joined += "::";
        joined += segments[si];
    }
    return joined;
}

// plans/018 P6b 工作流2（规格08-三 3.6 名称解析，Rust E0252 对照）：
//   显式导入冲突检查——visitProgram 第零趟b（导入表构建）后调用。
//   ①×②：显式导入符号与归属文件本地定义同名 = 编译错误「导入与本地定义同名」
//     （此前静默接受且错编：纯名调用绑定 模块$符号 而定义侧发射不一致，
//     链接期 undefined reference——基线 p6b_base3 实证）。
//   ②×②：同归属文件把同一绑定名从不同外部来源显式导入 = 编译错误「多次显式
//     导入同名」（同来源重复导入幂等合法；纯名使用点歧义另有既有诊断覆盖）。
//   自导入（来源首段 == 归属模块，如 52 的 导入 主::版本）完全跳过：不引入
//     新名字（本地定义恒 ① 优先），仅启用限定自引用，与任何导入不构成冲突。
//   显式导入 = 花括号项 / 多段路径尾段（含 作为 别名改写绑定名）；模块整体
//     导入（单段无别名）与通配符属 ③ 通配层，不参与本检查（③×③ 冲突由
//     使用点歧义诊断覆盖，A-2 类型/常量多模块限定诊断已实证）。
void SemanticAnalyzer::checkImportLocalConflicts(Program* node) {
    // 绑定名 -> (归属模块, 来源路径)：同归属不同来源 = ②×②
    std::unordered_map<std::string,
                       std::pair<std::string, std::string>> explicitImports;
    for (const auto& imp : node->imports) {
        if (imp->isModuleDecl || imp->wildcard) continue;
        // 提取显式导入的绑定名（别名优先——绑定名是 别名）
        std::vector<std::string> bindings;
        if (!imp->names.empty()) {
            for (const auto& item : imp->names) {
                bindings.push_back(item.alias.empty() ? item.name : item.alias);
            }
        } else if (imp->segments.size() >= 2) {
            bindings.push_back(imp->alias.empty() ? imp->segments.back() : imp->alias);
        } else {
            continue;  // 单段无别名 = 模块整体导入（③ 通配层）
        }
        const std::string& owner = imp->ownerModule;
        if (owner.empty()) continue;  // 未合并单文件（无归属），无冲突面
        const bool selfImport =
            (!imp->segments.empty() && imp->segments[0] == owner);
        for (const auto& bind : bindings) {
            // ②×②：同归属文件、不同外部来源的同名显式导入
            auto it = explicitImports.find(bind);
            if (it != explicitImports.end()) {
                if (!selfImport && it->second.first == owner &&
                    it->second.second != imp->importPath) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, imp->location,
                        "多次显式导入同名 '" + bind + "'（" + it->second.second +
                            " 与 " + imp->importPath + "，规格08-三 名称解析）");
                }
                continue;
            }
            if (!selfImport) {
                explicitImports[bind] = {owner, imp->importPath};
            }
            if (selfImport) continue;  // 自导入不参与 ①×②（与本地定义同源）
            // ①×②：与归属文件本地定义同名（函数/结构体/枚举/类/接口/泛型/
            //        顶层常量与静态——合并后 moduleName == owner 即本地定义）
            bool conflicted = false;
            for (const auto& f : node->declarations) {
                if (f->moduleName == owner && f->name == bind) {
                    conflicted = true;
                    break;
                }
            }
            if (!conflicted) {
                for (const auto& t : node->structs) {
                    if (t->moduleName == owner && t->name == bind) {
                        conflicted = true;
                        break;
                    }
                }
            }
            if (!conflicted) {
                for (const auto& t : node->enums) {
                    if (t->moduleName == owner && t->name == bind) {
                        conflicted = true;
                        break;
                    }
                }
            }
            if (!conflicted) {
                for (const auto& c : node->classes) {
                    if (c->moduleName == owner && c->name == bind) {
                        conflicted = true;
                        break;
                    }
                }
            }
            if (!conflicted) {
                for (const auto& i : node->interfaces) {
                    if (i->moduleName == owner && i->name == bind) {
                        conflicted = true;
                        break;
                    }
                }
            }
            if (!conflicted) {
                for (const auto& g : node->generics) {
                    const std::string gname =
                        (g->innerClass != nullptr) ? g->innerClass->name
                                                   : (g->innerFunc != nullptr)
                                                         ? g->innerFunc->name
                                                         : "";
                    if (g->moduleName == owner && gname == bind) {
                        conflicted = true;
                        break;
                    }
                }
            }
            if (!conflicted) {
                for (const auto& g : node->globals) {
                    if (g->moduleName == owner && g->name == bind) {
                        conflicted = true;
                        break;
                    }
                }
            }
            if (conflicted) {
                diagnostics_.report(
                    DiagnosticLevel::Error, imp->location,
                    "导入与本地定义同名 '" + bind + "'（导入 " + imp->importPath +
                        " 与当前文件定义冲突，规格08-三 名称解析）");
            }
        }
    }
}

} // namespace cn_compiler
