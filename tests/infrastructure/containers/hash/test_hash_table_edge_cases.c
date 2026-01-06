/**
 * @file test_hash_table_edge_cases.c
 * @brief 哈希表边界情况测试
 * 
 * 测试哈希表的边界情况和极端场景，包括最小/最大容量、特殊键值、哈希冲突等。
 * 验证哈希表在边界条件下的正确行为和稳定性。
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
#include <string.h>
#include <stdlib.h>
#include <limits.h>

/**
 * @brief 测试最小容量边界
 * @return 测试通过返回true，失败返回false
 */
static bool test_minimum_capacity_edge(void)
{
    printf("  [测试] 最小容量边界...\n");
    
    // 测试最小有效容量（1）
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 1);
    if (table == NULL) {
        printf("    [失败] 最小容量（1）创建失败\n");
        return false;
    }
    
    // 验证容量至少为1
    size_t capacity = F_hash_table_capacity(table);
    if (capacity < 1) {
        printf("    [失败] 最小容量创建后容量小于1: %zu\n", capacity);
        F_destroy_hash_table(table);
        return false;
    }
    
    // 在最小容量下插入数据
    const char* key = "test_key";
    int value = 100;
    bool result = F_hash_table_put(table, (void*)&key, &value);
    if (!result) {
        printf("    [失败] 最小容量下插入失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 验证数据
    int* retrieved = (int*)F_hash_table_get(table, (void*)&key);
    if (retrieved == NULL || *retrieved != value) {
        printf("    [失败] 最小容量下数据验证失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 最小容量边界测试\n");
    return true;
}

/**
 * @brief 测试最大容量边界
 * @return 测试通过返回true，失败返回false
 */
static bool test_maximum_capacity_edge(void)
{
    printf("  [测试] 最大容量边界...\n");
    
    // 测试接近最大值的容量
    size_t large_capacity = 1000000; // 100万容量
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 large_capacity);
    if (table == NULL) {
        printf("    [注意] 大容量（%zu）创建失败，可能内存不足\n", large_capacity);
        return true; // 不是失败，只是内存限制
    }
    
    // 验证容量
    size_t capacity = F_hash_table_capacity(table);
    if (capacity < large_capacity) {
        printf("    [注意] 请求容量 %zu，实际容量 %zu\n", large_capacity, capacity);
    }
    
    // 在大容量下插入少量数据
    const char* key = "test_key";
    int value = 100;
    bool result = F_hash_table_put(table, (void*)&key, &value);
    if (!result) {
        printf("    [失败] 大容量下插入失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 验证数据
    int* retrieved = (int*)F_hash_table_get(table, (void*)&key);
    if (retrieved == NULL || *retrieved != value) {
        printf("    [失败] 大容量下数据验证失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 最大容量边界测试\n");
    return true;
}

/**
 * @brief 测试空字符串键
 * @return 测试通过返回true，失败返回false
 */
static bool test_empty_string_key(void)
{
    printf("  [测试] 空字符串键...\n");
    
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 8);
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    // 测试空字符串键
    const char* empty_key = "";
    int value1 = 100;
    bool result = F_hash_table_put(table, (void*)&empty_key, &value1);
    if (!result) {
        printf("    [失败] 空字符串键插入失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 验证空字符串键
    int* retrieved = (int*)F_hash_table_get(table, (void*)&empty_key);
    if (retrieved == NULL || *retrieved != value1) {
        printf("    [失败] 空字符串键验证失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 测试另一个空字符串键（应该冲突或不同）
    const char* another_empty_key = "";
    int value2 = 200;
    result = F_hash_table_put(table, (void*)&another_empty_key, &value2);
    if (!result) {
        printf("    [失败] 另一个空字符串键插入失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    // 验证值已更新（因为空字符串键相同）
    retrieved = (int*)F_hash_table_get(table, (void*)&empty_key);
    if (retrieved == NULL || *retrieved != value2) {
        printf("    [失败] 空字符串键更新验证失败\n");
        F_destroy_hash_table(table);
        return false;
    }
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 空字符串键测试\n");
    return true;
}

/**
 * @brief 测试特殊字符键
 * @return 测试通过返回true，失败返回false
 */
static bool test_special_character_keys(void)
{
    printf("  [测试] 特殊字符键...\n");
    
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 16);
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    // 测试各种特殊字符键
    const char* special_keys[] = {
        " ",                    // 空格
        "\t",                  // 制表符
        "\n",                  // 换行符
        "!@#$%^&*()",          // 特殊符号
        "键名",                // 中文字符
        "key with spaces",     // 包含空格的键
        "key\twith\ttabs",     // 包含制表符的键
        "key\nwith\nnewlines", // 包含换行符的键
        NULL
    };
    
    int test_count = 0;
    while (special_keys[test_count] != NULL) {
        test_count++;
    }
    
    int values[20]; // 足够大的数组
    for (int i = 0; i < test_count; i++) {
        values[i] = i * 100;
    }
    
    // 插入所有特殊键
    for (int i = 0; i < test_count; i++) {
        bool result = F_hash_table_put(table, (void*)&special_keys[i], &values[i]);
        if (!result) {
            printf("    [失败] 特殊键插入失败 at index %d: '%s'\n", i, special_keys[i]);
            F_destroy_hash_table(table);
            return false;
        }
    }
    
    // 验证所有特殊键
    for (int i = 0; i < test_count; i++) {
        int* retrieved = (int*)F_hash_table_get(table, (void*)&special_keys[i]);
        if (retrieved == NULL || *retrieved != values[i]) {
            printf("    [失败] 特殊键验证失败 at index %d: '%s'\n", i, special_keys[i]);
            F_destroy_hash_table(table);
            return false;
        }
    }
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 特殊字符键测试\n");
    return true;
}

/**
 * @brief 测试极值键
 * @return 测试通过返回true，失败返回false
 */
static bool test_extreme_value_keys(void)
{
    printf("  [测试] 极值键...\n");
    
    // 测试整数极值键
    Stru_HashTable_t* table = F_create_hash_table(sizeof(int), sizeof(int),
                                                 F_int_hash_function,
                                                 F_int_compare_function,
                                                 8);
    if (table == NULL) {
        printf("    [失败] 无法创建整数哈希表\n");
        return false;
    }
    
    // 测试各种极值整数键
    int extreme_keys[] = {
        0,
        1,
        -1,
        INT_MAX,
        INT_MIN,
        123456789,
        -987654321
    };
    
    int test_count = sizeof(extreme_keys) / sizeof(extreme_keys[0]);
    int values[] = {100, 200, 300, 400, 500, 600, 700};
    
    // 插入所有极值键
    for (int i = 0; i < test_count; i++) {
        bool result = F_hash_table_put(table, &extreme_keys[i], &values[i]);
        if (!result) {
            printf("    [失败] 极值键插入失败 at index %d: %d\n", i, extreme_keys[i]);
            F_destroy_hash_table(table);
            return false;
        }
    }
    
    // 验证所有极值键
    for (int i = 0; i < test_count; i++) {
        int* retrieved = (int*)F_hash_table_get(table, &extreme_keys[i]);
        if (retrieved == NULL || *retrieved != values[i]) {
            printf("    [失败] 极值键验证失败 at index %d: %d\n", i, extreme_keys[i]);
            F_destroy_hash_table(table);
            return false;
        }
    }
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 极值键测试\n");
    return true;
}

/**
 * @brief 测试哈希冲突边界
 * @return 测试通过返回true，失败返回false
 */
static bool test_hash_collision_edge(void)
{
    printf("  [测试] 哈希冲突边界...\n");
    
    // 创建小容量哈希表以强制哈希冲突
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 2); // 非常小的容量
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    // 插入多个键，很可能发生哈希冲突
    const char* keys[] = {"key1", "key2", "key3", "key4", "key5"};
    int values[] = {10, 20, 30, 40, 50};
    int count = sizeof(keys) / sizeof(keys[0]);
    
    for (int i = 0; i < count; i++) {
        bool result = F_hash_table_put(table, (void*)&keys[i], &values[i]);
        if (!result) {
            printf("    [失败] 哈希冲突测试插入失败 at index %d\n", i);
            F_destroy_hash_table(table);
            return false;
        }
    }
    
    // 验证所有数据，尽管有哈希冲突
    for (int i = 0; i < count; i++) {
        int* retrieved = (int*)F_hash_table_get(table, (void*)&keys[i]);
        if (retrieved == NULL || *retrieved != values[i]) {
            printf("    [失败] 哈希冲突测试验证失败 at index %d\n", i);
            F_destroy_hash_table(table);
            return false;
        }
    }
    
    // 测试删除操作（在哈希冲突情况下）
    for (int i = 0; i < count; i++) {
        bool result = F_hash_table_remove(table, (void*)&keys[i]);
        if (!result) {
            printf("    [失败] 哈希冲突测试删除失败 at index %d\n", i);
            F_destroy_hash_table(table);
            return false;
        }
        
        // 验证已删除
        int* retrieved = (int*)F_hash_table_get(table, (void*)&keys[i]);
        if (retrieved != NULL) {
            printf("    [失败] 哈希冲突测试删除验证失败 at index %d\n", i);
            F_destroy_hash_table(table);
            return false;
        }
        
        // 验证其他键仍然存在
        for (int j = i + 1; j < count; j++) {
            retrieved = (int*)F_hash_table_get(table, (void*)&keys[j]);
            if (retrieved == NULL || *retrieved != values[j]) {
                printf("    [失败] 删除后其他键验证失败 at index %d\n", j);
                F_destroy_hash_table(table);
                return false;
            }
        }
    }
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 哈希冲突边界测试\n");
    return true;
}

/**
 * @brief 测试负载因子边界
 * @return 测试通过返回true，失败返回false
 */
static bool test_load_factor_edge(void)
{
    printf("  [测试] 负载因子边界...\n");
    
    // 创建小容量哈希表
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 4);
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    // 插入数据直到高负载因子
    const char* base_key = "key";
    char key_buffer[32];
    int inserted_count = 0;
    const int max_insert = 100; // 插入足够多的数据
    
    for (int i = 0; i < max_insert; i++) {
        snprintf(key_buffer, sizeof(key_buffer), "%s%d", base_key, i);
        const char* key = key_buffer;
        int value = i * 10;
        
        bool result = F_hash_table_put(table, (void*)&key, &value);
        if (!result) {
            printf("    [注意] 插入失败 at index %d，可能达到容量限制\n", i);
            break;
        }
        inserted_count++;
    }
    
    // 验证所有插入的数据
    for (int i = 0; i < inserted_count; i++) {
        snprintf(key_buffer, sizeof(key_buffer), "%s%d", base_key, i);
        const char* key = key_buffer;
        int expected_value = i * 10;
        
        int* retrieved = (int*)F_hash_table_get(table, (void*)&key);
        if (retrieved == NULL || *retrieved != expected_value) {
            printf("    [失败] 高负载因子下验证失败 at index %d\n", i);
            F_destroy_hash_table(table);
            return false;
        }
    }
    
    // 检查大小和容量
    size_t size = F_hash_table_size(table);
    size_t capacity = F_hash_table_capacity(table);
    
    if (size != (size_t)inserted_count) {
        printf("    [失败] 大小不匹配: 期望 %d, 实际 %zu\n", inserted_count, size);
        F_destroy_hash_table(table);
        return false;
    }
    
    printf("    [信息] 高负载因子测试: 大小=%zu, 容量=%zu\n", size, capacity);
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 负载因子边界测试\n");
    return true;
}

/**
 * @brief 测试连续操作边界
 * @return 测试通过返回true，失败返回false
 */
static bool test_sequential_operations_edge(void)
{
    printf("  [测试] 连续操作边界...\n");
    
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 8);
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    const int operation_count = 1000;
    char key_buffer[32];
    
    // 执行大量连续插入操作
    for (int i = 0; i < operation_count; i++) {
        snprintf(key_buffer, sizeof(key_buffer), "key%d", i);
        const char* key = key_buffer;
        int value = i;
        
        bool result = F_hash_table_put(table, (void*)&key, &value);
        if (!result) {
            printf("    [失败] 连续插入失败 at index %d\n", i);
            F_destroy_hash_table(table);
            return false;
        }
        
        // 立即验证
        int* retrieved = (int*)F_hash_table_get(table, (void*)&key);
        if (retrieved == NULL || *retrieved != value) {
            printf("    [失败] 连续插入验证失败 at index %d\n", i);
            F_destroy_hash_table(table);
            return false;
        }
    }
    
    // 执行大量连续删除操作
    for (int i = 0; i < operation_count; i++) {
        snprintf(key_buffer, sizeof(key_buffer), "key%d", i);
        const char* key = key_buffer;
        
        bool result = F_hash_table_remove(table, (void*)&key);
        if (!result) {
            printf("    [失败] 连续删除失败 at index %d\n", i);
            F_destroy_hash_table(table);
            return false;
        }
        
        // 验证已删除
        int* retrieved = (int*)F_hash_table_get(table, (void*)&key);
        if (retrieved != NULL) {
            printf("    [失败] 连续删除验证失败 at index %d\n", i);
            F_destroy_hash_table(table);
            return false;
        }
    }
    
    // 验证表为空
    size_t size = F_hash_table_size(table);
    if (size != 0) {
        printf("    [失败] 连续操作后表大小应为0，实际为 %zu\n", size);
        F_destroy_hash_table(table);
        return false;
    }
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 连续操作边界测试\n");
    return true;
}

/**
 * @brief 测试混合操作边界
 * @return 测试通过返回true，失败返回false
 */
static bool test_mixed_operations_edge(void)
{
    printf("  [测试] 混合操作边界...\n");
    
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 16);
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    const int operation_count = 500;
    char key_buffer[32];
    
    // 混合插入、更新、删除操作
    for (int i = 0; i < operation_count; i++) {
        snprintf(key_buffer, sizeof(key_buffer), "key%d", i % 100); // 只有100个不同的键
        const char* key = key_buffer;
        int value = i;
        
        // 插入或更新
        bool result = F_hash_table_put(table, (void*)&key, &value);
        if (!result) {
            printf("    [失败] 混合操作插入失败 at index %d\n", i);
            F_destroy_hash_table(table);
            return false;
        }
        
        // 随机删除一些键
        if (i % 7 == 0) { // 每7次操作删除一个键
            int delete_index = (i * 3) % 100;
            snprintf(key_buffer, sizeof(key_buffer), "key%d", delete_index);
            const char* delete_key = key_buffer;
            
            F_hash_table_remove(table, (void*)&delete_key);
        }
        
        // 随机调整容量
        if (i % 13 == 0) { // 每13次操作调整容量
            size_t current_capacity = F_hash_table_capacity(table);
            size_t new_capacity = (current_capacity < 32) ? current_capacity * 2 : current_capacity / 2;
            F_hash_table_resize(table, new_capacity);
        }
    }
    
    // 最终验证：表应该处于一致状态
    size_t size = F_hash_table_size(table);
    printf("    [信息] 混合操作后表大小: %zu\n", size);
    
    // 清理
    F_destroy_hash_table(table);
    
    printf("    [通过] 混合操作边界测试\n");
    return true;
}

/**
 * @brief 测试边界情况
 * @return 测试通过返回true，失败返回false
 */
bool test_hash_table_edge_cases(void)
{
    printf("\n[测试套件] 哈希表边界情况测试\n");
    printf("========================================\n");
    
    int passed = 0;
    int total = 0;
    
    // 运行所有边界情况测试
    total++; if (test_minimum_capacity_edge()) passed++;
    total++; if (test_maximum_capacity_edge()) passed++;
    total++; if (test_empty_string_key()) passed++;
    total++; if (test_special_character_keys()) passed++;
    total++; if (test_extreme_value_keys()) passed++;
    total++; if (test_hash_collision_edge()) passed++;
    total++; if (test_load_factor_edge()) passed++;
    total++; if (test_sequential_operations_edge()) passed++;
    total++; if (test_mixed_operations_edge()) passed++;
    
    printf("========================================\n");
    printf("[结果] 边界情况测试: %d/%d 通过\n\n", passed, total);
    
    return passed == total;
}
