/******************************************************************************
 * 文件名: CN_math_random.h
 * 功能: CN_Language随机数生成器模块
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义随机数生成器接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_MATH_RANDOM_H
#define CN_MATH_RANDOM_H

#include <stdbool.h>
#include <stdint.h>
#include "CN_math_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 随机数生成器类型
// ============================================================================

/**
 * @brief 随机数生成器算法类型
 */
typedef enum Eum_CN_RandomAlgorithm_t
{
    Eum_RANDOM_MT19937 = 0,      /**< Mersenne Twister 19937算法 */
    Eum_RANDOM_XORSHIFT = 1,     /**< Xorshift算法 */
    Eum_RANDOM_LCG = 2,          /**< 线性同余生成器 */
    Eum_RANDOM_PCG = 3,          /**< Permuted Congruential Generator */
    Eum_RANDOM_SYSTEM = 4        /**< 系统随机数生成器 */
} Eum_CN_RandomAlgorithm_t;

/**
 * @brief 随机数生成器状态结构体
 */
typedef struct Stru_CN_RandomGenerator_t Stru_CN_RandomGenerator_t;

// ============================================================================
// 随机数生成器管理函数
// ============================================================================

/**
 * @brief 创建随机数生成器
 * 
 * @param algorithm 随机数算法类型
 * @param seed 随机数种子（0表示使用当前时间）
 * @return 随机数生成器指针，失败返回NULL
 */
Stru_CN_RandomGenerator_t* CN_math_random_create(Eum_CN_RandomAlgorithm_t algorithm, uint64_t seed);

/**
 * @brief 销毁随机数生成器
 * 
 * @param generator 随机数生成器指针
 */
void CN_math_random_destroy(Stru_CN_RandomGenerator_t* generator);

/**
 * @brief 重新设置随机数种子
 * 
 * @param generator 随机数生成器指针
 * @param seed 新的随机数种子
 */
void CN_math_random_reseed(Stru_CN_RandomGenerator_t* generator, uint64_t seed);

/**
 * @brief 获取随机数生成器算法类型
 * 
 * @param generator 随机数生成器指针
 * @return 算法类型
 */
Eum_CN_RandomAlgorithm_t CN_math_random_get_algorithm(const Stru_CN_RandomGenerator_t* generator);

// ============================================================================
// 基本随机数生成函数
// ============================================================================

/**
 * @brief 生成32位无符号随机整数
 * 
 * @param generator 随机数生成器指针
 * @return 32位无符号随机整数
 */
uint32_t CN_math_random_u32(Stru_CN_RandomGenerator_t* generator);

/**
 * @brief 生成64位无符号随机整数
 * 
 * @param generator 随机数生成器指针
 * @return 64位无符号随机整数
 */
uint64_t CN_math_random_u64(Stru_CN_RandomGenerator_t* generator);

/**
 * @brief 生成32位有符号随机整数
 * 
 * @param generator 随机数生成器指针
 * @return 32位有符号随机整数
 */
int32_t CN_math_random_i32(Stru_CN_RandomGenerator_t* generator);

/**
 * @brief 生成64位有符号随机整数
 * 
 * @param generator 随机数生成器指针
 * @return 64位有符号随机整数
 */
int64_t CN_math_random_i64(Stru_CN_RandomGenerator_t* generator);

// ============================================================================
// 范围限制随机数生成函数
// ============================================================================

/**
 * @brief 生成指定范围内的32位随机整数
 * 
 * @param generator 随机数生成器指针
 * @param min 最小值（包含）
 * @param max 最大值（包含）
 * @return 指定范围内的随机整数
 */
int32_t CN_math_random_range_i32(Stru_CN_RandomGenerator_t* generator, int32_t min, int32_t max);

/**
 * @brief 生成指定范围内的64位随机整数
 * 
 * @param generator 随机数生成器指针
 * @param min 最小值（包含）
 * @param max 最大值（包含）
 * @return 指定范围内的随机整数
 */
int64_t CN_math_random_range_i64(Stru_CN_RandomGenerator_t* generator, int64_t min, int64_t max);

/**
 * @brief 生成指定范围内的无符号32位随机整数
 * 
 * @param generator 随机数生成器指针
 * @param min 最小值（包含）
 * @param max 最大值（包含）
 * @return 指定范围内的随机整数
 */
uint32_t CN_math_random_range_u32(Stru_CN_RandomGenerator_t* generator, uint32_t min, uint32_t max);

/**
 * @brief 生成指定范围内的无符号64位随机整数
 * 
 * @param generator 随机数生成器指针
 * @param min 最小值（包含）
 * @param max 最大值（包含）
 * @return 指定范围内的随机整数
 */
uint64_t CN_math_random_range_u64(Stru_CN_RandomGenerator_t* generator, uint64_t min, uint64_t max);

// ============================================================================
// 浮点随机数生成函数
// ============================================================================

/**
 * @brief 生成[0.0, 1.0)范围内的随机浮点数
 * 
 * @param generator 随机数生成器指针
 * @return 随机浮点数
 */
float CN_math_random_float(Stru_CN_RandomGenerator_t* generator);

/**
 * @brief 生成[0.0, 1.0)范围内的随机双精度浮点数
 * 
 * @param generator 随机数生成器指针
 * @return 随机双精度浮点数
 */
double CN_math_random_double(Stru_CN_RandomGenerator_t* generator);

/**
 * @brief 生成指定范围内的随机浮点数
 * 
 * @param generator 随机数生成器指针
 * @param min 最小值（包含）
 * @param max 最大值（不包含）
 * @return 指定范围内的随机浮点数
 */
float CN_math_random_range_float(Stru_CN_RandomGenerator_t* generator, float min, float max);

/**
 * @brief 生成指定范围内的随机双精度浮点数
 * 
 * @param generator 随机数生成器指针
 * @param min 最小值（包含）
 * @param max 最大值（不包含）
 * @return 指定范围内的随机双精度浮点数
 */
double CN_math_random_range_double(Stru_CN_RandomGenerator_t* generator, double min, double max);

// ============================================================================
// 高级随机数生成函数
// ============================================================================

/**
 * @brief 生成随机布尔值
 * 
 * @param generator 随机数生成器指针
 * @param probability_true 为真的概率（0.0到1.0之间）
 * @return 随机布尔值
 */
bool CN_math_random_bool(Stru_CN_RandomGenerator_t* generator, float probability_true);

/**
 * @brief 从数组中随机选择一个元素
 * 
 * @param generator 随机数生成器指针
 * @param array 数组指针
 * @param count 数组元素数量
 * @param element_size 每个元素的大小（字节）
 * @return 随机选择的元素指针
 */
const void* CN_math_random_choice(Stru_CN_RandomGenerator_t* generator, const void* array, size_t count, size_t element_size);

/**
 * @brief 打乱数组顺序（Fisher-Yates洗牌算法）
 * 
 * @param generator 随机数生成器指针
 * @param array 数组指针
 * @param count 数组元素数量
 * @param element_size 每个元素的大小（字节）
 */
void CN_math_random_shuffle(Stru_CN_RandomGenerator_t* generator, void* array, size_t count, size_t element_size);

/**
 * @brief 生成随机字节序列
 * 
 * @param generator 随机数生成器指针
 * @param buffer 输出缓冲区
 * @param size 要生成的字节数
 */
void CN_math_random_bytes(Stru_CN_RandomGenerator_t* generator, void* buffer, size_t size);

// ============================================================================
// 概率分布函数
// ============================================================================

/**
 * @brief 生成正态分布（高斯分布）随机数
 * 
 * @param generator 随机数生成器指针
 * @param mean 均值
 * @param stddev 标准差
 * @return 正态分布随机数
 */
double CN_math_random_normal(Stru_CN_RandomGenerator_t* generator, double mean, double stddev);

/**
 * @brief 生成指数分布随机数
 * 
 * @param generator 随机数生成器指针
 * @param lambda 率参数（λ）
 * @return 指数分布随机数
 */
double CN_math_random_exponential(Stru_CN_RandomGenerator_t* generator, double lambda);

/**
 * @brief 生成泊松分布随机数
 * 
 * @param generator 随机数生成器指针
 * @param lambda 平均发生率（λ）
 * @return 泊松分布随机数
 */
uint32_t CN_math_random_poisson(Stru_CN_RandomGenerator_t* generator, double lambda);

/**
 * @brief 生成二项分布随机数
 * 
 * @param generator 随机数生成器指针
 * @param n 试验次数
 * @param p 成功概率
 * @return 二项分布随机数
 */
uint32_t CN_math_random_binomial(Stru_CN_RandomGenerator_t* generator, uint32_t n, double p);

// ============================================================================
// 全局随机数函数（使用默认生成器）
// ============================================================================

/**
 * @brief 初始化全局随机数生成器
 * 
 * @param algorithm 随机数算法类型
 * @param seed 随机数种子（0表示使用当前时间）
 * @return 初始化成功返回true，失败返回false
 */
bool CN_math_random_global_init(Eum_CN_RandomAlgorithm_t algorithm, uint64_t seed);

/**
 * @brief 关闭全局随机数生成器
 */
void CN_math_random_global_shutdown(void);

/**
 * @brief 生成全局32位随机整数
 * 
 * @return 32位随机整数
 */
uint32_t CN_math_random_global_u32(void);

/**
 * @brief 生成全局[0.0, 1.0)范围内的随机浮点数
 * 
 * @return 随机浮点数
 */
float CN_math_random_global_float(void);

/**
 * @brief 生成全局指定范围内的随机整数
 * 
 * @param min 最小值（包含）
 * @param max 最大值（包含）
 * @return 指定范围内的随机整数
 */
int32_t CN_math_random_global_range_i32(int32_t min, int32_t max);

#ifdef __cplusplus
}
#endif

#endif // CN_MATH_RANDOM_H
