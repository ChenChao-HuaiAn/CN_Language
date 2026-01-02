/******************************************************************************
 * 文件名: CN_debug_operations.c
 * 功能: CN_Language调试分配器操作实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建调试分配器操作实现
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_debug_internal.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// 公共API实现 - 内存分配和释放
// ============================================================================

void* CN_debug_alloc(Stru_CN_DebugAllocator_t* allocator, size_t size, 
                     const char* file, int line, const char* purpose)
{
    if (allocator == NULL)
    {
        return NULL;
    }
    
    debug_lock(allocator);
    
    // 验证分配大小
    if (!debug_validate_size(allocator, size))
    {
        debug_unlock(allocator);
        return NULL;
    }
    
    // 计算实际分配大小（包括头部和保护区域）
    size_t real_size = debug_calculate_real_size(allocator, size);
    size_t guard_zone_size = allocator->config.enable_boundary_check ? 
                             allocator->config.guard_zone_size : 0;
    
    // 分配内存
    Stru_CN_DebugBlockHeader_t* header = malloc(real_size);
    if (header == NULL)
    {
        allocator->stats.allocation_failures++;
        debug_output(allocator, 
                    "[DEBUG ERROR] 内存分配失败: 大小=%zu, 实际大小=%zu, 文件=%s, 行=%d\n",
                    size, real_size, file ? file : "未知", line);
        debug_unlock(allocator);
        return NULL;
    }
    
    // 初始化头部
    header->size = size;
    header->real_size = real_size;
    header->magic = CN_DEBUG_MAGIC_NORMAL;
    header->file = file;
    header->line = line;
    header->purpose = purpose;
    header->timestamp = debug_get_timestamp();
    header->allocation_id = allocator->next_allocation_id++;
    header->guard_zone_size = guard_zone_size;
    
    // 获取用户内存指针
    void* user_ptr = debug_get_user_pointer(header);
    
    // 设置保护区域（如果启用）
    if (allocator->config.enable_boundary_check && guard_zone_size > 0)
    {
        unsigned char* guard_before = (unsigned char*)header + sizeof(Stru_CN_DebugBlockHeader_t);
        unsigned char* guard_after = guard_before + guard_zone_size + size;
        
        // 填充前保护区域
        debug_fill_memory(guard_before, guard_zone_size, CN_DEBUG_FILL_GUARD_BEFORE);
        
        // 填充后保护区域
        debug_fill_memory(guard_after, guard_zone_size, CN_DEBUG_FILL_GUARD_AFTER);
    }
    
    // 记录分配信息
    if (allocator->config.track_allocations)
    {
        debug_record_allocation(allocator, user_ptr, header, size, real_size, 
                               file, line, purpose, header->allocation_id);
    }
    
    // 更新统计信息
    debug_update_stats_on_alloc(allocator, size, real_size);
    
    // 调试填充
    if (allocator->config.fill_on_alloc)
    {
        debug_fill_memory(user_ptr, size, CN_DEBUG_FILL_ALLOC);
    }
    
    // 清零内存
    if (allocator->config.zero_on_alloc)
    {
        memset(user_ptr, 0, size);
    }
    
    debug_output(allocator, 
                "[DEBUG INFO] 分配内存: ID=%llu, 地址=%p, 大小=%zu, 实际大小=%zu, 文件=%s, 行=%d, 目的=%s\n",
                (unsigned long long)header->allocation_id, user_ptr, size, real_size,
                file ? file : "未知", line, purpose ? purpose : "未知");
    
    debug_unlock(allocator);
    
    return user_ptr;
}

void CN_debug_free(Stru_CN_DebugAllocator_t* allocator, void* ptr, 
                   const char* file, int line)
{
    if (allocator == NULL || ptr == NULL)
    {
        return;
    }
    
    debug_lock(allocator);
    
    // 首先尝试作为普通分配处理
    Stru_CN_DebugBlockHeader_t* header = debug_get_header_pointer(ptr);
    
    if (header == NULL)
    {
        debug_output(allocator, 
                    "[DEBUG ERROR] 无效的内存指针: 地址=%p, 文件=%s, 行=%d\n",
                    ptr, file ? file : "未知", line);
        debug_unlock(allocator);
        return;
    }
    
    // 验证头部
    if (!debug_validate_block_header(header))
    {
        // 检查是否是双重释放
        if (header->magic == CN_DEBUG_MAGIC_FREED)
        {
            debug_output(allocator, 
                        "[DEBUG ERROR] 双重释放检测: 地址=%p, 文件=%s, 行=%d\n",
                        ptr, file ? file : "未知", line);
            
            // 更新双重释放统计
            debug_update_stats_on_double_free(allocator);
        }
        else
        {
            debug_output(allocator, 
                        "[DEBUG ERROR] 内存损坏或无效指针: 地址=%p, 魔术字=0x%08zX, 文件=%s, 行=%d\n",
                        ptr, header->magic, file ? file : "未知", line);
            
            // 更新内存损坏统计
            debug_update_stats_on_corruption(allocator);
        }
        
        debug_unlock(allocator);
        return;
    }
    
    // 检查保护区域完整性（如果启用）
    if (allocator->config.enable_boundary_check && 
        allocator->config.enable_memory_corruption_check)
    {
        if (!debug_check_guard_zones(allocator, header))
        {
            debug_output(allocator, 
                        "[DEBUG ERROR] 保护区域损坏: 地址=%p, ID=%llu, 文件=%s, 行=%d\n",
                        ptr, (unsigned long long)header->allocation_id,
                        file ? file : "未知", line);
            
            // 更新边界违规统计
            debug_update_stats_on_boundary_violation(allocator);
            
            // 继续释放，但记录错误
        }
    }
    
    size_t size = header->size;
    
    // 调试填充
    if (allocator->config.fill_on_free)
    {
        debug_fill_memory(ptr, size, CN_DEBUG_FILL_FREE);
    }
    
    // 标记分配为已释放
    if (allocator->config.track_allocations)
    {
        debug_mark_allocation_freed(allocator, ptr);
    }
    
    // 更新统计信息
    debug_update_stats_on_free(allocator, size);
    
    // 标记为已释放（防止双重释放检测）
    header->magic = CN_DEBUG_MAGIC_FREED;
    
    debug_output(allocator, 
                "[DEBUG INFO] 释放内存: ID=%llu, 地址=%p, 大小=%zu, 文件=%s, 行=%d\n",
                (unsigned long long)header->allocation_id, ptr, size,
                file ? file : "未知", line);
    
    // 释放内存
    free(header);
    
    debug_unlock(allocator);
}

void* CN_debug_realloc(Stru_CN_DebugAllocator_t* allocator, void* ptr, 
                       size_t new_size, const char* file, int line, 
                       const char* purpose)
{
    if (allocator == NULL)
    {
        return NULL;
    }
    
    // 如果ptr为NULL，相当于分配新内存
    if (ptr == NULL)
    {
        return CN_debug_alloc(allocator, new_size, file, line, purpose);
    }
    
    // 如果new_size为0，相当于释放内存
    if (new_size == 0)
    {
        CN_debug_free(allocator, ptr, file, line);
        return NULL;
    }
    
    debug_lock(allocator);
    
    // 获取原分配信息
    Stru_CN_DebugBlockHeader_t* old_header = debug_get_header_pointer(ptr);
    
    // 验证原分配
    if (old_header == NULL || !debug_validate_block_header(old_header))
    {
        debug_output(allocator, 
                    "[DEBUG ERROR] 重新分配失败: 无效的原指针: 地址=%p, 文件=%s, 行=%d\n",
                    ptr, file ? file : "未知", line);
        debug_unlock(allocator);
        return NULL;
    }
    
    size_t old_size = old_header->size;
    
    // 分配新内存
    void* new_ptr = CN_debug_alloc(allocator, new_size, file, line, purpose);
    if (new_ptr == NULL)
    {
        debug_unlock(allocator);
        return NULL;
    }
    
    // 复制数据（复制较小的大小）
    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    
    // 释放原内存
    // 注意：这里我们直接调用内部函数，避免重复锁定
    // 首先标记为已释放
    old_header->magic = CN_DEBUG_MAGIC_FREED;
    
    // 更新统计信息
    debug_update_stats_on_free(allocator, old_size);
    
    // 标记分配为已释放
    if (allocator->config.track_allocations)
    {
        debug_mark_allocation_freed(allocator, ptr);
    }
    
    // 释放原内存
    free(old_header);
    
    debug_output(allocator, 
                "[DEBUG INFO] 重新分配内存: 原地址=%p, 新地址=%p, 原大小=%zu, 新大小=%zu, 文件=%s, 行=%d\n",
                ptr, new_ptr, old_size, new_size, file ? file : "未知", line);
    
    debug_unlock(allocator);
    
    return new_ptr;
}

void* CN_debug_calloc(Stru_CN_DebugAllocator_t* allocator, size_t count, 
                      size_t size, const char* file, int line, 
                      const char* purpose)
{
    if (allocator == NULL)
    {
        return NULL;
    }
    
    // 检查乘法溢出
    if (count > 0 && size > SIZE_MAX / count)
    {
        debug_output(allocator, 
                    "[DEBUG ERROR] 乘法溢出: count=%zu * size=%zu\n", count, size);
        allocator->stats.allocation_failures++;
        return NULL;
    }
    
    size_t total_size = count * size;
    
    // 分配内存
    void* ptr = CN_debug_alloc(allocator, total_size, file, line, purpose);
    if (ptr != NULL)
    {
        // 清零内存
        memset(ptr, 0, total_size);
        
        // 更新统计
        allocator->stats.calloc_count++;
    }
    
    return ptr;
}

// ============================================================================
// 公共API实现 - 对齐内存分配
// ============================================================================

void* CN_debug_alloc_aligned(Stru_CN_DebugAllocator_t* allocator, size_t size, 
                             size_t alignment, const char* file, int line, 
                             const char* purpose)
{
    if (allocator == NULL)
    {
        return NULL;
    }
    
    debug_lock(allocator);
    
    // 验证对齐要求（必须是2的幂）
    if (alignment == 0 || (alignment & (alignment - 1)) != 0)
    {
        debug_output(allocator, 
                    "[DEBUG ERROR] 无效的对齐要求: %zu (必须是2的幂)\n", alignment);
        allocator->stats.allocation_failures++;
        debug_unlock(allocator);
        return NULL;
    }
    
    // 验证分配大小
    if (!debug_validate_size(allocator, size))
    {
        debug_unlock(allocator);
        return NULL;
    }
    
    // 计算额外空间以满足对齐要求
    size_t extra = alignment - 1 + sizeof(void*);
    size_t total_size = size + extra;
    
    // 分配原始内存
    void* original_ptr = malloc(total_size);
    if (original_ptr == NULL)
    {
        allocator->stats.allocation_failures++;
        debug_output(allocator, 
                    "[DEBUG ERROR] 对齐内存分配失败: 大小=%zu, 对齐=%zu, 总大小=%zu\n",
                    size, alignment, total_size);
        debug_unlock(allocator);
        return NULL;
    }
    
    // 计算对齐地址
    void* aligned_ptr = (void*)(((uintptr_t)original_ptr + sizeof(void*) + alignment - 1) & ~(alignment - 1));
    
    // 在对齐地址之前存储原始指针
    *((void**)((char*)aligned_ptr - sizeof(void*))) = original_ptr;
    
    // 记录分配信息（简化处理，不包含完整的调试头部）
    if (allocator->config.track_allocations)
    {
        // 使用下一个分配ID
        uint64_t allocation_id = allocator->next_allocation_id++;
        
        // 记录分配信息（简化版）
        debug_record_allocation(allocator, aligned_ptr, original_ptr, size, total_size,
                               file, line, purpose, allocation_id);
    }
    
    // 更新统计信息
    debug_update_stats_on_alloc(allocator, size, total_size);
    
    // 调试填充
    if (allocator->config.fill_on_alloc)
    {
        debug_fill_memory(aligned_ptr, size, CN_DEBUG_FILL_ALLOC);
    }
    
    // 清零内存
    if (allocator->config.zero_on_alloc)
    {
        memset(aligned_ptr, 0, size);
    }
    
    debug_output(allocator, 
                "[DEBUG INFO] 分配对齐内存: 地址=%p, 原始地址=%p, 大小=%zu, 对齐=%zu, 文件=%s, 行=%d\n",
                aligned_ptr, original_ptr, size, alignment, file ? file : "未知", line);
    
    debug_unlock(allocator);
    
    return aligned_ptr;
}

// 注意：为了保持文件大小不超过500行，我将继续在下一个文件中实现其他函数
