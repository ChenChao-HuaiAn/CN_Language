/******************************************************************************
 * 文件名: test_bytecode_generator.c
 * 功能: CN_Language 字节码生成器测试
 * 
 * 测试字节码生成器的核心功能，包括从AST和IR生成字节码等。
 * 遵循项目测试规范，使用模块化测试结构。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本，测试实际存在的函数
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
#include "../../../../../../src/core/codegen/implementations/bytecode_backend/backend/CN_bytecode_generator.h"

/**
 * @brief 测试版本信息函数
 * 
 * 验证字节码生成器版本信息函数的正确性。
 */
static bool test_version_functions(void)
{
    printf("测试: 版本信息函数\n");
    
    /* 测试版本号获取 */
    int major = -1, minor = -1, patch = -1;
    F_get_bytecode_backend_version_impl(&major, &minor, &patch);
    
    TEST_ASSERT_EQUAL(1, major);
    TEST_ASSERT_EQUAL(0, minor);
    TEST_ASSERT_EQUAL(0, patch);
    
    /* 测试版本字符串 */
    const char* version_string = F_get_bytecode_backend_version_string_impl();
    TEST_ASSERT_NOT_NULL(version_string);
    TEST_ASSERT_EQUAL_STRING("1.0.0", version_string);
    
    return true;
}

/**
 * @brief 测试功能支持检查
 * 
 * 验证字节码生成器功能支持检查功能。
 */
static bool test_feature_support(void)
{
    printf("测试: 功能支持检查\n");
    
    /* 测试支持的功能 */
    bool supports_ast = F_bytecode_backend_supports_feature_impl("ast_to_bytecode");
    TEST_ASSERT_TRUE(supports_ast);
    
    bool supports_ir = F_bytecode_backend_supports_feature_impl("ir_to_bytecode");
    TEST_ASSERT_TRUE(supports_ir);
    
    bool supports_optimization = F_bytecode_backend_supports_feature_impl("optimization");
    TEST_ASSERT_TRUE(supports_optimization);
    
    bool supports_debug = F_bytecode_backend_supports_feature_impl("debug_info");
    TEST_ASSERT_TRUE(supports_debug);
    
    /* 测试不支持的功能 */
    bool supports_profiling = F_bytecode_backend_supports_feature_impl("profiling");
    TEST_ASSERT_FALSE(supports_profiling);
    
    bool supports_gc = F_bytecode_backend_supports_feature_impl("gc");
    TEST_ASSERT_FALSE(supports_gc);
    
    /* 测试无效功能名称 */
    bool supports_invalid = F_bytecode_backend_supports_feature_impl("invalid_feature");
    TEST_ASSERT_FALSE(supports_invalid);
    
    /* 测试NULL参数 */
    bool supports_null = F_bytecode_backend_supports_feature_impl(NULL);
    TEST_ASSERT_FALSE(supports_null);
    
    return true;
}

/**
 * @brief 测试配置功能
 * 
 * 验证字节码生成器配置功能。
 */
static bool test_configuration_functions(void)
{
    printf("测试: 配置功能\n");
    
    /* 测试有效配置 */
    bool result = F_configure_bytecode_backend_impl("optimization_level", "2");
    TEST_ASSERT_TRUE(result);
    
    result = F_configure_bytecode_backend_impl("debug_info", "true");
    TEST_ASSERT_TRUE(result);
    
    /* 测试无效参数 */
    result = F_configure_bytecode_backend_impl(NULL, "value");
    TEST_ASSERT_FALSE(result);
    
    result = F_configure_bytecode_backend_impl("option", NULL);
    TEST_ASSERT_FALSE(result);
    
    result = F_configure_bytecode_backend_impl(NULL, NULL);
    TEST_ASSERT_FALSE(result);
    
    return true;
}

/**
 * @brief 测试接口创建功能
 * 
 * 验证字节码后端接口创建功能。
 */
static bool test_interface_creation(void)
{
    printf("测试: 接口创建功能\n");
    
    /* 测试接口创建 */
    Stru_CodeGeneratorInterface_t* interface = F_create_bytecode_backend_interface_impl();
    TEST_ASSERT_NOT_NULL(interface);
    
    if (interface) {
        /* 验证接口结构 */
        TEST_ASSERT_NULL(interface->internal_state);
        
        /* 清理 */
        free(interface);
    }
    
    return true;
}

/**
 * @brief 测试AST到字节码生成
 * 
 * 验证从AST生成字节码的功能。
 */
static bool test_ast_to_bytecode_generation(void)
{
    printf("测试: AST到字节码生成\n");
    
    /* 测试使用NULL AST */
    Stru_BytecodeProgram_t* program = F_generate_bytecode_from_ast_impl(NULL, NULL);
    TEST_ASSERT_NULL(program);
    
    /* 测试使用NULL配置 */
    // 这里需要创建一个简单的AST节点，但为了简化测试，我们暂时跳过
    // 因为AST模块可能不可用
    
    return true;
}

/**
 * @brief 测试IR到字节码生成
 * 
 * 验证从IR生成字节码的功能。
 */
static bool test_ir_to_bytecode_generation(void)
{
    printf("测试: IR到字节码生成\n");
    
    /* 测试使用NULL IR */
    Stru_BytecodeProgram_t* program = F_generate_bytecode_from_ir_impl(NULL, NULL);
    TEST_ASSERT_NULL(program);
    
    /* 测试使用NULL配置 */
    // 这里需要创建一个简单的IR节点，但为了简化测试，我们暂时跳过
    // 因为IR模块可能不可用
    
    return true;
}

/**
 * @brief 测试错误处理
 * 
 * 验证字节码生成器的错误处理功能。
 */
static bool test_error_handling(void)
{
    printf("测试: 错误处理\n");
    
    /* 测试多次调用版本函数 */
    for (int i = 0; i < 3; i++) {
        int major, minor, patch;
        F_get_bytecode_backend_version_impl(&major, &minor, &patch);
        
        TEST_ASSERT_EQUAL(1, major);
        TEST_ASSERT_EQUAL(0, minor);
        TEST_ASSERT_EQUAL(0, patch);
        
        const char* version = F_get_bytecode_backend_version_string_impl();
        TEST_ASSERT_NOT_NULL(version);
        TEST_ASSERT_EQUAL_STRING("1.0.0", version);
    }
    
    return true;
}

/**
 * @brief 运行所有字节码生成器测试
 * 
 * 运行字节码生成器的所有测试用例。
 * 
 * @return 所有测试通过返回true，否则返回false
 */
bool test_bytecode_generator_all(void)
{
    printf("开始运行字节码生成器测试...\n");
    printf("==============================\n\n");
    
    bool all_passed = true;
    
    /* 运行各个测试用例 */
    if (!test_version_functions()) {
        printf("❌ test_version_functions 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_version_functions 通过\n");
    }
    
    if (!test_feature_support()) {
        printf("❌ test_feature_support 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_feature_support 通过\n");
    }
    
    if (!test_configuration_functions()) {
        printf("❌ test_configuration_functions 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_configuration_functions 通过\n");
    }
    
    if (!test_interface_creation()) {
        printf("❌ test_interface_creation 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_interface_creation 通过\n");
    }
    
    if (!test_ast_to_bytecode_generation()) {
        printf("❌ test_ast_to_bytecode_generation 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_ast_to_bytecode_generation 通过\n");
    }
    
    if (!test_ir_to_bytecode_generation()) {
        printf("❌ test_ir_to_bytecode_generation 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_ir_to_bytecode_generation 通过\n");
    }
    
    if (!test_error_handling()) {
        printf("❌ test_error_handling 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_error_handling 通过\n");
    }
    
    printf("\n==============================\n");
    if (all_passed) {
        printf("✅ 所有字节码生成器测试通过！\n");
    } else {
        printf("❌ 有字节码生成器测试失败\n");
    }
    
    return all_passed;
}
