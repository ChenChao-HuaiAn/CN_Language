/**
 * @file CN_dynamic_array.c
 * @brief 动态数组模块向后兼容包装文件
 * 
 * 提供动态数组模块的向后兼容接口。
 * 这些函数调用新的模块化实现，确保现有代码继续工作。
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
#include "CN_dynamic_array_operations.h"
#include "CN_dynamic_array_utils.h"
#include <stdlib.h>
#include <string.h>

/* 向后兼容的简单API实现 */

Stru_DynamicArray_t* F_create_dynamic_array(size_t item_size)
{
    return F_dynamic_array_create_ex(item_size, 0, NULL);
}

void F_destroy_dynamic_array(Stru_DynamicArray_t* array)
{
    F_dynamic_array_destroy(array);
}

bool F_dynamic_array_push(Stru_DynamicArray_t* array, const void* item)
{
    return F_dynamic_array_push_ex(array, item, NULL);
}

void* F_dynamic_array_get(Stru_DynamicArray_t* array, size_t index)
{
    return F_dynamic_array_get_ex(array, index, NULL);
}

bool F_dynamic_array_set(Stru_DynamicArray_t* array, size_t index, const void* item)
{
    return F_dynamic_array_set_ex(array, index, item, NULL);
}

bool F_dynamic_array_remove(Stru_DynamicArray_t* array, size_t index)
{
    return F_dynamic_array_remove_ex(array, index, NULL);
}

size_t F_dynamic_array_length(Stru_DynamicArray_t* array)
{
    return F_dynamic_array_length_ex(array, NULL);
}

size_t F_dynamic_array_capacity(Stru_DynamicArray_t* array)
{
    return F_dynamic_array_capacity_ex(array, NULL);
}

bool F_dynamic_array_is_empty(Stru_DynamicArray_t* array)
{
    return F_dynamic_array_is_empty_ex(array, NULL);
}

void F_dynamic_array_clear(Stru_DynamicArray_t* array)
{
    F_dynamic_array_clear_ex(array, NULL);
}

bool F_dynamic_array_resize(Stru_DynamicArray_t* array, size_t new_capacity)
{
    return F_dynamic_array_resize_ex(array, new_capacity, NULL);
}

size_t F_dynamic_array_find(Stru_DynamicArray_t* array, const void* item,
                            F_DynamicArrayCompare_t compare)
{
    return F_dynamic_array_find_ex(array, item, compare, NULL);
}

bool F_dynamic_array_push_batch(Stru_DynamicArray_t* array, const void* items, size_t count)
{
    return F_dynamic_array_push_batch_ex(array, items, count, NULL);
}

// 辅助函数：将简单回调包装为迭代器
static bool foreach_wrapper(void* item, size_t index, void* user_data)
{
    struct ForeachWrapperData
    {
        void (*callback)(void* item, void* user_data);
        void* user_data;
    };
    
    struct ForeachWrapperData* data = (struct ForeachWrapperData*)user_data;
    if (data->callback != NULL)
    {
        data->callback(item, data->user_data);
    }
    return true; // 继续迭代
}

void F_dynamic_array_foreach(Stru_DynamicArray_t* array, 
                            void (*callback)(void* item, void* user_data),
                            void* user_data)
{
    // 将简单的回调函数包装为迭代器函数
    struct ForeachWrapperData
    {
        void (*callback)(void* item, void* user_data);
        void* user_data;
    };
    
    struct ForeachWrapperData wrapper_data = {callback, user_data};
    
    F_dynamic_array_foreach_ex(array, foreach_wrapper, &wrapper_data, NULL);
}

bool F_dynamic_array_get_memory_stats(Stru_DynamicArray_t* array, 
                                      size_t* total_bytes, size_t* used_bytes)
{
    return F_dynamic_array_get_memory_stats_ex(array, total_bytes, used_bytes, NULL);
}

bool F_dynamic_array_shrink_to_fit(Stru_DynamicArray_t* array)
{
    return F_dynamic_array_shrink_to_fit_ex(array, NULL);
}

/* 扩展API的向后兼容实现 */

Stru_DynamicArray_t* F_create_dynamic_array_ex(size_t item_size, size_t initial_capacity, 
                                               enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_create_ex(item_size, initial_capacity, error_code);
}

bool F_dynamic_array_push_ex(Stru_DynamicArray_t* array, const void* item, 
                             enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_push_ex(array, item, error_code);
}

void* F_dynamic_array_get_ex(Stru_DynamicArray_t* array, size_t index, 
                             enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_get_ex(array, index, error_code);
}

bool F_dynamic_array_set_ex(Stru_DynamicArray_t* array, size_t index, const void* item,
                            enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_set_ex(array, index, item, error_code);
}

bool F_dynamic_array_remove_ex(Stru_DynamicArray_t* array, size_t index,
                               enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_remove_ex(array, index, error_code);
}

size_t F_dynamic_array_length_ex(Stru_DynamicArray_t* array, 
                                 enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_length_ex(array, error_code);
}

size_t F_dynamic_array_capacity_ex(Stru_DynamicArray_t* array,
                                   enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_capacity_ex(array, error_code);
}

bool F_dynamic_array_is_empty_ex(Stru_DynamicArray_t* array,
                                 enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_is_empty_ex(array, error_code);
}

bool F_dynamic_array_clear_ex(Stru_DynamicArray_t* array,
                              enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_clear_ex(array, error_code);
}

bool F_dynamic_array_resize_ex(Stru_DynamicArray_t* array, size_t new_capacity,
                               enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_resize_ex(array, new_capacity, error_code);
}

size_t F_dynamic_array_find_ex(Stru_DynamicArray_t* array, const void* item,
                               F_DynamicArrayCompare_t compare,
                               enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_find_ex(array, item, compare, error_code);
}

bool F_dynamic_array_push_batch_ex(Stru_DynamicArray_t* array, const void* items, size_t count,
                                   enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_push_batch_ex(array, items, count, error_code);
}

bool F_dynamic_array_foreach_ex(Stru_DynamicArray_t* array, 
                               F_DynamicArrayIterator_t iterator, void* user_data,
                               enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_foreach_ex(array, iterator, user_data, error_code);
}

bool F_dynamic_array_get_memory_stats_ex(Stru_DynamicArray_t* array, 
                                         size_t* total_bytes, size_t* used_bytes,
                                         enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_get_memory_stats_ex(array, total_bytes, used_bytes, error_code);
}

bool F_dynamic_array_shrink_to_fit_ex(Stru_DynamicArray_t* array,
                                      enum Eum_DynamicArrayError* error_code)
{
    return F_dynamic_array_shrink_to_fit_ex(array, error_code);
}

const Stru_DynamicArrayInterface_t* F_get_default_dynamic_array_interface(void)
{
    // 这个函数在CN_dynamic_array_interface_impl.c中实现
    extern const Stru_DynamicArrayInterface_t* F_get_default_dynamic_array_interface_impl(void);
    return F_get_default_dynamic_array_interface_impl();
}
