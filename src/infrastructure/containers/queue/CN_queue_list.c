/******************************************************************************
 * 文件名: CN_queue_list.c
 * 功能: CN_Language队列容器 - 链表实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现链表队列
 * 版权: MIT许可证
 * 
 * 注意: 此文件实现链表队列，遵循单一职责原则
 ******************************************************************************/

#include "CN_queue_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief 创建链表实现的队列
 */
Stru_CN_Queue_t* CN_queue_internal_create_list(
    size_t element_size,
    Eum_CN_QueueThreadSafety_t thread_safety,
    CN_QueueFreeFunc free_func,
    CN_QueueCopyFunc copy_func)
{
    if (element_size == 0)
    {
        return NULL;
    }
    
    Stru_CN_Queue_t* queue = (Stru_CN_Queue_t*)cn_malloc(sizeof(Stru_CN_Queue_t), "CN_queue_list");
    if (queue == NULL)
    {
        return NULL;
    }
    
    // 初始化通用字段
    queue->implementation = Eum_QUEUE_IMPLEMENTATION_LIST;
    queue->element_size = element_size;
    queue->size = 0;
    queue->thread_safety = thread_safety;
    queue->free_func = free_func;
    queue->copy_func = copy_func;
    
    // 初始化链表特定字段
    queue->impl.list.head = NULL;
    queue->impl.list.tail = NULL;
    
    // 初始化锁
    if (!CN_queue_internal_init_lock(queue))
    {
        cn_free(queue);
        return NULL;
    }
    
    return queue;
}

/**
 * @brief 销毁链表实现的队列
 */
void CN_queue_internal_destroy_list(Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return;
    }
    
    CN_queue_internal_lock(queue);
    
    // 遍历链表，释放所有节点
    Stru_CN_QueueNode_t* current = queue->impl.list.head;
    while (current != NULL)
    {
        Stru_CN_QueueNode_t* next = current->next;
        
        // 如果有释放函数，释放元素数据
        if (queue->free_func != NULL && current->data != NULL)
        {
            queue->free_func(current->data);
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
    
    queue->impl.list.head = NULL;
    queue->impl.list.tail = NULL;
    queue->size = 0;
    
    CN_queue_internal_unlock(queue);
    CN_queue_internal_destroy_lock(queue);
}

/**
 * @brief 链表实现的入队操作
 */
bool CN_queue_internal_list_enqueue(Stru_CN_Queue_t* queue, const void* element)
{
    if (queue == NULL || element == NULL)
    {
        return false;
    }
    
    // 创建新节点
    Stru_CN_QueueNode_t* new_node = (Stru_CN_QueueNode_t*)cn_malloc(sizeof(Stru_CN_QueueNode_t), "CN_queue_list_node");
    if (new_node == NULL)
    {
        return false;
    }
    
    // 分配节点数据
    new_node->data = cn_malloc(queue->element_size, "CN_queue_list_node_data");
    if (new_node->data == NULL)
    {
        cn_free(new_node);
        return false;
    }
    
    // 复制元素数据
    memcpy(new_node->data, element, queue->element_size);
    new_node->next = NULL;
    new_node->prev = NULL;
    
    // 将新节点插入到链表尾部
    if (queue->impl.list.tail == NULL)
    {
        // 队列为空
        queue->impl.list.head = new_node;
        queue->impl.list.tail = new_node;
    }
    else
    {
        // 队列非空，插入到尾部
        new_node->prev = queue->impl.list.tail;
        queue->impl.list.tail->next = new_node;
        queue->impl.list.tail = new_node;
    }
    
    queue->size++;
    
    return true;
}

/**
 * @brief 链表实现的出队操作
 */
bool CN_queue_internal_list_dequeue(Stru_CN_Queue_t* queue, void* element)
{
    if (queue == NULL || queue->impl.list.head == NULL)
    {
        return false;
    }
    
    // 获取队头节点
    Stru_CN_QueueNode_t* head_node = queue->impl.list.head;
    
    // 如果提供了输出参数，复制元素值
    if (element != NULL && head_node->data != NULL)
    {
        memcpy(element, head_node->data, queue->element_size);
    }
    
    // 更新队头指针
    queue->impl.list.head = head_node->next;
    if (queue->impl.list.head != NULL)
    {
        queue->impl.list.head->prev = NULL;
    }
    else
    {
        // 队列变为空
        queue->impl.list.tail = NULL;
    }
    
    // 如果有释放函数，释放元素数据
    if (queue->free_func != NULL && head_node->data != NULL)
    {
        queue->free_func(head_node->data);
    }
    
    // 释放节点数据
    if (head_node->data != NULL)
    {
        cn_free(head_node->data);
    }
    
    // 释放节点本身
    cn_free(head_node);
    
    queue->size--;
    
    return true;
}

/**
 * @brief 链表实现的查看队头操作
 */
void* CN_queue_internal_list_front(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL || queue->impl.list.head == NULL)
    {
        return NULL;
    }
    
    return queue->impl.list.head->data;
}

/**
 * @brief 链表实现的查看队尾操作
 */
void* CN_queue_internal_list_rear(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL || queue->impl.list.tail == NULL)
    {
        return NULL;
    }
    
    return queue->impl.list.tail->data;
}

/**
 * @brief 复制链表队列
 */
Stru_CN_Queue_t* CN_queue_internal_list_copy(const Stru_CN_Queue_t* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    // 创建新队列
    Stru_CN_Queue_t* dst = CN_queue_internal_create_list(
        src->element_size,
        src->thread_safety,
        src->free_func,
        src->copy_func);
    
    if (dst == NULL)
    {
        return NULL;
    }
    
    // 复制元素
    Stru_CN_QueueNode_t* current = src->impl.list.head;
    while (current != NULL)
    {
        if (current->data == NULL)
        {
            CN_queue_internal_destroy_list(dst);
            return NULL;
        }
        
        if (!CN_queue_internal_list_enqueue(dst, current->data))
        {
            CN_queue_internal_destroy_list(dst);
            return NULL;
        }
        
        current = current->next;
    }
    
    return dst;
}

/**
 * @brief 获取链表队列元素指针
 */
void* CN_queue_internal_list_get_element(const Stru_CN_Queue_t* queue, size_t index)
{
    if (queue == NULL || index >= queue->size)
    {
        return NULL;
    }
    
    // 遍历链表找到指定索引的节点
    Stru_CN_QueueNode_t* current = queue->impl.list.head;
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
 * @brief 清空链表队列
 */
void CN_queue_internal_list_clear(Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return;
    }
    
    CN_queue_internal_lock(queue);
    
    // 遍历链表，释放所有节点
    Stru_CN_QueueNode_t* current = queue->impl.list.head;
    while (current != NULL)
    {
        Stru_CN_QueueNode_t* next = current->next;
        
        // 如果有释放函数，释放元素数据
        if (queue->free_func != NULL && current->data != NULL)
        {
            queue->free_func(current->data);
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
    
    queue->impl.list.head = NULL;
    queue->impl.list.tail = NULL;
    queue->size = 0;
    
    CN_queue_internal_unlock(queue);
}
