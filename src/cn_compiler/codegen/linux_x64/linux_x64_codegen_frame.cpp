// Linux x64 代码生成·栈帧管理族：prologue（SysV 16 对齐）/参数装载/
//   epilogue/溢出陷阱与错误序列。
// #836 先拆再加：linux_x64_codegen.cpp 已入 900 行警戒线（912 行），按
//   「先拆再加」纪律将栈帧构建内聚职责拆出本文件——纯搬运零语义变化
//   （函数体逐字未动）。
#include <string>

#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"

namespace cn_compiler {

// 生成函数 prologue（push rbp / mov rbp,rsp / [push rbx+mov rbx,rdi] / sub rsp,alloc
//   / 1256：push 被调用者保存寄存器（rbx/r12~r15 中实际分配的））
// SysV 对齐纪律：入口 rsp≡8；push rbp 后 ≡0；再 push rbx 后 ≡8——
// needHiddenRet 时分配量 frameSize+8 补齐偶数个 8 字节，保证 call 前 rsp≡0
void LinuxX64CodeGenerator::emitPrologue(LinuxX64AsmWriter& writer,
                                         const ir::IRFunction& function) {
    const bool needHiddenRet = (function.structReturn ||
                                function.returnType == "i128" ||
                                function.returnType == "u128");
    if (needHiddenRet) {
        // 压栈顺序（rbx 先、rbp 后）使 rbp 紧邻返回地址：epilogue 用
        //   mov rsp,rbp / pop rbp / pop rbx / ret 标准链逐一还原
        //   （若 rbp 先压，rbx 落在 rbp 下方，mov rsp,rbp 后无法 pop 到 rbx）
        writer.line("push rbx");
        writer.line("push rbp");
        writer.line("mov rbp, rsp");
        writer.line("mov rbx, rdi");
        writer.comment("保存隐藏返回指针（入口 rdi -> rbx，SysV 第1整型参数位）");
    } else {
        writer.line("push rbp");
        writer.line("mov rbp, rsp");
    }
    // 帧内固定 16 字节返回缓冲区（帧底）：对齐 win x64——调用点 hasBigRet 时
    //   rdi 指向此区（rbp 相对持久），被调方写入后 rax=该地址存回 result 槽，
    //   调用方跨调用读 .值 不悬垂
    const int frameSize = computeFrameSize(function) + 16;
    retbufFrameOffset_ = -frameSize;
    currentFrameSize_ = frameSize;
    // 1256：被调用者保存寄存器（rbx/r12~r15）在 sub 之后压栈保存。
    //   SysV 对齐：push rbp 后 rsp≡0；sub F 后 ≡-F；再 push N 个 -> ≡(-F-8N)。
    //   F 为 16 的倍数，故 N 为奇数时 F 额外 +8，保证 call 前 rsp≡0。
    const int calleeSavedPad =
        (static_cast<int>(calleeSavedRegs_.size()) % 2 == 1) ? 8 : 0;
    const int totalAlloc = frameSize + calleeSavedPad + (needHiddenRet ? 8 : 0);
    if (totalAlloc > 0) {
        writer.line("sub rsp, " + std::to_string(totalAlloc));
    }
    emitSaveCalleeSaved(writer);
}

// 从栈槽内存操作数文本解析偏移（"[rbp-24]" -> -24；"[rbp+32]" -> 32；"[rbp]" -> 0）
int LinuxX64CodeGenerator::parseStackOffset(const std::string& mem) {
    if (mem.size() < 6 || mem.compare(0, 4, "[rbp") != 0) return 0;
    const bool negative = (mem[4] == '-');
    if (!negative && mem[4] != '+') return 0;
    try {
        const int value = std::stoi(mem.substr(5, mem.size() - 6));
        return negative ? -value : value;
    } catch (...) {
        return 0;
    }
}

// 生成函数参数装载（SysV 核心：整型/浮点独立计数）
// 整型类（含指针/i128地址/结构体按值指针）：rdi,rsi,rdx,rcx,r8,r9，第7起栈上
// 浮点类：xmm0~xmm7，第9起栈上（与整型栈参数共享栈参数序列）
// 隐藏返回指针（结构体/i128 返回）占 rdi（第1整型位），真实整型参数位号 +1；
// 浮点位号不受影响（SysV：MEMORY 类返回只占整型首寄存器）
void LinuxX64CodeGenerator::emitParamSetup(LinuxX64AsmWriter& writer,
                                           const ir::IRFunction& function) {
    const std::size_t paramOffset =
        (function.structReturn || function.returnType == "i128" ||
         function.returnType == "u128") ? 1 : 0;
    // 被调方栈参数锚定基：单一归属 stackParamAnchorBase()（needHiddenRet 时
    //   prologue 在 mov rbp,rsp 前 push rbx 保存隐藏返回指针，真实栈参数自
    //   [rbp+24] 起——旧恒 16 会把「返回地址」当第 7 参数读，E2E 154 实测；
    //   arm64 侧同构=stackParamBase()，2026-09-07 同族收敛）
    const int stackAnchor = stackParamAnchorBase();
    int intIdx = static_cast<int>(paramOffset);  // 整型参数位号（rdi=0 起）
    int floatIdx = 0;                            // 浮点参数位号（xmm0 起）
    int stackIdx = 0;                            // 栈参数序号（[rbp+锚+8k]）
    for (std::size_t i = 0; i < function.params.size(); ++i) {
        const std::string& unique = (i < function.paramUniques.size())
                                        ? function.paramUniques[i]
                                        : function.params[i].first;
        const int slotOffset = varSlotOf(unique);
        const std::string& paramType = function.params[i].second;
        // 结构体按值参数：传入指针 -> 参数槽（多槽拷贝，按值语义）——占整型位
        if (isStructParam(function, i) &&
            function.varSlots.count(unique) > 0 &&
            function.varSlots.at(unique) >= 1) {
            const int bytes = function.varSlots.at(unique) * 8;
            if (intIdx < 6) {
                writer.line("mov r10, " + intParameterRegister(intIdx));
            } else {
                writer.line("mov r10, qword ptr " + stackMemText(stackAnchor + stackIdx * 8));
                ++stackIdx;
            }
            ++intIdx;
            const int words = bytes / 8;
            for (int w = 0; w < words; ++w) {
                // 数据临时须用 r11：r9 是第6整型参数寄存器，若本结构体参数位号
                // 早于第6参数且函数整型参数满6个，用 r9 会把未保存的参数值覆盖成
                // 拷贝残留（v2p 生成如果 实测崩溃：指令引用被覆盖为池容量）
                writer.line("mov r11, qword ptr [r10+" + std::to_string(w * 8) + "]");
                writer.line("mov qword ptr " + stackMemText(slotOffset + w * 8) + ", r11");
            }
            writer.comment("结构体参数 " + function.params[i].first +
                           " 拷贝 " + std::to_string(bytes) + " 字节");
            continue;
        }
        // 浮点参数：xmm0~xmm7 独立编址；第9起从栈参数区读（位模式直读）
        if (isFloatType(paramType)) {
            if (floatIdx < 8) {
                emitStackStore(writer, slotOffset, "xmm" + std::to_string(floatIdx),
                               paramType);
            } else {
                writer.line("mov" + std::string(paramType == "f64" ? "sd" : "ss") +
                            " xmm0, qword ptr " + stackMemText(stackAnchor + stackIdx * 8));
                emitStackStore(writer, slotOffset, "xmm0", paramType);
                ++stackIdx;
            }
            ++floatIdx;
            continue;
        }
        if (intIdx < 6) {
            if (paramType == "i128" || paramType == "u128") {
                // i128 参数：双槽地址指针 -> 参数双槽拷贝 16 字节（数据临时用 r11，
                //   理由同结构体按值拷贝——r9 可能承载尚未保存的第6参数）
                const std::string srcReg = intParameterRegister(intIdx);
                writer.line("mov r10, " + srcReg);
                writer.line("mov r11, qword ptr [r10]");
                writer.line("mov qword ptr " + stackMemText(slotOffset) + ", r11");
                writer.line("mov r11, qword ptr [r10+8]");
                writer.line("mov qword ptr " + stackMemText(slotOffset + 8) + ", r11");
                writer.comment("i128 参数 " + function.params[i].first + " 拷贝 16 字节");
            } else {
                emitStackStore(writer, slotOffset, intParameterRegister(intIdx), paramType);
            }
        } else {
            // 第7整型参数位起：从调用者栈帧拷贝到本函数参数槽（锚含 saved rbx）
            writer.line("mov r10, qword ptr " + stackMemText(stackAnchor + stackIdx * 8));
            ++stackIdx;
            if (paramType == "i128" || paramType == "u128") {
                // i128 栈参数：槽内是双槽地址指针，拷 16 字节（数据临时用 r11，
                //   理由同结构体按值拷贝——r9 可能承载尚未保存的第6参数）
                writer.line("mov r11, qword ptr [r10]");
                writer.line("mov qword ptr " + stackMemText(slotOffset) + ", r11");
                writer.line("mov r11, qword ptr [r10+8]");
                writer.line("mov qword ptr " + stackMemText(slotOffset + 8) + ", r11");
                writer.comment("i128 栈参数 " + function.params[i].first + " 拷贝 16 字节");
            } else {
                emitStackStore(writer, slotOffset, "r10", paramType);
            }
        }
        ++intIdx;
        writer.comment("参数 " + function.params[i].first + " -> 槽偏移 " +
                       std::to_string(slotOffset));
    }
}

// 生成函数 epilogue（恢复栈帧并返回）
// 结构体/i128 返回：把源数据拷贝到隐藏返回缓冲区（rbx 保存的入口 rdi），
//   返回值 rax = 缓冲区指针（SysV）
// 浮点返回：movss/movsd xmm0；整型返回：mov rax
// 收尾：mov rsp,rbp（一步还原，与分配量/push 数无关）；[pop rbx]; pop rbp; ret
void LinuxX64CodeGenerator::emitEpilogue(LinuxX64AsmWriter& writer,
                                         const std::string& returnReg) {
    if (currentStructReturn_ && !returnReg.empty()) {
        const int copyBytes = (currentStructReturnSize_ > 0)
                                  ? currentStructReturnSize_ : 16;
        const int words = (copyBytes + 7) / 8;
        // 源地址：returnReg 为栈槽（[rbp-N]）时，槽内存的是结构体地址（指针），
        //   须 mov 装载指针值（不能用 lea 取槽地址——那会从槽地址处读错数据），
        //   与 ARM64 的 ldr 装载语义一致
        if (returnReg.compare(0, 1, "[") == 0) {
            const int off = parseStackOffset(returnReg);
            emitStackLoad(writer, off, "r10", "ptr", __LINE__);
        } else if (returnReg.size() > 2 && returnReg[0] == '%' && returnReg[1] == 'v') {
            const int id = std::stoi(returnReg.substr(2));
            emitStackLoad(writer, regSlotOffset(id), "r10", "ptr", __LINE__);
        } else {
            writer.line("mov r10, " + returnReg);
        }
        for (int w = 0; w < words; ++w) {
            writer.line("mov r9, qword ptr [r10+" + std::to_string(w * 8) + "]");
            writer.line("mov qword ptr [rbx+" + std::to_string(w * 8) + "], r9");
        }
        writer.comment("结构体返回：按 " + std::to_string(copyBytes) +
                       " 字节拷贝到隐藏返回缓冲区");
        writer.line("mov rax, rbx");  // SysV：返回缓冲区指针放 rax
        emitEpilogueTail(writer);
        return;
    }
    if (!returnReg.empty()) {
        if (currentReturnType_ == "i128" || currentReturnType_ == "u128") {
            // i128 返回：缓冲区 = rbx，源 = returnReg（高64位槽），
            //   低64位槽 = regSlotOffset(loId) = regSlotOffset(hiId+1) = hi槽 - 8
            const int hiOffset = (returnReg.compare(0, 1, "[") == 0)
                                     ? parseStackOffset(returnReg) : -8;
            writer.line("mov r9, qword ptr " + stackMemText(hiOffset - 8));
            writer.line("mov qword ptr [rbx], r9");
            writer.line("mov r9, qword ptr " + stackMemText(hiOffset));
            writer.line("mov qword ptr [rbx+8], r9");
            writer.line("mov rax, rbx");
            emitEpilogueTail(writer);
            return;
        }
        if (currentReturnType_ == "f64" || currentReturnType_ == "f32") {
            const std::string vreg = "xmm0";
            const std::string mnem = (currentReturnType_ == "f64") ? "movsd" : "movss";
            if (returnReg.compare(0, 1, "[") == 0) {
                const int off = parseStackOffset(returnReg);
                emitStackLoad(writer, off, vreg, currentReturnType_);
            } else if (returnReg.size() > 2 && returnReg[0] == '%' && returnReg[1] == 'v') {
                const int id = std::stoi(returnReg.substr(2));
                const std::string phys = physRegOf(ir::IRValue::reg(id, currentReturnType_));
                // #1212 d：返回值已分配到 xmm -> 寄存器直搬（原槽装载仅未分配面）
                if (isXmmName(phys)) {
                    writer.line(mnem + " " + vreg + ", " + phys);
                } else if (!isXmmName(phys)) {
                    emitStackLoad(writer, regSlotOffset(id), vreg, currentReturnType_);
                }
            } else if (isXmmName(returnReg)) {
                // #1212 d：terminator 直传的已分配浮点返回寄存器（xmmN）
                writer.line(mnem + " " + vreg + ", " + returnReg);
            } else {
                // 常量文本返回（如 "0"/"1"）：浮点常量池加载
                loadOperandToV(writer, ir::IRValue::constant(returnReg, currentReturnType_), vreg);
            }
        } else if (returnReg.compare(0, 1, "[") == 0) {
            const int off = parseStackOffset(returnReg);
            emitStackLoad(writer, off, "rax", currentReturnType_);
        } else if (returnReg.size() > 2 && returnReg[0] == '%' && returnReg[1] == 'v') {
            // 返回 %vN 文本：解析寄存器ID
            const int id = std::stoi(returnReg.substr(2));
            emitStackLoad(writer, regSlotOffset(id), "rax", currentReturnType_);
        } else if (x86Reg32(returnReg) != returnReg) {
            // 1256：返回值在已分配的物理寄存器内（直写），按类型宽度搬入 rax
            //   （窄型/32 位用 32 位名——写 eax 清高 32 位，与槽装载语义一致）
            writer.line("mov " + widthFor(currentReturnType_, "rax") + ", " +
                        widthFor(currentReturnType_, returnReg));
        } else {
            // 常量文本返回（如 "0"/"1"）：立即数装载
            try {
                const long long v = std::stoll(returnReg);
                writer.line("mov rax, " + std::to_string(v));
            } catch (...) {
                writer.line("mov rax, 0");
            }
        }
    }
    emitEpilogueTail(writer);
}

// epilogue 收尾（mov rsp,rbp 一步还原 / [pop rbx] / pop rbp / ret）
// 拆出供 struct/i128 返回分支复用（保持每函数<=100行约束）
void LinuxX64CodeGenerator::emitEpilogueTail(LinuxX64AsmWriter& writer) {
    // 1256：先恢复被调用者保存寄存器（push 的逆序 pop），再 mov rsp,rbp 一步还原。
    //   隐藏返回指针函数强制关闭分配（calleeSavedRegs_ 为空）——rbx 此时承载
    //   返回缓冲区指针，恢复动作不影响该路径。
    emitRestoreCalleeSaved(writer);
    // 弹出顺序与压栈相反：prologue 是 push rbp 先、push rbx 后（rbx 位于更低位），
    //   mov rsp,rbp 后栈顶依次为 saved rbp、saved rbx、返回地址——
    //   必须先 pop rbp 再 pop rbx（顺序颠倒会把返回地址弹进 rbp、ret 读垃圾）
    writer.line("mov rsp, rbp");
    writer.line("pop rbp");
    if (currentNeedHiddenRet_) writer.line("pop rbx");
    writer.line("ret");
}

// 生成单个函数：登记变量槽 -> 函数头 -> prologue -> 参数 -> 基本块 -> .size
// 任务 474：溢出错误出口（错误码 8『溢出错误』）——冷路径错误块发射。
//   与除零（错误码1）/判空（错误码3）同款出口序列：错误码入 rdi（SysV 首参）
//   + __cn_runtime_error + ret；热路径以几乎不跳转的条件分支（jo/jc）跳入。
void LinuxX64CodeGenerator::emitOverflowTrap(LinuxX64AsmWriter& writer,
                                             const std::string& label) {
    writer.raw(label + ":");
    emitOverflowErrorBody(writer);
}

// 溢出错误序列（无标签）：错误码 8 入 rdi（SysV 首参）+ 运行时报告 + 终止
void LinuxX64CodeGenerator::emitOverflowErrorBody(LinuxX64AsmWriter& writer) {
    writer.line("mov rdi, 8");
    writer.line("call __cn_runtime_error");
    writer.line("ret");
}

} // namespace cn_compiler
