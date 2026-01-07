/**
 * @file CN_dynamic_array_operations.c
 * @brief 动态数组操作功能实现文件
 * 
 * 实现动态数组模块的高级操作功能。
 * 包括数组操作、批量操作、迭代操作和内存管理功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_dynamic_array.h"
#include "CN_dynamic_array_operations.h"
#include "CN_dynamic_array_utils.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

// 默认初始容量
#define CN_DYNAMIC_ARRAY_INITIAL_CAPACITY 16
// 扩容因子
#define CN_DYNAMIC_ARRAY_GROWTH_FACTOR 2
// 最小容量（防止容量为0）
#define CN_DYNAMIC_ARRAY_MIN_CAPACITY 4

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
                              enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
    {
        return false;
    }
    
    // 释放所有元素但不释放数组
    for (size_t i = 0; i < array->length; i++)
    {
        if (array->items[i] != NULL)
        {
            free(array->items[i]);
            array->items[i] = NULL;
        }
    }
    
    array->length = 0;
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

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
                               enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
    {
        return false;
    }
    
    // 确保最小容量
    if (new_capacity < CN_DYNAMIC_ARRAY_MIN_CAPACITY)
    {
        new_capacity = CN_DYNAMIC_ARRAY_MIN_CAPACITY;
    }
    
    // 如果新容量小于当前长度，需要截断
    if (new_capacity < array->length)
    {
        // 释放被截断的元素
        for (size_t i = new_capacity; i < array->length; i++)
        {
            if (array->items[i] != NULL)
            {
                free(array->items[i]);
                array->items[i] = NULL;
            }
        }
        array->length = new_capacity;
    }
    
    // 调整数组容量
    void** new_items = (void**)realloc(array->items, new_capacity * sizeof(void*));
    if (new_items == NULL && new_capacity > 0)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY);
        return false;
    }
    
    array->items = new_items;
    array->capacity = new_capacity;
    
    // 初始化新分配的空间
    for (size_t i = array->length; i < array->capacity; i++)
    {
        array->items[i] = NULL;
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

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
                               enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code) || item == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return SIZE_MAX;
    }
    
    for (size_t i = 0; i < array->length; i++)
    {
        if (array->items[i] != NULL)
        {
            bool found = false;
            
            if (compare != NULL)
            {
                // 使用比较函数
                if (compare(array->items[i], item) == 0)
                {
                    found = true;
                }
            }
            else
            {
                // 直接比较内存
                if (memcmp(array->items[i], item, array->item_size) == 0)
                {
                    found = true;
                }
            }
            
            if (found)
            {
                F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
                return i;
            }
        }
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_ITEM_NOT_FOUND);
    return SIZE_MAX;
}

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
                                   enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code) || items == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    if (count == 0)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
        return true;
    }
    
    // 确保有足够容量
    size_t required_capacity = array->length + count;
    if (required_capacity > array->capacity)
    {
        // 计算新的容量
        size_t new_capacity = array->capacity;
        while (new_capacity < required_capacity)
        {
            new_capacity *= CN_DYNAMIC_ARRAY_GROWTH_FACTOR;
        }
        
        if (!F_dynamic_array_resize_ex(array, new_capacity, error_code))
        {
            return false;
        }
    }
    
    // 批量添加元素
    const char* src = (const char*)items;
    for (size_t i = 0; i < count; i++)
    {
        void* new_item = F_dynamic_array_copy_item_data(array, src + (i * array->item_size), error_code);
        if (new_item == NULL)
        {
            // 回滚：释放已添加的元素
            for (size_t j = 0; j < i; j++)
            {
                size_t index = array->length - count + j;
                if (array->items[index] != NULL)
                {
                    free(array->items[index]);
                    array->items[index] = NULL;
                }
            }
            array->length -= i;
            return false;
        }
        
        array->items[array->length] = new_item;
        array->length++;
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

/**
 * @brief 遍历动态数组（带索引和错误码）
 * 
 * 对动态数组中的每个元素执行指定的操作，包含索引和错误处理。
 * 
 * @param array 动态数组指针
 * @param iterator 迭代器回调函数
 * @param user_data 用户数据
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 是否成功完成遍历（如果迭代器提前停止返回false）
 */
bool F_dynamic_array_foreach_ex(Stru_DynamicArray_t* array, 
                               F_DynamicArrayIterator_t iterator, void* user_data,
                               enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code) || iterator == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    for (size_t i = 0; i < array->length; i++)
    {
        if (!iterator(array->items[i], i, user_data))
        {
            // 迭代器请求停止，遍历未完成
            F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
            return false;
        }
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

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
                                         enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
    {
        return false;
    }
    
    // 计算总字节数（结构体 + 指针数组 + 元素数据）
    size_t total = sizeof(Stru_DynamicArray_t) + 
                   (array->capacity * sizeof(void*)) + 
                   (array->length * array->item_size);
    
    // 计算已使用字节数（结构体 + 指针数组 + 元素数据）
    size_t used = sizeof(Stru_DynamicArray_t) + 
                  (array->capacity * sizeof(void*)) + 
                  (array->length * array->item_size);
    
    if (total_bytes != NULL)
    {
        *total_bytes = total;
    }
    
    if (used_bytes != NULL)
    {
        *used_bytes = used;
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

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
                                      enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
    {
        return false;
    }
    
    // 如果容量大于长度，调整容量到长度
    if (array->capacity > array->length)
    {
        return F_dynamic_array_resize_ex(array, array->length, error_code);
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}
