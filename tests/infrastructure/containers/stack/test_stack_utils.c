/**
 * @file test_stack_utils.c
 * @brief 栈工具模块测试
 * 
 * 测试栈工具模块的基本功能。
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
 * @brief 测试批量操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_utils_batch_operations(void)
{
    TEST_START("批量操作功能");
    
    // 创建测试栈
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "栈创建失败");
    
    // 测试1: 批量压入
    int batch_data[] = {1, 2, 3, 4, 5};
    bool result = F_stack_push_batch(stack, batch_data, 5);
    TEST_ASSERT(result, "批量压入失败");
    TEST_ASSERT(stack->top == 5, "批量压入后栈大小不正确");
    
    // 验证压入的数据
    for (int i = 4; i >= 0; i--) {
        int popped;
        result = F_stack_pop(stack, &popped);
        TEST_ASSERT(result, "弹出元素失败");
        TEST_ASSERT(popped == batch_data[i], "弹出元素值不正确");
    }
    TEST_PASS("批量压入测试通过");
    
    // 测试2: 批量弹出
    // 重新压入数据
    for (int i = 0; i < 5; i++) {
        F_stack_push(stack, &batch_data[i]);
    }
    
    int output[5];
    result = F_stack_pop_batch(stack, output, 3);
    TEST_ASSERT(result, "批量弹出失败");
    TEST_ASSERT(stack->top == 2, "批量弹出后栈大小不正确");
    
    // 验证弹出的数据
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT(output[i] == batch_data[4 - i], "批量弹出元素值不正确");
    }
    TEST_PASS("批量弹出测试通过");
    
    F_destroy_stack(stack);
    
    TEST_END("批量操作功能");
    return true;
}

/**
 * @brief 测试栈复制功能
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_utils_copy_functions(void)
{
    TEST_START("栈复制功能");
    
    // 创建源栈
    Stru_Stack_t* source = F_create_stack(sizeof(int));
    TEST_ASSERT(source != NULL, "源栈创建失败");
    
    // 压入测试数据
    for (int i = 0; i < 5; i++) {
        F_stack_push(source, &i);
    }
    
    // 测试1: 深拷贝
    Stru_Stack_t* deep_copy = F_stack_copy_deep(source);
    TEST_ASSERT(deep_copy != NULL, "深拷贝失败");
    TEST_ASSERT(deep_copy != source, "深拷贝应创建新对象");
    TEST_ASSERT(deep_copy->top == source->top, "深拷贝栈大小不正确");
    
    // 验证深拷贝数据
    for (int i = 4; i >= 0; i--) {
        int source_val, copy_val;
        F_stack_pop(source, &source_val);
        F_stack_pop(deep_copy, &copy_val);
        TEST_ASSERT(source_val == i, "源栈元素值不正确");
        TEST_ASSERT(copy_val == i, "深拷贝元素值不正确");
    }
    TEST_PASS("深拷贝测试通过");
    
    F_destroy_stack(deep_copy);
    
    // 重新压入数据到源栈
    for (int i = 0; i < 5; i++) {
        F_stack_push(source, &i);
    }
    
    // 测试2: 浅拷贝
    Stru_Stack_t* shallow_copy = F_stack_copy_shallow(source);
    TEST_ASSERT(shallow_copy != NULL, "浅拷贝失败");
    TEST_ASSERT(shallow_copy->items == source->items, "浅拷贝应共享数据");
    TEST_PASS("浅拷贝测试通过");
    
    F_destroy_stack(shallow_copy);
    F_destroy_stack(source);
    
    TEST_END("栈复制功能");
    return true;
}

/**
 * @brief 测试栈比较功能
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_utils_comparison(void)
{
    TEST_START("栈比较功能");
    
    // 创建两个相同的栈
    Stru_Stack_t* stack1 = F_create_stack(sizeof(int));
    Stru_Stack_t* stack2 = F_create_stack(sizeof(int));
    TEST_ASSERT(stack1 != NULL && stack2 != NULL, "栈创建失败");
    
    // 压入相同数据
    for (int i = 0; i < 5; i++) {
        F_stack_push(stack1, &i);
        F_stack_push(stack2, &i);
    }
    
    // 测试1: 相等比较
    bool equal = F_stack_equals(stack1, stack2, NULL);
    TEST_ASSERT(equal, "相同栈应相等");
    TEST_PASS("相等比较测试通过");
    
    // 修改stack2
    int extra = 99;
    F_stack_push(stack2, &extra);
    
    // 测试2: 不相等比较
    equal = F_stack_equals(stack1, stack2, NULL);
    TEST_ASSERT(!equal, "不同栈应不相等");
    TEST_PASS("不相等比较测试通过");
    
    F_destroy_stack(stack1);
    F_destroy_stack(stack2);
    
    TEST_END("栈比较功能");
    return true;
}

/**
 * @brief 测试栈查找功能
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_utils_find_functions(void)
{
    TEST_START("栈查找功能");
    
    // 创建测试栈
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "栈创建失败");
    
    // 压入测试数据
    int data[] = {10, 20, 30, 20, 10};
    for (int i = 0; i < 5; i++) {
        F_stack_push(stack, &data[i]);
    }
    
    // 测试1: 查找元素
    int target = 20;
    void* found = F_stack_find(stack, &target, NULL);
    TEST_ASSERT(found != NULL, "查找元素失败");
    TEST_ASSERT(*(int*)found == 20, "查找到的元素值不正确");
    TEST_PASS("查找元素测试通过");
    
    // 测试2: 查找不存在的元素
    target = 99;
    found = F_stack_find(stack, &target, NULL);
    TEST_ASSERT(found == NULL, "不存在的元素应返回NULL");
    TEST_PASS("查找不存在元素测试通过");
    
    F_destroy_stack(stack);
    
    TEST_END("栈查找功能");
    return true;
}

/**
 * @brief 测试栈操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_utils_stack_operations(void)
{
    TEST_START("栈操作功能");
    
    // 创建测试栈
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "栈创建失败");
    
    // 压入测试数据
    for (int i = 0; i < 5; i++) {
        F_stack_push(stack, &i);
    }
    
    // 测试1: 反转栈
    bool result = F_stack_reverse(stack);
    TEST_ASSERT(result, "栈反转失败");
    
    // 验证反转结果
    for (int i = 0; i < 5; i++) {
        int popped;
        F_stack_pop(stack, &popped);
        TEST_ASSERT(popped == i, "反转后元素顺序不正确");
    }
    TEST_PASS("栈反转测试通过");
    
    // 重新压入数据
    for (int i = 0; i < 5; i++) {
        F_stack_push(stack, &i);
    }
    
    // 测试2: 统计满足条件的元素数量
    size_t count = F_stack_count_if(stack, NULL);
    TEST_ASSERT(count == 5, "统计元素数量不正确");
    TEST_PASS("统计元素测试通过");
    
    F_destroy_stack(stack);
    
    TEST_END("栈操作功能");
    return true;
}

/**
 * @brief 测试栈转换功能
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_utils_conversion(void)
{
    TEST_START("栈转换功能");
    
    // 创建测试栈
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "栈创建失败");
    
    // 压入测试数据
    for (int i = 0; i < 5; i++) {
        F_stack_push(stack, &i);
    }
    
    // 测试1: 栈转换为数组
    size_t array_size;
    void* array = F_stack_to_array(stack, &array_size);
    TEST_ASSERT(array != NULL, "栈转数组失败");
    TEST_ASSERT(array_size == 5, "数组大小不正确");
    
    // 验证数组数据
    int* int_array = (int*)array;
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT(int_array[i] == i, "数组元素值不正确");
    }
    TEST_PASS("栈转数组测试通过");
    
    free(array);
    
    // 测试2: 从数组创建栈
    int source_array[] = {10, 20, 30, 40, 50};
    bool result = F_stack_from_array(stack, source_array, 5);
    TEST_ASSERT(result, "数组转栈失败");
    TEST_ASSERT(stack->top == 5, "栈大小不正确");
    
    // 验证栈数据
    for (int i = 4; i >= 0; i--) {
        int popped;
        F_stack_pop(stack, &popped);
        TEST_ASSERT(popped == source_array[i], "栈元素值不正确");
    }
    TEST_PASS("数组转栈测试通过");
    
    F_destroy_stack(stack);
    
    TEST_END("栈转换功能");
    return true;
}

/**
 * @brief 测试工具模块错误处理
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_utils_error_handling(void)
{
    TEST_START("工具模块错误处理");
    
    // 创建测试栈
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "栈创建失败");
    
    // 测试1: 无效参数
    bool result = F_stack_push_batch(NULL, NULL, 0);
    TEST_ASSERT(!result, "NULL栈指针批量压入应返回false");
    
    result = F_stack_pop_batch(NULL, NULL, 0);
    TEST_ASSERT(!result, "NULL栈指针批量弹出应返回false");
    
    Stru_Stack_t* copy = F_stack_copy_deep(NULL);
    TEST_ASSERT(copy == NULL, "NULL栈指针深拷贝应返回NULL");
    
    copy = F_stack_copy_shallow(NULL);
    TEST_ASSERT(copy == NULL, "NULL栈指针浅拷贝应返回NULL");
    
    bool equal = F_stack_equals(NULL, NULL, NULL);
    TEST_ASSERT(!equal, "NULL栈指针比较应返回false");
    
    void* found = F_stack_find(NULL, NULL, NULL);
    TEST_ASSERT(found == NULL, "NULL栈指针查找应返回NULL");
    
    result = F_stack_reverse(NULL);
    TEST_ASSERT(!result, "NULL栈指针反转应返回false");
    
    size_t count = F_stack_count_if(NULL, NULL);
    TEST_ASSERT(count == 0, "NULL栈指针统计应返回0");
    
    size_t array_size;
    void* array = F_stack_to_array(NULL, &array_size);
    TEST_ASSERT(array == NULL, "NULL栈指针转数组应返回NULL");
    
    result = F_stack_from_array(NULL, NULL, 0);
    TEST_ASSERT(!result, "NULL栈指针从数组创建应返回false");
    
    TEST_PASS("无效参数处理测试通过");
    
    F_destroy_stack(stack);
    
    TEST_END("工具模块错误处理");
    return true;
}

/**
 * @brief 运行所有栈工具模块测试
 * @return 所有测试通过返回true，否则返回false
 */
bool test_stack_utils_all(void)
{
    printf("运行栈工具模块测试...\n");
    printf("======================\n\n");
    
    bool all_passed = true;
    
    if (!test_stack_utils_batch_operations()) {
        all_passed = false;
    }
    
    if (!test_stack_utils_copy_functions()) {
        all_passed = false;
    }
    
    if (!test_stack_utils_comparison()) {
        all_passed = false;
    }
    
    if (!test_stack_utils_find_functions()) {
        all_passed = false;
    }
    
    if (!test_stack_utils_stack_operations()) {
        all_passed = false;
    }
    
    if (!test_stack_utils_conversion()) {
        all_passed = false;
    }
    
    if (!test_stack_utils_error_handling()) {
        all_passed = false;
    }
    
    printf("栈工具模块测试总结:\n");
    printf("======================\n");
    if (all_passed) {
        printf("✅ 所有工具模块测试通过！\n");
    } else {
        printf("❌ 有工具模块测试失败\n");
    }
    printf("\n");
    
    return all_passed;
}
