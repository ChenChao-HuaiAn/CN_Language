# CN_Language 日志系统API文档

## 文档概述

本文档描述了CN_Language项目中日志系统的API接口。日志系统是基础设施层的一部分，提供统一的日志记录功能，支持多输出目标、日志级别过滤、自定义格式化和线程安全。

## 头文件

```c
#include "src/infrastructure/utils/logs/CN_log.h"
```

## 数据类型

### 日志级别枚举
```c
typedef enum Eum_CN_LogLevel_t
{
    Eum_LOG_LEVEL_TRACE = 0,    /**< 跟踪信息：最详细的调试信息 */
    Eum_LOG_LEVEL_DEBUG = 1,    /**< 调试信息：开发调试用 */
    Eum_LOG_LEVEL_INFO  = 2,    /**< 一般信息：正常操作信息 */
    Eum_LOG_LEVEL_WARN  = 3,    /**< 警告信息：可能有问题但程序可继续 */
    Eum_LOG_LEVEL_ERROR = 4,    /**< 错误信息：功能受影响但程序可继续 */
    Eum_LOG_LEVEL_FATAL = 5     /**< 致命错误：程序无法继续运行 */
} Eum_CN_LogLevel_t;
```

### 日志输出目标类型枚举
```c
typedef enum Eum_CN_LogOutputType_t
{
    Eum_LOG_OUTPUT_CONSOLE = 0,  /**< 控制台输出（stdout/stderr） */
    Eum_LOG_OUTPUT_FILE    = 1,  /**< 文件输出 */
    Eum_LOG_OUTPUT_NETWORK = 2,  /**< 网络输出（TCP/UDP） */
    Eum_LOG_OUTPUT_SYSLOG  = 3,  /**< 系统日志（syslog） */
    Eum_LOG_OUTPUT_CUSTOM  = 4   /**< 自定义输出处理器 */
} Eum_CN_LogOutputType_t;
```

### 日志格式化器接口
```c
typedef struct Stru_CN_LogFormatterInterface_t
{
    size_t (*format)(char* buffer, size_t buffer_size,
                     Eum_CN_LogLevel_t level,
                     long long timestamp,
                     const char* file,
                     int line,
                     const char* function,
                     const char* format,
                     va_list args);
    
    const char* (*get_name)(void);
    
} Stru_CN_LogFormatterInterface_t;
```

### 日志输出处理器接口
```c
typedef struct Stru_CN_LogOutputHandlerInterface_t
{
    bool (*initialize)(const void* config);
    bool (*write)(const char* message, size_t length, Eum_CN_LogLevel_t level);
    bool (*flush)(void);
    void (*shutdown)(void);
    Eum_CN_LogOutputType_t (*get_type)(void);
    
} Stru_CN_LogOutputHandlerInterface_t;
```

### 日志系统配置结构
```c
typedef struct Stru_CN_LogConfig_t
{
    Eum_CN_LogLevel_t default_level;      /**< 默认日志级别 */
    bool thread_safe;                     /**< 是否启用线程安全 */
    bool async_mode;                      /**< 是否启用异步日志 */
    size_t async_queue_size;              /**< 异步队列大小 */
    bool enable_colors;                   /**< 是否启用颜色输出 */
    bool show_timestamp;                  /**< 是否显示时间戳 */
    bool show_source_location;            /**< 是否显示源文件位置 */
    bool show_thread_id;                  /**< 是否显示线程ID */
    const char* default_formatter;        /**< 默认格式化器名称 */
    
} Stru_CN_LogConfig_t;
```

## API函数参考

### 系统初始化和关闭

#### `CN_log_init`
```c
bool CN_log_init(const Stru_CN_LogConfig_t* config);
```
**功能**：初始化日志系统

**参数**：
- `config`：日志系统配置，如果为NULL则使用默认配置

**返回值**：
- `true`：初始化成功
- `false`：初始化失败

**示例**：
```c
// 使用默认配置
CN_log_init(NULL);

// 使用自定义配置
Stru_CN_LogConfig_t config = {
    .default_level = Eum_LOG_LEVEL_DEBUG,
    .thread_safe = true,
    .async_mode = false,
    .enable_colors = true
};
CN_log_init(&config);
```

#### `CN_log_shutdown`
```c
void CN_log_shutdown(void);
```
**功能**：关闭日志系统，释放所有资源

**注意**：程序退出前应调用此函数

### 日志记录函数

#### `CN_log_core`
```c
void CN_log_core(Eum_CN_LogLevel_t level,
                 const char* file,
                 int line,
                 const char* function,
                 const char* format,
                 ...);
```
**功能**：记录日志（核心函数）

**参数**：
- `level`：日志级别
- `file`：源文件名（使用`__FILE__`）
- `line`：源文件行号（使用`__LINE__`）
- `function`：函数名（使用`__func__`）
- `format`：格式字符串
- `...`：可变参数

**示例**：
```c
CN_log_core(Eum_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, 
            "用户 %s 登录成功", username);
```

#### `CN_log`
```c
void CN_log(Eum_CN_LogLevel_t level, const char* format, ...);
```
**功能**：记录日志（简化接口，不包含源文件信息）

**参数**：
- `level`：日志级别
- `format`：格式字符串
- `...`：可变参数

**示例**：
```c
CN_log(Eum_LOG_LEVEL_WARN, "磁盘空间不足: %d%%", disk_usage);
```

### 便捷日志宏

#### `CN_LOG_TRACE`
```c
#define CN_LOG_TRACE(format, ...)
```
**功能**：记录TRACE级别日志

**示例**：
```c
CN_LOG_TRACE("进入函数 %s，参数: %d", __func__, param);
```

#### `CN_LOG_DEBUG`
```c
#define CN_LOG_DEBUG(format, ...)
```
**功能**：记录DEBUG级别日志

**示例**：
```c
CN_LOG_DEBUG("计算完成，结果: %f", result);
```

#### `CN_LOG_INFO`
```c
#define CN_LOG_INFO(format, ...)
```
**功能**：记录INFO级别日志

**示例**：
```c
CN_LOG_INFO("服务器启动在端口 %d", port);
```

#### `CN_LOG_WARN`
```c
#define CN_LOG_WARN(format, ...)
```
**功能**：记录WARN级别日志

**示例**：
```c
CN_LOG_WARN("配置项 %s 未设置，使用默认值", config_name);
```

#### `CN_LOG_ERROR`
```c
#define CN_LOG_ERROR(format, ...)
```
**功能**：记录ERROR级别日志

**示例**：
```c
CN_LOG_ERROR("文件 %s 打开失败: %s", filename, strerror(errno));
```

#### `CN_LOG_FATAL`
```c
#define CN_LOG_FATAL(format, ...)
```
**功能**：记录FATAL级别日志

**示例**：
```c
CN_LOG_FATAL("内存分配失败，程序退出");
```

### 输出管理函数

#### `CN_log_add_output`
```c
bool CN_log_add_output(Eum_CN_LogOutputType_t type, const void* config);
```
**功能**：添加日志输出处理器

**参数**：
- `type`：输出类型（控制台、文件、网络、系统日志）
- `config`：配置参数（类型特定）

**返回值**：
- `true`：添加成功
- `false`：添加失败

**示例**：
```c
// 添加控制台输出
CN_log_add_output(Eum_LOG_OUTPUT_CONSOLE, NULL);

// 添加文件输出
Stru_CN_FileConfig_t file_config = {
    .filename = "logs/app.log",
    .mode = "a",
    .max_size = 10 * 1024 * 1024, // 10MB
    .max_files = 5
};
CN_log_add_output(Eum_LOG_OUTPUT_FILE, &file_config);
```

#### `CN_log_remove_output`
```c
bool CN_log_remove_output(Eum_CN_LogOutputType_t type);
```
**功能**：移除日志输出处理器

**参数**：
- `type`：输出类型

**返回值**：
- `true`：移除成功
- `false`：移除失败（类型不存在）

#### `CN_log_remove_all_outputs`
```c
void CN_log_remove_all_outputs(void);
```
**功能**：移除所有输出处理器

#### `CN_log_set_output_handler`
```c
bool CN_log_set_output_handler(Eum_CN_LogOutputType_t type,
                               const Stru_CN_LogOutputHandlerInterface_t* handler);
```
**功能**：设置输出处理器

**参数**：
- `type`：输出类型
- `handler`：处理器接口

**返回值**：
- `true`：设置成功
- `false`：设置失败

### 格式化器管理函数

#### `CN_log_register_formatter`
```c
bool CN_log_register_formatter(const char* name,
                               const Stru_CN_LogFormatterInterface_t* formatter);
```
**功能**：注册日志格式化器

**参数**：
- `name`：格式化器名称
- `formatter`：格式化器接口

**返回值**：
- `true`：注册成功
- `false`：注册失败

#### `CN_log_set_formatter`
```c
bool CN_log_set_formatter(const char* name);
```
**功能**：设置当前格式化器

**参数**：
- `name`：格式化器名称

**返回值**：
- `true`：设置成功
- `false`：设置失败（名称不存在）

### 配置管理函数

#### `CN_log_get_config`
```c
Stru_CN_LogConfig_t CN_log_get_config(void);
```
**功能**：获取当前配置

**返回值**：当前配置

#### `CN_log_set_config`
```c
bool CN_log_set_config(const Stru_CN_LogConfig_t* config);
```
**功能**：更新配置

**参数**：
- `config`：新配置

**返回值**：
- `true`：更新成功
- `false`：更新失败

#### `CN_log_set_level`
```c
void CN_log_set_level(Eum_CN_LogLevel_t level);
```
**功能**：设置日志级别

**参数**：
- `level`：新的日志级别

**示例**：
```c
// 生产环境只记录WARN及以上级别
CN_log_set_level(Eum_LOG_LEVEL_WARN);
```

#### `CN_log_get_level`
```c
Eum_CN_LogLevel_t CN_log_get_level(void);
```
**功能**：获取当前日志级别

**返回值**：当前日志级别

### 性能优化函数

#### `CN_log_set_enabled`
```c
void CN_log_set_enabled(bool enabled);
```
**功能**：启用/禁用日志

**参数**：
- `enabled`：`true`启用日志，`false`禁用日志

**示例**：
```c
// 性能测试时禁用日志
CN_log_set_enabled(false);
```

#### `CN_log_is_enabled`
```c
bool CN_log_is_enabled(void);
```
**功能**：检查日志是否启用

**返回值**：日志启用状态

#### `CN_log_flush_all`
```c
void CN_log_flush_all(void);
```
**功能**：刷新所有输出

**注意**：确保所有缓冲的日志被写入

#### `CN_log_get_stats`
```c
void CN_log_get_stats(size_t* total_logs, size_t* filtered_logs, size_t* failed_writes);
```
**功能**：获取日志系统统计信息

**参数**：
- `total_logs`：输出参数，总日志数
- `filtered_logs`：输出参数，被过滤的日志数
- `failed_writes`：输出参数，写入失败的日志数

**示例**：
```c
size_t total, filtered, failed;
CN_log_get_stats(&total, &filtered, &failed);
CN_LOG_INFO("统计: 总日志=%zu, 过滤=%zu, 失败=%zu", total, filtered, failed);
```

## 配置结构体

### 控制台输出配置
```c
typedef struct Stru_CN_ConsoleConfig_t
{
    bool use_stderr_for_errors;  /**< 错误级别日志是否输出到stderr */
    bool enable_colors;          /**< 是否启用颜色输出 */
} Stru_CN_ConsoleConfig_t;
```

### 文件输出配置
```c
typedef struct Stru_CN_FileConfig_t
{
    const char* filename;        /**< 文件名 */
    const char* mode;            /**< 打开模式 ("a"=追加, "w"=覆盖) */
    size_t max_size;             /**< 最大文件大小（字节，0=无限制） */
    size_t max_files;            /**< 最大文件数（日志轮转） */
} Stru_CN_FileConfig_t;
```

### 网络输出配置
```c
typedef struct Stru_CN_NetworkConfig_t
{
    const char* host;            /**< 主机名或IP地址 */
    int port;                    /**< 端口号 */
    int protocol;                /**< 协议类型（0=TCP, 1=UDP） */
} Stru_CN_NetworkConfig_t;
```

## 使用示例

### 基本示例
```c
#include "src/infrastructure/utils/logs/CN_log.h"

int main()
{
    // 初始化日志系统
    CN_log_init(NULL);
    
    // 添加控制台输出
    CN_log_add_output(Eum_LOG_OUTPUT_CONSOLE, NULL);
    
    // 记录日志
    CN_LOG_INFO("应用程序启动");
    CN_LOG_DEBUG("初始化完成，版本: %s", "1.0.0");
    
    int error_count = 5;
    if (error_count > 0)
    {
        CN_LOG_WARN("发现 %d 个警告", error_count);
    }
    
    // 关闭日志系统
    CN_log_shutdown();
    
    return 0;
}
```

### 高级配置示例
```c
#include "src/infrastructure/utils/logs/CN_log.h"

int main()
{
    // 自定义配置
    Stru_CN_LogConfig_t config = {
        .default_level = Eum_LOG_LEVEL_DEBUG,
        .thread_safe = true,
        .async_mode = true,
        .async_queue_size = 1000,
        .enable_colors = true,
        .show_timestamp = true,
        .show_source_location = true,
        .show_thread_id = false,
        .default_formatter = "default"
    };
    
    // 初始化日志系统
    CN_log_init(&config);
    
    // 添加多个输出
    CN_log_add_output(Eum_LOG_OUTPUT_CONSOLE, NULL);
    
    Stru_CN_FileConfig_t file_config = {
        .filename = "logs/application.log",
        .mode = "a",
        .max_size = 5 * 1024 * 1024, // 5MB
        .max_files = 10
    };
    CN_log_add_output(Eum_LOG_OUTPUT_FILE, &file_config);
    
    // 使用不同级别的日志
    CN_LOG_TRACE("进入处理循环");
    for (int i = 0; i < 10; i++)
    {
        CN_LOG_DEBUG("处理第 %d 个项目", i + 1);
        
        if (i == 5)
        {
            CN_LOG_WARN("遇到特殊情况，索引: %d", i);
        }
    }
    CN_LOG_TRACE("退出处理循环");
    
    // 动态调整日志级别
    CN_log_set_level(Eum_LOG_LEVEL_INFO);
    CN_LOG_DEBUG("这条调试信息不会被记录"); // 被过滤
    
    // 获取统计信息
    size_t total, filtered, failed;
    CN_log_get_stats(&total, &filtered, &failed);
    CN_LOG_INFO("日志统计 - 总数: %zu, 过滤: %zu, 失败: %zu", 
                total, filtered, failed);
    
    // 关闭日志系统
    CN_log_shutdown();
    
    return 0;
}
```

### 自定义格式化器示例
```c
#include "src/infrastructure/utils/logs/CN_log.h"

// 自定义格式化器实现
static size_t my_formatter_format(char* buffer, size_t buffer_size,
                                  Eum_CN_LogLevel_t level,
                                  long long timestamp,
                                  const char* file,
                                  int line,
                                  const char* function,
                                  const char* format,
                                  va_list args)
{
    // 简单格式: [级别] 消息
    const char* level_str = CN_log_level_to_string(level);
    size_t offset = snprintf(buffer, buffer_size, "[%s] ", level_str);
    offset += vsnprintf(buffer + offset, buffer_size - offset, format, args);
    
    if (offset < buffer_size - 1)
    {
        buffer[offset] = '\n';
        buffer[offset + 1] = '\0';
        offset++;
    }
    
    return offset;
}

static const char* my_formatter_get_name(void)
{
    return "my_simple";
}

static const Stru_CN_LogFormatterInterface_t my_formatter = {
    .format = my_formatter_format,
    .get_name = my_formatter_get_name
};

int main()
{
    CN_log_init(NULL);
    CN_log_add_output(Eum_LOG_OUTPUT_CONSOLE, NULL);
    
    // 注册并使用自定义格式化器
    CN_log_register_formatter("my_simple", &my_formatter);
    CN_log_set_formatter("my_simple");
    
    CN_LOG_INFO("使用自定义格式化器");
    
    CN_log_shutdown();
    return 0;
}
```

## 错误处理

### 初始化失败
- `CN_log_init`返回`false`表示初始化失败
- 可能原因：内存不足、系统资源不足

### 输出处理器失败
- `CN_log_add_output`返回`false`表示添加失败
- 可能原因：配置无效、文件无法打开、资源不足

### 日志记录失败
- 日志记录失败时内部统计计数器增加
- 可通过`CN_log_get_stats`函数获取失败次数统计
- 失败不会导致程序崩溃，日志系统会继续运行

### 资源管理失败
- 内存分配失败：日志系统会尝试降级处理
- 文件系统错误：文件输出处理器会记录错误并尝试恢复
- 网络连接失败：网络输出处理器会尝试重连或降级到其他输出

## 线程安全

### 线程安全保证
- 当`thread_safe`配置为`true`时，日志系统是线程安全的
- 多个线程可以同时调用日志函数而不会导致数据竞争
- 异步模式下，日志消息通过队列传递，确保线程安全

### 使用建议
1. 多线程应用程序应启用线程安全模式
2. 单线程应用程序可禁用线程安全以提高性能
3. 异步模式适合高并发场景，但会增加内存使用

## 性能优化

### 性能特性
1. **级别过滤**：低级别日志在记录前被过滤，减少格式化开销
2. **异步日志**：将日志写入操作移到后台线程，减少主线程阻塞
3. **批量写入**：多个日志消息可能被批量写入输出
4. **内存池**：使用内存池减少动态内存分配

### 性能建议
1. 生产环境使用`Eum_LOG_LEVEL_INFO`或更高级别
2. 性能关键代码路径使用`CN_log_is_enabled`检查
3. 高并发场景启用异步模式
4. 定期调用`CN_log_flush_all`确保日志持久化

## 扩展指南

### 添加新的输出处理器
1. 实现`Stru_CN_LogOutputHandlerInterface_t`接口
2. 在初始化函数中注册处理器
3. 提供相应的配置结构

### 添加新的格式化器
1. 实现`Stru_CN_LogFormatterInterface_t`接口
2. 调用`CN_log_register_formatter`注册
3. 使用`CN_log_set_formatter`激活

### 平台特定实现
- 网络输出：需要平台特定的网络API
- 系统日志：需要平台特定的syslog API
- 文件系统：需要处理平台特定的路径和权限

## 兼容性

### 支持的平台
- Windows (MinGW/MSVC)
- Linux (GCC/Clang)
- macOS (Clang)
- 其他POSIX兼容系统

### 编译器要求
- C99标准或更高
- 支持可变参数宏
- 支持`__func__`预定义标识符

### 依赖关系
- C标准库：`stdio.h`, `stdlib.h`, `string.h`, `time.h`
- 线程库：`pthread.h`（Linux/macOS）或Windows线程API
- 网络库：平台特定的网络API（可选）

## 已知限制

### 功能限制
1. 网络输出需要平台特定实现
2. 系统日志输出需要平台特定实现
3. 异步模式会增加内存使用
4. 日志轮转仅支持文件输出

### 性能限制
1. 同步模式下，日志写入可能阻塞调用线程
2. 格式化复杂消息可能影响性能
3. 大量日志输出可能消耗磁盘/网络带宽

### 安全考虑
1. 日志文件应设置适当权限
2. 网络日志应考虑加密传输
3. 敏感信息不应记录在日志中

## 故障排除

### 常见问题

#### 1. 日志没有输出
- 检查日志级别设置
- 确认已添加输出处理器
- 验证日志系统已初始化

#### 2. 性能问题
- 考虑启用异步模式
- 提高日志级别过滤更多消息
- 检查输出处理器性能

#### 3. 内存泄漏
- 确保调用`CN_log_shutdown`释放资源
- 检查自定义格式化器/输出处理器的资源管理

#### 4. 线程安全问题
- 确认启用了线程安全模式
- 检查自定义组件的线程安全性

### 调试建议
1. 启用TRACE级别日志查看内部操作
2. 使用`CN_log_get_stats`监控日志系统状态
3. 检查系统资源使用情况

## 版本历史

### v1.0.0 (2026-01-03)
- 初始版本
- 支持多输出目标（控制台、文件）
- 支持多格式化器（默认、简单、JSON）
- 线程安全和异步日志支持
- 完整的API文档

## 相关文档

### 项目文档
- [架构设计原则](../architecture/架构设计原则.md)
- [模块依赖规范](../architecture/模块依赖规范.md)
- [实施路线图](../architecture/实施路线图.md)

### 技术规范
- [CN_Language项目 技术规范和编码标准](../../specifications/CN_Language项目%20技术规范和编码标准.md)
- [CN_Language项目 目录结构规范](../../specifications/CN_Language项目%20目录结构规范.md)

### 模块文档
- [日志系统README](../../../src/infrastructure/utils/logs/README.md)

## 版权和许可

版权所有 © 2026 CN_Language项目团队

本软件根据MIT许可证授权。有关详细信息，请参阅项目根目录中的LICENSE文件。

---

*文档最后更新: 2026-01-03*
