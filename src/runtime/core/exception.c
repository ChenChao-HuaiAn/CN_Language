/**
 * @file exception.c
 * @brief CN语言异常处理运行时实现
 * 
 * 实现基于setjmp/longjmp的异常处理机制
 * 
 * @version 1.0
 * @date 2026-03-29
 */

#include "cnlang/runtime/exception.h"
#include "cnlang/runtime/type_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * 全局异常栈
 * ============================================================================ */

/**
 * @brief 异常栈顶指针
 * 
 * 使用线程局部存储支持多线程（如果需要）
 */
#ifndef CN_THREAD_LOCAL
#define CN_THREAD_LOCAL
#endif

static CN_THREAD_LOCAL CnExceptionFrame *g_exception_stack = NULL;

/* ============================================================================
 * 异常栈管理实现
 * ============================================================================ */

CnExceptionFrame *cn_exception_get_top_frame(void)
{
    return g_exception_stack;
}

void cn_exception_push_frame(CnExceptionFrame *frame)
{
    if (frame == NULL) {
        return;
    }
    
    /* 初始化帧 */
    frame->prev = g_exception_stack;
    frame->current_exception = NULL;
    frame->catch_handled = 0;
    frame->finally_executed = 0;
    
    /* 压入栈 */
    g_exception_stack = frame;
}

CnExceptionFrame *cn_exception_pop_frame(void)
{
    if (g_exception_stack == NULL) {
        return NULL;
    }
    
    CnExceptionFrame *frame = g_exception_stack;
    g_exception_stack = frame->prev;
    
    return frame;
}

/* ============================================================================
 * 异常抛出和捕获实现
 * ============================================================================ */

void cn_throw(CnException *exception, const char *file, int line)
{
    if (exception == NULL) {
        /* 空异常，无法抛出 */
        fprintf(stderr, "错误: 尝试抛出空异常\n");
        abort();
    }
    
    /* 设置抛出位置 */
    exception->file = file;
    exception->line = line;
    exception->is_rethrown = false;
    
    /* 查找最近的异常帧 */
    CnExceptionFrame *frame = g_exception_stack;
    if (frame == NULL) {
        /* 没有异常处理帧，打印错误并终止 */
        fprintf(stderr, "未捕获的异常: %s\n", exception->type_name);
        if (exception->message) {
            fprintf(stderr, "  消息: %s\n", exception->message);
        }
        fprintf(stderr, "  位置: %s:%d\n", file, line);
        abort();
    }
    
    /* 设置当前异常 */
    frame->current_exception = exception;
    frame->catch_handled = 0;
    frame->finally_executed = 0;
    
    /* 跳转到异常处理点 */
    longjmp(frame->jump_buffer, 1);
}

int cn_try_begin(CnExceptionFrame *frame)
{
    if (frame == NULL) {
        return 0;
    }
    
    /* 压入异常帧 */
    cn_exception_push_frame(frame);
    
    /* 设置跳转点，返回0表示正常执行 */
    return setjmp(frame->jump_buffer);
}

void cn_try_end(CnExceptionFrame *frame)
{
    if (frame == NULL) {
        return;
    }
    
    /* 检查是否有未处理的异常 */
    if (frame->current_exception != NULL && !frame->catch_handled) {
        /* 异常未被处理，重新抛出 */
        CnException *ex = frame->current_exception;
        ex->is_rethrown = true;
        
        /* 弹出当前帧 */
        cn_exception_pop_frame();
        
        /* 重新抛出到上层 */
        cn_throw(ex, ex->file, ex->line);
    }
    
    /* 弹出异常帧 */
    cn_exception_pop_frame();
}

CnException *cn_catch(const char *type_name)
{
    CnExceptionFrame *frame = g_exception_stack;
    if (frame == NULL || frame->current_exception == NULL) {
        return NULL;
    }
    
    /* 检查类型匹配 */
    if (cn_exception_type_matches(frame->current_exception, type_name)) {
        frame->catch_handled = 1;
        return frame->current_exception;
    }
    
    return NULL;
}

CnException *cn_catch_any(void)
{
    CnExceptionFrame *frame = g_exception_stack;
    if (frame == NULL || frame->current_exception == NULL) {
        return NULL;
    }
    
    frame->catch_handled = 1;
    return frame->current_exception;
}

void cn_rethrow(void)
{
    CnExceptionFrame *frame = g_exception_stack;
    if (frame == NULL || frame->current_exception == NULL) {
        fprintf(stderr, "错误: 没有异常可以重新抛出\n");
        abort();
    }
    
    CnException *ex = frame->current_exception;
    ex->is_rethrown = true;
    
    /* 弹出当前帧 */
    cn_exception_pop_frame();
    
    /* 重新抛出 */
    cn_throw(ex, ex->file, ex->line);
}

/* ============================================================================
 * 异常类型匹配实现
 * ============================================================================ */

bool cn_exception_type_matches(const CnException *exception, const char *type_name)
{
    if (exception == NULL || type_name == NULL) {
        return false;
    }
    
    /* 精确匹配类型名 */
    if (exception->type_name == NULL) {
        return false;
    }
    
    /* 比较类型名 */
    size_t type_len = strlen(type_name);
    if (exception->type_name_length == type_len &&
        memcmp(exception->type_name, type_name, type_len) == 0) {
        return true;
    }
    
    /* 检查是否为基础异常类型（所有异常的基类） */
    if (strcmp(type_name, CN_EXCEPTION_BASE) == 0) {
        /* 所有异常都继承自基础异常 */
        return true;
    }
    
    /* TODO: 支持继承层次结构中的类型匹配 */
    /* 需要RTTI支持来检查异常类型的继承关系 */
    
    return false;
}

/* ============================================================================
 * 异常创建辅助函数实现
 * ============================================================================ */

void cn_exception_init(CnException *exception, const char *type_name, const char *message)
{
    if (exception == NULL) {
        return;
    }
    
    memset(exception, 0, sizeof(CnException));
    
    exception->type_name = type_name;
    if (type_name) {
        exception->type_name_length = strlen(type_name);
    }
    
    exception->message = message;
    if (message) {
        exception->message_length = strlen(message);
    }
    
    exception->file = NULL;
    exception->line = 0;
    exception->user_data = NULL;
    exception->is_rethrown = false;
}

void cn_throw_simple(const char *type_name, const char *message, 
                     const char *file, int line)
{
    /* 创建临时异常对象 */
    CnException exception;
    cn_exception_init(&exception, type_name, message);
    
    /* 抛出异常 */
    cn_throw(&exception, file, line);
}

/* ============================================================================
 * 调试支持函数
 * ============================================================================ */

/**
 * @brief 打印异常信息（用于调试）
 * 
 * @param exception 异常对象
 */
void cn_exception_print(const CnException *exception)
{
    if (exception == NULL) {
        printf("异常: (空)\n");
        return;
    }
    
    printf("异常类型: %s\n", exception->type_name ? exception->type_name : "(未知)");
    if (exception->message) {
        printf("消息: %s\n", exception->message);
    }
    if (exception->file) {
        printf("位置: %s:%d\n", exception->file, exception->line);
    }
    if (exception->is_rethrown) {
        printf("(重新抛出)\n");
    }
}

/**
 * @brief 获取异常栈深度（用于调试）
 * 
 * @return 当前异常栈的深度
 */
int cn_exception_stack_depth(void)
{
    int depth = 0;
    CnExceptionFrame *frame = g_exception_stack;
    while (frame != NULL) {
        depth++;
        frame = frame->prev;
    }
    return depth;
}
