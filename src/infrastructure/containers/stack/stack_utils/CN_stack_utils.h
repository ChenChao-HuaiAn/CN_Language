/**
 * @file CN_stack_utils.h
 * @brief 栈工具模块头文件
 * 
 * 提供栈数据结构的高级工具函数接口声明。
 * 包括批量操作、复制、比较、查找等功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_STACK_UTILS_H
#define CN_STACK_UTILS_H

#include "stack_core/CN_stack_core.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 栈操作 ==================== */

/**
 * @brief 复制栈
 * 
 * 创建一个栈的深拷贝。
 * 
 * @param src_stack 源栈指针
 * @return 指向新创建的栈副本的指针，失败返回NULL
 */
Stru_Stack_t* F_stack_copy(Stru_Stack_t* src_stack);

/**
 * @brief 比较栈
 * 
 * 比较两个栈是否相等。
 * 
 * @param stack1 第一个栈指针
 * @param stack2 第二个栈指针
 * @param compare_func 比较函数，用于比较元素
 * @return 栈相等返回true，否则返回false
 * 
 * @note 比较函数应返回0表示相等，非0表示不相等
 */
bool F_stack_equals(
    Stru_Stack_t* stack1, 
    Stru_Stack_t* stack2, 
    int (*compare_func)(const void*, const void*));

/**
 * @brief 批量压栈
 * 
 * 将多个元素批量压入栈中。
 * 
 * @param stack 栈指针
 * @param items 元素指针数组
 * @param count 元素数量
 * @return 成功压入的元素数量
 */
size_t F_stack_push_batch(Stru_Stack_t* stack, void** items, size_t count);

/**
 * @brief 批量弹栈
 * 
 * 从栈中批量弹出多个元素。
 * 
 * @param stack 栈指针
 * @param out_items 输出参数，用于接收弹出的元素数组（可为NULL）
 * @param max_count 最大弹出数量
 * @return 实际弹出的元素数量
 */
size_t F_stack_pop_batch(Stru_Stack_t* stack, void** out_items, size_t max_count);

/* ==================== 转换操作 ==================== */

/**
 * @brief 将栈转换为数组
 * 
 * 将栈中的元素复制到数组中。
 * 
 * @param stack 栈指针
 * @param out_array 输出参数，用于接收数组（必须预先分配足够内存）
 * @return 成功复制的元素数量
 * 
 * @note 数组中的元素顺序取决于遍历方向（默认从栈底到栈顶）
 */
size_t F_stack_to_array(Stru_Stack_t* stack, void** out_array);

/**
 * @brief 从数组创建栈
 * 
 * 从数组创建栈。
 * 
 * @param items 元素指针数组
 * @param count 元素数量
 * @param item_size 每个元素的大小（字节）
 * @return 指向新创建的栈的指针，失败返回NULL
 * 
 * @note 数组中的第一个元素将成为栈底，最后一个元素将成为栈顶
 */
Stru_Stack_t* F_stack_from_array(void** items, size_t count, size_t item_size);

/* ==================== 查找操作 ==================== */

/**
 * @brief 查找元素
 * 
 * 在栈中查找指定元素。
 * 
 * @param stack 栈指针
 * @param item 要查找的元素指针
 * @param compare_func 比较函数，用于比较元素
 * @return 元素在栈中的位置（从栈底开始计数），未找到返回-1
 * 
 * @note 位置从0开始计数，0表示栈底
 */
int F_stack_find(
    Stru_Stack_t* stack, 
    void* item, 
    int (*compare_func)(const void*, const void*));

/**
 * @brief 检查是否包含元素
 * 
 * 检查栈是否包含指定元素。
 * 
 * @param stack 栈指针
 * @param item 要检查的元素指针
 * @param compare_func 比较函数，用于比较元素
 * @return 包含元素返回true，否则返回false
 */
bool F_stack_contains(
    Stru_Stack_t* stack, 
    void* item, 
    int (*compare_func)(const void*, const void*));

/**
 * @brief 查找满足条件的元素
 * 
 * 查找栈中第一个满足条件的元素。
 * 
 * @param stack 栈指针
 * @param predicate 谓词函数，返回true表示元素满足条件
 * @param context 上下文指针，传递给谓词函数
 * @return 指向满足条件的元素的指针，未找到返回NULL
 */
void* F_stack_find_if(
    Stru_Stack_t* stack, 
    bool (*predicate)(const void*, void*), 
    void* context);

/* ==================== 栈操作工具 ==================== */

/**
 * @brief 反转栈
 * 
 * 反转栈中元素的顺序。
 * 
 * @param stack 栈指针
 * @return 反转成功返回true，失败返回false
 * 
 * @note 反转后，原来的栈底变为栈顶，栈顶变为栈底
 */
bool F_stack_reverse(Stru_Stack_t* stack);

/**
 * @brief 获取栈中最小元素
 * 
 * 获取栈中的最小元素。
 * 
 * @param stack 栈指针
 * @param compare_func 比较函数，用于比较元素
 * @param out_item 输出参数，用于接收最小元素
 * @return 找到最小元素返回true，栈为空返回false
 */
bool F_stack_min(
    Stru_Stack_t* stack, 
    int (*compare_func)(const void*, const void*), 
    void* out_item);

/**
 * @brief 获取栈中最大元素
 * 
 * 获取栈中的最大元素。
 * 
 * @param stack 栈指针
 * @param compare_func 比较函数，用于比较元素
 * @param out_item 输出参数，用于接收最大元素
 * @return 找到最大元素返回true，栈为空返回false
 */
bool F_stack_max(
    Stru_Stack_t* stack, 
    int (*compare_func)(const void*, const void*), 
    void* out_item);

#ifdef __cplusplus
}
#endif

#endif // CN_STACK_UTILS_H
