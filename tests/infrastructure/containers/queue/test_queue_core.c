/**
 * @file test_queue_core.c
 * @brief 队列核心模块测试实现文件
 * 
 * 实现队列核心模块的所有测试函数。
 * 遵循模块化测试设计，测试文件和main()函数分离。
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

/* ==================== 核心模块测试实现 ==================== */

bool test_queue_core_create_and_destroy(void)
{
    printf("测试: 队列创建和销毁... ");
    
    // 测试1: 创建队列
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL)
    {
        printf("失败: 创建队列返回NULL\n");
        return false;
    }
    
    // 测试2: 检查初始状态
    if (F_queue_size(queue) != 0)
    {
        printf("失败: 新队列大小不为0\n");
        F_destroy_queue(queue);
        return false;
    }
    
    if (!F_queue_is_empty(queue))
    {
        printf("失败: 新队列不为空\n");
        F_destroy_queue(queue);
        return false;
    }
    
    if (F_queue_is_full(queue))
    {
        printf("失败: 新队列已满\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试3: 销毁队列
    F_destroy_queue(queue);
    
    // 测试4: 创建队列失败情况
    Stru_Queue_t* invalid_queue = F_create_queue(0);
    if (invalid_queue != NULL)
    {
        printf("失败: item_size为0时应返回NULL\n");
        F_destroy_queue(invalid_queue);
        return false;
    }
    
    printf("通过\n");
    return true;
}

bool test_queue_core_basic_operations(void)
{
    printf("测试: 队列基本操作... ");
    
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL)
    {
        printf("失败: 创建队列失败\n");
        return false;
    }
    
    // 测试1: 入队操作
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++)
    {
        if (!F_queue_enqueue(queue, &values[i]))
        {
            printf("失败: 入队失败, 值=%d\n", values[i]);
            F_destroy_queue(queue);
            return false;
        }
    }
    
    // 测试2: 检查大小
    if (F_queue_size(queue) != 5)
    {
        printf("失败: 队列大小应为5, 实际为%zu\n", F_queue_size(queue));
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试3: 查看队头
    int* front = (int*)F_queue_peek(queue);
    if (front == NULL || *front != 10)
    {
        printf("失败: 队头元素应为10\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试4: 出队操作
    int dequeued;
    for (int i = 0; i < 5; i++)
    {
        if (!F_queue_dequeue(queue, &dequeued))
        {
            printf("失败: 出队失败, 索引=%d\n", i);
            F_destroy_queue(queue);
            return false;
        }
        
        if (dequeued != values[i])
        {
            printf("失败: 出队值应为%d, 实际为%d\n", values[i], dequeued);
            F_destroy_queue(queue);
            return false;
        }
    }
    
    // 测试5: 检查队列为空
    if (!F_queue_is_empty(queue))
    {
        printf("失败: 队列应为空\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试6: 空队列出队应失败
    if (F_queue_dequeue(queue, &dequeued))
    {
        printf("失败: 空队列出队应返回false\n");
        F_destroy_queue(queue);
        return false;
    }
    
    F_destroy_queue(queue);
    printf("通过\n");
    return true;
}

bool test_queue_core_query_operations(void)
{
    printf("测试: 队列查询操作... ");
    
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL)
    {
        printf("失败: 创建队列失败\n");
        return false;
    }
    
    // 测试容量
    size_t initial_capacity = F_queue_capacity(queue);
    if (initial_capacity == 0)
    {
        printf("失败: 队列容量应为非零\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 添加元素直到队列满
    int value = 42;
    size_t count = 0;
    while (F_queue_enqueue(queue, &value))
    {
        count++;
        
        // 检查大小
        if (F_queue_size(queue) != count)
        {
            printf("失败: 队列大小应为%zu, 实际为%zu\n", count, F_queue_size(queue));
            F_destroy_queue(queue);
            return false;
        }
        
        // 检查是否为空
        if (F_queue_is_empty(queue))
        {
            printf("失败: 非空队列不应报告为空\n");
            F_destroy_queue(queue);
            return false;
        }
    }
    
    // 此时队列应已满
    if (!F_queue_is_full(queue))
    {
        printf("失败: 队列应报告为满\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 检查容量
    size_t current_capacity = F_queue_capacity(queue);
    if (current_capacity <= initial_capacity)
    {
        printf("失败: 扩容后容量应增加\n");
        F_destroy_queue(queue);
        return false;
    }
    
    F_destroy_queue(queue);
    printf("通过\n");
    return true;
}

bool test_queue_core_management_operations(void)
{
    printf("测试: 队列管理操作... ");
    
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL)
    {
        printf("失败: 创建队列失败\n");
        return false;
    }
    
    // 添加一些元素
    int values[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++)
    {
        F_queue_enqueue(queue, &values[i]);
    }
    
    // 测试清空操作
    F_queue_clear(queue);
    
    if (!F_queue_is_empty(queue))
    {
        printf("失败: 清空后队列应为空\n");
        F_destroy_queue(queue);
        return false;
    }
    
    if (F_queue_size(queue) != 0)
    {
        printf("失败: 清空后队列大小应为0\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试容量调整
    size_t new_capacity = 100;
    if (!F_queue_reserve(queue, new_capacity))
    {
        printf("失败: 容量调整失败\n");
        F_destroy_queue(queue);
        return false;
    }
    
    if (F_queue_capacity(queue) < new_capacity)
    {
        printf("失败: 容量调整未生效\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试无效容量调整
    if (F_queue_reserve(queue, 0))
    {
        printf("失败: 调整到0容量应失败\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 添加元素后测试容量调整
    for (int i = 0; i < 10; i++)
    {
        F_queue_enqueue(queue, &values[i % 5]);
    }
    
    // 调整到小于当前大小的容量应失败
    if (F_queue_reserve(queue, 5))
    {
        printf("失败: 调整到小于当前大小的容量应失败\n");
        F_destroy_queue(queue);
        return false;
    }
    
    F_destroy_queue(queue);
    printf("通过\n");
    return true;
}

bool test_queue_core_error_handling(void)
{
    printf("测试: 队列错误处理... ");
    
    // 测试1: 无效参数
    if (F_queue_enqueue(NULL, NULL))
    {
        printf("失败: NULL队列入队应返回false\n");
        return false;
    }
    
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL)
    {
        printf("失败: 创建队列失败\n");
        return false;
    }
    
    if (F_queue_enqueue(queue, NULL))
    {
        printf("失败: NULL元素入队应返回false\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试2: 空队列操作
    if (F_queue_dequeue(queue, NULL))
    {
        printf("失败: 空队列出队应返回false\n");
        F_destroy_queue(queue);
        return false;
    }
    
    if (F_queue_peek(queue) != NULL)
    {
        printf("失败: 空队列查看应返回NULL\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试3: 无效迭代器参数
    if (F_create_queue_iterator(NULL) != NULL)
    {
        printf("失败: NULL队列创建迭代器应返回NULL\n");
        F_destroy_queue(queue);
        return false;
    }
    
    F_destroy_queue(queue);
    printf("通过\n");
    return true;
}
