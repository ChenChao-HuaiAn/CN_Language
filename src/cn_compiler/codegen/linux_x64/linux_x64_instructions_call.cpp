// Linux x86_64 代码生成器——函数调用降级（474 拆分：自 linux_x64_instructions.cpp 按族拆出·win x64_instructions_call.cpp 同构先例）
// 职责：emitCall（SysV 核心：整型/浮点独立计数分派寄存器、栈参数排队、
//   隐藏返回指针、变参 AL 契约、间接调用判零）
// 规范：英文API命名，中文仅注释；函数<=100行
#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 变参 C 库函数判定（SysV 调用点须设 AL）：__cn_format 为唯一 vdprintf 族
//   （string_api.cpp `__cn_format(const char*, ...)`）；后续新增变参运行时符号
//   在此追加。
static bool isVariadicCallee(const std::string& callee) {
    return callee == "__cn_format";
}
// ==================== 函数调用 ====================

// 函数调用（SysV 核心）：整型/浮点独立计数分派寄存器，超出走统一栈序列
//   1. 整型类（含指针/i128地址/结构体按值地址）rdi,rsi,rdx,rcx,r8,r9；浮点 xmm0~xmm7
//   2. 隐藏返回指针（结构体/i128 返回）占 rdi（第1整型位），真实整型位号后移
//   3. 栈参数：整型超6 / 浮点超8 的部分按声明顺序在栈上排队 [rsp+seq*8]
//   4. 返回缓冲区（16字节）放栈参数区上方 [rsp+stackArgs*8]
//   5. call 前 rsp 必须 16 字节对齐（alignPad 垫片）
//   6. i128 常量实参经 .data 全局常量池（L128cN）传地址，无栈临时区布局风险
void LinuxX64CodeGenerator::emitCall(LinuxX64AsmWriter& writer,
                                     const ir::IRInstruction& inst) {
    const bool isIndirect = (inst.opcode == ir::Opcode::CallIndirect);
    const std::string callee = inst.extra;
    const std::size_t argBase = isIndirect ? 1 : 0;
    const std::size_t argCount = inst.operands.size() - argBase;
    // #1212 d：SysV xmm 全为调用者保存——call 前把所有在调用点活跃的已分配
    //   浮点值保存到各自原槽（必须在浮点实参装载之前：xmm0~xmm7 参数位含池内
    //   xmm6），call 返回后恢复（call-clobber spill，镜像 LLVM 策略）。
    saveLiveFpForCall(writer);
    // 隐藏返回指针（i128/u128/struct* 结果类型）——三路判定，对齐 ARM64 后端。
    //   IR 契约（ir_call.cpp「隐藏返回指针作为第一个参数」）：用户结构体返回调用
    //   由 IR 层预插 retbuf 地址为 operands[0]、result.type=void——本后端原样
    //   传递使其自然落位0（rdi，SysV 隐藏指针位），无需 argOffset 后移
    //   （ARM64 单位机全量 E2E 锚定此形态；win x64 已于 2026-09-05 归真为同款
    //   三路判定+形态A原样传递，其旧第4路被调查询系死代码已删，plans/016）
    const bool hasBigRet = (inst.result.type == "i128" ||
                            inst.result.type == "u128" ||
                            inst.result.type.rfind("struct", 0) == 0);
    // ---- 第一遍：位置分配（整型位/浮点位/栈序列号） ----
    // argClass[i]: 0=整型寄存器 1=浮点寄存器 2=栈（stackSeq 记录序号）
    std::vector<int> argKind(argCount, 0);
    std::vector<int> argPos(argCount, -1);   // 寄存器位号 或 栈序列号
    int intIdx = hasBigRet ? 1 : 0;          // rdi 被 retbuf 占用时从 rsi 起
    int floatIdx = 0;
    int stackCounter = 0;
    for (std::size_t i = 0; i < argCount; ++i) {
        const ir::IRValue& av = inst.operands[argBase + i];
        const std::string& t = av.type;
        if (isFloatType(t)) {
            if (floatIdx < 8) {
                argKind[i] = 1;
                argPos[i] = floatIdx++;
            } else {
                argKind[i] = 2;
                argPos[i] = stackCounter++;
            }
        } else {
            // 整型类（i8..i64/u*/ptr/i1/布尔/i128地址/结构体地址）
            if (intIdx < 6) {
                argKind[i] = 0;
                argPos[i] = intIdx++;
            } else {
                argKind[i] = 2;
                argPos[i] = stackCounter++;
            }
        }
    }
    const int stackArgs = stackCounter;
    const int stackBytes = stackArgs * 8;
    const int alignPad = (stackBytes % 16 == 0) ? 0 : (16 - stackBytes % 16);
    const int totalAlloc = stackBytes + alignPad;
    if (totalAlloc > 0) {
        writer.line("sub rsp, " + std::to_string(totalAlloc));
    }
    // 隐藏返回指针：retbuf = 帧内固定区 [rbp+retbufFrameOffset_]（rbp 相对持久，
    //   对齐 win x64——rsp 临时区 add rsp 后悬垂，跨调用读 .值 读到垃圾）
    if (hasBigRet) {
        writer.line("lea rdi, " + stackMemText(retbufFrameOffset_));
    }
    // ---- 栈参数（声明顺序排队；先写栈再用临时寄存器装寄存器参数） ----
    // i128 常量辅助：解析 "LO:HI" 十六进制 / 十进制，落 .data 全局常量池返回标签
    auto i128ConstLabel = [this, &inst](const ir::IRValue& av) -> std::string {
        const std::string& ex = av.extra;
        const std::size_t colon = ex.find(':');
        std::uint64_t lo = 0, hi = 0;
        if (colon != std::string::npos) {
            lo = std::stoull(ex.substr(0, colon), nullptr, 16);
            hi = std::stoull(ex.substr(colon + 1), nullptr, 16);
        } else {
            // 1861（i128 常量文本解析族统一·与 ConstInt 面同款单点）：
            //   parseInt128InitText（十进制±/进制前缀/后缀自断）
            unsigned long long loP = 0, hiP = 0;
            if (types::parseInt128InitText(ex, inst.type == "i128", loP, hiP)) {
                lo = static_cast<std::uint64_t>(loP);
                hi = static_cast<std::uint64_t>(hiP);
            } else {
                try { lo = static_cast<std::uint64_t>(std::stoull(ex)); } catch (...) {}
            }
        }
        return registerI128Constant(lo, hi);
    };
    for (std::size_t i = 0; i < argCount; ++i) {
        if (argKind[i] != 2) continue;
        const ir::IRValue& av = inst.operands[argBase + i];
        const std::string& argType = av.type;
        const std::string mem = "[rsp+" + std::to_string(argPos[i] * 8) + "]";
        if (argType == "i128" || argType == "u128") {
            // i128 栈参数：传双槽地址指针（常量 -> 全局池；寄存器 -> 低64位槽地址）
            if (av.isConstant) {
                emitLoadSymbolAddr(writer, "r10", i128ConstLabel(av));
            } else if (!av.isConstant) {
                // #1212 d：成对分配实参先成对溢出到原双槽，再取低半槽地址
                spillI128Pair(writer, av);
                writer.line("lea r10, " + stackMemText(regSlotOffset(av.id + 1)));
            }
            writer.line("mov qword ptr " + mem + ", r10");
        } else if (isFloatType(argType)) {
            loadOperandToV(writer, av, "xmm0");
            writer.line("mov" + std::string(argType == "f64" ? "sd" : "ss") +
                        " qword ptr " + mem + ", xmm0");
        } else {
            const std::string reg = loadOperandToX(writer, av, "r10");
            writer.line("mov qword ptr " + mem + ", " + reg);
        }
    }
    // ---- 寄存器参数（整型 rdi..r9 / 浮点 xmm0..xmm7） ----
    for (std::size_t i = 0; i < argCount; ++i) {
        if (argKind[i] == 2) continue;
        const ir::IRValue& av = inst.operands[argBase + i];
        const std::string& argType = av.type;
        if (argKind[i] == 1) {
            loadOperandToV(writer, av, "xmm" + std::to_string(argPos[i]));
        } else if (argType == "i128" || argType == "u128") {
            // i128 寄存器实参：传双槽地址指针
            const std::string reg = intParameterRegister(argPos[i]);
            if (av.isConstant) {
                emitLoadSymbolAddr(writer, reg, i128ConstLabel(av));
            } else if (!av.isConstant) {
                // #1212 d：成对分配实参先成对溢出到原双槽，再取低半槽地址
                spillI128Pair(writer, av);
                writer.line("lea " + reg + ", " + stackMemText(regSlotOffset(av.id + 1)));
            }
        } else {
            loadOperandToX(writer, av, intParameterRegister(argPos[i]));
        }
    }
    // ---- 变参调用：AL = 使用的向量寄存器个数（SysV AMD64 ABI 强制契约） ----
    //   F6（2026-09-11 第七十二轮根治）：变参函数（vdprintf 族/__cn_format）经
    //   stdarg 读取 xmm0..xmm7 的保存区，AL 告诉被调方存了几个；不设 AL 时
    //   glibc 的 va_arg 按 AL 值判定——rax 被前序调用/释放污染后浮点实参读到
    //   0（探针：包装函数链下 `格式化("¥%.2f", 29.50)` 输出 ¥0.00；直连形态
    //   靠 rax 残留侥幸正确）。Rust/C 编译器对此无条件发 mov al, N
    //   （clang/gcc 变参调用点均有 xor eax,eax 或 mov al,imm）。
    if (!isIndirect && isVariadicCallee(callee)) {
        int vecUsed = 0;
        for (std::size_t i = 0; i < argCount; ++i) {
            if (argKind[i] == 1) ++vecUsed;
        }
        writer.line("mov eax, " + std::to_string(vecUsed));  // AL=变参向量寄存器个数（SysV 契约）
    }
    // ---- 调用（间接：函数指针经 r11——SysV 惯例 call-clobbered 非传参寄存器） ----
    if (isIndirect) {
        loadOperandToX(writer, inst.operands[0], "r11");
        // 040（001 §5.8 子案 A·2026-10-05 用户裁决）：空调用判零——零值 →
        //   运行时错误(3) 空指针解引用（093 判空家族同码同文案·GAS L 标签·
        //   SysV 错误码走 rdi·错误块不返回与 FieldAddr 判空同款）
        const int fnptrNullId = ptrCheckCounter_++;
        const std::string fnptrOk = "Lfnptr_ok" + std::to_string(fnptrNullId);
        writer.line("test r11, r11");
        writer.line("jnz " + fnptrOk);
        writer.line("mov rdi, 3");
        writer.line("call __cn_runtime_error");
        writer.line("ret");
        writer.raw(fnptrOk + ":");
        writer.line("call r11");
    } else {
        writer.line("call " + symbolName(callee));
    }
    // 恢复栈（与分配对称）
    if (totalAlloc > 0) {
        writer.line("add rsp, " + std::to_string(totalAlloc));
    }
    // #1212 d：恢复调用点活跃浮点值（call 已返回；池不含 xmm0，不覆盖返回值）
    restoreLiveFpAfterCall(writer);
    // 返回值 -> 结果槽（浮点 xmm0，整型/指针 rax；i128/结构体经 rax=retbuf 读回）
    if (inst.result.id >= 0) {
        const int dstOff = regSlotOffset(inst.result.id);
        if (inst.result.type == "i128" || inst.result.type == "u128") {
            // i128 返回：缓冲区指针在 rax，读回双半（#1212 d：成对分配 -> 双 GPR）
            writer.line("mov r9, qword ptr [rax]");
            storeI128Half(writer, inst.result, false, "r9");
            writer.line("mov r9, qword ptr [rax+8]");
            storeI128Half(writer, inst.result, true, "r9");
        } else if (isFloatType(inst.result.type)) {
            // #1212 d：结果已分配到 xmm -> 返回值直写（免槽往返）
            storeFloatResult(writer, inst.result, "xmm0", inst.result.type);
        } else if (!isFloatType(inst.result.type) &&
                   inst.result.type != "i128" && inst.result.type != "u128") {
            // 1256：结果已分配 -> 返回值直写物理寄存器（窄/32 位用 32 位名，
            //   写 eax 清高 32 位；被调方 epilogue 已按返回类型扩展 rax）
            const cn::Optional<std::string> dstPhys = physRegOf(inst.result);
            if (dstPhys.has_value()) {
                writer.line("mov " + widthFor(inst.result.type, dstPhys.value()) + ", " +
                            widthFor(inst.result.type, "rax"));
            } else {
                emitStackStore(writer, dstOff, "rax", inst.result.type);
            }
        }
    }
}

} // namespace cn_compiler
