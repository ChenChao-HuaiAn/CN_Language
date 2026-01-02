/******************************************************************************
 * 文件名: test_stack.c
 * 功能: CN_stack模块测试程序
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，测试CN_stack模块功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "../../../src/infrastructure/containers/stack/CN_stack.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// 测试函数声明
void test_stack_basic_operations(void);
void test_stack_different_implementations(void);
void test_stack_thread_safety(void);
void test_stack_iterator(void);
void test_stack_advanced_operations(void);
void test_stack_edge_cases(void);

int main(void)
{
    printf("=== CN_stack 模块测试 ===\n\n");
    
    printf("1. 测试栈基本操作:\n");
    test_stack_basic_operations();
    
    printf("\n2. 测试不同实现类型的栈:\n");
    test_stack_different_implementations();
    
    printf("\n3. 测试栈线程安全操作:\n");
    test_stack_thread_safety();
    
    printf("\n4. 测试栈迭代器:\n");
    test_stack_iterator();
    
    printf("\n5. 测试栈高级操作:\n");
    test_stack_advanced_operations();
    
    printf("\n6. 测试边界情况:\n");
    test_stack_edge_cases();
    
    printf("\n=== 所有测试完成 ===\n");
    return 0;
}

// 测试栈基本操作
void test_stack_basic_operations(void)
{
    printf("  创建数组实现的栈...\n");
    Stru_CN_Stack_t* stack = CN_stack_create(Eum_STACK_IMPLEMENTATION_ARRAY, 
                                            sizeof(int), 
                                            10, 
                                            Eum_STACK_THREAD_UNSAFE);
    
    if (stack == NULL)
    {
        printf("  错误: 无法创建栈\n");
        return;
    }
    
    printf("  栈创建成功\n");
    printf("  初始大小: %zu\n", CN_stack_size(stack));
    printf("  栈是否为空: %s\n", CN_stack_is_empty(stack) ? "是" : "否");
    
    // 测试压入元素
    printf("  测试压入元素...\n");
    for (int i = 1; i <= 5; i++)
    {
        if (CN_stack_push(stack, &i))
        {
            printf("    压入元素 %d 成功\n", i);
        }
        else
        {
            printf("    压入元素 %d 失败\n", i);
        }
    }
    
    printf("  栈大小: %zu\n", CN_stack_size(stack));
    printf("  栈是否为空: %s\n", CN_stack_is_empty(stack) ? "是" : "否");
    
    // 测试查看栈顶元素
    printf("  测试查看栈顶元素...\n");
    int* top = (int*)CN_stack_peek(stack);
    if (top != NULL)
    {
        printf("    栈顶元素: %d\n", *top);
    }
    
    // 测试弹出元素
    printf("  测试弹出元素...\n");
    int popped_value;
    while (!CN_stack_is_empty(stack))
    {
        if (CN_stack_pop(stack, &popped_value))
        {
            printf("    弹出元素: %d\n", popped_value);
        }
    }
    
    printf("  弹出所有元素后栈大小: %zu\n", CN_stack_size(stack));
    printf("  栈是否为空: %s\n", CN_stack_is_empty(stack) ? "是" : "否");
    
    // 清理
    CN_stack_destroy(stack);
    printf("  栈销毁成功\n");
}

// 测试不同实现类型的栈
void test_stack_different_implementations(void)
{
    printf("  测试数组实现的栈...\n");
    Stru_CN_Stack_t* array_stack = CN_stack_create(Eum_STACK_IMPLEMENTATION_ARRAY, 
                                                  sizeof(double), 
                                                  5, 
                                                  Eum_STACK_THREAD_UNSAFE);
    
    if (array_stack != NULL)
    {
        for (double i = 1.0; i <= 3.0; i += 1.0)
        {
            CN_stack_push(array_stack, &i);
        }
        printf("    数组栈大小: %zu, 容量: %zu\n", 
               CN_stack_size(array_stack), CN_stack_capacity(array_stack));
        CN_stack_destroy(array_stack);
    }
    
    printf("  测试链表实现的栈...\n");
    Stru_CN_Stack_t* list_stack = CN_stack_create(Eum_STACK_IMPLEMENTATION_LIST, 
                                                 sizeof(int), 
                                                 0, // 容量对链表实现无效
                                                 Eum_STACK_THREAD_UNSAFE);
    
    if (list_stack != NULL)
    {
        for (int i = 1; i <= 10; i++)
        {
            CN_stack_push(list_stack, &i);
        }
        printf("    链表栈大小: %zu\n", CN_stack_size(list_stack));
        CN_stack_destroy(list_stack);
    }
    
    printf("  测试循环数组实现的栈...\n");
    Stru_CN_Stack_t* circular_stack = CN_stack_create(Eum_STACK_IMPLEMENTATION_CIRCULAR, 
                                                     sizeof(int), 
                                                     8, 
                                                     Eum_STACK_THREAD_UNSAFE);
    
    if (circular_stack != NULL)
    {
        for (int i = 1; i <= 6; i++)
        {
            CN_stack_push(circular_stack, &i);
        }
        printf("    循环数组栈大小: %zu, 容量: %zu\n", 
               CN_stack_size(circular_stack), CN_stack_capacity(circular_stack));
        CN_stack_destroy(circular_stack);
    }
    
    printf("  不同实现类型的栈测试完成\n");
}

// 测试栈线程安全操作
void test_stack_thread_safety(void)
{
    printf("  创建线程安全栈...\n");
    Stru_CN_Stack_t* thread_safe_stack = CN_stack_create(Eum_STACK_IMPLEMENTATION_ARRAY, 
                                                        sizeof(int), 
                                                        20, 
                                                        Eum_STACK_THREAD_SAFE);
    
    if (thread_safe_stack == NULL)
    {
        printf("  错误: 无法创建线程安全栈\n");
        return;
    }
    
    printf("  线程安全栈创建成功\n");
    
    // 测试线程安全操作
    printf("  测试线程安全压入...\n");
    for (int i = 1; i <= 5; i++)
    {
        if (CN_stack_push_safe(thread_safe_stack, &i))
        {
            printf("    线程安全压入元素 %d 成功\n", i);
        }
    }
    
    printf("  测试线程安全查看栈顶...\n");
    int* safe_top = (int*)CN_stack_peek_safe(thread_safe_stack);
    if (safe_top != NULL)
    {
        printf("    线程安全查看栈顶元素: %d\n", *safe_top);
    }
    
    printf("  测试手动锁操作...\n");
    if (CN_stack_lock(thread_safe_stack))
    {
        printf("    成功获取栈锁\n");
        
        // 执行需要同步的操作
        int value = 99;
        CN_stack_push(thread_safe_stack, &value);
        
        CN_stack_unlock(thread_safe_stack);
        printf("    已释放栈锁\n");
    }
    
    printf("  测试线程安全弹出...\n");
    int safe_popped;
    while (CN_stack_pop_safe(thread_safe_stack, &safe_popped))
    {
        printf("    线程安全弹出元素: %d\n", safe_popped);
    }
    
    CN_stack_destroy(thread_safe_stack);
    printf("  线程安全栈测试完成\n");
}

// 测试栈迭代器
void test_stack_iterator(void)
{
    printf("  创建栈并测试迭代器...\n");
    Stru_CN_Stack_t* stack = CN_stack_create(Eum_STACK_IMPLEMENTATION_ARRAY, 
                                            sizeof(int), 
                                            10, 
                                            Eum_STACK_THREAD_UNSAFE);
    
    if (stack == NULL)
    {
        printf("  错误: 无法创建栈\n");
        return;
    }
    
    // 填充数据
    for (int i = 1; i <= 5; i++)
    {
        CN_stack_push(stack, &i);
    }
    
    printf("  创建栈迭代器...\n");
    Stru_CN_StackIterator_t* iter = CN_stack_iterator_create(stack);
    if (iter == NULL)
    {
        printf("  错误: 无法创建迭代器\n");
        CN_stack_destroy(stack);
        return;
    }
    
    printf("  使用迭代器遍历栈（从栈顶到底）:\n");
    printf("    遍历结果: ");
    while (CN_stack_iterator_has_next(iter))
    {
        int* value = (int*)CN_stack_iterator_next(iter);
        if (value != NULL)
        {
            printf("%d ", *value);
        }
    }
    printf("\n");
    
    printf("  测试重置迭代器...\n");
    CN_stack_iterator_reset(iter);
    
    printf("  重置后遍历: ");
    while (CN_stack_iterator_has_next(iter))
    {
        int* value = (int*)CN_stack_iterator_next(iter);
        if (value != NULL)
        {
            printf("%d ", *value);
        }
    }
    printf("\n");
    
    printf("  测试获取当前元素...\n");
    CN_stack_iterator_reset(iter);
    int* current = (int*)CN_stack_iterator_current(iter);
    if (current != NULL)
    {
        printf("    当前元素: %d\n", *current);
    }
    
    // 清理
    CN_stack_iterator_destroy(iter);
    CN_stack_destroy(stack);
    printf("  栈迭代器测试完成\n");
}

// 测试栈高级操作
void test_stack_advanced_operations(void)
{
    printf("  创建两个栈进行高级操作测试...\n");
    
    // 创建第一个栈
    Stru_CN_Stack_t* stack1 = CN_stack_create(Eum_STACK_IMPLEMENTATION_ARRAY, 
                                             sizeof(int), 
                                             10, 
                                             Eum_STACK_THREAD_UNSAFE);
    
    // 创建第二个栈
    Stru_CN_Stack_t* stack2 = CN_stack_create(Eum_STACK_IMPLEMENTATION_ARRAY, 
                                             sizeof(int), 
                                             10, 
                                             Eum_STACK_THREAD_UNSAFE);
    
    if (stack1 == NULL || stack2 == NULL)
    {
        printf("  错误: 无法创建栈\n");
        return;
    }
    
    // 填充数据
    for (int i = 1; i <= 5; i++)
    {
        CN_stack_push(stack1, &i);
        CN_stack_push(stack2, &i);
    }
    
    printf("  栈1大小: %zu, 栈2大小: %zu\n", 
           CN_stack_size(stack1), CN_stack_size(stack2));
    
    // 测试栈相等
    printf("  测试栈相等比较...\n");
    if (CN_stack_equal(stack1, stack2))
    {
        printf("    栈1和栈2相等\n");
    }
    
    // 测试栈复制
    printf("  测试栈复制...\n");
    Stru_CN_Stack_t* copy = CN_stack_copy(stack1);
    if (copy != NULL)
    {
        printf("    栈复制成功，副本大小: %zu\n", CN_stack_size(copy));
        
        if (CN_stack_equal(stack1, copy))
        {
            printf("    栈1和副本相等\n");
        }
        
        CN_stack_destroy(copy);
    }
    
    // 测试栈反转
    printf("  测试栈反转...\n");
    printf("    原始栈1: ");
    for (size_t i = 0; i < CN_stack_size(stack1); i++)
    {
        int* value = (int*)CN_stack_peek_at(stack1, i);
        if (value != NULL)
        {
            printf("%d ", *value);
        }
    }
    printf("\n");
    
    if (CN_stack_reverse(stack1))
    {
        printf("    栈反转成功\n");
        printf("    反转后栈1: ");
        for (size_t i = 0; i < CN_stack_size(stack1); i++)
        {
            int* value = (int*)CN_stack_peek_at(stack1, i);
            if (value != NULL)
            {
                printf("%d ", *value);
            }
        }
        printf("\n");
    }
    
    // 测试容量管理
    printf("  测试容量管理...\n");
    printf("    当前容量: %zu\n", CN_stack_capacity(stack1));
    
    if (CN_stack_ensure_capacity(stack1, 20))
    {
        printf("    确保容量20成功，新容量: %zu\n", CN_stack_capacity(stack1));
    }
    
    // 弹出一些元素
    int temp;
    for (int i = 0; i < 3; i++)
    {
        CN_stack_pop(stack1, &temp);
    }
    
    printf("    弹出3个元素后大小: %zu, 容量: %zu\n", 
           CN_stack_size(stack1), CN_stack_capacity(stack1));
    
    if (CN_stack_shrink_to_fit(stack1))
    {
        printf("    缩小容量以匹配大小成功，新容量: %zu\n", CN_stack_capacity(stack1));
    }
    
    // 测试栈转储
    printf("  测试栈转储...\n");
    char* dump_str = CN_stack_dump(stack1);
    if (dump_str != NULL)
    {
        printf("    栈转储信息:\n%s\n", dump_str);
        cn_free(dump_str);
    }
    
    // 测试栈转数组
    printf("  测试栈转数组...\n");
    void* array = CN_stack_to_array(stack1);
    if (array != NULL)
    {
        printf("    栈转数组成功，数组内容: ");
        for (size_t i = 0; i < CN_stack_size(stack1); i++)
        {
            int* value = (int*)((char*)array + i * sizeof(int));
            printf("%d ", *value);
        }
        printf("\n");
        cn_free(array);
    }
    
    // 清理
    CN_stack_destroy(stack1);
    CN_stack_destroy(stack2);
    printf("  栈高级操作测试完成\n");
}

// 测试边界情况
void test_stack_edge_cases(void)
{
    printf("  测试边界情况...\n");
    
    // 测试空栈
    printf("  测试空栈操作...\n");
    Stru_CN_Stack_t* empty_stack = CN_stack_create(Eum_STACK_IMPLEMENTATION_ARRAY, 
                                                  sizeof(int), 
                                                  5, 
                                                  Eum_STACK_THREAD_UNSAFE);
    
    if (empty_stack == NULL)
    {
        printf("  错误: 无法创建空栈\n");
        return;
    }
    
    printf("    空栈大小: %zu\n", CN_stack_size(empty_stack));
    printf("    空栈是否为空: %s\n", CN_stack_is_empty(empty_stack) ? "是" : "否");
    
    // 测试无效操作
    printf("  测试无效操作...\n");
    int value;
    if (!CN_stack_pop(empty_stack, &value))
    {
        printf("    从空栈弹出元素失败（正确）\n");
    }
    
    int* peek_result = CN_stack_peek(empty_stack);
    if (peek_result == NULL)
    {
        printf("    查看空栈栈顶返回NULL（正确）\n");
    }
    
    // 测试零大小元素
    printf("  测试零大小元素栈...\n");
    Stru_CN_Stack_t* zero_size_stack = CN_stack_create(Eum_STACK_IMPLEMENTATION_ARRAY, 
                                                      0, 
                                                      5, 
                                                      Eum_STACK_THREAD_UNSAFE);
    if (zero_size_stack == NULL)
    {
        printf("    创建零大小元素栈失败（正确）\n");
    }
    
    // 测试NULL参数
    printf("  测试NULL参数处理...\n");
    Stru_CN_Stack_t* null_stack = NULL;
    
    printf("    CN_stack_size(NULL) = %zu\n", CN_stack_size(null_stack));
    printf("    CN_stack_is_empty(NULL) = %s\n", 
           CN_stack_is_empty(null_stack) ? "true" : "false");
    
    // 测试容量溢出
    printf("  测试容量溢出...\n");
    Stru_CN_Stack_t* small_stack = CN_stack_create(Eum_STACK_IMPLEMENTATION_ARRAY, 
                                                  sizeof(int), 
                                                  3, 
                                                  Eum_STACK_THREAD_UNSAFE);
    
    if (small_stack != NULL)
    {
        for (int i = 1; i <= 5; i++)
        {
            if (CN_stack_push(small_stack, &i))
            {
                printf("    压入元素 %d 成功\n", i);
            }
            else
            {
                printf("    压入元素 %d 失败（可能容量不足）\n", i);
            }
        }
        
        CN_stack_destroy(small_stack);
    }
    
    // 测试大栈
    printf("  测试大栈操作...\n");
    Stru_CN_Stack_t* large_stack = CN_stack_create(Eum_STACK_IMPLEMENTATION_ARRAY, 
                                                  sizeof(int), 
                                                  10000, 
                                                  Eum_STACK_THREAD_UNSAFE);
    
    if (large_stack != NULL)
    {
        int count = 5000;
        for (int i = 0; i < count; i++)
        {
            CN_stack_push(large_stack, &i);
        }
        
        printf("    创建了包含%d个元素的大栈\n", count);
        printf("    大栈大小: %zu, 容量: %zu\n", 
               CN_stack_size(large_stack), CN_stack_capacity(large_stack));
        
        // 测试随机访问
        size_t random_index = 2500;
        int* random_value = (int*)CN_stack_peek_at(large_stack, random_index);
        if (random_value != NULL)
        {
            printf("    随机访问索引%zu的元素: %d\n", random_index, *random_value);
        }
        
        // 测试栈转储（大栈）
        printf("  测试大栈转储（仅基本信息）...\n");
        char* large_dump = CN_stack_dump(large_stack);
        if (large_dump != NULL)
        {
            // 只打印前200个字符，避免输出太多
            printf("    大栈转储信息（前200字符）:\n%.200s...\n", large_dump);
            cn_free(large_dump);
        }
        
        CN_stack_destroy(large_stack);
        printf("    大栈销毁成功\n");
    }
    
    // 清理
    CN_stack_destroy(empty_stack);
    printf("  边界情况测试完成\n");
}
