/**
 * @file CN_dynamic_array_utils.h
 * @brief 动态数组工具函数头文件
 * 
 * 提供动态数组模块的内部工具函数和辅助函数。
 * 这些函数主要用于内部实现，不直接暴露给用户。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_DYNAMIC_ARRAY_UTILS_H
#define CN_DYNAMIC_ARRAY_UTILS_H

#include <stddef.h>
#include <stdbool.h>

// 前向声明
typedef struct Stru_DynamicArray_t Stru_DynamicArray_t;
enum Eum_DynamicArrayError;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 设置错误码
 * 
 * 如果error_code指针不为NULL，则设置错误码。
 * 
 * @param error_code 错误码指针
 * @param code 错误码值
 */
void F_dynamic_array_set_error_code(enum Eum_DynamicArrayError* error_code, 
                                    enum Eum_DynamicArrayError code);

/**
 * @brief 验证动态数组指针
 * 
 * 验证动态数组指针是否有效。
 * 
 * @param array 动态数组指针
 * @param error_code 输出参数，错误码
 * @return 有效返回true，无效返回false
 */
bool F_dynamic_array_validate_array(Stru_DynamicArray_t* array, 
                                    enum Eum_DynamicArrayError* error_code);

/**
 * @brief 验证索引
 * 
 * 验证索引是否在有效范围内。
 * 
 * @param array 动态数组指针
 * @param index 索引
 * @param error_code 输出参数，错误码
 * @return 有效返回true，无效返回false
 */
bool F_dynamic_array_validate_index(Stru_DynamicArray_t* array, size_t index,
                                    enum Eum_DynamicArrayError* error_code);

/**
 * @brief 复制元素数据
 * 
 * 分配内存并复制元素数据。
 * 
 * @param array 动态数组指针
 * @param item 源元素指针
 * @param error_code 输出参数，错误码
 * @return 新分配的元素指针，失败返回NULL
 */
void* F_dynamic_array_copy_item_data(Stru_DynamicArray_t* array, const void* item,
                                     enum Eum_DynamicArrayError* error_code);

/**
 * @brief 确保数组有足够容量
 * 
 * 检查数组是否有足够容量容纳新元素，如果没有则扩容。
 * 
 * @param array 动态数组指针
 * @param error_code 输出参数，错误码
 * @return 容量足够或扩容成功返回true，失败返回false
 */
bool F_dynamic_array_ensure_capacity(Stru_DynamicArray_t* array,
                                     enum Eum_DynamicArrayError* error_code);

#ifdef __cplusplus
}
#endif

#endif // CN_DYNAMIC_ARRAY_UTILS_H
