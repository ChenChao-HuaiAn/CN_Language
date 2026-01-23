#include "cnlang/runtime/memory.h"
#include "cnlang/runtime/freestanding.h"
#include <stddef.h>
#include <stdint.h>

// 前向声明 freestanding 函数（它们在 freestanding_builtins.c 中实现）
void* cn_rt_memcpy(void* dest, const void* src, size_t n);
void* cn_rt_memset(void* dest, int c, size_t n);

/*
 * CN Language Freestanding 静态内存分配器
 * 
 * 提供一个简单的静态内存池管理方案，用于 freestanding 模式下
 * 无法使用标准 malloc/free 的场景（如 OS 内核早期启动阶段）。
 * 
 * 特点：
 * - 固定大小内存池（编译时配置）
 * - 简单的 bump allocator（线性分配器）
 * - 不支持释放单个块（仅支持重置整个池）
 * - 适用于短生命周期或无需释放的场景
 * 
 * 这些函数始终编译，但仅在定义 CN_FREESTANDING 宏时声明。
 */

// =============================================================================
// 配置参数
// =============================================================================

// 默认静态内存池大小（64KB）
#ifndef CN_FS_MEMORY_POOL_SIZE
#define CN_FS_MEMORY_POOL_SIZE (64 * 1024)
#endif

// 内存对齐大小
#ifndef CN_FS_MEMORY_ALIGN
#define CN_FS_MEMORY_ALIGN 8
#endif

// =============================================================================
// 内存池数据结构
// =============================================================================

static uint8_t g_memory_pool[CN_FS_MEMORY_POOL_SIZE];
static size_t g_pool_offset = 0;
static size_t g_allocation_count = 0;

// =============================================================================
// 辅助函数
// =============================================================================

/**
 * 将地址按指定对齐要求向上对齐
 */
static size_t align_up(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

// =============================================================================
// 静态分配器实现
// =============================================================================

/**
 * Freestanding 模式下的 malloc 实现
 * 使用简单的 bump allocator 策略
 */
static void* fs_malloc(size_t size, void* user_data) {
    (void)user_data; // 未使用
    
    if (size == 0) {
        return NULL;
    }
    
    // 对齐分配大小
    size_t aligned_size = align_up(size, CN_FS_MEMORY_ALIGN);
    
    // 检查是否有足够空间
    if (g_pool_offset + aligned_size > CN_FS_MEMORY_POOL_SIZE) {
        // 内存池耗尽
        return NULL;
    }
    
    // 分配内存
    void* ptr = &g_memory_pool[g_pool_offset];
    g_pool_offset += aligned_size;
    g_allocation_count++;
    
    // 初始化为零（类似 calloc 行为）
    cn_rt_memset(ptr, 0, size);
    
    return ptr;
}

/**
 * Freestanding 模式下的 free 实现
 * 注意：此实现不会真正释放内存，仅记录调用
 */
static void fs_free(void* ptr, void* user_data) {
    (void)ptr;       // 未使用
    (void)user_data; // 未使用
    
    // 在 bump allocator 中无法释放单个块
    // 仅用于统计或调试目的
}

/**
 * Freestanding 模式下的 realloc 实现
 * 总是分配新块并复制数据（效率较低）
 */
static void* fs_realloc(void* ptr, size_t new_size, void* user_data) {
    if (ptr == NULL) {
        return fs_malloc(new_size, user_data);
    }
    
    if (new_size == 0) {
        fs_free(ptr, user_data);
        return NULL;
    }
    
    // 分配新块
    void* new_ptr = fs_malloc(new_size, user_data);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    // 复制旧数据
    // 注意：我们无法知道原始块的大小，这里假设调用者知道
    // 在实际使用中，可以在每个分配块前存储大小元数据
    cn_rt_memcpy(new_ptr, ptr, new_size);
    
    // 注意：旧块不会被释放
    
    return new_ptr;
}

// =============================================================================
// 公共接口
// =============================================================================

/**
 * 初始化 freestanding 模式的静态分配器
 * 此函数应在运行时初始化时调用
 */
void cn_rt_freestanding_init_allocator(void) {
    // 配置内存管理器使用静态分配器
    CnRtMemoryConfig config = {
        .alloc_func = fs_malloc,
        .free_func = fs_free,
        .realloc_func = fs_realloc,
        .user_data = NULL
    };
    
    cn_rt_memory_init(&config);
}

/**
 * 重置内存池（释放所有已分配内存）
 * 警告：会使所有之前的分配失效！
 */
void cn_rt_freestanding_reset_pool(void) {
    g_pool_offset = 0;
    g_allocation_count = 0;
    cn_rt_memset(g_memory_pool, 0, CN_FS_MEMORY_POOL_SIZE);
}

/**
 * 获取内存池使用信息
 */
void cn_rt_freestanding_get_pool_info(size_t* used_bytes, size_t* total_bytes, size_t* alloc_count) {
    if (used_bytes) {
        *used_bytes = g_pool_offset;
    }
    if (total_bytes) {
        *total_bytes = CN_FS_MEMORY_POOL_SIZE;
    }
    if (alloc_count) {
        *alloc_count = g_allocation_count;
    }
}
