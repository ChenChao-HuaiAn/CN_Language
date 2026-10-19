// 发射后窥孔·冗余槽读写消除单元测试（任务 1144·波2）
// 覆盖：
//   1. 槽死 + 同寄存器 64 位：两行全删
//   2. 槽死 + 异寄存器：删 store、load 改写为寄存器搬运
//   3. 宽度语义：dword/dword、qword→dword（自读保清零）、movsx/movzx、存窄读宽拒绝
//   4. 安全守卫：槽后续仍被访问 / store 行前有标签 / 不同槽 / 浮点族不混
//   5. 无命中时逐字节原样返回（含无尾随换行形态）
#include <gtest/gtest.h>

#include <string>

#include "cn_compiler/codegen/peephole_slot.hpp"

namespace {

using cn_compiler::peephole::eliminateRedundantSlotTraffic;

// 辅助：多行文本拼接（行数组 -> \n 结尾文本）
std::string join(const std::vector<std::string>& lines) {
    std::string s;
    for (const auto& ln : lines) s += ln + "\n";
    return s;
}

} // namespace

// 1. 槽死 + 同寄存器 64 位：两行全删
TEST(PeepholeSlot, DeadSlotSameReg64FullDelete) {
    const std::string in = join({
        "    push rbp",
        "    mov qword ptr [rbp-8], r10",
        "    mov r10, qword ptr [rbp-8]",
        "    add r10, 1",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    push rbp", "    add r10, 1"}));
}

// 2. 槽死 + 异寄存器：删 store、load 改寄存器搬运
TEST(PeepholeSlot, DeadSlotDiffRegRewrite) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    mov r9, qword ptr [rbp-8]",
        "    mov qword ptr [rbp-16], r9",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    mov r9, r10", "    mov qword ptr [rbp-16], r9"}));
}

// 3a. dword/dword 异寄存器
TEST(PeepholeSlot, DwordDiffRegRewrite) {
    const std::string in = join({
        "    mov dword ptr [rbp-8], r10d",
        "    mov r9d, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, "    mov r9d, r10d\n");
}

// 3b. qword 存 + dword 读同寄存器：自读保留（清零高位语义），store 删
TEST(PeepholeSlot, NarrowReadSameRegKeepsSelfMove) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    mov r10d, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, "    mov r10d, r10d\n");
}

// 3c. movsx 形态：从源寄存器低位符号扩展
TEST(PeepholeSlot, MovsxRewrite) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    movsx r9, byte ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, "    movsx r9, r10b\n");
}

// 3d. movzx 形态
TEST(PeepholeSlot, MovzxRewrite) {
    const std::string in = join({
        "    mov word ptr [rbp-8], r10w",
        "    movzx r9, word ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, "    movzx r9, r10w\n");
}

// 3e. 存窄读宽：拒绝（槽内残留高位不可复现）
TEST(PeepholeSlot, NarrowStoreWideLoadRejected) {
    const std::string in = join({
        "    mov dword ptr [rbp-8], r10d",
        "    mov r9, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 4a. 槽后续仍被访问：不优化
TEST(PeepholeSlot, LiveSlotRejected) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    mov r9, qword ptr [rbp-8]",
        "    mov r11, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 4a2. 循环回边：槽在本对之前被读、回跳可再次读到本对写入的值 -> 拒绝
TEST(PeepholeSlot, LoopBackEdgeThirdOccurrenceRejected) {
    const std::string in = join({
        "Lloop:",
        "    mov r9, qword ptr [rbp-8]",
        "    mov qword ptr [rbp-8], r10",
        "    mov r11, qword ptr [rbp-8]",
        "    jmp Lloop",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 4a3. 循环体但槽仅本对两处：消除安全（无第三处访问可回跳读）
TEST(PeepholeSlot, LoopBodyPairEliminated) {
    const std::string in = join({
        "Lloop:",
        "    mov qword ptr [rbp-8], r10",
        "    mov r9, qword ptr [rbp-8]",
        "    jmp Lloop",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"Lloop:", "    mov r9, r10", "    jmp Lloop"}));
}

// 4b. store 行前是标签：跳转到 store 行仍顺序执行对 -> 优化照常（槽仅本对两处）
TEST(PeepholeSlot, LabelBeforeStoreStillEliminated) {
    const std::string in = join({
        "Lok:",
        "    mov qword ptr [rbp-8], r10",
        "    mov r10, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), "Lok:\n");
}

// 4c. 标签与 store 之间隔注释行：同上（注释不影响）
TEST(PeepholeSlot, LabelBeforeStoreWithCommentEliminated) {
    const std::string in = join({
        "Lok:",
        "    // 说明",
        "    mov qword ptr [rbp-8], r10",
        "    mov r9, qword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"Lok:", "    // 说明", "    mov r9, r10"}));
}

// 4d. 不同槽：拒绝
TEST(PeepholeSlot, DifferentSlotsRejected) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    mov r9, qword ptr [rbp-16]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 4e. 非相邻（中间隔一条指令）：拒绝
TEST(PeepholeSlot, NonAdjacentRejected) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    mov r9, 1",
        "    mov r11, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 4f. 浮点同族：同 xmm 同宽全删；异 xmm 改写为寄存器搬运
TEST(PeepholeSlot, FloatPairHandled) {
    const std::string in = join({
        "    movsd [rbp-8], xmm0",
        "    movsd xmm1, [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), "    movsd xmm1, xmm0\n");
    const std::string in2 = join({
        "    movsd [rbp-8], xmm0",
        "    movsd xmm0, [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in2), "");
}

// 4g. 浮点 movsd qword ptr 形态（emitConstLoad 直写）识别
TEST(PeepholeSlot, FloatWithPtrPrefix) {
    const std::string in = join({
        "    movsd qword ptr [rbp-8], xmm0",
        "    movsd xmm0, [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), "");
}

// 4h. 整型与浮点跨族：拒绝
TEST(PeepholeSlot, CrossFamilyRejected) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    movsd xmm0, [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 5a. 无 [rbp 引用：原样返回（快速路径）
TEST(PeepholeSlot, NoSlotReferenceReturnsIdentical) {
    const std::string in = join({"    mov r10, 1", "    ret"});
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 5b. 无尾随换行的文本：保持无尾随换行
TEST(PeepholeSlot, NoTrailingNewlinePreserved) {
    const std::string in =
        "    mov qword ptr [rbp-8], r10\n    mov r10, qword ptr [rbp-8]";
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), "");
}

// 5c. 取址（lea）也算对槽的访问：拒绝
TEST(PeepholeSlot, LeaCountsAsAccessRejected) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    mov r9, qword ptr [rbp-8]",
        "    lea rdi, [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 5d. 连续多对：各自独立消除
TEST(PeepholeSlot, MultiplePairsIndependent) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    mov qword ptr [rbp-16], r10",
        "    mov r10, qword ptr [rbp-16]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, "    mov qword ptr [rbp-8], r10\n");
}
