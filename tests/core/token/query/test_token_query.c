/**
 * @file test_token_query.c
 * @brief CN_Language 令牌类型查询模块测试
 * 
 * 测试令牌类型的查询和分类功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "../../../../src/core/token/query/CN_token_query.h"

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
static void test_is_keyword(void);
static void test_is_operator(void);
static void test_is_literal(void);
static void test_is_delimiter(void);
static void test_get_precedence(void);

/**
 * @brief 运行所有令牌类型查询测试
 */
bool test_token_query_all(void)
{
    printf("\n========================================\n");
    printf("   令牌类型查询模块测试\n");
    printf("========================================\n\n");
    
    // 重置测试计数器
    g_test_count = 0;
    g_test_passed = 0;
    
    // 运行测试
    test_is_keyword();
    test_is_operator();
    test_is_literal();
    test_is_delimiter();
    test_get_precedence();
    
    // 打印测试结果
    printf("\n========================================\n");
    printf("测试总结:\n");
    printf("  总测试数: %d\n", g_test_count);
    printf("  通过测试: %d\n", g_test_passed);
    printf("  失败测试: %d\n", g_test_count - g_test_passed);
    
    if (g_test_passed == g_test_count) {
        printf("\n✅ 所有令牌类型查询测试通过！\n");
        return true;
    } else {
        printf("\n❌ 部分令牌类型查询测试失败！\n");
        return false;
    }
}

/**
 * @brief 测试判断是否为关键字令牌
 */
static void test_is_keyword(void)
{
    printf("测试判断是否为关键字令牌:\n");
    printf("-------------------\n");
    
    // 测试关键字令牌
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_KEYWORD_VAR) == true, "变量关键字");
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_KEYWORD_IF) == true, "如果关键字");
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_KEYWORD_FUNCTION) == true, "函数关键字");
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_KEYWORD_AND) == true, "与关键字");
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_KEYWORD_TRUE) == true, "真关键字");
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_KEYWORD_MODULE) == true, "模块关键字");
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_KEYWORD_ADD) == true, "加关键字");
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_KEYWORD_TYPE) == true, "类型关键字");
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_KEYWORD_CONST) == true, "常量关键字");
    
    // 测试非关键字令牌
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_IDENTIFIER) == false, "标识符不是关键字");
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_LITERAL_INTEGER) == false, "整数字面量不是关键字");
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_OPERATOR_PLUS) == false, "加号运算符不是关键字");
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_DELIMITER_COMMA) == false, "逗号分隔符不是关键字");
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_EOF) == false, "文件结束不是关键字");
    TEST_ASSERT(F_token_query_is_keyword(Eum_TOKEN_ERROR) == false, "错误令牌不是关键字");
    
    printf("\n");
}

/**
 * @brief 测试判断是否为运算符令牌
 */
static void test_is_operator(void)
{
    printf("测试判断是否为运算符令牌:\n");
    printf("-------------------\n");
    
    // 测试关键字运算符
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_KEYWORD_ADD) == true, "加关键字运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_KEYWORD_SUBTRACT) == true, "减关键字运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_KEYWORD_MULTIPLY) == true, "乘关键字运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_KEYWORD_DIVIDE) == true, "除关键字运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_KEYWORD_EQUAL) == true, "等于关键字运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_KEYWORD_NOT_EQUAL) == true, "不等于关键字运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_KEYWORD_LESS) == true, "小于关键字运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_KEYWORD_GREATER) == true, "大于关键字运算符");
    
    // 测试符号运算符
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_OPERATOR_PLUS) == true, "加号运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_OPERATOR_MINUS) == true, "减号运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_OPERATOR_MULTIPLY) == true, "乘号运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_OPERATOR_DIVIDE) == true, "除号运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_OPERATOR_EQUAL) == true, "等于运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_OPERATOR_NOT_EQUAL) == true, "不等于运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_OPERATOR_LESS) == true, "小于运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_OPERATOR_GREATER) == true, "大于运算符");
    
    // 测试逻辑运算符关键字
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_KEYWORD_AND) == true, "与逻辑运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_KEYWORD_OR) == true, "或逻辑运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_KEYWORD_NOT) == true, "非逻辑运算符");
    
    // 测试非运算符令牌
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_KEYWORD_VAR) == false, "变量关键字不是运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_KEYWORD_IF) == false, "如果关键字不是运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_IDENTIFIER) == false, "标识符不是运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_LITERAL_INTEGER) == false, "整数字面量不是运算符");
    TEST_ASSERT(F_token_query_is_operator(Eum_TOKEN_DELIMITER_COMMA) == false, "逗号分隔符不是运算符");
    
    printf("\n");
}

/**
 * @brief 测试判断是否为字面量令牌
 */
static void test_is_literal(void)
{
    printf("测试判断是否为字面量令牌:\n");
    printf("-------------------\n");
    
    // 测试字面量令牌
    TEST_ASSERT(F_token_query_is_literal(Eum_TOKEN_LITERAL_INTEGER) == true, "整数字面量");
    TEST_ASSERT(F_token_query_is_literal(Eum_TOKEN_LITERAL_FLOAT) == true, "浮点数字面量");
    TEST_ASSERT(F_token_query_is_literal(Eum_TOKEN_LITERAL_STRING) == true, "字符串字面量");
    TEST_ASSERT(F_token_query_is_literal(Eum_TOKEN_LITERAL_BOOLEAN) == true, "布尔字面量");
    
    // 测试布尔关键字字面量
    TEST_ASSERT(F_token_query_is_literal(Eum_TOKEN_KEYWORD_TRUE) == true, "真关键字字面量");
    TEST_ASSERT(F_token_query_is_literal(Eum_TOKEN_KEYWORD_FALSE) == true, "假关键字字面量");
    TEST_ASSERT(F_token_query_is_literal(Eum_TOKEN_KEYWORD_NULL) == true, "空关键字字面量");
    
    // 测试非字面量令牌
    TEST_ASSERT(F_token_query_is_literal(Eum_TOKEN_KEYWORD_VAR) == false, "变量关键字不是字面量");
    TEST_ASSERT(F_token_query_is_literal(Eum_TOKEN_KEYWORD_IF) == false, "如果关键字不是字面量");
    TEST_ASSERT(F_token_query_is_literal(Eum_TOKEN_IDENTIFIER) == false, "标识符不是字面量");
    TEST_ASSERT(F_token_query_is_literal(Eum_TOKEN_OPERATOR_PLUS) == false, "加号运算符不是字面量");
    TEST_ASSERT(F_token_query_is_literal(Eum_TOKEN_DELIMITER_COMMA) == false, "逗号分隔符不是字面量");
    
    printf("\n");
}

/**
 * @brief 测试判断是否为分隔符令牌
 */
static void test_is_delimiter(void)
{
    printf("测试判断是否为分隔符令牌:\n");
    printf("-------------------\n");
    
    // 测试分隔符令牌
    TEST_ASSERT(F_token_query_is_delimiter(Eum_TOKEN_DELIMITER_COMMA) == true, "逗号分隔符");
    TEST_ASSERT(F_token_query_is_delimiter(Eum_TOKEN_DELIMITER_SEMICOLON) == true, "分号分隔符");
    TEST_ASSERT(F_token_query_is_delimiter(Eum_TOKEN_DELIMITER_LPAREN) == true, "左括号分隔符");
    TEST_ASSERT(F_token_query_is_delimiter(Eum_TOKEN_DELIMITER_RPAREN) == true, "右括号分隔符");
    TEST_ASSERT(F_token_query_is_delimiter(Eum_TOKEN_DELIMITER_LBRACE) == true, "左花括号分隔符");
    TEST_ASSERT(F_token_query_is_delimiter(Eum_TOKEN_DELIMITER_RBRACE) == true, "右花括号分隔符");
    TEST_ASSERT(F_token_query_is_delimiter(Eum_TOKEN_DELIMITER_LBRACKET) == true, "左方括号分隔符");
    TEST_ASSERT(F_token_query_is_delimiter(Eum_TOKEN_DELIMITER_RBRACKET) == true, "右方括号分隔符");
    
    // 测试非分隔符令牌
    TEST_ASSERT(F_token_query_is_delimiter(Eum_TOKEN_KEYWORD_VAR) == false, "变量关键字不是分隔符");
    TEST_ASSERT(F_token_query_is_delimiter(Eum_TOKEN_IDENTIFIER) == false, "标识符不是分隔符");
    TEST_ASSERT(F_token_query_is_delimiter(Eum_TOKEN_LITERAL_INTEGER) == false, "整数字面量不是分隔符");
    TEST_ASSERT(F_token_query_is_delimiter(Eum_TOKEN_OPERATOR_PLUS) == false, "加号运算符不是分隔符");
    TEST_ASSERT(F_token_query_is_delimiter(Eum_TOKEN_EOF) == false, "文件结束不是分隔符");
    
    printf("\n");
}

/**
 * @brief 测试获取运算符优先级
 */
static void test_get_precedence(void)
{
    printf("测试获取运算符优先级:\n");
    printf("-------------------\n");
    
    // 测试算术运算符优先级
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_MULTIPLY) == 3, "乘法运算符优先级3");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_DIVIDE) == 3, "除法运算符优先级3");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_MODULO) == 3, "取模运算符优先级3");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_ADD) == 4, "加法运算符优先级4");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_SUBTRACT) == 4, "减法运算符优先级4");
    
    // 测试比较运算符优先级
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_LESS) == 6, "小于运算符优先级6");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_GREATER) == 6, "大于运算符优先级6");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_LESS_EQUAL) == 6, "小于等于运算符优先级6");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_GREATER_EQUAL) == 6, "大于等于运算符优先级6");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_EQUAL) == 7, "等于运算符优先级7");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_NOT_EQUAL) == 7, "不等于运算符优先级7");
    
    // 测试逻辑运算符优先级
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_AND) == 11, "与逻辑运算符优先级11");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_OR) == 12, "或逻辑运算符优先级12");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_NOT) == 2, "非逻辑运算符优先级2");
    
    // 测试符号运算符优先级
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_OPERATOR_MULTIPLY) == 3, "乘号运算符优先级3");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_OPERATOR_DIVIDE) == 3, "除号运算符优先级3");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_OPERATOR_PLUS) == 4, "加号运算符优先级4");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_OPERATOR_MINUS) == 4, "减号运算符优先级4");
    
    // 测试非运算符令牌优先级（应为0）
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_KEYWORD_VAR) == 0, "变量关键字优先级0");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_IDENTIFIER) == 0, "标识符优先级0");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_LITERAL_INTEGER) == 0, "整数字面量优先级0");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_DELIMITER_COMMA) == 0, "逗号分隔符优先级0");
    TEST_ASSERT(F_token_query_get_precedence(Eum_TOKEN_EOF) == 0, "文件结束优先级0");
    
    printf("\n");
}

/**
 * @brief 主函数（独立测试时使用）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    if (test_token_query_all()) {
        return 0;
    } else {
        return 1;
    }
}
#endif
