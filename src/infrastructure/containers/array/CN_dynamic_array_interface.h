/**
 * @file CN_dynamic_array_interface.h
 * @brief 动态数组模块接口定义
 * 
 * 定义动态数组的抽象接口，支持不同的实现和测试桩。
 * 遵循CN_Language项目的接口设计规范。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_DYNAMIC_ARRAY_INTERFACE_H
#define CN_DYNAMIC_ARRAY_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 动态数组错误码枚举
 * 
 * 定义动态数组操作可能返回的错误码。
 */
enum Eum_DynamicArrayError
{
    Eum_DYNAMIC_ARRAY_SUCCESS = 0,          /**< 操作成功 */
    Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER,   /**< 空指针错误 */
    Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY,  /**< 内存不足错误 */
    Eum_DYNAMIC_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS, /**< 索引越界错误 */
    Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE,   /**< 无效大小错误 */
    Eum_DYNAMIC_ARRAY_ERROR_ITEM_NOT_FOUND, /**< 元素未找到错误 */
    Eum_DYNAMIC_ARRAY_ERROR_INTERNAL        /**< 内部错误 */
};

/**
 * @brief 动态数组迭代器回调函数类型
 * 
 * @param item 当前元素指针
 * @param index 元素索引
 * @param user_data 用户数据
 * @return 是否继续迭代（true继续，false停止）
 */
typedef bool (*F_DynamicArrayIterator_t)(void* item, size_t index, void* user_data);

/**
 * @brief 动态数组比较函数类型
 * 
 * @param item1 第一个元素指针
 * @param item2 第二个元素指针
 * @return 比较结果：<0表示item1<item2，=0表示相等，>0表示item1>item2
 */
typedef int (*F_DynamicArrayCompare_t)(const void* item1, const void* item2);

/**
 * @brief 动态数组抽象接口
 * 
 * 定义动态数组的抽象接口，支持不同的实现。
 * 遵循接口隔离原则，为不同客户端提供专用接口。
 */
typedef struct Stru_DynamicArrayInterface_t
{
    /* 创建和销毁 */
    
    /**
     * @brief 创建动态数组
     * 
     * @param item_size 每个元素的大小（字节）
     * @param initial_capacity 初始容量（0表示使用默认值）
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 动态数组句柄，失败返回NULL
     */
    void* (*create)(size_t item_size, size_t initial_capacity, enum Eum_DynamicArrayError* error_code);
    
    /**
     * @brief 销毁动态数组
     * 
     * @param array 动态数组句柄
     * @param error_code 输出参数，错误码（可为NULL）
     */
    void (*destroy)(void* array, enum Eum_DynamicArrayError* error_code);
    
    /* 基本操作 */
    
    /**
     * @brief 向数组末尾添加元素
     * 
     * @param array 动态数组句柄
     * @param item 要添加的元素指针
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*push)(void* array, const void* item, enum Eum_DynamicArrayError* error_code);
    
    /**
     * @brief 获取指定索引处的元素
     * 
     * @param array 动态数组句柄
     * @param index 元素索引
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 元素指针，失败返回NULL
     */
    void* (*get)(void* array, size_t index, enum Eum_DynamicArrayError* error_code);
    
    /**
     * @brief 设置指定索引处的元素
     * 
     * @param array 动态数组句柄
     * @param index 元素索引
     * @param item 新元素指针
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*set)(void* array, size_t index, const void* item, enum Eum_DynamicArrayError* error_code);
    
    /**
     * @brief 移除指定索引处的元素
     * 
     * @param array 动态数组句柄
     * @param index 要移除的元素索引
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*remove)(void* array, size_t index, enum Eum_DynamicArrayError* error_code);
    
    /* 查询操作 */
    
    /**
     * @brief 获取数组长度
     * 
     * @param array 动态数组句柄
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 数组长度，失败返回0
     */
    size_t (*length)(void* array, enum Eum_DynamicArrayError* error_code);
    
    /**
     * @brief 获取数组容量
     * 
     * @param array 动态数组句柄
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 数组容量，失败返回0
     */
    size_t (*capacity)(void* array, enum Eum_DynamicArrayError* error_code);
    
    /**
     * @brief 检查数组是否为空
     * 
     * @param array 动态数组句柄
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否为空
     */
    bool (*is_empty)(void* array, enum Eum_DynamicArrayError* error_code);
    
    /* 数组操作 */
    
    /**
     * @brief 清空数组
     * 
     * @param array 动态数组句柄
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*clear)(void* array, enum Eum_DynamicArrayError* error_code);
    
    /**
     * @brief 调整数组容量
     * 
     * @param array 动态数组句柄
     * @param new_capacity 新的容量
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*resize)(void* array, size_t new_capacity, enum Eum_DynamicArrayError* error_code);
    
    /**
     * @brief 查找元素
     * 
     * @param array 动态数组句柄
     * @param item 要查找的元素指针
     * @param compare 比较函数（NULL表示直接比较内存）
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 元素索引，未找到返回SIZE_MAX
     */
    size_t (*find)(void* array, const void* item, F_DynamicArrayCompare_t compare, 
                   enum Eum_DynamicArrayError* error_code);
    
    /**
     * @brief 批量添加元素
     * 
     * @param array 动态数组句柄
     * @param items 元素数组指针
     * @param count 元素数量
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*push_batch)(void* array, const void* items, size_t count, 
                       enum Eum_DynamicArrayError* error_code);
    
    /* 迭代操作 */
    
    /**
     * @brief 遍历数组
     * 
     * @param array 动态数组句柄
     * @param iterator 迭代器回调函数
     * @param user_data 用户数据
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功完成遍历
     */
    bool (*foreach)(void* array, F_DynamicArrayIterator_t iterator, void* user_data,
                    enum Eum_DynamicArrayError* error_code);
    
    /* 内存管理 */
    
    /**
     * @brief 获取内存使用统计
     * 
     * @param array 动态数组句柄
     * @param total_bytes 输出参数，总字节数（可为NULL）
     * @param used_bytes 输出参数，已使用字节数（可为NULL）
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*get_memory_stats)(void* array, size_t* total_bytes, size_t* used_bytes,
                             enum Eum_DynamicArrayError* error_code);
    
    /**
     * @brief 压缩数组（释放未使用的内存）
     * 
     * @param array 动态数组句柄
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*shrink_to_fit)(void* array, enum Eum_DynamicArrayError* error_code);
    
    /* 高级操作（新增功能） */
    
    /**
     * @brief 排序数组
     * 
     * @param array 动态数组句柄
     * @param compare 比较函数
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*sort)(void* array, F_DynamicArrayCompare_t compare,
                 enum Eum_DynamicArrayError* error_code);
    
    /**
     * @brief 反转数组
     * 
     * @param array 动态数组句柄
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*reverse)(void* array, enum Eum_DynamicArrayError* error_code);
    
    /**
     * @brief 映射函数（对每个元素应用函数）
     * 
     * @param array 动态数组句柄
     * @param mapper 映射函数
     * @param user_data 用户数据
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*map)(void* array, void (*mapper)(void* item, void* user_data), void* user_data,
                enum Eum_DynamicArrayError* error_code);
    
    /**
     * @brief 过滤数组（根据条件保留元素）
     * 
     * @param array 动态数组句柄
     * @param filter 过滤函数（返回true保留元素）
     * @param user_data 用户数据
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 是否成功
     */
    bool (*filter)(void* array, bool (*filter)(void* item, void* user_data), void* user_data,
                   enum Eum_DynamicArrayError* error_code);
    
    /**
     * @brief 获取子数组（切片）
     * 
     * @param array 动态数组句柄
     * @param start 起始索引
     * @param end 结束索引（不包含）
     * @param error_code 输出参数，错误码（可为NULL）
     * @return 新的动态数组句柄，失败返回NULL
     */
    void* (*slice)(void* array, size_t start, size_t end,
                   enum Eum_DynamicArrayError* error_code);
    
} Stru_DynamicArrayInterface_t;

/**
 * @brief 获取默认动态数组实现接口
 * 
 * @return 指向默认动态数组接口的指针
 */
const Stru_DynamicArrayInterface_t* F_get_default_dynamic_array_interface(void);

#ifdef __cplusplus
}
#endif

#endif // CN_DYNAMIC_ARRAY_INTERFACE_H
