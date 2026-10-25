// 发射后窥孔·就地入口单元测试（任务 1248 审查 P2-3）
// 覆盖：三后端就地入口与值语义入口逐字节一致；命中返回 true；未命中返回 false
//   且文本不动（含锚点存在但无可消除对、以及无锚点早退两种形态）。
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "cn_compiler/codegen/peephole_slot.hpp"

namespace {

using cn_compiler::peephole::eliminateRedundantSlotTraffic;
using cn_compiler::peephole::eliminateRedundantSlotTrafficAArch64;
using cn_compiler::peephole::eliminateRedundantSlotTrafficAArch64InPlace;
using cn_compiler::peephole::eliminateRedundantSlotTrafficInPlace;
using cn_compiler::peephole::eliminateRedundantSlotTrafficMasm;
using cn_compiler::peephole::eliminateRedundantSlotTrafficMasmInPlace;

std::string join(const std::vector<std::string>& lines) {
    std::string s;
    for (const auto& ln : lines) s += ln + "\n";
    return s;
}

} // namespace

// 1. linux_x86_64：异寄存器改写，就地结果 == 值语义结果
TEST(PeepholeSlotInPlace, GasMatchesValueSemantics) {
    const std::string in = join({
        "    push rbp",
        "    mov qword ptr [rbp-8], r10",
        "    mov r9, qword ptr [rbp-8]",
        "    mov qword ptr [rbp-16], r9",
    });
    const std::string want = eliminateRedundantSlotTraffic(in);
    std::string got = in;
    EXPECT_TRUE(eliminateRedundantSlotTrafficInPlace(got));
    EXPECT_EQ(got, want);
    // 1320 起：末尾 -16 的写无读=死槽写，一并消除
    EXPECT_EQ(got, join({"    push rbp", "    mov r9, r10"}));
}

// 2. win-x64（MASM 面）：同寄存器 64 位全删，就地结果 == 值语义结果
TEST(PeepholeSlotInPlace, MasmMatchesValueSemantics) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], rax",
        "    mov rax, qword ptr [rbp-8]",
        "    ret",
    });
    const std::string want = eliminateRedundantSlotTrafficMasm(in);
    std::string got = in;
    EXPECT_TRUE(eliminateRedundantSlotTrafficMasmInPlace(got));
    EXPECT_EQ(got, want);
    EXPECT_EQ(got, join({"    ret"}));
}

// 3. linux-arm64：str x3 -> ldr x9 改写，就地结果 == 值语义结果
TEST(PeepholeSlotInPlace, Arm64MatchesValueSemantics) {
    const std::string in = join({
        "    str x3, [x29,#-8]",
        "    ldr x9, [x29,#-8]",
    });
    const std::string want = eliminateRedundantSlotTrafficAArch64(in);
    std::string got = in;
    EXPECT_TRUE(eliminateRedundantSlotTrafficAArch64InPlace(got));
    EXPECT_EQ(got, want);
    EXPECT_EQ(got, "    mov x9, x3\n");
}

// 4. 全删（同寄存器同宽）：就地返回 true 且与值语义一致（产物为空）
TEST(PeepholeSlotInPlace, FullDeleteMatchesValueSemantics) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    mov r10, qword ptr [rbp-8]",
    });
    EXPECT_EQ(eliminateRedundantSlotTraffic(in), "");
    std::string got = in;
    EXPECT_TRUE(eliminateRedundantSlotTrafficInPlace(got));
    EXPECT_EQ(got, "");
}

// 5. 锚点存在但无可消除对（源寄存器被改写阻断转发、槽亦非死写）：返回 false
//    且文本逐字节不动
TEST(PeepholeSlotInPlace, MissKeepsTextUntouched) {
    const std::string in = join({
        "    mov qword ptr [rbp-8], r10",
        "    add r10, 1",
        "    mov r9, qword ptr [rbp-8]",
        "    mov qword ptr [rbp-16], r11",
        "    add r11, 1",
        "    mov r12, qword ptr [rbp-16]",
    });
    std::string got = in;
    EXPECT_FALSE(eliminateRedundantSlotTrafficInPlace(got));
    EXPECT_EQ(got, in);
}

// 6. 无锚点早退：返回 false 且文本逐字节不动
TEST(PeepholeSlotInPlace, NoAnchorKeepsTextUntouched) {
    const std::string in = join({
        "    push rbp",
        "    mov r10, 1",
        "    ret",
    });
    std::string got = in;
    EXPECT_FALSE(eliminateRedundantSlotTrafficInPlace(got));
    EXPECT_EQ(got, in);
}

// 7. 32 位同槽同宽高 32 零证明全删：就地入口与值语义入口一致
TEST(PeepholeSlotInPlace, DwordProofDeleteMatchesValueSemantics) {
    const std::string in = join({
        "    mov r10d, dword ptr [rbp-16]",
        "    mov dword ptr [rbp-8], r10d",
        "    mov r10d, dword ptr [rbp-8]",
    });
    const std::string want = eliminateRedundantSlotTraffic(in);
    std::string got = in;
    EXPECT_TRUE(eliminateRedundantSlotTrafficInPlace(got));
    EXPECT_EQ(got, want);
    EXPECT_EQ(got, "    mov r10d, dword ptr [rbp-16]\n");
}

// 8. AArch64 两阶段同时命中（1217 审查 P3-4 定向锁）：第一阶段固定帧槽命中并
//    替换文本存储（ASan 面跨阶段复用旧视图即红），第二阶段在替换后的文本上命中
//    [x13] 大偏移间接对——就地结果=值语义、二次应用稳定（产物 mov x9, x3 /
//    mov x10, x4）。
TEST(PeepholeSlotInPlace, Arm64TwoPhaseBothHitMatchesValueSemantics) {
    const std::string in = join({
        "    str x3, [x29,#-8]",
        "    ldr x9, [x29,#-8]",
        "    sub x13, x29, #300",
        "    str x4, [x13]",
        "    sub x13, x29, #300",
        "    ldr x10, [x13]",
    });
    const std::string want = eliminateRedundantSlotTrafficAArch64(in);
    EXPECT_EQ(want, join({"    mov x9, x3", "    mov x10, x4"}));
    std::string got = in;
    EXPECT_TRUE(eliminateRedundantSlotTrafficAArch64InPlace(got));
    EXPECT_EQ(got, want);
    // 二次应用：产物已无固定帧槽对与 [x13] 间接对，返回 false 且文本逐字节不动
    std::string again = got;
    EXPECT_FALSE(eliminateRedundantSlotTrafficAArch64InPlace(again));
    EXPECT_EQ(again, got);
}
