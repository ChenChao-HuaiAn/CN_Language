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
    const std::string calleeFnPtrType442 = isDirect
                                               ? std::string()
                                               : funcPtrTypeOfCallee(node->callee.get());
    const std::vector<std::string> effectiveParamTypes =
        resolveEffectiveParamTypes(node, isDirect, calleeName, calleeFnPtrType442);

    std::vector<ir::IRValue> args = buildCallArgs(
        node, effectiveParamTypes, isStrConsumer617, consumedArgTemps617);
    finalizeCallArgs(node, calleeName, isDirect, args);   // 106 装箱 + 默认实参
    if (isDirect) {
        emitDirectCall(node, calleeName, args, consumedArgTemps617);
        return;
    }
    // 337-a（T53 家系·间接调用路径）：i128/u128 形参的窄整实参宽化——函数指针
    //   调用（如 `整32(*指)(整128)` 的 `指(200000000000)`）原缺宽化 → 字面量实参
    //   物理 i64 直传 → 被调方按 i128 指针 ABI 解引用 SIGSEGV（探针 p_fnptr rc=139）。
    //   形参列表来自 callee 表达式的**源码类型串**（`函数指针<返回>(参数,...)`，
    //   与语义层 funcPtrParamsOf 同口径、同一规范化格式）——声明位/参数位
    //   登记时即写完整串（「登记字面量 `函数指针` 丢形参」=半截机制的根治）。
    //   树形态覆盖：标识符（局部变量/函数指针参数/全局）、成员访问（对象字段
    //   函数指针）；无法解析出形参列表时保守跳过（零行为变更）。
    // 485（间接调用返回面根治·探针 p8/p10 C0000005 实锤）：callee 源码类型串
    //   单次解析（funcPtrTypeOfCallee），形参宽化与返回类型同源消费——原返回
    //   类型硬编码 "i32"：i128 返回=调用点不传 sret 隐藏指针且按 eax 取值，
    //   蹦床（432 ?lambdaN$trap）按 sret 协议写 rcx 垃圾地址=访问违例 C0000005
    //   （[=] 闭包经返回真逃逸后跨函数调用立案形态·#435 轮探针）；整64 返回=
    //   eax 低 32 截断（10000000000→1410065408 实测）；浮点返回=eax 垃圾；
    //   结构体/数组返回=retbuf 预插缺失。三后端 emitCall 均按 result.type
    //   分派（i128 sret/浮点 xmm/整型位宽）——IR 层一处修三后端同受益。
    //   解析失败保守兜底 "i32"（现状零行为变更）。
    // 442：callee 源码类型串在循环前已单次解析（calleeFnPtrType442·实参定标
    //   判据源同源消费）——此处直接复用，不重复解析。
    const std::string& calleeFnPtrType = calleeFnPtrType442;
    if (!calleeFnPtrType.empty()) {
        const std::vector<std::string> fnPtrParams =
            types::funcPtrParamsOf(calleeFnPtrType);
        if (!fnPtrParams.empty() && fnPtrParams.size() == args.size()) {
            widenI128Args(args, fnPtrParams, node->location);
        }
        // 结构体/数组返回 fnptr：形态A同构（直接调用 087 同款）——retbuf 预插，
        //   operands=[fnptr, retbuf, 实参...]（operand[0]=call 目标·operand[1]=
        //   隐藏返回指针落 rcx），result=void，lastExpr_=retbuf 地址（调用方
        //   CopyStruct 消费与直接调用同链）。
        const std::string retSrc = types::funcPtrReturnTypeOf(calleeFnPtrType);
        if (retSrc.empty()) {
            // 648 miss 不可静默：函数指针类型串已解析但返回类型未登记——原静默
            //   落终态 "i32"（上方 801 行注释自认保守兜底；485 修的本链已登记
            //   面，此报错收口未登记面）。报诊断后保留 i32 兜底与控制流
            //   （driver generate() 后 hasErrors() 中止）。
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "内部错误: 函数指针返回类型未登记 '" + calleeFnPtrType + "'（#648）");
        }
        if (!retSrc.empty() && semantic_ != nullptr &&
            (semantic_->isStructType(types::canonical(retSrc)) ||
             types::isArray(retSrc))) {
            ir::IRValue calleeVal = genExpr(node->callee.get());
            const std::string temp = "__retbuf" + std::to_string(varCounter_++);
            emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
                 temp, "ptr", node->location);
            // 返回缓冲区按 64 字节（8 槽）分配——与被调方 epilogue 的 64 字节
            //   拷贝上限一致（结构体实际大小 ≤64 字节场景）
            function_->varSlots[temp] = 8;
            registerVarSlots(temp, retSrc);
            ir::IRValue buf = emitResult(ir::Opcode::AddrOf,
                                         {ir::IRValue::var(temp, "i64")},
                                         "ptr", temp, node->location);
            args.insert(args.begin(), buf);        // operand[1]=隐藏返回指针
            args.insert(args.begin(), calleeVal);  // operand[0]=指针寄存器
            emit(ir::Opcode::CallIndirect, args, ir::IRValue(), "", "void",
                 node->location);
            lastExpr_ = buf;
            return;
        }
        // 标量返回：返回类型按 fnptr 声明映射（空类型维持 "i32" 兜底——result
        //   无消费者·eax 装载无害·零行为变更）
        if (!retSrc.empty()) {
            const std::string mapped = mapType(retSrc);
            if (mapped != "void" && mapped != "") {
                ir::IRValue calleeVal = genExpr(node->callee.get());
                args.insert(args.begin(), calleeVal);  // operand[0]=指针寄存器
                lastExpr_ = emitResult(ir::Opcode::CallIndirect, args, mapped, "",
                                       node->location);
                return;
            }
        }
    }
    // 间接调用：先求被调者表达式（函数指针变量），再 CallIndirect
    // 655（648 余量②收口）：静态槽 fnptr 豁免已由 funcPtrTypeOfCallee 窄放宽
    //   根治。655 返工（720/859/939/942 连锁红根治）：本尾到达条件实为四种，
    //   原诊断把整个掉穿面误当全域 miss 面报错，现仅①报——
    //   ①calleeFnPtrType 空且非闭包变量=真全域 miss（签名终态缺失·静默错值
    //     面）→ 显形（探针 t1~t4 实证解析链无缺口·形参/局部/静态登记全通）；
    //   ②返回=空类型（retSrc 非空·mapped=="void" 不 return 掉穿）=合法面：
    //     result 无消费者·i32 兜底 eax 装载无害（stdlib/测试框架.cn:55 实锤·859/939/942 红根因）；
    //   ③retSrc 空已由上方 #648 报过，再报=双报；
    //   ④lambda 变量但 closureInfo_ 快径 miss（lambdaVarNames_ 留痕）=蹦床通道
    //     合法面：432 lambda 值=蹦床地址·CallIndirect 与快径语义等价——快径受
    //     visitLambdaExpr 入口 clear（485）影响（720 实录·取和走蹦床正确）。
    //   各形态发射同链零行为变化；若合法程序落①=新 miss 形态，须补签名解析
    //   而非回撤诊断（110/861-a 同教训；v2 侧 发射间接调用480 无此病）。
    const bool lambdaTrampoline655r = lambdaVarNames_.count(calleeName) > 0;
    if (calleeFnPtrType.empty() && !lambdaTrampoline655r) {
        diagnostics_.report(
            DiagnosticLevel::Error, node->location,
            "内部错误: 间接调用被调者类型串未解析（funcPtrTypeOfCallee 全域 miss·"
            "#655）");
    }
    ir::IRValue calleeVal = genExpr(node->callee.get());
    args.insert(args.begin(), calleeVal);  // operand[0]=指针寄存器
    lastExpr_ = emitResult(ir::Opcode::CallIndirect, args, "i32", "", node->location);
}


} // namespace cn_compiler
