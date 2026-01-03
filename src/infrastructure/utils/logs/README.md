# CN_Language 日志系统

## 概述

CN_Language日志系统是一个模块化、高性能、线程安全的日志库，专为CN_Language项目设计。它支持多输出目标、日志级别过滤、自定义格式化和异步日志记录。

## 特性

- **多输出目标**：支持控制台、文件、网络和系统日志输出
- **日志级别过滤**：6个日志级别（TRACE, DEBUG, INFO, WARN, ERROR, FATAL）
- **自定义格式化**：支持默认、简单和JSON格式，可扩展自定义格式
- **线程安全**：内置线程安全机制，支持多线程环境
- **异步日志**：支持异步日志记录，提高性能
- **日志轮转**：文件输出支持日志轮转和大小限制
- **颜色输出**：控制台输出支持颜色高亮

## 快速开始

### 基本使用

```c
#include "CN_log.h"

int main()
{
    // 初始化日志系统（使用默认配置）
    CN_log_init(NULL);
    
    // 添加控制台输出
    CN_log_add_output(Eum_LOG_OUTPUT_CONSOLE, NULL);
    
    // 记录日志
    CN_LOG_INFO("应用程序启动");
    CN_LOG_DEBUG("用户 %s 登录，ID: %d", "张三", 1001);
    CN_LOG_WARN("磁盘空间不足");
    CN_LOG_ERROR("文件打开失败: %s", "config.json");
    
    // 关闭日志系统
    CN_log_shutdown();
    
    return 0;
}
```

### 高级配置

```c
#include "CN_log.h"

int main()
{
    // 自定义配置
    Stru_CN_LogConfig_t config = {
        .default_level = Eum_LOG_LEVEL_DEBUG,
        .thread_safe = true,
        .async_mode = false,
        .async_queue_size = 1000,
        .enable_colors = true,
        .show_timestamp = true,
        .show_source_location = true,
        .show_thread_id = false,
        .default_formatter = "default"
    };
    
    // 初始化日志系统
    CN_log_init(&config);
    
    // 添加文件输出
    Stru_CN_FileConfig_t file_config = {
        .filename = "logs/app.log",
        .mode = "a",
        .max_size = 10 * 1024 * 1024, // 10MB
        .max_files = 5
    };
    CN_log_add_output(Eum_LOG_OUTPUT_FILE, &file_config);
    
    // 使用不同的日志级别
    CN_LOG_TRACE("详细的跟踪信息");
    CN_LOG_DEBUG("调试信息");
    CN_LOG_INFO("一般信息");
    CN_LOG_WARN("警告信息");
    CN_LOG_ERROR("错误信息");
    CN_LOG_FATAL("致命错误");
    
    // 动态调整日志级别
    CN_log_set_level(Eum_LOG_LEVEL_WARN);
    
    // 这个日志不会被记录（级别低于WARN）
    CN_LOG_INFO("这条信息不会被记录");
    
    // 获取统计信息
    size_t total, filtered, failed;
    CN_log_get_stats(&total, &filtered, &failed);
    CN_LOG_INFO("统计: 总日志=%zu, 过滤=%zu, 失败=%zu", total, filtered, failed);
    
    // 关闭日志系统
    CN_log_shutdown();
    
    return 0;
}
```

## API 参考

### 核心函数

#### `bool CN_log_init(const Stru_CN_LogConfig_t* config)`
初始化日志系统。

**参数**：
- `config`: 日志系统配置，如果为NULL则使用默认配置

**返回值**：
- `true`: 初始化成功
- `false`: 初始化失败

#### `void CN_log_shutdown(void)`
关闭日志系统，释放所有资源。

#### `void CN_log_core(Eum_CN_LogLevel_t level, const char* file, int line, const char* function, const char* format, ...)`
记录日志（核心函数）。

**参数**：
- `level`: 日志级别
- `file`: 源文件名（使用`__FILE__`）
- `line`: 源文件行号（使用`__LINE__`）
- `function`: 函数名（使用`__func__`）
- `format`: 格式字符串
- `...`: 可变参数

#### `void CN_log(Eum_CN_LogLevel_t level, const char* format, ...)`
记录日志（简化接口，不包含源文件信息）。

### 便捷宏

#### `CN_LOG_TRACE(format, ...)`
记录TRACE级别日志。

#### `CN_LOG_DEBUG(format, ...)`
记录DEBUG级别日志。

#### `CN_LOG_INFO(format, ...)`
记录INFO级别日志。

#### `CN_LOG_WARN(format, ...)`
记录WARN级别日志。

#### `CN_LOG_ERROR(format, ...)`
记录ERROR级别日志。

#### `CN_LOG_FATAL(format, ...)`
记录FATAL级别日志。

### 输出管理

#### `bool CN_log_add_output(Eum_CN_LogOutputType_t type, const void* config)`
添加日志输出处理器。

**参数**：
- `type`: 输出类型（控制台、文件、网络、系统日志）
- `config`: 配置参数（类型特定）

**返回值**：
- `true`: 添加成功
- `false`: 添加失败

#### `bool CN_log_remove_output(Eum_CN_LogOutputType_t type)`
移除日志输出处理器。

#### `void CN_log_remove_all_outputs(void)`
移除所有输出处理器。

### 配置管理

#### `Stru_CN_LogConfig_t CN_log_get_config(void)`
获取当前配置。

#### `bool CN_log_set_config(const Stru_CN_LogConfig_t* config)`
更新配置。

#### `void CN_log_set_level(Eum_CN_LogLevel_t level)`
设置日志级别。

#### `Eum_CN_LogLevel_t CN_log_get_level(void)`
获取当前日志级别。

### 性能优化

#### `void CN_log_set_enabled(bool enabled)`
启用/禁用日志。

#### `bool CN_log_is_enabled(void)`
检查日志是否启用。

#### `void CN_log_flush_all(void)`
刷新所有输出。

#### `void CN_log_get_stats(size_t* total_logs, size_t* filtered_logs, size_t* failed_writes)`
获取日志系统统计信息。

## 日志级别

| 级别 | 值 | 描述 |
|------|-----|------|
| TRACE | 0 | 最详细的调试信息，用于跟踪程序执行流程 |
| DEBUG | 1 | 调试信息，用于开发调试 |
| INFO | 2 | 一般信息，正常操作信息 |
| WARN | 3 | 警告信息，可能有问题但程序可继续 |
| ERROR | 4 | 错误信息，功能受影响但程序可继续 |
| FATAL | 5 | 致命错误，程序无法继续运行 |

## 输出类型

### 控制台输出 (`Eum_LOG_OUTPUT_CONSOLE`)
输出到标准输出（stdout）或标准错误（stderr）。

**配置**：`Stru_CN_ConsoleConfig_t`
- `use_stderr_for_errors`: 错误级别日志是否输出到stderr
- `enable_colors`: 是否启用颜色输出

### 文件输出 (`Eum_LOG_OUTPUT_FILE`)
输出到文件，支持日志轮转。

**配置**：`Stru_CN_FileConfig_t`
- `filename`: 文件名
- `mode`: 打开模式 ("a"=追加, "w"=覆盖)
- `max_size`: 最大文件大小（字节，0=无限制）
- `max_files`: 最大文件数（日志轮转）

### 网络输出 (`Eum_LOG_OUTPUT_NETWORK`)
输出到网络（TCP/UDP）。

**配置**：`Stru_CN_NetworkConfig_t`
- `host`: 主机名或IP地址
- `port`: 端口号
- `protocol`: 协议类型（0=TCP, 1=UDP）

### 系统日志输出 (`Eum_LOG_OUTPUT_SYSLOG`)
输出到系统日志（syslog）。

## 格式化器

### 默认格式化器 (`"default"`)
包含时间戳、日志级别、源文件位置和消息。

示例：`[2026-01-03 12:30:45.123] [INFO] [main.c:42 main] 应用程序启动`

### 简单格式化器 (`"simple"`)
只包含日志级别和消息。

示例：`[INFO] 应用程序启动`

### JSON格式化器 (`"json"`)
输出JSON格式的日志。

示例：`{"timestamp":"2026-01-03 12:30:45.123","level":"INFO","file":"main.c","line":42,"function":"main","message":"应用程序启动"}`

## 线程安全

日志系统默认启用线程安全。可以通过配置中的`thread_safe`字段控制：
- `true`: 启用线程安全（默认）
- `false`: 禁用线程安全（提高性能，但仅适用于单线程环境）

## 异步日志

通过配置中的`async_mode`字段启用异步日志：
- `true`: 启用异步日志，日志消息先放入队列，由后台线程处理
- `false`: 启用同步日志，日志消息立即处理（默认）

异步队列大小通过`async_queue_size`配置。

## 性能考虑

1. **日志级别过滤**：在生产环境中设置较高的日志级别（如WARN或ERROR），减少不必要的日志记录
2. **异步日志**：在高并发场景下启用异步日志，避免日志记录阻塞主线程
3. **文件输出缓冲**：文件输出使用系统缓冲，定期调用`CN_log_flush_all()`确保日志持久化
4. **内存管理**：使用预分配缓冲区，减少动态内存分配

## 扩展指南

### 自定义格式化器

实现`Stru_CN_LogFormatterInterface_t`接口并注册：

```c
static size_t my_formatter_format(char* buffer, size_t buffer_size,
                                  Eum_CN_LogLevel_t level,
                                  long long timestamp,
                                  const char* file,
                                  int line,
                                  const char* function,
                                  const char* format,
                                  va_list args)
{
    // 自定义格式化逻辑
    return snprintf(buffer, buffer_size, "自定义格式: %s", format);
}

static const char* my_formatter_get_name(void)
{
    return "my_formatter";
}

static const Stru_CN_LogFormatterInterface_t my_formatter = {
    .format = my_formatter_format,
    .get_name = my_formatter_get_name
};

// 注册格式化器
CN_log_register_formatter("my_formatter", &my_formatter);
CN_log_set_formatter("my_formatter");
```

### 自定义输出处理器

实现`Stru_CN_LogOutputHandlerInterface_t`接口并设置：

```c
static bool my_output_initialize(const void* config)
{
    // 初始化逻辑
    return true;
}

static bool my_output_write(const char* message, size_t length, Eum_CN_LogLevel_t level)
{
    // 输出逻辑
    return true;
}

static bool my_output_flush(void)
{
    // 刷新逻辑
    return true;
}

static void my_output_shutdown(void)
{
    // 清理逻辑
}

static Eum_CN_LogOutputType_t my_output_get_type(void)
{
    return Eum_LOG_OUTPUT_CUSTOM;
}

static const Stru_CN_LogOutputHandlerInterface_t my_output_handler = {
    .initialize = my_output_initialize,
    .write = my_output_write,
    .flush = my_output_flush,
    .shutdown = my_output_shutdown,
    .get_type = my_output_get_type
};

// 设置输出处理器
CN_log_set_output_handler(Eum_LOG_OUTPUT_CUSTOM, &my_output_handler);
```

## 文件结构

```
src/infrastructure/utils/logs/
├── CN_log.h              # 主头文件，公共API
├── CN_log_internal.h     # 内部头文件，内部结构和函数
├── CN_log_core.c         # 核心实现
├── CN_log_formatters.c   # 格式化器实现
├── CN_log_outputs.c      # 输出处理器实现
└── README.md            # 本文档
```

## 编译和链接

将日志系统文件添加到项目中并链接：

```makefile
# Makefile示例
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./src/infrastructure/utils/logs

LOG_SOURCES = src/infrastructure/utils/logs/CN_log_core.c \
              src/infrastructure/utils/logs/CN_log_formatters.c \
              src/infrastructure/utils/logs/CN_log_outputs.c

LOG_OBJECTS = $(LOG_SOURCES:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

libcnlog.a: $(LOG_OBJECTS)
	ar rcs $@ $(LOG_OBJECTS)

clean:
	rm -f $(LOG_OBJECTS) libcnlog.a
```

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件。

## 作者

CN_Language开发团队

## 版本历史

- v1.0.0 (2026-01-03): 初始版本
  - 支持多输出目标
  - 支持日志级别过滤
  - 支持自定义格式化
  - 线程安全和异步日志
