/**
 * @file test_token_interface.c
 * @brief CN_Language 令牌接口模块测试
 * 
 * 测试令牌模块的抽象接口功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "../../../../src/core/token/CN_token_interface.h"

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
static void test_get_interface(void);
static void test_interface_functions(void);
static void test_module_lifecycle(void);

/**
 * @brief 运行所有令牌接口测试
 */
bool test_token_interface_all(void)
{
    printf("\n========================================\n");
    printf("   令牌接口模块测试\n");
    printf("========================================\n\n");
    
    // 重置测试计数器
    g_test_count = 0;
    g_test_passed = 0;
    
    // 运行测试
    test_get_interface();
    test_interface_functions();
    test_module_lifecycle();
    
    // 打印测试结果
    printf("\n========================================\n");
    printf("测试总结:\n");
    printf("  总测试数: %d\n", g_test_count);
    printf("  通过测试: %d\n", g_test_passed);
    printf("  失败测试: %d\n", g_test_count - g_test_passed);
    
    if (g_test_passed == g_test_count) {
        printf("\n✅ 所有令牌接口测试通过！\n");
        return true;
    } else {
        printf("\n❌ 部分令牌接口测试失败！\n");
        return false;
    }
}

/**
 * @brief 测试获取接口实例
 */
static void test_get_interface(void)
{
    printf("测试获取接口实例:\n");
    printf("-------------------\n");
    
    // 测试获取接口实例
    const Stru_TokenInterface_t* interface = F_get_token_interface();
    TEST_ASSERT(interface != NULL, "获取令牌接口实例");
    
    // 测试接口函数指针不为空
    if (interface != NULL) {
        TEST_ASSERT(interface->create_token != NULL, "创建令牌函数指针不为空");
        TEST_ASSERT(interface->destroy_token != NULL, "销毁令牌函数指针不为空");
        TEST_ASSERT(interface->copy_token != NULL, "复制令牌函数指针不为空");
        TEST_ASSERT(interface->get_token_type != NULL, "获取令牌类型函数指针不为空");
        TEST_ASSERT(interface->get_token_lexeme != NULL, "获取令牌词素函数指针不为空");
        TEST_ASSERT(interface->get_token_position != NULL, "获取令牌位置函数指针不为空");
        TEST_ASSERT(interface->set_int_value != NULL, "设置整数值函数指针不为空");
        TEST_ASSERT(interface->set_float_value != NULL, "设置浮点数值函数指针不为空");
        TEST_ASSERT(interface->set_bool_value != NULL, "设置布尔值函数指针不为空");
        TEST_ASSERT(interface->get_int_value != NULL, "获取整数值函数指针不为空");
        TEST_ASSERT(interface->get_float_value != NULL, "获取浮点数值函数指针不为空");
        TEST_ASSERT(interface->get_bool_value != NULL, "获取布尔值函数指针不为空");
        TEST_ASSERT(interface->is_keyword != NULL, "判断关键字函数指针不为空");
        TEST_ASSERT(interface->is_operator != NULL, "判断运算符函数指针不为空");
        TEST_ASSERT(interface->is_literal != NULL, "判断字面量函数指针不为空");
        TEST_ASSERT(interface->is_delimiter != NULL, "判断分隔符函数指针不为空");
        TEST_ASSERT(interface->get_precedence != NULL, "获取优先级函数指针不为空");
        TEST_ASSERT(interface->get_chinese_keyword != NULL, "获取中文关键字函数指针不为空");
        TEST_ASSERT(interface->get_english_keyword != NULL, "获取英文关键字函数指针不为空");
        TEST_ASSERT(interface->get_keyword_category != NULL, "获取关键字分类函数指针不为空");
        TEST_ASSERT(interface->type_to_string != NULL, "类型转字符串函数指针不为空");
        TEST_ASSERT(interface->equals != NULL, "比较令牌函数指针不为空");
        TEST_ASSERT(interface->to_string != NULL, "打印令牌信息函数指针不为空");
        TEST_ASSERT(interface->initialize != NULL, "初始化模块函数指针不为空");
        TEST_ASSERT(interface->cleanup != NULL, "清理模块函数指针不为空");
    }
    
    printf("\n");
}

/**
 * @brief 测试接口函数
 */
static void test_interface_functions(void)
{
    printf("测试接口函数:\n");
    printf("-------------------\n");
    
    const Stru_TokenInterface_t* interface = F_get_token_interface();
    TEST_ASSERT(interface != NULL, "获取令牌接口实例");
    
    if (interface == NULL) {
        return;
    }
    
    // 测试模块初始化
    bool init_result = interface->initialize();
    TEST_ASSERT(init_result == true, "模块初始化成功");
    
    // 测试创建和销毁令牌
    Stru_Token_t* token = interface->create_token(Eum_TOKEN_KEYWORD_VAR, "变量", 1, 1);
    TEST_ASSERT(token != NULL, "通过接口创建令牌");
    
    if (token != NULL) {
        // 测试获取令牌属性
        Eum_TokenType type = interface->get_token_type(token);
        TEST_ASSERT(type == Eum_TOKEN_KEYWORD_VAR, "通过接口获取令牌类型");
        
        const char* lexeme = interface->get_token_lexeme(token);
        TEST_ASSERT(strcmp(lexeme, "变量") == 0, "通过接口获取令牌词素");
        
        size_t line = 0, column = 0;
        interface->get_token_position(token, &line, &column);
        TEST_ASSERT(line == 1 && column == 1, "通过接口获取令牌位置");
        
        // 测试字面量值操作
        interface->set_int_value(token, 42);
        long int_value = interface->get_int_value(token);
        TEST_ASSERT(int_value == 42, "通过接口设置和获取整数值");
        
        // 测试类型查询
        bool is_keyword = interface->is_keyword(Eum_TOKEN_KEYWORD_VAR);
        TEST_ASSERT(is_keyword == true, "通过接口判断关键字");
        
        bool is_operator = interface->is_operator(Eum_TOKEN_KEYWORD_ADD);
        TEST_ASSERT(is_operator == true, "通过接口判断运算符");
        
        bool is_literal = interface->is_literal(Eum_TOKEN_LITERAL_INTEGER);
        TEST_ASSERT(is_literal == true, "通过接口判断字面量");
        
        bool is_delimiter = interface->is_delimiter(Eum_TOKEN_DELIMITER_COMMA);
        TEST_ASSERT(is_delimiter == true, "通过接口判断分隔符");
        
        // 测试关键字信息查询
        int precedence = interface->get_precedence(Eum_TOKEN_KEYWORD_ADD);
        TEST_ASSERT(precedence == 4, "通过接口获取运算符优先级");
        
        const char* chinese = interface->get_chinese_keyword(Eum_TOKEN_KEYWORD_VAR);
        TEST_ASSERT(strcmp(chinese, "变量") == 0, "通过接口获取中文关键字");
        
        const char* english = interface->get_english_keyword(Eum_TOKEN_KEYWORD_VAR);
        TEST_ASSERT(strcmp(english, "var") == 0, "通过接口获取英文关键字");
        
        int category = interface->get_keyword_category(Eum_TOKEN_KEYWORD_VAR);
        TEST_ASSERT(category == 1, "通过接口获取关键字分类");
        
        // 测试工具函数
        const char* type_str = interface->type_to_string(Eum_TOKEN_KEYWORD_VAR);
        TEST_ASSERT(strstr(type_str, "KEYWORD_VAR") != NULL, "通过接口类型转字符串");
        
        // 测试复制令牌
        Stru_Token_t* copy = interface->copy_token(token);
        TEST_ASSERT(copy != NULL, "通过接口复制令牌");
        TEST_ASSERT(copy != token, "复制的是新令牌");
        
        bool equals = interface->equals(token, copy);
        TEST_ASSERT(equals == true, "通过接口比较相同令牌");
        
        // 测试打印令牌信息
        char buffer[256];
        int result = interface->to_string(token, buffer, sizeof(buffer));
        TEST_ASSERT(result > 0, "通过接口打印令牌信息");
        TEST_ASSERT(strstr(buffer, "变量") != NULL, "打印信息包含令牌词素");
        
        // 清理复制的令牌
        if (copy != NULL) {
            interface->destroy_token(copy);
        }
        
        // 销毁原始令牌
        interface->destroy_token(token);
    }
    
    // 测试空指针处理
    interface->destroy_token(NULL);
    TEST_ASSERT(true, "通过接口销毁空令牌不会崩溃");
    
    Stru_Token_t* null_copy = interface->copy_token(NULL);
    TEST_ASSERT(null_copy == NULL, "通过接口复制空令牌返回NULL");
    
    Eum_TokenType null_type = interface->get_token_type(NULL);
    TEST_ASSERT(null_type == Eum_TOKEN_ERROR, "通过接口获取空令牌类型返回错误");
    
    const char* null_lexeme = interface->get_token_lexeme(NULL);
    TEST_ASSERT(strcmp(null_lexeme, "") == 0, "通过接口获取空令牌词素返回空字符串");
    
    size_t line = 999, column = 999;
    interface->get_token_position(NULL, &line, &column);
    TEST_ASSERT(line == 999 && column == 999, "通过接口获取空令牌位置不修改参数");
    
    printf("\n");
}

/**
 * @brief 测试模块生命周期
 */
static void test_module_lifecycle(void)
{
    printf("测试模块生命周期:\n");
    printf("-------------------\n");
    
    const Stru_TokenInterface_t* interface = F_get_token_interface();
    TEST_ASSERT(interface != NULL, "获取令牌接口实例");
    
    if (interface == NULL) {
        return;
    }
    
    // 测试多次初始化（应该都成功）
    bool init1 = interface->initialize();
    TEST_ASSERT(init1 == true, "第一次模块初始化");
    
    bool init2 = interface->initialize();
    TEST_ASSERT(init2 == true, "第二次模块初始化");
    
    // 测试多次清理（应该不会崩溃）
    interface->cleanup();
    TEST_ASSERT(true, "第一次模块清理");
    
    interface->cleanup();
    TEST_ASSERT(true, "第二次模块清理");
    
    // 测试初始化后创建令牌
    bool init3 = interface->initialize();
    TEST_ASSERT(init3 == true, "第三次模块初始化");
    
    Stru_Token_t* token = interface->create_token(Eum_TOKEN_KEYWORD_IF, "如果", 1, 1);
    TEST_ASSERT(token != NULL, "初始化后创建令牌");
    
    if (token != NULL) {
        interface->destroy_token(token);
    }
    
    // 最终清理
    interface->cleanup();
    TEST_ASSERT(true, "最终模块清理");
    
    printf("\n");
}

/**
 * @brief 主函数（独立测试时使用）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    if (test_token_interface_all()) {
        return 0;
    } else {
        return 1;
    }
}
#endif
