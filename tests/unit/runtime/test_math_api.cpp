// 数学运行时 API 单元测试（Task 6.3，规格书10.5 数学库）
// 覆盖：__cn_sqrt/__cn_pow/__cn_sin/__cn_cos/__cn_tan/
//       __cn_fabs/__cn_ceil/__cn_floor 的数值正确性
// 注意：测试名英文（GCC 7 不支持中文标识符）；中文仅注释
#include <gtest/gtest.h>
#include <cmath>

#include "runtime/runtime.hpp"

namespace {

// 浮点近似比较（绝对误差 + 相对误差，覆盖接近0的值）
bool nearEqual(double a, double b, double absEps = 1e-9, double relEps = 1e-9) {
    const double diff = std::fabs(a - b);
    if (diff <= absEps) return true;
    return diff <= relEps * std::fabs(b);
}

} // namespace

// 平方根：__cn_sqrt
TEST(MathApiTest, Sqrt) {
    EXPECT_NEAR(__cn_sqrt(9.0), 3.0, 1e-9);
    EXPECT_NEAR(__cn_sqrt(16.0), 4.0, 1e-9);
    EXPECT_NEAR(__cn_sqrt(2.0), std::sqrt(2.0), 1e-9);
    EXPECT_NEAR(__cn_sqrt(0.0), 0.0, 1e-12);
}

// 幂：__cn_pow
TEST(MathApiTest, Pow) {
    EXPECT_NEAR(__cn_pow(2.0, 3.0), 8.0, 1e-9);
    EXPECT_NEAR(__cn_pow(3.0, 2.0), 9.0, 1e-9);
    EXPECT_NEAR(__cn_pow(2.0, 10.0), 1024.0, 1e-9);
    EXPECT_NEAR(__cn_pow(5.0, 0.0), 1.0, 1e-9);
    EXPECT_NEAR(__cn_pow(10.0, -1.0), 0.1, 1e-9);
}

// 正弦：__cn_sin（弧度制）
TEST(MathApiTest, Sin) {
    EXPECT_NEAR(__cn_sin(0.0), 0.0, 1e-12);
    EXPECT_NEAR(__cn_sin(1.5707963267948966), 1.0, 1e-9);  // π/2
    EXPECT_NEAR(__cn_sin(3.141592653589793), 0.0, 1e-9);   // π
    EXPECT_NEAR(__cn_sin(6.283185307179586), 0.0, 1e-9);   // 2π
}

// 余弦：__cn_cos（弧度制）
TEST(MathApiTest, Cos) {
    EXPECT_NEAR(__cn_cos(0.0), 1.0, 1e-12);
    EXPECT_NEAR(__cn_cos(1.5707963267948966), 0.0, 1e-9);  // π/2
    EXPECT_NEAR(__cn_cos(3.141592653589793), -1.0, 1e-9);  // π
}

// 正切：__cn_tan（弧度制）
TEST(MathApiTest, Tan) {
    EXPECT_NEAR(__cn_tan(0.0), 0.0, 1e-12);
    EXPECT_NEAR(__cn_tan(0.7853981633974483), 1.0, 1e-9);  // π/4
}

// 绝对值：__cn_fabs
TEST(MathApiTest, Fabs) {
    EXPECT_NEAR(__cn_fabs(-5.5), 5.5, 1e-12);
    EXPECT_NEAR(__cn_fabs(5.5), 5.5, 1e-12);
    EXPECT_NEAR(__cn_fabs(0.0), 0.0, 1e-12);
    EXPECT_NEAR(__cn_fabs(-1e10), 1e10, 1e-6);
}

// 向上取整：__cn_ceil
TEST(MathApiTest, Ceil) {
    EXPECT_NEAR(__cn_ceil(3.2), 4.0, 1e-12);
    EXPECT_NEAR(__cn_ceil(3.0), 3.0, 1e-12);
    EXPECT_NEAR(__cn_ceil(-3.2), -3.0, 1e-12);
    EXPECT_NEAR(__cn_ceil(0.5), 1.0, 1e-12);
}

// 向下取整：__cn_floor
TEST(MathApiTest, Floor) {
    EXPECT_NEAR(__cn_floor(3.8), 3.0, 1e-12);
    EXPECT_NEAR(__cn_floor(3.0), 3.0, 1e-12);
    EXPECT_NEAR(__cn_floor(-3.2), -4.0, 1e-12);
    EXPECT_NEAR(__cn_floor(0.5), 0.0, 1e-12);
}

// 组合运算：与 std 函数一致（平台无关验证）
TEST(MathApiTest, CombinedConsistency) {
    // 平方根(幂(3,2)) == 3
    EXPECT_TRUE(nearEqual(__cn_sqrt(__cn_pow(3.0, 2.0)), 3.0));
    // 绝对值(向下取整(-3.2)) == 4
    EXPECT_TRUE(nearEqual(__cn_fabs(__cn_floor(-3.2)), 4.0));
    // 三角函数平方和：sin² + cos² == 1（随机角度）
    for (double a : {0.1, 0.5, 1.0, 2.0, 3.5}) {
        const double sum = __cn_sin(a) * __cn_sin(a) + __cn_cos(a) * __cn_cos(a);
        EXPECT_TRUE(nearEqual(sum, 1.0, 1e-9, 1e-9));
    }
}
