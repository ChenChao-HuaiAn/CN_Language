#include "cnlang/runtime/memory.h"
#include <stdlib.h>
#include <string.h>
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
