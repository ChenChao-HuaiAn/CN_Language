# CN_platform API 文档

## 概述

`CN_platform` 模块是CN_Language项目的平台抽象层，提供跨平台的系统接口抽象。本文档详细描述了平台抽象层的API接口和使用方法。

## 文件列表

- `src/infrastructure/platform/CN_platform.h` - 主接口定义文件
- `src/infrastructure/platform/README.md` - 模块说明文档

## 基本类型定义

### 文件句柄类型
```c
typedef void* CN_FileHandle_t;
```
平台相关的文件句柄类型。

### 文件打开模式枚举
```c
typedef enum Eum_CN_FileMode_t
{
    Eum_FILE_MODE_READ = 0,           /**< 只读模式 */
    Eum_FILE_MODE_WRITE = 1,          /**< 只写模式（创建新文件） */
    Eum_FILE_MODE_APPEND = 2,         /**< 追加模式 */
    Eum_FILE_MODE_READ_WRITE = 3,     /**< 读写模式 */
    Eum_FILE_MODE_CREATE = 4          /**< 创建新文件（如果存在则截断） */
} Eum_CN_FileMode_t;
```

### 文件属性枚举
```c
typedef enum Eum_CN_FileAttribute_t
{
    Eum_FILE_ATTR_READONLY = 0x01,    /**< 只读文件 */
    Eum_FILE_ATTR_HIDDEN = 0x02,      /**< 隐藏文件 */
    Eum_FILE_ATTR_SYSTEM = 0x04,      /**< 系统文件 */
    Eum_FILE_ATTR_DIRECTORY = 0x08,   /**< 目录 */
    Eum_FILE_ATTR_ARCHIVE = 0x10,     /**< 存档文件 */
    Eum_FILE_ATTR_NORMAL = 0x20       /**< 普通文件 */
} Eum_CN_FileAttribute_t;
```

### 文件信息结构体
```c
typedef struct Stru_CN_FileInfo_t
{
    char name[256];                   /**< 文件名 */
    uint64_t size;                    /**< 文件大小（字节） */
    uint64_t creation_time;           /**< 创建时间（平台相关时间戳） */
    uint64_t modification_time;       /**< 修改时间（平台相关时间戳） */
    uint64_t access_time;             /**< 访问时间（平台相关时间戳） */
    uint32_t attributes;              /**< 文件属性（Eum_CN_FileAttribute_t的位组合） */
} Stru_CN_FileInfo_t;
```

### 线程和同步句柄类型
```c
typedef void* CN_ThreadHandle_t;      /**< 线程句柄 */
typedef void* CN_MutexHandle_t;       /**< 互斥锁句柄 */
typedef void* CN_SemaphoreHandle_t;   /**< 信号量句柄 */
typedef void* CN_ConditionHandle_t;   /**< 条件变量句柄 */
typedef void* CN_SocketHandle_t;      /**< 套接字句柄 */
```

### 线程函数类型
```c
typedef void (*CN_ThreadFunc)(void* user_data);
```

### 网络地址结构体
```c
typedef struct Stru_CN_NetworkAddress_t
{
    uint8_t family;                   /**< 地址族 */
    uint8_t data[16];                 /**< 地址数据（IPv4或IPv6） */
    uint16_t port;                    /**< 端口号（网络字节序） */
} Stru_CN_NetworkAddress_t;
```

### 网络协议枚举
```c
typedef enum Eum_CN_NetworkProtocol_t
{
    Eum_NET_PROTOCOL_TCP = 0,         /**< TCP协议 */
    Eum_NET_PROTOCOL_UDP = 1          /**< UDP协议 */
} Eum_CN_NetworkProtocol_t;
```

### 时间结构体
```c
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
```

### 系统信息结构体
```c
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
```

## 接口结构体

### 文件系统操作接口 (`Stru_CN_FileSystemInterface_t`)

#### 文件存在性检查
```c
bool (*file_exists)(const char* path);
```
检查指定路径的文件或目录是否存在。

**参数：**
- `path` - 文件或目录路径

**返回值：**
- `true` - 存在
- `false` - 不存在或发生错误

#### 文件打开
```c
CN_FileHandle_t (*file_open)(const char* path, Eum_CN_FileMode_t mode);
```
打开指定路径的文件。

**参数：**
- `path` - 文件路径
- `mode` - 打开模式

**返回值：**
- 非NULL - 文件句柄
- NULL - 打开失败

#### 文件关闭
```c
bool (*file_close)(CN_FileHandle_t handle);
```
关闭打开的文件。

**参数：**
- `handle` - 文件句柄

**返回值：**
- `true` - 关闭成功
- `false` - 关闭失败

#### 文件读取
```c
size_t (*file_read)(CN_FileHandle_t handle, void* buffer, size_t size);
```
从文件中读取数据。

**参数：**
- `handle` - 文件句柄
- `buffer` - 数据缓冲区
- `size` - 要读取的字节数

**返回值：**
- 实际读取的字节数

#### 文件写入
```c
size_t (*file_write)(CN_FileHandle_t handle, const void* buffer, size_t size);
```
向文件写入数据。

**参数：**
- `handle` - 文件句柄
- `buffer` - 数据缓冲区
- `size` - 要写入的字节数

**返回值：**
- 实际写入的字节数

#### 文件定位
```c
bool (*file_seek)(CN_FileHandle_t handle, int64_t offset, int whence);
int64_t (*file_tell)(CN_FileHandle_t handle);
```
文件定位和获取当前位置。

**参数：**
- `handle` - 文件句柄
- `offset` - 偏移量
- `whence` - 起始位置（SEEK_SET, SEEK_CUR, SEEK_END）

**返回值：**
- `file_seek`: `true`成功，`false`失败
- `file_tell`: 当前位置，-1表示错误

#### 文件信息获取
```c
bool (*file_get_info)(const char* path, Stru_CN_FileInfo_t* info);
```
获取文件或目录的详细信息。

**参数：**
- `path` - 文件或目录路径
- `info` - 输出参数，接收文件信息

**返回值：**
- `true` - 获取成功
- `false` - 获取失败

#### 文件操作
```c
bool (*file_delete)(const char* path);
bool (*file_rename)(const char* old_path, const char* new_path);
bool (*file_copy)(const char* src_path, const char* dst_path);
```
文件删除、重命名和复制操作。

**参数：**
- `path` - 文件路径
- `old_path` - 原路径
- `new_path` - 新路径
- `src_path` - 源文件路径
- `dst_path` - 目标文件路径

**返回值：**
- `true` - 操作成功
- `false` - 操作失败

#### 目录操作
```c
bool (*directory_create)(const char* path);
bool (*directory_delete)(const char* path);
bool (*directory_exists)(const char* path);
```
目录创建、删除和存在性检查。

**参数：**
- `path` - 目录路径

**返回值：**
- `true` - 操作成功/目录存在
- `false` - 操作失败/目录不存在

#### 目录遍历
```c
void* (*directory_open)(const char* path);
bool (*directory_read)(void* dir_handle, Stru_CN_FileInfo_t* info);
bool (*directory_close)(void* dir_handle);
```
打开目录、读取目录项和关闭目录。

**参数：**
- `path` - 目录路径
- `dir_handle` - 目录句柄
- `info` - 输出参数，接收文件信息

**返回值：**
- `directory_open`: 目录句柄，NULL表示失败
- `directory_read`: `true`读取成功，`false`没有更多项或错误
- `directory_close`: `true`关闭成功，`false`关闭失败

#### 路径操作
```c
bool (*path_is_absolute)(const char* path);
bool (*path_is_relative)(const char* path);
char* (*path_combine)(const char* path1, const char* path2);
char* (*path_get_directory)(const char* path);
char* (*path_get_filename)(const char* path);
char* (*path_get_extension)(const char* path);
```
路径相关操作。

**参数：**
- `path`, `path1`, `path2` - 路径字符串

**返回值：**
- `path_is_absolute/relative`: `true`是绝对/相对路径，`false`不是
- 其他函数: 新分配的字符串，需要调用者释放

#### 特殊路径
```c
char* (*get_current_directory)(void);
bool (*set_current_directory)(const char* path);
char* (*get_home_directory)(void);
char* (*get_temp_directory)(void);
```
获取和设置特殊路径。

**参数：**
- `path` - 要设置的当前目录路径

**返回值：**
- 字符串函数: 新分配的字符串，需要调用者释放
- `set_current_directory`: `true`设置成功，`false`设置失败

#### 磁盘空间查询
```c
uint64_t (*get_disk_free_space)(const char* path);
uint64_t (*get_disk_total_space)(const char* path);
```
获取磁盘空间信息。

**参数：**
- `path` - 磁盘路径

**返回值：**
- 磁盘空间大小（字节）

### 线程和同步接口 (`Stru_CN_ThreadInterface_t`)

#### 线程创建和管理
```c
CN_ThreadHandle_t (*thread_create)(CN_ThreadFunc func, void* user_data, const char* name);
bool (*thread_join)(CN_ThreadHandle_t thread, int* exit_code);
bool (*thread_detach)(CN_ThreadHandle_t thread);
void (*thread_sleep)(uint32_t milliseconds);
void (*thread_yield)(void);
```
线程创建、加入、分离、睡眠和让出CPU。

**参数：**
- `func` - 线程函数
- `user_data` - 用户数据
- `name` - 线程名称（可选）
- `thread` - 线程句柄
- `exit_code` - 输出参数，接收线程退出码

**返回值：**
- `thread_create`: 线程句柄，NULL表示失败
- `thread_join/detach`: `true`操作成功，`false`操作失败

#### 线程本地存储
```c
void* (*tls_allocate)(void);
bool (*tls_free)(void* key);
bool (*tls_set_value)(void* key, void* value);
void* (*tls_get_value)(void* key);
```
线程本地存储操作。

**参数：**
- `key` - TLS键
- `value` - 要设置的值

**返回值：**
- `tls_allocate`: TLS键，NULL表示失败
- `tls_free/set_value`: `true`操作成功，`false`操作失败
- `tls_get_value`: 存储的值，NULL表示未设置

#### 互斥锁操作
```c
CN_MutexHandle_t (*mutex_create)(void);
bool (*mutex_destroy)(CN_MutexHandle_t mutex);
bool (*mutex_lock)(CN_MutexHandle_t mutex);
bool (*mutex_try_lock)(CN_MutexHandle_t mutex);
bool (*mutex_unlock)(CN_MutexHandle_t mutex);
```
互斥锁创建、销毁、加锁和解锁。

**参数：**
- `mutex` - 互斥锁句柄

**返回值：**
- `mutex_create`: 互斥锁句柄，NULL表示失败
- 其他函数: `true`操作成功，`false`操作失败

#### 递归互斥锁
```c
CN_MutexHandle_t (*recursive_mutex_create)(void);
bool (*recursive_mutex_destroy)(CN_MutexHandle_t mutex);
bool (*recursive_mutex_lock)(CN_MutexHandle_t mutex);
bool (*recursive_mutex_try_lock)(CN_MutexHandle_t mutex);
bool (*recursive_mutex_unlock)(CN_MutexHandle_t mutex);
```
递归互斥锁操作（同一线程可多次加锁）。

#### 信号量操作
```c
CN_SemaphoreHandle_t (*semaphore_create)(int initial_count, int max_count);
bool (*semaphore_destroy)(CN_SemaphoreHandle_t semaphore);
bool (*semaphore_wait)(CN_SemaphoreHandle_t semaphore, uint32_t timeout_ms);
bool (*semaphore_post)(CN_SemaphoreHandle_t semaphore);
```
信号量创建、销毁、等待和释放。

**参数：**
- `initial_count` - 初始计数
- `max_count` - 最大计数
- `semaphore` - 信号量句柄
- `timeout_ms` - 超时时间（毫秒）

**返回值：**
- `semaphore_create`: 信号量句柄，NULL表示失败
- 其他函数: `true`操作成功，`false`操作失败

#### 条件变量操作
```c
CN_ConditionHandle_t (*condition_create)(void);
bool (*condition_destroy)(CN_ConditionHandle_t condition);
bool (*condition_wait)(CN_ConditionHandle_t condition, CN_MutexHandle_t mutex);
bool (*condition_timed_wait)(CN_ConditionHandle_t condition, 
                             CN_MutexHandle_t mutex, uint32_t timeout_ms);
bool (*condition_signal)(CN_ConditionHandle_t condition);
bool (*condition_broadcast)(CN_ConditionHandle_t condition);
```
条件变量创建、销毁、等待和通知。

**参数：**
- `condition` - 条件变量句柄
- `mutex` - 关联的互斥锁
- `timeout_ms` - 超时时间（毫秒）

**返回值：**
- `condition_create`: 条件变量句柄，NULL表示失败
- 其他函数: `true`操作成功，`false`操作失败

#### 原子操作
```c
int32_t (*atomic_increment)(volatile int32_t* value);
int32_t (*atomic_decrement)(volatile int32_t* value);
int32_t (*atomic_add)(volatile int32_t* value, int32_t addend);
int32_t (*atomic_compare_exchange)(volatile int32_t* dest, 
                                   int32_t exchange, int32_t comparand);
void (*memory_barrier)(void);
```
原子操作和内存屏障。

**参数：**
- `value`, `dest` - 原子变量指针
- `addend` - 要加的值
- `exchange` - 要交换的值
- `comparand` - 比较的值

**返回值：**
- 原子操作后的值（`atomic_compare_exchange`返回原始值）

### 网络接口 (`Stru_CN_NetworkInterface_t`)

#### 网络初始化
```c
bool (*network_initialize)(void);
void (*network_cleanup)(void);
```
网络子系统初始化和清理。

**返回值：**
- `network_initialize`: `true`初始化成功，`false`初始化失败

#### 地址解析
```c
bool (*address_from_string)(const char* str, Stru_CN_NetworkAddress_t* addr);
char* (*address_to_string)(const Stru_CN_NetworkAddress_t* addr);
```
网络地址字符串和结构体之间的转换。

**参数：**
- `str` - 地址字符串
- `addr` - 网络地址结构体

**返回值：**
- `address_from_string`: `true`转换成功，`false`转换失败
- `address_to_string`: 新分配的字符串，需要调用者释放

#### 套接字操作
```c
CN_SocketHandle_t (*socket_create)(Eum_CN_NetworkProtocol_t protocol);
bool (*socket_close)(CN_SocketHandle_t socket);
bool (*socket_bind)(CN_SocketHandle_t socket, const Stru_CN_NetworkAddress_t* addr);
bool (*socket_listen)(CN_SocketHandle_t socket, int backlog);
CN_SocketHandle_t (*socket_accept)(CN_SocketHandle_t server_socket, 
                                   Stru_CN_NetworkAddress_t* client_addr);
bool (*socket_connect)(CN_SocketHandle_t socket, const Stru_CN_NetworkAddress_t* addr);
```
套接字创建、关闭、绑定、监听、接受连接和连接。

**参数：**
- `protocol` - 网络协议
- `socket` - 套接字句柄
- `addr` - 网络地址
- `backlog` - 等待连接队列的最大长度
- `server_socket` - 服务器套接字
- `client_addr` - 输出参数，接收客户端地址

**返回值：**
- `socket_create`: 套接字句柄，NULL表示失败
- `socket_accept`: 客户端套接字句柄，NULL表示失败
- 其他函数: `true`操作成功，`false`操作失败

#### 数据发送和接收
```c
int32_t (*socket_send)(CN_SocketHandle_t socket, const void* data, size_t size);
int32_t (*socket_receive)(CN_SocketHandle_t socket, void* buffer, size_t size);
int32_t (*socket_send_to)(CN_SocketHandle_t socket, const void* data, size_t size,
                          const Stru_CN_NetworkAddress_t* addr);
int32_t (*socket_receive_from)(CN_SocketHandle_t socket, void* buffer, size_t size,
                               Stru_CN_NetworkAddress_t* addr);
```
TCP数据发送接收和UDP数据发送接收。

**参数：**
- `socket` - 套接字句柄
- `data` - 要发送的数据
- `buffer` - 接收数据的缓冲区
- `size` - 数据大小
- `addr` - 目标地址（UDP）或输出参数（接收源地址）

**返回值：**
- 实际发送/接收的字节数，负数表示错误

#### 套接字选项
```c
bool (*socket_set_option)(CN_SocketHandle_t socket, int option, const void* value, size_t size);
bool (*socket_get_option)(CN_SocketHandle_t socket, int option, void* value, size_t* size);
```
设置和获取套接字选项。

**参数：**
- `socket` - 套接字句柄
- `option` - 选项标识符
- `value` - 选项值
- `size` - 选项值大小

**返回值：**
- `true`操作成功，`false`操作失败

#### 非阻塞IO
```c
bool (*socket_set_nonblocking)(CN_SocketHandle_t socket, bool nonblocking);
bool (*socket_is_nonblocking)(CN_SocketHandle_t socket);
```
设置和检查套接字非阻塞模式。

**参数：**
- `socket` - 套接字句柄
- `nonblocking` - 是否设置为非阻塞模式

**返回值：**
- `true`操作成功，`false`操作失败
- `socket_is_nonblocking`: `true`是非阻塞模式，`false`是阻塞模式

#### 选择器/轮询
```c
void* (*selector_create)(void);
bool (*selector_destroy)(void* selector);
bool (*selector_add)(void* selector, CN_SocketHandle_t socket, void* user_data);
bool (*selector_remove)(void* selector, CN_SocketHandle_t socket);
int (*selector_wait)(void* selector, uint32_t timeout_ms);
bool (*selector_is_readable)(void* selector, CN_SocketHandle_t socket);
bool (*selector_is_writable)(void* selector, CN_SocketHandle_t socket);
```
IO多路复用选择器操作。

**参数：**
- `selector` - 选择器句柄
- `socket` - 套接字句柄
- `user_data` - 用户数据
- `timeout_ms` - 超时时间（毫秒）

**返回值：**
- `selector_create`: 选择器句柄，NULL表示失败
- `selector_wait`: 就绪的套接字数量，负数表示错误
- 其他函数: `true`操作成功，`false`操作失败

#### 网络信息
```c
char* (*get_host_name)(void);
bool (*get_host_addresses)(Stru_CN_NetworkAddress_t* addresses, size_t* count);
```
获取主机名和网络地址。

**参数：**
- `addresses` - 输出参数，接收地址数组
- `count` - 输入输出参数，输入数组大小，输出实际数量

**返回值：**
- `get_host_name`: 新分配的字符串，需要调用者释放
- `get_host_addresses`: `true`获取成功，`false`获取失败

### 时间和日期接口 (`Stru_CN_TimeInterface_t`)

#### 获取当前时间
```c
uint64_t (*get_current_time)(void);
uint64_t (*get_current_time_nanos)(void);
```
获取当前时间戳（毫秒和纳秒精度）。

**返回值：**
- 当前时间戳

#### 时间转换
```c
bool (*time_to_local)(uint64_t timestamp, Stru_CN_Time_t* local_time);
bool (*time_to_utc)(uint64_t timestamp, Stru_CN_Time_t* utc_time);
uint64_t (*time_from_local)(const Stru_CN_Time_t* local_time);
uint64_t (*time_from_utc)(const Stru_CN_Time_t* utc_time);
```
时间戳和本地时间/UTC时间之间的转换。

**参数：**
- `timestamp` - 时间戳
- `local_time` - 输出参数，接收本地时间
- `utc_time` - 输出参数，接收UTC时间

**返回值：**
- 转换函数: `true`转换成功，`false`转换失败
- 生成函数: 时间戳，0表示错误

#### 时间格式化
```c
char* (*format_time)(uint64_t timestamp, const char* format);
bool (*parse_time)(const char* str, const char* format, uint64_t* timestamp);
```
时间格式化和解析。

**参数：**
- `timestamp` - 时间戳
- `format` - 格式字符串
- `str` - 时间字符串

**返回值：**
- `format_time`: 新分配的字符串，需要调用者释放
- `parse_time`: `true`解析成功，`false`解析失败

#### 高精度计时
```c
uint64_t (*get_performance_counter)(void);
uint64_t (*get_performance_frequency)(void);
```
获取高精度性能计数器和频率。

**返回值：**
- 计数器值或频率

#### 时间操作
```c
void (*sleep)(uint32_t milliseconds);
void (*sleep_nanos)(uint64_t nanoseconds);
```
睡眠指定时间。

**参数：**
- `milliseconds` - 毫秒数
- `nanoseconds` - 纳秒数

#### 时区信息
```c
int32_t (*get_timezone_offset)(void);
bool (*is_daylight_saving_time)(void);
```
获取时区偏移和夏令时信息。

**返回值：**
- `get_timezone_offset`: 时区偏移（分钟）
- `is_daylight_saving_time`: `true`是夏令时，`false`不是

#### 日历操作
```c
uint8_t (*get_day_of_week)(uint16_t year, uint8_t month, uint8_t day);
uint16_t (*get_day_of_year)(uint16_t year, uint8_t month, uint8_t day);
bool (*is_leap_year)(uint16_t year);
```
日历相关计算。

**参数：**
- `year` - 年份
- `month` - 月份
- `day` - 日

**返回值：**
- `get_day_of_week`: 星期几（0=周日，1=周一，...，6=周六）
- `get_day_of_year`: 一年中的第几天
- `is_leap_year`: `true`是闰年，`false`不是

### 系统信息接口 (`Stru_CN_SystemInterface_t`)

#### 系统信息获取
```c
bool (*get_system_info)(Stru_CN_SystemInfo_t* info);
```
获取完整的系统信息。

**参数：**
- `info` - 输出参数，接收系统信息

**返回值：**
- `true`获取成功，`false`获取失败

#### 内存信息
```c
uint64_t (*get_total_memory)(void);
uint64_t (*get_available_memory)(void);
uint64_t (*get_process_memory_usage)(void);
```
获取内存使用信息。

**返回值：**
- 内存大小（字节）

#### CPU信息
```c
uint32_t (*get_cpu_count)(void);
double (*get_cpu_usage)(void);
```
获取CPU核心数和CPU使用率。

**返回值：**
- `get_cpu_count`: CPU核心数
- `get_cpu_usage`: CPU使用率（0.0-1.0）

#### 环境变量
```c
char* (*get_environment_variable)(const char* name);
bool (*set_environment_variable)(const char* name, const char* value);
```
获取和设置环境变量。

**参数：**
- `name` - 环境变量名
- `value` - 环境变量值

**返回值：**
- `get_environment_variable`: 新分配的字符串，需要调用者释放
- `set_environment_variable`: `true`设置成功，`false`设置失败

#### 进程信息
```c
uint32_t (*get_process_id)(void);
uint32_t (*get_parent_process_id)(void);
char* (*get_process_name)(void);
```
获取进程相关信息。

**返回值：**
- `get_process_id`: 当前进程ID
- `get_parent_process_id`: 父进程ID
- `get_process_name`: 新分配的进程名字符串，需要调用者释放

#### 命令行参数
```c
char* (*get_command_line)(void);
int (*get_argument_count)(void);
char* (*get_argument)(int index);
```
获取命令行参数。

**参数：**
- `index` - 参数索引

**返回值：**
- `get_command_line`: 新分配的命令行字符串，需要调用者释放
- `get_argument_count`: 参数数量
- `get_argument`: 新分配的参数字符串，需要调用者释放

#### 系统路径
```c
char* (*get_system_directory)(void);
char* (*get_program_files_directory)(void);
char* (*get_user_name)(void);
char* (*get_user_home_directory)(void);
```
获取系统特殊路径。

**返回值：**
- 新分配的路径字符串，需要调用者释放

#### 系统通知
```c
bool (*show_message_box)(const char* title, const char* message, uint32_t flags);
bool (*play_sound)(const char* sound_name);
```
显示消息框和播放声音。

**参数：**
- `title` - 消息框标题
- `message` - 消息内容
- `flags` - 消息框标志
- `sound_name` - 声音名称

**返回值：**
- `true`操作成功，`false`操作失败

#### 电源管理
```c
bool (*get_power_status)(uint32_t* battery_percent, bool* is_charging);
bool (*set_power_saving_mode)(bool enable);
```
获取电源状态和设置省电模式。

**参数：**
- `battery_percent` - 输出参数，接收电池百分比
- `is_charging` - 输出参数，接收是否在充电
- `enable` - 是否启用省电模式

**返回值：**
- `true`操作成功，`false`操作失败

#### 系统控制
```c
bool (*shutdown_system)(uint32_t timeout_seconds);
bool (*reboot_system)(uint32_t timeout_seconds);
bool (*logoff_user)(void);
```
系统关机、重启和用户注销。

**参数：**
- `timeout_seconds` - 超时时间（秒）

**返回值：**
- `true`操作成功，`false`操作失败

### 统一平台接口 (`Stru_CN_PlatformInterface_t`)

```c
typedef struct Stru_CN_PlatformInterface_t
{
    Stru_CN_FileSystemInterface_t* filesystem;
    Stru_CN_ThreadInterface_t* thread;
    Stru_CN_NetworkInterface_t* network;
    Stru_CN_TimeInterface_t* time;
    Stru_CN_SystemInterface_t* system;
} Stru_CN_PlatformInterface_t;
```
统一平台接口，包含所有平台功能模块的指针。

## 平台管理函数

### 获取默认平台接口
```c
Stru_CN_PlatformInterface_t* CN_platform_get_default(void);
```
根据当前运行平台返回相应的平台接口实现。

**返回值：**
- 平台接口指针，失败返回NULL

### 创建自定义平台接口
```c
Stru_CN_PlatformInterface_t* CN_platform_create_custom(
    Stru_CN_FileSystemInterface_t* filesystem,
    Stru_CN_ThreadInterface_t* thread,
    Stru_CN_NetworkInterface_t* network,
    Stru_CN_TimeInterface_t* time,
    Stru_CN_SystemInterface_t* system);
```
允许用户提供自定义的平台接口实现。

**参数：**
- `filesystem` - 文件系统接口（可为NULL）
- `thread` - 线程接口（可为NULL）
- `network` - 网络接口（可为NULL）
- `time` - 时间接口（可为NULL）
- `system` - 系统接口（可为NULL）

**返回值：**
- 新创建的平台接口，失败返回NULL

### 销毁平台接口
```c
void CN_platform_destroy(Stru_CN_PlatformInterface_t* platform);
```
销毁平台接口。

**参数：**
- `platform` - 要销毁的平台接口

### 检查平台接口完整性
```c
bool CN_platform_is_complete(const Stru_CN_PlatformInterface_t* platform);
```
检查平台接口的所有必需组件是否都已设置。

**参数：**
- `platform` - 要检查的平台接口

**返回值：**
- 如果接口完整返回true，否则返回false

## 平台检测函数

### 获取平台类型
```c
const char* CN_platform_get_type(void);
```
获取当前平台类型。

**返回值：**
- 平台类型字符串（"windows", "linux", "macos", "unknown"）

### 平台检查函数
```c
bool CN_platform_is_windows(void);
bool CN_platform_is_linux(void);
bool CN_platform_is_macos(void);
bool CN_platform_is_unix(void);
```
检查当前运行平台。

**返回值：**
- `true`是对应平台，`false`不是

## 平台初始化函数

### 初始化平台抽象层
```c
bool CN_platform_initialize(void);
```
必须在调用任何平台函数之前调用此函数。

**返回值：**
- 初始化成功返回true，失败返回false

### 清理平台抽象层
```c
void CN_platform_cleanup(void);
```
在程序退出前调用此函数释放平台资源。

## 使用示例

### 基本使用
```c
#include "CN_platform.h"

int main()
{
    // 初始化平台抽象层
    if (!CN_platform_initialize()) {
        return -1;
    }
    
    // 获取默认平台接口
    Stru_CN_PlatformInterface_t* platform = CN_platform_get_default();
    if (!platform) {
        CN_platform_cleanup();
        return -1;
    }
    
    // 使用文件系统接口
    if (platform->filesystem->file_exists("/path/to/file")) {
        CN_FileHandle_t file = platform->filesystem->file_open(
            "/path/to/file", Eum_FILE_MODE_READ);
        if (file) {
            char buffer[1024];
            size_t bytes_read = platform->filesystem->file_read(
                file, buffer, sizeof(buffer));
            platform->filesystem->file_close(file);
        }
    }
    
    // 使用线程接口
    CN_ThreadHandle_t thread = platform->thread->thread_create(
        my_thread_func, NULL, "my_thread");
    if (thread) {
        platform->thread->thread_join(thread, NULL);
    }
    
    // 清理平台抽象层
    CN_platform_destroy(platform);
    CN_platform_cleanup();
    return 0;
}
```

### 错误处理示例
```c
bool read_file_content(const char* filename, char** content, size_t* size)
{
    Stru_CN_PlatformInterface_t* platform = CN_platform_get_default();
    if (!platform) {
        return false;
    }
    
    CN_FileHandle_t file = platform->filesystem->file_open(
        filename, Eum_FILE_MODE_READ);
    if (!file) {
        return false;
    }
    
    // 获取文件大小
    Stru_CN_FileInfo_t info;
    if (!platform->filesystem->file_get_info(filename, &info)) {
        platform->filesystem->file_close(file);
        return false;
    }
    
    // 分配内存并读取文件
    *content = (char*)malloc(info.size + 1);
    if (!*content) {
        platform->filesystem->file_close(file);
        return false;
    }
    
    size_t bytes_read = platform->filesystem->file_read(file, *content, info.size);
    platform->filesystem->file_close(file);
    
    if (bytes_read != info.size) {
        free(*content);
        *content = NULL;
        return false;
    }
    
    (*content)[info.size] = '\0';
    *size = info.size;
    return true;
}
```

## 注意事项

### 内存管理
- 所有返回字符串的函数都需要调用者释放内存
- 平台接口指针由`CN_platform_get_default()`或`CN_platform_create_custom()`创建，需要调用`CN_platform_destroy()`释放
- 文件句柄、线程句柄等资源需要调用相应的关闭/销毁函数

### 线程安全
- 平台接口本身是线程安全的，可以在多线程环境中使用
- 但具体的资源（如文件、套接字）可能不是线程安全的，需要用户自行同步

### 错误处理
- 所有函数都提供明确的返回值指示成功或失败
- 某些函数可能设置平台特定的错误码，可以通过`errno`或类似机制获取
- 建议在关键操作后检查返回值并进行适当的错误处理

### 平台差异
- 某些功能可能在某些平台上不可用，函数会返回`false`或`NULL`
- 使用前应检查平台支持情况，或提供回退方案
- 平台特定行为在API文档中会特别注明

## 版本历史

### v1.0.0 (2026-01-03)
- 初始版本
- 支持Windows、Linux、macOS基础平台功能
- 提供完整的文件系统、线程、网络、时间、系统信息接口

## 相关文档

- [平台模块README](../src/infrastructure/platform/README.md)
- [架构设计原则](../../../architecture/架构设计原则.md)
- [编码标准](../../../specifications/CN_Language项目 技术规范和编码标准.md)

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件
