/******************************************************************************
 * 文件名: test_codegen_all.c
 * 功能: CN_Language 代码生成模块测试
 * 
 * 提供代码生成模块的完整测试套件，包括接口测试、工厂测试和功能测试。
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
#include "../../../src/core/codegen/CN_codegen_factory.c"

/* 测试函数声明 */
static bool test_create_codegen_interface(void);
static bool test_create_default_codegen_options(void);
static bool test_destroy_codegen_result(void);
static bool test_initialize_codegen(void);
static bool test_generate_code_basic(void);
static bool test_codegen_error_handling(void);

/**
 * @brief 运行所有代码生成测试
 * 
 * 运行代码生成模块的所有测试用例。
 * 
 * @return 所有测试通过返回true，否则返回false
 */
bool test_codegen_all(void)
{
    printf("开始运行代码生成模块测试...\n");
    printf("================================\n\n");
    
    bool all_passed = true;
    
    /* 运行各个测试用例 */
    if (!test_create_codegen_interface()) {
        printf("❌ test_create_codegen_interface 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_create_codegen_interface 通过\n");
    }
    
    if (!test_create_default_codegen_options()) {
        printf("❌ test_create_default_codegen_options 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_create_default_codegen_options 通过\n");
    }
    
    if (!test_destroy_codegen_result()) {
        printf("❌ test_destroy_codegen_result 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_destroy_codegen_result 通过\n");
    }
    
    if (!test_initialize_codegen()) {
        printf("❌ test_initialize_codegen 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_initialize_codegen 通过\n");
    }
    
    if (!test_generate_code_basic()) {
        printf("❌ test_generate_code_basic 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_generate_code_basic 通过\n");
    }
    
    if (!test_codegen_error_handling()) {
        printf("❌ test_codegen_error_handling 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_codegen_error_handling 通过\n");
    }
    
    printf("\n================================\n");
    if (all_passed) {
        printf("✅ 所有代码生成测试通过！\n");
    } else {
        printf("❌ 有测试失败\n");
    }
    
    return all_passed;
}

/**
 * @brief 测试创建代码生成器接口
 */
static bool test_create_codegen_interface(void)
{
    printf("测试: 创建代码生成器接口\n");
    
    /* 测试1: 正常创建 */
    Stru_CodeGeneratorInterface_t* codegen = F_create_codegen_interface();
    TEST_ASSERT_NOT_NULL(codegen);
    
    if (codegen) {
        /* 测试接口函数指针 */
        TEST_ASSERT_NOT_NULL(codegen->initialize);
        TEST_ASSERT_NOT_NULL(codegen->generate_code);
        TEST_ASSERT_NOT_NULL(codegen->destroy);
        
        /* 清理 */
        if (codegen->destroy) {
            codegen->destroy(codegen);
        } else {
            free(codegen);
        }
    }
    
    return true;
}

/**
 * @brief 测试创建默认代码生成选项
 */
static bool test_create_default_codegen_options(void)
{
    printf("测试: 创建默认代码生成选项\n");
    
    Stru_CodeGenOptions_t options = F_create_default_codegen_options();
    
    /* 验证默认值 */
    TEST_ASSERT_EQUAL(Eum_TARGET_C, options.target_type);
    TEST_ASSERT_TRUE(options.optimize);
    TEST_ASSERT_EQUAL(2, options.optimization_level);
    TEST_ASSERT_FALSE(options.debug_info);
    TEST_ASSERT_NULL(options.output_file);
    TEST_ASSERT_FALSE(options.verbose);
    
    return true;
}

/**
 * @brief 测试销毁代码生成结果
 */
static bool test_destroy_codegen_result(void)
{
    printf("测试: 销毁代码生成结果\n");
    
    /* 测试1: 销毁NULL指针（应该安全） */
    F_destroy_codegen_result(NULL);
    
    /* 测试2: 销毁空结果 */
    Stru_CodeGenResult_t* result = (Stru_CodeGenResult_t*)malloc(sizeof(Stru_CodeGenResult_t));
    TEST_ASSERT_NOT_NULL(result);
    
    if (result) {
        memset(result, 0, sizeof(Stru_CodeGenResult_t));
        F_destroy_codegen_result(result);
    }
    
    /* 测试3: 销毁有代码的结果 */
    result = (Stru_CodeGenResult_t*)malloc(sizeof(Stru_CodeGenResult_t));
    TEST_ASSERT_NOT_NULL(result);
    
    if (result) {
        memset(result, 0, sizeof(Stru_CodeGenResult_t));
        result->code = strdup("test code");
        result->code_length = strlen("test code");
        F_destroy_codegen_result(result);
    }
    
    return true;
}

/**
 * @brief 测试初始化代码生成器
 */
static bool test_initialize_codegen(void)
{
    printf("测试: 初始化代码生成器\n");
    
    Stru_CodeGeneratorInterface_t* codegen = F_create_codegen_interface();
    TEST_ASSERT_NOT_NULL(codegen);
    
    if (!codegen) {
        return false;
    }
    
    /* 测试1: 使用默认选项初始化 */
    Stru_CodeGenOptions_t options = F_create_default_codegen_options();
    bool init_result = codegen->initialize(codegen, &options);
    TEST_ASSERT_TRUE(init_result);
    
    /* 测试2: 使用无效优化级别初始化 */
    options.optimization_level = 5; // 无效级别
    init_result = codegen->initialize(codegen, &options);
    TEST_ASSERT_FALSE(init_result); // 应该失败
    
    /* 测试3: 使用NULL选项初始化 */
    init_result = codegen->initialize(codegen, NULL);
    TEST_ASSERT_FALSE(init_result); // 应该失败
    
    /* 清理 */
    codegen->destroy(codegen);
    
    return true;
}

/**
 * @brief 测试基本代码生成
 */
static bool test_generate_code_basic(void)
{
    printf("测试: 基本代码生成\n");
    
    Stru_CodeGeneratorInterface_t* codegen = F_create_codegen_interface();
    TEST_ASSERT_NOT_NULL(codegen);
    
    if (!codegen) {
        return false;
    }
    
    /* 初始化代码生成器 */
    Stru_CodeGenOptions_t options = F_create_default_codegen_options();
    options.target_type = Eum_TARGET_C;
    bool init_result = codegen->initialize(codegen, &options);
    TEST_ASSERT_TRUE(init_result);
    
    if (!init_result) {
        codegen->destroy(codegen);
        return false;
    }
    
    /* 测试代码生成（使用NULL AST，应该返回错误） */
    Stru_CodeGenResult_t* result = codegen->generate_code(codegen, NULL);
    TEST_ASSERT_NOT_NULL(result);
    
    if (result) {
        /* 由于AST为NULL，应该失败 */
        TEST_ASSERT_FALSE(result->success);
        F_destroy_codegen_result(result);
    }
    
    /* 清理 */
    codegen->destroy(codegen);
    
    return true;
}

/**
 * @brief 测试错误处理
 */
static bool test_codegen_error_handling(void)
{
    printf("测试: 代码生成错误处理\n");
    
    Stru_CodeGeneratorInterface_t* codegen = F_create_codegen_interface();
    TEST_ASSERT_NOT_NULL(codegen);
    
    if (!codegen) {
        return false;
    }
    
    /* 测试1: 检查初始错误状态 */
    bool has_errors = codegen->has_errors(codegen);
    TEST_ASSERT_FALSE(has_errors);
    
    /* 测试2: 获取初始错误列表 */
    Stru_DynamicArray_t* errors = codegen->get_errors(codegen);
    TEST_ASSERT_NULL(errors); // 初始应该为NULL或空
    
    /* 测试3: 清除错误（应该安全） */
    codegen->clear_errors(codegen);
    
    /* 测试4: 重置代码生成器 */
    codegen->reset(codegen);
    
    /* 清理 */
    codegen->destroy(codegen);
    
    return true;
}

/**
 * @brief 主函数（用于独立测试）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    printf("CN_Language 代码生成模块测试\n");
    printf("=============================\n\n");
    
    bool success = test_codegen_all();
    
    if (success) {
        printf("\n✅ 所有测试通过！\n");
        return EXIT_SUCCESS;
    } else {
        printf("\n❌ 测试失败！\n");
        return EXIT_FAILURE;
    }
}
#endif
