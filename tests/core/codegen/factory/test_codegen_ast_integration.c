/******************************************************************************
 * 文件名: test_codegen_ast_integration.c
 * 功能: CN_Language 代码生成器与AST模块集成测试
 * 
 * 测试代码生成器与AST模块的集成，验证代码生成功能。
 * 遵循项目测试规范，使用模块化测试结构。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月10日
 * 修改历史:
 *   - 2026年1月10日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* 测试框架包含 */
#ifdef USE_UNITY
#include "unity.h"
#else
/* 简单测试框架 */
#define TEST_ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_FALSE(condition) TEST_ASSERT_TRUE(!(condition))
#define TEST_ASSERT_EQUAL(expected, actual) TEST_ASSERT_TRUE((expected) == (actual))
#define TEST_ASSERT_NOT_NULL(ptr) TEST_ASSERT_TRUE((ptr) != NULL)
#define TEST_ASSERT_NULL(ptr) TEST_ASSERT_TRUE((ptr) == NULL)
#define TEST_ASSERT_EQUAL_STRING(expected, actual) TEST_ASSERT_TRUE(strcmp((expected), (actual)) == 0)
#define TEST_ASSERT_STRING_CONTAINS(haystack, needle) TEST_ASSERT_TRUE(strstr((haystack), (needle)) != NULL)
#endif

/* 包含被测试的头文件 */
#include "../../../../src/core/codegen/CN_codegen_interface.h"

/* 包含AST模块头文件 */
#include "../../../../src/core/ast/CN_ast.h"

/**
 * @brief 创建简单的测试AST
 * 
 * 创建一个简单的AST用于测试代码生成。
 * 使用现有的AST API创建节点。
 * 
 * @return Stru_AstNode_t* 测试AST节点，失败返回NULL
 */
static Stru_AstNode_t* create_test_ast(void)
{
    /* 创建一个简单的程序AST节点 */
    Stru_AstNode_t* program = F_create_ast_node(Eum_AST_PROGRAM, NULL, 1, 1);
    if (!program) {
        printf("  错误: 无法创建程序节点\n");
        return NULL;
    }
    
    /* 创建一个变量声明节点 */
    Stru_AstNode_t* var_decl = F_create_ast_node(Eum_AST_VARIABLE_DECL, NULL, 2, 1);
    if (!var_decl) {
        printf("  错误: 无法创建变量声明节点\n");
        F_destroy_ast_node(program);
        return NULL;
    }
    
    /* 设置变量名和类型 */
    F_ast_set_identifier(var_decl, "test_var");
    F_ast_set_type_name(var_decl, "整数");
    if (!F_ast_add_child(program, var_decl)) {
        printf("  错误: 无法添加变量声明到程序\n");
        F_destroy_ast_node(var_decl);
        F_destroy_ast_node(program);
        return NULL;
    }
    
    /* 创建一个表达式语句节点 */
    Stru_AstNode_t* expr_stmt = F_create_ast_node(Eum_AST_EXPRESSION_STMT, NULL, 3, 1);
    if (!expr_stmt) {
        printf("  错误: 无法创建表达式语句节点\n");
        F_destroy_ast_node(program);
        return NULL;
    }
    
    /* 创建一个赋值表达式节点 */
    Stru_AstNode_t* assign_expr = F_create_ast_node(Eum_AST_ASSIGN_EXPR, NULL, 3, 5);
    if (!assign_expr) {
        printf("  错误: 无法创建赋值表达式节点\n");
        F_destroy_ast_node(expr_stmt);
        F_destroy_ast_node(program);
        return NULL;
    }
    
    printf("  设置赋值表达式运算符类型...\n");
    F_ast_set_operator_type(assign_expr, 1); // 简单赋值运算符
    printf("  运算符类型设置完成\n");
    
    /* 左操作数：标识符节点 */
    Stru_AstNode_t* left_id = F_create_ast_node(Eum_AST_IDENTIFIER_EXPR, NULL, 3, 6);
    if (!left_id) {
        printf("  错误: 无法创建左操作数节点\n");
        F_destroy_ast_node(assign_expr);
        F_destroy_ast_node(expr_stmt);
        F_destroy_ast_node(program);
        return NULL;
    }
    
    F_ast_set_identifier(left_id, "test_var");
    if (!F_ast_add_child(assign_expr, left_id)) {
        printf("  错误: 无法添加左操作数到赋值表达式\n");
        F_destroy_ast_node(left_id);
        F_destroy_ast_node(assign_expr);
        F_destroy_ast_node(expr_stmt);
        F_destroy_ast_node(program);
        return NULL;
    }
    
    /* 右操作数：整数字面量节点 */
    Stru_AstNode_t* right_literal = F_create_ast_node(Eum_AST_INT_LITERAL, NULL, 3, 10);
    if (!right_literal) {
        printf("  错误: 无法创建右操作数节点\n");
        F_destroy_ast_node(assign_expr);
        F_destroy_ast_node(expr_stmt);
        F_destroy_ast_node(program);
        return NULL;
    }
    
    printf("  设置整数字面量值...\n");
    F_ast_set_int_value(right_literal, 42);
    printf("  整数字面量值设置完成\n");
    if (!F_ast_add_child(assign_expr, right_literal)) {
        printf("  错误: 无法添加右操作数到赋值表达式\n");
        F_destroy_ast_node(right_literal);
        F_destroy_ast_node(assign_expr);
        F_destroy_ast_node(expr_stmt);
        F_destroy_ast_node(program);
        return NULL;
    }
    
    if (!F_ast_add_child(expr_stmt, assign_expr)) {
        printf("  错误: 无法添加赋值表达式到表达式语句\n");
        F_destroy_ast_node(assign_expr);
        F_destroy_ast_node(expr_stmt);
        F_destroy_ast_node(program);
        return NULL;
    }
    
    if (!F_ast_add_child(program, expr_stmt)) {
        printf("  错误: 无法添加表达式语句到程序\n");
        F_destroy_ast_node(expr_stmt);
        F_destroy_ast_node(program);
        return NULL;
    }
    
    printf("  AST创建成功\n");
    return program;
}

/**
 * @brief 测试代码生成基本功能
 * 
 * 验证代码生成器能正确生成代码。
 */
static bool test_codegen_basic_generation(void)
{
    printf("测试: 代码生成基本功能\n");
    
    /* 创建代码生成器 */
    Stru_CodeGeneratorInterface_t* codegen = F_create_codegen_interface();
    TEST_ASSERT_NOT_NULL(codegen);
    
    if (!codegen) {
        return false;
    }
    
    /* 初始化代码生成器 */
    Stru_CodeGenOptions_t options = F_create_default_codegen_options();
    options.target_type = Eum_TARGET_C;
    options.optimize = false;
    options.optimization_level = 0;
    
    bool init_result = codegen->initialize(codegen, &options);
    TEST_ASSERT_TRUE(init_result);
    
    /* 创建测试AST - 简化版本，只创建程序节点 */
    Stru_AstNode_t* test_ast = F_create_ast_node(Eum_AST_PROGRAM, NULL, 1, 1);
    TEST_ASSERT_NOT_NULL(test_ast);
    
    if (!test_ast) {
        codegen->destroy(codegen);
        return false;
    }
    
    /* 生成代码 */
    Stru_CodeGenResult_t* result = codegen->generate_code(codegen, test_ast);
    TEST_ASSERT_NOT_NULL(result);
    
    if (result) {
        /* 验证生成结果 */
        TEST_ASSERT_TRUE(result->success);
        TEST_ASSERT_NOT_NULL(result->code);
        TEST_ASSERT_TRUE(result->code_length > 0);
        
        /* 验证生成的代码包含预期内容 */
        if (result->code) {
            printf("生成的代码长度: %zu\n", result->code_length);
            printf("生成的代码预览:\n%.200s...\n", result->code);
            
            /* 检查是否包含预期的C代码元素 */
            TEST_ASSERT_STRING_CONTAINS(result->code, "#include");
            TEST_ASSERT_STRING_CONTAINS(result->code, "int main");
        }
        
        /* 清理结果 */
        F_destroy_codegen_result(result);
    }
    
    /* 清理AST */
    F_destroy_ast_node(test_ast);
    
    /* 清理代码生成器 */
    codegen->destroy(codegen);
    
    return true;
}

/**
 * @brief 测试不同目标类型的代码生成
 * 
 * 验证代码生成器支持不同的目标类型。
 */
static bool test_codegen_different_targets(void)
{
    printf("测试: 不同目标类型的代码生成\n");
    
    /* 测试C目标类型 */
    {
        printf("  测试C目标类型...\n");
        Stru_CodeGeneratorInterface_t* codegen = F_create_codegen_interface();
        TEST_ASSERT_NOT_NULL(codegen);
        
        if (codegen) {
            Stru_CodeGenOptions_t options = F_create_default_codegen_options();
            options.target_type = Eum_TARGET_C;
            
            bool init_result = codegen->initialize(codegen, &options);
            TEST_ASSERT_TRUE(init_result);
            
            /* 创建简单AST */
            printf("  创建测试AST...\n");
            Stru_AstNode_t* test_ast = create_test_ast();
            if (test_ast) {
                printf("  AST创建成功，生成代码...\n");
                Stru_CodeGenResult_t* result = codegen->generate_code(codegen, test_ast);
                TEST_ASSERT_NOT_NULL(result);
                
                if (result) {
                    printf("  代码生成结果: %s\n", result->success ? "成功" : "失败");
                    if (result->code) {
                        printf("  生成的代码长度: %zu\n", result->code_length);
                        printf("  代码预览: %.100s...\n", result->code);
                    }
                    TEST_ASSERT_TRUE(result->success);
                    F_destroy_codegen_result(result);
                } else {
                    printf("  代码生成返回NULL结果\n");
                }
                
                F_destroy_ast_node(test_ast);
                printf("  AST已销毁\n");
            } else {
                printf("  无法创建测试AST\n");
            }
            
            codegen->destroy(codegen);
            printf("  代码生成器已销毁\n");
        }
    }
    
    /* 测试其他目标类型（应该返回未实现错误） */
    {
        printf("  测试LLVM IR目标类型...\n");
        Stru_CodeGeneratorInterface_t* codegen = F_create_codegen_interface();
        TEST_ASSERT_NOT_NULL(codegen);
        
        if (codegen) {
            Stru_CodeGenOptions_t options = F_create_default_codegen_options();
            options.target_type = Eum_TARGET_LLVM_IR;
            
            bool init_result = codegen->initialize(codegen, &options);
            TEST_ASSERT_TRUE(init_result);
            
            /* 创建简单AST */
            Stru_AstNode_t* test_ast = create_test_ast();
            if (test_ast) {
                Stru_CodeGenResult_t* result = codegen->generate_code(codegen, test_ast);
                TEST_ASSERT_NOT_NULL(result);
                
                if (result) {
                    /* LLVM IR后端尚未实现，应该返回失败或占位符代码 */
                    TEST_ASSERT_TRUE(result->code != NULL);
                    TEST_ASSERT_STRING_CONTAINS(result->code, "LLVM IR");
                    F_destroy_codegen_result(result);
                }
                
                F_destroy_ast_node(test_ast);
            }
            
            codegen->destroy(codegen);
        }
    }
    
    return true;
}

/**
 * @brief 测试错误处理
 * 
 * 验证代码生成器在无效输入时的错误处理。
 */
static bool test_codegen_error_handling(void)
{
    printf("测试: 代码生成错误处理\n");
    
    Stru_CodeGeneratorInterface_t* codegen = F_create_codegen_interface();
    TEST_ASSERT_NOT_NULL(codegen);
    
    if (!codegen) {
        return false;
    }
    
    /* 测试1: 未初始化时生成代码 */
    Stru_AstNode_t* test_ast = create_test_ast();
    if (test_ast) {
        Stru_CodeGenResult_t* result = codegen->generate_code(codegen, test_ast);
        TEST_ASSERT_NOT_NULL(result);
        
        if (result) {
            /* 应该失败，因为未初始化 */
            TEST_ASSERT_FALSE(result->success);
            F_destroy_codegen_result(result);
        }
        
        F_destroy_ast_node(test_ast);
    }
    
    /* 测试2: 使用NULL AST生成代码 */
    Stru_CodeGenOptions_t default_options = F_create_default_codegen_options();
    codegen->initialize(codegen, &default_options);
    Stru_CodeGenResult_t* result = codegen->generate_code(codegen, NULL);
    TEST_ASSERT_NOT_NULL(result);
    
    if (result) {
        TEST_ASSERT_FALSE(result->success);
        F_destroy_codegen_result(result);
    }
    
    /* 清理 */
    codegen->destroy(codegen);
    
    return true;
}

/**
 * @brief 运行所有集成测试
 * 
 * 运行代码生成器与AST模块集成的所有测试用例。
 * 
 * @return 所有测试通过返回true，否则返回false
 */
bool test_codegen_ast_integration_all(void)
{
    printf("开始运行代码生成器与AST模块集成测试...\n");
    printf("========================================\n\n");
    
    bool all_passed = true;
    
    /* 运行各个测试用例 */
    if (!test_codegen_basic_generation()) {
        printf("❌ test_codegen_basic_generation 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_codegen_basic_generation 通过\n");
    }
    
    if (!test_codegen_different_targets()) {
        printf("❌ test_codegen_different_targets 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_codegen_different_targets 通过\n");
    }
    
    if (!test_codegen_error_handling()) {
        printf("❌ test_codegen_error_handling 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_codegen_error_handling 通过\n");
    }
    
    printf("\n========================================\n");
    if (all_passed) {
        printf("✅ 所有集成测试通过！\n");
    } else {
        printf("❌ 有集成测试失败\n");
    }
    
    return all_passed;
}
