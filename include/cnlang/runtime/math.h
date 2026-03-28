#ifndef CNLANG_RUNTIME_MATH_H
#define CNLANG_RUNTIME_MATH_H

/*
 * CN Language 运行时数学模块
 *
 * Freestanding 模式支持：
 * - [FS] 所有数学函数在 freestanding 模式下也可用
 * - 但浮点函数（pow, sqrt）需要硬件支持或软件实现
 */

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// 数学函数 [FS - Freestanding 模式支持]
// =============================================================================
long long cn_rt_abs(long long val);
long long cn_rt_min(long long a, long long b);
long long cn_rt_max(long long a, long long b);

// =============================================================================
// 浮点数学函数 [FS - 需要硬件支持或软件实现]
// =============================================================================
double    cn_rt_pow(double base, double exp);
double    cn_rt_sqrt(double val);

// =============================================================================
// 中文函数名别名 (Chinese Function Name Aliases)
// 默认启用，可通过定义 CN_NO_CHINESE_NAMES 禁用
// =============================================================================
#ifndef CN_NO_CHINESE_NAMES

#define 求幂 cn_rt_pow
#define 求平方根 cn_rt_sqrt

#endif // CN_NO_CHINESE_NAMES

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_MATH_H */
