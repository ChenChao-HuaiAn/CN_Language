/******************************************************************************
 * 文件名: CN_queue.h
 * 功能: CN_Language队列容器
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义队列接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_QUEUE_H
#define CN_QUEUE_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 队列实现类型枚举
 */
typedef enum Eum_CN_QueueImplementation_t
{
    Eum_QUEUE_IMPLEMENTATION_ARRAY = 0,      /**< 基于数组实现的队列 */
    Eum_QUEUE_IMPLEMENTATION_LIST = 1,       /**< 基于链表实现的队列 */
    Eum_QUEUE_IMPLEMENTATION_CIRCULAR = 2    /**< 基于循环数组实现的队列（优化） */
} Eum_CN_QueueImplementation_t;

/**
 * @brief 队列线程安全选项枚举
 */
typedef enum Eum_CN_QueueThreadSafety_t
{
    Eum_QUEUE_THREAD_UNSAFE = 0,             /**< 非线程安全队列 */
    Eum_QUEUE_THREAD_SAFE = 1                /**< 线程安全队列 */
} Eum_CN_QueueThreadSafety_t;

/**
 * @brief 队列结构体（不透明类型）
 */
typedef struct Stru_CN_Queue_t Stru_CN_Queue_t;

/**
 * @brief 队列迭代器结构体
 */
typedef struct Stru_CN_QueueIterator_t
{
    Stru_CN_Queue_t* queue;                 /**< 关联的队列 */
    size_t current_index;                   /**< 当前索引（从队头开始） */
    void* current_element;                  /**< 当前元素指针 */
} Stru_CN_QueueIterator_t;

/**
 * @brief 队列元素释放函数类型
 * 
 * @param element 要释放的元素
 */
typedef void (*CN_QueueFreeFunc)(void* element);

/**
 * @brief 队列元素复制函数类型
 * 
 * @param src 源元素
 * @return 新分配的元素副本
 */
typedef void* (*CN_QueueCopyFunc)(const void* src);

// ============================================================================
// 队列创建和销毁
// ============================================================================

/**
 * @brief 创建队列
 * 
 * @param implementation 队列实现类型
 * @param element_size 元素大小（字节）
 * @param initial_capacity 初始容量（仅对数组实现有效）
 * @param thread_safety 线程安全选项
 * @return 新创建的队列，失败返回NULL
 */
Stru_CN_Queue_t* CN_queue_create(Eum_CN_QueueImplementation_t implementation,
                                 size_t element_size, 
                                 size_t initial_capacity,
                                 Eum_CN_QueueThreadSafety_t thread_safety);

/**
 * @brief 创建带自定义函数的队列
 * 
 * @param implementation 队列实现类型
 * @param element_size 元素大小（字节）
 * @param initial_capacity 初始容量
 * @param thread_safety 线程安全选项
 * @param free_func 释放函数（可为NULL）
 * @param copy_func 复制函数（可为NULL）
 * @return 新创建的队列，失败返回NULL
 */
Stru_CN_Queue_t* CN_queue_create_custom(
    Eum_CN_QueueImplementation_t implementation,
    size_t element_size,
    size_t initial_capacity,
    Eum_CN_QueueThreadSafety_t thread_safety,
    CN_QueueFreeFunc free_func,
    CN_QueueCopyFunc copy_func);

/**
 * @brief 销毁队列
 * 
 * @param queue 要销毁的队列
 */
void CN_queue_destroy(Stru_CN_Queue_t* queue);

/**
 * @brief 清空队列（移除所有元素）
 * 
 * @param queue 要清空的队列
 */
void CN_queue_clear(Stru_CN_Queue_t* queue);

// ============================================================================
// 队列属性查询
// ============================================================================

/**
 * @brief 获取队列大小（元素数量）
 * 
 * @param queue 队列
 * @return 队列大小
 */
size_t CN_queue_size(const Stru_CN_Queue_t* queue);

/**
 * @brief 检查队列是否为空
 * 
 * @param queue 队列
 * @return 如果队列为空返回true，否则返回false
 */
bool CN_queue_is_empty(const Stru_CN_Queue_t* queue);

/**
 * @brief 检查队列是否已满（仅对固定容量队列有效）
 * 
 * @param queue 队列
 * @return 如果队列已满返回true，否则返回false
 */
bool CN_queue_is_full(const Stru_CN_Queue_t* queue);

/**
 * @brief 获取队列容量（仅对数组实现有效）
 * 
 * @param queue 队列
 * @return 队列容量
 */
size_t CN_queue_capacity(const Stru_CN_Queue_t* queue);

/**
 * @brief 获取元素大小
 * 
 * @param queue 队列
 * @return 元素大小（字节）
 */
size_t CN_queue_element_size(const Stru_CN_Queue_t* queue);

/**
 * @brief 获取队列实现类型
 * 
 * @param queue 队列
 * @return 队列实现类型
 */
Eum_CN_QueueImplementation_t CN_queue_implementation(const Stru_CN_Queue_t* queue);

// ============================================================================
// 队列操作（核心功能）
// ============================================================================

/**
 * @brief 入队（在队尾添加元素）
 * 
 * @param queue 队列
 * @param element 要添加的元素
 * @return 入队成功返回true，失败返回false
 */
bool CN_queue_enqueue(Stru_CN_Queue_t* queue, const void* element);

/**
 * @brief 出队（移除并返回队头元素）
 * 
 * @param queue 队列
 * @param element 输出参数，接收移除的元素值（可为NULL）
 * @return 出队成功返回true，失败返回false
 */
bool CN_queue_dequeue(Stru_CN_Queue_t* queue, void* element);

/**
 * @brief 查看队头元素（不移除）
 * 
 * @param queue 队列
 * @return 队头元素的指针，队列为空返回NULL
 */
void* CN_queue_front(const Stru_CN_Queue_t* queue);

/**
 * @brief 查看队尾元素（不移除）
 * 
 * @param queue 队列
 * @return 队尾元素的指针，队列为空返回NULL
 */
void* CN_queue_rear(const Stru_CN_Queue_t* queue);

/**
 * @brief 查看队列中指定位置的元素
 * 
 * @param queue 队列
 * @param index 元素索引（0表示队头）
 * @return 指定位置元素的指针，索引无效返回NULL
 */
void* CN_queue_get(const Stru_CN_Queue_t* queue, size_t index);

// ============================================================================
// 队列操作（高级功能）
// ============================================================================

/**
 * @brief 确保队列有足够容量（仅对数组实现有效）
 * 
 * @param queue 队列
 * @param min_capacity 最小容量要求
 * @return 调整成功返回true，失败返回false
 */
bool CN_queue_ensure_capacity(Stru_CN_Queue_t* queue, size_t min_capacity);

/**
 * @brief 缩小队列容量以匹配大小（仅对数组实现有效）
 * 
 * @param queue 队列
 * @return 调整成功返回true，失败返回false
 */
bool CN_queue_shrink_to_fit(Stru_CN_Queue_t* queue);

/**
 * @brief 复制队列
 * 
 * @param src 源队列
 * @return 新创建的队列副本，失败返回NULL
 */
Stru_CN_Queue_t* CN_queue_copy(const Stru_CN_Queue_t* src);

/**
 * @brief 反转队列中元素的顺序
 * 
 * @param queue 队列
 * @return 反转成功返回true，失败返回false
 */
bool CN_queue_reverse(Stru_CN_Queue_t* queue);

// ============================================================================
// 循环队列特定操作
// ============================================================================

/**
 * @brief 检查是否为循环队列
 * 
 * @param queue 队列
 * @return 如果是循环队列返回true，否则返回false
 */
bool CN_queue_is_circular(const Stru_CN_Queue_t* queue);

/**
 * @brief 获取循环队列的头部索引（仅对循环队列有效）
 * 
 * @param queue 队列
 * @return 头部索引
 */
size_t CN_queue_head_index(const Stru_CN_Queue_t* queue);

/**
 * @brief 获取循环队列的尾部索引（仅对循环队列有效）
 * 
 * @param queue 队列
 * @return 尾部索引
 */
size_t CN_queue_tail_index(const Stru_CN_Queue_t* queue);

/**
 * @brief 循环队列的剩余空间（仅对循环队列有效）
 * 
 * @param queue 队列
 * @return 剩余可入队元素数量
 */
size_t CN_queue_remaining_space(const Stru_CN_Queue_t* queue);

// ============================================================================
// 线程安全操作（仅对线程安全队列有效）
// ============================================================================

/**
 * @brief 线程安全地入队
 * 
 * @param queue 队列
 * @param element 要添加的元素
 * @return 入队成功返回true，失败返回false
 */
bool CN_queue_enqueue_safe(Stru_CN_Queue_t* queue, const void* element);

/**
 * @brief 线程安全地出队
 * 
 * @param queue 队列
 * @param element 输出参数，接收移除的元素值（可为NULL）
 * @return 出队成功返回true，失败返回false
 */
bool CN_queue_dequeue_safe(Stru_CN_Queue_t* queue, void* element);

/**
 * @brief 线程安全地查看队头元素
 * 
 * @param queue 队列
 * @return 队头元素的指针，队列为空返回NULL
 */
void* CN_queue_front_safe(const Stru_CN_Queue_t* queue);

/**
 * @brief 获取队列的互斥锁（用于手动同步）
 * 
 * @param queue 队列
 * @return 成功获取锁返回true，失败返回false
 * @note 调用者必须在操作完成后调用CN_queue_unlock
 */
bool CN_queue_lock(Stru_CN_Queue_t* queue);

/**
 * @brief 释放队列的互斥锁
 * 
 * @param queue 队列
 */
void CN_queue_unlock(Stru_CN_Queue_t* queue);

// ============================================================================
// 迭代器支持
// ============================================================================

/**
 * @brief 创建队列迭代器（从队头开始）
 * 
 * @param queue 队列
 * @return 新创建的迭代器，失败返回NULL
 */
Stru_CN_QueueIterator_t* CN_queue_iterator_create(Stru_CN_Queue_t* queue);

/**
 * @brief 销毁迭代器
 * 
 * @param iterator 要销毁的迭代器
 */
void CN_queue_iterator_destroy(Stru_CN_QueueIterator_t* iterator);

/**
 * @brief 重置迭代器到队头
 * 
 * @param iterator 迭代器
 */
void CN_queue_iterator_reset(Stru_CN_QueueIterator_t* iterator);

/**
 * @brief 检查迭代器是否有下一个元素（向队尾方向）
 * 
 * @param iterator 迭代器
 * @return 如果有下一个元素返回true，否则返回false
 */
bool CN_queue_iterator_has_next(const Stru_CN_QueueIterator_t* iterator);

/**
 * @brief 获取下一个元素（向队尾方向）
 * 
 * @param iterator 迭代器
 * @return 下一个元素的指针，没有更多元素返回NULL
 */
void* CN_queue_iterator_next(Stru_CN_QueueIterator_t* iterator);

/**
 * @brief 获取当前元素
 * 
 * @param iterator 迭代器
 * @return 当前元素的指针
 */
void* CN_queue_iterator_current(const Stru_CN_QueueIterator_t* iterator);

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 比较两个队列
 * 
 * @param queue1 第一个队列
 * @param queue2 第二个队列
 * @return 如果队列相等返回true，否则返回false
 */
bool CN_queue_equal(const Stru_CN_Queue_t* queue1, const Stru_CN_Queue_t* queue2);

/**
 * @brief 转储队列信息到控制台（调试用）
 * 
 * @param queue 队列
 */
void CN_queue_dump(const Stru_CN_Queue_t* queue);

/**
 * @brief 将队列内容转换为数组（从队头到队尾）
 * 
 * @param queue 队列
 * @param array 输出数组（必须足够大）
 * @param max_elements 最大元素数量
 * @return 实际复制的元素数量
 */
size_t CN_queue_to_array(const Stru_CN_Queue_t* queue, void* array, size_t max_elements);

/**
 * @brief 批量入队多个元素
 * 
 * @param queue 队列
 * @param elements 元素数组
 * @param count 元素数量
 * @return 成功入队的元素数量
 */
size_t CN_queue_enqueue_batch(Stru_CN_Queue_t* queue, const void* elements, size_t count);

/**
 * @brief 批量出队多个元素
 * 
 * @param queue 队列
 * @param elements 输出数组
 * @param max_count 最大出队数量
 * @return 实际出队的元素数量
 */
size_t CN_queue_dequeue_batch(Stru_CN_Queue_t* queue, void* elements, size_t max_count);

#endif // CN_QUEUE_H
