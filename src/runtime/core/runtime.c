#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cnlang/runtime/runtime.h"
#include "cnlang/runtime/memory.h"

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
    
    // 初始化内存子系统
    cn_rt_memory_init(NULL);
    
    // 初始化其他必要的子系统
}

// 运行时退出实现
void cn_rt_exit(void) {
    // 执行清理操作
    // 如果有未释放的内存，报告统计信息
    cn_rt_memory_dump_stats();
    
    // 如果有明确的退出码，可以通过 exit() 退出，
    // 但通常在 main 函数结束时由 C 运行时处理。
}

// 基础打印函数实现
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

// 字符串支持函数实现
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
    
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    char *result = cn_rt_malloc(len_a + len_b + 1);
    if (result == NULL) {
        return NULL;  // 内存分配失败
    }
    
    strcpy(result, a);
    strcat(result, b);
    return result;
}

size_t cn_rt_string_length(const char *str) {
    if (str == NULL) {
        return 0;
    }
    return strlen(str);
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