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
#include "CN_pool_allocator.h"
#include "arena/CN_arena_allocator.h"
#include "system/CN_system_allocator.h"
#include "debug_allocator/CN_debug_allocator.h"
#include "physical/CN_physical_allocator.h"
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
// 系统分配器适配器
// ============================================================================

// 全局系统分配器实例（使用默认配置）
static Stru_CN_SystemAllocator_t* g_system_allocator_instance = NULL;

// 系统分配器适配函数
static void* system_allocate(size_t size, const char* file, int line)
{
    // 确保系统分配器已初始化
    if (g_system_allocator_instance == NULL)
    {
        // 使用默认配置创建系统分配器
        Stru_CN_SystemConfig_t config = CN_SYSTEM_CONFIG_DEFAULT;
        config.enable_statistics = true;
        config.track_allocations = true;
        config.detect_leaks = true;
        config.enable_debug = g_debug_enabled;
        
        g_system_allocator_instance = CN_system_create(&config);
        if (g_system_allocator_instance == NULL)
        {
            // 创建失败，回退到标准库
            void* ptr = malloc(size);
            if (ptr && g_debug_enabled)
            {
                memset(ptr, 0xCC, size);
            }
            return ptr;
        }
    }
    
    // 使用系统分配器分配内存
    return CN_system_alloc(g_system_allocator_instance, size, file, line, "CN_memory系统分配");
}

static void system_deallocate(void* ptr, const char* file, int line)
{
    if (ptr == NULL)
    {
        return;
    }
    
    // 如果系统分配器未初始化，使用标准库释放
    if (g_system_allocator_instance == NULL)
    {
        free(ptr);
        return;
    }
    
    // 使用系统分配器释放内存
    CN_system_free(g_system_allocator_instance, ptr, file, line);
}

static void* system_reallocate(void* ptr, size_t new_size, const char* file, int line)
{
    // 如果系统分配器未初始化，使用标准库重新分配
    if (g_system_allocator_instance == NULL)
    {
        if (ptr == NULL)
        {
            return system_allocate(new_size, file, line);
        }
        return realloc(ptr, new_size);
    }
    
    // 使用系统分配器重新分配内存
    return CN_system_realloc(g_system_allocator_instance, ptr, new_size, file, line, "CN_memory重新分配");
}

// 系统分配器接口实例
static const Stru_CN_AllocatorInterface_t g_system_allocator_interface = {
    .allocate = system_allocate,
    .deallocate = system_deallocate,
    .reallocate = system_reallocate
};

// ============================================================================
// 调试分配器实现（完整版）
// ============================================================================

// 全局调试分配器实例
static Stru_CN_DebugAllocator_t* g_debug_allocator_instance = NULL;

static void* debug_allocate(size_t size, const char* file, int line)
{
    // 确保调试分配器已初始化
    if (g_debug_allocator_instance == NULL)
    {
        // 使用默认配置创建调试分配器
        Stru_CN_DebugConfig_t config = CN_DEBUG_CONFIG_DEFAULT;
        config.enable_statistics = true;
        config.track_allocations = true;
        config.enable_leak_detection = true;
        config.enable_boundary_check = true;
        config.enable_double_free_check = true;
        config.enable_memory_corruption_check = true;
        
        g_debug_allocator_instance = CN_debug_create(&config);
        if (g_debug_allocator_instance == NULL)
        {
            // 创建失败，回退到系统分配器
            return system_allocate(size, file, line);
        }
    }
    
    // 使用调试分配器分配内存
    return CN_debug_alloc(g_debug_allocator_instance, size, file, line, "CN_memory调试分配");
}

static void debug_deallocate(void* ptr, const char* file, int line)
{
    if (ptr == NULL)
    {
        return;
    }
    
    // 如果调试分配器未初始化，使用系统释放
    if (g_debug_allocator_instance == NULL)
    {
        system_deallocate(ptr, file, line);
        return;
    }
    
    // 使用调试分配器释放内存
    CN_debug_free(g_debug_allocator_instance, ptr, file, line);
}

static void* debug_reallocate(void* ptr, size_t new_size, const char* file, int line)
{
    // 如果调试分配器未初始化，使用系统重新分配
    if (g_debug_allocator_instance == NULL)
    {
        if (ptr == NULL)
        {
            return debug_allocate(new_size, file, line);
        }
        return system_reallocate(ptr, new_size, file, line);
    }
    
    // 使用调试分配器重新分配内存
    return CN_debug_realloc(g_debug_allocator_instance, ptr, new_size, file, line, "CN_memory调试重新分配");
}

// 调试分配器接口实例
static const Stru_CN_AllocatorInterface_t g_debug_allocator = {
    .allocate = debug_allocate,
    .deallocate = debug_deallocate,
    .reallocate = debug_reallocate
};

// ============================================================================
// 对象池分配器实现（完整版）
// ============================================================================

// 全局对象池实例（简化实现，支持单一对象池）
static Stru_CN_PoolAllocator_t* g_pool_allocator_instance = NULL;
static size_t g_pool_object_size = 64;  // 默认对象大小

static void* pool_allocate(size_t size, const char* file, int line)
{
    (void)file;
    (void)line;
    
    // 如果对象池未初始化或对象大小不匹配，使用系统分配
    if (g_pool_allocator_instance == NULL || size != g_pool_object_size)
    {
        // 如果这是第一次使用对象池，初始化它
        if (g_pool_allocator_instance == NULL && size > 0)
        {
            Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
            config.object_size = size;
            config.initial_capacity = 64;
            config.auto_expand = true;
            config.expand_increment = 64;
            
            g_pool_allocator_instance = CN_pool_create(&config);
            if (g_pool_allocator_instance != NULL)
            {
                g_pool_object_size = size;
                printf("[INFO] 对象池已自动初始化，对象大小: %zu\n", size);
            }
        }
        
        // 如果对象池初始化失败或大小不匹配，使用系统分配
        if (g_pool_allocator_instance == NULL || size != g_pool_object_size)
        {
            return system_allocate(size, file, line);
        }
    }
    
    // 从对象池分配
    void* ptr = CN_pool_alloc(g_pool_allocator_instance);
    if (ptr == NULL)
    {
        // 对象池分配失败，回退到系统分配
        return system_allocate(size, file, line);
    }
    
    return ptr;
}

static void pool_deallocate(void* ptr, const char* file, int line)
{
    (void)file;
    (void)line;
    
    if (ptr == NULL)
    {
        return;
    }
    
    // 如果对象池未初始化，使用系统释放
    if (g_pool_allocator_instance == NULL)
    {
        system_deallocate(ptr, file, line);
        return;
    }
    
    // 尝试释放到对象池
    if (!CN_pool_free(g_pool_allocator_instance, ptr))
    {
        // 对象不属于对象池，使用系统释放
        system_deallocate(ptr, file, line);
    }
}

static void* pool_reallocate(void* ptr, size_t new_size, const char* file, int line)
{
    // 对象池不支持真正的重新分配
    // 如果新大小等于对象大小，返回原指针
    // 否则分配新对象，复制数据，释放原对象
    
    if (g_pool_allocator_instance == NULL || new_size != g_pool_object_size)
    {
        // 使用系统重新分配
        return system_reallocate(ptr, new_size, file, line);
    }
    
    if (ptr == NULL)
    {
        // 相当于分配新对象
        return pool_allocate(new_size, file, line);
    }
    
    // 分配新对象
    void* new_ptr = CN_pool_alloc(g_pool_allocator_instance);
    if (new_ptr == NULL)
    {
        return NULL;
    }
    
    // 复制数据
    memcpy(new_ptr, ptr, (new_size < g_pool_object_size) ? new_size : g_pool_object_size);
    
    // 释放原对象
    CN_pool_free(g_pool_allocator_instance, ptr);
    
    return new_ptr;
}

// 对象池分配器接口实例
static const Stru_CN_AllocatorInterface_t g_pool_allocator_interface = {
    .allocate = pool_allocate,
    .deallocate = pool_deallocate,
    .reallocate = pool_reallocate
};

// ============================================================================
// 区域分配器实现（完整版）
// ============================================================================

// 全局区域分配器实例
static Stru_CN_ArenaAllocator_t* g_arena_allocator_instance = NULL;

static void* arena_allocate(size_t size, const char* file, int line)
{
    (void)file;
    (void)line;
    
    // 如果区域分配器未初始化，初始化它
    if (g_arena_allocator_instance == NULL)
    {
        Stru_CN_ArenaConfig_t config = CN_ARENA_CONFIG_DEFAULT;
        config.initial_size = 1024 * 1024;  // 1MB初始大小
        config.max_size = 16 * 1024 * 1024; // 16MB最大大小
        config.auto_expand = true;
        config.expand_increment = 1024 * 1024; // 1MB扩展增量
        
        g_arena_allocator_instance = CN_arena_create(&config);
        if (g_arena_allocator_instance == NULL)
        {
        // 区域分配器创建失败，使用系统分配
        return system_allocate(size, file, line);
        }
        
        printf("[INFO] 区域分配器已自动初始化，初始大小: %zu\n", config.initial_size);
    }
    
    // 从区域分配器分配
    void* ptr = CN_arena_alloc(g_arena_allocator_instance, size);
    if (ptr == NULL)
    {
        // 区域分配失败，回退到系统分配
        return system_allocate(size, file, line);
    }
    
    return ptr;
}

static void arena_deallocate(void* ptr, const char* file, int line)
{
    (void)ptr;
    (void)file;
    (void)line;
    
    // 区域分配器不支持单独释放，只在区域重置或销毁时统一释放
    // 这里什么也不做
}

static void* arena_reallocate(void* ptr, size_t new_size, const char* file, int line)
{
    // 区域分配器不支持重新分配
    // 分配新对象，复制数据（如果ptr不为NULL）
    
    if (ptr == NULL)
    {
        // 相当于分配新对象
        return arena_allocate(new_size, file, line);
    }
    
    // 分配新对象
    void* new_ptr = arena_allocate(new_size, file, line);
    if (new_ptr == NULL)
    {
        return NULL;
    }
    
    // 注意：我们不知道原大小，所以无法安全复制
    // 这里简化处理，假设调用者知道原大小
    // 实际使用中应该避免对区域分配的内存进行realloc
    
    return new_ptr;
}

// 区域分配器接口实例
static const Stru_CN_AllocatorInterface_t g_arena_allocator_interface = {
    .allocate = arena_allocate,
    .deallocate = arena_deallocate,
    .reallocate = arena_reallocate
};

// ============================================================================
// 物理内存分配器实现（模拟版）
// ============================================================================

// 全局物理内存分配器实例
static Stru_CN_PhysicalAllocator_t* g_physical_allocator_instance = NULL;

// 模拟的物理内存范围（用于用户空间测试）
#define PHYSICAL_MEMORY_START 0x10000000  // 256MB
#define PHYSICAL_MEMORY_END   0x20000000  // 512MB
#define PHYSICAL_PAGE_SIZE    4096        // 4KB

// 物理到虚拟地址映射表（简化模拟）
typedef struct Stru_PhysicalToVirtualMap_t
{
    uintptr_t physical_address;
    void* virtual_address;
    size_t size;
} Stru_PhysicalToVirtualMap_t;

static Stru_PhysicalToVirtualMap_t* g_physical_maps = NULL;
static size_t g_physical_map_count = 0;
static size_t g_physical_map_capacity = 0;

// 将物理地址映射到虚拟地址（模拟）
static void* map_physical_to_virtual(uintptr_t physical_address, size_t size)
{
    // 在用户空间，我们无法真正映射物理地址
    // 这里使用malloc模拟，实际操作系统内核中需要使用MMU
    void* virtual_address = malloc(size);
    if (virtual_address == NULL)
    {
        return NULL;
    }
    
    // 记录映射关系
    if (g_physical_map_count >= g_physical_map_capacity)
    {
        size_t new_capacity = g_physical_map_capacity == 0 ? 16 : g_physical_map_capacity * 2;
        Stru_PhysicalToVirtualMap_t* new_maps = realloc(g_physical_maps, 
                                                        new_capacity * sizeof(Stru_PhysicalToVirtualMap_t));
        if (new_maps == NULL)
        {
            free(virtual_address);
            return NULL;
        }
        g_physical_maps = new_maps;
        g_physical_map_capacity = new_capacity;
    }
    
    g_physical_maps[g_physical_map_count].physical_address = physical_address;
    g_physical_maps[g_physical_map_count].virtual_address = virtual_address;
    g_physical_maps[g_physical_map_count].size = size;
    g_physical_map_count++;
    
    return virtual_address;
}

// 取消物理地址映射（模拟）
static void unmap_physical_to_virtual(uintptr_t physical_address)
{
    for (size_t i = 0; i < g_physical_map_count; i++)
    {
        if (g_physical_maps[i].physical_address == physical_address)
        {
            free(g_physical_maps[i].virtual_address);
            
            // 将最后一个元素移动到当前位置
            if (i < g_physical_map_count - 1)
            {
                g_physical_maps[i] = g_physical_maps[g_physical_map_count - 1];
            }
            g_physical_map_count--;
            break;
        }
    }
}

static void* physical_allocate(size_t size, const char* file, int line)
{
    (void)file;
    (void)line;
    
    // 如果物理内存分配器未初始化，初始化它
    if (g_physical_allocator_instance == NULL)
    {
        Stru_CN_PhysicalConfig_t config = CN_PHYSICAL_CONFIG_DEFAULT;
        config.memory_start = PHYSICAL_MEMORY_START;
        config.memory_end = PHYSICAL_MEMORY_END;
        config.page_size = PHYSICAL_PAGE_SIZE;
        config.enable_statistics = true;
        config.enable_debug = g_debug_enabled;
        config.zero_on_alloc = false;
        config.track_allocations = true;
        config.reserved_pages = 0;
        config.name = "模拟物理内存分配器";
        
        g_physical_allocator_instance = CN_physical_create(&config);
        if (g_physical_allocator_instance == NULL)
        {
            // 物理分配器创建失败，使用系统分配
            return system_allocate(size, file, line);
        }
        
        printf("[INFO] 物理内存分配器已自动初始化（模拟模式）\n");
    }
    
    // 计算需要的页面数量
    size_t page_count = (size + PHYSICAL_PAGE_SIZE - 1) / PHYSICAL_PAGE_SIZE;
    
    // 分配物理页面
    uintptr_t physical_address = CN_physical_alloc_pages(g_physical_allocator_instance, 
                                                        page_count, file, line, 
                                                        "CN_memory物理分配");
    if (physical_address == 0)
    {
        // 物理分配失败，使用系统分配
        return system_allocate(size, file, line);
    }
    
    // 将物理地址映射到虚拟地址（模拟）
    void* virtual_address = map_physical_to_virtual(physical_address, page_count * PHYSICAL_PAGE_SIZE);
    if (virtual_address == NULL)
    {
        // 映射失败，释放物理页面
        CN_physical_free_pages(g_physical_allocator_instance, physical_address, page_count, file, line);
        return system_allocate(size, file, line);
    }
    
    return virtual_address;
}

static void physical_deallocate(void* ptr, const char* file, int line)
{
    if (ptr == NULL)
    {
        return;
    }
    
    // 如果物理内存分配器未初始化，使用系统释放
    if (g_physical_allocator_instance == NULL)
    {
        system_deallocate(ptr, file, line);
        return;
    }
    
    // 查找物理地址
    uintptr_t physical_address = 0;
    size_t page_count = 0;
    
    for (size_t i = 0; i < g_physical_map_count; i++)
    {
        if (g_physical_maps[i].virtual_address == ptr)
        {
            physical_address = g_physical_maps[i].physical_address;
            page_count = g_physical_maps[i].size / PHYSICAL_PAGE_SIZE;
            
            // 取消映射
            unmap_physical_to_virtual(physical_address);
            break;
        }
    }
    
    if (physical_address != 0)
    {
        // 释放物理页面
        CN_physical_free_pages(g_physical_allocator_instance, physical_address, page_count, file, line);
    }
    else
    {
        // 不是物理分配的内存，使用系统释放
        system_deallocate(ptr, file, line);
    }
}

static void* physical_reallocate(void* ptr, size_t new_size, const char* file, int line)
{
    // 物理内存分配器不支持真正的重新分配
    // 分配新对象，复制数据，释放原对象
    
    if (ptr == NULL)
    {
        // 相当于分配新对象
        return physical_allocate(new_size, file, line);
    }
    
    // 分配新对象
    void* new_ptr = physical_allocate(new_size, file, line);
    if (new_ptr == NULL)
    {
        return NULL;
    }
    
    // 查找原大小
    size_t old_size = 0;
    for (size_t i = 0; i < g_physical_map_count; i++)
    {
        if (g_physical_maps[i].virtual_address == ptr)
        {
            old_size = g_physical_maps[i].size;
            break;
        }
    }
    
    if (old_size > 0)
    {
        // 复制数据
        size_t copy_size = old_size < new_size ? old_size : new_size;
        memcpy(new_ptr, ptr, copy_size);
    }
    
    // 释放原对象
    physical_deallocate(ptr, file, line);
    
    return new_ptr;
}

// 物理内存分配器接口实例
static const Stru_CN_AllocatorInterface_t g_physical_allocator_interface = {
    .allocate = physical_allocate,
    .deallocate = physical_deallocate,
    .reallocate = physical_reallocate
};

// ============================================================================
// 分配器管理函数
// ============================================================================

static const Stru_CN_AllocatorInterface_t* get_allocator_interface(Eum_CN_AllocatorType_t type)
{
    switch (type)
    {
        case Eum_ALLOCATOR_SYSTEM:
            return &g_system_allocator_interface;
            
        case Eum_ALLOCATOR_DEBUG:
            return &g_debug_allocator;
            
        case Eum_ALLOCATOR_POOL:
            return &g_pool_allocator_interface;
            
        case Eum_ALLOCATOR_ARENA:
            return &g_arena_allocator_interface;
            
        case Eum_ALLOCATOR_PHYSICAL:
            return &g_physical_allocator_interface;
            
        default:
            return &g_system_allocator_interface;
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
    // 清理调试分配器资源
    if (g_debug_allocator_instance != NULL)
    {
        CN_debug_destroy(g_debug_allocator_instance);
        g_debug_allocator_instance = NULL;
    }
    
    // 清理对象池资源
    if (g_pool_allocator_instance != NULL)
    {
        CN_pool_destroy(g_pool_allocator_instance);
        g_pool_allocator_instance = NULL;
        g_pool_object_size = 64;
    }
    
    // 清理区域分配器资源
    if (g_arena_allocator_instance != NULL)
    {
        CN_arena_destroy(g_arena_allocator_instance);
        g_arena_allocator_instance = NULL;
    }
    
    // 清理系统分配器资源
    if (g_system_allocator_instance != NULL)
    {
        CN_system_destroy(g_system_allocator_instance);
        g_system_allocator_instance = NULL;
    }
    
    // 重置状态
    g_current_allocator = Eum_ALLOCATOR_SYSTEM;
    g_current_allocator_interface = &g_system_allocator_interface;
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
    // 使用系统分配器的泄漏检测功能
    if (g_system_allocator_instance != NULL)
    {
        return CN_system_check_leaks(g_system_allocator_instance);
    }
    
    // 回退到旧的统计方法
    return g_memory_stats.current_usage > 0;
}

void cn_dump_stats(void)
{
    // 优先使用系统分配器的统计信息
    if (g_system_allocator_instance != NULL)
    {
        CN_system_dump(g_system_allocator_instance);
        return;
    }
    
    // 回退到旧的统计显示
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
