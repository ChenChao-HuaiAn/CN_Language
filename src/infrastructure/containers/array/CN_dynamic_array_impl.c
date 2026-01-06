/**
 * @file CN_dynamic_array_impl.c
 * @brief 动态数组模块扩展实现文件
 * 
 * 实现动态数组数据结构的扩展功能，包括接口实现和高级功能。
 * 遵循CN_Language项目的架构规范和编码标准。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_dynamic_array.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

// 默认初始容量
#define CN_DYNAMIC_ARRAY_INITIAL_CAPACITY 16
// 扩容因子
#define CN_DYNAMIC_ARRAY_GROWTH_FACTOR 2
// 最小容量（防止容量为0）
#define CN_DYNAMIC_ARRAY_MIN_CAPACITY 4

/**
 * @brief 内部函数：设置错误码
 * 
 * 如果error_code指针不为NULL，则设置错误码。
 * 
 * @param error_code 错误码指针
 * @param code 错误码值
 */
static void set_error_code(enum Eum_DynamicArrayError* error_code, enum Eum_DynamicArrayError code)
{
    if (error_code != NULL)
    {
        *error_code = code;
    }
}

/**
 * @brief 内部函数：验证动态数组指针
 * 
 * 验证动态数组指针是否有效。
 * 
 * @param array 动态数组指针
 * @param error_code 输出参数，错误码
 * @return 有效返回true，无效返回false
 */
static bool validate_array(Stru_DynamicArray_t* array, enum Eum_DynamicArrayError* error_code)
{
    if (array == NULL)
    {
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    if (array->items == NULL)
    {
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_INTERNAL);
        return false;
    }
    
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

/**
 * @brief 内部函数：验证索引
 * 
 * 验证索引是否在有效范围内。
 * 
 * @param array 动态数组指针
 * @param index 索引
 * @param error_code 输出参数，错误码
 * @return 有效返回true，无效返回false
 */
static bool validate_index(Stru_DynamicArray_t* array, size_t index, enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array(array, error_code))
    {
        return false;
    }
    
    if (index >= array->length)
    {
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS);
        return false;
    }
    
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

/**
 * @brief 内部函数：确保数组有足够容量
 * 
 * 检查数组是否有足够容量容纳新元素，如果没有则扩容。
 * 
 * @param array 动态数组指针
 * @param error_code 输出参数，错误码
 * @return 容量足够或扩容成功返回true，失败返回false
 */
static bool ensure_capacity_ex(Stru_DynamicArray_t* array, enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array(array, error_code))
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
            set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY);
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
    
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

/**
 * @brief 内部函数：复制元素数据
 * 
 * 分配内存并复制元素数据。
 * 
 * @param array 动态数组指针
 * @param item 源元素指针
 * @param error_code 输出参数，错误码
 * @return 新分配的元素指针，失败返回NULL
 */
static void* copy_item_data(Stru_DynamicArray_t* array, const void* item, enum Eum_DynamicArrayError* error_code)
{
    if (item == NULL)
    {
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return NULL;
    }
    
    void* new_item = malloc(array->item_size);
    if (new_item == NULL)
    {
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    
    memcpy(new_item, item, array->item_size);
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return new_item;
}

/* 扩展API实现 */

Stru_DynamicArray_t* F_create_dynamic_array_ex(size_t item_size, size_t initial_capacity, 
                                               enum Eum_DynamicArrayError* error_code)
{
    if (item_size == 0)
    {
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE);
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
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY);
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
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    
    // 初始化指针为NULL
    for (size_t i = 0; i < array->capacity; i++)
    {
        array->items[i] = NULL;
    }
    
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return array;
}

bool F_dynamic_array_push_ex(Stru_DynamicArray_t* array, const void* item, 
                             enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array(array, error_code) || item == NULL)
    {
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    // 确保有足够容量
    if (!ensure_capacity_ex(array, error_code))
    {
        return false;
    }
    
    // 分配新元素内存并复制数据
    void* new_item = copy_item_data(array, item, error_code);
    if (new_item == NULL)
    {
        return false;
    }
    
    array->items[array->length] = new_item;
    array->length++;
    
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

void* F_dynamic_array_get_ex(Stru_DynamicArray_t* array, size_t index, 
                             enum Eum_DynamicArrayError* error_code)
{
    if (!validate_index(array, index, error_code))
    {
        return NULL;
    }
    
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return array->items[index];
}

bool F_dynamic_array_set_ex(Stru_DynamicArray_t* array, size_t index, const void* item,
                            enum Eum_DynamicArrayError* error_code)
{
    if (!validate_index(array, index, error_code) || item == NULL)
    {
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    // 释放旧元素
    if (array->items[index] != NULL)
    {
        free(array->items[index]);
    }
    
    // 分配新元素内存并复制数据
    void* new_item = copy_item_data(array, item, error_code);
    if (new_item == NULL)
    {
        array->items[index] = NULL;
        return false;
    }
    
    array->items[index] = new_item;
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

bool F_dynamic_array_remove_ex(Stru_DynamicArray_t* array, size_t index,
                               enum Eum_DynamicArrayError* error_code)
{
    if (!validate_index(array, index, error_code))
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
    
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

size_t F_dynamic_array_length_ex(Stru_DynamicArray_t* array, 
                                 enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array(array, error_code))
    {
        return 0;
    }
    
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return array->length;
}

size_t F_dynamic_array_capacity_ex(Stru_DynamicArray_t* array,
                                   enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array(array, error_code))
    {
        return 0;
    }
    
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return array->capacity;
}

bool F_dynamic_array_is_empty_ex(Stru_DynamicArray_t* array,
                                 enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array(array, error_code))
    {
        return true;
    }
    
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return (array->length == 0);
}

bool F_dynamic_array_clear_ex(Stru_DynamicArray_t* array,
                              enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array(array, error_code))
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
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

bool F_dynamic_array_resize_ex(Stru_DynamicArray_t* array, size_t new_capacity,
                               enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array(array, error_code))
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
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY);
        return false;
    }
    
    array->items = new_items;
    array->capacity = new_capacity;
    
    // 初始化新分配的空间
    for (size_t i = array->length; i < array->capacity; i++)
    {
        array->items[i] = NULL;
    }
    
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

size_t F_dynamic_array_find_ex(Stru_DynamicArray_t* array, const void* item,
                               F_DynamicArrayCompare_t compare,
                               enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array(array, error_code) || item == NULL)
    {
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
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
                set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
                return i;
            }
        }
    }
    
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_ITEM_NOT_FOUND);
    return SIZE_MAX;
}

bool F_dynamic_array_push_batch_ex(Stru_DynamicArray_t* array, const void* items, size_t count,
                                   enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array(array, error_code) || items == NULL)
    {
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    if (count == 0)
    {
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
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
        void* new_item = copy_item_data(array, src + (i * array->item_size), error_code);
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
    
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

bool F_dynamic_array_foreach_ex(Stru_DynamicArray_t* array, 
                               F_DynamicArrayIterator_t iterator, void* user_data,
                               enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array(array, error_code) || iterator == NULL)
    {
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    for (size_t i = 0; i < array->length; i++)
    {
        if (!iterator(array->items[i], i, user_data))
        {
            // 迭代器请求停止
            set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
            return true;
        }
    }
    
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

bool F_dynamic_array_get_memory_stats_ex(Stru_DynamicArray_t* array, 
                                         size_t* total_bytes, size_t* used_bytes,
                                         enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array(array,
