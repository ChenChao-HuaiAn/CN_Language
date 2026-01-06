/**
 * @file CN_stack.c
 * @brief 栈模块实现文件
 * 
 * 实现栈数据结构的核心功能。
 * 包括创建、销毁、压栈、弹栈、查看等操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_stack.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 默认初始容量
#define CN_STACK_INITIAL_CAPACITY 16
// 扩容因子
#define CN_STACK_GROWTH_FACTOR 2

/**
 * @brief 内部函数：确保栈有足够容量
 * 
 * 检查栈是否有足够容量容纳新元素，如果没有则扩容。
 * 
 * @param stack 栈指针
 * @return 容量足够或扩容成功返回true，失败返回false
 */
static bool ensure_capacity(Stru_Stack_t* stack)
{
    if (stack == NULL)
    {
        return false;
    }
    
    // 如果栈已满，需要扩容
    if (stack->top >= stack->capacity)
    {
        size_t new_capacity = stack->capacity * CN_STACK_GROWTH_FACTOR;
        if (new_capacity < CN_STACK_INITIAL_CAPACITY)
        {
            new_capacity = CN_STACK_INITIAL_CAPACITY;
        }
        
        void** new_items = (void**)realloc(stack->items, new_capacity * sizeof(void*));
        if (new_items == NULL)
        {
            return false;
        }
        
        stack->items = new_items;
        stack->capacity = new_capacity;
    }
    
    return true;
}

Stru_Stack_t* F_create_stack(size_t item_size)
{
    if (item_size == 0)
    {
        return NULL;
    }
    
    // 分配栈结构体
    Stru_Stack_t* stack = (Stru_Stack_t*)malloc(sizeof(Stru_Stack_t));
    if (stack == NULL)
    {
        return NULL;
    }
    
    // 初始化字段
    stack->capacity = CN_STACK_INITIAL_CAPACITY;
    stack->top = 0;
    stack->item_size = item_size;
    
    // 分配初始元素数组
    stack->items = (void**)malloc(CN_STACK_INITIAL_CAPACITY * sizeof(void*));
    if (stack->items == NULL)
    {
        free(stack);
        return NULL;
    }
    
    // 初始化指针为NULL
    for (size_t i = 0; i < stack->capacity; i++)
    {
        stack->items[i] = NULL;
    }
    
    return stack;
}

void F_destroy_stack(Stru_Stack_t* stack)
{
    if (stack == NULL)
    {
        return;
    }
    
    // 释放所有元素
    for (size_t i = 0; i < stack->top; i++)
    {
        if (stack->items[i] != NULL)
        {
            free(stack->items[i]);
            stack->items[i] = NULL;
        }
    }
    
    // 释放元素数组
    if (stack->items != NULL)
    {
        free(stack->items);
        stack->items = NULL;
    }
    
    // 释放栈结构体
    free(stack);
}

bool F_stack_push(Stru_Stack_t* stack, void* item)
{
    if (stack == NULL || item == NULL)
    {
        return false;
    }
    
    // 确保有足够容量
    if (!ensure_capacity(stack))
    {
        return false;
    }
    
    // 分配新元素内存并复制数据
    void* new_item = malloc(stack->item_size);
    if (new_item == NULL)
    {
        return false;
    }
    
    memcpy(new_item, item, stack->item_size);
    stack->items[stack->top] = new_item;
    stack->top++;
    
    return true;
}

bool F_stack_pop(Stru_Stack_t* stack, void* out_item)
{
    if (stack == NULL || stack->top == 0)
    {
        return false;
    }
    
    // 获取栈顶元素
    void* top_item = stack->items[stack->top - 1];
    
    // 复制数据到输出参数（如果提供）
    if (out_item != NULL && top_item != NULL)
    {
        memcpy(out_item, top_item, stack->item_size);
    }
    
    // 释放栈顶元素
    if (top_item != NULL)
    {
        free(top_item);
    }
    
    stack->items[stack->top - 1] = NULL;
    stack->top--;
    
    return true;
}

void* F_stack_peek(Stru_Stack_t* stack)
{
    if (stack == NULL || stack->top == 0)
    {
        return NULL;
    }
    
    return stack->items[stack->top - 1];
}

size_t F_stack_size(Stru_Stack_t* stack)
{
    if (stack == NULL)
    {
        return 0;
    }
    
    return stack->top;
}

bool F_stack_is_empty(Stru_Stack_t* stack)
{
    if (stack == NULL)
    {
        return true;
    }
    
    return stack->top == 0;
}

void F_stack_clear(Stru_Stack_t* stack)
{
    if (stack == NULL)
    {
        return;
    }
    
    // 释放所有元素但不释放数组
    for (size_t i = 0; i < stack->top; i++)
    {
        if (stack->items[i] != NULL)
        {
            free(stack->items[i]);
            stack->items[i] = NULL;
        }
    }
    
    stack->top = 0;
}
