/**
 * @file CN_dynamic_array.c
 * @brief 动态数组模块实现文件
 * 
 * 实现动态数组数据结构的核心功能。
 * 包括创建、销毁、添加、删除、遍历等操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_dynamic_array.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 默认初始容量
#define CN_DYNAMIC_ARRAY_INITIAL_CAPACITY 16
// 扩容因子
#define CN_DYNAMIC_ARRAY_GROWTH_FACTOR 2

/**
 * @brief 内部函数：确保数组有足够容量
 * 
 * 检查数组是否有足够容量容纳新元素，如果没有则扩容。
 * 
 * @param array 动态数组指针
 * @return 容量足够或扩容成功返回true，失败返回false
 */
static bool ensure_capacity(Stru_DynamicArray_t* array)
{
    if (array == NULL)
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
            return false;
        }
        
        array->items = new_items;
        array->capacity = new_capacity;
    }
    
    return true;
}

Stru_DynamicArray_t* F_create_dynamic_array(size_t item_size)
{
    if (item_size == 0)
    {
        return NULL;
    }
    
    // 分配动态数组结构体
    Stru_DynamicArray_t* array = (Stru_DynamicArray_t*)malloc(sizeof(Stru_DynamicArray_t));
    if (array == NULL)
    {
        return NULL;
    }
    
    // 初始化字段
    array->capacity = CN_DYNAMIC_ARRAY_INITIAL_CAPACITY;
    array->length = 0;
    array->item_size = item_size;
    
    // 分配初始元素数组
    array->items = (void**)malloc(CN_DYNAMIC_ARRAY_INITIAL_CAPACITY * sizeof(void*));
    if (array->items == NULL)
    {
        free(array);
        return NULL;
    }
    
    // 初始化指针为NULL
    for (size_t i = 0; i < array->capacity; i++)
    {
        array->items[i] = NULL;
    }
    
    return array;
}

void F_destroy_dynamic_array(Stru_DynamicArray_t* array)
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

bool F_dynamic_array_push(Stru_DynamicArray_t* array, void* item)
{
    if (array == NULL || item == NULL)
    {
        return false;
    }
    
    // 确保有足够容量
    if (!ensure_capacity(array))
    {
        return false;
    }
    
    // 分配新元素内存并复制数据
    void* new_item = malloc(array->item_size);
    if (new_item == NULL)
    {
        return false;
    }
    
    memcpy(new_item, item, array->item_size);
    array->items[array->length] = new_item;
    array->length++;
    
    return true;
}

void* F_dynamic_array_get(Stru_DynamicArray_t* array, size_t index)
{
    if (array == NULL || index >= array->length)
    {
        return NULL;
    }
    
    return array->items[index];
}

bool F_dynamic_array_remove(Stru_DynamicArray_t* array, size_t index)
{
    if (array == NULL || index >= array->length)
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
    
    return true;
}

size_t F_dynamic_array_length(Stru_DynamicArray_t* array)
{
    if (array == NULL)
    {
        return 0;
    }
    
    return array->length;
}

size_t F_dynamic_array_capacity(Stru_DynamicArray_t* array)
{
    if (array == NULL)
    {
        return 0;
    }
    
    return array->capacity;
}

void F_dynamic_array_clear(Stru_DynamicArray_t* array)
{
    if (array == NULL)
    {
        return;
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
}

bool F_dynamic_array_resize(Stru_DynamicArray_t* array, size_t new_capacity)
{
    if (array == NULL)
    {
        return false;
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
        return false;
    }
    
    array->items = new_items;
    array->capacity = new_capacity;
    
    // 初始化新分配的空间
    for (size_t i = array->length; i < array->capacity; i++)
    {
        array->items[i] = NULL;
    }
    
    return true;
}

void F_dynamic_array_foreach(Stru_DynamicArray_t* array, 
                            void (*callback)(void* item, void* user_data),
                            void* user_data)
{
    if (array == NULL || callback == NULL)
    {
        return;
    }
    
    for (size_t i = 0; i < array->length; i++)
    {
        callback(array->items[i], user_data);
    }
}
