/**
 * @file test_queue_iterator.c
 * @brief 队列迭代器模块测试实现文件
 * 
 * 实现队列迭代器模块的所有测试函数。
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

/* ==================== 迭代器模块测试实现 ==================== */

bool test_queue_iterator_create_and_destroy(void)
{
    printf("测试: 迭代器创建和销毁... ");
    
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL)
    {
        printf("失败: 创建队列失败\n");
        return false;
    }
    
    // 添加一些元素
    int values[] = {1, 2, 3};
    for (int i = 0; i < 3; i++)
    {
        F_queue_enqueue(queue, &values[i]);
    }
    
    // 创建迭代器
    Stru_QueueIterator_t* iterator = F_create_queue_iterator(queue);
    if (iterator == NULL)
    {
        printf("失败: 创建迭代器失败\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 销毁迭代器
    F_destroy_queue_iterator(iterator);
    
    // 测试空队列迭代器
    Stru_Queue_t* empty_queue = F_create_queue(sizeof(int));
    Stru_QueueIterator_t* empty_iterator = F_create_queue_iterator(empty_queue);
    if (empty_iterator == NULL)
    {
        printf("失败: 创建空队列迭代器失败\n");
        F_destroy_queue(queue);
        F_destroy_queue(empty_queue);
        return false;
    }
    
    F_destroy_queue_iterator(empty_iterator);
    F_destroy_queue(empty_queue);
    F_destroy_queue(queue);
    
    printf("通过\n");
    return true;
}

bool test_queue_iterator_traversal(void)
{
    printf("测试: 迭代器遍历... ");
    
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL)
    {
        printf("失败: 创建队列失败\n");
        return false;
    }
    
    // 添加元素
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++)
    {
        F_queue_enqueue(queue, &values[i]);
    }
    
    Stru_QueueIterator_t* iterator = F_create_queue_iterator(queue);
    if (iterator == NULL)
    {
        printf("失败: 创建迭代器失败\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 遍历并检查元素
    int index = 0;
    while (F_queue_iterator_has_next(iterator))
    {
        int value;
        if (!F_queue_iterator_next(iterator, &value))
        {
            printf("失败: 获取下一个元素失败\n");
            F_destroy_queue_iterator(iterator);
            F_destroy_queue(queue);
            return false;
        }
        
        if (value != values[index])
        {
            printf("失败: 元素值应为%d, 实际为%d\n", values[index], value);
            F_destroy_queue_iterator(iterator);
            F_destroy_queue(queue);
            return false;
        }
        
        index++;
    }
    
    // 检查是否遍历了所有元素
    if (index != 5)
    {
        printf("失败: 应遍历5个元素, 实际遍历了%d个\n", index);
        F_destroy_queue_iterator(iterator);
        F_destroy_queue(queue);
        return false;
    }
    
    // 再次调用next应失败
    int dummy;
    if (F_queue_iterator_next(iterator, &dummy))
    {
        printf("失败: 遍历结束后next应返回false\n");
        F_destroy_queue_iterator(iterator);
        F_destroy_queue(queue);
        return false;
    }

    F_destroy_queue_iterator(iterator);
    F_destroy_queue(queue);
    
    printf("通过\n");
    return true;
}

bool test_queue_iterator_state_queries(void)
{
    printf("测试: 迭代器状态查询... ");
    
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL)
    {
        printf("失败: 创建队列失败\n");
        return false;
    }
    
    // 添加元素
    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; i++)
    {
        F_queue_enqueue(queue, &values[i]);
    }
    
    Stru_QueueIterator_t* iterator = F_create_queue_iterator(queue);
    if (iterator == NULL)
    {
        printf("失败: 创建迭代器失败\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试初始状态
    if (!F_queue_iterator_has_next(iterator))
    {
        printf("失败: 初始状态应有下一个元素\n");
        F_destroy_queue_iterator(iterator);
        F_destroy_queue(queue);
        return false;
    }
    
    // 遍历一个元素
    int value;
    if (!F_queue_iterator_next(iterator, &value))
    {
        printf("失败: 获取第一个元素失败\n");
        F_destroy_queue_iterator(iterator);
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试剩余元素数量
    // 注意：当前接口没有提供获取剩余元素数量的函数，这里假设有
    // 实际上，我们需要检查迭代器是否还有下一个元素
    if (!F_queue_iterator_has_next(iterator))
    {
        printf("失败: 遍历一个元素后应还有剩余元素\n");
        F_destroy_queue_iterator(iterator);
        F_destroy_queue(queue);
        return false;
    }
    
    // 遍历所有元素
    F_queue_iterator_next(iterator, &value);
    F_queue_iterator_next(iterator, &value);
    
    // 测试结束状态
    if (F_queue_iterator_has_next(iterator))
    {
        printf("失败: 遍历所有元素后不应有下一个元素\n");
        F_destroy_queue_iterator(iterator);
        F_destroy_queue(queue);
        return false;
    }
    
    F_destroy_queue_iterator(iterator);
    F_destroy_queue(queue);
    
    printf("通过\n");
    return true;
}

bool test_queue_iterator_reset(void)
{
    printf("测试: 迭代器重置... ");
    
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL)
    {
        printf("失败: 创建队列失败\n");
        return false;
    }
    
    // 添加元素
    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; i++)
    {
        F_queue_enqueue(queue, &values[i]);
    }
    
    Stru_QueueIterator_t* iterator = F_create_queue_iterator(queue);
    if (iterator == NULL)
    {
        printf("失败: 创建迭代器失败\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 第一次遍历
    int value;
    int index = 0;
    while (F_queue_iterator_has_next(iterator))
    {
        if (!F_queue_iterator_next(iterator, &value))
        {
            printf("失败: 获取元素失败\n");
            F_destroy_queue_iterator(iterator);
            F_destroy_queue(queue);
            return false;
        }
        
        if (value != values[index])
        {
            printf("失败: 元素值应为%d, 实际为%d\n", values[index], value);
            F_destroy_queue_iterator(iterator);
            F_destroy_queue(queue);
            return false;
        }
        
        index++;
    }
    
    // 重置迭代器
    F_queue_iterator_reset(iterator);
    
    // 第二次遍历
    index = 0;
    while (F_queue_iterator_has_next(iterator))
    {
        if (!F_queue_iterator_next(iterator, &value))
        {
            printf("失败: 重置后获取元素失败\n");
            F_destroy_queue_iterator(iterator);
            F_destroy_queue(queue);
            return false;
        }
        
        if (value != values[index])
        {
            printf("失败: 重置后元素值应为%d, 实际为%d\n", values[index], value);
            F_destroy_queue_iterator(iterator);
            F_destroy_queue(queue);
            return false;
        }
        
        index++;
    }
    
    // 检查是否遍历了所有元素
    if (index != 3)
    {
        printf("失败: 应遍历3个元素, 实际遍历了%d个\n", index);
        F_destroy_queue_iterator(iterator);
        F_destroy_queue(queue);
        return false;
    }
    
    F_destroy_queue_iterator(iterator);
    F_destroy_queue(queue);
    
    printf("通过\n");
    return true;
}
