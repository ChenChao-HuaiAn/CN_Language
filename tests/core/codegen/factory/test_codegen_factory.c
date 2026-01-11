/******************************************************************************
 * 文件名: test_codegen_factory.c
 * 功能: CN_Language 代码生成器工厂测试
 * 
 * 测试代码生成器工厂函数和接口实例创建。
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
#include "../../../../src/core/codegen/CN_codegen_interface.h"

/**
 * @brief 测试工厂函数创建接口
 * 
 * 验证工厂函数能正确创建代码生成器接口实例。
 */
static bool test_factory_create_interface(void)
{
    printf("测试: 工厂函数创建接口\n");
    
    /* 测试正常创建 */
    Stru_CodeGeneratorInterface_t* codegen = F_create_codegen_interface();
    TEST_ASSERT_NOT_NULL(codegen);
    
    if (codegen) {
        /* 验证接口函数指针 */
        TEST_ASSERT_NOT_NULL(codegen->initialize);
        TEST_ASSERT_NOT_NULL(codegen->generate_code);
        TEST_ASSERT_NOT_NULL(codegen->optimize);
        TEST_ASSERT_NOT_NULL(codegen->destroy);
        
        /* 验证内部状态 */
        TEST_ASSERT_NOT_NULL(codegen->internal_state);
        
        /* 清理 */
        codegen->destroy(codegen);
    }
    
    return true;
}

/**
 * @brief 测试接口初始化功能
 * 
 * 验证代码生成器接口的初始化功能。
 */
static bool test_interface_initialization(void)
{
    printf("测试: 接口初始化功能\n");
    
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
    
    /* 测试4: 使用不同目标类型初始化 */
    options = F_create_default_codegen_options();
    options.target_type = Eum_TARGET_LLVM_IR;
    init_result = codegen->initialize(codegen, &options);
    TEST_ASSERT_TRUE(init_result); // 应该成功
    
    /* 清理 */
    codegen->destroy(codegen);
    
    return true;
}

/**
 * @brief 测试目标类型设置和获取
 * 
 * 验证目标类型设置和获取功能。
 */
static bool test_target_type_operations(void)
{
    printf("测试: 目标类型操作\n");
    
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
    
    /* 测试获取目标类型 */
    Eum_TargetCodeType target_type = codegen->get_target_type(codegen);
    TEST_ASSERT_EQUAL(Eum_TARGET_C, target_type);
    
    /* 测试设置目标类型 */
    bool set_result = codegen->set_target(codegen, Eum_TARGET_LLVM_IR, NULL, NULL);
    TEST_ASSERT_TRUE(set_result);
    
    /* 验证目标类型已更新 */
    target_type = codegen->get_target_type(codegen);
    TEST_ASSERT_EQUAL(Eum_TARGET_LLVM_IR, target_type);
    
    /* 清理 */
    codegen->destroy(codegen);
    
    return true;
}

/**
 * @brief 测试优化级别支持查询
 * 
 * 验证优化级别支持查询功能。
 */
static bool test_optimization_level_support(void)
{
    printf("测试: 优化级别支持查询\n");
    
    Stru_CodeGeneratorInterface_t* codegen = F_create_codegen_interface();
    TEST_ASSERT_NOT_NULL(codegen);
    
    if (!codegen) {
        return false;
    }
    
    /* 查询支持的优化级别 */
    int min_level = -1;
    int max_level = -1;
    codegen->get_supported_optimization_levels(codegen, &min_level, &max_level);
    
    /* 验证返回的级别范围 */
    TEST_ASSERT_EQUAL(0, min_level);
    TEST_ASSERT_EQUAL(3, max_level);
    
    /* 清理 */
    codegen->destroy(codegen);
    
    return true;
}

/**
 * @brief 测试错误处理功能
 * 
 * 验证代码生成器的错误处理功能。
 */
static bool test_error_handling_functions(void)
{
    printf("测试: 错误处理功能\n");
    
    Stru_CodeGeneratorInterface_t* codegen = F_create_codegen_interface();
    TEST_ASSERT_NOT_NULL(codegen);
    
    if (!codegen) {
        return false;
    }
    
    /* 测试初始错误状态 */
    bool has_errors = codegen->has_errors(codegen);
    TEST_ASSERT_FALSE(has_errors);
    
    /* 测试获取错误列表 */
    Stru_DynamicArray_t* errors = codegen->get_errors(codegen);
    TEST_ASSERT_NULL(errors); // 初始应该为NULL或空
    
    /* 测试清除错误 */
    codegen->clear_errors(codegen);
    
    /* 测试重置功能 */
    codegen->reset(codegen);
    
    /* 清理 */
    codegen->destroy(codegen);
    
    return true;
}

/**
 * @brief 运行所有工厂测试
 * 
 * 运行代码生成器工厂的所有测试用例。
 * 
 * @return 所有测试通过返回true，否则返回false
 */
bool test_codegen_factory_all(void)
{
    printf("开始运行代码生成器工厂测试...\n");
    printf("================================\n\n");
    
    bool all_passed = true;
    
    /* 运行各个测试用例 */
    if (!test_factory_create_interface()) {
        printf("❌ test_factory_create_interface 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_factory_create_interface 通过\n");
    }
    
    if (!test_interface_initialization()) {
        printf("❌ test_interface_initialization 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_interface_initialization 通过\n");
    }
    
    if (!test_target_type_operations()) {
        printf("❌ test_target_type_operations 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_target_type_operations 通过\n");
    }
    
    if (!test_optimization_level_support()) {
        printf("❌ test_optimization_level_support 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_optimization_level_support 通过\n");
    }
    
    if (!test_error_handling_functions()) {
        printf("❌ test_error_handling_functions 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_error_handling_functions 通过\n");
    }
    
    printf("\n================================\n");
    if (all_passed) {
        printf("✅ 所有工厂测试通过！\n");
    } else {
        printf("❌ 有工厂测试失败\n");
    }
    
    return all_passed;
}
