/**
 * @file runtime_exception_test.c
 * @brief CN语言异常处理运行时单元测试
 * 
 * 测试异常处理运行时库的功能：
 * - 异常栈管理
 * - 异常抛出和捕获
 * - 异常类型匹配
 * - finally块执行
 * 
 * @version 1.0
 * @date 2026-03-29
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cnlang/runtime/exception.h"

/* ============================================================================
 * 测试计数器
 * ============================================================================ */

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_START(name) \
    printf("测试: %s... ", name)

#define TEST_PASS() \
    do { \
        printf("通过\n"); \
        g_tests_passed++; \
    } while (0)

#define TEST_FAIL(msg) \
    do { \
        printf("失败: %s\n", msg); \
        g_tests_failed++; \
    } while (0)

/* ============================================================================
 * 测试用例
 * ============================================================================ */

/**
 * @brief 测试异常初始化
 */
static void test_exception_init(void)
{
    TEST_START("异常初始化");
    
    CnException ex;
    cn_exception_init(&ex, "测试异常", "这是一个测试消息");
    
    if (ex.type_name && strcmp(ex.type_name, "测试异常") == 0 &&
        ex.message && strcmp(ex.message, "这是一个测试消息") == 0 &&
        ex.file == NULL && ex.line == 0 && ex.is_rethrown == false) {
        TEST_PASS();
    } else {
        TEST_FAIL("异常初始化字段不正确");
    }
}

/**
 * @brief 测试异常栈操作
 */
static void test_exception_stack(void)
{
    TEST_START("异常栈操作");
    
    /* 确保栈为空 */
    CnExceptionFrame *top = cn_exception_get_top_frame();
    if (top != NULL) {
        TEST_FAIL("异常栈初始状态不为空");
        return;
    }
    
    /* 压入帧 */
    CnExceptionFrame frame1;
    cn_exception_push_frame(&frame1);
    
    top = cn_exception_get_top_frame();
    if (top != &frame1) {
        TEST_FAIL("压入帧后栈顶不正确");
        return;
    }
    
    /* 压入第二个帧 */
    CnExceptionFrame frame2;
    cn_exception_push_frame(&frame2);
    
    top = cn_exception_get_top_frame();
    if (top != &frame2) {
        TEST_FAIL("压入第二个帧后栈顶不正确");
        return;
    }
    
    /* 弹出帧 */
    CnExceptionFrame *popped = cn_exception_pop_frame();
    if (popped != &frame2) {
        TEST_FAIL("弹出的帧不正确");
        return;
    }
    
    top = cn_exception_get_top_frame();
    if (top != &frame1) {
        TEST_FAIL("弹出后栈顶不正确");
        return;
    }
    
    /* 清理 */
    cn_exception_pop_frame();
    
    TEST_PASS();
}

/**
 * @brief 测试异常类型匹配
 */
static void test_exception_type_matching(void)
{
    TEST_START("异常类型匹配");
    
    CnException ex;
    cn_exception_init(&ex, "运行时异常", NULL);
    
    /* 测试精确匹配 */
    if (!cn_exception_type_matches(&ex, "运行时异常")) {
        TEST_FAIL("精确匹配失败");
        return;
    }
    
    /* 测试不匹配 */
    if (cn_exception_type_matches(&ex, "其他异常")) {
        TEST_FAIL("不应该匹配");
        return;
    }
    
    /* 测试基础异常匹配 */
    if (!cn_exception_type_matches(&ex, CN_EXCEPTION_BASE)) {
        TEST_FAIL("基础异常匹配失败");
        return;
    }
    
    TEST_PASS();
}

/**
 * @brief 测试try-catch基本功能
 */
static void test_try_catch_basic(void)
{
    TEST_START("try-catch基本功能");
    
    int catch_executed = 0;
    CnExceptionFrame frame;
    
    if (cn_try_begin(&frame) == 0) {
        /* try块：抛出异常 */
        cn_throw_simple("测试异常", "测试消息", __FILE__, __LINE__);
        /* 不应该执行到这里 */
        TEST_FAIL("异常抛出后继续执行");
        return;
    } else {
        /* catch块 */
        CnException *ex = cn_catch("测试异常");
        if (ex != NULL) {
            catch_executed = 1;
        }
    }
    
    cn_try_end(&frame);
    
    if (catch_executed) {
        TEST_PASS();
    } else {
        TEST_FAIL("catch块未执行");
    }
}

/**
 * @brief 测试catch任意异常
 */
static void test_catch_any(void)
{
    TEST_START("catch任意异常");
    
    int catch_executed = 0;
    CnExceptionFrame frame;
    
    if (cn_try_begin(&frame) == 0) {
        /* try块：抛出异常 */
        cn_throw_simple("任意异常", NULL, __FILE__, __LINE__);
    } else {
        /* catch块：捕获任意异常 */
        CnException *ex = cn_catch_any();
        if (ex != NULL) {
            catch_executed = 1;
        }
    }
    
    cn_try_end(&frame);
    
    if (catch_executed) {
        TEST_PASS();
    } else {
        TEST_FAIL("catch_any未捕获异常");
    }
}

/**
 * @brief 测试异常重新抛出
 */
static void test_rethrow(void)
{
    TEST_START("异常重新抛出");
    
    int outer_catch_executed = 0;
    CnExceptionFrame outer_frame;
    
    if (cn_try_begin(&outer_frame) == 0) {
        /* 外层try块 */
        
        int inner_catch_executed = 0;
        CnExceptionFrame inner_frame;
        
        if (cn_try_begin(&inner_frame) == 0) {
            /* 内层try块：抛出异常 */
            cn_throw_simple("重抛异常", NULL, __FILE__, __LINE__);
        } else {
            /* 内层catch块：捕获但不处理，重新抛出 */
            CnException *ex = cn_catch_any();
            if (ex != NULL) {
                inner_catch_executed = 1;
                cn_rethrow(); /* 重新抛出到外层 */
            }
        }
        
        cn_try_end(&inner_frame);
        
    } else {
        /* 外层catch块 */
        CnException *ex = cn_catch_any();
        if (ex != NULL && ex->is_rethrown) {
            outer_catch_executed = 1;
        }
    }
    
    cn_try_end(&outer_frame);
    
    if (outer_catch_executed) {
        TEST_PASS();
    } else {
        TEST_FAIL("重新抛出的异常未被外层捕获");
    }
}

/**
 * @brief 测试嵌套try块
 */
static void test_nested_try(void)
{
    TEST_START("嵌套try块");
    
    int inner_executed = 0;
    int outer_executed = 0;
    
    CnExceptionFrame outer_frame;
    if (cn_try_begin(&outer_frame) == 0) {
        /* 外层try块 */
        
        CnExceptionFrame inner_frame;
        if (cn_try_begin(&inner_frame) == 0) {
            /* 内层try块：抛出异常 */
            cn_throw_simple("内层异常", NULL, __FILE__, __LINE__);
        } else {
            /* 内层catch块 */
            CnException *ex = cn_catch("内层异常");
            if (ex != NULL) {
                inner_executed = 1;
            }
        }
        cn_try_end(&inner_frame);
        
        /* 外层try块继续执行 */
        outer_executed = 1;
    } else {
        /* 外层catch块（不应该执行） */
        TEST_FAIL("外层catch不应该执行");
        return;
    }
    cn_try_end(&outer_frame);
    
    if (inner_executed && outer_executed) {
        TEST_PASS();
    } else {
        TEST_FAIL("嵌套try块执行不正确");
    }
}

/**
 * @brief 测试异常栈深度
 */
static void test_exception_stack_depth(void)
{
    TEST_START("异常栈深度");
    
    /* 初始深度应为0 */
    if (cn_exception_stack_depth() != 0) {
        TEST_FAIL("初始栈深度不为0");
        return;
    }
    
    /* 压入帧后检查深度 */
    CnExceptionFrame frame1, frame2;
    
    cn_exception_push_frame(&frame1);
    if (cn_exception_stack_depth() != 1) {
        TEST_FAIL("压入1个帧后深度不为1");
        return;
    }
    
    cn_exception_push_frame(&frame2);
    if (cn_exception_stack_depth() != 2) {
        TEST_FAIL("压入2个帧后深度不为2");
        return;
    }
    
    /* 弹出帧后检查深度 */
    cn_exception_pop_frame();
    if (cn_exception_stack_depth() != 1) {
        TEST_FAIL("弹出1个帧后深度不为1");
        return;
    }
    
    cn_exception_pop_frame();
    if (cn_exception_stack_depth() != 0) {
        TEST_FAIL("弹出所有帧后深度不为0");
        return;
    }
    
    TEST_PASS();
}

/**
 * @brief 测试预定义异常类型
 */
static void test_predefined_exceptions(void)
{
    TEST_START("预定义异常类型");
    
    CnException ex;
    
    /* 测试运行时异常 */
    cn_exception_init(&ex, CN_EXCEPTION_RUNTIME, NULL);
    if (!cn_exception_type_matches(&ex, CN_EXCEPTION_RUNTIME)) {
        TEST_FAIL("运行时异常匹配失败");
        return;
    }
    
    /* 测试空指针异常 */
    cn_exception_init(&ex, CN_EXCEPTION_NULL_POINTER, NULL);
    if (!cn_exception_type_matches(&ex, CN_EXCEPTION_NULL_POINTER)) {
        TEST_FAIL("空指针异常匹配失败");
        return;
    }
    
    /* 测试除零异常 */
    cn_exception_init(&ex, CN_EXCEPTION_DIVIDE_BY_ZERO, NULL);
    if (!cn_exception_type_matches(&ex, CN_EXCEPTION_DIVIDE_BY_ZERO)) {
        TEST_FAIL("除零异常匹配失败");
        return;
    }
    
    TEST_PASS();
}

/* ============================================================================
 * 主函数
 * ============================================================================ */

int main(void)
{
    printf("========================================\n");
    printf("CN语言异常处理运行时单元测试\n");
    printf("========================================\n\n");
    
    /* 运行测试 */
    test_exception_init();
    test_exception_stack();
    test_exception_type_matching();
    test_try_catch_basic();
    test_catch_any();
    test_rethrow();
    test_nested_try();
    test_exception_stack_depth();
    test_predefined_exceptions();
    
    /* 输出结果 */
    printf("\n========================================\n");
    printf("测试结果: %d 通过, %d 失败\n", g_tests_passed, g_tests_failed);
    printf("========================================\n");
    
    return g_tests_failed > 0 ? 1 : 0;
}
