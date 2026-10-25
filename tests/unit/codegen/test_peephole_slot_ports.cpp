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

// 14. 非相邻（中间写源寄存器）：拒绝（1320 近邻窗口判据）
TEST(PeepholeSlotMasm, NonAdjacentClobberRejected) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], rax",
        "    add rax, 1",
        "    mov r11, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// 14b. 1320 近邻转发：中间中性行（call 前 sub rsp）-> 删 store 改搬运
TEST(PeepholeSlotMasm, NearForwardWithinLimitEliminated) {
    const std::string in = join({
        "    mov [rbp-8], rax",
        "    sub rsp, 32",
        "    mov rcx, [rbp-8]",
        "    call __cn_str_free",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in),
              join({"    sub rsp, 32", "    mov rcx, rax", "    call __cn_str_free"}));
}

// 14c. 1320 死槽写：槽全函数仅一处 store（Load 结果槽写）-> 删除
TEST(PeepholeSlotMasm, DeadStoreSingleOccurrenceEliminated) {
    const std::string in = join({
        "    mov [rbp-32], eax",
        "    mov eax, [rbp-40]",
        "    test eax, eax",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in),
              join({"    mov eax, [rbp-40]", "    test eax, eax"}));
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

// 16. 不同槽：不配对（1320 起补源寄存器改写阻断，防近邻转发误合并）
TEST(PeepholeSlotMasm, DifferentSlotsRejected) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], rax",
        "    mov r9, qword ptr [rbp-16]",
        "    add rax, 1",
        "    mov r11, qword ptr [rbp-8]",
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

// 18. 不同槽：不配对（1320 起补源寄存器改写阻断）
TEST(PeepholeSlotArm64, DifferentSlotsRejected) {
    const std::string in = join({
        "    str x3, [x29,#-8]",
        "    ldr x9, [x29,#-16]",
        "    add x3, x3, #1",
        "    ldr x10, [x29,#-8]",
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

// ==================== 1279：32 位同槽同宽对（高位零证明·三面） ====================

// 49. MASM：dword/dword 同寄存器 + 前态 32 位写 -> 两行全删
TEST(PeepholeSlotMasm, DwordSameRegZeroProofFullDelete) {
    const std::string in = join({
        "    mov eax, dword ptr [rbp-16]",
        "    mov [rbp-8], eax",
        "    mov eax, [rbp-8]",
        "    add r9d, eax",
    });
    const std::string out = eliminateRedundantSlotTrafficMasm(in);
    EXPECT_EQ(out, join({"    mov eax, dword ptr [rbp-16]", "    add r9d, eax"}));
}

// 50. MASM：前态为 64 位装载（高半未知）-> 保留自搬
TEST(PeepholeSlotMasm, DwordSameRegUnknownHighKeepsSelfMove) {
    const std::string in = join({
        "    mov rax, qword ptr [rbp-16]",
        "    mov dword ptr [rbp-8], eax",
        "    mov eax, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTrafficMasm(in);
    EXPECT_EQ(out, join({"    mov rax, qword ptr [rbp-16]",
                         "    mov eax, eax"}));
}

// 51. MASM：第三处访问 -> 拒绝
TEST(PeepholeSlotMasm, DwordThirdOccurrenceRejected) {
    const std::string in = join({
        "    mov eax, dword ptr [rbp-16]",
        "    mov dword ptr [rbp-8], eax",
        "    mov eax, dword ptr [rbp-8]",
        "    mov r9d, dword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// 52. ARM：W 宽同寄存器 + 前态 W 写（高 32 已清零）-> 两行全删
TEST(PeepholeSlotArm64, W32SameRegZeroProofFullDelete) {
    const std::string in = join({
        "    mov w3, w2",
        "    str w3, [x29,#-8]",
        "    ldr w3, [x29,#-8]",
        "    cmp w3, #0",
    });
    const std::string out = eliminateRedundantSlotTrafficAArch64(in);
    EXPECT_EQ(out, join({"    mov w3, w2", "    cmp w3, #0"}));
}

// 53. ARM：W 宽同寄存器、前态未知 -> 保留自搬（W 写零扩展语义）
TEST(PeepholeSlotArm64, W32SameRegWithoutProofKeepsSelfMove) {
    const std::string in = join({
        "    str w3, [x29,#-8]",
        "    ldr w3, [x29,#-8]",
        "    cmp w3, #0",
    });
    const std::string out = eliminateRedundantSlotTrafficAArch64(in);
    EXPECT_EQ(out, join({"    mov w3, w3", "    cmp w3, #0"}));
}

// 54. ARM：跨宽（x 存 w 读同寄存器）-> 不整对删除
TEST(PeepholeSlotArm64, XStoreWLoadSameRegKeepsSelfMove) {
    const std::string in = join({
        "    mov w3, w2",
        "    str x3, [x29,#-8]",
        "    ldr w3, [x29,#-8]",
    });
    const std::string out = eliminateRedundantSlotTrafficAArch64(in);
    EXPECT_EQ(out, join({"    mov w3, w2", "    mov w3, w3"}));
}

// 55. ARM：第三处访问 -> 拒绝
TEST(PeepholeSlotArm64, W32ThirdOccurrenceRejected) {
    const std::string in = join({
        "    mov w3, w2",
        "    str w3, [x29,#-8]",
        "    ldr w3, [x29,#-8]",
        "    ldr w4, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), in);
}

// 56. ARM 大偏移间接面：W 宽同寄存器 + 前态 W 写 -> 整组（含两块物化）全删
TEST(PeepholeSlotArm64Indirect, W32SameRegZeroProofFullDelete) {
    const std::string in = join({
        "    mov w3, w2",
        "    sub x13, x29, #300",
        "    str w3, [x13]",
        "    sub x13, x29, #300",
        "    ldr w3, [x13]",
        "    cmp w3, #0",
    });
    const std::string out = eliminateRedundantSlotTrafficAArch64(in);
    EXPECT_EQ(out, join({"    mov w3, w2", "    cmp w3, #0"}));
}

// 57. ARM 大偏移间接面：W 宽无证明 -> 保留自搬（不整组删除）
TEST(PeepholeSlotArm64Indirect, W32SameRegWithoutProofKeepsSelfMove) {
    const std::string in = join({
        "    sub x13, x29, #300",
        "    str w3, [x13]",
        "    sub x13, x29, #300",
        "    ldr w3, [x13]",
        "    cmp w3, #0",
    });
    const std::string out = eliminateRedundantSlotTrafficAArch64(in);
    EXPECT_EQ(out, join({"    mov w3, w3", "    cmp w3, #0"}));
}

// ==================== 1279 审查 P1/P2：ARM 分类器细分支 ====================

// 58. P1-1 负例：三操作数 mov xN,xN,lsr #4 非自搬（高 32 位=原低 32 位，可能非零）
//     -> 证明不成立，保留自搬
TEST(PeepholeSlotArm64, ShiftedSelfMoveBlocksProof) {
    const std::string in = join({
        "    mov x3, x3, lsr #4",
        "    str w3, [x29,#-8]",
        "    ldr w3, [x29,#-8]",
    });
    const std::string out = eliminateRedundantSlotTrafficAArch64(in);
    EXPECT_EQ(out, join({"    mov x3, x3, lsr #4", "    mov w3, w3"}));
}

// 59. P1-1 正例：三操作数 mov xN,xN,lsl #0 是恒等 -> 证明继续向前找到定值点
TEST(PeepholeSlotArm64, ZeroShiftSelfMoveIsIdentity) {
    const std::string in = join({
        "    mov w3, w2",
        "    mov x3, x3, lsl #0",
        "    str w3, [x29,#-8]",
        "    ldr w3, [x29,#-8]",
    });
    const std::string out = eliminateRedundantSlotTrafficAArch64(in);
    EXPECT_EQ(out, join({"    mov w3, w2", "    mov x3, x3, lsl #0"}));
}

// 60. P1-1 负例：mov xN,#imm,lsr #4 移位后高半未知 -> 证明不成立
TEST(PeepholeSlotArm64, ShiftedImmMoveBlocksProof) {
    const std::string in = join({
        "    mov x3, #1, lsr #4",
        "    str w3, [x29,#-8]",
        "    ldr w3, [x29,#-8]",
    });
    const std::string out = eliminateRedundantSlotTrafficAArch64(in);
    EXPECT_EQ(out, join({"    mov x3, #1, lsr #4", "    mov w3, w3"}));
}

// 61. P1-2 负例：stlr 首操作数是源（非目的写）——不得作为零证明
//     修前会误判 W 目的写->证明成立->整对误删；修后保留自搬
TEST(PeepholeSlotArm64, StoreReleaseIsReadOnly) {
    const std::string in = join({
        "    stlr w3, [x1]",
        "    str w3, [x29,#-8]",
        "    ldr w3, [x29,#-8]",
    });
    const std::string out = eliminateRedundantSlotTrafficAArch64(in);
    EXPECT_EQ(out, join({"    stlr w3, [x1]", "    mov w3, w3"}));
}

// 62. P1-2：stllr/stlrb/stlrh 同族均按只读处理
TEST(PeepholeSlotArm64, StoreReleaseFamilyIsReadOnly) {
    const std::string variants[] = {"stllr w3, [x1]", "stlrb w3, [x1]",
                                    "stlrh w3, [x1]"};
    for (const std::string& st : variants) {
        const std::string in = join({
            "    " + st,
            "    str w3, [x29,#-8]",
            "    ldr w3, [x29,#-8]",
        });
        const std::string out = eliminateRedundantSlotTrafficAArch64(in);
        EXPECT_EQ(out, join({"    " + st, "    mov w3, w3"}));
    }
}

// 63. movz xN,#0：高半确定清零 -> 证明成立
TEST(PeepholeSlotArm64, DwordProofFromMovz) {
    const std::string in = join({
        "    movz x3, #0",
        "    str w3, [x29,#-8]",
        "    ldr w3, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    movz x3, #0\n");
}

// 64. movz lsl #32 写高半 -> 证明不成立
TEST(PeepholeSlotArm64, DwordProofBlockedByMovzHighShift) {
    const std::string in = join({
        "    movz x3, #1, lsl #32",
        "    str w3, [x29,#-8]",
        "    ldr w3, [x29,#-8]",
    });
    const std::string out = eliminateRedundantSlotTrafficAArch64(in);
    EXPECT_EQ(out, join({"    movz x3, #1, lsl #32", "    mov w3, w3"}));
}

// 65. movk 低 16 位片段（lsl #16）：高 32 位保持，证明继续向前
TEST(PeepholeSlotArm64, DwordProofSkipsLowMovk) {
    const std::string in = join({
        "    mov w3, w2",
        "    movk x3, #1, lsl #16",
        "    str w3, [x29,#-8]",
        "    ldr w3, [x29,#-8]",
    });
    const std::string out = eliminateRedundantSlotTrafficAArch64(in);
    EXPECT_EQ(out, join({"    mov w3, w2", "    movk x3, #1, lsl #16"}));
}

// 66. movk lsl #48 写高半 -> 证明不成立
TEST(PeepholeSlotArm64, DwordProofBlockedByMovkHighShift) {
    const std::string in = join({
        "    mov w3, w2",
        "    movk x3, #1, lsl #48",
        "    str w3, [x29,#-8]",
        "    ldr w3, [x29,#-8]",
    });
    const std::string out = eliminateRedundantSlotTrafficAArch64(in);
    EXPECT_EQ(out, join({"    mov w3, w2", "    movk x3, #1, lsl #48",
                         "    mov w3, w3"}));
}

// 67. uxtw xN,wM：零扩展 -> 证明成立
TEST(PeepholeSlotArm64, DwordProofFromUxtw) {
    const std::string in = join({
        "    uxtw x3, w2",
        "    str w3, [x29,#-8]",
        "    ldr w3, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    uxtw x3, w2\n");
}

// 68. cset xN：结果恒 0/1，高 32 位为零 -> 证明成立
TEST(PeepholeSlotArm64, DwordProofFromCset) {
    const std::string in = join({
        "    cset x3, gt",
        "    str w3, [x29,#-8]",
        "    ldr w3, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    cset x3, gt\n");
}

// 69. 窗口边界（ARM 面）：定值点距 store 129 行 -> 证明失败
TEST(PeepholeSlotArm64, DwordProofWindowExclusiveAt129) {
    std::vector<std::string> lines;
    lines.push_back("    mov w3, w2");
    for (int k = 0; k < 128; ++k) lines.push_back("    nop");
    lines.push_back("    str w3, [x29,#-8]");
    lines.push_back("    ldr w3, [x29,#-8]");
    const std::string out = eliminateRedundantSlotTrafficAArch64(join(lines));
    std::string want = "    mov w3, w2\n";
    for (int k = 0; k < 128; ++k) want += "    nop\n";
    want += "    mov w3, w3\n";
    EXPECT_EQ(out, want);
}

// ==================== 任务 1351：浮点同寄存器全删的高位清零语义 ====================
// x86：movss(32)/movsd(64) 的 load 形态清零高 96/64 位——全删（同寄存器）或
//   改写为寄存器搬运（异寄存器）都无法复现该清零，故须先证明高半从未被宽读
//   观察；证明不成立时原样保留两行（x86 无等价改写）。
// ARM64：LDR Sd/Dd 清零 V 寄存器上位；标量寄存器搬运（fmov）本身清零上位，
//   故 D 宽全删证明不成立时回落 fmov（S 宽既有口径一致）。

// 70. x86：movsd 同寄存器 + 全函数无宽读观察 -> 两行全删
TEST(PeepholeSlotFloatHighHalf, MovsdSameRegFullDeleteWhenHighUnobserved) {
    const std::string in = join({
        "    movsd qword ptr [rbp-8], xmm2",
        "    movsd xmm2, [rbp-8]",
        "    movsd xmm1, xmm2",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), "    movsd xmm1, xmm2\n");
}

// 71. x86 反例：movsd 同寄存器 + 后续 128 位整宽读 -> 保留原两行（字节不变）
TEST(PeepholeSlotFloatHighHalf, MovsdSameRegFullDeleteBlockedByWideObserver) {
    const std::string in = join({
        "    movsd qword ptr [rbp-8], xmm2",
        "    movsd xmm2, [rbp-8]",
        "    movups [rbp-24], xmm2",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// 72. x86：movss 异寄存器改写 + 目的寄存器高半未被观察 -> 改写为寄存器搬运
TEST(PeepholeSlotFloatHighHalf, MovssDiffRegRewriteWhenHighUnobserved) {
    const std::string in = join({
        "    movss dword ptr [rbp-8], xmm0",
        "    movss xmm1, dword ptr [rbp-8]",
        "    movss xmm3, xmm1",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in),
              join({"    movss xmm1, xmm0", "    movss xmm3, xmm1"}));
}

// 73. x86 反例：movss 异寄存器改写 + 目的寄存器被 128 位宽读 -> 保留原两行
//     （原 load 清零 xmm1 高 96 位，寄存器搬运保留旧值——语义不可替）
TEST(PeepholeSlotFloatHighHalf, MovssDiffRegRewriteBlockedByWideObserver) {
    const std::string in = join({
        "    movss dword ptr [rbp-8], xmm0",
        "    movss xmm1, dword ptr [rbp-8]",
        "    movups [rbp-24], xmm1",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// 74. x86 反例：宽读观察点位于本对之前（循环回边可再次观察）-> 同样保留
TEST(PeepholeSlotFloatHighHalf, MovsdSameRegBlockedByObserverBeforePair) {
    const std::string in = join({
        "Lloop:",
        "    movups [rbp-24], xmm2",
        "    movsd qword ptr [rbp-8], xmm2",
        "    movsd xmm2, [rbp-8]",
        "    jmp Lloop",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(in), in);
}

// 75. ARM64：str d0/ldr d0 同寄存器 + 无 q/v 宽读 -> 两行全删
TEST(PeepholeSlotFloatHighHalf, ArmFloatDoubleSameRegFullDelete) {
    const std::string in = join({
        "    str d0, [x29,#-8]",
        "    ldr d0, [x29,#-8]",
        "    fmov d1, d0",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    fmov d1, d0\n");
}

// 76. ARM64 反例：D 宽同寄存器 + 后续 q 整宽观察 -> 不全删，回落 fmov 自搬
//     （fmov 标量写清零上位，与 LDR Dd 清零语义一致）
TEST(PeepholeSlotFloatHighHalf, ArmFloatDoubleBlockedByQObserverKeepsFmov) {
    const std::string in = join({
        "    str d0, [x29,#-8]",
        "    ldr d0, [x29,#-8]",
        "    str q0, [x1]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in),
              join({"    fmov d0, d0", "    str q0, [x1]"}));
}

// 77. ARM64：S 宽同寄存器保持既有口径（fmov 自搬），即使无宽读也不全删
TEST(PeepholeSlotFloatHighHalf, ArmFloatSingleSameRegKeepsFmov) {
    const std::string in = join({
        "    str s0, [x29,#-8]",
        "    ldr s0, [x29,#-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(in), "    fmov s0, s0\n");
}
