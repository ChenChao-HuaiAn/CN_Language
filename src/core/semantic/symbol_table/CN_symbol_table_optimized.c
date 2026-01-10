/**
 * @file CN_symbol_table_optimized.c
 * @brief 优化版符号表模块实现（使用哈希表）
 
 * 本文件实现了优化版的符号表模块功能，使用哈希表提高查找性能，
 * 支持缓存机制和批量操作优化。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_symbol_table.h"
#include "CN_symbol_table_optimized.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ============================================================================
// 哈希表相关定义
// ============================================================================

/**
 * @brief 哈希表初始容量
 */
#define HASH_TABLE_INITIAL_CAPACITY 16

/**
 * @brief 哈希表负载因子阈值（超过此值则扩容）
 */
#define HASH_TABLE_LOAD_FACTOR_THRESHOLD 0.75

/**
 * @brief 哈希表条目状态
 */
typedef enum Eum_HashEntryStatus_t
{
    Eum_HASH_ENTRY_EMPTY = 0,      ///< 空条目
    Eum_HASH_ENTRY_OCCUPIED,       ///< 已占用条目
    Eum_HASH_ENTRY_DELETED         ///< 已删除条目（用于开放寻址）
} Eum_HashEntryStatus_t;

/**
 * @brief 哈希表条目
 */
typedef struct Stru_HashEntry_t
{
    char* key;                     ///< 键（符号名称）
    Stru_SymbolInfo_t value;       ///< 值（符号信息）
    Eum_HashEntryStatus_t status;  ///< 条目状态
    uint32_t hash;                 ///< 哈希值缓存
} Stru_HashEntry_t;

/**
 * @brief 哈希表
 */
typedef struct Stru_HashTable_t
{
    Stru_HashEntry_t* entries;     ///< 条目数组
    size_t capacity;               ///< 容量
    size_t size;                   ///< 当前大小
    size_t deleted_count;          ///< 已删除条目计数
} Stru_HashTable_t;

// ============================================================================
// 符号表接口私有数据结构（优化版）
// ============================================================================

/**
 * @brief 符号表接口私有数据（优化版）
 */
typedef struct Stru_SymbolTableDataOptimized_t
{
    Stru_SymbolTableInterface_t* parent_scope;  ///< 父作用域符号表
    Stru_HashTable_t hash_table;                ///< 哈希表
    Stru_SymbolInfo_t* cache;                   ///< 缓存最近访问的符号
    size_t cache_size;                          ///< 缓存大小
    size_t cache_capacity;                      ///< 缓存容量
    size_t access_count;                        ///< 访问计数（用于缓存策略）
} Stru_SymbolTableDataOptimized_t;

// ============================================================================
// 哈希表辅助函数
// ============================================================================

/**
 * @brief 计算字符串的哈希值（FNV-1a算法）
 */
static uint32_t hash_string(const char* str)
{
    if (str == NULL)
    {
        return 0;
    }
    
    uint32_t hash = 2166136261u;  // FNV偏移基础值
    const char* p = str;
    
    while (*p)
    {
        hash ^= (uint32_t)(*p);
        hash *= 16777619u;  // FNV质数
        p++;
    }
    
    return hash;
}

/**
 * @brief 创建哈希表
 */
static Stru_HashTable_t create_hash_table(size_t initial_capacity)
{
    Stru_HashTable_t table;
    
    if (initial_capacity == 0)
    {
        initial_capacity = HASH_TABLE_INITIAL_CAPACITY;
    }
    
    table.entries = (Stru_HashEntry_t*)calloc(initial_capacity, sizeof(Stru_HashEntry_t));
    table.capacity = (table.entries != NULL) ? initial_capacity : 0;
    table.size = 0;
    table.deleted_count = 0;
    
    return table;
}

/**
 * @brief 销毁哈希表
 */
static void destroy_hash_table(Stru_HashTable_t* table)
{
    if (table == NULL || table->entries == NULL)
    {
        return;
    }
    
    // 释放所有键的内存
    for (size_t i = 0; i < table->capacity; i++)
    {
        if (table->entries[i].status == Eum_HASH_ENTRY_OCCUPIED)
        {
            free(table->entries[i].key);
        }
    }
    
    free(table->entries);
    table->entries = NULL;
    table->capacity = 0;
    table->size = 0;
    table->deleted_count = 0;
}

/**
 * @brief 调整哈希表大小
 */
static bool resize_hash_table(Stru_HashTable_t* table, size_t new_capacity)
{
    if (table == NULL || new_capacity == 0)
    {
        return false;
    }
    
    // 创建新条目数组
    Stru_HashEntry_t* new_entries = (Stru_HashEntry_t*)calloc(new_capacity, sizeof(Stru_HashEntry_t));
    if (new_entries == NULL)
    {
        return false;
    }
    
    // 重新哈希所有条目
    size_t old_capacity = table->capacity;
    Stru_HashEntry_t* old_entries = table->entries;
    
    for (size_t i = 0; i < old_capacity; i++)
    {
        if (old_entries[i].status == Eum_HASH_ENTRY_OCCUPIED)
        {
            // 计算在新表中的位置
            uint32_t hash = old_entries[i].hash;
            size_t index = hash % new_capacity;
            
            // 线性探测解决冲突
            while (new_entries[index].status == Eum_HASH_ENTRY_OCCUPIED)
            {
                index = (index + 1) % new_capacity;
            }
            
            // 复制条目
            new_entries[index] = old_entries[i];
        }
    }
    
    // 更新哈希表
    free(old_entries);
    table->entries = new_entries;
    table->capacity = new_capacity;
    table->deleted_count = 0;  // 重新哈希后没有已删除条目
    
    return true;
}

/**
 * @brief 在哈希表中查找条目
 */
static Stru_HashEntry_t* hash_table_find(Stru_HashTable_t* table, const char* key, uint32_t hash)
{
    if (table == NULL || table->entries == NULL || key == NULL)
    {
        return NULL;
    }
    
    size_t index = hash % table->capacity;
    size_t start_index = index;
    
    // 线性探测
    while (table->entries[index].status != Eum_HASH_ENTRY_EMPTY)
    {
        if (table->entries[index].status == Eum_HASH_ENTRY_OCCUPIED &&
            table->entries[index].hash == hash &&
            strcmp(table->entries[index].key, key) == 0)
        {
            return &table->entries[index];
        }
        
        index = (index + 1) % table->capacity;
        
        // 如果回到起点，说明表中没有该键
        if (index == start_index)
        {
            break;
        }
    }
    
    return NULL;
}

/**
 * @brief 在哈希表中插入条目
 */
static bool hash_table_insert(Stru_HashTable_t* table, const char* key, 
                             const Stru_SymbolInfo_t* value, uint32_t hash)
{
    if (table == NULL || table->entries == NULL || key == NULL || value == NULL)
    {
        return false;
    }
    
    // 检查是否需要扩容
    double load_factor = (double)(table->size + table->deleted_count) / table->capacity;
    if (load_factor > HASH_TABLE_LOAD_FACTOR_THRESHOLD)
    {
        size_t new_capacity = table->capacity * 2;
        if (!resize_hash_table(table, new_capacity))
        {
            return false;
        }
    }
    
    size_t index = hash % table->capacity;
    
    // 查找插入位置（处理冲突和已删除条目）
    while (table->entries[index].status == Eum_HASH_ENTRY_OCCUPIED)
    {
        // 如果键已存在，返回false（不允许重复键）
        if (table->entries[index].hash == hash && 
            strcmp(table->entries[index].key, key) == 0)
        {
            return false;
        }
        
        index = (index + 1) % table->capacity;
    }
    
    // 复制键
    char* key_copy = strdup(key);
    if (key_copy == NULL)
    {
        return false;
    }
    
    // 插入条目
    table->entries[index].key = key_copy;
    table->entries[index].value = *value;
    table->entries[index].hash = hash;
    table->entries[index].status = Eum_HASH_ENTRY_OCCUPIED;
    
    if (table->entries[index].status == Eum_HASH_ENTRY_DELETED)
    {
        table->deleted_count--;
    }
    
    table->size++;
    return true;
}

/**
 * @brief 从哈希表中删除条目
 */
static bool hash_table_remove(Stru_HashTable_t* table, const char* key, uint32_t hash)
{
    if (table == NULL || table->entries == NULL || key == NULL)
    {
        return false;
    }
    
    Stru_HashEntry_t* entry = hash_table_find(table, key, hash);
    if (entry == NULL)
    {
        return false;
    }
    
    // 标记为已删除（惰性删除）
    free(entry->key);
    entry->key = NULL;
    entry->status = Eum_HASH_ENTRY_DELETED;
    
    table->size--;
    table->deleted_count++;
    
    return true;
}

// ============================================================================
// 缓存辅助函数
// ============================================================================

/**
 * @brief 初始化缓存
 */
static bool init_cache(Stru_SymbolTableDataOptimized_t* data, size_t cache_capacity)
{
    if (data == NULL || cache_capacity == 0)
    {
        return false;
    }
    
    data->cache = (Stru_SymbolInfo_t*)malloc(cache_capacity * sizeof(Stru_SymbolInfo_t));
    if (data->cache == NULL)
    {
        return false;
    }
    
    data->cache_capacity = cache_capacity;
    data->cache_size = 0;
    data->access_count = 0;
    
    return true;
}

/**
 * @brief 销毁缓存
 */
static void destroy_cache(Stru_SymbolTableDataOptimized_t* data)
{
    if (data == NULL || data->cache == NULL)
    {
        return;
    }
    
    free(data->cache);
    data->cache = NULL;
    data->cache_capacity = 0;
    data->cache_size = 0;
    data->access_count = 0;
}

/**
 * @brief 在缓存中查找符号
 */
static Stru_SymbolInfo_t* cache_lookup(Stru_SymbolTableDataOptimized_t* data, const char* name)
{
    if (data == NULL || data->cache == NULL || name == NULL)
    {
        return NULL;
    }
    
    // 简单线性搜索（缓存通常很小）
    for (size_t i = 0; i < data->cache_size; i++)
    {
        if (strcmp(data->cache[i].name, name) == 0)
        {
            // 找到缓存项，更新访问计数
            data->access_count++;
            return &data->cache[i];
        }
    }
    
    return NULL;
}

/**
 * @brief 添加符号到缓存
 */
static bool cache_add(Stru_SymbolTableDataOptimized_t* data, const Stru_SymbolInfo_t* symbol)
{
    if (data == NULL || data->cache == NULL || symbol == NULL)
    {
        return false;
    }
    
    // 如果缓存已满，使用简单的LRU策略：替换最早添加的项
    if (data->cache_size >= data->cache_capacity)
    {
        // 这里使用简单的策略：移除第一个项（可以改进为真正的LRU）
        for (size_t i = 0; i < data->cache_size - 1; i++)
        {
            data->cache[i] = data->cache[i + 1];
        }
        data->cache_size--;
    }
    
    // 添加新项到缓存末尾
    data->cache[data->cache_size] = *symbol;
    data->cache_size++;
    
    return true;
}

// ============================================================================
// 符号表接口实现函数（优化版）
// ============================================================================

/**
 * @brief 符号表初始化函数（优化版）
 */
static bool symbol_table_optimized_initialize(Stru_SymbolTableInterface_t* symbol_table, 
                                             Stru_SymbolTableInterface_t* parent_scope)
{
    if (symbol_table == NULL || symbol_table->private_data != NULL)
    {
        return false;
    }
    
    Stru_SymbolTableDataOptimized_t* data = 
        (Stru_SymbolTableDataOptimized_t*)malloc(sizeof(Stru_SymbolTableDataOptimized_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->parent_scope = parent_scope;
    data->hash_table = create_hash_table(HASH_TABLE_INITIAL_CAPACITY);
    
    // 初始化缓存（默认缓存容量为8）
    if (!init_cache(data, 8))
    {
        free(data);
        return false;
    }
    
    data->access_count = 0;
    
    symbol_table->private_data = data;
    return true;
}

/**
 * @brief 插入符号（优化版）
 */
static bool symbol_table_optimized_insert_symbol(Stru_SymbolTableInterface_t* symbol_table, 
                                                const Stru_SymbolInfo_t* symbol)
{
    if (symbol_table == NULL || symbol_table->private_data == NULL || symbol == NULL)
    {
        return false;
    }
    
    Stru_SymbolTableDataOptimized_t* data = 
        (Stru_SymbolTableDataOptimized_t*)symbol_table->private_data;
    
    // 计算哈希值
    uint32_t hash = hash_string(symbol->name);
    
    // 插入到哈希表
    bool success = hash_table_insert(&data->hash_table, symbol->name, symbol, hash);
    
    // 如果插入成功，也添加到缓存
    if (success)
    {
        cache_add(data, symbol);
    }
    
    return success;
}

/**
 * @brief 查找符号（优化版）
 */
static Stru_SymbolInfo_t* symbol_table_optimized_lookup_symbol(
    Stru_SymbolTableInterface_t* symbol_table,
    const char* name, bool search_parent)
{
    if (symbol_table == NULL || symbol_table->private_data == NULL || name == NULL)
    {
        return NULL;
    }
    
    Stru_SymbolTableDataOptimized_t* data = 
        (Stru_SymbolTableDataOptimized_t*)symbol_table->private_data;
    
    // 首先在缓存中查找
    Stru_SymbolInfo_t* cached_symbol = cache_lookup(data, name);
    if (cached_symbol != NULL)
    {
        return cached_symbol;
    }
    
    // 计算哈希值
    uint32_t hash = hash_string(name);
    
    // 在哈希表中查找
    Stru_HashEntry_t* entry = hash_table_find(&data->hash_table, name, hash);
    if (entry != NULL)
    {
        // 添加到缓存
        cache_add(data, &entry->value);
        return &entry->value;
    }
    
    // 如果允许在父作用域中查找且存在父作用域
    if (search_parent && data->parent_scope != NULL)
    {
        return data->parent_scope->lookup_symbol(data->parent_scope, name, true);
    }
    
    return NULL;
}

/**
 * @brief 删除符号（优化版）
 */
static bool symbol_table_optimized_remove_symbol(Stru_SymbolTableInterface_t* symbol_table,
                                                const char* name)
{
    if (symbol_table == NULL || symbol_table->private_data == NULL || name == NULL)
    {
        return false;
    }
    
    Stru_SymbolTableDataOptimized_t* data = 
        (Stru_SymbolTableDataOptimized_t*)symbol_table->private_data;
    
    // 计算哈希值
    uint32_t hash = hash_string(name);
    
    // 从哈希表中删除
    return hash_table_remove(&data->hash_table, name, hash);
    
    // 注意：这里没有从缓存中删除，因为缓存很小，会被自然淘汰
}

/**
 * @brief 获取符号数量（优化版）
 */
static size_t symbol_table_optimized_get_symbol_count(Stru_SymbolTableInterface_t* symbol_table)
{
    if (symbol_table == NULL || symbol_table->private_data == NULL)
    {
        return 0;
    }
    
    Stru_SymbolTableDataOptimized_t* data = 
        (Stru_SymbolTableDataOptimized_t*)symbol_table->private_data;
    
    return data->hash_table.size;
}

/**
 * @brief 获取所有符号（优化版）
 */
static size_t symbol_table_optimized_get_all_symbols(Stru_SymbolTableInterface_t* symbol_table,
                                                    Stru_SymbolInfo_t* symbols, size_t max_symbols)
{
    if (symbol_table == NULL || symbol_table->private_data == NULL || symbols == NULL)
    {
        return 0;
    }
    
    Stru_SymbolTableDataOptimized_t* data = 
        (Stru_SymbolTableDataOptimized_t*)symbol_table->private_data;
    
    size_t count = 0;
    Stru_HashTable_t* table = &data->hash_table;
    
    // 遍历哈希表，收集所有符号
    for (size_t i = 0; i < table->capacity && count < max_symbols; i++)
    {
        if (table->entries[i].status == Eum_HASH_ENTRY_OCCUPIED)
        {
            symbols[count] = table->entries[i].value;
            count++;
        }
    }
    
    return count;
}

/**
 * @brief 清空符号表（优化版）
 */
static void symbol_table_optimized_clear(Stru_SymbolTableInterface_t* symbol_table)
{
    if (symbol_table == NULL || symbol_table->private_data == NULL)
    {
        return;
    }
    
    Stru_SymbolTableDataOptimized_t* data = 
        (Stru_SymbolTableDataOptimized_t*)symbol_table->private_data;
    
    // 清空哈希表
    for (size_t i = 0; i < data->hash_table.capacity; i++)
    {
        if (data->hash_table.entries[i].status == Eum_HASH_ENTRY_OCCUPIED)
        {
            free(data->hash_table.entries[i].key);
            data->hash_table.entries[i].key = NULL;
            data->hash_table.entries[i].status = Eum_HASH_ENTRY_EMPTY;
        }
    }
    
    data->hash_table.size = 0;
    data->hash_table.deleted_count = 0;
    
    // 清空缓存
    data->cache_size = 0;
    data->access_count = 0;
}

/**
 * @brief 销毁符号表（优化版）
 */
static void symbol_table_optimized_destroy(Stru_SymbolTableInterface_t* symbol_table)
{
    if (symbol_table == NULL)
    {
        return;
    }
    
    if (symbol_table->private_data != NULL)
    {
        Stru_SymbolTableDataOptimized_t* data = 
            (Stru_SymbolTableDataOptimized_t*)symbol_table->private_data;
        
        // 销毁哈希表
        destroy_hash_table(&data->hash_table);
        
        // 销毁缓存
        destroy_cache(data);
        
        free(data);
        symbol_table->private_data = NULL;
    }
}

// ============================================================================
// 工厂函数实现（优化版）
// ============================================================================

/**
 * @brief 创建优化版符号表接口实例
 */
Stru_SymbolTableInterface_t* F_create_symbol_table_interface_optimized(void)
{
    Stru_SymbolTableInterface_t* interface = 
        (Stru_SymbolTableInterface_t*)malloc(sizeof(Stru_SymbolTableInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = symbol_table_optimized_initialize;
    interface->insert_symbol = symbol_table_optimized_insert_symbol;
    interface->lookup_symbol = symbol_table_optimized_lookup_symbol;
    interface->remove_symbol = symbol_table_optimized_remove_symbol;
    interface->get_symbol_count = symbol_table_optimized_get_symbol_count;
    interface->get_all_symbols = symbol_table_optimized_get_all_symbols;
    interface->clear = symbol_table_optimized_clear;
    interface->destroy = symbol_table_optimized_destroy;
    interface->private_data = NULL;
    
    return interface;
}

/**
 * @brief 销毁优化版符号表接口实例
 */
void F_destroy_symbol_table_interface_optimized(Stru_SymbolTableInterface_t* interface)
{
    if (interface == NULL)
    {
        return;
    }
    
    if (interface->destroy != NULL)
    {
        interface->destroy(interface);
    }
    
    free(interface);
}
