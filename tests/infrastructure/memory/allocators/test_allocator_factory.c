/**
 * @file test_allocator_factory.c
 * @brief 分配器工厂测试
 * 
 * 测试分配器工厂的相关函数，包括工厂创建、分配器创建、统计信息和清理功能。
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
#include "../../../../src/infrastructure/memory/allocators/factory/CN_allocator_factory.h"
#include "../../../../src/infrastructure/memory/allocators/factory/CN_allocator_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief 测试工厂创建和销毁
 */
static bool test_factory_creation(void)
{
    printf("  测试: 工厂创建和销毁\n");
    
    // 测试创建工厂
    Stru_AllocatorFactory_t* factory = F_create_allocator_factory();
    if (factory == NULL) {
        printf("    ❌ 创建工厂失败\n");
        return false;
    }
    
    // 验证工厂接口不为NULL
    bool passed = true;
    if (factory->create_system_allocator == NULL) {
        printf("    ❌ create_system_allocator接口为NULL\n");
        passed = false;
    }
    
    if (factory->create_debug_allocator == NULL) {
        printf("    ❌ create_debug_allocator接口为NULL\n");
        passed = false;
    }
    
    if (factory->create_pool_allocator == NULL) {
        printf("    ❌ create_pool_allocator接口为NULL\n");
        passed = false;
    }
    
    if (factory->create_region_allocator == NULL) {
        printf("    ❌ create_region_allocator接口为NULL\n");
        passed = false;
    }
    
    if (factory->create_allocator_by_name == NULL) {
        printf("    ❌ create_allocator_by_name接口为NULL\n");
        passed = false;
    }
    
    if (factory->destroy_allocator == NULL) {
        printf("    ❌ destroy_allocator接口为NULL\n");
        passed = false;
    }
    
    if (factory->get_statistics == NULL) {
        printf("    ❌ get_statistics接口为NULL\n");
        passed = false;
    }
    
    if (factory->cleanup == NULL) {
        printf("    ❌ cleanup接口为NULL\n");
        passed = false;
    }
    
    // 测试销毁工厂
    F_destroy_allocator_factory(factory);
    
    if (passed) {
        printf("    ✅ 工厂创建和销毁测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 测试默认工厂
 */
static bool test_default_factory(void)
{
    printf("  测试: 默认工厂\n");
    
    // 第一次获取默认工厂
    Stru_AllocatorFactory_t* factory1 = F_get_default_allocator_factory();
    if (factory1 == NULL) {
        printf("    ❌ 获取默认工厂失败\n");
        return false;
    }
    
    // 第二次获取默认工厂，应该是同一个实例
    Stru_AllocatorFactory_t* factory2 = F_get_default_allocator_factory();
    if (factory2 == NULL) {
        printf("    ❌ 第二次获取默认工厂失败\n");
        return false;
    }
    
    bool passed = true;
    if (factory1 != factory2) {
        printf("    ❌ 默认工厂不是单例\n");
        passed = false;
    }
    
    // 验证默认工厂接口
    if (factory1->create_system_allocator == NULL) {
        printf("    ❌ 默认工厂接口不完整\n");
        passed = false;
    }
    
    if (passed) {
        printf("    ✅ 默认工厂测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 测试创建系统分配器
 */
static bool test_create_system_allocator(void)
{
    printf("  测试: 创建系统分配器\n");
    
    Stru_AllocatorFactory_t* factory = F_create_allocator_factory();
    if (factory == NULL) {
        printf("    ❌ 创建工厂失败\n");
        return false;
    }
    
    bool passed = true;
    
    // 测试使用NULL配置创建系统分配器
    Stru_MemoryAllocatorInterface_t* allocator1 = 
        factory->create_system_allocator(factory, NULL);
    if (allocator1 == NULL) {
        printf("    ❌ 使用NULL配置创建系统分配器失败\n");
        passed = false;
    } else {
        // 验证分配器接口
        if (allocator1->allocate == NULL || allocator1->deallocate == NULL) {
            printf("    ❌ 系统分配器接口不完整\n");
            passed = false;
        }
        
        // 测试分配和释放
        void* ptr = allocator1->allocate(allocator1, 100, 0);  // 对齐为0
        if (ptr == NULL) {
            printf("    ❌ 系统分配器分配内存失败\n");
            passed = false;
        } else {
            allocator1->deallocate(allocator1, ptr);
        }
        
        factory->destroy_allocator(factory, allocator1);
    }
    
    // 测试使用自定义配置创建系统分配器
    Stru_AllocatorConfig_t* config = F_create_default_allocator_config();
    if (config != NULL) {
        config->thread_safe = true;
        config->enable_statistics = true;
        
        Stru_MemoryAllocatorInterface_t* allocator2 = 
            factory->create_system_allocator(factory, config);
        if (allocator2 == NULL) {
            printf("    ❌ 使用自定义配置创建系统分配器失败\n");
            passed = false;
        } else {
            factory->destroy_allocator(factory, allocator2);
        }
        
        F_free_allocator_config(config);
    }
    
    F_destroy_allocator_factory(factory);
    
    if (passed) {
        printf("    ✅ 创建系统分配器测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 测试创建调试分配器
 */
static bool test_create_debug_allocator(void)
{
    printf("  测试: 创建调试分配器\n");
    
    Stru_AllocatorFactory_t* factory = F_create_allocator_factory();
    if (factory == NULL) {
        printf("    ❌ 创建工厂失败\n");
        return false;
    }
    
    bool passed = true;
    
    // 创建父分配器（系统分配器）
    Stru_MemoryAllocatorInterface_t* parent_allocator = 
        factory->create_system_allocator(factory, NULL);
    if (parent_allocator == NULL) {
        printf("    ❌ 创建父分配器失败\n");
        F_destroy_allocator_factory(factory);
        return false;
    }
    
    // 测试使用NULL配置创建调试分配器
    Stru_MemoryAllocatorInterface_t* debug_allocator1 = 
        factory->create_debug_allocator(factory, NULL, parent_allocator);
    if (debug_allocator1 == NULL) {
        printf("    ❌ 使用NULL配置创建调试分配器失败\n");
        passed = false;
    } else {
        // 验证分配器接口
        if (debug_allocator1->allocate == NULL || debug_allocator1->deallocate == NULL) {
            printf("    ❌ 调试分配器接口不完整\n");
            passed = false;
        }
        
        // 测试分配和释放
        void* ptr = debug_allocator1->allocate(debug_allocator1, 100, 0);  // 对齐为0
        if (ptr == NULL) {
            printf("    ❌ 调试分配器分配内存失败\n");
            passed = false;
        } else {
            debug_allocator1->deallocate(debug_allocator1, ptr);
        }
        
        factory->destroy_allocator(factory, debug_allocator1);
    }
    
    // 测试使用NULL父分配器（应该使用系统分配器）
    Stru_MemoryAllocatorInterface_t* debug_allocator2 = 
        factory->create_debug_allocator(factory, NULL, NULL);
    if (debug_allocator2 == NULL) {
        printf("    ❌ 使用NULL父分配器创建调试分配器失败\n");
        passed = false;
    } else {
        factory->destroy_allocator(factory, debug_allocator2);
    }
    
    // 销毁父分配器
    factory->destroy_allocator(factory, parent_allocator);
    F_destroy_allocator_factory(factory);
    
    if (passed) {
        printf("    ✅ 创建调试分配器测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 测试创建对象池分配器
 */
static bool test_create_pool_allocator(void)
{
    printf("  测试: 创建对象池分配器\n");
    
    Stru_AllocatorFactory_t* factory = F_create_allocator_factory();
    if (factory == NULL) {
        printf("    ❌ 创建工厂失败\n");
        return false;
    }
    
    bool passed = true;
    
    // 创建父分配器
    Stru_MemoryAllocatorInterface_t* parent_allocator = 
        factory->create_system_allocator(factory, NULL);
    if (parent_allocator == NULL) {
        printf("    ❌ 创建父分配器失败\n");
        F_destroy_allocator_factory(factory);
        return false;
    }
    
    // 测试创建对象池分配器
    size_t object_size = 64;  // 64字节对象
    size_t pool_size = 10;    // 10个对象的池
    
    Stru_MemoryAllocatorInterface_t* pool_allocator = 
        factory->create_pool_allocator(factory, NULL, object_size, pool_size, parent_allocator);
    if (pool_allocator == NULL) {
        printf("    ❌ 创建对象池分配器失败\n");
        passed = false;
    } else {
        // 验证分配器接口
        if (pool_allocator->allocate == NULL || pool_allocator->deallocate == NULL) {
            printf("    ❌ 对象池分配器接口不完整\n");
            passed = false;
        }
        
        // 测试分配多个对象（使用动态数组）
        void** objects = (void**)malloc(pool_size * sizeof(void*));
        if (objects != NULL) {
            for (size_t i = 0; i < pool_size; i++) {
                objects[i] = pool_allocator->allocate(pool_allocator, object_size, 0);  // 对齐为0
                if (objects[i] == NULL) {
                    printf("    ❌ 对象池分配器分配对象 %zu 失败\n", i);
                    passed = false;
                    break;
                }
            }
            
            // 释放所有对象
            for (size_t i = 0; i < pool_size; i++) {
                if (objects[i] != NULL) {
                    pool_allocator->deallocate(pool_allocator, objects[i]);
                }
            }
            
            free(objects);
        }
        
        factory->destroy_allocator(factory, pool_allocator);
    }
    
    // 测试无效参数
    Stru_MemoryAllocatorInterface_t* invalid_allocator = 
        factory->create_pool_allocator(factory, NULL, 0, 10, parent_allocator);  // object_size=0
    if (invalid_allocator != NULL) {
        printf("    ❌ 使用零对象大小创建对象池分配器应该失败\n");
        passed = false;
        factory->destroy_allocator(factory, invalid_allocator);
    }
    
    // 销毁父分配器
    factory->destroy_allocator(factory, parent_allocator);
    F_destroy_allocator_factory(factory);
    
    if (passed) {
        printf("    ✅ 创建对象池分配器测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 测试创建区域分配器
 */
static bool test_create_region_allocator(void)
{
    printf("  测试: 创建区域分配器\n");
    
    Stru_AllocatorFactory_t* factory = F_create_allocator_factory();
    if (factory == NULL) {
        printf("    ❌ 创建工厂失败\n");
        return false;
    }
    
    bool passed = true;
    
    // 创建父分配器
    Stru_MemoryAllocatorInterface_t* parent_allocator = 
        factory->create_system_allocator(factory, NULL);
    if (parent_allocator == NULL) {
        printf("    ❌ 创建父分配器失败\n");
        F_destroy_allocator_factory(factory);
        return false;
    }
    
    // 测试创建区域分配器
    size_t region_size = 1024 * 1024;  // 1MB区域
    
    Stru_MemoryAllocatorInterface_t* region_allocator = 
        factory->create_region_allocator(factory, NULL, region_size, parent_allocator);
    if (region_allocator == NULL) {
        printf("    ❌ 创建区域分配器失败\n");
        passed = false;
    } else {
        // 验证分配器接口
        if (region_allocator->allocate == NULL || region_allocator->deallocate == NULL) {
            printf("    ❌ 区域分配器接口不完整\n");
            passed = false;
        }
        
        // 测试分配内存
        void* ptr1 = region_allocator->allocate(region_allocator, 100, 0);  // 对齐为0
        void* ptr2 = region_allocator->allocate(region_allocator, 200, 0);  // 对齐为0
        void* ptr3 = region_allocator->allocate(region_allocator, 300, 0);  // 对齐为0
        
        if (ptr1 == NULL || ptr2 == NULL || ptr3 == NULL) {
            printf("    ❌ 区域分配器分配内存失败\n");
            passed = false;
        }
        
        // 释放内存（区域分配器可能不支持单独释放）
        if (ptr1 != NULL) region_allocator->deallocate(region_allocator, ptr1);
        if (ptr2 != NULL) region_allocator->deallocate(region_allocator, ptr2);
        if (ptr3 != NULL) region_allocator->deallocate(region_allocator, ptr3);
        
        factory->destroy_allocator(factory, region_allocator);
    }
    
    // 测试无效参数
    Stru_MemoryAllocatorInterface_t* invalid_allocator = 
        factory->create_region_allocator(factory, NULL, 0, parent_allocator);  // region_size=0
    if (invalid_allocator != NULL) {
        printf("    ❌ 使用零区域大小创建区域分配器应该失败\n");
        passed = false;
        factory->destroy_allocator(factory, invalid_allocator);
    }
    
    // 销毁父分配器
    factory->destroy_allocator(factory, parent_allocator);
    F_destroy_allocator_factory(factory);
    
    if (passed) {
        printf("    ✅ 创建区域分配器测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 测试根据名称创建分配器
 */
static bool test_create_allocator_by_name(void)
{
    printf("  测试: 根据名称创建分配器\n");
    
    Stru_AllocatorFactory_t* factory = F_create_allocator_factory();
    if (factory == NULL) {
        printf("    ❌ 创建工厂失败\n");
        return false;
    }
    
    bool passed = true;
    
    // 测试创建系统分配器
    Stru_MemoryAllocatorInterface_t* system_allocator = 
        factory->create_allocator_by_name(factory, "system", NULL);
    if (system_allocator == NULL) {
        printf("    ❌ 根据名称'system'创建分配器失败\n");
        passed = false;
    } else {
        factory->destroy_allocator(factory, system_allocator);
    }
    
    // 测试创建调试分配器（需要额外参数）
    Stru_MemoryAllocatorInterface_t* debug_allocator = 
        factory->create_allocator_by_name(factory, "debug", NULL, NULL);
    if (debug_allocator == NULL) {
        printf("    ❌ 根据名称'debug'创建分配器失败\n");
        passed = false;
    } else {
        factory->destroy_allocator(factory, debug_allocator);
    }
    
    // 测试创建对象池分配器（需要额外参数）
    Stru_MemoryAllocatorInterface_t* pool_allocator = 
        factory->create_allocator_by_name(factory, "pool", NULL, (size_t)64, (size_t)10, NULL);
    if (pool_allocator == NULL) {
        printf("    ❌ 根据名称'pool'创建分配器失败\n");
        passed = false;
    } else {
        factory->destroy_allocator(factory, pool_allocator);
    }
    
    // 测试创建区域分配器（需要额外参数）
    Stru_MemoryAllocatorInterface_t* region_allocator = 
        factory->create_allocator_by_name(factory, "region", NULL, (size_t)1024, NULL);
    if (region_allocator == NULL) {
        printf("    ❌ 根据名称'region'创建分配器失败\n");
        passed = false;
    } else {
        factory->destroy_allocator(factory, region_allocator);
    }
    
    // 测试无效名称
    Stru_MemoryAllocatorInterface_t* invalid_allocator = 
        factory->create_allocator_by_name(factory, "invalid", NULL);
    if (invalid_allocator != NULL) {
        printf("    ❌ 使用无效名称创建分配器应该失败\n");
        passed = false;
        factory->destroy_allocator(factory, invalid_allocator);
    }
    
    // 测试NULL名称
    Stru_MemoryAllocatorInterface_t* null_name_allocator = 
        factory->create_allocator_by_name(factory, NULL, NULL);
    if (null_name_allocator != NULL) {
        printf("    ❌ 使用NULL名称创建分配器应该失败\n");
        passed = false;
        factory->destroy_allocator(factory, null_name_allocator);
    }
    
    F_destroy_allocator_factory(factory);
    
    if (passed) {
        printf("    ✅ 根据名称创建分配器测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 测试工厂统计信息
 */
static bool test_factory_statistics(void)
{
    printf("  测试: 工厂统计信息\n");
    
    Stru_AllocatorFactory_t* factory = F_create_allocator_factory();
    if (factory == NULL) {
        printf("    ❌ 创建工厂失败\n");
        return false;
    }
    
    bool passed = true;
    
    // 获取初始统计信息
    size_t total_created = 0, active_allocators = 0, memory_usage = 0;
    factory->get_statistics(factory, &total_created, &active_allocators, &memory_usage);
    
    // 创建几个分配器
    Stru_MemoryAllocatorInterface_t* allocator1 = 
        factory->create_system_allocator(factory, NULL);
    Stru_MemoryAllocatorInterface_t* allocator2 = 
        factory->create_system_allocator(factory, NULL);
    
    if (allocator1 == NULL || allocator2 == NULL) {
        printf("    ❌ 创建分配器失败\n");
        passed = false;
    } else {
        // 获取更新后的统计信息
        size_t new_total_created = 0, new_active_allocators = 0, new_memory_usage = 0;
        factory->get_statistics(factory, &new_total_created, &new_active_allocators, &new_memory_usage);
        
        // 验证统计信息
        if (new_total_created < total_created + 2) {
            printf("    ❌ 总创建数统计错误\n");
            passed = false;
        }
        
        if (new_active_allocators < active_allocators + 2) {
            printf("    ❌ 活动分配器数统计错误\n");
            passed = false;
        }
        
        // 销毁一个分配器
        factory->destroy_allocator(factory, allocator1);
        
        // 再次获取统计信息
        factory->get_statistics(factory, &new_total_created, &new_active_allocators, &new_memory_usage);
        
        if (new_active_allocators < active_allocators + 1) {
            printf("    ❌ 销毁后活动分配器数统计错误\n");
            passed = false;
        }
        
        // 销毁另一个分配器
        factory->destroy_allocator(factory, allocator2);
    }
    
    F_destroy_allocator_factory(factory);
    
    if (passed) {
        printf("    ✅ 工厂统计信息测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 测试工厂清理
 */
static bool test_factory_cleanup(void)
{
    printf("  测试: 工厂清理\n");
    
    Stru_AllocatorFactory_t* factory = F_create_allocator_factory();
    if (factory == NULL) {
        printf("    ❌ 创建工厂失败\n");
        return false;
    }
    
    bool passed = true;
    
    // 创建几个分配器
    Stru_MemoryAllocatorInterface_t* allocator1 = 
        factory->create_system_allocator(factory, NULL);
    Stru_MemoryAllocatorInterface_t* allocator2 = 
        factory->create_debug_allocator(factory, NULL, NULL);
    
    if (allocator1 == NULL || allocator2 == NULL) {
        printf("    ❌ 创建分配器失败\n");
        passed = false;
    } else {
        // 调用清理函数
        factory->cleanup(factory);
        
        // 注意：清理函数的具体行为取决于实现
        // 这里我们只是验证它不会崩溃
        
        // 销毁分配器（如果清理没有销毁它们）
        factory->destroy_allocator(factory, allocator1);
        factory->destroy_allocator(factory, allocator2);
    }
    
    F_destroy_allocator_factory(factory);
    
    if (passed) {
        printf("    ✅ 工厂清理测试通过\n");
    }
    
    return passed;
}

/**
 * @brief 运行分配器工厂的所有测试
 * 
 * @return bool 所有测试通过返回true，否则返回false
 */
bool test_allocator_factory_all(void)
{
    printf("\n");
    printf("分配器工厂测试套件\n");
    printf("==================\n");
    printf("\n");
    
    // 初始化测试计数器
    TestCaseCounter counter;
    test_case_init(&counter);
    
    // 运行所有测试用例
    test_case_begin("工厂创建和销毁", &counter);
    test_case_end(test_factory_creation(), &counter);
    
    test_case_begin("默认工厂", &counter);
    test_case_end(test_default_factory(), &counter);
    
    test_case_begin("创建系统分配器", &counter);
    test_case_end(test_create_system_allocator(), &counter);
    
    test_case_begin("创建调试分配器", &counter);
    test_case_end(test_create_debug_allocator(), &counter);
    
    test_case_begin("创建对象池分配器", &counter);
    test_case_end(test_create_pool_allocator(), &counter);
    
    test_case_begin("创建区域分配器", &counter);
    test_case_end(test_create_region_allocator(), &counter);
    
    test_case_begin("根据名称创建分配器", &counter);
    test_case_end(test_create_allocator_by_name(), &counter);
    
    test_case_begin("工厂统计信息", &counter);
    test_case_end(test_factory_statistics(), &counter);
    
    test_case_begin("工厂清理", &counter);
    test_case_end(test_factory_cleanup(), &counter);
    
    // 打印测试总结
    printf("\n");
    test_case_summary(&counter);
    
    return (counter.failed_tests == 0);
}
