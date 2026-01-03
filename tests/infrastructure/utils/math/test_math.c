/******************************************************************************
 * 文件名: test_math.c
 * 功能: CN_Language数学模块测试程序
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现数学模块测试
 * 版权: MIT许可证
 ******************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// 包含数学模块头文件
#include "../../../../src/infrastructure/utils/math/CN_math.h"

// ============================================================================
// 测试辅助函数
// ============================================================================

static void print_test_result(const char* test_name, bool passed)
{
    if (passed)
    {
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
        int32_t abs_val = CN_math_abs(-10);
        bool passed = (abs_val == 10);
        print_test_result("绝对值计算", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试最大值和最小值
    {
        int32_t max_val = CN_math_max(10, 20);
        int32_t min_val = CN_math_min(10, 20);
        bool passed = (max_val == 20 && min_val == 10);
        print_test_result("最大值和最小值", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试限制函数
    {
        int32_t clamped = CN_math_clamp(15, 10, 20);
        bool passed = (clamped == 15);
        print_test_result("限制函数（范围内）", passed);
        if (!passed) all_passed = false;
        
        clamped = CN_math_clamp(5, 10, 20);
        passed = (clamped == 10);
        print_test_result("限制函数（小于最小值）", passed);
        if (!passed) all_passed = false;
        
        clamped = CN_math_clamp(25, 10, 20);
        passed = (clamped == 20);
        print_test_result("限制函数（大于最大值）", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试角度转换
    {
        double radians = CN_math_deg_to_rad(180.0);
        bool passed = float_approx_equal((float)radians, (float)3.14159265f, 0.0001f);
        print_test_result("角度转弧度", passed);
        if (!passed) all_passed = false;
        
        double degrees = CN_math_rad_to_deg(3.14159265);
        passed = float_approx_equal((float)degrees, 180.0f, 0.0001f);
        print_test_result("弧度转角度", passed);
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
    
    // 测试全局随机整数
    {
        int32_t value = CN_math_global_random_range_i32(1, 100);
        bool passed = (value >= 1 && value <= 100);
        print_test_result("全局随机整数范围", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试全局随机浮点数
    {
        float value = CN_math_global_random_float();
        bool passed = (value >= 0.0f && value < 1.0f);
        print_test_result("全局随机浮点数", passed);
        if (!passed) all_passed = false;
    }
    
    // 测试全局随机布尔值
    {
        bool value = CN_math_global_random_bool(0.3f);
        bool passed = (value == true || value == false);
        print_test_result("全局随机布尔值", passed);
        if (!passed) all_passed = false;
    }
    
    return all_passed;
}

// ============================================================================
// 主测试函数
// ============================================================================

int main(void)
{
    printf("CN_Language 数学模块测试程序\n");
    printf("============================\n");
    
    // 初始化数学模块
    if (!CN_math_init())
    {
        printf("错误: 无法初始化数学模块\n");
        return 1;
    }
    printf("数学模块初始化成功\n");
    
    bool all_tests_passed = true;
    
    // 运行各个测试
    if (!test_safe_arithmetic()) all_tests_passed = false;
    if (!test_random_generator()) all_tests_passed = false;
    if (!test_basic_math()) all_tests_passed = false;
    if (!test_global_random()) all_tests_passed = false;
    
    // 关闭数学模块
    CN_math_shutdown();
    printf("\n数学模块已关闭\n");
    
    // 输出测试结果
    printf("\n============================\n");
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
