/**
 * @file CN_hash_table_entry.c
 * @brief 哈希表条目实现文件
 * 
 * 实现哈希表条目的创建、销毁和管理功能。
 * 遵循单一职责原则，专注于条目级别的操作。
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

/**
 * @brief 创建哈希条目
 * 
 * 分配并初始化一个新的哈希条目。
 * 
 * @param key 键指针
 * @param value 值指针
 * @param key_size 键大小
 * @param value_size 值大小
 * @return 指向新条目的指针，失败返回NULL
 */
Stru_HashEntry_t* F_create_hash_entry(void* key, void* value, 
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
    
    // 特殊处理：当键大小等于指针大小时，存储指针值本身
    // 这是因为对于指针键，key参数是指针值，而不是指向数据的指针
    if (key_size == sizeof(void*))
    {
        // 存储key指针值本身
        void** key_ptr = (void**)entry->key;
        *key_ptr = key;
    }
    else
    {
        // 正常情况：复制key指向的数据
        memcpy(entry->key, key, key_size);
    }
    
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
 * @brief 释放哈希条目
 * 
 * 释放条目占用的内存。
 * 
 * @param entry 要释放的条目指针
 */
void F_free_hash_entry(Stru_HashEntry_t* entry)
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
 * @brief 复制哈希条目
 * 
 * 创建现有条目的深拷贝。
 * 
 * @param src 源条目指针
 * @param key_size 键大小
 * @param value_size 值大小
 * @return 指向新条目的指针，失败返回NULL
 */
Stru_HashEntry_t* F_copy_hash_entry(Stru_HashEntry_t* src,
                                   size_t key_size, size_t value_size)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    return F_create_hash_entry(src->key, src->value, key_size, value_size);
}

/**
 * @brief 比较哈希条目键
 * 
 * 比较两个条目的键是否相等。
 * 
 * @param entry1 第一个条目指针
 * @param entry2 第二个条目指针
 * @param compare_function 键比较函数
 * @return 相等返回true，否则返回false
 */
bool F_compare_hash_entry_keys(Stru_HashEntry_t* entry1, Stru_HashEntry_t* entry2,
                              int (*compare_function)(void* key1, void* key2))
{
    if (entry1 == NULL || entry2 == NULL || compare_function == NULL)
    {
        return false;
    }
    
    return compare_function(entry1->key, entry2->key) == 0;
}

/**
 * @brief 获取哈希条目键
 * 
 * 获取条目的键指针。
 * 
 * @param entry 条目指针
 * @return 键指针，如果entry为NULL则返回NULL
 */
void* F_get_hash_entry_key(Stru_HashEntry_t* entry)
{
    if (entry == NULL)
    {
        return NULL;
    }
    
    return entry->key;
}

/**
 * @brief 获取哈希条目值
 * 
 * 获取条目的值指针。
 * 
 * @param entry 条目指针
 * @return 值指针，如果entry为NULL则返回NULL
 */
void* F_get_hash_entry_value(Stru_HashEntry_t* entry)
{
    if (entry == NULL)
    {
        return NULL;
    }
    
    return entry->value;
}

/**
 * @brief 设置哈希条目值
 * 
 * 设置条目的值。
 * 
 * @param entry 条目指针
 * @param value 新值指针
 * @param value_size 值大小
 * @return 成功返回true，失败返回false
 */
bool F_set_hash_entry_value(Stru_HashEntry_t* entry, void* value, size_t value_size)
{
    if (entry == NULL || value == NULL)
    {
        return false;
    }
    
    // 分配新值内存
    void* new_value = malloc(value_size);
    if (new_value == NULL)
    {
        return false;
    }
    
    // 复制数据
    memcpy(new_value, value, value_size);
    
    // 释放旧值
    if (entry->value != NULL)
    {
        free(entry->value);
    }
    
    // 设置新值
    entry->value = new_value;
    
    return true;
}

/**
 * @brief 获取哈希条目下一个条目
 * 
 * 获取链表中的下一个条目。
 * 
 * @param entry 当前条目指针
 * @return 下一个条目指针，如果没有下一个条目则返回NULL
 */
Stru_HashEntry_t* F_get_hash_entry_next(Stru_HashEntry_t* entry)
{
    if (entry == NULL)
    {
        return NULL;
    }
    
    return entry->next;
}

/**
 * @brief 设置哈希条目下一个条目
 * 
 * 设置链表中的下一个条目。
 * 
 * @param entry 当前条目指针
 * @param next 下一个条目指针
 */
void F_set_hash_entry_next(Stru_HashEntry_t* entry, Stru_HashEntry_t* next)
{
    if (entry == NULL)
    {
        return;
    }
    
    entry->next = next;
}
