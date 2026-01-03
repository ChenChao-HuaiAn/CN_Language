/******************************************************************************
 * 文件名: CN_platform_windows.h
 * 功能: CN_Language Windows平台实现接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义Windows平台接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_PLATFORM_WINDOWS_H
#define CN_PLATFORM_WINDOWS_H

#include "../CN_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Windows平台常量定义
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

// ============================================================================
// Windows平台类型定义
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

// 网络错误枚举
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

// 套接字类型枚举
typedef enum Eum_CN_SocketType_t
{
    Eum_SOCKET_TYPE_STREAM = 0,      /**< 流套接字（TCP） */
    Eum_SOCKET_TYPE_DATAGRAM = 1,    /**< 数据报套接字（UDP） */
    Eum_SOCKET_TYPE_RAW = 2          /**< 原始套接字 */
} Eum_CN_SocketType_t;

// 套接字协议枚举
typedef enum Eum_CN_SocketProtocol_t
{
    Eum_SOCKET_PROTOCOL_TCP = 0,     /**< TCP协议 */
    Eum_SOCKET_PROTOCOL_UDP = 1,     /**< UDP协议 */
    Eum_SOCKET_PROTOCOL_IP = 2       /**< IP协议 */
} Eum_CN_SocketProtocol_t;

// 套接字选项枚举
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

// 网络接口信息结构体（数据存储）
typedef struct Stru_CN_WindowsNetworkInterfaceInfo_t
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
} Stru_CN_WindowsNetworkInterfaceInfo_t;

// 网络统计信息结构体
typedef struct Stru_CN_WindowsNetworkStatistics_t
{
    uint64_t bytes_received;                 /**< 接收字节数 */
    uint64_t bytes_sent;                     /**< 发送字节数 */
    uint64_t packets_received;               /**< 接收包数 */
    uint64_t packets_sent;                   /**< 发送包数 */
    uint64_t errors_received;                /**< 接收错误数 */
    uint64_t errors_sent;                    /**< 发送错误数 */
    uint64_t dropped_packets_received;       /**< 接收丢包数 */
    uint64_t dropped_packets_sent;           /**< 发送丢包数 */
} Stru_CN_WindowsNetworkStatistics_t;

// ============================================================================
// Windows平台接口声明
// ============================================================================

/**
 * @brief 获取Windows平台文件系统接口
 * 
 * @return Windows平台文件系统接口指针
 */
Stru_CN_FileSystemInterface_t* CN_platform_windows_get_filesystem(void);

/**
 * @brief 获取Windows平台线程接口
 * 
 * @return Windows平台线程接口指针
 */
Stru_CN_ThreadInterface_t* CN_platform_windows_get_thread(void);

/**
 * @brief 获取Windows平台网络接口
 * 
 * @return Windows平台网络接口指针
 */
Stru_CN_NetworkInterface_t* CN_platform_windows_get_network(void);

/**
 * @brief 获取Windows平台时间接口
 * 
 * @return Windows平台时间接口指针
 */
Stru_CN_TimeInterface_t* CN_platform_windows_get_time(void);

/**
 * @brief 获取Windows平台系统接口
 * 
 * @return Windows平台系统接口指针
 */
Stru_CN_SystemInterface_t* CN_platform_windows_get_system(void);

/**
 * @brief 获取完整的Windows平台接口
 * 
 * @return Windows平台接口指针
 */
Stru_CN_PlatformInterface_t* CN_platform_windows_get_interface(void);

/**
 * @brief 初始化Windows平台实现
 * 
 * @return 初始化成功返回true，失败返回false
 */
bool CN_platform_windows_initialize(void);

/**
 * @brief 清理Windows平台实现
 */
void CN_platform_windows_cleanup(void);

// ============================================================================
// Windows特定功能接口
// ============================================================================

/**
 * @brief Windows注册表操作接口
 */
typedef struct Stru_CN_WindowsRegistryInterface_t
{
    // 注册表键操作
    void* (*reg_open_key)(const char* path, uint32_t access);
    bool (*reg_close_key)(void* key);
    
    // 值读取
    bool (*reg_get_value_string)(void* key, const char* name, char* buffer, size_t size);
    bool (*reg_get_value_dword)(void* key, const char* name, uint32_t* value);
    bool (*reg_get_value_binary)(void* key, const char* name, void* buffer, size_t* size);
    
    // 值写入
    bool (*reg_set_value_string)(void* key, const char* name, const char* value);
    bool (*reg_set_value_dword)(void* key, const char* name, uint32_t value);
    bool (*reg_set_value_binary)(void* key, const char* name, const void* value, size_t size);
    
    // 枚举操作
    bool (*reg_enum_keys)(void* key, uint32_t index, char* name_buffer, size_t name_size);
    bool (*reg_enum_values)(void* key, uint32_t index, char* name_buffer, size_t name_size, 
                           uint32_t* type, void* data_buffer, size_t* data_size);
    
    // 键管理
    bool (*reg_create_key)(const char* path);
    bool (*reg_delete_key)(const char* path);
    bool (*reg_delete_value)(void* key, const char* name);
    
} Stru_CN_WindowsRegistryInterface_t;

/**
 * @brief Windows窗口系统接口
 */
typedef struct Stru_CN_WindowsGUIInterface_t
{
    // 窗口创建和管理
    void* (*window_create)(const char* title, int width, int height);
    bool (*window_destroy)(void* window);
    bool (*window_show)(void* window);
    bool (*window_hide)(void* window);
    
    // 消息循环
    bool (*process_messages)(void* window);
    void (*set_message_handler)(void* window, void (*handler)(void* msg));
    
    // 控件创建
    void* (*create_button)(void* parent, const char* text, int x, int y, int width, int height);
    void* (*create_label)(void* parent, const char* text, int x, int y, int width, int height);
    void* (*create_textbox)(void* parent, int x, int y, int width, int height);
    void* (*create_listbox)(void* parent, int x, int y, int width, int height);
    
    // 控件操作
    bool (*set_control_text)(void* control, const char* text);
    char* (*get_control_text)(void* control);
    bool (*add_list_item)(void* listbox, const char* item);
    bool (*clear_list)(void* listbox);
    
    // 对话框
    bool (*show_message_box)(const char* title, const char* message, uint32_t type);
    char* (*show_open_file_dialog)(const char* title, const char* filter);
    char* (*show_save_file_dialog)(const char* title, const char* filter);
    char* (*show_select_folder_dialog)(const char* title);
    
} Stru_CN_WindowsGUIInterface_t;

/**
 * @brief Windows COM组件接口
 */
typedef struct Stru_CN_WindowsCOMInterface_t
{
    // COM初始化
    bool (*com_initialize)(void);
    void (*com_uninitialize)(void);
    
    // COM对象创建
    void* (*com_create_instance)(const char* clsid, const char* iid);
    bool (*com_release_instance)(void* com_object);
    
    // COM方法调用
    bool (*com_invoke_method)(void* com_object, const char* method_name, 
                             void* params, uint32_t param_count, void* result);
    
    // 自动化支持
    bool (*com_get_property)(void* com_object, const char* property_name, void* value);
    bool (*com_set_property)(void* com_object, const char* property_name, void* value);
    bool (*com_call_method)(void* com_object, const char* method_name, 
                           void* args, uint32_t arg_count, void* result);
    
} Stru_CN_WindowsCOMInterface_t;

/**
 * @brief 获取Windows注册表接口
 * 
 * @return Windows注册表接口指针
 */
Stru_CN_WindowsRegistryInterface_t* CN_platform_windows_get_registry(void);

/**
 * @brief 获取Windows GUI接口
 * 
 * @return Windows GUI接口指针
 */
Stru_CN_WindowsGUIInterface_t* CN_platform_windows_get_gui(void);

/**
 * @brief 获取Windows COM接口
 * 
 * @return Windows COM接口指针
 */
Stru_CN_WindowsCOMInterface_t* CN_platform_windows_get_com(void);

// ============================================================================
// Windows平台全局接口变量声明（外部可见）
// ============================================================================

/** Windows文件系统接口全局变量 */
extern Stru_CN_FileSystemInterface_t g_windows_filesystem_interface;

/** Windows线程接口全局变量 */
extern Stru_CN_ThreadInterface_t g_windows_thread_interface;

/** Windows网络接口全局变量 */
extern Stru_CN_NetworkInterface_t g_windows_network_interface;

/** Windows时间接口全局变量 */
extern Stru_CN_TimeInterface_t g_windows_time_interface;

/** Windows系统接口全局变量 */
extern Stru_CN_SystemInterface_t g_windows_system_interface;

/** Windows注册表接口全局变量 */
extern Stru_CN_WindowsRegistryInterface_t g_windows_registry_interface;

/** Windows GUI接口全局变量 */
extern Stru_CN_WindowsGUIInterface_t g_windows_gui_interface;

/** Windows COM接口全局变量 */
extern Stru_CN_WindowsCOMInterface_t g_windows_com_interface;

#ifdef __cplusplus
}
#endif

#endif // CN_PLATFORM_WINDOWS_H
