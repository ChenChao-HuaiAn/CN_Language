/**
 * @file exception_simple_test.c
 * @brief CN语言异常处理简单测试
 * 
 * @version 1.0
 * @date 2026-03-29
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cnlang/runtime/exception.h"

int main(void)
{
    int tests_passed = 0;
    int tests_failed = 0;

    printf("========================================\n");
    printf("CN语言异常处理简单测试\n");
    printf("========================================\n\n");
    
    /* 测试1: 异常初始化 */
    printf("测试1: 异常初始化... ");
    CnException ex;
    cn_exception_init(&ex, "测试异常", "测试消息");
    if (ex.type_name && strcmp(ex.type_name, "测试异常") == 0) {
        printf("通过\n");
        tests_passed++;
    } else {
        printf("失败\n");
        tests_failed++;
    }
    
    /* 测试2: 异常栈操作 */
    printf("测试2: 异常栈操作... ");
    CnExceptionFrame *top = cn_exception_get_top_frame();
    if (top == NULL) {
        printf("通过\n");
        tests_passed++;
    } else {
        printf("失败\n");
        tests_failed++;
    }
    
    /* 测试3: 异常类型匹配 */
    printf("测试3: 异常类型匹配... ");
    CnException ex2;
    cn_exception_init(&ex2, "运行时异常", NULL);
    if (cn_exception_type_matches(&ex2, "运行时异常")) {
        printf("通过\n");
        tests_passed++;
    } else {
        printf("失败\n");
        tests_failed++;
    }
    
    /* 测试4: 基础异常匹配 */
    printf("测试4: 基础异常匹配... ");
    if (cn_exception_type_matches(&ex2, CN_EXCEPTION_BASE)) {
        printf("通过\n");
        tests_passed++;
    } else {
        printf("失败\n");
        tests_failed++;
    }
    
    /* 测试5: 异常栈深度 */
    printf("测试5: 异常栈深度... ");
    if (cn_exception_stack_depth() == 0) {
        printf("通过\n");
        tests_passed++;
    } else {
        printf("失败\n");
        tests_failed++;
    }
    
    /* 测试6: 异常帧压入弹出 */
    printf("测试6: 异常帧压入弹出... ");
    CnExceptionFrame frame;
    cn_exception_push_frame(&frame);
    int depth1 = cn_exception_stack_depth();
    cn_exception_pop_frame();
    int depth2 = cn_exception_stack_depth();
    if (depth1 == 1 && depth2 == 0) {
        printf("通过\n");
        tests_passed++;
    } else {
        printf("失败 (depth1=%d, depth2=%d)\n", depth1, depth2);
        tests_failed++;
    }
    
    printf("\n========================================\n");
    printf("测试结果: %d 通过, %d 失败\n", tests_passed, tests_failed);
    printf("========================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
