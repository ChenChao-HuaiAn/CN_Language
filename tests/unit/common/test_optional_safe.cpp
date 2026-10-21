// 单测：cn::Optional<T> 结果/可选类型化基建（#1303 一期）
// 覆盖：默认/nullopt 构造为空 / T 隐式构造有值 / has_value 判定 /
//       value 正常取值 / value_or 有值不取默认·空值取显形默认 /
//       整型与非可空默认形态。（坏值 abort 路径不测——会终止测试进程）
#include <gtest/gtest.h>

#include <string>

#include "cn_compiler/common/optional_safe.hpp"

using cn_compiler::cn::Optional;

// 默认构造=空（无值）
TEST(OptionalSafeTest, DefaultConstructIsEmpty) {
    const Optional<std::string> o;
    EXPECT_FALSE(o.has_value());
}

// nullopt 构造=空（显式无值）
TEST(OptionalSafeTest, NulloptConstructIsEmpty) {
    const Optional<std::string> o = std::nullopt;
    EXPECT_FALSE(o.has_value());
}

// T 隐式构造=有值（return 值 / 传参自然书写形态）
TEST(OptionalSafeTest, ImplicitConstructHasValue) {
    const Optional<std::string> o = std::string("整32");
    EXPECT_TRUE(o.has_value());
}

// value 正常取值：内容逐字一致
TEST(OptionalSafeTest, ValueReturnsStoredContent) {
    const Optional<std::string> o = std::string("结果文本");
    ASSERT_TRUE(o.has_value());
    EXPECT_EQ(o.value(), "结果文本");
}

// value_or：有值=存值（默认不参与）；空=调用点显形默认
TEST(OptionalSafeTest, ValueOrSelectsByPresence) {
    const Optional<std::string> filled = std::string("有值");
    EXPECT_EQ(filled.value_or("显形默认"), "有值");
    const Optional<std::string> empty = std::nullopt;
    EXPECT_EQ(empty.value_or("显形默认"), "显形默认");
}

// value_or 默认形态可为 const char* 字面量（可构造转发）
TEST(OptionalSafeTest, ValueOrAcceptsLiteralDefault) {
    const Optional<std::string> empty = std::nullopt;
    const std::string r = empty.value_or("字面量默认");
    EXPECT_EQ(r, "字面量默认");
}

// 整型承载：隐式构造 + has_value + value 往返
TEST(OptionalSafeTest, IntPayloadRoundTrip) {
    const Optional<int> o = 42;
    EXPECT_TRUE(o.has_value());
    EXPECT_EQ(o.value(), 42);
    const Optional<int> e = std::nullopt;
    EXPECT_FALSE(e.has_value());
    EXPECT_EQ(e.value_or(-1), -1);
}

// 空串值与无值是两态（类型层面区分——空串是合法业务值不再充哨兵）
TEST(OptionalSafeTest, EmptyStringIsValueNotAbsence) {
    const Optional<std::string> o = std::string("");
    EXPECT_TRUE(o.has_value());
    EXPECT_EQ(o.value(), "");
}
