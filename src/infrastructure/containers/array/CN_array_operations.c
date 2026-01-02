/******************************************************************************
 * 文件名: CN_array_operations.c
 * 功能: CN_Language动态数组操作函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现数组操作函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_array_internal.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 数组操作函数实现
// ============================================================================

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
        // 确保有足够容量
        if (!CN_array_internal_ensure_capacity(copy, src->length))
        {
            CN_array_destroy(copy);
            return NULL;
        }
        
        CN_ArrayCopyFunc copy_func = CN_array_internal_get_copy_func(src);
        if (copy_func != NULL)
        {
            // 使用自定义复制函数
            for (size_t i = 0; i < src->length; i++)
            {
                void* src_element = CN_array_internal_get_element_ptr(src, i);
                void* copy_element = copy_func(src_element);
                if (copy_element == NULL)
                {
                    CN_array_destroy(copy);
                    return NULL;
                }
                void* dest = CN_array_internal_get_element_ptr(copy, i);
                memcpy(dest, copy_element, copy->element_size);
                free(copy_element);
            }
        }
        else
        {
            // 直接内存复制
            void* src_data = CN_array_internal_get_data(src);
            void* dest_data = CN_array_internal_get_data(copy);
            memcpy(dest_data, src_data, src->length * src->element_size);
        }
        
        copy->length = src->length;
    }
    
    return copy;
}

Stru_CN_Array_t* CN_array_concat(const Stru_CN_Array_t* array1,
                                 const Stru_CN_Array_t* array2)
{
    if (array1 == NULL || array2 == NULL)
    {
        return NULL;
    }
    
    // 检查元素类型和大小是否兼容
    if (array1->element_type != array2->element_type ||
        array1->element_size != array2->element_size)
    {
        return NULL;
    }
    
    // 检查自定义函数是否兼容
    if (array1->compare_func != array2->compare_func ||
        array1->free_func != array2->free_func ||
        array1->copy_func != array2->copy_func)
    {
        return NULL;
    }
    
    // 创建新数组
    size_t total_length = array1->length + array2->length;
    Stru_CN_Array_t* result = CN_array_create(array1->element_type,
                                              array1->element_size,
                                              total_length);
    if (result == NULL)
    {
        return NULL;
    }
    
    result->compare_func = array1->compare_func;
    result->free_func = array1->free_func;
    result->copy_func = array1->copy_func;
    
    // 复制第一个数组
    if (array1->length > 0)
    {
        if (!CN_array_internal_ensure_capacity(result, array1->length))
        {
            CN_array_destroy(result);
            return NULL;
        }
        
        CN_ArrayCopyFunc copy_func = CN_array_internal_get_copy_func(array1);
        if (copy_func != NULL)
        {
            for (size_t i = 0; i < array1->length; i++)
            {
                void* src_element = CN_array_internal_get_element_ptr(array1, i);
                void* copy_element = copy_func(src_element);
                if (copy_element == NULL)
                {
                    CN_array_destroy(result);
                    return NULL;
                }
                void* dest = CN_array_internal_get_element_ptr(result, i);
                memcpy(dest, copy_element, result->element_size);
                free(copy_element);
            }
        }
        else
        {
            void* src_data = CN_array_internal_get_data(array1);
            void* dest_data = CN_array_internal_get_data(result);
            memcpy(dest_data, src_data, array1->length * array1->element_size);
        }
        result->length = array1->length;
    }
    
    // 添加第二个数组
    if (array2->length > 0)
    {
        if (!CN_array_internal_ensure_capacity(result, total_length))
        {
            CN_array_destroy(result);
            return NULL;
        }
        
        CN_ArrayCopyFunc copy_func = CN_array_internal_get_copy_func(array2);
        if (copy_func != NULL)
        {
            for (size_t i = 0; i < array2->length; i++)
            {
                void* src_element = CN_array_internal_get_element_ptr(array2, i);
                void* copy_element = copy_func(src_element);
                if (copy_element == NULL)
                {
                    CN_array_destroy(result);
                    return NULL;
                }
                void* dest = CN_array_internal_get_element_ptr(result, array1->length + i);
                memcpy(dest, copy_element, result->element_size);
                free(copy_element);
            }
        }
        else
        {
            void* src_data = CN_array_internal_get_data(array2);
            void* dest_data = (char*)CN_array_internal_get_data(result) + 
                              array1->length * array1->element_size;
            memcpy(dest_data, src_data, array2->length * array2->element_size);
        }
        result->length = total_length;
    }
    
    return result;
}

Stru_CN_Array_t* CN_array_slice(const Stru_CN_Array_t* array,
                                size_t start, size_t end)
{
    if (array == NULL || start > end || end > array->length)
    {
        return NULL;
    }
    
    if (start == end)
    {
        // 空切片
        return CN_array_create(array->element_type,
                               array->element_size, 0);
    }
    
    size_t slice_length = end - start;
    Stru_CN_Array_t* slice = CN_array_create(array->element_type,
                                             array->element_size,
                                             slice_length);
    if (slice == NULL)
    {
        return NULL;
    }
    
    slice->compare_func = array->compare_func;
    slice->free_func = array->free_func;
    slice->copy_func = array->copy_func;
    
    if (slice_length > 0)
    {
        if (!CN_array_internal_ensure_capacity(slice, slice_length))
        {
            CN_array_destroy(slice);
            return NULL;
        }
        
        CN_ArrayCopyFunc copy_func = CN_array_internal_get_copy_func(array);
        if (copy_func != NULL)
        {
            for (size_t i = 0; i < slice_length; i++)
            {
                void* src_element = CN_array_internal_get_element_ptr(array, start + i);
                void* copy_element = copy_func(src_element);
                if (copy_element == NULL)
                {
                    CN_array_destroy(slice);
                    return NULL;
                }
                void* dest = CN_array_internal_get_element_ptr(slice, i);
                memcpy(dest, copy_element, slice->element_size);
                free(copy_element);
            }
        }
        else
        {
            void* src = CN_array_internal_get_element_ptr(array, start);
            void* dest = CN_array_internal_get_data(slice);
            memcpy(dest, src, slice_length * slice->element_size);
        }
        
        slice->length = slice_length;
    }
    
    return slice;
}
