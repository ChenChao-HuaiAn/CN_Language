// CN-IR生成器实现：函数指针间接调用族（954 拆族：自 ir_call.cpp visitCallExpr
//   巨函数纯机械搬移·逐字保留零语义变化;函数≤100 行纪律）
//   族 = emitFnPtrCall（337-a 形参宽化/442 签名单源/485 返回 ABI 三形态:聚合 retbuf
//   预插或标量映射）+ emitIndirectCallTail（655r 四形态诊断 + CallIndirect 兜底）。
//   成员函数声明仍在 ir_generator.hpp。
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_generator.hpp"
#include "cn_compiler/model/ast_expr.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

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
// 954（visitCallExpr 拆族·机械搬移）：整体迁出;L120 别名并入参数名;命中 return true。
bool IRGenerator::emitFnPtrCall(CallExpr* node,
                                const std::string& calleeFnPtrType,
                                std::vector<ir::IRValue>& args) {
    // ---- 031（#1075）：绑定方法值变量调用（calleeFnPtrType=内嵌签名——
    //   funcPtrTypeOfCallee 031 窄放宽返回）——两槽装载 this/code 后
    //   CallIndirect：operands=[code, (retbuf), this, 实参...]——this 注入
    //   参数位 0（与实例方法直调 ABI 一致·被调方零特例）；sret 形态 retbuf
    //   预插后 this 顺延位 1（同实例直调 sret 协议）。实参已物理级
    //   （方法值实参两位展开已在 buildCallArgs 完成）。 ----
    if (node->callee != nullptr &&
        node->callee->getType() == NodeType::IdentifierExpr) {
        const std::string mvName =
            static_cast<IdentifierExpr*>(node->callee.get())->name;
        if (isMethodValueVar(mvName)) {
            const std::string unique = lookupVarName(mvName);
            // 静默豁免上游已诊断: 已报内部错误·return true=已消费占位（控制流不变）
            if (unique.empty()) {
                // miss 不可静默：语义层保证方法值变量已登记槽位
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                    "内部错误: 方法值变量 '" + mvName + "' 槽未登记（#1075）");
                return true;
            }
            ir::IRValue thisVal, codeVal;
            loadMethodValuePair(unique, thisVal, codeVal, node->location);
            args.insert(args.begin(), thisVal);   // 参数位 0（this 注入）
            const std::string retSrcMV =
                types::funcPtrReturnTypeOf(calleeFnPtrType);
            if (!retSrcMV.empty() && semantic_ != nullptr &&
                (semantic_->isStructType(types::canonical(retSrcMV)) ||
                 types::isArray(retSrcMV))) {
                const std::string temp = "__retbuf" + std::to_string(varCounter_++);
                emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
                     temp, "ptr", node->location);
                function_->varSlots[temp] = 8;
                registerVarSlots(temp, retSrcMV);
                ir::IRValue buf = emitResult(
                    ir::Opcode::AddrOf, {ir::IRValue::var(temp, "i64")},
                    "ptr", temp, node->location);
                args.insert(args.begin(), buf);       // 隐藏返回指针（位 0 前插）
                args.insert(args.begin(), codeVal);   // 间接调用目标
                emit(ir::Opcode::CallIndirect, args, ir::IRValue(), "", "void",
                     node->location);
                lastExpr_ = buf;
                return true;
            }
            // 标量返回（空类型维持 "i32" 兜底——同 fnptr 谱·result 无消费者）
            std::string mapped = mapType(retSrcMV);
            if (mapped.empty() || mapped == "void") mapped = "i32";
            args.insert(args.begin(), codeVal);   // operand[0]=指针寄存器
            lastExpr_ = emitResult(ir::Opcode::CallIndirect, args, mapped, "",
                                   node->location);
            return true;
        }
    }
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
            return true;
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
                return true;
            }
        }
    }
    return false;
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
// 954（visitCallExpr 拆族·机械搬移）：整体迁出（尾部 655r 诊断 + CallIndirect 兜底发射）。
void IRGenerator::emitIndirectCallTail(CallExpr* node,
                                       const std::string& calleeName,
                                       const std::string& calleeFnPtrType,
                                       std::vector<ir::IRValue>& args) {
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
