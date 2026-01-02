/******************************************************************************
 * 文件名: CN_queue.c
 * 功能: CN_Language队列容器主实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现队列核心功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_queue_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 初始化队列锁
 */
bool CN_queue_internal_init_lock(Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return false;
    }
    
    if (queue->thread_safety == Eum_QUEUE_THREAD_SAFE)
    {
#ifdef _WIN32
        InitializeCriticalSection(&queue->lock);
#else
        if (pthread_mutex_init(&queue->lock, NULL) != 0)
        {
            return false;
        }
#endif
        queue->lock_initialized = true;
    }
    else
    {
        queue->lock_initialized = false;
    }
    
    return true;
}

/**
 * @brief 销毁队列锁
 */
void CN_queue_internal_destroy_lock(Stru_CN_Queue_t* queue)
{
    if (queue == NULL || !queue->lock_initialized)
    {
        return;
    }
    
    if (queue->thread_safety == Eum_QUEUE_THREAD_SAFE)
    {
#ifdef _WIN32
        DeleteCriticalSection(&queue->lock);
#else
        pthread_mutex_destroy(&queue->lock);
#endif
    }
    
    queue->lock_initialized = false;
}

/**
 * @brief 锁定队列（线程安全）
 */
void CN_queue_internal_lock(Stru_CN_Queue_t* queue)
{
    if (queue == NULL || !queue->lock_initialized)
    {
        return;
    }
    
    if (queue->thread_safety == Eum_QUEUE_THREAD_SAFE)
    {
#ifdef _WIN32
        EnterCriticalSection(&queue->lock);
#else
        pthread_mutex_lock(&queue->lock);
#endif
    }
}

/**
 * @brief 解锁队列（线程安全）
 */
void CN_queue_internal_unlock(Stru_CN_Queue_t* queue)
{
    if (queue == NULL || !queue->lock_initialized)
    {
        return;
    }
    
    if (queue->thread_safety == Eum_QUEUE_THREAD_SAFE)
    {
#ifdef _WIN32
        LeaveCriticalSection(&queue->lock);
#else
        pthread_mutex_unlock(&queue->lock);
#endif
    }
}

/**
 * @brief 创建数组实现的队列
 */
Stru_CN_Queue_t* CN_queue_internal_create_array(
    size_t element_size,
    size_t initial_capacity,
    Eum_CN_QueueThreadSafety_t thread_safety,
    CN_QueueFreeFunc free_func,
    CN_QueueCopyFunc copy_func)
{
    if (element_size == 0 || initial_capacity == 0)
    {
        return NULL;
    }
    
    Stru_CN_Queue_t* queue = (Stru_CN_Queue_t*)cn_malloc(sizeof(Stru_CN_Queue_t), "CN_queue_array");
    if (queue == NULL)
    {
        return NULL;
    }
    
    // 初始化通用字段
    queue->implementation = Eum_QUEUE_IMPLEMENTATION_ARRAY;
    queue->element_size = element_size;
    queue->size = 0;
    queue->thread_safety = thread_safety;
    queue->free_func = free_func;
    queue->copy_func = copy_func;
    
    // 初始化数组特定字段
    queue->impl.array.capacity = initial_capacity;
    queue->impl.array.head_index = 0;
    queue->impl.array.tail_index = 0;
    
    // 分配数据数组
    size_t data_size = element_size * initial_capacity;
    queue->impl.array.data = cn_malloc(data_size, "CN_queue_array_data");
    if (queue->impl.array.data == NULL)
    {
        cn_free(queue);
        return NULL;
    }
    
    // 初始化锁
    if (!CN_queue_internal_init_lock(queue))
    {
        cn_free(queue->impl.array.data);
        cn_free(queue);
        return NULL;
    }
    
    return queue;
}

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
 * @brief 创建循环数组实现的队列
 */
Stru_CN_Queue_t* CN_queue_internal_create_circular(
    size_t element_size,
    size_t initial_capacity,
    Eum_CN_QueueThreadSafety_t thread_safety,
    CN_QueueFreeFunc free_func,
    CN_QueueCopyFunc copy_func)
{
    if (element_size == 0 || initial_capacity == 0)
    {
        return NULL;
    }
    
    Stru_CN_Queue_t* queue = (Stru_CN_Queue_t*)cn_malloc(sizeof(Stru_CN_Queue_t), "CN_queue_circular");
    if (queue == NULL)
    {
        return NULL;
    }
    
    // 初始化通用字段
    queue->implementation = Eum_QUEUE_IMPLEMENTATION_CIRCULAR;
    queue->element_size = element_size;
    queue->size = 0;
    queue->thread_safety = thread_safety;
    queue->free_func = free_func;
    queue->copy_func = copy_func;
    
    // 初始化循环数组特定字段
    queue->impl.circular.capacity = initial_capacity;
    queue->impl.circular.head_index = 0;
    queue->impl.circular.tail_index = 0;
    queue->impl.circular.is_full = false;
    
    // 分配数据数组
    size_t data_size = element_size * initial_capacity;
    queue->impl.circular.data = cn_malloc(data_size, "CN_queue_circular_data");
    if (queue->impl.circular.data == NULL)
    {
        cn_free(queue);
        return NULL;
    }
    
    // 初始化锁
    if (!CN_queue_internal_init_lock(queue))
    {
        cn_free(queue->impl.circular.data);
        cn_free(queue);
        return NULL;
    }
    
    return queue;
}

/**
 * @brief 销毁数组实现的队列
 */
void CN_queue_internal_destroy_array(Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return;
    }
    
    CN_queue_internal_lock(queue);
    
    // 如果有释放函数，释放所有元素
    if (queue->free_func != NULL)
    {
        for (size_t i = 0; i < queue->size; i++)
        {
            void* element = CN_queue_internal_array_get_element(queue, i);
            if (element != NULL)
            {
                queue->free_func(element);
            }
        }
    }
    
    // 释放数据数组
    if (queue->impl.array.data != NULL)
    {
        cn_free(queue->impl.array.data);
        queue->impl.array.data = NULL;
    }
    
    CN_queue_internal_unlock(queue);
    CN_queue_internal_destroy_lock(queue);
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
 * @brief 销毁循环数组实现的队列
 */
void CN_queue_internal_destroy_circular(Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return;
    }
    
    CN_queue_internal_lock(queue);
    
    // 如果有释放函数，释放所有元素
    if (queue->free_func != NULL)
    {
        for (size_t i = 0; i < queue->size; i++)
        {
            void* element = CN_queue_internal_circular_get_element(queue, i);
            if (element != NULL)
            {
                queue->free_func(element);
            }
        }
    }
    
    // 释放数据数组
    if (queue->impl.circular.data != NULL)
    {
        cn_free(queue->impl.circular.data);
        queue->impl.circular.data = NULL;
    }
    
    CN_queue_internal_unlock(queue);
    CN_queue_internal_destroy_lock(queue);
}

/**
 * @brief 数组实现的入队操作
 */
bool CN_queue_internal_array_enqueue(Stru_CN_Queue_t* queue, const void* element)
{
    if (queue == NULL || element == NULL)
    {
        return false;
    }
    
    // 检查是否需要扩容
    if (queue->size >= queue->impl.array.capacity)
    {
        size_t new_capacity = queue->impl.array.capacity * 2;
        if (!CN_queue_internal_array_ensure_capacity(queue, new_capacity))
        {
            return false;
        }
    }
    
    // 复制元素到队尾
    void* dest = (char*)queue->impl.array.data + (queue->impl.array.tail_index * queue->element_size);
    memcpy(dest, element, queue->element_size);
    
    // 更新队尾索引
    queue->impl.array.tail_index++;
    if (queue->impl.array.tail_index >= queue->impl.array.capacity)
    {
        // 如果队尾到达数组末尾，需要移动元素或扩容
        // 这里我们选择移动元素以保持连续性
        if (queue->impl.array.head_index > 0)
        {
            // 移动元素到数组开头
            size_t move_count = queue->size;
            if (move_count > 0)
            {
                void* src = (char*)queue->impl.array.data + (queue->impl.array.head_index * queue->element_size);
                void* dst = queue->impl.array.data;
                memmove(dst, src, move_count * queue->element_size);
            }
            queue->impl.array.tail_index = move_count;
            queue->impl.array.head_index = 0;
        }
        else
        {
            // 需要扩容
            size_t new_capacity = queue->impl.array.capacity * 2;
            if (!CN_queue_internal_array_ensure_capacity(queue, new_capacity))
            {
                return false;
            }
        }
    }
    
    queue->size++;
    
    return true;
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
 * @brief 循环数组实现的入队操作
 */
bool CN_queue_internal_circular_enqueue(Stru_CN_Queue_t* queue, const void* element)
{
    if (queue == NULL || element == NULL)
    {
        return false;
    }
    
    // 检查队列是否已满
    if (CN_queue_internal_circular_is_full(queue))
    {
        // 需要扩容
        size_t new_capacity = queue->impl.circular.capacity * 2;
        if (!CN_queue_internal_circular_ensure_capacity(queue, new_capacity))
        {
            return false;
        }
    }
    
    // 复制元素到队尾
    void* dest = (char*)queue->impl.circular.data + (queue->impl.circular.tail_index * queue->element_size);
    memcpy(dest, element, queue->element_size);
    
    // 更新队尾索引
    queue->impl.circular.tail_index++;
    if (queue->impl.circular.tail_index >= queue->impl.circular.capacity)
    {
        queue->impl.circular.tail_index = 0;
    }
    
    // 检查队列是否已满
    if (queue->impl.circular.tail_index == queue->impl.circular.head_index)
    {
        queue->impl.circular.is_full = true;
    }
    
    queue->size++;
    
    return true;
}

/**
 * @brief 数组实现的出队操作
 */
bool CN_queue_internal_array_dequeue(Stru_CN_Queue_t* queue, void* element)
{
    if (queue == NULL || queue->size == 0)
    {
        return false;
    }
    
    // 获取队头元素
    void* src = (char*)queue->impl.array.data + (queue->impl.array.head_index * queue->element_size);
    
    // 如果提供了输出参数，复制元素值
    if (element != NULL)
    {
        memcpy(element, src, queue->element_size);
    }
    
    // 如果有释放函数，释放元素
    if (queue->free_func != NULL)
    {
        queue->free_func(src);
    }
    
    // 更新队头索引
    queue->impl.array.head_index++;
    if (queue->impl.array.head_index >= queue->impl.array.capacity)
    {
        queue->impl.array.head_index = 0;
    }
    
    queue->size--;
    
    return true;
}

/**
 * @brief 数组实现的查看队头操作
 */
void* CN_queue_internal_array_front(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL || queue->size == 0)
    {
        return NULL;
    }
    
    return (char*)queue->impl.array.data + (queue->impl.array.head_index * queue->element_size);
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
 * @brief 循环数组实现的查看队头操作
 */
void* CN_queue_internal_circular_front(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL || CN_queue_internal_circular_is_empty(queue))
    {
        return NULL;
    }
    
    return (char*)queue->impl.circular.data + (queue->impl.circular.head_index * queue->element_size);
}

/**
 * @brief 数组实现的查看队尾操作
 */
void* CN_queue_internal_array_rear(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL || queue->size == 0)
    {
        return NULL;
    }
    
    // 计算队尾索引（考虑回绕）
    size_t rear_index;
    if (queue->impl.array.tail_index == 0)
    {
        rear_index = queue->impl.array.capacity - 1;
    }
    else
    {
        rear_index = queue->impl.array.tail_index - 1;
    }
    
    return (char*)queue->impl.array.data + (rear_index * queue->element_size);
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
 * @brief 循环数组实现的查看队尾操作
 */
void* CN_queue_internal_circular_rear(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL || CN_queue_internal_circular_is_empty(queue))
    {
        return NULL;
    }
    
    // 计算队尾索引（考虑回绕）
    size_t rear_index;
    if (queue->impl.circular.tail_index == 0)
    {
        rear_index = queue->impl.circular.capacity - 1;
    }
    else
    {
        rear_index = queue->impl.circular.tail_index - 1;
    }
    
    return (char*)queue->impl.circular.data + (rear_index * queue->element_size);
}

/**
 * @brief 确保数组队列有足够容量
 */
bool CN_queue_internal_array_ensure_capacity(Stru_CN_Queue_t* queue, size_t min_capacity)
{
    if (queue == NULL || min_capacity <= queue->impl.array.capacity)
    {
        return true;
    }
    
    // 计算新的容量（至少翻倍）
    size_t new_capacity = queue->impl.array.capacity;
    while (new_capacity < min_capacity)
    {
        new_capacity *= 2;
    }
    
    // 分配新的数据数组
    size_t new_data_size = queue->element_size * new_capacity;
    void* new_data = cn_malloc(new_data_size, "CN_queue_array_new_data");
    if (new_data == NULL)
    {
        return false;
    }
    
    // 复制现有元素
    if (queue->size > 0)
    {
        // 如果队列有回绕，需要分两次复制
        if (queue->impl.array.head_index < queue->impl.array.tail_index)
        {
            // 连续存储，一次复制
            void* src = (char*)queue->impl.array.data + (queue->impl.array.head_index * queue->element_size);
            memcpy(new_data, src, queue->size * queue->element_size);
        }
        else if (queue->impl.array.head_index > queue->impl.array.tail_index)
        {
            // 回绕存储，分两次复制
            size_t first_part = queue->impl.array.capacity - queue->impl.array.head_index;
            void* src_first = (char*)queue->impl.array.data + (queue->impl.array.head_index * queue->element_size);
            memcpy(new_data, src_first, first_part * queue->element_size);
            
            void* src_second = queue->impl.array.data;
            void* dst_second = (char*)new_data + (first_part * queue->element_size);
            memcpy(dst_second, src_second, queue->impl.array.tail_index * queue->element_size);
        }
    }
    
    // 释放旧数据
    cn_free(queue->impl.array.data);
    
    // 更新队列结构
    queue->impl.array.data = new_data;
    queue->impl.array.capacity = new_capacity;
    queue->impl.array.head_index = 0;
    queue->impl.array.tail_index = queue->size;
    
    return true;
}

/**
 * @brief 确保循环数组队列有足够容量
 */
bool CN_queue_internal_circular_ensure_capacity(Stru_CN_Queue_t* queue, size_t min_capacity)
{
    if (queue == NULL || min_capacity <= queue->impl.circular.capacity)
    {
        return true;
    }
    
    // 计算新的容量（至少翻倍）
    size_t new_capacity = queue->impl.circular.capacity;
    while (new_capacity < min_capacity)
    {
        new_capacity *= 2;
    }
    
    // 分配新的数据数组
    size_t new_data_size = queue->element_size * new_capacity;
    void* new_data = cn_malloc(new_data_size, "CN_queue_circular_new_data");
    if (new_data == NULL)
    {
        return false;
    }
    
    // 复制现有元素
    if (queue->size > 0)
    {
        if (!CN_queue_internal_circular_is_full(queue) && 
            queue->impl.circular.head_index < queue->impl.circular.tail_index)
        {
            // 连续存储，一次复制
            void* src = (char*)queue->impl.circular.data + (queue->impl.circular.head_index * queue->element_size);
            memcpy(new_data, src, queue->size * queue->element_size);
        }
        else
        {
            // 回绕存储或队列已满，分两次复制
            size_t first_part = queue->impl.circular.capacity - queue->impl.circular.head_index;
            if (first_part > queue->size)
            {
                first_part = queue->size;
            }
            
            void* src_first = (char*)queue->impl.circular.data + (queue->impl.circular.head_index * queue->element_size);
            memcpy(new_data, src_first, first_part * queue->element_size);
            
            if (first_part < queue->size)
            {
                size_t second_part = queue->size - first_part;
                void* src_second = queue->impl.circular.data;
                void* dst_second = (char*)new_data + (first_part * queue->element_size);
                memcpy(dst_second, src_second, second_part * queue->element_size);
            }
        }
    }
    
    // 释放旧数据
    cn_free(queue->impl.circular.data);
    
    // 更新队列结构
    queue->impl.circular.data = new_data;
    queue->impl.circular.capacity = new_capacity;
    queue->impl.circular.head_index = 0;
    queue->impl.circular.tail_index = queue->size;
    queue->impl.circular.is_full = (queue->size == new_capacity);
    
    return true;
}

/**
 * @brief 复制数组队列
 */
Stru_CN_Queue_t* CN_queue_internal_array_copy(const Stru_CN_Queue_t* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    // 创建新队列
    Stru_CN_Queue_t* dst = CN_queue_internal_create_array(
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
            void* src_element = CN_queue_internal_array_get_element(src, i);
            if (src_element == NULL)
            {
                CN_queue_internal_destroy_array(dst);
                return NULL;
            }
            
            if (!CN_queue_internal_array_enqueue(dst, src_element))
            {
                CN_queue_internal_destroy_array(dst);
                return NULL;
            }
        }
    }
    
    return dst;
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
 * @brief 复制循环数组队列
 */
Stru_CN_Queue_t* CN_queue_internal_circular_copy(const Stru_CN_Queue_t* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    // 创建新队列
    Stru_CN_Queue_t* dst = CN_queue_internal_create_circular(
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
            void* src_element = CN_queue_internal_circular_get_element(src, i);
            if (src_element == NULL)
            {
                CN_queue_internal_destroy_circular(dst);
                return NULL;
            }
            
            if (!CN_queue_internal_circular_enqueue(dst, src_element))
            {
                CN_queue_internal_destroy_circular(dst);
                return NULL;
            }
        }
    }
    
    return dst;
}

/**
 * @brief 获取队列元素指针（通用）
 */
void* CN_queue_internal_get_element(const Stru_CN_Queue_t* queue, size_t index)
{
    if (queue == NULL || index >= queue->size)
    {
        return NULL;
    }
    
    switch (queue->implementation)
    {
        case Eum_QUEUE_IMPLEMENTATION_ARRAY:
            return CN_queue_internal_array_get_element(queue, index);
            
        case Eum_QUEUE_IMPLEMENTATION_LIST:
            return CN_queue_internal_list_get_element(queue, index);
            
        case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
            return CN_queue_internal_circular_get_element(queue, index);
            
        default:
            return NULL;
    }
}

/**
 * @brief 获取数组队列元素指针
 */
void* CN_queue_internal_array_get_element(const Stru_CN_Queue_t* queue, size_t index)
{
    if (queue == NULL || index >= queue->size)
    {
        return NULL;
    }
    
    // 计算物理索引（考虑回绕）
    size_t physical_index = queue->impl.array.head_index + index;
    if (physical_index >= queue->impl.array.capacity)
    {
        physical_index -= queue->impl.array.capacity;
    }
    
    return (char*)queue->impl.array.data + (physical_index * queue->element_size);
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
 * @brief 获取循环数组队列元素指针
 */
void* CN_queue_internal_circular_get_element(const Stru_CN_Queue_t* queue, size_t index)
{
    if (queue == NULL || index >= queue->size)
    {
        return NULL;
    }
    
    // 计算物理索引（考虑回绕）
    size_t physical_index = queue->impl.circular.head_index + index;
    if (physical_index >= queue->impl.circular.capacity)
    {
        physical_index -= queue->impl.circular.capacity;
    }
    
    return (char*)queue->impl.circular.data + (physical_index * queue->element_size);
}

/**
 * @brief 计算循环数组索引（向前移动）
 */
size_t CN_queue_internal_circular_next_index(const Stru_CN_Queue_t* queue, size_t current_index, size_t steps)
{
    if (queue == NULL)
    {
        return 0;
    }
    
    size_t new_index = current_index + steps;
    if (new_index >= queue->impl.circular.capacity)
    {
        new_index -= queue->impl.circular.capacity;
    }
    
    return new_index;
}

/**
 * @brief 计算循环数组索引（向后移动）
 */
size_t CN_queue_internal_circular_prev_index(const Stru_CN_Queue_t* queue, size_t current_index, size_t steps)
{
    if (queue == NULL)
    {
        return 0;
    }
    
    if (steps > current_index)
    {
        steps -= current_index;
        return queue->impl.circular.capacity - steps;
    }
    else
    {
        return current_index - steps;
    }
}

/**
 * @brief 检查循环数组队列是否为空
 */
bool CN_queue_internal_circular_is_empty(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return true;
    }
    
    return (!queue->impl.circular.is_full && 
            queue->impl.circular.head_index == queue->impl.circular.tail_index);
}

/**
 * @brief 检查循环数组队列是否已满
 */
bool CN_queue_internal_circular_is_full(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return false;
    }
    
    return queue->impl.circular.is_full;
}

/**
 * @brief 获取循环数组队列的剩余空间
 */
size_t CN_queue_internal_circular_remaining_space(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return 0;
    }
    
    if (CN_queue_internal_circular_is_full(queue))
    {
        return 0;
    }
    
    if (queue->impl.circular.head_index <= queue->impl.circular.tail_index)
    {
        return queue->impl.circular.capacity - (queue->impl.circular.tail_index - queue->impl.circular.head_index);
    }
    else
    {
        return queue->impl.circular.head_index - queue->impl.circular.tail_index;
    }
}

/**
 * @brief 批量入队多个元素（通用）
 */
size_t CN_queue_internal_enqueue_batch(Stru_CN_Queue_t* queue, const void* elements, size_t count)
{
    if (queue == NULL || elements == NULL || count == 0)
    {
        return 0;
    }
    
    CN_queue_internal_lock(queue);
    
    size_t enqueued = 0;
    const char* element_ptr = (const char*)elements;
    
    for (size_t i = 0; i < count; i++)
    {
        const void* element = element_ptr + (i * queue->element_size);
        
        bool success;
        switch (queue->implementation)
        {
            case Eum_QUEUE_IMPLEMENTATION_ARRAY:
                success = CN_queue_internal_array_enqueue(queue, element);
                break;
                
            case Eum_QUEUE_IMPLEMENTATION_LIST:
                success = CN_queue_internal_list_enqueue(queue, element);
                break;
                
            case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
                success = CN_queue_internal_circular_enqueue(queue, element);
                break;
                
            default:
                success = false;
                break;
        }
        
        if (!success)
        {
            break;
        }
        
        enqueued++;
    }
    
    CN_queue_internal_unlock(queue);
    return enqueued;
}

/**
 * @brief 批量出队多个元素（通用）
 */
size_t CN_queue_internal_dequeue_batch(Stru_CN_Queue_t* queue, void* elements, size_t max_count)
{
    if (queue == NULL || max_count == 0)
    {
        return 0;
    }
    
    CN_queue_internal_lock(queue);
    
    size_t dequeued = 0;
    char* element_ptr = (char*)elements;
    
    for (size_t i = 0; i < max_count && queue->size > 0; i++)
    {
        void* element = (element_ptr != NULL) ? (element_ptr + (i * queue->element_size)) : NULL;
        
        bool success;
        switch (queue->implementation)
        {
            case Eum_QUEUE_IMPLEMENTATION_ARRAY:
                success = CN_queue_internal_array_dequeue(queue, element);
                break;
                
            case Eum_QUEUE_IMPLEMENTATION_LIST:
                success = CN_queue_internal_list_dequeue(queue, element);
                break;
                
            case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
                success = CN_queue_internal_circular_dequeue(queue, element);
                break;
                
            default:
                success = false;
                break;
        }
        
        if (!success)
        {
            break;
        }
        
        dequeued++;
    }
    
    CN_queue_internal_unlock(queue);
    return dequeued;
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
 * @brief 循环数组实现的出队操作
 */
bool CN_queue_internal_circular_dequeue(Stru_CN_Queue_t* queue, void* element)
{
    if (queue == NULL || CN_queue_internal_circular_is_empty(queue))
    {
        return false;
    }
    
    // 获取队头元素
    void* src = (char*)queue->impl.circular.data + (queue->impl.circular.head_index * queue->element_size);
    
    // 如果提供了输出参数，复制元素值
    if (element != NULL)
    {
        memcpy(element, src, queue->element_size);
    }
    
    // 如果有释放函数，释放元素
    if (queue->free_func != NULL)
    {
        queue->free_func(src);
    }
    
    // 更新队头索引
    queue->impl.circular.head_index++;
    if (queue->impl.circular.head_index >= queue->impl.circular.capacity)
    {
        queue->impl.circular.head_index = 0;
    }
    
    // 队列不再满
    queue->impl.circular.is_full = false;
    
    queue->size--;
    
    return true;
}
