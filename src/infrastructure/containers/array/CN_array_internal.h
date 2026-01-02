/******************************************************************************
 * 文件名: CN_array_internal.h
 * 功能: CN_Language动态数组容器内部结构定义
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义动态数组内部结构
 * 版权: MIT许可证
 * 
 * 注意: 此文件仅供CN_array模块内部使用，外部模块不应包含此文件
 ******************************************************************************/

#ifndef CN_ARRAY_INTERNAL_H
#define CN_ARRAY_INTERNAL_H

#include "CN_array.h"

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 动态数组结构体（完整定义，仅供内部使用）
 */
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
// 内部辅助函数声明
// ============================================================================

/**
 * @brief 确保数组有足够容量
 * 
 * @param array 数组
 * @param min_capacity 最小容量要求
 * @return 调整成功返回true，失败返回false
 */
bool CN_array_internal_ensure_capacity(Stru_CN_Array_t* array, size_t min_capacity);

/**
 * @brief 获取元素指针
 * 
 * @param array 数组
 * @param index 元素索引
 * @return 元素指针
 */
void* CN_array_internal_get_element_ptr(const Stru_CN_Array_t* array, size_t index);

/**
 * @brief 获取数组数据指针
 * 
 * @param array 数组
 * @return 数据指针
 */
void* CN_array_internal_get_data(const Stru_CN_Array_t* array);

/**
 * @brief 获取数组比较函数
 * 
 * @param array 数组
 * @return 比较函数指针
 */
CN_ArrayCompareFunc CN_array_internal_get_compare_func(const Stru_CN_Array_t* array);

/**
 * @brief 获取数组释放函数
 * 
 * @param array 数组
 * @return 释放函数指针
 */
CN_ArrayFreeFunc CN_array_internal_get_free_func(const Stru_CN_Array_t* array);

/**
 * @brief 获取数组复制函数
 * 
 * @param array 数组
 * @return 复制函数指针
 */
CN_ArrayCopyFunc CN_array_internal_get_copy_func(const Stru_CN_Array_t* array);

#endif // CN_ARRAY_INTERNAL_H
