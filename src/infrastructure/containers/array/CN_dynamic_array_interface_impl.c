/**
 * @file CN_dynamic_array_interface_impl.c
 * @brief 动态数组接口实现文件
 * 
 * 实现动态数组抽象接口的具体实现。
 * 遵循CN_Language项目的接口设计规范。
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
#include <limits.h>
#include <stdint.h>

// 默认初始容量
#define CN_DYNAMIC_ARRAY_INITIAL_CAPACITY 16
// 扩容因子
#define CN_DYNAMIC_ARRAY_GROWTH_FACTOR 2

/* 内部辅助函数 */

static void set_error_code(enum Eum_DynamicArrayError* error_code, enum Eum_DynamicArrayError code)
{
    if (error_code != NULL)
    {
        *error_code = code;
    }
}

static bool validate_array_ptr(void* array, enum Eum_DynamicArrayError* error_code)
{
    if (array == NULL)
    {
        set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    return true;
}

/* 接口函数实现 */

static void* interface_create(size_t item_size, size_t initial_capacity, 
                              enum Eum_DynamicArrayError* error_code)
{
    return (void*)F_create_dynamic_array_ex(item_size, initial_capacity, error_code);
}

static void interface_destroy(void* array, enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return;
    }
    F_destroy_dynamic_array((Stru_DynamicArray_t*)array);
    set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
}

static bool interface_push(void* array, const void* item, enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return false;
    }
    return F_dynamic_array_push_ex((Stru_DynamicArray_t*)array, item, error_code);
}

static void* interface_get(void* array, size_t index, enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return NULL;
    }
    return F_dynamic_array_get_ex((Stru_DynamicArray_t*)array, index, error_code);
}

static bool interface_set(void* array, size_t index, const void* item, 
                          enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return false;
    }
    return F_dynamic_array_set_ex((Stru_DynamicArray_t*)array, index, item, error_code);
}

static bool interface_remove(void* array, size_t index, enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return false;
    }
    return F_dynamic_array_remove_ex((Stru_DynamicArray_t*)array, index, error_code);
}

static size_t interface_length(void* array, enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return 0;
    }
    return F_dynamic_array_length_ex((Stru_DynamicArray_t*)array, error_code);
}

static size_t interface_capacity(void* array, enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return 0;
    }
    return F_dynamic_array_capacity_ex((Stru_DynamicArray_t*)array, error_code);
}

static bool interface_is_empty(void* array, enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return true;
    }
    return F_dynamic_array_is_empty_ex((Stru_DynamicArray_t*)array, error_code);
}

static bool interface_clear(void* array, enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return false;
    }
    return F_dynamic_array_clear_ex((Stru_DynamicArray_t*)array, error_code);
}

static bool interface_resize(void* array, size_t new_capacity, enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return false;
    }
    return F_dynamic_array_resize_ex((Stru_DynamicArray_t*)array, new_capacity, error_code);
}

static size_t interface_find(void* array, const void* item, F_DynamicArrayCompare_t compare,
                             enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return SIZE_MAX;
    }
    return F_dynamic_array_find_ex((Stru_DynamicArray_t*)array, item, compare, error_code);
}

static bool interface_push_batch(void* array, const void* items, size_t count,
                                 enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return false;
    }
    return F_dynamic_array_push_batch_ex((Stru_DynamicArray_t*)array, items, count, error_code);
}

static bool interface_foreach(void* array, F_DynamicArrayIterator_t iterator, void* user_data,
                              enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return false;
    }
    return F_dynamic_array_foreach_ex((Stru_DynamicArray_t*)array, iterator, user_data, error_code);
}

static bool interface_get_memory_stats(void* array, size_t* total_bytes, size_t* used_bytes,
                                       enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return false;
    }
    
    return F_dynamic_array_get_memory_stats_ex((Stru_DynamicArray_t*)array, total_bytes, used_bytes, error_code);
}

static bool interface_shrink_to_fit(void* array, enum Eum_DynamicArrayError* error_code)
{
    if (!validate_array_ptr(array, error_code))
    {
        return false;
    }
    
    return F_dynamic_array_shrink_to_fit_ex((Stru_DynamicArray_t*)array, error_code);
}

/* 默认接口实例 */

static const Stru_DynamicArrayInterface_t g_default_dynamic_array_interface = {
    /* 创建和销毁 */
    .create = interface_create,
    .destroy = interface_destroy,
    
    /* 基本操作 */
    .push = interface_push,
    .get = interface_get,
    .set = interface_set,
    .remove = interface_remove,
    
    /* 查询操作 */
    .length = interface_length,
    .capacity = interface_capacity,
    .is_empty = interface_is_empty,
    
    /* 数组操作 */
    .clear = interface_clear,
    .resize = interface_resize,
    .find = interface_find,
    .push_batch = interface_push_batch,
    
    /* 迭代操作 */
    .foreach = interface_foreach,
    
    /* 内存管理 */
    .get_memory_stats = interface_get_memory_stats,
    .shrink_to_fit = interface_shrink_to_fit
};

/* 公共接口函数 */

const Stru_DynamicArrayInterface_t* F_get_default_dynamic_array_interface(void)
{
    return &g_default_dynamic_array_interface;
}
