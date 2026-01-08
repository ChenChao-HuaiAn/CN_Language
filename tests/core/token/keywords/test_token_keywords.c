/**
 * @file test_token_keywords.c
 * @brief CN_Language 令牌关键字管理模块测试
 * 
 * 测试令牌关键字信息的存储和查询功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "../../../../src/core/token/keywords/CN_token_keywords.h"

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
static void test_find_info(void);
static void test_get_chinese(void);
static void test_get_english(void);
static void test_get_category(void);
static void test_get_count(void);
static void test_get_all(void);

/**
 * @brief 运行所有令牌关键字管理测试
 */
bool test_token_keywords_all(void)
{
    printf("\n========================================\n");
    printf("   令牌关键字管理模块测试\n");
    printf("========================================\n\n");
    
    // 重置测试计数器
    g_test_count = 0;
    g_test_passed = 0;
    
    // 运行测试
    test_find_info();
    test_get_chinese();
    test_get_english();
    test_get_category();
    test_get_count();
    test_get_all();
    
    // 打印测试结果
    printf("\n========================================\n");
    printf("测试总结:\n");
    printf("  总测试数: %d\n", g_test_count);
    printf("  通过测试: %d\n", g_test_passed);
    printf("  失败测试: %d\n", g_test_count - g_test_passed);
    
    if (g_test_passed == g_test_count) {
        printf("\n✅ 所有令牌关键字管理测试通过！\n");
        return true;
    } else {
        printf("\n❌ 部分令牌关键字管理测试失败！\n");
        return false;
    }
}

/**
 * @brief 测试查找关键字信息
 */
static void test_find_info(void)
{
    printf("测试查找关键字信息:\n");
    printf("-------------------\n");
    
    // 测试查找有效关键字
    const Stru_KeywordInfo_t* info = F_token_keywords_find_info(Eum_TOKEN_KEYWORD_VAR);
    TEST_ASSERT(info != NULL, "查找变量关键字信息");
    if (info != NULL) {
        TEST_ASSERT(strcmp(info->chinese, "变量") == 0, "变量关键字中文正确");
        TEST_ASSERT(strcmp(info->english, "var") == 0, "变量关键字英文正确");
        TEST_ASSERT(info->category == 1, "变量关键字分类正确");
    }
    
    info = F_token_keywords_find_info(Eum_TOKEN_KEYWORD_IF);
    TEST_ASSERT(info != NULL, "查找如果关键字信息");
    if (info != NULL) {
        TEST_ASSERT(strcmp(info->chinese, "如果") == 0, "如果关键字中文正确");
        TEST_ASSERT(strcmp(info->english, "if") == 0, "如果关键字英文正确");
        TEST_ASSERT(info->category == 2, "如果关键字分类正确");
    }
    
    info = F_token_keywords_find_info(Eum_TOKEN_KEYWORD_FUNCTION);
    TEST_ASSERT(info != NULL, "查找函数关键字信息");
    if (info != NULL) {
        TEST_ASSERT(strcmp(info->chinese, "函数") == 0, "函数关键字中文正确");
        TEST_ASSERT(strcmp(info->english, "function") == 0, "函数关键字英文正确");
        TEST_ASSERT(info->category == 3, "函数关键字分类正确");
    }
    
    info = F_token_keywords_find_info(Eum_TOKEN_KEYWORD_ADD);
    TEST_ASSERT(info != NULL, "查找加关键字信息");
    if (info != NULL) {
        TEST_ASSERT(strcmp(info->chinese, "加") == 0, "加关键字中文正确");
        TEST_ASSERT(strcmp(info->english, "add") == 0, "加关键字英文正确");
        TEST_ASSERT(info->category == 7, "加关键字分类正确");
        TEST_ASSERT(info->precedence == 4, "加关键字优先级正确");
    }
    
    // 测试查找非关键字（应返回NULL）
    info = F_token_keywords_find_info(Eum_TOKEN_IDENTIFIER);
    TEST_ASSERT(info == NULL, "查找标识符返回NULL");
    
    info = F_token_keywords_find_info(Eum_TOKEN_LITERAL_INTEGER);
    TEST_ASSERT(info == NULL, "查找整数字面量返回NULL");
    
    info = F_token_keywords_find_info(Eum_TOKEN_OPERATOR_PLUS);
    TEST_ASSERT(info == NULL, "查找加号运算符返回NULL");
    
    info = F_token_keywords_find_info(Eum_TOKEN_EOF);
    TEST_ASSERT(info == NULL, "查找文件结束返回NULL");
    
    printf("\n");
}

/**
 * @brief 测试获取关键字的中文表示
 */
static void test_get_chinese(void)
{
    printf("测试获取关键字的中文表示:\n");
    printf("-------------------\n");
    
    // 测试获取有效关键字的中文
    TEST_ASSERT(strcmp(F_token_keywords_get_chinese(Eum_TOKEN_KEYWORD_VAR), "变量") == 0, "获取变量关键字中文");
    TEST_ASSERT(strcmp(F_token_keywords_get_chinese(Eum_TOKEN_KEYWORD_IF), "如果") == 0, "获取如果关键字中文");
    TEST_ASSERT(strcmp(F_token_keywords_get_chinese(Eum_TOKEN_KEYWORD_FUNCTION), "函数") == 0, "获取函数关键字中文");
    TEST_ASSERT(strcmp(F_token_keywords_get_chinese(Eum_TOKEN_KEYWORD_AND), "与") == 0, "获取与关键字中文");
    TEST_ASSERT(strcmp(F_token_keywords_get_chinese(Eum_TOKEN_KEYWORD_TRUE), "真") == 0, "获取真关键字中文");
    TEST_ASSERT(strcmp(F_token_keywords_get_chinese(Eum_TOKEN_KEYWORD_MODULE), "模块") == 0, "获取模块关键字中文");
    TEST_ASSERT(strcmp(F_token_keywords_get_chinese(Eum_TOKEN_KEYWORD_ADD), "加") == 0, "获取加关键字中文");
    TEST_ASSERT(strcmp(F_token_keywords_get_chinese(Eum_TOKEN_KEYWORD_TYPE), "类型") == 0, "获取类型关键字中文");
    TEST_ASSERT(strcmp(F_token_keywords_get_chinese(Eum_TOKEN_KEYWORD_CONST), "常量") == 0, "获取常量关键字中文");
    
    // 测试获取非关键字的中文（应返回空字符串）
    TEST_ASSERT(strcmp(F_token_keywords_get_chinese(Eum_TOKEN_IDENTIFIER), "") == 0, "获取标识符中文返回空字符串");
    TEST_ASSERT(strcmp(F_token_keywords_get_chinese(Eum_TOKEN_LITERAL_INTEGER), "") == 0, "获取整数字面量中文返回空字符串");
    TEST_ASSERT(strcmp(F_token_keywords_get_chinese(Eum_TOKEN_OPERATOR_PLUS), "") == 0, "获取加号运算符中文返回空字符串");
    TEST_ASSERT(strcmp(F_token_keywords_get_chinese(Eum_TOKEN_EOF), "") == 0, "获取文件结束中文返回空字符串");
    
    printf("\n");
}

/**
 * @brief 测试获取关键字的英文表示
 */
static void test_get_english(void)
{
    printf("测试获取关键字的英文表示:\n");
    printf("-------------------\n");
    
    // 测试获取有效关键字的英文
    TEST_ASSERT(strcmp(F_token_keywords_get_english(Eum_TOKEN_KEYWORD_VAR), "var") == 0, "获取变量关键字英文");
    TEST_ASSERT(strcmp(F_token_keywords_get_english(Eum_TOKEN_KEYWORD_IF), "if") == 0, "获取如果关键字英文");
    TEST_ASSERT(strcmp(F_token_keywords_get_english(Eum_TOKEN_KEYWORD_FUNCTION), "function") == 0, "获取函数关键字英文");
    TEST_ASSERT(strcmp(F_token_keywords_get_english(Eum_TOKEN_KEYWORD_AND), "and") == 0, "获取与关键字英文");
    TEST_ASSERT(strcmp(F_token_keywords_get_english(Eum_TOKEN_KEYWORD_TRUE), "true") == 0, "获取真关键字英文");
    TEST_ASSERT(strcmp(F_token_keywords_get_english(Eum_TOKEN_KEYWORD_MODULE), "module") == 0, "获取模块关键字英文");
    TEST_ASSERT(strcmp(F_token_keywords_get_english(Eum_TOKEN_KEYWORD_ADD), "add") == 0, "获取加关键字英文");
    TEST_ASSERT(strcmp(F_token_keywords_get_english(Eum_TOKEN_KEYWORD_TYPE), "type") == 0, "获取类型关键字英文");
    TEST_ASSERT(strcmp(F_token_keywords_get_english(Eum_TOKEN_KEYWORD_CONST), "const") == 0, "获取常量关键字英文");
    
    // 测试获取非关键字的英文（应返回空字符串）
    TEST_ASSERT(strcmp(F_token_keywords_get_english(Eum_TOKEN_IDENTIFIER), "") == 0, "获取标识符英文返回空字符串");
    TEST_ASSERT(strcmp(F_token_keywords_get_english(Eum_TOKEN_LITERAL_INTEGER), "") == 0, "获取整数字面量英文返回空字符串");
    TEST_ASSERT(strcmp(F_token_keywords_get_english(Eum_TOKEN_OPERATOR_PLUS), "") == 0, "获取加号运算符英文返回空字符串");
    TEST_ASSERT(strcmp(F_token_keywords_get_english(Eum_TOKEN_EOF), "") == 0, "获取文件结束英文返回空字符串");
    
    printf("\n");
}

/**
 * @brief 测试获取关键字的分类
 */
static void test_get_category(void)
{
    printf("测试获取关键字的分类:\n");
    printf("-------------------\n");
    
    // 测试获取有效关键字的分类
    TEST_ASSERT(F_token_keywords_get_category(Eum_TOKEN_KEYWORD_VAR) == 1, "获取变量关键字分类");
    TEST_ASSERT(F_token_keywords_get_category(Eum_TOKEN_KEYWORD_IF) == 2, "获取如果关键字分类");
    TEST_ASSERT(F_token_keywords_get_category(Eum_TOKEN_KEYWORD_FUNCTION) == 3, "获取函数关键字分类");
    TEST_ASSERT(F_token_keywords_get_category(Eum_TOKEN_KEYWORD_AND) == 4, "获取与关键字分类");
    TEST_ASSERT(F_token_keywords_get_category(Eum_TOKEN_KEYWORD_TRUE) == 5, "获取真关键字分类");
    TEST_ASSERT(F_token_keywords_get_category(Eum_TOKEN_KEYWORD_MODULE) == 6, "获取模块关键字分类");
    TEST_ASSERT(F_token_keywords_get_category(Eum_TOKEN_KEYWORD_ADD) == 7, "获取加关键字分类");
    TEST_ASSERT(F_token_keywords_get_category(Eum_TOKEN_KEYWORD_TYPE) == 8, "获取类型关键字分类");
    TEST_ASSERT(F_token_keywords_get_category(Eum_TOKEN_KEYWORD_CONST) == 9, "获取常量关键字分类");
    
    // 测试获取非关键字的分类（应返回0）
    TEST_ASSERT(F_token_keywords_get_category(Eum_TOKEN_IDENTIFIER) == 0, "获取标识符分类返回0");
    TEST_ASSERT(F_token_keywords_get_category(Eum_TOKEN_LITERAL_INTEGER) == 0, "获取整数字面量分类返回0");
    TEST_ASSERT(F_token_keywords_get_category(Eum_TOKEN_OPERATOR_PLUS) == 0, "获取加号运算符分类返回0");
    TEST_ASSERT(F_token_keywords_get_category(Eum_TOKEN_EOF) == 0, "获取文件结束分类返回0");
    
    printf("\n");
}

/**
 * @brief 测试获取关键字表大小
 */
static void test_get_count(void)
{
    printf("测试获取关键字表大小:\n");
    printf("-------------------\n");
    
    size_t count = F_token_keywords_get_count();
    TEST_ASSERT(count == 70, "关键字表大小为70");
    
    printf("关键字数量: %zu\n", count);
    printf("\n");
}

/**
 * @brief 测试获取所有关键字信息
 */
static void test_get_all(void)
{
    printf("测试获取所有关键字信息:\n");
    printf("-------------------\n");
    
    const Stru_KeywordInfo_t* all_keywords = F_token_keywords_get_all();
    TEST_ASSERT(all_keywords != NULL, "获取所有关键字信息");
    
    size_t count = F_token_keywords_get_count();
    TEST_ASSERT(count > 0, "关键字数量大于0");
    
    // 测试遍历所有关键字
    bool found_var = false;
    bool found_if = false;
    bool found_function = false;
    
    for (size_t i = 0; i < count; i++) {
        const Stru_KeywordInfo_t* info = &all_keywords[i];
        
        if (info->type == Eum_TOKEN_KEYWORD_VAR) {
            found_var = true;
            TEST_ASSERT(strcmp(info->chinese, "变量") == 0, "遍历找到变量关键字");
        } else if (info->type == Eum_TOKEN_KEYWORD_IF) {
            found_if = true;
            TEST_ASSERT(strcmp(info->chinese, "如果") == 0, "遍历找到如果关键字");
        } else if (info->type == Eum_TOKEN_KEYWORD_FUNCTION) {
            found_function = true;
            TEST_ASSERT(strcmp(info->chinese, "函数") == 0, "遍历找到函数关键字");
        }
    }
    
    TEST_ASSERT(found_var, "遍历找到变量关键字");
    TEST_ASSERT(found_if, "遍历找到如果关键字");
    TEST_ASSERT(found_function, "遍历找到函数关键字");
    
    printf("遍历测试完成，找到 %zu 个关键字\n", count);
    printf("\n");
}

/**
 * @brief 主函数（独立测试时使用）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    if (test_token_keywords_all()) {
        return 0;
    } else {
        return 1;
    }
}
#endif
