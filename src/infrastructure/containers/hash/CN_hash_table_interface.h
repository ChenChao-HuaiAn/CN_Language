/**
 * @file CN_hash_table_interface.h
 * @brief 哈希表模块接口定义
 * 
 * 定义哈希表的抽象接口，支持不同的实现和测试桩。
 * 遵循CN_Language项目的接口设计规范。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_HASH_TABLE_INTERFACE_H
#define CN_HASH_TABLE_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 哈希表错误码枚举
 * 
 * 定义哈希表操作可能返回的错误码。
 */
enum Eum_HashTableError
{
    Eum_HASH_TABLE_SUCCESS = 0,          /**< 操作成功 */
    Eum_HASH_TABLE_ERROR_NULL_POINTER,   /**< 空指针错误 */
    Eum_HASH_TABLE_ERROR_OUT_OF_MEMORY,  /**< 内存不足错误 */
    Eum_HASH_TABLE_ERROR_KEY_NOT_FOUND,  /**< 键未找到错误 */
    Eum_HASH_TABLE_ERROR_INVALID_SIZE,   /**< 无效大小错误 */
    Eum_HASH_TABLE_ERROR_INVALID_HASH_FUNCTION, /**< 无效哈希函数错误 */
    Eum_HASH_TABLE_ERROR_INVALID_COMPARE_FUNCTION, /**< 无效比较函数错误 */
    Eum_HASH_TABLE_ERROR_INTERNAL        /**< 内部错误 */
};

/**
 * @brief 哈希函数类型
 * 
 * @param key 键指针
 * @return 哈希值
 */
typedef uint64_t (*F_HashFunction_t)(void* key);

/**
 * @brief 键比较函数类型
 * 
 * @param key1 第一个键指针
 * @param key2 第二个键指针
 * @return 比较结果：0表示相等，非0表示不相等
 */
typedef int (*F_KeyCompare_t)(void* key1, void* key2);

/**
 * @brief 哈希表迭代器回调函数类型
 * 
 * @param key 当前键指针
 * @param value 当前值指针
 * @param user_data 用户数据
 * @return 是否继续迭代（true继续，false停止）
 */
typedef bool (*F_HashTableIterator_t)(void* key, void* value, void* user_data);

/**
 * @brief 哈希表抽象接口
 * 
 * 定义哈希表的抽象接口，支持不同的实现。
 * 遵循接口隔离原则，为不同客户端提供专用接口。
 */
typedef struct Stru_HashTableInterface_t
{
    /* 创建和销毁 */
    
    /**
     * @brief 创建哈希表
     * 
     * @param key_size 键的大小（字节）
     * @param value_size 值的大小（字节）
     * @param hash_function 哈希函数
     * @param compare_function 键比较函数
     * @param initial_capacity 初始容量（0表示使用默认值）
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 哈希表句柄，失败返回NULL
     */
    void* (*create)(size_t key_size, size_t value_size, 
                   F_HashFunction_t hash_function, F_KeyCompare_t compare_function,
                   size_t initial_capacity, enum Eum_HashTableError* error_code);
    
    /**
     * @brief 销毁哈希表
     * 
     * @param table 哈希表句柄
     * @param error_code 输出参数，错误码（可为NULL）
     */
    void (*destroy)(void* table, enum Eum_HashTableError* error_code);
    
    /* 基本操作 */
    
    /**
     * @brief 向哈希表插入键值对
     * 
     * @param table 哈希表句柄
     * @param key 键指针
     * @param value 值指针
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     * 
     * @note 如果键已存在，会更新对应的值
     */
    bool (*put)(void* table, void* key, void* value, enum Eum_HashTableError* error_code);
    
    /**
     * @brief 从哈希表获取值
     * 
     * @param table 哈希表句柄
     * @param key 键指针
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 指向值的指针，键不存在返回NULL
     */
    void* (*get)(void* table, void* key, enum Eum_HashTableError* error_code);
    
    /**
     * @brief 从哈希表删除键值对
     * 
     * @param table 哈希表句柄
     * @param key 键指针
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*remove)(void* table, void* key, enum Eum_HashTableError* error_code);
    
    /**
     * @brief 检查键是否存在
     * 
     * @param table 哈希表句柄
     * @param key 键指针
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 键存在返回true，否则返回false
     */
    bool (*contains)(void* table, void* key, enum Eum_HashTableError* error_code);
    
    /* 查询操作 */
    
    /**
     * @brief 获取哈希表大小
     * 
     * @param table 哈希表句柄
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 哈希表大小（键值对数量），失败返回0
     */
    size_t (*size)(void* table, enum Eum_HashTableError* error_code);
    
    /**
     * @brief 获取哈希表容量
     * 
     * @param table 哈希表句柄
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 哈希表容量（桶的数量），失败返回0
     */
    size_t (*capacity)(void* table, enum Eum_HashTableError* error_code);
    
    /**
     * @brief 获取负载因子
     * 
     * @param table 哈希表句柄
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 负载因子（size/capacity），失败返回0.0
     */
    double (*load_factor)(void* table, enum Eum_HashTableError* error_code);
    
    /**
     * @brief 检查哈希表是否为空
     * 
     * @param table 哈希表句柄
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否为空
     */
    bool (*is_empty)(void* table, enum Eum_HashTableError* error_code);
    
    /* 哈希表操作 */
    
    /**
     * @brief 清空哈希表
     * 
     * @param table 哈希表句柄
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*clear)(void* table, enum Eum_HashTableError* error_code);
    
    /**
     * @brief 调整哈希表容量
     * 
     * @param table 哈希表句柄
     * @param new_capacity 新的容量大小
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*resize)(void* table, size_t new_capacity, enum Eum_HashTableError* error_code);
    
    /**
     * @brief 遍历哈希表
     * 
     * @param table 哈希表句柄
     * @param iterator 迭代器回调函数
     * @param user_data 用户数据
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功完成遍历
     */
    bool (*foreach)(void* table, F_HashTableIterator_t iterator, void* user_data,
                    enum Eum_HashTableError* error_code);
    
    /* 内存管理 */
    
    /**
     * @brief 获取内存使用统计
     * 
     * @param table 哈希表句柄
     * @param total_bytes 输出参数，总字节数（可为NULL）
     * @param used_bytes 输出参数，已使用字节数（可为NULL）
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*get_memory_stats)(void* table, size_t* total_bytes, size_t* used_bytes,
                             enum Eum_HashTableError* error_code);
    
    /**
     * @brief 压缩哈希表（释放未使用的内存）
     * 
     * @param table 哈希表句柄
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*shrink_to_fit)(void* table, enum Eum_HashTableError* error_code);
    
    /* 性能统计 */
    
    /**
     * @brief 获取性能统计
     * 
     * @param table 哈希表句柄
     * @param collision_count 输出参数，冲突次数（可为NULL）
     * @param max_chain_length 输出参数，最大链长度（可为NULL）
     * @param avg_chain_length 输出参数，平均链长度（可为NULL）
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*get_performance_stats)(void* table, size_t* collision_count, 
                                  size_t* max_chain_length, double* avg_chain_length,
                                  enum Eum_HashTableError* error_code);
    
} Stru_HashTableInterface_t;

/**
 * @brief 获取默认哈希表实现接口
 * 
 * @return 指向默认哈希表接口的指针
 */
const Stru_HashTableInterface_t* F_get_default_hash_table_interface(void);

#ifdef __cplusplus
}
#endif

#endif // CN_HASH_TABLE_INTERFACE_H
