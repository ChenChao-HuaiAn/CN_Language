/**
 * @file CN_dynamic_array_operations.c
 * @brief 动态数组操作功能实现文件
 * 
 * 实现动态数组模块的高级操作功能。
 * 包括数组操作、批量操作、迭代操作和内存管理功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_dynamic_array.h"
#include "CN_dynamic_array_operations.h"
#include "CN_dynamic_array_utils.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

// 默认初始容量
#define CN_DYNAMIC_ARRAY_INITIAL_CAPACITY 16
// 扩容因子
#define CN_DYNAMIC_ARRAY_GROWTH_FACTOR 2
// 最小容量（防止容量为0）
#define CN_DYNAMIC_ARRAY_MIN_CAPACITY 4

/**
 * @brief 清空动态数组（带错误码）
 * 
 * 移除动态数组中的所有元素，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 清空成功返回true，失败返回false
 */
bool F_dynamic_array_clear_ex(Stru_DynamicArray_t* array,
                              enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
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
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

/**
 * @brief 调整动态数组容量（带错误码）
 * 
 * 调整动态数组的容量到指定大小，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param new_capacity 新的容量大小
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 调整成功返回true，失败返回false
 */
bool F_dynamic_array_resize_ex(Stru_DynamicArray_t* array, size_t new_capacity,
                               enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
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
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

/**
 * @brief 查找动态数组元素（带错误码）
 * 
 * 在动态数组中查找指定元素，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param item 要查找的元素指针
 * @param compare 比较函数（NULL表示直接比较内存）
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 元素索引，未找到返回SIZE_MAX
 */
size_t F_dynamic_array_find_ex(Stru_DynamicArray_t* array, const void* item,
                               F_DynamicArrayCompare_t compare,
                               enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code) || item == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
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
                F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
                return i;
            }
        }
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_ITEM_NOT_FOUND);
    return SIZE_MAX;
}

/**
 * @brief 批量添加元素到动态数组（带错误码）
 * 
 * 批量添加多个元素到动态数组末尾，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param items 元素数组指针
 * @param count 元素数量
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 添加成功返回true，失败返回false
 */
bool F_dynamic_array_push_batch_ex(Stru_DynamicArray_t* array, const void* items, size_t count,
                                   enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code) || items == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    if (count == 0)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
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
        void* new_item = F_dynamic_array_copy_item_data(array, src + (i * array->item_size), error_code);
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
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

/**
 * @brief 遍历动态数组（带索引和错误码）
 * 
 * 对动态数组中的每个元素执行指定的操作，包含索引和错误处理。
 * 
 * @param array 动态数组指针
 * @param iterator 迭代器回调函数
 * @param user_data 用户数据
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 是否成功完成遍历（如果迭代器提前停止返回false）
 */
bool F_dynamic_array_foreach_ex(Stru_DynamicArray_t* array, 
                               F_DynamicArrayIterator_t iterator, void* user_data,
                               enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code) || iterator == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    for (size_t i = 0; i < array->length; i++)
    {
        if (!iterator(array->items[i], i, user_data))
        {
            // 迭代器请求停止，遍历未完成
            F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
            return false;
        }
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

/**
 * @brief 获取动态数组内存使用统计（带错误码）
 * 
 * 获取动态数组的内存使用情况，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param total_bytes 输出参数，总字节数（可为NULL）
 * @param used_bytes 输出参数，已使用字节数（可为NULL）
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 获取成功返回true，失败返回false
 */
bool F_dynamic_array_get_memory_stats_ex(Stru_DynamicArray_t* array, 
                                         size_t* total_bytes, size_t* used_bytes,
                                         enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
    {
        return false;
    }
    
    // 计算总字节数（结构体 + 指针数组 + 元素数据）
    size_t total = sizeof(Stru_DynamicArray_t) + 
                   (array->capacity * sizeof(void*)) + 
                   (array->length * array->item_size);
    
    // 计算已使用字节数（结构体 + 指针数组 + 元素数据）
    size_t used = sizeof(Stru_DynamicArray_t) + 
                  (array->capacity * sizeof(void*)) + 
                  (array->length * array->item_size);
    
    if (total_bytes != NULL)
    {
        *total_bytes = total;
    }
    
    if (used_bytes != NULL)
    {
        *used_bytes = used;
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

/**
 * @brief 压缩动态数组（带错误码）
 * 
 * 压缩动态数组，释放未使用的内存，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 压缩成功返回true，失败返回false
 */
bool F_dynamic_array_shrink_to_fit_ex(Stru_DynamicArray_t* array,
                                      enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
    {
        return false;
    }
    
    // 如果容量大于长度，调整容量到长度
    if (array->capacity > array->length)
    {
        return F_dynamic_array_resize_ex(array, array->length, error_code);
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

/* 高级操作函数实现 */

/**
 * @brief 排序动态数组（带错误码）
 * 
 * 对动态数组进行排序，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param compare 比较函数
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 排序成功返回true，失败返回false
 */
bool F_dynamic_array_sort_ex(Stru_DynamicArray_t* array, F_DynamicArrayCompare_t compare,
                             enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code) || compare == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    if (array->length <= 1)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
        return true;
    }
    
    // 使用简单的冒泡排序（对于小数组足够）
    for (size_t i = 0; i < array->length - 1; i++)
    {
        for (size_t j = 0; j < array->length - i - 1; j++)
        {
            void* item1 = array->items[j];
            void* item2 = array->items[j + 1];
            
            if (compare(item1, item2) > 0)
            {
                // 交换元素
                void* temp = array->items[j];
                array->items[j] = array->items[j + 1];
                array->items[j + 1] = temp;
            }
        }
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

/**
 * @brief 反转动态数组（带错误码）
 * 
 * 反转动态数组中元素的顺序，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 反转成功返回true，失败返回false
 */
bool F_dynamic_array_reverse_ex(Stru_DynamicArray_t* array,
                                enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
    {
        return false;
    }
    
    if (array->length <= 1)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
        return true;
    }
    
    // 反转数组
    size_t left = 0;
    size_t right = array->length - 1;
    
    while (left < right)
    {
        // 交换元素
        void* temp = array->items[left];
        array->items[left] = array->items[right];
        array->items[right] = temp;
        
        left++;
        right--;
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

/**
 * @brief 映射动态数组（带错误码）
 * 
 * 对动态数组中的每个元素应用指定的函数，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param mapper 映射函数
 * @param user_data 用户数据
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 映射成功返回true，失败返回false
 */
bool F_dynamic_array_map_ex(Stru_DynamicArray_t* array,
                            void (*mapper)(void* item, void* user_data),
                            void* user_data, enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code) || mapper == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    for (size_t i = 0; i < array->length; i++)
    {
        if (array->items[i] != NULL)
        {
            mapper(array->items[i], user_data);
        }
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

/**
 * @brief 过滤动态数组（带错误码）
 * 
 * 根据条件过滤动态数组中的元素，返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param filter 过滤函数（返回true保留元素）
 * @param user_data 用户数据
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 过滤成功返回true，失败返回false
 */
bool F_dynamic_array_filter_ex(Stru_DynamicArray_t* array,
                               bool (*filter)(void* item, void* user_data),
                               void* user_data, enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code) || filter == NULL)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER);
        return false;
    }
    
    size_t write_index = 0;
    
    for (size_t read_index = 0; read_index < array->length; read_index++)
    {
        void* item = array->items[read_index];
        
        if (item != NULL && filter(item, user_data))
        {
            // 保留元素
            if (write_index != read_index)
            {
                array->items[write_index] = item;
                array->items[read_index] = NULL;
            }
            write_index++;
        }
        else
        {
            // 释放被过滤掉的元素
            if (item != NULL)
            {
                free(item);
                array->items[read_index] = NULL;
            }
        }
    }
    
    // 更新长度
    array->length = write_index;
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return true;
}

/**
 * @brief 获取动态数组切片（带错误码）
 * 
 * 获取动态数组的子数组（切片），返回详细错误码。
 * 
 * @param array 动态数组指针
 * @param start 起始索引
 * @param end 结束索引（不包含）
 * @param error_code 输出参数，错误码（可为NULL）
 * @return 新的动态数组指针，失败返回NULL
 */
Stru_DynamicArray_t* F_dynamic_array_slice_ex(Stru_DynamicArray_t* array,
                                              size_t start, size_t end,
                                              enum Eum_DynamicArrayError* error_code)
{
    if (!F_dynamic_array_validate_array(array, error_code))
    {
        return NULL;
    }
    
    // 验证索引范围
    if (start > end || end > array->length)
    {
        F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS);
        return NULL;
    }
    
    // 计算切片长度
    size_t slice_length = end - start;
    
    // 创建新的动态数组
    Stru_DynamicArray_t* slice = F_create_dynamic_array_ex(array->item_size, slice_length, error_code);
    if (slice == NULL)
    {
        return NULL;
    }
    
    // 复制元素
    for (size_t i = start; i < end; i++)
    {
        void* item = array->items[i];
        if (item != NULL)
        {
            if (!F_dynamic_array_push_ex(slice, item, error_code))
            {
                F_destroy_dynamic_array(slice);
                return NULL;
            }
        }
    }
    
    F_dynamic_array_set_error_code(error_code, Eum_DYNAMIC_ARRAY_SUCCESS);
    return slice;
}
