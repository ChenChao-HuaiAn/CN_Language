/******************************************************************************
 * 文件名: CN_stack_api.c
 * 功能: CN_Language栈容器公共API实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现栈公共API
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_stack.h"
#include "CN_stack_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// 栈创建和销毁
// ============================================================================

/**
 * @brief 创建栈
 */
Stru_CN_Stack_t* CN_stack_create(Eum_CN_StackImplementation_t implementation,
                                 size_t element_size, 
                                 size_t initial_capacity,
                                 Eum_CN_StackThreadSafety_t thread_safety)
{
    return CN_stack_create_custom(implementation, element_size, initial_capacity,
                                  thread_safety, NULL, NULL);
}

/**
 * @brief 创建带自定义函数的栈
 */
Stru_CN_Stack_t* CN_stack_create_custom(
    Eum_CN_StackImplementation_t implementation,
    size_t element_size,
    size_t initial_capacity,
    Eum_CN_StackThreadSafety_t thread_safety,
    CN_StackFreeFunc free_func,
    CN_StackCopyFunc copy_func)
{
    if (element_size == 0)
    {
        return NULL;
    }
    
    // 根据实现类型创建栈
    switch (implementation)
    {
        case Eum_STACK_IMPLEMENTATION_ARRAY:
            if (initial_capacity == 0)
            {
                initial_capacity = 16; // 默认初始容量
            }
            return CN_stack_internal_create_array(element_size, initial_capacity,
                                                  thread_safety, free_func, copy_func);
            
        case Eum_STACK_IMPLEMENTATION_LIST:
            return CN_stack_internal_create_list(element_size, thread_safety,
                                                 free_func, copy_func);
            
        case Eum_STACK_IMPLEMENTATION_CIRCULAR:
            if (initial_capacity == 0)
            {
                initial_capacity = 16; // 默认初始容量
            }
            return CN_stack_internal_create_circular(element_size, initial_capacity,
                                                     thread_safety, free_func, copy_func);
            
        default:
            return NULL;
    }
}

/**
 * @brief 销毁栈
 */
void CN_stack_destroy(Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return;
    }
    
    // 根据实现类型销毁栈
    switch (stack->implementation)
    {
        case Eum_STACK_IMPLEMENTATION_ARRAY:
            CN_stack_internal_destroy_array(stack);
            break;
            
        case Eum_STACK_IMPLEMENTATION_LIST:
            CN_stack_internal_destroy_list(stack);
            break;
            
        case Eum_STACK_IMPLEMENTATION_CIRCULAR:
            CN_stack_internal_destroy_circular(stack);
            break;
            
        default:
            break;
    }
    
    // 释放栈结构本身
    cn_free(stack);
}

/**
 * @brief 清空栈（移除所有元素）
 */
void CN_stack_clear(Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return;
    }
    
    CN_stack_internal_lock(stack);
    
    // 根据实现类型清空栈
    switch (stack->implementation)
    {
        case Eum_STACK_IMPLEMENTATION_ARRAY:
            CN_stack_internal_array_clear(stack);
            break;
            
        case Eum_STACK_IMPLEMENTATION_LIST:
            CN_stack_internal_list_clear(stack);
            break;
            
        case Eum_STACK_IMPLEMENTATION_CIRCULAR:
            CN_stack_internal_circular_clear(stack);
            break;
            
        default:
            break;
    }
    
    CN_stack_internal_unlock(stack);
}

// ============================================================================
// 栈属性查询
// ============================================================================

/**
 * @brief 获取栈大小（元素数量）
 */
size_t CN_stack_size(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return 0;
    }
    
    return stack->size;
}

/**
 * @brief 检查栈是否为空
 */
bool CN_stack_is_empty(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return true;
    }
    
    return stack->size == 0;
}

/**
 * @brief 检查栈是否已满（仅对固定容量栈有效）
 */
bool CN_stack_is_full(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return false;
    }
    
    switch (stack->implementation)
    {
        case Eum_STACK_IMPLEMENTATION_ARRAY:
            return CN_stack_internal_array_is_full(stack);
            
        case Eum_STACK_IMPLEMENTATION_LIST:
            return CN_stack_internal_list_is_full(stack);
            
        case Eum_STACK_IMPLEMENTATION_CIRCULAR:
            return CN_stack_internal_circular_is_full(stack);
            
        default:
            return false;
    }
}

/**
 * @brief 获取栈容量（仅对数组实现有效）
 */
size_t CN_stack_capacity(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return 0;
    }
    
    switch (stack->implementation)
    {
        case Eum_STACK_IMPLEMENTATION_ARRAY:
            return CN_stack_internal_array_capacity(stack);
            
        case Eum_STACK_IMPLEMENTATION_LIST:
            return CN_stack_internal_list_capacity(stack);
            
        case Eum_STACK_IMPLEMENTATION_CIRCULAR:
            return CN_stack_internal_circular_capacity(stack);
            
        default:
            return 0;
    }
}

/**
 * @brief 获取元素大小
 */
size_t CN_stack_element_size(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return 0;
    }
    
    return stack->element_size;
}

/**
 * @brief 获取栈实现类型
 */
Eum_CN_StackImplementation_t CN_stack_implementation(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return Eum_STACK_IMPLEMENTATION_ARRAY; // 默认值
    }
    
    return stack->implementation;
}

// ============================================================================
// 栈操作（核心功能）
// ============================================================================

/**
 * @brief 压入元素到栈顶
 */
bool CN_stack_push(Stru_CN_Stack_t* stack, const void* element)
{
    if (stack == NULL || element == NULL)
    {
        return false;
    }
    
    CN_stack_internal_lock(stack);
    
    bool result;
    switch (stack->implementation)
    {
        case Eum_STACK_IMPLEMENTATION_ARRAY:
            result = CN_stack_internal_array_push(stack, element);
            break;
            
        case Eum_STACK_IMPLEMENTATION_LIST:
            result = CN_stack_internal_list_push(stack, element);
            break;
            
        case Eum_STACK_IMPLEMENTATION_CIRCULAR:
            result = CN_stack_internal_circular_push(stack, element);
            break;
            
        default:
            result = false;
            break;
    }
    
    CN_stack_internal_unlock(stack);
    return result;
}

/**
 * @brief 弹出栈顶元素
 */
bool CN_stack_pop(Stru_CN_Stack_t* stack, void* element)
{
    if (stack == NULL)
    {
        return false;
    }
    
    CN_stack_internal_lock(stack);
    
    bool result;
    switch (stack->implementation)
    {
        case Eum_STACK_IMPLEMENTATION_ARRAY:
            result = CN_stack_internal_array_pop(stack, element);
            break;
            
        case Eum_STACK_IMPLEMENTATION_LIST:
            result = CN_stack_internal_list_pop(stack, element);
            break;
            
        case Eum_STACK_IMPLEMENTATION_CIRCULAR:
            result = CN_stack_internal_circular_pop(stack, element);
            break;
            
        default:
            result = false;
            break;
    }
    
    CN_stack_internal_unlock(stack);
    return result;
}

/**
 * @brief 查看栈顶元素（不移除）
 */
void* CN_stack_peek(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return NULL;
    }
    
    // 注意：这里不锁定，因为peek是只读操作
    // 对于线程安全栈，调用者应该使用CN_stack_peek_safe
    
    switch (stack->implementation)
    {
        case Eum_STACK_IMPLEMENTATION_ARRAY:
            return CN_stack_internal_array_peek(stack);
            
        case Eum_STACK_IMPLEMENTATION_LIST:
            return CN_stack_internal_list_peek(stack);
            
        case Eum_STACK_IMPLEMENTATION_CIRCULAR:
            return CN_stack_internal_circular_peek(stack);
            
        default:
            return NULL;
    }
}

/**
 * @brief 查看栈顶元素（带索引偏移）
 */
void* CN_stack_peek_at(const Stru_CN_Stack_t* stack, size_t offset)
{
    if (stack == NULL || offset >= stack->size)
    {
        return NULL;
    }
    
    // 计算实际索引（从栈顶开始）
    size_t index = stack->size - offset - 1;
    
    // 根据实现类型获取元素
    switch (stack->implementation)
    {
        case Eum_STACK_IMPLEMENTATION_ARRAY:
            return CN_stack_internal_array_get_element(stack, index);
            
        case Eum_STACK_IMPLEMENTATION_LIST:
            return CN_stack_internal_list_get_element(stack, index);
            
        case Eum_STACK_IMPLEMENTATION_CIRCULAR:
            return CN_stack_internal_circular_get_element(stack, index);
            
        default:
            return NULL;
    }
}

// ============================================================================
// 栈操作（高级功能）
// ============================================================================

/**
 * @brief 确保栈有足够容量（仅对数组实现有效）
 */
bool CN_stack_ensure_capacity(Stru_CN_Stack_t* stack, size_t min_capacity)
{
    if (stack == NULL)
    {
        return false;
    }
    
    CN_stack_internal_lock(stack);
    
    bool result;
    switch (stack->implementation)
    {
        case Eum_STACK_IMPLEMENTATION_ARRAY:
            result = CN_stack_internal_array_ensure_capacity(stack, min_capacity);
            break;
            
        case Eum_STACK_IMPLEMENTATION_LIST:
            result = CN_stack_internal_list_ensure_capacity(stack, min_capacity);
            break;
            
        case Eum_STACK_IMPLEMENTATION_CIRCULAR:
            result = CN_stack_internal_circular_ensure_capacity(stack, min_capacity);
            break;
            
        default:
            result = false;
            break;
    }
    
    CN_stack_internal_unlock(stack);
    return result;
}

/**
 * @brief 缩小栈容量以匹配大小（仅对数组实现有效）
 */
bool CN_stack_shrink_to_fit(Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return false;
    }
    
    CN_stack_internal_lock(stack);
    
    bool result = false;
    
    switch (stack->implementation)
    {
        case Eum_STACK_IMPLEMENTATION_ARRAY:
            result = CN_stack_internal_array_shrink_to_fit(stack);
            break;
            
        case Eum_STACK_IMPLEMENTATION_LIST:
            result = CN_stack_internal_list_shrink_to_fit(stack);
            break;
            
        case Eum_STACK_IMPLEMENTATION_CIRCULAR:
            result = CN_stack_internal_circular_shrink_to_fit(stack);
            break;
            
        default:
            result = true; // 默认实现不需要此操作
            break;
    }
    
    CN_stack_internal_unlock(stack);
    return result;
}

/**
 * @brief 复制栈
 */
Stru_CN_Stack_t* CN_stack_copy(const Stru_CN_Stack_t* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    // 根据实现类型复制栈
    switch (src->implementation)
    {
        case Eum_STACK_IMPLEMENTATION_ARRAY:
            return CN_stack_internal_array_copy(src);
            
        case Eum_STACK_IMPLEMENTATION_LIST:
            return CN_stack_internal_list_copy(src);
            
        case Eum_STACK_IMPLEMENTATION_CIRCULAR:
            return CN_stack_internal_circular_copy(src);
            
        default:
            return NULL;
    }
}

/**
 * @brief 反转栈中元素的顺序
 */
bool CN_stack_reverse(Stru_CN_Stack_t* stack)
{
    if (stack == NULL || stack->size <= 1)
    {
        return true; // 空栈或单元素栈已经反转
    }
    
    CN_stack_internal_lock(stack);
    
    bool result = false;
    
    // 临时数组用于存储元素
    void* temp_array = cn_malloc(stack->element_size * stack->size, "CN_stack_reverse_temp");
    if (temp_array == NULL)
    {
        CN_stack_internal_unlock(stack);
        return false;
    }
    
    // 将栈元素复制到临时数组（从栈顶到底）
    for (size_t i = 0; i < stack->size; i++)
    {
        void* src = CN_stack_peek_at(stack, i);
        if (src == NULL)
        {
            cn_free(temp_array);
            CN_stack_internal_unlock(stack);
            return false;
        }
        
        void* dest = (char*)temp_array + (i * stack->element_size);
        memcpy(dest, src, stack->element_size);
    }
    
    // 清空栈
    CN_stack_clear(stack);
    
    // 将元素按相反顺序压回栈中
    for (size_t i = 0; i < stack->size; i++)
    {
        void* src = (char*)temp_array + (i * stack->element_size);
        if (!CN_stack_push(stack, src))
        {
            cn_free(temp_array);
            CN_stack_internal_unlock(stack);
            return false;
        }
    }
    
    cn_free(temp_array);
    CN_stack_internal_unlock(stack);
    return true;
}

// ============================================================================
// 线程安全操作（仅对线程安全栈有效）
// ============================================================================

/**
 * @brief 线程安全地压入元素
 */
bool CN_stack_push_safe(Stru_CN_Stack_t* stack, const void* element)
{
    // 对于线程安全栈，CN_stack_push已经线程安全
    return CN_stack_push(stack, element);
}

/**
 * @brief 线程安全地弹出元素
 */
bool CN_stack_pop_safe(Stru_CN_Stack_t* stack, void* element)
{
    // 对于线程安全栈，CN_stack_pop已经线程安全
    return CN_stack_pop(stack, element);
}

/**
 * @brief 线程安全地查看栈顶元素
 */
void* CN_stack_peek_safe(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return NULL;
    }
    
    // 锁定栈以进行线程安全的只读访问
    CN_stack_internal_lock((Stru_CN_Stack_t*)stack);
    void* result = CN_stack_peek(stack);
    CN_stack_internal_unlock((Stru_CN_Stack_t*)stack);
    
    return result;
}

/**
 * @brief 获取栈的互斥锁（用于手动同步）
 */
bool CN_stack_lock(Stru_CN_Stack_t* stack)
{
    if (stack == NULL || stack->thread_safety != Eum_STACK_THREAD_SAFE)
    {
        return false;
    }
    
    CN_stack_internal_lock(stack);
    return true;
}

/**
 * @brief 释放栈的互斥锁
 */
void CN_stack_unlock(Stru_CN_Stack_t* stack)
{
    if (stack == NULL || stack->thread_safety != Eum_STACK_THREAD_SAFE)
    {
        return;
    }
    
    CN_stack_internal_unlock(stack);
}

// ============================================================================
// 迭代器支持
// ============================================================================

/**
 * @brief 创建栈迭代器（从栈顶开始）
 */
Stru_CN_StackIterator_t* CN_stack_iterator_create(Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return NULL;
    }
    
    Stru_CN_StackIterator_t* iterator = (Stru_CN_StackIterator_t*)cn_malloc(
        sizeof(Stru_CN_StackIterator_t), "CN_stack_iterator");
    if (iterator == NULL)
    {
        return NULL;
    }
    
    iterator->stack = stack;
    iterator->current_index = 0; // 从栈顶开始
    iterator->current_element = CN_stack_peek_at(stack, 0);
    
    return iterator;
}

/**
 * @brief 销毁迭代器
 */
void CN_stack_iterator_destroy(Stru_CN_StackIterator_t* iterator)
{
    if (iterator == NULL)
    {
        return;
    }
    
    cn_free(iterator);
}

/**
 * @brief 重置迭代器到起始位置（栈顶）
 */
void CN_stack_iterator_reset(Stru_CN_StackIterator_t* iterator)
{
    if (iterator == NULL || iterator->stack == NULL)
    {
        return;
    }
    
    iterator->current_index = 0;
    iterator->current_element = CN_stack_peek_at(iterator->stack, 0);
}

/**
 * @brief 检查迭代器是否有下一个元素
 */
bool CN_stack_iterator_has_next(const Stru_CN_StackIterator_t* iterator)
{
    if (iterator == NULL || iterator->stack == NULL)
    {
        return false;
    }
    
    return iterator->current_index < CN_stack_size(iterator->stack);
}

/**
 * @brief 获取迭代器的下一个元素
 */
void* CN_stack_iterator_next(Stru_CN_StackIterator_t* iterator)
{
    if (iterator == NULL || iterator->stack == NULL)
    {
        return NULL;
    }
    
    if (!CN_stack_iterator_has_next(iterator))
    {
        return NULL;
    }
    
    // 获取当前元素
    void* current = iterator->current_element;
    
    // 移动到下一个元素
    iterator->current_index++;
    iterator->current_element = CN_stack_peek_at(iterator->stack, iterator->current_index);
    
    return current;
}

/**
 * @brief 获取迭代器的当前元素（不移动）
 */
void* CN_stack_iterator_current(const Stru_CN_StackIterator_t* iterator)
{
    if (iterator == NULL)
    {
        return NULL;
    }
    
    return iterator->current_element;
}

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 比较两个栈是否相等
 */
bool CN_stack_equal(const Stru_CN_Stack_t* stack1, const Stru_CN_Stack_t* stack2)
{
    if (stack1 == NULL && stack2 == NULL)
    {
        return true;
    }
    
    if (stack1 == NULL || stack2 == NULL)
    {
        return false;
    }
    
    // 检查基本属性
    if (stack1->element_size != stack2->element_size ||
        stack1->size != stack2->size)
    {
        return false;
    }
    
    // 如果两个栈都为空，则相等
    if (stack1->size == 0)
    {
        return true;
    }
    
    // 比较每个元素
    for (size_t i = 0; i < stack1->size; i++)
    {
        void* elem1 = CN_stack_peek_at(stack1, i);
        void* elem2 = CN_stack_peek_at(stack2, i);
        
        if (elem1 == NULL || elem2 == NULL)
        {
            return false;
        }
        
        // 如果有自定义比较函数，使用它
        if (stack1->copy_func != NULL)
        {
            // 使用自定义比较逻辑（这里简化处理，实际可能需要更复杂的比较）
            // 注意：这里假设copy_func可以用于比较，实际可能需要专门的比较函数
            // 为了简化，我们使用memcmp
            if (memcmp(elem1, elem2, stack1->element_size) != 0)
            {
                return false;
            }
        }
        else
        {
            // 使用内存比较
            if (memcmp(elem1, elem2, stack1->element_size) != 0)
            {
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief 转储栈信息到控制台（调试用）
 */
void CN_stack_dump(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        printf("Stack: NULL\n");
        return;
    }
    
    printf("Stack [size=%zu, capacity=%zu, element_size=%zu, implementation=%d, thread_safe=%d]:\n",
           stack->size, CN_stack_capacity(stack), stack->element_size,
           stack->implementation, stack->thread_safety);
    
    // 打印元素信息（从栈顶到底）
    for (size_t i = 0; i < stack->size; i++)
    {
        void* element = CN_stack_peek_at(stack, i);
        if (element == NULL)
        {
            printf("  [%zu]: NULL\n", i);
        }
        else
        {
            // 简化表示：只显示内存地址
            printf("  [%zu]: 0x%p\n", i, element);
        }
    }
}

/**
 * @brief 将栈内容转换为数组（从栈顶到底）
 */
size_t CN_stack_to_array(const Stru_CN_Stack_t* stack, void* array, size_t max_elements)
{
    if (stack == NULL || array == NULL || max_elements == 0)
    {
        return 0;
    }
    
    // 确定要复制的元素数量
    size_t elements_to_copy = stack->size;
    if (elements_to_copy > max_elements)
    {
        elements_to_copy = max_elements;
    }
    
    // 复制元素（从栈顶到底）
    for (size_t i = 0; i < elements_to_copy; i++)
    {
        void* src = CN_stack_peek_at(stack, i);
        if (src == NULL)
        {
            return i; // 返回已成功复制的元素数量
        }
        
        void* dest = (char*)array + (i * stack->element_size);
        memcpy(dest, src, stack->element_size);
    }
    
    return elements_to_copy;
}
