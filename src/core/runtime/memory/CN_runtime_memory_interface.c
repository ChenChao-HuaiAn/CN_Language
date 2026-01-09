/**
 * @file CN_runtime_memory_interface.c
 * @brief 运行时内存管理接口实现
 * 
 * 实现运行时内存管理抽象接口的工厂函数和桩实现。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_runtime_memory_interface.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 运行时内存管理接口私有数据结构
// ============================================================================

/**
 * @brief 运行时内存管理接口私有数据
 */
typedef struct Stru_RuntimeMemoryData_t
{
    size_t heap_size;          ///< 堆大小
    bool use_gc;               ///< 是否启用垃圾回收
    size_t total_allocated;    ///< 总分配字节数
    size_t total_freed;        ///< 总释放字节数
    size_t object_count;       ///< 对象数量
    size_t gc_count;           ///< 垃圾回收次数
} Stru_RuntimeMemoryData_t;

// ============================================================================
// 运行时内存管理接口函数实现
// ============================================================================

/**
 * @brief 运行时内存管理初始化函数
 */
static bool runtime_memory_initialize(Stru_RuntimeMemoryInterface_t* memory_interface,
                                     size_t heap_size, bool use_gc)
{
    if (memory_interface == NULL || memory_interface->private_data != NULL)
    {
        return false;
    }
    
    Stru_RuntimeMemoryData_t* data = 
        (Stru_RuntimeMemoryData_t*)malloc(sizeof(Stru_RuntimeMemoryData_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->heap_size = heap_size;
    data->use_gc = use_gc;
    data->total_allocated = 0;
    data->total_freed = 0;
    data->object_count = 0;
    data->gc_count = 0;
    
    memory_interface->private_data = data;
    return true;
}

/**
 * @brief 分配运行时对象内存
 */
static void* runtime_memory_allocate_object(Stru_RuntimeMemoryInterface_t* memory_interface,
                                           size_t size, uint32_t type_tag)
{
    (void)type_tag; // 暂时未使用
    
    if (memory_interface == NULL || memory_interface->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_RuntimeMemoryData_t* data = (Stru_RuntimeMemoryData_t*)memory_interface->private_data;
    
    // 简单实现：使用系统malloc
    void* object = malloc(size);
    if (object != NULL)
    {
        data->total_allocated += size;
        data->object_count++;
    }
    
    return object;
}

/**
 * @brief 分配运行时数组内存
 */
static void* runtime_memory_allocate_array(Stru_RuntimeMemoryInterface_t* memory_interface,
                                          size_t element_size, size_t element_count,
                                          uint32_t type_tag)
{
    (void)type_tag; // 暂时未使用
    
    if (memory_interface == NULL || memory_interface->private_data == NULL)
    {
        return NULL;
    }
    
    size_t total_size = element_size * element_count;
    return runtime_memory_allocate_object(memory_interface, total_size, 0);
}

/**
 * @brief 释放对象内存
 */
static void runtime_memory_free_object(Stru_RuntimeMemoryInterface_t* memory_interface,
                                      void* object)
{
    if (memory_interface == NULL || memory_interface->private_data == NULL || object == NULL)
    {
        return;
    }
    
    Stru_RuntimeMemoryData_t* data = (Stru_RuntimeMemoryData_t*)memory_interface->private_data;
    
    // 获取对象大小（简单实现：假设对象有头部信息）
    size_t object_size = sizeof(void*); // 简化处理
    
    free(object);
    data->total_freed += object_size;
    if (data->object_count > 0)
    {
        data->object_count--;
    }
}

/**
 * @brief 标记对象为活动状态
 */
static void runtime_memory_mark_object(Stru_RuntimeMemoryInterface_t* memory_interface,
                                      void* object)
{
    (void)memory_interface;
    (void)object;
    // 桩实现：垃圾回收未实现
}

/**
 * @brief 执行垃圾回收
 */
static size_t runtime_memory_collect_garbage(Stru_RuntimeMemoryInterface_t* memory_interface)
{
    if (memory_interface == NULL || memory_interface->private_data == NULL)
    {
        return 0;
    }
    
    Stru_RuntimeMemoryData_t* data = (Stru_RuntimeMemoryData_t*)memory_interface->private_data;
    data->gc_count++;
    
    // 桩实现：不实际回收内存
    return 0;
}

/**
 * @brief 获取内存使用统计
 */
static void runtime_memory_get_stats(Stru_RuntimeMemoryInterface_t* memory_interface,
                                    size_t* total_heap, size_t* used_heap,
                                    size_t* object_count, size_t* gc_count)
{
    if (memory_interface == NULL || memory_interface->private_data == NULL)
    {
        return;
    }
    
    Stru_RuntimeMemoryData_t* data = (Stru_RuntimeMemoryData_t*)memory_interface->private_data;
    
    if (total_heap != NULL) *total_heap = data->heap_size;
    if (used_heap != NULL) *used_heap = data->total_allocated - data->total_freed;
    if (object_count != NULL) *object_count = data->object_count;
    if (gc_count != NULL) *gc_count = data->gc_count;
}

/**
 * @brief 重置内存管理器状态
 */
static void runtime_memory_reset(Stru_RuntimeMemoryInterface_t* memory_interface)
{
    if (memory_interface == NULL || memory_interface->private_data == NULL)
    {
        return;
    }
    
    Stru_RuntimeMemoryData_t* data = (Stru_RuntimeMemoryData_t*)memory_interface->private_data;
    
    data->total_allocated = 0;
    data->total_freed = 0;
    data->object_count = 0;
    data->gc_count = 0;
}

/**
 * @brief 销毁运行时内存管理器
 */
static void runtime_memory_destroy(Stru_RuntimeMemoryInterface_t* memory_interface)
{
    if (memory_interface == NULL)
    {
        return;
    }
    
    if (memory_interface->private_data != NULL)
    {
        free(memory_interface->private_data);
        memory_interface->private_data = NULL;
    }
}

// ============================================================================
// 工厂函数实现
// ============================================================================

/**
 * @brief 创建运行时内存管理接口实例
 */
Stru_RuntimeMemoryInterface_t* F_create_runtime_memory_interface(void)
{
    Stru_RuntimeMemoryInterface_t* interface = 
        (Stru_RuntimeMemoryInterface_t*)malloc(sizeof(Stru_RuntimeMemoryInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = runtime_memory_initialize;
    interface->allocate_object = runtime_memory_allocate_object;
    interface->allocate_array = runtime_memory_allocate_array;
    interface->free_object = runtime_memory_free_object;
    interface->mark_object = runtime_memory_mark_object;
    interface->collect_garbage = runtime_memory_collect_garbage;
    interface->get_memory_stats = runtime_memory_get_stats;
    interface->reset = runtime_memory_reset;
    interface->destroy = runtime_memory_destroy;
    interface->private_data = NULL;
    
    return interface;
}
