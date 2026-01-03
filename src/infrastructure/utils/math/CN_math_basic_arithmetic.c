/******************************************************************************
 * 文件名: CN_math_basic_arithmetic.c
 * 功能: CN_Language基本数学运算模块算术部分实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现基本算术运算
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_math_basic.h"
#include <math.h>

// ============================================================================
// 基本算术运算实现
// ============================================================================

int32_t CN_math_abs_i32(int32_t value)
{
    return (value < 0) ? -value : value;
}

int64_t CN_math_abs_i64(int64_t value)
{
    return (value < 0) ? -value : value;
}

float CN_math_abs_f(float value)
{
    return fabsf(value);
}

double CN_math_abs_d(double value)
{
    return fabs(value);
}

int32_t CN_math_max_i32(int32_t a, int32_t b)
{
    return (a > b) ? a : b;
}

int64_t CN_math_max_i64(int64_t a, int64_t b)
{
    return (a > b) ? a : b;
}

float CN_math_max_f(float a, float b)
{
    return (a > b) ? a : b;
}

double CN_math_max_d(double a, double b)
{
    return (a > b) ? a : b;
}

int32_t CN_math_min_i32(int32_t a, int32_t b)
{
    return (a < b) ? a : b;
}

int64_t CN_math_min_i64(int64_t a, int64_t b)
{
    return (a < b) ? a : b;
}

float CN_math_min_f(float a, float b)
{
    return (a < b) ? a : b;
}

double CN_math_min_d(double a, double b)
{
    return (a < b) ? a : b;
}

int32_t CN_math_clamp_i32(int32_t value, int32_t min, int32_t max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

int64_t CN_math_clamp_i64(int64_t value, int64_t min, int64_t max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float CN_math_clamp_f(float value, float min, float max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

double CN_math_clamp_d(double value, double min, double max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float CN_math_lerp_f(float a, float b, float t)
{
    // 确保t在[0,1]范围内
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    return a + t * (b - a);
}

double CN_math_lerp_d(double a, double b, double t)
{
    // 确保t在[0,1]范围内
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    
    return a + t * (b - a);
}

// ============================================================================
// 幂运算实现
// ============================================================================

int32_t CN_math_square_i32(int32_t value)
{
    return value * value;
}

int64_t CN_math_square_i64(int64_t value)
{
    return value * value;
}

float CN_math_square_f(float value)
{
    return value * value;
}

double CN_math_square_d(double value)
{
    return value * value;
}

int32_t CN_math_cube_i32(int32_t value)
{
    return value * value * value;
}

int64_t CN_math_cube_i64(int64_t value)
{
    return value * value * value;
}

float CN_math_cube_f(float value)
{
    return value * value * value;
}

double CN_math_cube_d(double value)
{
    return value * value * value;
}

float CN_math_pow_f(float base, float exponent)
{
    return powf(base, exponent);
}

double CN_math_pow_d(double base, double exponent)
{
    return pow(base, exponent);
}

float CN_math_sqrt_f(float value)
{
    return sqrtf(value);
}

double CN_math_sqrt_d(double value)
{
    return sqrt(value);
}

float CN_math_cbrt_f(float value)
{
    return cbrtf(value);
}

double CN_math_cbrt_d(double value)
{
    return cbrt(value);
}

// ============================================================================
// 对数运算实现
// ============================================================================

float CN_math_log_f(float value)
{
    return logf(value);
}

double CN_math_log_d(double value)
{
    return log(value);
}

float CN_math_log10_f(float value)
{
    return log10f(value);
}

double CN_math_log10_d(double value)
{
    return log10(value);
}

float CN_math_log2_f(float value)
{
    return log2f(value);
}

double CN_math_log2_d(double value)
{
    return log2(value);
}
