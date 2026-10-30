// Linux ARM64 (AArch64) 代码生成器——函数调用降级（474 拆分：自 arm64_instructions.cpp 按族拆出·x64_instructions_call.cpp 同构先例）
// 职责：emitCall（AAPCS64 核心：x0~x7/v0~v7 分派、栈参数排队、隐藏返回指针、
//   变参禁止伪装、间接调用判零）
// 规范：英文API命名，中文仅注释；函数<=100行
#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {
// ==================== 函数调用 ====================

// 函数调用：前8参数入寄存器（x0~x7 / v0~v7），第9起写调用栈帧
// AAPCS64：
//   1. 前8整型/指针参数 x0~x7；浮点参数按位 v0~v7（第 N 参数用 xN 或 vN）
//   2. 栈参数（第9起）：调用方在 call 前 sub sp,#stackArgs*8 预留，
//      从 [sp] 起依次存放（第9参数 [sp]、第10 [sp+8]...）
//   3. 隐藏返回指针（结构体/i128 返回）占 x0，实参寄存器位号后移 1
//   4. call 前 sp 必须 16 字节对齐
//   5. 被调方 prologue 后栈参数位于 [x29, #16/32 + ...]（见 stackParamBase/
//      parameterRegister——锚定基单一归属）
void Arm64CodeGenerator::emitCall(Arm64AsmWriter& writer,
                                  const ir::IRInstruction& inst) {
    const bool isIndirect = (inst.opcode == ir::Opcode::CallIndirect);
    const std::string callee = inst.extra;
    const std::size_t argBase = isIndirect ? 1 : 0;
    const std::size_t argCount = inst.operands.size() - argBase;
    // 隐藏返回指针（i128/结构体返回）
    const bool hasBigRet = (inst.result.type == "i128" ||
                            inst.result.type == "u128" ||
                            inst.result.type.rfind("struct", 0) == 0);
    const std::size_t argOffset = hasBigRet ? 1 : 0;
    // 404：GP/FP 独立计数落位（AAPCS64 双队列·arm64AssignParamPlacements 单点，
    //   与 emitParamSetup/emitVirtualCall 三发射点同源；对齐 v2 分配参数位置 91-a）
    std::vector<std::string> argTypes;
    argTypes.reserve(argCount);
    for (std::size_t i = 0; i < argCount; ++i) {
        argTypes.push_back(inst.operands[argBase + i].type);
    }
    const std::vector<Arm64ArgPlacement> placements =
        arm64AssignParamPlacements(argTypes, static_cast<int>(argOffset));
    // 栈参数区大小（GP/FP 寄存器池用尽后的溢出参数·槽序=参数序）
    std::size_t stackArgs = 0;
    for (const Arm64ArgPlacement& p : placements) {
        if (p.cls == 2) ++stackArgs;
    }
    // 返回缓冲区（16 字节，栈顶下方）
    const int bigRetPad = hasBigRet ? 16 : 0;
    int stackBytes = static_cast<int>(stackArgs * 8 + bigRetPad);
    // 16 字节对齐
    const int alignPad = (stackBytes % 16 == 0) ? 0 : (16 - stackBytes % 16);
    const int totalAlloc = stackBytes + alignPad;
    if (totalAlloc > 0) {
        // 栈参数区/返回缓冲区分配（大偏移需分段）
        if (totalAlloc <= 4095) {
            writer.line("sub sp, sp, #" + std::to_string(totalAlloc));
        } else {
            emitMovImm(writer, "x13", static_cast<std::uint64_t>(totalAlloc));
            writer.line("sub sp, sp, x13");
        }
    }
    // 隐藏返回指针：返回缓冲区地址（[sp, #stackArgs*8]）-> x0
    if (hasBigRet) {
        if (stackArgs > 0) {
            writer.line("add x0, sp, #" + std::to_string(stackArgs * 8));
        } else {
            writer.line("mov x0, sp");
        }
    }
    // 栈参数（寄存器池溢出）：从 [sp] 开始（槽序=参数序·AAPCS64 Stage D）
    for (std::size_t i = 0; i < argCount; ++i) {
        const Arm64ArgPlacement& place = placements[i];
        if (place.cls != 2) continue;  // 寄存器参数稍后处理
        const ir::IRValue& av = inst.operands[argBase + i];
        const std::string& argType = av.type;
        const int memOff = static_cast<int>(place.num * 8);
        if (argType == "i128" || argType == "u128") {
            // i128 栈参数：传双槽地址指针（低64位槽地址）
            std::string addrReg;
            if (av.isConstant) {
                const std::string& ex = av.extra;
                const std::size_t colon = ex.find(':');
                std::uint64_t lo = 0, hi = 0;
                if (colon != std::string::npos) {
                    lo = std::stoull(ex.substr(0, colon), nullptr, 16);
                    hi = std::stoull(ex.substr(colon + 1), nullptr, 16);
                } else {  // 静默豁免业务值: i128 解析单点化（a727e097）回退守卫——parseInt128InitText 拒收形态回退既有 stoull 原路径（=单点化前原样行为·E2E 锁面钉死·异常零占位由验证器防线承担）
                    // 1861（i128 常量文本解析族统一·parseInt128InitText 单点）
                    unsigned long long loP = 0, hiP = 0;
                    if (types::parseInt128InitText(ex, argType == "i128",
                                                   loP, hiP)) {
                        lo = static_cast<std::uint64_t>(loP);
                        hi = static_cast<std::uint64_t>(hiP);
                    } else {
                        try { lo = static_cast<std::uint64_t>(std::stoull(ex)); } catch (...) {}
                    }
                }
                // 常量：写临时区 [sp, #stackArgs*8+32]（返回缓冲区上方）
                const int tmpOff = static_cast<int>(stackArgs * 8 + 32 + place.num * 16);
                emitMovImm(writer, "x10", lo);
                writer.line("str x10, [sp, #" + std::to_string(tmpOff) + "]");
                emitMovImm(writer, "x10", hi);
                writer.line("str x10, [sp, #" + std::to_string(tmpOff + 8) + "]");
                addrReg = "x10";
                writer.line("add x10, sp, #" + std::to_string(tmpOff));
            } else {
                const int loId = av.id + 1;
                emitStackAddr(writer, "x10", regSlotOffset(loId));
            }
            writer.line("str x10, [sp, #" + std::to_string(memOff) + "]");
        } else if (isFloatType(argType)) {
            // 浮点栈参数：位模式经整型寄存器压栈
            loadOperandToV(writer, av, (argType == "f64") ? "d0" : "s0");
            writer.line("fmov x10, " + std::string((argType == "f64") ? "d0" : "s0"));
            writer.line("str x10, [sp, #" + std::to_string(memOff) + "]");
        } else {
            // 整型/指针栈参数：值压栈（32位值经 w 寄存器）
            // D8（525-a）：源直读写栈参（免「mov x10, x27」中转）；
            //   未分配 -> 装载 x10 原路径逐字节不变
            const std::string reg = operandSourceReg(writer, av, "x10");
            writer.line("str " + reg + ", [sp, #" + std::to_string(memOff) + "]");
        }
    }
    // 寄存器参数（GP xN / FP dN|sN·404 独立计数）
    for (std::size_t i = 0; i < argCount; ++i) {
        const Arm64ArgPlacement& place = placements[i];
        if (place.cls == 2) continue;
        const ir::IRValue& av = inst.operands[argBase + i];
        const std::string& argType = av.type;
        if (place.cls == 0 && (argType == "i128" || argType == "u128")) {
            // i128 参数：传双槽地址指针
            if (av.isConstant) {
                const std::string& ex = av.extra;
                const std::size_t colon = ex.find(':');
                std::uint64_t lo = 0, hi = 0;
                if (colon != std::string::npos) {
                    lo = std::stoull(ex.substr(0, colon), nullptr, 16);
                    hi = std::stoull(ex.substr(colon + 1), nullptr, 16);
                } else {  // 静默豁免业务值: i128 解析单点化（a727e097）回退守卫——parseInt128InitText 拒收形态回退既有 stoull 原路径（=单点化前原样行为·E2E 锁面钉死·异常零占位由验证器防线承担）
                    // 1861（i128 常量文本解析族统一·parseInt128InitText 单点）
                    unsigned long long loP = 0, hiP = 0;
                    if (types::parseInt128InitText(ex, argType == "i128",
                                                   loP, hiP)) {
                        lo = static_cast<std::uint64_t>(loP);
                        hi = static_cast<std::uint64_t>(hiP);
                    } else {
                        try { lo = static_cast<std::uint64_t>(std::stoull(ex)); } catch (...) {}
                    }
                }
                const int tmpOff = static_cast<int>(stackArgs * 8 + 32 + static_cast<int>(i) * 16);
                emitMovImm(writer, "x10", lo);
                writer.line("str x10, [sp, #" + std::to_string(tmpOff) + "]");
                emitMovImm(writer, "x10", hi);
                writer.line("str x10, [sp, #" + std::to_string(tmpOff + 8) + "]");
                writer.line("add x" + std::to_string(place.num) + ", sp, #" +
                            std::to_string(tmpOff));
            } else {
                const int loId = av.id + 1;
                emitStackAddr(writer, "x" + std::to_string(place.num),
                              regSlotOffset(loId));
            }
        } else if (place.cls == 1) {
            // 浮点参数：vN（FP 队列独立序号·404）
            const std::string vreg = (argType == "f64") ? "d" : "s";
            loadOperandToV(writer, av, vreg + std::to_string(place.num));
        } else {
            // 整型/指针参数：xN（GP 队列序号·404）
            const std::string reg = loadOperandToX(writer, av, "x" + std::to_string(place.num));
            (void)reg;
        }
    }
    // 间接调用：函数指针先入 x16（被调用者保存，bl 不会破坏 x16）
    if (isIndirect) {
        loadOperandToX(writer, inst.operands[0], "x16");
        // 040（001 §5.8 子案 A·2026-10-05 用户裁决）：空调用判零——零值 →
        //   运行时错误(3) 空指针解引用（093 判空家族同码同文案·GAS L 标签·
        //   错误码走 x0·错误块不返回与 FieldAddr 判空同款）
        const int fnptrNullId = ptrCheckCounter_++;
        const std::string fnptrOk = "Lfnptr_ok" + std::to_string(fnptrNullId);
        writer.line("cmp x16, #0");
        writer.line("b.ne " + fnptrOk);
        emitMovImm(writer, "x0", 3);
        writer.line("bl __cn_runtime_error");
        writer.line("ret");
        writer.raw(fnptrOk + ":");
        writer.line("blr x16");
    } else {
        writer.line("bl " + symbolName(callee));
    }
    // 恢复栈（与分配对称；大偏移需分段）
    if (totalAlloc > 0) {
        if (totalAlloc <= 4095) {
            writer.line("add sp, sp, #" + std::to_string(totalAlloc));
        } else {
            emitMovImm(writer, "x13", static_cast<std::uint64_t>(totalAlloc));
            writer.line("add sp, sp, x13");
        }
    }
    // 返回值 -> 结果槽（浮点 d0/s0，整型 x0）
    if (inst.result.id >= 0) {
        if (inst.result.type == "i128" || inst.result.type == "u128") {
            // i128 返回：缓冲区指针在 x0，读回双槽
            const int loId = inst.result.id + 1;
            writer.line("ldr x9, [x0]");
            storeVirtualResult(writer, loId, "x9", "i64");
            writer.line("ldr x9, [x0, #8]");
            storeVirtualResult(writer, inst.result.id, "x9", "i64");
        } else if (isFloatType(inst.result.type)) {
            storeVirtualResultFp(writer, inst.result.id, (inst.result.type == "f64") ? "d0" : "s0", inst.result.type);
        } else {
            storeVirtualResult(writer, inst.result.id, "x0", inst.result.type);
        }
    }
}

} // namespace cn_compiler
