/******************************************************************************
 * 文件名: CN_stack_circular.c
 * 功能: CN_Language栈容器 - 循环数组实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现循环数组栈
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_stack_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>

/**
 * @brief 创建循环数组实现的栈
 */
Stru_CN_Stack_t* CN_stack_internal_create_circular(
    size_t element_size,
    size_t initial_capacity,
    Eum_CN_StackThreadSafety_t thread_safety,
    CN_StackFreeFunc free_func,
    CN_StackCopyFunc copy_func)
{
    if (element_size == 0 || initial_capacity == 0)
    {
        return NULL;
    }
    
    Stru_CN_Stack_t* stack = (Stru_CN_Stack_t*)cn_malloc(sizeof(Stru_CN_Stack_t), "CN_stack_circular");
    if (stack == NULL)
    {
        return NULL;
    }
    
    // 初始化通用字段
    stack->implementation = Eum_STACK_IMPLEMENTATION_CIRCULAR;
    stack->element_size = element_size;
    stack->size = 0;
    stack->thread_safety = thread_safety;
    stack->free_func = free_func;
    stack->copy_func = copy_func;
    
    // 初始化循环数组特定字段
    stack->impl.circular.capacity = initial_capacity;
    stack->impl.circular.top_index = 0;
    stack->impl.circular.base_index = 0;
    stack->impl.circular.is_wrapped = false;
    
    // 分配数据数组
    size_t data_size = element_size * initial_capacity;
    stack->impl.circular.data = cn_malloc(data_size, "CN_stack_circular_data");
    if (stack->impl.circular.data == NULL)
    {
        cn_free(stack);
        return NULL;
    }
    
    // 初始化锁
    if (!CN_stack_internal_init_lock(stack))
    {
        cn_free(stack->impl.circular.data);
        cn_free(stack);
        return NULL;
    }
    
    return stack;
}

/**
 * @brief 销毁循环数组实现的栈
 */
void CN_stack_internal_destroy_circular(Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return;
    }
    
    CN_stack_internal_lock(stack);
    
    // 如果有释放函数，释放所有元素
    if (stack->free_func != NULL)
    {
        for (size_t i = 0; i < stack->size; i++)
        {
            void* element = CN_stack_internal_circular_get_element(stack, i);
            if (element != NULL)
            {
                stack->free_func(element);
            }
        }
    }
    
    // 释放数据数组
    if (stack->impl.circular.data != NULL)
    {
        cn_free(stack->impl.circular.data);
        stack->impl.circular.data = NULL;
    }
    
    CN_stack_internal_unlock(stack);
    CN_stack_internal_destroy_lock(stack);
}

/**
 * @brief 循环数组实现的压栈操作
 */
bool CN_stack_internal_circular_push(Stru_CN_Stack_t* stack, const void* element)
{
    if (stack == NULL || element == NULL)
    {
        return false;
    }
    
    // 检查是否需要扩容
    if (stack->size >= stack->impl.circular.capacity)
    {
        size_t new_capacity = stack->impl.circular.capacity * 2;
        if (!CN_stack_internal_circular_ensure_capacity(stack, new_capacity))
        {
            return false;
        }
    }
    
    // 计算插入位置
    size_t insert_index = stack->impl.circular.top_index;
    
    // 复制元素
    void* dest = (char*)stack->impl.circular.data + (insert_index * stack->element_size);
    memcpy(dest, element, stack->element_size);
    
    // 更新栈顶索引
    stack->impl.circular.top_index++;
    if (stack->impl.circular.top_index >= stack->impl.circular.capacity)
    {
        stack->impl.circular.top_index = 0;
        stack->impl.circular.is_wrapped = true;
    }
    
    stack->size++;
    
    return true;
}

/**
 * @brief 循环数组实现的弹栈操作
 */
bool CN_stack_internal_circular_pop(Stru_CN_Stack_t* stack, void* element)
{
    if (stack == NULL || stack->size == 0)
    {
        return false;
    }
    
    // 计算栈顶元素位置（考虑回绕）
    size_t pop_index;
    if (stack->impl.circular.top_index == 0)
    {
        if (stack->impl.circular.is_wrapped)
        {
            pop_index = stack->impl.circular.capacity - 1;
        }
        else
        {
            // 栈为空但top_index为0，这是错误状态
            return false;
        }
    }
    else
    {
        pop_index = stack->impl.circular.top_index - 1;
    }
    
    // 获取元素
    void* src = (char*)stack->impl.circular.data + (pop_index * stack->element_size);
    
    // 如果提供了输出参数，复制元素值
    if (element != NULL)
    {
        memcpy(element, src, stack->element_size);
    }
    
    // 如果有释放函数，释放元素
    if (stack->free_func != NULL)
    {
        stack->free_func(src);
    }
    
    // 更新栈顶索引
    stack->impl.circular.top_index = pop_index;
    
    // 如果弹出后栈顶回到base_index，重置回绕标志
    if (stack->impl.circular.top_index == stack->impl.circular.base_index)
    {
        stack->impl.circular.is_wrapped = false;
    }
    
    stack->size--;
    
    return true;
}

/**
 * @brief 循环数组实现的查看栈顶操作
 */
void* CN_stack_internal_circular_peek(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL || stack->size == 0)
    {
        return NULL;
    }
    
    // 计算栈顶元素位置（考虑回绕）
    size_t peek_index;
    if (stack->impl.circular.top_index == 0)
    {
        if (stack->impl.circular.is_wrapped)
        {
            peek_index = stack->impl.circular.capacity - 1;
        }
        else
        {
            // 栈为空但top_index为0，这是错误状态
            return NULL;
        }
    }
    else
    {
        peek_index = stack->impl.circular.top_index - 1;
    }
    
    return (char*)stack->impl.circular.data + (peek_index * stack->element_size);
}

/**
 * @brief 确保循环数组栈有足够容量
 */
bool CN_stack_internal_circular_ensure_capacity(Stru_CN_Stack_t* stack, size_t min_capacity)
{
    if (stack == NULL || min_capacity <= stack->impl.circular.capacity)
    {
        return true;
    }
    
    // 计算新的容量（至少翻倍）
    size_t new_capacity = stack->impl.circular.capacity;
    while (new_capacity < min_capacity)
    {
        new_capacity *= 2;
    }
    
    // 分配新的数据数组
    size_t new_data_size = stack->element_size * new_capacity;
    void* new_data = cn_malloc(new_data_size, "CN_stack_circular_new_data");
    if (new_data == NULL)
    {
        return false;
    }
    
    // 复制现有元素
    if (stack->size > 0)
    {
        if (!stack->impl.circular.is_wrapped && 
            stack->impl.circular.base_index < stack->impl.circular.top_index)
        {
            // 连续存储，一次复制
            void* src = (char*)stack->impl.circular.data + (stack->impl.circular.base_index * stack->element_size);
            memcpy(new_data, src, stack->size * stack->element_size);
        }
        else
        {
            // 回绕存储，分两次复制
            size_t first_part = stack->impl.circular.capacity - stack->impl.circular.base_index;
            if (first_part > stack->size)
            {
                first_part = stack->size;
            }
            
            void* src_first = (char*)stack->impl.circular.data + (stack->impl.circular.base_index * stack->element_size);
            memcpy(new_data, src_first, first_part * stack->element_size);
            
            if (first_part < stack->size)
            {
                size_t second_part = stack->size - first_part;
                void* src_second = stack->impl.circular.data;
                void* dst_second = (char*)new_data + (first_part * stack->element_size);
                memcpy(dst_second, src_second, second_part * stack->element_size);
            }
        }
    }
    
    // 释放旧数据
    cn_free(stack->impl.circular.data);
    
    // 更新队列结构
    stack->impl.circular.data = new_data;
    stack->impl.circular.capacity = new_capacity;
    stack->impl.circular.base_index = 0;
    stack->impl.circular.top_index = stack->size;
    stack->impl.circular.is_wrapped = false;
    
    return true;
}

/**
 * @brief 获取循环数组栈元素指针
 */
void* CN_stack_internal_circular_get_element(const Stru_CN_Stack_t* stack, size_t index)
{
    if (stack == NULL || index >= stack->size)
    {
        return NULL;
    }
    
    // 计算物理索引（考虑回绕）
    size_t physical_index = CN_stack_internal_circular_compute_index(stack, index);
    return (char*)stack->impl.circular.data + (physical_index * stack->element_size);
}

/**
 * @brief 计算循环数组索引
 */
size_t CN_stack_internal_circular_compute_index(const Stru_CN_Stack_t* stack, size_t logical_index)
{
    if (stack == NULL)
    {
        return 0;
    }
    
    // 计算物理索引（考虑回绕）
    size_t physical_index = stack->impl.circular.base_index + logical_index;
    if (physical_index >= stack->impl.circular.capacity)
    {
        physical_index -= stack->impl.circular.capacity;
    }
    
    return physical_index;
}

/**
 * @brief 复制循环数组栈
 */
Stru_CN_Stack_t* CN_stack_internal_circular_copy(const Stru_CN_Stack_t* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    // 创建新栈
    Stru_CN_Stack_t* dst = CN_stack_internal_create_circular(
        src->element_size,
        src->impl.circular.capacity,
        src->thread_safety,
        src->free_func,
        src->copy_func);
    
    if (dst == NULL)
    {
        return NULL;
    }
    
    // 复制元素
    if (src->size > 0)
    {
        // 复制所有元素
        for (size_t i = 0; i < src->size; i++)
        {
            void* src_element = CN_stack_internal_circular_get_element(src, i);
            if (src_element == NULL)
            {
                CN_stack_internal_destroy_circular(dst);
                return NULL;
            }
            
            if (!CN_stack_internal_circular_push(dst, src_element))
            {
                CN_stack_internal_destroy_circular(dst);
                return NULL;
            }
        }
    }
    
    return dst;
}

/**
 * @brief 清空循环数组栈
 */
void CN_stack_internal_circular_clear(Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return;
    }
    
    CN_stack_internal_lock(stack);
    
    // 如果有释放函数，释放所有元素
    if (stack->free_func != NULL)
    {
        for (size_t i = 0; i < stack->size; i++)
        {
            void* element = CN_stack_internal_circular_get_element(stack, i);
            if (element != NULL)
            {
                stack->free_func(element);
            }
        }
    }
    
    // 重置栈
    stack->impl.circular.top_index = 0;
    stack->impl.circular.base_index = 0;
    stack->impl.circular.is_wrapped = false;
    stack->size = 0;
    
    CN_stack_internal_unlock(stack);
}

/**
 * @brief 获取循环数组栈容量
 */
size_t CN_stack_internal_circular_capacity(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return 0;
    }
    
    return stack->impl.circular.capacity;
}

/**
 * @brief 检查循环数组栈是否已满
 */
bool CN_stack_internal_circular_is_full(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return false;
    }
    
    return stack->size >= stack->impl.circular.capacity;
}

/**
 * @brief 缩小循环数组栈容量以匹配大小
 */
bool CN_stack_internal_circular_shrink_to_fit(Stru_CN_Stack_t* stack)
{
    if (stack == NULL || stack->size == stack->impl.circular.capacity)
    {
        return true;
    }
    
    return CN_stack_internal_circular_ensure_capacity(stack, stack->size);
}

/**
 * @brief 检查循环数组栈是否回绕
 */
bool CN_stack_internal_circular_is_wrapped(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return false;
    }
    
    return stack->impl.circular.is_wrapped;
}

/**
 * @brief 获取循环数组栈的剩余空间
 */
size_t CN_stack_internal_circular_remaining_space(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return 0;
    }
    
    if (CN_stack_internal_circular_is_full(stack))
    {
        return 0;
    }
    
    if (stack->impl.circular.base_index <= stack->impl.circular.top_index)
    {
        return stack->impl.circular.capacity - (stack->impl.circular.top_index - stack->impl.circular.base_index);
    }
    else
    {
        return stack->impl.circular.base_index - stack->impl.circular.top_index;
    }
}
