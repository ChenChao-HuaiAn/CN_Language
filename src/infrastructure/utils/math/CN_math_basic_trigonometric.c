/******************************************************************************
 * 文件名: CN_math_basic_trigonometric.c
 * 功能: CN_Language基本数学运算模块三角函数部分实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现三角函数和高级函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_math_basic.h"
#include <math.h>

// ============================================================================
// 三角函数实现
// ============================================================================

float CN_math_sin_f(float angle)
{
    return sinf(angle);
}

double CN_math_sin_d(double angle)
{
    return sin(angle);
}

float CN_math_cos_f(float angle)
{
    return cosf(angle);
}

double CN_math_cos_d(double angle)
{
    return cos(angle);
}

float CN_math_tan_f(float angle)
{
    return tanf(angle);
}

double CN_math_tan_d(double angle)
{
    return tan(angle);
}

float CN_math_asin_f(float value)
{
    return asinf(value);
}

double CN_math_asin_d(double value)
{
    return asin(value);
}

float CN_math_acos_f(float value)
{
    return acosf(value);
}

double CN_math_acos_d(double value)
{
    return acos(value);
}

float CN_math_atan_f(float value)
{
    return atanf(value);
}

double CN_math_atan_d(double value)
{
    return atan(value);
}

float CN_math_atan2_f(float y, float x)
{
    return atan2f(y, x);
}

double CN_math_atan2_d(double y, double x)
{
    return atan2(y, x);
}

// ============================================================================
// 双曲函数实现
// ============================================================================

float CN_math_sinh_f(float value)
{
    return sinhf(value);
}

double CN_math_sinh_d(double value)
{
    return sinh(value);
}

float CN_math_cosh_f(float value)
{
    return coshf(value);
}

double CN_math_cosh_d(double value)
{
    return cosh(value);
}

float CN_math_tanh_f(float value)
{
    return tanhf(value);
}

double CN_math_tanh_d(double value)
{
    return tanh(value);
}

// ============================================================================
// 角度转换函数实现
// ============================================================================

float CN_math_deg_to_rad_f(float degrees)
{
    return degrees * (CN_MATH_PI / 180.0f);
}

double CN_math_deg_to_rad_d(double degrees)
{
    return degrees * (CN_MATH_PI / 180.0);
}

float CN_math_rad_to_deg_f(float radians)
{
    return radians * (180.0f / CN_MATH_PI);
}

double CN_math_rad_to_deg_d(double radians)
{
    return radians * (180.0 / CN_MATH_PI);
}

// ============================================================================
// 舍入函数实现
// ============================================================================

float CN_math_ceil_f(float value)
{
    return ceilf(value);
}

double CN_math_ceil_d(double value)
{
    return ceil(value);
}

float CN_math_floor_f(float value)
{
    return floorf(value);
}

double CN_math_floor_d(double value)
{
    return floor(value);
}

float CN_math_round_f(float value)
{
    return roundf(value);
}

double CN_math_round_d(double value)
{
    return round(value);
}

float CN_math_trunc_f(float value)
{
    return truncf(value);
}

double CN_math_trunc_d(double value)
{
    return trunc(value);
}

// ============================================================================
// 模运算实现
// ============================================================================

float CN_math_fmod_f(float x, float y)
{
    return fmodf(x, y);
}

double CN_math_fmod_d(double x, double y)
{
    return fmod(x, y);
}

float CN_math_remainder_f(float x, float y)
{
    return remainderf(x, y);
}

double CN_math_remainder_d(double x, double y)
{
    return remainder(x, y);
}
