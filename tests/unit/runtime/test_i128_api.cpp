// i128 运行时辅助函数单元测试（Task 完善A：规格书10.5）
// 覆盖：128位加减乘除取余（有符号/无符号）、比较、转换、打印
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>

#include "runtime/runtime.hpp"

namespace {

// 便捷构造：低64位 + 高64位 -> 16字节缓冲（可拷贝的 std::array）
using U128 = std::array<std::uint64_t, 2>;

// 捕获输出
std::string capturePrint(void (*func)(const std::uint64_t*), const std::uint64_t* v) {
    testing::internal::CaptureStdout();
    func(v);
    return testing::internal::GetCapturedStdout();
}

} // namespace

// ==================== 加法/减法 ====================

// 128位加法：低位进位传播到高位
TEST(I128ApiTest, AddCarryPropagation) {
    U128 a = {0xFFFFFFFFFFFFFFFFULL, 0};
    U128 b = {1, 0};
    U128 out = {0, 0};
    __cn_add_i128(a.data(), b.data(), out.data());
    EXPECT_EQ(out[0], 0);                    // 低64位回绕
    EXPECT_EQ(out[1], 1);                    // 进位到高64位
}

// 128位减法：低位借位传播到高位
TEST(I128ApiTest, SubBorrowPropagation) {
    U128 a = {0, 1};                         // 2^64
    U128 b = {1, 0};
    U128 out = {0, 0};
    __cn_sub_i128(a.data(), b.data(), out.data());
    EXPECT_EQ(out[0], 0xFFFFFFFFFFFFFFFFULL); // 借位回绕
    EXPECT_EQ(out[1], 0);
}

// ==================== 乘法 ====================

// 128位乘法：2^64 * 2^64 = 2^128（超出128位，低128位=0）
TEST(I128ApiTest, MulOverflowToHigh) {
    U128 a = {0, 1};                         // 2^64
    U128 b = {0, 1};                         // 2^64
    U128 out = {0, 0};
    __cn_mul_u128(a.data(), b.data(), out.data());
    EXPECT_EQ(out[0], 0);                    // 低64位
    EXPECT_EQ(out[1], 0);                    // 高64位（2^128 溢出，仅低128位）
}

// 128位乘法：大数乘法（低64位相乘的高位部分进高位）
TEST(I128ApiTest, MulCrossTerms) {
    U128 a = {0xFFFFFFFFFFFFFFFFULL, 0};
    U128 b = {2, 0};
    U128 out = {0, 0};
    __cn_mul_u128(a.data(), b.data(), out.data());
    EXPECT_EQ(out[0], 0xFFFFFFFFFFFFFFFEULL);
    EXPECT_EQ(out[1], 1);
}

// ==================== 除法/取余 ====================

// 128位除法：2^64 / 2 = 2^63
TEST(I128ApiTest, DivHighWord) {
    U128 a = {0, 1};                         // 2^64
    U128 b = {2, 0};
    U128 out = {0, 0};
    __cn_div_u128(a.data(), b.data(), out.data());
    EXPECT_EQ(out[0], 0x8000000000000000ULL); // 2^63
    EXPECT_EQ(out[1], 0);
}

// 128位除法：大数除法 (2^64+1) / 3
TEST(I128ApiTest, DivBigNumber) {
    U128 a = {1, 1};                         // 2^64 + 1
    U128 b = {3, 0};
    U128 out = {0, 0};
    __cn_div_u128(a.data(), b.data(), out.data());
    EXPECT_EQ(out[0], 0x5555555555555555ULL); // (2^64+1)/3
    EXPECT_EQ(out[1], 0);
}

// 128位取余：大数取余
TEST(I128ApiTest, ModBigNumber) {
    U128 a = {10, 1};                        // 2^64 + 10
    U128 b = {3, 0};
    U128 out = {0, 0};
    __cn_mod_u128(a.data(), b.data(), out.data());
    EXPECT_EQ(out[0], 2);                    // (2^64+10) % 3 = 2
    EXPECT_EQ(out[1], 0);
}

// 有符号除法：负数 / 正数
TEST(I128ApiTest, DivSignedNegative) {
    U128 a = {0, 0x8000000000000000ULL};     // -2^127
    U128 b = {2, 0};
    U128 out = {0, 0};
    __cn_div_i128(a.data(), b.data(), out.data());
    // -2^127 / 2 = -2^126 = 高64位 0xC000000000000000
    EXPECT_EQ(out[0], 0);
    EXPECT_EQ(out[1], 0xC000000000000000ULL);
}

// ==================== 比较 ====================

// 有符号比较：负数 < 正数
TEST(I128ApiTest, CmpSignedNegative) {
    U128 neg = {0, 0x8000000000000000ULL};   // -2^127
    U128 pos = {0, 0x7FFFFFFFFFFFFFFFULL};   // 2^127-1
    EXPECT_LT(__cn_cmp_i128(neg.data(), pos.data()), 0);
    EXPECT_GT(__cn_cmp_i128(pos.data(), neg.data()), 0);
    EXPECT_EQ(__cn_cmp_i128(neg.data(), neg.data()), 0);
}

// 无符号比较：高位大的更大
TEST(I128ApiTest, CmpUnsigned) {
    U128 small = {0xFFFFFFFFFFFFFFFFULL, 0};
    U128 big = {0, 1};                       // 2^64
    EXPECT_LT(__cn_cmp_u128(small.data(), big.data()), 0);
}

// ==================== 转换 ====================

// i128 转浮点：2^64 转 double
TEST(I128ApiTest, I128ToF64) {
    U128 v = {0, 1};                         // 2^64
    const double d = __cn_i128_to_f64(v.data());
    EXPECT_DOUBLE_EQ(d, 18446744073709551616.0);
}

// 浮点转 i128：整数值
TEST(I128ApiTest, F64ToI128) {
    U128 out = {0, 0};
    __cn_f64_to_i128(42.0, out.data());
    EXPECT_EQ(out[0], 42);
    EXPECT_EQ(out[1], 0);
}

// ==================== 打印 ====================

// 打印有符号128位：正数
TEST(I128ApiTest, PrintI128Positive) {
    U128 v = {1234567890123456789ULL, 0};
    EXPECT_EQ(capturePrint(printLineI128, v.data()), "1234567890123456789\n");
}

// 打印有符号128位：负数（-1 = 全64位1）
TEST(I128ApiTest, PrintI128Negative) {
    U128 v = {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};  // -1
    EXPECT_EQ(capturePrint(printLineI128, v.data()), "-1\n");
}

// 打印无符号128位：最大边界
TEST(I128ApiTest, PrintU128Max) {
    U128 v = {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
    const std::string out = capturePrint(printLineU128, v.data());
    // 2^128-1 = 340282366920938463463374607431768211455
    EXPECT_EQ(out, "340282366920938463463374607431768211455\n");
}

// ==================== 边界值 ====================

// 2^127-1 打印（最大有符号128位）
TEST(I128ApiTest, PrintI128Max) {
    U128 v = {0xFFFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL};
    const std::string out = capturePrint(printLineI128, v.data());
    EXPECT_EQ(out, "170141183460469231731687303715884105727\n");
}
