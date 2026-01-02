/******************************************************************************
 * 文件名: CN_hash_table_utils.c
 * 功能: CN_Language哈希表容器工具函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现哈希表工具函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_hash_table_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 调整大小和重新哈希
// ============================================================================

bool CN_hash_table_needs_resize(const Stru_CN_HashTable_t* table,
                                double load_factor_threshold)
{
    if (!table) return false;
    
    double current_load_factor = CN_hash_table_load_factor(table);
    return current_load_factor > load_factor_threshold;
}

bool CN_hash_table_rehash(Stru_CN_HashTable_t* table, size_t new_capacity)
{
    if (!table || new_capacity <= table->capacity) return false;
    
    // 保存旧数据
    size_t old_capacity = table->capacity;
    Eum_CN_HashTableCollisionStrategy_t old_strategy = table->strategy;
    
    if (old_strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        Stru_CN_HashEntry_t* old_entries = table->data.entries;
        
        // 分配新条目数组
        Stru_CN_HashEntry_t* new_entries = (Stru_CN_HashEntry_t*)calloc(new_capacity, sizeof(Stru_CN_HashEntry_t));
        if (!new_entries) return false;
        
        // 初始化新条目
        for (size_t i = 0; i < new_capacity; i++)
        {
            new_entries[i].status = Eum_HASH_ENTRY_EMPTY;
            new_entries[i].key = NULL;
            new_entries[i].value = NULL;
        }
        
        // 更新表属性
        table->capacity = new_capacity;
        table->data.entries = new_entries;
        table->size = 0; // 将在重新插入时重新计算
        
        // 重新插入所有旧条目（直接插入，避免递归调用put）
        for (size_t i = 0; i < old_capacity; i++)
        {
            Stru_CN_HashEntry_t* old_entry = &old_entries[i];
            if (old_entry->status == Eum_HASH_ENTRY_OCCUPIED)
            {
                // 计算新哈希值
                size_t hash = compute_hash(table, old_entry->key);
                size_t probe_count = 0;
                size_t current_index = hash;
                
                // 找到空位置
                while (probe_count < table->capacity)
                {
                    Stru_CN_HashEntry_t* new_entry = &new_entries[current_index];
                    
                    if (new_entry->status == Eum_HASH_ENTRY_EMPTY || 
                        new_entry->status == Eum_HASH_ENTRY_DELETED)
                    {
                        // 复制键值
                        new_entry->key = allocate_key(table, old_entry->key);
                        new_entry->value = allocate_value(table, old_entry->value);
                        new_entry->status = Eum_HASH_ENTRY_OCCUPIED;
                        table->size++;
                        break;
                    }
                    
                    // 继续探测
                    probe_count++;
                    current_index = CN_hash_table_next_probe(table, hash, probe_count);
                }
                
                // 释放旧键值
                free_key(table, old_entry->key);
                free_value(table, old_entry->value);
            }
        }
        
        free(old_entries);
    }
    else // Eum_HASH_TABLE_SEPARATE_CHAINING
    {
        Stru_CN_HashChainNode_t** old_chains = table->data.chains;
        
        // 分配新链数组
        Stru_CN_HashChainNode_t** new_chains = (Stru_CN_HashChainNode_t**)calloc(new_capacity, sizeof(Stru_CN_HashChainNode_t*));
        if (!new_chains) return false;
        
        // 更新表属性
        table->capacity = new_capacity;
        table->data.chains = new_chains;
        table->size = 0; // 将在重新插入时重新计算
        
        // 重新插入所有旧节点
        for (size_t i = 0; i < old_capacity; i++)
        {
            Stru_CN_HashChainNode_t* node = old_chains[i];
            while (node)
            {
                Stru_CN_HashChainNode_t* next = node->next;
                
                // 重新插入节点
                size_t new_hash = compute_hash(table, node->key);
                node->next = new_chains[new_hash];
                new_chains[new_hash] = node;
                table->size++;
                
                node = next;
            }
        }
        
        free(old_chains);
    }
    
    table->resize_count++;
    return true;
}

bool CN_hash_table_resize(Stru_CN_HashTable_t* table, size_t new_capacity)
{
    if (!table) return false;
    
    // 确保新容量是质数
    size_t actual_new_capacity = next_prime(new_capacity);
    if (actual_new_capacity <= table->capacity) return false;
    
    return CN_hash_table_rehash(table, actual_new_capacity);
}

bool CN_hash_table_auto_resize(Stru_CN_HashTable_t* table, double load_factor_threshold)
{
    if (!table) return false;
    
    if (CN_hash_table_needs_resize(table, load_factor_threshold))
    {
        // 通常扩容为当前容量的2倍
        size_t new_capacity = table->capacity * 2;
        return CN_hash_table_resize(table, new_capacity);
    }
    
    return false;
}

// ============================================================================
// 复制和合并
// ============================================================================

Stru_CN_HashTable_t* CN_hash_table_copy(const Stru_CN_HashTable_t* src)
{
    if (!src) return NULL;
    
    // 创建新表
    Stru_CN_HashTable_t* dst = CN_hash_table_create_custom(
        src->key_size, src->value_size,
        src->capacity,
        src->strategy,
        src->hash_func,
        src->key_compare_func,
        src->key_free_func,
        src->value_free_func,
        src->key_copy_func,
        src->value_copy_func);
    
    if (!dst) return NULL;
    
    // 复制所有条目
    if (src->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        for (size_t i = 0; i < src->capacity; i++)
        {
            Stru_CN_HashEntry_t* src_entry = &src->data.entries[i];
            if (src_entry->status == Eum_HASH_ENTRY_OCCUPIED)
            {
                if (!CN_hash_table_put(dst, src_entry->key, src_entry->value))
                {
                    CN_hash_table_destroy(dst);
                    return NULL;
                }
            }
        }
    }
    else
    {
        for (size_t i = 0; i < src->capacity; i++)
        {
            Stru_CN_HashChainNode_t* node = src->data.chains[i];
            while (node)
            {
                if (!CN_hash_table_put(dst, node->key, node->value))
                {
                    CN_hash_table_destroy(dst);
                    return NULL;
                }
                node = node->next;
            }
        }
    }
    
    return dst;
}

Stru_CN_HashTable_t* CN_hash_table_merge(const Stru_CN_HashTable_t* table1,
                                         const Stru_CN_HashTable_t* table2,
                                         bool overwrite)
{
    if (!table1 || !table2) return NULL;
    
    // 检查表是否兼容
    if (table1->key_size != table2->key_size ||
        table1->value_size != table2->value_size ||
        table1->strategy != table2->strategy)
    {
        return NULL;
    }
    
    // 创建合并表
    size_t merged_capacity = table1->capacity + table2->capacity;
    Stru_CN_HashTable_t* merged = CN_hash_table_create_custom(
        table1->key_size, table1->value_size,
        merged_capacity,
        table1->strategy,
        table1->hash_func,
        table1->key_compare_func,
        table1->key_free_func,
        table1->value_free_func,
        table1->key_copy_func,
        table1->value_copy_func);
    
    if (!merged) return NULL;
    
    // 复制第一个表的所有条目
    if (table1->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        for (size_t i = 0; i < table1->capacity; i++)
        {
            Stru_CN_HashEntry_t* entry = &table1->data.entries[i];
            if (entry->status == Eum_HASH_ENTRY_OCCUPIED)
            {
                if (!CN_hash_table_put(merged, entry->key, entry->value))
                {
                    CN_hash_table_destroy(merged);
                    return NULL;
                }
            }
        }
    }
    else
    {
        for (size_t i = 0; i < table1->capacity; i++)
        {
            Stru_CN_HashChainNode_t* node = table1->data.chains[i];
            while (node)
            {
                if (!CN_hash_table_put(merged, node->key, node->value))
                {
                    CN_hash_table_destroy(merged);
                    return NULL;
                }
                node = node->next;
            }
        }
    }
    
    // 复制第二个表的所有条目
    if (table2->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        for (size_t i = 0; i < table2->capacity; i++)
        {
            Stru_CN_HashEntry_t* entry = &table2->data.entries[i];
            if (entry->status == Eum_HASH_ENTRY_OCCUPIED)
            {
                if (overwrite || !CN_hash_table_contains(merged, entry->key))
                {
                    if (!CN_hash_table_put(merged, entry->key, entry->value))
                    {
                        CN_hash_table_destroy(merged);
                        return NULL;
                    }
                }
            }
        }
    }
    else
    {
        for (size_t i = 0; i < table2->capacity; i++)
        {
            Stru_CN_HashChainNode_t* node = table2->data.chains[i];
            while (node)
            {
                if (overwrite || !CN_hash_table_contains(merged, node->key))
                {
                    if (!CN_hash_table_put(merged, node->key, node->value))
                    {
                        CN_hash_table_destroy(merged);
                        return NULL;
                    }
                }
                node = node->next;
            }
        }
    }
    
    return merged;
}

// ============================================================================
// 迭代器实现
// ============================================================================

Stru_CN_HashTableIterator_t* CN_hash_table_iterator_create(Stru_CN_HashTable_t* table)
{
    if (!table) return NULL;
    
    Stru_CN_HashTableIterator_t* iterator = (Stru_CN_HashTableIterator_t*)malloc(sizeof(Stru_CN_HashTableIterator_t));
    if (!iterator) return NULL;
    
    iterator->table = table;
    CN_hash_table_iterator_reset(iterator);
    
    return iterator;
}

void CN_hash_table_iterator_destroy(Stru_CN_HashTableIterator_t* iterator)
{
    if (iterator) free(iterator);
}

void CN_hash_table_iterator_reset(Stru_CN_HashTableIterator_t* iterator)
{
    if (!iterator || !iterator->table) return;
    
    iterator->current_bucket = 0;
    iterator->current_entry = 0;
    iterator->current_key = NULL;
    iterator->current_value = NULL;
}

bool CN_hash_table_iterator_has_next(const Stru_CN_HashTableIterator_t* iterator)
{
    if (!iterator || !iterator->table) return false;
    
    // 保存当前状态
    Stru_CN_HashTableIterator_t temp = *iterator;
    
    // 尝试获取下一个元素，但不修改原始迭代器
    void* key;
    void* value;
    return CN_hash_table_iterator_next(&temp, &key, &value);
}

bool CN_hash_table_iterator_next(Stru_CN_HashTableIterator_t* iterator,
                                 void** key, void** value)
{
    if (!iterator || !iterator->table) return false;
    
    Stru_CN_HashTable_t* table = iterator->table;
    
    if (table->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        // 开放寻址法：遍历条目数组
        while (iterator->current_bucket < table->capacity)
        {
            Stru_CN_HashEntry_t* entry = &table->data.entries[iterator->current_bucket];
            
            if (entry->status == Eum_HASH_ENTRY_OCCUPIED)
            {
                // 找到有效条目
                iterator->current_key = entry->key;
                iterator->current_value = entry->value;
                
                if (key) *key = entry->key;
                if (value) *value = entry->value;
                
                iterator->current_bucket++;
                return true;
            }
            
            iterator->current_bucket++;
        }
    }
    else
    {
        // 链地址法：遍历链表
        while (iterator->current_bucket < table->capacity)
        {
            Stru_CN_HashChainNode_t* chain = table->data.chains[iterator->current_bucket];
            
            if (chain)
            {
                // 找到链表中的节点
                Stru_CN_HashChainNode_t* node = chain;
                
                // 如果已经在链表中，移动到下一个节点
                if (iterator->current_entry > 0)
                {
                    for (size_t i = 0; i < iterator->current_entry && node; i++)
                    {
                        node = node->next;
                    }
                }
                
                if (node)
                {
                    // 找到有效节点
                    iterator->current_key = node->key;
                    iterator->current_value = node->value;
                    
                    if (key) *key = node->key;
                    if (value) *value = node->value;
                    
                    iterator->current_entry++;
                    
                    // 如果这是链表的最后一个节点，移动到下一个桶
                    if (!node->next)
                    {
                        iterator->current_bucket++;
                        iterator->current_entry = 0;
                    }
                    
                    return true;
                }
            }
            
            // 当前桶没有更多节点，移动到下一个桶
            iterator->current_bucket++;
            iterator->current_entry = 0;
        }
    }
    
    // 没有更多条目
    iterator->current_key = NULL;
    iterator->current_value = NULL;
    return false;
}

bool CN_hash_table_iterator_current(const Stru_CN_HashTableIterator_t* iterator,
                                    void** key, void** value)
{
    if (!iterator || !iterator->table || 
        !iterator->current_key || !iterator->current_value)
    {
        return false;
    }
    
    if (key) *key = iterator->current_key;
    if (value) *value = iterator->current_value;
    return true;
}

// ============================================================================
// 工具函数
// ============================================================================

void CN_hash_table_dump(const Stru_CN_HashTable_t* table)
{
    if (!table)
    {
        printf("Hash table: NULL\n");
        return;
    }
    
    printf("Hash Table Dump:\n");
    printf("  Size: %zu\n", table->size);
    printf("  Capacity: %zu\n", table->capacity);
    printf("  Load factor: %.2f\n", CN_hash_table_load_factor(table));
    printf("  Strategy: %s\n", 
           table->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING ? 
           "Open Addressing" : "Separate Chaining");
    printf("  Key size: %zu bytes\n", table->key_size);
    printf("  Value size: %zu bytes\n", table->value_size);
    printf("  Collisions: %zu\n", table->collisions);
    printf("  Total probes: %zu\n", table->total_probes);
    printf("  Resize count: %zu\n", table->resize_count);
    
    if (table->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        printf("  Entries:\n");
        for (size_t i = 0; i < table->capacity; i++)
        {
            Stru_CN_HashEntry_t* entry = &table->data.entries[i];
            printf("    [%zu]: ", i);
            
            switch (entry->status)
            {
                case Eum_HASH_ENTRY_EMPTY:
                    printf("EMPTY\n");
                    break;
                case Eum_HASH_ENTRY_OCCUPIED:
                    printf("OCCUPIED (key=%p, value=%p)\n", entry->key, entry->value);
                    break;
                case Eum_HASH_ENTRY_DELETED:
                    printf("DELETED\n");
                    break;
            }
        }
    }
    else
    {
        printf("  Chains:\n");
        for (size_t i = 0; i < table->capacity; i++)
        {
            printf("    [%zu]: ", i);
            
            Stru_CN_HashChainNode_t* node = table->data.chains[i];
            if (!node)
            {
                printf("EMPTY\n");
            }
            else
            {
                while (node)
                {
                    printf("(key=%p, value=%p)", node->key, node->value);
                    node = node->next;
                    if (node) printf(" -> ");
                }
                printf("\n");
            }
        }
    }
}

void CN_hash_table_stats(const Stru_CN_HashTable_t* table,
                         double* avg_probe_length,
                         size_t* max_probe_length,
                         size_t* empty_buckets)
{
    if (!table)
    {
        if (avg_probe_length) *avg_probe_length = 0.0;
        if (max_probe_length) *max_probe_length = 0;
        if (empty_buckets) *empty_buckets = 0;
        return;
    }
    
    size_t total_probes = 0;
    size_t max_probe = 0;
    size_t empty_count = 0;
    
    if (table->strategy == Eum_HASH_TABLE_OPEN_ADDRESSING)
    {
        // 对于开放寻址法，计算每个条目的探测长度
        for (size_t i = 0; i < table->capacity; i++)
        {
            Stru_CN_HashEntry_t* entry = &table->data.entries[i];
            if (entry->status == Eum_HASH_ENTRY_OCCUPIED)
            {
                // 计算这个条目的探测长度
                size_t hash = compute_hash(table, entry->key);
                size_t probe_count = 0;
                size_t current_index = hash;
                
                while (probe_count < table->capacity)
                {
                    Stru_CN_HashEntry_t* current_entry = &table->data.entries[current_index];
                    
                    if (current_entry == entry)
                    {
                        // 找到条目
                        total_probes += probe_count;
                        if (probe_count > max_probe) max_probe = probe_count;
                        break;
                    }
                    
                    probe_count++;
                    current_index = CN_hash_table_next_probe(table, hash, probe_count);
                }
            }
            else if (entry->status == Eum_HASH_ENTRY_EMPTY)
            {
                empty_count++;
            }
        }
    }
    else
    {
        // 对于链地址法，计算每个链的平均长度
        for (size_t i = 0; i < table->capacity; i++)
        {
            Stru_CN_HashChainNode_t* node = table->data.chains[i];
            
            if (!node)
            {
                empty_count++;
            }
            else
            {
                size_t chain_length = 0;
                while (node)
                {
                    chain_length++;
                    node = node->next;
                }
                
                // 链中第n个节点的探测长度为n-1
                total_probes += (chain_length * (chain_length - 1)) / 2;
                if (chain_length - 1 > max_probe) max_probe = chain_length - 1;
            }
        }
    }
    
    // 计算平均值
    if (avg_probe_length)
    {
        if (table->size > 0)
        {
            *avg_probe_length = (double)total_probes / (double)table->size;
        }
        else
        {
            *avg_probe_length = 0.0;
        }
    }
    
    if (max_probe_length) *max_probe_length = max_probe;
    if (empty_buckets) *empty_buckets = empty_count;
}
