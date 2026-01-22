#include "cnlang/runtime/memory.h"
#include <stdio.h>
#include <string.h>

static int test_basic_alloc(void) {
    void* ptr = cn_rt_malloc(100);
    if (ptr == NULL) {
        fprintf(stderr, "test_basic_alloc: malloc failed\n");
        return 1;
    }
    memset(ptr, 0xAA, 100);
    cn_rt_free(ptr);
    return 0;
}

static int test_calloc(void) {
    int* ptr = (int*)cn_rt_calloc(10, sizeof(int));
    if (ptr == NULL) {
        fprintf(stderr, "test_calloc: calloc failed\n");
        return 1;
    }
    for (int i = 0; i < 10; i++) {
        if (ptr[i] != 0) {
            fprintf(stderr, "test_calloc: memory not zeroed at index %d\n", i);
            cn_rt_free(ptr);
            return 1;
        }
    }
    cn_rt_free(ptr);
    return 0;
}

static int test_realloc(void) {
    void* ptr = cn_rt_malloc(50);
    memset(ptr, 0xBB, 50);
    
    void* new_ptr = cn_rt_realloc(ptr, 100);
    if (new_ptr == NULL) {
        fprintf(stderr, "test_realloc: realloc failed\n");
        cn_rt_free(ptr);
        return 1;
    }
    
    // 验证旧数据保留（前50字节）
    unsigned char* p = (unsigned char*)new_ptr;
    for (int i = 0; i < 50; i++) {
        if (p[i] != 0xBB) {
            fprintf(stderr, "test_realloc: data mismatch at index %d\n", i);
            cn_rt_free(new_ptr);
            return 1;
        }
    }
    
    cn_rt_free(new_ptr);
    return 0;
}

static int test_stats(void) {
    CnRtMemoryStats stats;
    cn_rt_memory_get_stats(&stats);
    uint64_t initial_allocs = stats.allocation_count;
    
    void* p1 = cn_rt_malloc(10);
    void* p2 = cn_rt_malloc(20);
    cn_rt_free(p1);
    
    cn_rt_memory_get_stats(&stats);
    if (stats.allocation_count != initial_allocs + 2) {
        fprintf(stderr, "test_stats: allocation count mismatch. Expected %llu, got %llu\n", 
                (unsigned long long)(initial_allocs + 2), (unsigned long long)stats.allocation_count);
        return 1;
    }
    
    cn_rt_free(p2);
    return 0;
}

int main(void) {
    if (test_basic_alloc() != 0) return 1;
    if (test_calloc() != 0) return 1;
    if (test_realloc() != 0) return 1;
    if (test_stats() != 0) return 1;
    
    printf("runtime_memory_test: OK\n");
    return 0;
}
