/******************************************************************************
 * 文件名: CN_virtual_memory_core.c
 * 功能: CN_Language虚拟内存管理器核心实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建虚拟内存管理器核心实现
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_virtual_memory_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>
#include <time.h>

// ============================================================================
// 默认MMU接口实现
// ============================================================================

/**
 * @brief 默认MMU初始化函数
 */
static bool default_mmu_initialize(const Stru_CN_VirtualMemoryConfig_t* config)
{
    CN_LOG_INFO("默认MMU初始化，页面大小: %zu, 页表级数: %zu", 
                config->page_size, config->page_table_levels);
    return true;
}

/**
 * @brief 默认MMU启用函数
 */
static bool default_mmu_enable(void)
{
    CN_LOG_INFO("启用默认MMU");
    return true;
}

/**
 * @brief 默认MMU禁用函数
 */
static void default_mmu_disable(void)
{
    CN_LOG_INFO("禁用默认MMU");
}

/**
 * @brief 默认MMU TLB刷新函数
 */
static void default_mmu_flush_tlb(uintptr_t virtual_address)
{
    if (virtual_address == 0)
    {
        CN_LOG_DEBUG("刷新全部TLB");
    }
    else
    {
        CN_LOG_DEBUG("刷新TLB地址: 0x%p", (void*)virtual_address);
    }
}

/**
 * @brief 默认MMU设置页表基地址函数
 */
static void default_mmu_set_page_table_base(uintptr_t page_table_base)
{
    CN_LOG_DEBUG("设置页表基地址: 0x%p", (void*)page_table_base);
}

/**
 * @brief 默认MMU获取页表基地址函数
 */
static uintptr_t default_mmu_get_page_table_base(void)
{
    return 0;
}

/**
 * @brief 默认MMU获取架构信息函数
 */
static void default_mmu_get_architecture_info(char* info_buffer, size_t buffer_size)
{
    if (info_buffer && buffer_size > 0)
    {
        snprintf(info_buffer, buffer_size, "默认MMU实现（软件模拟）");
    }
}

/**
 * @brief 默认MMU接口
 */
const Stru_CN_MMUInterface_t CN_default_mmu_interface = {
    .initialize = default_mmu_initialize,
    .enable = default_mmu_enable,
    .disable = default_mmu_disable,
    .flush_tlb = default_mmu_flush_tlb,
    .set_page_table_base = default_mmu_set_page_table_base,
    .get_page_table_base = default_mmu_get_page_table_base,
    .get_architecture_info = default_mmu_get_architecture_info
};

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 虚拟地址分解
 */
void CN_virtual_memory_decompose_address(uintptr_t virtual_address,
                                         size_t page_size,
                                         size_t levels,
                                         size_t* indices)
{
    if (!indices || levels == 0)
    {
        return;
    }
    
    // 计算每级索引的位数
    size_t bits_per_level = 0;
    size_t temp = page_size;
    while (temp > 1)
    {
        bits_per_level++;
        temp >>= 1;
    }
    
    // 对于多级页表，每级使用9位（512项）
    if (levels > 1)
    {
        bits_per_level = 9;
    }
    
    // 计算每级的掩码
    size_t mask = (1UL << bits_per_level) - 1;
    
    // 分解地址
    for (size_t i = 0; i < levels; i++)
    {
        indices[i] = (virtual_address >> (bits_per_level * (levels - i - 1))) & mask;
    }
}

/**
 * @brief 保护标志转换为页表项标志
 */
uint32_t CN_virtual_memory_protection_to_pte_flags(Eum_CN_PageProtection_t protection,
                                                   Eum_CN_PageMappingFlags_t flags)
{
    uint32_t pte_flags = Eum_PTE_PRESENT;
    
    // 转换保护标志
    if (protection & Eum_PAGE_PROTECTION_WRITE)
    {
        pte_flags |= Eum_PTE_WRITABLE;
    }
    
    if (protection & Eum_PAGE_PROTECTION_USER)
    {
        pte_flags |= Eum_PTE_USER_ACCESSIBLE;
    }
    
    if (protection & Eum_PAGE_PROTECTION_NO_CACHE)
    {
        pte_flags |= Eum_PTE_CACHE_DISABLED;
    }
    
    if (protection & Eum_PAGE_PROTECTION_WRITE_THROUGH)
    {
        pte_flags |= Eum_PTE_WRITE_THROUGH;
    }
    
    if (!(protection & Eum_PAGE_PROTECTION_EXECUTE))
    {
        pte_flags |= Eum_PTE_NO_EXECUTE;
    }
    
    // 转换映射标志
    if (flags & Eum_PAGE_MAPPING_SHARED)
    {
        pte_flags |= Eum_PTE_GLOBAL;
    }
    
    return pte_flags;
}

/**
 * @brief 页表项标志转换为保护标志
 */
void CN_virtual_memory_pte_flags_to_protection(uint32_t pte_flags,
                                               Eum_CN_PageProtection_t* protection,
                                               Eum_CN_PageMappingFlags_t* flags)
{
    if (protection)
    {
        *protection = Eum_PAGE_PROTECTION_NONE;
        
        if (pte_flags & Eum_PTE_PRESENT)
        {
            *protection |= Eum_PAGE_PROTECTION_READ;
        }
        
        if (pte_flags & Eum_PTE_WRITABLE)
        {
            *protection |= Eum_PAGE_PROTECTION_WRITE;
        }
        
        if (pte_flags & Eum_PTE_USER_ACCESSIBLE)
        {
            *protection |= Eum_PAGE_PROTECTION_USER;
        }
        else
        {
            *protection |= Eum_PAGE_PROTECTION_KERNEL;
        }
        
        if (!(pte_flags & Eum_PTE_NO_EXECUTE))
        {
            *protection |= Eum_PAGE_PROTECTION_EXECUTE;
        }
        
        if (pte_flags & Eum_PTE_CACHE_DISABLED)
        {
            *protection |= Eum_PAGE_PROTECTION_NO_CACHE;
        }
        
        if (pte_flags & Eum_PTE_WRITE_THROUGH)
        {
            *protection |= Eum_PAGE_PROTECTION_WRITE_THROUGH;
        }
    }
    
    if (flags)
    {
        *flags = 0;
        
        if (pte_flags & Eum_PTE_GLOBAL)
        {
            *flags |= Eum_PAGE_MAPPING_SHARED;
        }
    }
}

// ============================================================================
// 页表管理函数实现
// ============================================================================

/**
 * @brief 初始化页表
 */
bool CN_virtual_memory_init_page_tables(Stru_CN_VirtualMemoryManager_t* manager)
{
    if (!manager)
    {
        CN_LOG_ERROR("无效的虚拟内存管理器");
        return false;
    }
    
    size_t page_table_levels = manager->config.page_table_levels;
    if (page_table_levels < 1 || page_table_levels > CN_MAX_PAGE_TABLE_LEVELS)
    {
        CN_LOG_ERROR("无效的页表级数: %zu", page_table_levels);
        return false;
    }
    
    // 分配页表数组
    manager->page_table_count = page_table_levels;
    manager->page_tables = (Stru_CN_PageTable_t*)cn_calloc(
        page_table_levels, sizeof(Stru_CN_PageTable_t));
    
    if (!manager->page_tables)
    {
        CN_LOG_ERROR("分配页表数组失败");
        return false;
    }
    
    // 初始化每个页表
    for (size_t i = 0; i < page_table_levels; i++)
    {
        Stru_CN_PageTable_t* page_table = &manager->page_tables[i];
        page_table->level = (Eum_CN_PageTableLevel_t)i;
        page_table->entry_count = CN_PAGE_TABLE_ENTRIES_PER_LEVEL;
        
        // 分配页表项数组
        size_t entry_array_size = page_table->entry_count * sizeof(Stru_CN_PageTableEntry_t);
        page_table->entries = (Stru_CN_PageTableEntry_t*)cn_calloc(
            page_table->entry_count, sizeof(Stru_CN_PageTableEntry_t));
        
        if (!page_table->entries)
        {
            CN_LOG_ERROR("分配页表项数组失败，级别: %zu", i);
            // 清理已分配的资源
            for (size_t j = 0; j < i; j++)
            {
                cn_free(manager->page_tables[j].entries);
            }
            cn_free(manager->page_tables);
            manager->page_tables = NULL;
            return false;
        }
        
        // 初始化页表项
        for (size_t j = 0; j < page_table->entry_count; j++)
        {
            page_table->entries[j].physical_address = 0;
            page_table->entries[j].flags = 0;
            page_table->entries[j].reserved = 0;
        }
        
        page_table->is_allocated = true;
        page_table->physical_address = 0; // 将在后续设置
        
        CN_LOG_DEBUG("初始化页表级别 %zu，项数: %zu", i, page_table->entry_count);
    }
    
    // 设置页表基地址（指向第一级页表）
    manager->page_table_base = (uintptr_t)manager->page_tables[0].entries;
    
    CN_LOG_INFO("初始化页表完成，级数: %zu，基地址: 0x%p", 
                page_table_levels, (void*)manager->page_table_base);
    
    return true;
}

/**
 * @brief 销毁页表
 */
void CN_virtual_memory_destroy_page_tables(Stru_CN_VirtualMemoryManager_t* manager)
{
    if (!manager || !manager->page_tables)
    {
        return;
    }
    
    // 释放每个页表的项数组
    for (size_t i = 0; i < manager->page_table_count; i++)
    {
        Stru_CN_PageTable_t* page_table = &manager->page_tables[i];
        if (page_table->entries)
        {
            cn_free(page_table->entries);
            page_table->entries = NULL;
        }
        page_table->is_allocated = false;
    }
    
    // 释放页表数组
    cn_free(manager->page_tables);
    manager->page_tables = NULL;
    manager->page_table_count = 0;
    manager->page_table_base = 0;
    
    CN_LOG_INFO("销毁页表完成");
}

/**
 * @brief 更新页表项
 */
bool CN_virtual_memory_update_page_table_entry(Stru_CN_VirtualMemoryManager_t* manager,
                                               uintptr_t virtual_address,
                                               uintptr_t physical_address,
                                               Eum_CN_PageProtection_t protection,
                                               Eum_CN_PageMappingFlags_t flags)
{
    if (!manager || !manager->page_tables)
    {
        CN_LOG_ERROR("虚拟内存管理器或页表未初始化");
        return false;
    }
    
    // 检查地址对齐
    if (!CN_virtual_memory_is_page_aligned(virtual_address, manager->config.page_size) ||
        !CN_virtual_memory_is_page_aligned(physical_address, manager->config.page_size))
    {
        CN_LOG_ERROR("地址未页面对齐: 虚拟=0x%p, 物理=0x%p", 
                    (void*)virtual_address, (void*)physical_address);
        return false;
    }
    
    // 分解虚拟地址
    size_t indices[CN_MAX_PAGE_TABLE_LEVELS];
    CN_virtual_memory_decompose_address(virtual_address, 
                                        manager->config.page_size,
                                        manager->config.page_table_levels,
                                        indices);
    
    // 遍历页表层级
    Stru_CN_PageTable_t* current_table = &manager->page_tables[0];
    for (size_t i = 0; i < manager->config.page_table_levels - 1; i++)
    {
        size_t index = indices[i];
        if (index >= current_table->entry_count)
        {
            CN_LOG_ERROR("页表索引越界: 级别=%zu, 索引=%zu, 最大=%zu", 
                        i, index, current_table->entry_count);
            return false;
        }
        
        Stru_CN_PageTableEntry_t* entry = &current_table->entries[index];
        
        // 如果这是中间级别且条目不存在，需要分配下一级页表
        if (!(entry->flags & Eum_PTE_PRESENT))
        {
            // 分配下一级页表
            size_t next_level = i + 1;
            if (next_level >= manager->page_table_count)
            {
                CN_LOG_ERROR("页表级别越界: %zu", next_level);
                return false;
            }
            
            // 这里简化处理，实际需要分配物理内存
            entry->physical_address = (uintptr_t)manager->page_tables[next_level].entries;
            entry->flags = Eum_PTE_PRESENT | Eum_PTE_WRITABLE | Eum_PTE_USER_ACCESSIBLE;
            
            CN_LOG_DEBUG("分配中间页表级别 %zu，索引 %zu", i, index);
        }
        
        // 移动到下一级页表
        current_table = (Stru_CN_PageTable_t*)entry->physical_address;
    }
    
    // 最后一级：设置页表项
    size_t last_index = indices[manager->config.page_table_levels - 1];
    if (last_index >= current_table->entry_count)
    {
        CN_LOG_ERROR("最后一级页表索引越界: 索引=%zu, 最大=%zu", 
                    last_index, current_table->entry_count);
        return false;
    }
    
    Stru_CN_PageTableEntry_t* final_entry = &current_table->entries[last_index];
    final_entry->physical_address = physical_address;
    final_entry->flags = CN_virtual_memory_protection_to_pte_flags(protection, flags);
    
    // 更新统计信息
    manager->stats.mapped_pages++;
    
    CN_LOG_DEBUG("更新页表项: 虚拟=0x%p -> 物理=0x%p, 标志=0x%x", 
                (void*)virtual_address, (void*)physical_address, final_entry->flags);
    
    return true;
}

/**
 * @brief 清除页表项
 */
void CN_virtual_memory_clear_page_table_entry(Stru_CN_VirtualMemoryManager_t* manager,
                                              uintptr_t virtual_address)
{
    if (!manager || !manager->page_tables)
    {
        return;
    }
    
    // 分解虚拟地址
    size_t indices[CN_MAX_PAGE_TABLE_LEVELS];
    CN_virtual_memory_decompose_address(virtual_address, 
                                        manager->config.page_size,
                                        manager->config.page_table_levels,
                                        indices);
    
    // 遍历页表层级
    Stru_CN_PageTable_t* current_table = &manager->page_tables[0];
    for (size_t i = 0; i < manager->config.page_table_levels - 1; i++)
    {
        size_t index = indices[i];
        if (index >= current_table->entry_count)
        {
            return; // 索引越界，直接返回
        }
        
        Stru_CN_PageTableEntry_t* entry = &current_table->entries[index];
        if (!(entry->flags & Eum_PTE_PRESENT))
        {
            return; // 条目不存在，直接返回
        }
        
        // 移动到下一级页表
        current_table = (Stru_CN_PageTable_t*)entry->physical_address;
    }
    
    // 最后一级：清除页表项
    size_t last_index = indices[manager->config.page_table_levels - 1];
    if (last_index < current_table->entry_count)
    {
        Stru_CN_PageTableEntry_t* final_entry = &current_table->entries[last_index];
        final_entry->physical_address = 0;
        final_entry->flags = 0;
        
        // 更新统计信息
        if (manager->stats.mapped_pages > 0)
        {
            manager->stats.mapped_pages--;
        }
        
        CN_LOG_DEBUG("清除页表项: 虚拟=0x%p", (void*)virtual_address);
    }
}

// ============================================================================
// 物理内存管理函数实现
// ============================================================================

/**
 * @brief 分配物理页面
 */
uintptr_t CN_virtual_memory_alloc_physical_pages(Stru_CN_VirtualMemoryManager_t* manager,
                                                 size_t page_count)
{
    if (!manager || page_count == 0)
    {
        CN_LOG_ERROR("无效参数: manager=%p, page_count=%zu", manager, page_count);
        return 0;
    }
    
    // 检查是否有足够的空闲页面
    if (manager->free_physical_pages < page_count)
    {
        CN_LOG_ERROR("物理内存不足: 请求=%zu, 可用=%zu", 
                    page_count, manager->free_physical_pages);
        return 0;
    }
    
    // 简化实现：模拟物理内存分配
    // 在实际系统中，这里会调用物理内存分配器
    static uintptr_t next_physical_address = 0x10000000; // 从256MB开始
    
    uintptr_t physical_address = next_physical_address;
    next_physical_address += page_count * manager->config.page_size;
    
    // 更新统计信息
    manager->free_physical_pages -= page_count;
    manager->stats.committed_pages += page_count;
    manager->stats.page_table_allocations++;
    
    CN_LOG_DEBUG("分配物理页面: 地址=0x%p, 页面数=%zu", 
                (void*)physical_address, page_count);
    
    return physical_address;
}

/**
 * @brief 释放物理页面
 */
void CN_virtual_memory_free_physical_pages(Stru_CN_VirtualMemoryManager_t* manager,
                                           uintptr_t physical_address,
                                           size_t page_count)
{
    if (!manager || physical_address == 0 || page_count == 0)
    {
        CN_LOG_ERROR("无效参数: manager=%p, address=0x%p, count=%zu", 
                    manager, (void*)physical_address, page_count);
        return;
    }
    
    // 简化实现：在实际系统中，这里会调用物理内存分配器释放内存
    // 这里只更新统计信息
    
    // 更新统计信息
    manager->free_physical_pages += page_count;
    if (manager->stats.committed_pages >= page_count)
    {
        manager->stats.committed_pages -= page_count;
    }
    else
    {
        manager->stats.committed_pages = 0;
    }
    manager->stats.page_table_frees++;
    
    CN_LOG_DEBUG("释放物理页面: 地址=0x%p, 页面数=%zu", 
                (void*)physical_address, page_count);
}

// ============================================================================
// 虚拟地址映射管理函数实现
// ============================================================================

/**
 * @brief 查找虚拟地址映射
 */
int CN_virtual_memory_find_mapping(Stru_CN_VirtualMemoryManager_t* manager,
                                   uintptr_t virtual_address)
{
    if (!manager || !manager->virtual_mappings)
    {
        return -1;
    }
    
    // 线性搜索映射（简化实现）
    // 在实际系统中，应该使用更高效的数据结构
    size_t mapping_count = CN_array_size(manager->virtual_mappings);
    for (size_t i = 0; i < mapping_count; i++)
    {
        Stru_CN_VirtualMapping_t* mapping = 
            (Stru_CN_VirtualMapping_t*)CN_array_get(manager->virtual_mappings, i);
        
        if (mapping && 
            virtual_address >= mapping->virtual_address &&
            virtual_address < mapping->virtual_address + 
                mapping->page_count * manager->config.page_size)
        {
            return (int)i;
        }
    }
    
    return -1;
}

/**
 * @brief 添加虚拟地址映射
 */
bool CN_virtual_memory_add_mapping(Stru_CN_VirtualMemoryManager_t* manager,
                                   const Stru_CN_VirtualMapping_t* mapping)
{
    if (!manager || !mapping || !manager->virtual_mappings)
    {
        CN_LOG_ERROR("无效参数");
        return false;
    }
    
    // 检查地址对齐
    if (!CN_virtual_memory_is_page_aligned(mapping->virtual_address, manager->config.page_size) ||
        !CN_virtual_memory_is_page_aligned(mapping->physical_address, manager->config.page_size))
    {
        CN_LOG_ERROR("地址未页面对齐: 虚拟=0x%p, 物理=0x%p", 
                    (void*)mapping->virtual_address, (void*)mapping->physical_address);
        return false;
    }
    
    // 检查地址范围是否重叠
    int existing_index = CN_virtual_memory_find_mapping(manager, mapping->virtual_address);
    if (existing_index >= 0)
    {
        CN_LOG_ERROR("虚拟地址已映射: 0x%p", (void*)mapping->virtual_address);
        return false;
    }
    
    // 创建映射副本
    Stru_CN_VirtualMapping_t* new_mapping = 
        (Stru_CN_VirtualMapping_t*)cn_malloc(sizeof(Stru_CN_VirtualMapping_t));
    
    if (!new_mapping)
    {
        CN_LOG_ERROR("分配映射内存失败");
        return false;
    }
    
    // 复制映射信息
    memcpy(new_mapping, mapping, sizeof(Stru_CN_VirtualMapping_t));
    
    // 设置时间戳
    new_mapping->timestamp = (uint64_t)time(NULL);
    
    // 添加到数组
    if (!CN_array_push_back(manager->virtual_mappings, new_mapping))
    {
        CN_LOG_ERROR("添加映射到数组失败");
        cn_free(new_mapping);
        return false;
    }
    
    // 更新统计信息
    manager->stats.mapped_pages += mapping->page_count;
    
    CN_LOG_DEBUG("添加虚拟地址映射: 虚拟=0x%p -> 物理=0x%p, 页面数=%zu", 
                (void*)mapping->virtual_address, (void*)mapping->physical_address, 
                mapping->page_count);
    
    return true;
}

/**
 * @brief 移除虚拟地址映射
 */
bool CN_virtual_memory_remove_mapping(Stru_CN_VirtualMemoryManager_t* manager,
                                      uintptr_t virtual_address,
                                      size_t page_count)
{
    if (!manager || !manager->virtual_mappings)
    {
        return false;
    }
    
    // 查找映射
    int index = CN_virtual_memory_find_mapping(manager, virtual_address);
    if (index < 0)
    {
        CN_LOG_WARN("未找到虚拟地址映射: 0x%p", (void*)virtual_address);
        return false;
    }
    
    // 获取映射
    Stru_CN_VirtualMapping_t* mapping = 
        (Stru_CN_VirtualMapping_t*)CN_array_get(manager->virtual_mappings, (size_t)index);
    
    if (!mapping)
    {
        return false;
    }
    
    // 检查页面数量是否匹配
    if (mapping->page_count != page_count)
    {
        CN_LOG_WARN("页面数量不匹配: 期望=%zu, 实际=%zu", 
                   page_count, mapping->page_count);
        // 继续执行，但记录警告
    }
    
    // 从数组中移除
    CN_array_remove(manager->virtual_mappings, (size_t)index);
    
    // 释放映射内存
    cn_free(mapping);
    
    // 更新统计信息
    if (manager->stats.mapped_pages >= page_count)
    {
        manager->stats.mapped_pages -= page_count;
    }
    else
    {
        manager->stats.mapped_pages = 0;
    }
    
    CN_LOG_DEBUG("移除虚拟地址映射: 虚拟=0x%p, 页面数=%zu", 
                (void*)virtual_address, page_count);
    
    return true;
}

// ============================================================================
// 虚拟内存管理器核心函数实现
// ============================================================================

/**
 * @brief 创建虚拟内存管理器
 */
Stru_CN_VirtualMemoryManager_t* CN_virtual_memory_create(
    const Stru_CN_VirtualMemoryConfig_t* config,
    const Stru_CN_MMUInterface_t* mmu_interface)
{
    // 使用默认配置如果未提供
    Stru_CN_VirtualMemoryConfig_t actual_config;
    if (config)
    {
        actual_config = *config;
    }
    else
    {
        actual_config = CN_VIRTUAL_MEMORY_CONFIG_DEFAULT;
    }
    
    // 验证配置
    if (actual_config.page_size == 0)
    {
        actual_config.page_size = CN_VIRTUAL_PAGE_SIZE_4KB;
    }
    
    if (actual_config.page_table_levels == 0)
    {
        actual_config.page_table_levels = 2;
    }
    else if (actual_config.page_table_levels > CN_MAX_PAGE_TABLE_LEVELS)
    {
        actual_config.page_table_levels = CN_MAX_PAGE_TABLE_LEVELS;
    }
    
    // 分配管理器内存
    Stru_CN_VirtualMemoryManager_t* manager = 
        (Stru_CN_VirtualMemoryManager_t*)cn_malloc(sizeof(Stru_CN_VirtualMemoryManager_t));
    
    if (!manager)
    {
        CN_LOG_ERROR("分配虚拟内存管理器内存失败");
        return NULL;
    }
    
    // 初始化管理器
    memset(manager, 0, sizeof(Stru_CN_VirtualMemoryManager_t));
    manager->config = actual_config;
    
    // 设置MMU接口
    if (mmu_interface)
    {
        manager->mmu_interface = mmu_interface;
    }
    else
    {
        manager->mmu_interface = &CN_default_mmu_interface;
        manager->default_mmu_interface = CN_default_mmu_interface;
    }
    
    // 初始化统计信息
    memset(&manager->stats, 0, sizeof(Stru_CN_VirtualMemoryStats_t));
    
    // 设置日志级别
    manager->log_level = Eum_LOG_LEVEL_INFO;
    if (manager->config.enable_debug)
    {
        manager->log_level = Eum_LOG_LEVEL_DEBUG;
        manager->debug_enabled = true;
    }
    
    // 初始化虚拟地址空间
    manager->next_virtual_address = manager->config.virtual_address_start;
    
    // 计算总虚拟页面数量
    size_t virtual_address_space_size = 
        manager->config.virtual_address_end - manager->config.virtual_address_start;
    manager->stats.total_virtual_pages = 
        CN_virtual_memory_size_to_pages(virtual_address_space_size, manager->config.page_size);
    
    // 初始化物理内存池（简化实现）
    manager->total_physical_pages = CN_DEFAULT_PHYSICAL_MEMORY_POOL_SIZE / manager->config.page_size;
    manager->free_physical_pages = manager->total_physical_pages;
    
    // 创建虚拟映射数组
    manager->virtual_mappings = CN_array_create(sizeof(Stru_CN_VirtualMapping_t*), 16);
    if (!manager->virtual_mappings)
    {
        CN_LOG_ERROR("创建虚拟映射数组失败");
        cn_free(manager);
        return NULL;
    }
    
    // 初始化MMU
    if (!manager->mmu_interface->initialize(&manager->config))
    {
        CN_LOG_ERROR("MMU初始化失败");
        CN_array_destroy(manager->virtual_mappings);
        cn_free(manager);
        return NULL;
    }
    
    // 初始化页表
    if (!CN_virtual_memory_init_page_tables(manager))
    {
        CN_LOG_ERROR("页表初始化失败");
        CN_array_destroy(manager->virtual_mappings);
        cn_free(manager);
        return NULL;
    }
    
    manager->is_initialized = true;
    
    CN_LOG_INFO("创建虚拟内存管理器成功: %s", manager->config.name);
    CN_LOG_INFO("虚拟地址空间: 0x%p - 0x%p", 
                (void*)manager->config.virtual_address_start,
                (void*)manager->config.virtual_address_end);
    CN_LOG_INFO("页面大小: %zu, 页表级数: %zu", 
                manager->config.page_size, manager->config.page_table_levels);
    
    return manager;
}

/**
 * @brief 销毁虚拟内存管理器
 */
void CN_virtual_memory_destroy(Stru_CN_VirtualMemoryManager_t* manager)
{
    if (!manager)
    {
        return;
    }
    
    CN_LOG_INFO("销毁虚拟内存管理器: %s", manager->config.name);
    
    // 禁用MMU
    if (manager->mmu_interface)
    {
        manager->mmu_interface->disable();
    }
    
    // 销毁所有映射
    if (manager->virtual_mappings)
    {
        size_t mapping_count = CN_array_size(manager->virtual_mappings);
        for (size_t i = 0; i < mapping_count; i++)
        {
            Stru_CN_VirtualMapping_t* mapping = 
                (Stru_CN_VirtualMapping_t*)CN_array_get(manager->virtual_mappings, i);
            if (mapping)
            {
                cn_free(mapping);
            }
        }
        CN_array_destroy(manager->virtual_mappings);
    }
    
    // 销毁页表
    CN_virtual_memory_destroy_page_tables(manager);
    
    // 释放管理器内存
    cn_free(manager);
    
    CN_LOG_INFO("虚拟内存管理器销毁完成");
}

#ifdef __cplusplus
}
#endif
