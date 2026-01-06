/**
 * @file CN_hash_table.h
 * @brief 哈希表模块头文件
 * 
 * 提供哈希表数据结构的定义和接口声明。
 * 哈希表支持高效的键值对查找、插入和删除操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_HASH_TABLE_H
#define CN_HASH_TABLE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 哈希表条目结构体
 * 
 * 哈希表中的键值对条目。
 */
typedef struct Stru_HashEntry_t
{
    void* key;                  /**< 键指针 */
    void* value;                /**< 值指针 */
    struct Stru_HashEntry_t* next; /**< 下一个条目（解决哈希冲突） */
} Stru_HashEntry_t;

/**
 * @brief 哈希表结构体
 * 
 * 哈希表数据结构，使用链地址法解决哈希冲突。
 */
typedef struct Stru_HashTable_t
{
    Stru_HashEntry_t** buckets; /**< 桶数组 */
    size_t capacity;            /**< 哈希表容量（桶的数量） */
    size_t size;                /**< 哈希表中条目数量 */
    size_t key_size;            /**< 键的大小（字节） */
    size_t value_size;          /**< 值的大小（字节） */
    uint64_t (*hash_function)(void* key); /**< 哈希函数 */
    int (*compare_function)(void* key1, void* key2); /**< 键比较函数 */
} Stru_HashTable_t;

/**
 * @brief 创建哈希表
 * 
 * 分配并初始化一个新的哈希表。
 * 
 * @param key_size 键的大小（字节）
 * @param value_size 值的大小（字节）
 * @param hash_function 哈希函数
 * @param compare_function 键比较函数
 * @param initial_capacity 初始容量
 * @return 指向新创建的哈希表的指针，失败返回NULL
 */
Stru_HashTable_t* F_create_hash_table(size_t key_size, size_t value_size,
                                     uint64_t (*hash_function)(void* key),
                                     int (*compare_function)(void* key1, void* key2),
                                     size_t initial_capacity);

/**
 * @brief 销毁哈希表
 * 
 * 释放哈希表占用的所有内存。
 * 
 * @param table 要销毁的哈希表指针
 * 
 * @note 如果table为NULL，函数不执行任何操作
 */
void F_destroy_hash_table(Stru_HashTable_t* table);

/**
 * @brief 向哈希表插入键值对
 * 
 * 向哈希表中插入一个新的键值对。
 * 
 * @param table 哈希表指针
 * @param key 键指针
 * @param value 值指针
 * @return 插入成功返回true，失败返回false
 * 
 * @note 如果键已存在，会更新对应的值
 */
bool F_hash_table_put(Stru_HashTable_t* table, void* key, void* value);

/**
 * @brief 从哈希表获取值
 * 
 * 根据键从哈希表中获取对应的值。
 * 
 * @param table 哈希表指针
 * @param key 键指针
 * @return 指向值的指针，键不存在返回NULL
 */
void* F_hash_table_get(Stru_HashTable_t* table, void* key);

/**
 * @brief 从哈希表删除键值对
 * 
 * 根据键从哈希表中删除对应的键值对。
 * 
 * @param table 哈希表指针
 * @param key 键指针
 * @return 删除成功返回true，键不存在返回false
 */
bool F_hash_table_remove(Stru_HashTable_t* table, void* key);

/**
 * @brief 检查键是否存在
 * 
 * 检查哈希表中是否存在指定的键。
 * 
 * @param table 哈希表指针
 * @param key 键指针
 * @return 键存在返回true，否则返回false
 */
bool F_hash_table_contains(Stru_HashTable_t* table, void* key);

/**
 * @brief 获取哈希表大小
 * 
 * 返回哈希表中键值对的数量。
 * 
 * @param table 哈希表指针
 * @return 哈希表大小，如果table为NULL返回0
 */
size_t F_hash_table_size(Stru_HashTable_t* table);

/**
 * @brief 获取哈希表容量
 * 
 * 返回哈希表的容量（桶的数量）。
 * 
 * @param table 哈希表指针
 * @return 哈希表容量，如果table为NULL返回0
 */
size_t F_hash_table_capacity(Stru_HashTable_t* table);

/**
 * @brief 清空哈希表
 * 
 * 移除哈希表中的所有键值对，但不释放哈希表本身。
 * 
 * @param table 哈希表指针
 * 
 * @note 清空后哈希表大小变为0
 */
void F_hash_table_clear(Stru_HashTable_t* table);

/**
 * @brief 调整哈希表容量
 * 
 * 调整哈希表的容量到指定大小。
 * 
 * @param table 哈希表指针
 * @param new_capacity 新的容量大小
 * @return 调整成功返回true，失败返回false
 */
bool F_hash_table_resize(Stru_HashTable_t* table, size_t new_capacity);

/**
 * @brief 遍历哈希表
 * 
 * 对哈希表中的每个键值对执行指定的操作。
 * 
 * @param table 哈希表指针
 * @param callback 回调函数，接收键指针、值指针和用户数据
 * @param user_data 传递给回调函数的用户数据
 */
void F_hash_table_foreach(Stru_HashTable_t* table,
                         void (*callback)(void* key, void* value, void* user_data),
                         void* user_data);

/**
 * @brief 字符串哈希函数
 * 
 * 用于字符串键的哈希函数。
 * 
 * @param key 字符串键指针
 * @return 哈希值
 */
uint64_t F_string_hash_function(void* key);

/**
 * @brief 字符串比较函数
 * 
 * 用于字符串键的比较函数。
 * 
 * @param key1 第一个字符串键指针
 * @param key2 第二个字符串键指针
 * @return 相等返回0，否则返回非0值
 */
int F_string_compare_function(void* key1, void* key2);

/**
 * @brief 整数哈希函数
 * 
 * 用于整数键的哈希函数。
 * 
 * @param key 整数键指针
 * @return 哈希值
 */
uint64_t F_int_hash_function(void* key);

/**
 * @brief 整数比较函数
 * 
 * 用于整数键的比较函数。
 * 
 * @param key1 第一个整数键指针
 * @param key2 第二个整数键指针
 * @return 相等返回0，否则返回非0值
 */
int F_int_compare_function(void* key1, void* key2);

#ifdef __cplusplus
}
#endif

#endif // CN_HASH_TABLE_H
