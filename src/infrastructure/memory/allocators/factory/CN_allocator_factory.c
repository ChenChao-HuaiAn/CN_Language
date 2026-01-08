/**
 * @file CN_allocator_factory.c
 * @brief 分配器工厂实现
 * 
 * 实现了分配器工厂接口和相关函数，提供统一的分配器创建和管理功能。
 * 遵循项目编码标准和架构规范，支持工厂模式和依赖注入。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_allocator_factory.h"
#include "../system/CN_system_allocator.h"
#include "../debug/CN_debug_allocator.h"
#include "../pool/CN_pool_allocator.h"
#include "../region/CN_region_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/**
 * @brief 分配器工厂私有数据结构
 */
typedef struct Stru_AllocatorFactoryData_t {
    size_t total_created;          ///< 总创建的分配器数
    size_t active_allocators;      ///< 活动分配器数
    size_t memory_usage;           ///< 总内存使用量（字节）
    
    // 分配器注册表
    struct {
        const char* type_name;
        Stru_MemoryAllocatorInterface_t* (*creator)(const Stru_AllocatorConfig_t* config, ...);
    } registered_types[16];        ///< 注册的分配器类型
    
    size_t registered_count;       ///< 已注册的类型数量
} Stru_AllocatorFactoryData_t;


// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 创建系统分配器（内部实现）
 */
static Stru_MemoryAllocatorInterface_t* internal_create_system_allocator(
    Stru_AllocatorFactory_t* factory,
    const Stru_AllocatorConfig_t* config)
{
    (void)config;  // 暂时忽略配置
    
    Stru_MemoryAllocatorInterface_t* allocator = F_create_system_allocator();
    if (allocator != NULL) {
        // 更新统计信息
        Stru_AllocatorFactoryData_t* data = (Stru_AllocatorFactoryData_t*)factory->private_data;
        if (data != NULL) {
            data->total_created++;
            data->active_allocators++;
        }
    }
    
    return allocator;
}

/**
 * @brief 创建调试分配器（内部实现）
 */
static Stru_MemoryAllocatorInterface_t* internal_create_debug_allocator(
    Stru_AllocatorFactory_t* factory,
    const Stru_AllocatorConfig_t* config,
    Stru_MemoryAllocatorInterface_t* parent_allocator)
{
    (void)config;  // 暂时忽略配置
    
    // 如果父分配器为NULL，使用系统分配器
    if (parent_allocator == NULL) {
        parent_allocator = F_create_system_allocator();
        if (parent_allocator == NULL) {
            return NULL;
        }
    }
    
    Stru_MemoryAllocatorInterface_t* allocator = F_create_debug_allocator(parent_allocator);
    if (allocator != NULL) {
        // 更新统计信息
        Stru_AllocatorFactoryData_t* data = (Stru_AllocatorFactoryData_t*)factory->private_data;
        if (data != NULL) {
            data->total_created++;
            data->active_allocators++;
        }
    }
    
    return allocator;
}

/**
 * @brief 创建对象池分配器（内部实现）
 */
static Stru_MemoryAllocatorInterface_t* internal_create_pool_allocator(
    Stru_AllocatorFactory_t* factory,
    const Stru_AllocatorConfig_t* config,
    size_t object_size,
    size_t pool_size,
    Stru_MemoryAllocatorInterface_t* parent_allocator)
{
    (void)config;  // 暂时忽略配置
    
    // 如果父分配器为NULL，使用系统分配器
    if (parent_allocator == NULL) {
        parent_allocator = F_create_system_allocator();
        if (parent_allocator == NULL) {
            return NULL;
        }
    }
    
    Stru_MemoryAllocatorInterface_t* allocator = F_create_pool_allocator(object_size, pool_size, parent_allocator);
    if (allocator != NULL) {
        // 更新统计信息
        Stru_AllocatorFactoryData_t* data = (Stru_AllocatorFactoryData_t*)factory->private_data;
        if (data != NULL) {
            data->total_created++;
            data->active_allocators++;
        }
    }
    
    return allocator;
}

/**
 * @brief 创建区域分配器（内部实现）
 */
static Stru_MemoryAllocatorInterface_t* internal_create_region_allocator(
    Stru_AllocatorFactory_t* factory,
    const Stru_AllocatorConfig_t* config,
    size_t region_size,
    Stru_MemoryAllocatorInterface_t* parent_allocator)
{
    (void)config;  // 暂时忽略配置
    
    // 如果父分配器为NULL，使用系统分配器
    if (parent_allocator == NULL) {
        parent_allocator = F_create_system_allocator();
        if (parent_allocator == NULL) {
            return NULL;
        }
    }
    
    Stru_MemoryAllocatorInterface_t* allocator = F_create_region_allocator(region_size, parent_allocator);
    if (allocator != NULL) {
        // 更新统计信息
        Stru_AllocatorFactoryData_t* data = (Stru_AllocatorFactoryData_t*)factory->private_data;
        if (data != NULL) {
            data->total_created++;
            data->active_allocators++;
        }
    }
    
    return allocator;
}

/**
 * @brief 根据名称创建分配器（内部实现）
 */
static Stru_MemoryAllocatorInterface_t* internal_create_allocator_by_name(
    Stru_AllocatorFactory_t* factory,
    const char* allocator_type,
    const Stru_AllocatorConfig_t* config,
    ...)
{
    va_list args;
    va_start(args, config);
    Stru_MemoryAllocatorInterface_t* result = 
        factory->create_allocator_by_name_va(factory, allocator_type, config, args);
    va_end(args);
    return result;
}

/**
 * @brief 根据名称创建分配器（va_list版本，内部实现）
 */
static Stru_MemoryAllocatorInterface_t* internal_create_allocator_by_name_va(
    Stru_AllocatorFactory_t* factory,
    const char* allocator_type,
    const Stru_AllocatorConfig_t* config,
    va_list args)
{
    if (allocator_type == NULL) {
        return NULL;
    }
    
    // 获取工厂私有数据
    Stru_AllocatorFactoryData_t* data = (Stru_AllocatorFactoryData_t*)factory->private_data;
    if (data == NULL) {
        return NULL;
    }
    
    // 检查注册的类型
    for (size_t i = 0; i < data->registered_count; i++) {
        if (strcmp(data->registered_types[i].type_name, allocator_type) == 0) {
            // 如果creator为NULL，使用内置实现
            if (data->registered_types[i].creator == NULL) {
                // 使用内置实现
                break;
            }
            
            // 调用注册的创建函数
            va_list args_copy;
            va_copy(args_copy, args);
            Stru_MemoryAllocatorInterface_t* allocator = 
                data->registered_types[i].creator(config, args_copy);
            va_end(args_copy);
            
            if (allocator != NULL) {
                data->total_created++;
                data->active_allocators++;
            }
            
            return allocator;
        }
    }
    
    // 内置类型
    if (strcmp(allocator_type, "system") == 0) {
        return factory->create_system_allocator(factory, config);
    }
    else if (strcmp(allocator_type, "debug") == 0) {
        Stru_MemoryAllocatorInterface_t* parent = va_arg(args, Stru_MemoryAllocatorInterface_t*);
        return factory->create_debug_allocator(factory, config, parent);
    }
    else if (strcmp(allocator_type, "pool") == 0) {
        size_t object_size = va_arg(args, size_t);
        size_t pool_size = va_arg(args, size_t);
        Stru_MemoryAllocatorInterface_t* parent = va_arg(args, Stru_MemoryAllocatorInterface_t*);
        return factory->create_pool_allocator(factory, config, object_size, pool_size, parent);
    }
    else if (strcmp(allocator_type, "region") == 0) {
        size_t region_size = va_arg(args, size_t);
        Stru_MemoryAllocatorInterface_t* parent = va_arg(args, Stru_MemoryAllocatorInterface_t*);
        return factory->create_region_allocator(factory, config, region_size, parent);
    }
    
    return NULL;
}

/**
 * @brief 销毁分配器（内部实现）
 */
static void internal_destroy_allocator(
    Stru_AllocatorFactory_t* factory,
    Stru_MemoryAllocatorInterface_t* allocator)
{
    if (allocator == NULL) {
        return;
    }
    
    // 获取工厂私有数据
    Stru_AllocatorFactoryData_t* data = (Stru_AllocatorFactoryData_t*)factory->private_data;
    if (data != NULL) {
        data->active_allocators--;
    }
    
    // 调用分配器的清理函数
    if (allocator->cleanup != NULL) {
        allocator->cleanup(allocator);
    }
}

/**
 * @brief 获取工厂统计信息（内部实现）
 */
static void internal_get_statistics(
    Stru_AllocatorFactory_t* factory,
    size_t* total_created,
    size_t* active_allocators,
    size_t* memory_usage)
{
    Stru_AllocatorFactoryData_t* data = (Stru_AllocatorFactoryData_t*)factory->private_data;
    if (data == NULL) {
        if (total_created) *total_created = 0;
        if (active_allocators) *active_allocators = 0;
        if (memory_usage) *memory_usage = 0;
        return;
    }
    
    if (total_created) *total_created = data->total_created;
    if (active_allocators) *active_allocators = data->active_allocators;
    if (memory_usage) *memory_usage = data->memory_usage;
}

/**
 * @brief 清理工厂资源（内部实现）
 */
static void internal_cleanup(Stru_AllocatorFactory_t* factory)
{
    // 注意：这个实现不跟踪所有创建的分配器，所以无法自动清理
    // 更完整的实现需要维护分配器列表
    (void)factory;
}

/**
 * @brief 注册分配器类型（内部实现）
 */
static bool internal_register_allocator_type(
    Stru_AllocatorFactory_t* factory,
    const char* type_name,
    Stru_MemoryAllocatorInterface_t* (*creator)(const Stru_AllocatorConfig_t* config, ...))
{
    if (type_name == NULL || creator == NULL) {
        return false;
    }
    
    Stru_AllocatorFactoryData_t* data = (Stru_AllocatorFactoryData_t*)factory->private_data;
    if (data == NULL) {
        return false;
    }
    
    // 检查是否已满
    if (data->registered_count >= sizeof(data->registered_types) / sizeof(data->registered_types[0])) {
        return false;
    }
    
    // 检查是否已存在
    for (size_t i = 0; i < data->registered_count; i++) {
        if (strcmp(data->registered_types[i].type_name, type_name) == 0) {
            return false;  // 已存在
        }
    }
    
    // 注册新类型
    data->registered_types[data->registered_count].type_name = type_name;
    data->registered_types[data->registered_count].creator = creator;
    data->registered_count++;
    
    return true;
}

// ============================================================================
// 公共接口函数
// ============================================================================

/**
 * @brief 创建分配器工厂
 */
Stru_AllocatorFactory_t* F_create_allocator_factory(void)
{
    Stru_AllocatorFactory_t* factory = (Stru_AllocatorFactory_t*)malloc(sizeof(Stru_AllocatorFactory_t));
    if (factory == NULL) {
        return NULL;
    }
    
    // 分配私有数据
    Stru_AllocatorFactoryData_t* data = (Stru_AllocatorFactoryData_t*)malloc(sizeof(Stru_AllocatorFactoryData_t));
    if (data == NULL) {
        free(factory);
        return NULL;
    }
    
    // 初始化私有数据
    memset(data, 0, sizeof(Stru_AllocatorFactoryData_t));
    
    // 注册内置类型
    data->registered_types[0].type_name = "system";
    data->registered_types[0].creator = NULL;  // 使用内置实现
    data->registered_types[1].type_name = "debug";
    data->registered_types[1].creator = NULL;  // 使用内置实现
    data->registered_types[2].type_name = "pool";
    data->registered_types[2].creator = NULL;  // 使用内置实现
    data->registered_types[3].type_name = "region";
    data->registered_types[3].creator = NULL;  // 使用内置实现
    data->registered_count = 4;
    
    // 设置接口函数
    factory->create_system_allocator = internal_create_system_allocator;
    factory->create_debug_allocator = internal_create_debug_allocator;
    factory->create_pool_allocator = internal_create_pool_allocator;
    factory->create_region_allocator = internal_create_region_allocator;
    factory->create_allocator_by_name = internal_create_allocator_by_name;
    factory->create_allocator_by_name_va = internal_create_allocator_by_name_va;
    factory->destroy_allocator = internal_destroy_allocator;
    factory->get_statistics = internal_get_statistics;
    factory->cleanup = internal_cleanup;
    factory->register_allocator_type = internal_register_allocator_type;
    factory->private_data = data;
    
    return factory;
}

/**
 * @brief 销毁分配器工厂
 */
void F_destroy_allocator_factory(Stru_AllocatorFactory_t* factory)
{
    if (factory == NULL) {
        return;
    }
    
    // 清理工厂资源
    if (factory->cleanup != NULL) {
        factory->cleanup(factory);
    }
    
    // 释放私有数据
    if (factory->private_data != NULL) {
        free(factory->private_data);
    }
    
    free(factory);
}

/**
 * @brief 获取默认分配器工厂
 */
Stru_AllocatorFactory_t* F_get_default_allocator_factory(void)
{
    static Stru_AllocatorFactory_t* default_factory = NULL;
    static bool initialized = false;
    
    if (!initialized) {
        default_factory = F_create_allocator_factory();
        initialized = true;
    }
    
    return default_factory;
}

/**
 * @brief 创建系统分配器（快捷函数）
 */
Stru_MemoryAllocatorInterface_t* F_create_system_allocator_from_factory(
    const Stru_AllocatorConfig_t* config)
{
    Stru_AllocatorFactory_t* factory = F_get_default_allocator_factory();
    if (factory == NULL) {
        return NULL;
    }
    
    return factory->create_system_allocator(factory, config);
}

/**
 * @brief 创建调试分配器（快捷函数）
 */
Stru_MemoryAllocatorInterface_t* F_create_debug_allocator_from_factory(
    const Stru_AllocatorConfig_t* config,
    Stru_MemoryAllocatorInterface_t* parent_allocator)
{
    Stru_AllocatorFactory_t* factory = F_get_default_allocator_factory();
    if (factory == NULL) {
        return NULL;
    }
    
    return factory->create_debug_allocator(factory, config, parent_allocator);
}

/**
 * @brief 创建对象池分配器（快捷函数）
 */
Stru_MemoryAllocatorInterface_t* F_create_pool_allocator_from_factory(
    const Stru_AllocatorConfig_t* config,
    size_t object_size,
    size_t pool_size,
    Stru_MemoryAllocatorInterface_t* parent_allocator)
{
    Stru_AllocatorFactory_t* factory = F_get_default_allocator_factory();
    if (factory == NULL) {
        return NULL;
    }
    
    return factory->create_pool_allocator(factory, config, object_size, pool_size, parent_allocator);
}

/**
 * @brief 创建区域分配器（快捷函数）
 */
Stru_MemoryAllocatorInterface_t* F_create_region_allocator_from_factory(
    const Stru_AllocatorConfig_t* config,
    size_t region_size,
    Stru_MemoryAllocatorInterface_t* parent_allocator)
{
    Stru_AllocatorFactory_t* factory = F_get_default_allocator_factory();
    if (factory == NULL) {
        return NULL;
    }
    
    return factory->create_region_allocator(factory, config, region_size, parent_allocator);
}

/**
 * @brief 根据名称创建分配器（快捷函数）
 */
Stru_MemoryAllocatorInterface_t* F_create_allocator_by_name_from_factory(
    const char* allocator_type,
    const Stru_AllocatorConfig_t* config,
    ...)
{
    Stru_AllocatorFactory_t* factory = F_get_default_allocator_factory();
    if (factory == NULL) {
        return NULL;
    }
    
    va_list args;
    va_start(args, config);
    Stru_MemoryAllocatorInterface_t* result = 
        factory->create_allocator_by_name_va(factory, allocator_type, config, args);
    va_end(args);
    
    return result;
}
