/******************************************************************************
 * 文件名: test_math_full.c
 * 功能: CN_Language数学模块完整测试程序
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现数学模块完整测试
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

// ============================================================================
// 测试函数声明
// ============================================================================

static bool test_math_core(void);
static bool test_safe_arithmetic(void);
static bool test_random_generator(void);
static bool test_basic_math(void);
static bool test_global_random(void);
static bool test_module_lifecycle(void);

// ============================================================================
// 主测试函数
// ============================================================================

int main(void)
{
    printf("CN_Language 数学模块完整测试程序\n");
    printf("================================\n");
    
    // 初始化数学模块
    if (!CN_math_init())
    {
        printf("错误: 无法初始化数学模块\n");
        return 1;
    }
    printf("数学模块初始化成功\n");
    
    bool all_tests_passed = true;
    
    // 运行各个测试
    if (!test_math_core()) all_tests_passed = false;
    if (!test_safe_arithmetic()) all_tests_passed = false;
    if (!test_random_generator()) all_tests_passed = false;
    if (!test_basic_math()) all_tests_passed = false;
    if (!test_global_random()) all_tests_passed = false;
    if (!test_module_lifecycle()) all_tests_passed = false;
    
    // 关闭数学模块
    CN_math_shutdown();
    printf("\n数学模块已关闭\n");
    
    // 输出测试结果
    printf("\n================================\n");
    printf("测试统计: %d/%d 通过\n", passed_tests, total_tests);
    if (all_tests_passed)
    {
        printf("所有测试通过！\n");
        return 0;
    }
    else
    {
        printf("部分测试失败！\n");
        return 1;
    }
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

static bool test_safe_arithmetic(void)
{
    printf("\n=== 安全算术运算测试 ===\n");
    
    bool all_passed = true;
    
    // 测试安全整数加法
    {
        int32_t a = 100, b = 200, result;
        bool success = CN_math_safe_add_i32(a, b, &result);
        bool passed = success && (result == 300);
        print_test_result("安全整数加法（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试安全整数加法溢出
    {
        int32_t a = INT32_MAX, b = 1, result;
        bool success = CN_math_safe_add_i32(a, b, &result);
        bool passed = !success; // 应该失败（溢出）
        print_test_result("安全整数加法（溢出）", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试安全整数减法
    {
        int32_t a = 100, b = 50, result;
        bool success = CN_math_safe_sub_i32(a, b, &result);
        bool passed = success && (result == 50);
        print_test_result("安全整数减法（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试安全整数乘法
    {
        int32_t a = 10, b = 20, result;
        bool success = CN_math_safe_mul_i32(a, b, &result);
        bool passed = success && (result == 200);
        print_test_result("安全整数乘法（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试安全整数除法
    {
        int32_t a = 100, b = 5, result;
        bool success = CN_math_safe_div_i32(a, b, &result);
        bool passed = success && (result == 20);
        print_test_result("安全整数除法（正常）", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试安全整数除零
    {
        int32_t a = 100, b = 0, result;
        bool success = CN_math_safe_div_i32(a, b, &result);
        bool passed = !success; // 应该失败（除零）
        print_test_result("安全整数除法（除零）", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试安全浮点数加法
    {
        float a = 1.5f, b = 2.5f, result;
        bool success = CN_math_safe_add_f(a, b, &result);
        bool passed = success && float_approx_equal(result, 4.0f, 0.0001f);
        print_test_result("安全浮点数加法", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试安全浮点数乘法溢出
    {
        float a = FLT_MAX, b = 2.0f, result;
        bool success = CN_math_safe_mul_f(a, b, &result);
        bool passed = !success; // 应该失败（溢出）
        print_test_result("安全浮点数乘法（溢出）", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试64位整数安全运算
    {
        int64_t a = 1000LL, b = 2000LL, result;
        bool success = CN_math_safe_add_i64(a, b, &result);
        bool passed = success && (result == 3000LL);
        print_test_result("安全64位整数加法", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试无符号整数安全运算
    {
        uint32_t a = 100, b = 200, result;
        bool success = CN_math_safe_add_u32(a, b, &result);
        bool passed = success && (result == 300);
        print_test_result("安全无符号整数加法", passed);
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
    
    return all_passed;
}

// ============================================================================
// 随机数生成器测试
// ============================================================================

static bool test_random_generator(void)
{
    printf("\n=== 随机数生成器测试 ===\n");
    
    bool all_passed = true;
    
    // 创建随机数生成器
    Stru_CN_RandomGenerator_t* rng = CN_math_random_create(Eum_RANDOM_XORSHIFT, 12345);
    if (rng == NULL)
    {
        print_test_result("创建随机数生成器", false);
        return false;
    }
    print_test_result("创建随机数生成器", true);
    
    // 测试生成随机整数范围
    {
        int32_t min = 1, max = 100;
        int32_t value = CN_math_random_range_i32(rng, min, max);
        bool passed = (value >= min && value <= max);
        print_test_result("生成范围内随机整数", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试生成随机浮点数
    {
        float value = CN_math_random_float(rng);
        bool passed = (value >= 0.0f && value < 1.0f);
        print_test_result("生成随机浮点数[0,1)", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试生成随机布尔值
    {
        bool value = CN_math_random_bool(rng, 0.5f);
        bool passed = (value == true || value == false);
        print_test_result("生成随机布尔值", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试重新设置种子
    CN_math_random_reseed(rng, 54321);
    uint32_t rand1 = CN_math_random_u32(rng);
    CN_math_random_reseed(rng, 54321);
    uint32_t rand2 = CN_math_random_u32(rng);
    bool passed = (rand1 == rand2); // 相同种子应该产生相同序列
    print_test_result("重新设置随机数种子", passed);
    if (!passed) all_passed = false;
    
    // 测试算法类型获取
    Eum_CN_RandomAlgorithm_t algo = CN_math_random_get_algorithm(rng);
    passed = (algo == Eum_RANDOM_XORSHIFT);
    print_test_result("获取随机数算法类型", passed);
    if (!passed) all_passed = false;
    
    // 销毁随机数生成器
    CN_math_random_destroy(rng);
    print_test_result("销毁随机数生成器", true);
    
    return all_passed;
}

// ============================================================================
// 基本数学运算测试
// ============================================================================

static bool test_basic_math(void)
{
    printf("\n=== 基本数学运算测试 ===\n");
    
    bool all_passed = true;
    
    // 测试绝对值
    {
        int32_t abs_val = CN_math_abs_i32(-10);
        bool passed = (abs_val == 10);
        print_test_result("绝对值计算", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试最大值和最小值
    {
        int32_t max_val = CN_math_max_i32(10, 20);
        int32_t min_val = CN_math_min_i32(10, 20);
        bool passed = (max_val == 20 && min_val == 10);
        print_test_result("最大值和最小值", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试限制函数
    {
        int32_t clamped = CN_math_clamp_i32(15, 10, 20);
        bool passed = (clamped == 15);
        print_test_result("限制函数（范围内）", passed);
        if (!passed) all_passed = false;
        
        clamped = CN_math_clamp_i32(5, 10, 20);
        passed = (clamped == 10);
        print_test_result("限制函数（小于最小值）", passed);
        if (!passed) all_passed = false;
        
        clamped = CN_math_clamp_i32(25, 10, 20);
        passed = (clamped == 20);
        print_test_result("限制函数（大于最大值）", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试角度转换
    {
        float radians = CN_math_deg_to_rad_f(180.0f);
        bool passed = float_approx_equal(radians, CN_MATH_PI, 0.0001f);
        print_test_result("角度转弧度", passed);
        if (!passed) all_passed = false;
        
        float degrees = CN_math_rad_to_deg_f(CN_MATH_PI);
        passed = float_approx_equal(degrees, 180.0f, 0.0001f);
        print_test_result("弧度转角度", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试平方根
    {
        float sqrt_val = CN_math_sqrt_f(25.0f);
        bool passed = float_approx_equal(sqrt_val, 5.0f, 1e-6f);
        print_test_result("平方根计算", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试幂运算
    {
        float pow_val = CN_math_pow_f(2.0f, 3.0f);
        bool passed = float_approx_equal(pow_val, 8.0f, 1e-6f);
        print_test_result("幂运算", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试三角函数
    {
        float sin_val = CN_math_sin_f(CN_MATH_PI_2);
        bool passed = float_approx_equal(sin_val, 1.0f, 1e-6f);
        print_test_result("正弦函数", passed);
        if (!passed) all_passed = false;
    }
    
    {
        float cos_val = CN_math_cos_f(0.0f);
        bool passed = float_approx_equal(cos_val, 1.0f, 1e-6f);
        print_test_result("余弦函数", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试对数函数
    {
        float log_val = CN_math_log_f(CN_MATH_E);
        bool passed = float_approx_equal(log_val, 1.0f, 1e-6f);
        print_test_result("自然对数", passed);
        if (!passed) all_passed = false;
    }
    
    {
        float log10_val = CN_math_log10_f(100.0f);
        bool passed = float_approx_equal(log10_val, 2.0f, 1e-6f);
        print_test_result("以10为底对数", passed);
        if (!passed) all_passed = false;
    }
    
    return all_passed;
}

// ============================================================================
// 全局随机数函数测试
// ============================================================================

static bool test_global_random(void)
{
    printf("\n=== 全局随机数函数测试 ===\n");
    
    bool all_passed = true;
    
    // 初始化全局随机数生成器
    bool init_success = CN_math_random_global_init(Eum_RANDOM_XORSHIFT, 12345);
    print_test_result("初始化全局随机数生成器", init_success);
    if (!init_success) all_passed = false;
    
    if (init_success)
    {
        // 测试全局随机整数
        {
            int32_t value = CN_math_random_global_range_i32(1, 100);
            bool passed = (value >= 1 && value <= 100);
            print_test_result("全局随机整数范围", passed);
            if (!passed) all_passed = false;
        }
        
        // 测试全局随机浮点数
        {
            float value = CN_math_random_global_float();
            bool passed = (value >= 0.0f && value < 1.0f);
            print_test_result("全局随机浮点数", passed);
            if (!passed) all_passed = false;
        }
        
        // 测试全局随机浮点数范围
        {
            float value = CN_math_random_global_float();
            bool passed = (value >= 0.0f && value < 1.0f);
            print_test_result("全局随机浮点数范围", passed);
            if (!passed) all_passed = false;
        }
        
        // 关闭全局随机数生成器
        CN_math_random_global_shutdown();
        print_test_result("关闭全局随机数生成器", true);
    }
    
    return all_passed;
}

// ============================================================================
// 模块生命周期测试
// ============================================================================

static bool test_module_lifecycle(void)
{
    printf("\n=== 模块生命周期测试 ===\n");
    
    bool all_passed = true;
    
    // 测试多次初始化和关闭
    for (int i = 0; i < 3; i++)
    {
        bool init_success = CN_math_init();
        bool passed = init_success;
        print_test_result("数学模块初始化", passed);
        if (!passed) all_passed = false;
        
        if (init_success)
        {
            // 使用一些功能确保模块正常工作
            int32_t a = 10, b = 20, result;
            bool add_success = CN_math_safe_add_i32(a, b, &result);
            passed = add_success && (result == 30);
            print_test_result("初始化后功能测试", passed);
            if (!passed) all_passed = false;
            
            CN_math_shutdown();
            print_test_result("数学模块关闭", true);
        }
    }
    
    return all_passed;
}
