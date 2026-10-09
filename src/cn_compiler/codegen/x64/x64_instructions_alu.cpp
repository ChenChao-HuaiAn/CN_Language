
// CN Win x64 代码生成器——指令级降级（358：自 x64_instructions.cpp 按族拆出·D1 cast/mem/i128 拆分先例）
//   族 = 浮点二元运算（emitFloatBinary SSE）+ 移位运算（emitShift）；
//   纯重构：函数体自原文件逐字节搬移（成员声明仍在 x64_codegen.hpp）。
#include <cstdint>
#include <string>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"

namespace cn_compiler {
// ==================== 浮点运算（SSE，Task 2.3） ====================

// 浮点二元运算（SSE）：dst = op1 op op2
// f64 -> addsd/subsd/mulsd/divsd；f32 -> addss/subss/mulss/divss
// 浮点值存于8字节槽（movsd/movss 从槽加载到 xmm），结果写回槽
void X64CodeGenerator::emitFloatBinary(AsmWriter& writer, const ir::IRInstruction& inst,
                                       const std::string& mnemonic) {
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    const bool isDouble = (inst.type == "f64");
    // 后缀：sd（双精度）/ ss（单精度）
    const std::string suf = isDouble ? "sd" : "ss";
    const std::string load = isDouble ? "movsd" : "movss";
    const std::string mp = isDouble ? "qword ptr " : "dword ptr ";
    // xmm0 = op1；xmm1 = op2；运算；xmm0 -> dst（内存操作数均需显式大小前缀）
    writer.line(load + " xmm0, " + mp + op1);
    writer.line(load + " xmm1, " + mp + op2);
    writer.line(mnemonic + suf + " xmm0, xmm1");
    writer.line(load + " " + mp + dst + ", xmm0");
}

// 移位量常量文本解析（0x/0b/0o 前缀感知——std::stoi 对 "0x10" 返回 0 的潜伏
// 缺陷修复，2026-09-09；loadOperandToX 前缀转换同构）。异常兜底 0。
static int shiftAmtOf(const std::string& text) {
    if (text.size() > 2 && text[0] == '0' &&
        (text[1] == 'x' || text[1] == 'X' || text[1] == 'b' || text[1] == 'B' ||
         text[1] == 'o' || text[1] == 'O')) {
        const int base = (text[1] == 'x' || text[1] == 'X') ? 16
                         : (text[1] == 'b' || text[1] == 'B') ? 2 : 8;
        try { return static_cast<int>(std::stoull(text.substr(2), nullptr, base)); }
        catch (...) { return 0; }
    }
    try { return static_cast<int>(std::stoll(text)); }
    catch (...) { return 0; }
}

// ==================== 移位（Task 2.3） ====================

// 移位运算：dst = op1 << op2 / op1 >> op2
// shl（左移）/ sar（有符号算术右移）/ shr（无符号逻辑右移，u8~u128 用）
// 移位量必须是 cl（x86 规定）或立即数
void X64CodeGenerator::emitShift(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    const std::string& srcType = inst.operands[0].type;
    // 无符号类型 -> shr（逻辑右移）；有符号 -> sar（算术右移）
    const bool isUnsigned = (srcType == "u8" || srcType == "u16" ||
                             srcType == "u32" || srcType == "u64" ||
                             srcType == "u128");
    const std::string sh = (inst.opcode == ir::Opcode::Shl) ? "shl"
                           : (isUnsigned ? "shr" : "sar");
    // 8/16位：扩展后按32位运算（与 emitIntBinary 一致）
    // 移位量语义（2026-09-09 规范化，Rust release 掩码同构，与 const_fold 一致）：
    //   按操作数类型位宽取模。8/16 位经 32 位寄存器运算，硬件按 32 取模——须显
    //   式 and 掩码到类型位宽（否则 mod 32 与折叠 mod 8/16 分叉）；32/64 位路径
    //   硬件按操作数宽度取模恰为定义语义（常量 imm8 亦按宽度取模），零改动。
    if (srcType == "i8" || srcType == "i16" || srcType == "u8" || srcType == "u16") {
        const std::string ext = (srcType == "i8" || srcType == "i16") ? "movsx" : "movzx";
        const std::string mp = memSizePtr(srcType);
        const int shiftMask = (srcType == "i16" || srcType == "u16") ? 15 : 7;
        // D8（458-a）：dst 已分配时计算寄存器=dst 的 32 位形态（cl 固定约束不冲突）
        const std::string dstPhys = physRegOf(inst.result);
        std::string ew = "eax";
        if (!dstPhys.empty()) ew = widthFor("i32", dstPhys);
        writer.line(ext + " " + ew + ", " + mp + op1);
        // 移位量：常量 -> 立即数；否则 -> cl
        if (inst.operands[1].isConstant) {
            const int shiftAmt = shiftAmtOf(inst.operands[1].extra) & shiftMask;
            writer.line(sh + " " + ew + ", " + std::to_string(shiftAmt));
        } else {
            // 移位量须装载到 cl（rcx 低8位）：物理寄存器（寄存器分配）用 32 位名
            //   （mov ecx, r14 尺寸不匹配 A2022；mov ecx, r14d 写低32位值语义一致）
            writer.line("mov ecx, " + widthFor("i32", op2));
            writer.line("and ecx, " + std::to_string(shiftMask));
            writer.line(sh + " " + ew + ", cl");
        }
        if (dstPhys.empty()) {
            writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
        }
        return;
    }
    // 32/64位
    // D8（458-a）：dst 已分配且移位量寄存器（cl）不冲突 -> 计算落 dst 免尾部中转
    //   （cl=rcx 低 8 位·rcx 非分配池寄存器·与 dst 无冲突面）
    const std::string dstPhys64 = physRegOf(inst.result);
    std::string w;
    const bool direct64 = !dstPhys64.empty();
    if (direct64) {
        w = widthFor(srcType, dstPhys64);
    } else {
        w = widthFor(srcType, "rax");
    }
    writer.line("mov " + w + ", " + shrunkOperand(srcType, op1));  // 物理寄存器全名收缩（A2022）
    if (inst.operands[1].isConstant) {
        // 246-a（D20 根治）：常量移位量按操作数位宽取模后发射——
        //   负/超域立即数原文直发 A2070（`shl eax, -997`·CN-Smith 首采 109 例）。
        //   对齐第四十三轮 arm64 emitShift 常量掩码修复（win 侧余债）。
        const int width = (srcType == "i64" || srcType == "u64") ? 64 : 32;
        const int shiftAmt = shiftAmtOf(inst.operands[1].extra) & (width - 1);
        writer.line(sh + " " + w + ", " + std::to_string(shiftAmt));
    } else {
        // 移位量须装载到 cl（rcx 低8位）：物理寄存器（寄存器分配）用 32 位名
        //   （mov ecx, r14 尺寸不匹配 A2022；mov ecx, r14d 写低32位值语义一致）
        writer.line("mov ecx, " + widthFor("i32", op2));
        writer.line(sh + " " + w + ", cl");
    }
    if (dstPhys64.empty()) {
        writer.line("mov " + shrunkOperand(srcType, dst) + ", " + w);
    }
}

} // namespace cn_compiler
