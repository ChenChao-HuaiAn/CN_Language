/******************************************************************************
 * 文件名: CN_platform_windows_interrupt_special.c
 * 功能: CN_Language Windows平台中断特殊接口实现
 * 描述: Windows平台中断处理特殊接口实现，包括事件、APC、I/O完成端口等Windows特有功能
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-04: 创建文件，实现Windows平台中断特殊接口
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_windows_interrupt.h"
#include <stdlib.h>
#include <string.h>

/* 内部数据结构定义 */

/** Windows事件接口内部状态 */
typedef struct {
    bool is_initialized;
    HANDLE event_handles[CN_WINDOWS_EVENT_MAX];
    char event_names[CN_WINDOWS_EVENT_MAX][CN_WINDOWS_MAX_INTERRUPT_NAME];
} WindowsEventInterfaceState;

/** Windows APC接口内部状态 */
typedef struct {
    bool is_initialized;
    CN_WindowsAPCCallback_t registered_callbacks[32];
    ULONG_PTR callback_data[32];
    uint32_t callback_count;
} WindowsAPCInterfaceState;

/** Windows I/O完成端口接口内部状态 */
typedef struct {
    bool is_initialized;
    HANDLE iocp_handle;
    DWORD thread_count;
} WindowsIOCompletionInterfaceState;

/** Windows中断线程接口内部状态 */
typedef struct {
    bool is_initialized;
    HANDLE interrupt_thread;
    DWORD thread_id;
    int thread_priority;
    DWORD_PTR thread_affinity;
} WindowsInterruptThreadInterfaceState;

/* 静态全局状态变量 */

static WindowsEventInterfaceState g_event_state = {
    .is_initialized = false
};

static WindowsAPCInterfaceState g_apc_state = {
    .is_initialized = false,
    .callback_count = 0
};

static WindowsIOCompletionInterfaceState g_iocp_state = {
    .is_initialized = false,
    .iocp_handle = NULL,
    .thread_count = 0
};

static WindowsInterruptThreadInterfaceState g_interrupt_thread_state = {
    .is_initialized = false,
    .interrupt_thread = NULL,
    .thread_id = 0,
    .thread_priority = THREAD_PRIORITY_NORMAL,
    .thread_affinity = 0
};

/* 静态辅助函数声明 */

static bool windows_event_interface_initialize(void);
static void windows_event_interface_cleanup(void);
static HANDLE windows_event_create(BOOL manual_reset, BOOL initial_state, const char* name);
static BOOL windows_event_destroy(HANDLE event);
static BOOL windows_event_set(HANDLE event);
static BOOL windows_event_reset(HANDLE event);
static BOOL windows_event_pulse(HANDLE event);
static DWORD windows_event_wait_single(HANDLE event, DWORD timeout_ms);
static DWORD windows_event_wait_multiple(HANDLE* events, DWORD count, BOOL wait_all, DWORD timeout_ms);
static BOOL windows_event_wait_alertable(HANDLE event, DWORD timeout_ms);
static BOOL windows_event_get_state(HANDLE event);
static BOOL windows_event_set_name(HANDLE event, const char* name);
static char* windows_event_get_name(HANDLE event);

static bool windows_apc_interface_initialize(void);
static void windows_apc_interface_cleanup(void);
static BOOL windows_apc_queue_user(HANDLE thread, PAPCFUNC apc_func, ULONG_PTR data);
static BOOL windows_apc_queue_io(HANDLE file_handle, LPOVERLAPPED overlapped, 
                                 LPOVERLAPPED_COMPLETION_ROUTINE completion_routine);
static BOOL windows_apc_enter_alertable_wait(void);
static BOOL windows_apc_exit_alertable_wait(void);
static DWORD windows_apc_sleep_ex(DWORD timeout_ms, BOOL alertable);
static BOOL windows_apc_register_callback(CN_WindowsAPCCallback_t callback, ULONG_PTR data);
static BOOL windows_apc_unregister_callback(CN_WindowsAPCCallback_t callback);

static bool windows_iocp_interface_initialize(void);
static void windows_iocp_interface_cleanup(void);
static HANDLE windows_iocp_create(DWORD max_concurrent_threads);
static BOOL windows_iocp_destroy(HANDLE iocp);
static BOOL windows_iocp_associate_device(HANDLE iocp, HANDLE device, ULONG_PTR completion_key);
static BOOL windows_iocp_post_status(HANDLE iocp, ULONG_PTR completion_key, DWORD bytes_transferred, 
                                     LPOVERLAPPED overlapped);
static BOOL windows_iocp_get_status(HANDLE iocp, LPDWORD bytes_transferred, PULONG_PTR completion_key, 
                                    LPOVERLAPPED* overlapped, DWORD timeout_ms);
static BOOL windows_iocp_create_thread_pool(HANDLE iocp, DWORD thread_count);
static BOOL windows_iocp_destroy_thread_pool(HANDLE iocp);
static DWORD windows_iocp_get_thread_count(HANDLE iocp);

static bool windows_interrupt_thread_interface_initialize(void);
static void windows_interrupt_thread_interface_cleanup(void);
static HANDLE windows_interrupt_thread_create(LPTHREAD_START_ROUTINE thread_func, LPVOID arg);
static BOOL windows_interrupt_thread_join(HANDLE thread, DWORD timeout_ms);
static BOOL windows_interrupt_thread_detach(HANDLE thread);
static BOOL windows_interrupt_thread_set_priority(HANDLE thread, int priority);
static BOOL windows_interrupt_thread_get_priority(HANDLE thread, int* priority);
static BOOL windows_interrupt_thread_set_affinity(HANDLE thread, DWORD_PTR affinity_mask);
static BOOL windows_interrupt_thread_get_affinity(HANDLE thread, DWORD_PTR* affinity_mask);
static BOOL windows_interrupt_thread_signal(HANDLE thread, HANDLE event);
static BOOL windows_interrupt_thread_wait_event(HANDLE thread, HANDLE event, DWORD timeout_ms);

/* 全局接口变量定义 */

/** Windows事件处理接口 */
Stru_CN_WindowsEventInterface_t g_windows_event_interface = {
    .event_create = windows_event_create,
    .event_destroy = windows_event_destroy,
    .event_set = windows_event_set,
    .event_reset = windows_event_reset,
    .event_pulse = windows_event_pulse,
    .event_wait_single = windows_event_wait_single,
    .event_wait_multiple = windows_event_wait_multiple,
    .event_wait_alertable = windows_event_wait_alertable,
    .event_get_state = windows_event_get_state,
    .event_set_name = windows_event_set_name,
    .event_get_name = windows_event_get_name
};

/** Windows APC接口 */
Stru_CN_WindowsAPCInterface_t g_windows_apc_interface = {
    .apc_queue_user = windows_apc_queue_user,
    .apc_queue_io = windows_apc_queue_io,
    .apc_enter_alertable_wait = windows_apc_enter_alertable_wait,
    .apc_exit_alertable_wait = windows_apc_exit_alertable_wait,
    .apc_sleep_ex = windows_apc_sleep_ex,
    .apc_register_callback = windows_apc_register_callback,
    .apc_unregister_callback = windows_apc_unregister_callback
};

/** Windows I/O完成端口接口 */
Stru_CN_WindowsIOCompletionInterface_t g_windows_iocp_interface = {
    .iocp_create = windows_iocp_create,
    .iocp_destroy = windows_iocp_destroy,
    .iocp_associate_device = windows_iocp_associate_device,
    .iocp_post_status = windows_iocp_post_status,
    .iocp_get_status = windows_iocp_get_status,
    .iocp_create_thread_pool = windows_iocp_create_thread_pool,
    .iocp_destroy_thread_pool = windows_iocp_destroy_thread_pool,
    .iocp_get_thread_count = windows_iocp_get_thread_count
};

/** Windows中断线程接口 */
Stru_CN_WindowsInterruptThreadInterface_t g_windows_interrupt_thread_interface = {
    .interrupt_thread_create = windows_interrupt_thread_create,
    .interrupt_thread_join = windows_interrupt_thread_join,
    .interrupt_thread_detach = windows_interrupt_thread_detach,
    .interrupt_thread_set_priority = windows_interrupt_thread_set_priority,
    .interrupt_thread_get_priority = windows_interrupt_thread_get_priority,
    .interrupt_thread_set_affinity = windows_interrupt_thread_set_affinity,
    .interrupt_thread_get_affinity = windows_interrupt_thread_get_affinity,
    .interrupt_thread_signal = windows_interrupt_thread_signal,
    .interrupt_thread_wait_event = windows_interrupt_thread_wait_event
};

/******************************************************************************
 * Windows事件接口实现
 ******************************************************************************/

/**
 * @brief 初始化Windows事件接口
 */
static bool windows_event_interface_initialize(void)
{
    if (g_event_state.is_initialized) {
        return true;
    }
    
    // 初始化事件句柄数组
    for (int i = 0; i < CN_WINDOWS_EVENT_MAX; i++) {
        g_event_state.event_handles[i] = NULL;
        g_event_state.event_names[i][0] = '\0';
    }
    
    g_event_state.is_initialized = true;
    return true;
}

/**
 * @brief 清理Windows事件接口
 */
static void windows_event_interface_cleanup(void)
{
    if (!g_event_state.is_initialized) {
        return;
    }
    
    // 关闭所有事件句柄
    for (int i = 0; i < CN_WINDOWS_EVENT_MAX; i++) {
        if (g_event_state.event_handles[i] != NULL) {
            CloseHandle(g_event_state.event_handles[i]);
            g_event_state.event_handles[i] = NULL;
        }
        g_event_state.event_names[i][0] = '\0';
    }
    
    g_event_state.is_initialized = false;
}

/**
 * @brief 创建Windows事件
 */
static HANDLE windows_event_create(BOOL manual_reset, BOOL initial_state, const char* name)
{
    if (!g_event_state.is_initialized) {
        windows_event_interface_initialize();
    }
    
    HANDLE event = CreateEvent(NULL, manual_reset, initial_state, NULL);
    if (event == NULL) {
        return NULL;
    }
    
    // 尝试将事件存储在内部数组中（可选）
    if (name != NULL) {
        for (int i = 0; i < CN_WINDOWS_EVENT_MAX; i++) {
            if (g_event_state.event_handles[i] == NULL) {
                g_event_state.event_handles[i] = event;
                strncpy(g_event_state.event_names[i], name, CN_WINDOWS_MAX_INTERRUPT_NAME - 1);
                g_event_state.event_names[i][CN_WINDOWS_MAX_INTERRUPT_NAME - 1] = '\0';
                break;
            }
        }
    }
    
    return event;
}

/**
 * @brief 销毁Windows事件
 */
static BOOL windows_event_destroy(HANDLE event)
{
    if (event == NULL) {
        return FALSE;
    }
    
    // 从内部数组中移除事件
    for (int i = 0; i < CN_WINDOWS_EVENT_MAX; i++) {
        if (g_event_state.event_handles[i] == event) {
            g_event_state.event_handles[i] = NULL;
            g_event_state.event_names[i][0] = '\0';
            break;
        }
    }
    
    return CloseHandle(event);
}

/**
 * @brief 设置事件状态为有信号
 */
static BOOL windows_event_set(HANDLE event)
{
    return SetEvent(event);
}

/**
 * @brief 重置事件状态为无信号
 */
static BOOL windows_event_reset(HANDLE event)
{
    return ResetEvent(event);
}

/**
 * @brief 脉冲事件（设置后立即重置）
 */
static BOOL windows_event_pulse(HANDLE event)
{
    return PulseEvent(event);
}

/**
 * @brief 等待单个事件
 */
static DWORD windows_event_wait_single(HANDLE event, DWORD timeout_ms)
{
    return WaitForSingleObject(event, timeout_ms);
}

/**
 * @brief 等待多个事件
 */
static DWORD windows_event_wait_multiple(HANDLE* events, DWORD count, BOOL wait_all, DWORD timeout_ms)
{
    return WaitForMultipleObjects(count, events, wait_all, timeout_ms);
}

/**
 * @brief 可警报等待事件
 */
static BOOL windows_event_wait_alertable(HANDLE event, DWORD timeout_ms)
{
    DWORD result = WaitForSingleObjectEx(event, timeout_ms, TRUE);
    return (result == WAIT_OBJECT_0 || result == WAIT_IO_COMPLETION);
}

/**
 * @brief 获取事件状态
 */
static BOOL windows_event_get_state(HANDLE event)
{
    DWORD result = WaitForSingleObject(event, 0);
    return (result == WAIT_OBJECT_0);
}

/**
 * @brief 设置事件名称
 */
static BOOL windows_event_set_name(HANDLE event, const char* name)
{
    if (event == NULL || name == NULL) {
        return FALSE;
    }
    
    for (int i = 0; i < CN_WINDOWS_EVENT_MAX; i++) {
        if (g_event_state.event_handles[i] == event) {
            strncpy(g_event_state.event_names[i], name, CN_WINDOWS_MAX_INTERRUPT_NAME - 1);
            g_event_state.event_names[i][CN_WINDOWS_MAX_INTERRUPT_NAME - 1] = '\0';
            return TRUE;
        }
    }
    
    // 如果事件不在数组中，尝试添加到数组
    for (int i = 0; i < CN_WINDOWS_EVENT_MAX; i++) {
        if (g_event_state.event_handles[i] == NULL) {
            g_event_state.event_handles[i] = event;
            strncpy(g_event_state.event_names[i], name, CN_WINDOWS_MAX_INTERRUPT_NAME - 1);
            g_event_state.event_names[i][CN_WINDOWS_MAX_INTERRUPT_NAME - 1] = '\0';
            return TRUE;
        }
    }
    
    return FALSE;
}

/**
 * @brief 获取事件名称
 */
static char* windows_event_get_name(HANDLE event)
{
    if (event == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < CN_WINDOWS_EVENT_MAX; i++) {
        if (g_event_state.event_handles[i] == event) {
            return g_event_state.event_names[i];
        }
    }
    
    return NULL;
}

/******************************************************************************
 * Windows APC接口实现
 ******************************************************************************/

/**
 * @brief 初始化Windows APC接口
 */
static bool windows_apc_interface_initialize(void)
{
    if (g_apc_state.is_initialized) {
        return true;
    }
    
    // 初始化回调数组
    for (uint32_t i = 0; i < 32; i++) {
        g_apc_state.registered_callbacks[i] = NULL;
        g_apc_state.callback_data[i] = 0;
    }
    
    g_apc_state.callback_count = 0;
    g_apc_state.is_initialized = true;
    return true;
}

/**
 * @brief 清理Windows APC接口
 */
static void windows_apc_interface_cleanup(void)
{
    if (!g_apc_state.is_initialized) {
        return;
    }
    
    // 清除所有回调
    for (uint32_t i = 0; i < 32; i++) {
        g_apc_state.registered_callbacks[i] = NULL;
        g_apc_state.callback_data[i] = 0;
    }
    
    g_apc_state.callback_count = 0;
    g_apc_state.is_initialized = false;
}

/**
 * @brief 队列用户APC
 */
static BOOL windows_apc_queue_user(HANDLE thread, PAPCFUNC apc_func, ULONG_PTR data)
{
    return QueueUserAPC(apc_func, thread, data);
}

/**
 * @brief 队列I/O APC
 */
static BOOL windows_apc_queue_io(HANDLE file_handle, LPOVERLAPPED overlapped, 
                                 LPOVERLAPPED_COMPLETION_ROUTINE completion_routine)
{
    // 注意：这个函数实际上是通过ReadFileEx/WriteFileEx等函数间接调用的
    // 这里只是提供一个占位符实现
    (void)file_handle;
    (void)overlapped;
    (void)completion_routine;
    return FALSE;
}

/**
 * @brief 进入可警报等待状态
 */
static BOOL windows_apc_enter_alertable_wait(void)
{
    // 简单实现：睡眠0毫秒以允许APC执行
    SleepEx(0, TRUE);
    return TRUE;
}

/**
 * @brief 退出可警报等待状态
 */
static BOOL windows_apc_exit_alertable_wait(void)
{
    // 简单实现：什么也不做
    return TRUE;
}

/**
 * @brief 扩展睡眠函数，支持可警报等待
 */
static DWORD windows_apc_sleep_ex(DWORD timeout_ms, BOOL alertable)
{
    return SleepEx(timeout_ms, alertable);
}

/**
 * @brief 注册APC回调
 */
static BOOL windows_apc_register_callback(CN_WindowsAPCCallback_t callback, ULONG_PTR data)
{
    if (!g_apc_state.is_initialized) {
        windows_apc_interface_initialize();
    }
    
    if (callback == NULL || g_apc_state.callback_count >= 32) {
        return FALSE;
    }
    
    // 查找空闲槽位
    for (uint32_t i = 0; i < 32; i++) {
        if (g_apc_state.registered_callbacks[i] == NULL) {
            g_apc_state.registered_callbacks[i] = callback;
            g_apc_state.callback_data[i] = data;
            g_apc_state.callback_count++;
            return TRUE;
        }
    }
    
    return FALSE;
}

/**
 * @brief 注销APC回调
 */
static BOOL windows_apc_unregister_callback(CN_WindowsAPCCallback_t callback)
{
    if (!g_apc_state.is_initialized || callback == NULL) {
        return FALSE;
    }
    
    // 查找并移除回调
    for (uint32_t i = 0; i < 32; i++) {
        if (g_apc_state.registered_callbacks[i] == callback) {
            g_apc_state.registered_callbacks[i] = NULL;
            g_apc_state.callback_data[i] = 0;
            g_apc_state.callback_count--;
            return TRUE;
        }
    }
    
    return FALSE;
}

/******************************************************************************
 * Windows I/O完成端口接口实现
 ******************************************************************************/

/**
 * @brief 初始化Windows I/O完成端口接口
 */
static bool windows_iocp_interface_initialize(void)
{
    if (g_iocp_state.is_initialized) {
        return true;
    }
    
    g_iocp_state.iocp_handle = NULL;
    g_iocp_state.thread_count = 0;
    g_iocp_state.is_initialized = true;
    
    return true;
}

/**
 * @brief 清理Windows I/O完成端口接口
 */
static void windows_iocp_interface_cleanup(void)
{
    if (!g_iocp_state.is_initialized) {
        return;
    }
    
    if (g_iocp_state.iocp_handle != NULL) {
        CloseHandle(g_iocp_state.iocp_handle);
        g_iocp_state.iocp_handle = NULL;
    }
    
    g_iocp_state.thread_count = 0;
    g_iocp_state.is_initialized = false;
}

/**
 * @brief 创建I/O完成端口
 */
static HANDLE windows_iocp_create(DWORD max_concurrent_threads)
{
    if (!g_iocp_state.is_initialized) {
        windows_iocp_interface_initialize();
    }
    
    HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, max_concurrent_threads);
    if (iocp != NULL) {
        g_iocp_state.iocp_handle = iocp;
    }
    
    return iocp;
}

/**
 * @brief 销毁I/O完成端口
 */
static BOOL windows_iocp_destroy(HANDLE iocp)
{
    if (iocp == NULL) {
        return FALSE;
    }
    
    BOOL result = CloseHandle(iocp);
    
    if (g_iocp_state.iocp_handle == iocp) {
        g_iocp_state.iocp_handle = NULL;
    }
    
    return result;
}

/**
 * @brief 关联设备到I/O完成端口
 */
static BOOL windows_iocp_associate_device(HANDLE iocp, HANDLE device, ULONG_PTR completion_key)
{
    if (iocp == NULL || device == NULL) {
        return FALSE;
    }
    
    HANDLE result = CreateIoCompletionPort(device, iocp, completion_key, 0);
    return (result != NULL);
}

/**
 * @brief 发布状态到I/O完成端口
 */
static BOOL windows_iocp_post_status(HANDLE iocp, ULONG_PTR completion_key, DWORD bytes_transferred, 
                                     LPOVERLAPPED overlapped)
{
    if (iocp == NULL) {
        return FALSE;
    }
    
    return PostQueuedCompletionStatus(iocp, bytes_transferred, completion_key, overlapped);
}

/**
 * @brief 从I/O完成端口获取状态
 */
static BOOL windows_iocp_get_status(HANDLE iocp, LPDWORD bytes_transferred, PULONG_PTR completion_key, 
                                    LPOVERLAPPED* overlapped, DWORD timeout_ms)
{
    if (iocp == NULL) {
        return FALSE;
    }
    
    return GetQueuedCompletionStatus(iocp, bytes_transferred, completion_key, overlapped, timeout_ms);
}

/**
 * @brief 为I/O完成端口创建线程池
 */
static BOOL windows_iocp_create_thread_pool(HANDLE iocp, DWORD thread_count)
{
    // 简单实现：记录线程数量
    (void)iocp;
    g_iocp_state.thread_count = thread_count;
    return TRUE;
}

/**
 * @brief 销毁I/O完成端口的线程池
 */
static BOOL windows_iocp_destroy_thread_pool(HANDLE iocp)
{
    // 简单实现：重置线程数量
    (void)iocp;
    g_iocp_state.thread_count = 0;
    return TRUE;
}

/**
 * @brief 获取I/O完成端口的线程数量
 */
static DWORD windows_iocp_get_thread_count(HANDLE iocp)
{
    (void)iocp;
    return g_iocp_state.thread_count;
}

/******************************************************************************
 * Windows中断线程接口实现
 ******************************************************************************/

/**
 * @brief 初始化Windows中断线程接口
 */
static bool windows_interrupt_thread_interface_initialize(void)
{
    if (g_interrupt_thread_state.is_initialized) {
        return true;
    }
    
    g_interrupt_thread_state.interrupt_thread = NULL;
    g_interrupt_thread_state.thread_id = 0;
    g_interrupt_thread_state.thread_priority = THREAD_PRIORITY_NORMAL;
    g_interrupt_thread_state.thread_affinity = 0;
    g_interrupt_thread_state.is_initialized = true;
    
    return true;
}

/**
 * @brief 清理Windows中断线程接口
 */
static void windows_interrupt_thread_interface_cleanup(void)
{
    if (!g_interrupt_thread_state.is_initialized) {
        return;
    }
    
    if (g_interrupt_thread_state.interrupt_thread != NULL) {
        CloseHandle(g_interrupt_thread_state.interrupt_thread);
        g_interrupt_thread_state.interrupt_thread = NULL;
    }
    
    g_interrupt_thread_state.thread_id = 0;
    g_interrupt_thread_state.thread_priority = THREAD_PRIORITY_NORMAL;
    g_interrupt_thread_state.thread_affinity = 0;
    g_interrupt_thread_state.is_initialized = false;
}

/**
 * @brief 创建中断线程
 */
static HANDLE windows_interrupt_thread_create(LPTHREAD_START_ROUTINE thread_func, LPVOID arg)
{
    if (!g_interrupt_thread_state.is_initialized) {
        windows_interrupt_thread_interface_initialize();
    }
    
    DWORD thread_id;
    HANDLE thread = CreateThread(NULL, 0, thread_func, arg, 0, &thread_id);
    
    if (thread != NULL) {
        g_interrupt_thread_state.interrupt_thread = thread;
        g_interrupt_thread_state.thread_id = thread_id;
    }
    
    return thread;
}

/**
 * @brief 等待中断线程结束
 */
static BOOL windows_interrupt_thread_join(HANDLE thread, DWORD timeout_ms)
{
    if (thread == NULL) {
        return FALSE;
    }
    
    DWORD result = WaitForSingleObject(thread, timeout_ms);
    return (result == WAIT_OBJECT_0);
}

/**
 * @brief 分离中断线程
 */
static BOOL windows_interrupt_thread_detach(HANDLE thread)
{
    // 在Windows中，线程创建后默认就是可分离的
    // 这里只是关闭句柄，让线程继续运行
    if (thread == NULL) {
        return FALSE;
    }
    
    return CloseHandle(thread);
}

/**
 * @brief 设置中断线程优先级
 */
static BOOL windows_interrupt_thread_set_priority(HANDLE thread, int priority)
{
    if (thread == NULL) {
        return FALSE;
    }
    
    return SetThreadPriority(thread, priority);
}

/**
 * @brief 获取中断线程优先级
 */
static BOOL windows_interrupt_thread_get_priority(HANDLE thread, int* priority)
{
    if (thread == NULL || priority == NULL) {
        return FALSE;
    }
    
    *priority = GetThreadPriority(thread);
    return (*priority != THREAD_PRIORITY_ERROR_RETURN);
}

/**
 * @brief 设置中断线程亲和性
 */
static BOOL windows_interrupt_thread_set_affinity(HANDLE thread, DWORD_PTR affinity_mask)
{
    if (thread == NULL) {
        return FALSE;
    }
    
    return SetThreadAffinityMask(thread, affinity_mask) != 0;
}

/**
 * @brief 获取中断线程亲和性
 */
static BOOL windows_interrupt_thread_get_affinity(HANDLE thread, DWORD_PTR* affinity_mask)
{
    if (thread == NULL || affinity_mask == NULL) {
        return FALSE;
    }
    
    // Windows没有直接获取线程亲和性的API
    // 这里返回一个默认值
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    *affinity_mask = (DWORD_PTR)((1 << sys_info.dwNumberOfProcessors) - 1);
    
    return TRUE;
}

/**
 * @brief 向中断线程发送信号
 */
static BOOL windows_interrupt_thread_signal(HANDLE thread, HANDLE event)
{
    if (thread == NULL || event == NULL) {
        return FALSE;
    }
    
    // 通过APC向线程发送信号
    return QueueUserAPC((PAPCFUNC)SetEvent, thread, (ULONG_PTR)event);
}

/**
 * @brief 中断线程等待事件
 */
static BOOL windows_interrupt_thread_wait_event(HANDLE thread, HANDLE event, DWORD timeout_ms)
{
    // 这个函数通常在线程内部调用，而不是从外部调用
    // 这里提供一个简单的实现
    (void)thread;
    
    if (event == NULL) {
        return FALSE;
    }
    
    DWORD result = WaitForSingleObject(event, timeout_ms);
    return (result == WAIT_OBJECT_0);
}

/******************************************************************************
 * 公共接口函数实现
 ******************************************************************************/

/**
 * @brief 获取Windows事件处理接口
 */
Stru_CN_WindowsEventInterface_t* CN_platform_windows_get_event_interface(void)
{
    if (!g_event_state.is_initialized) {
        windows_event_interface_initialize();
    }
    
    return &g_windows_event_interface;
}

/**
 * @brief 获取Windows APC接口
 */
Stru_CN_WindowsAPCInterface_t* CN_platform_windows_get_apc_interface(void)
{
    if (!g_apc_state.is_initialized) {
        windows_apc_interface_initialize();
    }
    
    return &g_windows_apc_interface;
}

/**
 * @brief 获取Windows I/O完成端口接口
 */
Stru_CN_WindowsIOCompletionInterface_t* CN_platform_windows_get_iocp_interface(void)
{
    if (!g_iocp_state.is_initialized) {
        windows_iocp_interface_initialize();
    }
    
    return &g_windows_iocp_interface;
}

/**
 * @brief 获取Windows中断线程接口
 */
Stru_CN_WindowsInterruptThreadInterface_t* CN_platform_windows_get_interrupt_thread_interface(void)
{
    if (!g_interrupt_thread_state.is_initialized) {
        windows_interrupt_thread_interface_initialize();
    }
    
    return &g_windows_interrupt_thread_interface;
}

/**
 * @brief 将Windows事件转换为CN中断号
 */
CN_InterruptNumber_t CN_platform_windows_event_to_interrupt(int event_id)
{
    if (event_id < CN_WINDOWS_EVENT_MIN || event_id > CN_WINDOWS_EVENT_MAX) {
        return 0;
    }
    
    return CN_WINDOWS_EVENT_TO_INTERRUPT(event_id);
}

/**
 * @brief 将CN中断号转换为Windows事件
 */
int CN_platform_windows_interrupt_to_event(CN_InterruptNumber_t interrupt)
{
    if (interrupt < CN_WINDOWS_EVENT_TO_INTERRUPT(CN_WINDOWS_EVENT_MIN) || 
        interrupt > CN_WINDOWS_EVENT_TO_INTERRUPT(CN_WINDOWS_EVENT_MAX)) {
        return -1;
    }
    
    return CN_WINDOWS_INTERRUPT_TO_EVENT(interrupt);
}

/**
 * @brief 检查事件是否可用作中断
 */
bool CN_platform_windows_event_is_available(int event_id)
{
    if (event_id < CN_WINDOWS_EVENT_MIN || event_id > CN_WINDOWS_EVENT_MAX) {
        return false;
    }
    
    // 简单实现：总是返回true
    return true;
}

/**
 * @brief 获取默认中断事件掩码
 */
bool CN_platform_windows_get_default_event_mask(DWORD* mask)
{
    if (mask == NULL) {
        return false;
    }
    
    // 返回所有事件位的掩码
    *mask = (1 << (CN_WINDOWS_EVENT_MAX - CN_WINDOWS_EVENT_MIN + 1)) - 1;
    return true;
}
