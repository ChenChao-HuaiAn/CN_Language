// ARM64 代码生成·栈帧管理族：prologue（x29/x30 与可选 x19 压栈）/参数装载/
//   epilogue/溢出陷阱与错误序列。
// #836 先拆再加：arm64_codegen.cpp 已入 900 行警戒线（982 行），按「先拆再加」
//   纪律将栈帧构建内聚职责拆出本文件——纯搬运零语义变化（函数体逐字未动）。
#include <cstdint>
#include <string>
#include <vector>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"

namespace cn_compiler {

// 生成函数 prologue（stp x29,x30 / 可选 stp x19,xzr / mov x29,sp / sub sp,#frameSize）
// AAPCS64：
//   - x29 帧指针、x30 链接寄存器，均被调用者保存
//   - 需要保存隐藏返回指针（结构体/i128 返回）时用 x19（被调用者保存），
//     在 x29/x30 之后压 stp x19, xzr（xzr 占位保持 16 对齐），恢复时 ldp 丢弃
//   - 压栈顺序固定：先 x29,x30 再 x19（栈参数偏移 = stackParamBase() + 8k，
//     与 parameterRegister 一致）
// 栈调整辅助：|amount| <= 4095 用单条 sub/add；否则先 mov 到 x13 再 sub/add
// AArch64 立即数栈调整最大 4095（12 位），大栈帧（如 4224 字节）需分段
void Arm64CodeGenerator::emitStackAdjust(Arm64AsmWriter& writer, int amount) {
    if (amount == 0) return;
    if (amount > 0 && amount <= 4095) {
        writer.line("add sp, sp, #" + std::to_string(amount));
    } else if (amount < 0 && amount >= -4095) {
        writer.line("sub sp, sp, #" + std::to_string(-amount));
    } else {
        // 大偏移：movz/movk 装载到 x13 再 sub/add（x13 为地址计算临时寄存器）
        emitMovImm(writer, "x13", static_cast<std::uint64_t>(amount < 0 ? -amount : amount));
        if (amount < 0) {
            writer.line("sub sp, sp, x13");
        } else {
            writer.line("add sp, sp, x13");
        }
    }
}

void Arm64CodeGenerator::emitPrologue(Arm64AsmWriter& writer,
                                      const ir::IRFunction& function) {
    const bool needHiddenRet = (function.structReturn ||
                                function.returnType == "i128" ||
                                function.returnType == "u128");
    writer.line("stp x29, x30, [sp, #-16]!");
    if (needHiddenRet) {
        writer.line("stp x19, xzr, [sp, #-16]!");
        writer.line("mov x19, x0");
        writer.comment("保存隐藏返回指针（入口 x0 -> x19）");
    }
    // F1-28：被调用者保存寄存器（寄存器分配占用）成对压栈
    //   必须在 mov x29, sp 之前——栈参数基址（stackParamBase）随之补偿压栈对数
    for (int p = 0; p + 1 < static_cast<int>(calleeSavedRegs_.size()); p += 2) {
        writer.line("stp " + calleeSavedRegs_[p] + ", " + calleeSavedRegs_[p + 1] +
                    ", [sp, #-16]!");
    }
    if (static_cast<int>(calleeSavedRegs_.size()) % 2 != 0) {
        writer.line("stp " + calleeSavedRegs_.back() + ", xzr, [sp, #-16]!");
    }
    writer.line("mov x29, sp");
    const int frameSize = computeFrameSize(function);
    currentFrameSize_ = frameSize;
    if (frameSize > 0) {
        emitStackAdjust(writer, -frameSize);
    }
    // #1212：浮点被调用者保存寄存器存入帧底保存区（str dN，低 64 位=全部
    //   f32/f64 有效位；[x29] 为 sub sp 前的栈顶，偏移相对 x29 恒定）
    for (std::size_t i = 0; i < calleeSavedFpRegs_.size(); ++i) {
        const int off = -frameSize + 16 * static_cast<int>(i);
        const std::string mem = stackMemText(off, writer);
        writer.line("str " + calleeSavedFpRegs_[i] + ", " + mem);
    }
}

// 尾声恢复被调用者保存寄存器（逆序弹出，与序言压栈相反）
void Arm64CodeGenerator::emitRestoreCalleeSaved(Arm64AsmWriter& writer) {
    const int count = static_cast<int>(calleeSavedRegs_.size());
    int p = count - 1;
    if (count % 2 != 0) {
        writer.line("ldp " + calleeSavedRegs_[count - 1] + ", xzr, [sp], #16");
        p = count - 2;
    }
    for (; p >= 1; p -= 2) {
        writer.line("ldp " + calleeSavedRegs_[p - 1] + ", " + calleeSavedRegs_[p] +
                    ", [sp], #16");
    }
}

// 从栈槽内存操作数文本解析偏移（"[x29,#-N]" -> -N；"[x13]" -> 0）
int Arm64CodeGenerator::parseStackOffset(const std::string& mem) {
    if (mem.compare(0, 6, "[x29,#") == 0) {
        return std::stoi(mem.substr(6, mem.size() - 7));
    }
    return 0;  // [x13] 或其他
}

// 生成函数参数装载：前8寄存器 x0~x7 / v0~v7 存入参数槽，第9起从栈读
// AAPCS64：
//   - 整型/指针参数按位 x0~x7；浮点参数按位 v0~v7（第 N 个参数用 xN 或 vN）
//   - 隐藏返回指针（结构体/i128 返回）占 x0，真实参数位号 = i + 1（paramOffset=1）
//   - 栈参数（第9起）位于 [x29, #base + (i-8)*8]（base=16/32 视 x19 而定）
//   - i128 参数以"双槽地址指针"传入（调用方传 16 字节缓冲地址），拷贝 16 字节
//   - 结构体按值参数以指针传入，拷贝到参数槽（按值语义）
void Arm64CodeGenerator::emitParamSetup(Arm64AsmWriter& writer,
                                        const ir::IRFunction& function) {
    const std::size_t paramOffset =
        (function.structReturn || function.returnType == "i128" ||
         function.returnType == "u128") ? 1 : 0;
    // 404：GP/FP 独立计数落位（与 emitCall/emitVirtualCall 同源单点·AAPCS64 双队列）
    std::vector<std::string> paramTypes;
    paramTypes.reserve(function.params.size());
    for (const auto& p : function.params) paramTypes.push_back(p.second);
    const std::vector<Arm64ArgPlacement> placements =
        arm64AssignParamPlacements(paramTypes, static_cast<int>(paramOffset));
    for (std::size_t i = 0; i < function.params.size(); ++i) {
        const std::string& unique = (i < function.paramUniques.size())
                                        ? function.paramUniques[i]
                                        : function.params[i].first;
        const int slotOffset = varSlotOf(unique);
        const std::string& paramType = function.params[i].second;
        const Arm64ArgPlacement& place = placements[i];
        // 结构体按值参数：传入指针 -> 参数槽（多槽拷贝，按值语义）
        if (isStructParam(function, i) &&
            function.varSlots.count(unique) > 0 &&
            function.varSlots.at(unique) >= 1) {
            const int bytes = function.varSlots.at(unique) * 8;
            std::string srcReg;
            if (place.cls == 0) {
                srcReg = parameterRegister(place.num);
            } else {
                // 栈上传入指针：按栈槽序读取（404 独立计数·非位号公式）
                srcReg = stackMemText(stackParamBase() + place.num * 8, writer);
                writer.line("ldr x10, " + srcReg);
                srcReg = "x10";
            }
            // D8（525-a）：寄存器参数位（x2~x7）直读作拷贝基址——免
            //   「mov x10, x2」中转（prologue 无调用介入，参数寄存器稳定）。
            //   D8 596 修复：拷贝循环源基址须用 srcReg——原实现删了
            //   「mov x10, srcReg」中转却仍硬编码 [x10]，寄存器参数时
            //   x10 未初始化=垃圾基址（v2p 启动即段错误实锤）
            emitStackAddr(writer, "x12", slotOffset);
            const int words = bytes / 8;
            for (int w = 0; w < words; ++w) {
                writer.line("ldr x11, [" + srcReg + ", #" + std::to_string(w * 8) + "]");
                writer.line("str x11, [x12, #" + std::to_string(w * 8) + "]");
            }
            writer.comment("结构体参数 " + function.params[i].first +
                           " 拷贝 " + std::to_string(bytes) + " 字节");
            continue;
        }
        // 浮点参数：sN/dN（FP 队列独立序号·404；f32 用 s、f64 用 d）
        if (isFloatType(paramType)) {
            if (place.cls == 1) {
                const std::string vreg = (paramType == "f64") ? "d" : "s";
                emitStackStore(writer, slotOffset, vreg + std::to_string(place.num), paramType);
            } else {
                // 栈上浮点位模式：按栈槽序读取（404 独立计数·非位号公式）
                const std::string mem = stackMemText(
                    stackParamBase() + place.num * 8, writer);
                writer.line("ldr x10, " + mem);
                emitStackStore(writer, slotOffset, "x10", "i64");
            }
            continue;
        }
        if (place.cls == 0) {
            // GP 寄存器整型/指针参数：寄存器 -> 栈槽
            if (paramType == "i128" || paramType == "u128") {
                // i128 参数：双槽地址指针 -> 参数双槽拷贝 16 字节
                const std::string srcReg = parameterRegister(place.num);
                emitStackAddr(writer, "x12", slotOffset);
                writer.line("ldr x10, [" + srcReg + "]");
                writer.line("str x10, [x12]");
                writer.line("ldr x10, [" + srcReg + ", #8]");
                writer.line("str x10, [x12, #8]");
                writer.comment("i128 参数 " + function.params[i].first +
                               " 拷贝 16 字节");
            } else {
                const std::string reg = parameterRegister(place.num);
                emitStackStore(writer, slotOffset, reg, paramType);
            }
        } else {
            // 栈参数（寄存器池溢出·槽序=参数序）：从调用者栈帧拷贝到本函数参数槽
            //   （锚定基单一归属 stackParamBase——隐藏返回时 prologue 多压 x19，
            //   真实栈参上移 16）
            const std::string stackSrc = stackMemText(
                stackParamBase() + place.num * 8, writer);
            if (paramType == "i128" || paramType == "u128") {
                writer.line("ldr x10, " + stackSrc);  // i128 双槽地址指针
                emitStackAddr(writer, "x12", slotOffset);
                writer.line("ldr x11, [x10]");
                writer.line("str x11, [x12]");
                writer.line("ldr x11, [x10, #8]");
                writer.line("str x11, [x12, #8]");
                writer.comment("i128 栈参数 " + function.params[i].first +
                               " 拷贝 16 字节");
            } else {
                writer.line("ldr x10, " + stackSrc);
                emitStackStore(writer, slotOffset, "x10", paramType);
            }
        }
        writer.comment("参数 " + function.params[i].first + " -> 槽偏移 " +
                       std::to_string(slotOffset));
    }
}

// 生成函数 epilogue（恢复栈帧并返回）
// 结构体/i128 返回：把 returnReg 指向的数据拷贝到隐藏返回缓冲区（x19 保存的入口 x0），
//   返回值 = 缓冲区指针（x0）
// 浮点返回：fmov d0/s0（经栈槽装载）；整型返回：mov x0
// 恢复：add sp,#frameSize; ldp x29,x30,[sp],#16; [ldp x19,xzr,[sp],#16]; ret
void Arm64CodeGenerator::emitEpilogue(Arm64AsmWriter& writer,
                                  const std::string& returnReg) {
if (currentStructReturn_ && !returnReg.empty()) {
        writer.line("mov x0, x19");
        const int copyBytes = (currentStructReturnSize_ > 0)
                                  ? currentStructReturnSize_ : 16;
        const int words = (copyBytes + 7) / 8;
        // 源地址：returnReg 为栈槽（[x29,#-N]）时，槽内存的是结构体地址（指针），
        //   须 ldr 装载指针值（不能用 add 取槽地址——那会从槽地址处读错数据），
        //   与 X64 的 "mov rsi, returnReg"（装载值）语义一致
        if (returnReg.compare(0, 6, "[x29,#") == 0) {
            const int off = parseStackOffset(returnReg);
            // 装载槽值（结构体地址指针）：大偏移（|off|>255）经 stackMemText
            //   生成 x13 间接寻址，避免 ldr [x29,#-N] 负偏移非法
            emitStackLoad(writer, off, "x11", "ptr");
        } else {
            writer.line("mov x11, " + returnReg);
        }
        for (int w = 0; w < words; ++w) {
            writer.line("ldr x10, [x11, #" + std::to_string(w * 8) + "]");
            writer.line("str x10, [x0, #" + std::to_string(w * 8) + "]");
        }
        writer.comment("结构体返回：按 " + std::to_string(copyBytes) +
                       " 字节拷贝到隐藏返回缓冲区");
        writer.line("mov x0, x19");  // ABI：返回缓冲区指针放 x0
        emitStackAdjust(writer, currentFrameSize_);
        // 恢复顺序与压栈相反：x19 后压（栈顶），先弹 x19 再弹 x29/x30
        // （prologue: stp x29,x30 先、stp x19,xzr 后；mov x29,sp 在 x19 压栈后）
        emitRestoreCalleeSaved(writer);
        if (currentNeedHiddenRet_) writer.line("ldp x19, xzr, [sp], #16");
        writer.line("ldp x29, x30, [sp], #16");
        writer.line("ret");
        return;
    }
    if (!returnReg.empty()) {
        if (currentReturnType_ == "i128" || currentReturnType_ == "u128") {
            // i128 返回：缓冲区 = x19，源 = returnReg（高64位槽），
            //   低64位槽 = regSlotOffset(loId) = regSlotOffset(hiId+1) = hi槽 - 8
            //   （regSlotOffset(id) = -8*id-8，loId=hiId+1 -> lo槽 = hi槽 - 8）
            const int hiOffset = (returnReg.compare(0, 6, "[x29,#") == 0)
                                     ? parseStackOffset(returnReg) : -8;
            writer.line("mov x0, x19");
            const std::string loMem = stackMemText(hiOffset - 8, writer);
            writer.line("ldr x10, " + loMem);
            writer.line("str x10, [x0]");
            const std::string hiMem = stackMemText(hiOffset, writer);
            writer.line("ldr x10, " + hiMem);
            writer.line("str x10, [x0, #8]");
            writer.line("mov x0, x19");
            emitStackAdjust(writer, currentFrameSize_);
            emitRestoreCalleeSaved(writer);
            if (currentNeedHiddenRet_) writer.line("ldp x19, xzr, [sp], #16");
            writer.line("ldp x29, x30, [sp], #16");
            writer.line("ret");
            return;
        }
        if (currentReturnType_ == "f64" || currentReturnType_ == "f32") {
            const std::string vreg = (currentReturnType_ == "f64") ? "d0" : "s0";
            const bool fpPhys = returnReg.size() >= 2 &&
                (returnReg[0] == 'd' || returnReg[0] == 's') &&
                returnReg.substr(1).find_first_not_of("0123456789") == std::string::npos;
            if (fpPhys) {
                // #1212：返回值为已分配浮点寄存器（sN/dN 宽度视图）
                writer.line("fmov " + vreg + ", " + returnReg);
            } else if (isPhysRegName(returnReg)) {
                // F1-28：返回值为已分配虚拟寄存器（整型寄存器驻留的浮点位模式）
                writer.line("fmov " + vreg + ", " + returnReg);
            } else if (returnReg.compare(0, 6, "[x29,#") == 0) {
                const int off = parseStackOffset(returnReg);
                emitStackLoad(writer, off, vreg, currentReturnType_);
            } else if (returnReg.size() > 2 && returnReg[0] == '%' && returnReg[1] == 'v') {
                const int id = std::stoi(returnReg.substr(2));
                emitStackLoad(writer, regSlotOffset(id), vreg, currentReturnType_);
            } else {
                // 常量文本返回（如 "0"/"1"）：浮点常量池加载
                loadOperandToV(writer, ir::IRValue::constant(returnReg, currentReturnType_), vreg);
            }
        } else if (isPhysRegName(returnReg)) {
            // F1-28：返回值为已分配虚拟寄存器（物理寄存器直接入 x0）
            if (returnReg != "x0") writer.line("mov x0, " + returnReg);
        } else if (returnReg.compare(0, 6, "[x29,#") == 0) {
            const int off = parseStackOffset(returnReg);
            emitStackLoad(writer, off, "x0", currentReturnType_);
        } else if (returnReg.size() > 2 && returnReg[0] == '%' && returnReg[1] == 'v') {
            // 返回 %vN 文本：解析寄存器ID
            const int id = std::stoi(returnReg.substr(2));
            emitStackLoad(writer, regSlotOffset(id), "x0", currentReturnType_);
        } else {
            // 常量文本返回（如 "0"/"1"）：立即数装载
            emitMovImm(writer, "x0", static_cast<std::uint64_t>(std::stoll(returnReg)));
        }
    }
    // #1212：浮点被调用者保存寄存器恢复（帧底保存区，先于栈回收）
    for (std::size_t i = 0; i < calleeSavedFpRegs_.size(); ++i) {
        const int off = -currentFrameSize_ + 16 * static_cast<int>(i);
        const std::string mem = stackMemText(off, writer);
        writer.line("ldr " + calleeSavedFpRegs_[i] + ", " + mem);
    }
    emitStackAdjust(writer, currentFrameSize_);
    // 恢复顺序与压栈相反（x19 后压先弹）
    emitRestoreCalleeSaved(writer);
    if (currentNeedHiddenRet_) writer.line("ldp x19, xzr, [sp], #16");
    writer.line("ldp x29, x30, [sp], #16");
    writer.line("ret");
}

// 任务 474：溢出错误出口（错误码 8『溢出错误』）——冷路径错误块发射。
//   与除零（错误码1）/判空（错误码3）同款出口序列：错误码入 x0（AAPCS64
//   首参）+ __cn_runtime_error + ret；热路径以几乎不跳转的条件分支
//   （b.vs/b.cs/b.gt 等）跳入。
void Arm64CodeGenerator::emitOverflowTrap(Arm64AsmWriter& writer,
                                          const std::string& label) {
    writer.raw(label + ":");
    emitOverflowErrorBody(writer);
}

// 溢出错误序列（无标签）：错误码 8 入 x0（AAPCS64 首参）+ 运行时报告 + 终止
void Arm64CodeGenerator::emitOverflowErrorBody(Arm64AsmWriter& writer) {
    emitMovImm(writer, "x0", 8);
    writer.line("bl __cn_runtime_error");
    writer.line("ret");
}

} // namespace cn_compiler
