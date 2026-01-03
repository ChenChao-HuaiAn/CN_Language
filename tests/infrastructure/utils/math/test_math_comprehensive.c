/******************************************************************************
 * 文件名: test_math_comprehensive.c
 * 功能: CN_Language数学模块全面测试程序
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现数学模块全面测试
 * 版权: MIT许可证
 ******************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <limits.h>

// 包含数学模块头文件
#include "../../../../src/infrastructure/utils/math/CN_math.h"

// ============================================================================
// 测试辅助函数
// ============================================================================

static int total_tests = 0;
static int passed_tests = 0;

static void print_test_result(const char* test_name, bool passed)
{
    total_tests++;
    if (passed)
    {
        passed_tests++;
        printf("✓ %s: 通过\n", test_name);
    }
    else
    {
        printf("✗ %s: 失败\n", test_name);
    }
}

static bool float_approx_equal(float a, float b, float epsilon)
{
    float diff = a - b;
    if (diff < 0) diff = -diff;
    return diff < epsilon;
}

static bool double_approx_equal(double a, double b, double epsilon)
{
    double diff = a - b;
    if (diff < 0) diff = -diff;
    return diff < epsilon;
}

// ============================================================================
// 数学核心模块测试
// ============================================================================

static bool test_math_core(void)
{
    printf("\n=== 数学核心模块测试 ===\n");
    
    bool all_passed = true;
    
    // 测试数学常量
    {
        bool passed = float_approx_equal(CN_MATH_PI, 3.14159265358979323846f, 1e-6f);
        print_test_result("数学常量PI", passed);
        if (!passed) all_passed = false;
    }
    
    {
        bool passed = float_approx_equal(CN_MATH_E, 2.71828182845904523536f, 1e-6f);
        print_test_result("数学常量E", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试浮点数检查函数
    {
        float nan_val = NAN;
        float inf_val = INFINITY;
        float normal_val = 3.14f;
        
        bool passed = CN_math_is_nan_f(nan_val) && 
                     !CN_math_is_nan_f(inf_val) && 
                     !CN_math_is_nan_f(normal_val);
        print_test_result("浮点数NaN检查", passed);
        if (!passed) all_passed = false;
    }
    
    {
        float inf_val = INFINITY;
        float nan_val = NAN;
        float normal_val = 3.14f;
        
        bool passed = CN_math_is_inf_f(inf_val) && 
                     !CN_math_is_inf_f(nan_val) && 
                     !CN_math_is_inf_f(normal_val);
        print_test_result("浮点数无穷大检查", passed);
        if (!passed) all_passed = false;
    }
    
    {
        float normal_val = 3.14f;
        float inf_val = INFINITY;
        float nan_val = NAN;
        
        bool passed = CN_math_is_finite_f(normal_val) && 
                     !CN_math_is_finite_f(inf_val) && 
                     !CN_math_is_finite_f(nan_val);
        print_test_result("浮点数有限性检查", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试浮点数相等比较
    {
        float a = 1.0f / 3.0f;
        float b = 0.33333333f;
        bool passed = CN_math_float_equal_f(a, b, 1e-6f);
        print_test_result("浮点数相等比较", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试错误码字符串
    {
        const char* str = CN_math_error_string(Eum_MATH_SUCCESS);
        bool passed = (str != NULL && strlen(str) > 0);
        print_test_result("错误码字符串", passed);
        if (!passed) all_passed = false;
    }
    
    return all_passed;
}

// ============================================================================
// 安全算术运算测试
// ============================================================================

static bool test_safe_arithmetic_comprehensive(void)
{
    printf("\n=== 安全算术运算全面测试 ===\n");
    
    bool all_passed = true;
    
    // 测试32位整数安全运算
    {
        int32_t a = 100, b = 200, result;
        bool success = CN_math_safe_add_i32(a, b, &result);
        bool passed = success && (result == 300);
        print_test_result("安全32位整数加法（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        int32_t a = INT32_MAX, b = 1, result;
        bool success = CN_math_safe_add_i32(a, b, &result);
        bool passed = !success; // 应该失败（溢出）
        print_test_result("安全32位整数加法（溢出）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        int32_t a = INT32_MIN, b = -1, result;
        bool success = CN_math_safe_add_i32(a, b, &result);
        bool passed = !success; // 应该失败（下溢）
        print_test_result("安全32位整数加法（下溢）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        int32_t a = 100, b = 50, result;
        bool success = CN_math_safe_sub_i32(a, b, &result);
        bool passed = success && (result == 50);
        print_test_result("安全32位整数减法（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        int32_t a = INT32_MIN, b = 1, result;
        bool success = CN_math_safe_sub_i32(a, b, &result);
        bool passed = !success; // 应该失败（下溢）
        print_test_result("安全32位整数减法（下溢）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        int32_t a = 10, b = 20, result;
        bool success = CN_math_safe_mul_i32(a, b, &result);
        bool passed = success && (result == 200);
        print_test_result("安全32位整数乘法（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        int32_t a = INT32_MAX / 2 + 1, b = 2, result;
        bool success = CN_math_safe_mul_i32(a, b, &result);
        bool passed = !success; // 应该失败（溢出）
        print_test_result("安全32位整数乘法（溢出）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        int32_t a = 100, b = 5, result;
        bool success = CN_math_safe_div_i32(a, b, &result);
        bool passed = success && (result == 20);
        print_test_result("安全32位整数除法（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        int32_t a = 100, b = 0, result;
        bool success = CN_math_safe_div_i32(a, b, &result);
        bool passed = !success; // 应该失败（除零）
        print_test_result("安全32位整数除法（除零）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        int32_t a = 10, b = 3, result;
        bool success = CN_math_safe_mod_i32(a, b, &result);
        bool passed = success && (result == 1);
        print_test_result("安全32位整数取模（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试64位整数安全运算
    {
        int64_t a = 1000LL, b = 2000LL, result;
        bool success = CN_math_safe_add_i64(a, b, &result);
        bool passed = success && (result == 3000LL);
        print_test_result("安全64位整数加法（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        int64_t a = INT64_MAX, b = 1, result;
        bool success = CN_math_safe_add_i64(a, b, &result);
        bool passed = !success; // 应该失败（溢出）
        print_test_result("安全64位整数加法（溢出）", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试无符号整数安全运算
    {
        uint32_t a = 100, b = 200, result;
        bool success = CN_math_safe_add_u32(a, b, &result);
        bool passed = success && (result == 300);
        print_test_result("安全无符号32位整数加法（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        uint32_t a = UINT32_MAX, b = 1, result;
        bool success = CN_math_safe_add_u32(a, b, &result);
        bool passed = !success; // 应该失败（溢出）
        print_test_result("安全无符号32位整数加法（溢出）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        uint32_t a = 100, b = 50, result;
        bool success = CN_math_safe_sub_u32(a, b, &result);
        bool passed = success && (result == 50);
        print_test_result("安全无符号32位整数减法（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        uint32_t a = 50, b = 100, result;
        bool success = CN_math_safe_sub_u32(a, b, &result);
        bool passed = !success; // 应该失败（下溢）
        print_test_result("安全无符号32位整数减法（下溢）", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试浮点数安全运算
    {
        float a = 1.5f, b = 2.5f, result;
        bool success = CN_math_safe_add_f(a, b, &result);
        bool passed = success && float_approx_equal(result, 4.0f, 1e-6f);
        print_test_result("安全浮点数加法（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        float a = FLT_MAX, b = FLT_MAX, result;
        bool success = CN_math_safe_add_f(a, b, &result);
        bool passed = !success; // 应该失败（溢出）
        print_test_result("安全浮点数加法（溢出）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        float a = 1e-30f, b = 1e-30f, result;
        bool success = CN_math_safe_mul_f(a, b, &result);
        bool passed = !success; // 应该失败（下溢）
        print_test_result("安全浮点数乘法（下溢）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        float a = 1.0f, b = 0.0f, result;
        bool success = CN_math_safe_div_f(a, b, &result);
        bool passed = !success; // 应该失败（除零）
        print_test_result("安全浮点数除法（除零）", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试双精度浮点数安全运算
    {
        double a = 1.5, b = 2.5, result;
        bool success = CN_math_safe_add_d(a, b, &result);
        bool passed = success && double_approx_equal(result, 4.0, 1e-12);
        print_test_result("安全双精度浮点数加法（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试高级安全运算
    {
        int32_t base = 2, exponent = 10, result;
        bool success = CN_math_safe_pow_i32(base, exponent, &result);
        bool passed = success && (result == 1024);
        print_test_result("安全整数幂运算（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        int32_t value = -10, result;
        bool success = CN_math_safe_abs_i32(value, &result);
        bool passed = success && (result == 10);
        print_test_result("安全整数绝对值（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        int32_t value = INT32_MIN, result;
        bool success = CN_math_safe_abs_i32(value, &result);
        bool passed = !success; // 应该失败（溢出）
        print_test_result("安全整数绝对值（溢出）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        int32_t a = 10, b = 20, result;
        bool success = CN_math_safe_avg_i32(a, b, &result);
        bool passed = success && (result == 15);
        print_test_result("安全整数平均值", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试批量安全运算
    {
        int32_t array[] = {1, 2, 3, 4, 5};
        int32_t result;
        bool success = CN_math_safe_sum_i32(array, 5, &result);
        bool passed = success && (result == 15);
        print_test_result("安全整数数组求和", passed);
        if (!passed) all_passed = false;
    }
    
    {
        int32_t array[] = {INT32_MAX, 1};
        int32_t result;
        bool success = CN_math_safe_sum_i32(array, 2, &result);
        bool passed = !success; // 应该失败（溢出）
        print_test_result("安全整数数组求和（溢出）", passed);
        if (!passed) all_passed = false;
    }
    
    {
        float f_array[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
        float result;
        bool success = CN_math_safe_sum_f(f_array, 5, &result);
        bool passed = success && float_approx_equal(result, 15.0f, 1e-6f);
        print_test_result("安全浮点数数组求和", passed);
        if (!passed) all_passed = false;
    }
    
    return all_passed;
}

// ============================================================================
// 随机数生成器全面测试
// ============================================================================

static bool test_random_generator_comprehensive(void)
{
    printf("\n=== 随机数生成器全面测试 ===\n");
    
    bool all_passed = true;
    
    // 测试所有算法类型的随机数生成器
    Eum_CN_RandomAlgorithm_t algorithms[] = {
        Eum_RANDOM_XORSHIFT,
        Eum_RANDOM_MT19937,
        Eum_RANDOM_LCG,
        Eum_RANDOM_SYSTEM
    };
    
    const char* algorithm_names[] = {
        "Xorshift",
        "Mersenne Twister",
        "线性同余生成器",
        "系统随机数"
    };
    
    for (int i = 0; i < 4; i++)
    {
        Stru_CN_RandomGenerator_t* rng = CN_math_random_create(algorithms[i], 12345);
        if (rng == NULL)
        {
            printf("✗ 创建%s随机数生成器: 失败\n", algorithm_names[i]);
            all_passed = false;
            continue;
        }
        
        // 测试算法类型获取
        Eum_CN_RandomAlgorithm_t algo = CN_math_random_get_algorithm(rng);
        bool passed = (algo == algorithms[i]);
        print_test_result(algorithm_names[i], passed);
        if (!passed) all_passed = false;
        
        // 测试基本随机数生成
        {
            uint32_t rand_u32 = CN_math_random_u32(rng);
            bool passed = (rand_u32 >= 0);
            print_test_result("生成32位无符号随机数", passed);
            if (!passed) all_passed = false;
        }
        
        {
            uint64_t rand_u64 = CN_math_random_u64(rng);
            bool passed = (rand_u64 >= 0);
            print_test_result("生成64位无符号随机数", passed);
            if (!passed) all_passed = false;
        }
        
        {
            int32_t rand_i32 = CN_math_random_i32(rng);
            bool passed = (rand_i32 >= INT32_MIN && rand_i32 <= INT32_MAX);
            print_test_result("生成32位有符号随机数", passed);
            if (!passed) all_passed = false;
        }
        
        // 测试范围限制随机数
        {
            int32_t min = 1, max = 100;
            int32_t
