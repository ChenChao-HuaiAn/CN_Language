#include <stdio.h>
#include <stdlib.h>
#ifndef CN_FREESTANDING
#include <string.h>
#endif
#include "cnlang/runtime/runtime.h"
#include "cnlang/runtime/memory.h"
#include "cnlang/runtime/freestanding.h"

/*
 * CN Language 运行时库实现
 * 提供 CN Language 程序运行所需的基础函数
 */

// 全局运行时状态
static CnRuntimeState g_rt_state;

// 运行时初始化实现
void cn_rt_init(void) {
    g_rt_state.exit_code = 0;
    g_rt_state.total_allocations = 0;
    g_rt_state.total_freed = 0;
    
#ifdef CN_FREESTANDING
    // Freestanding 模式：使用静态分配器
    cn_rt_freestanding_init_allocator();
#else
    // 初始化内存子系统（宿主模式使用标准 malloc/free）
    cn_rt_memory_init(NULL);
#endif
    
    // 初始化其他必要的子系统
}

// 运行时退出实现
void cn_rt_exit(void) {
    // 执行清理操作
    // 如果有未释放的内存，报告统计信息
#ifndef CN_RT_NO_MEMORY_STATS
    cn_rt_memory_dump_stats();
#endif
    
    // 如果有明确的退出码，可以通过 exit() 退出，
    // 但通常在 main 函数结束时由 C 运行时处理。
}

// =============================================================================
// 基础打印函数实现（仅在宿主模式或未禁用时可用）
// =============================================================================
#ifndef CN_RT_NO_PRINT
void cn_rt_print_int(long long val) {
    printf("%lld", val);
}

void cn_rt_print_bool(int val) {
    printf(val ? "真" : "假");
}

void cn_rt_print_string(const char *str) {
    if (str != NULL) {
        printf("%s", str);
    }
}

void cn_rt_print_newline() {
    printf("\n");
}
#endif /* CN_RT_NO_PRINT */

// =============================================================================
// 字符串支持函数实现 [FS - 核心子集]
// =============================================================================
char* cn_rt_string_concat(const char *a, const char *b) {
    if (a == NULL && b == NULL) {
        char *result = cn_rt_malloc(1);
        if (result) result[0] = '\0';
        return result;
    }
    if (a == NULL) {
        return cn_rt_string_dup(b);
    }
    if (b == NULL) {
        return cn_rt_string_dup(a);
    }
    
    size_t len_a, len_b;
#ifdef CN_FREESTANDING
    len_a = cn_rt_strlen(a);
    len_b = cn_rt_strlen(b);
#else
    len_a = strlen(a);
    len_b = strlen(b);
#endif
    char *result = cn_rt_malloc(len_a + len_b + 1);
    if (result == NULL) {
        return NULL;  // 内存分配失败
    }
    
#ifdef CN_FREESTANDING
    cn_rt_strcpy(result, a);
    cn_rt_strcat(result, b);
#else
    strcpy(result, a);
    strcat(result, b);
#endif
    return result;
}

size_t cn_rt_string_length(const char *str) {
    if (str == NULL) {
        return 0;
    }
#ifdef CN_FREESTANDING
    return cn_rt_strlen(str);
#else
    return strlen(str);
#endif
}

// 数组支持函数实现
void* cn_rt_array_alloc(size_t elem_size, size_t count) {
    if (elem_size == 0 || count == 0) {
        return NULL;
    }
    
    // 为了存储数组长度信息，在实际数据前面预留一个 size_t 大小的空间
    size_t *ptr = cn_rt_malloc(sizeof(size_t) + elem_size * count);
    if (ptr == NULL) {
        return NULL;  // 内存分配失败
    }
    
    // 将数组长度存储在第一个 size_t 位置
    *ptr = count;
    
    // 返回指向实际数据区域的指针（跳过长度存储位置）
    return (void*)(ptr + 1);
}

size_t cn_rt_array_length(void *arr) {
    if (arr == NULL) {
        return 0;
    }
    
    // 从数组指针前一个 size_t 位置获取数组长度
    size_t *length_ptr = ((size_t*)arr) - 1;
    return *length_ptr;
}

int cn_rt_array_bounds_check(void *arr, size_t index) {
    if (arr == NULL) {
        return 0;  // 空指针访问，越界
    }
    
    size_t length = cn_rt_array_length(arr);
    return index < length;  // 返回 1 表示未越界，0 表示越界
}