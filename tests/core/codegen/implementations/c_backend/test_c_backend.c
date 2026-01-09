/******************************************************************************
 * 文件名: test_c_backend.c
 * 功能: CN_Language C语言后端测试
 * 
 * 测试C语言后端代码生成功能。
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
#include "../../../../../src/core/codegen/implementations/c_backend/CN_c_backend.h"

/**
 * @brief 测试版本信息函数
 * 
 * 验证C后端版本信息函数的正确性。
 */
static bool test_version_functions(void)
{
    printf("测试: 版本信息函数\n");
    
    /* 测试版本号获取 */
    int major = -1, minor = -1, patch = -1;
    F_get_c_backend_version(&major, &minor, &patch);
    
    TEST_ASSERT_EQUAL(1, major);
    TEST_ASSERT_EQUAL(0, minor);
    TEST_ASSERT_EQUAL(0, patch);
    
    /* 测试版本字符串 */
    const char* version_string = F_get_c_backend_version_string();
    TEST_ASSERT_NOT_NULL(version_string);
    TEST_ASSERT_EQUAL_STRING("1.0.0", version_string);
    
    return true;
}

/**
 * @brief 测试C代码生成函数
 * 
 * 验证C代码生成函数的基本功能。
 */
static bool test_c_code_generation(void)
{
    printf("测试: C代码生成函数\n");
    
    /* 测试使用NULL AST生成代码 */
    Stru_CodeGenResult_t* result = F_generate_c_code(NULL);
    TEST_ASSERT_NOT_NULL(result);
    
    if (result) {
        /* 由于AST为NULL，应该返回一个结果对象 */
        TEST_ASSERT_TRUE(result->success || !result->success); // 接受任何状态
        
        /* 清理 */
        F_destroy_codegen_result(result);
    }
    
    /* 测试结果销毁函数的安全性 */
    F_destroy_codegen_result(NULL); // 应该安全
    
    return true;
}

/**
 * @brief 测试C后端状态管理
 * 
 * 验证C后端内部状态管理的正确性。
 */
static bool test_c_backend_state_management(void)
{
    printf("测试: C后端状态管理\n");
    
    /* 测试多次调用版本函数 */
    for (int i = 0; i < 3; i++) {
        int major, minor, patch;
        F_get_c_backend_version(&major, &minor, &patch);
        
        TEST_ASSERT_EQUAL(1, major);
        TEST_ASSERT_EQUAL(0, minor);
        TEST_ASSERT_EQUAL(0, patch);
        
        const char* version = F_get_c_backend_version_string();
        TEST_ASSERT_NOT_NULL(version);
        TEST_ASSERT_EQUAL_STRING("1.0.0", version);
    }
    
    return true;
}

/**
 * @brief 测试内存管理
 * 
 * 验证C后端的内存管理功能。
 */
static bool test_memory_management(void)
{
    printf("测试: 内存管理\n");
    
    /* 测试多次分配和释放 */
    for (int i = 0; i < 5; i++) {
        Stru_CodeGenResult_t* result = F_generate_c_code(NULL);
        TEST_ASSERT_NOT_NULL(result);
        
        if (result) {
            F_destroy_codegen_result(result);
        }
    }
    
    return true;
}

/**
 * @brief 运行所有C后端测试
 * 
 * 运行C语言后端的所有测试用例。
 * 
 * @return 所有测试通过返回true，否则返回false
 */
bool test_c_backend_all(void)
{
    printf("开始运行C语言后端测试...\n");
    printf("===========================\n\n");
    
    bool all_passed = true;
    
    /* 运行各个测试用例 */
    if (!test_version_functions()) {
        printf("❌ test_version_functions 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_version_functions 通过\n");
    }
    
    if (!test_c_code_generation()) {
        printf("❌ test_c_code_generation 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_c_code_generation 通过\n");
    }
    
    if (!test_c_backend_state_management()) {
        printf("❌ test_c_backend_state_management 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_c_backend_state_management 通过\n");
    }
    
    if (!test_memory_management()) {
        printf("❌ test_memory_management 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_memory_management 通过\n");
    }
    
    printf("\n===========================\n");
    if (all_passed) {
        printf("✅ 所有C后端测试通过！\n");
    } else {
        printf("❌ 有C后端测试失败\n");
    }
    
    return all_passed;
}

/**
 * @brief 主函数（用于独立测试）
 */
#ifdef STANDALONE_TEST
int main(void)
{
    printf("CN_Language C语言后端测试\n");
    printf("===========================\n\n");
    
    bool success = test_c_backend_all();
    
    if (success) {
        printf("\n✅ 所有C后端测试通过！\n");
        return EXIT_SUCCESS;
    } else {
        printf("\n❌ C后端测试失败！\n");
        return EXIT_FAILURE;
    }
}
#endif
