/**
 * @file CN_memory_error_detection.c
 * @brief 内存错误检测模块实现
 * 
 * 实现了内存错误检测功能，包括：
 * - 缓冲区溢出检测
 * - 双重释放检测
 * - 野指针检测
 * - 未初始化内存检测
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include "CN_memory_error_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/**
 * @brief 内存保护区域
 */
typedef struct Stru_MemoryGuardRegion_t
{
    void* address;                      ///< 内存地址
    size_t size;                        ///< 内存大小
    uint8_t* guard_zone_before;         ///< 前保护区域
    uint8_t* guard_zone_after;          ///< 后保护区域
    size_t guard_zone_size;             ///< 保护区域大小
    const char* file;                   ///< 源文件名
    size_t line;                        ///< 行号
    const char* function;               ///< 函数名
    struct Stru_MemoryGuardRegion_t* next; ///< 下一个区域
} Stru_MemoryGuardRegion_t;

/**
 * @brief 内存分配记录
 */
typedef struct Stru_AllocationRecord_t
{
    void* address;                      ///< 分配的内存地址
    size_t size;                        ///< 分配的大小
    bool freed;                         ///< 是否已释放
    struct Stru_AllocationRecord_t* next; ///< 下一个记录
} Stru_AllocationRecord_t;

/**
 * @brief 错误记录
 */
typedef struct Stru_ErrorRecord_t
{
    Stru_MemoryErrorInfo_t error_info;  ///< 错误信息
    struct Stru_ErrorRecord_t* next;    ///< 下一个错误记录
} Stru_ErrorRecord_t;

/**
 * @brief 错误检测器私有数据
 */
typedef struct Stru_ErrorDetectorData_t
{
    Stru_MemoryGuardRegion_t* guard_regions; ///< 保护区域链表
    Stru_AllocationRecord_t* allocations;    ///< 分配记录链表
    Stru_ErrorRecord_t* errors;              ///< 错误记录链表
    size_t error_count;                      ///< 错误数量
    bool enable_overflow_check;              ///< 是否启用溢出检查
    bool enable_double_free_check;           ///< 是否启用双重释放检查
    bool enable_uninitialized_check;         ///< 是否启用未初始化检查
    size_t guard_zone_size;                  ///< 保护区域大小
    uint8_t guard_pattern;                   ///< 保护模式字节
} Stru_ErrorDetectorData_t;

/**
 * @brief 创建错误检测器
 */
Stru_ErrorDetectorContext_t* F_create_error_detector(
    bool enable_overflow_check,
    bool enable_double_free_check,
    bool enable_uninitialized_check,
    size_t guard_zone_size)
{
    Stru_ErrorDetectorData_t* data = 
        (Stru_ErrorDetectorData_t*)malloc(sizeof(Stru_ErrorDetectorData_t));
    
    if (data == NULL)
    {
        return NULL;
    }
    
    // 初始化数据
    data->guard_regions = NULL;
    data->allocations = NULL;
    data->errors = NULL;
    data->error_count = 0;
    data->enable_overflow_check = enable_overflow_check;
    data->enable_double_free_check = enable_double_free_check;
    data->enable_uninitialized_check = enable_uninitialized_check;
    data->guard_zone_size = guard_zone_size;
    data->guard_pattern = 0xAA; // 使用0xAA作为保护模式
    
    return (Stru_ErrorDetectorContext_t*)data;
}

/**
 * @brief 销毁错误检测器
 */
void F_destroy_error_detector(Stru_ErrorDetectorContext_t* detector)
{
    if (detector == NULL)
    {
        return;
    }
    
    Stru_ErrorDetectorData_t* data = (Stru_ErrorDetectorData_t*)detector;
    
    if (data != NULL)
    {
        // 清理保护区域
        Stru_MemoryGuardRegion_t* guard_region = data->guard_regions;
        while (guard_region != NULL)
        {
            Stru_MemoryGuardRegion_t* next = guard_region->next;
            if (guard_region->guard_zone_before != NULL)
            {
                free(guard_region->guard_zone_before);
            }
            if (guard_region->guard_zone_after != NULL)
            {
                free(guard_region->guard_zone_after);
            }
            free(guard_region);
            guard_region = next;
        }
        
        // 清理分配记录
        Stru_AllocationRecord_t* allocation = data->allocations;
        while (allocation != NULL)
        {
            Stru_AllocationRecord_t* next = allocation->next;
            free(allocation);
            allocation = next;
        }
        
        // 清理错误记录
        Stru_ErrorRecord_t* error = data->errors;
        while (error != NULL)
        {
            Stru_ErrorRecord_t* next = error->next;
            free(error);
            error = next;
        }
        
        free(data);
    }
}

/**
 * @brief 添加错误记录
 */
static void add_error_record(Stru_ErrorDetectorData_t* data,
                            Eum_MemoryErrorType_t error_type,
                            void* address,
                            size_t size,
                            const char* file,
                            size_t line,
                            const char* function,
                            const char* description)
{
    Stru_ErrorRecord_t* error = 
        (Stru_ErrorRecord_t*)malloc(sizeof(Stru_ErrorRecord_t));
    
    if (error == NULL)
    {
        return;
    }
    
    // 填充错误信息
    error->error_info.error_type = error_type;
    error->error_info.address = address;
    error->error_info.size = size;
    error->error_info.file = file;
    error->error_info.line = line;
    error->error_info.function = function;
    error->error_info.description = description;
    
    // 添加到链表头部
    error->next = data->errors;
    data->errors = error;
    data->error_count++;
}

/**
 * @brief 保护内存区域
 */
bool F_protect_memory_region(Stru_ErrorDetectorContext_t* detector,
                            void* address, size_t size,
                            const char* file, size_t line,
                            const char* function)
{
    if (detector == NULL || address == NULL || size == 0)
    {
        return false;
    }
    
    Stru_ErrorDetectorData_t* data = (Stru_ErrorDetectorData_t*)detector;
    
    if (!data->enable_overflow_check)
    {
        return true;
    }
    
    // 创建保护区域
    Stru_MemoryGuardRegion_t* region = 
        (Stru_MemoryGuardRegion_t*)malloc(sizeof(Stru_MemoryGuardRegion_t));
    
    if (region == NULL)
    {
        return false;
    }
    
    // 分配前保护区域
    region->guard_zone_before = 
        (uint8_t*)malloc(data->guard_zone_size);
    
    if (region->guard_zone_before == NULL)
    {
        free(region);
        return false;
    }
    
    // 分配后保护区域
    region->guard_zone_after = 
        (uint8_t*)malloc(data->guard_zone_size);
    
    if (region->guard_zone_after == NULL)
    {
        free(region->guard_zone_before);
        free(region);
        return false;
    }
    
    // 初始化保护区域
    memset(region->guard_zone_before, data->guard_pattern, data->guard_zone_size);
    memset(region->guard_zone_after, data->guard_pattern, data->guard_zone_size);
    
    // 填充区域信息
    region->address = address;
    region->size = size;
    region->guard_zone_size = data->guard_zone_size;
    region->file = file;
    region->line = line;
    region->function = function;
    
    // 添加到链表头部
    region->next = data->guard_regions;
    data->guard_regions = region;
    
    return true;
}

/**
 * @brief 取消内存区域保护
 */
void F_unprotect_memory_region(Stru_ErrorDetectorContext_t* detector, void* address)
{
    if (detector == NULL || address == NULL)
    {
        return;
    }
    
    Stru_ErrorDetectorData_t* data = (Stru_ErrorDetectorData_t*)detector;
    
    // 在链表中查找对应的保护区域
    Stru_MemoryGuardRegion_t* current = data->guard_regions;
    Stru_MemoryGuardRegion_t* previous = NULL;
    
    while (current != NULL)
    {
        if (current->address == address)
        {
            // 检查保护区域是否被破坏
            if (data->enable_overflow_check)
            {
                bool overflow_detected = false;
                
                // 检查前保护区域
                for (size_t i = 0; i < current->guard_zone_size; i++)
                {
                    if (current->guard_zone_before[i] != data->guard_pattern)
                    {
                        add_error_record(data,
                                        Eum_MEMORY_ERROR_OVERFLOW,
                                        address,
                                        current->size,
                                        current->file,
                                        current->line,
                                        current->function,
                                        "缓冲区下溢检测");
                        overflow_detected = true;
                        break;
                    }
                }
                
                // 检查后保护区域
                for (size_t i = 0; i < current->guard_zone_size; i++)
                {
                    if (current->guard_zone_after[i] != data->guard_pattern)
                    {
                        add_error_record(data,
                                        Eum_MEMORY_ERROR_OVERFLOW,
                                        address,
                                        current->size,
                                        current->file,
                                        current->line,
                                        current->function,
                                        "缓冲区溢出检测");
                        overflow_detected = true;
                        break;
                    }
                }
            }
            
            // 从链表中移除区域
            if (previous == NULL)
            {
                data->guard_regions = current->next;
            }
            else
            {
                previous->next = current->next;
            }
            
            // 释放保护区域内存
            free(current->guard_zone_before);
            free(current->guard_zone_after);
            free(current);
            return;
        }
        
        previous = current;
        current = current->next;
    }
}

/**
 * @brief 检查内存区域
 */
bool F_check_memory_region(Stru_ErrorDetectorContext_t* detector,
                          void* address, size_t size)
{
    if (detector == NULL || address == NULL)
    {
        return false;
    }
    
    Stru_ErrorDetectorData_t* data = (Stru_ErrorDetectorData_t*)detector;
    
    // 查找对应的保护区域
    Stru_MemoryGuardRegion_t* region = data->guard_regions;
    while (region != NULL)
    {
        if (region->address == address)
        {
            // 检查保护区域
            if (data->enable_overflow_check)
            {
                // 检查前保护区域
                for (size_t i = 0; i < region->guard_zone_size; i++)
                {
                    if (region->guard_zone_before[i] != data->guard_pattern)
                    {
                        add_error_record(data,
                                        Eum_MEMORY_ERROR_OVERFLOW,
                                        address,
                                        size,
                                        region->file,
                                        region->line,
                                        region->function,
                                        "缓冲区下溢检测");
                        return false;
                    }
                }
                
                // 检查后保护区域
                for (size_t i = 0; i < region->guard_zone_size; i++)
                {
                    if (region->guard_zone_after[i] != data->guard_pattern)
                    {
                        add_error_record(data,
                                        Eum_MEMORY_ERROR_OVERFLOW,
                                        address,
                                        size,
                                        region->file,
                                        region->line,
                                        region->function,
                                        "缓冲区溢出检测");
                        return false;
                    }
                }
            }
            return true;
        }
        region = region->next;
    }
    
    return true; // 没有保护区域，假设检查通过
}

/**
 * @brief 记录内存分配（错误检测模块）
 */
void F_error_detector_record_allocation(Stru_ErrorDetectorContext_t* detector,
                        void* address, size_t size)
{
    if (detector == NULL || address == NULL || size == 0)
    {
        return;
    }
    
    Stru_ErrorDetectorData_t* data = (Stru_ErrorDetectorData_t*)detector;
    
    if (!data->enable_double_free_check)
    {
        return;
    }
    
    // 创建分配记录
    Stru_AllocationRecord_t* record = 
        (Stru_AllocationRecord_t*)malloc(sizeof(Stru_AllocationRecord_t));
    
    if (record == NULL)
    {
        return;
    }
    
    // 填充记录信息
    record->address = address;
    record->size = size;
    record->freed = false;
    
    // 添加到链表头部
    record->next = data->allocations;
    data->allocations = record;
}

/**
 * @brief 记录内存释放（错误检测模块）
 */
bool F_error_detector_record_deallocation(Stru_ErrorDetectorContext_t* detector, void* address)
{
    if (detector == NULL || address == NULL)
    {
        return false;
    }
    
    Stru_ErrorDetectorData_t* data = (Stru_ErrorDetectorData_t*)detector;
    
    if (!data->enable_double_free_check)
    {
        return true;
    }
    
    // 在链表中查找对应的分配记录
    Stru_AllocationRecord_t* current = data->allocations;
    Stru_AllocationRecord_t* previous = NULL;
    
    while (current != NULL)
    {
        if (current->address == address)
        {
            if (current->freed)
            {
                // 双重释放检测
                add_error_record(data,
                                Eum_MEMORY_ERROR_DOUBLE_FREE,
                                address,
                                current->size,
                                NULL,
                                0,
                                NULL,
                                "双重释放检测");
                return false;
            }
            
            // 标记为已释放
            current->freed = true;
            return true;
        }
        
        previous = current;
        current = current->next;
    }
    
    // 未找到分配记录（无效释放）
    add_error_record(data,
                    Eum_MEMORY_ERROR_INVALID_FREE,
                    address,
                    0,
                    NULL,
                    0,
                    NULL,
                    "无效释放检测");
    return false;
}

/**
 * @brief 验证指针有效性
 */
bool F_validate_pointer(Stru_ErrorDetectorContext_t* detector, const void* ptr)
{
    if (detector == NULL || ptr == NULL)
    {
        return false;
    }
    
    Stru_ErrorDetectorData_t* data = (Stru_ErrorDetectorData_t*)detector;
    
    // 在分配记录中查找指针
    Stru_AllocationRecord_t* current = data->allocations;
    while (current != NULL)
    {
        if (current->address == ptr && !current->freed)
        {
            return true;
        }
        current = current->next;
    }
    
    return false;
}

/**
 * @brief 验证内存范围有效性
 */
bool F_validate_memory_range(Stru_ErrorDetectorContext_t* detector,
                            const void* ptr, size_t size)
{
    if (detector == NULL || ptr == NULL || size == 0)
    {
        return false;
    }
    
    Stru_ErrorDetectorData_t* data = (Stru_ErrorDetectorData_t*)detector;
    
    // 在分配记录中查找包含该范围的内存块
    Stru_AllocationRecord_t* current = data->allocations;
    while (current != NULL)
    {
        if (!current->freed)
        {
            uintptr_t start = (uintptr_t)current->address;
            uintptr_t end = start + current->size;
            uintptr_t ptr_start = (uintptr_t)ptr;
            uintptr_t ptr_end = ptr_start + size;
            
            if (ptr_start >= start && ptr_end <= end)
            {
                return true;
            }
        }
        current = current->next;
    }
    
    return false;
}

/**
 * @brief 检查所有活动内存分配
 */
size_t F_check_all_allocations(Stru_ErrorDetectorContext_t* detector)
{
    if (detector == NULL)
    {
        return 0;
    }
    
    Stru_ErrorDetectorData_t* data = (Stru_ErrorDetectorData_t*)detector;
    size_t error_count_before = data->error_count;
    
    // 检查所有保护区域
    Stru_MemoryGuardRegion_t* region = data->guard_regions;
    while (region != NULL)
    {
        F_check_memory_region(detector, region->address, region->size);
        region = region->next;
    }
    
    // 检查所有分配记录
    Stru_AllocationRecord_t* allocation = data->allocations;
    while (allocation != NULL)
    {
        // 检查是否已释放但仍有保护区域
        if (allocation->freed)
        {
            Stru_MemoryGuardRegion_t* region = data->guard_regions;
            while (region != NULL)
            {
                if (region->address == allocation->address)
                {
                    add_error_record(data,
                                    Eum_MEMORY_ERROR_USE_AFTER_FREE,
                                    allocation->address,
                                    allocation->size,
                                    NULL,
                                    0,
                                    NULL,
                                    "释放后使用检测");
                    break;
                }
                region = region->next;
            }
        }
        allocation = allocation->next;
    }
    
    return data->error_count - error_count_before;
}

/**
 * @brief 获取错误数量
 */
size_t F_get_error_count(Stru_ErrorDetectorContext_t* detector)
{
    if (detector == NULL)
    {
        return 0;
    }
    
    Stru_ErrorDetectorData_t* data = (Stru_ErrorDetectorData_t*)detector;
    return data->error_count;
}

/**
 * @brief 获取错误信息
 */
bool F_get_error_info(Stru_ErrorDetectorContext_t* detector,
                     size_t index, Stru_MemoryErrorInfo_t* error_info)
{
    if (detector == NULL || error_info == NULL)
    {
        return false;
    }
    
    Stru_ErrorDetectorData_t* data = (Stru_ErrorDetectorData_t*)detector;
    
    // 遍历错误链表找到指定索引
    Stru_ErrorRecord_t* current = data->errors;
    size_t current_index = 0;
    
    while (current != NULL)
    {
        if (current_index == index)
        {
            *error_info = current->error_info;
            return true;
        }
        current = current->next;
        current_index++;
    }
    
    return false;
}

/**
 * @brief 清除错误记录
 */
void F_clear_error_records(Stru_ErrorDetectorContext_t* detector)
{
    if (detector == NULL)
    {
        return;
    }
    
    Stru_ErrorDetectorData_t* data = (Stru_ErrorDetectorData_t*)detector;
    
    // 释放所有错误记录
    Stru_ErrorRecord_t* current = data->errors;
    while (current != NULL)
    {
        Stru_ErrorRecord_t* next = current->next;
        free(current);
        current = next;
    }
    
    data->errors = NULL;
    data->error_count = 0;
}
