#include "cnlang/runtime/interrupt.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// 测试用的中断处理函数
static int g_test_irq_0_count = 0;
static int g_test_irq_1_count = 0;

void test_irq_0_handler(void) {
    g_test_irq_0_count++;
}

void test_irq_1_handler(void) {
    g_test_irq_1_count++;
}

// 测试中断初始化
void test_interrupt_init() {
    printf("测试中断初始化...\n");
    cn_rt_interrupt_init();
    
    const CnRtInterruptState *state = cn_rt_interrupt_get_state();
    assert(state != NULL);
    assert(state->global_enabled == 1);
    assert(state->enabled_mask == 0);
    assert(state->pending_mask == 0);
    assert(state->nesting_level == 0);
    
    printf("  [通过] 中断系统初始化成功\n");
}

// 测试中断注册
void test_interrupt_register() {
    printf("测试中断注册...\n");
    cn_rt_interrupt_init();
    
    // 注册中断处理程序
    int ret = cn_rt_interrupt_register(0, test_irq_0_handler, "测试中断0");
    assert(ret == 0);
    
    // 重复注册应该失败
    ret = cn_rt_interrupt_register(0, test_irq_1_handler, "测试中断0重复");
    assert(ret == -3);
    
    // 无效向量号
    ret = cn_rt_interrupt_register(CN_RT_IRQ_MAX + 1, test_irq_0_handler, "无效");
    assert(ret == -1);
    
    // NULL处理程序
    ret = cn_rt_interrupt_register(1, NULL, "NULL处理");
    assert(ret == -2);
    
    printf("  [通过] 中断注册测试通过\n");
}

// 测试中断注销
void test_interrupt_unregister() {
    printf("测试中断注销...\n");
    cn_rt_interrupt_init();
    
    // 注册并注销
    cn_rt_interrupt_register(0, test_irq_0_handler, "测试");
    int ret = cn_rt_interrupt_unregister(0);
    assert(ret == 0);
    
    // 无效向量号
    ret = cn_rt_interrupt_unregister(CN_RT_IRQ_MAX + 1);
    assert(ret == -1);
    
    printf("  [通过] 中断注销测试通过\n");
}

// 测试中断启用/禁用
void test_interrupt_enable_disable() {
    printf("测试中断启用/禁用...\n");
    cn_rt_interrupt_init();
    
    // 启用中断
    cn_rt_interrupt_enable(0);
    assert(cn_rt_interrupt_is_enabled(0) == 1);
    
    // 禁用中断
    cn_rt_interrupt_disable(0);
    assert(cn_rt_interrupt_is_enabled(0) == 0);
    
    // 全局启用/禁用
    cn_rt_interrupt_disable_all();
    const CnRtInterruptState *state = cn_rt_interrupt_get_state();
    assert(state->global_enabled == 0);
    
    cn_rt_interrupt_enable_all();
    state = cn_rt_interrupt_get_state();
    assert(state->global_enabled == 1);
    
    printf("  [通过] 中断启用/禁用测试通过\n");
}

// 测试中断触发
void test_interrupt_trigger() {
    printf("测试中断触发...\n");
    cn_rt_interrupt_init();
    
    // 注册中断处理程序
    cn_rt_interrupt_register(0, test_irq_0_handler, "测试中断0");
    cn_rt_interrupt_register(1, test_irq_1_handler, "测试中断1");
    
    // 启用中断
    cn_rt_interrupt_enable(0);
    cn_rt_interrupt_enable(1);
    
    // 重置计数器
    g_test_irq_0_count = 0;
    g_test_irq_1_count = 0;
    
    // 触发中断
    cn_rt_interrupt_trigger(0);
    assert(g_test_irq_0_count == 1);
    assert(g_test_irq_1_count == 0);
    
    cn_rt_interrupt_trigger(1);
    assert(g_test_irq_0_count == 1);
    assert(g_test_irq_1_count == 1);
    
    // 多次触发
    cn_rt_interrupt_trigger(0);
    cn_rt_interrupt_trigger(0);
    assert(g_test_irq_0_count == 3);
    
    printf("  [通过] 中断触发测试通过\n");
}

// 测试中断挂起状态
void test_interrupt_pending() {
    printf("测试中断挂起状态...\n");
    cn_rt_interrupt_init();
    
    // 初始状态：无挂起
    assert(cn_rt_interrupt_is_pending(0) == 0);
    
    // 注：当前实现在中断处理完成后清除挂起标志
    // 所以触发后挂起标志会被清除
    cn_rt_interrupt_register(0, test_irq_0_handler, "测试");
    cn_rt_interrupt_enable(0);
    cn_rt_interrupt_trigger(0);
    
    // 处理完成后挂起标志应被清除
    assert(cn_rt_interrupt_is_pending(0) == 0);
    
    printf("  [通过] 中断挂起状态测试通过\n");
}

// 测试中断嵌套
void test_interrupt_nesting() {
    printf("测试中断嵌套...\n");
    cn_rt_interrupt_init();
    
    const CnRtInterruptState *state = cn_rt_interrupt_get_state();
    assert(state->nesting_level == 0);
    
    // 模拟进入/退出中断
    cn_rt_interrupt_enter();
    state = cn_rt_interrupt_get_state();
    assert(state->nesting_level == 1);
    
    cn_rt_interrupt_enter();
    state = cn_rt_interrupt_get_state();
    assert(state->nesting_level == 2);
    
    cn_rt_interrupt_exit();
    state = cn_rt_interrupt_get_state();
    assert(state->nesting_level == 1);
    
    cn_rt_interrupt_exit();
    state = cn_rt_interrupt_get_state();
    assert(state->nesting_level == 0);
    
    printf("  [通过] 中断嵌套测试通过\n");
}

// 测试全局中断禁用时的触发
void test_interrupt_global_disabled() {
    printf("测试全局中断禁用...\n");
    cn_rt_interrupt_init();
    
    cn_rt_interrupt_register(0, test_irq_0_handler, "测试");
    cn_rt_interrupt_enable(0);
    
    // 禁用全局中断
    cn_rt_interrupt_disable_all();
    
    g_test_irq_0_count = 0;
    cn_rt_interrupt_trigger(0);
    
    // 全局禁用时不应执行处理程序
    assert(g_test_irq_0_count == 0);
    
    // 重新启用全局中断
    cn_rt_interrupt_enable_all();
    cn_rt_interrupt_trigger(0);
    assert(g_test_irq_0_count == 1);
    
    printf("  [通过] 全局中断禁用测试通过\n");
}

int main(void) {
    printf("========================================\n");
    printf("CN Language 运行时中断管理单元测试\n");
    printf("========================================\n\n");
    
    test_interrupt_init();
    test_interrupt_register();
    test_interrupt_unregister();
    test_interrupt_enable_disable();
    test_interrupt_trigger();
    test_interrupt_pending();
    test_interrupt_nesting();
    test_interrupt_global_disabled();
    
    printf("\n========================================\n");
    printf("所有测试通过！✓\n");
    printf("========================================\n");
    
    return 0;
}
