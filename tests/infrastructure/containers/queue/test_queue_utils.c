/**
 * @file test_queue_utils.c
 * @brief 队列工具模块测试实现文件
 * 
 * 实现队列工具模块的所有测试函数。
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

/* ==================== 工具模块测试实现 ==================== */

bool test_queue_utils_copy(void)
{
    printf("测试: 队列复制功能... ");
    
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL)
    {
        printf("失败: 创建队列失败\n");
        return false;
    }
    
    // 添加一些元素
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++)
    {
        F_queue_enqueue(queue, &values[i]);
    }
    
    // 复制队列
    Stru_Queue_t* copy = F_queue_copy(queue);
    if (copy == NULL)
    {
        printf("失败: 复制队列失败\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 检查复制后的队列大小
    if (F_queue_size(copy) != F_queue_size(queue))
    {
        printf("失败: 复制队列大小不匹配\n");
        F_destroy_queue(queue);
        F_destroy_queue(copy);
        return false;
    }
    
    // 检查复制后的队列内容
    for (int i = 0; i < 5; i++)
    {
        int original_value, copied_value;
        F_queue_dequeue(queue, &original_value);
        F_queue_dequeue(copy, &copied_value);
        
        if (original_value != copied_value)
        {
            printf("失败: 复制队列内容不匹配, 索引=%d\n", i);
            F_destroy_queue(queue);
            F_destroy_queue(copy);
            return false;
        }
    }
    
    // 检查两个队列都为空
    if (!F_queue_is_empty(queue) || !F_queue_is_empty(copy))
    {
        printf("失败: 出队后队列应为空\n");
        F_destroy_queue(queue);
        F_destroy_queue(copy);
        return false;
    }
    
    F_destroy_queue(queue);
    F_destroy_queue(copy);
    
    // 测试空队列复制
    Stru_Queue_t* empty_queue = F_create_queue(sizeof(int));
    Stru_Queue_t* empty_copy = F_queue_copy(empty_queue);
    if (empty_copy == NULL)
    {
        printf("失败: 复制空队列失败\n");
        F_destroy_queue(empty_queue);
        return false;
    }
    
    if (!F_queue_is_empty(empty_copy))
    {
        printf("失败: 空队列复制后应为空\n");
        F_destroy_queue(empty_queue);
        F_destroy_queue(empty_copy);
        return false;
    }
    
    F_destroy_queue(empty_queue);
    F_destroy_queue(empty_copy);
    
    printf("通过\n");
    return true;
}

bool test_queue_utils_equals(void)
{
    printf("测试: 队列比较功能... ");
    
    // 创建两个相同的队列
    Stru_Queue_t* queue1 = F_create_queue(sizeof(int));
    Stru_Queue_t* queue2 = F_create_queue(sizeof(int));
    if (queue1 == NULL || queue2 == NULL)
    {
        printf("失败: 创建队列失败\n");
        if (queue1) F_destroy_queue(queue1);
        if (queue2) F_destroy_queue(queue2);
        return false;
    }
    
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++)
    {
        F_queue_enqueue(queue1, &values[i]);
        F_queue_enqueue(queue2, &values[i]);
    }
    
    // 测试相等的队列
    if (!F_queue_equals(queue1, queue2, NULL))
    {
        printf("失败: 相同队列应相等\n");
        F_destroy_queue(queue1);
        F_destroy_queue(queue2);
        return false;
    }
    
    // 测试不同的队列
    Stru_Queue_t* queue3 = F_create_queue(sizeof(int));
    if (queue3 == NULL)
    {
        printf("失败: 创建队列失败\n");
        F_destroy_queue(queue1);
        F_destroy_queue(queue2);
        return false;
    }
    
    int different_values[] = {10, 20, 30, 40, 55}; // 最后一个元素不同
    for (int i = 0; i < 5; i++)
    {
        F_queue_enqueue(queue3, &different_values[i]);
    }
    
    if (F_queue_equals(queue1, queue3, NULL))
    {
        printf("失败: 不同队列不应相等\n");
        F_destroy_queue(queue1);
        F_destroy_queue(queue2);
        F_destroy_queue(queue3);
        return false;
    }
    
    // 测试大小不同的队列
    Stru_Queue_t* queue4 = F_create_queue(sizeof(int));
    if (queue4 == NULL)
    {
        printf("失败: 创建队列失败\n");
        F_destroy_queue(queue1);
        F_destroy_queue(queue2);
        F_destroy_queue(queue3);
        return false;
    }
    
    for (int i = 0; i < 3; i++) // 只添加3个元素
    {
        F_queue_enqueue(queue4, &values[i]);
    }
    
    if (F_queue_equals(queue1, queue4, NULL))
    {
        printf("失败: 大小不同的队列不应相等\n");
        F_destroy_queue(queue1);
        F_destroy_queue(queue2);
        F_destroy_queue(queue3);
        F_destroy_queue(queue4);
        return false;
    }
    
    // 测试空队列
    Stru_Queue_t* empty_queue1 = F_create_queue(sizeof(int));
    Stru_Queue_t* empty_queue2 = F_create_queue(sizeof(int));
    if (empty_queue1 == NULL || empty_queue2 == NULL)
    {
        printf("失败: 创建空队列失败\n");
        F_destroy_queue(queue1);
        F_destroy_queue(queue2);
        F_destroy_queue(queue3);
        F_destroy_queue(queue4);
        if (empty_queue1) F_destroy_queue(empty_queue1);
        if (empty_queue2) F_destroy_queue(empty_queue2);
        return false;
    }
    
    if (!F_queue_equals(empty_queue1, empty_queue2, NULL))
    {
        printf("失败: 两个空队列应相等\n");
        F_destroy_queue(queue1);
        F_destroy_queue(queue2);
        F_destroy_queue(queue3);
        F_destroy_queue(queue4);
        F_destroy_queue(empty_queue1);
        F_destroy_queue(empty_queue2);
        return false;
    }
    
    F_destroy_queue(queue1);
    F_destroy_queue(queue2);
    F_destroy_queue(queue3);
    F_destroy_queue(queue4);
    F_destroy_queue(empty_queue1);
    F_destroy_queue(empty_queue2);
    
    printf("通过\n");
    return true;
}

bool test_queue_utils_batch_operations(void)
{
    printf("测试: 批量操作功能... ");
    
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL)
    {
        printf("失败: 创建队列失败\n");
        return false;
    }
    
    // 准备批量数据
    int items[10];
    void* item_ptrs[10];
    for (int i = 0; i < 10; i++)
    {
        items[i] = i * 10; // 0, 10, 20, ..., 90
        item_ptrs[i] = &items[i];
    }
    
    // 测试批量入队
    size_t enqueued = F_queue_enqueue_batch(queue, item_ptrs, 10);
    if (enqueued != 10)
    {
        printf("失败: 批量入队数量不正确, 期望10, 实际%zu\n", enqueued);
        F_destroy_queue(queue);
        return false;
    }
    
    if (F_queue_size(queue) != 10)
    {
        printf("失败: 批量入队后队列大小不正确\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试批量出队
    int out_items[10];
    void* out_ptrs[10];
    for (int i = 0; i < 10; i++)
    {
        out_ptrs[i] = &out_items[i];
    }
    
    size_t dequeued = F_queue_dequeue_batch(queue, out_ptrs, 10);
    if (dequeued != 10)
    {
        printf("失败: 批量出队数量不正确, 期望10, 实际%zu\n", dequeued);
        F_destroy_queue(queue);
        return false;
    }
    
    // 检查出队的元素
    for (int i = 0; i < 10; i++)
    {
        if (out_items[i] != items[i])
        {
            printf("失败: 批量出队元素不匹配, 索引=%d\n", i);
            F_destroy_queue(queue);
            return false;
        }
    }
    
    if (!F_queue_is_empty(queue))
    {
        printf("失败: 批量出队后队列应为空\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试部分批量操作
    // 先添加一些元素
    for (int i = 0; i < 5; i++)
    {
        F_queue_enqueue(queue, &items[i]);
    }
    
    // 批量出队部分元素
    int partial_out[3];
    void* partial_ptrs[3];
    for (int i = 0; i < 3; i++)
    {
        partial_ptrs[i] = &partial_out[i];
    }
    
    dequeued = F_queue_dequeue_batch(queue, partial_ptrs, 3);
    if (dequeued != 3)
    {
        printf("失败: 部分批量出队数量不正确\n");
        F_destroy_queue(queue);
        return false;
    }
    
    if (F_queue_size(queue) != 2) // 剩余2个元素
    {
        printf("失败: 部分批量出队后队列大小不正确\n");
        F_destroy_queue(queue);
        return false;
    }
    
    F_destroy_queue(queue);
    printf("通过\n");
    return true;
}

bool test_queue_utils_conversion(void)
{
    printf("测试: 队列转换功能... ");
    
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
    
    // 转换为数组
    void* array = NULL;
    size_t count = F_queue_to_array(queue, &array);
    if (count != 5 || array == NULL)
    {
        printf("失败: 队列转数组失败\n");
        F_destroy_queue(queue);
        if (array) free(array);
        return false;
    }
    
    // 检查数组内容
    int* int_array = (int*)array;
    for (int i = 0; i < 5; i++)
    {
        if (int_array[i] != values[i])
        {
            printf("失败: 数组内容不匹配, 索引=%d\n", i);
            F_destroy_queue(queue);
            free(array);
            return false;
        }
    }
    
    free(array);
    
    // 从数组创建队列
    void* item_ptrs[5];
    for (int i = 0; i < 5; i++)
    {
        item_ptrs[i] = &values[i];
    }
    
    Stru_Queue_t* new_queue = F_queue_from_array(item_ptrs, 5, sizeof(int));
    if (new_queue == NULL)
    {
        printf("失败: 从数组创建队列失败\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 检查新队列
    if (F_queue_size(new_queue) != 5)
    {
        printf("失败: 从数组创建的队列大小不正确\n");
        F_destroy_queue(queue);
        F_destroy_queue(new_queue);
        return false;
    }
    
    for (int i = 0; i < 5; i++)
    {
        int value;
        F_queue_dequeue(new_queue, &value);
        if (value != values[i])
        {
            printf("失败: 从数组创建的队列内容不匹配, 索引=%d\n", i);
            F_destroy_queue(queue);
            F_destroy_queue(new_queue);
            return false;
        }
    }
    
    F_destroy_queue(queue);
    F_destroy_queue(new_queue);
    
    printf("通过\n");
    return true;
}

bool test_queue_utils_search(void)
{
    printf("测试: 队列查找功能... ");
    
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
    
    // 测试查找存在的元素
    int target = 30;
    int position = F_queue_find(queue, &target, NULL);
    if (position != 2) // 30在位置2（从0开始）
    {
        printf("失败: 查找元素位置不正确, 期望2, 实际%d\n", position);
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试查找不存在的元素
    int not_found = 99;
    position = F_queue_find(queue, &not_found, NULL);
    if (position != -1)
    {
        printf("失败: 查找不存在的元素应返回-1, 实际%d\n", position);
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试contains函数
    if (!F_queue_contains(queue, &target, NULL))
    {
        printf("失败: 队列应包含元素30\n");
        F_destroy_queue(queue);
        return false;
    }
    
    if (F_queue_contains(queue, &not_found, NULL))
    {
        printf("失败: 队列不应包含元素99\n");
        F_destroy_queue(queue);
        return false;
    }
    
    // 测试空队列查找
    Stru_Queue_t* empty_queue = F_create_queue(sizeof(int));
    if (empty_queue == NULL)
    {
        printf("失败: 创建空队列失败\n");
        F_destroy_queue(queue);
        return false;
    }
    
    position = F_queue_find(empty_queue, &target, NULL);
    if (position != -1)
    {
        printf("失败: 空队列查找应返回-1\n");
        F_destroy_queue(queue);
        F_destroy_queue(empty_queue);
        return false;
    }
    
    if (F_queue_contains(empty_queue, &target, NULL))
    {
        printf("失败: 空队列不应包含任何元素\n");
        F_destroy_queue(queue);
        F_destroy_queue(empty_queue);
        return false;
    }
    
    F_destroy_queue(queue);
    F_destroy_queue(empty_queue);
    
    printf("通过\n");
    return true;
}
