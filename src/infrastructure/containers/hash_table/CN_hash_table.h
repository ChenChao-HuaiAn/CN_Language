/******************************************************************************
 * 文件名: CN_hash_table.h
 * 功能: CN_Language哈希表容器
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义哈希表接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_HASH_TABLE_H
#define CN_HASH_TABLE_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 哈希表冲突解决策略枚举
 */
typedef enum Eum_CN_HashTableCollisionStrategy_t
{
    Eum_HASH_TABLE_OPEN_ADDRESSING = 0,    /**< 开放寻址法 */
    Eum_HASH_TABLE_SEPARATE_CHAINING = 1   /**< 链地址法 */
} Eum_CN_HashTableCollisionStrategy_t;

/**
 * @brief 哈希表条目状态枚举
 */
typedef enum Eum_CN_HashEntryStatus_t
{
    Eum_HASH_ENTRY_EMPTY = 0,      /**< 空条目 */
    Eum_HASH_ENTRY_OCCUPIED = 1,   /**< 已占用条目 */
    Eum_HASH_ENTRY_DELETED = 2     /**< 已删除条目（仅开放寻址法使用） */
} Eum_CN_HashEntryStatus_t;

/**
 * @brief 哈希表结构体（不透明类型）
 */
typedef struct Stru_CN_HashTable_t Stru_CN_HashTable_t;

/**
 * @brief 哈希表迭代器结构体
 */
typedef struct Stru_CN_HashTableIterator_t
{
    Stru_CN_HashTable_t* table;    /**< 关联的哈希表 */
    size_t current_bucket;         /**< 当前桶索引 */
    size_t current_entry;          /**< 当前条目索引（链地址法使用） */
    void* current_key;             /**< 当前键指针 */
    void* current_value;           /**< 当前值指针 */
} Stru_CN_HashTableIterator_t;

/**
 * @brief 哈希函数类型定义
 * 
 * @param key 键指针
 * @param table_size 哈希表大小
 * @return 哈希值（0到table_size-1之间）
 */
typedef size_t (*CN_HashFunc)(const void* key, size_t table_size);

/**
 * @brief 键比较函数类型定义
 * 
 * @param key1 第一个键指针
 * @param key2 第二个键指针
 * @return 如果键相等返回true，否则返回false
 */
typedef bool (*CN_KeyCompareFunc)(const void* key1, const void* key2);

/**
 * @brief 键释放函数类型定义
 * 
 * @param key 要释放的键
 */
typedef void (*CN_KeyFreeFunc)(void* key);

/**
 * @brief 值释放函数类型定义
 * 
 * @param value 要释放的值
 */
typedef void (*CN_ValueFreeFunc)(void* value);

/**
 * @brief 键复制函数类型定义
 * 
 * @param src 源键
 * @return 新分配的键副本
 */
typedef void* (*CN_KeyCopyFunc)(const void* src);

/**
 * @brief 值复制函数类型定义
 * 
 * @param src 源值
 * @return 新分配的值副本
 */
typedef void* (*CN_ValueCopyFunc)(const void* src);

// ============================================================================
// 哈希表创建和销毁
// ============================================================================

/**
 * @brief 创建哈希表
 * 
 * @param key_size 键大小（字节）
 * @param value_size 值大小（字节）
 * @param initial_capacity 初始容量（桶数量）
 * @param strategy 冲突解决策略
 * @return 新创建的哈希表，失败返回NULL
 */
Stru_CN_HashTable_t* CN_hash_table_create(size_t key_size, size_t value_size,
                                          size_t initial_capacity,
                                          Eum_CN_HashTableCollisionStrategy_t strategy);

/**
 * @brief 创建带自定义函数的哈希表
 * 
 * @param key_size 键大小（字节）
 * @param value_size 值大小（字节）
 * @param initial_capacity 初始容量（桶数量）
 * @param strategy 冲突解决策略
 * @param hash_func 哈希函数（可为NULL，使用默认哈希函数）
 * @param key_compare_func 键比较函数（可为NULL，使用memcmp）
 * @param key_free_func 键释放函数（可为NULL）
 * @param value_free_func 值释放函数（可为NULL）
 * @param key_copy_func 键复制函数（可为NULL）
 * @param value_copy_func 值复制函数（可为NULL）
 * @return 新创建的哈希表，失败返回NULL
 */
Stru_CN_HashTable_t* CN_hash_table_create_custom(
    size_t key_size, size_t value_size,
    size_t initial_capacity,
    Eum_CN_HashTableCollisionStrategy_t strategy,
    CN_HashFunc hash_func,
    CN_KeyCompareFunc key_compare_func,
    CN_KeyFreeFunc key_free_func,
    CN_ValueFreeFunc value_free_func,
    CN_KeyCopyFunc key_copy_func,
    CN_ValueCopyFunc value_copy_func);

/**
 * @brief 销毁哈希表
 * 
 * @param table 要销毁的哈希表
 */
void CN_hash_table_destroy(Stru_CN_HashTable_t* table);

/**
 * @brief 清空哈希表（移除所有条目）
 * 
 * @param table 要清空的哈希表
 */
void CN_hash_table_clear(Stru_CN_HashTable_t* table);

// ============================================================================
// 哈希表属性查询
// ============================================================================

/**
 * @brief 获取哈希表大小（条目数量）
 * 
 * @param table 哈希表
 * @return 哈希表大小
 */
size_t CN_hash_table_size(const Stru_CN_HashTable_t* table);

/**
 * @brief 获取哈希表容量（桶数量）
 * 
 * @param table 哈希表
 * @return 哈希表容量
 */
size_t CN_hash_table_capacity(const Stru_CN_HashTable_t* table);

/**
 * @brief 检查哈希表是否为空
 * 
 * @param table 哈希表
 * @return 如果哈希表为空返回true，否则返回false
 */
bool CN_hash_table_is_empty(const Stru_CN_HashTable_t* table);

/**
 * @brief 获取当前负载因子
 * 
 * @param table 哈希表
 * @return 负载因子（大小/容量）
 */
double CN_hash_table_load_factor(const Stru_CN_HashTable_t* table);

/**
 * @brief 获取冲突解决策略
 * 
 * @param table 哈希表
 * @return 冲突解决策略
 */
Eum_CN_HashTableCollisionStrategy_t CN_hash_table_strategy(const Stru_CN_HashTable_t* table);

/**
 * @brief 获取键大小
 * 
 * @param table 哈希表
 * @return 键大小（字节）
 */
size_t CN_hash_table_key_size(const Stru_CN_HashTable_t* table);

/**
 * @brief 获取值大小
 * 
 * @param table 哈希表
 * @return 值大小（字节）
 */
size_t CN_hash_table_value_size(const Stru_CN_HashTable_t* table);

// ============================================================================
// 条目操作
// ============================================================================

/**
 * @brief 插入或更新键值对
 * 
 * @param table 哈希表
 * @param key 键指针
 * @param value 值指针
 * @return 插入成功返回true，失败返回false
 */
bool CN_hash_table_put(Stru_CN_HashTable_t* table, const void* key, const void* value);

/**
 * @brief 获取键对应的值
 * 
 * @param table 哈希表
 * @param key 键指针
 * @return 值指针，键不存在返回NULL
 */
void* CN_hash_table_get(const Stru_CN_HashTable_t* table, const void* key);

/**
 * @brief 检查键是否存在
 * 
 * @param table 哈希表
 * @param key 键指针
 * @return 如果键存在返回true，否则返回false
 */
bool CN_hash_table_contains(const Stru_CN_HashTable_t* table, const void* key);

/**
 * @brief 移除键值对
 * 
 * @param table 哈希表
 * @param key 键指针
 * @return 移除成功返回true，键不存在返回false
 */
bool CN_hash_table_remove(Stru_CN_HashTable_t* table, const void* key);

/**
 * @brief 获取所有键
 * 
 * @param table 哈希表
 * @param keys 输出参数，接收键指针数组（调用者负责释放）
 * @return 键的数量
 */
size_t CN_hash_table_keys(const Stru_CN_HashTable_t* table, void*** keys);

/**
 * @brief 获取所有值
 * 
 * @param table 哈希表
 * @param values 输出参数，接收值指针数组（调用者负责释放）
 * @return 值的数量
 */
size_t CN_hash_table_values(const Stru_CN_HashTable_t* table, void*** values);

// ============================================================================
// 哈希表操作
// ============================================================================

/**
 * @brief 调整哈希表容量
 * 
 * @param table 哈希表
 * @param new_capacity 新容量
 * @return 调整成功返回true，失败返回false
 */
bool CN_hash_table_resize(Stru_CN_HashTable_t* table, size_t new_capacity);

/**
 * @brief 自动调整哈希表容量（基于负载因子）
 * 
 * @param table 哈希表
 * @param load_factor_threshold 负载因子阈值（默认0.75）
 * @return 如果进行了调整返回true，否则返回false
 */
bool CN_hash_table_auto_resize(Stru_CN_HashTable_t* table, double load_factor_threshold);

/**
 * @brief 复制哈希表
 * 
 * @param src 源哈希表
 * @return 新创建的哈希表副本，失败返回NULL
 */
Stru_CN_HashTable_t* CN_hash_table_copy(const Stru_CN_HashTable_t* src);

/**
 * @brief 合并两个哈希表
 * 
 * @param table1 第一个哈希表
 * @param table2 第二个哈希表
 * @param overwrite 如果键冲突，是否用table2的值覆盖table1的值
 * @return 新创建的合并后的哈希表，失败返回NULL
 */
Stru_CN_HashTable_t* CN_hash_table_merge(const Stru_CN_HashTable_t* table1,
                                         const Stru_CN_HashTable_t* table2,
                                         bool overwrite);

// ============================================================================
// 迭代器支持
// ============================================================================

/**
 * @brief 创建哈希表迭代器
 * 
 * @param table 哈希表
 * @return 新创建的迭代器，失败返回NULL
 */
Stru_CN_HashTableIterator_t* CN_hash_table_iterator_create(Stru_CN_HashTable_t* table);

/**
 * @brief 销毁迭代器
 * 
 * @param iterator 要销毁的迭代器
 */
void CN_hash_table_iterator_destroy(Stru_CN_HashTableIterator_t* iterator);

/**
 * @brief 重置迭代器到开始位置
 * 
 * @param iterator 迭代器
 */
void CN_hash_table_iterator_reset(Stru_CN_HashTableIterator_t* iterator);

/**
 * @brief 检查迭代器是否有下一个条目
 * 
 * @param iterator 迭代器
 * @return 如果有下一个条目返回true，否则返回false
 */
bool CN_hash_table_iterator_has_next(const Stru_CN_HashTableIterator_t* iterator);

/**
 * @brief 获取下一个条目
 * 
 * @param iterator 迭代器
 * @param key 输出参数，接收键指针（可为NULL）
 * @param value 输出参数，接收值指针（可为NULL）
 * @return 如果有下一个条目返回true，否则返回false
 */
bool CN_hash_table_iterator_next(Stru_CN_HashTableIterator_t* iterator,
                                 void** key, void** value);

/**
 * @brief 获取当前条目
 * 
 * @param iterator 迭代器
 * @param key 输出参数，接收键指针（可为NULL）
 * @param value 输出参数，接收值指针（可为NULL）
 * @return 如果有当前条目返回true，否则返回false
 */
bool CN_hash_table_iterator_current(const Stru_CN_HashTableIterator_t* iterator,
                                    void** key, void** value);

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 默认字符串哈希函数
 * 
 * @param key 字符串键
 * @param table_size 哈希表大小
 * @return 哈希值
 */
size_t CN_hash_table_default_string_hash(const void* key, size_t table_size);

/**
 * @brief 默认整数哈希函数
 * 
 * @param key 整数键
 * @param table_size 哈希表大小
 * @return 哈希值
 */
size_t CN_hash_table_default_int_hash(const void* key, size_t table_size);

/**
 * @brief 默认指针哈希函数
 * 
 * @param key 指针键
 * @param table_size 哈希表大小
 * @return 哈希值
 */
size_t CN_hash_table_default_ptr_hash(const void* key, size_t table_size);

/**
 * @brief 转储哈希表信息到控制台（调试用）
 * 
 * @param table 哈希表
 */
void CN_hash_table_dump(const Stru_CN_HashTable_t* table);

/**
 * @brief 统计哈希表性能信息
 * 
 * @param table 哈希表
 * @param avg_probe_length 输出参数，平均探测长度
 * @param max_probe_length 输出参数，最大探测长度
 * @param empty_buckets 输出参数，空桶数量
 */
void CN_hash_table_stats(const Stru_CN_HashTable_t* table,
                         double* avg_probe_length,
                         size_t* max_probe_length,
                         size_t* empty_buckets);

#endif // CN_HASH_TABLE_H
