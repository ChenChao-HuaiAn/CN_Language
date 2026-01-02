/******************************************************************************
 * 文件名: CN_memory.c
 * 功能: CN_Language统一内存管理实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 重构为统一内存管理接口，支持多种分配器策略
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

// 当前分配器接口
static const Stru_CN_AllocatorInterface_t* g_current_allocator_interface = NULL;

// 调试模式
static bool g_debug_enabled = false;

// 内存统计信息
typedef struct Stru_CN_MemoryStats_t
{
    size_t total_allocated;        /**< 总分配字节数 */
    size_t total_freed;            /**< 总释放字节数 */
    size_t current_usage;          /**< 当前使用字节数 */
    size_t peak_usage;             /**< 峰值使用字节数 */
    size_t allocation_count;       /**< 分配次数 */
    size_t free_count;             /**< 释放次数 */
} Stru_CN_MemoryStats_t;

static Stru_CN_MemoryStats_t g_memory_stats = {0};

// ============================================================================
// 系统分配器实现
// ============================================================================

static void* system_allocate(size_t size, const char* file, int line)
{
    (void)file; // 暂未使用
    (void)line; // 暂未使用
    
    void* ptr = malloc(size);
    if (ptr)
    {
        g_memory_stats.total_allocated += size;
        g_memory_stats.current_usage += size;
        g_memory_stats.allocation_count++;
        
        if (g_memory_stats.current_usage > g_memory_stats.peak_usage)
        {
            g_memory_stats.peak_usage = g_memory_stats.current_usage;
        }
        
        if (g_debug_enabled)
        {
            // 调试模式下填充特定模式
            memset(ptr, 0xCC, size);
        }
    }
    
    return ptr;
}

static void system_deallocate(void* ptr, const char* file, int line)
{
    (void)file; // 暂未使用
    (void)line; // 暂未使用
    
    if (ptr == NULL)
    {
        return;
    }
    
    // 注意：我们无法知道释放的大小，所以统计可能不准确
    // 在实际实现中，需要跟踪分配大小
    free(ptr);
    g_memory_stats.free_count++;
    
    // 更新总释放字节数（估算）
    // 假设平均每次分配大小 = 总分配字节数 / 分配次数
    if (g_memory_stats.allocation_count > 0)
    {
        size_t avg_size = g_memory_stats.total_allocated / g_memory_stats.allocation_count;
        g_memory_stats.total_freed += avg_size;
        
        // 减少当前使用量（估算）
        if (g_memory_stats.current_usage > avg_size)
        {
            g_memory_stats.current_usage -= avg_size;
        }
        else
        {
            g_memory_stats.current_usage = 0;
        }
    }
}

static void* system_reallocate(void* ptr, size_t new_size, const char* file, int line)
{
    (void)file; // 暂未使用
    (void)line; // 暂未使用
    
    if (ptr == NULL)
    {
        return system_allocate(new_size, file, line);
    }
    
    // 注意：realloc的统计处理不准确，因为我们不知道原大小
    void* new_ptr = realloc(ptr, new_size);
    if (new_ptr)
    {
        // 简化处理：假设重新分配成功
        g_memory_stats.total_allocated += new_size;
        g_memory_stats.allocation_count++;
    }
    
    return new_ptr;
}

// 系统分配器接口实例
static const Stru_CN_AllocatorInterface_t g_system_allocator = {
    .allocate = system_allocate,
    .deallocate = system_deallocate,
    .reallocate = system_reallocate
};

// ============================================================================
// 调试分配器实现（简化版）
// ============================================================================

static void* debug_allocate(size_t size, const char* file, int line)
{
    // 调试信息输出
    if (g_debug_enabled)
    {
        printf("[DEBUG] 分配内存: %zu 字节, 文件: %s, 行: %d\n", size, file, line);
    }
    
    // 实际使用系统分配器
    return system_allocate(size, file, line);
}

static void debug_deallocate(void* ptr, const char* file, int line)
{
    if (g_debug_enabled)
    {
        printf("[DEBUG] 释放内存: 地址: %p, 文件: %s, 行: %d\n", ptr, file, line);
    }
    
    system_deallocate(ptr, file, line);
}

static void* debug_reallocate(void* ptr, size_t new_size, const char* file, int line)
{
    if (g_debug_enabled)
    {
        printf("[DEBUG] 重新分配内存: 原地址: %p, 新大小: %zu, 文件: %s, 行: %d\n", 
               ptr, new_size, file, line);
    }
    
    return system_reallocate(ptr, new_size, file, line);
}

// 调试分配器接口实例
static const Stru_CN_AllocatorInterface_t g_debug_allocator = {
    .allocate = debug_allocate,
    .deallocate = debug_deallocate,
    .reallocate = debug_reallocate
};

// ============================================================================
// 对象池分配器实现（简化版）
// ============================================================================

// 对象池分配器内部结构
typedef struct Stru_CN_PoolAllocator_t
{
    size_t object_size;
    size_t capacity;
    void** free_list;
    size_t free_count;
} Stru_CN_PoolAllocator_t;

static Stru_CN_PoolAllocator_t g_pool_allocator = {0};

static void* pool_allocate(size_t size, const char* file, int line)
{
    (void)file;
    (void)line;
    
    // 简化实现：如果对象大小不匹配或池未初始化，使用系统分配
    if (g_pool_allocator.object_size == 0 || g_pool_allocator.object_size != size)
    {
        return system_allocate(size, file, line);
    }
    
    // 从空闲列表获取对象
    if (g_pool_allocator.free_count > 0)
    {
        void* obj = g_pool_allocator.free_list[--g_pool_allocator.free_count];
        return obj;
    }
    
    // 空闲列表为空，使用系统分配
    return system_allocate(size, file, line);
}

static void pool_deallocate(void* ptr, const char* file, int line)
{
    (void)file;
    (void)line;
    
    if (ptr == NULL)
    {
        return;
    }
    
    // 如果池已满，直接释放
    if (g_pool_allocator.free_count >= g_pool_allocator.capacity)
    {
        system_deallocate(ptr, file, line);
        return;
    }
    
    // 添加到空闲列表
    g_pool_allocator.free_list[g_pool_allocator.free_count++] = ptr;
}

static void* pool_reallocate(void* ptr, size_t new_size, const char* file, int line)
{
    // 对象池不支持重新分配，使用系统重新分配
    return system_reallocate(ptr, new_size, file, line);
}

// 对象池分配器接口实例
static const Stru_CN_AllocatorInterface_t g_pool_allocator_interface = {
    .allocate = pool_allocate,
    .deallocate = pool_deallocate,
    .reallocate = pool_reallocate
};

// ============================================================================
// 区域分配器实现（简化版）
// ============================================================================

// 区域分配器内部结构
typedef struct Stru_CN_ArenaAllocator_t
{
    void* memory;
    size_t size;
    size_t used;
} Stru_CN_ArenaAllocator_t;

static Stru_CN_ArenaAllocator_t g_arena_allocator = {0};

static void* arena_allocate(size_t size, const char* file, int line)
{
    (void)file;
    (void)line;
    
    // 检查是否有足够空间
    if (g_arena_allocator.memory == NULL || 
        g_arena_allocator.used + size > g_arena_allocator.size)
    {
        // 区域不足，使用系统分配
        return system_allocate(size, file, line);
    }
    
    void* ptr = (char*)g_arena_allocator.memory + g_arena_allocator.used;
    g_arena_allocator.used += size;
    
    return ptr;
}

static void arena_deallocate(void* ptr, const char* file, int line)
{
    (void)ptr;
    (void)file;
    (void)line;
    
    // 区域分配器不支持单独释放，只在区域销毁时统一释放
    // 这里什么也不做
}

static void* arena_reallocate(void* ptr, size_t new_size, const char* file, int line)
{
    // 区域分配器不支持重新分配，使用系统重新分配
    return system_reallocate(ptr, new_size, file, line);
}

// 区域分配器接口实例
static const Stru_CN_AllocatorInterface_t g_arena_allocator_interface = {
    .allocate = arena_allocate,
    .deallocate = arena_deallocate,
    .reallocate = arena_reallocate
};

// ============================================================================
// 分配器管理函数
// ============================================================================

static const Stru_CN_AllocatorInterface_t* get_allocator_interface(Eum_CN_AllocatorType_t type)
{
    switch (type)
    {
        case Eum_ALLOCATOR_SYSTEM:
            return &g_system_allocator;
            
        case Eum_ALLOCATOR_DEBUG:
            return &g_debug_allocator;
            
        case Eum_ALLOCATOR_POOL:
            return &g_pool_allocator_interface;
            
        case Eum_ALLOCATOR_ARENA:
            return &g_arena_allocator_interface;
            
        default:
            return &g_system_allocator;
    }
}

bool CN_memory_init(Eum_CN_AllocatorType_t allocator_type)
{
    g_current_allocator = allocator_type;
    g_current_allocator_interface = get_allocator_interface(allocator_type);
    
    // 初始化统计信息
    memset(&g_memory_stats, 0, sizeof(g_memory_stats));
    
    return true;
}

void CN_memory_shutdown(void)
{
    // 清理资源
    if (g_pool_allocator.free_list != NULL)
    {
        free(g_pool_allocator.free_list);
        g_pool_allocator.free_list = NULL;
    }
    
    if (g_arena_allocator.memory != NULL)
    {
        free(g_arena_allocator.memory);
        g_arena_allocator.memory = NULL;
    }
    
    // 重置状态
    g_current_allocator = Eum_ALLOCATOR_SYSTEM;
    g_current_allocator_interface = &g_system_allocator;
    memset(&g_memory_stats, 0, sizeof(g_memory_stats));
}

bool cn_set_allocator(Eum_CN_AllocatorType_t allocator_type)
{
    // 先关闭当前分配器
    CN_memory_shutdown();
    
    // 设置新分配器
    return CN_memory_init(allocator_type);
}

Eum_CN_AllocatorType_t cn_get_allocator_type(void)
{
    return g_current_allocator;
}

const Stru_CN_AllocatorInterface_t* cn_get_allocator(void)
{
    return g_current_allocator_interface;
}

// ============================================================================
// 统一内存分配包装函数
// ============================================================================

void* cn_malloc(size_t size)
{
    if (g_current_allocator_interface == NULL)
    {
        // 默认使用系统分配器
        return system_allocate(size, __FILE__, __LINE__);
    }
    
    return g_current_allocator_interface->allocate(size, __FILE__, __LINE__);
}

void cn_free(void* ptr)
{
    if (g_current_allocator_interface == NULL || ptr == NULL)
    {
        return;
    }
    
    g_current_allocator_interface->deallocate(ptr, __FILE__, __LINE__);
}

void* cn_realloc(void* ptr, size_t new_size)
{
    if (g_current_allocator_interface == NULL)
    {
        return system_reallocate(ptr, new_size, __FILE__, __LINE__);
    }
    
    return g_current_allocator_interface->reallocate(ptr, new_size, __FILE__, __LINE__);
}

void* cn_calloc(size_t count, size_t size)
{
    size_t total_size = count * size;
    void* ptr = cn_malloc(total_size);
    if (ptr != NULL)
    {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

// ============================================================================
// 调试和统计函数
// ============================================================================

void cn_enable_debug(bool enable)
{
    g_debug_enabled = enable;
}

bool cn_check_leaks(void)
{
    return g_memory_stats.current_usage > 0;
}

void cn_dump_stats(void)
{
    printf("=== 内存统计信息 ===\n");
    printf("总分配字节数: %zu\n", g_memory_stats.total_allocated);
    printf("总释放字节数: %zu\n", g_memory_stats.total_freed);
    printf("当前使用字节数: %zu\n", g_memory_stats.current_usage);
    printf("峰值使用字节数: %zu\n", g_memory_stats.peak_usage);
    printf("分配次数: %zu\n", g_memory_stats.allocation_count);
    printf("释放次数: %zu\n", g_memory_stats.free_count);
    printf("========================\n");
}

bool cn_validate_heap(void)
{
    // 简化实现：总是返回true
    // 实际实现应该检查堆的完整性
    return true;
}
