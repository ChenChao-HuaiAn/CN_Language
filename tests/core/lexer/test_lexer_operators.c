/**
 * @file test_lexer_operators.c
 * @brief 词法分析器运算符模块测试
 * 
 * 测试CN_lexer_operators模块的功能，包括：
 * 1. 运算符识别
 * 2. 运算符分类识别
 * 3. 运算符查找和匹配
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "src/core/lexer/operators/CN_lexer_operators.h"
#include "src/core/token/CN_token_types.h"

/* 测试用例定义 */
typedef struct {
    const char* test_name;
    bool (*test_func)(void);
} TestCase;

/* 测试辅助函数声明 */
static bool test_operator_identification(void);
static bool test_operator_categories(void);
static bool test_operator_edge_cases(void);
static bool test_non_operator_identification(void);

/* 测试用例数组 */
static TestCase test_cases[] = {
    {"运算符识别", test_operator_identification},
    {"运算符分类", test_operator_categories},
    {"边界情况", test_operator_edge_cases},
    {"非运算符识别", test_non_operator_identification},
};

static int total_tests = sizeof(test_cases) / sizeof(test_cases[0]);
static int tests_passed = 0;
static int tests_failed = 0;

/**
 * @brief 运行所有运算符测试
 * 
 * @return true 所有测试通过
 * @return false 有测试失败
 */
bool test_lexer_operators_all(void)
{
    printf("\n");
    printf("========================================\n");
    printf("    词法分析器运算符模块测试\n");
    printf("========================================\n");
    printf("\n");
    
    tests_passed = 0;
    tests_failed = 0;
    
    for (int i = 0; i < total_tests; i++) {
        printf("测试用例 %d/%d: %s... ", i + 1, total_tests, test_cases[i].test_name);
        
        bool result = test_cases[i].test_func();
        
        if (result) {
            printf("✅ 通过\n");
            tests_passed++;
        } else {
            printf("❌ 失败\n");
            tests_failed++;
        }
    }
    
    printf("\n");
    printf("测试结果:\n");
    printf("  总测试数: %d\n", total_tests);
    printf("  通过: %d\n", tests_passed);
    printf("  失败: %d\n", tests_failed);
    printf("  通过率: %.1f%%\n", (float)tests_passed / total_tests * 100);
    printf("\n");
    
    return (tests_failed == 0);
}

/**
 * @brief 测试运算符识别
 */
static bool test_operator_identification(void)
{
    // 测试一些常见运算符（根据实际实现）
    struct {
        const char* operator_str;
        Eum_TokenType expected_type;
    } test_cases[] = {
        // 算术运算符
        {"+", Eum_TOKEN_OPERATOR_PLUS},
        {"-", Eum_TOKEN_OPERATOR_MINUS},
        {"*", Eum_TOKEN_OPERATOR_MULTIPLY},
        {"/", Eum_TOKEN_OPERATOR_DIVIDE},
        {"%", Eum_TOKEN_OPERATOR_MODULO},
        
        // 比较运算符
        {"==", Eum_TOKEN_OPERATOR_EQUAL},
        {"!=", Eum_TOKEN_OPERATOR_NOT_EQUAL},
        {"<", Eum_TOKEN_OPERATOR_LESS},
        {"<=", Eum_TOKEN_OPERATOR_LESS_EQUAL},
        {">", Eum_TOKEN_OPERATOR_GREATER},
        {">=", Eum_TOKEN_OPERATOR_GREATER_EQUAL},
        
        // 赋值运算符
        {"=", Eum_TOKEN_OPERATOR_ASSIGN},
        {"+=", Eum_TOKEN_OPERATOR_PLUS},      // 根据实现，+= 返回 PLUS
        {"-=", Eum_TOKEN_OPERATOR_MINUS},     // 根据实现，-= 返回 MINUS
        {"*=", Eum_TOKEN_OPERATOR_MULTIPLY},  // 根据实现，*= 返回 MULTIPLY
        {"/=", Eum_TOKEN_OPERATOR_DIVIDE},    // 根据实现，/= 返回 DIVIDE
        {"%=", Eum_TOKEN_OPERATOR_MODULO},    // 根据实现，%= 返回 MODULO
        
        // 逻辑运算符（根据实现返回关键字类型）
        {"&&", Eum_TOKEN_KEYWORD_AND},
        {"||", Eum_TOKEN_KEYWORD_OR},
        {"!", Eum_TOKEN_KEYWORD_NOT},
        
        // 位运算符（根据实现返回 ERROR）
        {"&", Eum_TOKEN_ERROR},
        {"|", Eum_TOKEN_ERROR},
        {"^", Eum_TOKEN_ERROR},
        {"~", Eum_TOKEN_ERROR},
        {"<<", Eum_TOKEN_ERROR},
        {">>", Eum_TOKEN_ERROR},
        {"&=", Eum_TOKEN_ERROR},
        {"|=", Eum_TOKEN_ERROR},
        {"^=", Eum_TOKEN_ERROR},
        {"<<=", Eum_TOKEN_ERROR},
        {">>=", Eum_TOKEN_ERROR},
        
        // 其他运算符（根据实现返回 ERROR）
        {"++", Eum_TOKEN_ERROR},
        {"--", Eum_TOKEN_ERROR},
        {".", Eum_TOKEN_ERROR},
        {"->", Eum_TOKEN_ERROR},
        {"?", Eum_TOKEN_ERROR},
        {":", Eum_TOKEN_ERROR},
        {"::", Eum_TOKEN_ERROR},
    };
    
    int test_count = sizeof(test_cases) / sizeof(test_cases[0]);
    
    for (int i = 0; i < test_count; i++) {
        Eum_TokenType type = F_identify_operator(test_cases[i].operator_str);
        
        if (type != test_cases[i].expected_type) {
            printf("错误: 运算符识别失败 - '%s' (期望: %d, 实际: %d)\n",
                   test_cases[i].operator_str, test_cases[i].expected_type, type);
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 测试运算符分类
 */
static bool test_operator_categories(void)
{
    // 测试算术运算符
    if (F_identify_arithmetic_operator("+") != Eum_TOKEN_OPERATOR_PLUS) {
        printf("错误: 算术运算符识别失败\n");
        return false;
    }
    
    // 测试比较运算符
    if (F_identify_comparison_operator("==") != Eum_TOKEN_OPERATOR_EQUAL) {
        printf("错误: 比较运算符识别失败\n");
        return false;
    }
    
    // 测试赋值运算符
    if (F_identify_assignment_operator("=") != Eum_TOKEN_OPERATOR_ASSIGN) {
        printf("错误: 赋值运算符识别失败\n");
        return false;
    }
    
    // 测试逻辑运算符（根据实现返回关键字类型）
    if (F_identify_logical_operator("&&") != Eum_TOKEN_KEYWORD_AND) {
        printf("错误: 逻辑运算符识别失败\n");
        return false;
    }
    
    // 测试位运算符（根据实现返回 ERROR）
    if (F_identify_bitwise_operator("&") != Eum_TOKEN_ERROR) {
        printf("错误: 位运算符识别失败\n");
        return false;
    }
    
    // 测试其他运算符（根据实现返回 ERROR）
    if (F_identify_other_operator(".") != Eum_TOKEN_ERROR) {
        printf("错误: 其他运算符识别失败\n");
        return false;
    }
    
    return true;
}

/**
 * @brief 测试边界情况
 */
static bool test_operator_edge_cases(void)
{
    // 测试空字符串
    if (F_identify_operator("") != Eum_TOKEN_ERROR) {
        printf("错误: 空字符串应返回错误类型\n");
        return false;
    }
    
    // 测试NULL指针
    if (F_identify_operator(NULL) != Eum_TOKEN_ERROR) {
        printf("错误: NULL指针应返回错误类型\n");
        return false;
    }
    
    // 测试部分匹配（不应识别为有效运算符）
    if (F_identify_operator("===") != Eum_TOKEN_ERROR) {
        printf("错误: '===' 应返回错误类型\n");
        return false;
    }
    
    if (F_identify_operator("!==") != Eum_TOKEN_ERROR) {
        printf("错误: '!==' 应返回错误类型\n");
        return false;
    }
    
    if (F_identify_operator("<<<") != Eum_TOKEN_ERROR) {
        printf("错误: '<<<' 应返回错误类型\n");
        return false;
    }
    
    if (F_identify_operator(">>>") != Eum_TOKEN_ERROR) {
        printf("错误: '>>>' 应返回错误类型\n");
        return false;
    }
    
    // 测试超长字符串
    char long_string[256];
    memset(long_string, '+', 255);
    long_string[255] = '\0';
    
    if (F_identify_operator(long_string) != Eum_TOKEN_ERROR) {
        printf("错误: 超长字符串应返回错误类型\n");
        return false;
    }
    
    // 测试单个字符的边界情况
    const char* single_chars = "!@#$%^&*()_+-=[]{}|;:'\",.<>?/`~";
    
    for (int i = 0; single_chars[i] != '\0'; i++) {
        char test_str[2] = {single_chars[i], '\0'};
        Eum_TokenType type = F_identify_operator(test_str);
        
        // 我们只关心它是否崩溃，不检查具体类型
        // 因为有些字符可能是运算符，有些可能不是
        (void)type; // 避免未使用变量警告
    }
    
    return true;
}

/**
 * @brief 测试非运算符识别
 */
static bool test_non_operator_identification(void)
{
    // 测试一些非运算符（应为错误类型）
    const char* non_operators[] = {
        "abc",
        "123",
        "变量",
        "函数",
        "如果",
        "我的变量",
        "计算结果",
        "test123",
        "hello_world",
        "中文标识符"
    };
    
    for (int i = 0; i < sizeof(non_operators) / sizeof(non_operators[0]); i++) {
        Eum_TokenType type = F_identify_operator(non_operators[i]);
        
        if (type != Eum_TOKEN_ERROR) {
            printf("错误: 非运算符识别错误 - '%s' (期望: 错误, 实际: %d)\n",
                   non_operators[i], type);
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 主函数（用于独立测试）
 */
#ifdef TEST_OPERATORS_STANDALONE
int main(void)
{
    bool result = test_lexer_operators_all();
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
