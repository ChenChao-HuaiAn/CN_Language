/******************************************************************************
 * 文件名: CN_debug_core.c
 * 功能: CN_Language调试分配器核心实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建调试分配器核心实现
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_debug_internal.h"
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
void debug_lock(Stru_CN_DebugAllocator_t* allocator)
{
    // 当前版本不支持线程安全，留作扩展
    (void)allocator;
}

/**
 * @brief 解锁分配器（如果支持线程安全）
 */
void debug_unlock(Stru_CN_DebugAllocator_t* allocator)
{
    // 当前版本不支持线程安全，留作扩展
    (void)allocator;
}

/**
 * @brief 获取当前时间戳
 */
uint64_t debug_get_timestamp(void)
{
    // 使用简单的时间戳实现
    // 在实际项目中可以使用更精确的时间函数
    return (uint64_t)time(NULL);
}

/**
 * @brief 调试输出
 */
void debug_output(Stru_CN_DebugAllocator_t* allocator,
                  const char* format, ...)
{
    if (allocator->config.enable_statistics || allocator->config.enable_leak_detection)
    {
        if (allocator->debug_callback != NULL)
        {
            char buffer[512];
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
void debug_fill_memory(void* ptr, size_t size, unsigned char pattern)
{
    if (ptr != NULL && size > 0)
    {
        memset(ptr, pattern, size);
    }
}

/**
 * @brief 验证分配大小
 */
bool debug_validate_size(Stru_CN_DebugAllocator_t* allocator,
                         size_t size)
{
    // 检查是否为0
    if (size == 0)
    {
        debug_output(allocator, 
                    "[DEBUG WARNING] 尝试分配0字节内存\n");
        return false;
    }
    
    // 检查单次分配限制
    if (allocator->config.max_single_alloc > 0 && 
        size > allocator->config.max_single_alloc)
    {
        debug_output(allocator, 
                    "[DEBUG ERROR] 分配大小超过单次分配限制: %zu > %zu\n",
                    size, allocator->config.max_single_alloc);
        return false;
    }
    
    // 检查总分配限制
    if (allocator->config.max_total_alloc > 0 && 
        allocator->stats.current_usage + size > allocator->config.max_total_alloc)
    {
        debug_output(allocator, 
                    "[DEBUG ERROR] 分配将超过总分配限制: %zu + %zu > %zu\n",
                    allocator->stats.current_usage, size,
                    allocator->config.max_total_alloc);
        return false;
    }
    
    return true;
}

/**
 * @brief 验证内存块头部
 */
bool debug_validate_block_header(const Stru_CN_DebugBlockHeader_t* header)
{
    if (header == NULL)
    {
        return false;
    }
    
    // 检查魔术字
    if (header->magic == CN_DEBUG_MAGIC_NORMAL)
    {
        return true;  // 正常分配
    }
    else if (header->magic == CN_DEBUG_MAGIC_FREED)
    {
        return false; // 已释放的块
    }
    
    return false; // 无效的魔术字
}

/**
 * @brief 验证对齐内存块头部
 */
bool debug_validate_aligned_block_header(
    const Stru_CN_DebugAlignedBlockHeader_t* header)
{
    if (header == NULL)
    {
        return false;
    }
    
    // 检查魔术字
    if (header->magic == CN_DEBUG_MAGIC_ALIGNED)
    {
        return true;  // 正常对齐分配
    }
    else if (header->magic == CN_DEBUG_MAGIC_FREED)
    {
        return false; // 已释放的块
    }
    
    return false; // 无效的魔术字
}

/**
 * @brief 记录分配信息
 */
bool debug_record_allocation(Stru_CN_DebugAllocator_t* allocator,
                             void* address, void* real_address,
                             size_t size, size_t real_size,
                             const char* file, int line,
                             const char* purpose, uint64_t allocation_id)
{
    if (!allocator->config.track_allocations)
    {
        return true;
    }
    
    if (allocator->allocation_count >= allocator->config.max_tracked_allocations)
    {
        debug_output(allocator, 
                    "[DEBUG WARNING] 达到最大跟踪分配数量限制: %zu\n",
                    allocator->config.max_tracked_allocations);
        return false;
    }
    
    // 创建分配信息记录
    Stru_CN_DebugAllocationInfo_t* info = malloc(sizeof(Stru_CN_DebugAllocationInfo_t));
    if (info == NULL)
    {
        debug_output(allocator, 
                    "[DEBUG ERROR] 无法分配内存来跟踪分配信息\n");
        return false;
    }
    
    info->address = address;
    info->real_address = real_address;
    info->size = size;
    info->real_size = real_size;
    info->file = file;
    info->line = line;
    info->timestamp = debug_get_timestamp();
    info->purpose = purpose;
    info->allocation_id = allocation_id;
    info->is_freed = false;
    
    // 添加到跟踪列表
    allocator->allocations[allocator->allocation_count++] = info;
    
    return true;
}

/**
 * @brief 标记分配为已释放
 */
bool debug_mark_allocation_freed(Stru_CN_DebugAllocator_t* allocator,
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
            allocator->allocations[i]->is_freed = true;
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 获取分配信息
 */
Stru_CN_DebugAllocationInfo_t* debug_find_allocation(
    Stru_CN_DebugAllocator_t* allocator, void* address)
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
 * @brief 检查保护区域完整性
 */
bool debug_check_guard_zones(Stru_CN_DebugAllocator_t* allocator,
                             const Stru_CN_DebugBlockHeader_t* header)
{
    if (!allocator->config.enable_boundary_check || header->guard_zone_size == 0)
    {
        return true;
    }
    
    // 获取保护区域指针
    unsigned char* guard_before = (unsigned char*)header + sizeof(Stru_CN_DebugBlockHeader_t);
    unsigned char* guard_after = guard_before + header->guard_zone_size + header->size;
    
    // 检查前保护区域
    for (size_t i = 0; i < header->guard_zone_size; i++)
    {
        if (guard_before[i] != CN_DEBUG_FILL_GUARD_BEFORE)
        {
            debug_output(allocator, 
                        "[DEBUG ERROR] 前保护区域损坏: 偏移=%zu, 期望=0x%02X, 实际=0x%02X\n",
                        i, CN_DEBUG_FILL_GUARD_BEFORE, guard_before[i]);
            return false;
        }
    }
    
    // 检查后保护区域
    for (size_t i = 0; i < header->guard_zone_size; i++)
    {
        if (guard_after[i] != CN_DEBUG_FILL_GUARD_AFTER)
        {
            debug_output(allocator, 
                        "[DEBUG ERROR] 后保护区域损坏: 偏移=%zu, 期望=0x%02X, 实际=0x%02X\n",
                        i, CN_DEBUG_FILL_GUARD_AFTER, guard_after[i]);
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 检查对齐保护区域完整性
 */
bool debug_check_aligned_guard_zones(Stru_CN_DebugAllocator_t* allocator,
                                     const Stru_CN_DebugAlignedBlockHeader_t* header)
{
    if (!allocator->config.enable_boundary_check || header->guard_zone_size == 0)
    {
        return true;
    }
    
    // 对齐分配的保护区域检查更复杂，这里简化处理
    // 实际实现需要根据对齐要求调整
    return true;
}

/**
 * @brief 更新统计信息（分配）
 */
void debug_update_stats_on_alloc(Stru_CN_DebugAllocator_t* allocator,
                                 size_t size, size_t real_size)
{
    if (!allocator->config.enable_statistics)
    {
        return;
    }
    
    allocator->stats.total_allocated += size;
    allocator->stats.current_usage += size;
    allocator->stats.allocation_count++;
    
    // 计算内存开销
    size_t overhead = real_size - size;
    allocator->stats.memory_overhead += overhead;
    
    if (allocator->stats.current_usage > allocator->stats.peak_usage)
    {
        allocator->stats.peak_usage = allocator->stats.current_usage;
    }
}

/**
 * @brief 更新统计信息（释放）
 */
void debug_update_stats_on_free(Stru_CN_DebugAllocator_t* allocator,
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

/**
 * @brief 更新统计信息（边界违规）
 */
void debug_update_stats_on_boundary_violation(Stru_CN_DebugAllocator_t* allocator)
{
    if (!allocator->config.enable_statistics)
    {
        return;
    }
    
    allocator->stats.boundary_violations++;
}

/**
 * @brief 更新统计信息（双重释放）
 */
void debug_update_stats_on_double_free(Stru_CN_DebugAllocator_t* allocator)
{
    if (!allocator->config.enable_statistics)
    {
        return;
    }
    
    allocator->stats.double_free_detections++;
}

/**
 * @brief 更新统计信息（内存损坏）
 */
void debug_update_stats_on_corruption(Stru_CN_DebugAllocator_t* allocator)
{
    if (!allocator->config.enable_statistics)
    {
        return;
    }
    
    allocator->stats.corruption_detections++;
}

/**
 * @brief 计算实际分配大小（包括头部和保护区域）
 */
size_t debug_calculate_real_size(Stru_CN_DebugAllocator_t* allocator,
                                 size_t size)
{
    size_t real_size = sizeof(Stru_CN_DebugBlockHeader_t);
    
    if (allocator->config.enable_boundary_check)
    {
        real_size += 2 * allocator->config.guard_zone_size; // 前后保护区域
    }
    
    real_size += size; // 用户数据
    
    return real_size;
}

/**
 * @brief 获取用户指针（从头部指针）
 */
void* debug_get_user_pointer(Stru_CN_DebugBlockHeader_t* header)
{
    if (header == NULL)
    {
        return NULL;
    }
    
    unsigned char* ptr = (unsigned char*)header;
    ptr += sizeof(Stru_CN_DebugBlockHeader_t);
    
    if (header->guard_zone_size > 0)
    {
        ptr += header->guard_zone_size; // 跳过前保护区域
    }
    
    return (void*)ptr;
}

/**
 * @brief 获取头部指针（从用户指针）
 */
Stru_CN_DebugBlockHeader_t* debug_get_header_pointer(void* user_ptr)
{
    if (user_ptr == NULL)
    {
        return NULL;
    }
    
    unsigned char* ptr = (unsigned char*)user_ptr;
    
    // 先减去保护区域大小（如果有）
    // 注意：我们需要知道保护区域大小，这需要从分配信息中获取
    // 简化处理：假设没有保护区域
    ptr -= sizeof(Stru_CN_DebugBlockHeader_t);
    
    return (Stru_CN_DebugBlockHeader_t*)ptr;
}

// ============================================================================
// 公共API实现 - 分配器管理
// ============================================================================

Stru_CN_DebugAllocator_t* CN_debug_create(const Stru_CN_DebugConfig_t* config)
{
    if (config == NULL)
    {
        return NULL;
    }
    
    // 分配分配器结构
    Stru_CN_DebugAllocator_t* allocator = malloc(sizeof(Stru_CN_DebugAllocator_t));
    if (allocator == NULL)
    {
        return NULL;
    }
    
    // 初始化配置
    memcpy(&allocator->config, config, sizeof(Stru_CN_DebugConfig_t));
    
    // 确保最大跟踪分配数量不超过内部限制
    if (allocator->config.max_tracked_allocations > CN_DEBUG_MAX_TRACKED_ALLOCATIONS)
    {
        allocator->config.max_tracked_allocations = CN_DEBUG_MAX_TRACKED_ALLOCATIONS;
    }
    
    // 初始化统计信息
    memset(&allocator->stats, 0, sizeof(Stru_CN_DebugStats_t));
    
    // 初始化分配跟踪
    allocator->allocation_count = 0;
    memset(allocator->allocations, 0, 
           sizeof(Stru_CN_DebugAllocationInfo_t*) * CN_DEBUG_MAX_TRACKED_ALLOCATIONS);
    
    // 初始化分配ID计数器
    allocator->next_allocation_id = 1;
    
    // 初始化调试回调
    allocator->debug_callback = NULL;
    allocator->debug_user_data = NULL;
    
    // 初始化互斥锁（当前版本不支持）
    allocator->mutex = NULL;
    
    debug_output(allocator, 
                "[DEBUG INFO] 调试分配器已创建，配置: 边界检查=%s, 泄漏检测=%s, 双重释放检测=%s\n",
                allocator->config.enable_boundary_check ? "是" : "否",
                allocator->config.enable_leak_detection ? "是" : "否",
                allocator->config.enable_double_free_check ? "是" : "否");
    
    return allocator;
}

void CN_debug_destroy(Stru_CN_DebugAllocator_t* allocator)
{
    if (allocator == NULL)
    {
        return;
    }
    
    debug_lock(allocator);
    
    // 检查内存泄漏
    if (allocator->config.enable_leak_detection)
    {
        size_t leak_count = 0;
        for (size_t i = 0; i < allocator->allocation_count; i++)
        {
            if (!allocator->allocations[i]->is_freed)
            {
                leak_count++;
            }
        }
        
        if (leak_count > 0)
        {
            debug_output(allocator, 
                        "[DEBUG WARNING] 检测到内存泄漏: %zu 个未释放的分配\n",
                        leak_count);
            
            // 转储泄漏信息
            CN_debug_dump_leaks(allocator);
            
            // 更新泄漏统计
            allocator->stats.leak_count = leak_count;
        }
    }
    
    // 释放所有跟踪的分配信息
    for (size_t i = 0; i < allocator->allocation_count; i++)
    {
        free(allocator->allocations[i]);
    }
    
    debug_unlock(allocator);
    
    // 在释放前记录销毁信息
    bool enable_statistics = allocator->config.enable_statistics;
    
    // 释放分配器本身
    free(allocator);
    
    // 注意：这里不能使用allocator指针，因为它已经被释放了
    if (enable_statistics)
    {
        printf("[DEBUG INFO] 调试分配器已销毁\n");
    }
}

// 注意：为了保持文件大小不超过500行，我将继续在下一个文件中实现其他函数
