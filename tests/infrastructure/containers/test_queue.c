/******************************************************************************
 * 文件名: test_queue.c
 * 功能: CN_queue模块测试程序
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，测试CN_queue模块功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "../../../src/infrastructure/containers/queue/CN_queue.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// 测试函数声明
void test_queue_basic_operations(void);
void test_queue_different_implementations(void);
void test_queue_thread_safety(void);
void test_queue_iterator(void);
void test_queue_advanced_operations(void);
void test_queue_edge_cases(void);

int main(void)
{
    printf("=== CN_queue 模块测试 ===\n\n");
    
    printf("1. 测试队列基本操作:\n");
    test_queue_basic_operations();
    
    printf("\n2. 测试不同实现类型的队列:\n");
    test_queue_different_implementations();
    
    printf("\n3. 测试队列线程安全操作:\n");
    test_queue_thread_safety();
    
    printf("\n4. 测试队列迭代器:\n");
    test_queue_iterator();
    
    printf("\n5. 测试队列高级操作:\n");
    test_queue_advanced_operations();
    
    printf("\n6. 测试边界情况:\n");
    test_queue_edge_cases();
    
    printf("\n=== 所有测试完成 ===\n");
    return 0;
}

// 测试队列基本操作
void test_queue_basic_operations(void)
{
    printf("  创建数组实现的队列...\n");
    Stru_CN_Queue_t* queue = CN_queue_create(Eum_QUEUE_IMPLEMENTATION_ARRAY, 
                                            sizeof(int), 
                                            10, 
                                            Eum_QUEUE_THREAD_UNSAFE);
    
    if (queue == NULL)
    {
        printf("  错误: 无法创建队列\n");
        return;
    }
    
    printf("  队列创建成功\n");
    printf("  初始大小: %zu\n", CN_queue_size(queue));
    printf("  队列是否为空: %s\n", CN_queue_is_empty(queue) ? "是" : "否");
    
    // 测试入队操作
    printf("  测试入队操作...\n");
    for (int i = 1; i <= 5; i++)
    {
        if (CN_queue_enqueue(queue, &i))
        {
            printf("    入队元素 %d 成功\n", i);
        }
        else
        {
            printf("    入队元素 %d 失败\n", i);
        }
    }
    
    printf("  队列大小: %zu\n", CN_queue_size(queue));
    printf("  队列是否为空: %s\n", CN_queue_is_empty(queue) ? "是" : "否");
    
    // 测试查看队头元素
    printf("  测试查看队头元素...\n");
    int* front = (int*)CN_queue_front(queue);
    if (front != NULL)
    {
        printf("    队头元素: %d\n", *front);
    }
    
    // 测试查看队尾元素
    printf("  测试查看队尾元素...\n");
    int* rear = (int*)CN_queue_rear(queue);
    if (rear != NULL)
    {
        printf("    队尾元素: %d\n", *rear);
    }
    
    // 测试出队操作
    printf("  测试出队操作...\n");
    int dequeued_value;
    while (!CN_queue_is_empty(queue))
    {
        if (CN_queue_dequeue(queue, &dequeued_value))
        {
            printf("    出队元素: %d\n", dequeued_value);
        }
    }
    
    printf("  出队所有元素后队列大小: %zu\n", CN_queue_size(queue));
    printf("  队列是否为空: %s\n", CN_queue_is_empty(queue) ? "是" : "否");
    
    // 清理
    CN_queue_destroy(queue);
    printf("  队列销毁成功\n");
}

// 测试不同实现类型的队列
void test_queue_different_implementations(void)
{
    printf("  测试数组实现的队列...\n");
    Stru_CN_Queue_t* array_queue = CN_queue_create(Eum_QUEUE_IMPLEMENTATION_ARRAY, 
                                                  sizeof(double), 
                                                  5, 
                                                  Eum_QUEUE_THREAD_UNSAFE);
    
    if (array_queue != NULL)
    {
        for (double i = 1.0; i <= 3.0; i += 1.0)
        {
            CN_queue_enqueue(array_queue, &i);
        }
        printf("    数组队列大小: %zu, 容量: %zu\n", 
               CN_queue_size(array_queue), CN_queue_capacity(array_queue));
        CN_queue_destroy(array_queue);
    }
    
    printf("  测试链表实现的队列...\n");
    Stru_CN_Queue_t* list_queue = CN_queue_create(Eum_QUEUE_IMPLEMENTATION_LIST, 
                                                 sizeof(int), 
                                                 0, // 容量对链表实现无效
                                                 Eum_QUEUE_THREAD_UNSAFE);
    
    if (list_queue != NULL)
    {
        for (int i = 1; i <= 10; i++)
        {
            CN_queue_enqueue(list_queue, &i);
        }
        printf("    链表队列大小: %zu\n", CN_queue_size(list_queue));
        CN_queue_destroy(list_queue);
    }
    
    printf("  测试循环数组实现的队列...\n");
    Stru_CN_Queue_t* circular_queue = CN_queue_create(Eum_QUEUE_IMPLEMENTATION_CIRCULAR, 
                                                     sizeof(int), 
                                                     8, 
                                                     Eum_QUEUE_THREAD_UNSAFE);
    
    if (circular_queue != NULL)
    {
        for (int i = 1; i <= 6; i++)
        {
            CN_queue_enqueue(circular_queue, &i);
        }
        printf("    循环数组队列大小: %zu, 容量: %zu\n", 
               CN_queue_size(circular_queue), CN_queue_capacity(circular_queue));
        
        // 测试循环队列特定功能
        printf("    循环队列头部索引: %zu\n", CN_queue_head_index(circular_queue));
        printf("    循环队列尾部索引: %zu\n", CN_queue_tail_index(circular_queue));
        printf("    循环队列剩余空间: %zu\n", CN_queue_remaining_space(circular_queue));
        
        CN_queue_destroy(circular_queue);
    }
    
    printf("  不同实现类型的队列测试完成\n");
}

// 测试队列线程安全操作
void test_queue_thread_safety(void)
{
    printf("  创建线程安全队列...\n");
    Stru_CN_Queue_t* thread_safe_queue = CN_queue_create(Eum_QUEUE_IMPLEMENTATION_ARRAY, 
                                                        sizeof(int), 
                                                        20, 
                                                        Eum_QUEUE_THREAD_SAFE);
    
    if (thread_safe_queue == NULL)
    {
        printf("  错误: 无法创建线程安全队列\n");
        return;
    }
    
    printf("  线程安全队列创建成功\n");
    
    // 测试线程安全操作
    printf("  测试线程安全入队...\n");
    for (int i = 1; i <= 5; i++)
    {
        if (CN_queue_enqueue_safe(thread_safe_queue, &i))
        {
            printf("    线程安全入队元素 %d 成功\n", i);
        }
    }
    
    printf("  测试线程安全查看队头...\n");
    int* safe_front = (int*)CN_queue_front_safe(thread_safe_queue);
    if (safe_front != NULL)
    {
        printf("    线程安全查看队头元素: %d\n", *safe_front);
    }
    
    printf("  测试手动锁操作...\n");
    if (CN_queue_lock(thread_safe_queue))
    {
        printf("    成功获取队列锁\n");
        
        // 执行需要同步的操作
        int value = 99;
        CN_queue_enqueue(thread_safe_queue, &value);
        
        CN_queue_unlock(thread_safe_queue);
        printf("    已释放队列锁\n");
    }
    
    printf("  测试线程安全出队...\n");
    int safe_dequeued;
    while (CN_queue_dequeue_safe(thread_safe_queue, &safe_dequeued))
    {
        printf("    线程安全出队元素: %d\n", safe_dequeued);
    }
    
    CN_queue_destroy(thread_safe_queue);
    printf("  线程安全队列测试完成\n");
}

// 测试队列迭代器
void test_queue_iterator(void)
{
    printf("  创建队列并测试迭代器...\n");
    Stru_CN_Queue_t* queue = CN_queue_create(Eum_QUEUE_IMPLEMENTATION_ARRAY, 
                                            sizeof(int), 
                                            10, 
                                            Eum_QUEUE_THREAD_UNSAFE);
    
    if (queue == NULL)
    {
        printf("  错误: 无法创建队列\n");
        return;
    }
    
    // 填充数据
    for (int i = 1; i <= 5; i++)
    {
        CN_queue_enqueue(queue, &i);
    }
    
    printf("  创建队列迭代器...\n");
    Stru_CN_QueueIterator_t* iter = CN_queue_iterator_create(queue);
    if (iter == NULL)
    {
        printf("  错误: 无法创建迭代器\n");
        CN_queue_destroy(queue);
        return;
    }
    
    printf("  使用迭代器遍历队列（从队头到队尾）:\n");
    printf("    遍历结果: ");
    while (CN_queue_iterator_has_next(iter))
    {
        int* value = (int*)CN_queue_iterator_next(iter);
        if (value != NULL)
        {
            printf("%d ", *value);
        }
    }
    printf("\n");
    
    printf("  测试重置迭代器...\n");
    CN_queue_iterator_reset(iter);
    
    printf("  重置后遍历: ");
    while (CN_queue_iterator_has_next(iter))
    {
        int* value = (int*)CN_queue_iterator_next(iter);
        if (value != NULL)
        {
            printf("%d ", *value);
        }
    }
    printf("\n");
    
    printf("  测试获取当前元素...\n");
    CN_queue_iterator_reset(iter);
    int* current = (int*)CN_queue_iterator_current(iter);
    if (current != NULL)
    {
        printf("    当前元素: %d\n", *current);
    }
    
    // 清理
    CN_queue_iterator_destroy(iter);
    CN_queue_destroy(queue);
    printf("  队列迭代器测试完成\n");
}

// 测试队列高级操作
void test_queue_advanced_operations(void)
{
    printf("  创建两个队列进行高级操作测试...\n");
    
    // 创建第一个队列
    Stru_CN_Queue_t* queue1 = CN_queue_create(Eum_QUEUE_IMPLEMENTATION_ARRAY, 
                                             sizeof(int), 
                                             10, 
                                             Eum_QUEUE_THREAD_UNSAFE);
    
    // 创建第二个队列
    Stru_CN_Queue_t* queue2 = CN_queue_create(Eum_QUEUE_IMPLEMENTATION_ARRAY, 
                                             sizeof(int), 
                                             10, 
                                             Eum_QUEUE_THREAD_UNSAFE);
    
    if (queue1 == NULL || queue2 == NULL)
    {
        printf("  错误: 无法创建队列\n");
        return;
    }
    
    // 填充数据
    for (int i = 1; i <= 5; i++)
    {
        CN_queue_enqueue(queue1, &i);
        CN_queue_enqueue(queue2, &i);
    }
    
    printf("  队列1大小: %zu, 队列2大小: %zu\n", 
           CN_queue_size(queue1), CN_queue_size(queue2));
    
    // 测试队列相等
    printf("  测试队列相等比较...\n");
    if (CN_queue_equal(queue1, queue2))
    {
        printf("    队列1和队列2相等\n");
    }
    
    // 测试队列复制
    printf("  测试队列复制...\n");
    Stru_CN_Queue_t* copy = CN_queue_copy(queue1);
    if (copy != NULL)
    {
        printf("    队列复制成功，副本大小: %zu\n", CN_queue_size(copy));
        
        if (CN_queue_equal(queue1, copy))
        {
            printf("    队列1和副本相等\n");
        }
        
        CN_queue_destroy(copy);
    }
    
    // 测试队列反转
    printf("  测试队列反转...\n");
    printf("    原始队列1: ");
    for (size_t i = 0; i < CN_queue_size(queue1); i++)
    {
        int* value = (int*)CN_queue_get(queue1, i);
        if (value != NULL)
        {
            printf("%d ", *value);
        }
    }
    printf("\n");
    
    if (CN_queue_reverse(queue1))
    {
        printf("    队列反转成功\n");
        printf("    反转后队列1: ");
        for (size_t i = 0; i < CN_queue_size(queue1); i++)
        {
            int* value = (int*)CN_queue_get(queue1, i);
            if (value != NULL)
            {
                printf("%d ", *value);
            }
        }
        printf("\n");
    }
    
    // 测试容量管理
    printf("  测试容量管理...\n");
    printf("    当前容量: %zu\n", CN_queue_capacity(queue1));
    
    if (CN_queue_ensure_capacity(queue1, 20))
    {
        printf("    确保容量20成功，新容量: %zu\n", CN_queue_capacity(queue1));
    }
    
    // 出队一些元素
    int temp;
    for (int i = 0; i < 3; i++)
    {
        CN_queue_dequeue(queue1, &temp);
    }
    
    printf("    出队3个元素后大小: %zu, 容量: %zu\n", 
           CN_queue_size(queue1), CN_queue_capacity(queue1));
    
    if (CN_queue_shrink_to_fit(queue1))
    {
        printf("    缩小容量以匹配大小成功，新容量: %zu\n", CN_queue_capacity(queue1));
    }
    
    // 测试队列转储
    printf("  测试队列转储...\n");
    CN_queue_dump(queue1);
    
    // 测试队列转数组
    printf("  测试队列转数组...\n");
    int array[10];
    size_t copied = CN_queue_to_array(queue1, array, 10);
    if (copied > 0)
    {
        printf("    队列转数组成功，复制了%zu个元素\n", copied);
        printf("    数组内容: ");
        for (size_t i = 0; i < copied; i++)
        {
            printf("%d ", array[i]);
        }
        printf("\n");
    }
    
    // 测试批量操作
    printf("  测试批量操作...\n");
    int batch_data[] = {10, 20, 30, 40, 50};
    size_t enqueued = CN_queue_enqueue_batch(queue1, batch_data, 5);
    printf("    批量入队了%zu个元素\n", enqueued);
    
    int dequeue_array[10];
    size_t dequeued = CN_queue_dequeue_batch(queue1, dequeue_array, 3);
    printf("    批量出队了%zu个元素\n", dequeued);
    
    // 清理
    CN_queue_destroy(queue1);
    CN_queue_destroy(queue2);
    printf("  队列高级操作测试完成\n");
}

// 测试边界情况
void test_queue_edge_cases(void)
{
    printf("  测试边界情况...\n");
    
    // 测试空队列
    printf("  测试空队列操作...\n");
    Stru_CN_Queue_t* empty_queue = CN_queue_create(Eum_QUEUE_IMPLEMENTATION_ARRAY, 
                                                  sizeof(int), 
                                                  5, 
                                                  Eum_QUEUE_THREAD_UNSAFE);
    
    if (empty_queue == NULL)
    {
        printf("  错误: 无法创建空队列\n");
        return;
    }
    
    printf("    空队列大小: %zu\n", CN_queue_size(empty_queue));
    printf("    空队列是否为空: %s\n", CN_queue_is_empty(empty_queue) ? "是" : "否");
    
    // 测试无效操作
    printf("  测试无效操作...\n");
    int value;
    if (!CN_queue_dequeue(empty_queue, &value))
    {
        printf("    从空队列出队元素失败（正确）\n");
    }
    
    int* front_result = CN_queue_front(empty_queue);
    if (front_result == NULL)
    {
        printf("    查看空队列队头返回NULL（正确）\n");
    }
    
    int* rear_result = CN_queue_rear(empty_queue);
    if (rear_result == NULL)
    {
        printf("    查看空队列队尾返回NULL（正确）\n");
    }
    
    // 测试零大小元素
    printf("  测试零大小元素队列...\n");
    Stru_CN_Queue_t* zero_size_queue = CN_queue_create(Eum_QUEUE_IMPLEMENTATION_ARRAY, 
                                                      0, 
                                                      5, 
                                                      Eum_QUEUE_THREAD_UNSAFE);
    if (zero_size_queue == NULL)
    {
        printf("    创建零大小元素队列失败（正确）\n");
    }
    
    // 测试NULL参数
    printf("  测试NULL参数处理...\n");
    Stru_CN_Queue_t* null_queue = NULL;
    
    printf("    CN_queue_size(NULL) = %zu\n", CN_queue_size(null_queue));
    printf("    CN_queue_is_empty(NULL) = %s\n", 
           CN_queue_is_empty(null_queue) ? "true" : "false");
    
    // 测试容量溢出
    printf("  测试容量溢出...\n");
    Stru_CN_Queue_t* small_queue = CN_queue_create(Eum_QUEUE_IMPLEMENTATION_ARRAY, 
                                                  sizeof(int), 
                                                  3, 
                                                  Eum_QUEUE_THREAD_UNSAFE);
    
    if (small_queue != NULL)
    {
        for (int i = 1; i <= 5; i++)
        {
            if (CN_queue_enqueue(small_queue, &i))
            {
                printf("    入队元素 %d 成功\n", i);
            }
            else
            {
                printf("    入队元素 %d 失败（可能容量不足）\n", i);
            }
        }
        
        CN_queue_destroy(small_queue);
    }
    
    // 测试大队列
    printf("  测试大队列操作...\n");
    Stru_CN_Queue_t* large_queue = CN_queue_create(Eum_QUEUE_IMPLEMENTATION_ARRAY, 
                                                  sizeof(int), 
                                                  10000, 
                                                  Eum_QUEUE_THREAD_UNSAFE);
    
    if (large_queue != NULL)
    {
        int count = 5000;
        for (int i = 0; i < count; i++)
        {
            CN_queue_enqueue(large_queue, &i);
        }
        
        printf("    创建了包含%d个元素的大队列\n", count);
        printf("    大队列大小: %zu, 容量: %zu\n", 
               CN_queue_size(large_queue), CN_queue_capacity(large_queue));
        
        // 测试随机访问
        size_t random_index = 2500;
        int* random_value = (int*)CN_queue_get(large_queue, random_index);
        if (random_value != NULL)
        {
            printf("    随机访问索引%zu的元素: %d\n", random_index, *random_value);
        }
        
        // 测试队列转储（大队列）
        printf("  测试大队列转储...\n");
        CN_queue_dump(large_queue);
        
        CN_queue_destroy(large_queue);
        printf("    大队列销毁成功\n");
    }
    
    // 测试循环队列边界情况
    printf("  测试循环队列边界情况...\n");
    Stru_CN_Queue_t* circular_edge_queue = CN_queue_create(Eum_QUEUE_IMPLEMENTATION_CIRCULAR, 
                                                          sizeof(int), 
                                                          5, 
                                                          Eum_QUEUE_THREAD_UNSAFE);
    
    if (circular_edge_queue != NULL)
    {
        // 填满队列
        for (int i = 1; i <= 5; i++)
        {
            CN_queue_enqueue(circular_edge_queue, &i);
        }
        
        printf("    循环队列填满后是否已满: %s\n", 
               CN_queue_is_full(circular_edge_queue) ? "是" : "否");
        
        // 出队一些元素
        int temp_val;
        CN_queue_dequeue(circular_edge_queue, &temp_val);
        CN_queue_dequeue(circular_edge_queue, &temp_val);
        
        printf("    出队2个元素后剩余空间: %zu\n", 
               CN_queue_remaining_space(circular_edge_queue));
        
        // 再入队一些元素（测试循环特性）
        for (int i = 6; i <= 7; i++)
        {
            CN_queue_enqueue(circular_edge_queue, &i);
        }
        
        printf("    循环操作后队列大小: %zu\n", CN_queue_size(circular_edge_queue));
        
        CN_queue_destroy(circular_edge_queue);
    }
    
    // 清理
    CN_queue_destroy(empty_queue);
    printf("  边界情况测试完成\n");
}
