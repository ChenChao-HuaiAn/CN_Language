/**
 * @file test_lexer_all.c
 * @brief 词法分析器模块总测试运行器
 * 
 * 运行词法分析器模块的所有测试，包括接口测试、扫描器测试、关键字测试等。
 * 
 * @author CN_Language测试团队
 * @date 2026-01-08
 * @version 1.0.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// 声明各个测试文件的测试函数
// 注意：这些函数应该在各自的测试文件中定义

// 接口测试函数（来自test_lexer_interface.c）
extern bool test_lexer_interface_all(void);

// 扫描器测试函数（来自test_lexer_scanner.c）
extern bool test_lexer_scanner_all(void);

// 关键字测试函数（来自test_lexer_keywords.c）
extern bool test_lexer_keywords_all(void);

// 运算符测试函数（来自test_lexer_operators.c）
extern bool test_lexer_operators_all(void);

// 工具函数测试（待实现）
extern bool test_lexer_utils_all(void);

/**
 * @brief 运行词法分析器接口测试
 */
static bool run_lexer_interface_tests(void)
{
    printf("运行词法分析器接口测试...\n");
    
    bool result = test_lexer_interface_all();
    
    printf("词法分析器接口测试 %s\n\n", result ? "✅ 通过" : "❌ 失败");
    return result;
}

/**
 * @brief 运行扫描器测试
 */
static bool run_scanner_tests(void)
{
    printf("运行扫描器测试...\n");
    
    bool result = test_lexer_scanner_all();
    
    printf("扫描器测试 %s\n\n", result ? "✅ 通过" : "❌ 失败");
    return result;
}

/**
 * @brief 运行关键字测试
 */
static bool run_keyword_tests(void)
{
    printf("运行关键字测试...\n");
    
    bool result = test_lexer_keywords_all();
    
    printf("关键字测试 %s\n\n", result ? "✅ 通过" : "❌ 失败");
    return result;
}

/**
 * @brief 运行运算符测试
 */
static bool run_operator_tests(void)
{
    printf("运行运算符测试...\n");
    
    bool result = test_lexer_operators_all();
    
    printf("运算符测试 %s\n\n", result ? "✅ 通过" : "❌ 失败");
    return result;
}

/**
 * @brief 运行工具函数测试
 */
static bool run_utility_tests(void)
{
    printf("运行工具函数测试...\n");
    
    // 暂时返回true，待实现
    printf("工具函数测试 ⚠️ 待实现\n\n");
    return true;
}

/**
 * @brief 运行所有词法分析器测试
 * 
 * 这个函数被主测试运行器调用，运行词法分析器模块的所有测试。
 * 
 * @return true 所有测试通过
 * @return false 有测试失败
 */
bool test_lexer_all(void)
{
    printf("\n");
    printf("========================================\n");
    printf("    词法分析器模块 - 完整测试套件\n");
    printf("========================================\n");
    printf("\n");
    
    bool all_passed = true;
    
    // 运行各个子模块的测试
    if (!run_lexer_interface_tests()) {
        all_passed = false;
    }
    
    if (!run_scanner_tests()) {
        all_passed = false;
    }
    
    if (!run_keyword_tests()) {
        all_passed = false;
    }
    
    if (!run_operator_tests()) {
        all_passed = false;
    }
    
    if (!run_utility_tests()) {
        all_passed = false;
    }
    
    // 打印总结
    printf("\n");
    printf("========================================\n");
    printf("    词法分析器模块测试总结\n");
    printf("========================================\n");
    
    if (all_passed) {
        printf("✅ 所有测试通过！\n");
    } else {
        printf("❌ 有测试失败\n");
    }
    
    printf("\n");
    
    return all_passed;
}

/**
 * @brief 主函数（用于独立运行词法分析器测试）
 * 
 * 这个函数允许词法分析器测试独立运行，而不需要主测试运行器。
 */
int main(void)
{
    printf("独立运行词法分析器模块测试...\n\n");
    
    bool success = test_lexer_all();
    
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
