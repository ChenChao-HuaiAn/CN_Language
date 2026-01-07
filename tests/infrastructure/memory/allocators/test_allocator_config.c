/**
 * @file test_allocator_config.c
 * @brief 分配器配置测试
 * 
 * 测试分配器配置结构的相关函数，包括创建、复制、验证和释放配置。
 * 遵循项目测试框架规范。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_allocators.h"
#include "../../../../src/infrastructure/memory/allocators/factory/CN_allocator_config.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief 测试创建默认配置
 */
static bool test_create_default_config(void)
{
    printf("  测试: 创建默认配置\n");
    
    Stru_AllocatorConfig_t* config = F_create_default_allocator_config();
    if (config == NULL) {
        printf("    ❌ 创建默认配置失败\n");
        return false;
    }
    
    // 验证默认值
    bool passed = true;
    
    if (config->name != NULL) {
        printf("    ❌ 默认名称应为NULL\n");
        passed = false;
    }
    
    if (config->thread_safe != false) {
        printf("    ❌ 默认thread_safe应为false\n");
        passed = false;
    }
    
    if (config->enable_statistics != false) {
        printf("    ❌ 默认enable_statistics应为false\n");
        passed = false;
    }
    
    if (config->enable_debugging != false) {
        printf("    ❌ 默认enable_debugging应为false\n");
        passed = false;
    }
    
    if (config->alignment != 0) {
        printf("    ❌ 默认alignment应为0\n");
        passed = false;
    }
    
    if (config->max_allocation_size != 0) {
        printf("    ❌ 默认max_allocation_size应为0\n");
        passed = false;
    }
    
    if (config->allocation_failure_callback != NULL) {
        printf("    ❌ 默认allocation_failure_callback应为NULL\n");
        passed = false;
    }
    
    // 验证配置
    if (!F_validate_allocator_config(config)) {
        printf("    ❌ 默认配置验证失败\n");
        passed = false;
    }
    
    F_free_allocator_config(config);
    
    if (passed) {
        printf("    ✅ 创建默认配置测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 测试配置复制
 */
static bool test_copy_config(void)
{
    printf("  测试: 配置复制\n");
    
    // 创建源配置
    Stru_AllocatorConfig_t* src = F_create_default_allocator_config();
    if (src == NULL) {
        printf("    ❌ 创建源配置失败\n");
        return false;
    }
    
    // 设置一些值
    src->thread_safe = true;
    src->enable_statistics = true;
    src->alignment = 16;
    src->max_allocation_size = 1024 * 1024;  // 1MB
    
    // 复制配置
    Stru_AllocatorConfig_t* dst = F_copy_allocator_config(src);
    if (dst == NULL) {
        printf("    ❌ 复制配置失败\n");
        F_free_allocator_config(src);
        return false;
    }
    
    // 验证复制结果
    bool passed = true;
    
    if (dst->thread_safe != src->thread_safe) {
        printf("    ❌ thread_safe复制错误\n");
        passed = false;
    }
    
    if (dst->enable_statistics != src->enable_statistics) {
        printf("    ❌ enable_statistics复制错误\n");
        passed = false;
    }
    
    if (dst->alignment != src->alignment) {
        printf("    ❌ alignment复制错误\n");
        passed = false;
    }
    
    if (dst->max_allocation_size != src->max_allocation_size) {
        printf("    ❌ max_allocation_size复制错误\n");
        passed = false;
    }
    
    // 验证配置
    if (!F_validate_allocator_config(dst)) {
        printf("    ❌ 复制配置验证失败\n");
        passed = false;
    }
    
    F_free_allocator_config(src);
    F_free_allocator_config(dst);
    
    if (passed) {
        printf("    ✅ 配置复制测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 测试设置分配器名称
 */
static bool test_set_allocator_name(void)
{
    printf("  测试: 设置分配器名称\n");
    
    Stru_AllocatorConfig_t* config = F_create_default_allocator_config();
    if (config == NULL) {
        printf("    ❌ 创建配置失败\n");
        return false;
    }
    
    bool passed = true;
    
    // 测试设置名称
    const char* test_name = "TestAllocator";
    if (!F_set_allocator_name(config, test_name)) {
        printf("    ❌ 设置分配器名称失败\n");
        passed = false;
    }
    
    // 验证名称
    if (config->name == NULL || strcmp(config->name, test_name) != 0) {
        printf("    ❌ 分配器名称设置错误\n");
        passed = false;
    }
    
    // 测试设置NULL名称
    if (F_set_allocator_name(config, NULL)) {
        printf("    ❌ 设置NULL名称应该失败\n");
        passed = false;
    }
    
    // 测试设置空字符串
    if (!F_set_allocator_name(config, "")) {
        printf("    ❌ 设置空字符串名称失败\n");
        passed = false;
    }
    
    F_free_allocator_config(config);
    
    if (passed) {
        printf("    ✅ 设置分配器名称测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 测试设置自定义数据
 */
static bool test_set_custom_data(void)
{
    printf("  测试: 设置自定义数据\n");
    
    Stru_AllocatorConfig_t* config = F_create_default_allocator_config();
    if (config == NULL) {
        printf("    ❌ 创建配置失败\n");
        return false;
    }
    
    bool passed = true;
    
    // 测试设置自定义数据
    const char test_data[] = "Test custom data";
    size_t test_data_size = sizeof(test_data);
    
    if (!F_set_custom_data(config, test_data, test_data_size)) {
        printf("    ❌ 设置自定义数据失败\n");
        passed = false;
    }
    
    // 验证自定义数据
    if (config->custom_data == NULL || config->custom_data_size != test_data_size) {
        printf("    ❌ 自定义数据设置错误\n");
        passed = false;
    }
    
    if (memcmp(config->custom_data, test_data, test_data_size) != 0) {
        printf("    ❌ 自定义数据内容错误\n");
        passed = false;
    }
    
    // 测试设置NULL数据
    if (F_set_custom_data(config, NULL, 10)) {
        printf("    ❌ 设置NULL数据应该失败\n");
        passed = false;
    }
    
    // 测试设置零大小数据
    if (F_set_custom_data(config, test_data, 0)) {
        printf("    ❌ 设置零大小数据应该失败\n");
        passed = false;
    }
    
    F_free_allocator_config(config);
    
    if (passed) {
        printf("    ✅ 设置自定义数据测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 测试配置验证
 */
static bool test_config_validation(void)
{
    printf("  测试: 配置验证\n");
    
    bool passed = true;
    
    // 测试NULL配置
    if (F_validate_allocator_config(NULL)) {
        printf("    ❌ NULL配置验证应该失败\n");
        passed = false;
    }
    
    // 测试无效对齐
    Stru_AllocatorConfig_t* config1 = F_create_default_allocator_config();
    if (config1 != NULL) {
        config1->alignment = 3;  // 不是2的幂
        if (F_validate_allocator_config(config1)) {
            printf("    ❌ 无效对齐配置验证应该失败\n");
            passed = false;
        }
        F_free_allocator_config(config1);
    }
    
    // 测试过大对齐
    Stru_AllocatorConfig_t* config2 = F_create_default_allocator_config();
    if (config2 != NULL) {
        config2->alignment = 8192;  // 超过4KB
        if (F_validate_allocator_config(config2)) {
            printf("    ❌ 过大对齐配置验证应该失败\n");
            passed = false;
        }
        F_free_allocator_config(config2);
    }
    
    // 测试有效对齐
    Stru_AllocatorConfig_t* config3 = F_create_default_allocator_config();
    if (config3 != NULL) {
        config3->alignment = 16;  // 2的幂
        if (!F_validate_allocator_config(config3)) {
            printf("    ❌ 有效对齐配置验证失败\n");
            passed = false;
        }
        F_free_allocator_config(config3);
    }
    
    // 测试不一致的自定义数据
    Stru_AllocatorConfig_t* config4 = F_create_default_allocator_config();
    if (config4 != NULL) {
        config4->custom_data = (void*)0x1234;  // 非NULL指针
        config4->custom_data_size = 0;  // 零大小
        if (F_validate_allocator_config(config4)) {
            printf("    ❌ 不一致自定义数据配置验证应该失败\n");
            passed = false;
        }
        F_free_allocator_config(config4);
    }
    
    if (passed) {
        printf("    ✅ 配置验证测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 测试配置摘要
 */
static bool test_config_summary(void)
{
    printf("  测试: 配置摘要\n");
    
    Stru_AllocatorConfig_t* config = F_create_default_allocator_config();
    if (config == NULL) {
        printf("    ❌ 创建配置失败\n");
        return false;
    }
    
    bool passed = true;
    
    // 设置一些值
    config->thread_safe = true;
    config->enable_statistics = true;
    config->alignment = 8;
    config->max_allocation_size = 4096;
    
    // 获取摘要
    char buffer[256];
    if (!F_get_config_summary(config, buffer, sizeof(buffer))) {
        printf("    ❌ 获取配置摘要失败\n");
        passed = false;
    } else {
        printf("    配置摘要: %s\n", buffer);
        
        // 检查摘要是否包含关键信息
        if (strstr(buffer, "thread_safe=yes") == NULL) {
            printf("    ❌ 摘要缺少thread_safe信息\n");
            passed = false;
        }
        
        if (strstr(buffer, "alignment=8") == NULL) {
            printf("    ❌ 摘要缺少alignment信息\n");
            passed = false;
        }
    }
    
    // 测试缓冲区太小
    char small_buffer[10];
    if (F_get_config_summary(config, small_buffer, sizeof(small_buffer))) {
        printf("    ❌ 小缓冲区应该失败\n");
        passed = false;
    }
    
    // 测试NULL参数
    if (F_get_config_summary(NULL, buffer, sizeof(buffer))) {
        printf("    ❌ NULL配置应该失败\n");
        passed = false;
    }
    
    F_free_allocator_config(config);
    
    if (passed) {
        printf("    ✅ 配置摘要测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 运行分配器配置的所有测试
 * 
 * @return bool 所有测试通过返回true，否则返回false
 */
bool test_allocator_config_all(void)
{
    printf("\n");
    printf("分配器配置测试套件\n");
    printf("==================\n");
    printf("\n");
    
    TestCaseCounter counter;
    test_case_init(&counter);
    
    // 运行所有测试用例
    test_case_begin("创建默认配置", &counter);
    test_case_end(test_create_default_config(), &counter);
    
    test_case_begin("配置复制", &counter);
    test_case_end(test_copy_config(), &counter);
    
    test_case_begin("设置分配器名称", &counter);
    test_case_end(test_set_allocator_name(), &counter);
    
    test_case_begin("设置自定义数据", &counter);
    test_case_end(test_set_custom_data(), &counter);
    
    test_case_begin("配置验证", &counter);
    test_case_end(test_config_validation(), &counter);
    
    test_case_begin("配置摘要", &counter);
    test_case_end(test_config_summary(), &counter);
    
    // 打印测试总结
    printf("\n");
    test_case_summary(&counter);
    
    return (counter.failed_tests == 0);
}
