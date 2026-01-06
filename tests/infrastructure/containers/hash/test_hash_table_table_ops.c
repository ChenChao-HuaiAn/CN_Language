/**
 * @file test_hash_table_table_ops.c
 * @brief 哈希表操作测试实现文件
 * 
 * 实现哈希表操作功能的测试函数，包括清空、调整容量和遍历。
 * 遵循模块化测试设计，每个测试功能在单独的文件中。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_hash_table.h"
#include "../../../../src/infrastructure/containers/hash/CN_hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// 遍历回调函数使用的数据结构
typedef struct {
    int count;
    int key_sum;
    int value_sum;
    char** keys;
    int* values;
    size_t max_items;
} ForeachTestData;

/**
 * @brief 遍历回调函数 - 统计信息
 */
static void foreach_count_callback(void* key, void* value, void* user_data)
{
    ForeachTestData* data = (ForeachTestData*)user_data;
    data->count++;
    
    // 如果是整数键值对，累加
    if (key != NULL && value != NULL) {
        // 这里假设是整数键值对，实际使用时需要根据类型处理
        // 为了测试简单，我们只统计数量
    }
}

/**
 * @brief 遍历回调函数 - 收集键值对
 */
static void foreach_collect_callback(void* key, void* value, void* user_data)
{
    ForeachTestData* data = (ForeachTestData*)user_data;
    
    if (data->count < data->max_items) {
        // 保存键（字符串）
        if (key != NULL) {
            data->keys[data->count] = strdup((char*)key);
        }
        
        // 保存值（整数）
        if (value != NULL) {
            data->values[data->count] = *(int*)value;
        }
        
        data->count++;
    }
}

/**
 * @brief 遍历回调函数 - 提前停止
 */
static void foreach_stop_early_callback(void* key, void* value, void* user_data)
{
    int* stop_after = (int*)user_data;
    
    if (*stop_after > 0) {
        (*stop_after)--;
    } else {
        // 设置一个标志表示应该停止，但当前API不支持停止遍历
        // 这里我们只是记录，实际不会停止
    }
}

/**
 * @brief 测试哈希表操作功能
 */
bool test_hash_table_table_operations(void)
{
    printf("测试: 哈希表操作\n");
    printf("----------------\n");
    
    bool all_passed = true;
    
    // 创建测试用的哈希表
    Stru_HashTable_t* table = F_create_hash_table(
        sizeof(char[32]),
        sizeof(int),
        F_string_hash_function,
        F_string_compare_function,
        8
    );
    
    if (table == NULL) {
        printf("  ✗ 无法创建哈希表，跳过哈希表操作测试\n");
        return false;
    }
    
    // 测试1: 清空空哈希表
    printf("  1. 测试清空空哈希表...\n");
    F_hash_table_clear(table);
    
    if (F_hash_table_size(table) != 0) {
        printf("    ✗ 清空空哈希表后大小不为0\n");
        all_passed = false;
    } else {
        printf("    ✓ 清空空哈希表测试通过\n");
    }
    
    // 测试2: 添加元素后清空
    printf("  2. 测试添加元素后清空...\n");
    char* keys[] = {"apple", "banana", "orange", "grape", "melon"};
    int values[] = {100, 200, 150, 80, 300};
    
    for (int i = 0; i < 5; i++) {
        F_hash_table_put(table, keys[i], &values[i]);
    }
    
    size_t size_before_clear = F_hash_table_size(table);
    size_t capacity_before_clear = F_hash_table_capacity(table);
    
    F_hash_table_clear(table);
    
    if (F_hash_table_size(table) != 0) {
        printf("    ✗ 清空后大小不为0\n");
        all_passed = false;
    }
    
    // 检查所有键都不存在
    for (int i = 0; i < 5; i++) {
        if (F_hash_table_contains(table, keys[i])) {
            printf("    ✗ 清空后键仍然存在: %s\n", keys[i]);
            all_passed = false;
        }
    }
    
    // 容量可能保持不变
    size_t capacity_after_clear = F_hash_table_capacity(table);
    printf("    清空前容量: %zu, 清空后容量: %zu\n", capacity_before_clear, capacity_after_clear);
    
    if (all_passed) {
        printf("    ✓ 添加元素后清空测试通过\n");
    }
    
    // 测试3: 清空NULL哈希表
    printf("  3. 测试清空NULL哈希表...\n");
    F_hash_table_clear(NULL); // 不应该崩溃
    printf("    ✓ 清空NULL哈希表测试通过\n");
    
    // 重新添加元素用于后续测试
    for (int i = 0; i < 5; i++) {
        F_hash_table_put(table, keys[i], &values[i]);
    }
    
    // 测试4: 调整容量（扩大）
    printf("  4. 测试调整容量（扩大）...\n");
    size_t original_capacity = F_hash_table_capacity(table);
    size_t new_capacity = original_capacity * 2;
    
    if (!F_hash_table_resize(table, new_capacity)) {
        printf("    ✗ 扩大容量失败\n");
        all_passed = false;
    } else {
        if (F_hash_table_capacity(table) < new_capacity) {
            printf("    ✗ 扩大容量后容量不正确\n");
            all_passed = false;
        }
        
        if (F_hash_table_size(table) != 5) {
            printf("    ✗ 扩大容量后大小改变\n");
            all_passed = false;
        }
        
        // 验证所有数据仍然存在
        for (int i = 0; i < 5; i++) {
            int* val = (int*)F_hash_table_get(table, keys[i]);
            if (val == NULL || *val != values[i]) {
                printf("    ✗ 扩大容量后数据丢失: %s\n", keys[i]);
                all_passed = false;
                break;
            }
        }
        
        if (all_passed) {
            printf("    ✓ 扩大容量测试通过\n");
        }
    }
    
    // 测试5: 调整容量（缩小）
    printf("  5. 测试调整容量（缩小）...\n");
    size_t smaller_capacity = F_hash_table_size(table) * 2; // 确保足够容纳所有元素
    if (smaller_capacity < 4) smaller_capacity = 4;
    
    if (!F_hash_table_resize(table, smaller_capacity)) {
        printf("    ✗ 缩小容量失败\n");
        all_passed = false;
    } else {
        if (F_hash_table_capacity(table) != smaller_capacity) {
            printf("    ✗ 缩小容量后容量不正确\n");
            all_passed = false;
        }
        
        // 验证所有数据仍然存在
        for (int i = 0; i < 5; i++) {
            if (!F_hash_table_contains(table, keys[i])) {
                printf("    ✗ 缩小容量后数据丢失: %s\n", keys[i]);
                all_passed = false;
                break;
            }
        }
        
        if (all_passed) {
            printf("    ✓ 缩小容量测试通过\n");
        }
    }
    
    // 测试6: 调整到无效容量
    printf("  6. 测试调整到无效容量...\n");
    if (F_hash_table_resize(table, 0)) {
        printf("    ✗ 调整到0容量应该失败\n");
        all_passed = false;
    }
    
    if (F_hash_table_resize(table, 1)) { // 1小于当前元素数量
        printf("    ✗ 调整到小于元素数量的容量应该失败\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 无效容量调整测试通过\n");
    }
    
    // 测试7: 调整NULL哈希表容量
    printf("  7. 测试调整NULL哈希表容量...\n");
    if (F_hash_table_resize(NULL, 100)) {
        printf("    ✗ 调整NULL哈希表容量应该失败\n");
        all_passed = false;
    } else {
        printf("    ✓ 调整NULL哈希表容量测试通过\n");
    }
    
    // 测试8: 遍历哈希表
    printf("  8. 测试遍历哈希表...\n");
    
    // 先清空并重新添加元素
    F_hash_table_clear(table);
    for (int i = 0; i < 5; i++) {
        F_hash_table_put(table, keys[i], &values[i]);
    }
    
    // 使用回调函数统计元素数量
    ForeachTestData count_data = {0};
    F_hash_table_foreach(table, foreach_count_callback, &count_data);
    
    if (count_data.count != 5) {
        printf("    ✗ 遍历统计数量不正确 (期望: 5, 实际: %d)\n", count_data.count);
        all_passed = false;
    } else {
        printf("    ✓ 遍历统计测试通过 (数量: %d)\n", count_data.count);
    }
    
    // 测试9: 遍历并收集数据
    printf("  9. 测试遍历并收集数据...\n");
    
    ForeachTestData collect_data = {0};
    collect_data.max_items = 10;
    collect_data.keys = (char**)malloc(sizeof(char*) * collect_data.max_items);
    collect_data.values = (int*)malloc(sizeof(int) * collect_data.max_items);
    
    if (collect_data.keys == NULL || collect_data.values == NULL) {
        printf("    ✗ 内存分配失败\n");
        all_passed = false;
    } else {
        F_hash_table_foreach(table, foreach_collect_callback, &collect_data);
        
        if (collect_data.count != 5) {
            printf("    ✗ 收集数据数量不正确\n");
            all_passed = false;
        } else {
            // 验证收集到的数据
            printf("    收集到的数据:\n");
            for (int i = 0; i < collect_data.count; i++) {
                printf("      %s -> %d\n", collect_data.keys[i], collect_data.values[i]);
            }
            
            // 清理
            for (int i = 0; i < collect_data.count; i++) {
                free(collect_data.keys[i]);
            }
            
            printf("    ✓ 遍历收集数据测试通过\n");
        }
        
        free(collect_data.keys);
        free(collect_data.values);
    }
    
    // 测试10: 遍历NULL哈希表
    printf("  10. 测试遍历NULL哈希表...\n");
    F_hash_table_foreach(NULL, foreach_count_callback, NULL); // 不应该崩溃
    printf("    ✓ 遍历NULL哈希表测试通过\n");
    
    // 测试11: 使用NULL回调函数遍历
    printf("  11. 测试使用NULL回调函数遍历...\n");
    F_hash_table_foreach(table, NULL, NULL); // 不应该崩溃
    printf("    ✓ 使用NULL回调函数遍历测试通过\n");
    
    // 测试12: 自动扩容测试
    printf("  12. 测试自动扩容...\n");
    F_hash_table_clear(table);
    
    size_t initial_capacity = F_hash_table_capacity(table);
    printf("    初始容量: %zu\n", initial_capacity);
    
    // 添加足够多的元素以触发自动扩容
    int item_count = 50;
    for (int i = 0; i < item_count; i++) {
        char key[32];
        sprintf(key, "item%d", i);
        int value = i * 10;
        F_hash_table_put(table, key, &value);
    }
    
    size_t final_capacity = F_hash_table_capacity(table);
    size_t final_size = F_hash_table_size(table);
    
    printf("    最终大小: %zu, 最终容量: %zu\n", final_size, final_capacity);
    
    if (final_size != item_count) {
        printf("    ✗ 自动扩容后大小不正确\n");
        all_passed = false;
    }
    
    if (final_capacity <= initial_capacity) {
        printf("    ✗ 自动扩容后容量未增加\n");
        all_passed = false;
    }
    
    // 验证所有数据仍然存在
    bool all_data_valid = true;
    for (int i = 0; i < item_count; i++) {
        char key[32];
        sprintf(key, "item%d", i);
        int* val = (int*)F_hash_table_get(table, key);
        if (val == NULL || *val != i * 10) {
            printf("    ✗ 自动扩容后数据丢失: %s\n", key);
            all_data_valid = false;
            break;
        }
    }
    
    if (all_data_valid && all_passed) {
        printf("    ✓ 自动扩容测试通过\n");
    }
    
    // 测试13: 重复调整容量
    printf("  13. 测试重复调整容量...\n");
    for (int i = 0; i < 10; i++) {
        size_t test_capacity = 4 * (i + 1);
        if (!F_hash_table_resize(table, test_capacity)) {
            printf("    ✗ 重复调整容量失败: 第%d次\n", i);
            all_passed = false;
            break;
        }
        
        if (F_hash_table_capacity(table) != test_capacity) {
            printf("    ✗ 重复调整容量后容量不正确\n");
            all_passed = false;
            break;
        }
    }
    
    if (all_passed) {
        printf("    ✓ 重复调整容量测试通过\n");
    }
    
    F_destroy_hash_table(table);
    printf("哈希表操作测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}
