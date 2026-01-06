/**
 * @file CN_platform_interface.h
 * @brief 平台抽象层接口定义
 * 
 * 本文件定义了跨平台的操作系统接口，包括文件系统、进程、时间等功能。
 * 通过抽象接口实现平台无关性，支持Windows、Linux、macOS等操作系统。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @note 设计原则：
 * 1. 单一职责：每个接口只负责一个功能领域
 * 2. 开闭原则：通过抽象接口支持扩展
 * 3. 接口隔离：为不同客户端提供专用接口
 * 4. 依赖倒置：高层模块定义接口，低层模块实现
 */

#ifndef CN_PLATFORM_INTERFACE_H
#define CN_PLATFORM_INTERFACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 常量定义
// ============================================================================

/** @brief 最大路径长度 */
#define CN_MAX_PATH_LENGTH 4096

/** @brief 最大文件名长度 */
#define CN_MAX_FILENAME_LENGTH 256

/** @brief 文件打开模式 */
typedef enum Eum_FileOpenMode
{
    Eum_FILE_MODE_READ,          /**< 只读模式 */
    Eum_FILE_MODE_WRITE,         /**< 只写模式（创建新文件） */
    Eum_FILE_MODE_APPEND,        /**< 追加模式 */
    Eum_FILE_MODE_READ_WRITE,    /**< 读写模式 */
    Eum_FILE_MODE_CREATE         /**< 创建新文件（如果存在则截断） */
} Eum_FileOpenMode;

/** @brief 文件属性 */
typedef enum Eum_FileAttribute
{
    Eum_FILE_ATTR_READONLY = 0x01,      /**< 只读文件 */
    Eum_FILE_ATTR_HIDDEN = 0x02,        /**< 隐藏文件 */
    Eum_FILE_ATTR_SYSTEM = 0x04,        /**< 系统文件 */
    Eum_FILE_ATTR_DIRECTORY = 0x10,     /**< 目录 */
    Eum_FILE_ATTR_ARCHIVE = 0x20,       /**< 存档文件 */
    Eum_FILE_ATTR_NORMAL = 0x80         /**< 普通文件 */
} Eum_FileAttribute;

/** @brief 进程退出状态 */
typedef int32_t CN_ProcessExitStatus;

/** @brief 无效的文件句柄 */
#define CN_INVALID_FILE_HANDLE ((void*)0)

/** @brief 无效的进程句柄 */
#define CN_INVALID_PROCESS_HANDLE ((void*)0)

// ============================================================================
// 数据结构定义
// ============================================================================

/** @brief 文件信息结构体 */
typedef struct Stru_FileInfo_t
{
    char filename[CN_MAX_FILENAME_LENGTH];  /**< 文件名 */
    uint64_t size;                          /**< 文件大小（字节） */
    uint64_t creation_time;                 /**< 创建时间（Unix时间戳） */
    uint64_t modification_time;             /**< 修改时间（Unix时间戳） */
    uint64_t access_time;                   /**< 访问时间（Unix时间戳） */
    uint32_t attributes;                    /**< 文件属性（Eum_FileAttribute的位组合） */
} Stru_FileInfo_t;

/** @brief 目录条目结构体 */
typedef struct Stru_DirectoryEntry_t
{
    char name[CN_MAX_FILENAME_LENGTH];      /**< 条目名称 */
    bool is_directory;                      /**< 是否为目录 */
    uint64_t size;                          /**< 文件大小（如果是文件） */
    uint64_t modification_time;             /**< 修改时间 */
} Stru_DirectoryEntry_t;

/** @brief 进程信息结构体 */
typedef struct Stru_ProcessInfo_t
{
    uint32_t process_id;                    /**< 进程ID */
    uint32_t parent_process_id;             /**< 父进程ID */
    uint64_t creation_time;                 /**< 创建时间 */
    uint64_t cpu_time;                      /**< CPU时间（毫秒） */
    uint64_t memory_usage;                  /**< 内存使用量（字节） */
} Stru_ProcessInfo_t;

/** @brief 时间信息结构体 */
typedef struct Stru_TimeInfo_t
{
    uint16_t year;                          /**< 年份 */
    uint8_t month;                          /**< 月份（1-12） */
    uint8_t day;                            /**< 日（1-31） */
    uint8_t hour;                           /**< 小时（0-23） */
    uint8_t minute;                         /**< 分钟（0-59） */
    uint8_t second;                         /**< 秒（0-59） */
    uint16_t millisecond;                   /**< 毫秒（0-999） */
    int8_t timezone_offset;                 /**< 时区偏移（小时） */
} Stru_TimeInfo_t;

// ============================================================================
// 文件系统接口
// ============================================================================

/** @brief 文件系统接口结构体 */
typedef struct Stru_FileSystemInterface_t
{
    /**
     * @brief 打开文件
     * @param path 文件路径
     * @param mode 打开模式
     * @return 文件句柄，失败返回CN_INVALID_FILE_HANDLE
     */
    void* (*open_file)(const char* path, Eum_FileOpenMode mode);
    
    /**
     * @brief 关闭文件
     * @param file_handle 文件句柄
     * @return 成功返回true，失败返回false
     */
    bool (*close_file)(void* file_handle);
    
    /**
     * @brief 读取文件
     * @param file_handle 文件句柄
     * @param buffer 缓冲区
     * @param size 要读取的字节数
     * @return 实际读取的字节数，失败返回-1
     */
    int64_t (*read_file)(void* file_handle, void* buffer, size_t size);
    
    /**
     * @brief 写入文件
     * @param file_handle 文件句柄
     * @param buffer 数据缓冲区
     * @param size 要写入的字节数
     * @return 实际写入的字节数，失败返回-1
     */
    int64_t (*write_file)(void* file_handle, const void* buffer, size_t size);
    
    /**
     * @brief 移动文件指针
     * @param file_handle 文件句柄
     * @param offset 偏移量
     * @param origin 起始位置（0=文件开始，1=当前位置，2=文件末尾）
     * @return 新的文件位置，失败返回-1
     */
    int64_t (*seek_file)(void* file_handle, int64_t offset, int origin);
    
    /**
     * @brief 获取文件大小
     * @param file_handle 文件句柄
     * @return 文件大小（字节），失败返回-1
     */
    int64_t (*get_file_size)(void* file_handle);
    
    /**
     * @brief 获取文件信息
     * @param path 文件路径
     * @param info 文件信息结构体指针
     * @return 成功返回true，失败返回false
     */
    bool (*get_file_info)(const char* path, Stru_FileInfo_t* info);
    
    /**
     * @brief 删除文件
     * @param path 文件路径
     * @return 成功返回true，失败返回false
     */
    bool (*delete_file)(const char* path);
    
    /**
     * @brief 重命名文件
     * @param old_path 原路径
     * @param new_path 新路径
     * @return 成功返回true，失败返回false
     */
    bool (*rename_file)(const char* old_path, const char* new_path);
    
    /**
     * @brief 复制文件
     * @param source_path 源文件路径
     * @param dest_path 目标文件路径
     * @return 成功返回true，失败返回false
     */
    bool (*copy_file)(const char* source_path, const char* dest_path);
    
    /**
     * @brief 创建目录
     * @param path 目录路径
     * @return 成功返回true，失败返回false
     */
    bool (*create_directory)(const char* path);
    
    /**
     * @brief 删除目录
     * @param path 目录路径
     * @return 成功返回true，失败返回false
     */
    bool (*delete_directory)(const char* path);
    
    /**
     * @brief 打开目录
     * @param path 目录路径
     * @return 目录句柄，失败返回NULL
     */
    void* (*open_directory)(const char* path);
    
    /**
     * @brief 读取目录条目
     * @param dir_handle 目录句柄
     * @param entry 目录条目结构体指针
     * @return 成功返回true，没有更多条目返回false
     */
    bool (*read_directory)(void* dir_handle, Stru_DirectoryEntry_t* entry);
    
    /**
     * @brief 关闭目录
     * @param dir_handle 目录句柄
     * @return 成功返回true，失败返回false
     */
    bool (*close_directory)(void* dir_handle);
    
    /**
     * @brief 检查路径是否存在
     * @param path 路径
     * @return 存在返回true，不存在返回false
     */
    bool (*path_exists)(const char* path);
    
    /**
     * @brief 检查路径是否为目录
     * @param path 路径
     * @return 是目录返回true，否则返回false
     */
    bool (*is_directory)(const char* path);
    
    /**
     * @brief 获取当前工作目录
     * @param buffer 缓冲区
     * @param size 缓冲区大小
     * @return 成功返回true，失败返回false
     */
    bool (*get_current_directory)(char* buffer, size_t size);
    
    /**
     * @brief 设置当前工作目录
     * @param path 目录路径
     * @return 成功返回true，失败返回false
     */
    bool (*set_current_directory)(const char* path);
    
    /**
     * @brief 获取临时目录路径
     * @param buffer 缓冲区
     * @param size 缓冲区大小
     * @return 成功返回true，失败返回false
     */
    bool (*get_temp_directory)(char* buffer, size_t size);
    
} Stru_FileSystemInterface_t;

// ============================================================================
// 进程管理接口
// ============================================================================

/** @brief 进程管理接口结构体 */
typedef struct Stru_ProcessInterface_t
{
    /**
     * @brief 创建进程
     * @param command 命令字符串
     * @param arguments 参数数组（以NULL结尾）
     * @param working_directory 工作目录（可为NULL）
     * @return 进程句柄，失败返回CN_INVALID_PROCESS_HANDLE
     */
    void* (*create_process)(const char* command, const char* const* arguments, const char* working_directory);
    
    /**
     * @brief 等待进程结束
     * @param process_handle 进程句柄
     * @param timeout_ms 超时时间（毫秒，0表示无限等待）
     * @param exit_status 退出状态指针（可为NULL）
     * @return 成功返回true，超时或失败返回false
     */
    bool (*wait_process)(void* process_handle, uint32_t timeout_ms, CN_ProcessExitStatus* exit_status);
    
    /**
     * @brief 终止进程
     * @param process_handle 进程句柄
     * @return 成功返回true，失败返回false
     */
    bool (*terminate_process)(void* process_handle);
    
    /**
     * @brief 获取进程信息
     * @param process_handle 进程句柄
     * @param info 进程信息结构体指针
     * @return 成功返回true，失败返回false
     */
    bool (*get_process_info)(void* process_handle, Stru_ProcessInfo_t* info);
    
    /**
     * @brief 获取当前进程ID
     * @return 当前进程ID
     */
    uint32_t (*get_current_process_id)(void);
    
    /**
     * @brief 获取当前线程ID
     * @return 当前线程ID
     */
    uint32_t (*get_current_thread_id)(void);
    
    /**
     * @brief 睡眠指定毫秒数
     * @param milliseconds 毫秒数
     */
    void (*sleep)(uint32_t milliseconds);
    
    /**
     * @brief 获取环境变量
     * @param name 变量名
     * @param buffer 缓冲区
     * @param size 缓冲区大小
     * @return 成功返回true，失败返回false
     */
    bool (*get_environment_variable)(const char* name, char* buffer, size_t size);
    
    /**
     * @brief 设置环境变量
     * @param name 变量名
     * @param value 变量值
     * @return 成功返回true，失败返回false
     */
    bool (*set_environment_variable)(const char* name, const char* value);
    
    /**
     * @brief 执行系统命令
     * @param command 命令字符串
     * @param output 输出缓冲区（可为NULL）
     * @param output_size 输出缓冲区大小
     * @return 命令退出状态
     */
    CN_ProcessExitStatus (*execute_command)(const char* command, char* output, size_t output_size);
    
} Stru_ProcessInterface_t;

// ============================================================================
// 时间管理接口
// ============================================================================

/** @brief 时间管理接口结构体 */
typedef struct Stru_TimeInterface_t
{
    /**
     * @brief 获取当前时间（Unix时间戳，毫秒）
     * @return Unix时间戳（毫秒）
     */
    uint64_t (*get_current_timestamp)(void);
    
    /**
     * @brief 获取当前时间（高精度，纳秒）
     * @return 高精度时间（纳秒）
     */
    uint64_t (*get_high_resolution_time)(void);
    
    /**
     * @brief 获取本地时间
     * @param timestamp Unix时间戳（毫秒）
     * @param time_info 时间信息结构体指针
     * @return 成功返回true，失败返回false
     */
    bool (*get_local_time)(uint64_t timestamp, Stru_TimeInfo_t* time_info);
    
    /**
     * @brief 获取UTC时间
     * @param timestamp Unix时间戳（毫秒）
     * @param time_info 时间信息结构体指针
     * @return 成功返回true，失败返回false
     */
    bool (*get_utc_time)(uint64_t timestamp, Stru_TimeInfo_t* time_info);
    
    /**
     * @brief 格式化时间字符串
     * @param time_info 时间信息结构体指针
     * @param format 格式字符串（如"%Y-%m-%d %H:%M:%S"）
     * @param buffer 输出缓冲区
     * @param size 缓冲区大小
     * @return 成功返回true，失败返回false
     */
    bool (*format_time)(const Stru_TimeInfo_t* time_info, const char* format, char* buffer, size_t size);
    
    /**
     * @brief 解析时间字符串
     * @param time_string 时间字符串
     * @param format 格式字符串
     * @param time_info 时间信息结构体指针
     * @return 成功返回true，失败返回false
     */
    bool (*parse_time)(const char* time_string, const char* format, Stru_TimeInfo_t* time_info);
    
    /**
     * @brief 获取系统启动时间
     * @return 系统启动时间（毫秒）
     */
    uint64_t (*get_system_uptime)(void);
    
    /**
     * @brief 获取CPU时钟频率
     * @return CPU时钟频率（Hz）
     */
    uint64_t (*get_cpu_frequency)(void);
    
    /**
     * @brief 获取时区信息
     * @param timezone_name 时区名称缓冲区
     * @param size 缓冲区大小
     * @param offset 时区偏移（小时）
     * @return 成功返回true，失败返回false
     */
    bool (*get_timezone_info)(char* timezone_name, size_t size, int8_t* offset);
    
} Stru_TimeInterface_t;

// ============================================================================
// 平台抽象层主接口
// ============================================================================

/** @brief 平台抽象层主接口结构体 */
typedef struct Stru_PlatformInterface_t
{
    /** @brief 文件系统接口 */
    const Stru_FileSystemInterface_t* file_system;
    
    /** @brief 进程管理接口 */
    const Stru_ProcessInterface_t* process;
    
    /** @brief 时间管理接口 */
    const Stru_TimeInterface_t* time;
    
    /**
     * @brief 初始化平台抽象层
     * @return 成功返回true，失败返回false
     */
    bool (*initialize)(void);
    
    /**
     * @brief 清理平台抽象层资源
     */
    void (*cleanup)(void);
    
    /**
     * @brief 获取平台名称
     * @return 平台名称字符串
     */
    const char* (*get_platform_name)(void);
    
    /**
     * @brief 获取平台版本
     * @return 平台版本字符串
     */
    const char* (*get_platform_version)(void);
    
    /**
     * @brief 获取CPU架构信息
     * @return CPU架构字符串
     */
    const char* (*get_cpu_architecture)(void);
    
    /**
     * @brief 获取系统内存信息
     * @param total_memory 总内存大小（字节）
     * @param available_memory 可用内存大小（字节）
     * @return 成功返回true，失败返回false
     */
    bool (*get_system_memory_info)(uint64_t* total_memory, uint64_t* available_memory);
    
    /**
     * @brief 获取CPU核心数
     * @return CPU核心数
     */
    uint32_t (*get_cpu_core_count)(void);
    
    /**
     * @brief 获取系统信息
     * @param info_buffer 信息缓冲区
     * @param buffer_size 缓冲区大小
     * @return 成功返回true，失败返回false
     */
    bool (*get_system_info)(char* info_buffer, size_t buffer_size);
    
} Stru_PlatformInterface_t;

// ============================================================================
// 平台抽象层工厂函数
// ============================================================================

/**
 * @brief 获取默认平台接口
 * @return 平台接口指针，失败返回NULL
 */
const Stru_PlatformInterface_t* F_get_default_platform_interface(void);

/**
 * @brief 获取Windows平台接口
 * @return Windows平台接口指针，失败返回NULL
 */
const Stru_PlatformInterface_t* F_get_windows_platform_interface(void);

/**
 * @brief 获取Linux平台接口
 * @return Linux平台接口指针，失败返回NULL
 */
const Stru_PlatformInterface_t* F_get_linux_platform_interface(void);

/**
 * @brief 获取macOS平台接口
 * @return macOS平台接口指针，失败返回NULL
 */
const Stru_PlatformInterface_t* F_get_macos_platform_interface(void);

/**
 * @brief 获取测试桩平台接口（用于单元测试）
 * @return 测试桩平台接口指针
 */
const Stru_PlatformInterface_t* F_get_stub_platform_interface(void);

#ifdef __cplusplus
}
#endif

#endif // CN_PLATFORM_INTERFACE_H
