/******************************************************************************
 * 文件名: test_bytecode_backend.c
 * 功能: CN_Language 字节码后端测试
 * 
 * 测试字节码后端的功能，包括版本信息、配置管理、程序创建和销毁等。
 * 遵循项目测试规范，使用模块化测试结构。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
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
#include "../../../../../../src/core/codegen/implementations/bytecode_backend/CN_bytecode_backend.h"

/**
 * @brief 测试版本信息函数
 * 
 * 验证字节码后端版本信息函数的正确性。
 */
static bool test_version_functions(void)
{
    printf("测试: 版本信息函数\n");
    
    /* 测试版本号获取 */
    int major = -1, minor = -1, patch = -1;
    F_get_bytecode_backend_version(&major, &minor, &patch);
    
    TEST_ASSERT_EQUAL(1, major);
    TEST_ASSERT_EQUAL(0, minor);
    TEST_ASSERT_EQUAL(0, patch);
    
    /* 测试版本字符串 */
    const char* version_string = F_get_bytecode_backend_version_string();
    TEST_ASSERT_NOT_NULL(version_string);
    TEST_ASSERT_EQUAL_STRING("1.0.0", version_string);
    
    return true;
}

/**
 * @brief 测试功能支持检查
 * 
 * 验证字节码后端支持的功能检查。
 */
static bool test_feature_support(void)
{
    printf("测试: 功能支持检查\n");
    
    /* 测试支持的功能 */
    TEST_ASSERT_TRUE(F_bytecode_backend_supports_feature("ast_to_bytecode"));
    TEST_ASSERT_TRUE(F_bytecode_backend_supports_feature("ir_to_bytecode"));
    
    /* 测试不支持的功能（可能未实现） */
    TEST_ASSERT_FALSE(F_bytecode_backend_supports_feature("invalid_feature"));
    
    return true;
}

/**
 * @brief 测试配置管理
 * 
 * 验证字节码后端配置管理功能。
 */
static bool test_configuration_management(void)
{
    printf("测试: 配置管理\n");
    
    /* 测试默认配置创建 */
    Stru_BytecodeBackendConfig_t config = F_create_default_bytecode_backend_config();
    
    TEST_ASSERT_TRUE(config.optimize_bytecode);
    TEST_ASSERT_EQUAL(2, config.optimization_level);
    TEST_ASSERT_FALSE(config.include_debug_info);
    TEST_ASSERT_TRUE(config.stack_size > 0);
    TEST_ASSERT_TRUE(config.heap_size > 0);
    TEST_ASSERT_FALSE(config.enable_gc);
    TEST_ASSERT_FALSE(config.enable_profiling);
    TEST_ASSERT_NOT_NULL(config.output_format);
    TEST_ASSERT_EQUAL_STRING("binary", config.output_format);
    
    /* 测试配置选项 */
    TEST_ASSERT_TRUE(F_configure_bytecode_backend("optimization_level", "3"));
    TEST_ASSERT_TRUE(F_configure_bytecode_backend("debug_info", "true"));
    TEST_ASSERT_TRUE(F_configure_bytecode_backend("stack_size", "65536"));
    
    /* 测试无效配置选项 */
    TEST_ASSERT_FALSE(F_configure_bytecode_backend("invalid_option", "value"));
    
    return true;
}

/**
 * @brief 测试接口创建
 * 
 * 验证字节码后端接口创建功能。
 */
static bool test_interface_creation(void)
{
    printf("测试: 接口创建\n");
    
    /* 测试接口创建 */
    Stru_CodeGeneratorInterface_t* interface = F_create_bytecode_backend_interface();
    TEST_ASSERT_NOT_NULL(interface);
    
    if (interface) {
        /* 验证接口函数指针不为NULL */
        TEST_ASSERT_NOT_NULL(interface->initialize);
        TEST_ASSERT_NOT_NULL(interface->generate_code);
        TEST_ASSERT_NOT_NULL(interface->destroy);
        
        /* 测试接口销毁 */
        interface->destroy(interface);
    }
    
    /* 测试多次创建和销毁 */
    for (int i = 0; i < 3; i++) {
        Stru_CodeGeneratorInterface_t* iface = F_create_bytecode_backend_interface();
        TEST_ASSERT_NOT_NULL(iface);
        
        if (iface) {
            iface->destroy(iface);
        }
    }
    
    return true;
}

/**
 * @brief 测试字节码程序管理
 * 
 * 验证字节码程序创建和销毁功能。
 */
static bool test_bytecode_program_management(void)
{
    printf("测试: 字节码程序管理\n");
    
    /* 测试使用NULL参数生成字节码 */
    Stru_BytecodeBackendConfig_t config = F_create_default_bytecode_backend_config();
    
    Stru_BytecodeProgram_t* program1 = F_generate_bytecode_from_ast(NULL, &config);
    /* 接受NULL AST的情况，可能返回NULL或空程序 */
    if (program1) {
        F_destroy_bytecode_program(program1);
    }
    
    Stru_BytecodeProgram_t* program2 = F_generate_bytecode_from_ir(NULL, &config);
    if (program2) {
        F_destroy_bytecode_program(program2);
    }
    
    /* 测试销毁NULL指针（应该安全） */
    F_destroy_bytecode_program(NULL);
    
    return true;
}

/**
 * @brief 测试序列化和反序列化
 * 
 * 验证字节码程序序列化和反序列化功能。
 */
static bool test_serialization(void)
{
    printf("测试: 序列化和反序列化\n");
    
    /* 测试使用NULL参数 */
    uint8_t* data = NULL;
    size_t size = 0;
    
    /* 序列化NULL程序应该失败 */
    TEST_ASSERT_FALSE(F_serialize_bytecode_program(NULL, &data, &size));
    
    /* 反序列化NULL数据应该失败 */
    TEST_ASSERT_NULL(F_deserialize_bytecode_program(NULL, 0));
    
    /* 清理 */
    if (data) {
        free(data);
    }
    
    return true;
}

/**
 * @brief 测试验证功能
 * 
 * 验证字节码程序验证功能。
 */
static bool test_validation(void)
{
    printf("测试: 验证功能\n");
    
    /* 测试验证NULL程序 */
    char** errors = NULL;
    size_t error_count = 0;
    
    TEST_ASSERT_FALSE(F_validate_bytecode_program(NULL, &errors, &error_count));
    
    /* 清理 */
    if (errors) {
        for (size_t i = 0; i < error_count; i++) {
            free(errors[i]);
        }
        free(errors);
    }
    
    return true;
}

/**
 * @brief 测试格式化功能
 * 
 * 验证字节码程序格式化功能。
 */
static bool test_formatting(void)
{
    printf("测试: 格式化功能\n");
    
    /* 测试格式化NULL程序 */
    char* formatted = F_format_bytecode_program(NULL);
    /* 接受NULL程序的情况，可能返回NULL或空字符串 */
    if (formatted) {
        free(formatted);
    }
    
    return true;
}

/**
 * @brief 测试优化功能
 * 
 * 验证字节码程序优化功能。
 */
static bool test_optimization(void)
{
    printf("测试: 优化功能\n");
    
    /* 测试优化NULL程序 */
    Stru_BytecodeProgram_t* optimized = F_optimize_bytecode_program(NULL, 2);
    TEST_ASSERT_NULL(optimized);
    
    /* 测试无效优化级别 */
    Stru_BytecodeBackendConfig_t config = F_create_default_bytecode_backend_config();
    Stru_BytecodeProgram_t* program = F_generate_bytecode_from_ast(NULL, &config);
    
    if (program) {
        Stru_BytecodeProgram_t* opt = F_optimize_bytecode_program(program, -1); /* 无效级别 */
        /* 接受无效级别的情况 */
        if (opt) {
            F_destroy_bytecode_program(opt);
        }
        F_destroy_bytecode_program(program);
    }
    
    return true;
}

/**
 * @brief 运行所有字节码后端测试
 * 
 * 运行字节码后端的所有测试用例。
 * 
 * @return 所有测试通过返回true，否则返回false
 */
bool test_bytecode_backend_all(void)
{
    printf("开始运行字节码后端测试...\n");
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
    
    if (!test_configuration_management()) {
        printf("❌ test_configuration_management 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_configuration_management 通过\n");
    }
    
    if (!test_interface_creation()) {
        printf("❌ test_interface_creation 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_interface_creation 通过\n");
    }
    
    if (!test_bytecode_program_management()) {
        printf("❌ test_bytecode_program_management 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_bytecode_program_management 通过\n");
    }
    
    if (!test_serialization()) {
        printf("❌ test_serialization 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_serialization 通过\n");
    }
    
    if (!test_validation()) {
        printf("❌ test_validation 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_validation 通过\n");
    }
    
    if (!test_formatting()) {
        printf("❌ test_formatting 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_formatting 通过\n");
    }
    
    if (!test_optimization()) {
        printf("❌ test_optimization 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_optimization 通过\n");
    }
    
    printf("\n==============================\n");
    if (all_passed) {
        printf("✅ 所有字节码后端测试通过！\n");
    } else {
        printf("❌ 有字节码后端测试失败\n");
    }
    
    return all_passed;
}

/* 主函数在test_runner_main.c中定义，这里只提供测试函数 */
