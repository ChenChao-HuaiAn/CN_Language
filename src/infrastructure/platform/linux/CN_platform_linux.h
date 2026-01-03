/******************************************************************************
 * 文件名: CN_platform_linux.h
 * 功能: CN_Language Linux平台实现接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义Linux平台接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_PLATFORM_LINUX_H
#define CN_PLATFORM_LINUX_H

#include "../CN_platform.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Linux平台常量定义
// ============================================================================

/** 最大线程名称长度 */
#define CN_MAX_THREAD_NAME 64

/** 最大互斥锁名称长度 */
#define CN_MAX_MUTEX_NAME 64

/** 最大信号量名称长度 */
#define CN_MAX_SEMAPHORE_NAME 64

/** 最大条件变量名称长度 */
#define CN_MAX_CONDITION_NAME 64

/** 最大读写锁名称长度 */
#define CN_MAX_RWLOCK_NAME 64

// 网络相关常量
#define CN_MAX_ADDR_STRING 46          /**< 最大地址字符串长度（IPv6地址） */
#define CN_MAX_INTERFACE_NAME 256      /**< 最大网络接口名称长度 */
#define CN_MAX_INTERFACE_DESC 256      /**< 最大网络接口描述长度 */
#define CN_MAX_MAC_ADDRESS 18          /**< 最大MAC地址字符串长度（XX:XX:XX:XX:XX:XX） */

// Linux特定功能标志
#define CN_LINUX_FEATURE_INOTIFY   0x0001  /**< inotify文件监控支持 */
#define CN_LINUX_FEATURE_EPOLL     0x0002  /**< epoll事件通知支持 */
#define CN_LINUX_FEATURE_SIGNALFD  0x0004  /**< signalfd信号处理支持 */
#define CN_LINUX_FEATURE_TIMERFD   0x0008  /**< timerfd定时器支持 */
#define CN_LINUX_FEATURE_EVENTFD   0x0010  /**< eventfd事件通知支持 */
#define CN_LINUX_FEATURE_MEMFD     0x0020  /**< memfd内存文件支持 */
#define CN_LINUX_FEATURE_USERFAULTFD 0x0040 /**< userfaultfd用户空间缺页处理 */

// ============================================================================
// Linux平台类型定义
// ============================================================================

/** 读写锁句柄类型 */
typedef void* CN_RWLockHandle_t;

/** 线程优先级枚举 */
typedef enum Eum_CN_ThreadPriority_t
{
    Eum_THREAD_PRIORITY_LOWEST = 0,          /**< 最低优先级 */
    Eum_THREAD_PRIORITY_BELOW_NORMAL = 1,    /**< 低于正常优先级 */
    Eum_THREAD_PRIORITY_NORMAL = 2,          /**< 正常优先级 */
    Eum_THREAD_PRIORITY_ABOVE_NORMAL = 3,    /**< 高于正常优先级 */
    Eum_THREAD_PRIORITY_HIGHEST = 4,         /**< 最高优先级 */
    Eum_THREAD_PRIORITY_TIME_CRITICAL = 5    /**< 时间关键优先级 */
} Eum_CN_ThreadPriority_t;

/** 网络错误枚举 */
typedef enum Eum_CN_NetworkError_t
{
    Eum_NETWORK_ERROR_NONE = 0,
    Eum_NETWORK_ERROR_ACCESS_DENIED,
    Eum_NETWORK_ERROR_ADDRESS_IN_USE,
    Eum_NETWORK_ERROR_ADDRESS_NOT_AVAILABLE,
    Eum_NETWORK_ERROR_ADDRESS_FAMILY_NOT_SUPPORTED,
    Eum_NETWORK_ERROR_ALREADY_IN_PROGRESS,
    Eum_NETWORK_ERROR_BAD_FILE_DESCRIPTOR,
    Eum_NETWORK_ERROR_CONNECTION_ABORTED,
    Eum_NETWORK_ERROR_CONNECTION_REFUSED,
    Eum_NETWORK_ERROR_CONNECTION_RESET,
    Eum_NETWORK_ERROR_DESTINATION_ADDRESS_REQUIRED,
    Eum_NETWORK_ERROR_FAULT,
    Eum_NETWORK_ERROR_HOST_UNREACHABLE,
    Eum_NETWORK_ERROR_IN_PROGRESS,
    Eum_NETWORK_ERROR_INTERRUPTED,
    Eum_NETWORK_ERROR_INVALID_ARGUMENT,
    Eum_NETWORK_ERROR_IS_CONNECTED,
    Eum_NETWORK_ERROR_TOO_MANY_OPEN_FILES,
    Eum_NETWORK_ERROR_MESSAGE_SIZE,
    Eum_NETWORK_ERROR_NETWORK_DOWN,
    Eum_NETWORK_ERROR_NETWORK_RESET,
    Eum_NETWORK_ERROR_NETWORK_UNREACHABLE,
    Eum_NETWORK_ERROR_NO_BUFFER_SPACE,
    Eum_NETWORK_ERROR_NO_PROTOCOL_OPTION,
    Eum_NETWORK_ERROR_NOT_CONNECTED,
    Eum_NETWORK_ERROR_NOT_A_SOCKET,
    Eum_NETWORK_ERROR_OPERATION_NOT_SUPPORTED,
    Eum_NETWORK_ERROR_PROTOCOL_NOT_SUPPORTED,
    Eum_NETWORK_ERROR_PROTOCOL_TYPE,
    Eum_NETWORK_ERROR_TIMED_OUT,
    Eum_NETWORK_ERROR_WOULD_BLOCK,
    Eum_NETWORK_ERROR_UNKNOWN
} Eum_CN_NetworkError_t;

/** 套接字类型枚举 */
typedef enum Eum_CN_SocketType_t
{
    Eum_SOCKET_TYPE_STREAM = 0,      /**< 流套接字（TCP） */
    Eum_SOCKET_TYPE_DATAGRAM = 1,    /**< 数据报套接字（UDP） */
    Eum_SOCKET_TYPE_RAW = 2          /**< 原始套接字 */
} Eum_CN_SocketType_t;

/** 套接字协议枚举 */
typedef enum Eum_CN_SocketProtocol_t
{
    Eum_SOCKET_PROTOCOL_TCP = 0,     /**< TCP协议 */
    Eum_SOCKET_PROTOCOL_UDP = 1,     /**< UDP协议 */
    Eum_SOCKET_PROTOCOL_IP = 2       /**< IP协议 */
} Eum_CN_SocketProtocol_t;

/** 套接字选项枚举 */
typedef enum Eum_CN_SocketOption_t
{
    Eum_SOCKET_OPTION_REUSE_ADDR = 0,    /**< 地址重用 */
    Eum_SOCKET_OPTION_KEEP_ALIVE = 1,    /**< 保持连接 */
    Eum_SOCKET_OPTION_BROADCAST = 2,     /**< 广播 */
    Eum_SOCKET_OPTION_LINGER = 3,        /**< 延迟关闭 */
    Eum_SOCKET_OPTION_RCVBUF = 4,        /**< 接收缓冲区大小 */
    Eum_SOCKET_OPTION_SNDBUF = 5,        /**< 发送缓冲区大小 */
    Eum_SOCKET_OPTION_RCVTIMEO = 6,      /**< 接收超时 */
    Eum_SOCKET_OPTION_SNDTIMEO = 7       /**< 发送超时 */
} Eum_CN_SocketOption_t;

/** 网络接口信息结构体 */
typedef struct Stru_CN_LinuxNetworkInterfaceInfo_t
{
    char name[CN_MAX_INTERFACE_NAME];        /**< 接口名称 */
    char description[CN_MAX_INTERFACE_DESC]; /**< 接口描述 */
    char mac_address[CN_MAX_MAC_ADDRESS];    /**< MAC地址 */
    char ip_address[CN_MAX_ADDR_STRING];     /**< IP地址 */
    char netmask[CN_MAX_ADDR_STRING];        /**< 子网掩码 */
    char gateway[CN_MAX_ADDR_STRING];        /**< 网关地址 */
    uint64_t speed;                          /**< 接口速度（bps） */
    bool is_up;                              /**< 接口是否启用 */
    bool is_loopback;                        /**< 是否是回环接口 */
} Stru_CN_LinuxNetworkInterfaceInfo_t;

/** 网络统计信息结构体 */
typedef struct Stru_CN_LinuxNetworkStatistics_t
{
    uint64_t bytes_received;                 /**< 接收字节数 */
    uint64_t bytes_sent;                     /**< 发送字节数 */
    uint64_t packets_received;               /**< 接收包数 */
    uint64_t packets_sent;                   /**< 发送包数 */
    uint64_t errors_received;                /**< 接收错误数 */
    uint64_t errors_sent;                    /**< 发送错误数 */
    uint64_t dropped_packets_received;       /**< 接收丢包数 */
    uint64_t dropped_packets_sent;           /**< 发送丢包数 */
} Stru_CN_LinuxNetworkStatistics_t;

/** inotify事件结构体 */
typedef struct Stru_CN_LinuxInotifyEvent_t
{
    int wd;                                 /**< 监视描述符 */
    uint32_t mask;                          /**< 事件掩码 */
    uint32_t cookie;                        /**< 关联cookie */
    char name[256];                         /**< 文件名（如果有） */
} Stru_CN_LinuxInotifyEvent_t;

/** epoll事件结构体 */
typedef struct Stru_CN_LinuxEpollEvent_t
{
    uint32_t events;                        /**< 事件类型 */
    void* data;                             /**< 用户数据 */
} Stru_CN_LinuxEpollEvent_t;

// ============================================================================
// Linux平台接口声明
// ============================================================================

/**
 * @brief 获取Linux平台文件系统接口
 * 
 * @return Linux平台文件系统接口指针
 */
Stru_CN_FileSystemInterface_t* CN_platform_linux_get_filesystem(void);

/**
 * @brief 获取Linux平台线程接口
 * 
 * @return Linux平台线程接口指针
 */
Stru_CN_ThreadInterface_t* CN_platform_linux_get_thread(void);

/**
 * @brief 获取Linux平台网络接口
 * 
 * @return Linux平台网络接口指针
 */
Stru_CN_NetworkInterface_t* CN_platform_linux_get_network(void);

/**
 * @brief 获取Linux平台时间接口
 * 
 * @return Linux平台时间接口指针
 */
Stru_CN_TimeInterface_t* CN_platform_linux_get_time(void);

/**
 * @brief 获取Linux平台系统接口
 * 
 * @return Linux平台系统接口指针
 */
Stru_CN_SystemInterface_t* CN_platform_linux_get_system(void);

/**
 * @brief 获取完整的Linux平台接口
 * 
 * @return Linux平台接口指针
 */
Stru_CN_PlatformInterface_t* CN_platform_linux_get_interface(void);

/**
 * @brief 初始化Linux平台实现
 * 
 * @return 初始化成功返回true，失败返回false
 */
bool CN_platform_linux_initialize(void);

/**
 * @brief 清理Linux平台实现
 */
void CN_platform_linux_cleanup(void);

// ============================================================================
// Linux特定功能接口
// ============================================================================

/**
 * @brief Linux inotify文件监控接口
 */
typedef struct Stru_CN_LinuxInotifyInterface_t
{
    // inotify实例管理
    int (*inotify_init)(void);
    int (*inotify_init1)(int flags);
    void (*inotify_close)(int fd);
    
    // 监视管理
    int (*inotify_add_watch)(int fd, const char* pathname, uint32_t mask);
    int (*inotify_rm_watch)(int fd, int wd);
    
    // 事件读取
    ssize_t (*inotify_read_events)(int fd, Stru_CN_LinuxInotifyEvent_t* events, 
                                   size_t max_events);
    
    // 事件掩码定义
    uint32_t (*inotify_get_access_mask)(void);      /**< 文件访问掩码 */
    uint32_t (*inotify_get_modify_mask)(void);      /**< 文件修改掩码 */
    uint32_t (*inotify_get_create_mask)(void);      /**< 文件创建掩码 */
    uint32_t (*inotify_get_delete_mask)(void);      /**< 文件删除掩码 */
    uint32_t (*inotify_get_all_events_mask)(void);  /**< 所有事件掩码 */
    
    // 批量操作
    bool (*inotify_add_watch_recursive)(int fd, const char* pathname, uint32_t mask);
    bool (*inotify_remove_watch_recursive)(int fd, const char* pathname);
    
} Stru_CN_LinuxInotifyInterface_t;

/**
 * @brief Linux epoll事件通知接口
 */
typedef struct Stru_CN_LinuxEpollInterface_t
{
    // epoll实例管理
    int (*epoll_create)(int size);
    int (*epoll_create1)(int flags);
    void (*epoll_close)(int epfd);
    
    // 事件管理
    int (*epoll_ctl)(int epfd, int op, int fd, Stru_CN_LinuxEpollEvent_t* event);
    int (*epoll_wait)(int epfd, Stru_CN_LinuxEpollEvent_t* events, 
                      int maxevents, int timeout);
    int (*epoll_pwait)(int epfd, Stru_CN_LinuxEpollEvent_t* events,
                       int maxevents, int timeout, const sigset_t* sigmask);
    
    // 事件类型定义
    uint32_t (*epoll_get_in_event)(void);           /**< EPOLLIN事件 */
    uint32_t (*epoll_get_out_event)(void);          /**< EPOLLOUT事件 */
    uint32_t (*epoll_get_err_event)(void);          /**< EPOLLERR事件 */
    uint32_t (*epoll_get_hup_event)(void);          /**< EPOLLHUP事件 */
    uint32_t (*epoll_get_et_mode)(void);            /**< 边缘触发模式 */
    uint32_t (*epoll_get_oneshot_mode)(void);       /**< 一次性事件模式 */
    
    // 高级功能
    bool (*epoll_add_timerfd)(int epfd, int timerfd, void* user_data);
    bool (*epoll_add_signalfd)(int epfd, int signalfd, void* user_data);
    bool (*epoll_add_eventfd)(int epfd, int eventfd, void* user_data);
    
} Stru_CN_LinuxEpollInterface_t;

/**
 * @brief Linux文件描述符工具接口
 */
typedef struct Stru_CN_LinuxFDInterface_t
{
    // 特殊文件描述符创建
    int (*timerfd_create)(int clockid, int flags);
    int (*signalfd)(int fd, const sigset_t* mask, int flags);
    int (*eventfd)(unsigned int initval, int flags);
    int (*memfd_create)(const char* name, unsigned int flags);
    
    // 文件描述符操作
    bool (*fd_set_nonblocking)(int fd, bool nonblocking);
    bool (*fd_set_cloexec)(int fd, bool cloexec);
    bool (*fd_duplicate)(int oldfd, int newfd);
    void (*fd_close)(int fd);
    
    // 管道和套接字对
    int (*pipe_create)(int pipefd[2], int flags);
    int (*socketpair)(int domain, int type, int protocol, int sv[2]);
    
    // 文件描述符传递
    bool (*fd_send)(int sockfd, int fd);
    int (*fd_receive)(int sockfd);
    
} Stru_CN_LinuxFDInterface_t;

/**
 * @brief Linux系统调用接口
 */
typedef struct Stru_CN_LinuxSyscallInterface_t
{
    // 进程管理
    pid_t (*fork)(void);
    pid_t (*vfork)(void);
    int (*execve)(const char* pathname, char* const argv[], char* const envp[]);
    pid_t (*waitpid)(pid_t pid, int* wstatus, int options);
    
    // 信号处理
    void (*signal)(int signum, void (*handler)(int));  // 使用void (*)(int)代替sighandler_t
    int (*sigaction)(int signum, const struct sigaction* act, struct sigaction* oldact);
    int (*sigprocmask)(int how, const sigset_t* set, sigset_t* oldset);
    
    // 内存管理
    void* (*mmap)(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
    int (*munmap)(void* addr, size_t length);
    int (*mprotect)(void* addr, size_t length, int prot);
    int (*madvise)(void* addr, size_t length, int advice);
    
    // IO控制
    int (*ioctl)(int fd, unsigned long request, ...);
    int (*fcntl)(int fd, int cmd, ...);
    
    // 系统信息
    long (*sysconf)(int name);
    int (*uname)(struct utsname* buf);
    
} Stru_CN_LinuxSyscallInterface_t;

/**
 * @brief 获取Linux inotify接口
 * 
 * @return Linux inotify接口指针
 */
Stru_CN_LinuxInotifyInterface_t* CN_platform_linux_get_inotify(void);

/**
 * @brief 获取Linux epoll接口
 * 
 * @return Linux epoll接口指针
 */
Stru_CN_LinuxEpollInterface_t* CN_platform_linux_get_epoll(void);

/**
 * @brief 获取Linux文件描述符工具接口
 * 
 * @return Linux文件描述符工具接口指针
 */
Stru_CN_LinuxFDInterface_t* CN_platform_linux_get_fd_tools(void);

/**
 * @brief 获取Linux系统调用接口
 * 
 * @return Linux系统调用接口指针
 */
Stru_CN_LinuxSyscallInterface_t* CN_platform_linux_get_syscall(void);

/**
 * @brief 检查Linux特定功能是否可用
 * 
 * @param feature 功能标志（CN_LINUX_FEATURE_*）
 * @return 如果功能可用返回true，否则返回false
 */
bool CN_platform_linux_check_feature(uint32_t feature);

/**
 * @brief 获取Linux内核版本
 * 
 * @param major 主版本号输出参数
 * @param minor 次版本号输出参数
 * @param patch 修订号输出参数
 * @return 如果成功获取返回true，否则返回false
 */
bool CN_platform_linux_get_kernel_version(int* major, int* minor, int* patch);

/**
 * @brief 获取Linux发行版信息
 * 
 * @param name 发行版名称缓冲区
 * @param name_size 名称缓冲区大小
 * @param version 版本号缓冲区
 * @param version_size 版本号缓冲区大小
 * @return 如果成功获取返回true，否则返回false
 */
bool CN_platform_linux_get_distro_info(char* name, size_t name_size, 
                                       char* version, size_t version_size);

// ============================================================================
// Linux平台全局接口变量声明（外部可见）
// ============================================================================

/** Linux文件系统接口全局变量 */
extern Stru_CN_FileSystemInterface_t g_linux_filesystem_interface;

/** Linux线程接口全局变量 */
extern Stru_CN_ThreadInterface_t g_linux_thread_interface;

/** Linux网络接口全局变量 */
extern Stru_CN_NetworkInterface_t g_linux_network_interface;

/** Linux时间接口全局变量 */
extern Stru_CN_TimeInterface_t g_linux_time_interface;

/** Linux系统接口全局变量 */
extern Stru_CN_SystemInterface_t g_linux_system_interface;

/** Linux inotify接口全局变量 */
extern Stru_CN_LinuxInotifyInterface_t g_linux_inotify_interface;

/** Linux epoll接口全局变量 */
extern Stru_CN_LinuxEpollInterface_t g_linux_epoll_interface;

/** Linux文件描述符工具接口全局变量 */
extern Stru_CN_LinuxFDInterface_t g_linux_fd_tools_interface;

/** Linux系统调用接口全局变量 */
extern Stru_CN_LinuxSyscallInterface_t g_linux_syscall_interface;

#ifdef __cplusplus
}
#endif

#endif // CN_PLATFORM_LINUX_H
