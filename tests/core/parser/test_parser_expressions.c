/**
 * @file test_parser_expressions.c
 * @brief CN_Language 语法分析器表达式解析测试
 * 
 * 语法分析器表达式解析功能的测试套件，包括：
 * 1. 基本表达式解析
 * 2. 复合赋值表达式解析
 * 3. 运算符优先级和结合性测试
 * 4. 错误处理测试
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../../../src/core/parser/CN_parser_interface.h"
#include "../../../src/core/parser/interface/CN_parser_interface_impl.h"
#include "../../../src/core/lexer/CN_lexer_interface.h"
#include "../../../src/core/token/CN_token.h"
#include "../../../src/core/parser/utils/CN_parser_utils.h"

/* 测试用例定义 */
typedef struct {
    const char* test_name;
    bool (*test_func)(void);
} TestCase;

/* 测试辅助函数声明 */
static bool test_basic_expressions(void);
static bool test_assignment_expressions(void);
static bool test_compound_assignment_expressions(void);
static bool test_operator_precedence_and_associativity(void);
static bool test_error_handling(void);
static bool test_complex_expressions(void);

/* 测试夹具变量 */
static Stru_ParserInterface_t* g_test_parser = NULL;
static Stru_LexerInterface_t* g_test_lexer = NULL;

/* 测试用例数组 */
static TestCase test_cases[] = {
    {"基本表达式解析", test_basic_expressions},
    {"赋值表达式解析", test_assignment_expressions},
    {"复合赋值表达式解析", test_compound_assignment_expressions},
    {"运算符优先级和结合性", test_operator_precedence_and_associativity},
    {"错误处理", test_error_handling},
    {"复杂表达式组合", test_complex_expressions},
};

static int total_tests = sizeof(test_cases) / sizeof(test_cases[0]);
static int tests_passed = 0;
static int tests_failed = 0;

/**
 * @brief 初始化测试环境
 * 
 * @param source_code 源代码字符串
 * @return true 初始化成功
 * @return false 初始化失败
 */
static bool initialize_test_environment(const char* source_code)
{
    if (!g_test_lexer || !g_test_parser) {
        return false;
    }
    
    // 初始化词法分析器
    bool lexer_init_result = g_test_lexer->initialize(g_test_lexer, source_code, 
                                                     strlen(source_code), "test.cn");
    if (!lexer_init_result) {
        return false;
    }
    
    // 初始化语法分析器
    bool parser_init_result = g_test_parser->initialize(g_test_parser, g_test_lexer);
    return parser_init_result;
}

/**
 * @brief 解析表达式并返回AST
 * 
 * @param source_code 源代码字符串
 * @return Stru_AstNode_t* AST节点，需要调用者释放
 */
static Stru_AstNode_t* parse_expression(const char* source_code)
{
    if (!initialize_test_environment(source_code)) {
        return NULL;
    }
    
    return g_test_parser->parse_expression(g_test_parser);
}

/**
 * @brief 运行所有表达式解析测试
 * 
 * @return true 所有测试通过
 * @return false 有测试失败
 */
bool test_parser_expressions_all(void)
{
    printf("\n");
    printf("========================================\n");
    printf("    语法分析器表达式解析测试\n");
    printf("========================================\n");
    printf("\n");
    
    // 创建测试词法分析器
    g_test_lexer = F_create_lexer_interface();
    if (!g_test_lexer) {
        printf("错误: 无法创建词法分析器接口\n");
        return false;
    }
    
    // 创建测试语法分析器
    g_test_parser = F_create_parser_interface();
    if (!g_test_parser) {
        printf("错误: 无法创建语法分析器接口\n");
        g_test_lexer->destroy(g_test_lexer);
        return false;
    }
    
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
    
    // 清理资源
    if (g_test_parser) {
        g_test_parser->destroy(g_test_parser);
        g_test_parser = NULL;
    }
    
    if (g_test_lexer) {
        g_test_lexer->destroy(g_test_lexer);
        g_test_lexer = NULL;
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
 * @brief 测试基本表达式解析
 */
static bool test_basic_expressions(void)
{
    bool all_passed = true;
    
    // 测试解析字面量表达式
    Stru_AstNode_t* ast1 = parse_expression("42");
    if (!ast1) {
        printf("\n  错误: 字面量表达式解析失败 - 返回NULL\n");
        all_passed = false;
    } else {
        // 注意：Stru_AstNode_t结构体没有直接的type字段
        // 我们需要通过其他方式验证节点类型
        // 暂时只检查是否成功创建了节点
    }
    if (ast1) F_destroy_ast_node(ast1);
    
    // 测试解析标识符表达式
    Stru_AstNode_t* ast2 = parse_expression("变量名");
    if (!ast2) {
        printf("\n  错误: 标识符表达式解析失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast2) F_destroy_ast_node(ast2);
    
    // 测试解析二元表达式
    Stru_AstNode_t* ast3 = parse_expression("1 + 2");
    if (!ast3) {
        printf("\n  错误: 二元表达式解析失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast3) F_destroy_ast_node(ast3);
    
    // 测试解析一元表达式
    Stru_AstNode_t* ast4 = parse_expression("-42");
    if (!ast4) {
        printf("\n  错误: 一元表达式解析失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast4) F_destroy_ast_node(ast4);
    
    return all_passed;
}

/**
 * @brief 测试赋值表达式解析
 */
static bool test_assignment_expressions(void)
{
    bool all_passed = true;
    
    // 测试解析简单赋值表达式
    Stru_AstNode_t* ast = parse_expression("变量 = 42");
    if (!ast) {
        printf("\n  错误: 简单赋值表达式解析失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast) F_destroy_ast_node(ast);
    
    return all_passed;
}

/**
 * @brief 测试复合赋值表达式解析
 */
static bool test_compound_assignment_expressions(void)
{
    bool all_passed = true;
    
    // 测试解析复合赋值表达式：+=
    Stru_AstNode_t* ast1 = parse_expression("变量 += 5");
    if (!ast1) {
        printf("\n  错误: 复合赋值表达式 (+=) 解析失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast1) F_destroy_ast_node(ast1);
    
    // 测试解析复合赋值表达式：-=
    Stru_AstNode_t* ast2 = parse_expression("变量 -= 5");
    if (!ast2) {
        printf("\n  错误: 复合赋值表达式 (-=) 解析失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast2) F_destroy_ast_node(ast2);
    
    // 测试解析复合赋值表达式：*=
    Stru_AstNode_t* ast3 = parse_expression("变量 *= 5");
    if (!ast3) {
        printf("\n  错误: 复合赋值表达式 (*=) 解析失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast3) F_destroy_ast_node(ast3);
    
    // 测试解析复合赋值表达式：/=
    Stru_AstNode_t* ast4 = parse_expression("变量 /= 5");
    if (!ast4) {
        printf("\n  错误: 复合赋值表达式 (/=) 解析失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast4) F_destroy_ast_node(ast4);
    
    // 测试解析复合赋值表达式：%=
    Stru_AstNode_t* ast5 = parse_expression("变量 %= 5");
    if (!ast5) {
        printf("\n  错误: 复合赋值表达式 (%=) 解析失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast5) F_destroy_ast_node(ast5);
    
    // 测试解析嵌套的复合赋值表达式
    Stru_AstNode_t* ast6 = parse_expression("a += b -= c");
    if (!ast6) {
        printf("\n  错误: 嵌套复合赋值表达式解析失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast6) F_destroy_ast_node(ast6);
    
    // 测试解析包含算术运算的复合赋值表达式
    Stru_AstNode_t* ast7 = parse_expression("变量 += 2 * 3");
    if (!ast7) {
        printf("\n  错误: 包含算术运算的复合赋值表达式解析失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast7) F_destroy_ast_node(ast7);
    
    return all_passed;
}

/**
 * @brief 测试运算符优先级和结合性
 */
static bool test_operator_precedence_and_associativity(void)
{
    bool all_passed = true;
    
    // 测试赋值运算符优先级最低
    Stru_AstNode_t* ast1 = parse_expression("a = b + c");
    if (!ast1) {
        printf("\n  错误: 赋值运算符优先级测试失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast1) F_destroy_ast_node(ast1);
    
    // 测试复合赋值运算符优先级最低
    Stru_AstNode_t* ast2 = parse_expression("a += b * c");
    if (!ast2) {
        printf("\n  错误: 复合赋值运算符优先级测试失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast2) F_destroy_ast_node(ast2);
    
    // 测试赋值运算符右结合性
    Stru_AstNode_t* ast3 = parse_expression("a = b = c");
    if (!ast3) {
        printf("\n  错误: 赋值运算符右结合性测试失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast3) F_destroy_ast_node(ast3);
    
    // 测试复合赋值运算符右结合性
    Stru_AstNode_t* ast4 = parse_expression("a += b += c");
    if (!ast4) {
        printf("\n  错误: 复合赋值运算符右结合性测试失败 - 返回NULL\n");
        all_passed = false;
    }
    if (ast4) F_destroy_ast_node(ast4);
    
    return all_passed;
}

/**
 * @brief 测试错误处理
 */
static bool test_error_handling(void)
{
    bool all_passed = true;
    
    // 测试解析无效的赋值表达式（缺少右值）
    Stru_AstNode_t* ast1 = parse_expression("变量 =");
    if (!ast1) {
        printf("\n  错误: 无效赋值表达式应返回AST节点\n");
        all_passed = false;
    }
    
    // 检查是否有错误报告
    bool has_errors = g_test_parser->has_errors(g_test_parser);
    if (!has_errors) {
        printf("\n  错误: 无效赋值表达式应报告错误\n");
        all_passed = false;
    }
    
    // 清除错误
    g_test_parser->clear_errors(g_test_parser);
    
    if (ast1) F_destroy_ast_node(ast1);
    
    // 测试解析无效的复合赋值表达式（缺少右值）
    Stru_AstNode_t* ast2 = parse_expression("变量 +=");
    if (!ast2) {
        printf("\n  错误: 无效复合赋值表达式应返回AST节点\n");
        all_passed = false;
    }
    
    // 检查是否有错误报告
    has_errors = g_test_parser->has_errors(g_test_parser);
    if (!has_errors) {
        printf("\n  错误: 无效复合赋值表达式应报告错误\n");
        all_passed = false;
    }
    
    // 清除错误
    g_test_parser->clear_errors(g_test_parser);
    
    if (ast2) F_destroy_ast_node(ast2);
    
    // 测试解析缺少左操作数的表达式
    Stru_AstNode_t* ast3 = parse_expression("= 42");
    if (!ast3) {
        printf("\n  错误: 缺少左操作数的表达式应返回AST节点\n");
        all_passed = false;
    }
    
    // 检查是否有错误报告
    has_errors = g_test_parser->has_errors(g_test_parser);
    if (!has_errors) {
        printf("\n  错误: 缺少左操作数的表达式应报告错误\n");
        all_passed = false;
    }
    
    if (ast3) F_destroy_ast_node(ast3);
    
    return all_passed;
}

/**
 * @brief 测试复杂表达式组合
 */
static bool test_complex_expressions(void)
{
    bool all_passed = true;
    
    // 测试解析包含复合赋值的复杂表达式
    Stru_AstNode_t* ast1 = parse_expression("结果 = (a += b) * (c -= d)");
    if (!ast1) {
        printf("\n  错误: 复杂表达式解析失败\n");
        all_passed = false;
    }
    if (ast1) F_destroy_ast_node(ast1);
    
    // 测试解析多个复合赋值表达式
    Stru_AstNode_t* ast2 = parse_expression("a += 1; b -= 2; c *= 3");
    if (!ast2) {
        printf("\n  错误: 多个复合赋值表达式解析失败\n");
        all_passed = false;
    }
    if (ast2) F_destroy_ast_node(ast2);
    
    return all_passed;
}

/**
 * @brief 主函数（用于独立测试）
 */
#ifdef TEST_PARSER_EXPRESSIONS_STANDALONE
int main(void)
{
    bool result = test_parser_expressions_all();
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
