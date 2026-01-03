/******************************************************************************
 * 文件名: CN_platform_windows_interrupt.h
 * 功能: CN_Language Windows平台中断处理实现接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义Windows平台中断处理接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_PLATFORM_WINDOWS_INTERRUPT_H
#define CN_PLATFORM_WINDOWS_INTERRUPT_H

#include "../CN_interrupt.h"
#include "../CN_platform.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

// 条件包含Windows特定头文件
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Windows平台中断处理常量定义
// ============================================================================

/** 最大中断处理程序数量 */
#define CN_WINDOWS_MAX_INTERRUPT_HANDLERS 256

/** 最大中断名称长度 */
#define CN_WINDOWS_MAX_INTERRUPT_NAME 64

/** Windows事件范围 */
#define CN_WINDOWS_EVENT_MIN 1
#define CN_WINDOWS_EVENT_MAX 64

/** Windows特定中断标志 */
#define CN_WINDOWS_INTERRUPT_FLAG_EVENT_BASED     0x0001  /**< 基于事件的中断 */
#define CN_WINDOWS_INTERRUPT_FLAG_THREAD_SAFE     0x0002  /**< 线程安全中断处理 */
#define CN_WINDOWS_INTERRUPT_FLAG_ASYNC_SAFE      0x0004  /**< 异步安全处理 */
#define CN_WINDOWS_INTERRUPT_FLAG_APC_BASED       0x0008  /**< 基于APC的中断 */
#define CN_WINDOWS_INTERRUPT_FLAG_IO_COMPLETION   0x0010  /**< I/O完成端口中断 */

/** Windows事件到中断映射 */
#define CN_WINDOWS_EVENT_TO_INTERRUPT(event) ((CN_InterruptNumber_t)((event) + 2000))

/** Windows中断到事件映射 */
#define CN_WINDOWS_INTERRUPT_TO_EVENT(intr) ((int)((intr) - 2000))

// ============================================================================
// Windows平台中断处理类型定义
// ============================================================================

/** Windows中断处理程序信息结构体 */
typedef struct Stru_CN_WindowsInterruptHandlerInfo_t
{
    CN_InterruptNumber_t interrupt_number;      /**< 中断号 */
    CN_InterruptHandler_t handler;              /**< 中断处理函数 */
    void* user_data;                            /**< 用户数据 */
    char name[CN_WINDOWS_MAX_INTERRUPT_NAME];   /**< 中断名称 */
    CN_InterruptFlags_t flags;                  /**< 中断标志 */
    void* event_handle;                         /**< Windows事件句柄（平台特定） */
    bool is_installed;                          /**< 是否已安装 */
    CRITICAL_SECTION* lock;                     /**< 线程安全锁（平台特定） */
} Stru_CN_WindowsInterruptHandlerInfo_t;

/** Windows中断控制器信息结构体 */
typedef struct Stru_CN_WindowsInterruptControllerInfo_t
{
    bool is_initialized;                        /**< 是否已初始化 */
    Stru_CN_WindowsInterruptHandlerInfo_t handlers[CN_WINDOWS_MAX_INTERRUPT_HANDLERS]; /**< 中断处理程序数组 */
    uint32_t handler_count;                     /**< 已注册的处理程序数量 */
    CRITICAL_SECTION global_lock;               /**< 全局锁（平台特定） */
    DWORD main_thread_id;                       /**< 主线程ID（平台特定） */
} Stru_CN_WindowsInterruptControllerInfo_t;

/** Windows中断管理器信息结构体 */
typedef struct Stru_CN_WindowsInterruptManagerInfo_t
{
    bool is_initialized;                        /**< 是否已初始化 */
    Stru_CN_WindowsInterruptControllerInfo_t* controller; /**< 关联的控制器 */
    CN_InterruptNumber_t allocated_interrupts[CN_WINDOWS_MAX_INTERRUPT_HANDLERS]; /**< 已分配的中断号 */
    uint32_t allocated_count;                   /**< 已分配数量 */
    CRITICAL_SECTION allocation_lock;           /**< 分配锁（平台特定） */
} Stru_CN_WindowsInterruptManagerInfo_t;

/** Windows中断工具信息结构体 */
typedef struct Stru_CN_WindowsInterruptToolsInfo_t
{
    bool is_initialized;                        /**< 是否已初始化 */
    Stru_CN_InterruptContext_t current_context; /**< 当前中断上下文 */
    bool in_interrupt_context;                  /**< 是否在中断上下文中 */
    DWORD interrupt_thread_id;                  /**< 中断线程ID（平台特定） */
    uint64_t interrupt_count;                   /**< 中断计数 */
    uint64_t nested_interrupt_depth;            /**< 嵌套中断深度 */
} Stru_CN_WindowsInterruptToolsInfo_t;

// ============================================================================
// Windows平台中断处理接口声明
// ============================================================================

/**
 * @brief 获取Windows平台中断控制器接口
 * 
 * @return Windows平台中断控制器接口指针
 */
Stru_CN_InterruptControllerInterface_t* CN_platform_windows_get_interrupt_controller(void);

/**
 * @brief 获取Windows平台中断管理器接口
 * 
 * @return Windows平台中断管理器接口指针
 */
Stru_CN_InterruptManagerInterface_t* CN_platform_windows_get_interrupt_manager(void);

/**
 * @brief 获取Windows平台中断工具接口
 * 
 * @return Windows平台中断工具接口指针
 */
Stru_CN_InterruptToolsInterface_t* CN_platform_windows_get_interrupt_tools(void);

/**
 * @brief 初始化Windows平台中断处理系统
 * 
 * @return 初始化成功返回true，失败返回false
 */
bool CN_platform_windows_interrupt_initialize(void);

/**
 * @brief 清理Windows平台中断处理系统
 */
void CN_platform_windows_interrupt_cleanup(void);

/**
 * @brief 检查Windows平台中断处理是否已初始化
 * 
 * @return 如果已初始化返回true，否则返回false
 */
bool CN_platform_windows_interrupt_is_initialized(void);

// ============================================================================
// Windows特定中断处理功能
// ============================================================================

// APC回调类型定义（需要在结构体之前定义）
typedef void (*CN_WindowsAPCCallback_t)(ULONG_PTR parameter);

/**
 * @brief Windows事件处理接口
 */
typedef struct Stru_CN_WindowsEventInterface_t
{
    // 事件创建和管理
    HANDLE (*event_create)(BOOL manual_reset, BOOL initial_state, const char* name);
    BOOL (*event_destroy)(HANDLE event);
    BOOL (*event_set)(HANDLE event);
    BOOL (*event_reset)(HANDLE event);
    BOOL (*event_pulse)(HANDLE event);
    
    // 事件等待
    DWORD (*event_wait_single)(HANDLE event, DWORD timeout_ms);
    DWORD (*event_wait_multiple)(HANDLE* events, DWORD count, BOOL wait_all, DWORD timeout_ms);
    BOOL (*event_wait_alertable)(HANDLE event, DWORD timeout_ms);
    
    // 事件属性
    BOOL (*event_get_state)(HANDLE event);
    BOOL (*event_set_name)(HANDLE event, const char* name);
    char* (*event_get_name)(HANDLE event);
    
} Stru_CN_WindowsEventInterface_t;

/**
 * @brief Windows APC（异步过程调用）接口
 */
typedef struct Stru_CN_WindowsAPCInterface_t
{
    // APC队列管理
    BOOL (*apc_queue_user)(HANDLE thread, PAPCFUNC apc_func, ULONG_PTR data);
    BOOL (*apc_queue_io)(HANDLE file_handle, LPOVERLAPPED overlapped, 
                        LPOVERLAPPED_COMPLETION_ROUTINE completion_routine);
    
    // APC执行控制
    BOOL (*apc_enter_alertable_wait)(void);
    BOOL (*apc_exit_alertable_wait)(void);
    DWORD (*apc_sleep_ex)(DWORD timeout_ms, BOOL alertable);
    
    // APC回调函数
    BOOL (*apc_register_callback)(CN_WindowsAPCCallback_t callback, ULONG_PTR data);
    BOOL (*apc_unregister_callback)(CN_WindowsAPCCallback_t callback);
    
} Stru_CN_WindowsAPCInterface_t;

/**
 * @brief Windows I/O完成端口接口
 */
typedef struct Stru_CN_WindowsIOCompletionInterface_t
{
    // 完成端口创建和管理
    HANDLE (*iocp_create)(DWORD max_concurrent_threads);
    BOOL (*iocp_destroy)(HANDLE iocp);
    BOOL (*iocp_associate_device)(HANDLE iocp, HANDLE device, ULONG_PTR completion_key);
    
    // I/O操作
    BOOL (*iocp_post_status)(HANDLE iocp, ULONG_PTR completion_key, DWORD bytes_transferred, 
                            LPOVERLAPPED overlapped);
    BOOL (*iocp_get_status)(HANDLE iocp, LPDWORD bytes_transferred, PULONG_PTR completion_key, 
                           LPOVERLAPPED* overlapped, DWORD timeout_ms);
    
    // 完成端口线程池
    BOOL (*iocp_create_thread_pool)(HANDLE iocp, DWORD thread_count);
    BOOL (*iocp_destroy_thread_pool)(HANDLE iocp);
    DWORD (*iocp_get_thread_count)(HANDLE iocp);
    
} Stru_CN_WindowsIOCompletionInterface_t;

/**
 * @brief Windows中断线程接口
 */
typedef struct Stru_CN_WindowsInterruptThreadInterface_t
{
    // 中断线程管理
    HANDLE (*interrupt_thread_create)(LPTHREAD_START_ROUTINE thread_func, LPVOID arg);
    BOOL (*interrupt_thread_join)(HANDLE thread, DWORD timeout_ms);
    BOOL (*interrupt_thread_detach)(HANDLE thread);
    
    // 中断线程属性
    BOOL (*interrupt_thread_set_priority)(HANDLE thread, int priority);
    BOOL (*interrupt_thread_get_priority)(HANDLE thread, int* priority);
    BOOL (*interrupt_thread_set_affinity)(HANDLE thread, DWORD_PTR affinity_mask);
    BOOL (*interrupt_thread_get_affinity)(HANDLE thread, DWORD_PTR* affinity_mask);
    
    // 中断线程同步
    BOOL (*interrupt_thread_signal)(HANDLE thread, HANDLE event);
    BOOL (*interrupt_thread_wait_event)(HANDLE thread, HANDLE event, DWORD timeout_ms);
    
} Stru_CN_WindowsInterruptThreadInterface_t;

/**
 * @brief 获取Windows事件处理接口
 * 
 * @return Windows事件处理接口指针
 */
Stru_CN_WindowsEventInterface_t* CN_platform_windows_get_event_interface(void);

/**
 * @brief 获取Windows APC接口
 * 
 * @return Windows APC接口指针
 */
Stru_CN_WindowsAPCInterface_t* CN_platform_windows_get_apc_interface(void);

/**
 * @brief 获取Windows I/O完成端口接口
 * 
 * @return Windows I/O完成端口接口指针
 */
Stru_CN_WindowsIOCompletionInterface_t* CN_platform_windows_get_iocp_interface(void);

/**
 * @brief 获取Windows中断线程接口
 * 
 * @return Windows中断线程接口指针
 */
Stru_CN_WindowsInterruptThreadInterface_t* CN_platform_windows_get_interrupt_thread_interface(void);

/**
 * @brief 将Windows事件转换为CN中断号
 * 
 * @param event_id Windows事件ID
 * @return 对应的CN中断号
 */
CN_InterruptNumber_t CN_platform_windows_event_to_interrupt(int event_id);

/**
 * @brief 将CN中断号转换为Windows事件
 * 
 * @param interrupt CN中断号
 * @return 对应的Windows事件ID，如果无效返回-1
 */
int CN_platform_windows_interrupt_to_event(CN_InterruptNumber_t interrupt);

/**
 * @brief 检查事件是否可用作中断
 * 
 * @param event_id 事件ID
 * @return 如果可用返回true，否则返回false
 */
bool CN_platform_windows_event_is_available(int event_id);

/**
 * @brief 获取默认中断事件掩码
 * 
 * @param mask 输出参数，事件掩码
 * @return 如果成功返回true，否则返回false
 */
bool CN_platform_windows_get_default_event_mask(DWORD* mask);

// ============================================================================
// Windows平台中断处理全局接口变量声明（外部可见）
// ============================================================================

/** Windows中断控制器接口全局变量 */
extern Stru_CN_InterruptControllerInterface_t g_windows_interrupt_controller_interface;

/** Windows中断管理器接口全局变量 */
extern Stru_CN_InterruptManagerInterface_t g_windows_interrupt_manager_interface;

/** Windows中断工具接口全局变量 */
extern Stru_CN_InterruptToolsInterface_t g_windows_interrupt_tools_interface;

/** Windows事件处理接口全局变量 */
extern Stru_CN_WindowsEventInterface_t g_windows_event_interface;

/** Windows APC接口全局变量 */
extern Stru_CN_WindowsAPCInterface_t g_windows_apc_interface;

/** Windows I/O完成端口接口全局变量 */
extern Stru_CN_WindowsIOCompletionInterface_t g_windows_iocp_interface;

/** Windows中断线程接口全局变量 */
extern Stru_CN_WindowsInterruptThreadInterface_t g_windows_interrupt_thread_interface;

#ifdef __cplusplus
}
#endif

#endif // CN_PLATFORM_WINDOWS_INTERRUPT_H
