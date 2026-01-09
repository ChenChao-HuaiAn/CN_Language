/******************************************************************************
 * 文件名: test_codegen_interface.c
 * 功能: CN_Language 代码生成器接口测试
 * 
 * 测试代码生成器接口的定义和基本功能。
 * 遵循项目测试规范，使用模块化测试结构。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月9日
 * 修改历史:
 *   - 2026年1月9日: 初始版本
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
#endif

/* 包含被测试的头文件 */
#include "../../../src/core/codegen/CN_codegen_interface.h"

/**
 * @brief 测试接口定义完整性
 * 
 * 验证代码生成器接口结构体定义是否完整。
 */
static bool test_interface_definition(void)
{
    printf("测试: 接口定义完整性\n");
    
    /* 验证接口结构体大小 */
    TEST_ASSERT_TRUE(sizeof(Stru_CodeGeneratorInterface_t) > 0);
    
    /* 验证枚举定义 */
    TEST_ASSERT_EQUAL(0, Eum_TARGET_C);
    TEST_ASSERT_EQUAL(1, Eum_TARGET_LLVM_IR);
    TEST_ASSERT_EQUAL(2, Eum_TARGET_X86_64);
    TEST_ASSERT_EQUAL(3, Eum_TARGET_ARM64);
    TEST_ASSERT_EQUAL(4, Eum_TARGET_WASM);
    TEST_ASSERT_EQUAL(5, Eum_TARGET_BYTECODE);
    
    return true;
}

/**
 * @brief 测试选项结构体
 * 
 * 验证代码生成选项结构体的功能。
 */
static bool test_options_structure(void)
{
    printf("测试: 选项结构体\n");
    
    /* 测试默认选项创建 */
    Stru_CodeGenOptions_t options = F_create_default_codegen_options();
    
    TEST_ASSERT_EQUAL(Eum_TARGET_C, options.target_type);
    TEST_ASSERT_TRUE(options.optimize);
    TEST_ASSERT_EQUAL(2, options.optimization_level);
    TEST_ASSERT_FALSE(options.debug_info);
    TEST_ASSERT_NULL(options.output_file);
    TEST_ASSERT_FALSE(options.verbose);
    
    /* 测试选项修改 */
    options.target_type = Eum_TARGET_LLVM_IR;
    options.optimize = false;
    options.optimization_level = 0;
    options.debug_info = true;
    options.verbose = true;
    
    TEST_ASSERT_EQUAL(Eum_TARGET_LLVM_IR, options.target_type);
    TEST_ASSERT_FALSE(options.optimize);
    TEST_ASSERT_EQUAL(0, options.optimization_level);
    TEST_ASSERT_TRUE(options.debug_info);
    TEST_ASSERT_TRUE(options.verbose);
    
    return true;
}

/**
 * @brief 测试结果结构体
 * 
 * 验证代码生成结果结构体的功能。
 */
static bool test_result_structure(void)
{
    printf("测试: 结果结构体\n");
    
    /* 测试结果创建和销毁 */
    Stru_CodeGenResult_t* result = (Stru_CodeGenResult_t*)malloc(sizeof(Stru_CodeGenResult_t));
    TEST_ASSERT_NOT_NULL(result);
    
    if (result) {
        memset(result, 0, sizeof(Stru_CodeGenResult_t));
        result->success = true;
        result->code = strdup("test code");
        result->code_length = strlen("test code");
        
        TEST_ASSERT_TRUE(result->success);
        TEST_ASSERT_NOT_NULL(result->code);
        TEST_ASSERT_EQUAL_STRING("test code", result->code);
        TEST_ASSERT_EQUAL(9, result->code_length);
        
        F_destroy_codegen_result(result);
    }
    
    /* 测试销毁NULL指针（应该安全） */
    F_destroy_codegen_result(NULL);
    
    return true;
}

/**
 * @brief 运行所有接口测试
 * 
 * 运行代码生成器接口的所有测试用例。
 * 
 * @return 所有测试通过返回true，否则返回false
 */
bool test_codegen_interface_all(void)
{
    printf("开始运行代码生成器接口测试...\n");
    printf("================================\n\n");
    
    bool all_passed = true;
    
    /* 运行各个测试用例 */
    if (!test_interface_definition()) {
        printf("❌ test_interface_definition 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_interface_definition 通过\n");
    }
    
    if (!test_options_structure()) {
        printf("❌ test_options_structure 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_options_structure 通过\n");
    }
    
    if (!test_result_structure()) {
        printf("❌ test_result_structure 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_result_structure 通过\n");
    }
    
    printf("\n================================\n");
    if (all_passed) {
        printf("✅ 所有接口测试通过！\n");
    } else {
        printf("❌ 有接口测试失败\n");
    }
    
    return all_passed;
}

/**
 * @brief 主函数（用于独立测试）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    printf("CN_Language 代码生成器接口测试\n");
    printf("===============================\n\n");
    
    bool success = test_codegen_interface_all();
    
    if (success) {
        printf("\n✅ 所有接口测试通过！\n");
        return EXIT_SUCCESS;
    } else {
        printf("\n❌ 接口测试失败！\n");
        return EXIT_FAILURE;
    }
}
#endif
