/**
 * @file CN_hash_table_interface_impl.c
 * @brief 哈希表接口实现文件
 * 
 * 实现哈希表抽象接口，将现有功能包装为接口形式。
 * 遵循CN_Language项目的接口设计规范。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_hash_table_interface.h"
#include "CN_hash_table.h"
#include <stdlib.h>

/**
 * @brief 接口实现：创建哈希表
 */
static void* interface_create(size_t key_size, size_t value_size,
                             F_HashFunction_t hash_function, F_KeyCompare_t compare_function,
                             size_t initial_capacity, enum Eum_HashTableError* error_code)
{
    if (key_size == 0 || value_size == 0 || hash_function == NULL || compare_function == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_INVALID_SIZE;
        }
        return NULL;
    }
    
    Stru_HashTable_t* table = F_create_hash_table(key_size, value_size,
                                                 (uint64_t (*)(void*))hash_function,
                                                 (int (*)(void*, void*))compare_function,
                                                 initial_capacity);
    
    if (table == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_OUT_OF_MEMORY;
        }
        return NULL;
    }
    
    if (error_code != NULL)
    {
        *error_code = Eum_HASH_TABLE_SUCCESS;
    }
    
    return (void*)table;
}

/**
 * @brief 接口实现：销毁哈希表
 */
static void interface_destroy(void* table, enum Eum_HashTableError* error_code)
{
    if (table == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return;
    }
    
    F_destroy_hash_table((Stru_HashTable_t*)table);
    
    if (error_code != NULL)
    {
        *error_code = Eum_HASH_TABLE_SUCCESS;
    }
}

/**
 * @brief 接口实现：向哈希表插入键值对
 */
static bool interface_put(void* table, void* key, void* value, enum Eum_HashTableError* error_code)
{
    if (table == NULL || key == NULL || value == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return false;
    }
    
    bool result = F_hash_table_put((Stru_HashTable_t*)table, key, value);
    
    if (error_code != NULL)
    {
        *error_code = result ? Eum_HASH_TABLE_SUCCESS : Eum_HASH_TABLE_ERROR_INTERNAL;
    }
    
    return result;
}

/**
 * @brief 接口实现：从哈希表获取值
 */
static void* interface_get(void* table, void* key, enum Eum_HashTableError* error_code)
{
    if (table == NULL || key == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return NULL;
    }
    
    void* result = F_hash_table_get((Stru_HashTable_t*)table, key);
    
    if (error_code != NULL)
    {
        *error_code = (result != NULL) ? Eum_HASH_TABLE_SUCCESS : Eum_HASH_TABLE_ERROR_KEY_NOT_FOUND;
    }
    
    return result;
}

/**
 * @brief 接口实现：从哈希表删除键值对
 */
static bool interface_remove(void* table, void* key, enum Eum_HashTableError* error_code)
{
    if (table == NULL || key == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return false;
    }
    
    bool result = F_hash_table_remove((Stru_HashTable_t*)table, key);
    
    if (error_code != NULL)
    {
        *error_code = result ? Eum_HASH_TABLE_SUCCESS : Eum_HASH_TABLE_ERROR_KEY_NOT_FOUND;
    }
    
    return result;
}

/**
 * @brief 接口实现：检查键是否存在
 */
static bool interface_contains(void* table, void* key, enum Eum_HashTableError* error_code)
{
    if (table == NULL || key == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return false;
    }
    
    bool result = F_hash_table_contains((Stru_HashTable_t*)table, key);
    
    if (error_code != NULL)
    {
        *error_code = Eum_HASH_TABLE_SUCCESS;
    }
    
    return result;
}

/**
 * @brief 接口实现：获取哈希表大小
 */
static size_t interface_size(void* table, enum Eum_HashTableError* error_code)
{
    if (table == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return 0;
    }
    
    size_t result = F_hash_table_size((Stru_HashTable_t*)table);
    
    if (error_code != NULL)
    {
        *error_code = Eum_HASH_TABLE_SUCCESS;
    }
    
    return result;
}

/**
 * @brief 接口实现：获取哈希表容量
 */
static size_t interface_capacity(void* table, enum Eum_HashTableError* error_code)
{
    if (table == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return 0;
    }
    
    size_t result = F_hash_table_capacity((Stru_HashTable_t*)table);
    
    if (error_code != NULL)
    {
        *error_code = Eum_HASH_TABLE_SUCCESS;
    }
    
    return result;
}

/**
 * @brief 接口实现：获取负载因子
 */
static double interface_load_factor(void* table, enum Eum_HashTableError* error_code)
{
    if (table == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return 0.0;
    }
    
    Stru_HashTable_t* hash_table = (Stru_HashTable_t*)table;
    size_t size = F_hash_table_size(hash_table);
    size_t capacity = F_hash_table_capacity(hash_table);
    
    if (capacity == 0)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_INTERNAL;
        }
        return 0.0;
    }
    
    if (error_code != NULL)
    {
        *error_code = Eum_HASH_TABLE_SUCCESS;
    }
    
    return (double)size / (double)capacity;
}

/**
 * @brief 接口实现：检查哈希表是否为空
 */
static bool interface_is_empty(void* table, enum Eum_HashTableError* error_code)
{
    if (table == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return true;
    }
    
    bool result = (F_hash_table_size((Stru_HashTable_t*)table) == 0);
    
    if (error_code != NULL)
    {
        *error_code = Eum_HASH_TABLE_SUCCESS;
    }
    
    return result;
}

/**
 * @brief 接口实现：清空哈希表
 */
static bool interface_clear(void* table, enum Eum_HashTableError* error_code)
{
    if (table == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return false;
    }
    
    F_hash_table_clear((Stru_HashTable_t*)table);
    
    if (error_code != NULL)
    {
        *error_code = Eum_HASH_TABLE_SUCCESS;
    }
    
    return true;
}

/**
 * @brief 接口实现：调整哈希表容量
 */
static bool interface_resize(void* table, size_t new_capacity, enum Eum_HashTableError* error_code)
{
    if (table == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return false;
    }
    
    bool result = F_hash_table_resize((Stru_HashTable_t*)table, new_capacity);
    
    if (error_code != NULL)
    {
        *error_code = result ? Eum_HASH_TABLE_SUCCESS : Eum_HASH_TABLE_ERROR_INTERNAL;
    }
    
    return result;
}

/**
 * @brief 迭代器包装函数
 */
static bool foreach_wrapper(void* key, void* value, void* user_data)
{
    F_HashTableIterator_t iterator = (F_HashTableIterator_t)user_data;
    return iterator(key, value, NULL);
}

/**
 * @brief 接口实现：遍历哈希表
 */
static bool interface_foreach(void* table, F_HashTableIterator_t iterator, void* user_data,
                             enum Eum_HashTableError* error_code)
{
    if (table == NULL || iterator == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return false;
    }
    
    // 使用现有的foreach函数，但需要适配回调函数签名
    F_hash_table_foreach((Stru_HashTable_t*)table, 
                        (void (*)(void*, void*, void*))foreach_wrapper, 
                        (void*)iterator);
    
    if (error_code != NULL)
    {
        *error_code = Eum_HASH_TABLE_SUCCESS;
    }
    
    return true;
}

/**
 * @brief 接口实现：获取内存使用统计
 */
static bool interface_get_memory_stats(void* table, size_t* total_bytes, size_t* used_bytes,
                                      enum Eum_HashTableError* error_code)
{
    if (table == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return false;
    }
    
    Stru_HashTable_t* hash_table = (Stru_HashTable_t*)table;
    
    // 计算总内存使用
    size_t total = sizeof(Stru_HashTable_t);
    total += hash_table->capacity * sizeof(Stru_HashEntry_t*);
    
    // 计算已使用内存
    size_t used = 0;
    for (size_t i = 0; i < hash_table->capacity; i++)
    {
        Stru_HashEntry_t* entry = hash_table->buckets[i];
        while (entry != NULL)
        {
            used += sizeof(Stru_HashEntry_t);
            used += hash_table->key_size;
            used += hash_table->value_size;
            entry = entry->next;
        }
    }
    
    if (total_bytes != NULL)
    {
        *total_bytes = total;
    }
    
    if (used_bytes != NULL)
    {
        *used_bytes = used;
    }
    
    if (error_code != NULL)
    {
        *error_code = Eum_HASH_TABLE_SUCCESS;
    }
    
    return true;
}

/**
 * @brief 接口实现：压缩哈希表
 */
static bool interface_shrink_to_fit(void* table, enum Eum_HashTableError* error_code)
{
    if (table == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return false;
    }
    
    Stru_HashTable_t* hash_table = (Stru_HashTable_t*)table;
    size_t size = F_hash_table_size(hash_table);
    
    // 计算合适的新容量（至少为size，且为2的幂）
    size_t new_capacity = 1;
    while (new_capacity < size)
    {
        new_capacity <<= 1;
    }
    
    if (new_capacity < 16)
    {
        new_capacity = 16;
    }
    
    bool result = F_hash_table_resize(hash_table, new_capacity);
    
    if (error_code != NULL)
    {
        *error_code = result ? Eum_HASH_TABLE_SUCCESS : Eum_HASH_TABLE_ERROR_INTERNAL;
    }
    
    return result;
}

/**
 * @brief 接口实现：获取性能统计
 */
static bool interface_get_performance_stats(void* table, size_t* collision_count,
                                           size_t* max_chain_length, double* avg_chain_length,
                                           enum Eum_HashTableError* error_code)
{
    if (table == NULL)
    {
        if (error_code != NULL)
        {
            *error_code = Eum_HASH_TABLE_ERROR_NULL_POINTER;
        }
        return false;
    }
    
    Stru_HashTable_t* hash_table = (Stru_HashTable_t*)table;
    size_t collisions = 0;
    size_t max_length = 0;
    size_t total_entries = 0;
    size_t non_empty_buckets = 0;
    
    for (size_t i = 0; i < hash_table->capacity; i++)
    {
        size_t chain_length = 0;
        Stru_HashEntry_t* entry = hash_table->buckets[i];
        
        while (entry != NULL)
        {
            chain_length++;
            entry = entry->next;
        }
        
        if (chain_length > 0)
        {
            non_empty_buckets++;
            total_entries += chain_length;
            
            if (chain_length > max_length)
            {
                max_length = chain_length;
            }
            
            if (chain_length > 1)
            {
                collisions += (chain_length - 1);
            }
        }
    }
    
    if (collision_count != NULL)
    {
        *collision_count = collisions;
    }
    
    if (max_chain_length != NULL)
    {
        *max_chain_length = max_length;
    }
    
    if (avg_chain_length != NULL)
    {
        *avg_chain_length = (non_empty_buckets > 0) ? 
                           (double)total_entries / (double)non_empty_buckets : 0.0;
    }
    
    if (error_code != NULL)
    {
        *error_code = Eum_HASH_TABLE_SUCCESS;
    }
    
    return true;
}

/**
 * @brief 默认哈希表接口实例
 */
static const Stru_HashTableInterface_t g_default_hash_table_interface = {
    .create = interface_create,
    .destroy = interface_destroy,
    .put = interface_put,
    .get = interface_get,
    .remove = interface_remove,
    .contains = interface_contains,
    .size = interface_size,
    .capacity = interface_capacity,
    .load_factor = interface_load_factor,
    .is_empty = interface_is_empty,
    .clear = interface_clear,
    .resize = interface_resize,
    .foreach = interface_foreach,
    .get_memory_stats = interface_get_memory_stats,
    .shrink_to_fit = interface_shrink_to_fit,
    .get_performance_stats = interface_get_performance_stats
};

/**
 * @brief 获取默认哈希表实现接口
 */
const Stru_HashTableInterface_t* F_get_default_hash_table_interface(void)
{
    return &g_default_hash_table_interface;
}
