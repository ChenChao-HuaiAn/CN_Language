// 单测：全局字符串驻留表（Task #1154 阶段A）
// 覆盖：空串固定 ID / 同串同 ID / 异串异 ID / ID→文本往返（含 UTF-8 与堆串）/
//       find 命中与未命中哨兵 / 增量驻留后既有文本引用稳定。
#include <gtest/gtest.h>

#include <string>

#include "cn_compiler/common/string_intern.hpp"

using cn_compiler::intern::find;
using cn_compiler::intern::intern;
using cn_compiler::intern::InternId;
using cn_compiler::intern::kEmptyId;
using cn_compiler::intern::kInvalidId;
using cn_compiler::intern::size;
using cn_compiler::intern::text;

// 空串恒为固定 ID 0（intern 与 find 同口径）
TEST(StringInternTest, EmptyStringIsFixedZero) {
    EXPECT_EQ(intern(""), kEmptyId);
    EXPECT_EQ(find(""), kEmptyId);
    EXPECT_EQ(text(kEmptyId), "");
}

// 同内容多次驻留返回同一 ID
TEST(StringInternTest, SameContentSameId) {
    const InternId a = intern("整32");
    const InternId b = intern(std::string("整32"));
    const InternId c = intern(std::string_view("整32"));
    EXPECT_EQ(a, b);
    EXPECT_EQ(b, c);
    EXPECT_NE(a, kInvalidId);
}

// 不同内容得到不同 ID（单射）
TEST(StringInternTest, DistinctContentDistinctIds) {
    const InternId a = intern("整32");
    const InternId b = intern("整64");
    const InternId c = intern("整32 ");
    EXPECT_NE(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(b, c);
}

// ID -> 文本往返：内容逐字节一致
TEST(StringInternTest, RoundTripText) {
    const std::string original = "结果$整32$字符串";
    const InternId id = intern(original);
    EXPECT_EQ(text(id), original);
}

// 长串（超出 SSO 的堆分配文本）同样往返一致
TEST(StringInternTest, LongTextRoundTrip) {
    const std::string original(512, 'x');
    const InternId id = intern(original);
    EXPECT_EQ(text(id), original);
    EXPECT_EQ(intern(original), id);
}

// UTF-8 多字节文本往返一致
TEST(StringInternTest, Utf8RoundTrip) {
    const std::string original = "向量$映射$整64$字符串";
    const InternId id = intern(original);
    EXPECT_EQ(text(id), original);
}

// find 命中返回 intern 分配的同一 ID；未驻留返回显式哨兵
TEST(StringInternTest, FindHitAndMiss) {
    const std::string content = "未登记前先查询_唯一串_1154";
    EXPECT_EQ(find(content), kInvalidId);
    const InternId id = intern(content);
    EXPECT_EQ(find(content), id);
    EXPECT_EQ(find("绝不存在的驻留串_1154"), kInvalidId);
}

// 增量驻留不使既有 ID->文本引用失效（deque 稳定引用承诺）
TEST(StringInternTest, TextReferenceStableAfterGrowth) {
    const InternId id = intern("稳定性锚点_1154");
    const std::string& ref = text(id);
    for (int i = 0; i < 64; ++i) {
        intern("增长填充串_" + std::to_string(i));
    }
    EXPECT_EQ(ref, "稳定性锚点_1154");
    EXPECT_EQ(&ref, &text(id));  // 同一存储地址（未被搬移）
}

// 驻留条目数随新内容单调增长（重复驻留不增长）
TEST(StringInternTest, SizeGrowsOnlyForNewContent) {
    const std::size_t before = size();
    const InternId id = intern("尺寸计数锚点_1154");
    const std::size_t afterFirst = size();
    EXPECT_GE(afterFirst, before + 1);
    EXPECT_EQ(intern("尺寸计数锚点_1154"), id);
    EXPECT_EQ(size(), afterFirst);
}
