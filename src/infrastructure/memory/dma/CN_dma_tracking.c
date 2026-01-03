/******************************************************************************
 * 文件名: CN_dma_tracking.c
 * 功能: CN_Language DMA内存分配器分配跟踪
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建DMA内存分配器分配跟踪，支持泄漏检测
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_dma_internal.h"
#include <string.h>

// ============================================================================
// 分配跟踪函数实现
// ============================================================================

/**
 * @brief 添加分配跟踪记录
 */
void CN_dma_add_allocation_tracking(Stru_CN_DmaAllocator_t* allocator,
                                   Stru_CN_DmaBuffer_t* buffer,
                                   const char* file, int line, const char* purpose)
{
    if (!allocator->config.track_allocations || allocator->allocations == NULL)
    {
        return;
    }
    
    if (allocator->current_allocations >= allocator->max_allocations)
    {
        // 分配跟踪数组已满，忽略新记录
        CN_dma_debug_log(allocator, "警告：分配跟踪数组已满，无法跟踪新分配");
        return;
    }
    
    Stru_CN_DmaAllocationInfo_t* info = &allocator->allocations[allocator->current_allocations];
    info->physical_address = buffer->physical_address;
    info->virtual_address = buffer->virtual_address;
    info->size = buffer->size;
    info->alignment = buffer->alignment;
    info->attribute = buffer->attribute;
    info->file = file;
    info->line = line;
    info->timestamp = 0; // 实际实现应该使用时间戳
    info->purpose = purpose;
    info->handle = buffer->handle;
    
    allocator->current_allocations++;
}

/**
 * @brief 移除分配跟踪记录
 */
void CN_dma_remove_allocation_tracking(Stru_CN_DmaAllocator_t* allocator,
                                      uint32_t handle)
{
    if (!allocator->config.track_allocations || allocator->allocations == NULL)
    {
        return;
    }
    
    for (size_t i = 0; i < allocator->current_allocations; i++)
    {
        if (allocator->allocations[i].handle == handle)
        {
            // 将最后一个元素移动到当前位置
            if (i < allocator->current_allocations - 1)
            {
                allocator->allocations[i] = allocator->allocations[allocator->current_allocations - 1];
            }
            allocator->current_allocations--;
            break;
        }
    }
}
