/******************************************************************************
 * 文件名: CN_math_safe_arithmetic_batch.c
 * 功能: CN_Language安全算术模块批量运算部分实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现批量安全算术运算
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_math_safe_arithmetic.h"
#include "CN_math_core.h"
#include <float.h>
#include <math.h>

// ============================================================================
// 批量安全运算实现
// ============================================================================

bool CN_math_safe_sum_i32(const int32_t* array, size_t count, int32_t* result)
{
    if (array == NULL || result == NULL || count == 0)
    {
        return false;
    }
    
    int32_t sum = 0;
    
    for (size_t i = 0; i < count; i++)
    {
        // 检查加法溢出
        if (!CN_math_safe_add_i32(sum, array[i], &sum))
        {
            return false;
        }
    }
    
    *result = sum;
    return true;
}

bool CN_math_safe_product_i32(const int32_t* array, size_t count, int32_t* result)
{
    if (array == NULL || result == NULL || count == 0)
    {
        return false;
    }
    
    int32_t product = 1;
    
    for (size_t i = 0; i < count; i++)
    {
        // 检查乘法溢出
        if (!CN_math_safe_mul_i32(product, array[i], &product))
        {
            return false;
        }
    }
    
    *result = product;
    return true;
}

bool CN_math_safe_sum_f(const float* array, size_t count, float* result)
{
    if (array == NULL || result == NULL || count == 0)
    {
        return false;
    }
    
    // 使用Kahan求和算法减少浮点数累加误差
    float sum = 0.0f;
    float compensation = 0.0f;  // 补偿项
    
    for (size_t i = 0; i < count; i++)
    {
        // Kahan求和算法
        float y = array[i] - compensation;
        float t = sum + y;
        compensation = (t - sum) - y;
        sum = t;
        
        // 检查溢出/NaN
        if (CN_math_is_nan_f(sum) || CN_math_is_inf_f(sum))
        {
            return false;
        }
    }
    
    *result = sum;
    return true;
}
