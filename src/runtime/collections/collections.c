#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cnlang/runtime/runtime.h"

/*
 * CN Language 运行时库 - 集合操作实现
 * 提供字符串和数组的高级操作函数
 */

// 字符串高级操作函数（如果需要）
// 这里可以添加更多字符串处理函数，例如：
char* cn_rt_string_substring(const char *str, size_t start, size_t length) {
    if (str == NULL) {
        return NULL;
    }
    
    size_t str_len = strlen(str);
    if (start >= str_len) {
        char *result = malloc(1);
        if (result) result[0] = '\0';
        return result;
    }
    
    if (start + length > str_len) {
        length = str_len - start;
    }
    
    char *result = malloc(length + 1);
    if (result == NULL) {
        return NULL;  // 内存分配失败
    }
    
    strncpy(result, str + start, length);
    result[length] = '\0';
    return result;
}

int cn_rt_string_compare(const char *a, const char *b) {
    if (a == NULL && b == NULL) {
        return 0;
    }
    if (a == NULL) {
        return -1;
    }
    if (b == NULL) {
        return 1;
    }
    return strcmp(a, b);
}

// 数组高级操作函数（如果需要）
// 这里可以添加更多数组处理函数，例如：
void cn_rt_array_free(void *arr) {
    if (arr != NULL) {
        // 由于数组内存是在长度信息之后分配的，我们需要获取完整的内存块指针
        size_t *full_ptr = ((size_t*)arr) - 1;
        free(full_ptr);
    }
}

// 为数组元素赋值的辅助函数
int cn_rt_array_set_element(void *arr, size_t index, const void *element, size_t elem_size) {
    if (!cn_rt_array_bounds_check(arr, index)) {
        return 0;  // 索引越界
    }
    
    char *data = (char*)arr;
    memcpy(data + index * elem_size, element, elem_size);
    return 1;  // 成功设置
}

// 获取数组元素的辅助函数
void* cn_rt_array_get_element(void *arr, size_t index, size_t elem_size) {
    if (!cn_rt_array_bounds_check(arr, index)) {
        return NULL;  // 索引越界
    }
    
    char *data = (char*)arr;
    return (void*)(data + index * elem_size);
}