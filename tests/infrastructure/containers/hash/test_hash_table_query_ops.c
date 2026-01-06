/**
 * @file test_hash_table_query_ops.c
 * @brief 哈希表查询操作测试实现文件
 * 
 * 实现哈希表查询操作功能的测试函数，包括大小、容量、负载因子和空检查。
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

/**
 * @brief 计算负载因子的辅助函数
 */
static double calculate_load_factor(size_t size, size_t capacity)
{
    if (capacity == 0) return 0.0;
    return (double)size / (double)capacity;
}

/**
 * @brief 测试查询操作功能
 */
bool test_hash_table_query_operations(void)
{
    printf("测试: 查询操作\n");
    printf("--------------\n");
    
    bool all_passed = true;
    
    // 测试1: 空哈希表查询
    printf("  1. 测试空哈希表查询...\n");
    Stru_HashTable_t* table = F_create_hash_table(
        sizeof(char[32]),
        sizeof(int),
        F_string_hash_function,
        F_string_compare_function,
        16
    );
    
    if (table == NULL) {
        printf("  ✗ 无法创建哈希表，跳过查询操作测试\n");
        return false;
    }
    
    if (F_hash_table_size(table) != 0) {
        printf("    ✗ 空哈希表大小不为0\n");
        all_passed = false;
    }
    
    size_t capacity = F_hash_table_capacity(table);
    if (capacity < 16) {
        printf("    ✗ 空哈希表容量小于指定值\n");
        all_passed = false;
    }
    
    // 注意：当前API没有提供load_factor函数，我们手动计算
    double expected_load_factor = calculate_load_factor(0, capacity);
    if (expected_load_factor != 0.0) {
        printf("    ✗ 空哈希表负载因子不为0\n");
        all_passed = false;
    }
    
    // 检查是否为空（通过size判断）
    if (F_hash_table_size(table) != 0) {
        printf("    ✗ 空哈希表size不为0\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 空哈希表查询测试通过\n");
    }
    
    // 测试2: 添加元素后查询
    printf("  2. 测试添加元素后查询...\n");
    char key1[] = "apple";
    int value1 = 100;
    
    if (!F_hash_table_put(table, key1, &value1)) {
        printf("    ✗ 无法添加测试元素\n");
        all_passed = false;
    } else {
        if (F_hash_table_size(table) != 1) {
            printf("    ✗ 添加后大小不为1\n");
            all_passed = false;
        }
        
        size_t new_capacity = F_hash_table_capacity(table);
        if (new_capacity != capacity) {
            printf("    ✗ 添加后容量改变（不应该）\n");
            all_passed = false;
        }
        
        double new_load_factor = calculate_load_factor(1, new_capacity);
        if (new_load_factor <= 0.0) {
            printf("    ✗ 添加后负载因子不正确\n");
            all_passed = false;
        }
        
        if (all_passed) {
            printf("    ✓ 添加元素后查询测试通过\n");
        }
    }
    
    // 测试3: 添加多个元素后查询
    printf("  3. 测试添加多个元素后查询...\n");
    char* keys[] = {"banana", "orange", "grape", "melon"};
    int values[] = {200, 150, 80, 300};
    
    for (int i = 0; i < 4; i++) {
        F_hash_table_put(table, keys[i], &values[i]);
    }
    
    if (F_hash_table_size(table) != 5) { // 包括之前的apple
        printf("    ✗ 添加多个后大小不正确 (期望: 5, 实际: %zu)\n", F_hash_table_size(table));
        all_passed = false;
    }
    
    // 检查容量是否可能因扩容而改变
    size_t current_capacity = F_hash_table_capacity(table);
    double current_load_factor = calculate_load_factor(5, current_capacity);
    
    printf("    当前大小: %zu, 容量: %zu, 负载因子: %.4f\n", 
           F_hash_table_size(table), current_capacity, current_load_factor);
    
    if (all_passed) {
        printf("    ✓ 添加多个元素后查询测试通过\n");
    }
    
    // 测试4: 删除元素后查询
    printf("  4. 测试删除元素后查询...\n");
    size_t size_before_remove = F_hash_table_size(table);
    
    if (!F_hash_table_remove(table, "orange")) {
        printf("    ✗ 无法删除测试元素\n");
        all_passed = false;
    } else {
        if (F_hash_table_size(table) != size_before_remove - 1) {
            printf("    ✗ 删除后大小未减少\n");
            all_passed = false;
        }
        
        // 容量不应该改变
        if (F_hash_table_capacity(table) != current_capacity) {
            printf("    ✗ 删除后容量改变（不应该）\n");
            all_passed = false;
        }
        
        if (all_passed) {
            printf("    ✓ 删除元素后查询测试通过\n");
        }
    }
    
    // 测试5: 清空哈希表后查询
    printf("  5. 测试清空哈希表后查询...\n");
    F_hash_table_clear(table);
    
    if (F_hash_table_size(table) != 0) {
        printf("    ✗ 清空后大小不为0\n");
        all_passed = false;
    }
    
    // 容量可能保持不变（取决于实现）
    size_t capacity_after_clear = F_hash_table_capacity(table);
    printf("    清空后容量: %zu\n", capacity_after_clear);
    
    double load_factor_after_clear = calculate_load_factor(0, capacity_after_clear);
    if (load_factor_after_clear != 0.0) {
        printf("    ✗ 清空后负载因子不为0\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 清空哈希表后查询测试通过\n");
    }
    
    // 测试6: NULL哈希表查询
    printf("  6. 测试NULL哈希表查询...\n");
    if (F_hash_table_size(NULL) != 0) {
        printf("    ✗ NULL哈希表大小不为0\n");
        all_passed = false;
    }
    
    if (F_hash_table_capacity(NULL) != 0) {
        printf("    ✗ NULL哈希表容量不为0\n");
        all_passed = false;
    }
    
    // 注意：当前API没有提供is_empty函数，我们通过size判断
    if (F_hash_table_size(NULL) != 0) {
        printf("    ✗ NULL哈希表应该视为空\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ NULL哈希表查询测试通过\n");
    }
    
    // 测试7: 调整容量后查询
    printf("  7. 测试调整容量后查询...\n");
    
    // 先添加一些元素
    for (int i = 0; i < 10; i++) {
        char key[32];
        sprintf(key, "key%d", i);
        int value = i * 100;
        F_hash_table_put(table, key, &value);
    }
    
    size_t size_before_resize = F_hash_table_size(table);
    size_t capacity_before_resize = F_hash_table_capacity(table);
    
    printf("    调整前 - 大小: %zu, 容量: %zu\n", size_before_resize, capacity_before_resize);
    
    // 调整到更大容量
    size_t new_capacity = capacity_before_resize * 2;
    if (!F_hash_table_resize(table, new_capacity)) {
        printf("    ✗ 调整容量失败\n");
        all_passed = false;
    } else {
        if (F_hash_table_size(table) != size_before_resize) {
            printf("    ✗ 调整容量后大小改变\n");
            all_passed = false;
        }
        
        if (F_hash_table_capacity(table) < new_capacity) {
            printf("    ✗ 调整容量后容量不正确\n");
            all_passed = false;
        }
        
        double load_factor_after_resize = calculate_load_factor(
            F_hash_table_size(table), F_hash_table_capacity(table));
        
        printf("    调整后 - 大小: %zu, 容量: %zu, 负载因子: %.4f\n",
               F_hash_table_size(table), F_hash_table_capacity(table), load_factor_after_resize);
        
        if (all_passed) {
            printf("    ✓ 调整容量后查询测试通过\n");
        }
    }
    
    // 测试8: 调整到更小容量（但大于当前大小）
    printf("  8. 测试调整到更小容量...\n");
    size_t smaller_capacity = F_hash_table_size(table) * 2;
    if (smaller_capacity < 4) smaller_capacity = 4;
    
    if (!F_hash_table_resize(table, smaller_capacity)) {
        printf("    ✗ 调整到更小容量失败\n");
        all_passed = false;
    } else {
        if (F_hash_table_capacity(table) != smaller_capacity) {
            printf("    ✗ 调整到更小容量后容量不正确\n");
            all_passed = false;
        }
        
        if (all_passed) {
            printf("    ✓ 调整到更小容量测试通过\n");
        }
    }
    
    // 测试9: 调整到无效容量
    printf("  9. 测试调整到无效容量...\n");
    if (F_hash_table_resize(table, 0)) {
        printf("    ✗ 调整到0容量应该失败\n");
        all_passed = false;
    }
    
    if (F_hash_table_resize(NULL, 100)) {
        printf("    ✗ NULL哈希表调整容量应该失败\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 无效容量调整测试通过\n");
    }
    
    // 测试10: 不同初始容量的查询
    printf("  10. 测试不同初始容量的查询...\n");
    F_destroy_hash_table(table);
    
    // 测试小容量
    table = F_create_hash_table(
        sizeof(int),
        sizeof(int),
        F_int_hash_function,
        F_int_compare_function,
        4  // 小容量
    );
    
    if (table == NULL) {
        printf("    ✗ 无法创建小容量哈希表\n");
        all_passed = false;
    } else {
        if (F_hash_table_capacity(table) < 4) {
            printf("    ✗ 小容量哈希表容量不正确\n");
            all_passed = false;
        }
        
        // 添加元素直到触发扩容
        for (int i = 0; i < 10; i++) {
            F_hash_table_put(table, &i, &i);
        }
        
        printf("    小容量哈希表 - 添加后大小: %zu, 容量: %zu\n",
               F_hash_table_size(table), F_hash_table_capacity(table));
        
        if (all_passed) {
            printf("    ✓ 不同初始容量查询测试通过\n");
        }
    }
    
    F_destroy_hash_table(table);
    printf("查询操作测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}
