// Linux x86_64 代码生成器——寄存器类消费设施（#1212 d：i128 双半 + 浮点 SSE）
// 职责（单一归属：linux_x64_instructions.cpp 已入 900 行警戒线，先拆再加）：
//   1. i128/u128 双 GPR 成对分配消费：hasI128Pair / 高半低半文本 / 成对
//      溢出与载回 / 半值装载与落位（禁单半捷径——任何 i128 值必须双半同进同出）
//   2. 浮点 xmm 寄存器类消费：xmm 名判定 / 浮点结果直写 / SysV 调用点活跃
//      浮点值保护（xmm0~xmm15 全为调用者保存，无被调用者保存浮点寄存器）
// 规范：英文API命名，中文仅注释；函数<=100行
#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"

namespace cn_compiler {

// ==================== #1212 d：i128 双半成对分配设施 ====================

// 该虚拟寄存器是否成对分配到两个 GPR（i128/u128；高半=%vN、低半=%vN+1）
//   ——win-x64 步骤 b 的 LinuxX64 镜像实现（禁单半捷径）。
bool LinuxX64CodeGenerator::hasI128Pair(int regId) const {
    const cn::Optional<const regalloc::RegAssignment*> ra =
        (regId >= 0) ? regAllocOf(regId)
                     : cn::Optional<const regalloc::RegAssignment*>(std::nullopt);
    return ra.has_value() && ra.value() != nullptr &&
           !ra.value()->assignedReg.empty() && !ra.value()->assignedReg2.empty();
}

// i128 高半文本（寄存器或原栈槽）。仅对 id>=0 的虚拟寄存器调用。
std::string LinuxX64CodeGenerator::i128HiText(const ir::IRValue& v) const {
    if (hasI128Pair(v.id)) return regAllocOf(v.id).value()->assignedReg;
    return regSlotMem(v.id);
}

// i128 低半文本（寄存器或原栈槽）
std::string LinuxX64CodeGenerator::i128LoText(const ir::IRValue& v) const {
    if (hasI128Pair(v.id)) return regAllocOf(v.id).value()->assignedReg2;
    return regSlotMem(v.id + 1);
}

// 成对分配双半溢出到原双槽（高半 -> regSlot(id)，低半 -> regSlot(id+1)）
//   内存实现（乘除/比较/浮转整128 辅助）以双槽地址为 ABI 契约——调用前必须
//   把寄存器值落到槽，调用后由 reloadI128Pair 载回（成对、禁单半）。
void LinuxX64CodeGenerator::spillI128Pair(LinuxX64AsmWriter& writer,
                                          const ir::IRValue& v) {
    if (!hasI128Pair(v.id)) return;
    const regalloc::RegAssignment* ra = regAllocOf(v.id).value();
    writer.line("mov " + regSlotMem(v.id) + ", " + ra->assignedReg);       // 高
    writer.line("mov " + regSlotMem(v.id + 1) + ", " + ra->assignedReg2);  // 低
}

// 原双槽载回成对分配的双半（内存实现后收尾）
void LinuxX64CodeGenerator::reloadI128Pair(LinuxX64AsmWriter& writer,
                                           const ir::IRValue& result) {
    if (!hasI128Pair(result.id)) return;
    const regalloc::RegAssignment* ra = regAllocOf(result.id).value();
    writer.line("mov " + ra->assignedReg2 + ", " + regSlotMem(result.id + 1));  // 低
    writer.line("mov " + ra->assignedReg + ", " + regSlotMem(result.id));       // 高
}

// 半值装载：成对分配 -> mov reg, 半寄存器（读值走寄存器类）；
//   未成对 -> 原双槽的单半装载（产物逐字节不变）。
void LinuxX64CodeGenerator::loadI128Half(LinuxX64AsmWriter& writer,
                                         const ir::IRValue& v, bool isHi,
                                         const std::string& reg) {
    if (!v.isConstant && hasI128Pair(v.id)) {
        writer.line("mov " + reg + ", " + (isHi ? i128HiText(v) : i128LoText(v)));
        return;
    }
    const int id = isHi ? v.id : v.id + 1;
    emitStackLoad(writer, regSlotOffset(id), reg, "i64", __LINE__);
}

// 半值落位：成对分配 -> mov 半寄存器, src；未成对 -> 原双槽存储
void LinuxX64CodeGenerator::storeI128Half(LinuxX64AsmWriter& writer,
                                          const ir::IRValue& result, bool isHi,
                                          const std::string& src) {
    if (hasI128Pair(result.id)) {
        const std::string dst = isHi ? i128HiText(result) : i128LoText(result);
        if (dst != src) writer.line("mov " + dst + ", " + src);
        return;
    }
    emitStackStore(writer, regSlotOffset(isHi ? result.id : result.id + 1),
                   src, "i64");
}

// ==================== #1212 d：浮点 SSE 寄存器类设施 ====================

// 文本是否为 xmm 寄存器名（xmm0~xmm15 闭集）
bool LinuxX64CodeGenerator::isXmmName(const std::string& text) {
    if (text.size() < 4 || text.size() > 6) return false;
    if (text.compare(0, 3, "xmm") != 0) return false;
    int num = 0;
    for (std::size_t i = 3; i < text.size(); ++i) {
        if (text[i] < '0' || text[i] > '9') return false;
        num = num * 10 + (text[i] - '0');
    }
    return num >= 0 && num <= 15;  // SSE 寄存器闭集 xmm0~xmm15
}

// 浮点结果落位：目标已分配到 xmm -> 寄存器直写（单条 movss/movsd）；
//   未分配 -> 原槽写（产物逐字节不变）
void LinuxX64CodeGenerator::storeFloatResult(LinuxX64AsmWriter& writer,
                                             const ir::IRValue& result,
                                             const std::string& srcXmm,
                                             const std::string& type) {
    const cn::Optional<std::string> physOpt = physRegOf(result);
    const std::string mnem = (type == "f64") ? "movsd" : "movss";
    if (physOpt.has_value()) {
        writer.line(mnem + " " + physOpt.value() + ", " + srcXmm);
        return;
    }
    emitStackStore(writer, regSlotOffset(result.id), srcXmm, type);
}

// 调用点浮点活跃值保护（SysV：xmm0~xmm15 全为调用者保存，无被调用者保存浮点
//   寄存器）。call 前把所有在调用点活跃的已分配 xmm 值保存到各自虚拟寄存器槽
//   （分配后该槽为弃用槽，但地址唯一且帧内持久），call 后逆序恢复。
//   —— 与 LLVM/Rust 的 call-clobber spill 同策：只保护活跃集，不是全池盲存。
//   保存必须在浮点实参装载之前（xmm0~xmm7 参数位含池内 xmm6），恢复在 call
//   返回后、结果写回之前（池不含 xmm0，不覆盖返回值）。
void LinuxX64CodeGenerator::saveLiveFpForCall(LinuxX64AsmWriter& writer) {
    if (currentInstPoint_ == -1 || regIntervals_.empty()) return;
    for (const auto& li : regIntervals_) {
        if (!li.isFloat) continue;
        if (li.start > currentInstPoint_ || currentInstPoint_ > li.end) continue;
        auto it = regAllocMap_.find(li.regId);
        if (it == regAllocMap_.end()) continue;
        if (!isXmmName(it->second.assignedReg)) continue;
        writer.line("movsd " + regSlotMem(li.regId) + ", " + it->second.assignedReg);
    }
}

void LinuxX64CodeGenerator::restoreLiveFpAfterCall(LinuxX64AsmWriter& writer) {
    if (currentInstPoint_ == -1 || regIntervals_.empty()) return;
    for (auto it = regIntervals_.rbegin(); it != regIntervals_.rend(); ++it) {
        const auto& li = *it;
        if (!li.isFloat) continue;
        if (li.start > currentInstPoint_ || currentInstPoint_ > li.end) continue;
        auto ra = regAllocMap_.find(li.regId);
        if (ra == regAllocMap_.end()) continue;
        if (!isXmmName(ra->second.assignedReg)) continue;
        writer.line("movsd " + ra->second.assignedReg + ", " + regSlotMem(li.regId));
    }
}

} // namespace cn_compiler
