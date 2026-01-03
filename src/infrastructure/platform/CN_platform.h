/******************************************************************************
 * 文件名: CN_platform.h
 * 功能: CN_Language平台抽象层接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义平台抽象层接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_PLATFORM_H
#define CN_PLATFORM_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 基本类型定义
// ============================================================================

/**
 * @brief 文件句柄类型（平台相关）
 */
typedef void* CN_FileHandle_t;

/**
 * @brief 文件打开模式枚举
 */
typedef enum Eum_CN_FileMode_t
{
    Eum_FILE_MODE_READ = 0,           /**< 只读模式 */
    Eum_FILE_MODE_WRITE = 1,          /**< 只写模式（创建新文件） */
    Eum_FILE_MODE_APPEND = 2,         /**< 追加模式 */
    Eum_FILE_MODE_READ_WRITE = 3,     /**< 读写模式 */
    Eum_FILE_MODE_CREATE = 4          /**< 创建新文件（如果存在则截断） */
} Eum_CN_FileMode_t;

/**
 * @brief 文件属性枚举
 */
typedef enum Eum_CN_FileAttribute_t
{
    Eum_FILE_ATTR_READONLY = 0x01,    /**< 只读文件 */
    Eum_FILE_ATTR_HIDDEN = 0x02,      /**< 隐藏文件 */
    Eum_FILE_ATTR_SYSTEM = 0x04,      /**< 系统文件 */
    Eum_FILE_ATTR_DIRECTORY = 0x08,   /**< 目录 */
    Eum_FILE_ATTR_ARCHIVE = 0x10,     /**< 存档文件 */
    Eum_FILE_ATTR_NORMAL = 0x20       /**< 普通文件 */
} Eum_CN_FileAttribute_t;

/**
 * @brief 文件信息结构体
 */
typedef struct Stru_CN_FileInfo_t
{
    char name[256];                   /**< 文件名 */
    uint64_t size;                    /**< 文件大小（字节） */
    uint64_t creation_time;           /**< 创建时间（平台相关时间戳） */
    uint64_t modification_time;       /**< 修改时间（平台相关时间戳） */
    uint64_t access_time;             /**< 访问时间（平台相关时间戳） */
    uint32_t attributes;              /**< 文件属性（Eum_CN_FileAttribute_t的位组合） */
} Stru_CN_FileInfo_t;

/**
 * @brief 线程句柄类型
 */
typedef void* CN_ThreadHandle_t;

/**
 * @brief 互斥锁句柄类型
 */
typedef void* CN_MutexHandle_t;

/**
 * @brief 信号量句柄类型
 */
typedef void* CN_SemaphoreHandle_t;

/**
 * @brief 条件变量句柄类型
 */
typedef void* CN_ConditionHandle_t;

/**
 * @brief 套接字句柄类型
 */
typedef void* CN_SocketHandle_t;

/**
 * @brief 线程函数类型
 */
typedef void (*CN_ThreadFunc)(void* user_data);

/**
 * @brief 网络地址结构体
 */
typedef struct Stru_CN_NetworkAddress_t
{
    uint8_t family;                   /**< 地址族 */
    uint8_t data[16];                 /**< 地址数据（IPv4或IPv6） */
    uint16_t port;                    /**< 端口号（网络字节序） */
} Stru_CN_NetworkAddress_t;

// ============================================================================
// 文件系统操作接口
// ============================================================================

/**
 * @brief 文件系统操作接口结构体
 */
typedef struct Stru_CN_FileSystemInterface_t
{
    // 文件存在性检查
    bool (*file_exists)(const char* path);
    
    // 文件打开和关闭
    CN_FileHandle_t (*file_open)(const char* path, Eum_CN_FileMode_t mode);
    bool (*file_close)(CN_FileHandle_t handle);
    
    // 文件读写
    size_t (*file_read)(CN_FileHandle_t handle, void* buffer, size_t size);
    size_t (*file_write)(CN_FileHandle_t handle, const void* buffer, size_t size);
    
    // 文件定位
    bool (*file_seek)(CN_FileHandle_t handle, int64_t offset, int whence);
    int64_t (*file_tell)(CN_FileHandle_t handle);
    
    // 文件属性
    bool (*file_get_info)(const char* path, Stru_CN_FileInfo_t* info);
    bool (*file_set_attributes)(const char* path, uint32_t attributes);
    
    // 文件操作
    bool (*file_delete)(const char* path);
    bool (*file_rename)(const char* old_path, const char* new_path);
    bool (*file_copy)(const char* src_path, const char* dst_path);
    
    // 目录操作
    bool (*directory_create)(const char* path);
    bool (*directory_delete)(const char* path);
    bool (*directory_exists)(const char* path);
    
    // 目录遍历
    void* (*directory_open)(const char* path);
    bool (*directory_read)(void* dir_handle, Stru_CN_FileInfo_t* info);
    bool (*directory_close)(void* dir_handle);
    
    // 路径操作
    bool (*path_is_absolute)(const char* path);
    bool (*path_is_relative)(const char* path);
    char* (*path_combine)(const char* path1, const char* path2);
    char* (*path_get_directory)(const char* path);
    char* (*path_get_filename)(const char* path);
    char* (*path_get_extension)(const char* path);
    
    // 特殊路径
    char* (*get_current_directory)(void);
    bool (*set_current_directory)(const char* path);
    char* (*get_home_directory)(void);
    char* (*get_temp_directory)(void);
    
    // 文件系统信息
    uint64_t (*get_disk_free_space)(const char* path);
    uint64_t (*get_disk_total_space)(const char* path);
    
} Stru_CN_FileSystemInterface_t;

// ============================================================================
// 线程和同步接口
// ============================================================================

/**
 * @brief 线程和同步接口结构体
 */
typedef struct Stru_CN_ThreadInterface_t
{
    // 线程创建和管理
    CN_ThreadHandle_t (*thread_create)(CN_ThreadFunc func, void* user_data, const char* name);
    bool (*thread_join)(CN_ThreadHandle_t thread, int* exit_code);
    bool (*thread_detach)(CN_ThreadHandle_t thread);
    void (*thread_sleep)(uint32_t milliseconds);
    void (*thread_yield)(void);
    
    // 线程本地存储
    void* (*tls_allocate)(void);
    bool (*tls_free)(void* key);
    bool (*tls_set_value)(void* key, void* value);
    void* (*tls_get_value)(void* key);
    
    // 互斥锁
    CN_MutexHandle_t (*mutex_create)(void);
    bool (*mutex_destroy)(CN_MutexHandle_t mutex);
    bool (*mutex_lock)(CN_MutexHandle_t mutex);
    bool (*mutex_try_lock)(CN_MutexHandle_t mutex);
    bool (*mutex_unlock)(CN_MutexHandle_t mutex);
    
    // 递归互斥锁
    CN_MutexHandle_t (*recursive_mutex_create)(void);
    bool (*recursive_mutex_destroy)(CN_MutexHandle_t mutex);
    bool (*recursive_mutex_lock)(CN_MutexHandle_t mutex);
    bool (*recursive_mutex_try_lock)(CN_MutexHandle_t mutex);
    bool (*recursive_mutex_unlock)(CN_MutexHandle_t mutex);
    
    // 信号量
    CN_SemaphoreHandle_t (*semaphore_create)(int initial_count, int max_count);
    bool (*semaphore_destroy)(CN_SemaphoreHandle_t semaphore);
    bool (*semaphore_wait)(CN_SemaphoreHandle_t semaphore, uint32_t timeout_ms);
    bool (*semaphore_post)(CN_SemaphoreHandle_t semaphore);
    
    // 条件变量
    CN_ConditionHandle_t (*condition_create)(void);
    bool (*condition_destroy)(CN_ConditionHandle_t condition);
    bool (*condition_wait)(CN_ConditionHandle_t condition, CN_MutexHandle_t mutex);
    bool (*condition_timed_wait)(CN_ConditionHandle_t condition, 
                                 CN_MutexHandle_t mutex, uint32_t timeout_ms);
    bool (*condition_signal)(CN_ConditionHandle_t condition);
    bool (*condition_broadcast)(CN_ConditionHandle_t condition);
    
    // 原子操作
    int32_t (*atomic_increment)(volatile int32_t* value);
    int32_t (*atomic_decrement)(volatile int32_t* value);
    int32_t (*atomic_add)(volatile int32_t* value, int32_t addend);
    int32_t (*atomic_compare_exchange)(volatile int32_t* dest, 
                                       int32_t exchange, int32_t comparand);
    
    // 内存屏障
    void (*memory_barrier)(void);
    
} Stru_CN_ThreadInterface_t;

// ============================================================================
// 网络接口
// ============================================================================

/**
 * @brief 网络协议枚举
 */
typedef enum Eum_CN_NetworkProtocol_t
{
    Eum_NET_PROTOCOL_TCP = 0,         /**< TCP协议 */
    Eum_NET_PROTOCOL_UDP = 1          /**< UDP协议 */
} Eum_CN_NetworkProtocol_t;

/**
 * @brief 网络接口结构体
 */
typedef struct Stru_CN_NetworkInterface_t
{
    // 网络初始化
    bool (*network_initialize)(void);
    void (*network_cleanup)(void);
    
    // 地址解析
    bool (*address_from_string)(const char* str, Stru_CN_NetworkAddress_t* addr);
    char* (*address_to_string)(const Stru_CN_NetworkAddress_t* addr);
    
    // 套接字创建和关闭
    CN_SocketHandle_t (*socket_create)(Eum_CN_NetworkProtocol_t protocol);
    bool (*socket_close)(CN_SocketHandle_t socket);
    
    // TCP服务器操作
    bool (*socket_bind)(CN_SocketHandle_t socket, const Stru_CN_NetworkAddress_t* addr);
    bool (*socket_listen)(CN_SocketHandle_t socket, int backlog);
    CN_SocketHandle_t (*socket_accept)(CN_SocketHandle_t server_socket, 
                                       Stru_CN_NetworkAddress_t* client_addr);
    
    // TCP客户端操作
    bool (*socket_connect)(CN_SocketHandle_t socket, const Stru_CN_NetworkAddress_t* addr);
    
    // 数据发送和接收
    int32_t (*socket_send)(CN_SocketHandle_t socket, const void* data, size_t size);
    int32_t (*socket_receive)(CN_SocketHandle_t socket, void* buffer, size_t size);
    
    // UDP操作
    int32_t (*socket_send_to)(CN_SocketHandle_t socket, const void* data, size_t size,
                              const Stru_CN_NetworkAddress_t* addr);
    int32_t (*socket_receive_from)(CN_SocketHandle_t socket, void* buffer, size_t size,
                                   Stru_CN_NetworkAddress_t* addr);
    
    // 套接字选项
    bool (*socket_set_option)(CN_SocketHandle_t socket, int option, const void* value, size_t size);
    bool (*socket_get_option)(CN_SocketHandle_t socket, int option, void* value, size_t* size);
    
    // 非阻塞IO
    bool (*socket_set_nonblocking)(CN_SocketHandle_t socket, bool nonblocking);
    bool (*socket_is_nonblocking)(CN_SocketHandle_t socket);
    
    // 选择器/轮询
    void* (*selector_create)(void);
    bool (*selector_destroy)(void* selector);
    bool (*selector_add)(void* selector, CN_SocketHandle_t socket, void* user_data);
    bool (*selector_remove)(void* selector, CN_SocketHandle_t socket);
    int (*selector_wait)(void* selector, uint32_t timeout_ms);
    bool (*selector_is_readable)(void* selector, CN_SocketHandle_t socket);
    bool (*selector_is_writable)(void* selector, CN_SocketHandle_t socket);
    
    // 网络信息
    char* (*get_host_name)(void);
    bool (*get_host_addresses)(Stru_CN_NetworkAddress_t* addresses, size_t* count);
    
} Stru_CN_NetworkInterface_t;

// ============================================================================
// 时间和日期接口
// ============================================================================

/**
 * @brief 时间结构体
 */
typedef struct Stru_CN_Time_t
{
    uint16_t year;          /**< 年份 */
    uint8_t month;          /**< 月份 (1-12) */
    uint8_t day;            /**< 日 (1-31) */
    uint8_t hour;           /**< 小时 (0-23) */
    uint8_t minute;         /**< 分钟 (0-59) */
    uint8_t second;         /**< 秒 (0-59) */
    uint16_t millisecond;   /**< 毫秒 (0-999) */
} Stru_CN_Time_t;

/**
 * @brief 时间和日期接口结构体
 */
typedef struct Stru_CN_TimeInterface_t
{
    // 获取当前时间
    uint64_t (*get_current_time)(void);                     /**< 获取当前时间戳（毫秒） */
    uint64_t (*get_current_time_nanos)(void);               /**< 获取当前时间戳（纳秒） */
    
    // 时间转换
    bool (*time_to_local)(uint64_t timestamp, Stru_CN_Time_t* local_time);
    bool (*time_to_utc)(uint64_t timestamp, Stru_CN_Time_t* utc_time);
    uint64_t (*time_from_local)(const Stru_CN_Time_t* local_time);
    uint64_t (*time_from_utc)(const Stru_CN_Time_t* utc_time);
    
    // 时间格式化
    char* (*format_time)(uint64_t timestamp, const char* format);
    bool (*parse_time)(const char* str, const char* format, uint64_t* timestamp);
    
    // 高精度计时
    uint64_t (*get_performance_counter)(void);              /**< 获取性能计数器值 */
    uint64_t (*get_performance_frequency)(void);            /**< 获取性能计数器频率 */
    
    // 时间操作
    void (*sleep)(uint32_t milliseconds);                   /**< 睡眠指定毫秒数 */
    void (*sleep_nanos)(uint64_t nanoseconds);              /**< 睡眠指定纳秒数 */
    
    // 时区信息
    int32_t (*get_timezone_offset)(void);                   /**< 获取时区偏移（分钟） */
    bool (*is_daylight_saving_time)(void);                  /**< 是否夏令时 */
    
    // 日历操作
    uint8_t (*get_day_of_week)(uint16_t year, uint8_t month, uint8_t day);
    uint16_t (*get_day_of_year)(uint16_t year, uint8_t month, uint8_t day);
    bool (*is_leap_year)(uint16_t year);
    
} Stru_CN_TimeInterface_t;

// ============================================================================
// 系统信息接口
// ============================================================================

/**
 * @brief 系统信息结构体
 */
typedef struct Stru_CN_SystemInfo_t
{
    char os_name[64];               /**< 操作系统名称 */
    char os_version[64];            /**< 操作系统版本 */
    char architecture[32];          /**< 系统架构 */
    uint32_t page_size;             /**< 内存页大小（字节） */
    uint64_t total_physical_memory; /**< 总物理内存（字节） */
    uint64_t available_physical_memory; /**< 可用物理内存（字节） */
    uint32_t processor_count;       /**< 处理器核心数 */
    char processor_name[64];        /**< 处理器名称 */
    uint32_t processor_speed_mhz;   /**< 处理器速度（MHz） */
} Stru_CN_SystemInfo_t;

/**
 * @brief 系统信息接口结构体
 */
typedef struct Stru_CN_SystemInterface_t
{
    // 系统信息获取
    bool (*get_system_info)(Stru_CN_SystemInfo_t* info);
    
    // 内存信息
    uint64_t (*get_total_memory)(void);
    uint64_t (*get_available_memory)(void);
    uint64_t (*get_process_memory_usage)(void);
    
    // CPU信息
    uint32_t (*get_cpu_count)(void);
    double (*get_cpu_usage)(void);                     /**< 获取CPU使用率（0.0-1.0） */
    
    // 环境变量
    char* (*get_environment_variable)(const char* name);
    bool (*set_environment_variable)(const char* name, const char* value);
    
    // 进程信息
    uint32_t (*get_process_id)(void);
    uint32_t (*get_parent_process_id)(void);
    char* (*get_process_name)(void);
    
    // 命令行参数
    char* (*get_command_line)(void);
    int (*get_argument_count)(void);
    char* (*get_argument)(int index);
    
    // 系统路径
    char* (*get_system_directory)(void);
    char* (*get_program_files_directory)(void);
    
    // 用户信息
    char* (*get_user_name)(void);
    char* (*get_user_home_directory)(void);
    
    // 系统通知
    bool (*show_message_box)(const char* title, const char* message, uint32_t flags);
    bool (*play_sound)(const char* sound_name);
    
    // 电源管理
    bool (*get_power_status)(uint32_t* battery_percent, bool* is_charging);
    bool (*set_power_saving_mode)(bool enable);
    
    // 系统控制
    bool (*shutdown_system)(uint32_t timeout_seconds);
    bool (*reboot_system)(uint32_t timeout_seconds);
    bool (*logoff_user)(void);
    
} Stru_CN_SystemInterface_t;

// ============================================================================
// 统一平台接口
// ============================================================================

/**
 * @brief 统一平台接口结构体
 * 
 * 包含所有平台相关功能的接口指针，通过依赖注入提供平台特定实现。
 */
typedef struct Stru_CN_PlatformInterface_t
{
    Stru_CN_FileSystemInterface_t* filesystem;   /**< 文件系统接口 */
    Stru_CN_ThreadInterface_t* thread;           /**< 线程和同步接口 */
    Stru_CN_NetworkInterface_t* network;         /**< 网络接口 */
    Stru_CN_TimeInterface_t* time;               /**< 时间和日期接口 */
    Stru_CN_SystemInterface_t* system;           /**< 系统信息接口 */
} Stru_CN_PlatformInterface_t;

// ============================================================================
// 平台接口管理函数
// ============================================================================

/**
 * @brief 获取默认平台接口
 * 
 * 根据当前运行平台返回相应的平台接口实现。
 * 
 * @return 平台接口指针，失败返回NULL
 */
Stru_CN_PlatformInterface_t* CN_platform_get_default(void);

/**
 * @brief 创建自定义平台接口
 * 
 * 允许用户提供自定义的平台接口实现。
 * 
 * @param filesystem 文件系统接口（可为NULL）
 * @param thread 线程接口（可为NULL）
 * @param network 网络接口（可为NULL）
 * @param time 时间接口（可为NULL）
 * @param system 系统接口（可为NULL）
 * @return 新创建的平台接口，失败返回NULL
 */
Stru_CN_PlatformInterface_t* CN_platform_create_custom(
    Stru_CN_FileSystemInterface_t* filesystem,
    Stru_CN_ThreadInterface_t* thread,
    Stru_CN_NetworkInterface_t* network,
    Stru_CN_TimeInterface_t* time,
    Stru_CN_SystemInterface_t* system);

/**
 * @brief 销毁平台接口
 * 
 * @param platform 要销毁的平台接口
 */
void CN_platform_destroy(Stru_CN_PlatformInterface_t* platform);

/**
 * @brief 检查平台接口是否完整
 * 
 * 检查平台接口的所有必需组件是否都已设置。
 * 
 * @param platform 要检查的平台接口
 * @return 如果接口完整返回true，否则返回false
 */
bool CN_platform_is_complete(const Stru_CN_PlatformInterface_t* platform);

// ============================================================================
// 平台检测函数
// ============================================================================

/**
 * @brief 获取当前平台类型
 * 
 * @return 平台类型字符串（"windows", "linux", "macos", "unknown"）
 */
const char* CN_platform_get_type(void);

/**
 * @brief 检查是否为Windows平台
 * 
 * @return 如果是Windows平台返回true，否则返回false
 */
bool CN_platform_is_windows(void);

/**
 * @brief 检查是否为Linux平台
 * 
 * @return 如果是Linux平台返回true，否则返回false
 */
bool CN_platform_is_linux(void);

/**
 * @brief 检查是否为macOS平台
 * 
 * @return 如果是macOS平台返回true，否则返回false
 */
bool CN_platform_is_macos(void);

/**
 * @brief 检查是否为Unix-like平台
 * 
 * @return 如果是Unix-like平台返回true，否则返回false
 */
bool CN_platform_is_unix(void);

// ============================================================================
// 平台初始化函数
// ============================================================================

/**
 * @brief 初始化平台抽象层
 * 
 * 必须在调用任何平台函数之前调用此函数。
 * 
 * @return 初始化成功返回true，失败返回false
 */
bool CN_platform_initialize(void);

/**
 * @brief 清理平台抽象层
 * 
 * 在程序退出前调用此函数释放平台资源。
 */
void CN_platform_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // CN_PLATFORM_H
