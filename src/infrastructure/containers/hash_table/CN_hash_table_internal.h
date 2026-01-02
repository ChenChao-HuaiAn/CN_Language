/******************************************************************************
 * 文件名: CN_hash_table_internal.h
 * 功能: CN_Language哈希表容器内部实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义哈希表内部结构
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_HASH_TABLE_INTERNAL_H
#define CN_HASH_TABLE_INTERNAL_H

#include "CN_hash_table.h"
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 哈希表条目结构体（开放寻址法）
 */
typedef struct Stru_CN_HashEntry_t
{
    void* key;                      /**< 键指针 */
    void* value;                    /**< 值指针 */
    Eum_CN_HashEntryStatus_t status; /**< 条目状态 */
} Stru_CN_HashEntry_t;

/**
 * @brief 链地址法节点结构体
 */
typedef struct Stru_CN_HashChainNode_t
{
    void* key;                      /**< 键指针 */
    void* value;                    /**< 值指针 */
    struct Stru_CN_HashChainNode_t* next; /**< 下一个节点 */
} Stru_CN_HashChainNode_t;

/**
 * @brief 哈希表内部结构体
 */
struct Stru_CN_HashTable_t
{
    // 基本属性
    size_t key_size;                /**< 键大小（字节） */
    size_t value_size;              /**< 值大小（字节） */
    size_t size;                    /**< 当前条目数量 */
    size_t capacity;                /**< 桶容量 */
    Eum_CN_HashTableCollisionStrategy_t strategy; /**< 冲突解决策略 */
    
    // 函数指针
    CN_HashFunc hash_func;          /**< 哈希函数 */
    CN_KeyCompareFunc key_compare_func; /**< 键比较函数 */
    CN_KeyFreeFunc key_free_func;   /**< 键释放函数 */
    CN_ValueFreeFunc value_free_func; /**< 值释放函数 */
    CN_KeyCopyFunc key_copy_func;   /**< 键复制函数 */
    CN_ValueCopyFunc value_copy_func; /**< 值复制函数 */
    
    // 数据存储（根据策略使用不同的存储方式）
    union
    {
        Stru_CN_HashEntry_t* entries;   /**< 开放寻址法：条目数组 */
        Stru_CN_HashChainNode_t** chains; /**< 链地址法：链表头数组 */
    } data;
    
    // 统计信息
    size_t collisions;              /**< 冲突次数 */
    size_t total_probes;            /**< 总探测次数 */
    size_t resize_count;            /**< 调整大小次数 */
};

/**
 * @brief 默认哈希函数（如果用户未提供）
 * 
 * @param key 键指针
 * @param table_size 哈希表大小
 * @return 哈希值
 */
size_t CN_hash_table_default_hash(const void* key, size_t key_size, size_t table_size);

/**
 * @brief 默认键比较函数（如果用户未提供）
 * 
 * @param key1 第一个键指针
 * @param key2 第二个键指针
 * @param key_size 键大小
 * @return 如果键相等返回true，否则返回false
 */
bool CN_hash_table_default_key_compare(const void* key1, const void* key2, size_t key_size);

/**
 * @brief 创建新条目（开放寻址法）
 * 
 * @param table 哈希表
 * @param key 键指针
 * @param value 值指针
 * @return 新创建的条目，失败返回NULL
 */
Stru_CN_HashEntry_t* CN_hash_table_create_entry(Stru_CN_HashTable_t* table,
                                                const void* key, const void* value);

/**
 * @brief 销毁条目（开放寻址法）
 * 
 * @param table 哈希表
 * @param entry 要销毁的条目
 */
void CN_hash_table_destroy_entry(Stru_CN_HashTable_t* table, Stru_CN_HashEntry_t* entry);

/**
 * @brief 创建新链节点（链地址法）
 * 
 * @param table 哈希表
 * @param key 键指针
 * @param value 值指针
 * @return 新创建的节点，失败返回NULL
 */
Stru_CN_HashChainNode_t* CN_hash_table_create_chain_node(Stru_CN_HashTable_t* table,
                                                         const void* key, const void* value);

/**
 * @brief 销毁链节点（链地址法）
 * 
 * @param table 哈希表
 * @param node 要销毁的节点
 */
void CN_hash_table_destroy_chain_node(Stru_CN_HashTable_t* table, Stru_CN_HashChainNode_t* node);

/**
 * @brief 查找条目（开放寻址法）
 * 
 * @param table 哈希表
 * @param key 键指针
 * @param index 输出参数，接收找到的索引
 * @return 如果找到返回条目指针，否则返回NULL
 */
Stru_CN_HashEntry_t* CN_hash_table_find_entry(const Stru_CN_HashTable_t* table,
                                              const void* key, size_t* index);

/**
 * @brief 查找链节点（链地址法）
 * 
 * @param table 哈希表
 * @param key 键指针
 * @param bucket_index 输出参数，接收桶索引
 * @param prev_node 输出参数，接收前一个节点（用于删除）
 * @return 如果找到返回节点指针，否则返回NULL
 */
Stru_CN_HashChainNode_t* CN_hash_table_find_chain_node(const Stru_CN_HashTable_t* table,
                                                       const void* key,
                                                       size_t* bucket_index,
                                                       Stru_CN_HashChainNode_t** prev_node);

/**
 * @brief 计算下一个探测位置（开放寻址法）
 * 
 * @param table 哈希表
 * @param start_index 起始索引
 * @param probe_count 探测次数
 * @return 下一个索引
 */
size_t CN_hash_table_next_probe(const Stru_CN_HashTable_t* table,
                                size_t start_index, size_t probe_count);

/**
 * @brief 重新哈希所有条目（调整大小时使用）
 * 
 * @param table 哈希表
 * @param new_capacity 新容量
 * @return 重新哈希成功返回true，失败返回false
 */
bool CN_hash_table_rehash(Stru_CN_HashTable_t* table, size_t new_capacity);

/**
 * @brief 检查是否需要调整大小
 * 
 * @param table 哈希表
 * @param load_factor_threshold 负载因子阈值
 * @return 如果需要调整大小返回true，否则返回false
 */
bool CN_hash_table_needs_resize(const Stru_CN_HashTable_t* table,
                                double load_factor_threshold);

/**
 * @brief 计算下一个质数（用于哈希表容量）
 * 
 * @param n 起始数字
 * @return 下一个质数
 */
size_t next_prime(size_t n);

/**
 * @brief 分配并初始化键内存
 * 
 * @param table 哈希表
 * @param key 源键
 * @return 新分配的键，失败返回NULL
 */
void* allocate_key(Stru_CN_HashTable_t* table, const void* key);

/**
 * @brief 分配并初始化值内存
 * 
 * @param table 哈希表
 * @param value 源值
 * @return 新分配的值，失败返回NULL
 */
void* allocate_value(Stru_CN_HashTable_t* table, const void* value);

/**
 * @brief 释放键内存
 * 
 * @param table 哈希表
 * @param key 要释放的键
 */
void free_key(Stru_CN_HashTable_t* table, void* key);

/**
 * @brief 释放值内存
 * 
 * @param table 哈希表
 * @param value 要释放的值
 */
void free_value(Stru_CN_HashTable_t* table, void* value);

/**
 * @brief 计算键的哈希值
 * 
 * @param table 哈希表
 * @param key 键指针
 * @return 哈希值
 */
size_t compute_hash(const Stru_CN_HashTable_t* table, const void* key);

/**
 * @brief 比较两个键是否相等
 * 
 * @param table 哈希表
 * @param key1 第一个键
 * @param key2 第二个键
 * @return 如果相等返回true，否则返回false
 */
bool compare_keys(const Stru_CN_HashTable_t* table, const void* key1, const void* key2);

#endif // CN_HASH_TABLE_INTERNAL_H
