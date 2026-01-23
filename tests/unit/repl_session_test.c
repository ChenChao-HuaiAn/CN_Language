/*
 * repl_session_test.c
 * REPL 会话状态管理单元测试
 * 
 * 测试场景：
 * - 会话创建和销毁
 * - 会话重置功能
 * - 函数符号注册
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "cnlang/frontend/semantics.h"

/* 模拟 ReplSession 结构 */
typedef struct {
    CnSemScope *global_scope;
    void *accumulated_ast;  // 简化版，只测试作用域
} ReplSession;

/* 测试辅助函数：创建会话 */
static ReplSession *repl_session_new(void)
{
    ReplSession *session = (ReplSession *)malloc(sizeof(ReplSession));
    if (!session) {
        return NULL;
    }

    // 创建全局作用域
    session->global_scope = cn_sem_scope_new(CN_SEM_SCOPE_GLOBAL, NULL);
    if (!session->global_scope) {
        free(session);
        return NULL;
    }

    session->accumulated_ast = NULL;

    return session;
}

/* 测试辅助函数：销毁会话 */
static void repl_session_free(ReplSession *session)
{
    if (!session) {
        return;
    }

    if (session->global_scope) {
        cn_sem_scope_free(session->global_scope);
    }

    free(session);
}

/* 测试辅助函数：重置会话 */
static void repl_session_reset(ReplSession *session)
{
    if (!session) {
        return;
    }

    // 释放旧的作用域
    if (session->global_scope) {
        cn_sem_scope_free(session->global_scope);
    }

    // 创建新的全局作用域
    session->global_scope = cn_sem_scope_new(CN_SEM_SCOPE_GLOBAL, NULL);
}

/* 测试 1：会话创建和销毁 */
static void test_session_lifecycle(void)
{
    printf("测试: 会话创建和销毁...\n");
    
    // 创建会话
    ReplSession *session = repl_session_new();
    assert(session != NULL);
    assert(session->global_scope != NULL);
    
    // 销毁会话
    repl_session_free(session);
    
    printf("  ✓ 会话生命周期测试通过\n\n");
}

/* 测试 2：符号注册和查找 */
static void test_symbol_registration(void)
{
    printf("测试: 符号注册和查找...\n");
    
    ReplSession *session = repl_session_new();
    assert(session != NULL);
    
    // 注册函数符号
    const char *func_name = "测试函数";
    CnSemSymbol *symbol = cn_sem_scope_insert_symbol(
        session->global_scope,
        func_name,
        strlen(func_name),
        CN_SEM_SYMBOL_FUNCTION
    );
    assert(symbol != NULL);
    
    // 查找已注册的符号
    symbol = cn_sem_scope_lookup(
        session->global_scope,
        func_name,
        strlen(func_name)
    );
    assert(symbol != NULL);
    assert(symbol->kind == CN_SEM_SYMBOL_FUNCTION);
    
    // 尝试重复注册（应该失败）
    symbol = cn_sem_scope_insert_symbol(
        session->global_scope,
        func_name,
        strlen(func_name),
        CN_SEM_SYMBOL_FUNCTION
    );
    assert(symbol == NULL);  // 重复注册应该失败
    
    repl_session_free(session);
    
    printf("  ✓ 符号注册和查找测试通过\n\n");
}

/* 测试 3：会话重置 */
static void test_session_reset(void)
{
    printf("测试: 会话重置...\n");
    
    ReplSession *session = repl_session_new();
    assert(session != NULL);
    
    // 注册一些符号
    CnSemSymbol *result = cn_sem_scope_insert_symbol(
        session->global_scope,
        "函数1",
        strlen("函数1"),
        CN_SEM_SYMBOL_FUNCTION
    );
    assert(result != NULL);
    
    // 验证符号存在
    CnSemSymbol *symbol = cn_sem_scope_lookup(
        session->global_scope,
        "函数1",
        strlen("函数1")
    );
    assert(symbol != NULL);
    
    // 重置会话
    repl_session_reset(session);
    
    // 验证符号已被清除
    symbol = cn_sem_scope_lookup(
        session->global_scope,
        "函数1",
        strlen("函数1")
    );
    assert(symbol == NULL);
    
    // 验证可以重新注册同名符号
    result = cn_sem_scope_insert_symbol(
        session->global_scope,
        "函数1",
        strlen("函数1"),
        CN_SEM_SYMBOL_FUNCTION
    );
    assert(result != NULL);
    
    repl_session_free(session);
    
    printf("  ✓ 会话重置测试通过\n\n");
}

/* 测试 4：多个符号管理 */
static void test_multiple_symbols(void)
{
    printf("测试: 多个符号管理...\n");
    
    ReplSession *session = repl_session_new();
    assert(session != NULL);
    
    // 注册多个符号
    const char *func_names[] = {"函数A", "函数B", "函数C"};
    for (int i = 0; i < 3; i++) {
        CnSemSymbol *symbol = cn_sem_scope_insert_symbol(
            session->global_scope,
            func_names[i],
            strlen(func_names[i]),
            CN_SEM_SYMBOL_FUNCTION
        );
        assert(symbol != NULL);
    }
    
    // 验证所有符号都可以找到
    for (int i = 0; i < 3; i++) {
        CnSemSymbol *symbol = cn_sem_scope_lookup(
            session->global_scope,
            func_names[i],
            strlen(func_names[i])
        );
        assert(symbol != NULL);
        assert(symbol->kind == CN_SEM_SYMBOL_FUNCTION);
    }
    
    // 验证不存在的符号查找返回 NULL
    CnSemSymbol *symbol = cn_sem_scope_lookup(
        session->global_scope,
        "不存在的函数",
        strlen("不存在的函数")
    );
    assert(symbol == NULL);
    
    repl_session_free(session);
    
    printf("  ✓ 多个符号管理测试通过\n\n");
}

/* 测试 5：NULL 检查 */
static void test_null_safety(void)
{
    printf("测试: NULL 安全检查...\n");
    
    // 测试销毁 NULL 会话
    repl_session_free(NULL);  // 不应崩溃
    
    // 测试重置 NULL 会话
    repl_session_reset(NULL);  // 不应崩溃
    
    printf("  ✓ NULL 安全检查测试通过\n\n");
}

int main(void)
{
    printf("=======================================\n");
    printf("REPL 会话状态管理单元测试\n");
    printf("=======================================\n\n");
    
    // 运行测试
    test_session_lifecycle();
    test_symbol_registration();
    test_session_reset();
    test_multiple_symbols();
    test_null_safety();
    
    printf("=======================================\n");
    printf("所有测试通过！\n");
    printf("=======================================\n");
    
    return 0;
}
