// 发射后窥孔·string_view 生命周期 + 边界/随机语料回归（任务 1217）
// 覆盖：
//   1. 三方言 × 边界/畸形语料（空文本/CRLF/无尾换行/超长行/大量标签/大量空行/
//      同槽海量出现/截断 token）：就地入口与值语义入口逐字节一致、命中标志与
//      文本变化同步、未命中时文本内容与存储均不动（零复制锁）；
//   2. 确定性随机语料（混合三方言合法/非法行、LF/CRLF、有/无尾换行）跨入口一致，
//      并做二次应用稳定性核对；
//   3. 就地改写后旧视图失效：ASan 构建以 EXPECT_DEATH 检出 heap-use-after-free
//      （正常路径=本文件其余用例全绿）；死亡例口径仅 Linux/WSL 的 ASan 构建纳入，
//      非 ASan 构建与 Windows（含 MSVC ASan 构建）显式跳过并指向机械锁脚本
//      scripts/check_peephole_sanitizer.py。
// 生命周期契约见 peephole_slot.hpp / peephole_slot_internal.hpp：行/令牌/槽视图
//   一律指向调用期 functionText 缓冲；内核先销毁全部视图、后替换源存储。
#include <gtest/gtest.h>

#include <cstddef>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "cn_compiler/codegen/peephole_slot.hpp"

namespace {

using cn_compiler::peephole::eliminateRedundantSlotTraffic;
using cn_compiler::peephole::eliminateRedundantSlotTrafficAArch64;
using cn_compiler::peephole::eliminateRedundantSlotTrafficAArch64InPlace;
using cn_compiler::peephole::eliminateRedundantSlotTrafficInPlace;
using cn_compiler::peephole::eliminateRedundantSlotTrafficMasm;
using cn_compiler::peephole::eliminateRedundantSlotTrafficMasmInPlace;

// ASan 检出宏：GCC/Clang 用内建宏；Windows（含 MSVC ASan 构建）不纳入死亡例口径
#if defined(__SANITIZE_ADDRESS__)
#define CN_PEEPHOLE_ASAN 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define CN_PEEPHOLE_ASAN 1
#endif
#endif
#ifndef CN_PEEPHOLE_ASAN
#define CN_PEEPHOLE_ASAN 0
#endif
#if CN_PEEPHOLE_ASAN && !defined(_WIN32)
#define CN_PEEPHOLE_ASAN_DEATH 1
#else
#define CN_PEEPHOLE_ASAN_DEATH 0
#endif

// 方言三元组：名称 + 值语义入口 + 就地入口
struct Dialect {
    const char* name;
    std::string (*value)(const std::string&);
    bool (*inPlace)(std::string&);
};

const Dialect kDialects[] = {
    {"gas", eliminateRedundantSlotTraffic, eliminateRedundantSlotTrafficInPlace},
    {"masm", eliminateRedundantSlotTrafficMasm, eliminateRedundantSlotTrafficMasmInPlace},
    {"arm64", eliminateRedundantSlotTrafficAArch64,
     eliminateRedundantSlotTrafficAArch64InPlace},
};

std::string joinEol(const std::vector<std::string>& lines, const char* eol, bool trailing) {
    std::string s;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        s += lines[i];
        if (i + 1 < lines.size() || trailing) s += eol;
    }
    return s;
}

// 单例核心不变量：值语义/就地逐字节一致；命中标志 = 文本变化；未命中文本不动；
//   二次应用仍一致（锁住改写结果不依赖视图残留）。
void expectSameAsValue(const Dialect& d, const std::string& in) {
    const std::string want = d.value(in);
    std::string got = in;
    const bool hit = d.inPlace(got);
    ASSERT_EQ(got, want) << d.name << " 就地/值语义不一致";
    ASSERT_EQ(hit, want != in) << d.name << " 命中标志与文本变化不同步";
    if (!hit) {
        ASSERT_EQ(got, in) << d.name << " 未命中却改动了文本";
    }
    const std::string want2 = d.value(want);
    std::string got2 = got;
    d.inPlace(got2);
    ASSERT_EQ(got2, want2) << d.name << " 二次应用不一致";
}

} // namespace

// ==================== 1. 边界/畸形语料 ====================

TEST(PeepholeSlotLifetime, BoundaryCorpusValueEqualsInPlace) {
    std::vector<std::string> corpus;
    corpus.push_back("");                                    // 空文本
    corpus.push_back("\n");                                  // 仅换行
    corpus.push_back("\r\n");                                // 仅 CRLF
    corpus.push_back("x");                                   // 无尾换行单字符
    corpus.push_back("    mov qword ptr [rbp-8], r10");      // 无尾换行命中形态
    corpus.push_back("    mov qword ptr [rbp-8], r10\n");    // 有尾换行命中形态
    corpus.push_back("    mov qword ptr [rbp-8], r10\n    mov r10, qword ptr [rbp-8]");
    corpus.push_back("    str x3, [x29,#-8]");
    corpus.push_back("    str x3, [x29,#-8]\n    ldr x9, [x29,#-8]");
    // CRLF：识别面可能保守拒绝，但两入口必须一致且不得越界/悬垂
    corpus.push_back(joinEol({"    mov qword ptr [rbp-8], r10",
                              "    mov r10, qword ptr [rbp-8]"},
                             "\r\n", true));
    corpus.push_back(joinEol({"    str x3, [x29,#-8]", "    ldr x9, [x29,#-8]"},
                             "\r\n", false));
    // 超长行（锚点在行首与注释中两种）
    corpus.push_back("    mov qword ptr [rbp-8], r10 // " + std::string(200000, 'x'));
    corpus.push_back("// " + std::string(200000, 'y') + " [rbp-8]");
    // 大量标签/大量空行（证明扫描与相邻性判据均按行号工作）
    {
        std::vector<std::string> many;
        for (int i = 0; i < 20000; ++i) many.push_back("L" + std::to_string(i) + ":");
        many.push_back("    mov qword ptr [rbp-8], r10");
        many.push_back("    mov r10, qword ptr [rbp-8]");
        corpus.push_back(joinEol(many, "\n", true));
        std::vector<std::string> blank(20000, "");
        blank.push_back("    mov qword ptr [rbp-8], r10");
        blank.push_back("    mov r10, qword ptr [rbp-8]");
        corpus.push_back(joinEol(blank, "\n", false));
    }
    // 海量不同槽偏移：槽计数平表多次扩容后键视图仍指向源文本
    {
        std::vector<std::string> many;
        for (int i = 0; i < 20000; ++i) {
            many.push_back("    mov qword ptr [rbp-" + std::to_string(i) + "], r10");
        }
        corpus.push_back(joinEol(many, "\n", true));
    }
    // 截断/畸形 token：解析失败即原样保留，不得产生越界读取
    const std::vector<std::string> malformed = {
        "    mov qword ptr [rbp",       "    mov r10, qword ptr [rbp-8",
        "    mov qword ptr [rbp-8],",  "    mov , [rbp-8]",
        "    mov qword ptr [rbp-8], xmm0", "    movsd [rbp-8], r10",
        "    str x999, [x29,#-8]",     "    str x3, [x29,#]",
        "    str x3, [x29,#-]",        "    str x3, [x29,#-8",
        "    ldr x, [x29,#-8]",       "    ldr x9, [x29, #-8]",
        "    mov rax, qword ptr [rbp-+8]",
        "    mov rax, qword ptr [rbp-0000000000000000000008]",
        "    mov qword ptr [rbp-8], r10 // trailing",
        "\t\tstr\tx3, [x29,#-8]",
    };
    corpus.insert(corpus.end(), malformed.begin(), malformed.end());

    for (const std::string& in : corpus) {
        for (const Dialect& d : kDialects) expectSameAsValue(d, in);
    }
}

// 命中形态的产物锚（防「随机语料全未命中→测试空转」）
TEST(PeepholeSlotLifetime, BoundaryHitProductsExact) {
    EXPECT_EQ(eliminateRedundantSlotTraffic(
                  "    mov qword ptr [rbp-8], r10\n    mov r9, qword ptr [rbp-8]"),
              "    mov r9, r10");
    EXPECT_EQ(eliminateRedundantSlotTrafficMasm(
                  "    mov qword ptr [rbp-8], rax\n    mov rax, qword ptr [rbp-8]\n"),
              "");
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(
                  "    str x3, [x29,#-8]\n    ldr x9, [x29,#-8]\n"),
              "    mov x9, x3\n");
    // AArch64 大偏移间接寻址：两块物化 + store 整体删除，load 改写
    const std::string indirect =
        "    sub x13, x29, #16\n"
        "    str x3, [x13]\n"
        "    sub x13, x29, #16\n"
        "    ldr x9, [x13]\n";
    EXPECT_EQ(eliminateRedundantSlotTrafficAArch64(indirect), "    mov x9, x3\n");
}

// 未命中：文本内容与存储均不动（未命中零复制契约；缓冲指针/容量不变）
TEST(PeepholeSlotLifetime, MissKeepsStorageUntouched) {
    std::string text = joinEol({"    push rbp",
                                "    mov qword ptr [rbp-8], r10",
                                "    mov qword ptr [rbp-16], r11",
                                "    ret"},
                               "\n", true);
    text.reserve(text.size() + 64);  // 强制堆缓冲（超 SSO），指针稳定可观测
    const char* const dataBefore = text.data();
    const std::size_t sizeBefore = text.size();
    const std::size_t capBefore = text.capacity();
    EXPECT_FALSE(eliminateRedundantSlotTrafficInPlace(text));
    EXPECT_EQ(text.data(), dataBefore);
    EXPECT_EQ(text.size(), sizeBefore);
    EXPECT_EQ(text.capacity(), capBefore);
}

// ==================== 2. 确定性随机语料 ====================

namespace {

const std::vector<std::string>& randomStoreLines() {
    static const std::vector<std::string> v = {
        "    mov qword ptr [rbp-8], r10",  "    mov dword ptr [rbp-16], r11d",
        "    mov byte ptr [rbp-24], al",   "    movsd [rbp-32], xmm0",
        "    mov [rbp-40], rax",           "    movsxd rax, [rbp-48]",
        "    MOV QWORD PTR [rbp-56], R9",  "    mov word ptr [rbp-64], r8w",
        "    str x3, [x29,#-8]",           "    str w4, [x29,#-16]",
        "    strb w5, [x29,#-24]",         "    str d0, [x29,#-40]",
        "    str x7, [x13]"};
    return v;
}

const std::vector<std::string>& randomLoadLines() {
    static const std::vector<std::string> v = {
        "    mov r9, qword ptr [rbp-8]",    "    mov r10d, dword ptr [rbp-16]",
        "    movzx r11, byte ptr [rbp-24]", "    movsx r12b, byte ptr [rbp-24]",
        "    movsd xmm1, [rbp-32]",         "    ldr x9, [x29,#-8]",
        "    ldr w9, [x29,#-16]",           "    ldrb w9, [x29,#-24]",
        "    ldrsb x9, [x29,#-24]",         "    ldr d1, [x29,#-40]",
        "    ldr x9, [x13]"};
    return v;
}

const std::vector<std::string>& randomFillerLines() {
    static const std::vector<std::string> v = {
        "    push rbp",         "    mov r10, 1",        "    add r10, 3",
        "Lok:",              "Lerr:",              ".text",
        "",                  "    // mov qword ptr [rbp-8], r10",
        "    // str x3, [x29,#-8]", "    sub x13, x29, #16",
        "    movz x13, #0x10", "    movk x13, #0, lsl #16",
        "    b Lok",           "    ret",              "    mov [rbp-8], rax",
        "    mov rax, [rbp-8]", "    STR X3, [x29,#-8]", "    str x3,[x29,#-8]",
        "    ldr x9,[x29,#-8]"};
    return v;
}

const std::vector<std::string>& randomMalformedLines() {
    static const std::vector<std::string> v = {
        "    mov qword ptr [rbp",     "    mov r10, qword ptr [rbp-8",
        "    mov qword ptr [rbp-8],", "    mov , [rbp-8]",
        "    mov xmm, 1",             "    mov qword ptr [rbp-8], xmm0",
        "    str x999, [x29,#-8]",    "    str x3, [x29,#]",
        "    str x3, [x29, #-8]",     "    [rbp-8]",          "xmm999"};
    return v;
}

class DocGenerator {
public:
    explicit DocGenerator(unsigned long long seed) : rng_(seed) {}

    std::string make() {
        const std::size_t n = 1 + static_cast<std::size_t>(next() % 40);
        std::vector<std::string> lines;
        lines.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            const std::size_t k = static_cast<std::size_t>(next() % 10);
            if (k < 3) lines.push_back(pick(randomStoreLines()));
            else if (k < 6) lines.push_back(pick(randomLoadLines()));
            else if (k < 9) lines.push_back(pick(randomFillerLines()));
            else lines.push_back(pick(randomMalformedLines()));
        }
        const char* eol = (next() % 3 == 0) ? "\r\n" : "\n";
        const bool trailing = (next() % 2) == 0;
        return joinEol(lines, eol, trailing);
    }

private:
    unsigned long long next() { return rng_(); }
    const std::string& pick(const std::vector<std::string>& v) {
        return v[static_cast<std::size_t>(next() % v.size())];
    }

    std::mt19937_64 rng_;
};

} // namespace

TEST(PeepholeSlotLifetime, DeterministicRandomCorpusValueEqualsInPlace) {
    DocGenerator gen(1217);
    for (int round = 0; round < 500; ++round) {
        const std::string doc = gen.make();
        for (const Dialect& d : kDialects) expectSameAsValue(d, doc);
    }
}

// ==================== 3. 旧视图失效（ASan 专项） ====================

#if CN_PEEPHOLE_ASAN_DEATH
// 就地命中会替换缓冲：命中前取得的旧视图在返回后读取=heap-use-after-free。
//   本用例只在 ASan/UBSan 构建执行（EXPECT_DEATH 匹配 ASan 报告）；正常路径
//   由本文件其余用例保证绿。机械锁入口=scripts/check_peephole_sanitizer.py。
TEST(PeepholeSlotLifetime, OldViewAfterGasInPlaceIsAsanRed) {
    std::string text;
    for (int i = 0; i < 300; ++i) text += "    nop\n";  // 超 SSO，强制堆缓冲
    text += "    mov qword ptr [rbp-8], r10\n    mov r9, qword ptr [rbp-8]\n";
    const std::string_view oldView(text);
    ASSERT_TRUE(eliminateRedundantSlotTrafficInPlace(text)) << "前置：样本须命中";
    EXPECT_DEATH(
        {
            volatile char sink = oldView[oldView.size() - 1];  // volatile：抑制读消除
            (void)sink;
        },
        "heap-use-after-free");
}

TEST(PeepholeSlotLifetime, OldViewAfterArm64IndirectInPlaceIsAsanRed) {
    std::string text;
    for (int i = 0; i < 300; ++i) text += "    nop\n";  // 超 SSO，强制堆缓冲
    text +=
        "    sub x13, x29, #16\n"
        "    str x3, [x13]\n"
        "    sub x13, x29, #16\n"
        "    ldr x9, [x13]\n";
    const std::string_view oldView(text);
    ASSERT_TRUE(eliminateRedundantSlotTrafficAArch64InPlace(text)) << "前置：样本须命中";
    EXPECT_DEATH(
        {
            volatile char sink = oldView[oldView.size() - 1];  // volatile：抑制读消除
            (void)sink;
        },
        "heap-use-after-free");
}
#else
// 死亡例口径仅 Linux/WSL ASan 构建纳入：其他构建（含 Windows/MSVC 的 ASan 构建）
//   显式登记诚实边界（不静默跳过），并指向独立机械锁脚本。
TEST(PeepholeSlotLifetime, OldViewAfterInPlaceDetectableOnlyUnderAsan) {
    GTEST_SKIP() << "旧视图 UAF 死亡例只在 Linux/WSL ASan 口径执行"
                    "（Windows 含 MSVC ASan 构建不纳入本锁）；"
                    "权威机械锁：python3 scripts/check_peephole_sanitizer.py";
}
#endif
