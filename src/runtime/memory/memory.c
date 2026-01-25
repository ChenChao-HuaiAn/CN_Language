#include "cnlang/runtime/memory.h"
#include "cnlang/runtime/freestanding.h"
#include <stdlib.h>
#ifndef CN_FREESTANDING
#include <string.h>
#include <stdint.h>
#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/mman.h>
#endif
#endif
#include <stdio.h>

/*
 * CN Language 运行时内存管理实现
 */

static CnRtMemoryConfig g_mem_config = {
    .alloc_func = NULL,
    .free_func = NULL,
    .realloc_func = NULL,
    .user_data = NULL
};

#ifndef CN_RT_NO_MEMORY_STATS
static CnRtMemoryStats g_mem_stats = {0};
#endif

void cn_rt_memory_init(CnRtMemoryConfig* config) {
    if (config) {
        g_mem_config = *config;
    }
}

void* cn_rt_malloc(size_t size) {
    if (size == 0) return NULL;
    
    void* ptr = NULL;
    if (g_mem_config.alloc_func) {
        ptr = g_mem_config.alloc_func(size, g_mem_config.user_data);
    } else {
        ptr = malloc(size);
    }
    
#ifndef CN_RT_NO_MEMORY_STATS
    if (ptr) {
        g_mem_stats.total_allocated_bytes += size;
        g_mem_stats.current_allocated_bytes += size;
        g_mem_stats.allocation_count++;
    }
#endif
    
    return ptr;
}

void* cn_rt_calloc(size_t count, size_t size) {
    size_t total = count * size;
    void* ptr = cn_rt_malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void* cn_rt_realloc(void* ptr, size_t new_size) {
    if (ptr == NULL) return cn_rt_malloc(new_size);
    if (new_size == 0) {
        cn_rt_free(ptr);
        return NULL;
    }
    
    // 注意：realloc 的统计比较复杂，因为不知道旧块大小
    // 在基础实现中，我们仅记录新的分配行为
    void* new_ptr = NULL;
    if (g_mem_config.realloc_func) {
        new_ptr = g_mem_config.realloc_func(ptr, new_size, g_mem_config.user_data);
    } else {
        new_ptr = realloc(ptr, new_size);
    }
    
#ifndef CN_RT_NO_MEMORY_STATS
    if (new_ptr) {
        g_mem_stats.allocation_count++;
        // 无法准确更新 current_allocated_bytes，除非我们在每个块前面存大小
    }
#endif
    
    return new_ptr;
}

void cn_rt_free(void* ptr) {
    if (ptr == NULL) return;
    
    if (g_mem_config.free_func) {
        g_mem_config.free_func(ptr, g_mem_config.user_data);
    } else {
        free(ptr);
    }
    
#ifndef CN_RT_NO_MEMORY_STATS
    g_mem_stats.free_count++;
    // 同样，无法准确扣除 current_allocated_bytes，除非有元数据
#endif
}

char* cn_rt_string_dup(const char* str) {
    if (str == NULL) return NULL;
    size_t len = strlen(str);
    char* new_str = (char*)cn_rt_malloc(len + 1);
    if (new_str) {
        memcpy(new_str, str, len + 1);
    }
    return new_str;
}

#ifndef CN_RT_NO_MEMORY_STATS
void cn_rt_memory_get_stats(CnRtMemoryStats* stats) {
    if (stats) {
        *stats = g_mem_stats;
    }
}

void cn_rt_memory_dump_stats(void) {
    printf("--- CN Runtime Memory Stats ---\n");
    printf("Total Allocated: %llu bytes\n", (unsigned long long)g_mem_stats.total_allocated_bytes);
    printf("Allocations: %llu\n", (unsigned long long)g_mem_stats.allocation_count);
    printf("Frees: %llu\n", (unsigned long long)g_mem_stats.free_count);
    printf("-------------------------------\n");
}
#endif /* CN_RT_NO_MEMORY_STATS */

/*
 * 内存访问安全检查实现
 */

// 全局安全检查配置
static int g_memory_safety_enabled = 1;

// 启用/禁用内存安全检查
void cn_rt_memory_set_safety_check(int enabled) {
    g_memory_safety_enabled = enabled;
}

// 检查内存地址是否对齐
int cn_rt_memory_check_alignment(uintptr_t addr, size_t alignment) {
    if (!g_memory_safety_enabled) {
        return 1; // 安全检查禁用时，总是返回成功
    }
    
    if (alignment == 0) {
        alignment = sizeof(void*); // 默认按指针大小对齐
    }
    
    if (addr % alignment != 0) {
        fprintf(stderr, "内存对齐错误：地址 0x%zx 未按 %zu 字节对齐\n", addr, alignment);
        return 0;
    }
    
    return 1;
}

// 检查内存地址是否可读
int cn_rt_memory_check_readable(uintptr_t addr, size_t size) {
    if (!g_memory_safety_enabled) {
        return 1;
    }
    
    if (addr == 0) {
        fprintf(stderr, "内存访问错误：空指针访问\n");
        return 0;
    }
    
    // 检查是否在合理的地址范围内
    // 在实际系统中，这里可以使用更复杂的检查，如 /proc/self/maps
    // 这里只做基本的范围检查
    
    return 1;
}

// 检查内存地址是否可写
int cn_rt_memory_check_writable(uintptr_t addr, size_t size) {
    if (!g_memory_safety_enabled) {
        return 1;
    }
    
    if (addr == 0) {
        fprintf(stderr, "内存访问错误：空指针写入\n");
        return 0;
    }
    
    // 检查是否在只读内存区域
    // 在实际系统中，这里可以使用更复杂的检查
    
    return 1;
}

// 安全的内存读取
uintptr_t cn_rt_memory_read_safe(uintptr_t addr, size_t size) {
    if (!cn_rt_memory_check_readable(addr, size)) {
        return 0;
    }
    
    if (!cn_rt_memory_check_alignment(addr, size)) {
        return 0;
    }
    
    switch (size) {
        case 1:
            return *((volatile uint8_t*)addr);
        case 2:
            return *((volatile uint16_t*)addr);
        case 4:
            return *((volatile uint32_t*)addr);
        case 8:
            return *((volatile uint64_t*)addr);
        default:
            fprintf(stderr, "内存读取错误：不支持的大小 %zu\n", size);
            return 0;
    }
}

// 安全的内存写入
void cn_rt_memory_write_safe(uintptr_t addr, uintptr_t value, size_t size) {
    if (!cn_rt_memory_check_writable(addr, size)) {
        return;
    }
    
    if (!cn_rt_memory_check_alignment(addr, size)) {
        return;
    }
    
    switch (size) {
        case 1:
            *((volatile uint8_t*)addr) = (uint8_t)value;
            break;
        case 2:
            *((volatile uint16_t*)addr) = (uint16_t)value;
            break;
        case 4:
            *((volatile uint32_t*)addr) = (uint32_t)value;
            break;
        case 8:
            *((volatile uint64_t*)addr) = (uint64_t)value;
            break;
        default:
            fprintf(stderr, "内存写入错误：不支持的大小 %zu\n", size);
            break;
    }
}

// 安全的内存复制
void cn_rt_memory_copy_safe(void* dest, const void* src, size_t size) {
    if (!g_memory_safety_enabled) {
        memcpy(dest, src, size);
        return;
    }
    
    if (dest == NULL || src == NULL) {
        fprintf(stderr, "内存复制错误：空指针\n");
        return;
    }
    
    // 检查目标地址是否可写
    if (!cn_rt_memory_check_writable((uintptr_t)dest, size)) {
        return;
    }
    
    // 检查源地址是否可读
    if (!cn_rt_memory_check_readable((uintptr_t)src, size)) {
        return;
    }
    
    memcpy(dest, src, size);
}

// 安全的内存设置
void cn_rt_memory_set_safe(void* addr, int value, size_t size) {
    if (!g_memory_safety_enabled) {
        memset(addr, value, size);
        return;
    }
    
    if (addr == NULL) {
        fprintf(stderr, "内存设置错误：空指针\n");
        return;
    }
    
    // 检查地址是否可写
    if (!cn_rt_memory_check_writable((uintptr_t)addr, size)) {
        return;
    }
    
    memset(addr, value, size);
}

// 内存映射（带安全检查）
void* cn_rt_memory_map_safe(void* addr, size_t size, int prot, int flags) {
    if (!g_memory_safety_enabled) {
#if !defined(CN_FREESTANDING) && !defined(_WIN32) && !defined(_WIN64)
        return mmap(addr, size, prot, flags, -1, 0);
#else
        (void)addr; (void)size; (void)prot; (void)flags; // 避免未使用参数警告
        return NULL;
#endif
    }
    
    if (size == 0) {
        fprintf(stderr, "内存映射错误：大小为0\n");
        return NULL;
    }
    
#if !defined(CN_FREESTANDING) && !defined(_WIN32) && !defined(_WIN64)
    void* result = mmap(addr, size, prot, flags, -1, 0);
    
    if (result == MAP_FAILED) {
        fprintf(stderr, "内存映射失败：地址=%p, 大小=%zu\n", addr, size);
        return NULL;
    }
    
    return result;
#else
    fprintf(stderr, "内存映射错误：当前平台不支持 mmap\n");
    (void)addr; (void)prot; (void)flags; // 避免未使用参数警告
    return NULL;
#endif
}

// 解除内存映射（带安全检查）
int cn_rt_memory_unmap_safe(void* addr, size_t size) {
    if (!g_memory_safety_enabled) {
#if !defined(CN_FREESTANDING) && !defined(_WIN32) && !defined(_WIN64)
        return munmap(addr, size);
#else
        (void)addr; (void)size; // 避免未使用参数警告
        return -1;
#endif
    }
    
    if (addr == NULL) {
        fprintf(stderr, "解除内存映射错误：空指针\n");
        return -1;
    }
    
    if (size == 0) {
        fprintf(stderr, "解除内存映射错误：大小为0\n");
        return -1;
    }
    
#if !defined(CN_FREESTANDING) && !defined(_WIN32) && !defined(_WIN64)
    int result = munmap(addr, size);
    
    if (result != 0) {
        fprintf(stderr, "解除内存映射失败：地址=%p, 大小=%zu\n", addr, size);
        return -1;
    }
    
    return 0;
#else
    fprintf(stderr, "解除内存映射错误：当前平台不支持 munmap\n");
    (void)addr; (void)size; // 避免未使用参数警告
    return -1;
#endif
}
