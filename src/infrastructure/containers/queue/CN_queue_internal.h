/******************************************************************************
 * 文件名: CN_queue_internal.h
 * 功能: CN_Language队列容器内部结构定义
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义队列内部结构
 * 版权: MIT许可证
 * 
 * 注意: 此文件仅供CN_queue模块内部使用，外部模块不应包含此文件
 ******************************************************************************/

#ifndef CN_QUEUE_INTERNAL_H
#define CN_QUEUE_INTERNAL_H

#include "CN_queue.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 队列节点结构体（用于链表实现）
 */
typedef struct Stru_CN_QueueNode_t
{
    void* data;                     /**< 节点数据 */
    struct Stru_CN_QueueNode_t* next; /**< 下一个节点指针 */
    struct Stru_CN_QueueNode_t* prev; /**< 上一个节点指针（双向链表） */
} Stru_CN_QueueNode_t;

/**
 * @brief 队列结构体（完整定义，仅供内部使用）
 */
struct Stru_CN_Queue_t
{
    // 通用字段
    Eum_CN_QueueImplementation_t implementation; /**< 队列实现类型 */
    size_t element_size;                         /**< 元素大小（字节） */
    size_t size;                                 /**< 当前大小（元素数量） */
    Eum_CN_QueueThreadSafety_t thread_safety;    /**< 线程安全选项 */
    CN_QueueFreeFunc free_func;                  /**< 释放函数 */
    CN_QueueCopyFunc copy_func;                  /**< 复制函数 */
    
    // 线程同步
#ifdef _WIN32
    CRITICAL_SECTION lock;                       /**< Windows临界区 */
#else
    pthread_mutex_t lock;                        /**< POSIX互斥锁 */
#endif
    bool lock_initialized;                       /**< 锁是否已初始化 */
    
    // 实现特定字段
    union
    {
        // 数组实现
        struct
        {
            void* data;                          /**< 数据数组 */
            size_t capacity;                     /**< 数组容量 */
            size_t head_index;                   /**< 队头索引 */
            size_t tail_index;                   /**< 队尾索引 */
        } array;
        
        // 链表实现
        struct
        {
            Stru_CN_QueueNode_t* head;           /**< 队头节点 */
            Stru_CN_QueueNode_t* tail;           /**< 队尾节点 */
        } list;
        
        // 循环数组实现
        struct
        {
            void* data;                          /**< 数据数组 */
            size_t capacity;                     /**< 数组容量 */
            size_t head_index;                   /**< 队头索引 */
            size_t tail_index;                   /**< 队尾索引 */
            bool is_full;                        /**< 队列是否已满 */
        } circular;
    } impl;
};

// ============================================================================
// 内部辅助函数声明
// ============================================================================

/**
 * @brief 初始化队列锁
 * 
 * @param queue 队列
 * @return 初始化成功返回true，失败返回false
 */
bool CN_queue_internal_init_lock(Stru_CN_Queue_t* queue);

/**
 * @brief 销毁队列锁
 * 
 * @param queue 队列
 */
void CN_queue_internal_destroy_lock(Stru_CN_Queue_t* queue);

/**
 * @brief 锁定队列（线程安全）
 * 
 * @param queue 队列
 */
void CN_queue_internal_lock(Stru_CN_Queue_t* queue);

/**
 * @brief 解锁队列（线程安全）
 * 
 * @param queue 队列
 */
void CN_queue_internal_unlock(Stru_CN_Queue_t* queue);

/**
 * @brief 创建数组实现的队列
 * 
 * @param element_size 元素大小
 * @param initial_capacity 初始容量
 * @param thread_safety 线程安全选项
 * @param free_func 释放函数
 * @param copy_func 复制函数
 * @return 新创建的队列，失败返回NULL
 */
Stru_CN_Queue_t* CN_queue_internal_create_array(
    size_t element_size,
    size_t initial_capacity,
    Eum_CN_QueueThreadSafety_t thread_safety,
    CN_QueueFreeFunc free_func,
    CN_QueueCopyFunc copy_func);

/**
 * @brief 创建链表实现的队列
 * 
 * @param element_size 元素大小
 * @param thread_safety 线程安全选项
 * @param free_func 释放函数
 * @param copy_func 复制函数
 * @return 新创建的队列，失败返回NULL
 */
Stru_CN_Queue_t* CN_queue_internal_create_list(
    size_t element_size,
    Eum_CN_QueueThreadSafety_t thread_safety,
    CN_QueueFreeFunc free_func,
    CN_QueueCopyFunc copy_func);

/**
 * @brief 创建循环数组实现的队列
 * 
 * @param element_size 元素大小
 * @param initial_capacity 初始容量
 * @param thread_safety 线程安全选项
 * @param free_func 释放函数
 * @param copy_func 复制函数
 * @return 新创建的队列，失败返回NULL
 */
Stru_CN_Queue_t* CN_queue_internal_create_circular(
    size_t element_size,
    size_t initial_capacity,
    Eum_CN_QueueThreadSafety_t thread_safety,
    CN_QueueFreeFunc free_func,
    CN_QueueCopyFunc copy_func);

/**
 * @brief 销毁数组实现的队列
 * 
 * @param queue 队列
 */
void CN_queue_internal_destroy_array(Stru_CN_Queue_t* queue);

/**
 * @brief 销毁链表实现的队列
 * 
 * @param queue 队列
 */
void CN_queue_internal_destroy_list(Stru_CN_Queue_t* queue);

/**
 * @brief 销毁循环数组实现的队列
 * 
 * @param queue 队列
 */
void CN_queue_internal_destroy_circular(Stru_CN_Queue_t* queue);

/**
 * @brief 数组实现的入队操作
 * 
 * @param queue 队列
 * @param element 要添加的元素
 * @return 入队成功返回true，失败返回false
 */
bool CN_queue_internal_array_enqueue(Stru_CN_Queue_t* queue, const void* element);

/**
 * @brief 链表实现的入队操作
 * 
 * @param queue 队列
 * @param element 要添加的元素
 * @return 入队成功返回true，失败返回false
 */
bool CN_queue_internal_list_enqueue(Stru_CN_Queue_t* queue, const void* element);

/**
 * @brief 循环数组实现的入队操作
 * 
 * @param queue 队列
 * @param element 要添加的元素
 * @return 入队成功返回true，失败返回false
 */
bool CN_queue_internal_circular_enqueue(Stru_CN_Queue_t* queue, const void* element);

/**
 * @brief 数组实现的出队操作
 * 
 * @param queue 队列
 * @param element 输出参数，接收移除的元素值
 * @return 出队成功返回true，失败返回false
 */
bool CN_queue_internal_array_dequeue(Stru_CN_Queue_t* queue, void* element);

/**
 * @brief 链表实现的出队操作
 * 
 * @param queue 队列
 * @param element 输出参数，接收移除的元素值
 * @return 出队成功返回true，失败返回false
 */
bool CN_queue_internal_list_dequeue(Stru_CN_Queue_t* queue, void* element);

/**
 * @brief 循环数组实现的出队操作
 * 
 * @param queue 队列
 * @param element 输出参数，接收移除的元素值
 * @return 出队成功返回true，失败返回false
 */
bool CN_queue_internal_circular_dequeue(Stru_CN_Queue_t* queue, void* element);

/**
 * @brief 数组实现的查看队头操作
 * 
 * @param queue 队列
 * @return 队头元素的指针
 */
void* CN_queue_internal_array_front(const Stru_CN_Queue_t* queue);

/**
 * @brief 链表实现的查看队头操作
 * 
 * @param queue 队列
 * @return 队头元素的指针
 */
void* CN_queue_internal_list_front(const Stru_CN_Queue_t* queue);

/**
 * @brief 循环数组实现的查看队头操作
 * 
 * @param queue 队列
 * @return 队头元素的指针
 */
void* CN_queue_internal_circular_front(const Stru_CN_Queue_t* queue);

/**
 * @brief 数组实现的查看队尾操作
 * 
 * @param queue 队列
 * @return 队尾元素的指针
 */
void* CN_queue_internal_array_rear(const Stru_CN_Queue_t* queue);

/**
 * @brief 链表实现的查看队尾操作
 * 
 * @param queue 队列
 * @return 队尾元素的指针
 */
void* CN_queue_internal_list_rear(const Stru_CN_Queue_t* queue);

/**
 * @brief 循环数组实现的查看队尾操作
 * 
 * @param queue 队列
 * @return 队尾元素的指针
 */
void* CN_queue_internal_circular_rear(const Stru_CN_Queue_t* queue);

/**
 * @brief 确保数组队列有足够容量
 * 
 * @param queue 队列
 * @param min_capacity 最小容量要求
 * @return 调整成功返回true，失败返回false
 */
bool CN_queue_internal_array_ensure_capacity(Stru_CN_Queue_t* queue, size_t min_capacity);

/**
 * @brief 确保循环数组队列有足够容量
 * 
 * @param queue 队列
 * @param min_capacity 最小容量要求
 * @return 调整成功返回true，失败返回false
 */
bool CN_queue_internal_circular_ensure_capacity(Stru_CN_Queue_t* queue, size_t min_capacity);

/**
 * @brief 复制数组队列
 * 
 * @param src 源队列
 * @return 新创建的队列副本，失败返回NULL
 */
Stru_CN_Queue_t* CN_queue_internal_array_copy(const Stru_CN_Queue_t* src);

/**
 * @brief 复制链表队列
 * 
 * @param src 源队列
 * @return 新创建的队列副本，失败返回NULL
 */
Stru_CN_Queue_t* CN_queue_internal_list_copy(const Stru_CN_Queue_t* src);

/**
 * @brief 复制循环数组队列
 * 
 * @param src 源队列
 * @return 新创建的队列副本，失败返回NULL
 */
Stru_CN_Queue_t* CN_queue_internal_circular_copy(const Stru_CN_Queue_t* src);

/**
 * @brief 获取队列元素指针（通用）
 * 
 * @param queue 队列
 * @param index 元素索引（0表示队头）
 * @return 元素指针，索引无效返回NULL
 */
void* CN_queue_internal_get_element(const Stru_CN_Queue_t* queue, size_t index);

/**
 * @brief 获取数组队列元素指针
 * 
 * @param queue 队列
 * @param index 元素索引
 * @return 元素指针
 */
void* CN_queue_internal_array_get_element(const Stru_CN_Queue_t* queue, size_t index);

/**
 * @brief 获取链表队列元素指针
 * 
 * @param queue 队列
 * @param index 元素索引
 * @return 元素指针
 */
void* CN_queue_internal_list_get_element(const Stru_CN_Queue_t* queue, size_t index);

/**
 * @brief 获取循环数组队列元素指针
 * 
 * @param queue 队列
 * @param index 元素索引
 * @return 元素指针
 */
void* CN_queue_internal_circular_get_element(const Stru_CN_Queue_t* queue, size_t index);

/**
 * @brief 计算循环数组索引（向前移动）
 * 
 * @param queue 队列
 * @param current_index 当前索引
 * @param steps 移动步数
 * @return 新的物理数组索引
 */
size_t CN_queue_internal_circular_next_index(const Stru_CN_Queue_t* queue, size_t current_index, size_t steps);

/**
 * @brief 计算循环数组索引（向后移动）
 * 
 * @param queue 队列
 * @param current_index 当前索引
 * @param steps 移动步数
 * @return 新的物理数组索引
 */
size_t CN_queue_internal_circular_prev_index(const Stru_CN_Queue_t* queue, size_t current_index, size_t steps);

/**
 * @brief 检查循环数组队列是否为空
 * 
 * @param queue 队列
 * @return 如果队列为空返回true，否则返回false
 */
bool CN_queue_internal_circular_is_empty(const Stru_CN_Queue_t* queue);

/**
 * @brief 检查循环数组队列是否已满
 * 
 * @param queue 队列
 * @return 如果队列已满返回true，否则返回false
 */
bool CN_queue_internal_circular_is_full(const Stru_CN_Queue_t* queue);

/**
 * @brief 获取循环数组队列的剩余空间
 * 
 * @param queue 队列
 * @return 剩余可入队元素数量
 */
size_t CN_queue_internal_circular_remaining_space(const Stru_CN_Queue_t* queue);

/**
 * @brief 批量入队多个元素（通用）
 * 
 * @param queue 队列
 * @param elements 元素数组
 * @param count 元素数量
 * @return 成功入队的元素数量
 */
size_t CN_queue_internal_enqueue_batch(Stru_CN_Queue_t* queue, const void* elements, size_t count);

/**
 * @brief 批量出队多个元素（通用）
 * 
 * @param queue 队列
 * @param elements 输出数组
 * @param max_count 最大出队数量
 * @return 实际出队的元素数量
 */
size_t CN_queue_internal_dequeue_batch(Stru_CN_Queue_t* queue, void* elements, size_t max_count);

/**
 * @brief 清空数组队列
 * 
 * @param queue 队列
 */
void CN_queue_internal_array_clear(Stru_CN_Queue_t* queue);

/**
 * @brief 清空链表队列
 * 
 * @param queue 队列
 */
void CN_queue_internal_list_clear(Stru_CN_Queue_t* queue);

/**
 * @brief 清空循环数组队列
 * 
 * @param queue 队列
 */
void CN_queue_internal_circular_clear(Stru_CN_Queue_t* queue);

#endif // CN_QUEUE_INTERNAL_H
