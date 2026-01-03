/******************************************************************************
 * 文件名: CN_virtual_memory_api.c
 * 功能: CN_Language虚拟内存管理器API实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建虚拟内存管理器API实现
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_virtual_memory_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>

// ============================================================================
// 虚拟地址空间管理API实现
// ============================================================================

/**
 * @brief 分配虚拟地址空间
 */
uintptr_t CN_virtual_memory_alloc(Stru_CN_VirtualMemoryManager_t* manager,
                                  size_t size,
                                  size_t alignment,
                                  Eum_CN_PageMappingFlags_t flags,
                                  Eum_CN_PageProtection_t protection)
{
    if (!manager || size == 0)
    {
        CN_LOG_ERROR("无效参数: manager=%p, size=%zu", manager, size);
        return 0;
    }
    
    // 使用默认对齐如果未指定
    if (alignment == 0)
    {
        alignment = manager->config.page_size;
    }
    
    // 检查对齐要求
    if (alignment % manager->config.page_size != 0)
    {
        CN_LOG_ERROR("对齐要求必须是页面大小的倍数: 对齐=%zu, 页面大小=%zu", 
                    alignment, manager->config.page_size);
        return 0;
    }
    
    // 计算需要的页面数量
    size_t page_count = CN_virtual_memory_size_to_pages(size, manager->config.page_size);
    
    // 查找可用的虚拟地址空间
    uintptr_t virtual_address = manager->next_virtual_address;
    
    // 对齐地址
    if (alignment > manager->config.page_size)
    {
        virtual_address = CN_virtual_memory_align_to_page(virtual_address, alignment);
    }
    
    // 检查地址范围是否有效
    if (virtual_address + size > manager->config.virtual_address_end)
    {
        CN_LOG_ERROR("虚拟地址空间不足: 请求大小=%zu, 可用空间=%zu", 
                    size, manager->config.virtual_address_end - manager->next_virtual_address);
        return 0;
    }
    
    // 检查地址是否已被使用
    int existing_mapping = CN_virtual_memory_find_mapping(manager, virtual_address);
    if (existing_mapping >= 0)
    {
        CN_LOG_ERROR("虚拟地址已被使用: 0x%p", (void*)virtual_address);
        return 0;
    }
    
    // 更新下一个可用地址
    manager->next_virtual_address = virtual_address + size;
    
    // 如果标志包含立即提交，分配物理内存
    if (flags & Eum_PAGE_MAPPING_COMMIT)
    {
        // 分配物理内存
        uintptr_t physical_address = CN_virtual_memory_alloc_physical_pages(manager, page_count);
        if (physical_address == 0)
        {
            CN_LOG_ERROR("分配物理内存失败");
            // 回滚虚拟地址分配
            manager->next_virtual_address = virtual_address;
            return 0;
        }
        
        // 创建映射
        Stru_CN_VirtualMapping_t mapping = {
            .virtual_address = virtual_address,
            .physical_address = physical_address,
            .page_count = page_count,
            .protection = protection,
            .flags = flags,
            .timestamp = 0,
            .purpose = "CN_virtual_memory_alloc"
        };
        
        // 添加映射
        if (!CN_virtual_memory_add_mapping(manager, &mapping))
        {
            CN_LOG_ERROR("添加虚拟地址映射失败");
            CN_virtual_memory_free_physical_pages(manager, physical_address, page_count);
            manager->next_virtual_address = virtual_address;
            return 0;
        }
        
        // 更新页表
        if (!CN_virtual_memory_update_page_table_entry(manager, virtual_address, 
                                                      physical_address, protection, flags))
        {
            CN_LOG_ERROR("更新页表项失败");
            CN_virtual_memory_remove_mapping(manager, virtual_address, page_count);
            CN_virtual_memory_free_physical_pages(manager, physical_address, page_count);
            manager->next_virtual_address = virtual_address;
            return 0;
        }
    }
    else if (flags & Eum_PAGE_MAPPING_RESERVE)
    {
        // 仅保留虚拟地址空间，不分配物理内存
        // 更新统计信息
        manager->stats.reserved_pages += page_count;
        
        CN_LOG_DEBUG("保留虚拟地址空间: 地址=0x%p, 大小=%zu", 
                    (void*)virtual_address, size);
    }
    
    CN_LOG_INFO("分配虚拟地址空间: 地址=0x%p, 大小=%zu, 对齐=%zu", 
                (void*)virtual_address, size, alignment);
    
    return virtual_address;
}

/**
 * @brief 在指定地址分配虚拟地址空间
 */
uintptr_t CN_virtual_memory_alloc_at(Stru_CN_VirtualMemoryManager_t* manager,
                                     uintptr_t virtual_address,
                                     size_t size,
                                     Eum_CN_PageMappingFlags_t flags,
                                     Eum_CN_PageProtection_t protection)
{
    if (!manager || virtual_address == 0 || size == 0)
    {
        CN_LOG_ERROR("无效参数: manager=%p, address=0x%p, size=%zu", 
                    manager, (void*)virtual_address, size);
        return 0;
    }
    
    // 检查地址对齐
    if (!CN_virtual_memory_is_page_aligned(virtual_address, manager->config.page_size))
    {
        CN_LOG_ERROR("虚拟地址未页面对齐: 0x%p, 页面大小=%zu", 
                    (void*)virtual_address, manager->config.page_size);
        return 0;
    }
    
    // 检查地址范围是否有效
    if (virtual_address + size > manager->config.virtual_address_end ||
        virtual_address < manager->config.virtual_address_start)
    {
        CN_LOG_ERROR("虚拟地址超出范围: 地址=0x%p, 大小=%zu, 范围=0x%p-0x%p", 
                    (void*)virtual_address, size,
                    (void*)manager->config.virtual_address_start,
                    (void*)manager->config.virtual_address_end);
        return 0;
    }
    
    // 检查地址是否已被使用
    int existing_mapping = CN_virtual_memory_find_mapping(manager, virtual_address);
    if (existing_mapping >= 0)
    {
        CN_LOG_ERROR("虚拟地址已被使用: 0x%p", (void*)virtual_address);
        return 0;
    }
    
    // 计算需要的页面数量
    size_t page_count = CN_virtual_memory_size_to_pages(size, manager->config.page_size);
    
    // 如果标志包含立即提交，分配物理内存
    if (flags & Eum_PAGE_MAPPING_COMMIT)
    {
        // 分配物理内存
        uintptr_t physical_address = CN_virtual_memory_alloc_physical_pages(manager, page_count);
        if (physical_address == 0)
        {
            CN_LOG_ERROR("分配物理内存失败");
            return 0;
        }
        
        // 创建映射
        Stru_CN_VirtualMapping_t mapping = {
            .virtual_address = virtual_address,
            .physical_address = physical_address,
            .page_count = page_count,
            .protection = protection,
            .flags = flags,
            .timestamp = 0,
            .purpose = "CN_virtual_memory_alloc_at"
        };
        
        // 添加映射
        if (!CN_virtual_memory_add_mapping(manager, &mapping))
        {
            CN_LOG_ERROR("添加虚拟地址映射失败");
            CN_virtual_memory_free_physical_pages(manager, physical_address, page_count);
            return 0;
        }
        
        // 更新页表
        if (!CN_virtual_memory_update_page_table_entry(manager, virtual_address, 
                                                      physical_address, protection, flags))
        {
            CN_LOG_ERROR("更新页表项失败");
            CN_virtual_memory_remove_mapping(manager, virtual_address, page_count);
            CN_virtual_memory_free_physical_pages(manager, physical_address, page_count);
            return 0;
        }
    }
    else if (flags & Eum_PAGE_MAPPING_RESERVE)
    {
        // 仅保留虚拟地址空间，不分配物理内存
        // 更新统计信息
        manager->stats.reserved_pages += page_count;
        
        CN_LOG_DEBUG("在指定地址保留虚拟地址空间: 地址=0x%p, 大小=%zu", 
                    (void*)virtual_address, size);
    }
    
    // 更新下一个可用地址（如果这个地址比当前的大）
    if (virtual_address + size > manager->next_virtual_address)
    {
        manager->next_virtual_address = virtual_address + size;
    }
    
    CN_LOG_INFO("在指定地址分配虚拟地址空间: 地址=0x%p, 大小=%zu", 
                (void*)virtual_address, size);
    
    return virtual_address;
}

/**
 * @brief 释放虚拟地址空间
 */
void CN_virtual_memory_free(Stru_CN_VirtualMemoryManager_t* manager,
                            uintptr_t virtual_address,
                            size_t size)
{
    if (!manager || virtual_address == 0 || size == 0)
    {
        CN_LOG_ERROR("无效参数: manager=%p, address=0x%p, size=%zu", 
                    manager, (void*)virtual_address, size);
        return;
    }
    
    // 检查地址对齐
    if (!CN_virtual_memory_is_page_aligned(virtual_address, manager->config.page_size))
    {
        CN_LOG_ERROR("虚拟地址未页面对齐: 0x%p", (void*)virtual_address);
        return;
    }
    
    // 计算页面数量
    size_t page_count = CN_virtual_memory_size_to_pages(size, manager->config.page_size);
    
    // 查找映射
    int mapping_index = CN_virtual_memory_find_mapping(manager, virtual_address);
    if (mapping_index >= 0)
    {
        // 获取映射信息
        Stru_CN_VirtualMapping_t* mapping = 
            (Stru_CN_VirtualMapping_t*)CN_array_get(manager->virtual_mappings, (size_t)mapping_index);
        
        if (mapping)
        {
            // 释放物理内存
            CN_virtual_memory_free_physical_pages(manager, mapping->physical_address, mapping->page_count);
            
            // 清除页表项
            CN_virtual_memory_clear_page_table_entry(manager, virtual_address);
            
            // 移除映射
            CN_virtual_memory_remove_mapping(manager, virtual_address, mapping->page_count);
        }
    }
    else
    {
        // 没有找到映射，可能是仅保留的地址空间
        // 更新统计信息
        if (manager->stats.reserved_pages >= page_count)
        {
            manager->stats.reserved_pages -= page_count;
        }
        else
        {
            manager->stats.reserved_pages = 0;
        }
        
        CN_LOG_DEBUG("释放保留的虚拟地址空间: 地址=0x%p, 大小=%zu", 
                    (void*)virtual_address, size);
    }
    
    // 如果释放的地址在下一个可用地址之前，可以调整下一个可用地址
    // 但这是一个复杂的内存碎片问题，这里简化处理
    
    CN_LOG_INFO("释放虚拟地址空间: 地址=0x%p, 大小=%zu", 
                (void*)virtual_address, size);
}

/**
 * @brief 保留虚拟地址空间
 */
uintptr_t CN_virtual_memory_reserve(Stru_CN_VirtualMemoryManager_t* manager,
                                    size_t size,
                                    size_t alignment)
{
    // 使用分配函数，但只设置保留标志
    return CN_virtual_memory_alloc(manager, size, alignment, 
                                   Eum_PAGE_MAPPING_RESERVE, 
                                   Eum_PAGE_PROTECTION_NONE);
}

/**
 * @brief 提交物理内存到虚拟地址空间
 */
bool CN_virtual_memory_commit(Stru_CN_VirtualMemoryManager_t* manager,
                              uintptr_t virtual_address,
                              size_t size,
                              Eum_CN_PageProtection_t protection)
{
    if (!manager || virtual_address == 0 || size == 0)
    {
        CN_LOG_ERROR("无效参数: manager=%p, address=0x%p, size=%zu", 
                    manager, (void*)virtual_address, size);
        return false;
    }
    
    // 检查地址对齐
    if (!CN_virtual_memory_is_page_aligned(virtual_address, manager->config.page_size))
    {
        CN_LOG_ERROR("虚拟地址未页面对齐: 0x%p", (void*)virtual_address);
        return false;
    }
    
    // 计算页面数量
    size_t page_count = CN_virtual_memory_size_to_pages(size, manager->config.page_size);
    
    // 检查是否已映射
    int mapping_index = CN_virtual_memory_find_mapping(manager, virtual_address);
    if (mapping_index >= 0)
    {
        CN_LOG_ERROR("虚拟地址已映射: 0x%p", (void*)virtual_address);
        return false;
    }
    
    // 分配物理内存
    uintptr_t physical_address = CN_virtual_memory_alloc_physical_pages(manager, page_count);
    if (physical_address == 0)
    {
        CN_LOG_ERROR("分配物理内存失败");
        return false;
    }
    
    // 创建映射
    Stru_CN_VirtualMapping_t mapping = {
        .virtual_address = virtual_address,
        .physical_address = physical_address,
        .page_count = page_count,
        .protection = protection,
        .flags = Eum_PAGE_MAPPING_COMMIT,
        .timestamp = 0,
        .purpose = "CN_virtual_memory_commit"
    };
    
    // 添加映射
    if (!CN_virtual_memory_add_mapping(manager, &mapping))
    {
        CN_LOG_ERROR("添加虚拟地址映射失败");
        CN_virtual_memory_free_physical_pages(manager, physical_address, page_count);
        return false;
    }
    
    // 更新页表
    if (!CN_virtual_memory_update_page_table_entry(manager, virtual_address, 
                                                  physical_address, protection, 
                                                  Eum_PAGE_MAPPING_COMMIT))
    {
        CN_LOG_ERROR("更新页表项失败");
        CN_virtual_memory_remove_mapping(manager, virtual_address, page_count);
        CN_virtual_memory_free_physical_pages(manager, physical_address, page_count);
        return false;
    }
    
    // 更新统计信息
    if (manager->stats.reserved_pages >= page_count)
    {
        manager->stats.reserved_pages -= page_count;
    }
    
    CN_LOG_INFO("提交物理内存到虚拟地址空间: 虚拟=0x%p -> 物理=0x%p, 大小=%zu", 
                (void*)virtual_address, (void*)physical_address, size);
    
    return true;
}

/**
 * @brief 取消提交物理内存
 */
void CN_virtual_memory_decommit(Stru_CN_VirtualMemoryManager_t* manager,
                                uintptr_t virtual_address,
                                size_t size)
{
    if (!manager || virtual_address == 0 || size == 0)
    {
        CN_LOG_ERROR("无效参数: manager=%p, address=0x%p, size=%zu", 
                    manager, (void*)virtual_address, size);
        return;
    }
    
    // 检查地址对齐
    if (!CN_virtual_memory_is_page_aligned(virtual_address, manager->config.page_size))
    {
        CN_LOG_ERROR("虚拟地址未页面对齐: 0x%p", (void*)virtual_address);
        return;
    }
    
    // 计算页面数量
    size_t page_count = CN_virtual_memory_size_to_pages(size, manager->config.page_size);
    
    // 查找映射
    int mapping_index = CN_virtual_memory_find_mapping(manager, virtual_address);
    if (mapping_index >= 0)
    {
        // 获取映射信息
        Stru_CN_VirtualMapping_t* mapping = 
            (Stru_CN_VirtualMapping_t*)CN_array_get(manager->virtual_mappings, (size_t)mapping_index);
        
        if (mapping)
        {
            // 释放物理内存
            CN_virtual_memory_free_physical_pages(manager, mapping->physical_address, mapping->page_count);
            
            // 清除页表项
            CN_virtual_memory_clear_page_table_entry(manager, virtual_address);
            
            // 移除映射
            CN_virtual_memory_remove_mapping(manager, virtual_address, mapping->page_count);
            
            // 更新统计信息（变为保留状态）
            manager->stats.reserved_pages += mapping->page_count;
            
            CN_LOG_INFO("取消提交物理内存: 地址=0x%p, 大小=%zu", 
                        (void*)virtual_address, size);
        }
    }
    else
    {
        CN_LOG_WARN("未找到虚拟地址映射: 0x%p", (void*)virtual_address);
    }
}

// ============================================================================
// 页面映射API实现
// ============================================================================

/**
 * @brief 映射物理内存到虚拟地址空间
 */
bool CN_virtual_memory_map_physical(Stru_CN_VirtualMemoryManager_t* manager,
                                    uintptr_t virtual_address,
                                    uintptr_t physical_address,
                                    size_t size,
                                    Eum_CN_PageProtection_t protection,
                                    Eum_CN_PageMappingFlags_t flags)
{
    if (!manager || virtual_address == 0 || physical_address == 0 || size == 0)
    {
        CN_LOG_ERROR("无效参数: manager=%p, 虚拟=0x%p, 物理=0x%p, 大小=%zu", 
                    manager, (void*)virtual_address, (void*)physical_address, size);
        return false;
    }
    
    // 检查地址对齐
    if (!CN_virtual_memory_is_page_aligned(virtual_address, manager->config.page_size) ||
        !CN_virtual_memory_is_page_aligned(physical_address, manager->config.page_size))
    {
        CN_LOG_ERROR("地址未页面对齐: 虚拟=0x%p, 物理=0x%p, 页面大小=%zu", 
                    (void*)virtual_address, (void*)physical_address, manager->config.page_size);
        return false;
    }
    
    // 检查虚拟地址范围是否有效
    if (virtual_address + size > manager->config.virtual_address_end ||
        virtual_address < manager->config.virtual_address_start)
    {
        CN_LOG_ERROR("虚拟地址超出范围: 地址=0x%p, 大小=%zu, 范围=0x%p-0x%p", 
                    (void*)virtual_address, size,
                    (void*)manager->config.virtual_address_start,
                    (void*)manager->config.virtual_address_end);
        return false;
    }
    
    // 检查虚拟地址是否已被使用
    int existing_mapping = CN_virtual_memory_find_mapping(manager, virtual_address);
    if (existing_mapping >= 0)
    {
        CN_LOG_ERROR("虚拟地址已被使用: 0x%p", (void*)virtual_address);
        return false;
    }
    
    // 计算需要的页面数量
    size_t page_count = CN_virtual_memory_size_to_pages(size, manager->config.page_size);
    
    // 创建映射
    Stru_CN_VirtualMapping_t mapping = {
        .virtual_address = virtual_address,
        .physical_address = physical_address,
        .page_count = page_count,
        .protection = protection,
        .flags = flags,
        .timestamp = 0,
        .purpose = "CN_virtual_memory_map_physical"
    };
    
    // 添加映射
    if (!CN_virtual_memory_add_mapping(manager, &mapping))
    {
        CN_LOG_ERROR("添加虚拟地址映射失败");
        return false;
    }
    
    // 更新页表
    if (!CN_virtual_memory_update_page_table_entry(manager, virtual_address, 
                                                  physical_address, protection, flags))
    {
        CN_LOG_ERROR("更新页表项失败");
        CN_virtual_memory_remove_mapping(manager, virtual_address, page_count);
        return false;
    }
    
    // 更新统计信息
    manager->stats.mapped_pages += page_count;
    
    CN_LOG_INFO("映射物理内存到虚拟地址空间: 虚拟=0x%p -> 物理=0x%p, 大小=%zu, 保护=%u", 
                (void*)virtual_address, (void*)physical_address, size, protection);
    
    return true;
}

/**
 * @brief 取消映射虚拟地址空间
 */
void CN_virtual_memory_unmap(Stru_CN_VirtualMemoryManager_t* manager,
                             uintptr_t virtual_address,
                             size_t size)
{
    if (!manager || virtual_address == 0 || size == 0)
    {
        CN_LOG_ERROR("无效参数: manager=%p, address=0x%p, size=%zu", 
                    manager, (void*)virtual_address, size);
        return;
    }
    
    // 检查地址对齐
    if (!CN_virtual_memory_is_page_aligned(virtual_address, manager->config.page_size))
    {
        CN_LOG_ERROR("虚拟地址未页面对齐: 0x%p", (void*)virtual_address);
        return;
    }
    
    // 计算页面数量
    size_t page_count = CN_virtual_memory_size_to_pages(size, manager->config.page_size);
    
    // 查找映射
    int mapping_index = CN_virtual_memory_find_mapping(manager, virtual_address);
    if (mapping_index >= 0)
    {
        // 获取映射信息
        Stru_CN_VirtualMapping_t* mapping = 
            (Stru_CN_VirtualMapping_t*)CN_array_get(manager->virtual_mappings, (size_t)mapping_index);
        
        if (mapping)
        {
            // 释放物理内存（如果不是共享映射）
            if (!(mapping->flags & Eum_PAGE_MAPPING_SHARED))
            {
                CN_virtual_memory_free_physical_pages(manager, mapping->physical_address, mapping->page_count);
            }
            
            // 清除页表项
            CN_virtual_memory_clear_page_table_entry(manager, virtual_address);
            
            // 移除映射
            CN_virtual_memory_remove_mapping(manager, virtual_address, mapping->page_count);
            
            // 更新统计信息
            if (mapping->flags & Eum_PAGE_MAPPING_SHARED)
            {
                if (manager->stats.shared_mappings > 0)
                {
                    manager->stats.shared_mappings--;
                }
            }
            else
            {
                if (manager->stats.private_mappings > 0)
                {
                    manager->stats.private_mappings--;
                }
            }
            
            CN_LOG_INFO("取消映射虚拟地址空间: 地址=0x%p, 大小=%zu", 
                        (void*)virtual_address, size);
        }
    }
    else
    {
        CN_LOG_WARN("未找到虚拟地址映射: 0x%p", (void*)virtual_address);
    }
}

/**
 * @brief 重新映射虚拟地址空间（更改保护标志）
 */
bool CN_virtual_memory_remap(Stru_CN_VirtualMemoryManager_t* manager,
                             uintptr_t virtual_address,
                             size_t size,
                             Eum_CN_PageProtection_t new_protection)
{
    if (!manager || virtual_address == 0 || size == 0)
    {
        CN_LOG_ERROR("无效参数: manager=%p, address=0x%p, size=%zu", 
                    manager, (void*)virtual_address, size);
        return false;
    }
    
    // 检查地址对齐
    if (!CN_virtual_memory_is_page_aligned(virtual_address, manager->config.page_size))
    {
        CN_LOG_ERROR("虚拟地址未页面对齐: 0x%p", (void*)virtual_address);
        return false;
    }
    
    // 计算页面数量
    size_t page_count = CN_virtual_memory_size_to_pages(size, manager->config.page_size);
    
    // 查找映射
    int mapping_index = CN_virtual_memory_find_mapping(manager, virtual_address);
    if (mapping_index < 0)
    {
        CN_LOG_ERROR("未找到虚拟地址映射: 0x%p", (void*)virtual_address);
        return false;
    }
    
    // 获取映射信息
    Stru_CN_VirtualMapping_t* mapping = 
        (Stru_CN_VirtualMapping_t*)CN_array_get(manager->virtual_mappings, (size_t)mapping_index);
    
    if (!mapping)
    {
        CN_LOG_ERROR("获取映射信息失败");
        return false;
    }
    
    // 检查大小是否匹配
    if (mapping->page_count < page_count)
    {
        CN_LOG_ERROR("重新映射大小超出原始映射范围: 原始=%zu页, 请求=%zu页", 
                    mapping->page_count, page_count);
        return false;
    }
    
    // 更新页表项的保护标志
    if (!CN_virtual_memory_update_page_table_entry(manager, virtual_address, 
                                                  mapping->physical_address, new_protection, mapping->flags))
    {
        CN_LOG_ERROR("更新页表项保护标志失败");
        return false;
    }
    
    // 更新映射的保护标志
    mapping->protection = new_protection;
    
    CN_LOG_INFO("重新映射虚拟地址空间: 地址=0x%p, 大小=%zu, 新保护=%u", 
                (void*)virtual_address, size, new_protection);
    
    return true;
}

/**
 * @brief 查询虚拟地址的映射信息
 */
bool CN_virtual_memory_query(Stru_CN_VirtualMemoryManager_t* manager,
                             uintptr_t virtual_address,
                             uintptr_t* physical_address,
                             Eum_CN_PageProtection_t* protection,
                             Eum_CN_PageMappingFlags_t* flags)
{
    if (!manager || virtual_address == 0)
    {
        CN_LOG_ERROR("无效参数: manager=%p, address=0x%p", 
                    manager, (void*)virtual_address);
        return false;
    }
    
    // 检查地址对齐
    if (!CN_virtual_memory_is_page_aligned(virtual_address, manager->config.page_size))
    {
        CN_LOG_ERROR("虚拟地址未页面对齐: 0x%p", (void*)virtual_address);
        return false;
    }
    
    // 查找映射
    int mapping_index = CN_virtual_memory_find_mapping(manager, virtual_address);
    if (mapping_index < 0)
    {
        // 检查是否是保留的地址空间
        // 这里简化处理：如果地址在虚拟地址范围内且未被映射，则认为是保留的
        if (virtual_address >= manager->config.virtual_address_start &&
            virtual_address < manager->config.virtual_address_end)
        {
            if (physical_address) *physical_address = 0;
            if (protection) *protection = Eum_PAGE_PROTECTION_NONE;
            if (flags) *flags = Eum_PAGE_MAPPING_RESERVE;
            return true;
        }
        
        return false;
    }
    
    // 获取映射信息
    Stru_CN_VirtualMapping_t* mapping = 
        (Stru_CN_VirtualMapping_t*)CN_array_get(manager->virtual_mappings, (size_t)mapping_index);
    
    if (!mapping)
    {
        CN_LOG_ERROR("获取映射信息失败");
        return false;
    }
    
    // 返回映射信息
    if (physical_address) *physical_address = mapping->physical_address;
    if (protection) *protection = mapping->protection;
    if (flags) *flags = mapping->flags;
    
    return true;
}
