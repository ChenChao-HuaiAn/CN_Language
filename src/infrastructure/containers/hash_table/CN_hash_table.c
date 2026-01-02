/******************************************************************************
 * 文件名: CN_hash_table.c
 * 功能: CN_Language哈希表容器主实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现哈希表基本功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_hash_table_internal.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 计算下一个质数（用于哈希表容量）
 * 
 * @param n 起始数字
 * @return 下一个质数
 */
size_t next_prime(size_t n)
{
    if (n <= 2) return 2;
    if (n % 2 == 0) n++;
    
    while (true)
    {
        bool is_prime = true;
        size_t limit = (size_t)sqrt((double)n);
        
        for (size_t i = 3; i <= limit; i += 2)
        {
            if (n % i == 0)
            {
                is_prime = false;
                break;
            }
        }
        
        if (is_prime) return n;
        n += 2;
    }
}

/**
 * @brief 分配并初始化键内存
 * 
 * @param table 哈希表
 * @param key 源键
 * @return 新分配的键，失败返回NULL
 */
void* allocate_key(Stru_CN_HashTable_t* table, const void* key)
{
    if (table->key_copy_func)
    {
        return table->key_copy_func(key);
    }
    else
    {
        void* new_key = malloc(table->key_size);
        if (new_key)
        {
            memcpy(new_key, key, table->key_size);
        }
        return new_key;
    }
}

/**
 * @brief 分配并初始化值内存
 * 
 * @param table 哈希表
 * @param value 源值
 * @return 新分配的值，失败返回NULL
 */
void* allocate_value(Stru_CN_HashTable_t* table, const void* value)
{
    if (table->value_copy_func)
    {
        return table->value_copy_func(value);
    }
    else
    {
        void* new_value = malloc(table->value_size);
        if (new_value)
        {
            memcpy(new_value, value, table->value_size);
        }
        return new_value;
    }
}

/**
 * @brief 释放键内存
 * 
 * @param table 哈希表
 * @param key 要释放的键
 */
void free_key(Stru_CN_HashTable_t* table, void* key)
{
    if (table->key_free_func)
    {
        table->key_free_func(key);
    }
    else
    {
        free(key);
    }
}

/**
 * @brief 释放值内存
 * 
 * @param table 哈希表
 * @param value 要释放的值
 */
void free_value(Stru_CN_HashTable_t* table, void* value)
{
    if (table->value_free_func)
    {
        table->value_free_func(value);
    }
    else
    {
        free(value);
    }
}

// ============================================================================
// 默认哈希和比较函数
// ============================================================================

size_t CN_hash_table_default_hash(const void* key, size_t key_size, size_t table_size)
{
    // 简单的djb2哈希算法
    const unsigned char* str = (const unsigned char*)key;
    size_t hash = 5381;
    
    for (size_t i = 0; i < key_size; i++)
    {
        hash = ((hash << 5) + hash) + str[i]; // hash * 33 + c
    }
    
    return hash % table_size;
}

bool CN_hash_table_default_key_compare(const void* key1, const void* key2, size_t key_size)
{
    return memcmp(key1, key2, key_size) == 0;
}

size_t CN_hash_table_default_string_hash(const void* key, size_t table_size)
{
    const char* str = (const char*)key;
    size_t hash = 5381;
    int c;
    
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    
    return hash % table_size;
}

size_t CN_hash_table_default_int_hash(const void* key, size_t table_size)
{
    // 简单的乘法哈希
    size_t value = *(size_t*)key;
    return (value * 2654435761UL) % table_size;
}

size_t CN_hash_table_default_ptr_hash(const void* key, size_t table_size)
{
    // 指针地址哈希
    size_t ptr_value = (size_t)key;
    return ptr_value % table_size;
}

// ============================================================================
// 哈希表创建和销毁
// ============================================================================

Stru_CN_HashTable_t* CN_hash_table_create(size_t key_size, size_t value_size,
                                          size_t initial_capacity,
                                          Eum_CN_HashTableCollisionStrategy_t strategy)
{
    return CN_hash_table_create_custom(key_size, value_size, initial_capacity,
                                       strategy, NULL, NULL, NULL, NULL, NULL, NULL);
}

Stru_CN_HashTable_t* CN_hash_table_create_custom(
    size_t key_size, size_t value_size,
    size_t initial_capacity,
    Eum_CN_HashTableCollisionStrategy_t strategy,
    CN_HashFunc hash_func,
    CN_KeyCompareFunc key_compare_func,
    CN_KeyFreeFunc key_free_func,
    CN_ValueFreeFunc value_free_func,
    CN_KeyCopyFunc key_copy_func,
    CN_ValueCopyFunc value_copy_func)
{
    // 验证参数
    if (key_size == 0 || value_size == 0 || initial_capacity == 0)
    {
        return NULL;
    }
    
    // 分配哈希表结构体
    Stru_CN_HashTable_t* table = (Stru_CN_HashTable_t*)malloc(sizeof(Stru_CN_HashTable_t));
    if (!table)
    {
        return NULL;
    }
    
    // 初始化基本属性
    table->key_size = key_size;
    table->value_size = value_size;
    table->size = 0;
    table->capacity = next_prime(initial_capacity);
    table->strategy = strategy;
    
    // 设置函数指针
    table->hash_func = hash_func;
    table->key_compare_func = key_compare_func;
    table->key_free_func = key_free_func;
    table->value_free_func = value_free_func;
    table->key_copy_func = key_copy_func;
    table->value_copy_func = value_copy_func;
    
    // 初始化统计信息
    table->collisions = 0;
    table->total_probes = 0;
    table->resize_count = 0;
    
    // 根据策略分配数据存储
    if (strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        table->data.entries = (Stru_CN_HashEntry_t*)calloc(table->capacity, sizeof(Stru_CN_HashEntry_t));
        if (!table->data.entries)
        {
            free(table);
            return NULL;
        }
        
        // 初始化所有条目为空
        for (size_t i = 0; i < table->capacity; i++)
        {
            table->data.entries[i].status = Eum_HASH_ENTRY_EMPTY;
            table->data.entries[i].key = NULL;
            table->data.entries[i].value = NULL;
        }
    }
    else // Eum_HASH_TABLE_SEPARATE_CHAINING
    {
        table->data.chains = (Stru_CN_HashChainNode_t**)calloc(table->capacity, sizeof(Stru_CN_HashChainNode_t*));
        if (!table->data.chains)
        {
            free(table);
            return NULL;
        }
        
        // 所有链初始化为NULL
        for (size_t i = 0; i < table->capacity; i++)
        {
            table->data.chains[i] = NULL;
        }
    }
    
    return table;
}

void CN_hash_table_destroy(Stru_CN_HashTable_t* table)
{
    if (!table) return;
    
    CN_hash_table_clear(table);
    
    if (table->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        free(table->data.entries);
    }
    else
    {
        free(table->data.chains);
    }
    
    free(table);
}

void CN_hash_table_clear(Stru_CN_HashTable_t* table)
{
    if (!table) return;
    
    if (table->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        for (size_t i = 0; i < table->capacity; i++)
        {
            Stru_CN_HashEntry_t* entry = &table->data.entries[i];
            if (entry->status == Eum_HASH_ENTRY_OCCUPIED)
            {
                free_key(table, entry->key);
                free_value(table, entry->value);
                entry->status = Eum_HASH_ENTRY_EMPTY;
                entry->key = NULL;
                entry->value = NULL;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < table->capacity; i++)
        {
            Stru_CN_HashChainNode_t* node = table->data.chains[i];
            while (node)
            {
                Stru_CN_HashChainNode_t* next = node->next;
                free_key(table, node->key);
                free_value(table, node->value);
                free(node);
                node = next;
            }
            table->data.chains[i] = NULL;
        }
    }
    
    table->size = 0;
    table->collisions = 0;
    table->total_probes = 0;
}

// ============================================================================
// 哈希表属性查询
// ============================================================================

size_t CN_hash_table_size(const Stru_CN_HashTable_t* table)
{
    return table ? table->size : 0;
}

size_t CN_hash_table_capacity(const Stru_CN_HashTable_t* table)
{
    return table ? table->capacity : 0;
}

bool CN_hash_table_is_empty(const Stru_CN_HashTable_t* table)
{
    return table ? (table->size == 0) : true;
}

double CN_hash_table_load_factor(const Stru_CN_HashTable_t* table)
{
    if (!table || table->capacity == 0) return 0.0;
    return (double)table->size / (double)table->capacity;
}

Eum_CN_HashTableCollisionStrategy_t CN_hash_table_strategy(const Stru_CN_HashTable_t* table)
{
    return table ? table->strategy : Eum_HASH_TABLE_OPEN_ADDRESSING;
}

size_t CN_hash_table_key_size(const Stru_CN_HashTable_t* table)
{
    return table ? table->key_size : 0;
}

size_t CN_hash_table_value_size(const Stru_CN_HashTable_t* table)
{
    return table ? table->value_size : 0;
}

// ============================================================================
// 条目操作（继续在下一个文件中实现）
// ============================================================================
