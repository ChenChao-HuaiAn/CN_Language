/******************************************************************************
 * 文件名: CN_dma_cache.c
 * 功能: CN_Language DMA内存分配器缓存一致性管理
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建DMA内存分配器缓存一致性管理，支持DMA同步操作
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_dma_internal.h"

// ============================================================================
// 缓存一致性函数实现
// ============================================================================

/**
 * @brief 根据内存属性设置缓存一致性标志
 */
bool CN_dma_determine_cache_coherence(Eum_CN_DmaMemoryAttribute_t attribute)
{
    switch (attribute)
    {
        case Eum_DMA_ATTRIBUTE_UNCACHED:
        case Eum_DMA_ATTRIBUTE_DEVICE:
            return false; // 不可缓存，不需要缓存一致性
        
        case Eum_DMA_ATTRIBUTE_WRITE_COMBINE:
        case Eum_DMA_ATTRIBUTE_WRITE_THROUGH:
        case Eum_DMA_ATTRIBUTE_WRITE_BACK:
        case Eum_DMA_ATTRIBUTE_NORMAL:
        case Eum_DMA_ATTRIBUTE_NONE:
        default:
            return true; // 可缓存，需要缓存一致性
    }
}

// ============================================================================
// DMA同步函数实现
// ============================================================================

void CN_dma_sync_to_device(Stru_CN_DmaBuffer_t* buffer,
                           size_t offset, size_t size)
{
    if (buffer == NULL)
    {
        return;
    }
    
    // 简化实现：对于不可缓存内存，不需要同步
    // 实际实现应该调用平台特定的缓存刷新指令
    if (buffer->attribute == Eum_DMA_ATTRIBUTE_UNCACHED ||
        buffer->attribute == Eum_DMA_ATTRIBUTE_DEVICE)
    {
        return;
    }
    
    // 实际实现应该在这里刷新CPU缓存
    // 例如：__builtin___clear_cache() 或平台特定的指令
    
    // 更新统计信息（如果有分配器上下文）
    // 注意：这里简化实现，实际应该通过分配器获取统计信息
}

void CN_dma_sync_from_device(Stru_CN_DmaBuffer_t* buffer,
                             size_t offset, size_t size)
{
    if (buffer == NULL)
    {
        return;
    }
    
    // 简化实现：对于不可缓存内存，不需要同步
    // 实际实现应该调用平台特定的缓存失效指令
    if (buffer->attribute == Eum_DMA_ATTRIBUTE_UNCACHED ||
        buffer->attribute == Eum_DMA_ATTRIBUTE_DEVICE)
    {
        return;
    }
    
    // 实际实现应该在这里失效CPU缓存
    // 例如：__builtin___clear_cache() 或平台特定的指令
    
    // 更新统计信息（如果有分配器上下文）
    // 注意：这里简化实现，实际应该通过分配器获取统计信息
}

void CN_dma_sync_sg_to_device(Stru_CN_DmaScatterGatherList_t* sg_list)
{
    if (sg_list == NULL || sg_list->elements == NULL)
    {
        return;
    }
    
    // 简化实现：同步所有元素
    for (size_t i = 0; i < sg_list->element_count; i++)
    {
        // 实际实现应该为每个元素调用CN_dma_sync_to_device
    }
}

void CN_dma_sync_sg_from_device(Stru_CN_DmaScatterGatherList_t* sg_list)
{
    if (sg_list == NULL || sg_list->elements == NULL)
    {
        return;
    }
    
    // 简化实现：同步所有元素
    for (size_t i = 0; i < sg_list->element_count; i++)
    {
        // 实际实现应该为每个元素调用CN_dma_sync_from_device
    }
}
