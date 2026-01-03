/******************************************************************************
 * 文件名: CN_virtual_memory_operations.c
 * 功能: CN_Language虚拟内存管理器操作函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建虚拟内存管理器操作函数实现
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_virtual_memory_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>

// ============================================================================
// 页表管理接口实现
// ============================================================================

/**
 * @brief 获取页表基地址
 */
uintptr_t CN_virtual_memory_get_page_table_base(Stru_CN_VirtualMemoryManager_t* manager)
{
    if (!manager)
    {
        CN_LOG_ERROR("无效参数: manager=NULL");
        return 0;
    }
    
    if (!manager->mmu_interface || !manager->mmu_interface->get_page_table_base)
    {
        CN_LOG_ERROR("MMU接口不支持获取页表基地址");
        return 0;
    }
    
    return manager->mmu_interface->get_page_table_base();
}

/**
 * @brief 设置页表基地址
 */
void CN_virtual_memory_set_page_table_base(Stru_CN_VirtualMemoryManager_t* manager,
                                           uintptr_t page_table_base)
{
    if (!manager)
    {
        CN_LOG_ERROR("无效参数: manager=NULL");
        return;
    }
    
    if (!manager->mmu_interface || !manager->mmu_interface->set_page_table_base)
    {
        CN_LOG_ERROR("MMU接口不支持设置页表基地址");
        return;
    }
    
    manager->mmu_interface->set_page_table_base(page_table_base);
    
    CN_LOG_INFO("设置页表基地址: 0x%p", (void*)page_table_base);
}

/**
 * @brief 刷新TLB（转换后备缓冲区）
 */
void CN_virtual_memory_flush_tlb(Stru_CN_VirtualMemoryManager_t* manager,
                                 uintptr_t virtual_address)
{
    if (!manager)
    {
        CN_LOG_ERROR("无效参数: manager=NULL");
        return;
    }
    
    if (!manager->mmu_interface || !manager->mmu_interface->flush_tlb)
    {
        CN_LOG_ERROR("MMU接口不支持刷新TLB");
        return;
    }
    
    manager->mmu_interface->flush_tlb(virtual_address);
    
    // 更新统计信息
    manager->stats.tlb_flushes++;
    
    if (virtual_address == 0)
    {
        CN_LOG_DEBUG("刷新全部TLB");
    }
    else
    {
        CN_LOG_DEBUG("刷新TLB条目: 地址=0x%p", (void*)virtual_address);
    }
}

// ============================================================================
// 统计和调试接口实现
// ============================================================================

/**
 * @brief 获取虚拟内存管理器统计信息
 */
bool CN_virtual_memory_get_stats(Stru_CN_VirtualMemoryManager_t* manager,
                                 Stru_CN_VirtualMemoryStats_t* stats)
{
    if (!manager || !stats)
    {
        CN_LOG_ERROR("无效参数: manager=%p, stats=%p", manager, stats);
        return false;
    }
    
    // 复制统计信息
    memcpy(stats, &manager->stats, sizeof(Stru_CN_VirtualMemoryStats_t));
    
    // 计算总虚拟页面数量
    stats->total_virtual_pages = (manager->config.virtual_address_end - 
                                 manager->config.virtual_address_start) / 
                                 manager->config.page_size;
    
    return true;
}

/**
 * @brief 重置虚拟内存管理器统计信息
 */
void CN_virtual_memory_reset_stats(Stru_CN_VirtualMemoryManager_t* manager)
{
    if (!manager)
    {
        CN_LOG_ERROR("无效参数: manager=NULL");
        return;
    }
    
    // 重置统计信息
    memset(&manager->stats, 0, sizeof(Stru_CN_VirtualMemoryStats_t));
    
    // 重新计算总虚拟页面数量
    manager->stats.total_virtual_pages = (manager->config.virtual_address_end - 
                                         manager->config.virtual_address_start) / 
                                         manager->config.page_size;
    
    CN_LOG_INFO("重置虚拟内存管理器统计信息");
}

/**
 * @brief 启用/禁用调试模式
 */
void CN_virtual_memory_enable_debug(Stru_CN_VirtualMemoryManager_t* manager, bool enable)
{
    if (!manager)
    {
        CN_LOG_ERROR("无效参数: manager=NULL");
        return;
    }
    
    manager->config.enable_debug = enable;
    
    if (enable)
    {
        CN_LOG_INFO("启用虚拟内存管理器调试模式");
    }
    else
    {
        CN_LOG_INFO("禁用虚拟内存管理器调试模式");
    }
}

/**
 * @brief 验证虚拟内存管理器完整性
 */
bool CN_virtual_memory_validate(Stru_CN_VirtualMemoryManager_t* manager)
{
    if (!manager)
    {
        CN_LOG_ERROR("无效参数: manager=NULL");
        return false;
    }
    
    // 检查配置有效性
    if (manager->config.virtual_address_start >= manager->config.virtual_address_end)
    {
        CN_LOG_ERROR("虚拟地址范围无效: 开始=0x%p, 结束=0x%p",
                    (void*)manager->config.virtual_address_start,
                    (void*)manager->config.virtual_address_end);
        return false;
    }
    
    if (manager->config.page_size == 0)
    {
        CN_LOG_ERROR("页面大小无效: %zu", manager->config.page_size);
        return false;
    }
    
    // 检查页表级数
    if (manager->config.page_table_levels < 1 || manager->config.page_table_levels > 4)
    {
        CN_LOG_ERROR("页表级数无效: %zu", manager->config.page_table_levels);
        return false;
    }
    
    // 检查MMU接口
    if (!manager->mmu_interface)
    {
        CN_LOG_ERROR("MMU接口未设置");
        return false;
    }
    
    // 检查页表
    if (!manager->page_tables || manager->page_table_count == 0)
    {
        CN_LOG_ERROR("页表未初始化");
        return false;
    }
    
    // 检查虚拟映射数组
    if (!manager->virtual_mappings)
    {
        CN_LOG_ERROR("虚拟映射数组未初始化");
        return false;
    }
    
    CN_LOG_DEBUG("虚拟内存管理器验证通过");
    return true;
}

/**
 * @brief 转储虚拟内存管理器状态到标准输出
 */
void CN_virtual_memory_dump(Stru_CN_VirtualMemoryManager_t* manager)
{
    if (!manager)
    {
        CN_LOG_ERROR("无效参数: manager=NULL");
        return;
    }
    
    CN_LOG_INFO("=== 虚拟内存管理器状态转储 ===");
    CN_LOG_INFO("名称: %s", manager->config.name);
    CN_LOG_INFO("虚拟地址范围: 0x%p - 0x%p",
                (void*)manager->config.virtual_address_start,
                (void*)manager->config.virtual_address_end);
    CN_LOG_INFO("页面大小: %zu 字节", manager->config.page_size);
    CN_LOG_INFO("页表级数: %zu", manager->config.page_table_levels);
    CN_LOG_INFO("大页面支持: %s", manager->config.enable_huge_pages ? "是" : "否");
    CN_LOG_INFO("统计信息: %s", manager->config.enable_statistics ? "启用" : "禁用");
    CN_LOG_INFO("调试模式: %s", manager->config.enable_debug ? "启用" : "禁用");
    CN_LOG_INFO("页面保护: %s", manager->config.enable_page_protection ? "启用" : "禁用");
    
    // 统计信息
    CN_LOG_INFO("=== 统计信息 ===");
    CN_LOG_INFO("总虚拟页面: %zu", manager->stats.total_virtual_pages);
    CN_LOG_INFO("已映射页面: %zu", manager->stats.mapped_pages);
    CN_LOG_INFO("保留页面: %zu", manager->stats.reserved_pages);
    CN_LOG_INFO("已提交页面: %zu", manager->stats.committed_pages);
    CN_LOG_INFO("页面错误: %zu", manager->stats.page_faults);
    CN_LOG_INFO("TLB未命中: %zu", manager->stats.tlb_misses);
    CN_LOG_INFO("TLB刷新: %zu", manager->stats.tlb_flushes);
    CN_LOG_INFO("页表分配: %zu", manager->stats.page_table_allocations);
    CN_LOG_INFO("页表释放: %zu", manager->stats.page_table_frees);
    CN_LOG_INFO("保护违规: %zu", manager->stats.protection_violations);
    CN_LOG_INFO("共享映射: %zu", manager->stats.shared_mappings);
    CN_LOG_INFO("私有映射: %zu", manager->stats.private_mappings);
    
    // 映射信息
    CN_LOG_INFO("=== 虚拟地址映射 ===");
    size_t mapping_count = CN_array_size(manager->virtual_mappings);
    CN_LOG_INFO("映射数量: %zu", mapping_count);
    
    for (size_t i = 0; i < mapping_count; i++)
    {
        Stru_CN_VirtualMapping_t* mapping = 
            (Stru_CN_VirtualMapping_t*)CN_array_get(manager->virtual_mappings, i);
        
        if (mapping)
        {
            CN_LOG_INFO("  映射 %zu: 虚拟=0x%p -> 物理=0x%p, 页面=%zu, 保护=%u, 标志=%u, 目的=%s",
                       i, (void*)mapping->virtual_address, (void*)mapping->physical_address,
                       mapping->page_count, mapping->protection, mapping->flags,
                       mapping->purpose);
        }
    }
    
    CN_LOG_INFO("=== 转储完成 ===");
}

/**
 * @brief 转储所有未释放的映射信息
 */
void CN_virtual_memory_dump_leaks(Stru_CN_VirtualMemoryManager_t* manager)
{
    if (!manager)
    {
        CN_LOG_ERROR("无效参数: manager=NULL");
        return;
    }
    
    size_t mapping_count = CN_array_size(manager->virtual_mappings);
    
    if (mapping_count == 0)
    {
        CN_LOG_INFO("没有未释放的映射");
        return;
    }
    
    CN_LOG_WARN("=== 未释放的映射（内存泄漏）===");
    CN_LOG_WARN("未释放的映射数量: %zu", mapping_count);
    
    for (size_t i = 0; i < mapping_count; i++)
    {
        Stru_CN_VirtualMapping_t* mapping = 
            (Stru_CN_VirtualMapping_t*)CN_array_get(manager->virtual_mappings, i);
        
        if (mapping)
        {
            CN_LOG_WARN("  泄漏 %zu: 虚拟=0x%p -> 物理=0x%p, 页面=%zu, 目的=%s",
                       i, (void*)mapping->virtual_address, (void*)mapping->physical_address,
                       mapping->page_count, mapping->purpose);
        }
    }
    
    CN_LOG_WARN("=== 泄漏转储完成 ===");
}

/**
 * @brief 处理页面错误
 */
bool CN_virtual_memory_handle_page_fault(Stru_CN_VirtualMemoryManager_t* manager,
                                         const Stru_CN_PageFaultInfo_t* fault_info)
{
    if (!manager || !fault_info)
    {
        CN_LOG_ERROR("无效参数: manager=%p, fault_info=%p", manager, fault_info);
        return false;
    }
    
    // 更新统计信息
    manager->stats.page_faults++;
    
    if (fault_info->is_protection_violation)
    {
        manager->stats.protection_violations++;
    }
    
    CN_LOG_DEBUG("处理页面错误: 地址=0x%p, 指令指针=0x%p, 写操作=%s, 用户模式=%s, 存在=%s, 保护违规=%s",
                (void*)fault_info->virtual_address,
                (void*)fault_info->instruction_pointer,
                fault_info->is_write ? "是" : "否",
                fault_info->is_user_mode ? "是" : "否",
                fault_info->is_present ? "是" : "否",
                fault_info->is_protection_violation ? "是" : "否");
    
    // 检查是否是保护页访问
    if (fault_info->is_protection_violation)
    {
        // 查找映射
        int mapping_index = CN_virtual_memory_find_mapping(manager, fault_info->virtual_address);
        if (mapping_index >= 0)
        {
            Stru_CN_VirtualMapping_t* mapping = 
                (Stru_CN_VirtualMapping_t*)CN_array_get(manager->virtual_mappings, (size_t)mapping_index);
            
            if (mapping && (mapping->protection & Eum_PAGE_PROTECTION_GUARD))
            {
                CN_LOG_WARN("保护页访问: 地址=0x%p, 目的=%s",
                           (void*)fault_info->virtual_address, mapping->purpose);
                return false; // 保护页访问应该被阻止
            }
        }
    }
    
    // 检查页面是否存在
    if (!fault_info->is_present)
    {
        // 按需分页：分配物理内存并建立映射
        uintptr_t virtual_address = fault_info->virtual_address;
        
        // 对齐到页面边界
        virtual_address = CN_virtual_memory_align_to_page(virtual_address, manager->config.page_size);
        
        // 检查是否在虚拟地址范围内
        if (virtual_address < manager->config.virtual_address_start ||
            virtual_address >= manager->config.virtual_address_end)
        {
            CN_LOG_ERROR("页面错误地址超出范围: 0x%p", (void*)virtual_address);
            return false;
        }
        
        // 分配物理内存
        uintptr_t physical_address = CN_virtual_memory_alloc_physical_pages(manager, 1);
        if (physical_address == 0)
        {
            CN_LOG_ERROR("按需分页分配物理内存失败");
            return false;
        }
        
        // 确定保护标志
        Eum_CN_PageProtection_t protection = Eum_PAGE_PROTECTION_READ_WRITE;
        if (fault_info->is_user_mode)
        {
            protection |= Eum_PAGE_PROTECTION_USER;
        }
        else
        {
            protection |= Eum_PAGE_PROTECTION_KERNEL;
        }
        
        // 创建映射
        Stru_CN_VirtualMapping_t mapping = {
            .virtual_address = virtual_address,
            .physical_address = physical_address,
            .page_count = 1,
            .protection = protection,
            .flags = Eum_PAGE_MAPPING_COMMIT | Eum_PAGE_MAPPING_LAZY,
            .timestamp = 0,
            .purpose = "按需分页"
        };
        
        // 添加映射
        if (!CN_virtual_memory_add_mapping(manager, &mapping))
        {
            CN_LOG_ERROR("添加按需分页映射失败");
            CN_virtual_memory_free_physical_pages(manager, physical_address, 1);
            return false;
        }
        
        // 更新页表
        if (!CN_virtual_memory_update_page_table_entry(manager, virtual_address, 
                                                      physical_address, protection, 
                                                      Eum_PAGE_MAPPING_COMMIT | Eum_PAGE_MAPPING_LAZY))
        {
            CN_LOG_ERROR("更新按需分页页表项失败");
            CN_virtual_memory_remove_mapping(manager, virtual_address, 1);
            CN_virtual_memory_free_physical_pages(manager, physical_address, 1);
            return false;
        }
        
        CN_LOG_INFO("按需分页: 虚拟=0x%p -> 物理=0x%p", 
                   (void*)virtual_address, (void*)physical_address);
        
        return true;
    }
    
    // 如果是保护违规但不是保护页，返回失败
    if (fault_info->is_protection_violation)
    {
        CN_LOG_ERROR("页面保护违规: 地址=0x%p", (void*)fault_info->virtual_address);
        return false;
    }
    
    // 其他类型的页面错误
    CN_LOG_ERROR("未知页面错误类型: 地址=0x%p, 错误代码=%u",
                (void*)fault_info->virtual_address, fault_info->error_code);
    
    return false;
}
