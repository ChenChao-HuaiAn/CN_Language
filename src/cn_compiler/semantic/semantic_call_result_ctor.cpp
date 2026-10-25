// 语义调用检查·族2（#1588 自 semantic_call.cpp 搬迁：原文件 993 行贴 1000 红线，
//   先拆再加纪律；搬迁同时根治内置构造器目标载荷宽度缺陷——见下方 1588 段注释）
// 职责：内置构造器 正常(值)/错误(值)/某些(值) 的类型推导（Task 3.5，规格书07-五）
//   + 上下文目标类型校准（声明初始化/赋值/返回/实参位——#1588）。
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"

namespace cn_compiler {

// #1588：构造器上下文目标的解析结果（semantic.hpp 前向声明·定义单点在此）。
//   matched=false=目标顶层形态与构造器族不符（正常/错误 须 结果；某些 须 可选）
//   ——此时不校准（原「缺失侧补全」逻辑仍按目标 T/E 生效，行为零漂移）。
struct CtorTargetInfo {
    bool matched = false;
    std::string target;    // 名匹配时的目标整串（结果<...>/可选<...>，已归一尖括号）
    std::string payload;   // 目标载荷（正常/某些→T；错误→E）
};

namespace {

// 目标类型 + 构造器名 → 解析结果。$ 合成名（结果$整64$整32 / 可选$整64）先归一并
//   尖括号形态；解析不出=T/E 残缺，按不匹配处理（交回原推导）。
CtorTargetInfo ctorTargetInfoOf(const std::string& ctorName,
                                const std::string& targetRaw) {
    CtorTargetInfo info;
    const std::string target = syntheticNameToTemplate(targetRaw);
    if (isResultType(target)) {
        if (ctorName != "正常" && ctorName != "错误") return info;
        const std::vector<std::string> targs =
            resultTypeArgs(target).value_or((std::vector<std::string>()));
        if (targs.size() != 2) return info;
        info.matched = true;
        info.target = target;
        info.payload = (ctorName == "错误") ? targs[1] : targs[0];
        return info;
    }
    if (isOptionalType(target)) {
        if (ctorName != "某些") return info;
        const std::string t = optionalTypeArg(target).value_or(std::string());
        if (t.empty()) return info;
        info.matched = true;
        info.target = target;
        info.payload = t;
        return info;
    }
    return info;
}

// #1588：从「无目标推导」（node->resolvedType，构造器按实参类型的自然推导）
//   取出实参位占位类型（正常/某些→T；错误→E）——校准的载荷兼容性检查基准。
//   外部实参位（普通调用/构造调用/接口方法）在遮蔽外层目标后求值得到的正是
//   该推导，故无需另存实参类型（语义层不缓存表达式类型）。
std::string ctorPayloadArgTypeOf(const std::string& ctorName,
                                 const std::string& derivedType) {
    if (isResultType(derivedType)) {
        const std::vector<std::string> targs =
            resultTypeArgs(derivedType).value_or((std::vector<std::string>()));
        if (targs.size() != 2) return std::string();
        return (ctorName == "错误") ? targs[1] : targs[0];
    }
    if (isOptionalType(derivedType)) {
        return optionalTypeArg(derivedType).value_or(std::string());
    }
    return std::string();
}

}  // namespace

// #1588：当前权威构造器目标类型（已归一尖括号；空串=无目标）。
//   栈非空时栈顶权威——空串元素=显式遮蔽（实参位求值/嵌套构造器实参：目标
//   不得透传给内层表达式；原实现栈顶为空即回退返回类型，外层目标会污染实参
//   推导）。栈空回退当前函数返回类型（返回语句位）。
std::string SemanticAnalyzer::currentCtorTargetType() const {
    if (!ctorTargetStack_.empty()) {
        const std::string& top = ctorTargetStack_.back();
        if (top.empty()) return std::string();
        return syntheticNameToTemplate(top);
    }
    return syntheticNameToTemplate(currentReturnType_);
}

// #1588：按目标类型校准构造器 resolvedType（外部实参位调用：普通函数/构造/接口
//   方法的形参类型在重载决议后已知）。载荷不可转换=用户类型错误——报可行动诊断
//   并仍采用目标类型（防外层「无法将 可选<整32> 转换为 可选<整64>」二次级联，
//   错误信息指向真正的失配点：载荷）。
//   载荷兼容性基准=node->resolvedType 的「无目标推导」（见 ctorPayloadArgTypeOf）；
//   无实参构造器（正常()）无载荷实参，跳过检查。
bool SemanticAnalyzer::calibrateCtorTargetType(CallExpr* node,
                                               const std::string& targetRaw) {
    // 正向形态分派（谓词式探测：调用形 + 内置构造器名 + 目标族匹配才校准）
    const bool callShape = node != nullptr && node->callee != nullptr &&
                           node->callee->getType() == NodeType::IdentifierExpr;
    if (callShape) {
        const std::string& ctorName =
            static_cast<IdentifierExpr*>(node->callee.get())->name;
        const bool ctorNameBuiltin =
            ctorName == "正常" || ctorName == "错误" || ctorName == "某些";
        if (ctorNameBuiltin) {
            const CtorTargetInfo info = ctorTargetInfoOf(ctorName, targetRaw);
            if (info.matched) {
                calibrateCtorPayload(node, ctorName, info);
                node->resolvedType = info.target;
                return true;
            }
        }
    }
    return false;
}

// #1588：载荷兼容性检查单点（校准与推导两路共用）——目标载荷不可转换=用户类型
//   错误（可行动文案指向失配点）；无实参（正常()）或实参类型未知（上游已诊断）
//   不复检。校准仍采用目标类型（防外层二次级联诊断）。
void SemanticAnalyzer::calibrateCtorPayload(CallExpr* node,
                                           const std::string& ctorName,
                                           const CtorTargetInfo& info) {
    if (!node->arguments.empty() && !node->resolvedType.empty()) {
        const std::string argType =
            ctorPayloadArgTypeOf(ctorName, node->resolvedType);
        if (!argType.empty() && argType != "未知" &&
            !canConvertType(argType, info.payload)) {
            reportCtorPayloadMismatch(node, ctorName, argType, info);
        }
    }
}

// #1588：载荷失配诊断单点（内置构造器实参无法转换为目标载荷）。
void SemanticAnalyzer::reportCtorPayloadMismatch(CallExpr* node,
                                                 const std::string& ctorName,
                                                 const std::string& argType,
                                                 const CtorTargetInfo& info) {
    diagnostics_.report(
        DiagnosticLevel::Error, node->arguments[0]->location,
        "内置构造器 '" + ctorName + "' 实参无法将 '" + argType +
            "' 隐式转换为目标 '" + info.target + "' 的载荷类型 '" +
            info.payload + "'");
}

// 族2：内置构造器 正常(值)/错误(值)/某些(值)（Task 3.5）——实参求值（载荷遮蔽）
//   + 结果/可选类型推导与写回（resolvedType 供 IR 层降级合成结构体）。
bool SemanticAnalyzer::checkBuiltinCtorCall(CallExpr* node) {
    // 正向形态分派（谓词式探测：内置构造器三名为编译器预置闭集·001 §5.4）
    const bool callShape = node->callee != nullptr &&
                           node->callee->getType() == NodeType::IdentifierExpr;
    const std::string builtinName =
        callShape ? static_cast<IdentifierExpr*>(node->callee.get())->name
                  : std::string();
    const bool ctorNameBuiltin =
        builtinName == "正常" || builtinName == "错误" || builtinName == "某些";
    if (callShape && ctorNameBuiltin) {
        // 参数类型检查：正常/错误/某些 期望 1 个实参；例外——正常() 无参数
        //   用于 结果<空类型,E>（空类型正常值，容器库 追加/删除 等返回
        //   结果<空类型,整32> 的 返回 正常()，Task 6.1）。
        if (node->arguments.size() != 1 &&
            !(builtinName == "正常" && node->arguments.empty())) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "内置构造器 '" + builtinName + "' 期望 1 个实参");
        }
        // #1588：本层目标的载荷 = 实参位期望类型——实参求值期间压入栈顶（就近
        //   原则：嵌套构造器（某些(某些(7))）按本层载荷推导，外层目标不再直接
        //   作用其内层）；无目标/名不匹配时压空串=显式遮蔽（内层不得透用外层
        //   返回类型目标）。
        const CtorTargetInfo outer =
            ctorTargetInfoOf(builtinName, currentCtorTargetType());
        std::string argType = "未知";
        for (auto& arg : node->arguments) {
            ctorTargetStack_.push_back(outer.matched ? outer.payload
                                                     : std::string());
            // checkExpr 非幂等（打印实参改写等）：实参类型=本次求值返回值
            argType = checkExpr(arg.get());
            ctorTargetStack_.pop_back();
        }
        deriveCtorResultType(node, builtinName, argType);
        // 任务 094（原 checkBuiltinCtorCall 逻辑·#1588 拆分时遗漏，回归修复）：
        //   装箱 move 语义对齐——IR 层 emitResultCtorValue 对「正常/某些 + 标识符
        //   实参 + 字符串值类型 + 非借用污染 + 局部槽」真 move 清源槽；语义层
        //   同判据 markMovedVar 挂既有已转移拦截（reportMovedUse），根除「装箱后
        //   旧名编译不禁用、运行静默读空串」（550 负测锁定）。判据逐条对齐 IR：
        //   ①仅 正常/某些（错误() 不清零）②实参纯标识符 ③值类型字符串
        //   ④非借用污染（污染=借用装箱指针共享随源存活）⑤排除类字段（字段走
        //   this 寻址·IR lookupVarName 空=不清零，标记即误禁）。
        if ((builtinName == "正常" || builtinName == "某些") &&
            node->arguments.size() == 1 &&
            node->arguments[0]->getType() == NodeType::IdentifierExpr &&
            types::canonicalEquals(argType, "字符串")) {
            const std::string& srcName =
                static_cast<const IdentifierExpr*>(node->arguments[0].get())
                    ->name;
            if (!isCurrentClassFieldName(srcName) && !isStrTainted(srcName)) {
                markMovedVar(srcName, node->location.getLine());
            }
        }
        // 写回推导类型（Task 3.5 E2E 24 修复）：IR 层按 resolvedType 降级为
        //   合成结构体构造（分配槽 + 写 是否正常/是否某些 + 值/错误值）
        node->resolvedType = lastType_;
        return true;
    }
    return false;
}

// 内置构造器结果类型推导（#1588）：目标名匹配 → 采用目标类型（载荷按目标宽度
//   由 IR 层 Cast 写满）；否则按实参类型推导（缺失侧由目标 T/E 补全，原 574-a
//   逻辑；目标不可用按整32 兜底）。lastType_ 为唯一输出。
void SemanticAnalyzer::deriveCtorResultType(CallExpr* node,
                                           const std::string& builtinName,
                                           const std::string& argType) {
    const std::string target = currentCtorTargetType();
    // ---- 目标校准（#1588 主修复面）----
    const CtorTargetInfo info = ctorTargetInfoOf(builtinName, target);
    if (info.matched) {
        // 载荷失配检查（与实参位校准共用单点）：实参类型=本次 checkExpr 返回值
        if (!node->arguments.empty() && !argType.empty() && argType != "未知" &&
            !canConvertType(argType, info.payload)) {
            reportCtorPayloadMismatch(node, builtinName, argType, info);
        }
        lastType_ = info.target;
        return;
    }
    // ---- 无目标（或目标族不匹配）：按实参类型推导（原逻辑，缺失侧目标补全）----
    std::string ctxT = "";
    std::string ctxE = "";
    if (isResultType(target)) {
        const std::vector<std::string> targs =
            resultTypeArgs(target).value_or((std::vector<std::string>()));
        if (targs.size() == 2) {
            ctxT = targs[0];
            ctxE = targs[1];
        }
    } else if (isOptionalType(target)) {
        ctxT = optionalTypeArg(target).value_or(std::string());
    }
    // 构造器名闭集显式枚举（调用方 checkBuiltinCtorCall 已限定三选一；无 else 兜底）
    if (builtinName == "正常") {
        const std::string e = (ctxE.empty() ? "整32" : ctxE);
        lastType_ =
            "结果<" + (argType == "未知" ? "整32" : argType) + "," + e + ">";
    }
    if (builtinName == "错误") {
        const std::string t = (ctxT.empty() ? "整32" : ctxT);
        lastType_ =
            "结果<" + t + "," + (argType == "未知" ? "整32" : argType) + ">";
    }
    if (builtinName == "某些") {
        lastType_ = "可选<" + (argType == "未知" ? "整32" : argType) + ">";
    }
}

}  // namespace cn_compiler
