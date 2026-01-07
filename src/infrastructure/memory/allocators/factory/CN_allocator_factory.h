/**
 * @file CN_allocator_factory.h
 * @brief 分配器工厂接口定义
 * 
 * 定义了分配器工厂接口和相关函数，提供统一的分配器创建和管理功能。
 * 遵循项目编码标准和架构规范，支持工厂模式和依赖注入。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_ALLOCATOR_FACTORY_H
#define CN_ALLOCATOR_FACTORY_H

#include "CN_allocator_config.h"
#include "../../CN_memory_interface.h"
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 分配器工厂接口
 * 
 * 提供统一的分配器创建和管理功能，支持多种分配器类型。
 */
typedef struct Stru_AllocatorFactory_t Stru_AllocatorFactory_t;

/**
 * @brief 分配器工厂接口定义
 */
struct Stru_AllocatorFactory_t {
    /**
     * @brief 创建系统分配器
     * 
     * 创建基于标准C库的系统分配器。
     * 
     * @param factory 工厂实例
     * @param config 分配器配置（可为NULL，使用默认配置）
     * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针，失败返回NULL
     */
    Stru_MemoryAllocatorInterface_t* (*create_system_allocator)(
        Stru_AllocatorFactory_t* factory,
        const Stru_AllocatorConfig_t* config);
    
    /**
     * @brief 创建调试分配器
     * 
     * 创建用于调试的内存分配器，支持内存错误检测。
     * 
     * @param factory 工厂实例
     * @param config 分配器配置（可为NULL，使用默认配置）
     * @param parent_allocator 父分配器（可为NULL，使用系统分配器）
     * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针，失败返回NULL
     */
    Stru_MemoryAllocatorInterface_t* (*create_debug_allocator)(
        Stru_AllocatorFactory_t* factory,
        const Stru_AllocatorConfig_t* config,
        Stru_MemoryAllocatorInterface_t* parent_allocator);
    
    /**
     * @brief 创建对象池分配器
     * 
     * 创建固定大小对象的高效分配器。
     * 
     * @param factory 工厂实例
     * @param config 分配器配置（可为NULL，使用默认配置）
     * @param object_size 对象大小（字节）
     * @param pool_size 池大小（对象数量）
     * @param parent_allocator 父分配器（可为NULL，使用系统分配器）
     * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针，失败返回NULL
     */
    Stru_MemoryAllocatorInterface_t* (*create_pool_allocator)(
        Stru_AllocatorFactory_t* factory,
        const Stru_AllocatorConfig_t* config,
        size_t object_size,
        size_t pool_size,
        Stru_MemoryAllocatorInterface_t* parent_allocator);
    
    /**
     * @brief 创建区域分配器
     * 
     * 创建区域式内存分配器，支持一次性分配和释放。
     * 
     * @param factory 工厂实例
     * @param config 分配器配置（可为NULL，使用默认配置）
     * @param region_size 区域大小（字节）
     * @param parent_allocator 父分配器（可为NULL，使用系统分配器）
     * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针，失败返回NULL
     */
    Stru_MemoryAllocatorInterface_t* (*create_region_allocator)(
        Stru_AllocatorFactory_t* factory,
        const Stru_AllocatorConfig_t* config,
        size_t region_size,
        Stru_MemoryAllocatorInterface_t* parent_allocator);
    
    /**
     * @brief 根据名称创建分配器
     * 
     * 根据分配器类型名称创建分配器，支持可变参数。
     * 
     * @param factory 工厂实例
     * @param allocator_type 分配器类型名称（"system", "debug", "pool", "region"）
     * @param config 分配器配置（可为NULL，使用默认配置）
     * @param ... 可变参数（根据分配器类型不同）
     * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针，失败返回NULL
     */
    Stru_MemoryAllocatorInterface_t* (*create_allocator_by_name)(
        Stru_AllocatorFactory_t* factory,
        const char* allocator_type,
        const Stru_AllocatorConfig_t* config,
        ...);
    
    /**
     * @brief 根据名称创建分配器（va_list版本）
     * 
     * create_allocator_by_name的va_list版本，用于可变参数处理。
     * 
     * @param factory 工厂实例
     * @param allocator_type 分配器类型名称
     * @param config 分配器配置（可为NULL）
     * @param args 可变参数列表
     * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针，失败返回NULL
     */
    Stru_MemoryAllocatorInterface_t* (*create_allocator_by_name_va)(
        Stru_AllocatorFactory_t* factory,
        const char* allocator_type,
        const Stru_AllocatorConfig_t* config,
        va_list args);
    
    /**
     * @brief 销毁分配器
     * 
     * 销毁由工厂创建的分配器，释放相关资源。
     * 
     * @param factory 工厂实例
     * @param allocator 要销毁的分配器
     */
    void (*destroy_allocator)(
        Stru_AllocatorFactory_t* factory,
        Stru_MemoryAllocatorInterface_t* allocator);
    
    /**
     * @brief 获取工厂统计信息
     * 
     * 获取工厂的统计信息，包括创建的分配器数量等。
     * 
     * @param factory 工厂实例
     * @param total_created 输出：总创建的分配器数
     * @param active_allocators 输出：活动分配器数
     * @param memory_usage 输出：总内存使用量（字节）
     */
    void (*get_statistics)(
        Stru_AllocatorFactory_t* factory,
        size_t* total_created,
        size_t* active_allocators,
        size_t* memory_usage);
    
    /**
     * @brief 清理工厂资源
     * 
     * 清理工厂占用的资源，销毁所有由工厂创建的分配器。
     * 
     * @param factory 工厂实例
     */
    void (*cleanup)(Stru_AllocatorFactory_t* factory);
    
    /**
     * @brief 注册分配器类型
     * 
     * 注册新的分配器类型，支持扩展。
     * 
     * @param factory 工厂实例
     * @param type_name 分配器类型名称
     * @param creator 分配器创建函数
     * @return bool 注册成功返回true，失败返回false
     */
    bool (*register_allocator_type)(
        Stru_AllocatorFactory_t* factory,
        const char* type_name,
        Stru_MemoryAllocatorInterface_t* (*creator)(const Stru_AllocatorConfig_t* config, ...));
    
    /**
     * @brief 私有数据指针
     * 
     * 工厂内部使用的私有数据，对外部透明。
     */
    void* private_data;
};

/**
 * @brief 创建分配器工厂
 * 
 * 创建并初始化一个新的分配器工厂实例。
 * 
 * @return Stru_AllocatorFactory_t* 工厂实例，失败返回NULL
 */
Stru_AllocatorFactory_t* F_create_allocator_factory(void);

/**
 * @brief 销毁分配器工厂
 * 
 * 销毁分配器工厂实例，释放所有相关资源。
 * 
 * @param factory 要销毁的工厂实例
 */
void F_destroy_allocator_factory(Stru_AllocatorFactory_t* factory);

/**
 * @brief 获取默认分配器工厂
 * 
 * 获取全局默认的分配器工厂实例。
 * 第一次调用时会创建工厂，后续调用返回同一实例。
 * 
 * @return Stru_AllocatorFactory_t* 默认工厂实例
 */
Stru_AllocatorFactory_t* F_get_default_allocator_factory(void);

/**
 * @brief 创建系统分配器（快捷函数）
 * 
 * 使用默认工厂创建系统分配器的快捷函数。
 * 
 * @param config 分配器配置（可为NULL）
 * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针，失败返回NULL
 */
Stru_MemoryAllocatorInterface_t* F_create_system_allocator_from_factory(
    const Stru_AllocatorConfig_t* config);

/**
 * @brief 创建调试分配器（快捷函数）
 * 
 * 使用默认工厂创建调试分配器的快捷函数。
 * 
 * @param config 分配器配置（可为NULL）
 * @param parent_allocator 父分配器（可为NULL）
 * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针，失败返回NULL
 */
Stru_MemoryAllocatorInterface_t* F_create_debug_allocator_from_factory(
    const Stru_AllocatorConfig_t* config,
    Stru_MemoryAllocatorInterface_t* parent_allocator);

/**
 * @brief 创建对象池分配器（快捷函数）
 * 
 * 使用默认工厂创建对象池分配器的快捷函数。
 * 
 * @param config 分配器配置（可为NULL）
 * @param object_size 对象大小
 * @param pool_size 池大小
 * @param parent_allocator 父分配器（可为NULL）
 * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针，失败返回NULL
 */
Stru_MemoryAllocatorInterface_t* F_create_pool_allocator_from_factory(
    const Stru_AllocatorConfig_t* config,
    size_t object_size,
    size_t pool_size,
    Stru_MemoryAllocatorInterface_t* parent_allocator);

/**
 * @brief 创建区域分配器（快捷函数）
 * 
 * 使用默认工厂创建区域分配器的快捷函数。
 * 
 * @param config 分配器配置（可为NULL）
 * @param region_size 区域大小
 * @param parent_allocator 父分配器（可为NULL）
 * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针，失败返回NULL
 */
Stru_MemoryAllocatorInterface_t* F_create_region_allocator_from_factory(
    const Stru_AllocatorConfig_t* config,
    size_t region_size,
    Stru_MemoryAllocatorInterface_t* parent_allocator);

/**
 * @brief 根据名称创建分配器（快捷函数）
 * 
 * 使用默认工厂根据名称创建分配器的快捷函数。
 * 
 * @param allocator_type 分配器类型名称
 * @param config 分配器配置（可为NULL）
 * @param ... 可变参数
 * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针，失败返回NULL
 */
Stru_MemoryAllocatorInterface_t* F_create_allocator_by_name_from_factory(
    const char* allocator_type,
    const Stru_AllocatorConfig_t* config,
    ...);

#ifdef __cplusplus
}
#endif

#endif /* CN_ALLOCATOR_FACTORY_H */
