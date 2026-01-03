/******************************************************************************
 * 文件名: CN_platform_windows_interrupt.c
 * 功能: CN_Language Windows平台中断控制器实现
 * 描述: Windows平台中断控制器基础实现，提供基本的中断处理功能
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Windows平台中断控制器基础功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_windows_interrupt.h"
#include <stdlib.h>
#include <string.h>

/* 内部数据结构定义 */

/** Windows中断控制器内部状态 */
typedef struct {
    bool is_initialized;
    uint32_t handler_count;
    Stru_CN_WindowsInterruptHandlerInfo_t handlers[CN_WINDOWS_MAX_INTERRUPT_HANDLERS];
    CRITICAL_SECTION global_lock;
    DWORD main_thread_id;
} WindowsInterruptControllerState;

static WindowsInterruptControllerState g_controller_state = {
    .is_initialized = false,
    .handler_count = 0,
    .main_thread_id = 0
};

/* 静态函数声明 */

static bool windows_interrupt_controller_initialize(void);
static void windows_interrupt_controller_cleanup(void);
static bool windows_interrupt_controller_get_info(Stru_CN_InterruptControllerInfo_t* info);
static bool windows_interrupt_controller_is_initialized(void);
static bool windows_interrupt_controller_setup_vector_table(void* table_addr, size_t table_size);
static bool windows_interrupt_controller_get_vector_table(void** table_addr, size_t* table_size);
static bool windows_interrupt_controller_register_handler(CN_InterruptNumber_t irq, 
                                                         CN_InterruptHandler_t handler, 
                                                         void* context, 
                                                         const Stru_CN_InterruptDescriptor_t* desc);
static bool windows_interrupt_controller_unregister_handler(CN_InterruptNumber_t irq);
static bool windows_interrupt_controller_enable_interrupt(CN_InterruptNumber_t irq);
static bool windows_interrupt_controller_disable_interrupt(CN_InterruptNumber_t irq);
static bool windows_interrupt_controller_is_interrupt_enabled(CN_InterruptNumber_t irq);
static bool windows_interrupt_controller_mask_interrupt(CN_InterruptNumber_t irq);
static bool windows_interrupt_controller_unmask_interrupt(CN_InterruptNumber_t irq);
static bool windows_interrupt_controller_is_interrupt_masked(CN_InterruptNumber_t irq);
static bool windows_interrupt_controller_set_priority(CN_InterruptNumber_t irq, CN_InterruptPriority_t priority);
static bool windows_interrupt_controller_get_priority(CN_InterruptNumber_t irq, CN_InterruptPriority_t* priority);
static bool windows_interrupt_controller_set_trigger_mode(CN_InterruptNumber_t irq, Eum_CN_InterruptTriggerMode_t mode);
static bool windows_interrupt_controller_set_polarity(CN_InterruptNumber_t irq, Eum_CN_InterruptPolarity_t polarity);
static bool windows_interrupt_controller_set_affinity(CN_InterruptNumber_t irq, uint32_t cpu_mask);
static bool windows_interrupt_controller_get_affinity(CN_InterruptNumber_t irq, uint32_t* cpu_mask);
static bool windows_interrupt_controller_is_pending(CN_InterruptNumber_t irq);
static bool windows_interrupt_controller_acknowledge(CN_InterruptNumber_t irq);
static bool windows_interrupt_controller_send_eoi(CN_InterruptNumber_t irq);
static bool windows_interrupt_controller_enter_critical(void);
static bool windows_interrupt_controller_exit_critical(void);
static bool windows_interrupt_controller_save_interrupt_state(CN_InterruptFlags_t* flags);
static bool windows_interrupt_controller_restore_interrupt_state(CN_InterruptFlags_t flags);
static bool windows_interrupt_controller_get_statistics(CN_InterruptNumber_t irq, Stru_CN_InterruptStatistics_t* stats);
static bool windows_interrupt_controller_reset_statistics(CN_InterruptNumber_t irq);
static bool windows_interrupt_controller_enable_wakeup(CN_InterruptNumber_t irq);
static bool windows_interrupt_controller_disable_wakeup(CN_InterruptNumber_t irq);
static bool windows_interrupt_controller_is_wakeup_enabled(CN_InterruptNumber_t irq);
static bool windows_interrupt_controller_dump_state(void* output_buffer, size_t buffer_size);
static bool windows_interrupt_controller_validate_configuration(void);

/* 全局接口变量定义 */

/** Windows中断控制器接口 */
Stru_CN_InterruptControllerInterface_t g_windows_interrupt_controller_interface = {
    .initialize = windows_interrupt_controller_initialize,
    .cleanup = windows_interrupt_controller_cleanup,
    .get_info = windows_interrupt_controller_get_info,
    .is_initialized = windows_interrupt_controller_is_initialized,
    .setup_vector_table = windows_interrupt_controller_setup_vector_table,
    .get_vector_table = windows_interrupt_controller_get_vector_table,
    .register_handler = windows_interrupt_controller_register_handler,
    .unregister_handler = windows_interrupt_controller_unregister_handler,
    .enable_interrupt = windows_interrupt_controller_enable_interrupt,
    .disable_interrupt = windows_interrupt_controller_disable_interrupt,
    .is_interrupt_enabled = windows_interrupt_controller_is_interrupt_enabled,
    .mask_interrupt = windows_interrupt_controller_mask_interrupt,
    .unmask_interrupt = windows_interrupt_controller_unmask_interrupt,
    .is_interrupt_masked = windows_interrupt_controller_is_interrupt_masked,
    .set_priority = windows_interrupt_controller_set_priority,
    .get_priority = windows_interrupt_controller_get_priority,
    .set_trigger_mode = windows_interrupt_controller_set_trigger_mode,
    .set_polarity = windows_interrupt_controller_set_polarity,
    .set_affinity = windows_interrupt_controller_set_affinity,
    .get_affinity = windows_interrupt_controller_get_affinity,
    .is_pending = windows_interrupt_controller_is_pending,
    .acknowledge = windows_interrupt_controller_acknowledge,
    .send_eoi = windows_interrupt_controller_send_eoi,
    .enter_critical = windows_interrupt_controller_enter_critical,
    .exit_critical = windows_interrupt_controller_exit_critical,
    .save_interrupt_state = windows_interrupt_controller_save_interrupt_state,
    .restore_interrupt_state = windows_interrupt_controller_restore_interrupt_state,
    .get_statistics = windows_interrupt_controller_get_statistics,
    .reset_statistics = windows_interrupt_controller_reset_statistics,
    .enable_wakeup = windows_interrupt_controller_enable_wakeup,
    .disable_wakeup = windows_interrupt_controller_disable_wakeup,
    .is_wakeup_enabled = windows_interrupt_controller_is_wakeup_enabled,
    .dump_state = windows_interrupt_controller_dump_state,
    .validate_configuration = windows_interrupt_controller_validate_configuration
};

/******************************************************************************
 * 辅助函数
 ******************************************************************************/

/**
 * @brief 查找中断处理程序
 */
static Stru_CN_WindowsInterruptHandlerInfo_t* windows_find_handler_by_interrupt(CN_InterruptNumber_t interrupt)
{
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_controller_state.handlers[i].interrupt_number == interrupt &&
            g_controller_state.handlers[i].is_installed) {
            return &g_controller_state.handlers[i];
        }
    }
    return NULL;
}

/**
 * @brief 查找空闲的处理程序槽位
 */
static Stru_CN_WindowsInterruptHandlerInfo_t* windows_find_free_handler_slot(void)
{
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (!g_controller_state.handlers[i].is_installed) {
            return &g_controller_state.handlers[i];
        }
    }
    return NULL;
}

/******************************************************************************
 * 中断控制器函数实现
 ******************************************************************************/

/**
 * @brief 初始化Windows中断控制器
 */
static bool windows_interrupt_controller_initialize(void)
{
    if (g_controller_state.is_initialized) {
        return true;
    }
    
    // 初始化临界区
    InitializeCriticalSection(&g_controller_state.global_lock);
    
    // 初始化处理程序数组
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        g_controller_state.handlers[i].interrupt_number = 0;
        g_controller_state.handlers[i].handler = NULL;
        g_controller_state.handlers[i].user_data = NULL;
        g_controller_state.handlers[i].name[0] = '\0';
        g_controller_state.handlers[i].flags = 0;
        g_controller_state.handlers[i].event_handle = NULL;
        g_controller_state.handlers[i].is_installed = false;
        g_controller_state.handlers[i].lock = NULL;
    }
    
    g_controller_state.handler_count = 0;
    g_controller_state.main_thread_id = GetCurrentThreadId();
    g_controller_state.is_initialized = true;
    
    return true;
}

/**
 * @brief 清理Windows中断控制器
 */
static void windows_interrupt_controller_cleanup(void)
{
    if (!g_controller_state.is_initialized) {
        return;
    }
    
    EnterCriticalSection(&g_controller_state.global_lock);
    
    // 清除所有处理程序信息
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_controller_state.handlers[i].is_installed) {
            // 释放事件句柄
            if (g_controller_state.handlers[i].event_handle != NULL) {
                CloseHandle(g_controller_state.handlers[i].event_handle);
            }
            
            // 释放临界区
            if (g_controller_state.handlers[i].lock != NULL) {
                DeleteCriticalSection(g_controller_state.handlers[i].lock);
                free(g_controller_state.handlers[i].lock);
            }
            
            g_controller_state.handlers[i].interrupt_number = 0;
            g_controller_state.handlers[i].handler = NULL;
            g_controller_state.handlers[i].user_data = NULL;
            g_controller_state.handlers[i].name[0] = '\0';
            g_controller_state.handlers[i].flags = 0;
            g_controller_state.handlers[i].event_handle = NULL;
            g_controller_state.handlers[i].is_installed = false;
            g_controller_state.handlers[i].lock = NULL;
        }
    }
    
    g_controller_state.handler_count = 0;
    g_controller_state.is_initialized = false;
    
    LeaveCriticalSection(&g_controller_state.global_lock);
    
    // 删除全局临界区
    DeleteCriticalSection(&g_controller_state.global_lock);
}

/**
 * @brief 获取中断控制器信息
 */
static bool windows_interrupt_controller_get_info(Stru_CN_InterruptControllerInfo_t* info)
{
    if (!info || !g_controller_state.is_initialized) {
        return false;
    }
    
    memset(info, 0, sizeof(Stru_CN_InterruptControllerInfo_t));
    
    info->type = Eum_INTERRUPT_CONTROLLER_CUSTOM;
    strncpy(info->name, "Windows Event Controller", sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    info->version = 1;
    info->max_interrupts = CN_WINDOWS_MAX_INTERRUPT_HANDLERS;
    info->max_priority = 32;
    info->supports_nesting = false;
    info->supports_priority = false;
    info->supports_affinity = false;
    info->supports_wakeup = false;
    
    return true;
}

/**
 * @brief 检查中断控制器是否已初始化
 */
static bool windows_interrupt_controller_is_initialized(void)
{
    return g_controller_state.is_initialized;
}

/**
 * @brief 设置中断向量表
 */
static bool windows_interrupt_controller_setup_vector_table(void* table_addr, size_t table_size)
{
    // Windows平台不支持硬件中断向量表
    (void)table_addr;
    (void)table_size;
    return true;
}

/**
 * @brief 获取中断向量表
 */
static bool windows_interrupt_controller_get_vector_table(void** table_addr, size_t* table_size)
{
    // Windows平台不支持硬件中断向量表
    if (table_addr) *table_addr = NULL;
    if (table_size) *table_size = 0;
    return true;
}

/**
 * @brief 注册中断处理程序
 */
static bool windows_interrupt_controller_register_handler(CN_InterruptNumber_t irq, 
                                                         CN_InterruptHandler_t handler, 
                                                         void* context, 
                                                         const Stru_CN_InterruptDescriptor_t* desc)
{
    if (!g_controller_state.is_initialized || !handler) {
        return false;
    }
    
    // 检查中断号是否在有效范围内
    if (irq < 2001 || irq > 2064) {
        return false;
    }
    
    EnterCriticalSection(&g_controller_state.global_lock);
    
    // 查找现有的处理程序或空闲槽位
    Stru_CN_WindowsInterruptHandlerInfo_t* handler_info = windows_find_handler_by_interrupt(irq);
    if (!handler_info) {
        handler_info = windows_find_free_handler_slot();
        if (!handler_info) {
            LeaveCriticalSection(&g_controller_state.global_lock);
            return false;
        }
    }
    
    // 创建事件句柄
    if (handler_info->event_handle == NULL) {
        handler_info->event_handle = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (handler_info->event_handle == NULL) {
            LeaveCriticalSection(&g_controller_state.global_lock);
            return false;
        }
    }
    
    // 创建临界区
    if (handler_info->lock == NULL) {
        handler_info->lock = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
        if (handler_info->lock == NULL) {
            CloseHandle(handler_info->event_handle);
            handler_info->event_handle = NULL;
            LeaveCriticalSection(&g_controller_state.global_lock);
            return false;
        }
        InitializeCriticalSection(handler_info->lock);
    }
    
    // 更新处理程序信息
    handler_info->interrupt_number = irq;
    handler_info->handler = handler;
    handler_info->user_data = context;
    handler_info->flags = 0;
    handler_info->is_installed = true;
    
    if (desc && desc->description[0] != '\0') {
        strncpy(handler_info->name, desc->description, CN_WINDOWS_MAX_INTERRUPT_NAME - 1);
        handler_info->name[CN_WINDOWS_MAX_INTERRUPT_NAME - 1] = '\0';
    } else {
        snprintf(handler_info->name, CN_WINDOWS_MAX_INTERRUPT_NAME, 
                "interrupt_%u", (unsigned int)irq);
    }
    
    if (handler_info->interrupt_number == 0) {
        g_controller_state.handler_count++;
    }
    
    LeaveCriticalSection(&g_controller_state.global_lock);
    
    return true;
}

/**
 * @brief 注销中断处理程序
 */
static bool windows_interrupt_controller_unregister_handler(CN_InterruptNumber_t irq)
{
    if (!g_controller_state.is_initialized) {
        return false;
    }
    
    EnterCriticalSection(&g_controller_state.global_lock);
    
    // 查找处理程序
    Stru_CN_WindowsInterruptHandlerInfo_t* handler_info = windows_find_handler_by_interrupt(irq);
    if (!handler_info) {
        LeaveCriticalSection(&g_controller_state.global_lock);
        return false;
    }
    
    // 释放事件句柄
    if (handler_info->event_handle != NULL) {
        CloseHandle(handler_info->event_handle);
        handler_info->event_handle = NULL;
    }
    
    // 释放临界区
    if (handler_info->lock != NULL) {
        DeleteCriticalSection(handler_info->lock);
        free(handler_info->lock);
        handler_info->lock = NULL;
    }
    
    // 清除处理程序信息
    handler_info->interrupt_number = 0;
    handler_info->handler = NULL;
    handler_info->user_data = NULL;
    handler_info->name[0] = '\0';
    handler_info->flags = 0;
    handler_info->is_installed = false;
    
    g_controller_state.handler_count--;
    
    LeaveCriticalSection(&g_controller_state.global_lock);
    
    return true;
}

/**
 * @brief 启用中断
 */
static bool windows_interrupt_controller_enable_interrupt(CN_InterruptNumber_t irq)
{
    // 检查处理程序是否存在
    Stru_CN_WindowsInterruptHandlerInfo_t* handler_info = windows_find_handler_by_interrupt(irq);
    return handler_info != NULL;
}

/**
 * @brief 禁用中断
 */
static bool windows_interrupt_controller_disable_interrupt(CN_InterruptNumber_t irq)
{
    // 简单实现：总是成功
    (void)irq;
    return true;
}

/**
 * @brief 检查中断是否启用
 */
static bool windows_interrupt_controller_is_interrupt_enabled(CN_InterruptNumber_t irq)
{
    Stru_CN_WindowsInterruptHandlerInfo_t* handler_info = windows_find_handler_by_interrupt(irq);
    return handler_info != NULL;
}

/**
 * @brief 屏蔽中断
 */
static bool windows_interrupt_controller_mask_interrupt(CN_InterruptNumber_t irq)
{
    // 简单实现：等同于禁用中断
    return windows_interrupt_controller_disable_interrupt(irq);
}

/**
 * @brief 取消屏蔽中断
 */
static bool windows_interrupt_controller_unmask_interrupt(CN_InterruptNumber_t irq)
{
    // 简单实现：等同于启用中断
    return windows_interrupt_controller_enable_interrupt(irq);
}

/**
 * @brief 检查中断是否被屏蔽
 */
static bool windows_interrupt_controller_is_interrupt_masked(CN_InterruptNumber_t irq)
{
    // 简单实现：检查是否启用
    return !windows_interrupt_controller_is_interrupt_enabled(irq);
}

/**
 * @brief 设置中断优先级
 */
static bool windows_interrupt_controller_set_priority(CN_InterruptNumber_t irq, CN_InterruptPriority_t priority)
{
    // 简单实现：总是成功
    (void)irq;
    (void)priority;
    return true;
}

/**
 * @brief 获取中断优先级
 */
static bool windows_interrupt_controller_get_priority(CN_InterruptNumber_t irq, CN_InterruptPriority_t* priority)
{
    if (!priority) {
        return false;
    }
    
    // 简单实现：返回默认优先级
    *priority = 0;
    (void)irq;
    return true;
}

/**
 * @brief 设置中断触发模式
 */
static bool windows_interrupt_controller_set_trigger_mode(CN_InterruptNumber_t irq, Eum_CN_InterruptTriggerMode_t mode)
{
    // 简单实现：总是成功
    (void)irq;
    (void)mode;
    return true;
}

/**
 * @brief 设置中断极性
 */
static bool windows_interrupt_controller_set_polarity(CN_InterruptNumber_t irq, Eum_CN_InterruptPolarity_t polarity)
{
    // 简单实现：总是成功
    (void)irq;
    (void)polarity;
    return true;
}

/**
 * @brief 设置中断亲和性
 */
static bool windows_interrupt_controller_set_affinity(CN_InterruptNumber_t irq, uint32_t cpu_mask)
{
    // 简单实现：总是成功
    (void)irq;
    (void)cpu_mask;
    return true;
}

/**
 * @brief 获取中断亲和性
 */
static bool windows_interrupt_controller_get_affinity(CN_InterruptNumber_t irq, uint32_t* cpu_mask)
{
    if (!cpu_mask) {
        return false;
    }
    
    // 简单实现：返回所有CPU
    *cpu_mask = 0xFFFFFFFF;
    (void)irq;
    return true;
}

/**
 * @brief 检查中断是否挂起
 */
static bool windows_interrupt_controller_is_pending(CN_InterruptNumber_t irq)
{
    // 简单实现：总是返回false
    (void)irq;
    return false;
}

/**
 * @brief 确认中断
 */
static bool windows_interrupt_controller_acknowledge(CN_InterruptNumber_t irq)
{
    // 简单实现：总是成功
    (void)irq;
    return true;
}

/**
 * @brief 发送中断结束信号
 */
static bool windows_interrupt_controller_send_eoi(CN_InterruptNumber_t irq)
{
    // 简单实现：总是成功
    (void)irq;
    return true;
}

/**
 * @brief 进入临界区
 */
static bool windows_interrupt_controller_enter_critical(void)
{
    // 简单实现：总是成功
    return true;
}

/**
 * @brief 退出临界区
 */
static bool windows_interrupt_controller_exit_critical(void)
{
    // 简单实现：总是成功
    return true;
}

/**
 * @brief 保存中断状态
 */
static bool windows_interrupt_controller_save_interrupt_state(CN_InterruptFlags_t* flags)
{
    if (!flags) {
        return false;
    }
    
    // 简单实现：返回0
    *flags = 0;
    return true;
}

/**
 * @brief 恢复中断状态
 */
static bool windows_interrupt_controller_restore_interrupt_state(CN_InterruptFlags_t flags)
{
    // 简单实现：总是成功
    (void)flags;
    return true;
}

/**
 * @brief 获取中断统计信息
 */
static bool windows_interrupt_controller_get_statistics(CN_InterruptNumber_t irq, Stru_CN_InterruptStatistics_t* stats)
{
    if (!stats) {
        return false;
    }
    
    // 简单实现：返回空统计
    memset(stats, 0, sizeof(Stru_CN_InterruptStatistics_t));
    (void)irq;
    return true;
}

/**
 * @brief 重置中断统计信息
 */
static bool windows_interrupt_controller_reset_statistics(CN_InterruptNumber_t irq)
{
    // 简单实现：总是成功
    (void)irq;
    return true;
}

/**
 * @brief 启用中断唤醒功能
 */
static bool windows_interrupt_controller_enable_wakeup(CN_InterruptNumber_t irq)
{
    // 简单实现：总是成功
    (void)irq;
    return true;
}

/**
 * @brief 禁用中断唤醒功能
 */
static bool windows_interrupt_controller_disable_wakeup(CN_InterruptNumber_t irq)
{
    // 简单实现：总是成功
    (void)irq;
    return true;
}

/**
 * @brief 检查中断唤醒功能是否启用
 */
static bool windows_interrupt_controller_is_wakeup_enabled(CN_InterruptNumber_t irq)
{
    // 简单实现：总是返回false
    (void)irq;
    return false;
}

/**
 * @brief 转储控制器状态
 */
static bool windows_interrupt_controller_dump_state(void* output_buffer, size_t buffer_size)
{
    // 简单实现：总是成功
    (void)output_buffer;
    (void)buffer_size;
    return true;
}

/**
 * @brief 验证控制器配置
 */
static bool windows_interrupt_controller_validate_configuration(void)
{
    // 简单实现：总是成功
    return true;
}

/**
 * @brief 获取Windows平台中断控制器接口
 */
Stru_CN_InterruptControllerInterface_t* CN_platform_windows_get_interrupt_controller(void)
{
    return &g_windows_interrupt_controller_interface;
}

/**
 * @brief 初始化Windows平台中断处理系统
 */
bool CN_platform_windows_interrupt_initialize(void)
{
    // 初始化中断控制器
    if (!windows_interrupt_controller_initialize()) {
        return false;
    }
    
    // 这里可以添加其他组件的初始化
    // 例如：中断管理器、中断工具等
    
    return true;
}

/**
 * @brief 清理Windows平台中断处理系统
 */
void CN_platform_windows_interrupt_cleanup(void)
{
    windows_interrupt_controller_cleanup();
    
    // 这里可以添加其他组件的清理
    // 例如：中断管理器、中断工具等
}

/**
 * @brief 检查Windows平台中断处理是否已初始化
 */
bool CN_platform_windows_interrupt_is_initialized(void)
{
    return windows_interrupt_controller_is_initialized();
}
