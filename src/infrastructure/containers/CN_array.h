/******************************************************************************
 * 文件名: CN_array.h
 * 功能: CN_Language动态数组容器
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义动态数组接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_ARRAY_H
#define CN_ARRAY_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 数组元素类型枚举
 */
typedef enum Eum_CN_ArrayElementType_t
{
    Eum_ARRAY_ELEMENT_INT8 = 0,     /**< 8位有符号整数 */
    Eum_ARRAY_ELEMENT_UINT8 = 1,    /**< 8位无符号整数 */
    Eum_ARRAY_ELEMENT_INT16 = 2,    /**< 16位有符号整数 */
    Eum_ARRAY_ELEMENT_UINT16 = 3,   /**< 16位无符号整数 */
    Eum_ARRAY_ELEMENT_INT32 = 4,    /**< 32位有符号整数 */
    Eum_ARRAY_ELEMENT_UINT32 = 5,   /**< 32位无符号整数 */
    Eum_ARRAY_ELEMENT_INT64 = 6,    /**< 64位有符号整数 */
    Eum_ARRAY_ELEMENT_UINT64 = 7,   /**< 64位无符号整数 */
    Eum_ARRAY_ELEMENT_FLOAT = 8,    /**< 单精度浮点数 */
    Eum_ARRAY_ELEMENT_DOUBLE = 9,   /**< 双精度浮点数 */
    Eum_ARRAY_ELEMENT_BOOL = 10,    /**< 布尔值 */
    Eum_ARRAY_ELEMENT_CHAR = 11,    /**< 字符 */
    Eum_ARRAY_ELEMENT_POINTER = 12, /**< 指针 */
    Eum_ARRAY_ELEMENT_CUSTOM = 13   /**< 自定义类型 */
} Eum_CN_ArrayElementType_t;

/**
 * @brief 动态数组结构体（不透明类型）
 */
typedef struct Stru_CN_Array_t Stru_CN_Array_t;

/**
 * @brief 数组迭代器结构体
 */
typedef struct Stru_CN_ArrayIterator_t
{
    Stru_CN_Array_t* array;        /**< 关联的数组 */
    size_t current_index;          /**< 当前索引 */
    void* current_element;         /**< 当前元素指针 */
} Stru_CN_ArrayIterator_t;

/**
 * @brief 数组比较函数类型
 * 
 * @param elem1 第一个元素
 * @param elem2 第二个元素
 * @return 比较结果：<0表示elem1<elem2，0表示相等，>0表示elem1>elem2
 */
typedef int (*CN_ArrayCompareFunc)(const void* elem1, const void* elem2);

/**
 * @brief 数组元素释放函数类型
 * 
 * @param element 要释放的元素
 */
typedef void (*CN_ArrayFreeFunc)(void* element);

/**
 * @brief 数组元素复制函数类型
 * 
 * @param src 源元素
 * @return 新分配的元素副本
 */
typedef void* (*CN_ArrayCopyFunc)(const void* src);

// ============================================================================
// 数组创建和销毁
// ============================================================================

/**
 * @brief 创建动态数组
 * 
 * @param element_type 元素类型
 * @param element_size 元素大小（字节），对自定义类型必需
 * @param initial_capacity 初始容量
 * @return 新创建的数组，失败返回NULL
 */
Stru_CN_Array_t* CN_array_create(Eum_CN_ArrayElementType_t element_type,
                                 size_t element_size, size_t initial_capacity);

/**
 * @brief 创建带自定义函数的数组
 * 
 * @param element_size 元素大小
 * @param initial_capacity 初始容量
 * @param compare_func 比较函数（可为NULL）
 * @param free_func 释放函数（可为NULL）
 * @param copy_func 复制函数（可为NULL）
 * @return 新创建的数组，失败返回NULL
 */
Stru_CN_Array_t* CN_array_create_custom(size_t element_size,
                                        size_t initial_capacity,
                                        CN_ArrayCompareFunc compare_func,
                                        CN_ArrayFreeFunc free_func,
                                        CN_ArrayCopyFunc copy_func);

/**
 * @brief 销毁数组
 * 
 * @param array 要销毁的数组
 */
void CN_array_destroy(Stru_CN_Array_t* array);

/**
 * @brief 清空数组（移除所有元素）
 * 
 * @param array 要清空的数组
 */
void CN_array_clear(Stru_CN_Array_t* array);

// ============================================================================
// 数组属性查询
// ============================================================================

/**
 * @brief 获取数组长度
 * 
 * @param array 数组
 * @return 数组长度（元素数量）
 */
size_t CN_array_length(const Stru_CN_Array_t* array);

/**
 * @brief 获取数组容量
 * 
 * @param array 数组
 * @return 数组容量
 */
size_t CN_array_capacity(const Stru_CN_Array_t* array);

/**
 * @brief 检查数组是否为空
 * 
 * @param array 数组
 * @return 如果数组为空返回true，否则返回false
 */
bool CN_array_is_empty(const Stru_CN_Array_t* array);

/**
 * @brief 获取元素大小
 * 
 * @param array 数组
 * @return 元素大小（字节）
 */
size_t CN_array_element_size(const Stru_CN_Array_t* array);

/**
 * @brief 获取元素类型
 * 
 * @param array 数组
 * @return 元素类型
 */
Eum_CN_ArrayElementType_t CN_array_element_type(const Stru_CN_Array_t* array);

// ============================================================================
// 元素访问和修改
// ============================================================================

/**
 * @brief 获取元素指针
 * 
 * @param array 数组
 * @param index 元素索引
 * @return 元素指针，索引无效返回NULL
 */
void* CN_array_get(const Stru_CN_Array_t* array, size_t index);

/**
 * @brief 设置元素值
 * 
 * @param array 数组
 * @param index 元素索引
 * @param element 元素值指针
 * @return 设置成功返回true，失败返回false
 */
bool CN_array_set(Stru_CN_Array_t* array, size_t index, const void* element);

/**
 * @brief 在末尾添加元素
 * 
 * @param array 数组
 * @param element 要添加的元素
 * @return 添加成功返回true，失败返回false
 */
bool CN_array_append(Stru_CN_Array_t* array, const void* element);

/**
 * @brief 在指定位置插入元素
 * 
 * @param array 数组
 * @param index 插入位置
 * @param element 要插入的元素
 * @return 插入成功返回true，失败返回false
 */
bool CN_array_insert(Stru_CN_Array_t* array, size_t index, const void* element);

/**
 * @brief 移除指定位置的元素
 * 
 * @param array 数组
 * @param index 要移除的元素索引
 * @return 移除成功返回true，失败返回false
 */
bool CN_array_remove(Stru_CN_Array_t* array, size_t index);

/**
 * @brief 移除并返回最后一个元素
 * 
 * @param array 数组
 * @param element 输出参数，接收移除的元素值
 * @return 移除成功返回true，失败返回false
 */
bool CN_array_pop(Stru_CN_Array_t* array, void* element);

// ============================================================================
// 数组操作
// ============================================================================

/**
 * @brief 确保数组有足够容量
 * 
 * @param array 数组
 * @param min_capacity 最小容量要求
 * @return 调整成功返回true，失败返回false
 */
bool CN_array_ensure_capacity(Stru_CN_Array_t* array, size_t min_capacity);

/**
 * @brief 缩小数组容量以匹配长度
 * 
 * @param array 数组
 * @return 调整成功返回true，失败返回false
 */
bool CN_array_shrink_to_fit(Stru_CN_Array_t* array);

/**
 * @brief 复制数组
 * 
 * @param src 源数组
 * @return 新创建的数组副本，失败返回NULL
 */
Stru_CN_Array_t* CN_array_copy(const Stru_CN_Array_t* src);

/**
 * @brief 连接两个数组
 * 
 * @param array1 第一个数组
 * @param array2 第二个数组
 * @return 新创建的连接后的数组，失败返回NULL
 */
Stru_CN_Array_t* CN_array_concat(const Stru_CN_Array_t* array1,
                                 const Stru_CN_Array_t* array2);

/**
 * @brief 获取子数组
 * 
 * @param array 源数组
 * @param start 起始索引
 * @param end 结束索引（不包含）
 * @return 新创建的子数组，失败返回NULL
 */
Stru_CN_Array_t* CN_array_slice(const Stru_CN_Array_t* array,
                                size_t start, size_t end);

// ============================================================================
// 搜索和排序
// ============================================================================

/**
 * @brief 查找元素
 * 
 * @param array 数组
 * @param element 要查找的元素
 * @return 元素索引，未找到返回SIZE_MAX
 */
size_t CN_array_find(const Stru_CN_Array_t* array, const void* element);

/**
 * @brief 查找元素（使用自定义比较函数）
 * 
 * @param array 数组
 * @param element 要查找的元素
 * @param compare_func 比较函数
 * @return 元素索引，未找到返回SIZE_MAX
 */
size_t CN_array_find_custom(const Stru_CN_Array_t* array, const void* element,
                            CN_ArrayCompareFunc compare_func);

/**
 * @brief 检查数组是否包含元素
 * 
 * @param array 数组
 * @param element 要检查的元素
 * @return 如果包含返回true，否则返回false
 */
bool CN_array_contains(const Stru_CN_Array_t* array, const void* element);

/**
 * @brief 排序数组
 * 
 * @param array 数组
 */
void CN_array_sort(Stru_CN_Array_t* array);

/**
 * @brief 使用自定义比较函数排序数组
 * 
 * @param array 数组
 * @param compare_func 比较函数
 */
void CN_array_sort_custom(Stru_CN_Array_t* array, CN_ArrayCompareFunc compare_func);

/**
 * @brief 反转数组
 * 
 * @param array 数组
 */
void CN_array_reverse(Stru_CN_Array_t* array);

// ============================================================================
// 迭代器支持
// ============================================================================

/**
 * @brief 创建数组迭代器
 * 
 * @param array 数组
 * @return 新创建的迭代器，失败返回NULL
 */
Stru_CN_ArrayIterator_t* CN_array_iterator_create(Stru_CN_Array_t* array);

/**
 * @brief 销毁迭代器
 * 
 * @param iterator 要销毁的迭代器
 */
void CN_array_iterator_destroy(Stru_CN_ArrayIterator_t* iterator);

/**
 * @brief 重置迭代器到开始位置
 * 
 * @param iterator 迭代器
 */
void CN_array_iterator_reset(Stru_CN_ArrayIterator_t* iterator);

/**
 * @brief 检查迭代器是否有下一个元素
 * 
 * @param iterator 迭代器
 * @return 如果有下一个元素返回true，否则返回false
 */
bool CN_array_iterator_has_next(const Stru_CN_ArrayIterator_t* iterator);

/**
 * @brief 获取下一个元素
 * 
 * @param iterator 迭代器
 * @return 下一个元素的指针，没有更多元素返回NULL
 */
void* CN_array_iterator_next(Stru_CN_ArrayIterator_t* iterator);

/**
 * @brief 获取当前元素
 * 
 * @param iterator 迭代器
 * @return 当前元素的指针
 */
void* CN_array_iterator_current(const Stru_CN_ArrayIterator_t* iterator);

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 交换两个元素
 * 
 * @param array 数组
 * @param index1 第一个元素索引
 * @param index2 第二个元素索引
 * @return 交换成功返回true，失败返回false
 */
bool CN_array_swap(Stru_CN_Array_t* array, size_t index1, size_t index2);

/**
 * @brief 填充数组
 * 
 * @param array 数组
 * @param element 填充元素
 * @param count 填充数量（从开始位置）
 * @return 填充成功返回true，失败返回false
 */
bool CN_array_fill(Stru_CN_Array_t* array, const void* element, size_t count);

/**
 * @brief 比较两个数组
 * 
 * @param array1 第一个数组
 * @param array2 第二个数组
 * @return 如果数组相等返回true，否则返回false
 */
bool CN_array_equal(const Stru_CN_Array_t* array1, const Stru_CN_Array_t* array2);

/**
 * @brief 转储数组信息到控制台（调试用）
 * 
 * @param array 数组
 */
void CN_array_dump(const Stru_CN_Array_t* array);

#endif // CN_ARRAY_H
