/**
 * @file CN_stack_iterator.h
 * @brief 栈迭代器模块头文件
 * 
 * 提供栈数据结构的迭代器接口声明。
 * 支持从栈顶到栈底（LIFO顺序）和从栈底到栈顶（FIFO顺序）的遍历。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_STACK_ITERATOR_H
#define CN_STACK_ITERATOR_H

#include "stack_core/CN_stack_core.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 栈遍历方向枚举
 * 
 * 定义栈迭代器的遍历方向。
 */
typedef enum Eum_StackTraversalDirection
{
    Eum_STACK_TRAVERSAL_TOP_TO_BOTTOM = 0,  /**< 从栈顶到栈底（LIFO顺序） */
    Eum_STACK_TRAVERSAL_BOTTOM_TO_TOP = 1   /**< 从栈底到栈顶（FIFO顺序） */
} Eum_StackTraversalDirection;

/**
 * @brief 栈迭代器结构体
 * 
 * 用于遍历栈中元素的迭代器。
 */
typedef struct Stru_StackIterator_t
{
    Stru_Stack_t* stack;                    /**< 要遍历的栈 */
    size_t current_index;                   /**< 当前遍历位置 */
    size_t visited_count;                   /**< 已访问元素数量 */
    Eum_StackTraversalDirection direction;  /**< 遍历方向 */
} Stru_StackIterator_t;

/* ==================== 迭代器管理 ==================== */

/**
 * @brief 创建栈迭代器
 * 
 * 创建一个新的栈迭代器，默认从栈顶到栈底遍历。
 * 
 * @param stack 要遍历的栈
 * @return 指向新创建的迭代器的指针，失败返回NULL
 */
Stru_StackIterator_t* F_create_stack_iterator(Stru_Stack_t* stack);

/**
 * @brief 创建带方向的栈迭代器
 * 
 * 创建一个新的栈迭代器，指定遍历方向。
 * 
 * @param stack 要遍历的栈
 * @param direction 遍历方向
 * @return 指向新创建的迭代器的指针，失败返回NULL
 */
Stru_StackIterator_t* F_create_stack_iterator_with_direction(
    Stru_Stack_t* stack, 
    Eum_StackTraversalDirection direction);

/**
 * @brief 销毁栈迭代器
 * 
 * 释放迭代器占用的内存。
 * 
 * @param iterator 要销毁的迭代器指针
 * 
 * @note 如果iterator为NULL，函数不执行任何操作
 */
void F_destroy_stack_iterator(Stru_StackIterator_t* iterator);

/* ==================== 遍历操作 ==================== */

/**
 * @brief 检查是否有下一个元素
 * 
 * 检查迭代器是否还有下一个元素可以遍历。
 * 
 * @param iterator 迭代器指针
 * @return 有下一个元素返回true，否则返回false
 */
bool F_stack_iterator_has_next(Stru_StackIterator_t* iterator);

/**
 * @brief 获取下一个元素
 * 
 * 获取迭代器的下一个元素，并移动迭代器位置。
 * 
 * @param iterator 迭代器指针
 * @param out_item 输出参数，用于接收元素（可为NULL）
 * @return 获取成功返回true，没有更多元素返回false
 */
bool F_stack_iterator_next(Stru_StackIterator_t* iterator, void* out_item);

/**
 * @brief 获取当前元素
 * 
 * 获取迭代器当前位置的元素，但不移动迭代器位置。
 * 
 * @param iterator 迭代器指针
 * @return 指向当前元素的指针，没有当前元素返回NULL
 */
void* F_stack_iterator_current(Stru_StackIterator_t* iterator);

/**
 * @brief 重置迭代器
 * 
 * 将迭代器重置到起始位置。
 * 
 * @param iterator 迭代器指针
 * 
 * @note 重置后，迭代器将从起始位置重新开始遍历
 */
void F_stack_iterator_reset(Stru_StackIterator_t* iterator);

/**
 * @brief 设置遍历方向
 * 
 * 设置迭代器的遍历方向。
 * 
 * @param iterator 迭代器指针
 * @param direction 新的遍历方向
 * 
 * @note 设置方向后，迭代器会自动重置
 */
void F_stack_iterator_set_direction(
    Stru_StackIterator_t* iterator, 
    Eum_StackTraversalDirection direction);

/* ==================== 状态查询 ==================== */

/**
 * @brief 获取已访问元素数量
 * 
 * 返回迭代器已访问的元素数量。
 * 
 * @param iterator 迭代器指针
 * @return 已访问元素数量
 */
size_t F_stack_iterator_visited_count(Stru_StackIterator_t* iterator);

/**
 * @brief 获取剩余元素数量
 * 
 * 返回迭代器剩余未访问的元素数量。
 * 
 * @param iterator 迭代器指针
 * @return 剩余元素数量
 */
size_t F_stack_iterator_remaining_count(Stru_StackIterator_t* iterator);

/**
 * @brief 获取遍历方向
 * 
 * 返回迭代器的当前遍历方向。
 * 
 * @param iterator 迭代器指针
 * @return 遍历方向
 */
Eum_StackTraversalDirection F_stack_iterator_get_direction(
    Stru_StackIterator_t* iterator);

#ifdef __cplusplus
}
#endif

#endif // CN_STACK_ITERATOR_H
