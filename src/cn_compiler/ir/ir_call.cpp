// CN-IR生成器实现：AST -> 三地址码IR（Task 1.6）
// 实现要点：
//   1. 表达式生成：字面量 -> ConstInt/ConstFloat/ConstBool/ConstString
//   2. 算术/比较/逻辑运算 -> 对应Opcode；操作数先递归生成
//   3. 变量声明 -> Alloca + Store；变量引用 -> Load
//   4. 控制流：如果/当/循环生成基本块与跳转；中断/继续通过循环上下文解析目标
//   5. 函数调用 -> Call（extra=函数名，操作数=实参寄存器）
//   6. 字符串常量 -> 模块常量池去重收集
#include <cstdio>
#include <string>
#include <utility>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

void IRGenerator::visitCallExpr(CallExpr* node) {
    // 81-a：调用返回结构体实参临时清理（RAII 守卫，单点覆盖全部 return 路径）。
    //   实参求值期登记（genArgValueWithCleanup），本函数返回（=调用已发射）后
    //   统一释放——元素槽/形参槽已持有独立副本，释放安全；幂等清零。
    //   基准式隔离：嵌套调用（实参本身是调用）各自持基准，内层只清理内层新增项。
    struct ArgCleanupGuard {
        IRGenerator* gen;
        std::size_t base;
        ~ArgCleanupGuard() { gen->flushPendingArgCleanups(base); }
    } argCleanupGuard{this, pendingArgCleanups_.size()};
    (void)argCleanupGuard;
    // 转移(变量) 展开（plans/019 阶段1·954 族体见 emitTransferCallExpand）
    if (isTransferCall(node) && !node->resolvedType.empty()) {
        emitTransferCallExpand(node);
        return;
    }
    // ---- 206-b（波 4·plans/022 §四.5）：复制(表达式) 泛型克隆内置 ----
    //   语义层已推导 resolvedType（=实参类型，genCopyBuiltin 分派依据）。
    //   与 转移(x) 构成显式「复制/移动」双内置（C++ copy/move 对照物）。
    if (node->callee->getType() == NodeType::IdentifierExpr &&
        !node->resolvedType.empty() &&
        static_cast<IdentifierExpr*>(node->callee.get())->name == "复制" &&
        node->arguments.size() == 1) {
        genCopyBuiltin(node, node->location);
        return;
    }
    // ---- 任务 474：回绕内置（954 族体见 emitWrapBuiltinCall·ir_call_builtin.cpp）----
    if (emitWrapBuiltinCall(node)) return;
    // ---- #1674 甲案：摘除 内置（954 族体见 emitRemoveBuiltinCall·ir_call_builtin.cpp）----
    if (emitRemoveBuiltinCall(node)) return;
    // ---- 阶段3 OOP（Task 3.1/3.2）：构造调用/成员方法调用/虚调用 ----
    // 构造调用 类名(实参)：callee 为类类型名（NewObject + 构造体 Call）；
    // 成员方法调用 对象.方法(实参)：callee 为 MemberExpr（虚 -> VirtualCall，
    //   非虚/父类/静态 -> 直接 Call）。命中即返回，未命中（普通函数调用）走原路径。
    if (handleClassCallExpr(node)) {
        return;
    }
    // ---- 阶段3 错误处理（Task 3.5）：内置构造器 正常/错误/某些 降级 ----
    // 语义层已推导 结果<T,E>/可选<T> 类型（resolvedType），此处分配合成结构体
    // 临时槽 + 写 是否正常/是否某些 + 值/错误值，返回结构体地址（ptr）。
    // 命中即返回，未命中（普通函数调用）走原路径。
    if (handleResultCtor(node)) {
        return;
    }
    // 判断是否为直接函数名调用（函数名不在变量表中）
    bool isDirect = false;
    std::string calleeName;
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        calleeName = static_cast<IdentifierExpr*>(node->callee.get())->name;
        // Task 2.10 lambda 闭包调用（速径展开·族体见 genClosureCallExpr·ir_call_closure.cpp）
        if (genClosureCallExpr(node, calleeName)) return;
        // 直调性判定（040 静态槽 fnptr）+ 重载/FFI 符号名决议（954 族体见 resolveCalleeDirectness）
        isDirect = resolveCalleeDirectness(node, calleeName);
    }
    // ---- 默认实参补全（Task 2.10·954 族体见 prepareDefaultArgValues）----
    prepareDefaultArgValues(node, isDirect);

    // ---- 打印 / 打印行 展开（Task 2.5 + Task 2.9·954 族体见 tryEmitPrintLikeCall）----
    if (tryEmitPrintLikeCall(node, isDirect, calleeName)) return;

    // ---- 字符串API名称映射（Task 2.5 + Task 2.8）：中文函数名 -> 运行时符号 ----
    // 617：内置字符串消费族判定（954 族体见 isBuiltinStrConsumer617）
    const bool isStrConsumer617 = isBuiltinStrConsumer617(calleeName, isDirect);
    // 662：收集项携带来源 AST（三元实参结算走分支拥有性感知条件释放）
    std::vector<std::pair<const Expr*, ir::IRValue>> consumedArgTemps617;
    if (isDirect) {
        mapBuiltinCalleeNameApi(calleeName);   // 954 族体两段迁出（ir_call_builtin.cpp）
        mapBuiltinCalleeNameLib(calleeName);
    }

    // ---- 442（判据统一·421 审查 P2-2/P2-3）：有效形参表（954 族体见 resolveEffectiveParamTypes）----
    const std::string calleeFnPtrType442 =
        isDirect ? std::string()
                 : funcPtrTypeOfCallee(node->callee.get()).value_or("");
    const std::vector<std::string> effectiveParamTypes =
        resolveEffectiveParamTypes(node, isDirect, calleeName, calleeFnPtrType442);

    std::vector<ir::IRValue> args = buildCallArgs(
        node, effectiveParamTypes, isStrConsumer617, consumedArgTemps617);
    finalizeCallArgs(node, calleeName, isDirect, args);   // 106 装箱 + 默认实参
    if (isDirect) {
        emitDirectCall(node, calleeName, args, consumedArgTemps617);
        return;
    }
    // 337-a/485/442：函数指针间接调用族（954 族体见 emitFnPtrCall 与 emitIndirectCallTail·
    //   ir_call_indirect.cpp）。原 L120 别名并入参数名。
    if (emitFnPtrCall(node, calleeFnPtrType442, args)) return;
    emitIndirectCallTail(node, calleeName, calleeFnPtrType442, args);
}


} // namespace cn_compiler
