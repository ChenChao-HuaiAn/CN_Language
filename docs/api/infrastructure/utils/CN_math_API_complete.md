# CN_Language 数学模块 API 文档

## 文档概述

本文档描述了CN_Language数学模块的应用程序编程接口（API）。数学模块提供安全、可靠的数学运算功能，包括安全算术运算、随机数生成和基本数学函数。

## 模块结构

### 包含文件
```c
#include "src/infrastructure/utils/math/CN_math.h"  // 主头文件
```

### 模块初始化
在使用数学模块之前，需要先初始化模块。模块初始化会设置默认的随机数生成器。

## 核心模块 API

### 数学常量

#### 预定义常量
```c
// 圆周率相关
#define CN_MATH_PI          3.14159265358979323846  // 圆周率π
#define CN_MATH_TWOPI       6.28318530717958647692  // 2π
#define CN_MATH_PI_2        1.57079632679489661923  // π/2
#define CN_MATH_PI_4        0.78539816339744830962  // π/4

// 自然常数
#define CN_MATH_E           2.71828182845904523536  // 自然常数e

// 常用根号值
#define CN_MATH_SQRT2       1.41421356237309504880  // 根号2
#define CN_MATH_SQRT1_2     0.70710678118654752440  // 根号1/2

// 对数常数
#define CN_MATH_LN2         0.69314718055994530942  // ln(2)
#define CN_MATH_LN10        2.30258509299404568402  // ln(10)
#define CN_MATH_LOG2E       1.44269504088896340736  // log2(e)
#define CN_MATH_LOG10E      0.43429448190325182765  // log10(e)

// 角度转换系数
#define CN_MATH_DEG_TO_RAD  0.01745329251994329577  // 度转弧度系数
#define CN_MATH_RAD_TO_DEG  57.29577951308232087680 // 弧度转度系数

// 精度定义
#define CN_MATH_EPSILON_FLOAT   1e-6f   // 单精度浮点数比较精度
#define CN_MATH_EPSILON_DOUBLE  1e-12   // 双精度浮点数比较精度
```

### 错误码枚举

#### Eum_CN_MathError_t
```c
typedef enum Eum_CN_MathError_t
{
    Eum_MATH_SUCCESS = 0,               // 成功
    Eum_MATH_ERROR_OVERFLOW = 1,        // 溢出错误
    Eum_MATH_ERROR_UNDERFLOW = 2,       // 下溢错误
    Eum_MATH_ERROR_DIVISION_BY_ZERO = 3, // 除零错误
    Eum_MATH_ERROR_INVALID_ARGUMENT = 4, // 无效参数错误
    Eum_MATH_ERROR_DOMAIN = 5,          // 定义域错误
    Eum_MATH_ERROR_RANGE = 6,           // 值域错误
    Eum_MATH_ERROR_NAN = 7,             // NaN错误
    Eum_MATH_ERROR_INFINITY = 8,        // 无穷大错误
    Eum_MATH_ERROR_NOT_IMPLEMENTED = 9  // 未实现错误
} Eum_CN_MathError_t;
```

### 数学结果结构体

#### Stru_CN_MathResult_t
```c
typedef struct Stru_CN_MathResult_t
{
    union
    {
        int32_t i32_result;     // 32位整数结果
        int64_t i64_result;     // 64位整数结果
        float   f32_result;     // 32位浮点数结果
        double  f64_result;     // 64位浮点数结果
    } value;                    // 运算结果值
    
    Eum_CN_MathError_t error;   // 错误码
    bool has_error;             // 是否有错误
} Stru_CN_MathResult_t;
```

## 模块管理 API

### 初始化函数

#### CN_math_init
```c
/**
 * @brief 初始化数学模块
 * 
 * @return 初始化成功返回true，失败返回false
 * @note 此函数初始化随机数生成器等需要初始化的组件
 */
bool CN_math_init(void);
```

#### CN_math_shutdown
```c
/**
 * @brief 关闭数学模块
 * 
 * @note 清理数学模块使用的资源
 */
void CN_math_shutdown(void);
```

## 安全算术模块 API

### 32位整数安全运算

#### CN_math_safe_add_i32
```c
/**
 * @brief 安全整数加法（带溢出检查）
 * 
 * @param a 第一个加数
 * @param b 第二个加数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_add_i32(int32_t a, int32_t b, int32_t* result);
```

#### CN_math_safe_sub_i32
```c
/**
 * @brief 安全整数减法（带溢出检查）
 * 
 * @param a 被减数
 * @param b 减数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_sub_i32(int32_t a, int32_t b, int32_t* result);
```

#### CN_math_safe_mul_i32
```c
/**
 * @brief 安全整数乘法（带溢出检查）
 * 
 * @param a 第一个乘数
 * @param b 第二个乘数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_mul_i32(int32_t a, int32_t b, int32_t* result);
```

#### CN_math_safe_div_i32
```c
/**
 * @brief 安全整数除法（带除零检查）
 * 
 * @param a 被除数
 * @param b 除数
 * @param result 存储结果的指针
 * @return 如果成功返回true，除零返回false
 */
bool CN_math_safe_div_i32(int32_t a, int32_t b, int32_t* result);
```

#### CN_math_safe_mod_i32
```c
/**
 * @brief 安全整数取模（带除零检查）
 * 
 * @param a 被除数
 * @param b 除数
 * @param result 存储结果的指针
 * @return 如果成功返回true，除零返回false
 */
bool CN_math_safe_mod_i32(int32_t a, int32_t b, int32_t* result);
```

### 64位整数安全运算

#### CN_math_safe_add_i64
```c
bool CN_math_safe_add_i64(int64_t a, int64_t b, int64_t* result);
```

#### CN_math_safe_sub_i64
```c
bool CN_math_safe_sub_i64(int64_t a, int64_t b, int64_t* result);
```

#### CN_math_safe_mul_i64
```c
bool CN_math_safe_mul_i64(int64_t a, int64_t b, int64_t* result);
```

#### CN_math_safe_div_i64
```c
bool CN_math_safe_div_i64(int64_t a, int64_t b, int64_t* result);
```

#### CN_math_safe_mod_i64
```c
bool CN_math_safe_mod_i64(int64_t a, int64_t b, int64_t* result);
```

### 无符号整数安全运算

#### CN_math_safe_add_u32
```c
bool CN_math_safe_add_u32(uint32_t a, uint32_t b, uint32_t* result);
```

#### CN_math_safe_sub_u32
```c
bool CN_math_safe_sub_u32(uint32_t a, uint32_t b, uint32_t* result);
```

#### CN_math_safe_mul_u32
```c
bool CN_math_safe_mul_u32(uint32_t a, uint32_t b, uint32_t* result);
```

#### CN_math_safe_div_u32
```c
bool CN_math_safe_div_u32(uint32_t a, uint32_t b, uint32_t* result);
```

#### CN_math_safe_mod_u32
```c
bool CN_math_safe_mod_u32(uint32_t a, uint32_t b, uint32_t* result);
```

### 浮点数安全运算

#### CN_math_safe_add_f
```c
/**
 * @brief 安全浮点数加法（带溢出/下溢检查）
 * 
 * @param a 第一个加数
 * @param b 第二个加数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出/下溢/NaN返回false
 */
bool CN_math_safe_add_f(float a, float b, float* result);
```

#### CN_math_safe_sub_f
```c
bool CN_math_safe_sub_f(float a, float b, float* result);
```

#### CN_math_safe_mul_f
```c
bool CN_math_safe_mul_f(float a, float b, float* result);
```

#### CN_math_safe_div_f
```c
bool CN_math_safe_div_f(float a, float b, float* result);
```

### 双精度浮点数安全运算

#### CN_math_safe_add_d
```c
bool CN_math_safe_add_d(double a, double b, double* result);
```

#### CN_math_safe_sub_d
```c
bool CN_math_safe_sub_d(double a, double b, double* result);
```

#### CN_math_safe_mul_d
```c
bool CN_math_safe_mul_d(double a, double b, double* result);
```

#### CN_math_safe_div_d
```c
bool CN_math_safe_div_d(double a, double b, double* result);
```

### 高级安全运算

#### CN_math_safe_pow_i32
```c
/**
 * @brief 安全整数幂运算（带溢出检查）
 * 
 * @param base 底数
 * @param exponent 指数
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_pow_i32(int32_t base, uint32_t exponent, int32_t* result);
```

#### CN_math_safe_abs_i32
```c
bool CN_math_safe_abs_i32(int32_t value, int32_t* result);
```

#### CN_math_safe_neg_i32
```c
bool CN_math_safe_neg_i32(int32_t value, int32_t* result);
```

#### CN_math_safe_avg_i32
```c
bool CN_math_safe_avg_i32(int32_t a, int32_t b, int32_t* result);
```

### 批量安全运算

#### CN_math_safe_sum_i32
```c
/**
 * @brief 安全整数数组求和（带溢出检查）
 * 
 * @param array 整数数组
 * @param count 数组元素数量
 * @param result 存储结果的指针
 * @return 如果成功返回true，溢出返回false
 */
bool CN_math_safe_sum_i32(const int32_t* array, size_t count, int32_t* result);
```

#### CN_math_safe_product_i32
```c
bool CN_math_safe_product_i32(const int32_t* array, size_t count, int32_t* result);
```

#### CN_math_safe_sum_f
```c
bool CN_math_safe_sum_f(const float* array, size_t count, float* result);
```

## 随机数生成器模块 API

### 随机数算法类型

#### Eum_CN_RandomAlgorithm_t
```c
typedef enum Eum_CN_RandomAlgorithm_t
{
    Eum_RANDOM_MT19937 = 0,      // Mersenne Twister 19937算法
    Eum_RANDOM_XORSHIFT = 1,     // Xorshift算法
    Eum_RANDOM_LCG = 2,          // 线性同余生成器
    Eum_RANDOM_PCG = 3,          // Permuted Congruential Generator
    Eum_RANDOM_SYSTEM = 4        // 系统随机数生成器
} Eum_CN_RandomAlgorithm_t;
```

### 随机数生成器管理

#### CN_math_random_create
```c
/**
 * @brief 创建随机数生成器
 * 
 * @param algorithm 随机数算法类型
 * @param seed 随机数种子（0表示使用当前时间）
 * @return 随机数生成器指针，失败返回NULL
 */
Stru_CN_RandomGenerator_t* CN_math_random_create(Eum_CN_RandomAlgorithm_t algorithm, uint64_t seed);
```

#### CN_math_random_destroy
```c
/**
 * @brief 销毁随机数生成器
 * 
 * @param generator 随机数生成器指针
 */
void CN_math_random_destroy(Stru_CN_RandomGenerator_t* generator);
```

#### CN_math_random_reseed
```c
/**
 * @brief 重新设置随机数种子
 * 
 * @param generator 随机数生成器指针
 * @param seed 新的随机数种子
 */
void CN_math_random_reseed(Stru_CN_RandomGenerator_t* generator, uint64_t seed);
```

#### CN_math_random_get_algorithm
```c
/**
 * @brief 获取随机数生成器算法类型
 * 
 * @param generator 随机数生成器指针
 * @return 算法类型
 */
Eum_CN_RandomAlgorithm_t CN_math_random_get_algorithm(const Stru_CN_RandomGenerator_t* generator);
```

### 基本随机数生成

#### CN_math_random_u32
```c
/**
 * @brief 生成32位无符号随机整数
 * 
 * @param generator 随机数生成器指针
 * @return 32位无符号随机整数
 */
uint32_t CN_math_random_u32(Stru_CN_RandomGenerator_t* generator);
```

#### CN_math_random_u64
```c
uint64_t CN_math_random_u64(Stru_CN_RandomGenerator_t* generator);
```

#### CN_math_random_i32
```c
int32_t CN_math_random_i32(Stru_CN_RandomGenerator_t* generator);
```

#### CN_math_random_i64
```c
int64_t CN_math_random_i64(Stru_CN_RandomGenerator_t* generator);
```

### 范围限制随机数生成

#### CN_math_random_range_i32
```c
/**
 * @brief 生成指定范围内的32位随机整数
 * 
 * @param generator 随机数生成器指针
 * @param min 最小值（包含）
 * @param max 最大值（包含）
 * @return 指定范围内的随机整数
 */
int32_t CN_math_random_range_i32(Stru_CN_RandomGenerator_t* generator, int32_t min, int32_t max);
```

#### CN_math_random_range_i64
```c
int64_t CN_math_random_range_i64(Stru_CN_RandomGenerator_t* generator, int64_t min, int64_t max);
```

#### CN_math_random_range_u32
```c
uint32_t CN_math_random_range_u32(Stru_CN_RandomGenerator_t* generator, uint32_t min, uint32_t max);
```

#### CN_math_random_range_u64
```c
uint64_t CN_math_random_range_u64(Stru_CN_RandomGenerator_t* generator, uint64_t min, uint64_t max);
```

### 浮点随机数生成

#### CN_math_random_float
```c
/**
 * @brief 生成[0.0, 1.0)范围内的随机浮点数
 * 
 * @param generator 随机数生成器指针
 * @return 随机浮点数
 */
float CN_math_random_float(Stru_CN_RandomGenerator_t* generator);
```

#### CN_math_random_double
```c
double CN_math_random_double(Stru_CN_RandomGenerator_t* generator);
```

#### CN_math_random_range_float
```c
float CN_math_random_range_float(Stru_CN_RandomGenerator_t* generator, float min, float max);
```

#### CN_math_random_range_double
```c
double CN_math_random_range_double(Stru_CN_RandomGenerator_t* generator, double min, double max);
```

### 高级随机数生成

#### CN_math_random_bool
```c
/**
 * @brief 生成随机布尔值
 * 
 * @param generator 随机数生成器指针
 * @param probability_true 为真的概率（0.0到1.0之间）
 * @return 随机布尔值
 */
bool CN_math_random_bool(Stru_CN_RandomGenerator_t* generator, float probability_true);
```

#### CN_math_random_choice
```c
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
```

#### CN_math_random_shuffle
```c
/**
 * @brief 打乱数组顺序（Fisher-Yates洗牌算法）
 * 
 * @param generator 随机数生成器指针
 * @param array 数组指针
 * @param count 数组元素数量
 * @param element_size 每个元素的大小（字节）
 */
void CN_math_random_shuffle(Stru_CN_RandomGenerator_t* generator, void* array, size_t count, size_t element_size);
```

### 字节随机数生成

#### CN_math_random_bytes
```c
/**
 * @brief 生成随机字节序列
 * 
 * @param generator 随机数生成器指针
 * @param buffer 存储随机字节的缓冲区
 * @param size 需要生成的字节数
 */
void CN_math_random_bytes(Stru_CN_RandomGenerator_t* generator, uint8_t* buffer, size_t size);
```

### 概率分布函数

#### CN_math_random_normal
```c
/**
 * @brief 生成正态分布（高斯分布）随机数
 * 
 * @param generator 随机数生成器指针
 * @param mean 均值
 * @param stddev 标准差
 * @return 正态分布随机数
 */
float CN_math_random_normal(Stru_CN_RandomGenerator_t* generator, float mean, float stddev);
```

#### CN_math_random_exponential
```c
/**
 * @brief 生成指数分布随机数
 * 
 * @param generator 随机数生成器指针
 * @param lambda 速率参数（λ）
 * @return 指数分布随机数
 */
float CN_math_random_exponential(Stru_CN_RandomGenerator_t* generator, float lambda);
```

#### CN_math_random_uniform
```c
/**
 * @brief 生成均匀分布随机数
 * 
 * @param generator 随机数生成器指针
 * @param min 最小值
 * @param max 最大值
 * @return 均匀分布随机数
 */
float CN_math_random_uniform(Stru_CN_RandomGenerator_t* generator, float min, float max);
```

#### CN_math_random_poisson
```c
/**
 * @brief 生成泊松分布随机数
 * 
 * @param generator 随机数生成器指针
 * @param lambda 平均发生率（λ）
 * @return 泊松分布随机数
 */
uint32_t CN_math_random_poisson(Stru_CN_RandomGenerator_t* generator, float lambda);
```

### 全局随机数函数

#### CN_math_global_random_u32
```c
/**
 * @brief 使用全局随机数生成器生成32位无符号随机整数
 * 
 * @return 32位无符号随机整数
 */
uint32_t CN_math_global_random_u32(void);
```

#### CN_math_global_random_range_i32
```c
/**
 * @brief 使用全局随机数生成器生成指定范围内的32位随机整数
 * 
 * @param min 最小值（包含）
 * @param max 最大值（包含）
 * @return 指定范围内的随机整数
 */
int32_t CN_math_global_random_range_i32(int32_t min, int32_t max);
```

#### CN_math_global_random_float
```c
/**
 * @brief 使用全局随机数生成器生成[0.0, 1.0)范围内的随机浮点数
 * 
 * @return 随机浮点数
 */
float CN_math_global_random_float(void);
```

#### CN_math_global_random_bool
```c
/**
 * @brief 使用全局随机数生成器生成随机布尔值
 * 
 * @param probability_true 为真的概率（0.0到1.0之间）
 * @return 随机布尔值
 */
bool CN_math_global_random_bool(float probability_true);
```

## 基本数学运算模块 API

### 基本算术运算

#### CN_math_abs
```c
/**
 * @brief 计算绝对值
 * 
 * @param value 输入值
 * @return 绝对值
 */
int32_t CN_math_abs(int32_t value);
```

#### CN_math_abs_f
```c
float CN_math_abs_f(float value);
```

#### CN_math_abs_d
```c
double CN_math_abs_d(double value);
```

#### CN_math_min
```c
/**
 * @brief 返回两个值中的较小值
 * 
 * @param a 第一个值
 * @param b 第二个值
 * @return 较小值
 */
int32_t CN_math_min(int32_t a, int32_t b);
```

#### CN_math_max
```c
/**
 * @brief 返回两个值中的较大值
 * 
 * @param a 第一个值
 * @param b 第二个值
 * @return 较大值
 */
int32_t CN_math_max(int32_t a, int32_t b);
```

#### CN_math_clamp
```c
/**
 * @brief 将值限制在指定范围内
 * 
 * @param value 输入值
 * @param min 最小值
 * @param max 最大值
 * @return 限制后的值
 */
int32_t CN_math_clamp(int32_t value, int32_t min, int32_t max);
```

#### CN_math_clamp_f
```c
float CN_math_clamp_f(float value, float min, float max);
```

#### CN_math_clamp_d
```c
double CN_math_clamp_d(double value, double min, double max);
```

### 幂运算和对数运算

#### CN_math_pow
```c
/**
 * @brief 计算幂运算
 * 
 * @param base 底数
 * @param exponent 指数
 * @return 幂运算结果
 */
double CN_math_pow(double base, double exponent);
```

#### CN_math_sqrt
```c
/**
 * @brief 计算平方根
 * 
 * @param value 输入值
 * @return 平方根
 */
double CN_math_sqrt(double value);
```

#### CN_math_cbrt
```c
/**
 * @brief 计算立方根
 * 
 * @param value 输入值
 * @return 立方根
 */
double CN_math_cbrt(double value);
```

#### CN_math_exp
```c
/**
 * @brief 计算指数函数 e^x
 * 
 * @param x 指数
 * @return e^x
 */
double CN_math_exp(double x);
```

#### CN_math_log
```c
/**
 * @brief 计算自然对数 ln(x)
 * 
 * @param x 输入值
 * @return 自然对数
 */
double CN_math_log(double x);
```

#### CN_math_log10
```c
/**
 * @brief 计算以10为底的对数 log10(x)
 * 
 * @param x 输入值
 * @return 以10为底的对数
 */
double CN_math_log10(double x);
```

#### CN_math_log2
```c
/**
 * @brief 计算以2为底的对数 log2(x)
 * 
 * @param x 输入值
 * @return 以2为底的对数
 */
double CN_math_log2(double x);
```

### 三角函数

#### CN_math_sin
```c
/**
 * @brief 计算正弦函数
 * 
 * @param angle 角度（弧度）
 * @return 正弦值
 */
double CN_math_sin(double angle);
```

#### CN_math_cos
```c
/**
 * @brief 计算余弦函数
 * 
 * @param angle 角度（弧度）
 * @return 余弦值
 */
double CN_math_cos(double angle);
```

#### CN_math_tan
```c
/**
 * @brief 计算正切函数
 * 
 * @param angle 角度（弧度）
 * @return 正切值
 */
double CN_math_tan(double angle);
```

#### CN_math_asin
```c
/**
 * @brief 计算反正弦函数
 * 
 * @param value 输入值（-1到1之间）
 * @return 反正弦值（弧度）
 */
double CN_math_asin(double value);
```

#### CN_math_acos
```c
/**
 * @brief 计算反余弦函数
 * 
 * @param value 输入值（-1到1之间）
 * @return 反余弦值（弧度）
 */
double CN_math_acos(double value);
```

#### CN_math_atan
```c
/**
 * @brief 计算反正切函数
 * 
 * @param value 输入值
 * @return 反正切值（弧度）
 */
double CN_math_atan(double value);
```

#### CN_math_atan2
```c
/**
 * @brief 计算两个参数的反正切函数
 * 
 * @param y y坐标
 * @param x x坐标
 * @return 反正切值（弧度）
 */
double CN_math_atan2(double y, double x);
```

### 双曲函数

#### CN_math_sinh
```c
/**
 * @brief 计算双曲正弦函数
 * 
 * @param x 输入值
 * @return 双曲正弦值
 */
double CN_math_sinh(double x);
```

#### CN_math_cosh
```c
/**
 * @brief 计算双曲余弦函数
 * 
 * @param x 输入值
 * @return 双曲余弦值
 */
double CN_math_cosh(double x);
```

#### CN_math_tanh
```c
/**
 * @brief 计算双曲正切函数
 * 
 * @param x 输入值
 * @return 双曲正切值
 */
double CN_math_tanh(double x);
```

### 角度转换函数

#### CN_math_deg_to_rad
```c
/**
 * @brief 将角度转换为弧度
 * 
 * @param degrees 角度值
 * @return 弧度值
 */
double CN_math_deg_to_rad(double degrees);
```

#### CN_math_rad_to_deg
```c
/**
 * @brief 将弧度转换为角度
 * 
 * @param radians 弧度值
 * @return 角度值
 */
double CN_math_rad_to_deg(double radians);
```

### 舍入和取整函数

#### CN_math_floor
```c
/**
 * @brief 向下取整
 * 
 * @param value 输入值
 * @return 向下取整后的值
 */
double CN_math_floor(double value);
```

#### CN_math_ceil
```c
/**
 * @brief 向上取整
 * 
 * @param value 输入值
 * @return 向上取整后的值
 */
double CN_math_ceil(double value);
```

#### CN_math_round
```c
/**
 * @brief 四舍五入
 * 
 * @param value 输入值
 * @return 四舍五入后的值
 */
double CN_math_round(double value);
```

#### CN_math_trunc
```c
/**
 * @brief 截断小数部分
 * 
 * @param value 输入值
 * @return 截断后的值
 */
double CN_math_trunc(double value);
```

### 浮点数比较函数

#### CN_math_approx_equal_f
```c
/**
 * @brief 比较两个浮点数是否近似相等
 * 
 * @param a 第一个浮点数
 * @param b 第二个浮点数
 * @param epsilon 允许的误差范围
 * @return 如果近似相等返回true，否则返回false
 */
bool CN_math_approx_equal_f(float a, float b, float epsilon);
```

#### CN_math_approx_equal_d
```c
bool CN_math_approx_equal_d(double a, double b, double epsilon);
```

#### CN_math_is_nan_f
```c
/**
 * @brief 检查浮点数是否为NaN
 * 
 * @param value 输入值
 * @return 如果是NaN返回true，否则返回false
 */
bool CN_math_is_nan_f(float value);
```

#### CN_math_is_inf_f
```c
/**
 * @brief 检查浮点数是否为无穷大
 * 
 * @param value 输入值
 * @return 如果是无穷大返回true，否则返回false
 */
bool CN_math_is_inf_f(float value);
```

#### CN_math_is_finite_f
```c
/**
 * @brief 检查浮点数是否为有限值
 * 
 * @param value 输入值
 * @return 如果是有限值返回true，否则返回false
 */
bool CN_math_is_finite_f(float value);
```

## 使用示例

### 示例1：安全算术运算
```c
#include "src/infrastructure/utils/math/CN_math.h"

int main() {
    // 初始化数学模块
    CN_math_init();
    
    // 安全整数加法
    int32_t a = 1000000000;
    int32_t b = 2000000000;
    int32_t result;
    
    if (CN_math_safe_add_i32(a, b, &result)) {
        printf("安全加法成功: %d + %d = %d\n", a, b, result);
    } else {
        printf("安全加法失败: 溢出\n");
    }
    
    // 关闭数学模块
    CN_math_shutdown();
    return 0;
}
```

### 示例2：随机数生成
```c
#include "src/infrastructure/utils/math/CN_math.h"

int main() {
    // 初始化数学模块
    CN_math_init();
    
    // 创建随机数生成器
    Stru_CN_RandomGenerator_t* rng = CN_math_random_create(Eum_RANDOM_MT19937, 12345);
    
    // 生成随机数
    for (int i = 0; i < 10; i++) {
        int32_t random_num = CN_math_random_range_i32(rng, 1, 100);
        printf("随机数 %d: %d\n", i + 1, random_num);
    }
    
    // 销毁随机数生成器
    CN_math_random_destroy(rng);
    
    // 关闭数学模块
    CN_math_shutdown();
    return 0;
}
```

### 示例3：基本数学运算
```c
#include "src/infrastructure/utils/math/CN_math.h"

int main() {
    // 初始化数学模块
    CN_math_init();
    
    // 计算三角函数
    double angle = CN_math_deg_to_rad(45.0);
    double sin_val = CN_math_sin(angle);
    double cos_val = CN_math_cos(angle);
    
    printf("sin(45°) = %f\n", sin_val);
    printf("cos(45°) = %f\n", cos_val);
    
    // 计算平方根和幂运算
    double sqrt_val = CN_math_sqrt(2.0);
    double pow_val = CN_math_pow(2.0, 3.0);
    
    printf("sqrt(2) = %f\n", sqrt_val);
    printf("2^3 = %f\n", pow_val);
    
    // 关闭数学模块
    CN_math_shutdown();
    return 0;
}
```

## 注意事项

1. **内存管理**：使用`CN_math_random_create`创建的随机数生成器必须使用`CN_math_random_destroy`销毁。
2. **模块初始化**：在使用数学模块的任何功能之前，必须先调用`CN_math_init`进行初始化。
3. **错误处理**：安全算术函数返回布尔值指示操作是否成功，调用者应检查返回值。
4. **线程安全**：数学模块不是线程安全的，在多线程环境中使用时需要外部同步。
5. **浮点数精度**：浮点数运算可能存在精度误差，比较浮点数时应使用`CN_math_approx_equal_f`或`CN_math_approx_equal_d`。
6. **资源清理**：程序退出前应调用`CN_math_shutdown`清理模块资源。

## 性能考虑

1. **安全算术运算**：安全算术运算比普通算术运算稍慢，因为需要额外的溢出检查。
2. **随机数生成**：不同的随机数算法有不同的性能特征：
   - Xorshift：最快，适合一般用途
   - Mersenne Twister：较慢但随机性更好
   - 系统随机数：最慢但安全性最高
3. **三角函数**：三角函数计算相对较慢，应避免在性能关键循环中频繁调用。
4. **内存使用**：随机数生成器会占用一定内存，使用后应及时销毁。

## 兼容性说明

1. **平台兼容性**：数学模块设计为跨平台兼容，支持Windows、Linux和macOS。
2. **编译器兼容性**：支持GCC、Clang和MSVC编译器。
3. **C标准兼容性**：遵循C99标准，可在符合C99标准的任何编译器中编译。
4. **依赖关系**：数学模块不依赖任何外部库，仅使用C标准库。

## 版本历史

- **v1.0.0** (2026-01-03): 初始版本发布
  - 安全算术运算模块
  - 随机数生成器模块
  - 基本数学运算模块
  - 完整的API文档

## 联系和支持

如有问题或建议，请联系：
- 项目维护者：CN_Language架构委员会
- 文档版本：1.0.0
- 最后更新：2026年1月3日
