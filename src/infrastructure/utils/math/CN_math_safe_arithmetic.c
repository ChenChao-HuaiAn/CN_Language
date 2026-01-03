/******************************************************************************
 * 文件名: CN_math_safe_arithmetic.c
 * 功能: CN_Language安全算术模块实现（溢出检查）
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现安全算术运算
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_math_safe_arithmetic.h"
#include <limits.h>
#include <float.h>
#include <math.h>

// ============================================================================
// 32位整数安全运算实现
// ============================================================================

bool CN_math_safe_add_i32(int32_t a, int32_t b, int32_t* result)
{
    // 检查溢出：如果a和b同号，且结果符号与a不同，则溢出
    if ((b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b))
    {
        return false;
    }
    
    *result = a + b;
    return true;
}

bool CN_math_safe_sub_i32(int32_t a, int32_t b, int32_t* result)
{
    // 检查溢出：如果b是INT_MIN，减法可能溢出
    if ((b > 0 && a < INT_MIN + b) || (b < 0 && a > INT_MAX + b))
    {
        return false;
    }
    
    *result = a - b;
    return true;
}

bool CN_math_safe_mul_i32(int32_t a, int32_t b, int32_t* result)
{
    // 检查溢出
    if (a > 0)
    {
        if (b > 0)
        {
            if (a > INT_MAX / b)
            {
                return false;
            }
        }
        else if (b < 0)
        {
            if (b < INT_MIN / a)
            {
                return false;
            }
        }
    }
    else if (a < 0)
    {
        if (b > 0)
        {
            if (a < INT_MIN / b)
            {
                return false;
            }
        }
        else if (b < 0)
        {
            if (a < INT_MAX / b)
            {
                return false;
            }
        }
    }
    
    *result = a * b;
    return true;
}

bool CN_math_safe_div_i32(int32_t a, int32_t b, int32_t* result)
{
    // 检查除零
    if (b == 0)
    {
        return false;
    }
    
    // 检查特殊情况：INT_MIN / -1 会溢出
    if (a == INT_MIN && b == -1)
    {
        return false;
    }
    
    *result = a / b;
    return true;
}

bool CN_math_safe_mod_i32(int32_t a, int32_t b, int32_t* result)
{
    // 检查除零
    if (b == 0)
    {
        return false;
    }
    
    *result = a % b;
    return true;
}

// ============================================================================
// 64位整数安全运算实现
// ============================================================================

bool CN_math_safe_add_i64(int64_t a, int64_t b, int64_t* result)
{
    // 检查溢出：如果a和b同号，且结果符号与a不同，则溢出
    if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b))
    {
        return false;
    }
    
    *result = a + b;
    return true;
}

bool CN_math_safe_sub_i64(int64_t a, int64_t b, int64_t* result)
{
    // 检查溢出：如果b是INT64_MIN，减法可能溢出
    if ((b > 0 && a < INT64_MIN + b) || (b < 0 && a > INT64_MAX + b))
    {
        return false;
    }
    
    *result = a - b;
    return true;
}

bool CN_math_safe_mul_i64(int64_t a, int64_t b, int64_t* result)
{
    // 检查溢出
    if (a > 0)
    {
        if (b > 0)
        {
            if (a > INT64_MAX / b)
            {
                return false;
            }
        }
        else if (b < 0)
        {
            if (b < INT64_MIN / a)
            {
                return false;
            }
        }
    }
    else if (a < 0)
    {
        if (b > 0)
        {
            if (a < INT64_MIN / b)
            {
                return false;
            }
        }
        else if (b < 0)
        {
            if (a < INT64_MAX / b)
            {
                return false;
            }
        }
    }
    
    *result = a * b;
    return true;
}

bool CN_math_safe_div_i64(int64_t a, int64_t b, int64_t* result)
{
    // 检查除零
    if (b == 0)
    {
        return false;
    }
    
    // 检查特殊情况：INT64_MIN / -1 会溢出
    if (a == INT64_MIN && b == -1)
    {
        return false;
    }
    
    *result = a / b;
    return true;
}

bool CN_math_safe_mod_i64(int64_t a, int64_t b, int64_t* result)
{
    // 检查除零
    if (b == 0)
    {
        return false;
    }
    
    *result = a % b;
    return true;
}

// ============================================================================
// 无符号32位整数安全运算实现
// ============================================================================

bool CN_math_safe_add_u32(uint32_t a, uint32_t b, uint32_t* result)
{
    // 检查溢出
    if (a > UINT32_MAX - b)
    {
        return false;
    }
    
    *result = a + b;
    return true;
}

bool CN_math_safe_sub_u32(uint32_t a, uint32_t b, uint32_t* result)
{
    // 检查下溢
    if (a < b)
    {
        return false;
    }
    
    *result = a - b;
    return true;
}

bool CN_math_safe_mul_u32(uint32_t a, uint32_t b, uint32_t* result)
{
    // 检查溢出
    if (b != 0 && a > UINT32_MAX / b)
    {
        return false;
    }
    
    *result = a * b;
    return true;
}

bool CN_math_safe_div_u32(uint32_t a, uint32_t b, uint32_t* result)
{
    // 检查除零
    if (b == 0)
    {
        return false;
    }
    
    *result = a / b;
    return true;
}

bool CN_math_safe_mod_u32(uint32_t a, uint32_t b, uint32_t* result)
{
    // 检查除零
    if (b == 0)
    {
        return false;
    }
    
    *result = a % b;
    return true;
}
