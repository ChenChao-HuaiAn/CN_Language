/******************************************************************************
 * 文件名: CN_array_search_sort.c
 * 功能: CN_Language动态数组搜索和排序函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现数组搜索和排序函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_array_internal.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// ============================================================================
// 搜索函数实现
// ============================================================================

size_t CN_array_find(const Stru_CN_Array_t* array, const void* element)
{
    if (array == NULL || element == NULL || array->length == 0)
    {
        return SIZE_MAX;
    }
    
    CN_ArrayCompareFunc compare_func = CN_array_internal_get_compare_func(array);
    
    for (size_t i = 0; i < array->length; i++)
    {
        void* current = CN_array_internal_get_element_ptr(array, i);
        
        if (compare_func != NULL)
        {
            if (compare_func(current, element) == 0)
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

size_t CN_array_find_custom(const Stru_CN_Array_t* array, const void* element,
                            CN_ArrayCompareFunc compare_func)
{
    if (array == NULL || element == NULL || array->length == 0 || compare_func == NULL)
    {
        return SIZE_MAX;
    }
    
    for (size_t i = 0; i < array->length; i++)
    {
        void* current = CN_array_internal_get_element_ptr(array, i);
        if (compare_func(current, element) == 0)
        {
            return i;
        }
    }
    
    return SIZE_MAX;
}

bool CN_array_contains(const Stru_CN_Array_t* array, const void* element)
{
    return CN_array_find(array, element) != SIZE_MAX;
}

// ============================================================================
// 排序函数实现
// ============================================================================

/**
 * @brief 默认比较函数包装器（用于内置类型）
 */
static int default_compare_wrapper(const void* a, const void* b)
{
    // 这个函数不能直接使用，需要配合元素大小
    // 实际实现中使用memcmp直接比较
    return 0;
}

/**
 * @brief 快速排序分区函数
 */
static size_t partition(void* data, size_t element_size, size_t low, size_t high,
                        CN_ArrayCompareFunc compare_func)
{
    char* bytes = (char*)data;
    void* pivot = bytes + high * element_size;
    size_t i = low;
    
    for (size_t j = low; j < high; j++)
    {
        void* current = bytes + j * element_size;
        if (compare_func(current, pivot) < 0)
        {
            // 交换元素
            for (size_t k = 0; k < element_size; k++)
            {
                char temp = bytes[i * element_size + k];
                bytes[i * element_size + k] = bytes[j * element_size + k];
                bytes[j * element_size + k] = temp;
            }
            i++;
        }
    }
    
    // 交换枢轴元素
    for (size_t k = 0; k < element_size; k++)
    {
        char temp = bytes[i * element_size + k];
        bytes[i * element_size + k] = bytes[high * element_size + k];
        bytes[high * element_size + k] = temp;
    }
    
    return i;
}

/**
 * @brief 快速排序递归函数
 */
static void quick_sort(void* data, size_t element_size, size_t low, size_t high,
                       CN_ArrayCompareFunc compare_func)
{
    if (low < high)
    {
        size_t pi = partition(data, element_size, low, high, compare_func);
        
        if (pi > 0) // 防止下溢
        {
            quick_sort(data, element_size, low, pi - 1, compare_func);
        }
        quick_sort(data, element_size, pi + 1, high, compare_func);
    }
}

void CN_array_sort(Stru_CN_Array_t* array)
{
    if (array == NULL || array->length < 2)
    {
        return;
    }
    
    CN_ArrayCompareFunc compare_func = CN_array_internal_get_compare_func(array);
    
    if (compare_func == NULL)
    {
        // 如果没有自定义比较函数，使用qsort和memcmp
        qsort(array->data, array->length, array->element_size, 
              (int (*)(const void*, const void*))memcmp);
    }
    else
    {
        // 使用快速排序
        quick_sort(array->data, array->element_size, 0, array->length - 1, compare_func);
    }
}

void CN_array_sort_custom(Stru_CN_Array_t* array, CN_ArrayCompareFunc compare_func)
{
    if (array == NULL || array->length < 2 || compare_func == NULL)
    {
        return;
    }
    
    // 使用快速排序
    quick_sort(array->data, array->element_size, 0, array->length - 1, compare_func);
}

void CN_array_reverse(Stru_CN_Array_t* array)
{
    if (array == NULL || array->length < 2)
    {
        return;
    }
    
    char* bytes = (char*)array->data;
    size_t element_size = array->element_size;
    size_t left = 0;
    size_t right = array->length - 1;
    
    while (left < right)
    {
        // 交换元素
        for (size_t k = 0; k < element_size; k++)
        {
            char temp = bytes[left * element_size + k];
            bytes[left * element_size + k] = bytes[right * element_size + k];
            bytes[right * element_size + k] = temp;
        }
        
        left++;
        right--;
    }
}
