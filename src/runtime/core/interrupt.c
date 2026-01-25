#include "cnlang/runtime/interrupt.h"
#include <string.h>
#include <stdio.h>

/*
 * CN Language 中断管理运行时实现
 * 提供中断向量管理和中断服务程序注册功能
 */

// 全局中断管理状态
static CnRtInterruptState g_interrupt_state;
static uint32_t g_current_vector = 0;  // 当前处理的中断向量号

// 初始化中断管理系统
void cn_rt_interrupt_init(void)
{
    memset(&g_interrupt_state, 0, sizeof(CnRtInterruptState));
    g_interrupt_state.global_enabled = 1;  // 默认启用全局中断
    g_current_vector = 0;
}

// 注册中断服务程序
int cn_rt_interrupt_register(uint32_t vector_num, CnRtInterruptHandler handler, const char *name)
{
    if (vector_num >= CN_RT_IRQ_MAX) {
        return -1;  // 向量号超出范围
    }
    
    if (handler == NULL) {
        return -2;  // 无效的处理程序
    }
    
    // 检查是否已注册
    if (g_interrupt_state.vectors[vector_num].handler != NULL) {
        return -3;  // 已存在中断处理程序
    }
    
    g_interrupt_state.vectors[vector_num].vector_num = vector_num;
    g_interrupt_state.vectors[vector_num].handler = handler;
    g_interrupt_state.vectors[vector_num].name = name;
    
    return 0;  // 成功
}

// 注销中断服务程序
int cn_rt_interrupt_unregister(uint32_t vector_num)
{
    if (vector_num >= CN_RT_IRQ_MAX) {
        return -1;  // 向量号超出范围
    }
    
    g_interrupt_state.vectors[vector_num].handler = NULL;
    g_interrupt_state.vectors[vector_num].name = NULL;
    
    return 0;
}

// 启用特定中断
void cn_rt_interrupt_enable(uint32_t vector_num)
{
    if (vector_num < CN_RT_IRQ_MAX) {
        g_interrupt_state.enabled_mask |= (1U << vector_num);
    }
}

// 禁用特定中断
void cn_rt_interrupt_disable(uint32_t vector_num)
{
    if (vector_num < CN_RT_IRQ_MAX) {
        g_interrupt_state.enabled_mask &= ~(1U << vector_num);
    }
}

// 全局启用所有中断
void cn_rt_interrupt_enable_all(void)
{
    g_interrupt_state.global_enabled = 1;
}

// 全局禁用所有中断
void cn_rt_interrupt_disable_all(void)
{
    g_interrupt_state.global_enabled = 0;
}

// 获取中断状态
const CnRtInterruptState* cn_rt_interrupt_get_state(void)
{
    return &g_interrupt_state;
}

// 触发中断（用于测试或软件中断）
void cn_rt_interrupt_trigger(uint32_t vector_num)
{
    if (vector_num >= CN_RT_IRQ_MAX) {
        return;  // 向量号超出范围
    }
    
    // 检查中断是否启用
    if (!g_interrupt_state.global_enabled) {
        return;  // 全局中断已禁用
    }
    
    if (!(g_interrupt_state.enabled_mask & (1U << vector_num))) {
        return;  // 该中断未启用
    }
    
    // 检查是否有注册的处理程序
    CnRtInterruptHandler handler = g_interrupt_state.vectors[vector_num].handler;
    if (handler == NULL) {
        return;  // 没有处理程序
    }
    
    // 标记中断挂起
    g_interrupt_state.pending_mask |= (1U << vector_num);
    
    // 进入中断处理
    cn_rt_interrupt_enter();
    g_current_vector = vector_num;
    
    // 调用中断处理程序
    handler();
    
    // 清除挂起标志
    g_interrupt_state.pending_mask &= ~(1U << vector_num);
    
    // 退出中断处理
    cn_rt_interrupt_exit();
    g_current_vector = 0;
}

// 进入中断服务程序
void cn_rt_interrupt_enter(void)
{
    g_interrupt_state.nesting_level++;
}

// 退出中断服务程序
void cn_rt_interrupt_exit(void)
{
    if (g_interrupt_state.nesting_level > 0) {
        g_interrupt_state.nesting_level--;
    }
}

// 获取当前中断向量号
uint32_t cn_rt_interrupt_get_current(void)
{
    return g_current_vector;
}

// 检查中断是否挂起
int cn_rt_interrupt_is_pending(uint32_t vector_num)
{
    if (vector_num >= CN_RT_IRQ_MAX) {
        return 0;
    }
    
    return (g_interrupt_state.pending_mask & (1U << vector_num)) != 0;
}

// 检查中断是否已启用
int cn_rt_interrupt_is_enabled(uint32_t vector_num)
{
    if (vector_num >= CN_RT_IRQ_MAX) {
        return 0;
    }
    
    return (g_interrupt_state.enabled_mask & (1U << vector_num)) != 0;
}
