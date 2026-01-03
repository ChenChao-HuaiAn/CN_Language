/******************************************************************************
 * 文件名: CN_math_core.h
 * 功能: CN_Language数学核心模块
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义数学核心结构和常量
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_MATH_CORE_H
#define CN_MATH_CORE_H

#include <stdbool.h>
#include <stdint.h>
#include <float.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 数学常量定义
// ============================================================================

/**
 * @brief 数学常量定义
 */
#define CN_MATH_PI          3.14159265358979323846  /**< 圆周率π */
#define CN_MATH_E           2.71828182845904523536  /**< 自然常数e */
#define CN_MATH_SQRT2       1.41421356237309504880  /**< 根号2 */
#define CN_MATH_SQRT1_2     0.70710678118654752440  /**< 根号1/2 */
#define CN_MATH_LN2         0.69314718055994530942  /**< ln(2) */
#define CN_MATH_LN10        2.30258509299404568402  /**< ln(10) */
#define CN_MATH_LOG2E       1.44269504088896340736  /**< log2(e) */
#define CN_MATH_LOG10E      0.43429448190325182765  /**< log10(e) */
#define CN_MATH_TWOPI       6.28318530717958647692  /**< 2π */
#define CN_MATH_PI_2        1.57079632679489661923  /**< π/2 */
#define CN_MATH_PI_4        0.78539816339744830962  /**< π/4 */
#define CN_MATH_1_PI        0.31830988618379067154  /**< 1/π */
#define CN_MATH_2_PI        0.63661977236758134308  /**< 2/π */
#define CN_MATH_2_SQRTPI    1.12837916709551257390  /**< 2/√π */
#define CN_MATH_DEG_TO_RAD  0.01745329251994329577  /**< 度转弧度系数 */
#define CN_MATH_RAD_TO_DEG  57.29577951308232087680 /**< 弧度转度系数 */

// ============================================================================
// 数学精度定义
// ============================================================================

/**
 * @brief 浮点数比较精度
 */
#define CN_MATH_EPSILON_FLOAT   1e-6f   /**< 单精度浮点数比较精度 */
#define CN_MATH_EPSILON_DOUBLE  1e-12   /**< 双精度浮点数比较精度 */

/**
 * @brief 无穷大和NaN定义
 */
#define CN_MATH_INFINITY_FLOAT  INFINITY
#define CN_MATH_NAN_FLOAT       NAN
#define CN_MATH_INFINITY_DOUBLE ((double)INFINITY)
#define CN_MATH_NAN_DOUBLE      ((double)NAN)

// ============================================================================
// 数学错误码定义
// ============================================================================

/**
 * @brief 数学运算错误码枚举
 */
typedef enum Eum_CN_MathError_t
{
    Eum_MATH_SUCCESS = 0,               /**< 成功 */
    Eum_MATH_ERROR_OVERFLOW = 1,        /**< 溢出错误 */
    Eum_MATH_ERROR_UNDERFLOW = 2,       /**< 下溢错误 */
    Eum_MATH_ERROR_DIVISION_BY_ZERO = 3, /**< 除零错误 */
    Eum_MATH_ERROR_INVALID_ARGUMENT = 4, /**< 无效参数错误 */
    Eum_MATH_ERROR_DOMAIN = 5,          /**< 定义域错误 */
    Eum_MATH_ERROR_RANGE = 6,           /**< 值域错误 */
    Eum_MATH_ERROR_NAN = 7,             /**< NaN错误 */
    Eum_MATH_ERROR_INFINITY = 8,        /**< 无穷大错误 */
    Eum_MATH_ERROR_NOT_IMPLEMENTED = 9  /**< 未实现错误 */
} Eum_CN_MathError_t;

// ============================================================================
// 数学结果结构体
// ============================================================================

/**
 * @brief 数学运算结果结构体
 * 
 * 用于返回数学运算结果和错误状态
 */
typedef struct Stru_CN_MathResult_t
{
    union
    {
        int32_t i32_result;     /**< 32位整数结果 */
        int64_t i64_result;     /**< 64位整数结果 */
        float   f32_result;     /**< 32位浮点数结果 */
        double  f64_result;     /**< 64位浮点数结果 */
    } value;                    /**< 运算结果值 */
    
    Eum_CN_MathError_t error;   /**< 错误码 */
    bool has_error;             /**< 是否有错误 */
} Stru_CN_MathResult_t;

// ============================================================================
// 核心数学函数
// ============================================================================

/**
 * @brief 检查浮点数是否为NaN
 * 
 * @param value 要检查的值
 * @return 如果是NaN返回true，否则返回false
 */
bool CN_math_is_nan_f(float value);

/**
 * @brief 检查浮点数是否为无穷大
 * 
 * @param value 要检查的值
 * @return 如果是无穷大返回true，否则返回false
 */
bool CN_math_is_inf_f(float value);

/**
 * @brief 检查浮点数是否为有限数
 * 
 * @param value 要检查的值
 * @return 如果是有限数返回true，否则返回false
 */
bool CN_math_is_finite_f(float value);

/**
 * @brief 检查双精度浮点数是否为NaN
 * 
 * @param value 要检查的值
 * @return 如果是NaN返回true，否则返回false
 */
bool CN_math_is_nan_d(double value);

/**
 * @brief 检查双精度浮点数是否为无穷大
 * 
 * @param value 要检查的值
 * @return 如果是无穷大返回true，否则返回false
 */
bool CN_math_is_inf_d(double value);

/**
 * @brief 检查双精度浮点数是否为有限数
 * 
 * @param value 要检查的值
 * @return 如果是有限数返回true，否则返回false
 */
bool CN_math_is_finite_d(double value);

/**
 * @brief 比较两个浮点数是否相等（考虑精度误差）
 * 
 * @param a 第一个浮点数
 * @param b 第二个浮点数
 * @param epsilon 比较精度
 * @return 如果相等返回true，否则返回false
 */
bool CN_math_float_equal_f(float a, float b, float epsilon);

/**
 * @brief 比较两个双精度浮点数是否相等（考虑精度误差）
 * 
 * @param a 第一个双精度浮点数
 * @param b 第二个双精度浮点数
 * @param epsilon 比较精度
 * @return 如果相等返回true，否则返回false
 */
bool CN_math_float_equal_d(double a, double b, double epsilon);

/**
 * @brief 获取数学错误码的描述
 * 
 * @param error 错误码
 * @return 错误描述字符串
 */
const char* CN_math_error_string(Eum_CN_MathError_t error);

#ifdef __cplusplus
}
#endif

#endif // CN_MATH_CORE_H
