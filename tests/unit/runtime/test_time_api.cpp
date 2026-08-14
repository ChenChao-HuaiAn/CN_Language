// 时间运行时 API 单元测试（Task 6.5，规格书10.4 时间；对标 C++ chrono/ctime）
// 覆盖：__cn_time（时间戳>0、递增）/
//       __cn_clock_ms（单调递增、非负）/
//       __cn_time_format（格式正确、长度/分隔符断言、非法时间戳 nullptr）
// 测试名英文（GCC 7 不支持中文标识符）；中文仅注释
#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "runtime/runtime.hpp"

namespace {

// 断言格式化结果形如 "YYYY-MM-DD"（长度10，第4/7位为 '-'）
void expectDateDashFormat(const char* text) {
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(std::strlen(text), 10u);
    EXPECT_EQ(text[4], '-');
    EXPECT_EQ(text[7], '-');
}

} // namespace

// ==================== 1. 当前时间戳 ====================

// 时间戳大于 0（Unix 纪元后）
TEST(TimeApiTest, TimePositive) {
    EXPECT_GT(__cn_time(), 0LL);
}

// 两次调用：第二次 ≥ 第一次（秒级，同秒内可能相等）
TEST(TimeApiTest, TimeMonotonicNonDecreasing) {
    const long long t1 = __cn_time();
    const long long t2 = __cn_time();
    EXPECT_GE(t2, t1);
}

// ==================== 2. 单调时钟毫秒 ====================

// 时钟毫秒非负
TEST(TimeApiTest, ClockMsNonNegative) {
    EXPECT_GE(__cn_clock_ms(), 0LL);
}

// 两次调用：第二次 ≥ 第一次（单调时钟，毫秒级）
TEST(TimeApiTest, ClockMsMonotonicNonDecreasing) {
    const long long m1 = __cn_clock_ms();
    const long long m2 = __cn_clock_ms();
    EXPECT_GE(m2, m1);
}

// ==================== 3. 格式化时间 ====================

// "%Y-%m-%d"：长度10 + 分隔符位置正确
TEST(TimeApiTest, TimeFormatDateDash) {
    char* text = __cn_time_format(__cn_time(), "%Y-%m-%d");
    expectDateDashFormat(text);
    __cn_str_free(text);
}

// "%Y"：4 位数字年份
TEST(TimeApiTest, TimeFormatYearDigits) {
    char* text = __cn_time_format(__cn_time(), "%Y");
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(std::strlen(text), 4u);
    for (int i = 0; i < 4; ++i) {
        EXPECT_GE(text[i], '0');
        EXPECT_LE(text[i], '9');
    }
    __cn_str_free(text);
}

// 默认格式（fmt=nullptr）：回退 "%Y-%m-%d %H:%M:%S"（长度19）
TEST(TimeApiTest, TimeFormatDefaultFormat) {
    char* text = __cn_time_format(__cn_time(), nullptr);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(std::strlen(text), 19u);
    EXPECT_EQ(text[4], '-');
    EXPECT_EQ(text[7], '-');
    EXPECT_EQ(text[10], ' ');
    EXPECT_EQ(text[13], ':');
    __cn_str_free(text);
}

// 非法时间戳（负数）→ nullptr
TEST(TimeApiTest, TimeFormatInvalidTimestamp) {
    EXPECT_EQ(__cn_time_format(-1, "%Y"), nullptr);
}

// 合法时间戳固定验证：1970-01-01 00:00:00 UTC（本机时区 UTC+8 显示 1970-01-01）
// 只断言 "%Y-%m-%d" 长度与分隔符（避免时区差异导致日期断言脆弱）
TEST(TimeApiTest, TimeFormatEpoch) {
    char* text = __cn_time_format(0, "%Y-%m-%d");
    expectDateDashFormat(text);
    __cn_str_free(text);
}
