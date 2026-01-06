# 平台抽象层 API 文档

## 概述

本文档详细描述了CN_Language项目平台抽象层的API接口。平台抽象层提供跨平台的操作系统服务抽象，包括文件系统、进程管理和时间管理等功能。

## 文件结构

```
src/infrastructure/platform/
├── CN_platform_interface.h      # 接口定义头文件
├── CN_platform_windows.c        # Windows平台实现
├── CN_platform_linux.c          # Linux平台实现（待实现）
├── CN_platform_macos.c          # macOS平台实现（待实现）
├── CN_platform_stub.c           # 测试桩实现
└── README.md                    # 模块文档
```

## 常量定义

### 路径相关常量
```c
#define CN_MAX_PATH_LENGTH 4096          // 最大路径长度
#define CN_MAX_FILENAME_LENGTH 256       // 最大文件名长度
```

### 文件句柄常量
```c
#define CN_INVALID_FILE_HANDLE ((void*)0)        // 无效文件句柄
#define CN_INVALID_PROCESS_HANDLE ((void*)0)     // 无效进程句柄
```

## 枚举类型

### 文件打开模式 (`Eum_FileOpenMode`)
```c
typedef enum Eum_FileOpenMode
{
    Eum_FILE_MODE_READ,          // 只读模式
    Eum_FILE_MODE_WRITE,         // 只写模式（创建新文件）
    Eum_FILE_MODE_APPEND,        // 追加模式
    Eum_FILE_MODE_READ_WRITE,    // 读写模式
    Eum_FILE_MODE_CREATE         // 创建新文件（如果存在则截断）
} Eum_FileOpenMode;
```

### 文件属性 (`Eum_FileAttribute`)
```c
typedef enum Eum_FileAttribute
{
    Eum_FILE_ATTR_READONLY = 0x01,      // 只读文件
    Eum_FILE_ATTR_HIDDEN = 0x02,        // 隐藏文件
    Eum_FILE_ATTR_SYSTEM = 0x04,        // 系统文件
    Eum_FILE_ATTR_DIRECTORY = 0x10,     // 目录
    Eum_FILE_ATTR_ARCHIVE = 0x20,       // 存档文件
    Eum_FILE_ATTR_NORMAL = 0x80         // 普通文件
} Eum_FileAttribute;
```

## 数据结构

### 文件信息结构体 (`Stru_FileInfo_t`)
```c
typedef struct Stru_FileInfo_t
{
    char filename[CN_MAX_FILENAME_LENGTH];  // 文件名
    uint64_t size;                          // 文件大小（字节）
    uint64_t creation_time;                 // 创建时间（Unix时间戳，毫秒）
    uint64_t modification_time;             // 修改时间（Unix时间戳，毫秒）
    uint64_t access_time;                   // 访问时间（Unix时间戳，毫秒）
    uint32_t attributes;                    // 文件属性（Eum_FileAttribute的位组合）
} Stru_FileInfo_t;
```

### 目录条目结构体 (`Stru_DirectoryEntry_t`)
```c
typedef struct Stru_DirectoryEntry_t
{
    char name[CN_MAX_FILENAME_LENGTH];      // 条目名称
    bool is_directory;                      // 是否为目录
    uint64_t size;                          // 文件大小（如果是文件）
    uint64_t modification_time;             // 修改时间（Unix时间戳，毫秒）
} Stru_DirectoryEntry_t;
```

### 进程信息结构体 (`Stru_ProcessInfo_t`)
```c
typedef struct Stru_ProcessInfo_t
{
    uint32_t process_id;                    // 进程ID
    uint32_t parent_process_id;             // 父进程ID
    uint64_t creation_time;                 // 创建时间（Unix时间戳，毫秒）
    uint64_t cpu_time;                      // CPU时间（毫秒）
    uint64_t memory_usage;                  // 内存使用量（字节）
} Stru_ProcessInfo_t;
```

### 时间信息结构体 (`Stru_TimeInfo_t`)
```c
typedef struct Stru_TimeInfo_t
{
    uint16_t year;                          // 年份
    uint8_t month;                          // 月份（1-12）
    uint8_t day;                            // 日（1-31）
    uint8_t hour;                           // 小时（0-23）
    uint8_t minute;                         // 分钟（0-59）
    uint8_t second;                         // 秒（0-59）
    uint16_t millisecond;                   // 毫秒（0-999）
    int8_t timezone_offset;                 // 时区偏移（小时）
} Stru_TimeInfo_t;
```

## 接口定义

### 文件系统接口 (`Stru_FileSystemInterface_t`)

#### `open_file`
```c
void* (*open_file)(const char* path, Eum_FileOpenMode mode);
```
打开指定路径的文件。

**参数：**
- `path`: 文件路径
- `mode`: 打开模式

**返回值：**
- 成功：文件句柄
- 失败：`CN_INVALID_FILE_HANDLE`

#### `close_file`
```c
bool (*close_file)(void* file_handle);
```
关闭文件句柄。

**参数：**
- `file_handle`: 文件句柄

**返回值：**
- 成功：`true`
- 失败：`false`

#### `read_file`
```c
int64_t (*read_file)(void* file_handle, void* buffer, size_t size);
```
从文件读取数据。

**参数：**
- `file_handle`: 文件句柄
- `buffer`: 数据缓冲区
- `size`: 要读取的字节数

**返回值：**
- 成功：实际读取的字节数
- 失败：`-1`

#### `write_file`
```c
int64_t (*write_file)(void* file_handle, const void* buffer, size_t size);
```
向文件写入数据。

**参数：**
- `file_handle`: 文件句柄
- `buffer`: 数据缓冲区
- `size`: 要写入的字节数

**返回值：**
- 成功：实际写入的字节数
- 失败：`-1`

#### `seek_file`
```c
int64_t (*seek_file)(void* file_handle, int64_t offset, int origin);
```
移动文件指针。

**参数：**
- `file_handle`: 文件句柄
- `offset`: 偏移量
- `origin`: 起始位置（0=文件开始，1=当前位置，2=文件末尾）

**返回值：**
- 成功：新的文件位置
- 失败：`-1`

#### `get_file_size`
```c
int64_t (*get_file_size)(void* file_handle);
```
获取文件大小。

**参数：**
- `file_handle`: 文件句柄

**返回值：**
- 成功：文件大小（字节）
- 失败：`-1`

#### `get_file_info`
```c
bool (*get_file_info)(const char* path, Stru_FileInfo_t* info);
```
获取文件信息。

**参数：**
- `path`: 文件路径
- `info`: 文件信息结构体指针

**返回值：**
- 成功：`true`
- 失败：`false`

#### `delete_file`
```c
bool (*delete_file)(const char* path);
```
删除文件。

**参数：**
- `path`: 文件路径

**返回值：**
- 成功：`true`
- 失败：`false`

#### `rename_file`
```c
bool (*rename_file)(const char* old_path, const char* new_path);
```
重命名文件。

**参数：**
- `old_path`: 原路径
- `new_path`: 新路径

**返回值：**
- 成功：`true`
- 失败：`false`

#### `copy_file`
```c
bool (*copy_file)(const char* source_path, const char* dest_path);
```
复制文件。

**参数：**
- `source_path`: 源文件路径
- `dest_path`: 目标文件路径

**返回值：**
- 成功：`true`
- 失败：`false`

#### `create_directory`
```c
bool (*create_directory)(const char* path);
```
创建目录。

**参数：**
- `path`: 目录路径

**返回值：**
- 成功：`true`
- 失败：`false`

#### `delete_directory`
```c
bool (*delete_directory)(const char* path);
```
删除目录。

**参数：**
- `path`: 目录路径

**返回值：**
- 成功：`true`
- 失败：`false`

#### `open_directory`
```c
void* (*open_directory)(const char* path);
```
打开目录。

**参数：**
- `path`: 目录路径

**返回值：**
- 成功：目录句柄
- 失败：`NULL`

#### `read_directory`
```c
bool (*read_directory)(void* dir_handle, Stru_DirectoryEntry_t* entry);
```
读取目录条目。

**参数：**
- `dir_handle`: 目录句柄
- `entry`: 目录条目结构体指针

**返回值：**
- 成功：`true`
- 没有更多条目：`false`

#### `close_directory`
```c
bool (*close_directory)(void* dir_handle);
```
关闭目录。

**参数：**
- `dir_handle`: 目录句柄

**返回值：**
- 成功：`true`
- 失败：`false`

#### `path_exists`
```c
bool (*path_exists)(const char* path);
```
检查路径是否存在。

**参数：**
- `path`: 路径

**返回值：**
- 存在：`true`
- 不存在：`false`

#### `is_directory`
```c
bool (*is_directory)(const char* path);
```
检查路径是否为目录。

**参数：**
- `path`: 路径

**返回值：**
- 是目录：`true`
- 不是目录：`false`

#### `get_current_directory`
```c
bool (*get_current_directory)(char* buffer, size_t size);
```
获取当前工作目录。

**参数：**
- `buffer`: 缓冲区
- `size`: 缓冲区大小

**返回值：**
- 成功：`true`
- 失败：`false`

#### `set_current_directory`
```c
bool (*set_current_directory)(const char* path);
```
设置当前工作目录。

**参数：**
- `path`: 目录路径

**返回值：**
- 成功：`true`
- 失败：`false`

#### `get_temp_directory`
```c
bool (*get_temp_directory)(char* buffer, size_t size);
```
获取临时目录路径。

**参数：**
- `buffer`: 缓冲区
- `size`: 缓冲区大小

**返回值：**
- 成功：`true`
- 失败：`false`

### 进程管理接口 (`Stru_ProcessInterface_t`)

#### `create_process`
```c
void* (*create_process)(const char* command, const char* const* arguments, const char* working_directory);
```
创建进程。

**参数：**
- `command`: 命令字符串
- `arguments`: 参数数组（以NULL结尾）
- `working_directory`: 工作目录（可为NULL）

**返回值：**
- 成功：进程句柄
- 失败：`CN_INVALID_PROCESS_HANDLE`

#### `wait_process`
```c
bool (*wait_process)(void* process_handle, uint32_t timeout_ms, CN_ProcessExitStatus* exit_status);
```
等待进程结束。

**参数：**
- `process_handle`: 进程句柄
- `timeout_ms`: 超时时间（毫秒，0表示无限等待）
- `exit_status`: 退出状态指针（可为NULL）

**返回值：**
- 成功：`true`
- 超时或失败：`false`

#### `terminate_process`
```c
bool (*terminate_process)(void* process_handle);
```
终止进程。

**参数：**
- `process_handle`: 进程句柄

**返回值：**
- 成功：`true`
- 失败：`false`

#### `get_process_info`
```c
bool (*get_process_info)(void* process_handle, Stru_ProcessInfo_t* info);
```
获取进程信息。

**参数：**
- `process_handle`: 进程句柄
- `info`: 进程信息结构体指针

**返回值：**
- 成功：`true`
- 失败：`false`

#### `get_current_process_id`
```c
uint32_t (*get_current_process_id)(void);
```
获取当前进程ID。

**返回值：**
- 当前进程ID

#### `get_current_thread_id`
```c
uint32_t (*get_current_thread_id)(void);
```
获取当前线程ID。

**返回值：**
- 当前线程ID

#### `sleep`
```c
void (*sleep)(uint32_t milliseconds);
```
睡眠指定毫秒数。

**参数：**
- `milliseconds`: 毫秒数

#### `get_environment_variable`
```c
bool (*get_environment_variable)(const char* name, char* buffer, size_t size);
```
获取环境变量。

**参数：**
- `name`: 变量名
- `buffer`: 缓冲区
- `size`: 缓冲区大小

**返回值：**
- 成功：`true`
- 失败：`false`

#### `set_environment_variable`
```c
bool (*set_environment_variable)(const char* name, const char* value);
```
设置环境变量。

**参数：**
- `name`: 变量名
- `value`: 变量值

**返回值：**
- 成功：`true`
- 失败：`false`

#### `execute_command`
```c
CN_ProcessExitStatus (*execute_command)(const char* command, char* output, size_t output_size);
```
执行系统命令。

**参数：**
- `command`: 命令字符串
- `output`: 输出缓冲区（可为NULL）
- `output_size`: 输出缓冲区大小

**返回值：**
- 命令退出状态

### 时间管理接口 (`Stru_TimeInterface_t`)

#### `get_current_timestamp`
```c
uint64_t (*get_current_timestamp)(void);
```
获取当前时间（Unix时间戳，毫秒）。

**返回值：**
- Unix时间戳（毫秒）

#### `get_high_resolution_time`
```c
uint64_t (*get_high_resolution_time)(void);
```
获取当前时间（高精度，纳秒）。

**返回值：**
- 高精度时间（纳秒）

#### `get_local_time`
```c
bool (*get_local_time)(uint64_t timestamp, Stru_TimeInfo_t* time_info);
```
获取本地时间。

**参数：**
- `timestamp`: Unix时间戳（毫秒）
- `time_info`: 时间信息结构体指针

**返回值：**
- 成功：`true`
- 失败：`false`

#### `get_utc_time`
```c
bool (*get_utc_time)(uint64_t timestamp, Stru_TimeInfo_t* time_info);
```
获取UTC时间。

**参数：**
- `timestamp`: Unix时间戳（毫秒）
- `time_info`: 时间信息结构体指针

**返回值：**
- 成功：`true`
- 失败：`false`

#### `format_time`
```c
bool (*format_time)(const Stru_TimeInfo_t* time_info, const char* format, char* buffer, size_t size);
```
格式化时间字符串。

**参数：**
- `time_info`: 时间信息结构体指针
- `format`: 格式字符串（如"%Y-%m-%d %H:%M:%S"）
- `buffer`: 输出缓冲区
- `size`: 缓冲区大小

**返回值：**
- 成功：`true`
- 失败：`false`

#### `parse_time`
```c
bool (*parse_time)(const char* time_string, const char* format, Stru_TimeInfo_t* time_info);
```
解析时间字符串。

**参数：**
- `time_string`: 时间字符串
- `format`: 格式字符串
- `time_info`: 时间信息结构体指针

**返回值：**
- 成功：`true`
- 失败：`false`

#### `get_system_uptime`
```c
uint64_t (*get_system_uptime)(void);
```
获取系统启动时间。

**返回值：**
- 系统启动时间（毫秒）

#### `get_cpu_frequency`
```c
uint64_t (*get_cpu_frequency)(void);
```
获取CPU时钟频率。

**返回值：**
- CPU时钟频率（Hz）

#### `get_timezone_info`
```c
bool (*get_timezone_info)(char* timezone_name, size_t size, int8_t* offset);
```
获取时区信息。

**参数：**
- `timezone_name`: 时区名称缓冲区
- `size`: 缓冲区大小
- `offset`: 时区偏移（小时）

**返回值：**
- 成功：`true`
- 失败：`false`

### 平台主接口 (`Stru_PlatformInterface_t`)

#### 成员变量
```c
const Stru_FileSystemInterface_t* file_system;    // 文件系统接口
const Stru_ProcessInterface_t* process;           // 进程管理接口
const Stru_TimeInterface_t* time;                 // 时间管理接口
```

#### `initialize`
```c
bool (*initialize)(void);
```
初始化平台抽象层。

**返回值：**
- 成功：`true`
- 失败：`false`

#### `cleanup`
```c
void (*cleanup)(void);
```
清理平台抽象层资源。

#### `get_platform_name`
```c
const char* (*get_platform_name)(void);
```
获取平台名称。

**返回值：**
- 平台名称字符串

#### `get_platform_version`
```c
const char* (*get_platform_version)(void);
```
获取平台版本。

**返回值：**
- 平台版本字符串

#### `get_cpu_architecture`
```c
const char* (*get_cpu_architecture)(void);
```
获取CPU架构信息。

**返回值：**
- CPU架构字符串

#### `get_system_memory_info`
```c
bool (*get_system_memory_info)(uint64_t* total_memory, uint64_t* available_memory);
```
获取系统内存信息。

**参数：**
- `total_memory`: 总内存大小（字节）
- `available_memory`: 可用内存大小（字节）

**返回值：**
- 成功：`true`
- 失败：`false`

#### `get_cpu_core_count`
```c
uint32_t (*get_cpu_core_count)(void);
```
获取CPU核心数。

**返回值：**
- CPU核心数

#### `get_system_info`
```c
bool (*get_system_info)(char* info_buffer, size_t buffer_size);
```
获取系统信息。

**参数：**
- `info_buffer`: 信息缓冲区
- `buffer_size`: 缓冲区大小

**返回值：**
- 成功：`true`
- 失败：`false`

## 工厂函数

### `F_get_default_platform_interface`
```c
const Stru_PlatformInterface_t* F_get_default_platform_interface(void);
```
获取默认平台接口（自动检测当前平台）。

**返回值：**
- 成功：平台接口指针
- 失败：`NULL`

### `F_get_windows_platform_interface`
```c
const Stru_PlatformInterface_t* F_get_windows_platform_interface(void);
```
获取Windows平台接口。

**返回值：**
- 成功：Windows平台接口指针
- 失败：`NULL`

### `F_get_linux_platform_interface`
```c
const Stru_PlatformInterface_t* F_get_linux_platform_interface(void);
```
获取Linux平台接口。

**返回值：**
- 成功：Linux平台接口指针
- 失败：`NULL`

### `F_get_macos_platform_interface`
```c
const Stru_PlatformInterface_t* F_get_macos_platform_interface(void);
```
获取macOS平台接口。

**返回值：**
- 成功：macOS平台接口指针
- 失败：`NULL`

### `F_get_stub_platform_interface`
```c
const Stru_PlatformInterface_t* F_get_stub_platform_interface(void);
```
获取测试桩平台接口（用于单元测试）。

**返回值：**
- 测试桩平台接口指针

## 使用示例

### 基本使用
```c
#include "CN_platform_interface.h"
#include <stdio.h>

int main()
{
    // 获取默认平台接口
    const Stru_PlatformInterface_t* platform = F_get_default_platform_interface();
    if (!platform)
    {
        printf("无法获取平台接口\n");
        return -1;
    }
    
    // 初始化平台
    if (!platform->initialize())
    {
        printf("平台初始化失败\n");
        return -1;
    }
    
    // 使用文件系统接口
    void* file = platform->file_system->open_file("test.txt", Eum_FILE_MODE_READ);
    if (file != CN_INVALID_FILE_HANDLE)
    {
        char buffer[1024];
        int64_t bytes_read = platform->file_system->read_file(file, buffer, sizeof(buffer));
        printf("读取了 %lld 字节\n", bytes_read);
        platform->file_system->close_file(file);
    }
    
    // 使用时间管理接口
    uint64_t timestamp = platform->time->get_current_timestamp();
    printf("当前时间戳: %llu\n", timestamp);
    
    // 获取系统信息
    char system_info[1024];
    if (platform->get_system_info(system_info, sizeof(system_info)))
    {
        printf("系统信息:\n%s\n", system_info);
    }
    
    // 清理资源
    platform->cleanup();
    
    return 0;
}
```

### 单元测试中使用测试桩
```c
#include "CN_platform_interface.h"
#include "unity.h"

void setUp(void)
{
    // 使用测试桩接口
    const Stru_PlatformInterface_t* platform = F_get_stub_platform_interface();
    platform->initialize();
}

void test_file_operations(void)
{
    const Stru_PlatformInterface_t* platform = F_get_stub_platform_interface();
    
    // 测试文件打开
    void* file = platform->file_system->open_file("test.txt", Eum_FILE_MODE_READ);
    TEST_ASSERT_NOT_NULL(file);
    TEST_ASSERT_NOT_EQUAL(CN_INVALID_FILE_HANDLE, file);
    
    // 测试文件读取
    char buffer[100];
    int64_t bytes_read = platform->file_system->read_file(file, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT64(sizeof(buffer), bytes_read);
    
    // 测试文件关闭
    bool closed = platform->file_system->close_file(file);
    TEST_ASSERT_TRUE(closed);
}

void test_time_operations(void)
{
    const Stru_PlatformInterface_t* platform = F_get_stub_platform_interface();
    
    // 测试时间戳获取
    uint64_t timestamp = platform->time->get_current_timestamp();
    TEST_ASSERT_NOT_EQUAL(0, timestamp);
    
    // 测试时间转换
    Stru_TimeInfo_t time_info;
    bool success = platform->time->get_local_time(timestamp, &time_info);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_EQUAL(0, time_info.year);
}

void tearDown(void)
{
    const Stru_PlatformInterface_t* platform = F_get_stub_platform_interface();
    platform->cleanup();
}
```

## 错误处理

### 返回值检查
所有API函数都提供明确的返回值，调用者必须检查返回值：
- 布尔函数：检查`true`/`false`
- 指针函数：检查`NULL`或`CN_INVALID_*_HANDLE`
- 整数函数：检查特定错误值（通常是`-1`）

### 错误信息记录
重要错误应该记录日志：
```c
if (!platform->file_system->delete_file("important.txt"))
{
    CN_LOG(Eum_LOG_LEVEL_ERROR, "删除文件失败: %s", "important.txt");
}
```

## 线程安全

### 线程安全级别
1. **完全线程安全**：接口本身是线程安全的，多个线程可以同时调用
2. **句柄级别安全**：每个句柄（文件、目录、进程）独立，不同句柄可并行操作
3. **非线程安全**：需要调用者同步（文档会明确说明）

### 推荐用法
```c
// 正确：每个线程使用独立的文件句柄
void* file1 = platform->file_system->open_file("file1.txt", Eum_FILE_MODE_READ);
void* file2 = platform->file_system->open_file("file2.txt", Eum_FILE_MODE_READ);

// 在不同线程中并行读取
thread1_read(file1);
thread2_read(file2);

// 分别关闭
platform->file_system->close_file(file1);
platform->file_system->close_file(file2);
```

## 性能注意事项

### 接口调用开销
- 接口通过函数指针调用，有轻微性能开销
- 关键路径考虑批量操作减少调用次数
- 高频调用考虑缓存接口指针

### 资源管理
- 及时释放资源避免泄漏
- 使用RAII模式管理资源生命周期
- 避免频繁打开/关闭同一文件

## 平台特定行为

### 文件路径分隔符
- Windows: `\` 或 `/`
- Linux/macOS: `/`
- 建议使用平台无关的路径构建函数

### 文件权限
- Windows: 基于ACL的权限系统
- Linux/macOS: 基于UNIX权限位
- 接口提供统一的权限抽象

### 进程管理
- Windows: 基于CreateProcess/WaitForSingleObject
- Linux/macOS: 基于fork/exec/waitpid
- 接口提供统一的进程管理抽象

## 版本兼容性

### API版本
当前API版本：1.0.0

### 向后兼容性
- 现有API函数签名保持不变
- 新增函数添加到接口末尾
- 废弃函数标记为已弃用，但保持功能

### 平台支持矩阵
| 功能 | Windows | Linux | macOS | 测试桩 |
|------|---------|-------|-------|--------|
| 文件系统 | ✓ | 待实现 | 待实现 | ✓ |
| 进程管理 | ✓ | 待实现 | 待实现 | ✓ |
| 时间管理 | ✓ | 待实现 | 待实现 | ✓ |
| 系统信息 | ✓ | 待实现 | 待实现 | ✓ |

## 相关文档

- [平台抽象层模块文档](../../../src/infrastructure/platform/README.md)
- [CN_Language架构设计文档](../../../docs/architecture/001-中文编程语言CN_Language开发规划.md)
- [编码规范](../../../docs/specifications/CN_Language项目 技术规范和编码标准.md)

## 更新日志

### 版本 1.0.0 (2026-01-06)
- 初始版本发布
- 定义完整的平台抽象层接口
- 实现Windows平台支持
- 实现测试桩用于单元测试
- 提供完整的API文档

## 维护者

- CN_Language架构委员会
- 平台抽象层开发小组

## 许可证

MIT License
