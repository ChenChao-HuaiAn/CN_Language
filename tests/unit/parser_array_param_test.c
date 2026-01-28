/*
 * 单元测试：函数参数数组语法解析测试
 * 测试解析器对 "整数 arr[]" 和 "整数 arr[10]" 语法的支持
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

// 测试计数
static int tests_passed = 0;
static int tests_failed = 0;

// 简单断言宏
#define TEST_ASSERT(condition, message) do { \
    if (condition) { \
        tests_passed++; \
        printf("[PASS] %s\n", message); \
    } else { \
        tests_failed++; \
        printf("[FAIL] %s\n", message); \
    } \
} while (0)

// 测试1：不带大小的数组参数（整数 arr[]）解析为指针类型
static void test_array_param_without_size(void) {
    const char *source =
        "函数 求和(整数 arr[], 整数 长度) -> 整数 {\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_no_size.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    TEST_ASSERT(success && program != NULL, "解析程序成功");
    TEST_ASSERT(program != NULL && program->function_count == 1, "有一个函数声明");
    
    if (program && program->function_count > 0) {
        CnAstFunctionDecl *fn = program->functions[0];
        TEST_ASSERT(fn->parameter_count == 2, "函数有两个参数");
        
        if (fn->parameter_count >= 1) {
            CnAstParameter *param = &fn->parameters[0];
            TEST_ASSERT(param->declared_type != NULL, "第一个参数有类型");
            
            if (param->declared_type) {
                // 不带大小的数组参数应该解析为指针类型
                TEST_ASSERT(param->declared_type->kind == CN_TYPE_POINTER,
                           "不带大小的数组参数解析为指针类型");
                
                if (param->declared_type->kind == CN_TYPE_POINTER) {
                    TEST_ASSERT(param->declared_type->as.pointer_to != NULL,
                               "指针有基础类型");
                    TEST_ASSERT(param->declared_type->as.pointer_to->kind == CN_TYPE_INT,
                               "指针指向整数类型");
                }
            }
        }
    }
    
    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试2：带大小的数组参数（整数 arr[10]）解析为数组类型
static void test_array_param_with_size(void) {
    const char *source =
        "函数 处理(整数 arr[10]) {\n"
        "    返回;\n"
        "}\n";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_with_size.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    TEST_ASSERT(success && program != NULL, "解析程序成功（带大小）");
    TEST_ASSERT(program != NULL && program->function_count == 1, "有一个函数声明（带大小）");
    
    if (program && program->function_count > 0) {
        CnAstFunctionDecl *fn = program->functions[0];
        TEST_ASSERT(fn->parameter_count == 1, "函数有一个参数");
        
        if (fn->parameter_count >= 1) {
            CnAstParameter *param = &fn->parameters[0];
            TEST_ASSERT(param->declared_type != NULL, "参数有类型（带大小）");
            
            if (param->declared_type) {
                // 带大小的数组参数应该解析为数组类型
                TEST_ASSERT(param->declared_type->kind == CN_TYPE_ARRAY,
                           "带大小的数组参数解析为数组类型");
                
                if (param->declared_type->kind == CN_TYPE_ARRAY) {
                    TEST_ASSERT(param->declared_type->as.array.length == 10,
                               "数组大小为10");
                    TEST_ASSERT(param->declared_type->as.array.element_type != NULL,
                               "数组有元素类型");
                    if (param->declared_type->as.array.element_type) {
                        TEST_ASSERT(param->declared_type->as.array.element_type->kind == CN_TYPE_INT,
                                   "数组元素类型为整数");
                    }
                }
            }
        }
    }
    
    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试3：多个数组参数
static void test_multiple_array_params(void) {
    const char *source =
        "函数 合并(整数 arr1[], 整数 arr2[5], 整数 长度) -> 整数 {\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_multiple.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    TEST_ASSERT(success && program != NULL, "解析多参数成功");
    
    if (program && program->function_count > 0) {
        CnAstFunctionDecl *fn = program->functions[0];
        TEST_ASSERT(fn->parameter_count == 3, "函数有三个参数");
        
        if (fn->parameter_count >= 3) {
            // 第一个参数：整数 arr1[] - 指针类型
            TEST_ASSERT(fn->parameters[0].declared_type->kind == CN_TYPE_POINTER,
                       "第一个参数是指针类型");
            
            // 第二个参数：整数 arr2[5] - 数组类型
            TEST_ASSERT(fn->parameters[1].declared_type->kind == CN_TYPE_ARRAY,
                       "第二个参数是数组类型");
            TEST_ASSERT(fn->parameters[1].declared_type->as.array.length == 5,
                       "第二个参数数组大小为5");
            
            // 第三个参数：整数 长度 - 普通整数类型
            TEST_ASSERT(fn->parameters[2].declared_type->kind == CN_TYPE_INT,
                       "第三个参数是整数类型");
        }
    }
    
    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试4：不同类型的数组参数
static void test_different_type_array_params(void) {
    const char *source =
        "函数 处理(小数 数组[], 布尔 标志[]) {\n"
        "}\n";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_types.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    TEST_ASSERT(success && program != NULL, "解析不同类型成功");
    
    if (program && program->function_count > 0) {
        CnAstFunctionDecl *fn = program->functions[0];
        TEST_ASSERT(fn->parameter_count == 2, "函数有两个参数");
        
        if (fn->parameter_count >= 2) {
            // 小数数组参数
            TEST_ASSERT(fn->parameters[0].declared_type->kind == CN_TYPE_POINTER,
                       "小数数组是指针类型");
            TEST_ASSERT(fn->parameters[0].declared_type->as.pointer_to->kind == CN_TYPE_FLOAT,
                       "小数数组元素是浮点类型");
            
            // 布尔数组参数
            TEST_ASSERT(fn->parameters[1].declared_type->kind == CN_TYPE_POINTER,
                       "布尔数组是指针类型");
            TEST_ASSERT(fn->parameters[1].declared_type->as.pointer_to->kind == CN_TYPE_BOOL,
                       "布尔数组元素是布尔类型");
        }
    }
    
    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

int main(void) {
    printf("====== 函数参数数组语法解析测试 ======\n\n");
    
    test_array_param_without_size();
    printf("\n");
    
    test_array_param_with_size();
    printf("\n");
    
    test_multiple_array_params();
    printf("\n");
    
    test_different_type_array_params();
    printf("\n");
    
    printf("======================================\n");
    printf("测试完成: %d 通过, %d 失败\n", tests_passed, tests_failed);
    printf("======================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
