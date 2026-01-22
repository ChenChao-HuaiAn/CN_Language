#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cnlang/runtime/runtime.h"

/*
 * CN Language 运行时库实现
 * 提供 CN Language 程序运行所需的基础函数
 */

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
        char *result = malloc(1);
        if (result) result[0] = '\0';
        return result;
    }
    if (a == NULL) {
        return strdup(b);
    }
    if (b == NULL) {
        return strdup(a);
    }
    
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    char *result = malloc(len_a + len_b + 1);
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
    size_t *ptr = malloc(sizeof(size_t) + elem_size * count);
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