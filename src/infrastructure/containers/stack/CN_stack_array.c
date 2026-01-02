/******************************************************************************
 * 文件名: CN_stack_array.c
 * 功能: CN_Language栈容器 - 数组实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现数组栈
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_stack_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>

/**
 * @brief 创建数组实现的栈
 */
Stru_CN_Stack_t* CN_stack_internal_create_array(
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
    
    Stru_CN_Stack_t* stack = (Stru_CN_Stack_t*)cn_malloc(sizeof(Stru_CN_Stack_t), "CN_stack_array");
    if (stack == NULL)
    {
        return NULL;
    }
    
    // 初始化通用字段
    stack->implementation = Eum_STACK_IMPLEMENTATION_ARRAY;
    stack->element_size = element_size;
    stack->size = 0;
    stack->thread_safety = thread_safety;
    stack->free_func = free_func;
    stack->copy_func = copy_func;
    
    // 初始化数组特定字段
    stack->impl.array.capacity = initial_capacity;
    stack->impl.array.top_index = 0;
    
    // 分配数据数组
    size_t data_size = element_size * initial_capacity;
    stack->impl.array.data = cn_malloc(data_size, "CN_stack_array_data");
    if (stack->impl.array.data == NULL)
    {
        cn_free(stack);
        return NULL;
    }
    
    // 初始化锁
    if (!CN_stack_internal_init_lock(stack))
    {
        cn_free(stack->impl.array.data);
        cn_free(stack);
        return NULL;
    }
    
    return stack;
}

/**
 * @brief 销毁数组实现的栈
 */
void CN_stack_internal_destroy_array(Stru_CN_Stack_t* stack)
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
            void* element = CN_stack_internal_array_get_element(stack, i);
            if (element != NULL)
            {
                stack->free_func(element);
            }
        }
    }
    
    // 释放数据数组
    if (stack->impl.array.data != NULL)
    {
        cn_free(stack->impl.array.data);
        stack->impl.array.data = NULL;
    }
    
    CN_stack_internal_unlock(stack);
    CN_stack_internal_destroy_lock(stack);
}

/**
 * @brief 数组实现的压栈操作
 */
bool CN_stack_internal_array_push(Stru_CN_Stack_t* stack, const void* element)
{
    if (stack == NULL || element == NULL)
    {
        return false;
    }
    
    // 检查是否需要扩容
    if (stack->size >= stack->impl.array.capacity)
    {
        size_t new_capacity = stack->impl.array.capacity * 2;
        if (!CN_stack_internal_array_ensure_capacity(stack, new_capacity))
        {
            return false;
        }
    }
    
    // 复制元素到栈顶
    void* dest = (char*)stack->impl.array.data + (stack->impl.array.top_index * stack->element_size);
    memcpy(dest, element, stack->element_size);
    
    stack->impl.array.top_index++;
    stack->size++;
    
    return true;
}

/**
 * @brief 数组实现的弹栈操作
 */
bool CN_stack_internal_array_pop(Stru_CN_Stack_t* stack, void* element)
{
    if (stack == NULL || stack->size == 0)
    {
        return false;
    }
    
    // 获取栈顶元素
    stack->impl.array.top_index--;
    void* src = (char*)stack->impl.array.data + (stack->impl.array.top_index * stack->element_size);
    
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
    
    stack->size--;
    
    return true;
}

/**
 * @brief 数组实现的查看栈顶操作
 */
void* CN_stack_internal_array_peek(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL || stack->size == 0)
    {
        return NULL;
    }
    
    size_t top_index = stack->impl.array.top_index - 1;
    return (char*)stack->impl.array.data + (top_index * stack->element_size);
}

/**
 * @brief 确保数组栈有足够容量
 */
bool CN_stack_internal_array_ensure_capacity(Stru_CN_Stack_t* stack, size_t min_capacity)
{
    if (stack == NULL || min_capacity <= stack->impl.array.capacity)
    {
        return true;
    }
    
    // 计算新的容量（至少翻倍）
    size_t new_capacity = stack->impl.array.capacity;
    while (new_capacity < min_capacity)
    {
        new_capacity *= 2;
    }
    
    // 分配新的数据数组
    size_t new_data_size = stack->element_size * new_capacity;
    void* new_data = cn_malloc(new_data_size, "CN_stack_array_new_data");
    if (new_data == NULL)
    {
        return false;
    }
    
    // 复制现有元素
    if (stack->size > 0)
    {
        memcpy(new_data, stack->impl.array.data, stack->size * stack->element_size);
    }
    
    // 释放旧数据
    cn_free(stack->impl.array.data);
    
    // 更新队列结构
    stack->impl.array.data = new_data;
    stack->impl.array.capacity = new_capacity;
    
    return true;
}

/**
 * @brief 获取数组栈元素指针
 */
void* CN_stack_internal_array_get_element(const Stru_CN_Stack_t* stack, size_t index)
{
    if (stack == NULL || index >= stack->size)
    {
        return NULL;
    }
    
    return (char*)stack->impl.array.data + (index * stack->element_size);
}

/**
 * @brief 复制数组栈
 */
Stru_CN_Stack_t* CN_stack_internal_array_copy(const Stru_CN_Stack_t* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    // 创建新栈
    Stru_CN_Stack_t* dst = CN_stack_internal_create_array(
        src->element_size,
        src->impl.array.capacity,
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
            void* src_element = CN_stack_internal_array_get_element(src, i);
            if (src_element == NULL)
            {
                CN_stack_internal_destroy_array(dst);
                return NULL;
            }
            
            if (!CN_stack_internal_array_push(dst, src_element))
            {
                CN_stack_internal_destroy_array(dst);
                return NULL;
            }
        }
    }
    
    return dst;
}

/**
 * @brief 清空数组栈
 */
void CN_stack_internal_array_clear(Stru_CN_Stack_t* stack)
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
            void* element = CN_stack_internal_array_get_element(stack, i);
            if (element != NULL)
            {
                stack->free_func(element);
            }
        }
    }
    
    // 重置栈
    stack->impl.array.top_index = 0;
    stack->size = 0;
    
    CN_stack_internal_unlock(stack);
}

/**
 * @brief 获取数组栈容量
 */
size_t CN_stack_internal_array_capacity(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return 0;
    }
    
    return stack->impl.array.capacity;
}

/**
 * @brief 检查数组栈是否已满
 */
bool CN_stack_internal_array_is_full(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return false;
    }
    
    return stack->size >= stack->impl.array.capacity;
}

/**
 * @brief 缩小数组栈容量以匹配大小
 */
bool CN_stack_internal_array_shrink_to_fit(Stru_CN_Stack_t* stack)
{
    if (stack == NULL || stack->size == stack->impl.array.capacity)
    {
        return true;
    }
    
    return CN_stack_internal_array_ensure_capacity(stack, stack->size);
}
