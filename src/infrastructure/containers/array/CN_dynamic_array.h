/**
 * @file CN_dynamic_array.h
 * @brief 动态数组模块头文件
 * 
 * 提供动态数组数据结构的定义和接口声明。
 * 动态数组支持自动扩容、随机访问和动态大小调整。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_DYNAMIC_ARRAY_H
#define CN_DYNAMIC_ARRAY_H

#include <stddef.h>
#include <stdbool.h>
#include "CN_dynamic_array_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 动态数组结构体
 * 
 * 动态数组是一种可以自动调整大小的数组数据结构。
 * 它支持在数组末尾高效地添加和删除元素。
 */
typedef struct Stru_DynamicArray_t
{
    void** items;           /**< 元素指针数组 */
    size_t capacity;        /**< 当前分配的容量 */
    size_t length;          /**< 当前元素数量 */
    size_t item_size;       /**< 每个元素的大小（字节） */
} Stru_DynamicArray_t;

/* 模块化API结构说明 */
/*
 * 动态数组模块已重构为模块化结构，包含以下子模块：
 * 1. CN_dynamic_array_core.h/.c - 核心功能（创建、销毁、基本操作）
 * 2. CN_dynamic_array_operations.h/.c - 高级操作（查找、批量操作、迭代等）
 * 3. CN_dynamic_array_utils.h/.c - 工具函数（错误处理、验证、辅助函数）
 * 
 * 以下API提供向后兼容的接口，内部调用相应的模块化实现。
 */

/* 基础API（向后兼容） */

/**
 * @brief 创建动态数组
 * 
 * 分配并初始化一个新的动态数组。
 * 
 * @param item_size 每个元素的大小（字节）
 * @return 指向新创建的动态数组的指针，失败返回NULL
 * 
 * @note 初始容量为16个元素，当数组满时会自动扩容
 */
Stru_DynamicArray_t* F_create_dynamic_array(size_t item_size);

/**
 * @brief 创建动态数组（带初始容量）
 * 
 * 分配并初始化一个新的动态数组，可指定初始容量。
 * 
 * @param item_size 每个元素的大小（字节）
 * @param initial_capacity 初始容量（0表示使用默认值）
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 指向新创建的动态数组的指针，失败返回NULL
 */
Stru_DynamicArray_t* F_create_dynamic_array_ex(size_t item_size, size_t initial_capacity, 
                                               enum Eum_DynamicArrayError* error_code);

/**
 * @brief 销毁动态数组
 * 
 * 释放动态数组占用的所有内存。
 * 
 * @param array 要销毁的动态数组指针
 * 
 * @note 如果array为NULL，函数不执行任何操作
 */
void F_destroy_dynamic_array(Stru_DynamicArray_t* array);

/**
 * @brief 向动态数组添加元素
 * 
 * 在动态数组的末尾添加一个新元素。
 * 
 * @param array 动态数组指针
 * @param item 要添加的元素指针
 * @return 添加成功返回true，失败返回false
 * 
 * @note 如果数组已满，会自动扩容为原来的2倍
 */
bool F_dynamic_array_push(Stru_DynamicArray_t* array, const void* item);

/**
 * @brief 向动态数组添加元素（带错误码）
 * 
 * 在动态数组的末尾添加一个新元素，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param item 要添加的元素指针
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 添加成功返回true，失败返回false
 */
bool F_dynamic_array_push_ex(Stru_DynamicArray_t* array, const void* item, 
                             enum Eum_DynamicArrayError* error_code);

/**
 * @brief 从动态数组获取元素
 * 
 * 获取指定索引处的元素。
 * 
 * @param array 动态数组指针
 * @param index 元素索引（0-based）
 * @return 指向元素的指针，索引无效返回NULL
 */
void* F_dynamic_array_get(Stru_DynamicArray_t* array, size_t index);

/**
 * @brief 从动态数组获取元素（带错误码）
 * 
 * 获取指定索引处的元素，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param index 元素索引（0-based）
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 指向元素的指针，失败返回NULL
 */
void* F_dynamic_array_get_ex(Stru_DynamicArray_t* array, size_t index, 
                             enum Eum_DynamicArrayError* error_code);

/**
 * @brief 设置动态数组元素
 * 
 * 设置指定索引处的元素。
 * 
 * @param array 动态数组指针
 * @param index 元素索引
 * @param item 新元素指针
 * @return 设置成功返回true，失败返回false
 */
bool F_dynamic_array_set(Stru_DynamicArray_t* array, size_t index, const void* item);

/**
 * @brief 设置动态数组元素（带错误码）
 * 
 * 设置指定索引处的元素，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param index 元素索引
 * @param item 新元素指针
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 设置成功返回true，失败返回false
 */
bool F_dynamic_array_set_ex(Stru_DynamicArray_t* array, size_t index, const void* item,
                            enum Eum_DynamicArrayError* error_code);

/**
 * @brief 从动态数组移除元素
 * 
 * 移除指定索引处的元素。
 * 
 * @param array 动态数组指针
 * @param index 要移除的元素索引
 * @return 移除成功返回true，索引无效返回false
 * 
 * @note 移除元素后，后面的元素会向前移动
 */
bool F_dynamic_array_remove(Stru_DynamicArray_t* array, size_t index);

/**
 * @brief 从动态数组移除元素（带错误码）
 * 
 * 移除指定索引处的元素，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param index 要移除的元素索引
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 移除成功返回true，失败返回false
 */
bool F_dynamic_array_remove_ex(Stru_DynamicArray_t* array, size_t index,
                               enum Eum_DynamicArrayError* error_code);

/**
 * @brief 获取动态数组长度
 * 
 * 返回动态数组中当前元素的数量。
 * 
 * @param array 动态数组指针
 * @return 数组长度，如果array为NULL返回0
 */
size_t F_dynamic_array_length(Stru_DynamicArray_t* array);

/**
 * @brief 获取动态数组长度（带错误码）
 * 
 * 返回动态数组中当前元素的数量，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 数组长度，失败返回0
 */
size_t F_dynamic_array_length_ex(Stru_DynamicArray_t* array, 
                                 enum Eum_DynamicArrayError* error_code);

/**
 * @brief 获取动态数组容量
 * 
 * 返回动态数组当前分配的容量。
 * 
 * @param array 动态数组指针
 * @return 数组容量，如果array为NULL返回0
 */
size_t F_dynamic_array_capacity(Stru_DynamicArray_t* array);

/**
 * @brief 获取动态数组容量（带错误码）
 * 
 * 返回动态数组当前分配的容量，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 数组容量，失败返回0
 */
size_t F_dynamic_array_capacity_ex(Stru_DynamicArray_t* array,
                                   enum Eum_DynamicArrayError* error_code);

/**
 * @brief 检查动态数组是否为空
 * 
 * 检查动态数组是否为空（长度为0）。
 * 
 * @param array 动态数组指针
 * @return 如果数组为空返回true，否则返回false
 */
bool F_dynamic_array_is_empty(Stru_DynamicArray_t* array);

/**
 * @brief 检查动态数组是否为空（带错误码）
 * 
 * 检查动态数组是否为空，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 如果数组为空返回true，否则返回false
 */
bool F_dynamic_array_is_empty_ex(Stru_DynamicArray_t* array,
                                 enum Eum_DynamicArrayError* error_code);

/**
 * @brief 清空动态数组
 * 
 * 移除动态数组中的所有元素，但不释放数组本身。
 * 
 * @param array 动态数组指针
 * 
 * @note 清空后数组长度变为0，但容量保持不变
 */
void F_dynamic_array_clear(Stru_DynamicArray_t* array);

/**
 * @brief 清空动态数组（带错误码）
 * 
 * 移除动态数组中的所有元素，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 清空成功返回true，失败返回false
 */
bool F_dynamic_array_clear_ex(Stru_DynamicArray_t* array,
                              enum Eum_DynamicArrayError* error_code);

/**
 * @brief 调整动态数组容量
 * 
 * 调整动态数组的容量到指定大小。
 * 
 * @param array 动态数组指针
 * @param new_capacity 新的容量大小
 * @return 调整成功返回true，失败返回false
 * 
 * @note 如果new_capacity小于当前长度，会截断数组
 */
bool F_dynamic_array_resize(Stru_DynamicArray_t* array, size_t new_capacity);

/**
 * @brief 调整动态数组容量（带错误码）
 * 
 * 调整动态数组的容量到指定大小，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param new_capacity 新的容量大小
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 调整成功返回true，失败返回false
 */
bool F_dynamic_array_resize_ex(Stru_DynamicArray_t* array, size_t new_capacity,
                               enum Eum_DynamicArrayError* error_code);

/**
 * @brief 查找动态数组元素
 * 
 * 在动态数组中查找指定元素。
 * 
 * @param array 动态数组指针
 * @param item 要查找的元素指针
 * @param compare 比较函数（NULL表示直接比较内存）
 * @return 元素索引，未找到返回SIZE_MAX
 */
size_t F_dynamic_array_find(Stru_DynamicArray_t* array, const void* item,
                            F_DynamicArrayCompare_t compare);

/**
 * @brief 查找动态数组元素（带错误码）
 * 
 * 在动态数组中查找指定元素，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param item 要查找的元素指针
 * @param compare 比较函数（NULL表示直接比较内存）
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 元素索引，未找到返回SIZE_MAX
 */
size_t F_dynamic_array_find_ex(Stru_DynamicArray_t* array, const void* item,
                               F_DynamicArrayCompare_t compare,
                               enum Eum_DynamicArrayError* error_code);

/**
 * @brief 批量添加元素到动态数组
 * 
 * 批量添加多个元素到动态数组末尾。
 * 
 * @param array 动态数组指针
 * @param items 元素数组指针
 * @param count 元素数量
 * @return 添加成功返回true，失败返回false
 */
bool F_dynamic_array_push_batch(Stru_DynamicArray_t* array, const void* items, size_t count);

/**
 * @brief 批量添加元素到动态数组（带错误码）
 * 
 * 批量添加多个元素到动态数组末尾，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param items 元素数组指针
 * @param count 元素数量
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 添加成功返回true，失败返回false
 */
bool F_dynamic_array_push_batch_ex(Stru_DynamicArray_t* array, const void* items, size_t count,
                                   enum Eum_DynamicArrayError* error_code);

/**
 * @brief 遍历动态数组
 * 
 * 对动态数组中的每个元素执行指定的操作。
 * 
 * @param array 动态数组指针
 * @param callback 回调函数，接收元素指针和用户数据
 * @param user_data 传递给回调函数的用户数据
 */
void F_dynamic_array_foreach(Stru_DynamicArray_t* array, 
                            void (*callback)(void* item, void* user_data),
                            void* user_data);

/**
 * @brief 遍历动态数组（带索引和错误码）
 * 
 * 对动态数组中的每个元素执行指定的操作，包含索引和错误处理。
 * 
 * @param array 动态数组指针
 * @param iterator 迭代器回调函数
 * @param user_data 用户数据
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 是否成功完成遍历
 */
bool F_dynamic_array_foreach_ex(Stru_DynamicArray_t* array, 
                               F_DynamicArrayIterator_t iterator, void* user_data,
                               enum Eum_DynamicArrayError* error_code);

/**
 * @brief 获取动态数组内存使用统计
 * 
 * 获取动态数组的内存使用情况。
 * 
 * @param array 动态数组指针
 * @param total_bytes 输出参数，总字节数（可为NULL）
 * @param used_bytes 输出参数，已使用字节数（可为NULL）
 * @return 获取成功返回true，失败返回false
 */
bool F_dynamic_array_get_memory_stats(Stru_DynamicArray_t* array, 
                                      size_t* total_bytes, size_t* used_bytes);

/**
 * @brief 获取动态数组内存使用统计（带错误码）
 * 
 * 获取动态数组的内存使用情况，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param total_bytes 输出参数，总字节数（可为NULL）
 * @param used_bytes 输出参数，已使用字节数（可为NULL）
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 获取成功返回true，失败返回false
 */
bool F_dynamic_array_get_memory_stats_ex(Stru_DynamicArray_t* array, 
                                         size_t* total_bytes, size_t* used_bytes,
                                         enum Eum_DynamicArrayError* error_code);

/**
 * @brief 压缩动态数组（释放未使用的内存）
 * 
 * 压缩动态数组，释放未使用的内存。
 * 
 * @param array 动态数组指针
 * @return 压缩成功返回true，失败返回false
 */
bool F_dynamic_array_shrink_to_fit(Stru_DynamicArray_t* array);

/**
 * @brief 压缩动态数组（带错误码）
 * 
 * 压缩动态数组，释放未使用的内存，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 压缩成功返回true，失败返回false
 */
bool F_dynamic_array_shrink_to_fit_ex(Stru_DynamicArray_t* array,
                                      enum Eum_DynamicArrayError* error_code);

/**
 * @brief 获取默认动态数组接口
 * 
 * 获取默认的动态数组接口实现。
 * 
 * @return 指向默认动态数组接口的指针
 */
const Stru_DynamicArrayInterface_t* F_get_default_dynamic_array_interface(void);

#ifdef __cplusplus
}
#endif

#endif // CN_DYNAMIC_ARRAY_H
