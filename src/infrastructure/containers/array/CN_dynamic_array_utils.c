/**
 * @file CN_dynamic_array_utils.c
 * @brief 动态数组工具函数实现文件
 * 
 * 实现动态数组模块的内部工具函数和辅助函数。
 * 这些函数主要用于内部实现，不直接暴露给用户。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_dynamic_array.h"
#include "CN_dynamic_array_utils.h"
#include <stdlib.h>
#include <string.h>

// 默认初始容量
#define CN_DYNAMIC_ARRAY_INITIAL_CAPACITY 16
// 扩容因子
#define CN_DYNAMIC_ARRAY_GROWTH_FACTOR 2
// 最小容量（防止容量为0）
#define CN_DYNAMIC_ARRAY_MIN_CAPACITY 4

void F_dynamic_array_set_error_code(enum Eum_DynamicArrayError* error_code, 
                                    enum Eum_DynamicArrayError code)
{
    if (error_code != NULL)
    {
        *error_code = code;
    }
}

bool F_dynamic_array_validate_array(Stru_DynamicArray_t* array, 
                                    enum Eum_DynamicArrayError* error_code)
{
    if (array == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    if (array->items == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_INTERNAL);
        return false;
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

bool F_dynamic_array_validate_index(Stru_DynamicArray_t* array, size_t index,
                                    enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
    {
        return false;
    }
    
    if (index >= array->length)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS);
        return false;
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

void* F_dynamic_array_copy_item_data(Stru_DynamicArray_t* array, const void* item,
                                     enum Eum_DynamicArrayError* error_code)
{
    if (item == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return NULL;
    }
    
    void* new_item = malloc(array->item_size);
    if (new_item == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    
    memcpy(new_item, item, array->item_size);
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return new_item;
}

bool F_dynamic_array_ensure_capacity(Stru_DynamicArray_t* array,
                                     enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
    {
        return false;
    }
    
    // 如果数组已满，需要扩容
    if (array->length >= array->capacity)
    {
        size_t new_capacity = array->capacity * CN_DYNAMIC_ARRAY_GROWTH_FACTOR;
        if (new_capacity < CN_DYNAMIC_ARRAY_INITIAL_CAPACITY)
        {
            new_capacity = CN_DYNAMIC_ARRAY_INITIAL_CAPACITY;
        }
        
        void** new_items = (void**)realloc(array->items, new_capacity * sizeof(void*));
        if (new_items == NULL)
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
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}
