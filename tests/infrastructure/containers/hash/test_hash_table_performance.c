/**
 * @file test_hash_table_performance.c
 * @brief 哈希表性能特征测试
 * 
 * 测试哈希表的性能特征，包括插入、查找、删除操作的时间复杂度，
 * 以及内存使用和扩展性能。验证哈希表在大数据量下的表现。
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
#include <time.h>

/**
 * @brief 简单计时器结构
 */
typedef struct {
    clock_t start_time;
    clock_t end_time;
} PerformanceTimer_t;

/**
 * @brief 开始计时
 */
static void start_timer(PerformanceTimer_t* timer)
{
    timer->start_time = clock();
}

/**
 * @brief 结束计时
 * @return 经过的时间（毫秒）
 */
static double end_timer(PerformanceTimer_t* timer)
{
    timer->end_time = clock();
    double elapsed_ms = ((double)(timer->end_time - timer->start_time) * 1000.0) / CLOCKS_PER_SEC;
    return elapsed_ms;
}

/**
 * @brief 测试批量插入性能
 * @return 测试通过返回true，失败返回false
 */
static bool test_bulk_insert_performance(void)
{
    printf("  [测试] 批量插入性能...\n");
    
    // 创建哈希表
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 1000); // 初始容量1000
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    const int item_count = 10000;
    char** keys = (char**)malloc(item_count * sizeof(char*));
    int* values = (int*)malloc(item_count * sizeof(int));
    
    if (keys == NULL || values == NULL) {
        printf("    [失败] 内存分配失败\n");
        free(keys);
        free(values);
        F_destroy_hash_table(table);
        return false;
    }
    
    // 准备测试数据
    for (int i = 0; i < item_count; i++) {
        keys[i] = (char*)malloc(32 * sizeof(char));
        if (keys[i] != NULL) {
            snprintf(keys[i], 32, "key_%08d", i);
        }
        values[i] = i * 10;
    }
    
    // 批量插入性能测试
    PerformanceTimer_t timer;
    start_timer(&timer);
    
    int successful_inserts = 0;
    for (int i = 0; i < item_count; i++) {
        if (keys[i] != NULL) {
            if (F_hash_table_put(table, (void*)&keys[i], &values[i])) {
                successful_inserts++;
            }
        }
    }
    
    double insert_time = end_timer(&timer);
    
    // 验证插入数量
    size_t table_size = F_hash_table_size(table);
    if (table_size != (size_t)successful_inserts) {
        printf("    [失败] 插入数量不匹配: 期望 %d, 实际 %zu\n", successful_inserts, table_size);
        
        // 清理
        for (int i = 0; i < item_count; i++) {
            free(keys[i]);
        }
        free(keys);
        free(values);
        F_destroy_hash_table(table);
        return false;
    }
    
    printf("    [信息] 批量插入 %d 个项目耗时: %.2f ms (平均: %.4f ms/个)\n",
           successful_inserts, insert_time, insert_time / successful_inserts);
    
    // 清理
    for (int i = 0; i < item_count; i++) {
        free(keys[i]);
    }
    free(keys);
    free(values);
    F_destroy_hash_table(table);
    
    printf("    [通过] 批量插入性能测试\n");
    return true;
}

/**
 * @brief 测试查找性能
 * @return 测试通过返回true，失败返回false
 */
static bool test_lookup_performance(void)
{
    printf("  [测试] 查找性能...\n");
    
    // 创建哈希表并插入测试数据
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 1000);
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    const int item_count = 5000;
    char** keys = (char**)malloc(item_count * sizeof(char*));
    int* values = (int*)malloc(item_count * sizeof(int));
    
    if (keys == NULL || values == NULL) {
        printf("    [失败] 内存分配失败\n");
        free(keys);
        free(values);
        F_destroy_hash_table(table);
        return false;
    }
    
    // 插入测试数据
    for (int i = 0; i < item_count; i++) {
        keys[i] = (char*)malloc(32 * sizeof(char));
        if (keys[i] != NULL) {
            snprintf(keys[i], 32, "key_%08d", i);
            values[i] = i * 10;
            F_hash_table_put(table, (void*)&keys[i], &values[i]);
        }
    }
    
    // 查找性能测试 - 查找所有存在的键
    PerformanceTimer_t timer;
    start_timer(&timer);
    
    int found_count = 0;
    for (int i = 0; i < item_count; i++) {
        if (keys[i] != NULL) {
            int* retrieved = (int*)F_hash_table_get(table, (void*)&keys[i]);
            if (retrieved != NULL && *retrieved == values[i]) {
                found_count++;
            }
        }
    }
    
    double lookup_time = end_timer(&timer);
    
    if (found_count != item_count) {
        printf("    [失败] 查找数量不匹配: 期望 %d, 实际 %d\n", item_count, found_count);
        
        // 清理
        for (int i = 0; i < item_count; i++) {
            free(keys[i]);
        }
        free(keys);
        free(values);
        F_destroy_hash_table(table);
        return false;
    }
    
    printf("    [信息] 查找 %d 个项目耗时: %.2f ms (平均: %.4f ms/个)\n",
           item_count, lookup_time, lookup_time / item_count);
    
    // 查找性能测试 - 查找不存在的键
    start_timer(&timer);
    
    int not_found_count = 0;
    const int missing_count = 1000;
    for (int i = 0; i < missing_count; i++) {
        char missing_key[32];
        snprintf(missing_key, 32, "missing_key_%08d", i);
        const char* key_ptr = missing_key;
        
        int* retrieved = (int*)F_hash_table_get(table, (void*)&key_ptr);
        if (retrieved == NULL) {
            not_found_count++;
        }
    }
    
    double missing_lookup_time = end_timer(&timer);
    
    printf("    [信息] 查找 %d 个不存在键耗时: %.2f ms (平均: %.4f ms/个)\n",
           missing_count, missing_lookup_time, missing_lookup_time / missing_count);
    
    // 清理
    for (int i = 0; i < item_count; i++) {
        free(keys[i]);
    }
    free(keys);
    free(values);
    F_destroy_hash_table(table);
    
    printf("    [通过] 查找性能测试\n");
    return true;
}

/**
 * @brief 测试删除性能
 * @return 测试通过返回true，失败返回false
 */
static bool test_delete_performance(void)
{
    printf("  [测试] 删除性能...\n");
    
    // 创建哈希表并插入测试数据
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 1000);
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    const int item_count = 5000;
    char** keys = (char**)malloc(item_count * sizeof(char*));
    int* values = (int*)malloc(item_count * sizeof(int));
    
    if (keys == NULL || values == NULL) {
        printf("    [失败] 内存分配失败\n");
        free(keys);
        free(values);
        F_destroy_hash_table(table);
        return false;
    }
    
    // 插入测试数据
    for (int i = 0; i < item_count; i++) {
        keys[i] = (char*)malloc(32 * sizeof(char));
        if (keys[i] != NULL) {
            snprintf(keys[i], 32, "key_%08d", i);
            values[i] = i * 10;
            F_hash_table_put(table, (void*)&keys[i], &values[i]);
        }
    }
    
    // 删除性能测试
    PerformanceTimer_t timer;
    start_timer(&timer);
    
    int deleted_count = 0;
    for (int i = 0; i < item_count; i++) {
        if (keys[i] != NULL) {
            if (F_hash_table_remove(table, (void*)&keys[i])) {
                deleted_count++;
            }
        }
    }
    
    double delete_time = end_timer(&timer);
    
    // 验证所有项都已删除
    size_t table_size = F_hash_table_size(table);
    if (table_size != 0) {
        printf("    [失败] 删除后表大小应为0，实际为 %zu\n", table_size);
        
        // 清理
        for (int i = 0; i < item_count; i++) {
            free(keys[i]);
        }
        free(keys);
        free(values);
        F_destroy_hash_table(table);
        return false;
    }
    
    printf("    [信息] 删除 %d 个项目耗时: %.2f ms (平均: %.4f ms/个)\n",
           deleted_count, delete_time, delete_time / deleted_count);
    
    // 清理
    for (int i = 0; i < item_count; i++) {
        free(keys[i]);
    }
    free(keys);
    free(values);
    F_destroy_hash_table(table);
    
    printf("    [通过] 删除性能测试\n");
    return true;
}

/**
 * @brief 测试内存使用性能
 * @return 测试通过返回true，失败返回false
 */
static bool test_memory_usage_performance(void)
{
    printf("  [测试] 内存使用性能...\n");
    
    const int test_sizes[] = {100, 1000, 5000, 10000};
    int test_count = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (int t = 0; t < test_count; t++) {
        int item_count = test_sizes[t];
        
        // 创建哈希表
        Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                     F_string_hash_function,
                                                     F_string_compare_function,
                                                     item_count / 2); // 初始容量为一半
        if (table == NULL) {
            printf("    [失败] 无法创建测试哈希表，大小: %d\n", item_count);
            return false;
        }
        
        // 插入数据
        char key_buffer[32];
        for (int i = 0; i < item_count; i++) {
            snprintf(key_buffer, sizeof(key_buffer), "key_%08d", i);
            const char* key = key_buffer;
            int value = i;
            F_hash_table_put(table, (void*)&key, &value);
        }
        
        // 获取容量和大小信息
        size_t capacity = F_hash_table_capacity(table);
        size_t size = F_hash_table_size(table);
        
        printf("    [信息] 大小 %d: 容量=%zu, 实际大小=%zu, 负载因子=%.2f\n",
               item_count, capacity, size, (double)size / capacity);
        
        F_destroy_hash_table(table);
    }
    
    printf("    [通过] 内存使用性能测试\n");
    return true;
}

/**
 * @brief 测试调整容量性能
 * @return 测试通过返回true，失败返回false
 */
static bool test_resize_performance(void)
{
    printf("  [测试] 调整容量性能...\n");
    
    // 创建小容量哈希表
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 10); // 很小的初始容量
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    const int item_count = 1000;
    PerformanceTimer_t timer;
    
    // 测试插入时自动调整容量的性能
    start_timer(&timer);
    
    char key_buffer[32];
    for (int i = 0; i < item_count; i++) {
        snprintf(key_buffer, sizeof(key_buffer), "key_%08d", i);
        const char* key = key_buffer;
        int value = i;
        F_hash_table_put(table, (void*)&key, &value);
    }
    
    double insert_with_resize_time = end_timer(&timer);
    
    size_t final_capacity = F_hash_table_capacity(table);
    size_t final_size = F_hash_table_size(table);
    
    printf("    [信息] 插入 %d 项（带自动调整容量）耗时: %.2f ms\n", item_count, insert_with_resize_time);
    printf("    [信息] 最终容量: %zu, 最终大小: %zu\n", final_capacity, final_size);
    
    // 测试手动调整容量性能
    start_timer(&timer);
    
    bool resize_result = F_hash_table_resize(table, final_capacity * 2);
    if (!resize_result) {
        printf("    [注意] 调整容量失败，可能内存不足\n");
    } else {
        double resize_time = end_timer(&timer);
        size_t new_capacity = F_hash_table_capacity(table);
        printf("    [信息] 调整容量到 %zu 耗时: %.2f ms\n", new_capacity, resize_time);
    }
    
    // 验证数据完整性
    int verified_count = 0;
    for (int i = 0; i < item_count; i++) {
        snprintf(key_buffer, sizeof(key_buffer), "key_%08d", i);
        const char* key = key_buffer;
        int expected_value = i;
        
        int* retrieved = (int*)F_hash_table_get(table, (void*)&key);
        if (retrieved != NULL && *retrieved == expected_value) {
            verified_count++;
        }
    }
    
    if (verified_count != item_count) {
        printf("    [失败] 调整容量后数据验证失败: 期望 %d, 实际 %d\n", item_count, verified_count);
        F_destroy_hash_table(table);
        return false;
    }
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 调整容量性能测试\n");
    return true;
}

/**
 * @brief 测试混合操作性能
 * @return 测试通过返回true，失败返回false
 */
static bool test_mixed_operations_performance(void)
{
    printf("  [测试] 混合操作性能...\n");
    
    // 创建哈希表
    Stru_HashTable_t* table = F_create_hash_table(sizeof(char*), sizeof(int),
                                                 F_string_hash_function,
                                                 F_string_compare_function,
                                                 1000);
    if (table == NULL) {
        printf("    [失败] 无法创建测试哈希表\n");
        return false;
    }
    
    const int operation_count = 10000;
    PerformanceTimer_t timer;
    start_timer(&timer);
    
    char key_buffer[32];
    int existing_items = 0;
    
    // 执行混合操作
    for (int i = 0; i < operation_count; i++) {
        int operation_type = i % 10;
        
        if (operation_type < 6) {
            // 60% 插入/更新操作
            int key_index = i % 500; // 500个不同的键
            snprintf(key_buffer, sizeof(key_buffer), "key_%08d", key_index);
            const char* key = key_buffer;
            int value = i;
            
            F_hash_table_put(table, (void*)&key, &value);
            if (key_index >= existing_items) {
                existing_items = key_index + 1;
            }
        } else if (operation_type < 8) {
            // 20% 查找操作
            int key_index = (i * 3) % existing_items;
            if (key_index < existing_items) {
                snprintf(key_buffer, sizeof(key_buffer), "key_%08d", key_index);
                const char* key = key_buffer;
                F_hash_table_get(table, (void*)&key);
            }
        } else {
            // 20% 删除操作
            int key_index = (i * 7) % existing_items;
            if (key_index < existing_items && existing_items > 0) {
                snprintf(key_buffer, sizeof(key_buffer), "key_%08d", key_index);
                const char* key = key_buffer;
                F_hash_table_remove(table, (void*)&key);
            }
        }
    }
    
    double mixed_operations_time = end_timer(&timer);
    
    printf("    [信息] %d 次混合操作耗时: %.2f ms (平均: %.4f ms/次)\n",
           operation_count, mixed_operations_time, mixed_operations_time / operation_count);
    
    // 验证最终状态
    size_t final_size = F_hash_table_size(table);
    printf("    [信息] 混合操作后表大小: %zu\n", final_size);
    
    F_destroy_hash_table(table);
    
    printf("    [通过] 混合操作性能测试\n");
    return true;
}

/**
 * @brief 测试性能特征
 * @return 测试通过返回true，失败返回false
 */
bool test_hash_table_performance(void)
{
    printf("\n[测试套件] 哈希表性能特征测试\n");
    printf("========================================\n");
    printf("注意：性能测试结果可能因系统负载和配置而异\n");
    printf("========================================\n");
    
    int passed = 0;
    int total = 0;
    
    // 运行所有性能测试
    total++; if (test_bulk_insert_performance()) passed++;
    total++; if (test_lookup_performance()) passed++;
    total++; if (test_delete_performance()) passed++;
    total++; if (test_memory_usage_performance()) passed++;
    total++; if (test_resize_performance()) passed++;
    total++; if (test_mixed_operations_performance()) passed++;
    
    printf("========================================\n");
    printf("[结果] 性能测试: %d/%d 通过\n", passed, total);
    printf("注意：性能测试主要验证功能正确性，时间数据仅供参考\n");
    printf("========================================\n\n");
    
    return passed == total;
}
