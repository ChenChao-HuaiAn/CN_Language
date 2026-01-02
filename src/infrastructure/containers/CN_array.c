/******************************************************************************
 * 文件名: CN_array.c
 * 功能: CN_Language动态数组容器实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现动态数组接口
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_array.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部数据结构
// ============================================================================

struct Stru_CN_Array_t
{
    void* data;                     /**< 数据指针 */
    size_t element_size;            /**< 元素大小（字节） */
    size_t length;                  /**< 当前长度（元素数量） */
    size_t capacity;                /**< 当前容量（元素数量） */
    Eum_CN_ArrayElementType_t element_type; /**< 元素类型 */
    CN_ArrayCompareFunc compare_func; /**< 比较函数 */
    CN_ArrayFreeFunc free_func;     /**< 释放函数 */
    CN_ArrayCopyFunc copy_func;     /**< 复制函数 */
};

// ============================================================================
// 内部辅助函数
// ============================================================================

static bool ensure_capacity(Stru_CN_Array_t* array, size_t min_capacity)
{
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

static void* get_element_ptr(const Stru_CN_Array_t* array, size_t index)
{
    return (char*)array->data + index * array->element_size;
}

// ============================================================================
// 公共API实现
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

// 简化实现，只返回基本功能
Stru_CN_Array_t* CN_array_copy(const Stru_CN_Array_t* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    Stru_CN_Array_t* copy = CN_array_create(src->element_type,
                                            src->element_size,
                                            src->capacity);
    if (copy == NULL)
    {
        return NULL;
    }
    
    copy->compare_func = src->compare_func;
    copy->free_func = src->free_func;
    copy->copy_func = src->copy_func;
    
    if (src->length > 0)
    {
        if (!ensure_capacity(copy, src->length))
        {
            CN_array_destroy(copy);
            return NULL;
        }
        
        if (copy->copy_func != NULL)
        {
            for (size_t i = 0; i < src->length; i++)
            {
                void* src_element = get_element_ptr(src, i);
                void* copy_element = copy->copy_func(src_element);
                if (copy_element == NULL)
                {
                    CN_array_destroy(copy);
                    return NULL;
                }
                memcpy(get_element_ptr(copy, i), copy_element, copy->element_size);
                free(copy_element);
            }
        }
        else
        {
            memcpy(copy->data, src->data, src->length * src->element_size);
        }
        
        copy->length = src->length;
    }
    
    return copy;
}

// 其他函数简化实现
size_t CN_array_find(const Stru_CN_Array_t* array, const void* element)
{
    if (array == NULL || element == NULL || array->length == 0)
    {
        return SIZE_MAX;
    }
    
    for (size_t i = 0; i < array->length; i++)
    {
        void* current = get_element_ptr(array, i);
        if (array->compare_func != NULL)
        {
            if (array->compare_func(current, element) == 0)
            {
                return i;
            }
        }
        else if (memcmp(current, element, array->element_size) == 0)
        {
            return i;
        }
    }
    
    return SIZE_MAX;
}

void CN_array_sort(Stru_CN_Array_t* array)
{
    // 简化实现：使用qsort
    if (array == NULL || array->length < 2)
    {
        return;
    }
    
    if (array->compare_func != NULL)
    {
        qsort(array->data, array->length, array->element_size,
              (int (*)(const void*, const void*))array->compare_func);
    }
}

void CN_array_dump(const Stru_CN_Array_t* array)
{
    if (array == NULL)
    {
        printf("Array: NULL\n");
        return;
    }
    
    printf("=== Array Information ===\n");
    printf("Length: %lu\n", (unsigned long)array->length);
    printf("Capacity: %lu\n", (unsigned long)array->capacity);
    printf("Element size: %lu bytes\n", (unsigned long)array->element_size);
    printf("Element type: %d\n", array->element_type);
    printf("=========================\n");
}
