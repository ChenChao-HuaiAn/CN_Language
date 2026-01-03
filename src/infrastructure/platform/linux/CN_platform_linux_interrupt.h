/******************************************************************************
 * 文件名: CN_platform_linux_interrupt.h
 * 功能: CN_Language Linux平台中断处理实现接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义Linux平台中断处理接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_PLATFORM_LINUX_INTERRUPT_H
#define CN_PLATFORM_LINUX_INTERRUPT_H

#include "../CN_interrupt.h"
#include "../CN_platform.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

// 条件包含Linux特定头文件
#ifdef __linux__
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Linux平台中断处理常量定义
// ============================================================================

/** 最大中断处理程序数量 */
#define CN_LINUX_MAX_INTERRUPT_HANDLERS 256

/** 最大中断名称长度 */
#define CN_LINUX_MAX_INTERRUPT_NAME 64

/** 中断信号范围 */
#define CN_LINUX_SIGNAL_MIN 1
#define CN_LINUX_SIGNAL_MAX 64

/** Linux特定中断标志 */
#define CN_LINUX_INTERRUPT_FLAG_SIGNAL_BASED   0x0001  /**< 基于信号的中断 */
#define CN_LINUX_INTERRUPT_FLAG_THREAD_SAFE    0x0002  /**< 线程安全中断处理 */
#define CN_LINUX_INTERRUPT_FLAG_ASYNC_SAFE     0x0004  /**< 异步信号安全处理 */
#define CN_LINUX_INTERRUPT_FLAG_REALTIME       0x0008  /**< 实时信号支持 */
#define CN_LINUX_INTERRUPT_FLAG_SA_RESTART     0x0010  /**< 系统调用自动重启 */

/** Linux信号到中断映射 */
#define CN_LINUX_SIGNAL_TO_INTERRUPT(sig) ((CN_InterruptNumber_t)((sig) + 1000))

/** Linux中断到信号映射 */
#define CN_LINUX_INTERRUPT_TO_SIGNAL(intr) ((int)((intr) - 1000))

// ============================================================================
// Linux平台中断处理类型定义
// ============================================================================

/** Linux中断处理程序信息结构体 */
typedef struct Stru_CN_LinuxInterruptHandlerInfo_t
{
    CN_InterruptNumber_t interrupt_number;      /**< 中断号 */
    CN_InterruptHandler_t handler;              /**< 中断处理函数 */
    void* user_data;                            /**< 用户数据 */
    char name[CN_LINUX_MAX_INTERRUPT_NAME];     /**< 中断名称 */
    CN_InterruptFlags_t flags;                  /**< 中断标志 */
    void* old_action;                           /**< 旧的信号处理动作（平台特定） */
    bool is_installed;                          /**< 是否已安装 */
    void* lock;                                 /**< 线程安全锁（平台特定） */
} Stru_CN_LinuxInterruptHandlerInfo_t;

/** Linux中断控制器信息结构体 */
typedef struct Stru_CN_LinuxInterruptControllerInfo_t
{
    bool is_initialized;                        /**< 是否已初始化 */
    Stru_CN_LinuxInterruptHandlerInfo_t handlers[CN_LINUX_MAX_INTERRUPT_HANDLERS]; /**< 中断处理程序数组 */
    uint32_t handler_count;                     /**< 已注册的处理程序数量 */
    void* global_lock;                          /**< 全局锁（平台特定） */
    void* blocked_signals;                      /**< 被阻塞的信号集（平台特定） */
    void* original_signal_mask;                 /**< 原始信号掩码（平台特定） */
} Stru_CN_LinuxInterruptControllerInfo_t;

/** Linux中断管理器信息结构体 */
typedef struct Stru_CN_LinuxInterruptManagerInfo_t
{
    bool is_initialized;                        /**< 是否已初始化 */
    Stru_CN_LinuxInterruptControllerInfo_t* controller; /**< 关联的控制器 */
    CN_InterruptNumber_t allocated_interrupts[CN_LINUX_MAX_INTERRUPT_HANDLERS]; /**< 已分配的中断号 */
    uint32_t allocated_count;                   /**< 已分配数量 */
    void* allocation_lock;                      /**< 分配锁（平台特定） */
} Stru_CN_LinuxInterruptManagerInfo_t;

/** Linux中断工具信息结构体 */
typedef struct Stru_CN_LinuxInterruptToolsInfo_t
{
    bool is_initialized;                        /**< 是否已初始化 */
    Stru_CN_InterruptContext_t current_context; /**< 当前中断上下文 */
    bool in_interrupt_context;                  /**< 是否在中断上下文中 */
    void* interrupt_context_key;                /**< 中断上下文线程特定数据键（平台特定） */
    uint64_t interrupt_count;                   /**< 中断计数 */
    uint64_t nested_interrupt_depth;            /**< 嵌套中断深度 */
} Stru_CN_LinuxInterruptToolsInfo_t;

// ============================================================================
// Linux平台中断处理接口声明
// ============================================================================

/**
 * @brief 获取Linux平台中断控制器接口
 * 
 * @return Linux平台中断控制器接口指针
 */
Stru_CN_InterruptControllerInterface_t* CN_platform_linux_get_interrupt_controller(void);

/**
 * @brief 获取Linux平台中断管理器接口
 * 
 * @return Linux平台中断管理器接口指针
 */
Stru_CN_InterruptManagerInterface_t* CN_platform_linux_get_interrupt_manager(void);

/**
 * @brief 获取Linux平台中断工具接口
 * 
 * @return Linux平台中断工具接口指针
 */
Stru_CN_InterruptToolsInterface_t* CN_platform_linux_get_interrupt_tools(void);

/**
 * @brief 初始化Linux平台中断处理系统
 * 
 * @return 初始化成功返回true，失败返回false
 */
bool CN_platform_linux_interrupt_initialize(void);

/**
 * @brief 清理Linux平台中断处理系统
 */
void CN_platform_linux_interrupt_cleanup(void);

/**
 * @brief 检查Linux平台中断处理是否已初始化
 * 
 * @return 如果已初始化返回true，否则返回false
 */
bool CN_platform_linux_interrupt_is_initialized(void);

// ============================================================================
// Linux特定中断处理功能
// ============================================================================

/**
 * @brief Linux信号处理接口
 */
typedef struct Stru_CN_LinuxSignalInterface_t
{
    // 信号处理管理
    bool (*signal_register)(int signum, void (*handler)(int), void* user_data);
    bool (*signal_unregister)(int signum);
    bool (*signal_restore)(int signum);
    
    // 信号集操作
    bool (*sigemptyset)(sigset_t* set);
    bool (*sigfillset)(sigset_t* set);
    bool (*sigaddset)(sigset_t* set, int signum);
    bool (*sigdelset)(sigset_t* set, int signum);
    bool (*sigismember)(const sigset_t* set, int signum);
    
    // 信号阻塞控制
    bool (*sigprocmask_block)(const sigset_t* set, sigset_t* oldset);
    bool (*sigprocmask_unblock)(const sigset_t* set, sigset_t* oldset);
    bool (*sigprocmask_set)(const sigset_t* set, sigset_t* oldset);
    
    // 信号等待
    int (*sigwait)(const sigset_t* set, int* sig);
    int (*sigtimedwait)(const sigset_t* set, siginfo_t* info, const struct timespec* timeout);
    int (*sigwaitinfo)(const sigset_t* set, siginfo_t* info);
    
    // 信号发送
    int (*kill)(pid_t pid, int sig);
    int (*raise)(int sig);
    int (*sigqueue)(pid_t pid, int sig, const union sigval value);
    
    // 信号处理属性
    bool (*sigaction_set_flags)(int signum, int flags);
    bool (*sigaction_get_flags)(int signum, int* flags);
    
} Stru_CN_LinuxSignalInterface_t;

/**
 * @brief Linux实时信号接口
 */
typedef struct Stru_CN_LinuxRealtimeSignalInterface_t
{
    // 实时信号管理
    int (*realtime_signal_allocate)(void);
    bool (*realtime_signal_free)(int sig);
    bool (*realtime_signal_is_available)(int sig);
    
    // 实时信号操作
    bool (*realtime_signal_register)(int sig, void (*handler)(int, siginfo_t*, void*), void* user_data);
    bool (*realtime_signal_unregister)(int sig);
    
    // 实时信号队列
    bool (*realtime_signal_queue_empty)(int sig);
    int (*realtime_signal_queue_size)(int sig);
    bool (*realtime_signal_queue_clear)(int sig);
    
    // 实时信号属性
    bool (*realtime_signal_set_priority)(int sig, int priority);
    bool (*realtime_signal_get_priority)(int sig, int* priority);
    
} Stru_CN_LinuxRealtimeSignalInterface_t;

/**
 * @brief Linux中断线程接口
 */
typedef struct Stru_CN_LinuxInterruptThreadInterface_t
{
    // 中断线程管理
    pthread_t (*interrupt_thread_create)(void (*thread_func)(void*), void* arg);
    bool (*interrupt_thread_join)(pthread_t thread, void** retval);
    bool (*interrupt_thread_detach)(pthread_t thread);
    
    // 中断线程属性
    bool (*interrupt_thread_set_priority)(pthread_t thread, int priority);
    bool (*interrupt_thread_get_priority)(pthread_t thread, int* priority);
    bool (*interrupt_thread_set_signal_mask)(pthread_t thread, const sigset_t* mask);
    bool (*interrupt_thread_get_signal_mask)(pthread_t thread, sigset_t* mask);
    
    // 中断线程同步
    bool (*interrupt_thread_signal)(pthread_t thread, int sig);
    bool (*interrupt_thread_wait_signal)(const sigset_t* set, int* sig);
    
} Stru_CN_LinuxInterruptThreadInterface_t;

/**
 * @brief 获取Linux信号处理接口
 * 
 * @return Linux信号处理接口指针
 */
Stru_CN_LinuxSignalInterface_t* CN_platform_linux_get_signal_interface(void);

/**
 * @brief 获取Linux实时信号接口
 * 
 * @return Linux实时信号接口指针
 */
Stru_CN_LinuxRealtimeSignalInterface_t* CN_platform_linux_get_realtime_signal_interface(void);

/**
 * @brief 获取Linux中断线程接口
 * 
 * @return Linux中断线程接口指针
 */
Stru_CN_LinuxInterruptThreadInterface_t* CN_platform_linux_get_interrupt_thread_interface(void);

/**
 * @brief 将Linux信号转换为CN中断号
 * 
 * @param signum Linux信号编号
 * @return 对应的CN中断号
 */
CN_InterruptNumber_t CN_platform_linux_signal_to_interrupt(int signum);

/**
 * @brief 将CN中断号转换为Linux信号
 * 
 * @param interrupt CN中断号
 * @return 对应的Linux信号编号，如果无效返回-1
 */
int CN_platform_linux_interrupt_to_signal(CN_InterruptNumber_t interrupt);

/**
 * @brief 检查信号是否可用作中断
 * 
 * @param signum 信号编号
 * @return 如果可用返回true，否则返回false
 */
bool CN_platform_linux_signal_is_available(int signum);

/**
 * @brief 获取默认中断信号掩码
 * 
 * @param mask 输出参数，信号掩码
 * @return 如果成功返回true，否则返回false
 */
bool CN_platform_linux_get_default_signal_mask(sigset_t* mask);

// ============================================================================
// Linux平台中断处理全局接口变量声明（外部可见）
// ============================================================================

/** Linux中断控制器接口全局变量 */
extern Stru_CN_InterruptControllerInterface_t g_linux_interrupt_controller_interface;

/** Linux中断管理器接口全局变量 */
extern Stru_CN_InterruptManagerInterface_t g_linux_interrupt_manager_interface;

/** Linux中断工具接口全局变量 */
extern Stru_CN_InterruptToolsInterface_t g_linux_interrupt_tools_interface;

/** Linux信号处理接口全局变量 */
extern Stru_CN_LinuxSignalInterface_t g_linux_signal_interface;

/** Linux实时信号接口全局变量 */
extern Stru_CN_LinuxRealtimeSignalInterface_t g_linux_realtime_signal_interface;

/** Linux中断线程接口全局变量 */
extern Stru_CN_LinuxInterruptThreadInterface_t g_linux_interrupt_thread_interface;

#ifdef __cplusplus
}
#endif

#endif // CN_PLATFORM_LINUX_INTERRUPT_H
