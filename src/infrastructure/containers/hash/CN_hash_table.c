/**
 * @file CN_hash_table.c
 * @brief 哈希表模块实现文件
 * 
 * 实现哈希表数据结构的核心功能。
 * 包括创建、销毁、插入、查找、删除、遍历等操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_hash_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 默认初始容量
#define CN_HASH_TABLE_INITIAL_CAPACITY 16
// 负载因子阈值（当size/capacity >= 此值时扩容）
#define CN_HASH_TABLE_LOAD_FACTOR 0.75
// 扩容因子
#define CN_HASH_TABLE_GROWTH_FACTOR 2

/**
 * @brief 内部函数：创建哈希条目
 * 
 * 分配并初始化一个新的哈希条目。
 * 
 * @param key 键指针
 * @param value 值指针
 * @param key_size 键大小
 * @param value_size 值大小
 * @return 指向新条目的指针，失败返回NULL
 */
static Stru_HashEntry_t* create_entry(void* key, void* value, 
                                     size_t key_size, size_t value_size)
{
    if (key == NULL || value == NULL)
    {
        return NULL;
    }
    
    // 分配条目结构体
    Stru_HashEntry_t* entry = (Stru_HashEntry_t*)malloc(sizeof(Stru_HashEntry_t));
    if (entry == NULL)
    {
        return NULL;
    }
    
    // 分配键内存并复制数据
    entry->key = malloc(key_size);
    if (entry->key == NULL)
    {
        free(entry);
        return NULL;
    }
    memcpy(entry->key, key, key_size);
    
    // 分配值内存并复制数据
    entry->value = malloc(value_size);
    if (entry->value == NULL)
    {
        free(entry->key);
        free(entry);
        return NULL;
    }
    memcpy(entry->value, value, value_size);
    
    entry->next = NULL;
    
    return entry;
}

/**
 * @brief 内部函数：释放哈希条目
 * 
 * 释放条目占用的内存。
 * 
 * @param entry 要释放的条目指针
 */
static void free_entry(Stru_HashEntry_t* entry)
{
    if (entry == NULL)
    {
        return;
    }
    
    if (entry->key != NULL)
    {
        free(entry->key);
        entry->key = NULL;
    }
    
    if (entry->value != NULL)
    {
        free(entry->value);
        entry->value = NULL;
    }
    
    free(entry);
}

/**
 * @brief 内部函数：计算桶索引
 * 
 * 根据键计算桶索引。
 * 
 * @param table 哈希表指针
 * @param key 键指针
 * @return 桶索引
 */
static size_t get_bucket_index(Stru_HashTable_t* table, void* key)
{
    if (table == NULL || key == NULL || table->capacity == 0)
    {
        return 0;
    }
    
    uint64_t hash = table->hash_function(key);
    return hash % table->capacity;
}

/**
 * @brief 内部函数：查找条目
 * 
 * 在指定桶中查找键对应的条目。
 * 
 * @param table 哈希表指针
 * @param bucket_index 桶索引
 * @param key 键指针
 * @param prev_entry 输出参数，指向找到条目的前一个条目（可为NULL）
 * @return 指向条目的指针，未找到返回NULL
 */
static Stru_HashEntry_t* find_entry(Stru_HashTable_t* table, size_t bucket_index,
                                   void* key, Stru_HashEntry_t** prev_entry)
{
    if (table == NULL || key == NULL || bucket_index >= table->capacity)
    {
        return NULL;
    }
    
    Stru_HashEntry_t* current = table->buckets[bucket_index];
    Stru_HashEntry_t* prev = NULL;
    
    while (current != NULL)
    {
        if (table->compare_function(current->key, key) == 0)
        {
            if (prev_entry != NULL)
            {
                *prev_entry = prev;
            }
            return current;
        }
        
        prev = current;
        current = current->next;
    }
    
    return NULL;
}

/**
 * @brief 内部函数：确保哈希表有足够容量
 * 
 * 检查哈希表是否需要扩容，如果需要则扩容。
 * 
 * @param table 哈希表指针
 * @return 容量足够或扩容成功返回true，失败返回false
 */
static bool ensure_capacity(Stru_HashTable_t* table)
{
    if (table == NULL)
    {
        return false;
    }
    
    // 检查是否需要扩容
    if (table->capacity == 0 || 
        (double)table->size / (double)table->capacity >= CN_HASH_TABLE_LOAD_FACTOR)
    {
        size_t new_capacity = table->capacity * CN_HASH_TABLE_GROWTH_FACTOR;
        if (new_capacity < CN_HASH_TABLE_INITIAL_CAPACITY)
        {
            new_capacity = CN_HASH_TABLE_INITIAL_CAPACITY;
        }
        
        return F_hash_table_resize(table, new_capacity);
    }
    
    return true;
}

Stru_HashTable_t* F_create_hash_table(size_t key_size, size_t value_size,
                                     uint64_t (*hash_function)(void* key),
                                     int (*compare_function)(void* key1, void* key2),
                                     size_t initial_capacity)
{
    if (key_size == 0 || value_size == 0 || 
        hash_function == NULL || compare_function == NULL)
    {
        return NULL;
    }
    
    // 确保初始容量至少为1
    if (initial_capacity == 0)
    {
        initial_capacity = CN_HASH_TABLE_INITIAL_CAPACITY;
    }
    
    // 分配哈希表结构体
    Stru_HashTable_t* table = (Stru_HashTable_t*)malloc(sizeof(Stru_HashTable_t));
    if (table == NULL)
    {
        return NULL;
    }
    
    // 初始化字段
    table->capacity = initial_capacity;
    table->size = 0;
    table->key_size = key_size;
    table->value_size = value_size;
    table->hash_function = hash_function;
    table->compare_function = compare_function;
    
    // 分配桶数组
    table->buckets = (Stru_HashEntry_t**)calloc(initial_capacity, sizeof(Stru_HashEntry_t*));
    if (table->buckets == NULL)
    {
        free(table);
        return NULL;
    }
    
    return table;
}

void F_destroy_hash_table(Stru_HashTable_t* table)
{
    if (table == NULL)
    {
        return;
    }
    
    // 清空哈希表
    F_hash_table_clear(table);
    
    // 释放桶数组
    if (table->buckets != NULL)
    {
        free(table->buckets);
        table->buckets = NULL;
    }
    
    // 释放哈希表结构体
    free(table);
}

bool F_hash_table_put(Stru_HashTable_t* table, void* key, void* value)
{
    if (table == NULL || key == NULL || value == NULL)
    {
        return false;
    }
    
    // 确保有足够容量
    if (!ensure_capacity(table))
    {
        return false;
    }
    
    // 计算桶索引
    size_t bucket_index = get_bucket_index(table, key);
    
    // 查找是否已存在相同键的条目
    Stru_HashEntry_t* existing_entry = find_entry(table, bucket_index, key, NULL);
    
    if (existing_entry != NULL)
    {
        // 更新现有条目的值
        void* new_value = malloc(table->value_size);
        if (new_value == NULL)
        {
            return false;
        }
        
        memcpy(new_value, value, table->value_size);
        free(existing_entry->value);
        existing_entry->value = new_value;
        return true;
    }
    else
    {
        // 创建新条目
        Stru_HashEntry_t* new_entry = create_entry(key, value, 
                                                  table->key_size, table->value_size);
        if (new_entry == NULL)
        {
            return false;
        }
        
        // 插入到桶的头部
        new_entry->next = table->buckets[bucket_index];
        table->buckets[bucket_index] = new_entry;
        table->size++;
        
        return true;
    }
}

void* F_hash_table_get(Stru_HashTable_t* table, void* key)
{
    if (table == NULL || key == NULL)
    {
        return NULL;
    }
    
    size_t bucket_index = get_bucket_index(table, key);
    Stru_HashEntry_t* entry = find_entry(table, bucket_index, key, NULL);
    
    if (entry != NULL)
    {
        return entry->value;
    }
    
    return NULL;
}

bool F_hash_table_remove(Stru_HashTable_t* table, void* key)
{
    if (table == NULL || key == NULL)
    {
        return false;
    }
    
    size_t bucket_index = get_bucket_index(table, key);
    Stru_HashEntry_t* prev = NULL;
    Stru_HashEntry_t* entry = find_entry(table, bucket_index, key, &prev);
    
    if (entry == NULL)
    {
        return false;
    }
    
    // 从链表中移除条目
    if (prev == NULL)
    {
        // 条目是链表的第一个节点
        table->buckets[bucket_index] = entry->next;
    }
    else
    {
        prev->next = entry->next;
    }
    
    // 释放条目
    free_entry(entry);
    table->size--;
    
    return true;
}

bool F_hash_table_contains(Stru_HashTable_t* table, void* key)
{
    if (table == NULL || key == NULL)
    {
        return false;
    }
    
    size_t bucket_index = get_bucket_index(table, key);
    return find_entry(table, bucket_index, key, NULL) != NULL;
}

size_t F_hash_table_size(Stru_HashTable_t* table)
{
    if (table == NULL)
    {
        return 0;
    }
    
    return table->size;
}

size_t F_hash_table_capacity(Stru_HashTable_t* table)
{
    if (table == NULL)
    {
        return 0;
    }
    
    return table->capacity;
}

void F_hash_table_clear(Stru_HashTable_t* table)
{
    if (table == NULL)
    {
        return;
    }
    
    // 释放所有桶中的所有条目
    for (size_t i = 0; i < table->capacity; i++)
    {
        Stru_HashEntry_t* current = table->buckets[i];
        while (current != NULL)
        {
            Stru_HashEntry_t* next = current->next;
            free_entry(current);
            current = next;
        }
        table->buckets[i] = NULL;
    }
    
    table->size = 0;
}

bool F_hash_table_resize(Stru_HashTable_t* table, size_t new_capacity)
{
    if (table == NULL || new_capacity == 0)
    {
        return false;
    }
    
    // 分配新桶数组
    Stru_HashEntry_t** new_buckets = (Stru_HashEntry_t**)calloc(new_capacity, sizeof(Stru_HashEntry_t*));
    if (new_buckets == NULL)
    {
        return false;
    }
    
    // 重新哈希所有条目
    for (size_t i = 0; i < table->capacity; i++)
    {
        Stru_HashEntry_t* current = table->buckets[i];
        while (current != NULL)
        {
            Stru_HashEntry_t* next = current->next;
            
            // 计算在新桶数组中的索引
            uint64_t hash = table->hash_function(current->key);
            size_t new_index = hash % new_capacity;
            
            // 插入到新桶的头部
            current->next = new_buckets[new_index];
            new_buckets[new_index] = current;
            
            current = next;
        }
    }
    
    // 释放旧桶数组
    free(table->buckets);
    
    // 更新哈希表状态
    table->buckets = new_buckets;
    table->capacity = new_capacity;
    
    return true;
}

void F_hash_table_foreach(Stru_HashTable_t* table,
                         void (*callback)(void* key, void* value, void* user_data),
                         void* user_data)
{
    if (table == NULL || callback == NULL)
    {
        return;
    }
    
    for (size_t i = 0; i < table->capacity; i++)
    {
        Stru_HashEntry_t* current = table->buckets[i];
        while (current != NULL)
        {
            callback(current->key, current->value, user_data);
            current = current->next;
        }
    }
}

// 字符串哈希函数实现
uint64_t F_string_hash_function(void* key)
{
    if (key == NULL)
    {
        return 0;
    }
    
    char* str = (char*)key;
    uint64_t hash = 5381;
    int c;
    
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    
    return hash;
}

// 字符串比较函数实现
int F_string_compare_function(void* key1, void* key2)
{
    if (key1 == NULL && key2 == NULL)
    {
        return 0;
    }
    
    if (key1 == NULL || key2 == NULL)
    {
        return 1;
    }
    
    return strcmp((char*)key1, (char*)key2);
}

// 整数哈希函数实现
uint64_t F_int_hash_function(void* key)
{
    if (key == NULL)
    {
        return 0;
    }
    
    int* int_key = (int*)key;
    // 简单的乘法哈希
    return (uint64_t)(*int_key * 2654435761);
}

// 整数比较函数实现
int F_int_compare_function(void* key1, void* key2)
{
    if (key1 == NULL && key2 == NULL)
    {
        return 0;
    }
    
    if (key1 == NULL || key2 == NULL)
    {
        return 1;
    }
    
    int* int1 = (int*)key1;
    int* int2 = (int*)key2;
    
    if (*int1 == *int2)
    {
        return 0;
    }
    else if (*int1 < *int2)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}
