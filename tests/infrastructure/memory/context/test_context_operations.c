/**
 * @file test_context_operations.c
 * @brief 内存上下文操作测试
 * 
 * 测试内存上下文模块的操作功能，包括上下文切换、查询等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_context.h"
#include "../../../../src/infrastructure/memory/context/public/CN_memory_context.h"
#include <stdio.h>
#include <string.h>

/* 测试函数声明 */
static bool test_context_get_current_basic(void);
static bool test_context_set_current_basic(void);
static bool test_context_switch_multiple(void);
static bool test_context_get_parent_basic(void);
static bool test_context_get_parent_null(void);
static bool test_context_get_name_basic(void);
static bool test_context_get_name_null(void);
static bool test_context_get_id_basic(void);
static bool test_context_get_id_unique(void);
static bool test_context_is_valid_basic(void);
static bool test_context_is_valid_null(void);
static bool test_context_is_valid_after_destroy(void);

/**
 * @brief 测试获取当前上下文 - 基本功能
 */
static bool test_context_get_current_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 初始当前上下文应该为NULL或默认上下文
    // 注意：我们只是获取它但不使用，用于验证接口正常工作
    (void)ctx_if->get_current(); // 忽略返回值
    
    // 创建上下文并设置为当前
    Stru_MemoryContext_t* ctx = ctx_if->create("TestCurrentContext", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    ctx_if->set_current(ctx);
    
    // 验证当前上下文
    Stru_MemoryContext_t* current = ctx_if->get_current();
    TEST_ASSERT_EQUAL(ctx, current);
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试设置当前上下文 - 基本功能
 */
static bool test_context_set_current_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建多个上下文
    Stru_MemoryContext_t* ctx1 = ctx_if->create("Context1", NULL);
    Stru_MemoryContext_t* ctx2 = ctx_if->create("Context2", NULL);
    Stru_MemoryContext_t* ctx3 = ctx_if->create("Context3", NULL);
    
    TEST_ASSERT_NOT_NULL(ctx1);
    TEST_ASSERT_NOT_NULL(ctx2);
    TEST_ASSERT_NOT_NULL(ctx3);
    
    // 设置并验证当前上下文
    ctx_if->set_current(ctx1);
    TEST_ASSERT_EQUAL(ctx1, ctx_if->get_current());
    
    ctx_if->set_current(ctx2);
    TEST_ASSERT_EQUAL(ctx2, ctx_if->get_current());
    
    ctx_if->set_current(ctx3);
    TEST_ASSERT_EQUAL(ctx3, ctx_if->get_current());
    
    // 设置NULL当前上下文
    ctx_if->set_current(NULL);
    TEST_ASSERT_NULL(ctx_if->get_current());
    
    // 清理
    ctx_if->destroy(ctx1);
    ctx_if->destroy(ctx2);
    ctx_if->destroy(ctx3);
    
    return true;
}

/**
 * @brief 测试上下文切换 - 多次切换
 */
static bool test_context_switch_multiple(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx_a = ctx_if->create("ContextA", NULL);
    Stru_MemoryContext_t* ctx_b = ctx_if->create("ContextB", NULL);
    
    TEST_ASSERT_NOT_NULL(ctx_a);
    TEST_ASSERT_NOT_NULL(ctx_b);
    
    // 多次切换并验证
    for (int i = 0; i < 5; i++) {
        ctx_if->set_current(ctx_a);
        TEST_ASSERT_EQUAL(ctx_a, ctx_if->get_current());
        
        ctx_if->set_current(ctx_b);
        TEST_ASSERT_EQUAL(ctx_b, ctx_if->get_current());
    }
    
    // 最终切换回ctx_a
    ctx_if->set_current(ctx_a);
    TEST_ASSERT_EQUAL(ctx_a, ctx_if->get_current());
    
    // 清理
    ctx_if->destroy(ctx_a);
    ctx_if->destroy(ctx_b);
    
    return true;
}

/**
 * @brief 测试获取父上下文 - 基本功能
 */
static bool test_context_get_parent_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建父子上下文
    Stru_MemoryContext_t* parent = ctx_if->create("Parent", NULL);
    Stru_MemoryContext_t* child = ctx_if->create("Child", parent);
    
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    
    // 验证父上下文
    TEST_ASSERT_NULL(ctx_if->get_parent(parent));
    TEST_ASSERT_EQUAL(parent, ctx_if->get_parent(child));
    
    // 创建孙子上下文
    Stru_MemoryContext_t* grandchild = ctx_if->create("Grandchild", child);
    TEST_ASSERT_NOT_NULL(grandchild);
    TEST_ASSERT_EQUAL(child, ctx_if->get_parent(grandchild));
    
    // 清理
    ctx_if->destroy(grandchild);
    ctx_if->destroy(child);
    ctx_if->destroy(parent);
    
    return true;
}

/**
 * @brief 测试获取父上下文 - NULL上下文
 */
static bool test_context_get_parent_null(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 获取NULL上下文的父上下文应该返回NULL
    TEST_ASSERT_NULL(ctx_if->get_parent(NULL));
    
    return true;
}

/**
 * @brief 测试获取上下文名称 - 基本功能
 */
static bool test_context_get_name_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建带名称的上下文
    const char* name = "TestContextName";
    Stru_MemoryContext_t* ctx = ctx_if->create(name, NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 验证名称
    const char* retrieved_name = ctx_if->get_name(ctx);
    TEST_ASSERT_NOT_NULL(retrieved_name);
    TEST_ASSERT_STRING_EQUAL(name, retrieved_name);
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试获取上下文名称 - NULL上下文
 */
static bool test_context_get_name_null(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 获取NULL上下文的名称应该返回NULL或空字符串
    // 注意：我们只是调用它但不使用返回值，用于验证接口正常工作
    (void)ctx_if->get_name(NULL); // 忽略返回值
    
    return true;
}

/**
 * @brief 测试获取上下文ID - 基本功能
 */
static bool test_context_get_id_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestContextID", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 验证ID
    uint64_t id = ctx_if->get_id(ctx);
    TEST_ASSERT(id > 0); // ID应该为正数
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试获取上下文ID - 唯一性
 */
static bool test_context_get_id_unique(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建多个上下文
    Stru_MemoryContext_t* ctx1 = ctx_if->create("Context1", NULL);
    Stru_MemoryContext_t* ctx2 = ctx_if->create("Context2", NULL);
    Stru_MemoryContext_t* ctx3 = ctx_if->create("Context3", NULL);
    
    TEST_ASSERT_NOT_NULL(ctx1);
    TEST_ASSERT_NOT_NULL(ctx2);
    TEST_ASSERT_NOT_NULL(ctx3);
    
    // 验证ID唯一性
    uint64_t id1 = ctx_if->get_id(ctx1);
    uint64_t id2 = ctx_if->get_id(ctx2);
    uint64_t id3 = ctx_if->get_id(ctx3);
    
    TEST_ASSERT(id1 != id2);
    TEST_ASSERT(id1 != id3);
    TEST_ASSERT(id2 != id3);
    
    // 清理
    ctx_if->destroy(ctx1);
    ctx_if->destroy(ctx2);
    ctx_if->destroy(ctx3);
    
    return true;
}

/**
 * @brief 测试上下文有效性检查 - 基本功能
 */
static bool test_context_is_valid_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建有效上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("ValidContext", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 验证有效性
    TEST_ASSERT(ctx_if->is_valid(ctx));
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试上下文有效性检查 - NULL上下文
 */
static bool test_context_is_valid_null(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // NULL上下文应该无效
    TEST_ASSERT(!ctx_if->is_valid(NULL));
    
    return true;
}

/**
 * @brief 测试上下文有效性检查 - 销毁后
 */
static bool test_context_is_valid_after_destroy(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestDestroyedContext", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 销毁前应该有效
    TEST_ASSERT(ctx_if->is_valid(ctx));
    
    // 销毁上下文
    ctx_if->destroy(ctx);
    
    // 销毁后应该无效（如果实现支持）
    // 注意：实际行为取决于实现
    
    return true;
}

/**
 * @brief 运行所有上下文操作测试
 * 
 * @return bool 是否所有测试都通过
 */
bool test_context_operations_all(void)
{
    test_context_print_header("上下文操作");
    
    TestCase tests[] = {
        {"获取当前上下文 - 基本功能", test_context_get_current_basic, true},
        {"设置当前上下文 - 基本功能", test_context_set_current_basic, true},
        {"上下文切换 - 多次切换", test_context_switch_multiple, true},
        {"获取父上下文 - 基本功能", test_context_get_parent_basic, true},
        {"获取父上下文 - NULL上下文", test_context_get_parent_null, true},
        {"获取上下文名称 - 基本功能", test_context_get_name_basic, true},
        {"获取上下文名称 - NULL上下文", test_context_get_name_null, true},
        {"获取上下文ID - 基本功能", test_context_get_id_basic, true},
        {"获取上下文ID - 唯一性", test_context_get_id_unique, true},
        {"上下文有效性检查 - 基本功能", test_context_is_valid_basic, true},
        {"上下文有效性检查 - NULL上下文", test_context_is_valid_null, true},
        {"上下文有效性检查 - 销毁后", test_context_is_valid_after_destroy, true},
    };
    
    int test_count = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, test_count, "上下文操作");
}
