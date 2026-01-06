/**
 * @file CN_stack.h
 * @brief 栈模块头文件
 * 
 * 提供栈数据结构的定义和接口声明。
 * 栈是一种后进先出（LIFO）的数据结构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_STACK_H
#define CN_STACK_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 栈结构体
 * 
 * 基于动态数组实现的栈数据结构。
 */
typedef struct Stru_Stack_t
{
    void** items;           /**< 元素指针数组 */
    size_t capacity;        /**< 当前分配的容量 */
    size_t top;             /**< 栈顶位置（下一个空闲位置） */
    size_t item_size;       /**< 每个元素的大小（字节） */
} Stru_Stack_t;

/**
 * @brief 创建栈
 * 
 * 分配并初始化一个新的栈。
 * 
 * @param item_size 每个元素的大小（字节）
 * @return 指向新创建的栈的指针，失败返回NULL
 */
Stru_Stack_t* F_create_stack(size_t item_size);

/**
 * @brief 销毁栈
 * 
 * 释放栈占用的所有内存。
 * 
 * @param stack 要销毁的栈指针
 * 
 * @note 如果stack为NULL，函数不执行任何操作
 */
void F_destroy_stack(Stru_Stack_t* stack);

/**
 * @brief 压栈
 * 
 * 将元素压入栈顶。
 * 
 * @param stack 栈指针
 * @param item 要压入的元素指针
 * @return 压栈成功返回true，失败返回false
 */
bool F_stack_push(Stru_Stack_t* stack, void* item);

/**
 * @brief 弹栈
 * 
 * 从栈顶弹出元素。
 * 
 * @param stack 栈指针
 * @param out_item 输出参数，用于接收弹出的元素（可为NULL）
 * @return 弹栈成功返回true，栈为空返回false
 */
bool F_stack_pop(Stru_Stack_t* stack, void* out_item);

/**
 * @brief 查看栈顶元素
 * 
 * 查看栈顶元素但不弹出。
 * 
 * @param stack 栈指针
 * @return 指向栈顶元素的指针，栈为空返回NULL
 */
void* F_stack_peek(Stru_Stack_t* stack);

/**
 * @brief 获取栈大小
 * 
 * 返回栈中元素的数量。
 * 
 * @param stack 栈指针
 * @return 栈大小，如果stack为NULL返回0
 */
size_t F_stack_size(Stru_Stack_t* stack);

/**
 * @brief 检查栈是否为空
 * 
 * 检查栈是否不包含任何元素。
 * 
 * @param stack 栈指针
 * @return 栈为空返回true，否则返回false
 */
bool F_stack_is_empty(Stru_Stack_t* stack);

/**
 * @brief 清空栈
 * 
 * 移除栈中的所有元素，但不释放栈本身。
 * 
 * @param stack 栈指针
 * 
 * @note 清空后栈大小变为0
 */
void F_stack_clear(Stru_Stack_t* stack);

#ifdef __cplusplus
}
#endif

#endif // CN_STACK_H
