/******************************************************************************
 * 文件名: CN_math_core.c
 * 功能: CN_Language数学核心模块实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现数学核心功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_math_core.h"
#include <math.h>
#include <string.h>

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 检查浮点数是否为NaN（内部实现）
 */
static bool is_nan_f_impl(float value)
{
    return value != value;  // NaN不等于自身
}

/**
 * @brief 检查浮点数是否为无穷大（内部实现）
 */
static bool is_inf_f_impl(float value)
{
    return !is_nan_f_impl(value) && (value * 0 != 0);
}

/**
 * @brief 检查双精度浮点数是否为NaN（内部实现）
 */
static bool is_nan_d_impl(double value)
{
    return value != value;  // NaN不等于自身
}

/**
 * @brief 检查双精度浮点数是否为无穷大（内部实现）
 */
static bool is_inf_d_impl(double value)
{
    return !is_nan_d_impl(value) && (value * 0 != 0);
}

// ============================================================================
// 公共函数实现
// ============================================================================

bool CN_math_is_nan_f(float value)
{
    return is_nan_f_impl(value);
}

bool CN_math_is_inf_f(float value)
{
    return is_inf_f_impl(value);
}

bool CN_math_is_finite_f(float value)
{
    return !is_nan_f_impl(value) && !is_inf_f_impl(value);
}

bool CN_math_is_nan_d(double value)
{
    return is_nan_d_impl(value);
}

bool CN_math_is_inf_d(double value)
{
    return is_inf_d_impl(value);
}

bool CN_math_is_finite_d(double value)
{
    return !is_nan_d_impl(value) && !is_inf_d_impl(value);
}

bool CN_math_float_equal_f(float a, float b, float epsilon)
{
    // 处理NaN和无穷大的特殊情况
    if (is_nan_f_impl(a) || is_nan_f_impl(b))
    {
        return false;  // NaN不等于任何值，包括自身
    }
    
    if (is_inf_f_impl(a) && is_inf_f_impl(b))
    {
        // 两个无穷大相等当且仅当符号相同
        return (a > 0 && b > 0) || (a < 0 && b < 0);
    }
    
    // 常规浮点数比较
    float diff = a - b;
    if (diff < 0)
    {
        diff = -diff;
    }
    
    return diff <= epsilon;
}

bool CN_math_float_equal_d(double a, double b, double epsilon)
{
    // 处理NaN和无穷大的特殊情况
    if (is_nan_d_impl(a) || is_nan_d_impl(b))
    {
        return false;  // NaN不等于任何值，包括自身
    }
    
    if (is_inf_d_impl(a) && is_inf_d_impl(b))
    {
        // 两个无穷大相等当且仅当符号相同
        return (a > 0 && b > 0) || (a < 0 && b < 0);
    }
    
    // 常规双精度浮点数比较
    double diff = a - b;
    if (diff < 0)
    {
        diff = -diff;
    }
    
    return diff <= epsilon;
}

const char* CN_math_error_string(Eum_CN_MathError_t error)
{
    switch (error)
    {
        case Eum_MATH_SUCCESS:
            return "成功";
        case Eum_MATH_ERROR_OVERFLOW:
            return "溢出错误";
        case Eum_MATH_ERROR_UNDERFLOW:
            return "下溢错误";
        case Eum_MATH_ERROR_DIVISION_BY_ZERO:
            return "除零错误";
        case Eum_MATH_ERROR_INVALID_ARGUMENT:
            return "无效参数错误";
        case Eum_MATH_ERROR_DOMAIN:
            return "定义域错误";
        case Eum_MATH_ERROR_RANGE:
            return "值域错误";
        case Eum_MATH_ERROR_NAN:
            return "NaN错误";
        case Eum_MATH_ERROR_INFINITY:
            return "无穷大错误";
        case Eum_MATH_ERROR_NOT_IMPLEMENTED:
            return "未实现错误";
        default:
            return "未知错误";
    }
}
