/**
 * @file test_stack_core.c
 * @brief 栈核心模块测试
 * 
 * 测试栈核心模块的所有功能，包括创建、销毁、基本操作等。
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

/* 辅助函数声明 */
static bool compare_int(const void* a, const void* b);

/**
 * @brief 测试栈创建和销毁功能
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_create_destroy(void)
{
    TEST_START("栈创建和销毁");
    
    // 测试1: 创建栈
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "栈创建失败");
    TEST_ASSERT(stack->capacity == 16, "栈容量不正确"); // 默认初始容量是16
    TEST_ASSERT(stack->top == 0, "栈顶位置不正确");
    TEST_ASSERT(stack->item_size == sizeof(int), "元素大小不正确");
    TEST_PASS("栈创建成功");
    
    // 测试2: 销毁栈
    F_destroy_stack(stack);
    TEST_PASS("栈销毁成功");
    
    // 测试3: 创建零元素大小栈
    stack = F_create_stack(0);
    TEST_ASSERT(stack == NULL, "零元素大小应返回NULL");
    TEST_PASS("零元素大小测试通过");
    
    TEST_END("栈创建和销毁");
    return true;
}

/**
 * @brief 测试栈基本操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_basic_operations(void)
{
    TEST_START("栈基本操作");
    
    // 创建测试栈
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "栈创建失败");
    
    // 测试1: 压入元素
    int value1 = 42;
    bool result = F_stack_push(stack, &value1);
    TEST_ASSERT(result, "压入元素失败");
    TEST_ASSERT(stack->top == 1, "栈顶位置不正确");
    TEST_PASS("压入元素成功");
    
    // 测试2: 查看栈顶元素
    int* peeked = (int*)F_stack_peek(stack);
    TEST_ASSERT(peeked != NULL, "查看栈顶元素失败");
    TEST_ASSERT(*peeked == 42, "栈顶元素值不正确");
    TEST_PASS("查看栈顶元素成功");
    
    // 测试3: 弹出元素
    int popped;
    result = F_stack_pop(stack, &popped);
    TEST_ASSERT(result, "弹出元素失败");
    TEST_ASSERT(popped == 42, "弹出元素值不正确");
    TEST_ASSERT(stack->top == 0, "栈顶位置不正确");
    TEST_PASS("弹出元素成功");
    
    // 测试4: 压入多个元素
    for (int i = 0; i < 5; i++) {
        result = F_stack_push(stack, &i);
        TEST_ASSERT(result, "批量压入元素失败");
    }
    TEST_ASSERT(stack->top == 5, "栈顶位置不正确");
    TEST_PASS("批量压入元素成功");
    
    // 测试5: 栈大小查询
    size_t size = F_stack_size(stack);
    TEST_ASSERT(size == 5, "栈大小查询不正确");
    TEST_PASS("栈大小查询成功");
    
    // 测试6: 栈是否为空查询
    bool empty = F_stack_is_empty(stack);
    TEST_ASSERT(!empty, "栈空状态查询不正确");
    TEST_PASS("栈空状态查询成功");
    
    // 测试7: 清空栈
    F_stack_clear(stack);
    TEST_ASSERT(stack->top == 0, "清空栈失败");
    empty = F_stack_is_empty(stack);
    TEST_ASSERT(empty, "清空后栈空状态查询不正确");
    TEST_PASS("清空栈成功");
    
    // 清理
    F_destroy_stack(stack);
    
    TEST_END("栈基本操作");
    return true;
}

/**
 * @brief 测试栈边界情况
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_edge_cases(void)
{
    TEST_START("栈边界情况");
    
    // 测试1: 空栈操作
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "栈创建失败");
    
    // 空栈查看
    int* peeked = (int*)F_stack_peek(stack);
    TEST_ASSERT(peeked == NULL, "空栈查看应返回NULL");
    
    // 空栈弹出
    int popped;
    bool result = F_stack_pop(stack, &popped);
    TEST_ASSERT(!result, "空栈弹出应返回false");
    
    // 空栈大小
    size_t size = F_stack_size(stack);
    TEST_ASSERT(size == 0, "空栈大小应为0");
    
    // 空栈是否为空
    bool empty = F_stack_is_empty(stack);
    TEST_ASSERT(empty, "空栈应为空");
    TEST_PASS("空栈操作测试通过");
    
    // 测试2: 单个元素栈
    int value = 100;
    result = F_stack_push(stack, &value);
    TEST_ASSERT(result, "压入元素失败");
    
    peeked = (int*)F_stack_peek(stack);
    TEST_ASSERT(peeked != NULL && *peeked == 100, "查看栈顶元素失败");
    
    result = F_stack_pop(stack, &popped);
    TEST_ASSERT(result && popped == 100, "弹出元素失败");
    
    empty = F_stack_is_empty(stack);
    TEST_ASSERT(empty, "弹出后栈应为空");
    TEST_PASS("单元素栈测试通过");
    
    // 测试3: 满栈操作
    for (int i = 0; i < 5; i++) {
        F_stack_push(stack, &i);
    }
    
    size = F_stack_size(stack);
    TEST_ASSERT(size == 5, "满栈大小应为5");
    
    // 尝试压入超出容量
    int extra = 999;
    result = F_stack_push(stack, &extra);
    TEST_ASSERT(!result, "满栈压入应返回false");
    TEST_PASS("满栈操作测试通过");
    
    // 清理
    F_destroy_stack(stack);
    
    TEST_END("栈边界情况");
    return true;
}

/**
 * @brief 测试栈内存管理
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_memory_management(void)
{
    TEST_START("栈内存管理");
    
    // 测试1: 自动扩容
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "栈创建失败");
    
    // 压入超过初始容量的元素
    for (int i = 0; i < 10; i++) {
        bool result = F_stack_push(stack, &i);
        TEST_ASSERT(result, "自动扩容压入失败");
    }
    
    TEST_ASSERT(stack->capacity >= 10, "自动扩容失败");
    TEST_ASSERT(stack->top == 10, "栈顶位置不正确");
    TEST_PASS("自动扩容测试通过");
    
    // 测试2: 内存释放
    F_destroy_stack(stack);
    TEST_PASS("内存释放测试通过");
    
    // 测试3: 大容量栈
    stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "大容量栈创建失败");
    
    for (int i = 0; i < 1000; i++) {
        F_stack_push(stack, &i);
    }
    
    TEST_ASSERT(stack->top == 1000, "大容量栈操作失败");
    F_destroy_stack(stack);
    TEST_PASS("大容量栈测试通过");
    
    TEST_END("栈内存管理");
    return true;
}

/**
 * @brief 测试栈错误处理
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_error_handling(void)
{
    TEST_START("栈错误处理");
    
    // 测试1: 无效参数
    Stru_Stack_t* stack = F_create_stack(0);
    TEST_ASSERT(stack == NULL, "零元素大小应返回NULL");
    
    stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "有效参数应创建成功");
    
    // 测试2: 无效栈指针
    bool result = F_stack_push(NULL, NULL);
    TEST_ASSERT(!result, "NULL栈指针压入应返回false");
    
    int value = 42;
    result = F_stack_push(stack, NULL);
    TEST_ASSERT(!result, "NULL数据指针压入应返回false");
    
    // 测试3: 无效弹出
    result = F_stack_pop(NULL, NULL);
    TEST_ASSERT(!result, "NULL栈指针弹出应返回false");
    
    result = F_stack_pop(stack, NULL);
    TEST_ASSERT(!result, "NULL输出指针弹出应返回false");
    
    // 测试4: 无效查看
    void* peeked = F_stack_peek(NULL);
    TEST_ASSERT(peeked == NULL, "NULL栈指针查看应返回NULL");
    
    // 测试5: 无效大小查询
    size_t size = F_stack_size(NULL);
    TEST_ASSERT(size == 0, "NULL栈指针大小查询应返回0");
    
    // 测试6: 无效空状态查询
    bool empty = F_stack_is_empty(NULL);
    TEST_ASSERT(empty, "NULL栈指针空状态查询应返回true");
    
    // 测试7: 无效清空
    F_stack_clear(NULL); // 应该不会崩溃
    
    // 清理
    F_destroy_stack(stack);
    
    TEST_PASS("栈错误处理测试通过");
    TEST_END("栈错误处理");
    return true;
}

/**
 * @brief 测试栈性能特征
 * @return 测试通过返回true，失败返回false
 */
bool test_stack_performance(void)
{
    TEST_START("栈性能特征");
    
    // 创建大容量栈
    const size_t large_size = 10000;
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    TEST_ASSERT(stack != NULL, "大容量栈创建失败");
    
    // 测试1: 批量压入性能
    for (size_t i = 0; i < large_size; i++) {
        int value = (int)i;
        bool result = F_stack_push(stack, &value);
        TEST_ASSERT(result, "批量压入失败");
    }
    TEST_ASSERT(stack->top == large_size, "批量压入后栈大小不正确");
    TEST_PASS("批量压入性能测试通过");
    
    // 测试2: 批量弹出性能
    for (size_t i = 0; i < large_size; i++) {
        int popped;
        bool result = F_stack_pop(stack, &popped);
        TEST_ASSERT(result, "批量弹出失败");
        TEST_ASSERT(popped == (int)(large_size - i - 1), "弹出值不正确");
    }
    TEST_ASSERT(stack->top == 0, "批量弹出后栈大小不正确");
    TEST_PASS("批量弹出性能测试通过");
    
    // 测试3: 混合操作性能
    for (int i = 0; i < 1000; i++) {
        F_stack_push(stack, &i);
        if (i % 3 == 0) {
            int popped;
            F_stack_pop(stack, &popped);
        }
    }
    TEST_PASS("混合操作性能测试通过");
    
    // 清理
    F_destroy_stack(stack);
    
    TEST_END("栈性能特征");
    return true;
}

/**
 * @brief 运行所有栈核心模块测试
 * @return 所有测试通过返回true，否则返回false
 */
bool test_stack_core_all(void)
{
    printf("运行栈核心模块测试...\n");
    printf("=====================\n\n");
    
    bool all_passed = true;
    
    if (!test_stack_create_destroy()) {
        all_passed = false;
    }
    
    if (!test_stack_basic_operations()) {
        all_passed = false;
    }
    
    if (!test_stack_edge_cases()) {
        all_passed = false;
    }
    
    if (!test_stack_memory_management()) {
        all_passed = false;
    }
    
    if (!test_stack_error_handling()) {
        all_passed = false;
    }
    
    if (!test_stack_performance()) {
        all_passed = false;
    }
    
    printf("栈核心模块测试总结:\n");
    printf("====================\n");
    if (all_passed) {
        printf("✅ 所有核心模块测试通过！\n");
    } else {
        printf("❌ 有核心模块测试失败\n");
    }
    printf("\n");
    
    return all_passed;
}

/* 辅助函数实现 */
static bool compare_int(const void* a, const void* b)
{
    if (a == NULL || b == NULL) {
        return false;
    }
    return *(const int*)a == *(const int*)b;
}
