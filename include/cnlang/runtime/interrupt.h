#ifndef CNLANG_RUNTIME_INTERRUPT_H
#define CNLANG_RUNTIME_INTERRUPT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CN Language 运行时中断管理
 * 提供中断向量管理和中断服务程序注册功能
 */

// 中断向量号定义
#define CN_RT_IRQ_TIMER_0        0
#define CN_RT_IRQ_TIMER_1        1
#define CN_RT_IRQ_KEYBOARD       2
#define CN_RT_IRQ_MOUSE         3
#define CN_RT_IRQ_SERIAL        4
#define CN_RT_IRQ_MAX           32

// 中断服务程序类型定义
typedef void (*CnRtInterruptHandler)(void);

// 中断向量表项
typedef struct CnRtInterruptVectorEntry {
    uint32_t vector_num;              // 中断向量号
    CnRtInterruptHandler handler;   // 中断服务程序
    const char *name;                // 中断名称（用于调试）
} CnRtInterruptVectorEntry;

// 中断管理状态
typedef struct CnRtInterruptState {
    CnRtInterruptVectorEntry vectors[CN_RT_IRQ_MAX];  // 中断向量表
    uint32_t enabled_mask;             // 中断使能掩码
    uint32_t pending_mask;             // 中断挂起掩码
    uint32_t nesting_level;            // 中断嵌套级别
    int global_enabled;                 // 全局中断使能状态
} CnRtInterruptState;

// 初始化中断管理系统
void cn_rt_interrupt_init(void);

// 注册中断服务程序
int cn_rt_interrupt_register(uint32_t vector_num, CnRtInterruptHandler handler, const char *name);

// 注销中断服务程序
int cn_rt_interrupt_unregister(uint32_t vector_num);

// 启用特定中断
void cn_rt_interrupt_enable(uint32_t vector_num);

// 禁用特定中断
void cn_rt_interrupt_disable(uint32_t vector_num);

// 全局启用所有中断
void cn_rt_interrupt_enable_all(void);

// 全局禁用所有中断
void cn_rt_interrupt_disable_all(void);

// 获取中断状态
const CnRtInterruptState* cn_rt_interrupt_get_state(void);

// 触发中断（用于测试或软件中断）
void cn_rt_interrupt_trigger(uint32_t vector_num);

// 进入中断服务程序
void cn_rt_interrupt_enter(void);

// 退出中断服务程序
void cn_rt_interrupt_exit(void);

// 获取当前中断向量号
uint32_t cn_rt_interrupt_get_current(void);

// 检查中断是否挂起
int cn_rt_interrupt_is_pending(uint32_t vector_num);

// 检查中断是否已启用
int cn_rt_interrupt_is_enabled(uint32_t vector_num);

// =============================================================================
// 中文函数名别名 (Chinese Function Name Aliases)
// 默认启用，可通过定义 CN_NO_CHINESE_NAMES 禁用
// =============================================================================
#ifndef CN_NO_CHINESE_NAMES

#define 初始化中断 cn_rt_interrupt_init
#define 注册中断 cn_rt_interrupt_register
#define 注销中断 cn_rt_interrupt_unregister
#define 启用中断 cn_rt_interrupt_enable
#define 禁用中断 cn_rt_interrupt_disable
#define 全局启用中断 cn_rt_interrupt_enable_all
#define 全局禁用中断 cn_rt_interrupt_disable_all
#define 获取中断状态 cn_rt_interrupt_get_state
#define 触发中断 cn_rt_interrupt_trigger
#define 进入中断 cn_rt_interrupt_enter
#define 退出中断 cn_rt_interrupt_exit
#define 获取当前中断 cn_rt_interrupt_get_current
#define 中断是否挂起 cn_rt_interrupt_is_pending
#define 中断是否启用 cn_rt_interrupt_is_enabled

#endif // CN_NO_CHINESE_NAMES

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_INTERRUPT_H */
