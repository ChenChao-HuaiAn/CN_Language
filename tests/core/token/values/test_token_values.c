/**
 * @file test_token_values.c
 * @brief CN_Language 令牌字面量值操作模块测试
 * 
 * 测试令牌字面量值的设置和获取功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "../../../../src/core/token/values/CN_token_values.h"
#include "../../../../src/core/token/lifecycle/CN_token_lifecycle.h"

// 测试计数器
static int g_test_count = 0;
static int g_test_passed = 0;

// 测试辅助宏
#define TEST_ASSERT(condition, message) \
    do { \
        g_test_count++; \
        if (condition) { \
            g_test_passed++; \
            printf("✅ 测试 %d 通过: %s\n", g_test_count, message); \
        } else { \
            printf("❌ 测试 %d 失败: %s\n", g_test_count, message); \
        } \
    } while (0)

// 测试函数声明
static void test_set_int_value(void);
static void test_set_float_value(void);
static void test_set_bool_value(void);
static void test_get_int_value(void);
static void test_get_float_value(void);
static void test_get_bool_value(void);
static void test_invalid_token_operations(void);

/**
 * @brief 运行所有令牌字面量值操作测试
 */
bool test_token_values_all(void)
{
    printf("\n========================================\n");
    printf("   令牌字面量值操作模块测试\n");
    printf("========================================\n\n");
    
    // 重置测试计数器
    g_test_count = 0;
    g_test_passed = 0;
    
    // 运行测试
    test_set_int_value();
    test_set_float_value();
    test_set_bool_value();
    test_get_int_value();
    test_get_float_value();
    test_get_bool_value();
    test_invalid_token_operations();
    
    // 打印测试结果
    printf("\n========================================\n");
    printf("测试总结:\n");
    printf("  总测试数: %d\n", g_test_count);
    printf("  通过测试: %d\n", g_test_passed);
    printf("  失败测试: %d\n", g_test_count - g_test_passed);
    
    if (g_test_passed == g_test_count) {
        printf("\n✅ 所有令牌字面量值操作测试通过！\n");
        return true;
    } else {
        printf("\n❌ 部分令牌字面量值操作测试失败！\n");
        return false;
    }
}

/**
 * @brief 测试设置整数字面量值
 */
static void test_set_int_value(void)
{
    printf("测试设置整数字面量值:\n");
    printf("-------------------\n");
    
    // 创建整数字面量令牌
    Stru_Token_t* token = F_token_lifecycle_create(
        Eum_TOKEN_LITERAL_INTEGER, "42", 1, 1);
    TEST_ASSERT(token != NULL, "创建整数字面量令牌");
    
    // 测试设置整数值
    F_token_values_set_int(token, 42);
    TEST_ASSERT(token->literal_value.int_value == 42, "设置整数值");
    
    // 测试设置负整数值
    F_token_values_set_int(token, -100);
    TEST_ASSERT(token->literal_value.int_value == -100, "设置负整数值");
    
    // 测试设置大整数值
    F_token_values_set_int(token, 2147483647);
    TEST_ASSERT(token->literal_value.int_value == 2147483647, "设置大整数值");
    
    // 清理
    if (token != NULL) {
        F_token_lifecycle_destroy(token);
    }
    
    printf("\n");
}

/**
 * @brief 测试设置浮点数字面量值
 */
static void test_set_float_value(void)
{
    printf("测试设置浮点数字面量值:\n");
    printf("-------------------\n");
    
    // 创建浮点数字面量令牌
    Stru_Token_t* token = F_token_lifecycle_create(
        Eum_TOKEN_LITERAL_FLOAT, "3.14", 1, 1);
    TEST_ASSERT(token != NULL, "创建浮点数字面量令牌");
    
    // 测试设置浮点数值
    F_token_values_set_float(token, 3.14);
    TEST_ASSERT(token->literal_value.float_value == 3.14, "设置浮点数值");
    
    // 测试设置负浮点数值
    F_token_values_set_float(token, -2.718);
    TEST_ASSERT(token->literal_value.float_value == -2.718, "设置负浮点数值");
    
    // 测试设置科学计数法值
    F_token_values_set_float(token, 1.23e-4);
    TEST_ASSERT(token->literal_value.float_value == 1.23e-4, "设置科学计数法值");
    
    // 清理
    if (token != NULL) {
        F_token_lifecycle_destroy(token);
    }
    
    printf("\n");
}

/**
 * @brief 测试设置布尔字面量值
 */
static void test_set_bool_value(void)
{
    printf("测试设置布尔字面量值:\n");
    printf("-------------------\n");
    
    // 创建布尔字面量令牌
    Stru_Token_t* token = F_token_lifecycle_create(
        Eum_TOKEN_LITERAL_BOOLEAN, "真", 1, 1);
    TEST_ASSERT(token != NULL, "创建布尔字面量令牌");
    
    // 测试设置真值
    F_token_values_set_bool(token, true);
    TEST_ASSERT(token->literal_value.bool_value == true, "设置布尔真值");
    
    // 测试设置假值
    F_token_values_set_bool(token, false);
    TEST_ASSERT(token->literal_value.bool_value == false, "设置布尔假值");
    
    // 清理
    if (token != NULL) {
        F_token_lifecycle_destroy(token);
    }
    
    printf("\n");
}

/**
 * @brief 测试获取整数字面量值
 */
static void test_get_int_value(void)
{
    printf("测试获取整数字面量值:\n");
    printf("-------------------\n");
    
    // 创建整数字面量令牌
    Stru_Token_t* token = F_token_lifecycle_create(
        Eum_TOKEN_LITERAL_INTEGER, "42", 1, 1);
    TEST_ASSERT(token != NULL, "创建整数字面量令牌");
    
    // 设置值并获取
    token->literal_value.int_value = 42;
    long value = F_token_values_get_int(token);
    TEST_ASSERT(value == 42, "获取整数值");
    
    // 测试负值
    token->literal_value.int_value = -100;
    value = F_token_values_get_int(token);
    TEST_ASSERT(value == -100, "获取负整数值");
    
    // 测试大值
    token->literal_value.int_value = 2147483647;
    value = F_token_values_get_int(token);
    TEST_ASSERT(value == 2147483647, "获取大整数值");
    
    // 清理
    if (token != NULL) {
        F_token_lifecycle_destroy(token);
    }
    
    printf("\n");
}

/**
 * @brief 测试获取浮点数字面量值
 */
static void test_get_float_value(void)
{
    printf("测试获取浮点数字面量值:\n");
    printf("-------------------\n");
    
    // 创建浮点数字面量令牌
    Stru_Token_t* token = F_token_lifecycle_create(
        Eum_TOKEN_LITERAL_FLOAT, "3.14", 1, 1);
    TEST_ASSERT(token != NULL, "创建浮点数字面量令牌");
    
    // 设置值并获取
    token->literal_value.float_value = 3.14;
    double value = F_token_values_get_float(token);
    TEST_ASSERT(value == 3.14, "获取浮点数值");
    
    // 测试负值
    token->literal_value.float_value = -2.718;
    value = F_token_values_get_float(token);
    TEST_ASSERT(value == -2.718, "获取负浮点数值");
    
    // 测试科学计数法值
    token->literal_value.float_value = 1.23e-4;
    value = F_token_values_get_float(token);
    TEST_ASSERT(value == 1.23e-4, "获取科学计数法值");
    
    // 清理
    if (token != NULL) {
        F_token_lifecycle_destroy(token);
    }
    
    printf("\n");
}

/**
 * @brief 测试获取布尔字面量值
 */
static void test_get_bool_value(void)
{
    printf("测试获取布尔字面量值:\n");
    printf("-------------------\n");
    
    // 创建布尔字面量令牌
    Stru_Token_t* token = F_token_lifecycle_create(
        Eum_TOKEN_LITERAL_BOOLEAN, "真", 1, 1);
    TEST_ASSERT(token != NULL, "创建布尔字面量令牌");
    
    // 设置真值并获取
    token->literal_value.bool_value = true;
    bool value = F_token_values_get_bool(token);
    TEST_ASSERT(value == true, "获取布尔真值");
    
    // 设置假值并获取
    token->literal_value.bool_value = false;
    value = F_token_values_get_bool(token);
    TEST_ASSERT(value == false, "获取布尔假值");
    
    // 清理
    if (token != NULL) {
        F_token_lifecycle_destroy(token);
    }
    
    printf("\n");
}

/**
 * @brief 测试无效令牌操作
 */
static void test_invalid_token_operations(void)
{
    printf("测试无效令牌操作:\n");
    printf("-------------------\n");
    
    // 测试空指针操作
    F_token_values_set_int(NULL, 42);
    TEST_ASSERT(true, "设置空令牌整数值不会崩溃");
    
    F_token_values_set_float(NULL, 3.14);
    TEST_ASSERT(true, "设置空令牌浮点数值不会崩溃");
    
    F_token_values_set_bool(NULL, true);
    TEST_ASSERT(true, "设置空令牌布尔值不会崩溃");
    
    // 测试获取空令牌值
    long int_value = F_token_values_get_int(NULL);
    TEST_ASSERT(int_value == 0, "获取空令牌整数值返回0");
    
    double float_value = F_token_values_get_float(NULL);
    TEST_ASSERT(float_value == 0.0, "获取空令牌浮点数值返回0.0");
    
    bool bool_value = F_token_values_get_bool(NULL);
    TEST_ASSERT(bool_value == false, "获取空令牌布尔值返回false");
    
    // 测试非字面量令牌操作
    Stru_Token_t* keyword_token = F_token_lifecycle_create(
        Eum_TOKEN_KEYWORD_VAR, "变量", 1, 1);
    TEST_ASSERT(keyword_token != NULL, "创建关键字令牌");
    
    // 对关键字令牌设置值（应该不会崩溃）
    F_token_values_set_int(keyword_token, 42);
    TEST_ASSERT(true, "对关键字令牌设置整数值不会崩溃");
    
    // 对关键字令牌获取值
    int_value = F_token_values_get_int(keyword_token);
    TEST_ASSERT(int_value == 0, "对关键字令牌获取整数值返回0");
    
    // 清理
    if (keyword_token != NULL) {
        F_token_lifecycle_destroy(keyword_token);
    }
    
    printf("\n");
}

/**
 * @brief 主函数（独立测试时使用）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    if (test_token_values_all()) {
        return 0;
    } else {
        return 1;
    }
}
#endif
