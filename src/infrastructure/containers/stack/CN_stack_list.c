/******************************************************************************
 * 文件名: CN_stack_list.c
 * 功能: CN_Language栈容器 - 链表实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现链表栈
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_stack_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>

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
 * @brief 获取链表栈元素指针
 */
void* CN_stack_internal_list_get_element(const Stru_CN_Stack_t* stack, size_t index)
{
    if (stack == NULL || index >= stack->size)
    {
        return NULL;
    }
    
    // 遍历链表找到指定索引的节点
    Stru_CN_StackNode_t* current = stack->impl.list.top;
    for (size_t i = 0; i < index && current != NULL; i++)
    {
        current = current->next;
    }
    
    if (current == NULL)
    {
        return NULL;
    }
    
    return current->data;
}

/**
 * @brief 复制链表栈
 */
Stru_CN_Stack_t* CN_stack_internal_list_copy(const Stru_CN_Stack_t* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    // 创建新栈
    Stru_CN_Stack_t* dst = CN_stack_internal_create_list(
        src->element_size,
        src->thread_safety,
        src->free_func,
        src->copy_func);
    
    if (dst == NULL)
    {
        return NULL;
    }
    
    // 由于栈是LIFO，我们需要反向复制以保持顺序
    // 先创建一个临时数组来存储元素
    void* temp_array = cn_malloc(src->element_size * src->size, "CN_stack_list_copy_temp");
    if (temp_array == NULL && src->size > 0)
    {
        CN_stack_internal_destroy_list(dst);
        return NULL;
    }
    
    // 将源栈元素复制到临时数组（从栈顶到底）
    Stru_CN_StackNode_t* current = src->impl.list.top;
    for (size_t i = 0; i < src->size && current != NULL; i++)
    {
        void* dest = (char*)temp_array + (i * src->element_size);
        memcpy(dest, current->data, src->element_size);
        current = current->next;
    }
    
    // 将临时数组元素压入新栈（反向顺序以保持原始顺序）
    for (size_t i = src->size; i > 0; i--)
    {
        void* src_element = (char*)temp_array + ((i - 1) * src->element_size);
        if (!CN_stack_internal_list_push(dst, src_element))
        {
            cn_free(temp_array);
            CN_stack_internal_destroy_list(dst);
            return NULL;
        }
    }
    
    if (temp_array != NULL)
    {
        cn_free(temp_array);
    }
    
    return dst;
}

/**
 * @brief 清空链表栈
 */
void CN_stack_internal_list_clear(Stru_CN_Stack_t* stack)
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
}

/**
 * @brief 获取链表栈容量（链表实现无固定容量）
 */
size_t CN_stack_internal_list_capacity(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return 0;
    }
    
    return SIZE_MAX; // 链表实现理论上无限容量
}

/**
 * @brief 检查链表栈是否已满（链表实现永远不会满）
 */
bool CN_stack_internal_list_is_full(const Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return false;
    }
    
    return false; // 链表实现没有固定容量限制
}

/**
 * @brief 缩小链表栈容量以匹配大小（链表实现不需要此操作）
 */
bool CN_stack_internal_list_shrink_to_fit(Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return false;
    }
    
    return true; // 链表实现不需要此操作
}

/**
 * @brief 确保链表栈有足够容量（链表实现不需要容量管理）
 */
bool CN_stack_internal_list_ensure_capacity(Stru_CN_Stack_t* stack, size_t min_capacity)
{
    if (stack == NULL)
    {
        return false;
    }
    
    return true; // 链表实现不需要容量管理
}
