/**
 * @file test_context_management.c
 * @brief 内存上下文管理测试
 * 
 * 测试内存上下文模块的创建、销毁、重置等管理功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_context.h"
#include "../../../../src/infrastructure/memory/context/CN_memory_context.h"
#include <stdio.h>
#include <string.h>

/* 测试函数声明 */
static bool test_context_create_basic(void);
static bool test_context_create_with_parent(void);
static bool test_context_create_invalid_name(void);
static bool test_context_destroy_basic(void);
static bool test_context_destroy_null(void);
static bool test_context_destroy_with_allocations(void);
static bool test_context_reset_basic(void);
static bool test_context_reset_null(void);
static bool test_context_reset_with_allocations(void);
static bool test_context_lifecycle(void);

/**
 * @brief 测试上下文创建 - 基本功能
 */
static bool test_context_create_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建根上下文
    Stru_MemoryContext_t* root_ctx = ctx_if->create("TestRootContext", NULL);
    TEST_ASSERT_NOT_NULL(root_ctx);
    
    // 验证上下文属性
    TEST_ASSERT_NOT_NULL(ctx_if->get_name(root_ctx));
    TEST_ASSERT_STRING_EQUAL("TestRootContext", ctx_if->get_name(root_ctx));
    TEST_ASSERT(ctx_if->get_id(root_ctx) > 0);
    TEST_ASSERT(ctx_if->is_valid(root_ctx));
    
    // 清理
    ctx_if->destroy(root_ctx);
    
    return true;
}

/**
 * @brief 测试上下文创建 - 带父上下文
 */
static bool test_context_create_with_parent(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建父上下文
    Stru_MemoryContext_t* parent_ctx = ctx_if->create("ParentContext", NULL);
    TEST_ASSERT_NOT_NULL(parent_ctx);
    
    // 创建子上下文
    Stru_MemoryContext_t* child_ctx = ctx_if->create("ChildContext", parent_ctx);
    TEST_ASSERT_NOT_NULL(child_ctx);
    
    // 验证父子关系
    Stru_MemoryContext_t* actual_parent = ctx_if->get_parent(child_ctx);
    TEST_ASSERT_EQUAL(parent_ctx, actual_parent);
    
    // 验证子上下文属性
    TEST_ASSERT_STRING_EQUAL("ChildContext", ctx_if->get_name(child_ctx));
    TEST_ASSERT(ctx_if->is_valid(child_ctx));
    
    // 清理
    ctx_if->destroy(child_ctx);
    ctx_if->destroy(parent_ctx);
    
    return true;
}

/**
 * @brief 测试上下文创建 - 无效名称
 */
static bool test_context_create_invalid_name(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 测试空名称
    Stru_MemoryContext_t* ctx1 = ctx_if->create(NULL, NULL);
    TEST_ASSERT_NOT_NULL(ctx1); // 应该允许空名称
    
    // 测试空字符串名称
    Stru_MemoryContext_t* ctx2 = ctx_if->create("", NULL);
    TEST_ASSERT_NOT_NULL(ctx2); // 应该允许空字符串
    
    // 清理
    ctx_if->destroy(ctx1);
    ctx_if->destroy(ctx2);
    
    return true;
}

/**
 * @brief 测试上下文销毁 - 基本功能
 */
static bool test_context_destroy_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestDestroyContext", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT(ctx_if->is_valid(ctx));
    
    // 销毁上下文
    ctx_if->destroy(ctx);
    
    // 注意：销毁后不应再访问上下文
    // 实际实现中可能有标记无效的机制
    
    return true;
}

/**
 * @brief 测试上下文销毁 - NULL上下文
 */
static bool test_context_destroy_null(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 销毁NULL上下文应该安全
    ctx_if->destroy(NULL);
    
    return true;
}

/**
 * @brief 测试上下文销毁 - 带分配的内存
 */
static bool test_context_destroy_with_allocations(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestDestroyWithAlloc", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 分配一些内存
    void* mem1 = ctx_if->allocate(ctx, 100, 8);
    void* mem2 = ctx_if->allocate(ctx, 200, 16);
    void* mem3 = ctx_if->allocate(ctx, 300, 32);
    
    TEST_ASSERT_NOT_NULL(mem1);
    TEST_ASSERT_NOT_NULL(mem2);
    TEST_ASSERT_NOT_NULL(mem3);
    
    // 销毁上下文（应该自动释放所有内存）
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试上下文重置 - 基本功能
 */
static bool test_context_reset_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestResetContext", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 重置空上下文
    ctx_if->reset(ctx);
    
    // 上下文应该仍然有效
    TEST_ASSERT(ctx_if->is_valid(ctx));
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试上下文重置 - NULL上下文
 */
static bool test_context_reset_null(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 重置NULL上下文应该安全
    ctx_if->reset(NULL);
    
    return true;
}

/**
 * @brief 测试上下文重置 - 带分配的内存
 */
static bool test_context_reset_with_allocations(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestResetWithAlloc", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 分配一些内存
    void* mem1 = ctx_if->allocate(ctx, 100, 8);
    void* mem2 = ctx_if->allocate(ctx, 200, 16);
    
    TEST_ASSERT_NOT_NULL(mem1);
    TEST_ASSERT_NOT_NULL(mem2);
    
    // 获取重置前的统计信息
    size_t before_total = ctx_if->get_total_allocated(ctx);
    TEST_ASSERT(before_total >= 300); // 至少100+200字节
    
    // 重置上下文
    ctx_if->reset(ctx);
    
    // 获取重置后的统计信息
    size_t after_total = ctx_if->get_total_allocated(ctx);
    TEST_ASSERT_EQUAL(0, after_total); // 重置后应该为0
    
    // 上下文应该仍然有效
    TEST_ASSERT(ctx_if->is_valid(ctx));
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试上下文完整生命周期
 */
static bool test_context_lifecycle(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文层次结构
    Stru_MemoryContext_t* grandparent = ctx_if->create("Grandparent", NULL);
    Stru_MemoryContext_t* parent = ctx_if->create("Parent", grandparent);
    Stru_MemoryContext_t* child = ctx_if->create("Child", parent);
    
    TEST_ASSERT_NOT_NULL(grandparent);
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child);
    
    // 验证层次结构
    TEST_ASSERT_NULL(ctx_if->get_parent(grandparent));
    TEST_ASSERT_EQUAL(grandparent, ctx_if->get_parent(parent));
    TEST_ASSERT_EQUAL(parent, ctx_if->get_parent(child));
    
    // 在各个上下文中分配内存
    void* gp_mem = ctx_if->allocate(grandparent, 50, 8);
    void* p_mem = ctx_if->allocate(parent, 75, 16);
    void* c_mem = ctx_if->allocate(child, 100, 32);
    
    TEST_ASSERT_NOT_NULL(gp_mem);
    TEST_ASSERT_NOT_NULL(p_mem);
    TEST_ASSERT_NOT_NULL(c_mem);
    
    // 验证统计信息
    TEST_ASSERT(ctx_if->get_total_allocated(grandparent) >= 50);
    TEST_ASSERT(ctx_if->get_total_allocated(parent) >= 75);
    TEST_ASSERT(ctx_if->get_total_allocated(child) >= 100);
    
    // 重置子上下文
    ctx_if->reset(child);
    TEST_ASSERT_EQUAL(0, ctx_if->get_total_allocated(child));
    
    // 销毁父上下文（应该自动销毁子上下文）
    ctx_if->destroy(parent);
    
    // 清理剩余上下文
    ctx_if->destroy(grandparent);
    
    return true;
}

/**
 * @brief 运行所有上下文管理测试
 * 
 * @return bool 是否所有测试都通过
 */
bool test_context_management_all(void)
{
    test_context_print_header("上下文管理");
    
    TestCase tests[] = {
        {"创建上下文 - 基本功能", test_context_create_basic, true},
        {"创建上下文 - 带父上下文", test_context_create_with_parent, true},
        {"创建上下文 - 无效名称", test_context_create_invalid_name, true},
        {"销毁上下文 - 基本功能", test_context_destroy_basic, true},
        {"销毁上下文 - NULL上下文", test_context_destroy_null, true},
        {"销毁上下文 - 带分配的内存", test_context_destroy_with_allocations, true},
        {"重置上下文 - 基本功能", test_context_reset_basic, true},
        {"重置上下文 - NULL上下文", test_context_reset_null, true},
        {"重置上下文 - 带分配的内存", test_context_reset_with_allocations, true},
        {"上下文完整生命周期", test_context_lifecycle, true},
    };
    
    int test_count = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, test_count, "上下文管理");
}
