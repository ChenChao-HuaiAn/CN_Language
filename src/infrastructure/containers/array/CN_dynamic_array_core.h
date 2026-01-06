/**
 * @file CN_dynamic_array_core.h
 * @brief 动态数组核心功能头文件
 * 
 * 提供动态数组模块的核心功能接口声明。
 * 包括创建、销毁、基本操作和查询功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_DYNAMIC_ARRAY_CORE_H
#define CN_DYNAMIC_ARRAY_CORE_H

#include <stddef.h>
#include <stdbool.h>

// 前向声明
typedef struct Stru_DynamicArray_t Stru_DynamicArray_t;
enum Eum_DynamicArrayError;

#ifdef __cplusplus
extern "C" {
#endif

/* 核心创建和销毁函数 */

/**
 * @brief 创建动态数组（带错误码）
 * 
 * 分配并初始化一个新的动态数组，可指定初始容量。
 * 
 * @param item_size 每个元素的大小（字节）
 * @param initial_capacity 初始容量（0表示使用默认值）
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 指向新创建的动态数组的指针，失败返回NULL
 */
Stru_DynamicArray_t* F_dynamic_array_create_ex(size_t item_size, size_t initial_capacity, 
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
void F_dynamic_array_destroy(Stru_DynamicArray_t* array);

/* 核心元素操作函数 */

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

/* 核心查询函数 */

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

#ifdef __cplusplus
}
#endif

#endif // CN_DYNAMIC_ARRAY_CORE_H
