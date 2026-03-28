#ifndef CNLANG_RUNTIME_MEMORY_H
#define CNLANG_RUNTIME_MEMORY_H

#include <stddef.h>
#include <stdint.h>

/*
 * CN Language 运行时内存管理
 * 提供统一的内存分配接口，支持分配追踪和自定义后端
 *
 * Freestanding 模式支持：
 * - [FS] 基础内存分配接口必须可用，但实现可自定义
 * - [OPTIONAL] 统计功能可通过条件编译禁用
 */

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// 内存分配回调函数类型 [FS - 用于 OS 开发场景自定义分配器]
// =============================================================================
typedef void* (*CnRtAllocFunc)(size_t size, void* user_data);
typedef void  (*CnRtFreeFunc)(void* ptr, void* user_data);
typedef void* (*CnRtReallocFunc)(void* ptr, size_t new_size, void* user_data);

// =============================================================================
// 内存管理器配置 [FS - 必须支持]
// =============================================================================
typedef struct {
    CnRtAllocFunc alloc_func;
    CnRtFreeFunc free_func;
    CnRtReallocFunc realloc_func;
    void* user_data;
} CnRtMemoryConfig;

// =============================================================================
// 初始化内存管理系统 [FS - 必须支持]
// 可选自定义配置：freestanding 模式下必须提供自定义分配器
// =============================================================================
void cn_rt_memory_init(CnRtMemoryConfig* config);

// =============================================================================
// 基础分配接口 [FS - 必须支持]
// =============================================================================
void* cn_rt_malloc(size_t size);
void* cn_rt_calloc(size_t count, size_t size);
void* cn_rt_realloc(void* ptr, size_t new_size);
void  cn_rt_free(void* ptr);

// =============================================================================
// 辅助函数 [FS - 必须支持]
// =============================================================================
char* cn_rt_string_dup(const char* str);

// =============================================================================
// 内存统计接口 [OPTIONAL - 可禁用]
// =============================================================================
#ifndef CN_RT_NO_MEMORY_STATS
typedef struct {
    uint64_t current_allocated_bytes;
    uint64_t total_allocated_bytes;
    uint64_t allocation_count;
    uint64_t free_count;
} CnRtMemoryStats;

void cn_rt_memory_get_stats(CnRtMemoryStats* stats);
void cn_rt_memory_dump_stats(void);
#endif

// =============================================================================
// 内存访问安全检查接口 [FS - 必须支持]
// =============================================================================
// 启用/禁用内存安全检查
void cn_rt_memory_set_safety_check(int enabled);

// 检查内存地址是否对齐
int cn_rt_memory_check_alignment(uintptr_t addr, size_t alignment);

// 检查内存地址是否可读
int cn_rt_memory_check_readable(uintptr_t addr, size_t size);

// 检查内存地址是否可写
int cn_rt_memory_check_writable(uintptr_t addr, size_t size);

// 安全的内存读取
uintptr_t cn_rt_memory_read_safe(uintptr_t addr, size_t size);

// 安全的内存写入
void cn_rt_memory_write_safe(uintptr_t addr, uintptr_t value, size_t size);

// 安全的内存复制
void cn_rt_memory_copy_safe(void* dest, const void* src, size_t size);

// 安全的内存设置
void cn_rt_memory_set_safe(void* addr, int value, size_t size);

// 内存映射（带安全检查）
void* cn_rt_memory_map_safe(void* addr, size_t size, int prot, int flags);

// 解除内存映射（带安全检查）
int cn_rt_memory_unmap_safe(void* addr, size_t size);

// =============================================================================
// 中文函数名别名 (Chinese Function Name Aliases)
// 默认启用，可通过定义 CN_NO_CHINESE_NAMES 禁用
// =============================================================================
#ifndef CN_NO_CHINESE_NAMES

// 基础分配接口
#define 分配内存 cn_rt_malloc
#define 分配清零内存 cn_rt_calloc
#define 重新分配内存 cn_rt_realloc
#define 释放内存 cn_rt_free

// 辅助函数
#define 复制字符串副本 cn_rt_string_dup

// 内存安全检查接口
#define 设置内存安全检查 cn_rt_memory_set_safety_check
#define 检查内存对齐 cn_rt_memory_check_alignment
#define 检查内存可读 cn_rt_memory_check_readable
#define 检查内存可写 cn_rt_memory_check_writable
#define 安全读取内存 cn_rt_memory_read_safe
#define 安全写入内存 cn_rt_memory_write_safe
#define 安全复制内存 cn_rt_memory_copy_safe
#define 安全设置内存 cn_rt_memory_set_safe
#define 安全映射内存 cn_rt_memory_map_safe
#define 安全解除映射 cn_rt_memory_unmap_safe

#endif // CN_NO_CHINESE_NAMES

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_MEMORY_H */
