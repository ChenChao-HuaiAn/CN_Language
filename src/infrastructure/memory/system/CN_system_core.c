/******************************************************************************
 * 文件名: CN_system_core.c
 * 功能: CN_Language系统分配器核心实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建系统分配器核心实现
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_system_internal.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 锁定分配器（如果支持线程安全）
 */
void system_lock(Stru_CN_SystemAllocator_t* allocator)
{
    // 当前版本不支持线程安全，留作扩展
    (void)allocator;
}

/**
 * @brief 解锁分配器（如果支持线程安全）
 */
void system_unlock(Stru_CN_SystemAllocator_t* allocator)
{
    // 当前版本不支持线程安全，留作扩展
    (void)allocator;
}

/**
 * @brief 获取当前时间戳
 */
uint64_t system_get_timestamp(void)
{
    // 使用简单的时间戳实现
    // 在实际项目中可以使用更精确的时间函数
    return (uint64_t)time(NULL);
}

/**
 * @brief 调试输出
 */
void system_debug_output(Stru_CN_SystemAllocator_t* allocator,
                         const char* format, ...)
{
    if (allocator->config.enable_debug)
    {
        if (allocator->debug_callback != NULL)
        {
            char buffer[256];
            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            
            allocator->debug_callback(buffer, allocator->debug_user_data);
        }
        else
        {
            va_list args;
            va_start(args, format);
            vprintf(format, args);
            va_end(args);
        }
    }
}

/**
 * @brief 填充内存（调试用）
 */
void system_fill_memory(void* ptr, size_t size, unsigned char pattern)
{
    if (ptr != NULL && size > 0)
    {
        memset(ptr, pattern, size);
    }
}

/**
 * @brief 验证分配大小
 */
bool system_validate_size(Stru_CN_SystemAllocator_t* allocator,
                          size_t size)
{
    // 检查是否为0
    if (size == 0)
    {
        system_debug_output(allocator, 
                           "[WARNING] 尝试分配0字节内存\n");
        return false;
    }
    
    // 检查单次分配限制
    if (allocator->config.max_single_alloc > 0 && 
        size > allocator->config.max_single_alloc)
    {
        system_debug_output(allocator, 
                           "[ERROR] 分配大小超过单次分配限制: %zu > %zu\n",
                           size, allocator->config.max_single_alloc);
        return false;
    }
    
    // 检查总分配限制
    if (allocator->config.max_total_alloc > 0 && 
        allocator->stats.current_usage + size > allocator->config.max_total_alloc)
    {
        system_debug_output(allocator, 
                           "[ERROR] 分配将超过总分配限制: %zu + %zu > %zu\n",
                           allocator->stats.current_usage, size,
                           allocator->config.max_total_alloc);
        return false;
    }
    
    return true;
}

/**
 * @brief 验证内存块头部
 */
bool system_validate_block_header(const Stru_CN_BlockHeader_t* header)
{
    if (header == NULL)
    {
        return false;
    }
    
    return header->magic == CN_SYSTEM_MAGIC_NORMAL;
}

/**
 * @brief 验证对齐内存块头部
 */
bool system_validate_aligned_block_header(
    const Stru_CN_AlignedBlockHeader_t* header)
{
    if (header == NULL)
    {
        return false;
    }
    
    return header->magic == CN_SYSTEM_MAGIC_ALIGNED;
}

/**
 * @brief 记录分配信息
 */
bool system_record_allocation(Stru_CN_SystemAllocator_t* allocator,
                              void* address, size_t size,
                              const char* file, int line,
                              const char* purpose)
{
    if (!allocator->config.track_allocations)
    {
        return true;
    }
    
    if (allocator->allocation_count >= CN_SYSTEM_MAX_TRACKED_ALLOCATIONS)
    {
        system_debug_output(allocator, 
                           "[WARNING] 达到最大跟踪分配数量限制\n");
        return false;
    }
    
    // 创建分配信息记录
    Stru_CN_AllocationInfo_t* info = malloc(sizeof(Stru_CN_AllocationInfo_t));
    if (info == NULL)
    {
        return false;
    }
    
    info->address = address;
    info->size = size;
    info->file = file;
    info->line = line;
    info->timestamp = system_get_timestamp();
    info->purpose = purpose;
    
    // 添加到跟踪列表
    allocator->allocations[allocator->allocation_count++] = info;
    
    return true;
}

/**
 * @brief 移除分配记录
 */
bool system_remove_allocation(Stru_CN_SystemAllocator_t* allocator,
                              void* address)
{
    if (!allocator->config.track_allocations)
    {
        return true;
    }
    
    for (size_t i = 0; i < allocator->allocation_count; i++)
    {
        if (allocator->allocations[i]->address == address)
        {
            // 释放分配信息记录
            free(allocator->allocations[i]);
            
            // 移动后续元素
            for (size_t j = i; j < allocator->allocation_count - 1; j++)
            {
                allocator->allocations[j] = allocator->allocations[j + 1];
            }
            
            allocator->allocation_count--;
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 获取分配信息
 */
Stru_CN_AllocationInfo_t* system_find_allocation(
    Stru_CN_SystemAllocator_t* allocator, void* address)
{
    if (!allocator->config.track_allocations)
    {
        return NULL;
    }
    
    for (size_t i = 0; i < allocator->allocation_count; i++)
    {
        if (allocator->allocations[i]->address == address)
        {
            return allocator->allocations[i];
        }
    }
    
    return NULL;
}

/**
 * @brief 更新统计信息（分配）
 */
void system_update_stats_on_alloc(Stru_CN_SystemAllocator_t* allocator,
                                  size_t size)
{
    if (!allocator->config.enable_statistics)
    {
        return;
    }
    
    allocator->stats.total_allocated += size;
    allocator->stats.current_usage += size;
    allocator->stats.allocation_count++;
    
    if (allocator->stats.current_usage > allocator->stats.peak_usage)
    {
        allocator->stats.peak_usage = allocator->stats.current_usage;
    }
    
    // 估算内存开销（头部大小）
    allocator->stats.memory_overhead += sizeof(Stru_CN_BlockHeader_t);
}

/**
 * @brief 更新统计信息（释放）
 */
void system_update_stats_on_free(Stru_CN_SystemAllocator_t* allocator,
                                 size_t size)
{
    if (!allocator->config.enable_statistics)
    {
        return;
    }
    
    allocator->stats.total_freed += size;
    allocator->stats.free_count++;
    
    if (allocator->stats.current_usage >= size)
    {
        allocator->stats.current_usage -= size;
    }
    else
    {
        allocator->stats.current_usage = 0;
    }
}

// ============================================================================
// 公共API实现
// ============================================================================

Stru_CN_SystemAllocator_t* CN_system_create(const Stru_CN_SystemConfig_t* config)
{
    if (config == NULL)
    {
        return NULL;
    }
    
    // 分配分配器结构
    Stru_CN_SystemAllocator_t* allocator = malloc(sizeof(Stru_CN_SystemAllocator_t));
    if (allocator == NULL)
    {
        return NULL;
    }
    
    // 初始化配置
    memcpy(&allocator->config, config, sizeof(Stru_CN_SystemConfig_t));
    
    // 初始化统计信息
    memset(&allocator->stats, 0, sizeof(Stru_CN_SystemStats_t));
    
    // 初始化分配跟踪
    allocator->allocation_count = 0;
    memset(allocator->allocations, 0, 
           sizeof(Stru_CN_AllocationInfo_t*) * CN_SYSTEM_MAX_TRACKED_ALLOCATIONS);
    
    // 初始化调试回调
    allocator->debug_callback = NULL;
    allocator->debug_user_data = NULL;
    
    // 初始化互斥锁（当前版本不支持）
    allocator->mutex = NULL;
    
    system_debug_output(allocator, 
                       "[INFO] 系统分配器已创建，配置: 统计=%s, 调试=%s, 跟踪=%s\n",
                       allocator->config.enable_statistics ? "是" : "否",
                       allocator->config.enable_debug ? "是" : "否",
                       allocator->config.track_allocations ? "是" : "否");
    
    return allocator;
}

void CN_system_destroy(Stru_CN_SystemAllocator_t* allocator)
{
    if (allocator == NULL)
    {
        return;
    }
    
    system_lock(allocator);
    
    // 检查内存泄漏
    if (allocator->config.detect_leaks && allocator->allocation_count > 0)
    {
        system_debug_output(allocator, 
                           "[WARNING] 检测到内存泄漏: %zu 个未释放的分配\n",
                           allocator->allocation_count);
        
        // 转储泄漏信息
        CN_system_dump_leaks(allocator);
    }
    
    // 释放所有跟踪的分配信息
    for (size_t i = 0; i < allocator->allocation_count; i++)
    {
        free(allocator->allocations[i]);
    }
    
    system_unlock(allocator);
    
    // 在释放前记录销毁信息
    bool enable_debug = allocator->config.enable_debug;
    
    // 释放分配器本身
    free(allocator);
    
    // 注意：这里不能使用allocator指针，因为它已经被释放了
    // 如果需要输出销毁信息，应该在释放前输出
    if (enable_debug)
    {
        printf("[INFO] 系统分配器已销毁\n");
    }
}

void* CN_system_alloc(Stru_CN_SystemAllocator_t* allocator, size_t size, 
                      const char* file, int line, const char* purpose)
{
    if (allocator == NULL)
    {
        return NULL;
    }
    
    system_lock(allocator);
    
    // 验证分配大小
    if (!system_validate_size(allocator, size))
    {
        system_unlock(allocator);
        return NULL;
    }
    
    // 计算总大小（包括头部）
    size_t total_size = sizeof(Stru_CN_BlockHeader_t) + size;
    
    // 分配内存
    Stru_CN_BlockHeader_t* header = malloc(total_size);
    if (header == NULL)
    {
        allocator->stats.allocation_failures++;
        system_debug_output(allocator, 
                           "[ERROR] 内存分配失败: 大小=%zu, 文件=%s, 行=%d\n",
                           size, file ? file : "未知", line);
        system_unlock(allocator);
        return NULL;
    }
    
    // 初始化头部
    header->size = size;
    header->magic = CN_SYSTEM_MAGIC_NORMAL;
    header->file = file;
    header->line = line;
    header->purpose = purpose;
    header->timestamp = system_get_timestamp();
    
    // 获取用户内存指针
    void* user_ptr = (char*)header + sizeof(Stru_CN_BlockHeader_t);
    
    // 记录分配信息
    if (allocator->config.track_allocations)
    {
        system_record_allocation(allocator, user_ptr, size, file, line, purpose);
    }
    
    // 更新统计信息
    system_update_stats_on_alloc(allocator, size);
    
    // 调试填充
    if (allocator->config.fill_on_alloc)
    {
        system_fill_memory(user_ptr, size, CN_SYSTEM_DEBUG_FILL_ALLOC);
    }
    
    // 清零内存
    if (allocator->config.zero_on_alloc)
    {
        memset(user_ptr, 0, size);
    }
    
    system_debug_output(allocator, 
                       "[DEBUG] 分配内存: 地址=%p, 大小=%zu, 文件=%s, 行=%d, 目的=%s\n",
                       user_ptr, size, file ? file : "未知", line,
                       purpose ? purpose : "未知");
    
    system_unlock(allocator);
    
    return user_ptr;
}

void CN_system_free(Stru_CN_SystemAllocator_t* allocator, void* ptr, 
                    const char* file, int line)
{
    if (allocator == NULL || ptr == NULL)
    {
        return;
    }
    
    system_lock(allocator);
    
    // 首先尝试作为普通分配处理
    Stru_CN_BlockHeader_t* normal_header = (Stru_CN_BlockHeader_t*)
        ((char*)ptr - sizeof(Stru_CN_BlockHeader_t));
    
    if (system_validate_block_header(normal_header))
    {
        // 普通分配
        size_t size = normal_header->size;
        
        // 调试填充
        if (allocator->config.fill_on_free)
        {
            system_fill_memory(ptr, size, CN_SYSTEM_DEBUG_FILL_FREE);
        }
        
        // 移除分配记录
        if (allocator->config.track_allocations)
        {
            system_remove_allocation(allocator, ptr);
        }
        
        // 更新统计信息
        system_update_stats_on_free(allocator, size);
        
        // 清除头部魔术字（防止重复释放检测）
        normal_header->magic = 0;
        
        system_debug_output(allocator, 
                           "[DEBUG] 释放普通内存: 地址=%p, 大小=%zu, 文件=%s, 行=%d\n",
                           ptr, size, file ? file : "未知", line);
        
        // 释放内存
        free(normal_header);
    }
    else
    {
        // 尝试作为对齐分配处理（新实现）
        // 从用户指针之前获取原始指针的地址
        size_t pointer_size = sizeof(void*);
        void** original_ptr_ptr = (void**)((char*)ptr - pointer_size);
        void* original_ptr = *original_ptr_ptr;
        
        // 验证原始指针是否有效
        if (original_ptr == NULL)
        {
            system_debug_output(allocator, 
                               "[ERROR] 无效的原始指针: 地址=%p, 文件=%s, 行=%d\n",
                               ptr, file ? file : "未知", line);
            system_unlock(allocator);
            return;
        }
        
        // 注意：在新实现中，我们不知道分配的大小
        // 我们需要从分配记录中获取大小
        size_t size = 0;
        Stru_CN_AllocationInfo_t* info = system_find_allocation(allocator, ptr);
        if (info != NULL)
        {
            size = info->size;
        }
        else
        {
            // 如果没有找到分配记录，使用默认大小
            // 这应该不会发生，除非track_allocations被禁用
            system_debug_output(allocator, 
                               "[WARNING] 未找到对齐分配的记录，使用默认大小\n");
            size = 0; // 我们不知道实际大小
        }
        
        // 调试填充
        if (allocator->config.fill_on_free && size > 0)
        {
            system_fill_memory(ptr, size, CN_SYSTEM_DEBUG_FILL_FREE);
        }
        
        // 移除分配记录
        if (allocator->config.track_allocations)
        {
            system_remove_allocation(allocator, ptr);
        }
        
        // 更新统计信息
        if (size > 0)
        {
            system_update_stats_on_free(allocator, size);
        }
        
        system_debug_output(allocator, 
                           "[DEBUG] 释放对齐内存: 地址=%p, 大小=%zu, 文件=%s, 行=%d\n",
                           ptr, size, file ? file : "未知", line);
        
        // 释放原始内存
        free(original_ptr);
    }
    
    system_unlock(allocator);
}

// 注意：为了保持文件大小不超过500行，我将继续在下一个文件中实现其他函数
