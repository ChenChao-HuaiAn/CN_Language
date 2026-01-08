/**
 * @file CN_stack_iterator.c
 * @brief 栈迭代器模块实现文件
 * 
 * 实现栈数据结构的迭代器功能。
 * 支持从栈顶到栈底（LIFO顺序）和从栈底到栈顶（FIFO顺序）的遍历。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_stack_iterator.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ==================== 内部函数声明 ==================== */

/**
 * @brief 内部函数：获取起始索引
 * 
 * 根据遍历方向获取起始索引。
 * 
 * @param iterator 迭代器指针
 * @return 起始索引
 */
static size_t get_start_index(Stru_StackIterator_t* iterator);

/**
 * @brief 内部函数：获取下一个索引
 * 
 * 根据当前索引和遍历方向获取下一个索引。
 * 
 * @param iterator 迭代器指针
 * @param current_index 当前索引
 * @return 下一个索引，如果已到达末尾返回SIZE_MAX
 */
static size_t get_next_index(Stru_StackIterator_t* iterator, size_t current_index);

/**
 * @brief 内部函数：检查是否已到达末尾
 * 
 * 检查当前索引是否已到达遍历末尾。
 * 
 * @param iterator 迭代器指针
 * @param current_index 当前索引
 * @return 已到达末尾返回true，否则返回false
 */
static bool is_at_end(Stru_StackIterator_t* iterator, size_t current_index);

/* ==================== 内部函数实现 ==================== */

static size_t get_start_index(Stru_StackIterator_t* iterator)
{
    if (iterator == NULL || iterator->stack == NULL)
    {
        return 0;
    }
    
    if (iterator->direction == Eum_STACK_TRAVERSAL_TOP_TO_BOTTOM)
    {
        // 从栈顶开始：top - 1
        return (iterator->stack->top > 0) ? iterator->stack->top - 1 : 0;
    }
    else
    {
        // 从栈底开始：0
        return 0;
    }
}

static size_t get_next_index(Stru_StackIterator_t* iterator, size_t current_index)
{
    if (iterator == NULL || iterator->stack == NULL)
    {
        return SIZE_MAX;
    }
    
    if (iterator->direction == Eum_STACK_TRAVERSAL_TOP_TO_BOTTOM)
    {
        // 从栈顶到栈底：递减
        if (current_index == 0)
        {
            return SIZE_MAX; // 已到达栈底
        }
        return current_index - 1;
    }
    else
    {
        // 从栈底到栈顶：递增
        if (current_index >= iterator->stack->top - 1)
        {
            return SIZE_MAX; // 已到达栈顶
        }
        return current_index + 1;
    }
}

static bool is_at_end(Stru_StackIterator_t* iterator, size_t current_index)
{
    if (iterator == NULL || iterator->stack == NULL)
    {
        return true;
    }
    
    if (iterator->stack->top == 0)
    {
        return true; // 空栈
    }
    
    return current_index == SIZE_MAX;
}

/* ==================== 公开函数实现 ==================== */

Stru_StackIterator_t* F_create_stack_iterator(Stru_Stack_t* stack)
{
    return F_create_stack_iterator_with_direction(
        stack, 
        Eum_STACK_TRAVERSAL_TOP_TO_BOTTOM);
}

Stru_StackIterator_t* F_create_stack_iterator_with_direction(
    Stru_Stack_t* stack, 
    Eum_StackTraversalDirection direction)
{
    if (stack == NULL)
    {
        return NULL;
    }
    
    Stru_StackIterator_t* iterator = (Stru_StackIterator_t*)malloc(
        sizeof(Stru_StackIterator_t));
    if (iterator == NULL)
    {
        return NULL;
    }
    
    iterator->stack = stack;
    iterator->direction = direction;
    
    // 重置迭代器
    F_stack_iterator_reset(iterator);
    
    return iterator;
}

void F_destroy_stack_iterator(Stru_StackIterator_t* iterator)
{
    if (iterator == NULL)
    {
        return;
    }
    
    free(iterator);
}

bool F_stack_iterator_has_next(Stru_StackIterator_t* iterator)
{
    if (iterator == NULL || iterator->stack == NULL)
    {
        return false;
    }
    
    if (iterator->stack->top == 0)
    {
        return false; // 空栈
    }
    
    // 检查是否已访问所有元素
    return iterator->visited_count < iterator->stack->top;
}

bool F_stack_iterator_next(Stru_StackIterator_t* iterator, void* out_item)
{
    if (iterator == NULL || iterator->stack == NULL)
    {
        return false;
    }
    
    if (iterator->stack->top == 0)
    {
        return false; // 空栈
    }
    
    if (iterator->visited_count >= iterator->stack->top)
    {
        return false; // 已访问所有元素
    }
    
    // 如果是第一次访问，设置当前索引
    if (iterator->visited_count == 0)
    {
        iterator->current_index = get_start_index(iterator);
    }
    else
    {
        // 获取下一个索引
        iterator->current_index = get_next_index(iterator, iterator->current_index);
    }
    
    // 检查是否已到达末尾
    if (is_at_end(iterator, iterator->current_index))
    {
        return false;
    }
    
    // 获取当前元素
    void* current_item = iterator->stack->items[iterator->current_index];
    if (current_item == NULL)
    {
        return false;
    }
    
    // 复制数据到输出参数（如果提供）
    if (out_item != NULL)
    {
        memcpy(out_item, current_item, iterator->stack->item_size);
    }
    
    iterator->visited_count++;
    return true;
}

void* F_stack_iterator_current(Stru_StackIterator_t* iterator)
{
    if (iterator == NULL || iterator->stack == NULL)
    {
        return NULL;
    }
    
    if (iterator->stack->top == 0)
    {
        return NULL; // 空栈
    }
    
    if (iterator->visited_count == 0)
    {
        return NULL; // 尚未开始遍历
    }
    
    if (is_at_end(iterator, iterator->current_index))
    {
        return NULL; // 已到达末尾
    }
    
    return iterator->stack->items[iterator->current_index];
}

void F_stack_iterator_reset(Stru_StackIterator_t* iterator)
{
    if (iterator == NULL)
    {
        return;
    }
    
    iterator->current_index = 0;
    iterator->visited_count = 0;
}

void F_stack_iterator_set_direction(
    Stru_StackIterator_t* iterator, 
    Eum_StackTraversalDirection direction)
{
    if (iterator == NULL)
    {
        return;
    }
    
    iterator->direction = direction;
    F_stack_iterator_reset(iterator);
}

size_t F_stack_iterator_visited_count(Stru_StackIterator_t* iterator)
{
    if (iterator == NULL)
    {
        return 0;
    }
    
    return iterator->visited_count;
}

size_t F_stack_iterator_remaining_count(Stru_StackIterator_t* iterator)
{
    if (iterator == NULL || iterator->stack == NULL)
    {
        return 0;
    }
    
    if (iterator->stack->top < iterator->visited_count)
    {
        return 0;
    }
    
    return iterator->stack->top - iterator->visited_count;
}

Eum_StackTraversalDirection F_stack_iterator_get_direction(
    Stru_StackIterator_t* iterator)
{
    if (iterator == NULL)
    {
        return Eum_STACK_TRAVERSAL_TOP_TO_BOTTOM; // 默认值
    }
    
    return iterator->direction;
}
