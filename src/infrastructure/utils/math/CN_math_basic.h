/******************************************************************************
 * 文件名: CN_math_basic.h
 * 功能: CN_Language基本数学运算模块
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义基本数学运算接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_MATH_BASIC_H
#define CN_MATH_BASIC_H

#include <stdbool.h>
#include <stdint.h>
#include "CN_math_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 基本算术运算
// ============================================================================

/**
 * @brief 计算绝对值
 * 
 * @param value 输入值
 * @return 绝对值
 */
int32_t CN_math_abs_i32(int32_t value);
int64_t CN_math_abs_i64(int64_t value);
float CN_math_abs_f(float value);
double CN_math_abs_d(double value);

/**
 * @brief 计算最大值
 * 
 * @param a 第一个值
 * @param b 第二个值
 * @return 最大值
 */
int32_t CN_math_max_i32(int32_t a, int32_t b);
int64_t CN_math_max_i64(int64_t a, int64_t b);
float CN_math_max_f(float a, float b);
double CN_math_max_d(double a, double b);

/**
 * @brief 计算最小值
 * 
 * @param a 第一个值
 * @param b 第二个值
 * @return 最小值
 */
int32_t CN_math_min_i32(int32_t a, int32_t b);
int64_t CN_math_min_i64(int64_t a, int64_t b);
float CN_math_min_f(float a, float b);
double CN_math_min_d(double a, double b);

/**
 * @brief 限制值在指定范围内
 * 
 * @param value 输入值
 * @param min 最小值
 * @param max 最大值
 * @return 限制后的值
 */
int32_t CN_math_clamp_i32(int32_t value, int32_t min, int32_t max);
int64_t CN_math_clamp_i64(int64_t value, int64_t min, int64_t max);
float CN_math_clamp_f(float value, float min, float max);
double CN_math_clamp_d(double value, double min, double max);

/**
 * @brief 线性插值
 * 
 * @param a 起始值
 * @param b 结束值
 * @param t 插值因子（0.0到1.0之间）
 * @return 插值结果
 */
float CN_math_lerp_f(float a, float b, float t);
double CN_math_lerp_d(double a, double b, double t);

// ============================================================================
// 幂运算和对数运算
// ============================================================================

/**
 * @brief 计算平方
 * 
 * @param value 输入值
 * @return 平方值
 */
int32_t CN_math_square_i32(int32_t value);
int64_t CN_math_square_i64(int64_t value);
float CN_math_square_f(float value);
double CN_math_square_d(double value);

/**
 * @brief 计算立方
 * 
 * @param value 输入值
 * @return 立方值
 */
int32_t CN_math_cube_i32(int32_t value);
int64_t CN_math_cube_i64(int64_t value);
float CN_math_cube_f(float value);
double CN_math_cube_d(double value);

/**
 * @brief 计算幂运算
 * 
 * @param base 底数
 * @param exponent 指数
 * @return 幂运算结果
 */
float CN_math_pow_f(float base, float exponent);
double CN_math_pow_d(double base, double exponent);

/**
 * @brief 计算平方根
 * 
 * @param value 输入值（必须非负）
 * @return 平方根
 */
float CN_math_sqrt_f(float value);
double CN_math_sqrt_d(double value);

/**
 * @brief 计算立方根
 * 
 * @param value 输入值
 * @return 立方根
 */
float CN_math_cbrt_f(float value);
double CN_math_cbrt_d(double value);

/**
 * @brief 计算自然对数
 * 
 * @param value 输入值（必须大于0）
 * @return 自然对数
 */
float CN_math_log_f(float value);
double CN_math_log_d(double value);

/**
 * @brief 计算以10为底的对数
 * 
 * @param value 输入值（必须大于0）
 * @return 以10为底的对数
 */
float CN_math_log10_f(float value);
double CN_math_log10_d(double value);

/**
 * @brief 计算以2为底的对数
 * 
 * @param value 输入值（必须大于0）
 * @return 以2为底的对数
 */
float CN_math_log2_f(float value);
double CN_math_log2_d(double value);

// ============================================================================
// 三角函数
// ============================================================================

/**
 * @brief 计算正弦值
 * 
 * @param angle 角度（弧度）
 * @return 正弦值
 */
float CN_math_sin_f(float angle);
double CN_math_sin_d(double angle);

/**
 * @brief 计算余弦值
 * 
 * @param angle 角度（弧度）
 * @return 余弦值
 */
float CN_math_cos_f(float angle);
double CN_math_cos_d(double angle);

/**
 * @brief 计算正切值
 * 
 * @param angle 角度（弧度）
 * @return 正切值
 */
float CN_math_tan_f(float angle);
double CN_math_tan_d(double angle);

/**
 * @brief 计算反正弦值
 * 
 * @param value 输入值（-1.0到1.0之间）
 * @return 反正弦值（弧度）
 */
float CN_math_asin_f(float value);
double CN_math_asin_d(double value);

/**
 * @brief 计算反余弦值
 * 
 * @param value 输入值（-1.0到1.0之间）
 * @return 反余弦值（弧度）
 */
float CN_math_acos_f(float value);
double CN_math_acos_d(double value);

/**
 * @brief 计算反正切值
 * 
 * @param value 输入值
 * @return 反正切值（弧度）
 */
float CN_math_atan_f(float value);
double CN_math_atan_d(double value);

/**
 * @brief 计算atan2值（四象限反正切）
 * 
 * @param y Y坐标
 * @param x X坐标
 * @return 反正切值（弧度）
 */
float CN_math_atan2_f(float y, float x);
double CN_math_atan2_d(double y, double x);

// ============================================================================
// 双曲函数
// ============================================================================

/**
 * @brief 计算双曲正弦值
 * 
 * @param value 输入值
 * @return 双曲正弦值
 */
float CN_math_sinh_f(float value);
double CN_math_sinh_d(double value);

/**
 * @brief 计算双曲余弦值
 * 
 * @param value 输入值
 * @return 双曲余弦值
 */
float CN_math_cosh_f(float value);
double CN_math_cosh_d(double value);

/**
 * @brief 计算双曲正切值
 * 
 * @param value 输入值
 * @return 双曲正切值
 */
float CN_math_tanh_f(float value);
double CN_math_tanh_d(double value);

// ============================================================================
// 角度转换函数
// ============================================================================

/**
 * @brief 角度转弧度
 * 
 * @param degrees 角度
 * @return 弧度
 */
float CN_math_deg_to_rad_f(float degrees);
double CN_math_deg_to_rad_d(double degrees);

/**
 * @brief 弧度转角度
 * 
 * @param radians 弧度
 * @return 角度
 */
float CN_math_rad_to_deg_f(float radians);
double CN_math_rad_to_deg_d(double radians);

// ============================================================================
// 舍入函数
// ============================================================================

/**
 * @brief 向上取整
 * 
 * @param value 输入值
 * @return 向上取整结果
 */
float CN_math_ceil_f(float value);
double CN_math_ceil_d(double value);

/**
 * @brief 向下取整
 * 
 * @param value 输入值
 * @return 向下取整结果
 */
float CN_math_floor_f(float value);
double CN_math_floor_d(double value);

/**
 * @brief 四舍五入
 * 
 * @param value 输入值
 * @return 四舍五入结果
 */
float CN_math_round_f(float value);
double CN_math_round_d(double value);

/**
 * @brief 截断小数部分
 * 
 * @param value 输入值
 * @return 截断结果
 */
float CN_math_trunc_f(float value);
double CN_math_trunc_d(double value);

// ============================================================================
// 模运算
// ============================================================================

/**
 * @brief 浮点数取模运算
 * 
 * @param x 被除数
 * @param y 除数
 * @return 余数
 */
float CN_math_fmod_f(float x, float y);
double CN_math_fmod_d(double x, double y);

/**
 * @brief 浮点数取余运算（带符号）
 * 
 * @param x 被除数
 * @param y 除数
 * @return 余数
 */
float CN_math_remainder_f(float x, float y);
double CN_math_remainder_d(double x, double y);

// ============================================================================
// 特殊函数
// ============================================================================

/**
 * @brief 计算伽马函数
 * 
 * @param value 输入值
 * @return 伽马函数值
 */
float CN_math_gamma_f(float value);
double CN_math_gamma_d(double value);

/**
 * @brief 计算对数伽马函数
 * 
 * @param value 输入值
 * @return 对数伽马函数值
 */
float CN_math_lgamma_f(float value);
double CN_math_lgamma_d(double value);

/**
 * @brief 计算误差函数
 * 
 * @param value 输入值
 * @return 误差函数值
 */
float CN_math_erf_f(float value);
double CN_math_erf_d(double value);

/**
 * @brief 计算互补误差函数
 * 
 * @param value 输入值
 * @return 互补误差函数值
 */
float CN_math_erfc_f(float value);
double CN_math_erfc_d(double value);

// ============================================================================
// 统计函数
// ============================================================================

/**
 * @brief 计算平均值
 * 
 * @param array 数组
 * @param count 元素数量
 * @return 平均值
 */
float CN_math_mean_f(const float* array, size_t count);
double CN_math_mean_d(const double* array, size_t count);

/**
 * @brief 计算方差
 * 
 * @param array 数组
 * @param count 元素数量
 * @param is_sample 是否为样本方差（true为样本方差，false为总体方差）
 * @return 方差
 */
float CN_math_variance_f(const float* array, size_t count, bool is_sample);
double CN_math_variance_d(const double* array, size_t count, bool is_sample);

/**
 * @brief 计算标准差
 * 
 * @param array 数组
 * @param count 元素数量
 * @param is_sample 是否为样本标准差
 * @return 标准差
 */
float CN_math_stddev_f(const float* array, size_t count, bool is_sample);
double CN_math_stddev_d(const double* array, size_t count, bool is_sample);

/**
 * @brief 计算中位数
 * 
 * @param array 数组（会被排序）
 * @param count 元素数量
 * @return 中位数
 */
float CN_math_median_f(float* array, size_t count);
double CN_math_median_d(double* array, size_t count);

// ============================================================================
// 几何函数
// ============================================================================

/**
 * @brief 计算两点间距离（欧几里得距离）
 * 
 * @param x1 第一个点的X坐标
 * @param y1 第一个点的Y坐标
 * @param x2 第二个点的X坐标
 * @param y2 第二个点的Y坐标
 * @return 距离
 */
float CN_math_distance_2d_f(float x1, float y1, float x2, float y2);
double CN_math_distance_2d_d(double x1, double y1, double x2, double y2);

/**
 * @brief 计算三维空间两点间距离
 * 
 * @param x1 第一个点的X坐标
 * @param y1 第一个点的Y坐标
 * @param z1 第一个点的Z坐标
 * @param x2 第二个点的X坐标
 * @param y2 第二个点的Y坐标
 * @param z2 第二个点的Z坐标
 * @return 距离
 */
float CN_math_distance_3d_f(float x1, float y1, float z1, float x2, float y2, float z2);
double CN_math_distance_3d_d(double x1, double y1, double z1, double x2, double y2, double z2);

/**
 * @brief 计算向量的长度（模）
 * 
 * @param x X分量
 * @param y Y分量
 * @return 向量长度
 */
float CN_math_vector_length_2d_f(float x, float y);
double CN_math_vector_length_2d_d(double x, double y);

/**
 * @brief 计算三维向量的长度
 * 
 * @param x X分量
 * @param y Y分量
 * @param z Z分量
 * @return 向量长度
 */
float CN_math_vector_length_3d_f(float x, float y, float z);
double CN_math_vector_length_3d_d(double x, double y, double z);

#ifdef __cplusplus
}
#endif

#endif // CN_MATH_BASIC_H
