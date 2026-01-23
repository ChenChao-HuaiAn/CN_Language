#include "cnlang/runtime/runtime.h"
#include <stdio.h>
#include <string.h>

static int test_array_alloc_length(void) {
    void* arr = cn_rt_array_alloc(sizeof(int), 5);
    if (arr == NULL) return 1;
    
    if (cn_rt_array_length(arr) != 5) {
        fprintf(stderr, "test_array_alloc_length: length mismatch\n");
        cn_rt_array_free(arr);
        return 1;
    }
    
    cn_rt_array_free(arr);
    return 0;
}

static int test_array_bounds(void) {
    void* arr = cn_rt_array_alloc(sizeof(int), 3);
    if (!cn_rt_array_bounds_check(arr, 0)) return 1;
    if (!cn_rt_array_bounds_check(arr, 2)) return 1;
    if (cn_rt_array_bounds_check(arr, 3)) return 1;
    
    cn_rt_array_free(arr);
    return 0;
}

static int test_array_elements(void) {
    void* arr = cn_rt_array_alloc(sizeof(int), 10);
    int val = 42;
    cn_rt_array_set_element(arr, 5, &val, sizeof(int));
    
    int* retrieved = (int*)cn_rt_array_get_element(arr, 5, sizeof(int));
    if (*retrieved != 42) {
        fprintf(stderr, "test_array_elements: data mismatch\n");
        cn_rt_array_free(arr);
        return 1;
    }
    
    cn_rt_array_free(arr);
    return 0;
}

// 测试空数组长度
static int test_empty_array_length(void) {
    void* arr = cn_rt_array_alloc(sizeof(int), 0);
    if (cn_rt_array_length(arr) != 0) {
        fprintf(stderr, "test_empty_array_length: expected 0\n");
        cn_rt_array_free(arr);
        return 1;
    }
    cn_rt_array_free(arr);
    return 0;
}

// 测试单元素数组长度
static int test_single_element_array_length(void) {
    void* arr = cn_rt_array_alloc(sizeof(int), 1);
    if (cn_rt_array_length(arr) != 1) {
        fprintf(stderr, "test_single_element_array_length: expected 1\n");
        cn_rt_array_free(arr);
        return 1;
    }
    cn_rt_array_free(arr);
    return 0;
}

// 测试大数组长度
static int test_large_array_length(void) {
    void* arr = cn_rt_array_alloc(sizeof(int), 10000);
    if (cn_rt_array_length(arr) != 10000) {
        fprintf(stderr, "test_large_array_length: expected 10000\n");
        cn_rt_array_free(arr);
        return 1;
    }
    cn_rt_array_free(arr);
    return 0;
}

int main(void) {
    if (test_array_alloc_length() != 0) return 1;
    if (test_array_bounds() != 0) return 1;
    if (test_array_elements() != 0) return 1;
    if (test_empty_array_length() != 0) return 1;          // 新增测试
    if (test_single_element_array_length() != 0) return 1; // 新增测试
    if (test_large_array_length() != 0) return 1;          // 新增测试
    
    printf("runtime_array_test: OK\n");
    return 0;
}
