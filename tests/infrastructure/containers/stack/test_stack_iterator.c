/**
 * @file test_stack_iterator.c
 * @brief 栈迭代器模块测试
 * 
 * 测试栈迭代器模块的所有功能，包括创建、销毁、遍历等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_stack.h"
#include "../../../../src/infrastructure/containers/stack/CN_stack.h"

/**
 * @brief 测试迭代器创建和销毁功能
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_iterator_create_destroy(void)
{
    TEST_START("迭代器创建和销毁");
    
    // 创建测试栈
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "栈创建失败");
    
    // 压入测试数据
    for (int i = 0; i < 5; i++) {
        F_stack_push(stack, &i);
    }
    
    // 测试1: 创建迭代器（默认方向）
    Stru_StackIterator_t* iterator = F_create_stack_iterator(stack);
    TEST_ASSERT(iterator != NULL, "迭代器创建失败");
    TEST_ASSERT(iterator->stack == stack, "迭代器栈指针不正确");
    TEST_ASSERT(iterator->direction == Eum_STACK_TRAVERSAL_TOP_TO_BOTTOM, 
                "迭代器方向不正确");
    TEST_ASSERT(iterator->visited_count == 0, "已访问计数应为0");
    TEST_PASS("默认迭代器创建成功");
    
    // 销毁迭代器
    F_destroy_stack_iterator(iterator);
    TEST_PASS("迭代器销毁成功");
    
    // 测试2: 创建迭代器（栈底到栈顶）
    iterator = F_create_stack_iterator_with_direction(
        stack, 
        Eum_STACK_TRAVERSAL_BOTTOM_TO_TOP
    );
    TEST_ASSERT(iterator != NULL, "迭代器创建失败");
    TEST_ASSERT(iterator->direction == Eum_STACK_TRAVERSAL_BOTTOM_TO_TOP, 
                "迭代器方向不正确");
    TEST_PASS("栈底到栈顶迭代器创建成功");
    
    F_destroy_stack_iterator(iterator);
    
    // 测试3: 空栈迭代器
    Stru_Stack_t* empty_stack = F_create_stack(sizeof(int));
    TEST_ASSERT(empty_stack != NULL, "空栈创建失败");
    
    iterator = F_create_stack_iterator(empty_stack);
    TEST_ASSERT(iterator != NULL, "空栈迭代器创建失败");
    TEST_ASSERT(iterator->visited_count == 0, "已访问计数应为0");
    
    F_destroy_stack_iterator(iterator);
    F_destroy_stack(empty_stack);
    TEST_PASS("空栈迭代器测试通过");
    
    // 测试4: 无效参数
    iterator = F_create_stack_iterator(NULL);
    TEST_ASSERT(iterator == NULL, "NULL栈指针应返回NULL");
    
    // 清理
    F_destroy_stack(stack);
    
    TEST_END("迭代器创建和销毁");
    return true;
}

/**
 * @brief 测试迭代器遍历功能
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_iterator_traversal(void)
{
    TEST_START("迭代器遍历功能");
    
    // 创建测试栈
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "栈创建失败");
    
    // 压入测试数据 [0, 1, 2, 3, 4]（栈底到栈顶）
    for (int i = 0; i < 5; i++) {
        F_stack_push(stack, &i);
    }
    
    // 测试1: 栈顶到栈底遍历（LIFO顺序：4, 3, 2, 1, 0）
    Stru_StackIterator_t* iterator = F_create_stack_iterator_with_direction(
        stack, 
        Eum_STACK_TRAVERSAL_TOP_TO_BOTTOM
    );
    TEST_ASSERT(iterator != NULL, "迭代器创建失败");
    
    int expected_top_to_bottom[] = {4, 3, 2, 1, 0};
    int actual_values[5];
    int index = 0;
    
    while (F_stack_iterator_has_next(iterator)) {
        int value;
        bool result = F_stack_iterator_next(iterator, &value);
        TEST_ASSERT(result, "获取下一个元素失败");
        TEST_ASSERT(value == expected_top_to_bottom[index], 
                   "栈顶到栈底遍历顺序不正确");
        actual_values[index] = value;
        index++;
    }
    
    TEST_ASSERT(index == 5, "遍历元素数量不正确");
    TEST_PASS("栈顶到栈底遍历测试通过");
    
    // 检查已访问计数
    size_t visited = F_stack_iterator_visited_count(iterator);
    TEST_ASSERT(visited == 5, "已访问计数不正确");
    
    F_destroy_stack_iterator(iterator);
    
    // 测试2: 栈底到栈顶遍历（FIFO顺序：0, 1, 2, 3, 4）
    iterator = F_create_stack_iterator_with_direction(
        stack, 
        Eum_STACK_TRAVERSAL_BOTTOM_TO_TOP
    );
    TEST_ASSERT(iterator != NULL, "迭代器创建失败");
    
    int expected_bottom_to_top[] = {0, 1, 2, 3, 4};
    index = 0;
    
    while (F_stack_iterator_has_next(iterator)) {
        int value;
        bool result = F_stack_iterator_next(iterator, &value);
        TEST_ASSERT(result, "获取下一个元素失败");
        TEST_ASSERT(value == expected_bottom_to_top[index], 
                   "栈底到栈顶遍历顺序不正确");
        index++;
    }
    
    TEST_ASSERT(index == 5, "遍历元素数量不正确");
    TEST_PASS("栈底到栈顶遍历测试通过");
    
    F_destroy_stack_iterator(iterator);
    
    // 清理
    F_destroy_stack(stack);
    
    TEST_END("迭代器遍历功能");
    return true;
}

/**
 * @brief 测试迭代器方向功能
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_iterator_directions(void)
{
    TEST_START("迭代器方向功能");
    
    // 创建测试栈
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "栈创建失败");
    
    // 压入测试数据
    for (int i = 0; i < 3; i++) {
        F_stack_push(stack, &i);
    }
    
    // 创建迭代器
    Stru_StackIterator_t* iterator = F_create_stack_iterator(stack);
    TEST_ASSERT(iterator != NULL, "迭代器创建失败");
    
    // 测试1: 当前元素（初始应为NULL）
    void* current = F_stack_iterator_current(iterator);
    TEST_ASSERT(current == NULL, "初始时当前元素应为NULL");
    
    // 测试2: 遍历一个元素后检查当前元素
    int value;
    bool result = F_stack_iterator_next(iterator, &value);
    TEST_ASSERT(result, "获取下一个元素失败");
    
    current = F_stack_iterator_current(iterator);
    TEST_ASSERT(current != NULL, "遍历后当前元素不应为NULL");
    TEST_ASSERT(*(int*)current == value, "当前元素值不正确");
    
    // 测试3: 已访问计数
    size_t visited = F_stack_iterator_visited_count(iterator);
    TEST_ASSERT(visited == 1, "已访问计数不正确");
    
    // 测试4: 剩余元素数量
    size_t remaining = F_stack_iterator_remaining_count(iterator);
    TEST_ASSERT(remaining == 2, "剩余元素数量不正确");
    
    // 测试5: 遍历方向
    Eum_StackTraversalDirection direction = F_stack_iterator_get_direction(iterator);
    TEST_ASSERT(direction == Eum_STACK_TRAVERSAL_TOP_TO_BOTTOM, "遍历方向不正确");
    
    // 测试6: 设置遍历方向
    F_stack_iterator_set_direction(iterator, Eum_STACK_TRAVERSAL_BOTTOM_TO_TOP);
    direction = F_stack_iterator_get_direction(iterator);
    TEST_ASSERT(direction == Eum_STACK_TRAVERSAL_BOTTOM_TO_TOP, "设置遍历方向失败");
    
    // 测试7: 重置迭代器
    F_stack_iterator_reset(iterator);
    visited = F_stack_iterator_visited_count(iterator);
    TEST_ASSERT(visited == 0, "重置后已访问计数应为0");
    
    F_destroy_stack_iterator(iterator);
    F_destroy_stack(stack);
    
    TEST_PASS("迭代器方向功能测试通过");
    TEST_END("迭代器方向功能");
    return true;
}

/**
 * @brief 测试迭代器边界情况
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_iterator_edge_cases(void)
{
    TEST_START("迭代器边界情况");
    
    // 测试1: 空栈迭代器
    Stru_Stack_t* empty_stack = F_create_stack(sizeof(int));
    TEST_ASSERT(empty_stack != NULL, "空栈创建失败");
    
    Stru_StackIterator_t* iterator = F_create_stack_iterator(empty_stack);
    TEST_ASSERT(iterator != NULL, "空栈迭代器创建失败");
    
    TEST_ASSERT(!F_stack_iterator_has_next(iterator), "空栈应无下一个元素");
    
    int value;
    bool result = F_stack_iterator_next(iterator, &value);
    TEST_ASSERT(!result, "空栈获取下一个元素应返回false");
    
    void* current = F_stack_iterator_current(iterator);
    TEST_ASSERT(current == NULL, "空栈当前元素应为NULL");
    
    size_t visited = F_stack_iterator_visited_count(iterator);
    TEST_ASSERT(visited == 0, "空栈已访问计数应为0");
    
    size_t remaining = F_stack_iterator_remaining_count(iterator);
    TEST_ASSERT(remaining == 0, "空栈剩余元素数量应为0");
    
    F_destroy_stack_iterator(iterator);
    F_destroy_stack(empty_stack);
    TEST_PASS("空栈迭代器测试通过");
    
    // 测试2: 单元素栈迭代器
    Stru_Stack_t* single_stack = F_create_stack(sizeof(int));
    TEST_ASSERT(single_stack != NULL, "单元素栈创建失败");
    
    int single_value = 99;
    F_stack_push(single_stack, &single_value);
    
    iterator = F_create_stack_iterator(single_stack);
    TEST_ASSERT(iterator != NULL, "单元素栈迭代器创建失败");
    
    TEST_ASSERT(F_stack_iterator_has_next(iterator), "应有下一个元素");
    
    result = F_stack_iterator_next(iterator, &value);
    TEST_ASSERT(result, "获取元素失败");
    TEST_ASSERT(value == 99, "元素值不正确");
    
    TEST_ASSERT(!F_stack_iterator_has_next(iterator), "应无下一个元素");
    
    F_destroy_stack_iterator(iterator);
    F_destroy_stack(single_stack);
    TEST_PASS("单元素栈迭代器测试通过");
    
    TEST_END("迭代器边界情况");
    return true;
}

/**
 * @brief 测试迭代器错误处理
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_iterator_error_handling(void)
{
    TEST_START("迭代器错误处理");
    
    // 创建测试栈
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "栈创建失败");
    
    for (int i = 0; i < 3; i++) {
        F_stack_push(stack, &i);
    }
    
    // 测试1: 无效参数处理
    F_destroy_stack_iterator(NULL); // 应该不会崩溃
    
    Stru_StackIterator_t* iterator = F_create_stack_iterator(NULL);
    TEST_ASSERT(iterator == NULL, "NULL栈指针应返回NULL");
    
    // 测试2: 无效迭代器操作
    bool has_next = F_stack_iterator_has_next(NULL);
    TEST_ASSERT(!has_next, "NULL迭代器has_next应返回false");
    
    int value;
    bool result = F_stack_iterator_next(NULL, &value);
    TEST_ASSERT(!result, "NULL迭代器next应返回false");
    
    result = F_stack_iterator_next(NULL, NULL);
    TEST_ASSERT(!result, "NULL迭代器next应返回false");
    
    void* current = F_stack_iterator_current(NULL);
    TEST_ASSERT(current == NULL, "NULL迭代器current应返回NULL");
    
    F_stack_iterator_reset(NULL); // 应该不会崩溃
    
    size_t visited = F_stack_iterator_visited_count(NULL);
    TEST_ASSERT(visited == 0, "NULL迭代器已访问计数应为0");
    
    size_t remaining = F_stack_iterator_remaining_count(NULL);
    TEST_ASSERT(remaining == 0, "NULL迭代器剩余元素数量应为0");
    
    Eum_StackTraversalDirection direction = F_stack_iterator_get_direction(NULL);
    TEST_ASSERT(direction == Eum_STACK_TRAVERSAL_TOP_TO_BOTTOM, 
                "NULL迭代器方向应为默认值");
    
    F_stack_iterator_set_direction(NULL, Eum_STACK_TRAVERSAL_BOTTOM_TO_TOP); // 应该不会崩溃
    
    TEST_PASS("无效参数处理测试通过");
    
    // 测试3: 已销毁的栈上的迭代器
    iterator = F_create_stack_iterator(stack);
    TEST_ASSERT(iterator != NULL, "迭代器创建失败");
    
    // 销毁栈
    F_destroy_stack(stack);
    
    // 迭代器现在引用已销毁的栈，行为未定义
    // 我们只测试不会崩溃
    F_destroy_stack_iterator(iterator);
    TEST_PASS("已销毁栈的迭代器处理测试通过");
    
    TEST_END("迭代器错误处理");
    return true;
}

/**
 * @brief 运行所有栈迭代器模块测试
 * @return 所有测试通过返回true，否则返回false
 */
bool test_stack_iterator_all(void)
{
    printf("运行栈迭代器模块测试...\n");
    printf("=======================\n\n");
    
    bool all_passed = true;
    
    if (!test_stack_iterator_create_destroy()) {
        all_passed = false;
    }
    
    if (!test_stack_iterator_traversal()) {
        all_passed = false;
    }
    
    if (!test_stack_iterator_directions()) {
        all_passed = false;
    }
    
    if (!test_stack_iterator_edge_cases()) {
        all_passed = false;
    }
    
    if (!test_stack_iterator_error_handling()) {
        all_passed = false;
    }
    
    printf("栈迭代器模块测试总结:\n");
    printf("=======================\n");
    if (all_passed) {
        printf("✅ 所有迭代器模块测试通过！\n");
    } else {
        printf("❌ 有迭代器模块测试失败\n");
    }
    printf("\n");
    
    return all_passed;
}
