/**
 * @file CN_dynamic_array_operations.h
 * @brief 动态数组操作功能头文件
 * 
 * 提供动态数组模块的高级操作功能接口声明。
 * 包括数组操作、批量操作、迭代操作和内存管理功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_DYNAMIC_ARRAY_OPERATIONS_H
#define CN_DYNAMIC_ARRAY_OPERATIONS_H

#include <stddef.h>
#include <stdbool.h>

// 前向声明
typedef struct Stru_DynamicArray_t Stru_DynamicArray_t;
enum Eum_DynamicArrayError;
typedef bool (*F_DynamicArrayIterator_t)(void* item, size_t index, void* user_data);
typedef int (*F_DynamicArrayCompare_t)(const void* item1, const void* item2);

#ifdef __cplusplus
extern "C" {
#endif

/* 数组操作函数 */

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

/* 批量操作函数 */

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

/* 迭代操作函数 */

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

/* 内存管理函数 */

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

#ifdef __cplusplus
}
#endif

#endif // CN_DYNAMIC_ARRAY_OPERATIONS_H
