/******************************************************************************
 * 文件名: CN_hash_table_operations.c
 * 功能: CN_Language哈希表容器操作实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现哈希表操作功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_hash_table_internal.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 内部辅助函数实现
// ============================================================================

Stru_CN_HashEntry_t* CN_hash_table_create_entry(Stru_CN_HashTable_t* table,
                                                const void* key, const void* value)
{
    Stru_CN_HashEntry_t* entry = (Stru_CN_HashEntry_t*)malloc(sizeof(Stru_CN_HashEntry_t));
    if (!entry) return NULL;
    
    entry->key = allocate_key(table, key);
    if (!entry->key)
    {
        free(entry);
        return NULL;
    }
    
    entry->value = allocate_value(table, value);
    if (!entry->value)
    {
        free_key(table, entry->key);
        free(entry);
        return NULL;
    }
    
    entry->status = Eum_HASH_ENTRY_OCCUPIED;
    return entry;
}

void CN_hash_table_destroy_entry(Stru_CN_HashTable_t* table, Stru_CN_HashEntry_t* entry)
{
    if (!entry) return;
    
    if (entry->key) free_key(table, entry->key);
    if (entry->value) free_value(table, entry->value);
    free(entry);
}

Stru_CN_HashChainNode_t* CN_hash_table_create_chain_node(Stru_CN_HashTable_t* table,
                                                         const void* key, const void* value)
{
    Stru_CN_HashChainNode_t* node = (Stru_CN_HashChainNode_t*)malloc(sizeof(Stru_CN_HashChainNode_t));
    if (!node) return NULL;
    
    node->key = allocate_key(table, key);
    if (!node->key)
    {
        free(node);
        return NULL;
    }
    
    node->value = allocate_value(table, value);
    if (!node->value)
    {
        free_key(table, node->key);
        free(node);
        return NULL;
    }
    
    node->next = NULL;
    return node;
}

void CN_hash_table_destroy_chain_node(Stru_CN_HashTable_t* table, Stru_CN_HashChainNode_t* node)
{
    if (!node) return;
    
    if (node->key) free_key(table, node->key);
    if (node->value) free_value(table, node->value);
    free(node);
}

size_t CN_hash_table_next_probe(const Stru_CN_HashTable_t* table,
                                size_t start_index, size_t probe_count)
{
    // 线性探测：h(k, i) = (h1(k) + i) % m
    return (start_index + probe_count) % table->capacity;
}

// ============================================================================
// 哈希计算和比较
// ============================================================================

/**
 * @brief 计算键的哈希值
 * 
 * @param table 哈希表
 * @param key 键指针
 * @return 哈希值
 */
size_t compute_hash(const Stru_CN_HashTable_t* table, const void* key)
{
    if (table->hash_func)
    {
        return table->hash_func(key, table->capacity);
    }
    else
    {
        return CN_hash_table_default_hash(key, table->key_size, table->capacity);
    }
}

/**
 * @brief 比较两个键是否相等
 * 
 * @param table 哈希表
 * @param key1 第一个键
 * @param key2 第二个键
 * @return 如果相等返回true，否则返回false
 */
bool compare_keys(const Stru_CN_HashTable_t* table, const void* key1, const void* key2)
{
    if (table->key_compare_func)
    {
        return table->key_compare_func(key1, key2);
    }
    else
    {
        return CN_hash_table_default_key_compare(key1, key2, table->key_size);
    }
}

// ============================================================================
// 查找函数
// ============================================================================

Stru_CN_HashEntry_t* CN_hash_table_find_entry(const Stru_CN_HashTable_t* table,
                                              const void* key, size_t* index)
{
    if (!table || !key) return NULL;
    
    size_t hash = compute_hash(table, key);
    size_t probe_count = 0;
    size_t current_index = hash;
    
    while (probe_count < table->capacity)
    {
        Stru_CN_HashEntry_t* entry = &table->data.entries[current_index];
        
        if (entry->status == Eum_HASH_ENTRY_EMPTY)
        {
            // 找到空位置，键不存在
            if (index) *index = current_index;
            return NULL;
        }
        else if (entry->status == Eum_HASH_ENTRY_OCCUPIED && 
                 compare_keys(table, entry->key, key))
        {
            // 找到键
            if (index) *index = current_index;
            return entry;
        }
        
        // 继续探测
        probe_count++;
        current_index = CN_hash_table_next_probe(table, hash, probe_count);
        
        // 统计探测次数
        if (probe_count > 0)
        {
            ((Stru_CN_HashTable_t*)table)->total_probes++;
            if (probe_count > 1)
            {
                ((Stru_CN_HashTable_t*)table)->collisions++;
            }
        }
    }
    
    // 表已满，未找到
    if (index) *index = SIZE_MAX;
    return NULL;
}

Stru_CN_HashChainNode_t* CN_hash_table_find_chain_node(const Stru_CN_HashTable_t* table,
                                                       const void* key,
                                                       size_t* bucket_index,
                                                       Stru_CN_HashChainNode_t** prev_node)
{
    if (!table || !key) return NULL;
    
    size_t hash = compute_hash(table, key);
    if (bucket_index) *bucket_index = hash;
    
    Stru_CN_HashChainNode_t* node = table->data.chains[hash];
    Stru_CN_HashChainNode_t* prev = NULL;
    
    while (node)
    {
        if (compare_keys(table, node->key, key))
        {
            if (prev_node) *prev_node = prev;
            return node;
        }
        prev = node;
        node = node->next;
        
        // 统计探测次数
        if (prev)
        {
            ((Stru_CN_HashTable_t*)table)->total_probes++;
            ((Stru_CN_HashTable_t*)table)->collisions++;
        }
    }
    
    if (prev_node) *prev_node = NULL;
    return NULL;
}

// ============================================================================
// 条目操作实现
// ============================================================================

bool CN_hash_table_put(Stru_CN_HashTable_t* table, const void* key, const void* value)
{
    if (!table || !key || !value) return false;
    
    // 检查是否需要自动调整大小
    CN_hash_table_auto_resize(table, 0.75);
    
    if (table->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        size_t index;
        Stru_CN_HashEntry_t* entry = CN_hash_table_find_entry(table, key, &index);
        
        if (entry)
        {
            // 键已存在，更新值
            free_value(table, entry->value);
            entry->value = allocate_value(table, value);
            return entry->value != NULL;
        }
        else if (index != SIZE_MAX)
        {
            // 键不存在，插入新条目
            Stru_CN_HashEntry_t* new_entry = CN_hash_table_create_entry(table, key, value);
            if (!new_entry) return false;
            
            // 复制到条目数组
            table->data.entries[index] = *new_entry;
            free(new_entry); // 只复制内容，不保留结构体
            table->size++;
            return true;
        }
    }
    else // Eum_HASH_TABLE_SEPARATE_CHAINING
    {
        size_t bucket_index;
        Stru_CN_HashChainNode_t* prev_node;
        Stru_CN_HashChainNode_t* node = CN_hash_table_find_chain_node(table, key, &bucket_index, &prev_node);
        
        if (node)
        {
            // 键已存在，更新值
            free_value(table, node->value);
            node->value = allocate_value(table, value);
            return node->value != NULL;
        }
        else
        {
            // 键不存在，创建新节点
            Stru_CN_HashChainNode_t* new_node = CN_hash_table_create_chain_node(table, key, value);
            if (!new_node) return false;
            
            // 插入到链表头部
            new_node->next = table->data.chains[bucket_index];
            table->data.chains[bucket_index] = new_node;
            table->size++;
            return true;
        }
    }
    
    return false;
}

void* CN_hash_table_get(const Stru_CN_HashTable_t* table, const void* key)
{
    if (!table || !key) return NULL;
    
    if (table->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        Stru_CN_HashEntry_t* entry = CN_hash_table_find_entry(table, key, NULL);
        return entry ? entry->value : NULL;
    }
    else
    {
        Stru_CN_HashChainNode_t* node = CN_hash_table_find_chain_node(table, key, NULL, NULL);
        return node ? node->value : NULL;
    }
}

bool CN_hash_table_contains(const Stru_CN_HashTable_t* table, const void* key)
{
    if (!table || !key) return false;
    
    if (table->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        return CN_hash_table_find_entry(table, key, NULL) != NULL;
    }
    else
    {
        return CN_hash_table_find_chain_node(table, key, NULL, NULL) != NULL;
    }
}

bool CN_hash_table_remove(Stru_CN_HashTable_t* table, const void* key)
{
    if (!table || !key) return false;
    
    if (table->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        size_t index;
        Stru_CN_HashEntry_t* entry = CN_hash_table_find_entry(table, key, &index);
        
        if (entry)
        {
            // 标记为已删除（惰性删除）
            free_key(table, entry->key);
            free_value(table, entry->value);
            entry->status = Eum_HASH_ENTRY_DELETED;
            entry->key = NULL;
            entry->value = NULL;
            table->size--;
            return true;
        }
    }
    else
    {
        size_t bucket_index;
        Stru_CN_HashChainNode_t* prev_node;
        Stru_CN_HashChainNode_t* node = CN_hash_table_find_chain_node(table, key, &bucket_index, &prev_node);
        
        if (node)
        {
            if (prev_node)
            {
                prev_node->next = node->next;
            }
            else
            {
                table->data.chains[bucket_index] = node->next;
            }
            
            CN_hash_table_destroy_chain_node(table, node);
            table->size--;
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// 键和值集合获取
// ============================================================================

size_t CN_hash_table_keys(const Stru_CN_HashTable_t* table, void*** keys)
{
    if (!table || !keys) return 0;
    
    void** key_array = (void**)malloc(table->size * sizeof(void*));
    if (!key_array) return 0;
    
    size_t count = 0;
    
    if (table->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        for (size_t i = 0; i < table->capacity && count < table->size; i++)
        {
            Stru_CN_HashEntry_t* entry = &table->data.entries[i];
            if (entry->status == Eum_HASH_ENTRY_OCCUPIED)
            {
                key_array[count] = allocate_key((Stru_CN_HashTable_t*)table, entry->key);
                if (!key_array[count])
                {
                    // 清理已分配的内存
                    for (size_t j = 0; j < count; j++)
                    {
                        free_key((Stru_CN_HashTable_t*)table, key_array[j]);
                    }
                    free(key_array);
                    return 0;
                }
                count++;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < table->capacity && count < table->size; i++)
        {
            Stru_CN_HashChainNode_t* node = table->data.chains[i];
            while (node)
            {
                key_array[count] = allocate_key((Stru_CN_HashTable_t*)table, node->key);
                if (!key_array[count])
                {
                    // 清理已分配的内存
                    for (size_t j = 0; j < count; j++)
                    {
                        free_key((Stru_CN_HashTable_t*)table, key_array[j]);
                    }
                    free(key_array);
                    return 0;
                }
                count++;
                node = node->next;
            }
        }
    }
    
    *keys = key_array;
    return count;
}

size_t CN_hash_table_values(const Stru_CN_HashTable_t* table, void*** values)
{
    if (!table || !values) return 0;
    
    void** value_array = (void**)malloc(table->size * sizeof(void*));
    if (!value_array) return 0;
    
    size_t count = 0;
    
    if (table->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        for (size_t i = 0; i < table->capacity && count < table->size; i++)
        {
            Stru_CN_HashEntry_t* entry = &table->data.entries[i];
            if (entry->status == Eum_HASH_ENTRY_OCCUPIED)
            {
                value_array[count] = allocate_value((Stru_CN_HashTable_t*)table, entry->value);
                if (!value_array[count])
                {
                    // 清理已分配的内存
                    for (size_t j = 0; j < count; j++)
                    {
                        free_value((Stru_CN_HashTable_t*)table, value_array[j]);
                    }
                    free(value_array);
                    return 0;
                }
                count++;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < table->capacity && count < table->size; i++)
        {
            Stru_CN_HashChainNode_t* node = table->data.chains[i];
            while (node)
            {
                value_array[count] = allocate_value((Stru_CN_HashTable_t*)table, node->value);
                if (!value_array[count])
                {
                    // 清理已分配的内存
                    for (size_t j = 0; j < count; j++)
                    {
                        free_value((Stru_CN_HashTable_t*)table, value_array[j]);
                    }
                    free(value_array);
                    return 0;
                }
                count++;
                node = node->next;
            }
        }
    }
    
    *values = value_array;
    return count;
}
