#include <stdio.h>
#include <stdlib.h>
#include "cnlang/runtime/runtime.h"

int main() {
    printf("测试 CN Language 运行时库...\n");
    
    // 测试基础打印函数
    printf("\n--- 基础打印函数测试 ---\n");
    cn_rt_print_int(42);
    cn_rt_print_newline();
    
    cn_rt_print_bool(1);
    cn_rt_print_newline();
    
    cn_rt_print_bool(0);
    cn_rt_print_newline();
    
    cn_rt_print_string("Hello from runtime!");
    cn_rt_print_newline();
    
    // 测试字符串操作
    printf("\n--- 字符串操作测试 ---\n");
    char *str1 = "Hello, ";
    char *str2 = "World!";
    char *concat_result = cn_rt_string_concat(str1, str2);
    if (concat_result) {
        printf("字符串连接结果: %s\n", concat_result);
        free(concat_result); // 记得释放内存
    }
    
    printf("字符串 '%s' 的长度: %zu\n", str1, cn_rt_string_length(str1));
    
    // 测试数组操作
    printf("\n--- 数组操作测试 ---\n");
    int *int_array = (int*)cn_rt_array_alloc(sizeof(int), 5); // 创建包含5个整数的数组
    if (int_array) {
        printf("分配的数组长度: %zu\n", cn_rt_array_length(int_array));
        
        // 测试边界检查
        printf("索引 3 是否有效: %s\n", cn_rt_array_bounds_check(int_array, 3) ? "是" : "否");
        printf("索引 10 是否有效: %s\n", cn_rt_array_bounds_check(int_array, 10) ? "是" : "否");
        
        // 为数组元素赋值
        int value = 100;
        if (cn_rt_array_set_element(int_array, 3, &value, sizeof(int))) {
            printf("成功设置索引 3 的值为 %d\n", value);
            
            // 获取数组元素
            int *retrieved_value = (int*)cn_rt_array_get_element(int_array, 3, sizeof(int));
            if (retrieved_value) {
                printf("从索引 3 获取的值: %d\n", *retrieved_value);
            }
        }
        
        cn_rt_array_free(int_array); // 释放数组内存
    }
    
    printf("\n运行时库测试完成!\n");
    return 0;
}