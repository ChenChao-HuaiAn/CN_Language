/******************************************************************************
 * 文件名: CN_platform_linux_interrupt.c
 * 功能: CN_Language Linux平台中断处理实现
 * 描述: 基于Linux信号机制的中断处理实现，提供信号到中断的映射、
 *       中断处理程序注册、中断优先级管理等功能。
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Linux平台中断处理
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_linux_interrupt.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

/* 内部数据结构定义 */

/** Linux中断控制器内部状态 */
static Stru_CN_LinuxInterruptControllerInfo_t g_linux_interrupt_controller_info = {
    .is_initialized = false,
    .handler_count = 0,
    .global_lock = NULL,
    .blocked_signals = NULL,
    .original_signal_mask = NULL
};

/** Linux中断管理器内部状态 */
static Stru_CN_LinuxInterruptManagerInfo_t g_linux_interrupt_manager_info = {
    .is_initialized = false,
    .controller = &g_linux_interrupt_controller_info,
    .allocated_count = 0,
    .allocation_lock = NULL
};

/** Linux中断工具内部状态 */
static Stru_CN_LinuxInterruptToolsInfo_t g_linux_interrupt_tools_info = {
    .is_initialized = false,
    .in_interrupt_context = false,
    .interrupt_context_key = NULL,
    .interrupt_count = 0,
    .nested_interrupt_depth = 0
};

/* 静态函数声明 */

// 中断控制器函数
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

// 中断管理器函数
static bool linux_interrupt_manager_initialize(void);
static void linux_interrupt_manager_cleanup(void);
static bool linux_interrupt_manager_register_controller(const char* name, 
                                                       Stru_CN_InterruptControllerInterface_t* controller);
static bool linux_interrupt_manager_unregister_controller(const char* name);
static Stru_CN_InterruptControllerInterface_t* linux_interrupt_manager_get_controller(const char* name);
static bool linux_interrupt_manager_allocate_interrupt(CN_InterruptNumber_t* irq, 
                                                      const Stru_CN_InterruptDescriptor_t* desc);
static bool linux_interrupt_manager_free_interrupt(CN_InterruptNumber_t irq);
static bool linux_interrupt_manager_route_interrupt(CN_InterruptNumber_t source_irq, 
                                                   CN_InterruptNumber_t dest_irq, 
                                                   const char* controller_name);
static bool linux_interrupt_manager_unroute_interrupt(CN_InterruptNumber_t irq);
static bool linux_interrupt_manager_share_interrupt(CN_InterruptNumber_t irq, 
                                                   CN_InterruptHandler_t handler, 
                                                   void* context);
static bool linux_interrupt_manager_unshare_interrupt(CN_InterruptNumber_t irq, 
                                                     CN_InterruptHandler_t handler);
static bool linux_interrupt_manager_chain_handlers(CN_InterruptNumber_t irq, 
                                                  CN_InterruptHandler_t* handlers, 
                                                  size_t count);
static bool linux_interrupt_manager_unchain_handlers(CN_InterruptNumber_t irq);
static bool linux_interrupt_manager_enable_all_interrupts(void);
static bool linux_interrupt_manager_disable_all_interrupts(void);
static bool linux_interrupt_manager_suspend_interrupts(void);
static bool linux_interrupt_manager_resume_interrupts(void);
static bool linux_interrupt_manager_wait_for_interrupt(CN_InterruptNumber_t irq, uint32_t timeout_ms);
static bool linux_interrupt_manager_poll_interrupt(CN_InterruptNumber_t irq, bool* occurred);
static bool linux_interrupt_manager_simulate_interrupt(CN_InterruptNumber_t irq);
static bool linux_interrupt_manager_load_configuration(const char* config_file);
static bool linux_interrupt_manager_save_configuration(const char* config_file);
static bool linux_interrupt_manager_diagnose(CN_InterruptNumber_t irq, void* report_buffer, size_t buffer_size);
static bool linux_interrupt_manager_trace_interrupt(CN_InterruptNumber_t irq, bool enable);
static bool linux_interrupt_manager_start_monitoring(CN_InterruptNumber_t irq);
static bool linux_interrupt_manager_stop_monitoring(CN_InterruptNumber_t irq);
static bool linux_interrupt_manager_get_performance_data(CN_InterruptNumber_t irq, void* data_buffer, size_t buffer_size);

// 中断工具函数
static bool linux_interrupt_tools_save_context(Stru_CN_InterruptContext_t* context);
static bool linux_interrupt_tools_restore_context(const Stru_CN_InterruptContext_t* context);
static bool linux_interrupt_tools_copy_context(Stru_CN_InterruptContext_t* dest, const Stru_CN_InterruptContext_t* src);
static void* linux_interrupt_tools_allocate_interrupt_stack(size_t size);
static bool linux_interrupt_tools_free_interrupt_stack(void* stack);
static bool linux_interrupt_tools_check_stack_overflow(void* stack_base, size_t stack_size);
static bool linux_interrupt_tools_schedule_deferred(CN_InterruptHandler_t handler, void* context, uint32_t delay_ms);
static bool linux_interrupt_tools_cancel_deferred(CN_InterruptHandler_t handler);
static bool linux_interrupt_tools_process_deferred(void);
static bool linux_interrupt_tools_install_filter(CN_InterruptNumber_t irq, 
                                                bool (*filter)(CN_InterruptNumber_t, void*), 
                                                void* filter_context);
static bool linux_interrupt_tools_remove_filter(CN_InterruptNumber_t irq);
static bool linux_interrupt_tools_enable_logging(CN_InterruptNumber_t irq, uint32_t log_level);
static bool linux_interrupt_tools_disable_logging(CN_InterruptNumber_t irq);
static bool linux_interrupt_tools_get_log_entries(CN_InterruptNumber_t irq, void* log_buffer, size_t buffer_size, size_t* entry_count);
static bool linux_interrupt_tools_start_profiling(CN_InterruptNumber_t irq);
static bool linux_interrupt_tools_stop_profiling(CN_InterruptNumber_t irq);
static bool linux_interrupt_tools_get_profile_data(CN_InterruptNumber_t irq, void* profile_buffer, size_t buffer_size);

// 信号处理函数
static void linux_signal_handler(int signum);
static bool linux_convert_signal_to_interrupt(int signum, CN_InterruptNumber_t* interrupt);
static bool linux_convert_interrupt_to_signal(CN_InterruptNumber_t interrupt, int* signum);
static bool linux_install_signal_handler(int signum);
static bool linux_restore_signal_handler(int signum);

// 工具函数
static Stru_CN_LinuxInterruptHandlerInfo_t* linux_find_handler_by_interrupt(CN_InterruptNumber_t interrupt);
static Stru_CN_LinuxInterruptHandlerInfo_t* linux_find_free_handler_slot(void);
static bool linux_is_signal_available(int signum);
static void linux_init_default_signal_mask(sigset_t* mask);

/* 全局接口变量定义 */

/** Linux中断控制器接口 */
static Stru_CN_InterruptControllerInterface_t g_linux_interrupt_controller_interface = {
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

/** Linux中断管理器接口 */
static Stru_CN_InterruptManagerInterface_t g_linux_interrupt_manager_interface = {
    .initialize = linux_interrupt_manager_initialize,
    .cleanup = linux_interrupt_manager_cleanup,
    .register_controller = linux_interrupt_manager_register_controller,
    .unregister_controller = linux_interrupt_manager_unregister_controller,
    .get_controller = linux_interrupt_manager_get_controller,
    .allocate_interrupt = linux_interrupt_manager_allocate_interrupt,
    .free_interrupt = linux_interrupt_manager_free_interrupt,
    .route_interrupt = linux_interrupt_manager_route_interrupt,
    .unroute_interrupt = linux_interrupt_manager_unroute_interrupt,
    .share_interrupt = linux_interrupt_manager_share_interrupt,
    .unshare_interrupt = linux_interrupt_manager_unshare_interrupt,
    .chain_handlers = linux_interrupt_manager_chain_handlers,
    .unchain_handlers = linux_interrupt_manager_unchain_handlers,
    .enable_all_interrupts = linux_interrupt_manager_enable_all_interrupts,
    .disable_all_interrupts = linux_interrupt_manager_disable_all_interrupts,
    .suspend_interrupts = linux_interrupt_manager_suspend_interrupts,
    .resume_interrupts = linux_interrupt_manager_resume_interrupts,
    .wait_for_interrupt = linux_interrupt_manager_wait_for_interrupt,
    .poll_interrupt = linux_interrupt_manager_poll_interrupt,
    .simulate_interrupt = linux_interrupt_manager_simulate_interrupt,
    .load_configuration = linux_interrupt_manager_load_configuration,
    .save_configuration = linux_interrupt_manager_save_configuration,
    .diagnose = linux_interrupt_manager_diagnose,
    .trace_interrupt = linux_interrupt_manager_trace_interrupt,
    .start_monitoring = linux_interrupt_manager_start_monitoring,
    .stop_monitoring = linux_interrupt_manager_stop_monitoring,
    .get_performance_data = linux_interrupt_manager_get_performance_data
};

/** Linux中断工具接口 */
static Stru_CN_InterruptToolsInterface_t g_linux_interrupt_tools_interface = {
    .save_context = linux_interrupt_tools_save_context,
    .restore_context = linux_interrupt_tools_restore_context,
    .copy_context = linux_interrupt_tools_copy_context,
    .allocate_interrupt_stack = linux_interrupt_tools_allocate_interrupt_stack,
    .free_interrupt_stack = linux_interrupt_tools_free_interrupt_stack,
    .check_stack_overflow = linux_interrupt_tools_check_stack_overflow,
    .schedule_deferred = linux_interrupt_tools_schedule_deferred,
    .cancel_deferred = linux_interrupt_tools_cancel_deferred,
    .process_deferred = linux_interrupt_tools_process_deferred,
    .install_filter = linux_interrupt_tools_install_filter,
    .remove_filter = linux_interrupt_tools_remove_filter,
    .enable_logging = linux_interrupt_tools_enable_logging,
    .disable_logging = linux_interrupt_tools_disable_logging,
    .get_log_entries = linux_interrupt_tools_get_log_entries,
    .start_profiling = linux_interrupt_tools_start_profiling,
    .stop_profiling = linux_interrupt_tools_stop_profiling,
    .get_profile_data = linux_interrupt_tools_get_profile_data
};

/* 模块初始化状态 */
static bool g_interrupt_module_initialized = false;

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
            int signum;
            if (linux_convert_interrupt_to_signal(
                g_linux_interrupt_controller_info.handlers[i].interrupt_number, &signum)) {
                linux_restore_signal_handler(signum);
            }
            
            pthread_mutex_destroy(&g_linux_interrupt_controller_info.handlers[i].lock);
        }
    }
    
    // 恢复原始信号掩码
    sigprocmask(SIG_SETMASK, &g_linux_interrupt_controller_info.original_signal_mask, NULL);
    
    g_linux_interrupt_controller_info.is_initialized = false;
    g_linux_interrupt_controller_info.handler_count = 0;
    
    pthread_mutex_unlock(&g_linux_interrupt_controller_info.global_lock);
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
    int signum;
    if (!linux_convert_interrupt_to_signal(interrupt, &signum)) {
        return false;
    }
    
    // 检查信号是否可用
    if (!linux_is_signal_available(signum)) {
        return false;
    }
    
    pthread_mutex_lock(&g_linux_interrupt_controller_info.global_lock);
    
    // 查找现有的处理程序或空闲槽位
    Stru_CN_LinuxInterruptHandlerInfo_t* handler_info = linux_find_handler_by_interrupt(interrupt);
    if (!handler_info) {
        handler_info = linux_find_free_handler_slot();
        if (!handler_info) {
            pthread_mutex_unlock(&g_linux_interrupt_controller_info.global_lock);
            return false;
        }
    }
    
    // 如果已经安装了处理程序，先恢复旧的
    if (handler_info->is_installed) {
        linux_restore_signal_handler(signum);
    }
    
    // 安装新的信号处理程序
    if (!linux_install_signal_handler(signum)) {
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
    
    Stru_CN_LinuxInterruptHandlerInfo_t* handler_info = linux_find_handler_by_interrupt(interrupt);
    if (!handler_info || !handler_info->is_installed) {
        pthread_mutex_unlock(&g_linux_interrupt_controller_info.global_lock);
        return false;
    }
    
    // 转换为Linux信号
    int signum;
    if (!linux_convert_interrupt_to_signal(interrupt, &signum)) {
        pthread_mutex_unlock(&g_linux_interrupt_controller_info.global_lock);
        return false;
    }
    
    // 恢复信号处理程序
    linux_restore_signal_handler(signum);
    
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
    pthread_mutex_unlock(&g_linux_interrupt_controller_info.global_lock);
    
    return true;
}

/**
 * @brief 启用中断
 */
static bool linux_interrupt_controller_enable_interrupt(CN_InterruptNumber_t interrupt)
{
    if (!g_linux_interrupt_controller_info.is_initialized) {
        return false;
    }
    
    // 转换为Linux信号
    int signum;
    if (!linux_convert_interrupt_to_signal(interrupt, &signum)) {
        return false;
    }
    
    // 从阻塞信号集中移除该信号
    pthread_mutex_lock(&g_linux_interrupt_controller_info.global_lock);
    sigdelset(&g_linux_interrupt_controller_info.blocked_signals, signum);
    
    // 应用新的信号掩码
    sigset_t current_mask;
    sigprocmask(SIG_BLOCK, NULL, &current_mask);
    sigdelset(&current_mask, signum);
    sigprocmask(SIG_SETMASK, &current_mask, NULL);
    
    pthread_mutex_unlock(&g_linux_interrupt_controller_info.global_lock);
    
    return true;
}

/**
 * @brief 禁用中断
 */
static bool linux_interrupt_controller_disable_interrupt(CN_InterruptNumber_t interrupt)
{
    if (!g_linux_interrupt_controller_info.is_initialized) {
        return false;
    }
    
    // 转换为Linux信号
    int signum;
    if (!linux_convert_interrupt_to_signal(interrupt, &signum)) {
        return false;
    }
    
    // 添加到阻塞信号集
    pthread_mutex_lock(&g_linux_interrupt_controller_info.global_lock);
    sigaddset(&g_linux_interrupt_controller_info.blocked_signals, signum);
    
    //
