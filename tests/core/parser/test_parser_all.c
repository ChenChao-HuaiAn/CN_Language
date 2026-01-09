/**
 * @file test_parser_all.c
 * @brief CN_Language 语法分析器模块测试
 * 
 * 语法分析器模块的完整测试套件，包括接口测试、功能测试和错误处理测试。
 * 使用Unity测试框架，支持模块化测试。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "unity.h"
#include "unity_fixture.h"
#include "../../../src/core/parser/CN_parser_interface.h"
#include "../../../src/core/parser/interface/CN_parser_interface_impl.h"
#include "../../../src/core/lexer/CN_lexer_interface.h"
#include "../../../src/core/token/CN_token.h"
#include <stdlib.h>
#include <string.h>

/* 表达式测试函数声明 */
extern bool test_parser_expressions_all(void);

// ============================================
// 测试夹具定义
// ============================================

TEST_GROUP(parser_interface);

// 测试夹具变量
static Stru_ParserInterface_t* g_test_parser = NULL;
static Stru_LexerInterface_t* g_test_lexer = NULL;
static const char* g_test_source = NULL;

// ============================================
// 测试夹具设置和清理
// ============================================

TEST_SETUP(parser_interface)
{
    // 创建测试词法分析器
    g_test_lexer = F_create_lexer_interface();
    TEST_ASSERT_NOT_NULL(g_test_lexer);
    
    // 创建测试语法分析器
    g_test_parser = F_create_parser_interface();
    TEST_ASSERT_NOT_NULL(g_test_parser);
}

TEST_TEAR_DOWN(parser_interface)
{
    // 清理测试语法分析器
    if (g_test_parser) {
        g_test_parser->destroy(g_test_parser);
        g_test_parser = NULL;
    }
    
    // 清理测试词法分析器
    if (g_test_lexer) {
        g_test_lexer->destroy(g_test_lexer);
        g_test_lexer = NULL;
    }
}

// ============================================
// 测试用例 - 接口创建和销毁
// ============================================

TEST(parser_interface, test_create_and_destroy)
{
    // 测试创建语法分析器接口
    Stru_ParserInterface_t* parser = F_create_parser_interface();
    TEST_ASSERT_NOT_NULL(parser);
    
    // 测试接口函数指针不为NULL
    TEST_ASSERT_NOT_NULL(parser->initialize);
    TEST_ASSERT_NOT_NULL(parser->parse_program);
    TEST_ASSERT_NOT_NULL(parser->parse_statement);
    TEST_ASSERT_NOT_NULL(parser->parse_expression);
    TEST_ASSERT_NOT_NULL(parser->parse_declaration);
    TEST_ASSERT_NOT_NULL(parser->create_ast_node);
    TEST_ASSERT_NOT_NULL(parser->add_child_node);
    TEST_ASSERT_NOT_NULL(parser->set_node_attribute);
    TEST_ASSERT_NOT_NULL(parser->get_node_attribute);
    TEST_ASSERT_NOT_NULL(parser->report_error);
    TEST_ASSERT_NOT_NULL(parser->has_errors);
    TEST_ASSERT_NOT_NULL(parser->get_errors);
    TEST_ASSERT_NOT_NULL(parser->clear_errors);
    TEST_ASSERT_NOT_NULL(parser->get_current_token);
    TEST_ASSERT_NOT_NULL(parser->get_previous_token);
    TEST_ASSERT_NOT_NULL(parser->get_next_token);
    TEST_ASSERT_NOT_NULL(parser->reset);
    TEST_ASSERT_NOT_NULL(parser->destroy);
    
    // 测试销毁语法分析器接口
    parser->destroy(parser);
}

TEST(parser_interface, test_initialize_with_null_lexer)
{
    // 测试使用NULL词法分析器初始化
    bool result = g_test_parser->initialize(g_test_parser, NULL);
    TEST_ASSERT_FALSE(result);
}

TEST(parser_interface, test_initialize_with_valid_lexer)
{
    // 创建测试源代码
    const char* source_code = "变量 数量 = 42;";
    
    // 初始化词法分析器
    bool lexer_init_result = g_test_lexer->initialize(g_test_lexer, source_code, strlen(source_code), "test.cn");
    TEST_ASSERT_TRUE(lexer_init_result);
    
    // 初始化语法分析器
    bool parser_init_result = g_test_parser->initialize(g_test_parser, g_test_lexer);
    TEST_ASSERT_TRUE(parser_init_result);
}

// ============================================
// 测试用例 - 错误处理
// ============================================

TEST(parser_interface, test_error_reporting)
{
    // 初始化测试环境
    const char* source_code = "变量 数量 = 42;";
    g_test_lexer->initialize(g_test_lexer, source_code, strlen(source_code), "test.cn");
    g_test_parser->initialize(g_test_parser, g_test_lexer);
    
    // 测试初始状态无错误
    bool has_errors = g_test_parser->has_errors(g_test_parser);
    TEST_ASSERT_FALSE(has_errors);
    
    // 报告错误
    g_test_parser->report_error(g_test_parser, 1, 1, "测试错误", NULL);
    
    // 测试错误状态
    has_errors = g_test_parser->has_errors(g_test_parser);
    TEST_ASSERT_TRUE(has_errors);
    
    // 获取错误列表
    Stru_DynamicArray_t* errors = g_test_parser->get_errors(g_test_parser);
    TEST_ASSERT_NOT_NULL(errors);
    
    // 清除错误
    g_test_parser->clear_errors(g_test_parser);
    
    // 测试错误已清除
    has_errors = g_test_parser->has_errors(g_test_parser);
    TEST_ASSERT_FALSE(has_errors);
}

// ============================================
// 测试用例 - 令牌状态查询
// ============================================

TEST(parser_interface, test_token_state_queries)
{
    // 初始化测试环境
    const char* source_code = "变量 数量 = 42;";
    g_test_lexer->initialize(g_test_lexer, source_code, strlen(source_code), "test.cn");
    g_test_parser->initialize(g_test_parser, g_test_lexer);
    
    // 测试当前令牌
    Stru_Token_t* current_token = g_test_parser->get_current_token(g_test_parser);
    TEST_ASSERT_NOT_NULL(current_token);
    
    // 测试前一个令牌（应为NULL，因为还没有前进）
    Stru_Token_t* previous_token = g_test_parser->get_previous_token(g_test_parser);
    TEST_ASSERT_NULL(previous_token);
    
    // 测试下一个令牌
    Stru_Token_t* next_token = g_test_parser->get_next_token(g_test_parser);
    TEST_ASSERT_NOT_NULL(next_token);
}

// ============================================
// 测试用例 - 解析功能
// ============================================

TEST(parser_interface, test_parse_empty_program)
{
    // 初始化测试环境 - 空程序
    const char* source_code = "";
    g_test_lexer->initialize(g_test_lexer, source_code, strlen(source_code), "test.cn");
    g_test_parser->initialize(g_test_parser, g_test_lexer);
    
    // 解析空程序
    Stru_AstNode_t* ast = g_test_parser->parse_program(g_test_parser);
    TEST_ASSERT_NOT_NULL(ast);
    
    // 检查AST节点类型
    TEST_ASSERT_EQUAL(Eum_AST_PROGRAM, ast->type);
    
    // 清理AST
    F_destroy_ast_node(ast);
}

TEST(parser_interface, test_parse_simple_variable_declaration)
{
    // 初始化测试环境 - 简单变量声明
    const char* source_code = "变量 数量 = 42;";
    g_test_lexer->initialize(g_test_lexer, source_code, strlen(source_code), "test.cn");
    g_test_parser->initialize(g_test_parser, g_test_lexer);
    
    // 解析程序
    Stru_AstNode_t* ast = g_test_parser->parse_program(g_test_parser);
    TEST_ASSERT_NOT_NULL(ast);
    
    // 检查AST节点类型
    TEST_ASSERT_EQUAL(Eum_AST_PROGRAM, ast->type);
    
    // 清理AST
    F_destroy_ast_node(ast);
}

// ============================================
// 测试用例 - AST构建功能
// ============================================

TEST(parser_interface, test_create_ast_node)
{
    // 初始化测试环境
    const char* source_code = "测试";
    g_test_lexer->initialize(g_test_lexer, source_code, strlen(source_code), "test.cn");
    g_test_parser->initialize(g_test_parser, g_test_lexer);
    
    // 获取当前令牌
    Stru_Token_t* token = g_test_parser->get_current_token(g_test_parser);
    TEST_ASSERT_NOT_NULL(token);
    
    // 创建AST节点
    Stru_AstNode_t* node = g_test_parser->create_ast_node(g_test_parser, Eum_AST_IDENTIFIER_EXPR, token);
    TEST_ASSERT_NOT_NULL(node);
    
    // 检查节点属性
    TEST_ASSERT_EQUAL(Eum_AST_IDENTIFIER_EXPR, node->type);
    TEST_ASSERT_EQUAL(token->line, node->line);
    TEST_ASSERT_EQUAL(token->column, node->column);
    
    // 清理节点
    F_destroy_ast_node(node);
}

TEST(parser_interface, test_add_child_node)
{
    // 初始化测试环境
    const char* source_code = "父节点 子节点";
    g_test_lexer->initialize(g_test_lexer, source_code, strlen(source_code), "test.cn");
    g_test_parser->initialize(g_test_parser, g_test_lexer);
    
    // 创建父节点
    Stru_Token_t* parent_token = g_test_parser->get_current_token(g_test_parser);
    Stru_AstNode_t* parent = g_test_parser->create_ast_node(g_test_parser, Eum_AST_PROGRAM, parent_token);
    TEST_ASSERT_NOT_NULL(parent);
    
    // 前进到下一个令牌
    Stru_ParserState_t* state = (Stru_ParserState_t*)g_test_parser->internal_state;
    F_advance_token(state);
    
    // 创建子节点
    Stru_Token_t* child_token = g_test_parser->get_current_token(g_test_parser);
    Stru_AstNode_t* child = g_test_parser->create_ast_node(g_test_parser, Eum_AST_IDENTIFIER_EXPR, child_token);
    TEST_ASSERT_NOT_NULL(child);
    
    // 添加子节点
    bool result = g_test_parser->add_child_node(g_test_parser, parent, child);
    TEST_ASSERT_TRUE(result);
    
    // 清理节点
    F_destroy_ast_node(parent); // 这会同时清理子节点
}

// ============================================
// 测试用例 - 重置功能
// ============================================

TEST(parser_interface, test_reset_parser)
{
    // 初始化测试环境
    const char* source_code = "变量 数量 = 42;";
    g_test_lexer->initialize(g_test_lexer, source_code, strlen(source_code), "test.cn");
    g_test_parser->initialize(g_test_parser, g_test_lexer);
    
    // 报告一些错误
    g_test_parser->report_error(g_test_parser, 1, 1, "测试错误", NULL);
    
    // 重置语法分析器
    g_test_parser->reset(g_test_parser);
    
    // 测试错误已清除
    bool has_errors = g_test_parser->has_errors(g_test_parser);
    TEST_ASSERT_FALSE(has_errors);
    
    // 测试令牌状态已重置
    Stru_Token_t* current_token = g_test_parser->get_current_token(g_test_parser);
    TEST_ASSERT_NOT_NULL(current_token);
}

// ============================================
// 测试运行器
// ============================================

TEST_GROUP_RUNNER(parser_interface)
{
    RUN_TEST_CASE(parser_interface, test_create_and_destroy);
    RUN_TEST_CASE(parser_interface, test_initialize_with_null_lexer);
    RUN_TEST_CASE(parser_interface, test_initialize_with_valid_lexer);
    RUN_TEST_CASE(parser_interface, test_error_reporting);
    RUN_TEST_CASE(parser_interface, test_token_state_queries);
    RUN_TEST_CASE(parser_interface, test_parse_empty_program);
    RUN_TEST_CASE(parser_interface, test_parse_simple_variable_declaration);
    RUN_TEST_CASE(parser_interface, test_create_ast_node);
    RUN_TEST_CASE(parser_interface, test_add_child_node);
    RUN_TEST_CASE(parser_interface, test_reset_parser);
}

// ============================================
// 主测试函数
// ============================================

/**
 * @brief 运行所有语法分析器测试
 * 
 * @return bool 所有测试通过返回true，否则返回false
 */
bool test_parser_all(void)
{
    bool all_passed = true;
    
    printf("\n");
    printf("========================================\n");
    printf("    语法分析器模块完整测试\n");
    printf("========================================\n");
    printf("\n");
    
    // 运行接口测试
    printf("1. 运行接口测试...\n");
    UnityTestRunnerFunction runner = &TEST_GROUP_RUNNER_NAME(parser_interface);
    bool interface_passed = (UnityMain(0, NULL, runner) == 0);
    
    if (interface_passed) {
        printf("✅ 接口测试通过\n");
    } else {
        printf("❌ 接口测试失败\n");
        all_passed = false;
    }
    
    printf("\n");
    
    // 运行表达式测试
    printf("2. 运行表达式测试...\n");
    bool expressions_passed = test_parser_expressions_all();
    
    if (expressions_passed) {
        printf("✅ 表达式测试通过\n");
    } else {
        printf("❌ 表达式测试失败\n");
        all_passed = false;
    }
    
    printf("\n");
    printf("========================================\n");
    printf("    语法分析器模块测试完成\n");
    printf("========================================\n");
    printf("\n");
    
    return all_passed;
}
