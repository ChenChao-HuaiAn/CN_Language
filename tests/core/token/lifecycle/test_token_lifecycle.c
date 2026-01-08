/**
 * @file test_token_lifecycle.c
 * @brief CN_Language 令牌生命周期管理模块测试
 * 
 * 测试令牌生命周期管理功能，包括创建、销毁、复制等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "../../../src/core/token/lifecycle/CN_token_lifecycle.h"

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
static void test_create_token(void);
static void test_destroy_token(void);
static void test_copy_token(void);
static void test_equals_token(void);
static void test_get_token_properties(void);

/**
 * @brief 运行所有令牌生命周期测试
 */
bool test_token_lifecycle_all(void)
{
    printf("\n========================================\n");
    printf("   令牌生命周期管理模块测试\n");
    printf("========================================\n\n");
    
    // 重置测试计数器
    g_test_count = 0;
    g_test_passed = 0;
    
    // 运行测试
    test_create_token();
    test_destroy_token();
    test_copy_token();
    test_equals_token();
    test_get_token_properties();
    
    // 打印测试结果
    printf("\n========================================\n");
    printf("测试总结:\n");
    printf("  总测试数: %d\n", g_test_count);
    printf("  通过测试: %d\n", g_test_passed);
    printf("  失败测试: %d\n", g_test_count - g_test_passed);
    
    if (g_test_passed == g_test_count) {
        printf("\n✅ 所有令牌生命周期测试通过！\n");
        return true;
    } else {
        printf("\n❌ 部分令牌生命周期测试失败！\n");
        return false;
    }
}

/**
 * @brief 测试创建令牌
 */
static void test_create_token(void)
{
    printf("测试创建令牌:\n");
    printf("-------------------\n");
    
    // 测试1: 创建有效令牌
    Stru_Token_t* token = F_token_lifecycle_create(
        Eum_TOKEN_KEYWORD_VAR, "变量", 1, 1);
    TEST_ASSERT(token != NULL, "创建有效令牌");
    TEST_ASSERT(token->type == Eum_TOKEN_KEYWORD_VAR, "令牌类型正确");
    TEST_ASSERT(strcmp(token->lexeme, "变量") == 0, "令牌词素正确");
    TEST_ASSERT(token->line == 1, "令牌行号正确");
    TEST_ASSERT(token->column == 1, "令牌列号正确");
    
    if (token != NULL) {
        F_token_lifecycle_destroy(token);
    }
    
    // 测试2: 创建空词素令牌（应该失败）
    Stru_Token_t* token2 = F_token_lifecycle_create(
        Eum_TOKEN_IDENTIFIER, NULL, 1, 1);
    TEST_ASSERT(token2 == NULL, "创建空词素令牌失败");
    
    // 测试3: 创建不同位置的令牌
    Stru_Token_t* token3 = F_token_lifecycle_create(
        Eum_TOKEN_LITERAL_INTEGER, "123", 10, 20);
    TEST_ASSERT(token3 != NULL, "创建位置令牌");
    TEST_ASSERT(token3->line == 10, "令牌行号正确");
    TEST_ASSERT(token3->column == 20, "令牌列号正确");
    
    if (token3 != NULL) {
        F_token_lifecycle_destroy(token3);
    }
    
    printf("\n");
}

/**
 * @brief 测试销毁令牌
 */
static void test_destroy_token(void)
{
    printf("测试销毁令牌:\n");
    printf("-------------------\n");
    
    // 测试1: 销毁有效令牌
    Stru_Token_t* token = F_token_lifecycle_create(
        Eum_TOKEN_KEYWORD_IF, "如果", 1, 1);
    TEST_ASSERT(token != NULL, "创建测试令牌");
    
    F_token_lifecycle_destroy(token);
    // 销毁后不应访问令牌，这里只是测试不会崩溃
    TEST_ASSERT(true, "销毁令牌不会崩溃");
    
    // 测试2: 销毁空指针
    F_token_lifecycle_destroy(NULL);
    TEST_ASSERT(true, "销毁空指针不会崩溃");
    
    printf("\n");
}

/**
 * @brief 测试复制令牌
 */
static void test_copy_token(void)
{
    printf("测试复制令牌:\n");
    printf("-------------------\n");
    
    // 测试1: 复制有效令牌
    Stru_Token_t* original = F_token_lifecycle_create(
        Eum_TOKEN_LITERAL_INTEGER, "42", 5, 10);
    TEST_ASSERT(original != NULL, "创建原始令牌");
    
    // 设置字面量值
    original->literal_value.int_value = 42;
    
    Stru_Token_t* copy = F_token_lifecycle_copy(original);
    TEST_ASSERT(copy != NULL, "复制令牌");
    TEST_ASSERT(copy != original, "复制的是新令牌");
    TEST_ASSERT(copy->type == original->type, "复制类型正确");
    TEST_ASSERT(strcmp(copy->lexeme, original->lexeme) == 0, "复制词素正确");
    TEST_ASSERT(copy->line == original->line, "复制行号正确");
    TEST_ASSERT(copy->column == original->column, "复制列号正确");
    TEST_ASSERT(copy->literal_value.int_value == original->literal_value.int_value, "复制字面量值正确");
    
    // 测试2: 复制空指针
    Stru_Token_t* copy2 = F_token_lifecycle_copy(NULL);
    TEST_ASSERT(copy2 == NULL, "复制空指针返回空");
    
    // 清理
    if (original != NULL) {
        F_token_lifecycle_destroy(original);
    }
    if (copy != NULL) {
        F_token_lifecycle_destroy(copy);
    }
    
    printf("\n");
}

/**
 * @brief 测试比较令牌
 */
static void test_equals_token(void)
{
    printf("测试比较令牌:\n");
    printf("-------------------\n");
    
    // 创建测试令牌
    Stru_Token_t* token1 = F_token_lifecycle_create(
        Eum_TOKEN_KEYWORD_VAR, "变量", 1, 1);
    Stru_Token_t* token2 = F_token_lifecycle_create(
        Eum_TOKEN_KEYWORD_VAR, "变量", 1, 1);
    Stru_Token_t* token3 = F_token_lifecycle_create(
        Eum_TOKEN_KEYWORD_IF, "如果", 1, 1);
    Stru_Token_t* token4 = F_token_lifecycle_create(
        Eum_TOKEN_KEYWORD_VAR, "var", 1, 1);
    Stru_Token_t* token5 = F_token_lifecycle_create(
        Eum_TOKEN_KEYWORD_VAR, "变量", 2, 1);
    
    TEST_ASSERT(token1 != NULL && token2 != NULL, "创建测试令牌");
    
    // 测试1: 相同令牌比较
    TEST_ASSERT(F_token_lifecycle_equals(token1, token2) == true, "相同令牌比较");
    
    // 测试2: 不同类型令牌比较
    TEST_ASSERT(F_token_lifecycle_equals(token1, token3) == false, "不同类型令牌比较");
    
    // 测试3: 不同词素令牌比较
    TEST_ASSERT(F_token_lifecycle_equals(token1, token4) == false, "不同词素令牌比较");
    
    // 测试4: 不同位置令牌比较
    TEST_ASSERT(F_token_lifecycle_equals(token1, token5) == false, "不同位置令牌比较");
    
    // 测试5: 空指针比较
    TEST_ASSERT(F_token_lifecycle_equals(NULL, NULL) == true, "两个空指针比较");
    TEST_ASSERT(F_token_lifecycle_equals(token1, NULL) == false, "令牌与空指针比较");
    TEST_ASSERT(F_token_lifecycle_equals(NULL, token1) == false, "空指针与令牌比较");
    
    // 清理
    if (token1 != NULL) F_token_lifecycle_destroy(token1);
    if (token2 != NULL) F_token_lifecycle_destroy(token2);
    if (token3 != NULL) F_token_lifecycle_destroy(token3);
    if (token4 != NULL) F_token_lifecycle_destroy(token4);
    if (token5 != NULL) F_token_lifecycle_destroy(token5);
    
    printf("\n");
}

/**
 * @brief 测试获取令牌属性
 */
static void test_get_token_properties(void)
{
    printf("测试获取令牌属性:\n");
    printf("-------------------\n");
    
    // 创建测试令牌
    Stru_Token_t* token = F_token_lifecycle_create(
        Eum_TOKEN_KEYWORD_WHILE, "当", 3, 15);
    TEST_ASSERT(token != NULL, "创建测试令牌");
    
    // 测试获取令牌类型
    Eum_TokenType type = F_token_lifecycle_get_type(token);
    TEST_ASSERT(type == Eum_TOKEN_KEYWORD_WHILE, "获取令牌类型");
    
    // 测试获取空令牌类型
    type = F_token_lifecycle_get_type(NULL);
    TEST_ASSERT(type == Eum_TOKEN_ERROR, "获取空令牌类型返回错误");
    
    // 测试获取令牌词素
    const char* lexeme = F_token_lifecycle_get_lexeme(token);
    TEST_ASSERT(strcmp(lexeme, "当") == 0, "获取令牌词素");
    
    // 测试获取空令牌词素
    lexeme = F_token_lifecycle_get_lexeme(NULL);
    TEST_ASSERT(strcmp(lexeme, "") == 0, "获取空令牌词素返回空字符串");
    
    // 测试获取令牌位置
    size_t line = 0, column = 0;
    F_token_lifecycle_get_position(token, &line, &column);
    TEST_ASSERT(line == 3 && column == 15, "获取令牌位置");
    
    // 测试获取空令牌位置
    line = column = 0;
    F_token_lifecycle_get_position(NULL, &line, &column);
    TEST_ASSERT(line == 0 && column == 0, "获取空令牌位置不修改参数");
    
    // 测试空指针参数
    F_token_lifecycle_get_position(token, NULL, NULL);
    TEST_ASSERT(true, "空指针参数不会崩溃");
    
    // 清理
    if (token != NULL) {
        F_token_lifecycle_destroy(token);
    }
    
    printf("\n");
}

/**
 * @brief 主函数（独立测试时使用）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    if (test_token_lifecycle_all()) {
        return 0;
    } else {
        return 1;
    }
}
#endif
