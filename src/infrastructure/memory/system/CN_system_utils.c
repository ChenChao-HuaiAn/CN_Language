/******************************************************************************
 * 文件名: CN_system_utils.c
 * 功能: CN_Language系统分配器工具函数
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建系统分配器工具函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_system_internal.h"
#include <stdarg.h>

// ============================================================================
// 调试函数实现（续）
// ============================================================================

void CN_system_dump_leaks(Stru_CN_SystemAllocator_t* allocator)
{
    if (allocator == NULL)
    {
        return;
    }
    
    system_lock(allocator);
    
    if (allocator->allocation_count == 0)
    {
        printf("=== 内存泄漏检查 ===\n");
        printf("未检测到内存泄漏。\n");
        printf("===================\n");
    }
    else
    {
        printf("=== 检测到内存泄漏 ===\n");
        printf("未释放的分配数量: %zu\n", allocator->allocation_count);
        printf("总泄漏字节数: %zu\n", allocator->stats.current_usage);
        
        for (size_t i = 0; i < allocator->allocation_count; i++)
        {
            Stru_CN_AllocationInfo_t* info = allocator->allocations[i];
            printf("\n泄漏 [%zu]:\n", i);
            printf("  地址: %p\n", info->address);
            printf("  大小: %zu 字节\n", info->size);
            printf("  文件: %s\n", info->file ? info->file : "未知");
            printf("  行号: %d\n", info->line);
            printf("  时间戳: %llu\n", (unsigned long long)info->timestamp);
            printf("  目的: %s\n", info->purpose ? info->purpose : "未知");
        }
        
        printf("=====================\n");
    }
    
    system_unlock(allocator);
}

void CN_system_set_debug_callback(Stru_CN_SystemAllocator_t* allocator, 
                                  CN_SystemDebugCallback_t callback, 
                                  void* user_data)
{
    if (allocator == NULL)
    {
        return;
    }
    
    system_lock(allocator);
    allocator->debug_callback = callback;
    allocator->debug_user_data = user_data;
    system_unlock(allocator);
    
    system_debug_output(allocator, "[INFO] 调试回调已设置\n");
}

// ============================================================================
// 默认分配器实现
// ============================================================================

Stru_CN_SystemAllocator_t* CN_system_get_default(void)
{
    // 线程安全的懒加载单例
    static Stru_CN_SystemAllocator_t* default_allocator = NULL;
    
    if (default_allocator == NULL)
    {
        // 使用默认配置创建分配器
        Stru_CN_SystemConfig_t config = CN_SYSTEM_CONFIG_DEFAULT;
        default_allocator = CN_system_create(&config);
        
        if (default_allocator != NULL)
        {
            // 注册atexit处理函数
            atexit(CN_system_release_default);
        }
    }
    
    return default_allocator;
}

void CN_system_release_default(void)
{
    Stru_CN_SystemAllocator_t* default_allocator = CN_system_get_default();
    
    if (default_allocator != NULL)
    {
        // 检查泄漏
        if (CN_system_check_leaks(default_allocator))
        {
            printf("=== 程序退出时检测到内存泄漏 ===\n");
            CN_system_dump_leaks(default_allocator);
        }
        
        // 销毁分配器
        CN_system_destroy(default_allocator);
        
        // 清除全局指针
        // 注意：这里不能直接设置为NULL，因为get_default函数可能再次被调用
        // 在实际实现中，需要更复杂的线程安全处理
    }
}

// ============================================================================
// 对齐辅助函数实现
// ============================================================================

// 注意：对齐内存的释放现在由CN_system_free函数直接处理
// 对齐分配在CN_system_alloc_aligned中存储原始指针
// CN_system_free从用户指针之前读取原始指针并释放

// ============================================================================
// 内存操作包装函数（兼容现有代码）
// ============================================================================

// 注意：为了保持系统分配器模块的独立性，这里不实现与CN_memory.h的适配器。
// 适配器应该在CN_memory.c中实现，使用系统分配器的公共API。

// ============================================================================
// 测试辅助函数
// ============================================================================

#ifdef CN_SYSTEM_TEST

/**
 * @brief 运行系统分配器基本测试
 */
void CN_system_run_basic_tests(void)
{
    printf("=== 系统分配器基本测试 ===\n");
    
    // 创建分配器
    Stru_CN_SystemConfig_t config = CN_SYSTEM_CONFIG_DEFAULT;
    config.enable_debug = true;
    config.track_allocations = true;
    
    Stru_CN_SystemAllocator_t* allocator = CN_system_create(&config);
    if (allocator == NULL)
    {
        printf("测试失败: 无法创建分配器\n");
        return;
    }
    
    // 测试1: 基本分配和释放
    printf("测试1: 基本分配和释放\n");
    void* ptr1 = CN_system_alloc(allocator, 100, __FILE__, __LINE__, "测试1");
    if (ptr1 == NULL)
    {
        printf("  失败: 分配失败\n");
    }
    else
    {
        printf("  成功: 分配了100字节\n");
        CN_system_free(allocator, ptr1, __FILE__, __LINE__);
        printf("  成功: 释放内存\n");
    }
    
    // 测试2: 多个分配
    printf("\n测试2: 多个分配\n");
    void* ptrs[10];
    for (int i = 0; i < 10; i++)
    {
        ptrs[i] = CN_system_alloc(allocator, (i + 1) * 10, __FILE__, __LINE__, "测试2");
        if (ptrs[i] == NULL)
        {
            printf("  失败: 分配 %d 失败\n", i);
        }
    }
    
    // 释放一半
    for (int i = 0; i < 5; i++)
    {
        CN_system_free(allocator, ptrs[i], __FILE__, __LINE__);
    }
    
    // 测试3: calloc
    printf("\n测试3: calloc分配\n");
    int* array = (int*)CN_system_calloc(allocator, 10, sizeof(int), __FILE__, __LINE__, "测试3");
    if (array == NULL)
    {
        printf("  失败: calloc分配失败\n");
    }
    else
    {
        // 检查是否清零
        bool is_zero = true;
        for (int i = 0; i < 10; i++)
        {
            if (array[i] != 0)
            {
                is_zero = false;
                break;
            }
        }
        printf("  %s: 内存已正确清零\n", is_zero ? "成功" : "失败");
        CN_system_free(allocator, array, __FILE__, __LINE__);
    }
    
    // 测试4: 统计信息
    printf("\n测试4: 统计信息\n");
    Stru_CN_SystemStats_t stats;
    if (CN_system_get_stats(allocator, &stats))
    {
        printf("  总分配: %zu 字节\n", stats.total_allocated);
        printf("  当前使用: %zu 字节\n", stats.current_usage);
        printf("  分配次数: %zu\n", stats.allocation_count);
    }
    
    // 测试5: 泄漏检测
    printf("\n测试5: 泄漏检测\n");
    void* leak_ptr = CN_system_alloc(allocator, 50, __FILE__, __LINE__, "泄漏测试");
    bool has_leak = CN_system_check_leaks(allocator);
    printf("  %s: 检测到泄漏\n", has_leak ? "成功" : "失败");
    
    // 清理
    CN_system_free(allocator, leak_ptr, __FILE__, __LINE__);
    
    // 释放剩余内存
    for (int i = 5; i < 10; i++)
    {
        if (ptrs[i] != NULL)
        {
            CN_system_free(allocator, ptrs[i], __FILE__, __LINE__);
        }
    }
    
    // 销毁分配器
    CN_system_destroy(allocator);
    
    printf("\n=== 测试完成 ===\n");
}

#endif // CN_SYSTEM_TEST

// ============================================================================
// 初始化函数（模块初始化）
// ============================================================================

/**
 * @brief 初始化系统分配器模块
 * 
 * 这个函数在模块加载时被调用，用于初始化全局状态。
 */
__attribute__((constructor))
static void system_allocator_module_init(void)
{
    printf("[INFO] 系统分配器模块已加载\n");
}

/**
 * @brief 清理系统分配器模块
 * 
 * 这个函数在模块卸载时被调用，用于清理全局状态。
 */
__attribute__((destructor))
static void system_allocator_module_cleanup(void)
{
    printf("[INFO] 系统分配器模块已卸载\n");
}
