/**
 * @file test_lexer_interface.c
 * @brief 词法分析器接口测试
 * 
 * 测试词法分析器接口的基本功能，包括初始化、令牌获取、错误处理等。
 * 
 * @author CN_Language测试团队
 * @date 2026-01-08
 * @version 1.0.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "src/core/lexer/CN_lexer_interface.h"
#include "src/core/token/CN_token.h"

// ============================================
// 测试辅助函数
// ============================================

// 全局测试状态变量
static bool g_test_failed = false;
static const char* g_test_failure_message = NULL;
static const char* g_test_failure_file = NULL;
static int g_test_failure_line = 0;

/**
 * @brief 重置测试状态
 */
static void reset_test_state(void)
{
    g_test_failed = false;
    g_test_failure_message = NULL;
    g_test_failure_file = NULL;
    g_test_failure_line = 0;
}

/**
 * @brief 简单的测试断言宏
 */
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            if (!g_test_failed) { \
                g_test_failed = true; \
                g_test_failure_message = #condition; \
                g_test_failure_file = __FILE__; \
                g_test_failure_line = __LINE__; \
                printf("  断言失败: %s (文件: %s, 行: %d)\n", #condition, __FILE__, __LINE__); \
            } \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            if (!g_test_failed) { \
                g_test_failed = true; \
                g_test_failure_message = "值不相等"; \
                g_test_failure_file = __FILE__; \
                g_test_failure_line = __LINE__; \
                printf("  断言失败: 期望 %lld, 实际 %lld (文件: %s, 行: %d)\n", (long long)(expected), (long long)(actual), __FILE__, __LINE__); \
            } \
        } \
    } while (0)

#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            if (!g_test_failed) { \
                g_test_failed = true; \
                g_test_failure_message = "指针为NULL"; \
                g_test_failure_file = __FILE__; \
                g_test_failure_line = __LINE__; \
                printf("  断言失败: 指针为NULL (文件: %s, 行: %d)\n", __FILE__, __LINE__); \
            } \
        } \
    } while (0)

#define TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            if (!g_test_failed) { \
                g_test_failed = true; \
                g_test_failure_message = "指针不为NULL"; \
                g_test_failure_file = __FILE__; \
                g_test_failure_line = __LINE__; \
                printf("  断言失败: 指针不为NULL (文件: %s, 行: %d)\n", __FILE__, __LINE__); \
            } \
        } \
    } while (0)

#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))

#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            if (!g_test_failed) { \
                g_test_failed = true; \
                g_test_failure_message = "字符串不相等"; \
                g_test_failure_file = __FILE__; \
                g_test_failure_line = __LINE__; \
                printf("  断言失败: 期望字符串 '%s', 实际字符串 '%s' (文件: %s, 行: %d)\n", (expected), (actual), __FILE__, __LINE__); \
            } \
        } \
    } while (0)

#define TEST_ASSERT_NOT_EQUAL_STRING(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) == 0) { \
            if (!g_test_failed) { \
                g_test_failed = true; \
                g_test_failure_message = "字符串不应相等"; \
                g_test_failure_file = __FILE__; \
                g_test_failure_line = __LINE__; \
                printf("  断言失败: 字符串不应相等 '%s' (文件: %s, 行: %d)\n", (expected), __FILE__, __LINE__); \
            } \
        } \
    } while (0)

#define TEST_ASSERT_GREATER_OR_EQUAL(min, actual) \
    do { \
        if ((actual) < (min)) { \
            if (!g_test_failed) { \
                g_test_failed = true; \
                g_test_failure_message = "值小于最小值"; \
                g_test_failure_file = __FILE__; \
                g_test_failure_line = __LINE__; \
                printf("  断言失败: 实际值 %lld 小于最小值 %lld (文件: %s, 行: %d)\n", (long long)(actual), (long long)(min), __FILE__, __LINE__); \
            } \
        } \
    } while (0)

// ============================================
// 测试用例
// ============================================

/**
 * @brief 测试词法分析器创建和销毁
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_lexer_creation_and_destruction(void)
{
    printf("测试：词法分析器创建和销毁\n");
    
    reset_test_state();
    
    // 创建词法分析器
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    TEST_ASSERT_NOT_NULL(lexer);
    
    // 检查接口函数指针
    TEST_ASSERT_NOT_NULL(lexer->initialize);
    TEST_ASSERT_NOT_NULL(lexer->next_token);
    TEST_ASSERT_NOT_NULL(lexer->has_more_tokens);
    TEST_ASSERT_NOT_NULL(lexer->tokenize_all);
    TEST_ASSERT_NOT_NULL(lexer->get_position);
    TEST_ASSERT_NOT_NULL(lexer->get_source_name);
    TEST_ASSERT_NOT_NULL(lexer->has_errors);
    TEST_ASSERT_NOT_NULL(lexer->get_last_error);
    TEST_ASSERT_NOT_NULL(lexer->reset);
    TEST_ASSERT_NOT_NULL(lexer->destroy);
    
    // 销毁词法分析器
    lexer->destroy(lexer);
    
    if (g_test_failed) {
        printf("  失败\n");
        return false;
    }
    
    printf("  通过\n");
    return true;
}

/**
 * @brief 测试词法分析器初始化
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_lexer_initialization(void)
{
    printf("测试：词法分析器初始化\n");
    
    reset_test_state();
    
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    TEST_ASSERT_NOT_NULL(lexer);
    
    // 有效初始化
    const char* source = "变量 x = 42";
    bool success = lexer->initialize(lexer, source, strlen(source), "test.cn");
    TEST_ASSERT_TRUE(success);
    
    // 无效初始化（空源代码）
    success = lexer->initialize(lexer, NULL, 0, "test.cn");
    TEST_ASSERT_FALSE(success);
    
    // 检查错误状态
    bool has_errors = lexer->has_errors(lexer);
    TEST_ASSERT_TRUE(has_errors);
    
    const char* error = lexer->get_last_error(lexer);
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_NOT_EQUAL_STRING("", error);
    
    lexer->destroy(lexer);
    
    if (g_test_failed) {
        printf("  失败\n");
        return false;
    }
    
    printf("  通过\n");
    return true;
}

/**
 * @brief 测试简单令牌识别
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_simple_token_recognition(void)
{
    printf("测试：简单令牌识别\n");
    
    reset_test_state();
    
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    TEST_ASSERT_NOT_NULL(lexer);
    
    const char* source = "变量 x = 42";
    bool success = lexer->initialize(lexer, source, strlen(source), "test.cn");
    TEST_ASSERT_TRUE(success);
    
    // 检查是否有更多令牌
    bool has_more = lexer->has_more_tokens(lexer);
    TEST_ASSERT_TRUE(has_more);
    
    // 获取第一个令牌（关键字"变量"）
    Stru_Token_t* token = lexer->next_token(lexer);
    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_EQUAL(Eum_TOKEN_KEYWORD_VAR, token->type);
    TEST_ASSERT_EQUAL_STRING("变量", token->lexeme);
    F_destroy_token(token);
    
    // 获取第二个令牌（标识符"x"）
    token = lexer->next_token(lexer);
    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_EQUAL(Eum_TOKEN_IDENTIFIER, token->type);
    TEST_ASSERT_EQUAL_STRING("x", token->lexeme);
    F_destroy_token(token);
    
    // 获取第三个令牌（运算符"="）
    token = lexer->next_token(lexer);
    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_EQUAL(Eum_TOKEN_OPERATOR_ASSIGN, token->type);
    TEST_ASSERT_EQUAL_STRING("=", token->lexeme);
    F_destroy_token(token);
    
    // 获取第四个令牌（整数"42"）
    token = lexer->next_token(lexer);
    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_EQUAL(Eum_TOKEN_LITERAL_INTEGER, token->type);
    TEST_ASSERT_EQUAL_STRING("42", token->lexeme);
    F_destroy_token(token);
    
    // 检查是否还有更多令牌
    has_more = lexer->has_more_tokens(lexer);
    TEST_ASSERT_FALSE(has_more);
    
    lexer->destroy(lexer);
    
    if (g_test_failed) {
        printf("  失败\n");
        return false;
    }
    
    printf("  通过\n");
    return true;
}

/**
 * @brief 测试批量令牌化
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_batch_tokenization(void)
{
    printf("测试：批量令牌化\n");
    
    reset_test_state();
    
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    TEST_ASSERT_NOT_NULL(lexer);
    
    const char* source = "变量 x = 42 + 3.14";
    bool success = lexer->initialize(lexer, source, strlen(source), "test.cn");
    TEST_ASSERT_TRUE(success);
    
    // 批量令牌化
    Stru_DynamicArray_t* tokens = lexer->tokenize_all(lexer);
    TEST_ASSERT_NOT_NULL(tokens);
    
    // 检查令牌数量
    size_t token_count = F_dynamic_array_length(tokens);
    TEST_ASSERT_EQUAL(7, token_count); // 变量, x, =, 42, +, 3.14, EOF
    
    // 检查令牌类型
    for (size_t i = 0; i < token_count; i++)
    {
        Stru_Token_t** token_ptr = (Stru_Token_t**)F_dynamic_array_get(tokens, i);
        TEST_ASSERT_NOT_NULL(token_ptr);
        TEST_ASSERT_NOT_NULL(*token_ptr);
        
        Stru_Token_t* token = *token_ptr;
        
        // 根据位置检查令牌类型
        switch (i)
        {
            case 0:
                TEST_ASSERT_EQUAL(Eum_TOKEN_KEYWORD_VAR, token->type);
                break;
            case 1:
                TEST_ASSERT_EQUAL(Eum_TOKEN_IDENTIFIER, token->type);
                break;
            case 2:
                TEST_ASSERT_EQUAL(Eum_TOKEN_OPERATOR_ASSIGN, token->type);
                break;
            case 3:
                TEST_ASSERT_EQUAL(Eum_TOKEN_LITERAL_INTEGER, token->type);
                break;
            case 4:
                TEST_ASSERT_EQUAL(Eum_TOKEN_OPERATOR_PLUS, token->type);
                break;
            case 5:
                TEST_ASSERT_EQUAL(Eum_TOKEN_LITERAL_FLOAT, token->type);
                break;
            case 6:
                TEST_ASSERT_EQUAL(Eum_TOKEN_EOF, token->type);
                break;
        }
        
        F_destroy_token(token);
    }
    
    // 清理动态数组
    F_destroy_dynamic_array(tokens);
    
    lexer->destroy(lexer);
    
    if (g_test_failed) {
        printf("  失败\n");
        return false;
    }
    
    printf("  通过\n");
    return true;
}

/**
 * @brief 测试错误处理
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_error_handling(void)
{
    printf("测试：错误处理\n");
    
    reset_test_state();
    
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    TEST_ASSERT_NOT_NULL(lexer);
    
    // 包含无效字符的源代码
    const char* source = "变量 x = @ 42";
    bool success = lexer->initialize(lexer, source, strlen(source), "test.cn");
    TEST_ASSERT_TRUE(success);
    
    // 获取令牌直到遇到错误
    int token_count = 0;
    while (lexer->has_more_tokens(lexer))
    {
        Stru_Token_t* token = lexer->next_token(lexer);
        TEST_ASSERT_NOT_NULL(token);
        
        if (token->type == Eum_TOKEN_ERROR)
        {
            // 检查错误令牌
            TEST_ASSERT_EQUAL(Eum_TOKEN_ERROR, token->type);
            TEST_ASSERT_EQUAL_STRING("@", token->lexeme);
            
            // 检查错误状态
            bool has_errors = lexer->has_errors(lexer);
            TEST_ASSERT_TRUE(has_errors);
            
            const char* error = lexer->get_last_error(lexer);
            TEST_ASSERT_NOT_NULL(error);
            TEST_ASSERT_NOT_EQUAL_STRING("", error);
            
            F_destroy_token(token);
            break;
        }
        
        F_destroy_token(token);
        token_count++;
    }
    
    // 应该至少处理了3个有效令牌（变量, x, =）
    TEST_ASSERT_GREATER_OR_EQUAL(3, token_count);
    
    lexer->destroy(lexer);
    
    if (g_test_failed) {
        printf("  失败\n");
        return false;
    }
    
    printf("  通过\n");
    return true;
}

/**
 * @brief 测试位置跟踪
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_position_tracking(void)
{
    printf("测试：位置跟踪\n");
    
    reset_test_state();
    
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    TEST_ASSERT_NOT_NULL(lexer);
    
    // 包含换行符的源代码
    const char* source = "变量 x\n= 42";
    bool success = lexer->initialize(lexer, source, strlen(source), "test.cn");
    TEST_ASSERT_TRUE(success);
    
    // 检查初始位置
    size_t line, column;
    lexer->get_position(lexer, &line, &column);
    TEST_ASSERT_EQUAL(1, line);
    TEST_ASSERT_EQUAL(1, column);
    
    // 获取第一个令牌
    Stru_Token_t* token = lexer->next_token(lexer);
    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_EQUAL(1, token->line);
    TEST_ASSERT_EQUAL(1, token->column);
    F_destroy_token(token);
    
    // 获取第二个令牌
    token = lexer->next_token(lexer);
    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_EQUAL(1, token->line);
    TEST_ASSERT_EQUAL(4, token->column); // "变量"占2个字符 + 空格
    F_destroy_token(token);
    
    // 获取第三个令牌（在第二行）
    token = lexer->next_token(lexer);
    TEST_ASSERT_NOT_NULL(token);
    TEST_ASSERT_EQUAL(2, token->line);
    TEST_ASSERT_EQUAL(1, token->column);
    F_destroy_token(token);
    
    lexer->destroy(lexer);
    
    if (g_test_failed) {
        printf("  失败\n");
        return false;
    }
    
    printf("  通过\n");
    return true;
}

/**
 * @brief 测试重置功能
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_reset_functionality(void)
{
    printf("测试：重置功能\n");
    
    reset_test_state();
    
    Stru_LexerInterface_t* lexer = F_create_lexer_interface();
    TEST_ASSERT_NOT_NULL(lexer);
    
    const char* source = "变量 x = 42";
    bool success = lexer->initialize(lexer, source, strlen(source), "test.cn");
    TEST_ASSERT_TRUE(success);
    
    // 获取一些令牌
    Stru_Token_t* token1 = lexer->next_token(lexer);
    TEST_ASSERT_NOT_NULL(token1);
    F_destroy_token(token1);
    
    // 重置词法分析器
    lexer->reset(lexer);
    
    // 检查位置是否重置
    size_t line, column;
    lexer->get_position(lexer, &line, &column);
    TEST_ASSERT_EQUAL(1, line);
    TEST_ASSERT_EQUAL(1, column);
    
    // 再次获取令牌（应该从开始）
    Stru_Token_t* token2 = lexer->next_token(lexer);
    TEST_ASSERT_NOT_NULL(token2);
    TEST_ASSERT_EQUAL(Eum_TOKEN_KEYWORD_VAR, token2->type);
    F_destroy_token(token2);
    
    lexer->destroy(lexer);
    
    if (g_test_failed) {
        printf("  失败\n");
        return false;
    }
    
    printf("  通过\n");
    return true;
}

// ============================================
// 测试运行器
// ============================================

/**
 * @brief 运行所有词法分析器接口测试
 * 
 * 这个函数被主测试运行器调用，运行词法分析器接口的所有测试。
 * 
 * @return true 所有测试通过
 * @return false 有测试失败
 */
bool test_lexer_interface_all(void)
{
    printf("\n");
    printf("========================================\n");
    printf("    词法分析器接口测试\n");
    printf("========================================\n");
    printf("\n");
    
    int tests_passed = 0;
    int tests_failed = 0;
    
    // 定义测试用例
    typedef bool (*TestFunc)(void);
    
    struct {
        const char* name;
        TestFunc func;
    } test_cases[] = {
        {"词法分析器创建和销毁", test_lexer_creation_and_destruction},
        {"词法分析器初始化", test_lexer_initialization},
        {"简单令牌识别", test_simple_token_recognition},
        {"批量令牌化", test_batch_tokenization},
        {"错误处理", test_error_handling},
        {"位置跟踪", test_position_tracking},
        {"重置功能", test_reset_functionality},
    };
    
    int total_tests = sizeof(test_cases) / sizeof(test_cases[0]);
    
    for (int i = 0; i < total_tests; i++) {
        printf("测试用例 %d/%d: %s... ", i + 1, total_tests, test_cases[i].name);
        
        // 调用测试函数并检查返回值
        bool test_result = test_cases[i].func();
        if (test_result) {
            tests_passed++;
            printf("✅ 通过\n");
        } else {
            tests_failed++;
            printf("❌ 失败\n");
        }
    }
    
    printf("\n测试结果:\n");
    printf("  总测试数: %d\n", total_tests);
    printf("  通过: %d\n", tests_passed);
    printf("  失败: %d\n", tests_failed);
    printf("  通过率: %.1f%%\n", (float)tests_passed / total_tests * 100);
    printf("\n");
    
    return (tests_failed == 0);
}

/**
 * @brief 主函数（用于独立运行词法分析器接口测试）
 * 
 * 注意：这个函数只在定义了STANDALONE_TEST宏时才被编译，
 * 以避免与总测试运行器的main函数冲突。
 */
#ifdef STANDALONE_TEST
int main(void)
{
    bool success = test_lexer_interface_all();
    
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
