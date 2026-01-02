/******************************************************************************
 * 文件名: CN_stack.c
 * 功能: CN_Language栈容器主实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现栈核心功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_stack_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 初始化栈锁
 */
bool CN_stack_internal_init_lock(Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return false;
    }
    
    if (stack->thread_safety == Eum_STACK_THREAD_SAFE)
    {
#ifdef _WIN32
        InitializeCriticalSection(&stack->lock);
#else
        if (pthread_mutex_init(&stack->lock, NULL) != 0)
        {
            return false;
        }
#endif
        stack->lock_initialized = true;
    }
    else
    {
        stack->lock_initialized = false;
    }
    
    return true;
}

/**
 * @brief 销毁栈锁
 */
void CN_stack_internal_destroy_lock(Stru_CN_Stack_t* stack)
{
    if (stack == NULL || !stack->lock_initialized)
    {
        return;
    }
    
    if (stack->thread_safety == Eum_STACK_THREAD_SAFE)
    {
#ifdef _WIN32
        DeleteCriticalSection(&stack->lock);
#else
        pthread_mutex_destroy(&stack->lock);
#endif
    }
    
    stack->lock_initialized = false;
}

/**
 * @brief 锁定栈（线程安全）
 */
void CN_stack_internal_lock(Stru_CN_Stack_t* stack)
{
    if (stack == NULL || !stack->lock_initialized)
    {
        return;
    }
    
    if (stack->thread_safety == Eum_STACK_THREAD_SAFE)
    {
#ifdef _WIN32
        EnterCriticalSection(&stack->lock);
#else
        pthread_mutex_lock(&stack->lock);
#endif
    }
}

/**
 * @brief 解锁栈（线程安全）
 */
void CN_stack_internal_unlock(Stru_CN_Stack_t* stack)
{
    if (stack == NULL || !stack->lock_initialized)
    {
        return;
    }
    
    if (stack->thread_safety == Eum_STACK_THREAD_SAFE)
    {
#ifdef _WIN32
        LeaveCriticalSection(&stack->lock);
#else
        pthread_mutex_unlock(&stack->lock);
#endif
    }
}

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
 * @brief 创建链表实现的栈
 */
Stru_CN_Stack_t* CN_stack_internal_create_list(
    size_t element_size,
    Eum_CN_StackThreadSafety_t thread_safety,
    CN_StackFreeFunc free_func,
    CN_StackCopyFunc copy_func)
{
    if (element_size == 0)
    {
        return NULL;
    }
    
    Stru_CN_Stack_t* stack = (Stru_CN_Stack_t*)cn_malloc(sizeof(Stru_CN_Stack_t), "CN_stack_list");
    if (stack == NULL)
    {
        return NULL;
    }
    
    // 初始化通用字段
    stack->implementation = Eum_STACK_IMPLEMENTATION_LIST;
    stack->element_size = element_size;
    stack->size = 0;
    stack->thread_safety = thread_safety;
    stack->free_func = free_func;
    stack->copy_func = copy_func;
    
    // 初始化链表特定字段
    stack->impl.list.top = NULL;
    
    // 初始化锁
    if (!CN_stack_internal_init_lock(stack))
    {
        cn_free(stack);
        return NULL;
    }
    
    return stack;
}

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
 * @brief 销毁链表实现的栈
 */
void CN_stack_internal_destroy_list(Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return;
    }
    
    CN_stack_internal_lock(stack);
    
    // 遍历链表，释放所有节点
    Stru_CN_StackNode_t* current = stack->impl.list.top;
    while (current != NULL)
    {
        Stru_CN_StackNode_t* next = current->next;
        
        // 如果有释放函数，释放元素数据
        if (stack->free_func != NULL && current->data != NULL)
        {
            stack->free_func(current->data);
        }
        
        // 释放节点数据
        if (current->data != NULL)
        {
            cn_free(current->data);
        }
        
        // 释放节点本身
        cn_free(current);
        
        current = next;
    }
    
    stack->impl.list.top = NULL;
    stack->size = 0;
    
    CN_stack_internal_unlock(stack);
    CN_stack_internal_destroy_lock(stack);
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
 * @brief 链表实现的压栈操作
 */
bool CN_stack_internal_list_push(Stru_CN_Stack_t* stack, const void* element)
{
    if (stack == NULL || element == NULL)
    {
        return false;
    }
    
    // 创建新节点
    Stru_CN_StackNode_t* new_node = (Stru_CN_StackNode_t*)cn_malloc(sizeof(Stru_CN_StackNode_t), "CN_stack_list_node");
    if (new_node == NULL)
    {
        return false;
    }
    
    // 分配节点数据
    new_node->data = cn_malloc(stack->element_size, "CN_stack_list_node_data");
    if (new_node->data == NULL)
    {
        cn_free(new_node);
        return false;
    }
    
    // 复制元素数据
    memcpy(new_node->data, element, stack->element_size);
    
    // 将新节点插入到链表头部
    new_node->next = stack->impl.list.top;
    stack->impl.list.top = new_node;
    
    stack->size++;
    
    return true;
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
 * @brief 链表实现的弹栈操作
 */
bool CN_stack_internal_list_pop(Stru_CN_Stack_t* stack, void* element)
{
    if (stack == NULL || stack->impl.list.top == NULL)
    {
        return false;
    }
    
    // 获取栈顶节点
    Stru_CN_StackNode_t* top_node = stack->impl.list.top;
    
    // 如果提供了输出参数，复制元素值
    if (element != NULL && top_node->data != NULL)
    {
        memcpy(element, top_node->data, stack->element_size);
    }
    
    // 更新栈顶指针
    stack->impl.list.top = top_node->next;
    
    // 如果有释放函数，释放元素数据
    if (stack->free_func != NULL && top_node->data != NULL)
    {
        stack->free_func(top_node->data);
    }
    
    // 释放节点数据
    if (top_node->data != NULL)
    {
        cn_free(top_node->data);
    }
    
    // 释放节点本身
    cn_free(top_node);
    
    stack->size--;
    
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
 * @brief 链表实现的查看栈顶操作
 */
void* CN_stack_internal_list_peek(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL || stack->impl.list.top == NULL)
    {
        return NULL;
    }
    
    return stack->impl.list.top->data;
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
