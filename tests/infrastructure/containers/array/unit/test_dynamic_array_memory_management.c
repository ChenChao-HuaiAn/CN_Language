/**
 * @file test_dynamic_array_memory_management.c
 * @brief 动态数组内存管理测试
 * 
 * 测试动态数组的内存管理功能（内存统计、压缩等）。
 * 遵循模块化测试设计，每个测试功能在单独的文件中。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_dynamic_array_common.h"
#include "../../../../../src/infrastructure/containers/array/CN_dynamic_array.h"
#include <stdio.h>
#include <stdbool.h>

/**
 * @brief 测试内存管理功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_memory_management(void)
{
    printf("测试: 内存管理\n");
    printf("--------------\n");
    
    TestResults results;
    init_test_results(&results);
    
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
    
    if (array == NULL) {
        printf("  ✗ 无法创建数组，跳過内存管理测试\n");
        return false;
    }
    
    // 添加测试数据
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        F_dynamic_array_push(array, &values[i]);
    }
    
    // 测试1: 获取内存统计
    printf("  1. 测试获取内存统计...\n");
    bool test_passed = true;
    size_t total_bytes = 0, used_bytes = 0;
    bool stats_result = F_dynamic_array_get_memory_stats(array, &total_bytes, &used_bytes);
    
    if (!stats_result) {
        printf("    ✗ 获取内存统计失败\n");
        test_passed = false;
    } else if (total_bytes == 0 || used_bytes == 0) {
        printf("    ✗ 内存统计值不正确\n");
        test_passed = false;
    } else {
        printf("    总内存: %zu 字节, 已使用: %zu 字节\n", total_bytes, used_bytes);
        printf("    ✓ 获取内存统计测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试2: 带错误码的内存统计
    printf("  2. 测试带错误码的内存统计...\n");
    test_passed = true;
    enum Eum_DynamicArrayError error_code;
    stats_result = F_dynamic_array_get_memory_stats_ex(array, &total_bytes, &used_bytes, &error_code);
    
    if (!stats_result || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的内存统计失败\n");
        test_passed = false;
    } else {
        printf("    ✓ 带错误码的内存统计测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试3: 压缩数组
    printf("  3. 测试压缩数组...\n");
    test_passed = true;
    size_t original_capacity = F_dynamic_array_capacity(array);
    
    // 先调整容量到更大值
    F_dynamic_array_resize(array, 100);
    
    // 然后压缩
    bool shrink_result = F_dynamic_array_shrink_to_fit(array);
    size_t new_capacity = F_dynamic_array_capacity(array);
    
    if (!shrink_result) {
        printf("    ✗ 压缩数组失败\n");
        test_passed = false;
    } else if (new_capacity > original_capacity) {
        printf("    ✗ 压缩后容量仍然太大\n");
        test_passed = false;
    } else {
        printf("    原始容量: %zu, 压缩后容量: %zu\n", original_capacity, new_capacity);
        printf("    ✓ 压缩数组测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试4: 带错误码的压缩
    printf("  4. 测试带错误码的压缩...\n");
    test_passed = true;
    shrink_result = F_dynamic_array_shrink_to_fit_ex(array, &error_code);
    
    if (!shrink_result || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的压缩失败\n");
        test_passed = false;
    } else {
        printf("    ✓ 带错误码的压缩测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试5: NULL数组内存操作
    printf("  5. 测试NULL数组内存操作...\n");
    test_passed = true;
    stats_result = F_dynamic_array_get_memory_stats_ex(NULL, &total_bytes, &used_bytes, &error_code);
    if (stats_result || error_code != Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER) {
        printf("    ✗ NULL数组内存统计失败\n");
        test_passed = false;
    }
    
    shrink_result = F_dynamic_array_shrink_to_fit_ex(NULL, &error_code);
    if (shrink_result || error_code != Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER) {
        printf("    ✗ NULL数组压缩失败\n");
        test_passed = false;
    }
    
    if (test_passed) {
        printf("    ✓ NULL数组内存操作测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    F_destroy_dynamic_array(array);
    print_test_results(&results, "内存管理");
    return results.failed_tests == 0;
}
