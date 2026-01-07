/**
 * @file test_queue_integration.c
 * @brief 队列模块集成测试实现文件
 * 
 * 实现队列模块的集成测试、边界情况测试和性能测试。
 * 遵循模块化测试设计，每个测试文件专注于单一功能领域。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_queue.h"
#include "../../../../src/infrastructure/containers/queue/CN_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ==================== 边界情况测试 ==================== */

bool test_queue_edge_cases(void)
{
    printf("测试: 队列边界情况... ");
    
    // 测试1: 创建超大元素大小的队列
    Stru_Queue_t* large_item_queue = F_create_queue(1024 * 1024); // 1MB元素
    if (large_item_queue == NULL)
    {
        printf("失败: 创建大元素队列失败\n");
        return false;
    }
    
    // 分配一个大元素
    char* large_item = (char*)malloc(1024 * 1024);
    if (large_item == NULL)
    {
        printf("失败: 分配大元素内存失败\n");
        F_destroy_queue(large_item_queue);
        return false;
    }
    
    memset(large_item, 'A', 1024 * 1024);
    
    // 入队大元素
    if (!F_queue_enqueue(large_item_queue, large_item))
    {
        printf("失败: 入队大元素失败\n");
        free(large_item);
        F_destroy_queue(large_item_queue);
        return false;
    }
    
    // 出队大元素
    char* dequeued_large_item = (char*)malloc(1024 * 1024);
    if (dequeued_large_item == NULL)
    {
        printf("失败: 分配出队内存失败\n");
        free(large_item);
        F_destroy_queue(large_item_queue);
        return false;
    }
    
    if (!F_queue_dequeue(large_item_queue, dequeued_large_item))
    {
        printf("失败: 出队大元素失败\n");
        free(large_item);
        free(dequeued_large_item);
        F_destroy_queue(large_item_queue);
        return false;
    }
    
    // 检查出队的元素
    if (memcmp(large_item, dequeued_large_item, 1024 * 1024) != 0)
    {
        printf("失败: 大元素内容不匹配\n");
        free(large_item);
        free(dequeued_large_item);
        F_destroy_queue(large_item_queue);
        return false;
    }
    
    free(large_item);
    free(dequeued_large_item);
    F_destroy_queue(large_item_queue);
    
    // 测试2: 创建大量元素的队列
    Stru_Queue_t* many_items_queue = F_create_queue(sizeof(int));
    if (many_items_queue == NULL)
    {
        printf("失败: 创建队列失败\n");
        return false;
    }
    
    // 添加大量元素
    int max_items = 10000;
    for (int i = 0; i < max_items; i++)
    {
        if (!F_queue_enqueue(many_items_queue, &i))
        {
            printf("失败: 添加大量元素失败, 索引=%d\n", i);
            F_destroy_queue(many_items_queue);
            return false;
        }
    }
    
    if (F_queue_size(many_items_queue) != (size_t)max_items)
    {
        printf("失败: 大量元素队列大小不正确\n");
        F_destroy_queue(many_items_queue);
        return false;
    }
    
    // 检查所有元素
    for (int i = 0; i < max_items; i++)
    {
        int value;
        if (!F_queue_dequeue(many_items_queue, &value))
        {
            printf("失败: 出队大量元素失败, 索引=%d\n", i);
            F_destroy_queue(many_items_queue);
            return false;
        }
        
        if (value != i)
        {
            printf("失败: 大量元素值不匹配, 期望%d, 实际%d\n", i, value);
            F_destroy_queue(many_items_queue);
            return false;
        }
    }
    
    if (!F_queue_is_empty(many_items_queue))
    {
        printf("失败: 大量元素出队后队列应为空\n");
        F_destroy_queue(many_items_queue);
        return false;
    }
    
    F_destroy_queue(many_items_queue);
    
    printf("通过\n");
    return true;
}

/* ==================== 性能测试 ==================== */

bool test_queue_performance(void)
{
    printf("测试: 队列性能特征... ");
    
    // 测试入队性能
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL)
    {
        printf("失败: 创建队列失败\n");
        return false;
    }
    
    // 批量入队测试
    int items[1000];
    void* item_ptrs[1000];
    for (int i = 0; i < 1000; i++)
    {
        items[i] = i;
        item_ptrs[i] = &items[i];
    }
    
    // 使用批量入队
    size_t enqueued = F_queue_enqueue_batch(queue, item_ptrs, 1000);
    if (enqueued != 1000)
    {
        printf("失败: 批量入队性能测试失败\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 批量出队测试
    int out_items[1000];
    void* out_ptrs[1000];
    for (int i = 0; i < 1000; i++)
    {
        out_ptrs[i] = &out_items[i];
    }
    
    size_t dequeued = F_queue_dequeue_batch(queue, out_ptrs, 1000);
    if (dequeued != 1000)
    {
        printf("失败: 批量出队性能测试失败\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 检查出队的元素
    for (int i = 0; i < 1000; i++)
    {
        if (out_items[i] != items[i])
        {
            printf("失败: 性能测试中元素不匹配\n");
            F_destroy_queue(queue);
            return false;
        }
    }
    
    F_destroy_queue(queue);
    
    // 测试迭代器性能
    Stru_Queue_t* queue2 = F_create_queue(sizeof(int));
    if (queue2 == NULL)
    {
        printf("失败: 创建队列失败\n");
        return false;
    }
    
    // 添加元素
    for (int i = 0; i < 500; i++)
    {
        F_queue_enqueue(queue2, &i);
    }
    
    // 使用迭代器遍历
    Stru_QueueIterator_t* iterator = F_create_queue_iterator(queue2);
    if (iterator == NULL)
    {
        printf("失败: 创建迭代器失败\n");
        F_destroy_queue(queue2);
        return false;
    }
    
    int count = 0;
    while (F_queue_iterator_has_next(iterator))
    {
        int value;
        if (!F_queue_iterator_next(iterator, &value))
        {
            printf("失败: 迭代器遍历失败\n");
            F_destroy_queue_iterator(iterator);
            F_destroy_queue(queue2);
            return false;
        }
        
        if (value != count)
        {
            printf("失败: 迭代器遍历元素不匹配\n");
            F_destroy_queue_iterator(iterator);
            F_destroy_queue(queue2);
            return false;
        }
        
        count++;
    }
    
    if (count != 500)
    {
        printf("失败: 迭代器遍历数量不正确\n");
        F_destroy_queue_iterator(iterator);
        F_destroy_queue(queue2);
        return false;
    }
    
    F_destroy_queue_iterator(iterator);
    F_destroy_queue(queue2);
    
    printf("通过\n");
    return true;
}

/* ==================== 集成测试 ==================== */

bool test_queue_integration(void)
{
    printf("测试: 队列模块集成... ");
    
    // 测试完整的队列使用流程
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL)
    {
        printf("失败: 创建队列失败\n");
        return false;
    }
    
    // 1. 添加元素
    int values[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++)
    {
        if (!F_queue_enqueue(queue, &values[i]))
        {
            printf("失败: 入队失败\n");
            F_destroy_queue(queue);
            return false;
        }
    }
    
    // 2. 使用迭代器
    Stru_QueueIterator_t* iterator = F_create_queue_iterator(queue);
    if (iterator == NULL)
    {
        printf("失败: 创建迭代器失败\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 3. 复制队列
    Stru_Queue_t* copy = F_queue_copy(queue);
    if (copy == NULL)
    {
        printf("失败: 复制队列失败\n");
        F_destroy_queue_iterator(iterator);
        F_destroy_queue(queue);
        return false;
    }
    
    // 4. 比较队列
    if (!F_queue_equals(queue, copy, NULL))
    {
        printf("失败: 队列比较失败\n");
        F_destroy_queue_iterator(iterator);
        F_destroy_queue(queue);
        F_destroy_queue(copy);
        return false;
    }
    
    // 5. 转换为数组
    void* array = NULL;
    size_t count = F_queue_to_array(queue, &array);
    if (count != 5 || array == NULL)
    {
        printf("失败: 队列转数组失败\n");
        F_destroy_queue_iterator(iterator);
        F_destroy_queue(queue);
        F_destroy_queue(copy);
        if (array) free(array);
        return false;
    }
    
    // 6. 从数组创建队列
    void* item_ptrs[5];
    for (int i = 0; i < 5; i++)
    {
        item_ptrs[i] = &values[i];
    }
    
    Stru_Queue_t* from_array = F_queue_from_array(item_ptrs, 5, sizeof(int));
    if (from_array == NULL)
    {
        printf("失败: 从数组创建队列失败\n");
        F_destroy_queue_iterator(iterator);
        F_destroy_queue(queue);
        F_destroy_queue(copy);
        free(array);
        return false;
    }
    
    // 7. 清理所有资源
    free(array);
    F_destroy_queue_iterator(iterator);
    F_destroy_queue(queue);
    F_destroy_queue(copy);
    F_destroy_queue(from_array);
    
    printf("通过\n");
    return true;
}
