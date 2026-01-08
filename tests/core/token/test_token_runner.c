/**
 * @file test_token_runner.c
 * @brief CN_Language 令牌模块总测试运行器
 * 
 * 运行所有令牌模块的测试，包括：
 * 1. 令牌生命周期管理测试
 * 2. 令牌字面量值操作测试
 * 3. 令牌类型查询测试
 * 4. 令牌关键字管理测试
 * 5. 令牌工具函数测试
 * 6. 令牌接口测试
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdbool.h>

// 声明各个测试模块的函数
extern bool test_token_lifecycle_all(void);
extern bool test_token_values_all(void);
extern bool test_token_query_all(void);
extern bool test_token_keywords_all(void);
extern bool test_token_tools_all(void);
extern bool test_token_interface_all(void);

/**
 * @brief 运行所有令牌模块测试
 */
bool test_token_all(void)
{
    printf("\n========================================\n");
    printf("   令牌模块 - 总测试运行器\n");
    printf("========================================\n\n");
    
    int total_modules = 6;
    int passed_modules = 0;
    
    // 运行令牌生命周期管理测试
    printf("1. 令牌生命周期管理测试\n");
    printf("-------------------\n");
    if (test_token_lifecycle_all())
    {
        passed_modules++;
        printf("✅ 令牌生命周期管理测试通过\n");
    }
    else
    {
        printf("❌ 令牌生命周期管理测试失败\n");
    }
    printf("\n");
    
    // 运行令牌字面量值操作测试
    printf("2. 令牌字面量值操作测试\n");
    printf("-------------------\n");
    if (test_token_values_all())
    {
        passed_modules++;
        printf("✅ 令牌字面量值操作测试通过\n");
    }
    else
    {
        printf("❌ 令牌字面量值操作测试失败\n");
    }
    printf("\n");
    
    // 运行令牌类型查询测试
    printf("3. 令牌类型查询测试\n");
    printf("-------------------\n");
    if (test_token_query_all())
    {
        passed_modules++;
        printf("✅ 令牌类型查询测试通过\n");
    }
    else
    {
        printf("❌ 令牌类型查询测试失败\n");
    }
    printf("\n");
    
    // 运行令牌关键字管理测试
    printf("4. 令牌关键字管理测试\n");
    printf("-------------------\n");
    if (test_token_keywords_all())
    {
        passed_modules++;
        printf("✅ 令牌关键字管理测试通过\n");
    }
    else
    {
        printf("❌ 令牌关键字管理测试失败\n");
    }
    printf("\n");
    
    // 运行令牌工具函数测试
    printf("5. 令牌工具函数测试\n");
    printf("-------------------\n");
    if (test_token_tools_all())
    {
        passed_modules++;
        printf("✅ 令牌工具函数测试通过\n");
    }
    else
    {
        printf("❌ 令牌工具函数测试失败\n");
    }
    printf("\n");
    
    // 运行令牌接口测试
    printf("6. 令牌接口测试\n");
    printf("-------------------\n");
    if (test_token_interface_all())
    {
        passed_modules++;
        printf("✅ 令牌接口测试通过\n");
    }
    else
    {
        printf("❌ 令牌接口测试失败\n");
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
        printf("\n✅ 所有令牌模块测试通过！\n");
    }
    else
    {
        printf("\n❌ 部分令牌模块测试失败！\n");
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
    if (test_token_all())
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
#endif
