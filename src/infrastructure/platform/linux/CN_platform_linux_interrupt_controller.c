/******************************************************************************
 * 文件名: CN_platform_linux_interrupt_controller.c
 * 功能: CN_Language Linux平台中断控制器实现
 * 描述: Linux平台中断控制器子模块，负责中断的注册、启用、禁用、优先级管理等
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Linux平台中断控制器
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_linux_interrupt.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* 内部数据结构定义 */

/** Linux中断控制器内部状态 */
static Stru_CN_LinuxInterruptControllerInfo_t g_linux_interrupt_controller_info = {
    .is_initialized = false,
    .handler_count = 0,
    .global_lock = NULL,
    .blocked_signals = NULL,
    .original_signal_mask = NULL
};

/* 静态函数声明 */

static bool linux_interrupt_controller_initialize(void);
static void linux_interrupt_controller_cleanup(void);
static bool linux_interrupt_controller_get_info(Stru_CN_InterruptControllerInfo_t* info);
static bool linux_interrupt_controller_is_initialized(void);
static bool linux_interrupt_controller_setup_vector_table(void* table_addr, size_t table_size);
static bool linux_interrupt_controller_get_vector_table(void** table_addr, size_t* table_size);
static bool linux_interrupt_controller_register_handler(CN_InterruptNumber_t irq, 
                                                       CN_InterruptHandler_t handler, 
                                                       void* context, 
                                                       const Stru_CN_InterruptDescriptor_t* desc);
static bool linux_interrupt_controller_unregister_handler(CN_InterruptNumber_t irq);
static bool linux_interrupt_controller_enable_interrupt(CN_InterruptNumber_t irq);
static bool linux_interrupt_controller_disable_interrupt(CN_InterruptNumber_t irq);
static bool linux_interrupt_controller_is_interrupt_enabled(CN_InterruptNumber_t irq);
static bool linux_interrupt_controller_mask_interrupt(CN_InterruptNumber_t irq);
static bool linux_interrupt_controller_unmask_interrupt(CN_InterruptNumber_t irq);
static bool linux_interrupt_controller_is_interrupt_masked(CN_InterruptNumber_t irq);
static bool linux_interrupt_controller_set_priority(CN_InterruptNumber_t irq, CN_InterruptPriority_t priority);
static bool linux_interrupt_controller_get_priority(CN_InterruptNumber_t irq, CN_InterruptPriority_t* priority);
static bool linux_interrupt_controller_set_trigger_mode(CN_InterruptNumber_t irq, Eum_CN_InterruptTriggerMode_t mode);
static bool linux_interrupt_controller_set_polarity(CN_InterruptNumber_t irq, Eum_CN_InterruptPolarity_t polarity);
static bool linux_interrupt_controller_set_affinity(CN_InterruptNumber_t irq, uint32_t cpu_mask);
static bool linux_interrupt_controller_get_affinity(CN_InterruptNumber_t irq, uint32_t* cpu_mask);
static bool linux_interrupt_controller_is_pending(CN_InterruptNumber_t irq);
static bool linux_interrupt_controller_acknowledge(CN_InterruptNumber_t irq);
static bool linux_interrupt_controller_send_eoi(CN_InterruptNumber_t irq);
static bool linux_interrupt_controller_enter_critical(void);
static bool linux_interrupt_controller_exit_critical(void);
static bool linux_interrupt_controller_save_interrupt_state(CN_InterruptFlags_t* flags);
static bool linux_interrupt_controller_restore_interrupt_state(CN_InterruptFlags_t flags);
static bool linux_interrupt_controller_get_statistics(CN_InterruptNumber_t irq, Stru_CN_InterruptStatistics_t* stats);
static bool linux_interrupt_controller_reset_statistics(CN_InterruptNumber_t irq);
static bool linux_interrupt_controller_enable_wakeup(CN_InterruptNumber_t irq);
static bool linux_interrupt_controller_disable_wakeup(CN_InterruptNumber_t irq);
static bool linux_interrupt_controller_is_wakeup_enabled(CN_InterruptNumber_t irq);
static bool linux_interrupt_controller_dump_state(void* output_buffer, size_t buffer_size);
static bool linux_interrupt_controller_validate_configuration(void);

/* 全局接口变量定义 */

/** Linux中断控制器接口 */
Stru_CN_InterruptControllerInterface_t g_linux_interrupt_controller_interface = {
    .initialize = linux_interrupt_controller_initialize,
    .cleanup = linux_interrupt_controller_cleanup,
    .get_info = linux_interrupt_controller_get_info,
    .is_initialized = linux_interrupt_controller_is_initialized,
    .setup_vector_table = linux_interrupt_controller_setup_vector_table,
    .get_vector_table = linux_interrupt_controller_get_vector_table,
    .register_handler = linux_interrupt_controller_register_handler,
    .unregister_handler = linux_interrupt_controller_unregister_handler,
    .enable_interrupt = linux_interrupt_controller_enable_interrupt,
    .disable_interrupt = linux_interrupt_controller_disable_interrupt,
    .is_interrupt_enabled = linux_interrupt_controller_is_interrupt_enabled,
    .mask_interrupt = linux_interrupt_controller_mask_interrupt,
    .unmask_interrupt = linux_interrupt_controller_unmask_interrupt,
    .is_interrupt_masked = linux_interrupt_controller_is_interrupt_masked,
    .set_priority = linux_interrupt_controller_set_priority,
    .get_priority = linux_interrupt_controller_get_priority,
    .set_trigger_mode = linux_interrupt_controller_set_trigger_mode,
    .set_polarity = linux_interrupt_controller_set_polarity,
    .set_affinity = linux_interrupt_controller_set_affinity,
    .get_affinity = linux_interrupt_controller_get_affinity,
    .is_pending = linux_interrupt_controller_is_pending,
    .acknowledge = linux_interrupt_controller_acknowledge,
    .send_eoi = linux_interrupt_controller_send_eoi,
    .enter_critical = linux_interrupt_controller_enter_critical,
    .exit_critical = linux_interrupt_controller_exit_critical,
    .save_interrupt_state = linux_interrupt_controller_save_interrupt_state,
    .restore_interrupt_state = linux_interrupt_controller_restore_interrupt_state,
    .get_statistics = linux_interrupt_controller_get_statistics,
    .reset_statistics = linux_interrupt_controller_reset_statistics,
    .enable_wakeup = linux_interrupt_controller_enable_wakeup,
    .disable_wakeup = linux_interrupt_controller_disable_wakeup,
    .is_wakeup_enabled = linux_interrupt_controller_is_wakeup_enabled,
    .dump_state = linux_interrupt_controller_dump_state,
    .validate_configuration = linux_interrupt_controller_validate_configuration
};

/******************************************************************************
 * 中断控制器函数实现
 ******************************************************************************/

/**
 * @brief 初始化Linux中断控制器
 */
static bool linux_interrupt_controller_initialize(void)
{
    if (g_linux_interrupt_controller_info.is_initialized) {
        return true;
    }
    
    // 初始化全局锁
    pthread_mutex_init(&g_linux_interrupt_controller_info.global_lock, NULL);
    
    pthread_mutex_lock(&g_linux_interrupt_controller_info.global_lock);
    
    // 初始化处理程序数组
    for (uint32_t i = 0; i < CN_LINUX_MAX_INTERRUPT_HANDLERS; i++) {
        g_linux_interrupt_controller_info.handlers[i].interrupt_number = 0;
        g_linux_interrupt_controller_info.handlers[i].handler = NULL;
        g_linux_interrupt_controller_info.handlers[i].user_data = NULL;
        g_linux_interrupt_controller_info.handlers[i].name[0] = '\0';
        g_linux_interrupt_controller_info.handlers[i].flags = 0;
        g_linux_interrupt_controller_info.handlers[i].is_installed = false;
        pthread_mutex_init(&g_linux_interrupt_controller_info.handlers[i].lock, NULL);
    }
    
    g_linux_interrupt_controller_info.handler_count = 0;
    
    // 初始化信号掩码
    sigemptyset(&g_linux_interrupt_controller_info.blocked_signals);
    sigemptyset(&g_linux_interrupt_controller_info.original_signal_mask);
    
    // 保存原始信号掩码
    if (sigprocmask(SIG_BLOCK, NULL, &g_linux_interrupt_controller_info.original_signal_mask) != 0) {
        pthread_mutex_unlock(&g_linux_interrupt_controller_info.global_lock);
        return false;
    }
    
    g_linux_interrupt_controller_info.is_initialized = true;
    
    pthread_mutex_unlock(&g_linux_interrupt_controller_info.global_lock);
    
    return true;
}

/**
 * @brief 清理Linux中断控制器
 */
static void linux_interrupt_controller_cleanup(void)
{
    if (!g_linux_interrupt_controller_info.is_initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_linux_interrupt_controller_info.global_lock);
    
    // 恢复所有信号处理程序
    for (uint32_t i = 0; i < CN_LINUX_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_linux_interrupt_controller_info.handlers[i].is_installed) {
            int signum = CN_platform_linux_interrupt_to_signal(
                g_linux_interrupt_controller_info.handlers[i].interrupt_number);
            if (signum > 0) {
                // 恢复默认信号处理
                struct sigaction sa;
                memset(&sa, 0, sizeof(sa));
                sa.sa_handler = SIG_DFL;
                sigaction(signum, &sa, NULL);
            }
            
            pthread_mutex_destroy(&g_linux_interrupt_controller_info.handlers[i].lock);
        }
    }
    
    // 恢复原始信号掩码
    sigprocmask(SIG_SETMASK, &g_linux_interrupt_controller_info.original_signal_mask, NULL);
    
    g_linux_interrupt_controller_info.is_initialized = false;
    g_linux_interrupt_controller_info.handler_count = 0;
    
    pthread_mutex_unlock(&g_linux_interrupt_controller_info.global_lock);
    pthread_mutex_destroy(&g_linux_interrupt_controller_info.global_lock);
}

/**
 * @brief 获取中断控制器信息
 */
static bool linux_interrupt_controller_get_info(Stru_CN_InterruptControllerInfo_t* info)
{
    if (!info || !g_linux_interrupt_controller_info.is_initialized) {
        return false;
    }
    
    memset(info, 0, sizeof(Stru_CN_InterruptControllerInfo_t));
    
    info->type = Eum_CN_INTERRUPT_CONTROLLER_CUSTOM;
    info->max_interrupts = CN_LINUX_MAX_INTERRUPT_HANDLERS;
    info->supported_features = CN_INTERRUPT_FEATURE_MASKING | 
                              CN_INTERRUPT_FEATURE_PRIORITY |
                              CN_INTERRUPT_FEATURE_SHARING;
    info->handler_count = g_linux_interrupt_controller_info.handler_count;
    info->is_initialized = true;
    
    return true;
}

/**
 * @brief 检查中断控制器是否已初始化
 */
static bool linux_interrupt_controller_is_initialized(void)
{
    return g_linux_interrupt_controller_info.is_initialized;
}

/**
 * @brief 设置中断向量表
 */
static bool linux_interrupt_controller_setup_vector_table(void* table_addr, size_t table_size)
{
    // Linux平台不支持硬件中断向量表
    // 此函数仅用于接口兼容性
    (void)table_addr;
    (void)table_size;
    return true;
}

/**
 * @brief 获取中断向量表
 */
static bool linux_interrupt_controller_get_vector_table(void** table_addr, size_t* table_size)
{
    // Linux平台不支持硬件中断向量表
    if (table_addr) *table_addr = NULL;
    if (table_size) *table_size = 0;
    return true;
}

/**
 * @brief 注册中断处理程序
 */
static bool linux_interrupt_controller_register_handler(CN_InterruptNumber_t interrupt,
                                                       CN_InterruptHandler_t handler,
                                                       void* user_data,
                                                       const char* name,
                                                       CN_InterruptFlags_t flags)
{
    if (!g_linux_interrupt_controller_info.is_initialized || !handler) {
        return false;
    }
    
    // 转换为Linux信号
    int signum = CN_platform_linux_interrupt_to_signal(interrupt);
    if (signum <= 0) {
        return false;
    }
    
    // 检查信号是否可用
    if (!CN_platform_linux_signal_is_available(signum)) {
        return false;
    }
    
    pthread_mutex_lock(&g_linux_interrupt_controller_info.global_lock);
    
    // 查找现有的处理程序或空闲槽位
    Stru_CN_LinuxInterruptHandlerInfo_t* handler_info = NULL;
    for (uint32_t i = 0; i < CN_LINUX_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_linux_interrupt_controller_info.handlers[i].interrupt_number == interrupt) {
            handler_info = &g_linux_interrupt_controller_info.handlers[i];
            break;
        }
    }
    
    if (!handler_info) {
        // 查找空闲槽位
        for (uint32_t i = 0; i < CN_LINUX_MAX_INTERRUPT_HANDLERS; i++) {
            if (!g_linux_interrupt_controller_info.handlers[i].is_installed) {
                handler_info = &g_linux_interrupt_controller_info.handlers[i];
                break;
            }
        }
        
        if (!handler_info) {
            pthread_mutex_unlock(&g_linux_interrupt_controller_info.global_lock);
            return false;
        }
    }
    
    // 如果已经安装了处理程序，先恢复旧的
    if (handler_info->is_installed) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_DFL;
        sigaction(signum, &sa, NULL);
    }
    
    // 安装新的信号处理程序
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = (void (*)(int))handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(signum, &sa, NULL) != 0) {
        pthread_mutex_unlock(&g_linux_interrupt_controller_info.global_lock);
        return false;
    }
    
    // 更新处理程序信息
    pthread_mutex_lock(&handler_info->lock);
    
    handler_info->interrupt_number = interrupt;
    handler_info->handler = handler;
    handler_info->user_data = user_data;
    handler_info->flags = flags;
    handler_info->is_installed = true;
    
    if (name) {
        strncpy(handler_info->name, name, CN_LINUX_MAX_INTERRUPT_NAME - 1);
        handler_info->name[CN_LINUX_MAX_INTERRUPT_NAME - 1] = '\0';
    } else {
        snprintf(handler_info->name, CN_LINUX_MAX_INTERRUPT_NAME, 
                "interrupt_%u", (unsigned int)interrupt);
    }
    
    if (handler_info->interrupt_number == 0) {
        g_linux_interrupt_controller_info.handler_count++;
    }
    
    pthread_mutex_unlock(&handler_info->lock);
    pthread_mutex_unlock(&g_linux_interrupt_controller_info.global_lock);
    
    return true;
}

/**
 * @brief 注销中断处理程序
 */
static bool linux_interrupt_controller_unregister_handler(CN_InterruptNumber_t interrupt)
{
    if (!g_linux_interrupt_controller_info.is_initialized) {
        return false;
    }
    
    pthread_mutex_lock(&g_linux_interrupt_controller_info.global_lock);
    
    // 查找处理程序
    Stru_CN_LinuxInterruptHandlerInfo_t* handler_info = NULL;
    for (uint32_t i = 0; i < CN_LINUX_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_linux_interrupt_controller_info.handlers[i].interrupt_number == interrupt &&
            g_linux_interrupt_controller_info.handlers[i].is_installed) {
            handler_info = &g_linux_interrupt_controller_info.handlers[i];
            break;
        }
    }
    
    if (!handler_info) {
        pthread_mutex_unlock(&g_linux_interrupt_controller_info.global_lock);
        return false;
    }
    
    // 转换为Linux信号
    int signum = CN_platform_linux_interrupt_to_signal(interrupt);
    if (signum > 0) {
        // 恢复默认信号处理
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_DFL;
        sigaction(signum, &sa, NULL);
    }
    
    // 清除处理程序信息
    pthread_mutex_lock(&handler_info->lock);
    
    handler_info->interrupt_number = 0;
    handler_info->handler = NULL;
    handler_info->user_data = NULL;
    handler_info->name[0] = '\0';
    handler_info->flags = 0;
    handler_info->is_installed = false;
    
    g_linux_interrupt_controller_info.handler_count--;
    
    pthread_mutex_unlock(&handler_info->lock);
    pthread
