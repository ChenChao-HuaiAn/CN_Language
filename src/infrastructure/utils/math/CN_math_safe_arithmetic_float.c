/******************************************************************************
 * 文件名: CN_math_safe_arithmetic_float.c
 * 功能: CN_Language安全算术模块浮点数部分实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现浮点数安全算术运算
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_math_safe_arithmetic.h"
#include "CN_math_core.h"
#include <float.h>
#include <limits.h>
#include <math.h>

// ============================================================================
// 浮点数安全运算实现
// ============================================================================

bool CN_math_safe_add_f(float a, float b, float* result)
{
    // 检查NaN
    if (CN_math_is_nan_f(a) || CN_math_is_nan_f(b))
    {
        return false;
    }
    
    float sum = a + b;
    
    // 检查溢出/下溢
    if (CN_math_is_inf_f(sum) || CN_math_is_nan_f(sum))
    {
        return false;
    }
    
    // 检查精度损失（如果a和b数量级相差很大）
    if (sum == a && b != 0)
    {
        // 加法没有效果，可能精度损失
        return false;
    }
    
    *result = sum;
    return true;
}

bool CN_math_safe_sub_f(float a, float b, float* result)
{
    // 检查NaN
    if (CN_math_is_nan_f(a) || CN_math_is_nan_f(b))
    {
        return false;
    }
    
    float diff = a - b;
    
    // 检查溢出/下溢
    if (CN_math_is_inf_f(diff) || CN_math_is_nan_f(diff))
    {
        return false;
    }
    
    *result = diff;
    return true;
}

bool CN_math_safe_mul_f(float a, float b, float* result)
{
    // 检查NaN
    if (CN_math_is_nan_f(a) || CN_math_is_nan_f(b))
    {
        return false;
    }
    
    float product = a * b;
    
    // 检查溢出/下溢
    if (CN_math_is_inf_f(product) || CN_math_is_nan_f(product))
    {
        return false;
    }
    
    // 检查下溢（结果太小）
    if (product == 0 && a != 0 && b != 0)
    {
        return false;
    }
    
    *result = product;
    return true;
}

bool CN_math_safe_div_f(float a, float b, float* result)
{
    // 检查NaN
    if (CN_math_is_nan_f(a) || CN_math_is_nan_f(b))
    {
        return false;
    }
    
    // 检查除零
    if (b == 0.0f)
    {
        return false;
    }
    
    float quotient = a / b;
    
    // 检查溢出/下溢
    if (CN_math_is_inf_f(quotient) || CN_math_is_nan_f(quotient))
    {
        return false;
    }
    
    *result = quotient;
    return true;
}

// ============================================================================
// 双精度浮点数安全运算实现
// ============================================================================

bool CN_math_safe_add_d(double a, double b, double* result)
{
    // 检查NaN
    if (CN_math_is_nan_d(a) || CN_math_is_nan_d(b))
    {
        return false;
    }
    
    double sum = a + b;
    
    // 检查溢出/下溢
    if (CN_math_is_inf_d(sum) || CN_math_is_nan_d(sum))
    {
        return false;
    }
    
    // 检查精度损失（如果a和b数量级相差很大）
    if (sum == a && b != 0)
    {
        // 加法没有效果，可能精度损失
        return false;
    }
    
    *result = sum;
    return true;
}

bool CN_math_safe_sub_d(double a, double b, double* result)
{
    // 检查NaN
    if (CN_math_is_nan_d(a) || CN_math_is_nan_d(b))
    {
        return false;
    }
    
    double diff = a - b;
    
    // 检查溢出/下溢
    if (CN_math_is_inf_d(diff) || CN_math_is_nan_d(diff))
    {
        return false;
    }
    
    *result = diff;
    return true;
}

bool CN_math_safe_mul_d(double a, double b, double* result)
{
    // 检查NaN
    if (CN_math_is_nan_d(a) || CN_math_is_nan_d(b))
    {
        return false;
    }
    
    double product = a * b;
    
    // 检查溢出/下溢
    if (CN_math_is_inf_d(product) || CN_math_is_nan_d(product))
    {
        return false;
    }
    
    // 检查下溢（结果太小）
    if (product == 0 && a != 0 && b != 0)
    {
        return false;
    }
    
    *result = product;
    return true;
}

bool CN_math_safe_div_d(double a, double b, double* result)
{
    // 检查NaN
    if (CN_math_is_nan_d(a) || CN_math_is_nan_d(b))
    {
        return false;
    }
    
    // 检查除零
    if (b == 0.0)
    {
        return false;
    }
    
    double quotient = a / b;
    
    // 检查溢出/下溢
    if (CN_math_is_inf_d(quotient) || CN_math_is_nan_d(quotient))
    {
        return false;
    }
    
    *result = quotient;
    return true;
}

// ============================================================================
// 高级安全运算实现
// ============================================================================

bool CN_math_safe_pow_i32(int32_t base, uint32_t exponent, int32_t* result)
{
    // 特殊情况处理
    if (exponent == 0)
    {
        *result = 1;
        return true;
    }
    
    if (base == 0)
    {
        *result = 0;
        return true;
    }
    
    if (base == 1)
    {
        *result = 1;
        return true;
    }
    
    if (base == -1)
    {
        *result = (exponent % 2 == 0) ? 1 : -1;
        return true;
    }
    
    // 检查溢出
    int32_t r = 1;
    int32_t b = base;
    uint32_t e = exponent;
    
    while (e > 0)
    {
        if (e & 1)
        {
            // 检查乘法溢出
            if (!CN_math_safe_mul_i32(r, b, &r))
            {
                return false;
            }
        }
        
        e >>= 1;
        if (e > 0)
        {
            // 检查平方溢出
            if (!CN_math_safe_mul_i32(b, b, &b))
            {
                return false;
            }
        }
    }
    
    *result = r;
    return true;
}

bool CN_math_safe_abs_i32(int32_t value, int32_t* result)
{
    // 检查特殊情况：INT_MIN的绝对值会溢出
    if (value == INT_MIN)
    {
        return false;
    }
    
    *result = (value < 0) ? -value : value;
    return true;
}

bool CN_math_safe_neg_i32(int32_t value, int32_t* result)
{
    // 检查特殊情况：INT_MIN取负会溢出
    if (value == INT_MIN)
    {
        return false;
    }
    
    *result = -value;
    return true;
}

bool CN_math_safe_avg_i32(int32_t a, int32_t b, int32_t* result)
{
    // 使用防溢出的平均值计算方法
    // (a + b) / 2 = a/2 + b/2 + (a%2 + b%2)/2
    
    int32_t half_a = a / 2;
    int32_t half_b = b / 2;
    int32_t remainder = (a % 2 + b % 2) / 2;
    
    // 检查加法溢出
    int32_t sum;
    if (!CN_math_safe_add_i32(half_a, half_b, &sum))
    {
        return false;
    }
    
    if (!CN_math_safe_add_i32(sum, remainder, result))
    {
        return false;
    }
    
    return true;
}
