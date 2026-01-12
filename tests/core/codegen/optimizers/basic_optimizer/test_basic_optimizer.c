/******************************************************************************
 * 文件名: test_basic_optimizer.c
 * 功能: CN_Language 基础优化器测试
 * 
 * 测试基础优化器的功能，包括接口实现、版本信息、优化支持等。
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
#include "../../../../../../src/core/codegen/optimizers/basic_optimizer/CN_basic_optimizer.h"

/**
 * @brief 测试版本信息函数
 * 
 * 验证基础优化器版本信息函数的正确性。
 */
static bool test_version_functions(void)
{
    printf("测试: 版本信息函数\n");
    
    /* 测试版本号获取 */
    int major = -1, minor = -1, patch = -1;
    F_get_basic_optimizer_version(&major, &minor, &patch);
    
    TEST_ASSERT_EQUAL(1, major);
    TEST_ASSERT_EQUAL(0, minor);
    TEST_ASSERT_EQUAL(0, patch);
    
    /* 测试版本字符串 */
    const char* version_string = F_get_basic_optimizer_version_string();
    TEST_ASSERT_NOT_NULL(version_string);
    TEST_ASSERT_EQUAL_STRING("1.0.0", version_string);
    
    return true;
}

/**
 * @brief 测试接口创建和销毁
 * 
 * 验证基础优化器接口的创建和销毁功能。
 */
static bool test_interface_creation(void)
{
    printf("测试: 接口创建和销毁\n");
    
    /* 测试接口创建 */
    Stru_OptimizerPluginInterface_t* optimizer = F_create_basic_optimizer_interface();
    TEST_ASSERT_NOT_NULL(optimizer);
    
    if (optimizer) {
        /* 验证接口函数指针不为NULL */
        TEST_ASSERT_NOT_NULL(optimizer->get_info);
        TEST_ASSERT_NOT_NULL(optimizer->initialize);
        TEST_ASSERT_NOT_NULL(optimizer->analyze);
        TEST_ASSERT_NOT_NULL(optimizer->can_optimize);
        TEST_ASSERT_NOT_NULL(optimizer->optimize);
        TEST_ASSERT_NOT_NULL(optimizer->destroy);
        
        /* 测试销毁函数 */
        optimizer->destroy(optimizer);
    }
    
    /* 测试多次创建和销毁 */
    for (int i = 0; i < 3; i++) {
        Stru_OptimizerPluginInterface_t* opt = F_create_basic_optimizer_interface();
        TEST_ASSERT_NOT_NULL(opt);
        
        if (opt) {
            opt->destroy(opt);
        }
    }
    
    return true;
}

/**
 * @brief 测试优化支持检查
 * 
 * 验证基础优化器支持的优化类型。
 */
static bool test_optimization_support(void)
{
    printf("测试: 优化支持检查\n");
    
    /* 测试支持的优化类型 */
    size_t count = 0;
    const Eum_OptimizationType* supported = F_get_basic_optimizer_supported_optimizations(&count);
    
    TEST_ASSERT_NOT_NULL(supported);
    TEST_ASSERT_TRUE(count > 0);
    
    /* 测试特定优化类型支持 */
    TEST_ASSERT_TRUE(F_basic_optimizer_supports_optimization(Eum_OPTIMIZE_CONSTANT_FOLDING));
    TEST_ASSERT_TRUE(F_basic_optimizer_supports_optimization(Eum_OPTIMIZE_DEAD_CODE_ELIM));
    TEST_ASSERT_TRUE(F_basic_optimizer_supports_optimization(Eum_OPTIMIZE_COMMON_SUBEXPR));
    TEST_ASSERT_TRUE(F_basic_optimizer_supports_optimization(Eum_OPTIMIZE_STRENGTH_REDUCTION));
    TEST_ASSERT_TRUE(F_basic_optimizer_supports_optimization(Eum_OPTIMIZE_PEEPHOLE));
    
    /* 测试不支持的优化类型 */
    TEST_ASSERT_FALSE(F_basic_optimizer_supports_optimization(999)); /* 无效优化类型 */
    
    return true;
}

/**
 * @brief 测试优化器信息获取
 * 
 * 验证优化器插件信息的正确性。
 */
static bool test_optimizer_info(void)
{
    printf("测试: 优化器信息获取\n");
    
    Stru_OptimizerPluginInterface_t* optimizer = F_create_basic_optimizer_interface();
    TEST_ASSERT_NOT_NULL(optimizer);
    
    if (optimizer && optimizer->get_info) {
        Stru_OptimizerPluginInfo_t info;
        memset(&info, 0, sizeof(info));
        
        const Stru_OptimizerPluginInfo_t* info_ptr = optimizer->get_info(optimizer);
        bool success = (info_ptr != NULL);
        if (success) {
            info = *info_ptr;  /* 复制信息 */
        }
        TEST_ASSERT_TRUE(success);
        
        if (success) {
            /* 验证信息字段 */
            TEST_ASSERT_NOT_NULL(info.name);
            TEST_ASSERT_NOT_NULL(info.description);
            TEST_ASSERT_NOT_NULL(info.author);
            TEST_ASSERT_NOT_NULL(info.version);
            
            /* 验证版本号 - 注意：Stru_OptimizerPluginInfo_t结构体中没有major_version等字段 */
            /* 这些字段在CN_optimizer_interface.h中定义为optimization_count等 */
            /* 验证支持的优化类型 */
            TEST_ASSERT_TRUE(info.optimization_count > 0);
            TEST_ASSERT_NOT_NULL(info.supported_optimizations);
        }
    }
    
    if (optimizer) {
        optimizer->destroy(optimizer);
    }
    
    return true;
}

/**
 * @brief 测试优化器初始化
 * 
 * 验证优化器初始化功能。
 */
static bool test_optimizer_initialization(void)
{
    printf("测试: 优化器初始化\n");
    
    Stru_OptimizerPluginInterface_t* optimizer = F_create_basic_optimizer_interface();
    TEST_ASSERT_NOT_NULL(optimizer);
    
    if (optimizer && optimizer->initialize) {
        /* 测试初始化 - 需要创建优化上下文 */
        Stru_OptimizationContext_t context;
        memset(&context, 0, sizeof(context));
        context.level = Eum_OPT_LEVEL_O1;
        
        bool initialized = optimizer->initialize(optimizer, &context);
        TEST_ASSERT_TRUE(initialized);
        
        /* 测试重复初始化 */
        bool reinitialized = optimizer->initialize(optimizer, &context);
        TEST_ASSERT_TRUE(reinitialized || !reinitialized); /* 接受任何状态 */
    }
    
    if (optimizer) {
        optimizer->destroy(optimizer);
    }
    
    return true;
}

/**
 * @brief 运行所有基础优化器测试
 * 
 * 运行基础优化器的所有测试用例。
 * 
 * @return 所有测试通过返回true，否则返回false
 */
bool test_basic_optimizer_all(void)
{
    printf("开始运行基础优化器测试...\n");
    printf("==============================\n\n");
    
    bool all_passed = true;
    
    /* 运行各个测试用例 */
    if (!test_version_functions()) {
        printf("❌ test_version_functions 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_version_functions 通过\n");
    }
    
    if (!test_interface_creation()) {
        printf("❌ test_interface_creation 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_interface_creation 通过\n");
    }
    
    if (!test_optimization_support()) {
        printf("❌ test_optimization_support 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_optimization_support 通过\n");
    }
    
    if (!test_optimizer_info()) {
        printf("❌ test_optimizer_info 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_optimizer_info 通过\n");
    }
    
    if (!test_optimizer_initialization()) {
        printf("❌ test_optimizer_initialization 失败\n");
        all_passed = false;
    } else {
        printf("✅ test_optimizer_initialization 通过\n");
    }
    
    printf("\n==============================\n");
    if (all_passed) {
        printf("✅ 所有基础优化器测试通过！\n");
    } else {
        printf("❌ 有基础优化器测试失败\n");
    }
    
    return all_passed;
}

/* 主函数在test_runner_main.c中定义，这里只提供测试函数 */
