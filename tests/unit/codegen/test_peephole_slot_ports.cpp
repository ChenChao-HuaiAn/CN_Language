// 发射后窥孔·冗余槽读写消除——win-x64(MASM) 与 linux-arm64(AArch64) 语法面单测
// （任务 1157；linux_x86_64 面原 21 例见 test_peephole_slot.cpp）
// 覆盖每面：正例（全删/改写/宽度语义/浮点/窄载扩展）+ 反例（槽三现/跨标签/
//   非相邻/异槽/跨族/读宽>存宽/奇异语法/无尾随换行/快速路径）。
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "cn_compiler/codegen/peephole_slot.hpp"

namespace {

using cn_compiler::peephole::eliminateRedundantSlotTrafficAArch64;
using cn_compiler::peephole::eliminateRedundantSlotTrafficMasm;

std::string join(const std::vector<std::string>& lines) {
    std::string s;
    for (const auto& ln : lines) s += ln + "\n";
    return s;
}

} // namespace

// ==================== win-x64（MASM）面 ====================

// 1. 槽死 + 同寄存器 64 位：两行全删
TEST(PeepholeSlotMasm, DeadSlotSameReg64FullDelete) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], rax",
        "    mov rax, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), "");
}

// 2. 无 ptr 前缀（x64 后端直写/直装实际形态）：同寄存器全删
TEST(PeepholeSlotMasm, NoPtrPrefixFullDelete) {
    const std::string in = join({
        "    mov [rbp-8], r12",
        "    mov r12, [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), "");
}

// 3. 槽死 + 异寄存器：删 store、load 改写为寄存器搬运
TEST(PeepholeSlotMasm, DeadSlotDiffRegRewrite) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], rax",
        "    mov r9, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), "    mov r9, rax\n");
}

// 4. MASM 大写前缀（语法大小写不敏感）：同寄存器全删
TEST(PeepholeSlotMasm, UppercasePrefixFullDelete) {
    const std::string in = join({
        "    mov QWORD PTR [rbp-8], rax",
        "    mov rax, QWORD PTR [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), "");
}

// 5. dword/dword 异寄存器
TEST(PeepholeSlotMasm, DwordDiffRegRewrite) {
    const std::string in = join({
        "    mov dword ptr [rbp-8], eax",
        "    mov r9d, dword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), "    mov r9d, eax\n");
}

// 6. 存窄读宽：拒绝（槽内残留高位不可复现）
TEST(PeepholeSlotMasm, NarrowStoreWideLoadRejected) {
    const std::string in = join({
        "    mov dword ptr [rbp-8], eax",
        "    mov r9, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// 7. movsx 32 位目的（x64 后端实际产物）：源取低位字节
TEST(PeepholeSlotMasm, Movsx32DestRewrite) {
    const std::string in = join({
        "    mov [rbp-8], rax",
        "    movsx eax, byte ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), "    movsx eax, al\n");
}

// 8. movzx 32 位目的 + word 源
TEST(PeepholeSlotMasm, Movzx32DestWordRewrite) {
    const std::string in = join({
        "    mov [rbp-8], rax",
        "    movzx eax, word ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), "    movzx eax, ax\n");
}

// 9. movsx 64 位目的（GAS 同款形态在 MASM 面亦识别）
TEST(PeepholeSlotMasm, Movsx64DestRewrite) {
    const std::string in = join({
        "    mov [rbp-8], r10",
        "    movsx r9, byte ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), "    movsx r9, r10b\n");
}

// 10. movss 浮点异寄存器改写
TEST(PeepholeSlotMasm, MovssDiffRegRewrite) {
    const std::string in = join({
        "    movss dword ptr [rbp-8], xmm0",
        "    movss xmm1, dword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), "    movss xmm1, xmm0\n");
}

// 11. movsd 浮点同寄存器全删（无 ptr 前缀读形态）
TEST(PeepholeSlotMasm, MovsdSameRegFullDelete) {
    const std::string in = join({
        "    movsd qword ptr [rbp-8], xmm2",
        "    movsd xmm2, [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), "");
}

// 12. 槽出现三次（循环回边可达）：拒绝
TEST(PeepholeSlotMasm, ThirdOccurrenceRejected) {
    const std::string in = join({
        "    mov r9, qword ptr [rbp-8]",
        "    mov qword ptr [rbp-8], rax",
        "    mov r10, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// 13. 槽在本对之后仍被访问：拒绝
TEST(PeepholeSlotMasm, LiveSlotRejected) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], rax",
        "    mov r9, qword ptr [rbp-8]",
        "    mov r11, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// 14. 非相邻（中间隔一条指令）：拒绝
TEST(PeepholeSlotMasm, NonAdjacentRejected) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], rax",
        "    mov r9, 1",
        "    mov r11, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// 15. 中间隔标签：拒绝（跨块路径可跳过 store）
TEST(PeepholeSlotMasm, LabelBetweenRejected) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], rax",
        "L跳点:",
        "    mov r9, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// 16. 不同槽：拒绝
TEST(PeepholeSlotMasm, DifferentSlotsRejected) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], rax",
        "    mov r9, qword ptr [rbp-16]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// 17. 整型/浮点跨族：拒绝
TEST(PeepholeSlotMasm, CrossFamilyRejected) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], rax",
        "    movsd xmm0, [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// 18. 奇异语法（QWORD 缺 PTR 尺寸前缀）：不识别、原样保留
TEST(PeepholeSlotMasm, WeirdSizeKeywordRejected) {
    const std::string in = join({
        "    mov QWORD [rbp-8], rax",
        "    mov rax, QWORD [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// 19. 32 位同寄存器：保留自读（写 32 位清零高位的语义须复现，不整对删除）
TEST(PeepholeSlotMasm, DwordSameRegKeepsSelfMove) {
    const std::string in = join({
        "    mov [rbp-8], eax",
        "    mov eax, [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), "    mov eax, eax\n");
}

// 20. 无槽引用：原样返回（快速路径）
TEST(PeepholeSlotMasm, NoSlotReferenceReturnsIdentical) {
    const std::string in = join({"    mov r10, 1", "    ret"});
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// 21. 无尾随换行：保持无尾随换行
TEST(PeepholeSlotMasm, NoTrailingNewlinePreserved) {
    const std::string in = "    mov [rbp-8], rax\n    mov rax, [rbp-8]";
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), "");
}

// 22. movsxd 不在识别族（扩展目的非 8/16 位源）：拒绝
TEST(PeepholeSlotMasm, MovsxdLoadRejected) {
    const std::string in = join({
        "    mov dword ptr [rbp-8], ecx",
        "    movsxd rax, dword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// ==================== linux-arm64（AArch64）面 ====================

// 1. str/ldr 同寄存器 x 宽：两行全删
TEST(PeepholeSlotArm64, DeadSlotSameRegXFullDelete) {
    const std::string in = join({
        "    str x3, [x29,#-8]",
        "    ldr x3, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "");
}

// 2. str x3 -> ldr x9：寄存器搬运改写
TEST(PeepholeSlotArm64, DiffRegXRewrite) {
    const std::string in = join({
        "    str x3, [x29,#-8]",
        "    ldr x9, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    mov x9, x3\n");
}

// 3. w 宽同宽：mov w9, w3
TEST(PeepholeSlotArm64, W32Rewrite) {
    const std::string in = join({
        "    str w3, [x29,#-8]",
        "    ldr w9, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    mov w9, w3\n");
}

// 4. 宽存窄读（x 存 w 读）：源取 w 形态
TEST(PeepholeSlotArm64, WideStoreNarrowReadRewrite) {
    const std::string in = join({
        "    str x3, [x29,#-8]",
        "    ldr w9, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    mov w9, w3\n");
}

// 5. ldrb 零扩展：uxtb
TEST(PeepholeSlotArm64, LdrbZeroExtendRewrite) {
    const std::string in = join({
        "    str w3, [x29,#-8]",
        "    ldrb w9, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    uxtb w9, w3\n");
}

// 6. ldrh 零扩展：uxth
TEST(PeepholeSlotArm64, LdrhZeroExtendRewrite) {
    const std::string in = join({
        "    str w3, [x29,#-8]",
        "    ldrh w9, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    uxth w9, w3\n");
}

// 7. strb + ldrsb：sxtb（源 w 形态）
TEST(PeepholeSlotArm64, StrbLdrsbSignExtendRewrite) {
    const std::string in = join({
        "    strb w10, [x29,#-24]",
        "    ldrsb x9, [x29,#-24]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    sxtb x9, w10\n");
}

// 8. strh + ldrsh：sxth
TEST(PeepholeSlotArm64, StrhLdrshSignExtendRewrite) {
    const std::string in = join({
        "    strh w10, [x29,#-48]",
        "    ldrsh x9, [x29,#-48]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    sxth x9, w10\n");
}

// 9. strb + ldrb：uxtb（8 位零扩展）
TEST(PeepholeSlotArm64, StrbLdrbZeroExtendRewrite) {
    const std::string in = join({
        "    strb w10, [x29,#-24]",
        "    ldrb w9, [x29,#-24]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    uxtb w9, w10\n");
}

// 10. ldrsb 目的 w 形态：保持 w 目的
TEST(PeepholeSlotArm64, LdrsbWDestRewrite) {
    const std::string in = join({
        "    str x3, [x29,#-8]",
        "    ldrsb w9, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    sxtb w9, w3\n");
}

// 11. 浮点 d 宽异寄存器：fmov d1, d0
TEST(PeepholeSlotArm64, FloatDoubleRewrite) {
    const std::string in = join({
        "    str d0, [x29,#-8]",
        "    ldr d1, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    fmov d1, d0\n");
}

// 12. 浮点 s 宽异寄存器：fmov s1, s0
TEST(PeepholeSlotArm64, FloatSingleRewrite) {
    const std::string in = join({
        "    str s0, [x29,#-8]",
        "    ldr s1, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    fmov s1, s0\n");
}

// 13. 浮点宽存窄读（d 存 s 读）：fmov s1, s0
TEST(PeepholeSlotArm64, FloatWideNarrowReadRewrite) {
    const std::string in = join({
        "    str d0, [x29,#-8]",
        "    ldr s1, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    fmov s1, s0\n");
}

// 14. 浮点窄存宽读（s 存 d 读）：拒绝
TEST(PeepholeSlotArm64, FloatNarrowStoreWideReadRejected) {
    const std::string in = join({
        "    str s0, [x29,#-8]",
        "    ldr d1, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 15. s 宽同寄存器：不全删（LDR Sd 清零高位语义以 fmov 自读保真）
TEST(PeepholeSlotArm64, FloatSingleSameRegKeepsSelfMove) {
    const std::string in = join({
        "    str s0, [x29,#-8]",
        "    ldr s0, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    fmov s0, s0\n");
}

// 16. 槽出现三次：拒绝
TEST(PeepholeSlotArm64, ThirdOccurrenceRejected) {
    const std::string in = join({
        "    ldr x9, [x29,#-8]",
        "    str x3, [x29,#-8]",
        "    ldr x10, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 17. 中间隔标签：拒绝
TEST(PeepholeSlotArm64, LabelBetweenRejected) {
    const std::string in = join({
        "    str x3, [x29,#-8]",
        "L跳点:",
        "    ldr x9, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 18. 不同槽：拒绝
TEST(PeepholeSlotArm64, DifferentSlotsRejected) {
    const std::string in = join({
        "    str x3, [x29,#-8]",
        "    ldr x9, [x29,#-16]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 19. 整型/浮点跨族：拒绝
TEST(PeepholeSlotArm64, CrossFamilyRejected) {
    const std::string in = join({
        "    str x0, [x29,#-8]",
        "    ldr d0, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 20. 含空格槽变体（非本产物族）：保守拒绝
TEST(PeepholeSlotArm64, SpacedSlotRejected) {
    const std::string in = join({
        "    str x0, [x29, #-8]",
        "    ldr x1, [x29, #-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 21. 前索引写回（[x29,#-8]!）：拒绝
TEST(PeepholeSlotArm64, PreIndexWritebackRejected) {
    const std::string in = join({
        "    str x0, [x29,#-8]",
        "    ldr x1, [x29,#-8]!",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 22. x13 间接寻址（大偏移形态）：不是固定帧槽，拒绝
TEST(PeepholeSlotArm64, IndirectX13Ignored) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x0, [x13]",
        "    ldr x1, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 23. 无偏移槽 [x29]：可识别并改写
TEST(PeepholeSlotArm64, ZeroOffsetSlotRewrite) {
    const std::string in = join({
        "    str x0, [x29]",
        "    ldr x1, [x29]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    mov x1, x0\n");
}

// 24. 大写助记符（GAS 大小写不敏感）：识别并改写
TEST(PeepholeSlotArm64, UppercaseMnemonicRewrite) {
    const std::string in = join({
        "    STR x0, [x29,#-8]",
        "    LDR x1, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    mov x1, x0\n");
}

// 25. 读宽>存宽（w 存 x 读）：拒绝
TEST(PeepholeSlotArm64, NarrowStoreWideReadRejected) {
    const std::string in = join({
        "    str w3, [x29,#-8]",
        "    ldr x9, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// ==================== linux-arm64 大偏移间接寻址面（任务 1243） ====================
// 真实产物形态：sub x13,x29,#N / str ..,[x13] / sub x13,x29,#N / ldr ..,[x13]

// 26. 4 行同寄存器 64 位：整体全删（含两条地址物化）
TEST(PeepholeSlotArm64Indirect, SameReg64FullDelete) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x0, [x13]",
        "    sub x13, x29, #300",
        "    ldr x0, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "");
}

// 27. 4 行异寄存器：删两块+store，load 改写为 mov
TEST(PeepholeSlotArm64Indirect, DiffRegRewrite) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x0, [x13]",
        "    sub x13, x29, #300",
        "    ldr x1, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    mov x1, x0\n");
}

// 28. 正向偏移 add 形态
TEST(PeepholeSlotArm64Indirect, PositiveOffsetRewrite) {
    const std::string in = join({
        "    add x13, x29, #264",
        "    str x5, [x13]",
        "    add x13, x29, #264",
        "    ldr x6, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    mov x6, x5\n");
}

// 29. 32 位存读：mov w 形态
TEST(PeepholeSlotArm64Indirect, W32Rewrite) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str w3, [x13]",
        "    sub x13, x29, #300",
        "    ldr w9, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    mov w9, w3\n");
}

// 30. 宽存窄读 ldrb：uxtb 零扩展
TEST(PeepholeSlotArm64Indirect, LdrbZeroExtendRewrite) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x3, [x13]",
        "    sub x13, x29, #300",
        "    ldrb w9, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    uxtb w9, w3\n");
}

// 31. 窄存 ldrsb：sxtb 符号扩展
TEST(PeepholeSlotArm64Indirect, LdrsbSignExtendRewrite) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    strb w10, [x13]",
        "    sub x13, x29, #300",
        "    ldrsb x9, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    sxtb x9, w10\n");
}

// 32. 浮点 d 宽：fmov
TEST(PeepholeSlotArm64Indirect, FloatDoubleRewrite) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str d0, [x13]",
        "    sub x13, x29, #300",
        "    ldr d1, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    fmov d1, d0\n");
}

// 33. movz/movk 物化链（|off|>4095）：跨 6 行消除
TEST(PeepholeSlotArm64Indirect, MovzMovkChainRewrite) {
    const std::string in = join({
        "    movz x13, #4464",
        "    movk x13, #1, lsl #16",
        "    sub x13, x29, x13",
        "    str x0, [x13]",
        "    movz x13, #4464",
        "    movk x13, #1, lsl #16",
        "    sub x13, x29, x13",
        "    ldr x1, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    mov x1, x0\n");
}

// 34. 尾部后续访问有完整物化：旧值不泄漏，仍消除
TEST(PeepholeSlotArm64Indirect, TailRematerializedStillEliminates) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x0, [x13]",
        "    sub x13, x29, #300",
        "    ldr x1, [x13]",
        "    sub x13, x29, #8",
        "    str x1, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in),
              join({"    mov x1, x0", "    sub x13, x29, #8", "    str x1, [x13]"}));
}

// 35. 对前已有 x13 其它使用（block1 会覆盖）：不影响消除
TEST(PeepholeSlotArm64Indirect, PriorX13UseIgnored) {
    const std::string in = join({
        "    mov x13, x5",
        "    sub x13, x29, #300",
        "    str x0, [x13]",
        "    sub x13, x29, #300",
        "    ldr x1, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), join({"    mov x13, x5", "    mov x1, x0"}));
}

// 36. 尾部把 x13 当值读（mov x0, x13）：删除定义后旧值泄漏 -> 拒绝
TEST(PeepholeSlotArm64Indirect, TailValueReadRejected) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x0, [x13]",
        "    sub x13, x29, #300",
        "    ldr x1, [x13]",
        "    mov x0, x13",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 37. 尾部 x13 参与栈调整（读未定义值）-> 拒绝
TEST(PeepholeSlotArm64Indirect, TailStackAdjustRejected) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x0, [x13]",
        "    sub x13, x29, #300",
        "    ldr x1, [x13]",
        "    sub sp, sp, x13",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 38. 同槽经另一寄存器物化访问（偏移计数 3）-> 拒绝
TEST(PeepholeSlotArm64Indirect, SameOffsetViaOtherRegRejected) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x0, [x13]",
        "    sub x13, x29, #300",
        "    ldr x1, [x13]",
        "    sub x14, x29, #300",
        "    ldr x2, [x14]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 39. 同槽另有直接寻址访问（偏移计数 3）-> 拒绝
TEST(PeepholeSlotArm64Indirect, SameOffsetDirectAccessRejected) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x0, [x13]",
        "    sub x13, x29, #300",
        "    ldr x1, [x13]",
        "    ldr x2, [x29,#-300]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 40. 中间隔标签（load 不可达块内）-> 拒绝
TEST(PeepholeSlotArm64Indirect, LabelBetweenRejected) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x0, [x13]",
        "    sub x13, x29, #300",
        "L跳点:",
        "    ldr x1, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 41. 读宽>存宽：拒绝
TEST(PeepholeSlotArm64Indirect, NarrowStoreWideReadRejected) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str w0, [x13]",
        "    sub x13, x29, #300",
        "    ldr x1, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 42. 两块物化偏移不同：拒绝
TEST(PeepholeSlotArm64Indirect, DifferentMaterializationRejected) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x0, [x13]",
        "    sub x13, x29, #304",
        "    ldr x1, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 43. store 源就是地址寄存器：拒绝（删块后源值无定义）
TEST(PeepholeSlotArm64Indirect, StoreSourceIsAddrRegRejected) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x13, [x13]",
        "    sub x13, x29, #300",
        "    ldr x1, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 44. 尾部 movk 无 movz 头（读未定义值）：拒绝
TEST(PeepholeSlotArm64Indirect, TailMovkWithoutMovzRejected) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x0, [x13]",
        "    sub x13, x29, #300",
        "    ldr x1, [x13]",
        "    movk x13, #1, lsl #16",
        "    str x0, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 45. 无尾随换行：形态保持
TEST(PeepholeSlotArm64Indirect, NoTrailingNewlinePreserved) {
    const std::string in =
        "    sub x13, x29, #300\n    str x0, [x13]\n"
        "    sub x13, x29, #300\n    ldr x1, [x13]";
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    mov x1, x0");
}

// 46. 单 movz + sub 的物化链（|off| 略超 4095）：跨 6 行消除
TEST(PeepholeSlotArm64Indirect, MovzSubChainRewrite) {
    const std::string in = join({
        "    movz x13, #4992",
        "    sub x13, x29, x13",
        "    str w9, [x13]",
        "    movz x13, #4992",
        "    sub x13, x29, x13",
        "    ldr w9, [x13]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    mov w9, w9\n");
}

// 47. 尾部栈调整前有 movz 定义（纯读 x13）：仍消除
TEST(PeepholeSlotArm64Indirect, TailRematerializedStackAdjustStillEliminates) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x0, [x13]",
        "    sub x13, x29, #300",
        "    ldr x1, [x13]",
        "    movz x13, #8",
        "    sub sp, sp, x13",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in),
              join({"    mov x1, x0", "    movz x13, #8", "    sub sp, sp, x13"}));
}

// 48. 尾部地址计算前有 movz 定义（纯读 x13）：仍消除
TEST(PeepholeSlotArm64Indirect, TailRematerializedAddrCalcStillEliminates) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str x0, [x13]",
        "    sub x13, x29, #300",
        "    ldr x1, [x13]",
        "    movz x13, #28000",
        "    add x9, x29, x13",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in),
              join({"    mov x1, x0", "    movz x13, #28000", "    add x9, x29, x13"}));
}
