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
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"
#include "cn_compiler/semantic/semantic_lambda_escape.hpp"
#include "cn_compiler/semantic/semantic_method_value_943.hpp"
#include "cn_compiler/semantic/semantic_staticguard.hpp"  // 916：方法调用位 §747-8 执法

namespace cn_compiler {

// 067-002（p0927_05 实证）：$ 合成名 → 尖括号模板形态——ctorTargetStack_ 消费点

//   （isResultType/resultTypeArgs/optionalTypeArg）只认尖括号形态；容器/类方法

//   形参类型为 $ 形态（结果$盒子$整32），不归一则实参目标类型推导失效。

//   切分能力受限：结果 的 T 段若含 $（结果$向量$整32$整32）保守不转换（原行为）。





// ==================== plans/019 阶段1（2026-09-10）：显式转移 转移() ====================


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
    rewriteCharPrintArgs(node);   // 306：打印/打印行 字符实参改写（#1855 归位 semantic_call_builtin.cpp·段体逐字搬移）
    if (checkTransferCall(node)) return;   // 族1：显式转移表达式位特判
    if (checkCopyBuiltinCall(node)) return;  // 族1.5：复制 泛型克隆内置（206-a 波 4）
    if (checkWrapBuiltinCall(node)) return;  // 族1.6：回绕算术内置（任务 474·错误码 8 豁免面）
    if (checkRemoveBuiltinCall(node)) return;  // 族1.7：摘除 内置（#1674 甲案·拥有槽释放+清槽）
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
    if (dispatchMemberMethodCall(node)) return;   // 阶段3：成员方法调用分派（916+值捕写+族C1~C4·#1855 归位 semantic_call_member.cpp·真=已处理与原段内 return 终止等价）

    // ---- 直接函数名调用：函数名(实参) ----
    if (isDirect) {
        if (checkDirectCall(node, calleeName)) return;   // 族D：直接函数名调用
    }


    // ---- 函数指针间接调用：回调(10, 20) ----
    std::string calleeType = checkExpr(node->callee.get());
    if (calleeType == "未知") {
        // 648 归档（错误恢复：上游已报）：此分支到达面=checkExpr(callee) 已报
        //   Error 的失败路径（未声明标识符/无此成员/嵌套调用失败等）——本处静默
        //   吞错继续查实参=错误恢复惯例（同 checkTransferCall 的 mt=="未知"
        //   早退先例），非查表 miss 终态，按立法不重复报错。
        for (auto& arg : node->arguments) checkExpr(arg.get());
        lastType_ = "未知";
        return;
    }
    // ---- 031（#1075）：绑定方法值变量调用 `cb(实参)`（零新语法）----
    //   callee 静态类型=方法值<类,签名>：参数检查按内嵌签名；IR 层
    //   CallIndirect(code)(this, 实参...)（this 注入参数位 0）----
    if (types::isMethodValueType(calleeType)) {
        checkMethodValueCall(node, calleeType);
        return;
    }
    if (checkFuncPtrCall(node, calleeType)) return;   // 族3：函数指针间接调用

    // 其他被调者（成员函数等）：后续Task实现，跳过
    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "无法调用非函数类型 '" + calleeType + "'");
    lastType_ = "未知";
}

bool SemanticAnalyzer::checkFuncPtrCall(CallExpr* node, const std::string& calleeType) {
    if (isFuncPtrType(calleeType)) {
        // 1360 结果化：calleeType 已过 isFuncPtrType 前缀守卫·规范形态恒有值；
        //   显形默认（空返回/零参）=原残缺回落行为
        const std::string retType =
            funcPtrReturnOf(calleeType).value_or(std::string());
        const std::vector<std::string> paramTypes =
            funcPtrParamsOf(calleeType).value_or(std::vector<std::string>());
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
            // #1588：实参位目标=形参类型（构造器按目标载荷宽度发射）
            ctorTargetStack_.push_back(syntheticNameToTemplate(paramTypes[i]));
            std::string argType = checkExpr(node->arguments[i].get());
            ctorTargetStack_.pop_back();
            const std::string& paramType = paramTypes[i];
            if (!canConvertWithLiteral(node->arguments[i].get(), argType, paramType) &&
                !canConvertArgNarrow(node->arguments[i].get(), argType, paramType)) {
                // 1819（1784 丁案实施④）：值形态类切片（派生类实参 → 基类值
                //   形参）=专用切片诊断（869 面·函数指针实参同款）；切片判定
                //   真=已报，不再叠加通用诊断。
                if (!reportSliceReject(argType, paramType,
                                       node->arguments[i]->location)) {
                    diagnostics_.report(DiagnosticLevel::Error, node->arguments[i]->location,
                                        "函数指针第 " + std::to_string(i + 1) +
                                        " 个参数无法将 '" + argType + "' 隐式转换为 '" +
                                        paramType + "'");
                }
            }
        }
        lastType_ = retType;
        // 1578（#1578·2026-10-30 伞批·#1571 同族）：**函数指针/闭包变量调用**
        //   的 A2 拥有契约写回——原实现只置 lastType_，不置 retOwnedString：
        //   声明位 RAII 登记/赋值位释放/语句位弃串释放/链段结算全部按「非拥有」
        //   处理=返回堆串无人接管，每个消费点净泄 1（1517 形二值/形二引/形二写
        //   活差 2 应为 1 实录·v2 同源）。返回段**精确**等于 "字符串"=拥有
        //   （A2 契约）；"字符串&"（引用返回借用视图）与标量返回不置位。
        node->retOwnedString = (retType == "字符串");
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
        // #1588：实参位求值期间遮蔽外层目标（就近原则——外层声明/赋值目标不得
        //   透传给实参表达式；形参类型在决议后经 calibrateCtorTargetType 回填）
        ctorTargetStack_.push_back(std::string());
        std::vector<std::string> argTypes =
            collectCallArgTypes(node->arguments, node->location);
        ctorTargetStack_.pop_back();
        // 217（027 波2a·001 §5.8a）：新线程 实参位可搬运检查——按值拷贝进新线程的
        //   值须可搬运（§1.1a① 编译期硬错误·Rust Send 对照）。置于重载决议之前：
        //   可搬运诊断是实参槽语义（先于形参签名不匹配诊断·双侧诊断面同构）。
        //   用户面=stdlib 包装 新线程；模块体内直调=内置 线程::新（旧点号 线程.新）。
        //   实参2 形态：顶层 空类型*/空类型/未知=内置豁免窗口放行（sunset=218）。
        if ((calleeName == "新线程" || calleeName == "线程::新" ||
             calleeName == "线程.新") &&
            argTypes.size() >= 2) {
            const std::string& argTy = argTypes[1];
            // 218 sunset 收窄（001 §5.8b）：顶层 空类型* 豁免仅限 不安全 函数体内
            //   （§1.1a④·安全区传裸指针值=同拦）
            const bool exemptWindow218 = (argTy == "空类型*") && currentFnUnsafe_;
            if (!argTy.empty() && argTy != "未知" && argTy != "空类型" &&
                !exemptWindow218) {
                std::string offender;
                if (!isPortableType(argTy, offender)) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, node->arguments[1]->location,
                        "类型 '" + argTy + "' 含裸指针字段 '" + offender +
                            "'，不可跨线程搬运（001 §5.8a；如确需跨线程请声明 不安全 可搬运）");
                }
            }
        }
        // 第 4 层（crate 隔离）：限定调用按模块过滤（数学::双倍 只解析数学.cn 的）
        // 55-c 方案A：实参字面量标志供决议豁免（`读值(100)` 传 正32 形参保留）
        std::vector<bool> argLitFlags;
        std::vector<const Expr*> argExprs;
        argLitFlags.reserve(node->arguments.size());
        argExprs.reserve(node->arguments.size());
        for (const auto& a : node->arguments) {
            argLitFlags.push_back(isIntLiteralExpr(a.get()));
            // 1263：字面量表达式随传（conversionLevel 值域判据——4000000000L
            //   对整32 形参超域=该候选淘汰·治 total 相等误歧义）
            argExprs.push_back(a.get());
        }
        // 1360 结果化批3B：决议失败/歧义（已报诊断）无值——value_or 显形空串
        //   回落保持「决议失败恢复兼容」原控制流（checkDirectCallFallback 空
        //   key 判据）。
        const cn::Optional<std::string> sigKeyOpt =
            resolveOverload(calleeName, argTypes, node->location,
                            node->moduleFilter, argLitFlags, argExprs);
        const std::string sigKey = sigKeyOpt.value_or(std::string());
    if (checkDirectCallFallback(node, calleeName, sigKey)) return true;
        // plans/018 呈报二 A′（2026-09-07 用户裁决）：resolvedSignature = 注册键。
        //   注册键在 registerFunction 已按函数链接键公式生成（模块条目自带
        //   模块$ 前缀、主/单文件恒裸键），与定义侧 mangledName 同源——
        //   旧「决议后按 it->second.moduleName 补拼前缀」的补丁块删除
        //   （其仅为旧裸键注册方案的对齐补丁，公式化后结构上不存在劈叉）。
        node->resolvedSignature = sigKey;
        auto it = functions_.find(sigKey);
        const FunctionInfo& info = it->second;
        // #1588：构造器实参目标校准——形参类型为权威目标（原实现按实参窄类型
        //   发射临时合成体，被调方按宽目标 CopyStruct = 高位栈垃圾静默错值；
        //   校准后 IR 按目标载荷宽度 Cast 写满）。argTypes 同步为校准后类型，
        //   转换检查按真实类型判定。
        for (std::size_t i = 0;
             i < node->arguments.size() && i < info.paramTypes.size(); ++i) {
            auto* argCall = dynamic_cast<CallExpr*>(node->arguments[i].get());
            if (argCall == nullptr) continue;
            if (calibrateCtorTargetType(argCall, info.paramTypes[i])) {
                argTypes[i] = argCall->resolvedType;
            }
        }
        // plans/019 阶段4（2026-09-10）：安全区边界观察期——外部 函数 调用
        //   （FFI=C 边界，类型安全不保证）与裸释放（释放=手动内存管理）应在
        //   不安全 函数 内
        if (info.isExtern) {
            reportUnsafeBoundary(node->location, "外部函数调用", calleeName);
        }
        if (calleeName == "释放") {
            reportUnsafeBoundary(node->location, "裸释放", "释放(指针)");
        }
        // #1674 丙案（001 §5.6 ②）：析构体内对资源字段手动 字符串释放=编译期
        //   拒——字段级联将再 free 同句柄=双释放。正路=摘除(字段) 或裸指针字段+
        //   不安全 函数。局部变量不拦（污染名防护既有）。字段形态=裸字段名
        //   （当前类实例字段）或成员访问（字段拥有者=对象·嵌套深度不限）。
        if (calleeName == "字符串释放" && currentFnIsDestructor_ &&
            !node->arguments.empty()) {
            const Expr* rel0 = node->arguments[0].get();
            bool isResourceField = false;
            if (rel0->getType() == NodeType::IdentifierExpr) {
                isResourceField = isCurrentClassFieldName(
                    static_cast<const IdentifierExpr*>(rel0)->name);
            } else if (rel0->getType() == NodeType::MemberExpr) {
                isResourceField = true;
            }
            if (isResourceField) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "析构函数体内禁止对资源字段手动 字符串释放："
                                    "该字段由编译器级联自动释放（手动释放将导致"
                                    "二次释放）——请改用 摘除(字段) 提前放手，或"
                                    "将字段改为裸指针类型并在 不安全 函数 内自行"
                                    "管理");
            }
        }
        // 616（2026-10-14 伞批·102 甲案第三形态补齐）：字符串释放(字符串形参)
        //   ＝消耗标记——102 甲案注释宣称「体内赋值/转移()/字符串释放() 命中即
        //   置 ownedConsumed」，字符串释放 路径原缺标记：非消耗形参=借用直传
        //   （形参槽=调用方句柄），被调 __cn_str_free 直放=释放调用方缓冲区
        //   （违反 001 §10.3「参数=借用·被调方不释放」；探针残留 -1+调用方变量
        //   悬垂实锤）。标记后 IR 层 prologue 深拷，释放只作用于自身副本。
        //   markStringParamConsumed 只对形参名命中（非形参零影响）。
        if (calleeName == "字符串释放" && !node->arguments.empty() &&
            node->arguments[0]->getType() == NodeType::IdentifierExpr) {
            auto* relArg =
                static_cast<IdentifierExpr*>(node->arguments[0].get());
            std::string relVarType;
            if (lookupVar(relArg->name, relVarType) &&
                types::canonicalEquals(relVarType, "字符串")) {
                markStringParamConsumed(relArg->name);
            }
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
            // 1891（2026-11-01·1283 拒面转正配套）：整数转字符串/正数转字符串
            //   整128/正128 实参已由 resolveOverload 入口精确路由至 128 位重载
            //   签名（semantic_sig.cpp·semantic_builtins 注册）——1283 甲案编译期
            //   拒面随重载落地撤销转正，本循环按选中签名形参逐位检查零特判。
            //   整128 形参=精确匹配零转换；IR 层按 resolvedSignature 经
            //   calibrateCallArgABI 定标双槽 ABI + __cn_str_from_i128/
            //   __cn_str_from_u128 全宽转串（ir_call_direct 分派）。
            const std::string& paramType = info.paramTypes[i];
            if (!canConvertWithLiteral(node->arguments[i].get(), argTypes[i], paramType) &&
                !canConvertArgNarrow(node->arguments[i].get(), argTypes[i], paramType)) {
                // 031（#1075）：方法值→fnptr 实参拒绝=专用文案（单点）——
                //   合法面（方法值实参→方法值形参）由 canConvert 同串放行。
                if (!method_value_943::reportConvertReject(
                        diagnostics_, argTypes[i], paramType,
                        node->arguments[i]->location, node->arguments[i].get())) {
                    diagnostics_.report(DiagnosticLevel::Error, node->arguments[i]->location,
                                        "函数 '" + calleeName + "' 第 " + std::to_string(i + 1) +
                                        " 个参数无法将 '" + argTypes[i] + "' 隐式转换为 '" +
                                        paramType + "'");
                }
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
