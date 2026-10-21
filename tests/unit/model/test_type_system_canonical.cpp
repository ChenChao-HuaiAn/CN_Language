// #1215 阶段B 机械锁：canonicalEquals / splitTypeSuffixView 与 canonical /
//   splitTypeSuffix 的逐值等价性（防两套规则静默分叉）。
// 背景：canonicalEquals 与 splitTypeSuffixView 是 canonical/splitTypeSuffix 的
//   无分配视图实现（热点路径去掉临时 std::string）。两套规则若各自演进（新增
//   类型别名、后缀规则变化），编译器行为会在无诊断的情况下分叉——本测试用
//   固定语料 + 系统性覆盖语料逐项对照两套实现，规则漂移即红。
// 判据：
//   ① 对每个类型 t 与每个期望串 e：canonicalEquals(t,e) == (canonical(t)==e)。
//   ② 对每个类型 t：splitTypeSuffix 与 splitTypeSuffixView 的 core/suffix 全等。
#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cn_compiler/model/type_system.hpp"

using namespace cn_compiler;

namespace {

// 固定语料：审查要求的最低覆盖面（空串/纯 * /多层 & /数组/别名/泛型实例名/
//   合成类型/结果可选前缀），另含边界畸形形态（不完整括号、非数字下标）。
std::vector<std::string> fixedCorpus() {
    return {
        "",
        "*", "&", "&&", "&&&", "**", "*&", "&*",
        "整32", "整数", "小数", "指针", "空类型", "字符串", "字符", "布尔",
        "整8", "整16", "整32", "整64", "整128", "正8", "正16", "正32", "正64",
        "正128", "浮32", "浮64",
        "结果<整32,整32>", "可选<整32>", "结果<映射<整64,整64>,整32>",
        "可选<结果<整32,整32>>", "映射<整64,整64>", "向量<字符串>",
        "向量$整32", "结果$整32$整32", "可选$整32", "映射$整64$整64",
        "向量$字符串", "甲::类", "甲::乙::类", "T", "类",
        "整32[10]", "整32[10][20]", "整32[10][20][30]", "整32[10]*",
        "整32*[10]", "整32&&", "整32*&", "整32&*", "整32[a]", "整32[]",
        "整32[10]x", "x]", "[", "]", "[10]", "[]",
        "整数*", "整数&", "整数[10]", "小数*", "指针&", "指针*",
        "结果<整32,整32>&", "结果<整32,整32>*", "可选<整32>[2]",
    };
}

// 覆盖语料：基础类型 × 后缀组合（含合法与非法的后缀文本），保证两套规则在
//   每一类分支（别名/&/*/数组/停止点）上都被对照。
std::vector<std::string> coverageCorpus() {
    const std::vector<std::string> bases = {
        "整数", "小数", "指针", "整32", "整8", "字符串", "空类型", "类", "T",
        "结果<整32,整32>", "可选<整32>", "映射<整64,整64>", "向量<字符串>",
        "结果$整32$整32", "可选$整32", "",
    };
    const std::vector<std::string> suffixes = {
        "", "*", "&", "&&", "&&&", "*&", "&*", "[10]", "[10][20]", "[a]",
        "[]", "[3]x", "*[10]", "&[2]", "*&*", "]",
    };
    std::vector<std::string> corpus;
    corpus.reserve(bases.size() * suffixes.size());
    for (const auto& base : bases) {
        for (const auto& suffix : suffixes) corpus.push_back(base + suffix);
    }
    return corpus;
}

// 确定性随机语料（固定种子 LCG）：覆盖前述规则未显式枚举的字符组合。
std::vector<std::string> generatedCorpus() {
    static const char kAlphabet[] = "abT<>[],*&0123456789 :";
    std::uint32_t seed = 132211u;
    std::vector<std::string> corpus;
    corpus.reserve(3000);
    for (int k = 0; k < 3000; ++k) {
        seed = seed * 1103515245u + 12345u;
        const std::size_t len = static_cast<std::size_t>((seed >> 16) % 11u);
        std::string text;
        text.reserve(len);
        for (std::size_t i = 0; i < len; ++i) {
            seed = seed * 1103515245u + 12345u;
            text.push_back(kAlphabet[(seed >> 16) % (sizeof(kAlphabet) - 1)]);
        }
        corpus.push_back(std::move(text));
    }
    return corpus;
}

std::vector<std::string> allCorpus() {
    std::vector<std::string> corpus = fixedCorpus();
    const std::vector<std::string> coverage = coverageCorpus();
    corpus.insert(corpus.end(), coverage.begin(), coverage.end());
    const std::vector<std::string> generated = generatedCorpus();
    corpus.insert(corpus.end(), generated.begin(), generated.end());
    return corpus;
}

// canonical 的全部输出（保证每个类型至少有一个正例期望）+ 审查点名的字面量。
std::vector<std::string> expectedCandidates(const std::vector<std::string>& corpus) {
    std::set<std::string> unique;
    for (const auto& type : corpus) unique.insert(types::canonical(type));
    for (const char* literal : {
             "", "整32", "浮64", "空类型*", "字符串", "字符串*", "字符", "布尔",
             "整8", "整16", "整64", "整128", "浮32", "类", "结果<整32,整32>",
             "可选<整32>", "整32*", "整32&", "结果$整32$整32", "可选$整32",
         }) {
        unique.insert(literal);
    }
    return {unique.begin(), unique.end()};
}

}  // namespace

// ① 无分配等值判定与 canonical 逐值等价（全语料 × 全期望）。
TEST(TypeCanonicalViewTest, CanonicalEqualsMatchesCanonical) {
    const std::vector<std::string> corpus = allCorpus();
    const std::vector<std::string> expected = expectedCandidates(corpus);
    std::size_t checked = 0;
    for (const auto& type : corpus) {
        const std::string canonical = types::canonical(type);
        for (const auto& candidate : expected) {
            const bool want = canonical == candidate;
            const bool got = types::canonicalEquals(type, candidate);
            ASSERT_EQ(want, got)
                << "类型=[" << type << "] 期望=[" << candidate
                << "] canonical=[" << canonical << "]";
            ++checked;
        }
    }
    EXPECT_GE(checked, 5000u);  // 语料规模下限（防测试被空语料静默弱化）
}

// ② 视图切分与双串切分逐值等价（core/suffix 全等）。
TEST(TypeCanonicalViewTest, SplitTypeSuffixViewMatchesLegacy) {
    const std::vector<std::string> corpus = allCorpus();
    for (const auto& type : corpus) {
        std::string oldCore;
        std::string oldSuffix;
        types::splitTypeSuffix(type, oldCore, oldSuffix);
        std::string_view newCore;
        std::string_view newSuffix;
        types::splitTypeSuffixView(type, newCore, newSuffix);
        ASSERT_EQ(oldCore, std::string(newCore)) << "core 不等：类型=[" << type << "]";
        ASSERT_EQ(oldSuffix, std::string(newSuffix)) << "suffix 不等：类型=[" << type << "]";
    }
}

// ③ 关键语义点显式断言（可读性：失败时直接指出哪条规则漂移）。
TEST(TypeCanonicalViewTest, AliasAndSuffixSemantics) {
    EXPECT_TRUE(types::canonicalEquals("整数", "整32"));
    EXPECT_FALSE(types::canonicalEquals("整数", "整数"));
    EXPECT_TRUE(types::canonicalEquals("小数", "浮64"));
    EXPECT_TRUE(types::canonicalEquals("指针", "空类型*"));
    EXPECT_TRUE(types::canonicalEquals("整32&&&", "整32"));
    EXPECT_TRUE(types::canonicalEquals("整数*", "整32*"));
    EXPECT_TRUE(types::canonicalEquals("整数*[10]", "整32*[10]"));
    EXPECT_TRUE(types::canonicalEquals("整32[10][20]", "整32[10][20]"));
    EXPECT_FALSE(types::canonicalEquals("整32[10][20]", "整32[10]"));
    EXPECT_FALSE(types::canonicalEquals("整32[a]", "整32"));
    EXPECT_FALSE(types::canonicalEquals("整32[]", "整32"));
    EXPECT_FALSE(types::canonicalEquals("整32[10]x", "整32[10]"));
    EXPECT_TRUE(types::canonicalEquals("可选<整32>", "可选<整32>"));
    EXPECT_FALSE(types::canonicalEquals("结果<整32,整32>", "可选<整32>"));
}
