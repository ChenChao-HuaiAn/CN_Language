/******************************************************************************
 * 文件名: CN_platform_windows_interrupt_manager.c
 * 功能: CN_Language Windows平台中断管理器实现
 * 描述: Windows平台中断管理器实现，提供中断分配、路由、共享、链式处理、
 *       中断监控和诊断等功能。
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Windows平台中断管理器
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_windows_interrupt.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 内部数据结构定义 */

/** Windows中断管理器内部状态 */
static struct {
    bool is_initialized;
    Stru_CN_InterruptControllerInterface_t* controller;  // 改为正确的类型
    CN_InterruptNumber_t allocated_interrupts[CN_WINDOWS_MAX_INTERRUPT_HANDLERS];
    uint32_t allocated_count;
    CRITICAL_SECTION allocation_lock;
} g_windows_interrupt_manager_info = {
    .is_initialized = false,
    .controller = NULL,
    .allocated_count = 0,
    .allocation_lock = {0}
};

/** 已注册的控制器列表 */
static struct {
    char name[64];
    Stru_CN_InterruptControllerInterface_t* controller;
} g_registered_controllers[16];
static uint32_t g_registered_controller_count = 0;

/** 中断路由表 */
static struct {
    CN_InterruptNumber_t source_irq;
    CN_InterruptNumber_t dest_irq;
    char controller_name[64];
    bool is_routed;
} g_interrupt_routes[CN_WINDOWS_MAX_INTERRUPT_HANDLERS];
static uint32_t g_route_count = 0;

/** 中断共享表 */
static struct {
    CN_InterruptNumber_t irq;
    CN_InterruptHandler_t handler;
    void* context;
    bool is_shared;
} g_shared_handlers[CN_WINDOWS_MAX_INTERRUPT_HANDLERS * 4];
static uint32_t g_shared_handler_count = 0;

/** 中断链式处理表 */
static struct {
    CN_InterruptNumber_t irq;
    CN_InterruptHandler_t handlers[8];
    size_t handler_count;
    bool is_chained;
} g_chained_handlers[CN_WINDOWS_MAX_INTERRUPT_HANDLERS];
static uint32_t g_chained_handler_count = 0;

/** 中断监控状态 */
static struct {
    CN_InterruptNumber_t irq;
    bool is_monitoring;
    uint64_t interrupt_count;
    uint64_t last_interrupt_time;
    uint64_t max_latency;
    uint64_t min_latency;
    uint64_t total_latency;
} g_monitored_interrupts[CN_WINDOWS_MAX_INTERRUPT_HANDLERS];
static uint32_t g_monitored_count = 0;

/* 静态函数声明 */

// 中断管理器函数
static bool windows_interrupt_manager_initialize(void);
static void windows_interrupt_manager_cleanup(void);
static bool windows_interrupt_manager_register_controller(const char* name, 
                                                         Stru_CN_InterruptControllerInterface_t* controller);
static bool windows_interrupt_manager_unregister_controller(const char* name);
static Stru_CN_InterruptControllerInterface_t* windows_interrupt_manager_get_controller(const char* name);
static bool windows_interrupt_manager_allocate_interrupt(CN_InterruptNumber_t* irq, 
                                                        const Stru_CN_InterruptDescriptor_t* desc);
static bool windows_interrupt_manager_free_interrupt(CN_InterruptNumber_t irq);
static bool windows_interrupt_manager_route_interrupt(CN_InterruptNumber_t source_irq, 
                                                     CN_InterruptNumber_t dest_irq, 
                                                     const char* controller_name);
static bool windows_interrupt_manager_unroute_interrupt(CN_InterruptNumber_t irq);
static bool windows_interrupt_manager_share_interrupt(CN_InterruptNumber_t irq, 
                                                     CN_InterruptHandler_t handler, 
                                                     void* context);
static bool windows_interrupt_manager_unshare_interrupt(CN_InterruptNumber_t irq, 
                                                       CN_InterruptHandler_t handler);
static bool windows_interrupt_manager_chain_handlers(CN_InterruptNumber_t irq, 
                                                    CN_InterruptHandler_t* handlers, 
                                                    size_t count);
static bool windows_interrupt_manager_unchain_handlers(CN_InterruptNumber_t irq);
static bool windows_interrupt_manager_enable_all_interrupts(void);
static bool windows_interrupt_manager_disable_all_interrupts(void);
static bool windows_interrupt_manager_suspend_interrupts(void);
static bool windows_interrupt_manager_resume_interrupts(void);
static bool windows_interrupt_manager_wait_for_interrupt(CN_InterruptNumber_t irq, uint32_t timeout_ms);
static bool windows_interrupt_manager_poll_interrupt(CN_InterruptNumber_t irq, bool* occurred);
static bool windows_interrupt_manager_simulate_interrupt(CN_InterruptNumber_t irq);
static bool windows_interrupt_manager_load_configuration(const char* config_file);
static bool windows_interrupt_manager_save_configuration(const char* config_file);
static bool windows_interrupt_manager_diagnose(CN_InterruptNumber_t irq, void* report_buffer, size_t buffer_size);
static bool windows_interrupt_manager_trace_interrupt(CN_InterruptNumber_t irq, bool enable);
static bool windows_interrupt_manager_start_monitoring(CN_InterruptNumber_t irq);
static bool windows_interrupt_manager_stop_monitoring(CN_InterruptNumber_t irq);
static bool windows_interrupt_manager_get_performance_data(CN_InterruptNumber_t irq, void* data_buffer, size_t buffer_size);

// 内部工具函数
static bool windows_is_interrupt_allocated(CN_InterruptNumber_t irq);
static bool windows_find_free_interrupt_slot(CN_InterruptNumber_t* irq);
static bool windows_is_controller_registered(const char* name);
static int windows_find_controller_index(const char* name);
static int windows_find_route_index(CN_InterruptNumber_t irq);
static int windows_find_shared_handler_index(CN_InterruptNumber_t irq, CN_InterruptHandler_t handler);
static int windows_find_chained_handler_index(CN_InterruptNumber_t irq);
static int windows_find_monitored_interrupt_index(CN_InterruptNumber_t irq);
static bool windows_validate_interrupt_number(CN_InterruptNumber_t irq);
static bool windows_load_config_from_file(const char* filename);
static bool windows_save_config_to_file(const char* filename);
static void windows_generate_diagnostic_report(CN_InterruptNumber_t irq, char* buffer, size_t buffer_size);

/* Windows中断管理器接口定义 */

/** Windows中断管理器接口 */
Stru_CN_InterruptManagerInterface_t g_windows_interrupt_manager_interface = {
    .initialize = windows_interrupt_manager_initialize,
    .cleanup = windows_interrupt_manager_cleanup,
    .register_controller = windows_interrupt_manager_register_controller,
    .unregister_controller = windows_interrupt_manager_unregister_controller,
    .get_controller = windows_interrupt_manager_get_controller,
    .allocate_interrupt = windows_interrupt_manager_allocate_interrupt,
    .free_interrupt = windows_interrupt_manager_free_interrupt,
    .route_interrupt = windows_interrupt_manager_route_interrupt,
    .unroute_interrupt = windows_interrupt_manager_unroute_interrupt,
    .share_interrupt = windows_interrupt_manager_share_interrupt,
    .unshare_interrupt = windows_interrupt_manager_unshare_interrupt,
    .chain_handlers = windows_interrupt_manager_chain_handlers,
    .unchain_handlers = windows_interrupt_manager_unchain_handlers,
    .enable_all_interrupts = windows_interrupt_manager_enable_all_interrupts,
    .disable_all_interrupts = windows_interrupt_manager_disable_all_interrupts,
    .suspend_interrupts = windows_interrupt_manager_suspend_interrupts,
    .resume_interrupts = windows_interrupt_manager_resume_interrupts,
    .wait_for_interrupt = windows_interrupt_manager_wait_for_interrupt,
    .poll_interrupt = windows_interrupt_manager_poll_interrupt,
    .simulate_interrupt = windows_interrupt_manager_simulate_interrupt,
    .load_configuration = windows_interrupt_manager_load_configuration,
    .save_configuration = windows_interrupt_manager_save_configuration,
    .diagnose = windows_interrupt_manager_diagnose,
    .trace_interrupt = windows_interrupt_manager_trace_interrupt,
    .start_monitoring = windows_interrupt_manager_start_monitoring,
    .stop_monitoring = windows_interrupt_manager_stop_monitoring,
    .get_performance_data = windows_interrupt_manager_get_performance_data
};

/******************************************************************************
 * 中断管理器函数实现
 ******************************************************************************/

/**
 * @brief 初始化Windows中断管理器
 */
static bool windows_interrupt_manager_initialize(void)
{
    if (g_windows_interrupt_manager_info.is_initialized) {
        return true;
    }
    
    // 初始化临界区
    InitializeCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    // 初始化已分配中断数组
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        g_windows_interrupt_manager_info.allocated_interrupts[i] = 0;
    }
    g_windows_interrupt_manager_info.allocated_count = 0;
    
    // 初始化控制器列表
    for (uint32_t i = 0; i < 16; i++) {
        g_registered_controllers[i].name[0] = '\0';
        g_registered_controllers[i].controller = NULL;
    }
    g_registered_controller_count = 0;
    
    // 初始化路由表
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        g_interrupt_routes[i].source_irq = 0;
        g_interrupt_routes[i].dest_irq = 0;
        g_interrupt_routes[i].controller_name[0] = '\0';
        g_interrupt_routes[i].is_routed = false;
    }
    g_route_count = 0;
    
    // 初始化共享处理程序表
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS * 4; i++) {
        g_shared_handlers[i].irq = 0;
        g_shared_handlers[i].handler = NULL;
        g_shared_handlers[i].context = NULL;
        g_shared_handlers[i].is_shared = false;
    }
    g_shared_handler_count = 0;
    
    // 初始化链式处理程序表
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        g_chained_handlers[i].irq = 0;
        for (uint32_t j = 0; j < 8; j++) {
            g_chained_handlers[i].handlers[j] = NULL;
        }
        g_chained_handlers[i].handler_count = 0;
        g_chained_handlers[i].is_chained = false;
    }
    g_chained_handler_count = 0;
    
    // 初始化监控表
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        g_monitored_interrupts[i].irq = 0;
        g_monitored_interrupts[i].is_monitoring = false;
        g_monitored_interrupts[i].interrupt_count = 0;
        g_monitored_interrupts[i].last_interrupt_time = 0;
        g_monitored_interrupts[i].max_latency = 0;
        g_monitored_interrupts[i].min_latency = UINT64_MAX;
        g_monitored_interrupts[i].total_latency = 0;
    }
    g_monitored_count = 0;
    
    // 设置默认控制器
    g_windows_interrupt_manager_info.controller = NULL;
    
    g_windows_interrupt_manager_info.is_initialized = true;
    
    return true;
}

/**
 * @brief 清理Windows中断管理器
 */
static void windows_interrupt_manager_cleanup(void)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    // 停止所有监控
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_monitored_interrupts[i].is_monitoring) {
            g_monitored_interrupts[i].is_monitoring = false;
        }
    }
    
    // 清理所有链式处理程序
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_chained_handlers[i].is_chained) {
            g_chained_handlers[i].is_chained = false;
            g_chained_handlers[i].handler_count = 0;
        }
    }
    
    // 清理所有共享处理程序
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS * 4; i++) {
        if (g_shared_handlers[i].is_shared) {
            g_shared_handlers[i].is_shared = false;
        }
    }
    
    // 清理所有路由
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_interrupt_routes[i].is_routed) {
            g_interrupt_routes[i].is_routed = false;
        }
    }
    
    // 注销所有控制器
    for (uint32_t i = 0; i < 16; i++) {
        if (g_registered_controllers[i].controller != NULL) {
            g_registered_controllers[i].controller = NULL;
            g_registered_controllers[i].name[0] = '\0';
        }
    }
    g_registered_controller_count = 0;
    
    // 清理已分配中断
    g_windows_interrupt_manager_info.allocated_count = 0;
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    // 删除临界区
    DeleteCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    g_windows_interrupt_manager_info.is_initialized = false;
}

/**
 * @brief 注册中断控制器
 */
static bool windows_interrupt_manager_register_controller(const char* name, 
                                                         Stru_CN_InterruptControllerInterface_t* controller)
{
    if (!g_windows_interrupt_manager_info.is_initialized || !name || !controller) {
        return false;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    // 检查是否已注册
    if (windows_is_controller_registered(name)) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 查找空闲槽位
    int free_index = -1;
    for (uint32_t i = 0; i < 16; i++) {
        if (g_registered_controllers[i].controller == NULL) {
            free_index = i;
            break;
        }
    }
    
    if (free_index == -1) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 注册控制器
    strncpy(g_registered_controllers[free_index].name, name, 63);
    g_registered_controllers[free_index].name[63] = '\0';
    g_registered_controllers[free_index].controller = controller;
    g_registered_controller_count++;
    
    // 如果是第一个控制器，设置为默认控制器
    if (g_windows_interrupt_manager_info.controller == NULL) {
        g_windows_interrupt_manager_info.controller = controller;
    }
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    return true;
}

/**
 * @brief 注销中断控制器
 */
static bool windows_interrupt_manager_unregister_controller(const char* name)
{
    if (!g_windows_interrupt_manager_info.is_initialized || !name) {
        return false;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    int index = windows_find_controller_index(name);
    if (index == -1) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 如果这是默认控制器，需要重新选择默认控制器
    if (g_windows_interrupt_manager_info.controller == g_registered_controllers[index].controller) {
        g_windows_interrupt_manager_info.controller = NULL;
        
        // 选择另一个控制器作为默认
        for (uint32_t i = 0; i < 16; i++) {
            if (i != (uint32_t)index && g_registered_controllers[i].controller != NULL) {
                g_windows_interrupt_manager_info.controller = g_registered_controllers[i].controller;
                break;
            }
        }
    }
    
    // 注销控制器
    g_registered_controllers[index].name[0] = '\0';
    g_registered_controllers[index].controller = NULL;
    g_registered_controller_count--;
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    return true;
}

/**
 * @brief 获取中断控制器
 */
static Stru_CN_InterruptControllerInterface_t* windows_interrupt_manager_get_controller(const char* name)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return NULL;
    }
    
    if (name == NULL) {
        return g_windows_interrupt_manager_info.controller;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    int index = windows_find_controller_index(name);
    Stru_CN_InterruptControllerInterface_t* controller = NULL;
    
    if (index != -1) {
        controller = g_registered_controllers[index].controller;
    }
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    return controller;
}

/**
 * @brief 分配中断
 */
static bool windows_interrupt_manager_allocate_interrupt(CN_InterruptNumber_t* irq, 
                                                        const Stru_CN_InterruptDescriptor_t* desc)
{
    if (!g_windows_interrupt_manager_info.is_initialized || !irq) {
        return false;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    // 查找空闲中断槽位
    bool found = false;
    for (CN_InterruptNumber_t i = 1; i <= CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (!windows_is_interrupt_allocated(i)) {
            *irq = i;
            found = true;
            break;
        }
    }
    
    if (!found) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 添加到已分配列表
    g_windows_interrupt_manager_info.allocated_interrupts[g_windows_interrupt_manager_info.allocated_count] = *irq;
    g_windows_interrupt_manager_info.allocated_count++;
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    return true;
}

/**
 * @brief 释放中断
 */
static bool windows_interrupt_manager_free_interrupt(CN_InterruptNumber_t irq)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return false;
    }
    
    if (!windows_validate_interrupt_number(irq)) {
        return false;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    // 查找中断在已分配列表中的位置
    int index = -1;
    for (uint32_t i = 0; i < g_windows_interrupt_manager_info.allocated_count; i++) {
        if (g_windows_interrupt_manager_info.allocated_interrupts[i] == irq) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 从列表中移除
    for (uint32_t i = index; i < g_windows_interrupt_manager_info.allocated_count - 1; i++) {
        g_windows_interrupt_manager_info.allocated_interrupts[i] = 
            g_windows_interrupt_manager_info.allocated_interrupts[i + 1];
    }
    g_windows_interrupt_manager_info.allocated_count--;
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    return true;
}

/**
 * @brief 路由中断
 */
static bool windows_interrupt_manager_route_interrupt(CN_InterruptNumber_t source_irq, 
                                                     CN_InterruptNumber_t dest_irq, 
                                                     const char* controller_name)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return false;
    }
    
    if (!windows_validate_interrupt_number(source_irq) || 
        !windows_validate_interrupt_number(dest_irq) || 
        !controller_name) {
        return false;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    // 检查是否已存在路由
    int existing_index = windows_find_route_index(source_irq);
    if (existing_index != -1) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 查找空闲路由槽位
    int free_index = -1;
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (!g_interrupt_routes[i].is_routed) {
            free_index = i;
            break;
        }
    }
    
    if (free_index == -1) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 设置路由
    g_interrupt_routes[free_index].source_irq = source_irq;
    g_interrupt_routes[free_index].dest_irq = dest_irq;
    strncpy(g_interrupt_routes[free_index].controller_name, controller_name, 63);
    g_interrupt_routes[free_index].controller_name[63] = '\0';
    g_interrupt_routes[free_index].is_routed = true;
    g_route_count++;
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    return true;
}

/**
 * @brief 取消路由中断
 */
static bool windows_interrupt_manager_unroute_interrupt(CN_InterruptNumber_t irq)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return false;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    int index = windows_find_route_index(irq);
    if (index == -1) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 清除路由
    g_interrupt_routes[index].source_irq = 0;
    g_interrupt_routes[index].dest_irq = 0;
    g_interrupt_routes[index].controller_name[0] = '\0';
    g_interrupt_routes[index].is_routed = false;
    g_route_count--;
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    return true;
}

/**
 * @brief 共享中断
 */
static bool windows_interrupt_manager_share_interrupt(CN_InterruptNumber_t irq, 
                                                     CN_InterruptHandler_t handler, 
                                                     void* context)
{
    if (!g_windows_interrupt_manager_info.is_initialized || !handler) {
        return false;
    }
    
    if (!windows_validate_interrupt_number(irq)) {
        return false;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    // 检查是否已存在相同的共享处理程序
    int existing_index = windows_find_shared_handler_index(irq, handler);
    if (existing_index != -1) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 查找空闲共享槽位
    int free_index = -1;
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS * 4; i++) {
        if (!g_shared_handlers[i].is_shared) {
            free_index = i;
            break;
        }
    }
    
    if (free_index == -1) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 设置共享处理程序
    g_shared_handlers[free_index].irq = irq;
    g_shared_handlers[free_index].handler = handler;
    g_shared_handlers[free_index].context = context;
    g_shared_handlers[free_index].is_shared = true;
    g_shared_handler_count++;
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    return true;
}

/**
 * @brief 取消共享中断
 */
static bool windows_interrupt_manager_unshare_interrupt(CN_InterruptNumber_t irq, 
                                                       CN_InterruptHandler_t handler)
{
    if (!g_windows_interrupt_manager_info.is_initialized || !handler) {
        return false;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    int index = windows_find_shared_handler_index(irq, handler);
    if (index == -1) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 清除共享处理程序
    g_shared_handlers[index].irq = 0;
    g_shared_handlers[index].handler = NULL;
    g_shared_handlers[index].context = NULL;
    g_shared_handlers[index].is_shared = false;
    g_shared_handler_count--;
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    return true;
}

/**
 * @brief 链式处理程序
 */
static bool windows_interrupt_manager_chain_handlers(CN_InterruptNumber_t irq, 
                                                    CN_InterruptHandler_t* handlers, 
                                                    size_t count)
{
    if (!g_windows_interrupt_manager_info.is_initialized || !handlers || count == 0 || count > 8) {
        return false;
    }
    
    if (!windows_validate_interrupt_number(irq)) {
        return false;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    // 检查是否已存在链式处理程序
    int existing_index = windows_find_chained_handler_index(irq);
    if (existing_index != -1) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 查找空闲链式槽位
    int free_index = -1;
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (!g_chained_handlers[i].is_chained) {
            free_index = i;
            break;
        }
    }
    
    if (free_index == -1) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 设置链式处理程序
    g_chained_handlers[free_index].irq = irq;
    for (size_t i = 0; i < count; i++) {
        g_chained_handlers[free_index].handlers[i] = handlers[i];
    }
    g_chained_handlers[free_index].handler_count = count;
    g_chained_handlers[free_index].is_chained = true;
    g_chained_handler_count++;
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    return true;
}

/**
 * @brief 取消链式处理程序
 */
static bool windows_interrupt_manager_unchain_handlers(CN_InterruptNumber_t irq)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return false;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    int index = windows_find_chained_handler_index(irq);
    if (index == -1) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 清除链式处理程序
    g_chained_handlers[index].irq = 0;
    for (uint32_t i = 0; i < 8; i++) {
        g_chained_handlers[index].handlers[i] = NULL;
    }
    g_chained_handlers[index].handler_count = 0;
    g_chained_handlers[index].is_chained = false;
    g_chained_handler_count--;
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    return true;
}

/**
 * @brief 启用所有中断
 */
static bool windows_interrupt_manager_enable_all_interrupts(void)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return false;
    }
    
    // 获取默认控制器接口
    Stru_CN_InterruptControllerInterface_t* controller = 
        windows_interrupt_manager_get_controller(NULL);
    
    if (controller && controller->enable_interrupt) {
        // 遍历所有已分配的中断并启用它们
        EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        
        bool success = true;
        for (uint32_t i = 0; i < g_windows_interrupt_manager_info.allocated_count; i++) {
            CN_InterruptNumber_t irq = g_windows_interrupt_manager_info.allocated_interrupts[i];
            if (!controller->enable_interrupt(irq)) {
                success = false;
            }
        }
        
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return success;
    }
    
    return false;
}

/**
 * @brief 禁用所有中断
 */
static bool windows_interrupt_manager_disable_all_interrupts(void)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return false;
    }
    
    // 获取默认控制器接口
    Stru_CN_InterruptControllerInterface_t* controller = 
        windows_interrupt_manager_get_controller(NULL);
    
    if (controller && controller->disable_interrupt) {
        // 遍历所有已分配的中断并禁用它们
        EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        
        bool success = true;
        for (uint32_t i = 0; i < g_windows_interrupt_manager_info.allocated_count; i++) {
            CN_InterruptNumber_t irq = g_windows_interrupt_manager_info.allocated_interrupts[i];
            if (!controller->disable_interrupt(irq)) {
                success = false;
            }
        }
        
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return success;
    }
    
    return false;
}

/**
 * @brief 挂起中断
 */
static bool windows_interrupt_manager_suspend_interrupts(void)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return false;
    }
    
    // 保存当前中断状态
    CN_InterruptFlags_t flags = 0;
    
    // 这里需要实际的保存机制
    // 暂时禁用所有中断
    return windows_interrupt_manager_disable_all_interrupts();
}

/**
 * @brief 恢复中断
 */
static bool windows_interrupt_manager_resume_interrupts(void)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return false;
    }
    
    // 恢复之前保存的中断状态
    // 这里需要实际的恢复机制
    // 暂时启用所有中断
    return windows_interrupt_manager_enable_all_interrupts();
}

/**
 * @brief 等待中断
 */
static bool windows_interrupt_manager_wait_for_interrupt(CN_InterruptNumber_t irq, uint32_t timeout_ms)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return false;
    }
    
    if (!windows_validate_interrupt_number(irq)) {
        return false;
    }
    
    // 转换为Windows事件
    int event_id = CN_WINDOWS_INTERRUPT_TO_EVENT(irq);
    if (event_id < CN_WINDOWS_EVENT_MIN || event_id > CN_WINDOWS_EVENT_MAX) {
        return false;
    }
    
    // 这里需要实际的等待逻辑
    // 暂时返回成功
    return true;
}

/**
 * @brief 轮询中断
 */
static bool windows_interrupt_manager_poll_interrupt(CN_InterruptNumber_t irq, bool* occurred)
{
    if (!g_windows_interrupt_manager_info.is_initialized || !occurred) {
        return false;
    }
    
    if (!windows_validate_interrupt_number(irq)) {
        return false;
    }
    
    // 这里需要实际的轮询逻辑
    // 暂时返回未发生
    *occurred = false;
    
    return true;
}

/**
 * @brief 模拟中断
 */
static bool windows_interrupt_manager_simulate_interrupt(CN_InterruptNumber_t irq)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return false;
    }
    
    if (!windows_validate_interrupt_number(irq)) {
        return false;
    }
    
    // 这里需要实际的模拟逻辑
    // 暂时返回成功
    return true;
}

/**
 * @brief 加载配置
 */
static bool windows_interrupt_manager_load_configuration(const char* config_file)
{
    if (!g_windows_interrupt_manager_info.is_initialized || !config_file) {
        return false;
    }
    
    return windows_load_config_from_file(config_file);
}

/**
 * @brief 保存配置
 */
static bool windows_interrupt_manager_save_configuration(const char* config_file)
{
    if (!g_windows_interrupt_manager_info.is_initialized || !config_file) {
        return false;
    }
    
    return windows_save_config_to_file(config_file);
}

/**
 * @brief 诊断中断
 */
static bool windows_interrupt_manager_diagnose(CN_InterruptNumber_t irq, void* report_buffer, size_t buffer_size)
{
    if (!g_windows_interrupt_manager_info.is_initialized || !report_buffer || buffer_size == 0) {
        return false;
    }
    
    if (!windows_validate_interrupt_number(irq)) {
        return false;
    }
    
    windows_generate_diagnostic_report(irq, (char*)report_buffer, buffer_size);
    
    return true;
}

/**
 * @brief 跟踪中断
 */
static bool windows_interrupt_manager_trace_interrupt(CN_InterruptNumber_t irq, bool enable)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return false;
    }
    
    if (!windows_validate_interrupt_number(irq)) {
        return false;
    }
    
    // 这里需要实际的跟踪逻辑
    // 暂时返回成功
    return true;
}

/**
 * @brief 开始监控中断
 */
static bool windows_interrupt_manager_start_monitoring(CN_InterruptNumber_t irq)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return false;
    }
    
    if (!windows_validate_interrupt_number(irq)) {
        return false;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    int index = windows_find_monitored_interrupt_index(irq);
    if (index != -1 && g_monitored_interrupts[index].is_monitoring) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 查找空闲监控槽位或使用现有槽位
    if (index == -1) {
        for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
            if (!g_monitored_interrupts[i].is_monitoring && g_monitored_interrupts[i].irq == 0) {
                index = i;
                break;
            }
        }
    }
    
    if (index == -1) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 开始监控
    g_monitored_interrupts[index].irq = irq;
    g_monitored_interrupts[index].is_monitoring = true;
    g_monitored_interrupts[index].interrupt_count = 0;
    g_monitored_interrupts[index].last_interrupt_time = 0;
    g_monitored_interrupts[index].max_latency = 0;
    g_monitored_interrupts[index].min_latency = UINT64_MAX;
    g_monitored_interrupts[index].total_latency = 0;
    
    if (g_monitored_interrupts[index].irq == 0) {
        g_monitored_count++;
    }
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    return true;
}

/**
 * @brief 停止监控中断
 */
static bool windows_interrupt_manager_stop_monitoring(CN_InterruptNumber_t irq)
{
    if (!g_windows_interrupt_manager_info.is_initialized) {
        return false;
    }
    
    if (!windows_validate_interrupt_number(irq)) {
        return false;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    int index = windows_find_monitored_interrupt_index(irq);
    if (index == -1 || !g_monitored_interrupts[index].is_monitoring) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 停止监控
    g_monitored_interrupts[index].is_monitoring = false;
    g_monitored_interrupts[index].irq = 0;
    g_monitored_count--;
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    return true;
}

/**
 * @brief 获取性能数据
 */
static bool windows_interrupt_manager_get_performance_data(CN_InterruptNumber_t irq, void* data_buffer, size_t buffer_size)
{
    if (!g_windows_interrupt_manager_info.is_initialized || !data_buffer || buffer_size == 0) {
        return false;
    }
    
    if (!windows_validate_interrupt_number(irq)) {
        return false;
    }
    
    EnterCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    int index = windows_find_monitored_interrupt_index(irq);
    if (index == -1 || !g_monitored_interrupts[index].is_monitoring) {
        LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
        return false;
    }
    
    // 复制性能数据
    if (buffer_size >= sizeof(g_monitored_interrupts[index])) {
        memcpy(data_buffer, &g_monitored_interrupts[index], sizeof(g_monitored_interrupts[index]));
    } else {
        memcpy(data_buffer, &g_monitored_interrupts[index], buffer_size);
    }
    
    LeaveCriticalSection(&g_windows_interrupt_manager_info.allocation_lock);
    
    return true;
}

/******************************************************************************
 * 内部工具函数实现
 ******************************************************************************/

/**
 * @brief 检查中断是否已分配
 */
static bool windows_is_interrupt_allocated(CN_InterruptNumber_t irq)
{
    for (uint32_t i = 0; i < g_windows_interrupt_manager_info.allocated_count; i++) {
        if (g_windows_interrupt_manager_info.allocated_interrupts[i] == irq) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 查找空闲中断槽位
 */
static bool windows_find_free_interrupt_slot(CN_InterruptNumber_t* irq)
{
    for (CN_InterruptNumber_t i = 1; i <= CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (!windows_is_interrupt_allocated(i)) {
            *irq = i;
            return true;
        }
    }
    return false;
}

/**
 * @brief 检查控制器是否已注册
 */
static bool windows_is_controller_registered(const char* name)
{
    return windows_find_controller_index(name) != -1;
}

/**
 * @brief 查找控制器索引
 */
static int windows_find_controller_index(const char* name)
{
    for (uint32_t i = 0; i < 16; i++) {
        if (g_registered_controllers[i].controller != NULL && 
            strcmp(g_registered_controllers[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 查找路由索引
 */
static int windows_find_route_index(CN_InterruptNumber_t irq)
{
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_interrupt_routes[i].is_routed && g_interrupt_routes[i].source_irq == irq) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 查找共享处理程序索引
 */
static int windows_find_shared_handler_index(CN_InterruptNumber_t irq, CN_InterruptHandler_t handler)
{
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS * 4; i++) {
        if (g_shared_handlers[i].is_shared && 
            g_shared_handlers[i].irq == irq && 
            g_shared_handlers[i].handler == handler) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 查找链式处理程序索引
 */
static int windows_find_chained_handler_index(CN_InterruptNumber_t irq)
{
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_chained_handlers[i].is_chained && g_chained_handlers[i].irq == irq) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 查找监控中断索引
 */
static int windows_find_monitored_interrupt_index(CN_InterruptNumber_t irq)
{
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_monitored_interrupts[i].irq == irq) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 验证中断号
 */
static bool windows_validate_interrupt_number(CN_InterruptNumber_t irq)
{
    return irq >= 1 && irq <= CN_WINDOWS_MAX_INTERRUPT_HANDLERS;
}

/**
 * @brief 从文件加载配置
 */
static bool windows_load_config_from_file(const char* filename)
{
    // 这里需要实际的配置加载逻辑
    // 暂时返回成功
    return true;
}

/**
 * @brief 保存配置到文件
 */
static bool windows_save_config_to_file(const char* filename)
{
    // 这里需要实际的配置保存逻辑
    // 暂时返回成功
    return true;
}

/**
 * @brief 生成诊断报告
 */
static void windows_generate_diagnostic_report(CN_InterruptNumber_t irq, char* buffer, size_t buffer_size)
{
    if (!buffer || buffer_size == 0) {
        return;
    }
    
    // 生成简单的诊断报告
    snprintf(buffer, buffer_size, 
             "中断诊断报告 - 中断号: %u\n"
             "状态: %s\n"
             "监控: %s\n"
             "路由: %s\n"
             "共享处理程序: %u\n"
             "链式处理程序: %u\n",
             (unsigned int)irq,
             windows_is_interrupt_allocated(irq) ? "已分配" : "未分配",
             windows_find_monitored_interrupt_index(irq) != -1 ? "已监控" : "未监控",
             windows_find_route_index(irq) != -1 ? "已路由" : "未路由",
             g_shared_handler_count,
             g_chained_handler_count);
}

/******************************************************************************
 * 公共接口函数
 ******************************************************************************/

/**
 * @brief 获取Windows平台中断管理器接口
 */
Stru_CN_InterruptManagerInterface_t* CN_platform_windows_get_interrupt_manager(void)
{
    return &g_windows_interrupt_manager_interface;
}
