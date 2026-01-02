/******************************************************************************
 * 文件名: CN_memory.c
 * 功能: CN_Language内存管理实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现统一内存管理接口
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部数据结构
// ============================================================================

// 当前分配器类型
static Eum_CN_AllocatorType_t g_current_allocator = Eum_ALLOCATOR_SYSTEM;

// 内存统计信息
static Stru_CN_MemoryStats_t g_memory_stats = {0};

// 调试设置
static bool g_debug_enabled = false;
static bool g_track_allocations = false;
static unsigned char g_fill_pattern = 0xCC;
static size_t g_allocation_failure_after = 0;
static size_t g_allocation_count = 0;

// ============================================================================
// 系统分配器实现
// ============================================================================

static void* system_allocate(size_t size, const char* purpose)
{
    (void)purpose; // 未使用参数
    
    if (g_allocation_failure_after > 0 && g_allocation_count >= g_allocation_failure_after)
    {
        return NULL;
    }
    
    void* ptr = malloc(size);
    if (ptr)
    {
        g_allocation_count++;
        g_memory_stats.total_allocated += size;
        g_memory_stats.current_usage += size;
        g_memory_stats.allocation_count++;
        
        if (g_memory_stats.current_usage > g_memory_stats.peak_usage)
        {
            g_memory_stats.peak_usage = g_memory_stats.current_usage;
        }
        
        if (g_debug_enabled && g_fill_pattern != 0)
        {
            memset(ptr, g_fill_pattern, size);
        }
    }
    
    return ptr;
}

static void* system_allocate_aligned(size_t size, size_t alignment, const char* purpose)
{
    (void)purpose; // 未使用参数
    
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    void* ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0)
    {
        return NULL;
    }
    return ptr;
#endif
}

static void* system_reallocate(void* ptr, size_t new_size, const char* purpose)
{
    (void)purpose; // 未使用参数
    
    if (ptr == NULL)
    {
        return system_allocate(new_size, purpose);
    }
    
    // 注意：我们无法知道原分配的大小，所以统计可能不准确
    void* new_ptr = realloc(ptr, new_size);
    if (new_ptr)
    {
        // 简化处理：假设重新分配成功
        g_memory_stats.total_allocated += new_size;
        g_memory_stats.allocation_count++;
    }
    
    return new_ptr;
}

static void system_deallocate(void* ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    
    // 注意：我们无法知道释放的大小，所以统计可能不准确
    free(ptr);
    g_memory_stats.free_count++;
}

static Stru_CN_MemoryStats_t system_get_stats(void)
{
    return g_memory_stats;
}

static void system_reset_stats(void)
{
    memset(&g_memory_stats, 0, sizeof(g_memory_stats));
    g_allocation_count = 0;
}

static bool system_initialize(void)
{
    // 系统分配器不需要特殊初始化
    return true;
}

static void system_shutdown(void)
{
    // 系统分配器不需要特殊清理
}

// 系统分配器接口
static Stru_CN_AllocatorInterface_t g_system_allocator = {
    .allocate = system_allocate,
    .allocate_aligned = system_allocate_aligned,
    .reallocate = system_reallocate,
    .deallocate = system_deallocate,
    .get_stats = system_get_stats,
    .reset_stats = system_reset_stats,
    .initialize = system_initialize,
    .shutdown = system_shutdown
};

// ============================================================================
// 当前分配器接口
// ============================================================================

static Stru_CN_AllocatorInterface_t* get_current_allocator(void)
{
    switch (g_current_allocator)
    {
        case Eum_ALLOCATOR_SYSTEM:
        case Eum_ALLOCATOR_POOL:
        case Eum_ALLOCATOR_ARENA:
        case Eum_ALLOCATOR_DEBUG:
        default:
            return &g_system_allocator;
    }
}

// ============================================================================
// 公共API实现
// ============================================================================

bool CN_memory_init(Eum_CN_AllocatorType_t allocator_type)
{
    g_current_allocator = allocator_type;
    Stru_CN_AllocatorInterface_t* allocator = get_current_allocator();
    
    if (allocator->initialize)
    {
        return allocator->initialize();
    }
    
    return true;
}

void CN_memory_shutdown(void)
{
    Stru_CN_AllocatorInterface_t* allocator = get_current_allocator();
    
    if (allocator->shutdown)
    {
        allocator->shutdown();
    }
}

void* cn_malloc(size_t size, const char* purpose)
{
    Stru_CN_AllocatorInterface_t* allocator = get_current_allocator();
    return allocator->allocate(size, purpose);
}

void* cn_malloc_aligned(size_t size, size_t alignment, const char* purpose)
{
    Stru_CN_AllocatorInterface_t* allocator = get_current_allocator();
    return allocator->allocate_aligned(size, alignment, purpose);
}

void* cn_calloc(size_t count, size_t size, const char* purpose)
{
    size_t total_size = count * size;
    void* ptr = cn_malloc(total_size, purpose);
    if (ptr)
    {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void* cn_realloc(void* ptr, size_t new_size, const char* purpose)
{
    Stru_CN_AllocatorInterface_t* allocator = get_current_allocator();
    return allocator->reallocate(ptr, new_size, purpose);
}

void cn_free(void* ptr)
{
    Stru_CN_AllocatorInterface_t* allocator = get_current_allocator();
    allocator->deallocate(ptr);
}

bool cn_set_allocator(Eum_CN_AllocatorType_t allocator_type)
{
    // 先关闭当前分配器
    CN_memory_shutdown();
    
    // 设置新分配器
    g_current_allocator = allocator_type;
    
    // 初始化新分配器
    return CN_memory_init(allocator_type);
}

Eum_CN_AllocatorType_t cn_get_allocator_type(void)
{
    return g_current_allocator;
}

Stru_CN_MemoryStats_t cn_get_memory_stats(void)
{
    Stru_CN_AllocatorInterface_t* allocator = get_current_allocator();
    return allocator->get_stats();
}

void cn_reset_memory_stats(void)
{
    Stru_CN_AllocatorInterface_t* allocator = get_current_allocator();
    allocator->reset_stats();
}

bool cn_check_memory_leaks(void)
{
    Stru_CN_MemoryStats_t stats = cn_get_memory_stats();
    return stats.current_usage > 0;
}

void cn_dump_memory_stats(void)
{
    Stru_CN_MemoryStats_t stats = cn_get_memory_stats();
    
    printf("=== Memory Statistics ===\n");
    printf("Total allocated: %lu bytes\n", (unsigned long)stats.total_allocated);
    printf("Total freed: %lu bytes\n", (unsigned long)stats.total_freed);
    printf("Current usage: %lu bytes\n", (unsigned long)stats.current_usage);
    printf("Peak usage: %lu bytes\n", (unsigned long)stats.peak_usage);
    printf("Allocation count: %lu\n", (unsigned long)stats.allocation_count);
    printf("Free count: %lu\n", (unsigned long)stats.free_count);
    printf("Leak count: %lu\n", (unsigned long)stats.leak_count);
    printf("=========================\n");
}

// ============================================================================
// 内存调试工具实现
// ============================================================================

void cn_enable_memory_debug(bool track_allocations, unsigned char fill_pattern)
{
    g_debug_enabled = true;
    g_track_allocations = track_allocations;
    g_fill_pattern = fill_pattern;
}

void cn_disable_memory_debug(void)
{
    g_debug_enabled = false;
    g_track_allocations = false;
    g_fill_pattern = 0xCC;
}

bool cn_validate_heap(void)
{
    // 简单实现：总是返回true
    // 实际实现应该检查堆的完整性
    return true;
}

void cn_set_allocation_failure_simulation(size_t fail_after)
{
    g_allocation_failure_after = fail_after;
    g_allocation_count = 0;
}
