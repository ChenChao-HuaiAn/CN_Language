/**
 * @file test_hash_table_basic_ops.c
 * @brief 哈希表基本操作测试实现文件
 * 
 * 实现哈希表基本操作功能的测试函数，包括插入、获取、删除和检查存在性。
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
 * @brief 测试基本操作功能
 */
bool test_hash_table_basic_operations(void)
{
    printf("测试: 基本操作\n");
    printf("--------------\n");
    
    bool all_passed = true;
    
    // 创建测试用的哈希表（字符串键，整数值）
    Stru_HashTable_t* table = F_create_hash_table(
        sizeof(char[32]),
        sizeof(int),
        F_string_hash_function,
        F_string_compare_function,
        16
    );
    
    if (table == NULL) {
        printf("  ✗ 无法创建哈希表，跳过基本操作测试\n");
        return false;
    }
    
    // 测试1: 插入键值对
    printf("  1. 测试插入键值对...\n");
    char key1[] = "apple";
    int value1 = 100;
    
    if (!F_hash_table_put(table, key1, &value1)) {
        printf("    ✗ 插入键值对失败: %s -> %d\n", key1, value1);
        all_passed = false;
    } else {
        if (F_hash_table_size(table) != 1) {
            printf("    ✗ 插入后大小不正确\n");
            all_passed = false;
        }
    }
    
    // 测试2: 获取值
    printf("  2. 测试获取值...\n");
    int* retrieved = (int*)F_hash_table_get(table, "apple");
    if (retrieved == NULL) {
        printf("    ✗ 获取值失败: 返回NULL\n");
        all_passed = false;
    } else if (*retrieved != value1) {
        printf("    ✗ 获取值失败: 值不正确 (期望: %d, 实际: %d)\n", value1, *retrieved);
        all_passed = false;
    }
    
    // 测试3: 检查键是否存在
    printf("  3. 测试检查键是否存在...\n");
    if (!F_hash_table_contains(table, "apple")) {
        printf("    ✗ 检查键存在失败: apple应该存在\n");
        all_passed = false;
    }
    
    if (F_hash_table_contains(table, "banana")) {
        printf("    ✗ 检查键存在失败: banana不应该存在\n");
        all_passed = false;
    }
    
    // 测试4: 插入更多键值对
    printf("  4. 测试插入更多键值对...\n");
    char* keys[] = {"banana", "orange", "grape", "melon", "peach"};
    int values[] = {200, 150, 80, 300, 120};
    
    for (int i = 0; i < 5; i++) {
        if (!F_hash_table_put(table, keys[i], &values[i])) {
            printf("    ✗ 插入键值对失败: %s -> %d\n", keys[i], values[i]);
            all_passed = false;
            break;
        }
    }
    
    if (all_passed && F_hash_table_size(table) != 6) {
        printf("    ✗ 插入多个后大小不正确 (期望: 6, 实际: %zu)\n", F_hash_table_size(table));
        all_passed = false;
    }
    
    // 测试5: 获取所有插入的值
    printf("  5. 测试获取所有插入的值...\n");
    for (int i = 0; i < 5; i++) {
        int* val = (int*)F_hash_table_get(table, keys[i]);
        if (val == NULL || *val != values[i]) {
            printf("    ✗ 获取值失败: %s (期望: %d)\n", keys[i], values[i]);
            all_passed = false;
            break;
        }
    }
    
    // 测试6: 更新现有键的值
    printf("  6. 测试更新现有键的值...\n");
    int new_value = 999;
    if (!F_hash_table_put(table, "apple", &new_value)) {
        printf("    ✗ 更新值失败\n");
        all_passed = false;
    } else {
        int* updated = (int*)F_hash_table_get(table, "apple");
        if (updated == NULL || *updated != new_value) {
            printf("    ✗ 更新后值不正确 (期望: %d)\n", new_value);
            all_passed = false;
        }
        
        // 大小不应该改变
        if (F_hash_table_size(table) != 6) {
            printf("    ✗ 更新后大小改变\n");
            all_passed = false;
        }
    }
    
    // 测试7: 删除键值对
    printf("  7. 测试删除键值对...\n");
    size_t original_size = F_hash_table_size(table);
    
    if (!F_hash_table_remove(table, "orange")) {
        printf("    ✗ 删除键值对失败: orange\n");
        all_passed = false;
    } else {
        if (F_hash_table_size(table) != original_size - 1) {
            printf("    ✗ 删除后大小未减少\n");
            all_passed = false;
        }
        
        if (F_hash_table_contains(table, "orange")) {
            printf("    ✗ 删除后键仍然存在\n");
            all_passed = false;
        }
        
        if (F_hash_table_get(table, "orange") != NULL) {
            printf("    ✗ 删除后仍然能获取到值\n");
            all_passed = false;
        }
    }
    
    // 测试8: 删除不存在的键
    printf("  8. 测试删除不存在的键...\n");
    if (F_hash_table_remove(table, "nonexistent")) {
        printf("    ✗ 删除不存在的键应该返回false\n");
        all_passed = false;
    }
    
    // 测试9: 获取不存在的键
    printf("  9. 测试获取不存在的键...\n");
    void* not_found = F_hash_table_get(table, "nonexistent");
    if (not_found != NULL) {
        printf("    ✗ 获取不存在的键应该返回NULL\n");
        all_passed = false;
    }
    
    // 测试10: 检查不存在的键
    printf("  10. 测试检查不存在的键...\n");
    if (F_hash_table_contains(table, "nonexistent")) {
        printf("    ✗ 检查不存在的键应该返回false\n");
        all_passed = false;
    }
    
    // 测试11: 插入NULL键或值
    printf("  11. 测试插入NULL键或值...\n");
    if (F_hash_table_put(table, NULL, &value1)) {
        printf("    ✗ 插入NULL键应该返回false\n");
        all_passed = false;
    }
    
    if (F_hash_table_put(table, "test", NULL)) {
        printf("    ✗ 插入NULL值应该返回false\n");
        all_passed = false;
    }
    
    // 测试12: 获取NULL键
    printf("  12. 测试获取NULL键...\n");
    if (F_hash_table_get(table, NULL) != NULL) {
        printf("    ✗ 获取NULL键应该返回NULL\n");
        all_passed = false;
    }
    
    // 测试13: 检查NULL键
    printf("  13. 测试检查NULL键...\n");
    if (F_hash_table_contains(table, NULL)) {
        printf("    ✗ 检查NULL键应该返回false\n");
        all_passed = false;
    }
    
    // 测试14: 删除NULL键
    printf("  14. 测试删除NULL键...\n");
    if (F_hash_table_remove(table, NULL)) {
        printf("    ✗ 删除NULL键应该返回false\n");
        all_passed = false;
    }
    
    // 测试15: 整数键测试
    printf("  15. 测试整数键...\n");
    F_destroy_hash_table(table);
    
    // 创建整数键哈希表
    table = F_create_hash_table(
        sizeof(int),
        sizeof(char[32]),
        F_int_hash_function,
        F_int_compare_function,
        8
    );
    
    if (table == NULL) {
        printf("    ✗ 无法创建整数键哈希表\n");
        all_passed = false;
    } else {
        int int_keys[] = {1, 2, 3, 4, 5};
        char* int_values[] = {"one", "two", "three", "four", "five"};
        
        for (int i = 0; i < 5; i++) {
            if (!F_hash_table_put(table, &int_keys[i], int_values[i])) {
                printf("    ✗ 插入整数键值对失败: %d -> %s\n", int_keys[i], int_values[i]);
                all_passed = false;
                break;
            }
        }
        
        if (all_passed) {
            for (int i = 0; i < 5; i++) {
                char* val = (char*)F_hash_table_get(table, &int_keys[i]);
                if (val == NULL || strcmp(val, int_values[i]) != 0) {
                    printf("    ✗ 获取整数键值失败: %d\n", int_keys[i]);
                    all_passed = false;
                    break;
                }
            }
        }
    }
    
    F_destroy_hash_table(table);
    printf("基本操作测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}
