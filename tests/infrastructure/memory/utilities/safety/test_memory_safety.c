/**
 * @file test_memory_safety.c
 * @brief 内存安全函数测试
 * 
 * 测试内存安全函数的正确性和安全性。
 * 包括安全清零、内存验证和模式初始化等功能的测试。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../../../../../src/infrastructure/memory/utilities/safety/CN_memory_safety.h"

/**
 * @brief 测试安全清零功能
 */
static bool test_secure_zero(void)
{
    printf("  测试安全清零... ");
    
    char sensitive_data[256];
    memset(sensitive_data, 'S', sizeof(sensitive_data));
    
    Stru_MemorySafetyInterface_t* safety = F_get_global_memory_safety();
    if (safety == NULL)
    {
        printf("❌ 无法获取内存安全接口\n");
        return false;
    }
    
    // 执行安全清零
    safety->secure_zero(sensitive_data, sizeof(sensitive_data));
    
    // 检查是否全部清零
    for (size_t i = 0; i < sizeof(sensitive_data); i++)
    {
        if (sensitive_data[i] != 0)
        {
            printf("❌ 安全清零失败\n");
            return false;
        }
    }
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试内存范围验证功能
 */
static bool test_validate_range(void)
{
    printf("  测试内存范围验证... ");
    
    Stru_MemorySafetyInterface_t* safety = F_get_global_memory_safety();
    if (safety == NULL)
    {
        printf("❌ 无法获取内存安全接口\n");
        return false;
    }
    
    // 测试有效范围
    char valid_buffer[100];
    bool result1 = safety->validate_range(valid_buffer, sizeof(valid_buffer));
    if (!result1)
    {
        printf("❌ 有效范围验证失败\n");
        return false;
    }
    
    // 测试NULL指针
    bool result2 = safety->validate_range(NULL, 100);
    if (result2) // NULL指针应该验证失败
    {
        printf("❌ NULL指针验证错误\n");
        return false;
    }
    
    // 测试零大小
    bool result3 = safety->validate_range(valid_buffer, 0);
    if (result3) // 零大小应该验证失败
    {
        printf("❌ 零大小验证错误\n");
        return false;
    }
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试模式初始化功能
 */
static bool test_initialize_with_pattern(void)
{
    printf("  测试模式初始化... ");
    
    char buffer[100];
    
    Stru_MemorySafetyInterface_t* safety = F_get_global_memory_safety();
    if (safety == NULL)
    {
        printf("❌ 无法获取内存安全接口\n");
        return false;
    }
    
    // 使用特定模式初始化
    uint8_t pattern = 0xAA;
    safety->initialize_with_pattern(buffer, sizeof(buffer), pattern);
    
    // 检查初始化结果
    for (size_t i = 0; i < sizeof(buffer); i++)
    {
        if (buffer[i] != pattern)
        {
            printf("❌ 模式初始化失败\n");
            return false;
        }
    }
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试初始化检查功能
 */
static bool test_check_initialized(void)
{
    printf("  测试初始化检查... ");
    
    char buffer[100];
    
    Stru_MemorySafetyInterface_t* safety = F_get_global_memory_safety();
    if (safety == NULL)
    {
        printf("❌ 无法获取内存安全接口\n");
        return false;
    }
    
    // 使用模式初始化
    uint8_t pattern = 0x55;
    safety->initialize_with_pattern(buffer, sizeof(buffer), pattern);
    
    // 检查是否已初始化
    bool result1 = safety->check_initialized(buffer, sizeof(buffer), pattern);
    if (!result1)
    {
        printf("❌ 已初始化检查失败\n");
        return false;
    }
    
    // 修改一个字节，应该检查失败
    buffer[50] = 0x00;
    bool result2 = safety->check_initialized(buffer, sizeof(buffer), pattern);
    if (result2) // 应该返回false
    {
        printf("❌ 未初始化检查失败\n");
        return false;
    }
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试边界条件
 */
static bool test_safety_edge_cases(void)
{
    printf("  测试安全边界条件... ");
    
    Stru_MemorySafetyInterface_t* safety = F_get_global_memory_safety();
    if (safety == NULL)
    {
        printf("❌ 无法获取内存安全接口\n");
        return false;
    }
    
    // 测试零大小操作
    char buffer[10];
    safety->secure_zero(buffer, 0); // 应该不崩溃
    safety->initialize_with_pattern(buffer, 0, 0xAA); // 应该不崩溃
    
    // 测试NULL指针（某些操作可能允许）
    // 这里我们只测试不崩溃
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 运行所有内存安全测试
 */
bool test_memory_safety_all(void)
{
    printf("\n运行内存安全函数测试\n");
    printf("====================\n\n");
    
    int passed = 0;
    int total = 0;
    
    // 运行各个测试
    if (test_secure_zero()) passed++; total++;
    if (test_validate_range()) passed++; total++;
    if (test_initialize_with_pattern()) passed++; total++;
    if (test_check_initialized()) passed++; total++;
    if (test_safety_edge_cases()) passed++; total++;
    
    printf("\n测试结果: %d/%d 通过\n", passed, total);
    
    return passed == total;
}

/**
 * @brief 测试主函数（独立测试时使用）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    if (test_memory_safety_all())
    {
        printf("\n✅ 所有内存安全测试通过！\n");
        return 0;
    }
    else
    {
        printf("\n❌ 部分内存安全测试失败！\n");
        return 1;
    }
}
#endif
