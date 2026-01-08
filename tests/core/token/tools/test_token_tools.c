/**
 * @file test_token_tools.c
 * @brief CN_Language 令牌工具模块测试
 * 
 * 测试令牌类型转换和字符串格式化等工具功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "../../../../src/core/token/tools/CN_token_tools.h"
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
static void test_type_to_string(void);
static void test_to_string(void);
static void test_print(void);
static void test_format_position(void);

/**
 * @brief 运行所有令牌工具函数测试
 */
bool test_token_tools_all(void)
{
    printf("\n========================================\n");
    printf("   令牌工具函数模块测试\n");
    printf("========================================\n\n");
    
    // 重置测试计数器
    g_test_count = 0;
    g_test_passed = 0;
    
    // 运行测试
    test_type_to_string();
    test_to_string();
    test_print();
    test_format_position();
    
    // 打印测试结果
    printf("\n========================================\n");
    printf("测试总结:\n");
    printf("  总测试数: %d\n", g_test_count);
    printf("  通过测试: %d\n", g_test_passed);
    printf("  失败测试: %d\n", g_test_count - g_test_passed);
    
    if (g_test_passed == g_test_count) {
        printf("\n✅ 所有令牌工具函数测试通过！\n");
        return true;
    } else {
        printf("\n❌ 部分令牌工具函数测试失败！\n");
        return false;
    }
}

/**
 * @brief 测试令牌类型转字符串
 */
static void test_type_to_string(void)
{
    printf("测试令牌类型转字符串:\n");
    printf("-------------------\n");
    
    // 测试关键字类型
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_KEYWORD_VAR), "KEYWORD_VAR") != NULL, "变量关键字类型转字符串");
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_KEYWORD_IF), "KEYWORD_IF") != NULL, "如果关键字类型转字符串");
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_KEYWORD_FUNCTION), "KEYWORD_FUNCTION") != NULL, "函数关键字类型转字符串");
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_KEYWORD_ADD), "KEYWORD_ADD") != NULL, "加关键字类型转字符串");
    
    // 测试标识符类型
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_IDENTIFIER), "IDENTIFIER") != NULL, "标识符类型转字符串");
    
    // 测试字面量类型
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_LITERAL_INTEGER), "LITERAL_INTEGER") != NULL, "整数字面量类型转字符串");
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_LITERAL_FLOAT), "LITERAL_FLOAT") != NULL, "浮点数字面量类型转字符串");
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_LITERAL_STRING), "LITERAL_STRING") != NULL, "字符串字面量类型转字符串");
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_LITERAL_BOOLEAN), "LITERAL_BOOLEAN") != NULL, "布尔字面量类型转字符串");
    
    // 测试运算符类型
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_OPERATOR_PLUS), "OPERATOR_PLUS") != NULL, "加号运算符类型转字符串");
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_OPERATOR_MINUS), "OPERATOR_MINUS") != NULL, "减号运算符类型转字符串");
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_OPERATOR_MULTIPLY), "OPERATOR_MULTIPLY") != NULL, "乘号运算符类型转字符串");
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_OPERATOR_DIVIDE), "OPERATOR_DIVIDE") != NULL, "除号运算符类型转字符串");
    
    // 测试分隔符类型
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_DELIMITER_COMMA), "DELIMITER_COMMA") != NULL, "逗号分隔符类型转字符串");
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_DELIMITER_SEMICOLON), "DELIMITER_SEMICOLON") != NULL, "分号分隔符类型转字符串");
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_DELIMITER_LPAREN), "DELIMITER_LPAREN") != NULL, "左括号分隔符类型转字符串");
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_DELIMITER_RPAREN), "DELIMITER_RPAREN") != NULL, "右括号分隔符类型转字符串");
    
    // 测试特殊令牌类型
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_EOF), "EOF") != NULL, "文件结束类型转字符串");
    TEST_ASSERT(strstr(F_token_tools_type_to_string(Eum_TOKEN_ERROR), "ERROR") != NULL, "错误令牌类型转字符串");
    
    // 测试无效类型（应返回"UNKNOWN"或类似）
    TEST_ASSERT(strlen(F_token_tools_type_to_string((Eum_TokenType)999)) > 0, "无效类型转字符串返回非空字符串");
    
    printf("\n");
}

/**
 * @brief 测试打印令牌信息
 */
static void test_to_string(void)
{
    printf("测试打印令牌信息:\n");
    printf("-------------------\n");
    
    // 创建测试令牌
    Stru_Token_t* token = F_token_lifecycle_create(
        Eum_TOKEN_KEYWORD_VAR, "变量", 10, 5);
    TEST_ASSERT(token != NULL, "创建测试令牌");
    
    // 测试缓冲区足够大
    char buffer[256];
    int result = F_token_tools_to_string(token, buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "打印令牌信息返回正数");
    TEST_ASSERT(strstr(buffer, "变量") != NULL, "令牌信息包含词素");
    TEST_ASSERT(strstr(buffer, "10") != NULL, "令牌信息包含行号");
    TEST_ASSERT(strstr(buffer, "5") != NULL, "令牌信息包含列号");
    
    // 测试缓冲区太小
    char small_buffer[10];
    result = F_token_tools_to_string(token, small_buffer, sizeof(small_buffer));
    TEST_ASSERT(result > 0, "小缓冲区打印令牌信息返回正数");
    
    // 测试空令牌
    result = F_token_tools_to_string(NULL, buffer, sizeof(buffer));
    TEST_ASSERT(result == 0, "空令牌打印返回0");
    TEST_ASSERT(strlen(buffer) == 0 || buffer[0] == '\0', "空令牌打印清空缓冲区");
    
    // 测试空缓冲区
    result = F_token_tools_to_string(token, NULL, sizeof(buffer));
    TEST_ASSERT(result == 0, "空缓冲区打印返回0");
    
    // 测试零大小缓冲区
    result = F_token_tools_to_string(token, buffer, 0);
    TEST_ASSERT(result == 0, "零大小缓冲区打印返回0");
    
    // 清理
    if (token != NULL) {
        F_token_lifecycle_destroy(token);
    }
    
    // 测试整数字面量令牌
    Stru_Token_t* int_token = F_token_lifecycle_create(
        Eum_TOKEN_LITERAL_INTEGER, "42", 1, 1);
    TEST_ASSERT(int_token != NULL, "创建整数字面量令牌");
    if (int_token != NULL) {
        int_token->literal_value.int_value = 42;
        result = F_token_tools_to_string(int_token, buffer, sizeof(buffer));
        TEST_ASSERT(result > 0, "打印整数字面量令牌信息");
        TEST_ASSERT(strstr(buffer, "42") != NULL, "整数字面量令牌信息包含值");
        F_token_lifecycle_destroy(int_token);
    }
    
    // 测试浮点数字面量令牌
    Stru_Token_t* float_token = F_token_lifecycle_create(
        Eum_TOKEN_LITERAL_FLOAT, "3.14", 1, 1);
    TEST_ASSERT(float_token != NULL, "创建浮点数字面量令牌");
    if (float_token != NULL) {
        float_token->literal_value.float_value = 3.14;
        result = F_token_tools_to_string(float_token, buffer, sizeof(buffer));
        TEST_ASSERT(result > 0, "打印浮点数字面量令牌信息");
        TEST_ASSERT(strstr(buffer, "3.14") != NULL, "浮点数字面量令牌信息包含值");
        F_token_lifecycle_destroy(float_token);
    }
    
    // 测试布尔字面量令牌
    Stru_Token_t* bool_token = F_token_lifecycle_create(
        Eum_TOKEN_LITERAL_BOOLEAN, "真", 1, 1);
    TEST_ASSERT(bool_token != NULL, "创建布尔字面量令牌");
    if (bool_token != NULL) {
        bool_token->literal_value.bool_value = true;
        result = F_token_tools_to_string(bool_token, buffer, sizeof(buffer));
        TEST_ASSERT(result > 0, "打印布尔字面量令牌信息");
        F_token_lifecycle_destroy(bool_token);
    }
    
    printf("\n");
}

/**
 * @brief 测试打印令牌信息到文件
 */
static void test_print(void)
{
    printf("测试打印令牌信息到文件:\n");
    printf("-------------------\n");
    
    // 创建测试令牌
    Stru_Token_t* token = F_token_lifecycle_create(
        Eum_TOKEN_KEYWORD_IF, "如果", 20, 15);
    TEST_ASSERT(token != NULL, "创建测试令牌");
    
    // 测试打印到标准输出
    int result = F_token_tools_print(token, stdout);
    TEST_ASSERT(result > 0, "打印令牌信息到标准输出返回正数");
    
    // 测试打印到文件
    FILE* temp_file = tmpfile();
    TEST_ASSERT(temp_file != NULL, "创建临时文件");
    if (temp_file != NULL) {
        result = F_token_tools_print(token, temp_file);
        TEST_ASSERT(result > 0, "打印令牌信息到文件返回正数");
        
        // 检查文件内容
        rewind(temp_file);
        char buffer[256];
        fgets(buffer, sizeof(buffer), temp_file);
        TEST_ASSERT(strstr(buffer, "如果") != NULL, "文件内容包含令牌词素");
        fclose(temp_file);
    }
    
    // 测试空令牌
    result = F_token_tools_print(NULL, stdout);
    TEST_ASSERT(result == 0, "空令牌打印返回0");
    
    // 测试空文件流
    result = F_token_tools_print(token, NULL);
    TEST_ASSERT(result == 0, "空文件流打印返回0");
    
    // 清理
    if (token != NULL) {
        F_token_lifecycle_destroy(token);
    }
    
    printf("\n");
}

/**
 * @brief 测试格式化令牌位置信息
 */
static void test_format_position(void)
{
    printf("测试格式化令牌位置信息:\n");
    printf("-------------------\n");
    
    char buffer[256];
    
    // 测试有效位置
    int result = F_token_tools_format_position(10, 5, buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "格式化有效位置返回正数");
    TEST_ASSERT(strstr(buffer, "10") != NULL, "位置信息包含行号");
    TEST_ASSERT(strstr(buffer, "5") != NULL, "位置信息包含列号");
    
    // 测试零位置
    result = F_token_tools_format_position(0, 0, buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "格式化零位置返回正数");
    
    // 测试大位置
    result = F_token_tools_format_position(9999, 9999, buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "格式化大位置返回正数");
    
    // 测试空缓冲区
    result = F_token_tools_format_position(10, 5, NULL, sizeof(buffer));
    TEST_ASSERT(result == 0, "空缓冲区格式化返回0");
    
    // 测试零大小缓冲区
    result = F_token_tools_format_position(10, 5, buffer, 0);
    TEST_ASSERT(result == 0, "零大小缓冲区格式化返回0");
    
    // 测试小缓冲区
    char small_buffer[5];
    result = F_token_tools_format_position(10, 5, small_buffer, sizeof(small_buffer));
    TEST_ASSERT(result > 0, "小缓冲区格式化返回正数");
    
    printf("\n");
}

/**
 * @brief 主函数（独立测试时使用）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    if (test_token_tools_all()) {
        return 0;
    } else {
        return 1;
    }
}
#endif
