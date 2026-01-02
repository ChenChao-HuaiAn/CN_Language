/******************************************************************************
 * 文件名: CN_array.c
 * 功能: CN_Language动态数组容器核心实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现动态数组核心接口
 *  2026-01-02: 重构为模块化设计，遵循单一职责原则
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_array_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 确保数组有足够容量
 * 
 * @param array 数组
 * @param min_capacity 最小容量要求
 * @return 调整成功返回true，失败返回false
 */
static bool ensure_capacity(Stru_CN_Array_t* array, size_t min_capacity)
{
    if (array == NULL)
    {
        return false;
    }
    
    if (array->capacity >= min_capacity)
    {
        return true;
    }
    
    // 计算新容量（按1.5倍增长）
    size_t new_capacity = array->capacity * 3 / 2;
    if (new_capacity < min_capacity)
    {
        new_capacity = min_capacity;
    }
    if (new_capacity < 4)
    {
        new_capacity = 4;
    }
    
    void* new_data = realloc(array->data, new_capacity * array->element_size);
    if (new_data == NULL)
    {
        return false;
    }
    
    array->data = new_data;
    array->capacity = new_capacity;
    return true;
}

/**
 * @brief 获取元素指针
 * 
 * @param array 数组
 * @param index 元素索引
 * @return 元素指针
 */
static void* get_element_ptr(const Stru_CN_Array_t* array, size_t index)
{
    if (array == NULL || array->data == NULL)
    {
        return NULL;
    }
    
    return (char*)array->data + index * array->element_size;
}

// ============================================================================
// 数组创建和销毁
// ============================================================================

Stru_CN_Array_t* CN_array_create(Eum_CN_ArrayElementType_t element_type,
                                 size_t element_size, size_t initial_capacity)
{
    Stru_CN_Array_t* array = malloc(sizeof(Stru_CN_Array_t));
    if (array == NULL)
    {
        return NULL;
    }
    
    array->element_type = element_type;
    array->element_size = element_size;
    array->length = 0;
    array->capacity = 0;
    array->data = NULL;
    array->compare_func = NULL;
    array->free_func = NULL;
    array->copy_func = NULL;
    
    if (initial_capacity > 0)
    {
        if (!ensure_capacity(array, initial_capacity))
        {
            free(array);
            return NULL;
        }
    }
    
    return array;
}

Stru_CN_Array_t* CN_array_create_custom(size_t element_size,
                                        size_t initial_capacity,
                                        CN_ArrayCompareFunc compare_func,
                                        CN_ArrayFreeFunc free_func,
                                        CN_ArrayCopyFunc copy_func)
{
    Stru_CN_Array_t* array = CN_array_create(Eum_ARRAY_ELEMENT_CUSTOM,
                                             element_size, initial_capacity);
    if (array == NULL)
    {
        return NULL;
    }
    
    array->compare_func = compare_func;
    array->free_func = free_func;
    array->copy_func = copy_func;
    
    return array;
}

void CN_array_destroy(Stru_CN_Array_t* array)
{
    if (array == NULL)
    {
        return;
    }
    
    if (array->free_func != NULL)
    {
        for (size_t i = 0; i < array->length; i++)
        {
            void* element = get_element_ptr(array, i);
            array->free_func(element);
        }
    }
    
    free(array->data);
    free(array);
}

void CN_array_clear(Stru_CN_Array_t* array)
{
    if (array == NULL)
    {
        return;
    }
    
    if (array->free_func != NULL)
    {
        for (size_t i = 0; i < array->length; i++)
        {
            void* element = get_element_ptr(array, i);
            array->free_func(element);
        }
    }
    
    array->length = 0;
}

// ============================================================================
// 数组属性查询
// ============================================================================

size_t CN_array_length(const Stru_CN_Array_t* array)
{
    return array != NULL ? array->length : 0;
}

size_t CN_array_capacity(const Stru_CN_Array_t* array)
{
    return array != NULL ? array->capacity : 0;
}

bool CN_array_is_empty(const Stru_CN_Array_t* array)
{
    return array == NULL || array->length == 0;
}

size_t CN_array_element_size(const Stru_CN_Array_t* array)
{
    return array != NULL ? array->element_size : 0;
}

Eum_CN_ArrayElementType_t CN_array_element_type(const Stru_CN_Array_t* array)
{
    return array != NULL ? array->element_type : Eum_ARRAY_ELEMENT_CUSTOM;
}

// ============================================================================
// 元素访问和修改
// ============================================================================

void* CN_array_get(const Stru_CN_Array_t* array, size_t index)
{
    if (array == NULL || index >= array->length)
    {
        return NULL;
    }
    
    return get_element_ptr(array, index);
}

bool CN_array_set(Stru_CN_Array_t* array, size_t index, const void* element)
{
    if (array == NULL || index >= array->length || element == NULL)
    {
        return false;
    }
    
    void* dest = get_element_ptr(array, index);
    
    // 如果设置了释放函数，先释放旧元素
    if (array->free_func != NULL)
    {
        array->free_func(dest);
    }
    
    memcpy(dest, element, array->element_size);
    return true;
}

bool CN_array_append(Stru_CN_Array_t* array, const void* element)
{
    if (array == NULL || element == NULL)
    {
        return false;
    }
    
    if (!ensure_capacity(array, array->length + 1))
    {
        return false;
    }
    
    void* dest = get_element_ptr(array, array->length);
    memcpy(dest, element, array->element_size);
    array->length++;
    
    return true;
}

bool CN_array_insert(Stru_CN_Array_t* array, size_t index, const void* element)
{
    if (array == NULL || index > array->length || element == NULL)
    {
        return false;
    }
    
    if (index == array->length)
    {
        return CN_array_append(array, element);
    }
    
    if (!ensure_capacity(array, array->length + 1))
    {
        return false;
    }
    
    // 移动元素
    void* src = get_element_ptr(array, index);
    void* dest = get_element_ptr(array, index + 1);
    size_t bytes_to_move = (array->length - index) * array->element_size;
    memmove(dest, src, bytes_to_move);
    
    // 插入新元素
    memcpy(src, element, array->element_size);
    array->length++;
    
    return true;
}

bool CN_array_remove(Stru_CN_Array_t* array, size_t index)
{
    if (array == NULL || index >= array->length)
    {
        return false;
    }
    
    // 如果设置了释放函数，先释放要移除的元素
    if (array->free_func != NULL)
    {
        void* element = get_element_ptr(array, index);
        array->free_func(element);
    }
    
    // 移动元素
    if (index < array->length - 1)
    {
        void* src = get_element_ptr(array, index + 1);
        void* dest = get_element_ptr(array, index);
        size_t bytes_to_move = (array->length - index - 1) * array->element_size;
        memmove(dest, src, bytes_to_move);
    }
    
    array->length--;
    return true;
}

bool CN_array_pop(Stru_CN_Array_t* array, void* element)
{
    if (array == NULL || array->length == 0)
    {
        return false;
    }
    
    if (element != NULL)
    {
        void* src = get_element_ptr(array, array->length - 1);
        memcpy(element, src, array->element_size);
    }
    
    // 如果设置了释放函数，释放元素
    if (array->free_func != NULL)
    {
        void* elem_ptr = get_element_ptr(array, array->length - 1);
        array->free_func(elem_ptr);
    }
    
    array->length--;
    return true;
}

// ============================================================================
// 数组容量管理
// ============================================================================

bool CN_array_ensure_capacity(Stru_CN_Array_t* array, size_t min_capacity)
{
    return array != NULL && ensure_capacity(array, min_capacity);
}

bool CN_array_shrink_to_fit(Stru_CN_Array_t* array)
{
    if (array == NULL || array->capacity == array->length)
    {
        return true;
    }
    
    if (array->length == 0)
    {
        free(array->data);
        array->data = NULL;
        array->capacity = 0;
        return true;
    }
    
    void* new_data = realloc(array->data, array->length * array->element_size);
    if (new_data == NULL)
    {
        return false;
    }
    
    array->data = new_data;
    array->capacity = array->length;
    return true;
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

bool CN_array_internal_ensure_capacity(Stru_CN_Array_t* array, size_t min_capacity)
{
    return ensure_capacity(array, min_capacity);
}

void* CN_array_internal_get_element_ptr(const Stru_CN_Array_t* array, size_t index)
{
    return get_element_ptr(array, index);
}

void* CN_array_internal_get_data(const Stru_CN_Array_t* array)
{
    return array != NULL ? array->data : NULL;
}

CN_ArrayCompareFunc CN_array_internal_get_compare_func(const Stru_CN_Array_t* array)
{
    return array != NULL ? array->compare_func : NULL;
}

CN_ArrayFreeFunc CN_array_internal_get_free_func(const Stru_CN_Array_t* array)
{
    return array != NULL ? array->free_func : NULL;
}

CN_ArrayCopyFunc CN_array_internal_get_copy_func(const Stru_CN_Array_t* array)
{
    return array != NULL ? array->copy_func : NULL;
}
