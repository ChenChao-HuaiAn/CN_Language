/******************************************************************************
 * 文件名: CN_debug_utils.c
 * 功能: CN_Language调试分配器工具函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建调试分配器工具函数实现
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_debug_internal.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// 公共API实现 - 查询和配置
// ============================================================================

bool CN_debug_get_config(Stru_CN_DebugAllocator_t* allocator, 
                         Stru_CN_DebugConfig_t* config)
{
    if (allocator == NULL || config == NULL)
    {
        return false;
    }
    
    debug_lock(allocator);
    memcpy(config, &allocator->config, sizeof(Stru_CN_DebugConfig_t));
    debug_unlock(allocator);
    
    return true;
}

bool CN_debug_get_stats(Stru_CN_DebugAllocator_t* allocator, 
                        Stru_CN_DebugStats_t* stats)
{
    if (allocator == NULL || stats == NULL)
    {
        return false;
    }
    
    debug_lock(allocator);
    memcpy(stats, &allocator->stats, sizeof(Stru_CN_DebugStats_t));
    debug_unlock(allocator);
    
    return true;
}

void CN_debug_reset_stats(Stru_CN_DebugAllocator_t* allocator)
{
    if (allocator == NULL)
    {
        return;
    }
    
    debug_lock(allocator);
    memset(&allocator->stats, 0, sizeof(Stru_CN_DebugStats_t));
    debug_unlock(allocator);
    
    debug_output(allocator, "[DEBUG INFO] 统计信息已重置\n");
}

bool CN_debug_check_leaks(Stru_CN_DebugAllocator_t* allocator)
{
    if (allocator == NULL)
    {
        return false;
    }
    
    debug_lock(allocator);
    
    bool has_leaks = false;
    size_t leak_count = 0;
    size_t leak_size = 0;
    
    for (size_t i = 0; i < allocator->allocation_count; i++)
    {
        if (!allocator->allocations[i]->is_freed)
        {
            has_leaks = true;
            leak_count++;
            leak_size += allocator->allocations[i]->size;
        }
    }
    
    if (has_leaks)
    {
        allocator->stats.leak_count = leak_count;
        
        debug_output(allocator, 
                    "[DEBUG WARNING] 检测到内存泄漏: %zu 个分配, 总大小: %zu 字节\n",
                    leak_count, leak_size);
    }
    
    debug_unlock(allocator);
    
    return has_leaks;
}

size_t CN_debug_current_usage(Stru_CN_DebugAllocator_t* allocator)
{
    if (allocator == NULL)
    {
        return 0;
    }
    
    debug_lock(allocator);
    size_t usage = allocator->stats.current_usage;
    debug_unlock(allocator);
    
    return usage;
}

size_t CN_debug_peak_usage(Stru_CN_DebugAllocator_t* allocator)
{
    if (allocator == NULL)
    {
        return 0;
    }
    
    debug_lock(allocator);
    size_t peak = allocator->stats.peak_usage;
    debug_unlock(allocator);
    
    return peak;
}

// ============================================================================
// 公共API实现 - 验证和调试
// ============================================================================

bool CN_debug_validate_heap(Stru_CN_DebugAllocator_t* allocator)
{
    if (allocator == NULL)
    {
        return false;
    }
    
    debug_lock(allocator);
    
    bool is_valid = true;
    size_t valid_count = 0;
    size_t invalid_count = 0;
    
    // 检查所有跟踪的分配
    for (size_t i = 0; i < allocator->allocation_count; i++)
    {
        Stru_CN_DebugAllocationInfo_t* info = allocator->allocations[i];
        
        if (!info->is_freed)
        {
            // 尝试获取头部并验证
            Stru_CN_DebugBlockHeader_t* header = debug_get_header_pointer(info->address);
            
            if (header != NULL && debug_validate_block_header(header))
            {
                // 检查保护区域（如果启用）
                if (allocator->config.enable_boundary_check && 
                    allocator->config.enable_memory_corruption_check)
                {
                    if (!debug_check_guard_zones(allocator, header))
                    {
                        debug_output(allocator, 
                                    "[DEBUG ERROR] 堆验证失败: 分配ID=%llu 的保护区域损坏\n",
                                    (unsigned long long)info->allocation_id);
                        is_valid = false;
                        invalid_count++;
                        continue;
                    }
                }
                
                valid_count++;
            }
            else
            {
                debug_output(allocator, 
                            "[DEBUG ERROR] 堆验证失败: 分配ID=%llu 的头部无效或已损坏\n",
                            (unsigned long long)info->allocation_id);
                is_valid = false;
                invalid_count++;
            }
        }
    }
    
    debug_output(allocator, 
                "[DEBUG INFO] 堆验证完成: 有效分配=%zu, 无效分配=%zu, 结果=%s\n",
                valid_count, invalid_count, is_valid ? "通过" : "失败");
    
    debug_unlock(allocator);
    
    return is_valid;
}

bool CN_debug_validate_block(Stru_CN_DebugAllocator_t* allocator, void* ptr)
{
    if (allocator == NULL || ptr == NULL)
    {
        return false;
    }
    
    debug_lock(allocator);
    
    bool is_valid = false;
    
    // 获取分配信息
    Stru_CN_DebugAllocationInfo_t* info = debug_find_allocation(allocator, ptr);
    
    if (info != NULL && !info->is_freed)
    {
        // 获取头部并验证
        Stru_CN_DebugBlockHeader_t* header = debug_get_header_pointer(ptr);
        
        if (header != NULL && debug_validate_block_header(header))
        {
            // 检查保护区域（如果启用）
            if (allocator->config.enable_boundary_check && 
                allocator->config.enable_memory_corruption_check)
            {
                if (debug_check_guard_zones(allocator, header))
                {
                    is_valid = true;
                }
                else
                {
                    debug_output(allocator, 
                                "[DEBUG ERROR] 内存块验证失败: 分配ID=%llu 的保护区域损坏\n",
                                (unsigned long long)info->allocation_id);
                }
            }
            else
            {
                is_valid = true;
            }
        }
        else
        {
            debug_output(allocator, 
                        "[DEBUG ERROR] 内存块验证失败: 分配ID=%llu 的头部无效或已损坏\n",
                        (unsigned long long)info->allocation_id);
        }
    }
    else if (info == NULL)
    {
        debug_output(allocator, 
                    "[DEBUG ERROR] 内存块验证失败: 未找到分配记录\n");
    }
    else
    {
        debug_output(allocator, 
                    "[DEBUG ERROR] 内存块验证失败: 分配已释放\n");
    }
    
    debug_unlock(allocator);
    
    return is_valid;
}

void CN_debug_dump(Stru_CN_DebugAllocator_t* allocator)
{
    if (allocator == NULL)
    {
        return;
    }
    
    debug_lock(allocator);
    
    printf("\n=== 调试分配器状态转储 ===\n");
    printf("配置信息:\n");
    printf("  边界检查: %s\n", allocator->config.enable_boundary_check ? "启用" : "禁用");
    printf("  泄漏检测: %s\n", allocator->config.enable_leak_detection ? "启用" : "禁用");
    printf("  双重释放检测: %s\n", allocator->config.enable_double_free_check ? "启用" : "禁用");
    printf("  内存损坏检测: %s\n", allocator->config.enable_memory_corruption_check ? "启用" : "禁用");
    printf("  统计信息: %s\n", allocator->config.enable_statistics ? "启用" : "禁用");
    printf("  跟踪分配: %s\n", allocator->config.track_allocations ? "启用" : "禁用");
    printf("  保护区域大小: %zu 字节\n", allocator->config.guard_zone_size);
    printf("  最大跟踪分配: %zu\n", allocator->config.max_tracked_allocations);
    
    printf("\n统计信息:\n");
    printf("  总分配字节数: %zu\n", allocator->stats.total_allocated);
    printf("  总释放字节数: %zu\n", allocator->stats.total_freed);
    printf("  当前使用字节数: %zu\n", allocator->stats.current_usage);
    printf("  峰值使用字节数: %zu\n", allocator->stats.peak_usage);
    printf("  分配次数: %zu\n", allocator->stats.allocation_count);
    printf("  释放次数: %zu\n", allocator->stats.free_count);
    printf("  重新分配次数: %zu\n", allocator->stats.realloc_count);
    printf("  清零分配次数: %zu\n", allocator->stats.calloc_count);
    printf("  分配失败次数: %zu\n", allocator->stats.allocation_failures);
    printf("  内存开销: %zu 字节\n", allocator->stats.memory_overhead);
    printf("  边界违规次数: %zu\n", allocator->stats.boundary_violations);
    printf("  双重释放检测次数: %zu\n", allocator->stats.double_free_detections);
    printf("  内存损坏检测次数: %zu\n", allocator->stats.corruption_detections);
    printf("  泄漏次数: %zu\n", allocator->stats.leak_count);
    
    printf("\n分配跟踪信息:\n");
    printf("  当前跟踪的分配数量: %zu\n", allocator->allocation_count);
    
    debug_unlock(allocator);
}

void CN_debug_dump_leaks(Stru_CN_DebugAllocator_t* allocator)
{
    if (allocator == NULL)
    {
        return;
    }
    
    debug_lock(allocator);
    
    printf("\n=== 内存泄漏报告 ===\n");
    
    size_t leak_count = 0;
    size_t total_leak_size = 0;
    
    for (size_t i = 0; i < allocator->allocation_count; i++)
    {
        Stru_CN_DebugAllocationInfo_t* info = allocator->allocations[i];
        
        if (!info->is_freed)
        {
            leak_count++;
            total_leak_size += info->size;
            
            printf("泄漏 #%zu:\n", leak_count);
            printf("  分配ID: %llu\n", (unsigned long long)info->allocation_id);
            printf("  地址: %p\n", info->address);
            printf("  大小: %zu 字节\n", info->size);
            printf("  实际大小: %zu 字节\n", info->real_size);
            printf("  文件: %s\n", info->file ? info->file : "未知");
            printf("  行号: %d\n", info->line);
            printf("  目的: %s\n", info->purpose ? info->purpose : "未知");
            printf("  时间戳: %llu\n", (unsigned long long)info->timestamp);
            printf("\n");
        }
    }
    
    if (leak_count == 0)
    {
        printf("未检测到内存泄漏。\n");
    }
    else
    {
        printf("总计: %zu 个泄漏, 总大小: %zu 字节\n", leak_count, total_leak_size);
    }
    
    debug_unlock(allocator);
}

void CN_debug_dump_all_allocations(Stru_CN_DebugAllocator_t* allocator)
{
    if (allocator == NULL)
    {
        return;
    }
    
    debug_lock(allocator);
    
    printf("\n=== 所有分配记录 ===\n");
    
    for (size_t i = 0; i < allocator->allocation_count; i++)
    {
        Stru_CN_DebugAllocationInfo_t* info = allocator->allocations[i];
        
        printf("分配 #%zu:\n", i + 1);
        printf("  分配ID: %llu\n", (unsigned long long)info->allocation_id);
        printf("  地址: %p\n", info->address);
        printf("  大小: %zu 字节\n", info->size);
        printf("  实际大小: %zu 字节\n", info->real_size);
        printf("  文件: %s\n", info->file ? info->file : "未知");
        printf("  行号: %d\n", info->line);
        printf("  目的: %s\n", info->purpose ? info->purpose : "未知");
        printf("  时间戳: %llu\n", (unsigned long long)info->timestamp);
        printf("  状态: %s\n", info->is_freed ? "已释放" : "未释放");
        printf("\n");
    }
    
    printf("总计: %zu 个分配记录\n", allocator->allocation_count);
    
    debug_unlock(allocator);
}

void CN_debug_set_debug_callback(Stru_CN_DebugAllocator_t* allocator, 
                                 CN_DebugCallback_t callback, 
                                 void* user_data)
{
    if (allocator == NULL)
    {
        return;
    }
    
    debug_lock(allocator);
    
    allocator->debug_callback = callback;
    allocator->debug_user_data = user_data;
    
    debug_unlock(allocator);
    
    debug_output(allocator, "[DEBUG INFO] 调试回调已设置\n");
}

// ============================================================================
// 辅助函数实现
// ============================================================================

// 全局默认调试分配器实例
static Stru_CN_DebugAllocator_t* g_default_debug_allocator = NULL;

Stru_CN_DebugAllocator_t* CN_debug_get_default(void)
{
    if (g_default_debug_allocator == NULL)
    {
        // 使用默认配置创建调试分配器
        Stru_CN_DebugConfig_t config = CN_DEBUG_CONFIG_DEFAULT;
        g_default_debug_allocator = CN_debug_create(&config);
        
        if (g_default_debug_allocator != NULL)
        {
            printf("[DEBUG INFO] 默认调试分配器已创建\n");
        }
    }
    
    return g_default_debug_allocator;
}

void CN_debug_release_default(void)
{
    if (g_default_debug_allocator != NULL)
    {
        CN_debug_destroy(g_default_debug_allocator);
        g_default_debug_allocator = NULL;
        printf("[DEBUG INFO] 默认调试分配器已释放\n");
    }
}
