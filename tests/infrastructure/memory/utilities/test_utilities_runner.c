/**
 * @file test_utilities_runner.c
 * @brief 内存工具函数总测试运行器
 * 
 * 运行所有内存工具函数的测试，包括：
 * 1. 内存操作函数测试
 * 2. 内存安全函数测试
 * 3. 内存对齐工具测试
 * 4. 内存统计工具测试
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdbool.h>

// 声明各个测试模块的函数
extern bool test_memory_operations_all(void);
extern bool test_memory_safety_all(void);
extern bool test_memory_alignment_all(void);
extern bool test_memory_statistics_all(void);

/**
 * @brief 运行所有内存工具函数测试
 */
bool test_utilities_all(void)
{
    printf("\n========================================\n");
    printf("   内存工具函数模块 - 总测试运行器\n");
    printf("========================================\n\n");
    
    int total_modules = 4;
    int passed_modules = 0;
    
    // 运行内存操作函数测试
    printf("1. 内存操作函数测试\n");
    printf("-------------------\n");
    if (test_memory_operations_all())
    {
        passed_modules++;
        printf("✅ 内存操作函数测试通过\n");
    }
    else
    {
        printf("❌ 内存操作函数测试失败\n");
    }
    printf("\n");
    
    // 运行内存安全函数测试
    printf("2. 内存安全函数测试\n");
    printf("-------------------\n");
    if (test_memory_safety_all())
    {
        passed_modules++;
        printf("✅ 内存安全函数测试通过\n");
    }
    else
    {
        printf("❌ 内存安全函数测试失败\n");
    }
    printf("\n");
    
    // 运行内存对齐工具测试
    printf("3. 内存对齐工具测试\n");
    printf("-------------------\n");
    if (test_memory_alignment_all())
    {
        passed_modules++;
        printf("✅ 内存对齐工具测试通过\n");
    }
    else
    {
        printf("❌ 内存对齐工具测试失败\n");
    }
    printf("\n");
    
    // 运行内存统计工具测试
    printf("4. 内存统计工具测试\n");
    printf("-------------------\n");
    if (test_memory_statistics_all())
    {
        passed_modules++;
        printf("✅ 内存统计工具测试通过\n");
    }
    else
    {
        printf("❌ 内存统计工具测试失败\n");
    }
    printf("\n");
    
    // 打印总结果
    printf("========================================\n");
    printf("测试总结:\n");
    printf("  总模块数: %d\n", total_modules);
    printf("  通过模块: %d\n", passed_modules);
    printf("  失败模块: %d\n", total_modules - passed_modules);
    
    if (passed_modules == total_modules)
    {
        printf("\n✅ 所有内存工具函数测试通过！\n");
    }
    else
    {
        printf("\n❌ 部分内存工具函数测试失败！\n");
    }
    printf("========================================\n\n");
    
    return passed_modules == total_modules;
}

/**
 * @brief 主函数（独立测试时使用）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    if (test_utilities_all())
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
#endif
