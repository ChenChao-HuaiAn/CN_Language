/**
 * @file CN_dynamic_array_core.c
 * @brief 动态数组核心功能实现文件
 * 
 * 实现动态数组模块的核心功能。
 * 包括创建、销毁、基本操作和查询功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_dynamic_array.h"
#include "CN_dynamic_array_core.h"
#include "CN_dynamic_array_utils.h"
#include <stdlib.h>
#include <string.h>

// 默认初始容量
#define CN_DYNAMIC_ARRAY_INITIAL_CAPACITY 16
// 扩容因子
#define CN_DYNAMIC_ARRAY_GROWTH_FACTOR 2
// 最小容量（防止容量为0）
#define CN_DYNAMIC_ARRAY_MIN_CAPACITY 4

Stru_DynamicArray_t* F_dynamic_array_create_ex(size_t item_size, size_t initial_capacity, 
                                               enum Eum_DynamicArrayError* error_code)
{
    if (item_size == 0)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE);
        return NULL;
    }
    
    // 使用默认初始容量如果指定为0
    if (initial_capacity == 0)
    {
        initial_capacity = CN_DYNAMIC_ARRAY_INITIAL_CAPACITY;
    }
    
    // 确保最小容量
    if (initial_capacity < CN_DYNAMIC_ARRAY_MIN_CAPACITY)
    {
        initial_capacity = CN_DYNAMIC_ARRAY_MIN_CAPACITY;
    }
    
    // 分配动态数组结构体
    Stru_DynamicArray_t* array = (Stru_DynamicArray_t*)malloc(sizeof(Stru_DynamicArray_t));
    if (array == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    
    // 初始化字段
    array->capacity = initial_capacity;
    array->length = 0;
    array->item_size = item_size;
    
    // 分配初始元素数组
    array->items = (void**)malloc(initial_capacity * sizeof(void*));
    if (array->items == NULL)
    {
        free(array);
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    
    // 初始化指针为NULL
    for (size_t i = 0; i < array->capacity; i++)
    {
        array->items[i] = NULL;
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return array;
}

void F_dynamic_array_destroy(Stru_DynamicArray_t* array)
{
    if (array == NULL)
    {
        return;
    }
    
    // 释放所有元素
    for (size_t i = 0; i < array->length; i++)
    {
        if (array->items[i] != NULL)
        {
            free(array->items[i]);
            array->items[i] = NULL;
        }
    }
    
    // 释放元素数组
    if (array->items != NULL)
    {
        free(array->items);
        array->items = NULL;
    }
    
    // 释放动态数组结构体
    free(array);
}

bool F_dynamic_array_push_ex(Stru_DynamicArray_t* array, const void* item, 
                             enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code) || item == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    // 确保有足够容量
    if (!F_dynamic_array_ensure_capacity(array, error_code))
    {
        return false;
    }
    
    // 分配新元素内存并复制数据
    void* new_item = F_dynamic_array_copy_item_data(array, item, error_code);
    if (new_item == NULL)
    {
        return false;
    }
    
    array->items[array->length] = new_item;
    array->length++;
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

void* F_dynamic_array_get_ex(Stru_DynamicArray_t* array, size_t index, 
                             enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_index(array, index, error_code))
    {
        return NULL;
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return array->items[index];
}

bool F_dynamic_array_set_ex(Stru_DynamicArray_t* array, size_t index, const void* item,
                            enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_index(array, index, error_code) || item == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    // 释放旧元素
    if (array->items[index] != NULL)
    {
        free(array->items[index]);
    }
    
    // 分配新元素内存并复制数据
    void* new_item = F_dynamic_array_copy_item_data(array, item, error_code);
    if (new_item == NULL)
    {
        array->items[index] = NULL;
        return false;
    }
    
    array->items[index] = new_item;
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

bool F_dynamic_array_remove_ex(Stru_DynamicArray_t* array, size_t index,
                               enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_index(array, index, error_code))
    {
        return false;
    }
    
    // 释放要移除的元素
    if (array->items[index] != NULL)
    {
        free(array->items[index]);
    }
    
    // 将后面的元素向前移动
    for (size_t i = index; i < array->length - 1; i++)
    {
        array->items[i] = array->items[i + 1];
    }
    
    // 最后一个元素置为NULL
    array->items[array->length - 1] = NULL;
    array->length--;
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

size_t F_dynamic_array_length_ex(Stru_DynamicArray_t* array, 
                                 enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
    {
        return 0;
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return array->length;
}

size_t F_dynamic_array_capacity_ex(Stru_DynamicArray_t* array,
                                   enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
    {
        return 0;
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return array->capacity;
}

bool F_dynamic_array_is_empty_ex(Stru_DynamicArray_t* array,
                                 enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
    {
        return true;
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return (array->length == 0);
}
