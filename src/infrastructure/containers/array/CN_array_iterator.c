/******************************************************************************
 * 文件名: CN_array_iterator.c
 * 功能: CN_Language动态数组迭代器实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现数组迭代器
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_array_internal.h"
#include <stdlib.h>

// ============================================================================
// 迭代器函数实现
// ============================================================================

Stru_CN_ArrayIterator_t* CN_array_iterator_create(Stru_CN_Array_t* array)
{
    if (array == NULL)
    {
        return NULL;
    }
    
    Stru_CN_ArrayIterator_t* iterator = malloc(sizeof(Stru_CN_ArrayIterator_t));
    if (iterator == NULL)
    {
        return NULL;
    }
    
    iterator->array = array;
    iterator->current_index = 0;
    iterator->current_element = NULL;
    
    // 如果有元素，设置当前元素
    if (array->length > 0)
    {
        iterator->current_element = CN_array_internal_get_element_ptr(array, 0);
    }
    
    return iterator;
}

void CN_array_iterator_destroy(Stru_CN_ArrayIterator_t* iterator)
{
    if (iterator != NULL)
    {
        free(iterator);
    }
}

void CN_array_iterator_reset(Stru_CN_ArrayIterator_t* iterator)
{
    if (iterator == NULL || iterator->array == NULL)
    {
        return;
    }
    
    iterator->current_index = 0;
    
    if (iterator->array->length > 0)
    {
        iterator->current_element = CN_array_internal_get_element_ptr(iterator->array, 0);
    }
    else
    {
        iterator->current_element = NULL;
    }
}

bool CN_array_iterator_has_next(const Stru_CN_ArrayIterator_t* iterator)
{
    if (iterator == NULL || iterator->array == NULL)
    {
        return false;
    }
    
    return iterator->current_index < iterator->array->length;
}

void* CN_array_iterator_next(Stru_CN_ArrayIterator_t* iterator)
{
    if (iterator == NULL || iterator->array == NULL)
    {
        return NULL;
    }
    
    if (iterator->current_index >= iterator->array->length)
    {
        return NULL;
    }
    
    // 获取当前元素
    void* current = CN_array_internal_get_element_ptr(iterator->array, iterator->current_index);
    
    // 更新索引
    iterator->current_index++;
    
    // 更新当前元素指针
    if (iterator->current_index < iterator->array->length)
    {
        iterator->current_element = CN_array_internal_get_element_ptr(iterator->array, iterator->current_index);
    }
    else
    {
        iterator->current_element = NULL;
    }
    
    return current;
}

void* CN_array_iterator_current(const Stru_CN_ArrayIterator_t* iterator)
{
    if (iterator == NULL || iterator->array == NULL)
    {
        return NULL;
    }
    
    if (iterator->current_index >= iterator->array->length)
    {
        return NULL;
    }
    
    return CN_array_internal_get_element_ptr(iterator->array, iterator->current_index);
}
