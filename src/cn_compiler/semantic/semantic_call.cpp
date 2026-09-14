// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"
#include "cn_compiler/semantic/semantic_internal.hpp"

namespace cn_compiler {



void SemanticAnalyzer::wrapRefArgs(CallExpr* node,
                                   const std::vector<std::string>& paramTypes) {
    const std::size_t n = std::min(node->arguments.size(), paramTypes.size());
    for (std::size_t i = 0; i < n; ++i) {
        if (!types::isReference(paramTypes[i])) continue;
        Expr* arg = node->arguments[i].get();
        // 2026-08-25 H3 幂等：实参已是 &x（AddressOf）——共享 AST 重复检查时
        //   二次 wrapRefArgs 不再包装（否则 前驱 -> &前驱 -> &(&前驱)=整64*）
        if (arg->getType() == NodeType::UnaryExpr &&
            static_cast<UnaryExpr*>(arg)->op == Operator::AddressOf) {
            continue;
        }
        bool isLvalue = false;
        switch (arg->getType()) {
            case NodeType::IdentifierExpr: {
                std::string vt;
                isLvalue = lookupVar(static_cast<IdentifierExpr*>(arg)->name, vt);
                break;
            }
            case NodeType::IndexExpr:
            case NodeType::MemberExpr:
            case NodeType::SelfExpr:
                isLvalue = true;
                break;
            case NodeType::UnaryExpr:
                // 2026-08-25 H3：Deref 与 AddressOf（已 wrap 过 &前驱）都是左值——
                //   嵌套实例化共享 AST 二次检查时 &前驱 不应再报"非左值"
                isLvalue = (static_cast<UnaryExpr*>(arg)->op == Operator::Deref ||
                            static_cast<UnaryExpr*>(arg)->op == Operator::AddressOf);
                break;
            default:
                isLvalue = false;
                break;
        }
        if (!isLvalue) {
            diagnostics_.report(DiagnosticLevel::Error, arg->location,
                                "引用参数要求左值实参（不能对常量/字面量/临时值取地址）");
            continue;
        }
        const SourceLocation loc = arg->location;
        node->arguments[i] = std::make_unique<UnaryExpr>(
            Operator::AddressOf, std::move(node->arguments[i]));
        node->arguments[i]->location = loc;
    }
}
// ==================== plans/019 阶段1（2026-09-10）：显式转移 转移() ====================

bool SemanticAnalyzer::isTransferCall(const CallExpr* node) {
    return node != nullptr &&
           node->callee->getType() == NodeType::IdentifierExpr &&
           static_cast<const IdentifierExpr*>(node->callee.get())->name == "转移" &&
           node->arguments.size() == 1;
}

// plans/019 阶段4' A2（2026-09-11 第七十二轮 72-a 根治）：签名键是否为泛型函数
//   单态化实例（名$实参串，如 逆序$整32）。
//   原判定 sigKey.find('$') 是符号名模式匹配，会把**跨模块链接键**（模块$名，
//   functionLinkKey 公式）一并命中——凡导入模块的函数返回 字符串，调用方
//   retOwnedString 一律不置位=A2 拥有契约跨模块整体失效（静默永久泄漏）。
//   改按泛型实例表精确判定：genericFuncInstances_ 的 instanceName 即真实例名
//   （instantiateGeneric 登记）；Rust 对照=rustc 按 DefId 判实例，不做名字模式推断。
bool SemanticAnalyzer::isGenericFuncInstanceName(const std::string& sigKey) const {
    for (const auto& gi : genericFuncInstances_) {
        if (gi.instanceName == sigKey) return true;
    }
    return false;
}

int SemanticAnalyzer::transferArgKind(const std::string& type) const {
    // 复制语义类型：转移无意义（Rust Copy 类型惯例）
    if (types::isInteger(type) || types::isFloat(type) || type == "布尔" ||
        type == "字符" || types::isFuncPtr(type) || isEnumType(type)) {
        return 1;
    }
    // 值交接类型（无 RAII，纯槽位值）：任意表达式位放行
    if (types::isPointer(type) || type == "字符串") return 0;
    // 拥有资源类型（容器/类/结构体/结果/可选/数组）：仅声明初始化位
    //   （visitVarDecl 改写路径=深拷贝既有语义；表达式位随阶段3 浅拷贝优化）
    return 2;
}
void SemanticAnalyzer::visitCallExpr(CallExpr* node) {
    // P3-18 补完：本轮默认非引用返回；决议到引用返回函数时置 true
    node->isRefReturnCall = false;
    lastExprIsRefReturn_ = false;
    // plans/019 阶段3 扩展（A21 借出视图生命周期，第七十七轮）：本轮默认非
    //   借出调用；方法分支决议到字符串元素容器借出方法时置 true（绑定位消费）
    lastExprIsBorrowView_ = false;
    lastBorrowCallNode_ = nullptr;
    // plans/019 阶段4' A2（2026-09-11 方案甲）：本轮默认非拥有字符串返回；
    //   决议到返回 字符串 的被调者时置 true（IR 初始化位/赋值位消费——登记
    //   RAII 依据；驻留文本 已改 字符* 返回=自动不置位）
    node->retOwnedString = false;
    if (checkTransferCall(node)) return;   // 族1：显式转移表达式位特判
    // ---- 阶段3（Task 3.6）：模块限定调用 模块.函数(实参)——族A（170-a 提取）----
    //   分派依据/识别语法/内置优先级策略详见 collectQualifiedCallInfo 与
    //   rewriteQualifiedCall 方法头注释（原 116~268 段整体搬移）。
    if (node->callee->getType() == NodeType::MemberExpr) {
        auto* mem = static_cast<MemberExpr*>(node->callee.get());
        QualifiedCallInfo info;
        if (collectQualifiedCallInfo(*mem, info)) {
            rewriteQualifiedCall(node, info);
        }
    }  // 模块限定调用重写块结束（Task 3.6 / 第 4 层 P1-1）
    bool isDirect = false;
    std::string calleeName;
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        calleeName = static_cast<IdentifierExpr*>(node->callee.get())->name;
        // 第 4 层（use 导入表）：花括号/路径导入绑定名重写——导入 数学::{正弦 作为 正}
        //   或（呈报一B）导入 数学::正弦 后调用 绑定名() 时，calleeName 是绑定名；
        //   查 useImports_ 各模块别名表映射回原符号名（正弦）。须在 isDirect 判定
        //   之前（绑定名未注册为函数名，hasFunctionName("正") 失败会导致
        //   isDirect=false 走间接调用路径报错）。
        //   限定调用重写（上方 MemberExpr 块）已设置 moduleFilter 时跳过——
        //   呈报一B 后路径导入也登记绑定名（模块X::双倍 绑定 双倍），限定调用
        //   模块Y::双倍 重写为纯名后会被本块按 绑定名 双倍（来源 模块X）二次
        //   重写覆盖过滤器 → 解析到错误模块（44_crate_isolate 实测）。限定
        //   调用的过滤器优先（调用点已显式指定模块归属）。
        rewriteUseImportAlias(node, calleeName);      // 子族A：use 导入绑定名重写
        rewriteGenericFuncCall(node, calleeName);     // 子族B：泛型函数调用单态化
        if (hasFunctionName(calleeName)) isDirect = true;
    }

    if (checkBuiltinCtorCall(node)) return;   // 族2：内置构造器 正常/错误/某些
    // ---- 阶段3：构造函数调用 类名(实参)（Task 3.1，规格书06-三）----
    // 语法：点 p = 点(1, 2)——callee 为类类型名时视为构造调用。
    // 构造返回对象（结果类型 = 类名）；校验参数个数与类型（查构造方法）。
    // 阶段3（Task 3.8，E2E 26 修复）：泛型实例化构造 盒子<整32>(42)——callee
    //   为 名<实参>（IdentifierExpr 名字含 <），先触发单态化（instantiateGeneric）
    //   生成实例化类符号（盒子$整32），再按普通类构造处理。
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        std::string className =
            static_cast<IdentifierExpr*>(node->callee.get())->name;
        resolveGenericCtorName(node, className);        // 族B1：泛型类构造单态化
        if (checkCtorCall(node, className)) return;      // 族B2：类构造调用检查
    }
    // ---- 阶段3：成员方法调用 对象.方法(实参) / 类名.静态方法(实参)（Task 3.1/3.9）----
    if (node->callee->getType() == NodeType::MemberExpr) {
        MemberExpr* mem = static_cast<MemberExpr*>(node->callee.get());
        const std::string objType = checkExpr(mem->object.get());
        const std::string methodName = mem->memberName;
        std::string ownerClass;
        if (checkInterfaceMethodCall(node, mem, objType, methodName)) return;  // 族C1：接口方法
        std::string clsName;
        const ClassMemberInfo* method = nullptr;
        if (checkMemberCallCore(node, mem, methodName, objType, clsName, ownerClass,
                                method)) return;          // 族C2：类解析+静态性检查
        if (method != nullptr && !method->isStatic) {
            if (checkInstanceMethodCall(node, mem, clsName, methodName, ownerClass,
                                        method)) return;  // 族C3：实例方法调用
        }
        if (method != nullptr && method->isStatic) {
            if (checkStaticMethodCall(node, methodName, ownerClass, method)) return;  // 族C4：静态方法
        }
        }
        // 非类成员：继续走通用路径（结构体字段函数指针等）

    // ---- 直接函数名调用：函数名(实参) ----
    if (isDirect) {
        if (checkDirectCall(node, calleeName)) return;   // 族D：直接函数名调用
    }


    // ---- 函数指针间接调用：回调(10, 20) ----
    std::string calleeType = checkExpr(node->callee.get());
    if (calleeType == "未知") {
        for (auto& arg : node->arguments) checkExpr(arg.get());
        lastType_ = "未知";
        return;
    }
    if (checkFuncPtrCall(node, calleeType)) return;   // 族3：函数指针间接调用

    // 其他被调者（成员函数等）：后续Task实现，跳过
    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "无法调用非函数类型 '" + calleeType + "'");
    lastType_ = "未知";
}

// 族1：显式转移 转移(变量)——表达式位特判（原 visitCallExpr 115~161 段）
//   （声明初始化位由 visitVarDecl 先行拦截改写，不会到达此处）。表达式位仅放行
//   指针/字符串（值交接无 RAII，ir_call 按 resolvedType 特判展开为实参值加载）；
//   容器/类等拥有资源类型拒绝（防浅柄接管撞 RAII 双析构），随阶段3 放开。
bool SemanticAnalyzer::checkTransferCall(CallExpr* node) {
    if (isTransferCall(node)) {
        Expr* arg = node->arguments[0].get();
        if (arg->getType() != NodeType::IdentifierExpr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "转移目标须为变量（标识符）——成员/下标/解引用形态"
                                "的转移随 plans/019 阶段3 支持");
            lastType_ = "未知";
            return true;
        }
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(arg);
        std::string varType;
        if (!lookupVar(ident->name, varType)) {
            diagnostics_.report(DiagnosticLevel::Error, ident->location,
                                "未声明的标识符 '" + ident->name + "'");
            lastType_ = "未知";
            return true;
        }
        const int kind = transferArgKind(varType);
        if (kind == 1) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "变量 '" + ident->name + "'（类型 '" + varType +
                                    "'）具有复制语义，无需转移");
            lastType_ = "未知";
            return true;
        }
        if (kind == 2) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "变量 '" + ident->name + "'（类型 '" + varType +
                                    "'）的转移仅支持声明初始化位"
                                "（类型 名 = 转移(变量);），表达式位转移随 plans/019"
                                " 阶段3 浅拷贝优化支持");
            lastType_ = "未知";
            return true;
        }
        if (reportMovedUse(ident->name, node->location)) {  // 再转移=使用已转移变量
            lastType_ = "未知";
            return true;
        }
        markMovedVar(ident->name, node->location.getLine());
        node->resolvedType = varType;  // IR 层展开识别（ir_call 特判）
        lastType_ = varType;
        return true;
    }
    return false;
}

// 族2：内置构造器 正常(值)/错误(值)/某些(值)（Task 3.5；原 visitCallExpr 433~490 段）——
//   按「参数类型 + 返回上下文」推导实际 结果<T,E>/可选<T> 类型。
bool SemanticAnalyzer::checkBuiltinCtorCall(CallExpr* node) {
    // ---- 阶段3：内置构造器 正常(值)/错误(值)/某些(值)（Task 3.5）----
    // 这些函数已注册在 functions_（纯名 key），但返回类型含占位符"自动"；
    // 此处按"参数类型 + 返回上下文"推导实际 结果<T,E>/可选<T> 类型。
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        const std::string builtinName =
            static_cast<IdentifierExpr*>(node->callee.get())->name;
        auto bit = functions_.find(builtinName);
        if (bit != functions_.end() &&
            (builtinName == "正常" || builtinName == "错误" || builtinName == "某些")) {
            // 参数类型检查：正常/错误/某些 期望 1 个实参；例外——正常() 无参数
            //   用于 结果<空类型,E>（空类型正常值，容器库 追加/删除 等返回
            //   结果<空类型,整32> 的 返回 正常()，Task 6.1）。
            if (node->arguments.size() != 1 &&
                !(builtinName == "正常" && node->arguments.empty())) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "内置构造器 '" + builtinName + "' 期望 1 个实参");
            }
            std::string argType = "未知";
            for (auto& arg : node->arguments) {
                argType = checkExpr(arg.get());
            }
            // 正常() 无参数：正常值类型 = 返回上下文 T（结果<空类型,E> -> 空类型）
            if (builtinName == "正常" && node->arguments.empty()) {
                argType = "空类型";
            }
            // 构造器返回类型推导：
            //   正常(v) -> 结果<typeof(v), E>（E 由返回上下文/默认整32 决定）
            //   错误(v) -> 结果<T, typeof(v)>（T 由返回上下文/默认整32 决定）
            //   某些(v) -> 可选<typeof(v)>
            // 返回上下文推断（Task 3.5 E2E 24 修复）：构造器用于 返回 语句时，
            //   从当前函数返回类型 结果<T,E> 取缺失的 T/E（如 打开配置 返回
            //   结果<字符串,整32>，`返回 错误(5)` 的 T 推断为 字符串）。
            std::string ctxT = "";
            std::string ctxE = "";
            if (!currentReturnType_.empty() && isResultType(currentReturnType_)) {
                const std::vector<std::string> args = resultTypeArgs(currentReturnType_);
                if (args.size() == 2) {
                    ctxT = args[0];
                    ctxE = args[1];
                }
            } else if (!currentReturnType_.empty() && isOptionalType(currentReturnType_)) {
                ctxT = optionalTypeArg(currentReturnType_);
            }
            if (builtinName == "正常") {
                const std::string e = (ctxE.empty() ? "整32" : ctxE);
                lastType_ = "结果<" + (argType == "未知" ? "整32" : argType) + "," + e + ">";
            } else if (builtinName == "错误") {
                const std::string t = (ctxT.empty() ? "整32" : ctxT);
                lastType_ = "结果<" + t + "," + (argType == "未知" ? "整32" : argType) + ">";
            } else {
                lastType_ = "可选<" + (argType == "未知" ? "整32" : argType) + ">";
            }
            // 写回推导类型（Task 3.5 E2E 24 修复）：IR 层按 resolvedType 降级为
            //   合成结构体构造（分配槽 + 写 是否正常/是否某些 + 值/错误值）
            node->resolvedType = lastType_;
            return true;
        }
    }
    return false;
}

// 族3：函数指针间接调用（回调(10, 20)；原 visitCallExpr 949~981 段）——参数数量/类型检查。
bool SemanticAnalyzer::checkFuncPtrCall(CallExpr* node, const std::string& calleeType) {
    if (isFuncPtrType(calleeType)) {
        std::string retType = funcPtrReturnOf(calleeType);
        std::vector<std::string> paramTypes = funcPtrParamsOf(calleeType);
        // 参数数量检查
        if (node->arguments.size() != paramTypes.size()) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "函数指针调用期望 " + std::to_string(paramTypes.size()) +
                                " 个参数，实际提供 " +
                                std::to_string(node->arguments.size()) + " 个");
            lastType_ = retType;
            return true;
        }
        // 参数类型检查
        for (std::size_t i = 0; i < node->arguments.size(); i++) {
            std::string argType = checkExpr(node->arguments[i].get());
            const std::string& paramType = paramTypes[i];
            if (!canConvertWithLiteral(node->arguments[i].get(), argType, paramType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->arguments[i]->location,
                                    "函数指针第 " + std::to_string(i + 1) +
                                    " 个参数无法将 '" + argType + "' 隐式转换为 '" +
                                    paramType + "'");
            }
        }
        lastType_ = retType;
        return true;
    }
    return false;
}


// 子族A：第 4 层 use 导入表——花括号/路径导入绑定名重写（原 visitCallExpr 273~311 段）：
//   calleeName 为绑定名时映射回原符号名，并携带来源模块（moduleFilter）。
void SemanticAnalyzer::rewriteUseImportAlias(CallExpr* node, std::string& calleeName) {
        // 第 4 层（use 导入表）：花括号/路径导入绑定名重写——导入 数学::{正弦 作为 正}
        //   或（呈报一B）导入 数学::正弦 后调用 绑定名() 时，calleeName 是绑定名；
        //   查 useImports_ 各模块别名表映射回原符号名（正弦）。须在 isDirect 判定
        //   之前（绑定名未注册为函数名，hasFunctionName("正") 失败会导致
        //   isDirect=false 走间接调用路径报错）。
        //   限定调用重写（上方 MemberExpr 块）已设置 moduleFilter 时跳过——
        //   呈报一B 后路径导入也登记绑定名（模块X::双倍 绑定 双倍），限定调用
        //   模块Y::双倍 重写为纯名后会被本块按 绑定名 双倍（来源 模块X）二次
        //   重写覆盖过滤器 → 解析到错误模块（44_crate_isolate 实测）。限定
        //   调用的过滤器优先（调用点已显式指定模块归属）。
        if (calleeName.find('<') == std::string::npos &&
            node->moduleFilter.empty()) {
            for (const auto& ui : useImports_) {
                const auto& aliases = ui.second.aliases;
                const auto aliasIt = aliases.find(calleeName);
                if (aliasIt != aliases.end() && aliasIt->second != ui.first) {
                    // A-5（花括号项别名跨模块同名歧义根治）：重写回原符号名时
                    //   携带来源模块（moduleFilter）——resolveOverload 按模块过滤，
                    //   跨模块同名（模块X$双倍 与 模块Y$双倍）纯名别名调用不再歧义。
                    //   来源模块查 itemAliasModules_：按**原绑定名**查（别名 与
                    //   原符号名 不同名时，重写后查原符号名会命中同名的其他绑定
                    //   条目——91_别名跨模块同名 实测 Y双倍 误取 双倍 的来源模块），
                    //   未命中再回退重写后的符号名（自映射绑定 条目以符号名为键）。
                    const std::string bindingName = calleeName;
                    calleeName = aliasIt->second;  // 绑定名 -> 原符号名
                    static_cast<IdentifierExpr*>(node->callee.get())->name = calleeName;
                    auto iamIt = itemAliasModules_.find(bindingName);
                    if (iamIt == itemAliasModules_.end()) {
                        iamIt = itemAliasModules_.find(calleeName);
                    }
                    if (iamIt != itemAliasModules_.end()) {
                        node->moduleFilter = iamIt->second;
                    } else {
                        node->moduleFilter = ui.first;
                    }
                    break;
                }
            }
        }
}

// 子族B：Task 6.1 泛型函数调用单态化（原 visitCallExpr 312~383 段）：识别 名<类型>(实参)，
//   实例化（instantiateGeneric 注册 名$实参）并重写 callee；登记 genericFuncInstances_。
void SemanticAnalyzer::rewriteGenericFuncCall(CallExpr* node, std::string& calleeName) {
        // ---- Task 6.1（泛型函数调用打通）：函数名<类型>(实参) 泛型实例化调用 ----
        // 语法：最小<整32>(3, 7)——parser 把 callee 生成 IdentifierExpr("最小<整32>")。
        // 26_generics 遗留限制「泛型函数调用单态化注册未接入」：语义层此前只对
        //   泛型类构造（名<实参>(...)）触发单态化，泛型函数调用落入"非函数类型"错误。
        // 本子任务打通：识别 名<类型> 形态，若 名 是已注册泛型函数 -> 触发单态化
        //   （instantiateGeneric 注册 名$实参 函数符号），重写 callee 为实例化名，
        //   复用下方"直接函数名调用"路径（重载决议/参数检查/IR 符号生成均无需改动）。
        const std::size_t genLt = calleeName.find('<');
        const std::size_t genGt = calleeName.rfind('>');
        if (genLt != std::string::npos && genGt != std::string::npos &&
            genGt > genLt) {
            const std::string head = calleeName.substr(0, genLt);
            if (findGeneric(head) != nullptr &&
                findGeneric(head)->ast->innerFunc != nullptr) {
                const std::string inner =
                    calleeName.substr(genLt + 1, genGt - genLt - 1);
                std::vector<std::string> args;
                std::size_t pos = 0;
                int angleDepth = 0;
                std::size_t segStart = 0;
                while (pos <= inner.size()) {
                    if (pos == inner.size() ||
                        (inner[pos] == ',' && angleDepth == 0)) {
                        args.push_back(inner.substr(segStart, pos - segStart));
                        segStart = pos + 1;
                        if (pos == inner.size()) break;
                    } else if (inner[pos] == '<') {
                        angleDepth++;
                    } else if (inner[pos] == '>') {
                        angleDepth--;
                    }
                    pos++;
                }
                for (auto& a : args) {
                    const std::size_t b = a.find_first_not_of(" \t");
                    const std::size_t e = a.find_last_not_of(" \t");
                    if (b != std::string::npos && e != std::string::npos) {
                        a = a.substr(b, e - b + 1);
                    }
                    // Task 6.1（嵌套泛型 链表$整32 内 节点<T>() 构造）：类型实参
                    //   T 替换为当前泛型上下文实参（整32）——否则 节点$T 实例化失败。
                    auto pit = genericTypeParams_.find(a);
                    if (pit != genericTypeParams_.end()) a = pit->second;
                    if (a.find('<') != std::string::npos) {
                        a = resolveGenericTypeName(a, node->location);
                    }
                }
                const std::string instName =
                    instantiateGeneric(head, args, node->location);
                if (!instName.empty()) {
                    // 重写 callee 为实例化函数名（名$实参），直接函数调用路径命中
                    node->callee = std::make_unique<IdentifierExpr>(instName);
                    calleeName = instName;
                    // 登记泛型函数实例化记录（供 IR 层生成函数体）：
                    //   记录 实例化名 + 原泛型声明 + 类型实参（替换类型参数用）
                    const GenericInfo* ginfo = findGeneric(head);
                    if (ginfo != nullptr && ginfo->ast->innerFunc != nullptr) {
                        bool exists = false;
                        for (const auto& gi : genericFuncInstances_) {
                            if (gi.instanceName == instName) { exists = true; break; }
                        }
                        if (!exists) {
                            GenericFuncInstance gfi;
                            gfi.instanceName = instName;
                            gfi.gen = ginfo->ast;
                            gfi.args = args;
                            genericFuncInstances_.push_back(std::move(gfi));
                        }
                    }
                }
            }
        }
}


// ===== 族B：构造函数调用 类名(实参)（原 visitCallExpr 295~405 段）=====
// 族B1：泛型类构造单态化（原 298~341 段）——名<实参> → 实例化类符号（盒子$整32）。
void SemanticAnalyzer::resolveGenericCtorName(CallExpr* node, std::string& className) {
        const std::size_t genLt = className.find('<');
        const std::size_t genGt = className.rfind('>');
        if (genLt != std::string::npos && genGt != std::string::npos &&
            genGt > genLt) {
            const std::string head = className.substr(0, genLt);
            if (findGeneric(head) != nullptr) {
                const std::string inner =
                    className.substr(genLt + 1, genGt - genLt - 1);
                std::vector<std::string> args;
                std::size_t pos = 0;
                int angleDepth = 0;
                std::size_t segStart = 0;
                while (pos <= inner.size()) {
                    if (pos == inner.size() ||
                        (inner[pos] == ',' && angleDepth == 0)) {
                        args.push_back(inner.substr(segStart, pos - segStart));
                        segStart = pos + 1;
                        if (pos == inner.size()) break;
                    } else if (inner[pos] == '<') {
                        angleDepth++;
                    } else if (inner[pos] == '>') {
                        angleDepth--;
                    }
                    pos++;
                }
                for (auto& a : args) {
                    const std::size_t b = a.find_first_not_of(" \t");
                    const std::size_t e = a.find_last_not_of(" \t");
                    if (b != std::string::npos && e != std::string::npos) {
                        a = a.substr(b, e - b + 1);
                    }
                    // Task 6.1（嵌套泛型 链表$整32 内 节点<T>() 构造）：类型实参
                    //   T 替换为当前泛型上下文实参（整32）——否则 节点$T 实例化失败。
                    auto pit = genericTypeParams_.find(a);
                    if (pit != genericTypeParams_.end()) a = pit->second;
                    if (a.find('<') != std::string::npos) {
                        a = resolveGenericTypeName(a, node->location);
                    }
                }
                const std::string instName =
                    instantiateGeneric(head, args, node->location);
                if (!instName.empty()) className = instName;
            }
        }
}

// 族B2：类构造调用检查（原 342~404 段）——构造函数匹配（精确优先）+ 参数检查 +
//   借用纪律/引用实参包装 + resolvedSignature 记录。true = 已处理。
bool SemanticAnalyzer::checkCtorCall(CallExpr* node, const std::string& className) {
        const ClassInfo* ctorCls = findClass(className);
        if (ctorCls != nullptr) {
            // 查找构造函数（函数名 == 类名）。Debug 子任务修复（构造函数重载）：
            //   methods 表构造条目 key=sigKey（名#参数串），遍历按 isConstructor +
            //   ownerClass（排除父类构造，阶段A-3）+ 实参个数 + 类型可转换 匹配最优。
            std::vector<std::string> argTypes;
            for (auto& arg : node->arguments) {
                argTypes.push_back(checkExpr(arg.get()));
            }
            const ClassMemberInfo* ctor = nullptr;
            const ClassMemberInfo* ctorExact = nullptr;
            for (const auto& mk : ctorCls->methods) {
                const ClassMemberInfo& mi = mk.second;
                if (!mi.isConstructor || mi.ownerClass != className) continue;
                if (mi.paramTypes.size() != argTypes.size()) continue;
                bool ok = true;
                for (std::size_t i = 0; i < argTypes.size(); ++i) {
                    if (conversionLevel(argTypes[i], mi.paramTypes[i],
                                        isIntLiteralExpr(node->arguments[i].get())) < 0) { ok = false; break; }
                }
                if (!ok) continue;
                ctor = &mi;
                // 精确类型匹配（全部 0 级转换）优先
                bool exact = true;
                for (std::size_t i = 0; i < argTypes.size(); ++i) {
                    if (conversionLevel(argTypes[i], mi.paramTypes[i],
                                        isIntLiteralExpr(node->arguments[i].get())) != 0) { exact = false; break; }
                }
                if (exact) { ctorExact = &mi; break; }
            }
            if (ctorExact != nullptr) ctor = ctorExact;
            if (ctor != nullptr) {
                if (argTypes.size() != ctor->paramTypes.size()) {
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "构造函数 '" + className + "' 期望 " +
                                            std::to_string(ctor->paramTypes.size()) +
                                            " 个实参，实际提供 " +
                                            std::to_string(argTypes.size()) + " 个");
                } else {
                    for (std::size_t i = 0; i < argTypes.size(); ++i) {
                        if (!canConvertWithLiteral(node->arguments[i].get(), argTypes[i], ctor->paramTypes[i])) {
                            diagnostics_.report(
                                DiagnosticLevel::Error, node->arguments[i]->location,
                                "构造函数 '" + className + "' 第 " + std::to_string(i + 1) +
                                    " 个实参无法将 '" + argTypes[i] + "' 隐式转换为 '" +
                                    ctor->paramTypes[i] + "'");
                        }
                    }
                }
                // plans/019 阶段3b：构造调用面借用纪律（与普通函数面同构）
                checkConstRefBorrowDiscipline(node, ctor->paramTypes,
                                              ctor->constParams);
                // A-1（引用参数）：构造形参为引用时实参自动取地址
                wrapRefArgs(node, ctor->paramTypes);
                // 记录选中的构造 sigKey（IR 层按此生成构造体 Call 符号）
                node->resolvedSignature = className + "$" + ctor->sigKey;
                lastType_ = className;  // 构造返回对象
                return true;
            }
            // 无构造函数：允许默认构造（返回类类型）
            lastType_ = className;
            return true;
        }
    return false;
}

// ===== 族C：成员方法调用 对象.方法(实参) / 类名.静态方法(实参)（原 visitCallExpr 407~622 段）=====
// 族C1：接口对象方法调用（原 413~475 段，接口方法经全局槽位运行时分派）。true = 已处理。
bool SemanticAnalyzer::checkInterfaceMethodCall(CallExpr* node, MemberExpr* mem,
                                                const std::string& objType,
                                                const std::string& methodName) {
            const std::string ifaceName = canonicalType(
                types::isPointer(objType) ? types::pointeeOf(objType) : objType);
            const InterfaceInfo* iface = findInterface(ifaceName);
            if (iface != nullptr) {
                // 缺陷根治（第九十三轮，2026-09-13 B2 同族扫面）：接口名.方法()
                //   ——接口方法须经接口对象调用（receiver 为接口类型名时无对象
                //   可取，运行期 NULL 分派 → 段错误，探针 M10 实证）。Rust 同款
                //   纪律：trait 方法须经实现者实例调用。
                if (mem->object->getType() == NodeType::IdentifierExpr) {
                    const std::string& objName =
                        static_cast<IdentifierExpr*>(mem->object.get())->name;
                    if (isInterfaceType(objName)) {
                        diagnostics_.report(DiagnosticLevel::Error, node->location,
                                            "接口名.方法() 非法：接口方法 '" +
                                                methodName +
                                                "' 须经接口对象调用（接口类型名无实例）");
                        lastType_ = "未知";
                        return true;
                    }
                }
                const auto imit = iface->methods.find(methodName);
                if (imit == iface->methods.end()) {
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "接口 '" + ifaceName + "' 没有成员 '" +
                                            methodName + "'");
                    lastType_ = "未知";
                    return true;
                }
                std::vector<std::string> argTypes;
                for (auto& arg : node->arguments) {
                    argTypes.push_back(checkExpr(arg.get()));
                    // P3-23 补完（D2）：绑定方法值不可作裸 fnptr 实参
                    if (argIsBoundMethodValue(arg.get())) {
                        diagnostics_.report(
                            DiagnosticLevel::Error, arg->location,
                            "实例方法作值不能直接作为函数指针实参传递（绑定 this 须先赋值给变量：变量 cb = 对象.方法）");
                    }
                }
                if (argTypes.size() != imit->second.paramTypes.size()) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, node->location,
                        "接口方法 '" + methodName + "' 期望 " +
                            std::to_string(imit->second.paramTypes.size()) +
                            " 个实参，实际提供 " + std::to_string(argTypes.size()) + " 个");
                } else {
                    for (std::size_t i = 0; i < argTypes.size(); ++i) {
                        if (!canConvertWithLiteral(node->arguments[i].get(), argTypes[i], imit->second.paramTypes[i])) {
                            diagnostics_.report(
                                DiagnosticLevel::Error, node->arguments[i]->location,
                                "接口方法 '" + methodName + "' 第 " +
                                    std::to_string(i + 1) + " 个实参无法将 '" +
                                    argTypes[i] + "' 隐式转换为 '" +
                                    imit->second.paramTypes[i] + "'");
                        }
                    }
                }
                lastType_ = imit->second.type;
                node->retOwnedString = (lastType_ == "字符串");  // A2：接口方法拥有契约
                lastExprIsRefReturn_ = false;  // 接口方法引用返回暂不支持（方法返回类型 canonical 剥 &）
                return true;
            }
    return false;
}

// 族C2：类解析 + 静态性纪律检查（原 476~528 段）——objTypeForClass/clsName 推导、
//   成员查找、类名.实例方法() 与 实例.静态方法() 拒绝。输出 clsName/ownerClass/method。
bool SemanticAnalyzer::checkMemberCallCore(CallExpr* node, MemberExpr* mem,
                                          const std::string& methodName,
                                          const std::string& objType,
                                          std::string& clsName, std::string& ownerClass,
                                          const ClassMemberInfo*& method) {
        // 对象为类实例 或 类名.静态方法
        // 集成修复（自身/父类）：自身 类型为 类名*（this 指针），父类 类型为 父类名*，
        //   方法调用须剥指针取类类型（与 visitMemberExpr 的自身.成员 处理一致）；
        //   -> 访问 自身->方法() 同样剥指针。
        std::string objTypeForClass = objType;
        if (mem->object->getType() == NodeType::SelfExpr ||
            mem->object->getType() == NodeType::SuperExpr) {
            if (types::isPointer(objTypeForClass)) {
                objTypeForClass = types::pointeeOf(objTypeForClass);
            }
        }
        // v2.1（成员访问统一 .）：对象为类指针（账户* 账.方法()）自动解引用
        //   一级（≡ (*账).方法()）——类型驱动剥指针，不依赖语义遍历顺序。
        // 簇⑥根治（2026-09-04，与 visitMemberExpr 同款）：泛型实例名可含实参
        //   星号（盒子$整64*——合成名保留尾 *），尾 * 非对象指针语义——原名
        //   查类命中即用原名；真指针（盒子$整64**）不命中类表自然落入剥分支。
        clsName =
            (findClass(objTypeForClass) != nullptr)
                ? canonicalType(objTypeForClass)
                : (types::isPointer(objTypeForClass)
                       ? canonicalType(types::pointeeOf(objTypeForClass))
                       : canonicalType(objTypeForClass));
        method = lookupClassMember(clsName, methodName, ownerClass);
        // 缺陷根治（第九十三轮，2026-09-13 B2 立案复现）：类名.实例方法() ——
        //   调用路径漏检（visitMemberExpr 对 类名.实例成员 已有拒绝，本路径
        //   直查成员表后即按实例方法调用生成，无 this → IR 生成 NULL 间接调用
        //   → 运行期段错误〈宿主探针 M1/M7 实证〉/垃圾值〈v2 侧实证〉）。Rust
        //   同款纪律（E0061：实例方法须经实例调用；类名.成员 仅静态成员合法）。
        //   同处一并拒绝 实例.静态方法()（Rust E0599：关联函数不经实例访问；
        //   原实现同样崩——探针 M5）。receiver 判定与 visitMemberExpr 同款
        //   （IdentifierExpr 且 isClassType）；自身/父类 receiver 非类名 →
        //   实例语义（自身.方法/父类.方法 合法形态不受影响）。
        bool receiverIsTypeName = false;
        if (mem->object->getType() == NodeType::IdentifierExpr) {
            const std::string& objName =
                static_cast<IdentifierExpr*>(mem->object.get())->name;
            receiverIsTypeName = isClassType(objName);
        }
        if (method != nullptr && receiverIsTypeName && !method->isStatic) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "类名.实例方法() 非法：方法 '" + methodName +
                                    "' 是非静态方法，须经实例调用");
            lastType_ = "未知";
            return true;
        }
        if (method != nullptr && !receiverIsTypeName && method->isStatic) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "实例.静态方法() 非法：静态方法 '" + methodName +
                                    "' 须经类名调用（" + ownerClass + "." +
                                    methodName + "）");
            lastType_ = "未知";
            return true;
        }
    return false;
}

// 族C3：实例方法调用（原 529~590 段）——参数检查（含 H3 引用已包装豁免）+ 借用纪律 +
//   访问控制 + 借出调用点登记（noteBorrowCallSite）。true = 已处理。
bool SemanticAnalyzer::checkInstanceMethodCall(CallExpr* node, MemberExpr* mem,
                                              const std::string& clsName,
                                              const std::string& methodName,
                                              const std::string& ownerClass,
                                              const ClassMemberInfo* method) {
        if (method != nullptr && !method->isStatic) {
            // 实例方法调用：校验参数个数与类型
            std::vector<std::string> argTypes;
            for (auto& arg : node->arguments) {
                argTypes.push_back(checkExpr(arg.get()));
                // P3-23 补完（D2）：绑定方法值不可作裸 fnptr 实参
                if (argIsBoundMethodValue(arg.get())) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, arg->location,
                        "实例方法作值不能直接作为函数指针实参传递（绑定 this 须先赋值给变量：变量 cb = 对象.方法）");
                }
            }
            if (argTypes.size() != method->paramTypes.size()) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "方法 '" + methodName + "' 期望 " +
                                        std::to_string(method->paramTypes.size()) +
                                        " 个实参，实际提供 " +
                                        std::to_string(argTypes.size()) + " 个");
            } else {
                for (std::size_t i = 0; i < argTypes.size(); ++i) {
                    // 2026-08-25 H3：形参是引用（整64&）且实参已是 &x（AddressOf）——
                    //   共享 AST（实例化类同一 mi.ast）二次检查时已 wrap，引用已满足，
                    //   跳过转换比较（否则 &前驱=整64* 误报"无法转 整64&"）
                    bool refAlready =
                        types::isReference(method->paramTypes[i]) &&
                        node->arguments[i]->getType() == NodeType::UnaryExpr &&
                        static_cast<UnaryExpr*>(node->arguments[i].get())->op ==
                            Operator::AddressOf;
                    if (!refAlready &&
                        !canConvertWithLiteral(node->arguments[i].get(), argTypes[i], method->paramTypes[i])) {
                        diagnostics_.report(
                            DiagnosticLevel::Error, node->arguments[i]->location,
                            "方法 '" + methodName + "' 第 " + std::to_string(i + 1) +
                                " 个实参无法将 '" + argTypes[i] + "' 隐式转换为 '" +
                                method->paramTypes[i] + "'");
                    }
                }
            }
            // A-1（引用参数）：实例方法引用形参的实参自动取地址
            checkConstRefBorrowDiscipline(node, method->paramTypes,
                                              method->constParams);  // plans/019 阶段3b
                wrapRefArgs(node, method->paramTypes);
            // 访问控制检查（Task 3.4）
            const std::string contextClass = contextClassStack_.empty()
                                                 ? ""
                                                 : contextClassStack_.back();
            checkAccess(*findClass(ownerClass), *method, contextClass, node->location,
                        "方法");
            lastType_ = method->type;
            // A2：泛型实例化类成员（ownerClass 含 $，如 向量$字符串）不置位
            //   ——T 来源返回=借用（容器元素访问），保守不登记
            node->retOwnedString = (lastType_ == "字符串" &&
                                    ownerClass.find('$') == std::string::npos);
            // plans/019 阶段3 扩展（A21 借出视图生命周期，第七十七轮）：借出调用
            //   标记（字符串元素容器 元素/读取/栈顶/队首/头部元素/读取头部/
            //   读取尾部/获取=容器内句柄浅拷）与容器失效点登记（删除/设置/
            //   清空/弹出/出队/删除头部/删除尾部/释放内部数组）——绑定位
            //   （声明初始化/赋值）消费标记，函数尾结算与活跃区间比对。
            noteBorrowCallSite(*mem, clsName, methodName, node);
            lastExprIsRefReturn_ = false;  // 方法引用返回暂不支持（类型 canonical 剥 &）
            return true;
        }
    return false;
}

// 族C4：静态方法调用 类名.静态方法(实参)（原 591~620 段）。true = 已处理。
bool SemanticAnalyzer::checkStaticMethodCall(CallExpr* node, const std::string& methodName,
                                            const std::string& ownerClass,
                                            const ClassMemberInfo* method) {
        if (method != nullptr && method->isStatic) {
            // 静态方法调用（类名.静态方法(...)）
            std::vector<std::string> argTypes;
            for (auto& arg : node->arguments) {
                argTypes.push_back(checkExpr(arg.get()));
                // P3-23 补完（D2）：绑定方法值不可作裸 fnptr 实参
                if (argIsBoundMethodValue(arg.get())) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, arg->location,
                        "实例方法作值不能直接作为函数指针实参传递（绑定 this 须先赋值给变量：变量 cb = 对象.方法）");
                }
            }
            if (argTypes.size() != method->paramTypes.size()) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "静态方法 '" + methodName + "' 期望 " +
                                        std::to_string(method->paramTypes.size()) +
                                        " 个实参，实际提供 " +
                                        std::to_string(argTypes.size()) + " 个");
            }
            // A-1（引用参数）：静态方法引用形参的实参自动取地址
            checkConstRefBorrowDiscipline(node, method->paramTypes,
                                              method->constParams);  // plans/019 阶段3b
                wrapRefArgs(node, method->paramTypes);
            lastType_ = method->type;
            // A2：泛型实例化类成员（ownerClass 含 $）不置位——同实例方法口径
            node->retOwnedString = (lastType_ == "字符串" &&
                                    ownerClass.find('$') == std::string::npos);
            lastExprIsRefReturn_ = false;  // 方法引用返回暂不支持（类型 canonical 剥 &）
            return true;
        }
    return false;
}

// ===== 族D：直接函数名调用 函数名(实参)（原 visitCallExpr 625~745 段）=====
// 族D-1：变参内置函数（打印/打印行/格式化）——参数个数不限、逐个类型检查。true = 已处理。
//   变参内置判定：纯名 key 直接查（Task 2.5/2.9），逐个检查类型（字符串/字符*/整型/
//   浮点/布尔/字符/枚举均允许）。
//   方案C审查（2026-08-14）：okNum 补 字符——旧 打印行整数('A')（字符→整64
//     隐式转换）替换为 打印('A') 后，IR 层 字符(i32) Cast i64 走 __cn_print_int
//     输出ASCII码（65），行为等价；缺此检查会误拒 打印(字符变量)/打印('A')
bool SemanticAnalyzer::checkVariadicBuiltinCall(CallExpr* node, const std::string& calleeName) {
    auto builtinIt = functions_.find(calleeName);
        if (builtinIt != functions_.end() && builtinIt->second.variadic) {
            const FunctionInfo& info = builtinIt->second;
            for (auto& arg : node->arguments) {
                std::string argType = checkExpr(arg.get());
                const bool okStr = (argType == "字符串" || argType == "字符*" ||
                                    argType == "字符串*");
                const bool okNum = (isNumeric(argType) || argType == "布尔" ||
                                    argType == "字符" || isEnumType(argType));
                if (!okStr && !okNum) {
                    diagnostics_.report(DiagnosticLevel::Error, arg->location,
                                        "打印行 参数类型不支持：'" + argType + "'");
                }
            }
            lastType_ = info.returnType;
            node->retOwnedString = (lastType_ == "字符串");  // A2：变参路径拥有契约
            return true;
        }
    return false;
}

// 族D-2：决议失败回退（原 670~686 段）——纯名存在（内置单版本函数）时按旧逻辑放行。
//   true = 已处理（含决议失败但放行/报错）。
bool SemanticAnalyzer::checkDirectCallFallback(CallExpr* node, const std::string& calleeName,
                                              const std::string& sigKey) {
    if (!sigKey.empty()) return false;
            // 决议失败（参数个数/类型不匹配或歧义）：恢复兼容——若纯名存在（内置
            // 单版本函数），按旧逻辑检查，避免错误级联导致 IR 层找不到符号
            auto fallback = functions_.find(calleeName);
            if (fallback != functions_.end()) {
            lastType_ = fallback->second.returnType;
            node->retOwnedString = (lastType_ == "字符串");  // A2：内置单版本拥有契约
            // P3-18 补完：引用返回函数调用結果可作左值
            if (fallback->second.isRefReturn) {
                    node->isRefReturnCall = true;
                    lastExprIsRefReturn_ = true;
                }
            } else {
                lastType_ = "未知";
            }
            return true;
    return false;
}

// 族D 主体：非变参直接调用（原 626~631 + 649~667 + 687~744）——实参类型/字面量标志收集、
//   重载决议、外部函数/裸释放安全区边界、借用纪律、引用实参包装、参数检查、拥有契约。
bool SemanticAnalyzer::checkDirectCall(CallExpr* node, const std::string& calleeName) {
    if (checkVariadicBuiltinCall(node, calleeName)) return true;
        // 非变参直接调用：重载决议（先检查实参类型）
        std::vector<std::string> argTypes;
        argTypes.reserve(node->arguments.size());
        for (auto& arg : node->arguments) {
            argTypes.push_back(checkExpr(arg.get()));
            // P3-23 补完（D2）：绑定方法值不可作裸 fnptr 实参
            if (argIsBoundMethodValue(arg.get())) {
                diagnostics_.report(
                    DiagnosticLevel::Error, arg->location,
                    "实例方法作值不能直接作为函数指针实参传递（绑定 this 须先赋值给变量：变量 cb = 对象.方法）");
            }
        }
        // 第 4 层（crate 隔离）：限定调用按模块过滤（数学::双倍 只解析数学.cn 的）
        // 55-c 方案A：实参字面量标志供决议豁免（`读值(100)` 传 正32 形参保留）
        std::vector<bool> argLitFlags;
        argLitFlags.reserve(node->arguments.size());
        for (const auto& a : node->arguments) {
            argLitFlags.push_back(isIntLiteralExpr(a.get()));
        }
        std::string sigKey = resolveOverload(calleeName, argTypes, node->location,
                                             node->moduleFilter, argLitFlags);
    if (checkDirectCallFallback(node, calleeName, sigKey)) return true;
        // plans/018 呈报二 A′（2026-09-07 用户裁决）：resolvedSignature = 注册键。
        //   注册键在 registerFunction 已按函数链接键公式生成（模块条目自带
        //   模块$ 前缀、主/单文件恒裸键），与定义侧 mangledName 同源——
        //   旧「决议后按 it->second.moduleName 补拼前缀」的补丁块删除
        //   （其仅为旧裸键注册方案的对齐补丁，公式化后结构上不存在劈叉）。
        node->resolvedSignature = sigKey;
        auto it = functions_.find(sigKey);
        const FunctionInfo& info = it->second;
        // plans/019 阶段4（2026-09-10）：安全区边界观察期——外部 函数 调用
        //   （FFI=C 边界，类型安全不保证）与裸释放（释放=手动内存管理）应在
        //   不安全 函数 内
        if (info.isExtern) {
            reportUnsafeBoundary(node->location, "外部函数调用", calleeName);
        }
        if (calleeName == "释放") {
            reportUnsafeBoundary(node->location, "裸释放", "释放(指针)");
        }
        // plans/019 阶段3（2026-09-10）：常量引用借用纪律（只读借出可变拒 +
        //   同调用可变×只读互斥）——置于 wrapRefArgs 之前按原始实参形态判定
        checkConstRefBorrowDiscipline(node, info.paramTypes, info.constParams);
        // A-1（引用参数）：引用形参的实参自动取地址（重写为 &左值）——
        //   须在 IR 层实参求值之前（IR genExpr 对 AddressOf 生成 lvalueAddress）
        wrapRefArgs(node, info.paramTypes);
        // 参数类型检查（决议已保证可转换；此处再逐个报告具体错误位置）。
        //   55-c 方案A：字面量实参豁免与决议豁免（conversionLevel）同步——
        //   决议按宽化级放行的字面量形态此处不再误报
        for (std::size_t i = 0; i < node->arguments.size(); i++) {
            const std::string& paramType = info.paramTypes[i];
            if (!canConvertWithLiteral(node->arguments[i].get(), argTypes[i], paramType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->arguments[i]->location,
                                    "函数 '" + calleeName + "' 第 " + std::to_string(i + 1) +
                                    " 个参数无法将 '" + argTypes[i] + "' 隐式转换为 '" +
                                    paramType + "'");
            }
        }
        lastType_ = info.returnType;
        // A2（方案甲）：被调者返回类型 字符串=拥有——调用方登记 RAII（Rust
        //   签名即契约：fn f() -> String 拥有 / -> &str 借用）。泛型单态化产物
        //   （如 逆序$整32）不置位——泛型体返回 T 来源字符串=借用（容器元素
        //   访问，Rust Vec::get -> &T 同款），保守不登记=安全方向（调用方 free
        //   容器内部元素=悬垂）。
        // 70-a 根治（2026-09-11 用户裁决方案A）：恢复轮1 二分调试残留的
        //   false && 前缀——当时为排查 119/167-172 回归临时禁用，真根因
        //   （IR容器 字符串入容器浅共享悬垂，c668919）已另行根治。
        // 72-a 根治（2026-09-11 第七十二轮）：原判定 sigKey.find('$') 把
        //   **跨模块链接键**（模块$名，functionLinkKey 公式）误当泛型产物——
        //   凡导入模块的函数返回 字符串，调用方一律不登记 RAII=静默永久泄漏
        //   （探针：本模块 本地造串 → 有 free；跨模块 工具$造串 → 无 free）。
        //   改按泛型实例表精确判定（genericFuncInstances_ 登记的真单态化产物；
        //   rustc 按 DefId 判定实例、不用符号名模式同款）。
        node->retOwnedString =
            (lastType_ == "字符串" && !isGenericFuncInstanceName(sigKey));
        // P3-18 补完：引用返回函数调用結果可作左值（整32& r = 获取() / 获取()=值 / &获取()）
        if (info.isRefReturn) {
            node->isRefReturnCall = true;
            lastExprIsRefReturn_ = true;
        }
        return true;
}

} // namespace cn_compiler
