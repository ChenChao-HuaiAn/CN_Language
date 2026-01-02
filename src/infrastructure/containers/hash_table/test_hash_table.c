/******************************************************************************
 * 文件名: test_hash_table.c
 * 功能: CN_Language哈希表容器测试
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，测试哈希表功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_hash_table.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

// ============================================================================
// 测试辅助函数
// ============================================================================

/**
 * @brief 测试整数键值对
 */
static void test_int_hash_table(void)
{
    printf("测试整数哈希表...\n");
    
    // 创建哈希表（整数键，整数值）
    Stru_CN_HashTable_t* table = CN_hash_table_create(
        sizeof(int), sizeof(int), 10, Eum_HASH_TABLE_OPEN_ADDRESSING);
    
    assert(table != NULL);
    printf("  创建成功\n");
    
    // 测试插入
    for (int i = 0; i < 100; i++)
    {
        int key = i;
        int value = i * 10;
        assert(CN_hash_table_put(table, &key, &value));
    }
    
    assert(CN_hash_table_size(table) == 100);
    printf("  插入100个元素成功\n");
    
    // 测试查找
    for (int i = 0; i < 100; i++)
    {
        int key = i;
        int* value_ptr = (int*)CN_hash_table_get(table, &key);
        assert(value_ptr != NULL);
        assert(*value_ptr == i * 10);
    }
    printf("  查找100个元素成功\n");
    
    // 测试更新
    for (int i = 0; i < 50; i++)
    {
        int key = i;
        int new_value = i * 100;
        assert(CN_hash_table_put(table, &key, &new_value));
    }
    
    for (int i = 0; i < 50; i++)
    {
        int key = i;
        int* value_ptr = (int*)CN_hash_table_get(table, &key);
        assert(value_ptr != NULL);
        assert(*value_ptr == i * 100);
    }
    printf("  更新50个元素成功\n");
    
    // 测试删除
    for (int i = 0; i < 25; i++)
    {
        int key = i;
        assert(CN_hash_table_remove(table, &key));
    }
    
    assert(CN_hash_table_size(table) == 75);
    printf("  删除25个元素成功\n");
    
    // 测试不存在的键
    int non_existent_key = 999;
    assert(CN_hash_table_get(table, &non_existent_key) == NULL);
    assert(!CN_hash_table_contains(table, &non_existent_key));
    printf("  不存在的键测试成功\n");
    
    // 测试迭代器
    Stru_CN_HashTableIterator_t* iterator = CN_hash_table_iterator_create(table);
    assert(iterator != NULL);
    
    size_t count = 0;
    while (CN_hash_table_iterator_has_next(iterator))
    {
        int* key_ptr;
        int* value_ptr;
        assert(CN_hash_table_iterator_next(iterator, (void**)&key_ptr, (void**)&value_ptr));
        count++;
    }
    
    assert(count == CN_hash_table_size(table));
    printf("  迭代器测试成功，遍历了%zu个元素\n", count);
    
    CN_hash_table_iterator_destroy(iterator);
    CN_hash_table_destroy(table);
    printf("  整数哈希表测试通过\n\n");
}

/**
 * @brief 测试字符串键值对
 */
static void test_string_hash_table(void)
{
    printf("测试字符串哈希表...\n");
    
    // 创建哈希表（字符串键，整数值）
    Stru_CN_HashTable_t* table = CN_hash_table_create_custom(
        sizeof(char*), sizeof(int), 10, Eum_HASH_TABLE_SEPARATE_CHAINING,
        CN_hash_table_default_string_hash, NULL, NULL, NULL, NULL, NULL);
    
    assert(table != NULL);
    printf("  创建成功\n");
    
    // 测试数据
    const char* keys[] = {"apple", "banana", "cherry", "date", "elderberry",
                         "fig", "grape", "honeydew", "kiwi", "lemon"};
    int values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 测试插入
    for (int i = 0; i < 10; i++)
    {
        assert(CN_hash_table_put(table, &keys[i], &values[i]));
    }
    
    assert(CN_hash_table_size(table) == 10);
    printf("  插入10个元素成功\n");
    
    // 测试查找
    for (int i = 0; i < 10; i++)
    {
        int* value_ptr = (int*)CN_hash_table_get(table, &keys[i]);
        assert(value_ptr != NULL);
        assert(*value_ptr == values[i]);
    }
    printf("  查找10个元素成功\n");
    
    // 测试键集合
    void** key_array;
    size_t key_count = CN_hash_table_keys(table, &key_array);
    assert(key_count == 10);
    
    // 清理键数组
    for (size_t i = 0; i < key_count; i++)
    {
        free(key_array[i]);
    }
    free(key_array);
    printf("  获取键集合成功\n");
    
    // 测试值集合
    void** value_array;
    size_t value_count = CN_hash_table_values(table, &value_array);
    assert(value_count == 10);
    
    // 清理值数组
    for (size_t i = 0; i < value_count; i++)
    {
        free(value_array[i]);
    }
    free(value_array);
    printf("  获取值集合成功\n");
    
    // 测试复制
    Stru_CN_HashTable_t* copy = CN_hash_table_copy(table);
    assert(copy != NULL);
    assert(CN_hash_table_size(copy) == CN_hash_table_size(table));
    
    for (int i = 0; i < 10; i++)
    {
        int* value_ptr = (int*)CN_hash_table_get(copy, &keys[i]);
        assert(value_ptr != NULL);
        assert(*value_ptr == values[i]);
    }
    printf("  复制哈希表成功\n");
    
    CN_hash_table_destroy(copy);
    CN_hash_table_destroy(table);
    printf("  字符串哈希表测试通过\n\n");
}

/**
 * @brief 测试性能
 */
static void test_performance(void)
{
    printf("测试哈希表性能...\n");
    
    // 测试开放寻址法
    Stru_CN_HashTable_t* open_table = CN_hash_table_create(
        sizeof(int), sizeof(int), 100, Eum_HASH_TABLE_OPEN_ADDRESSING);
    
    clock_t start = clock();
    for (int i = 0; i < 10000; i++)
    {
        int key = i;
        int value = i * 2;
        CN_hash_table_put(open_table, &key, &value);
    }
    clock_t end = clock();
    
    double open_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("  开放寻址法插入10000个元素: %.4f秒\n", open_time);
    
    // 获取统计信息
    double avg_probe_length;
    size_t max_probe_length;
    size_t empty_buckets;
    CN_hash_table_stats(open_table, &avg_probe_length, &max_probe_length, &empty_buckets);
    
    printf("  平均探测长度: %.2f\n", avg_probe_length);
    printf("  最大探测长度: %zu\n", max_probe_length);
    printf("  空桶数量: %zu\n", empty_buckets);
    
    CN_hash_table_destroy(open_table);
    
    // 测试链地址法
    Stru_CN_HashTable_t* chain_table = CN_hash_table_create(
        sizeof(int), sizeof(int), 100, Eum_HASH_TABLE_SEPARATE_CHAINING);
    
    start = clock();
    for (int i = 0; i < 10000; i++)
    {
        int key = i;
        int value = i * 2;
        CN_hash_table_put(chain_table, &key, &value);
    }
    end = clock();
    
    double chain_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("  链地址法插入10000个元素: %.4f秒\n", chain_time);
    
    // 获取统计信息
    CN_hash_table_stats(chain_table, &avg_probe_length, &max_probe_length, &empty_buckets);
    
    printf("  平均探测长度: %.2f\n", avg_probe_length);
    printf("  最大探测长度: %zu\n", max_probe_length);
    printf("  空桶数量: %zu\n", empty_buckets);
    
    CN_hash_table_destroy(chain_table);
    printf("  性能测试完成\n\n");
}

/**
 * @brief 测试动态扩容
 */
static void test_resize(void)
{
    printf("测试哈希表动态扩容...\n");
    
    // 创建小容量哈希表
    Stru_CN_HashTable_t* table = CN_hash_table_create(
        sizeof(int), sizeof(int), 10, Eum_HASH_TABLE_OPEN_ADDRESSING);
    
    assert(table != NULL);
    size_t initial_capacity = CN_hash_table_capacity(table);
    printf("  初始容量: %zu\n", initial_capacity);
    
    // 插入大量元素触发多次扩容
    for (int i = 0; i < 1000; i++)
    {
        int key = i;
        int value = i * 3;
        CN_hash_table_put(table, &key, &value);
    }
    
    size_t final_capacity = CN_hash_table_capacity(table);
    printf("  最终容量: %zu\n", final_capacity);
    printf("  哈希表大小: %zu\n", CN_hash_table_size(table));
    printf("  负载因子: %.2f\n", CN_hash_table_load_factor(table));
    
    // 验证所有元素都存在
    for (int i = 0; i < 1000; i++)
    {
        int key = i;
        int* value_ptr = (int*)CN_hash_table_get(table, &key);
        assert(value_ptr != NULL);
        assert(*value_ptr == i * 3);
    }
    
    CN_hash_table_destroy(table);
    printf("  动态扩容测试通过\n\n");
}

/**
 * @brief 测试合并操作
 */
static void test_merge(void)
{
    printf("测试哈希表合并...\n");
    
    // 创建两个哈希表
    Stru_CN_HashTable_t* table1 = CN_hash_table_create(
        sizeof(int), sizeof(int), 10, Eum_HASH_TABLE_OPEN_ADDRESSING);
    Stru_CN_HashTable_t* table2 = CN_hash_table_create(
        sizeof(int), sizeof(int), 10, Eum_HASH_TABLE_OPEN_ADDRESSING);
    
    // 向第一个表插入元素
    for (int i = 0; i < 50; i++)
    {
        int key = i;
        int value = i * 10;
        CN_hash_table_put(table1, &key, &value);
    }
    
    // 向第二个表插入元素（部分重叠）
    for (int i = 25; i < 75; i++)
    {
        int key = i;
        int value = i * 20;
        CN_hash_table_put(table2, &key, &value);
    }
    
    // 测试不覆盖合并
    Stru_CN_HashTable_t* merged_no_overwrite = CN_hash_table_merge(table1, table2, false);
    assert(merged_no_overwrite != NULL);
    assert(CN_hash_table_size(merged_no_overwrite) == 75); // 0-74
    
    // 检查重叠键的值（应该使用table1的值）
    for (int i = 25; i < 50; i++)
    {
        int key = i;
        int* value_ptr = (int*)CN_hash_table_get(merged_no_overwrite, &key);
        assert(value_ptr != NULL);
        assert(*value_ptr == i * 10); // table1的值
    }
    printf("  不覆盖合并测试通过\n");
    
    // 测试覆盖合并
    Stru_CN_HashTable_t* merged_overwrite = CN_hash_table_merge(table1, table2, true);
    assert(merged_overwrite != NULL);
    assert(CN_hash_table_size(merged_overwrite) == 75);
    
    // 检查重叠键的值（应该使用table2的值）
    for (int i = 25; i < 50; i++)
    {
        int key = i;
        int* value_ptr = (int*)CN_hash_table_get(merged_overwrite, &key);
        assert(value_ptr != NULL);
        assert(*value_ptr == i * 20); // table2的值
    }
    printf("  覆盖合并测试通过\n");
    
    CN_hash_table_destroy(table1);
    CN_hash_table_destroy(table2);
    CN_hash_table_destroy(merged_no_overwrite);
    CN_hash_table_destroy(merged_overwrite);
    printf("  合并测试完成\n\n");
}

// ============================================================================
// 主测试函数
// ============================================================================

int main(void)
{
    printf("开始CN_Language哈希表测试\n");
    printf("========================\n\n");
    
    test_int_hash_table();
    test_string_hash_table();
    test_performance();
    test_resize();
    test_merge();
    
    printf("所有测试通过！\n");
    return 0;
}
