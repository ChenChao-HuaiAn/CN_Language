/**
 * @file CN_stack_utils.c
 * @brief 栈工具模块实现文件
 * 
 * 实现栈数据结构的高级工具函数。
 * 包括批量操作、复制、比较、查找等功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_stack_utils.h"
#include "stack_iterator/CN_stack_iterator.h"
#include <stdlib.h>
#include <string.h>

/* ==================== 栈操作 ==================== */

Stru_Stack_t* F_stack_copy(Stru_Stack_t* src_stack)
{
    if (src_stack == NULL)
    {
        return NULL;
    }
    
    // 创建新栈
    Stru_Stack_t* dst_stack = F_create_stack(src_stack->item_size);
    if (dst_stack == NULL)
    {
        return NULL;
    }
    
    // 调整容量
    if (!F_stack_reserve(dst_stack, src_stack->top))
    {
        F_destroy_stack(dst_stack);
        return NULL;
    }
    
    // 复制所有元素
    for (size_t i = 0; i < src_stack->top; i++)
    {
        void* src_item = src_stack->items[i];
        if (src_item == NULL)
        {
            F_destroy_stack(dst_stack);
            return NULL;
        }
        
        if (!F_stack_push(dst_stack, src_item))
        {
            F_destroy_stack(dst_stack);
            return NULL;
        }
    }
    
    return dst_stack;
}

bool F_stack_equals(
    Stru_Stack_t* stack1, 
    Stru_Stack_t* stack2, 
    int (*compare_func)(const void*, const void*))
{
    if (stack1 == NULL || stack2 == NULL)
    {
        return false;
    }
    
    // 检查基本属性
    if (stack1->item_size != stack2->item_size)
    {
        return false;
    }
    
    if (stack1->top != stack2->top)
    {
        return false;
    }
    
    // 如果两个栈都为空，它们相等
    if (stack1->top == 0)
    {
        return true;
    }
    
    // 如果没有提供比较函数，进行逐字节比较
    if (compare_func == NULL)
    {
        for (size_t i = 0; i < stack1->top; i++)
        {
            void* item1 = stack1->items[i];
            void* item2 = stack2->items[i];
            
            if (item1 == NULL && item2 == NULL)
            {
                continue;
            }
            
            if (item1 == NULL || item2 == NULL)
            {
                return false;
            }
            
            if (memcmp(item1, item2, stack1->item_size) != 0)
            {
                return false;
            }
        }
    }
    else
    {
        // 使用比较函数
        for (size_t i = 0; i < stack1->top; i++)
        {
            void* item1 = stack1->items[i];
            void* item2 = stack2->items[i];
            
            if (item1 == NULL && item2 == NULL)
            {
                continue;
            }
            
            if (item1 == NULL || item2 == NULL)
            {
                return false;
            }
            
            if (compare_func(item1, item2) != 0)
            {
                return false;
            }
        }
    }
    
    return true;
}

size_t F_stack_push_batch(Stru_Stack_t* stack, void** items, size_t count)
{
    if (stack == NULL || items == NULL || count == 0)
    {
        return 0;
    }
    
    size_t pushed_count = 0;
    
    for (size_t i = 0; i < count; i++)
    {
        if (F_stack_push(stack, items[i]))
        {
            pushed_count++;
        }
        else
        {
            // 如果压栈失败，停止批量操作
            break;
        }
    }
    
    return pushed_count;
}

size_t F_stack_pop_batch(Stru_Stack_t* stack, void** out_items, size_t max_count)
{
    if (stack == NULL || max_count == 0)
    {
        return 0;
    }
    
    size_t popped_count = 0;
    
    for (size_t i = 0; i < max_count; i++)
    {
        void* out_item = (out_items != NULL) ? out_items[i] : NULL;
        
        if (!F_stack_pop(stack, out_item))
        {
            // 如果弹栈失败，停止批量操作
            break;
        }
        
        popped_count++;
    }
    
    return popped_count;
}

/* ==================== 转换操作 ==================== */

size_t F_stack_to_array(Stru_Stack_t* stack, void** out_array)
{
    if (stack == NULL || out_array == NULL)
    {
        return 0;
    }
    
    // 从栈底到栈顶复制元素
    for (size_t i = 0; i < stack->top; i++)
    {
        void* src_item = stack->items[i];
        if (src_item == NULL)
        {
            return i; // 返回已复制的元素数量
        }
        
        // 分配内存并复制数据
        void* dst_item = malloc(stack->item_size);
        if (dst_item == NULL)
        {
            return i; // 返回已复制的元素数量
        }
        
        memcpy(dst_item, src_item, stack->item_size);
        out_array[i] = dst_item;
    }
    
    return stack->top;
}

Stru_Stack_t* F_stack_from_array(void** items, size_t count, size_t item_size)
{
    if (items == NULL || count == 0 || item_size == 0)
    {
        return NULL;
    }
    
    // 创建栈
    Stru_Stack_t* stack = F_create_stack(item_size);
    if (stack == NULL)
    {
        return NULL;
    }
    
    // 调整容量
    if (!F_stack_reserve(stack, count))
    {
        F_destroy_stack(stack);
        return NULL;
    }
    
    // 批量压栈
    size_t pushed_count = F_stack_push_batch(stack, items, count);
    
    if (pushed_count != count)
    {
        // 如果未能压入所有元素，清理并返回NULL
        F_destroy_stack(stack);
        return NULL;
    }
    
    return stack;
}

/* ==================== 查找操作 ==================== */

int F_stack_find(
    Stru_Stack_t* stack, 
    void* item, 
    int (*compare_func)(const void*, const void*))
{
    if (stack == NULL || item == NULL)
    {
        return -1;
    }
    
    // 从栈底开始查找
    for (size_t i = 0; i < stack->top; i++)
    {
        void* stack_item = stack->items[i];
        if (stack_item == NULL)
        {
            continue;
        }
        
        bool found = false;
        
        if (compare_func == NULL)
        {
            // 逐字节比较
            found = (memcmp(stack_item, item, stack->item_size) == 0);
        }
        else
        {
            // 使用比较函数
            found = (compare_func(stack_item, item) == 0);
        }
        
        if (found)
        {
            return (int)i; // 返回位置（从栈底开始计数）
        }
    }
    
    return -1; // 未找到
}

bool F_stack_contains(
    Stru_Stack_t* stack, 
    void* item, 
    int (*compare_func)(const void*, const void*))
{
    return F_stack_find(stack, item, compare_func) != -1;
}

void* F_stack_find_if(
    Stru_Stack_t* stack, 
    bool (*predicate)(const void*, void*), 
    void* context)
{
    if (stack == NULL || predicate == NULL)
    {
        return NULL;
    }
    
    // 从栈底开始查找
    for (size_t i = 0; i < stack->top; i++)
    {
        void* item = stack->items[i];
        if (item == NULL)
        {
            continue;
        }
        
        if (predicate(item, context))
        {
            return item;
        }
    }
    
    return NULL;
}

/* ==================== 栈操作工具 ==================== */

bool F_stack_reverse(Stru_Stack_t* stack)
{
    if (stack == NULL)
    {
        return false; // NULL指针是错误
    }
    
    if (stack->top <= 1)
    {
        return true; // 空栈或只有一个元素，已经反转
    }
    
    // 使用双指针法反转数组
    size_t left = 0;
    size_t right = stack->top - 1;
    
    while (left < right)
    {
        // 交换元素
        void* temp = stack->items[left];
        stack->items[left] = stack->items[right];
        stack->items[right] = temp;
        
        left++;
        right--;
    }
    
    return true;
}

bool F_stack_min(
    Stru_Stack_t* stack, 
    int (*compare_func)(const void*, const void*), 
    void* out_item)
{
    if (stack == NULL || stack->top == 0 || compare_func == NULL || out_item == NULL)
    {
        return false;
    }
    
    // 初始化最小元素为第一个元素
    void* min_item = stack->items[0];
    if (min_item == NULL)
    {
        return false;
    }
    
    // 查找最小元素
    for (size_t i = 1; i < stack->top; i++)
    {
        void* current_item = stack->items[i];
        if (current_item == NULL)
        {
            continue;
        }
        
        if (compare_func(current_item, min_item) < 0)
        {
            min_item = current_item;
        }
    }
    
    // 复制最小元素到输出参数
    memcpy(out_item, min_item, stack->item_size);
    return true;
}

bool F_stack_max(
    Stru_Stack_t* stack, 
    int (*compare_func)(const void*, const void*), 
    void* out_item)
{
    if (stack == NULL || stack->top == 0 || compare_func == NULL || out_item == NULL)
    {
        return false;
    }
    
    // 初始化最大元素为第一个元素
    void* max_item = stack->items[0];
    if (max_item == NULL)
    {
        return false;
    }
    
    // 查找最大元素
    for (size_t i = 1; i < stack->top; i++)
    {
        void* current_item = stack->items[i];
        if (current_item == NULL)
        {
            continue;
        }
        
        if (compare_func(current_item, max_item) > 0)
        {
            max_item = current_item;
        }
    }
    
    // 复制最大元素到输出参数
    memcpy(out_item, max_item, stack->item_size);
    return true;
}
