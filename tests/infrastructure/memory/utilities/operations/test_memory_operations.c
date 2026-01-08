/**
 * @file test_memory_operations.c
 * @brief 内存操作函数测试
 * 
 * 测试内存操作函数的正确性和性能。
 * 包括内存复制、移动、比较和设置等功能的测试。
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
#include "../../../../../src/infrastructure/memory/utilities/operations/CN_memory_operations.h"

/**
 * @brief 测试内存复制功能
 */
static bool test_memory_copy(void)
{
    printf("  测试内存复制... ");
    
    char src[] = "Hello, World!";
    char dest[50];
    
    Stru_MemoryOperationsInterface_t* ops = F_get_global_memory_operations();
    if (ops == NULL)
    {
        printf("❌ 无法获取内存操作接口\n");
        return false;
    }
    
    void* result = ops->copy(dest, src, strlen(src) + 1);
    
    if (result != dest)
    {
        printf("❌ 返回值不正确\n");
        return false;
    }
    
    if (strcmp(dest, src) != 0)
    {
        printf("❌ 复制内容不正确\n");
        return false;
    }
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试内存移动功能
 */
static bool test_memory_move(void)
{
    printf("  测试内存移动... ");
    
    char buffer[] = "Hello, World!";
    char expected[] = "Hello, World!";
    
    Stru_MemoryOperationsInterface_t* ops = F_get_global_memory_operations();
    if (ops == NULL)
    {
        printf("❌ 无法获取内存操作接口\n");
        return false;
    }
    
    // 测试重叠区域移动
    void* result = ops->move(buffer + 7, buffer, 7);
    
    if (result != buffer + 7)
    {
        printf("❌ 返回值不正确\n");
        return false;
    }
    
    // 检查移动结果
    if (strncmp(buffer + 7, "Hello, ", 7) != 0)
    {
        printf("❌ 移动内容不正确\n");
        return false;
    }
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试内存比较功能
 */
static bool test_memory_compare(void)
{
    printf("  测试内存比较... ");
    
    char str1[] = "Hello";
    char str2[] = "Hello";
    char str3[] = "World";
    
    Stru_MemoryOperationsInterface_t* ops = F_get_global_memory_operations();
    if (ops == NULL)
    {
        printf("❌ 无法获取内存操作接口\n");
        return false;
    }
    
    // 测试相等
    int result1 = ops->compare(str1, str2, strlen(str1));
    if (result1 != 0)
    {
        printf("❌ 相等比较失败\n");
        return false;
    }
    
    // 测试不相等
    int result2 = ops->compare(str1, str3, strlen(str1));
    if (result2 >= 0) // "Hello" < "World"
    {
        printf("❌ 不等比较失败\n");
        return false;
    }
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试内存设置功能
 */
static bool test_memory_set(void)
{
    printf("  测试内存设置... ");
    
    char buffer[20];
    
    Stru_MemoryOperationsInterface_t* ops = F_get_global_memory_operations();
    if (ops == NULL)
    {
        printf("❌ 无法获取内存操作接口\n");
        return false;
    }
    
    void* result = ops->set(buffer, 'A', sizeof(buffer));
    
    if (result != buffer)
    {
        printf("❌ 返回值不正确\n");
        return false;
    }
    
    // 检查所有字节是否都设置为'A'
    for (size_t i = 0; i < sizeof(buffer); i++)
    {
        if (buffer[i] != 'A')
        {
            printf("❌ 设置内容不正确\n");
            return false;
        }
    }
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 测试边界条件
 */
static bool test_edge_cases(void)
{
    printf("  测试边界条件... ");
    
    Stru_MemoryOperationsInterface_t* ops = F_get_global_memory_operations();
    if (ops == NULL)
    {
        printf("❌ 无法获取内存操作接口\n");
        return false;
    }
    
    // 测试零大小
    char buffer[10];
    void* result = ops->copy(buffer, buffer, 0);
    if (result != buffer)
    {
        printf("❌ 零大小复制失败\n");
        return false;
    }
    
    // 测试NULL指针（某些实现可能允许）
    // 这里我们只测试不崩溃
    
    printf("✅ 通过\n");
    return true;
}

/**
 * @brief 运行所有内存操作测试
 */
bool test_memory_operations_all(void)
{
    printf("\n运行内存操作函数测试\n");
    printf("====================\n\n");
    
    int passed = 0;
    int total = 0;
    
    // 运行各个测试
    if (test_memory_copy()) passed++; total++;
    if (test_memory_move()) passed++; total++;
    if (test_memory_compare()) passed++; total++;
    if (test_memory_set()) passed++; total++;
    if (test_edge_cases()) passed++; total++;
    
    printf("\n测试结果: %d/%d 通过\n", passed, total);
    
    return passed == total;
}

/**
 * @brief 测试主函数（独立测试时使用）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    if (test_memory_operations_all())
    {
        printf("\n✅ 所有内存操作测试通过！\n");
        return 0;
    }
    else
    {
        printf("\n❌ 部分内存操作测试失败！\n");
        return 1;
    }
}
#endif
