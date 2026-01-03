/******************************************************************************
 * 文件名: CN_math_safe_arithmetic.h
 * 功能: CN_Language安全算术模块（溢出检查）
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义安全算术运算接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_MATH_SAFE_ARITHMETIC_H
#define CN_MATH_SAFE_ARITHMETIC_H

#include <stdbool.h>
#include <stdint.h>
#include "CN_math_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 安全整数运算
// ============================================================================

/**
 * @brief 安全整数加法（带溢出检查）
 * 
 * @param a 第一个加数
 * @param b 第二个加数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_add_i32(int32_t a, int32_t b, int32_t* result);

/**
 * @brief 安全整数减法（带溢出检查）
 * 
 * @param a 被减数
 * @param b 减数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_sub_i32(int32_t a, int32_t b, int32_t* result);

/**
 * @brief 安全整数乘法（带溢出检查）
 * 
 * @param a 第一个乘数
 * @param b 第二个乘数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_mul_i32(int32_t a, int32_t b, int32_t* result);

/**
 * @brief 安全整数除法（带除零检查）
 * 
 * @param a 被除数
 * @param b 除数
 * @param result 存储结果的指针
 * @return 如果成功返回true，除零返回false
 */
bool CN_math_safe_div_i32(int32_t a, int32_t b, int32_t* result);

/**
 * @brief 安全整数取模（带除零检查）
 * 
 * @param a 被除数
 * @param b 除数
 * @param result 存储结果的指针
 * @return 如果成功返回true，除零返回false
 */
bool CN_math_safe_mod_i32(int32_t a, int32_t b, int32_t* result);

// ============================================================================
// 64位安全整数运算
// ============================================================================

/**
 * @brief 安全64位整数加法（带溢出检查）
 * 
 * @param a 第一个加数
 * @param b 第二个加数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_add_i64(int64_t a, int64_t b, int64_t* result);

/**
 * @brief 安全64位整数减法（带溢出检查）
 * 
 * @param a 被减数
 * @param b 减数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_sub_i64(int64_t a, int64_t b, int64_t* result);

/**
 * @brief 安全64位整数乘法（带溢出检查）
 * 
 * @param a 第一个乘数
 * @param b 第二个乘数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_mul_i64(int64_t a, int64_t b, int64_t* result);

/**
 * @brief 安全64位整数除法（带除零检查）
 * 
 * @param a 被除数
 * @param b 除数
 * @param result 存储结果的指针
 * @return 如果成功返回true，除零返回false
 */
bool CN_math_safe_div_i64(int64_t a, int64_t b, int64_t* result);

/**
 * @brief 安全64位整数取模（带除零检查）
 * 
 * @param a 被除数
 * @param b 除数
 * @param result 存储结果的指针
 * @return 如果成功返回true，除零返回false
 */
bool CN_math_safe_mod_i64(int64_t a, int64_t b, int64_t* result);

// ============================================================================
// 无符号整数安全运算
// ============================================================================

/**
 * @brief 安全无符号整数加法（带溢出检查）
 * 
 * @param a 第一个加数
 * @param b 第二个加数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_add_u32(uint32_t a, uint32_t b, uint32_t* result);

/**
 * @brief 安全无符号整数减法（带下溢检查）
 * 
 * @param a 被减数
 * @param b 减数
 * @param result 存储结果的指针
 * @return 如果成功返回true，下溢返回false
 */
bool CN_math_safe_sub_u32(uint32_t a, uint32_t b, uint32_t* result);

/**
 * @brief 安全无符号整数乘法（带溢出检查）
 * 
 * @param a 第一个乘数
 * @param b 第二个乘数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_mul_u32(uint32_t a, uint32_t b, uint32_t* result);

/**
 * @brief 安全无符号整数除法（带除零检查）
 * 
 * @param a 被除数
 * @param b 除数
 * @param result 存储结果的指针
 * @return 如果成功返回true，除零返回false
 */
bool CN_math_safe_div_u32(uint32_t a, uint32_t b, uint32_t* result);

/**
 * @brief 安全无符号整数取模（带除零检查）
 * 
 * @param a 被除数
 * @param b 除数
 * @param result 存储结果的指针
 * @return 如果成功返回true，除零返回false
 */
bool CN_math_safe_mod_u32(uint32_t a, uint32_t b, uint32_t* result);

// ============================================================================
// 浮点数安全运算
// ============================================================================

/**
 * @brief 安全浮点数加法（带溢出/下溢检查）
 * 
 * @param a 第一个加数
 * @param b 第二个加数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出/下溢/NaN返回false
 */
bool CN_math_safe_add_f(float a, float b, float* result);

/**
 * @brief 安全浮点数减法（带溢出/下溢检查）
 * 
 * @param a 被减数
 * @param b 减数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出/下溢/NaN返回false
 */
bool CN_math_safe_sub_f(float a, float b, float* result);

/**
 * @brief 安全浮点数乘法（带溢出/下溢检查）
 * 
 * @param a 第一个乘数
 * @param b 第二个乘数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出/下溢/NaN返回false
 */
bool CN_math_safe_mul_f(float a, float b, float* result);

/**
 * @brief 安全浮点数除法（带除零/溢出检查）
 * 
 * @param a 被除数
 * @param b 除数
 * @param result 存储结果的指针
 * @return 如果成功返回true，除零/溢出/NaN返回false
 */
bool CN_math_safe_div_f(float a, float b, float* result);

// ============================================================================
// 双精度浮点数安全运算
// ============================================================================

/**
 * @brief 安全双精度浮点数加法（带溢出/下溢检查）
 * 
 * @param a 第一个加数
 * @param b 第二个加数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出/下溢/NaN返回false
 */
bool CN_math_safe_add_d(double a, double b, double* result);

/**
 * @brief 安全双精度浮点数减法（带溢出/下溢检查）
 * 
 * @param a 被减数
 * @param b 减数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出/下溢/NaN返回false
 */
bool CN_math_safe_sub_d(double a, double b, double* result);

/**
 * @brief 安全双精度浮点数乘法（带溢出/下溢检查）
 * 
 * @param a 第一个乘数
 * @param b 第二个乘数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出/下溢/NaN返回false
 */
bool CN_math_safe_mul_d(double a, double b, double* result);

/**
 * @brief 安全双精度浮点数除法（带除零/溢出检查）
 * 
 * @param a 被除数
 * @param b 除数
 * @param result 存储结果的指针
 * @return 如果成功返回true，除零/溢出/NaN返回false
 */
bool CN_math_safe_div_d(double a, double b, double* result);

// ============================================================================
// 高级安全运算
// ============================================================================

/**
 * @brief 安全整数幂运算（带溢出检查）
 * 
 * @param base 底数
 * @param exponent 指数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_pow_i32(int32_t base, uint32_t exponent, int32_t* result);

/**
 * @brief 安全整数绝对值（带溢出检查，对于INT_MIN）
 * 
 * @param value 输入值
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_abs_i32(int32_t value, int32_t* result);

/**
 * @brief 安全整数取负（带溢出检查，对于INT_MIN）
 * 
 * @param value 输入值
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_neg_i32(int32_t value, int32_t* result);

/**
 * @brief 安全整数平均值（防溢出）
 * 
 * @param a 第一个值
 * @param b 第二个值
 * @param result 存储结果的指针
 * @return 总是返回true（不会溢出）
 */
bool CN_math_safe_avg_i32(int32_t a, int32_t b, int32_t* result);

// ============================================================================
// 批量安全运算
// ============================================================================

/**
 * @brief 安全整数数组求和（带溢出检查）
 * 
 * @param array 整数数组
 * @param count 数组元素数量
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_sum_i32(const int32_t* array, size_t count, int32_t* result);

/**
 * @brief 安全整数数组乘积（带溢出检查）
 * 
 * @param array 整数数组
 * @param count 数组元素数量
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_product_i32(const int32_t* array, size_t count, int32_t* result);

/**
 * @brief 安全浮点数数组求和（带溢出检查）
 * 
 * @param array 浮点数数组
 * @param count 数组元素数量
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出/NaN返回false
 */
bool CN_math_safe_sum_f(const float* array, size_t count, float* result);

#ifdef __cplusplus
}
#endif

#endif // CN_MATH_SAFE_ARITHMETIC_H
