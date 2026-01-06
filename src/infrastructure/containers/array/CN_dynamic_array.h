/**
 * @file CN_dynamic_array.h
 * @brief 动态数组模块头文件
 * 
 * 提供动态数组数据结构的定义和接口声明。
 * 动态数组支持自动扩容、随机访问和动态大小调整。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_DYNAMIC_ARRAY_H
#define CN_DYNAMIC_ARRAY_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 动态数组结构体
 * 
 * 动态数组是一种可以自动调整大小的数组数据结构。
 * 它支持在数组末尾高效地添加和删除元素。
 */
typedef struct Stru_DynamicArray_t
{
    void** items;           /**< 元素指针数组 */
    size_t capacity;        /**< 当前分配的容量 */
    size_t length;          /**< 当前元素数量 */
    size_t item_size;       /**< 每个元素的大小（字节） */
} Stru_DynamicArray_t;

/**
 * @brief 创建动态数组
 * 
 * 分配并初始化一个新的动态数组。
 * 
 * @param item_size 每个元素的大小（字节）
 * @return 指向新创建的动态数组的指针，失败返回NULL
 * 
 * @note 初始容量为16个元素，当数组满时会自动扩容
 */
Stru_DynamicArray_t* F_create_dynamic_array(size_t item_size);

/**
 * @brief 销毁动态数组
 * 
 * 释放动态数组占用的所有内存。
 * 
 * @param array 要销毁的动态数组指针
 * 
 * @note 如果array为NULL，函数不执行任何操作
 */
void F_destroy_dynamic_array(Stru_DynamicArray_t* array);

/**
 * @brief 向动态数组添加元素
 * 
 * 在动态数组的末尾添加一个新元素。
 * 
 * @param array 动态数组指针
 * @param item 要添加的元素指针
 * @return 添加成功返回true，失败返回false
 * 
 * @note 如果数组已满，会自动扩容为原来的2倍
 */
bool F_dynamic_array_push(Stru_DynamicArray_t* array, void* item);

/**
 * @brief 从动态数组获取元素
 * 
 * 获取指定索引处的元素。
 * 
 * @param array 动态数组指针
 * @param index 元素索引（0-based）
 * @return 指向元素的指针，索引无效返回NULL
 */
void* F_dynamic_array_get(Stru_DynamicArray_t* array, size_t index);

/**
 * @brief 从动态数组移除元素
 * 
 * 移除指定索引处的元素。
 * 
 * @param array 动态数组指针
 * @param index 要移除的元素索引
 * @return 移除成功返回true，索引无效返回false
 * 
 * @note 移除元素后，后面的元素会向前移动
 */
bool F_dynamic_array_remove(Stru_DynamicArray_t* array, size_t index);

/**
 * @brief 获取动态数组长度
 * 
 * 返回动态数组中当前元素的数量。
 * 
 * @param array 动态数组指针
 * @return 数组长度，如果array为NULL返回0
 */
size_t F_dynamic_array_length(Stru_DynamicArray_t* array);

/**
 * @brief 获取动态数组容量
 * 
 * 返回动态数组当前分配的容量。
 * 
 * @param array 动态数组指针
 * @return 数组容量，如果array为NULL返回0
 */
size_t F_dynamic_array_capacity(Stru_DynamicArray_t* array);

/**
 * @brief 清空动态数组
 * 
 * 移除动态数组中的所有元素，但不释放数组本身。
 * 
 * @param array 动态数组指针
 * 
 * @note 清空后数组长度变为0，但容量保持不变
 */
void F_dynamic_array_clear(Stru_DynamicArray_t* array);

/**
 * @brief 调整动态数组容量
 * 
 * 调整动态数组的容量到指定大小。
 * 
 * @param array 动态数组指针
 * @param new_capacity 新的容量大小
 * @return 调整成功返回true，失败返回false
 * 
 * @note 如果new_capacity小于当前长度，会截断数组
 */
bool F_dynamic_array_resize(Stru_DynamicArray_t* array, size_t new_capacity);

/**
 * @brief 遍历动态数组
 * 
 * 对动态数组中的每个元素执行指定的操作。
 * 
 * @param array 动态数组指针
 * @param callback 回调函数，接收元素指针和用户数据
 * @param user_data 传递给回调函数的用户数据
 */
void F_dynamic_array_foreach(Stru_DynamicArray_t* array, 
                            void (*callback)(void* item, void* user_data),
                            void* user_data);

#ifdef __cplusplus
}
#endif

#endif // CN_DYNAMIC_ARRAY_H
