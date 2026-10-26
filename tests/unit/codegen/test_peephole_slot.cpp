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
//    （1320 起：末尾 store 若再无读即为死槽写，本用例追加读保持「改写」面独立）
TEST(PeepholeSlot, DeadSlotDiffRegRewrite) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    mov r9, qword ptr [rbp-8]",
        "    mov qword ptr [rbp-16], r9",
        "    add r9, 1",  // 打断 r9 的后续转发：-16 保持存活，只测 -8 的改写
        "    mov r11, qword ptr [rbp-16]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    mov r9, r10", "    mov qword ptr [rbp-16], r9",
                         "    add r9, 1", "    mov r11, qword ptr [rbp-16]"}));
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

// 4d. 不同槽：不配对（1320 起补源寄存器改写阻断，防近邻转发误合并）
TEST(PeepholeSlot, DifferentSlotsRejected) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    mov r9, qword ptr [rbp-16]",
        "    add r10, 1",
        "    mov r11, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 4e. 非相邻（中间隔一条会写源寄存器的指令）：拒绝（1320 起近邻窗口内
//     无寄存器改写才允许转发；此处 add 打断值传递 -> 原样保留）
TEST(PeepholeSlot, NonAdjacentClobberRejected) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    add r10, 1",
        "    mov r11, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 4e2. 1320 近邻转发正例：中间为中性行（不写源寄存器、无控制流）-> 删 store 改搬运
TEST(PeepholeSlot, NearForwardWithinLimitEliminated) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    sub rsp, 32",
        "    mov r11, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), join({"    sub rsp, 32", "    mov r11, r10"}));
}

// 4e3. 1320 近邻窗口边界：行距 9 > 上限 8 -> 拒绝
TEST(PeepholeSlot, NearForwardBeyondLimitRejected) {
    std::vector<std::string> lines;
    lines.push_back("    mov qword ptr [rbp-8], r10");
    for (int k = 0; k < 8; ++k) lines.push_back("    nop");
    lines.push_back("    mov r11, qword ptr [rbp-8]");
    const std::string in = join(lines);
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 4e4. 1320 死槽写：槽全函数仅一处 store -> 删除
TEST(PeepholeSlot, DeadStoreSingleOccurrenceEliminated) {
    const std::string in = join({
        "    mov r10d, 7",
        "    mov qword ptr [rbp-8], r10",
        "    ret",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), join({"    mov r10d, 7", "    ret"}));
}

// 4e5. 1320 死槽写三现扩展：两写之间无读、后写覆盖前写 -> 前写删除；
//      存活两现（后写+读）按转发合并为寄存器搬运 -> 整链塌缩为一条搬运行
TEST(PeepholeSlot, DeadStoreCoveredByLaterStoreEliminated) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    mov qword ptr [rbp-8], r11",
        "    mov r9, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), "    mov r9, r11\n");
}

// 4e7. 1320 取址聚合别名保护：lea 基址 16 字节窗口覆盖该槽 -> 保留
//      （i128 双槽：lea [rbp-16] 指向聚合，[rbp-8] 是高半槽；删写会漏掉
//       指针侧的读——324_整128极值边界 实录）
TEST(PeepholeSlot, LeaAggregateAliasBlocksDeadStore) {
    const std::string in = join({
        "    lea rcx, [rbp-16]",
        "    mov qword ptr [rbp-8], r10",
        "    mov rax, [rbp-24]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 4e8. 1320 取址聚合别名：同上，转发对（store+load）亦须保留
TEST(PeepholeSlot, LeaAggregateAliasBlocksForwardPair) {
    const std::string in = join({
        "    lea rcx, [rbp-16]",
        "    mov qword ptr [rbp-8], r10",
        "    mov rax, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}


// 4e9. 1320 云端回归修复：结构体按值参数聚合（4 槽逐槽直接写 + 基址 lea 传递，
//      共 32 字节）——注入帧布局聚合区间后，聚合内第 4 槽写必须保留
//      （16 字节文本窗口覆盖不到；未注入即云端 79_v2 fix_p 编译失败根因）。
TEST(PeepholeSlot, AggregateRangeProtectsStructParamCopy) {
    const std::string in = join({
        "    mov qword ptr [rbp-200], r11",
        "    mov qword ptr [rbp-192], r11",
        "    mov qword ptr [rbp-184], r11",
        "    mov qword ptr [rbp-176], r11",
        "    lea r10, [rbp-200]",
        "    mov qword ptr [rbp-8], r10",
        "    mov r9, qword ptr [rbp-8]",
        "    ret",
    });
    const std::vector<cn_compiler::peephole::AggregateRange> aggregates = {{-200, 32}};
    std::string got = in;
    cn_compiler::peephole::eliminateRedundantSlotTrafficInPlace(got, aggregates);
    // 聚合内四槽写全部保留（第 4 槽 -176 是 16 字节窗口外的关键面）
    EXPECT_NE(got.find("mov qword ptr [rbp-176], r11"), std::string::npos) << got;
    EXPECT_NE(got.find("mov qword ptr [rbp-200], r11"), std::string::npos) << got;
    // 聚合外可正常优化：[rbp-8] 的 store+load 转发为寄存器搬运
    EXPECT_NE(got.find("mov r9, r10"), std::string::npos) << got;
}

// 4e10. 同上：聚合区间外的单现死写仍可删（区间注入不过度阻断）
TEST(PeepholeSlot, AggregateRangeKeepsOutOfRangeOptimization) {
    const std::string in = join({
        "    mov qword ptr [rbp-56], rsi",
        "    lea r10, [rbp-200]",
        "    mov qword ptr [rbp-8], r10",
        "    ret",
    });
    const std::vector<cn_compiler::peephole::AggregateRange> aggregates = {{-200, 32}};
    std::string got = in;
    cn_compiler::peephole::eliminateRedundantSlotTrafficInPlace(got, aggregates);
    // -56 的写无读且不在聚合区间内 -> 删除
    EXPECT_EQ(got.find("mov qword ptr [rbp-56], rsi"), std::string::npos) << got;
}

// 4e6. 1320 死槽写别名保护：16 字节整宽访存覆盖该槽 -> 保留
TEST(PeepholeSlot, DeadStoreAliasedByWideAccessRejected) {
    const std::string in = join({
        "    movups [rbp-16], xmm0",  // 16 字节写覆盖 [rbp-16, rbp)
        "    mov qword ptr [rbp-8], r10",
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

// 5d. 连续多对：各自独立消除（1320 起 -8 对以源寄存器改写阻断保持存活）
TEST(PeepholeSlot, MultiplePairsIndependent) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    mov qword ptr [rbp-16], r10",
        "    mov r10, qword ptr [rbp-16]",
        "    add r10, 1",
        "    mov r9, qword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    mov qword ptr [rbp-8], r10", "    add r10, 1",
                         "    mov r9, qword ptr [rbp-8]"}));
}

// ==================== 1279：32 位同槽同宽对（高位零证明） ====================

// 6a. dword/dword 同寄存器 + 前态 32 位写（高 32 已清零）：两行全删
TEST(PeepholeSlot, DwordSameRegWithZeroProofFullDelete) {
    const std::string in = join({
        "    mov r10d, dword ptr [rbp-16]",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
        "    add r9d, r10d",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    mov r10d, dword ptr [rbp-16]",
                         "    add r9d, r10d"}));
}

// 6b. 证明源=movzx（写 64 位目的零扩展）：1253 窄布尔真实形态
TEST(PeepholeSlot, DwordSameRegProofFromMovzx) {
    const std::string in = join({
        "    cmp r10d, r9d",
        "    setg r9b",
        "    movzx r10, r9b",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
        "    test r10, r10",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    cmp r10d, r9d",
                         "    setg r9b",
                         "    movzx r10, r9b",
                         "    test r10, r10"}));
}

// 6c. dword/dword 同寄存器、前态高位未知：自吞不消除（保留自搬保零扩展）
TEST(PeepholeSlot, DwordSameRegWithoutProofKeepsSelfMove) {
    const std::string in = join({
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
        "    add r9d, r10d",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    mov r10d, r10d", "    add r9d, r10d"}));
}

// 6d. dword/dword 同寄存器、前态为 64 位装载（高位未知）：不整对删除
TEST(PeepholeSlot, DwordSameRegUnknownHighKeepsSelfMove) {
    const std::string in = join({
        "    mov r10, qword ptr [rbp-16]",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    mov r10, qword ptr [rbp-16]",
                         "    mov r10d, r10d"}));
}

// 6e. 跨宽（dword 存 + qword 读同寄存器）：拒绝（槽内残留高位不可复现）
TEST(PeepholeSlot, DwordStoreQwordLoadSameRegRejected) {
    const std::string in = join({
        "    mov r10d, dword ptr [rbp-16]",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 6f. 第三处访问：拒绝（不得放宽到漏第三处）
TEST(PeepholeSlot, DwordThirdOccurrenceRejected) {
    const std::string in = join({
        "    mov r10d, dword ptr [rbp-16]",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
        "    mov eax, dword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 6g. 取址/别名访问（lea 第三处）：拒绝
TEST(PeepholeSlot, DwordAliasAccessRejected) {
    const std::string in = join({
        "    mov r10d, dword ptr [rbp-16]",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
        "    lea r11, [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 6h. volatile 标记形态（非本产物族）：不识别、原样保留
TEST(PeepholeSlot, DwordVolatileMarkerRejected) {
    const std::string in = join({
        "    mov dword ptr [rbp-8], r10d // volatile",
        "    mov r10d, dword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), in);
}

// 6i. 证明被控制流截断（前态写与 store 之间隔标签）：保留自搬
TEST(PeepholeSlot, DwordProofBrokenByLabelKeepsSelfMove) {
    const std::string in = join({
        "    mov r10d, dword ptr [rbp-16]",
        "Lentry:",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    mov r10d, dword ptr [rbp-16]",
                         "Lentry:",
                         "    mov r10d, r10d"}));
}

// ==================== 1279 审查 P2：分类器细分支 ====================

// 7a. movsx 64 位目的（符号扩展，高半未知）：证明不成立，保留自搬
TEST(PeepholeSlot, DwordProofBlockedByMovsx64) {
    const std::string in = join({
        "    movsx r10, r9b",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    movsx r10, r9b", "    mov r10d, r10d"}));
}

// 7b. movsxd（32->64 符号扩展，高半未知）：证明不成立，保留自搬
TEST(PeepholeSlot, DwordProofBlockedByMovsxd) {
    const std::string in = join({
        "    movsxd r10, r9d",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    movsxd r10, r9d", "    mov r10d, r10d"}));
}

// 7c. xor r32 目的写（32 位目的恒清高 32）：证明成立，两行全删
TEST(PeepholeSlot, DwordProofFromXor32) {
    const std::string in = join({
        "    xor r10d, r10d",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, "    xor r10d, r10d\n");
}

// 7d. sub r32 目的写：证明成立
TEST(PeepholeSlot, DwordProofFromSub32) {
    const std::string in = join({
        "    sub r10d, r9d",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, "    sub r10d, r9d\n");
}

// 7e. imul r32 目的写：证明成立
TEST(PeepholeSlot, DwordProofFromImul32) {
    const std::string in = join({
        "    imul r10d, r9d",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, "    imul r10d, r9d\n");
}

// 7f. mov r64, 非负 imm32：高 32 位确定为零，证明成立
TEST(PeepholeSlot, DwordProofFromNonNegativeImm32) {
    const std::string in = join({
        "    mov r10, 2147483647",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, "    mov r10, 2147483647\n");
}

// 7g. mov r64, 负立即数：高 32 位未知（符号扩展），证明不成立
TEST(PeepholeSlot, DwordProofBlockedByNegativeImm) {
    const std::string in = join({
        "    mov r10, -1",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    mov r10, -1", "    mov r10d, r10d"}));
}

// 7h. 8 位部分写：高 32 位保持，证明继续向前找到 32 位定值点
TEST(PeepholeSlot, DwordProofSkipsPartial8BitWrite) {
    const std::string in = join({
        "    mov r10d, dword ptr [rbp-16]",
        "    mov r10b, al",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    mov r10d, dword ptr [rbp-16]", "    mov r10b, al"}));
}

// 7i. 16 位部分写：同上
TEST(PeepholeSlot, DwordProofSkipsPartial16BitWrite) {
    const std::string in = join({
        "    mov r10d, dword ptr [rbp-16]",
        "    mov r10w, ax",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    mov r10d, dword ptr [rbp-16]", "    mov r10w, ax"}));
}

// 7j. call 截断证明（caller-saved 可能被清）：保留自搬
TEST(PeepholeSlot, DwordProofBlockedByCall) {
    const std::string in = join({
        "    mov r10d, dword ptr [rbp-16]",
        "    call foo",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
    });
    const std::string out = eliminateRedundantSlotTraffic(in);
    EXPECT_EQ(out, join({"    mov r10d, dword ptr [rbp-16]", "    call foo",
                         "    mov r10d, r10d"}));
}

// 7k. 证明窗口边界：定值点距 store 恰 128 行（窗口内）-> 全删
TEST(PeepholeSlot, DwordProofWindowInclusiveAt128) {
    std::vector<std::string> lines;
    lines.push_back("    mov r10d, dword ptr [rbp-16]");
    for (int k = 0; k < 127; ++k) lines.push_back("    nop");
    lines.push_back("    mov dword ptr [rbp-8], r10d");
    lines.push_back("    mov r10d, dword ptr [rbp-8]");
    const std::string out = eliminateRedundantSlotTraffic(join(lines));
    std::string want = "    mov r10d, dword ptr [rbp-16]\n";
    for (int k = 0; k < 127; ++k) want += "    nop\n";
    EXPECT_EQ(out, want);
}

// 7l. 证明窗口边界：定值点距 store 129 行（窗口外）-> 证明失败，保留自搬
TEST(PeepholeSlot, DwordProofWindowExclusiveAt129) {
    std::vector<std::string> lines;
    lines.push_back("    mov r10d, dword ptr [rbp-16]");
    for (int k = 0; k < 128; ++k) lines.push_back("    nop");
    lines.push_back("    mov dword ptr [rbp-8], r10d");
    lines.push_back("    mov r10d, dword ptr [rbp-8]");
    const std::string out = eliminateRedundantSlotTraffic(join(lines));
    std::string want = "    mov r10d, dword ptr [rbp-16]\n";
    for (int k = 0; k < 128; ++k) want += "    nop\n";
    want += "    mov r10d, r10d\n";
    EXPECT_EQ(out, want);
}
