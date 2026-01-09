/**
 * @file CN_runtime_memory_interface.h
 * @brief 运行时内存管理接口定义
 * 
 * 本文件定义了CN_Language项目的运行时内存管理接口，包括内存分配、
 * 垃圾回收、内存统计等功能。遵循SOLID设计原则和分层架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_RUNTIME_MEMORY_INTERFACE_H
#define CN_RUNTIME_MEMORY_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_RuntimeMemoryInterface_t Stru_RuntimeMemoryInterface_t;

// ============================================================================
// 运行时内存管理接口
// ============================================================================

/**
 * @brief 运行时内存管理接口
 * 
 * 专门为运行时系统设计的内存管理接口，支持垃圾回收、内存池、
 * 对象生命周期管理等高级功能。
 */
struct Stru_RuntimeMemoryInterface_t
{
    /**
     * @brief 初始化运行时内存管理器
     * 
     * @param memory_interface 内存接口实例
     * @param heap_size 堆大小（字节）
     * @param use_gc 是否启用垃圾回收
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_RuntimeMemoryInterface_t* memory_interface, 
                      size_t heap_size, bool use_gc);
    
    /**
     * @brief 分配运行时对象内存
     * 
     * @param memory_interface 内存接口实例
     * @param size 对象大小（字节）
     * @param type_tag 对象类型标签
     * @return void* 分配的对象指针，失败返回NULL
     */
    void* (*allocate_object)(Stru_RuntimeMemoryInterface_t* memory_interface, 
                            size_t size, uint32_t type_tag);
    
    /**
     * @brief 分配运行时数组内存
     * 
     * @param memory_interface 内存接口实例
     * @param element_size 元素大小（字节）
     * @param element_count 元素数量
     * @param type_tag 元素类型标签
     * @return void* 分配的数组指针，失败返回NULL
     */
    void* (*allocate_array)(Stru_RuntimeMemoryInterface_t* memory_interface,
                           size_t element_size, size_t element_count,
                           uint32_t type_tag);
    
    /**
     * @brief 释放对象内存
     * 
     * @param memory_interface 内存接口实例
     * @param object 要释放的对象指针
     */
    void (*free_object)(Stru_RuntimeMemoryInterface_t* memory_interface,
                       void* object);
    
    /**
     * @brief 标记对象为活动状态（用于垃圾回收）
     * 
     * @param memory_interface 内存接口实例
     * @param object 要标记的对象指针
     */
    void (*mark_object)(Stru_RuntimeMemoryInterface_t* memory_interface, 
                       void* object);
    
    /**
     * @brief 执行垃圾回收
     * 
     * @param memory_interface 内存接口实例
     * @return size_t 回收的字节数
     */
    size_t (*collect_garbage)(Stru_RuntimeMemoryInterface_t* memory_interface);
    
    /**
     * @brief 获取内存使用统计
     * 
     * @param memory_interface 内存接口实例
     * @param total_heap 输出参数：堆总大小
     * @param used_heap 输出参数：已使用堆大小
     * @param object_count 输出参数：对象数量
     * @param gc_count 输出参数：垃圾回收次数
     */
    void (*get_memory_stats)(Stru_RuntimeMemoryInterface_t* memory_interface,
                            size_t* total_heap, size_t* used_heap,
                            size_t* object_count, size_t* gc_count);
    
    /**
     * @brief 重置内存管理器状态
     * 
     * @param memory_interface 内存接口实例
     */
    void (*reset)(Stru_RuntimeMemoryInterface_t* memory_interface);
    
    /**
     * @brief 销毁运行时内存管理器
     * 
     * @param memory_interface 内存接口实例
     */
    void (*destroy)(Stru_RuntimeMemoryInterface_t* memory_interface);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 工厂函数
// ============================================================================

/**
 * @brief 创建运行时内存管理接口实例
 * 
 * @return Stru_RuntimeMemoryInterface_t* 内存管理接口实例
 */
Stru_RuntimeMemoryInterface_t* F_create_runtime_memory_interface(void);

#endif // CN_RUNTIME_MEMORY_INTERFACE_H
