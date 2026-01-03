# CN_Language 错误处理框架 API 文档

## 文档概述

本文档详细描述了CN_Language错误处理框架的API接口。错误处理框架为CN_Language项目提供统一的错误管理功能，包括错误码定义、错误上下文传递、错误链支持和全局错误处理。

## 1. 核心头文件

### 1.1 CN_error.h - 主头文件

主头文件提供统一的错误处理接口，包含框架初始化、错误报告、配置管理等核心功能。

#### 1.1.1 框架初始化和清理

```c
/**
 * @brief 错误处理框架初始化
 * 
 * 初始化错误处理框架，设置默认配置。
 * 此函数应在程序开始时调用。
 * 
 * @return true 如果初始化成功，false 如果失败
 */
bool CN_error_framework_init(void);

/**
 * @brief 错误处理框架清理
 * 
 * 清理错误处理框架，释放所有资源。
 * 此函数应在程序结束时调用。
 */
void CN_error_framework_cleanup(void);
```

#### 1.1.2 错误报告

```c
/**
 * @brief 报告错误到全局处理器
 * 
 * @param context 错误上下文
 */
void CN_error_report(const Stru_CN_ErrorContext_t* context);

/**
 * @brief 快速报告错误（简化接口）
 * 
 * @param error_code 错误码
 * @param message 错误消息
 * @param filename 文件名
 * @param line 行号
 */
void CN_error_report_quick(
    Eum_CN_ErrorCode_t error_code,
    const char* message,
    const char* filename,
    size_t line);

/**
 * @brief 获取最后一个报告的错误
 * 
 * @return 最后一个错误的上下文，如果没有错误则返回NULL
 */
const Stru_CN_ErrorContext_t* CN_error_get_last_reported(void);

/**
 * @brief 清除所有报告的错误
 */
void CN_error_clear_all_reported(void);

/**
 * @brief 检查是否有错误发生
 * 
 * @return true 如果有错误发生，false 如果没有错误
 */
bool CN_error_has_occurred(void);
```

#### 1.1.3 错误信息查询

```c
/**
 * @brief 获取错误严重性级别
 * 
 * @param error_code 错误码
 * @return 错误严重性级别（0-10，0表示无错误，10表示最严重）
 */
int CN_error_get_severity(Eum_CN_ErrorCode_t error_code);

/**
 * @brief 检查错误是否可恢复
 * 
 * @param error_code 错误码
 * @return true 如果错误可恢复，false 如果不可恢复
 */
bool CN_error_is_recoverable(Eum_CN_ErrorCode_t error_code);
```

#### 1.1.4 全局错误处理

```c
/**
 * @brief 设置全局错误处理回调
 * 
 * @param callback 错误处理回调函数
 * @param user_data 用户数据
 */
void CN_error_set_global_handler(
    void (*callback)(const Stru_CN_ErrorContext_t* context, void* user_data),
    void* user_data);
```

#### 1.1.5 框架配置

```c
/**
 * @brief 错误处理框架配置结构体
 */
typedef struct Stru_CN_ErrorConfig_t
{
    /** 是否启用详细日志 */
    bool verbose_logging;
    
    /** 最大错误链长度（0表示无限制） */
    size_t max_chain_length;
    
    /** 是否自动报告到标准错误输出 */
    bool auto_report_to_stderr;
    
    /** 是否启用错误统计 */
    bool enable_statistics;
    
    /** 错误报告格式 */
    const char* report_format;
    
} Stru_CN_ErrorConfig_t;

/**
 * @brief 获取当前配置
 * 
 * @return 当前配置
 */
Stru_CN_ErrorConfig_t CN_error_get_config(void);

/**
 * @brief 设置配置
 * 
 * @param config 新配置
 */
void CN_error_set_config(const Stru_CN_ErrorConfig_t* config);

/**
 * @brief 错误处理框架版本信息
 * 
 * @return 版本字符串
 */
const char* CN_error_framework_version(void);
```

### 1.2 CN_error_codes.h - 错误码定义

错误码定义头文件包含所有错误码的枚举定义和描述函数。

#### 1.2.1 错误码枚举

```c
/**
 * @brief CN_Language统一错误码枚举
 * 
 * 定义了CN_Language项目中使用的所有错误码，按模块分类。
 * 错误码范围分配：
 * - 0x0000-0x0FFF: 通用错误
 * - 0x1000-0x1FFF: 基础设施层错误
 * - 0x2000-0x2FFF: 核心层错误
 * - 0x3000-0x3FFF: 应用层错误
 * - 0x4000-0x4FFF: 用户自定义错误
 */
typedef enum Eum_CN_ErrorCode_t
{
    /* ==================== 通用错误 (0x0000-0x0FFF) ==================== */
    
    /** 成功，无错误 */
    Eum_CN_ERROR_SUCCESS = 0x0000,
    
    /** 通用失败，未指定具体原因 */
    Eum_CN_ERROR_FAILURE = 0x0001,
    
    /** 内存不足错误 */
    Eum_CN_ERROR_OUT_OF_MEMORY = 0x0002,
    
    /** 无效参数错误 */
    Eum_CN_ERROR_INVALID_ARGUMENT = 0x0003,
    
    /** 空指针错误 */
    Eum_CN_ERROR_NULL_POINTER = 0x0004,
    
    /** 索引越界错误 */
    Eum_CN_ERROR_OUT_OF_BOUNDS = 0x0005,
    
    // ... 更多错误码定义
} Eum_CN_ErrorCode_t;
```

#### 1.2.2 错误码描述函数

```c
/**
 * @brief 获取错误码的描述信息
 * 
 * @param error_code 错误码
 * @return 错误描述字符串
 */
const char* CN_error_get_description(Eum_CN_ErrorCode_t error_code);
```

### 1.3 CN_error_context.h - 错误上下文

错误上下文头文件定义错误上下文结构体和相关操作函数。

#### 1.3.1 错误上下文结构体

```c
/**
 * @brief 错误上下文结构体
 * 
 * 包含错误的详细上下文信息，如文件名、行号、列号等。
 */
typedef struct Stru_CN_ErrorContext_t
{
    /** 错误码 */
    Eum_CN_ErrorCode_t error_code;
    
    /** 错误消息（可选的详细描述） */
    const char* message;
    
    /** 文件名（发生错误的源文件） */
    const char* filename;
    
    /** 行号（从1开始） */
    size_t line;
    
    /** 列号（从1开始） */
    size_t column;
    
    /** 函数名 */
    const char* function;
    
    /** 模块名 */
    const char* module;
    
    /** 时间戳（毫秒） */
    uint64_t timestamp;
    
    /** 线程ID */
    uint64_t thread_id;
    
    /** 是否可恢复 */
    bool recoverable;
    
    /** 用户自定义数据指针 */
    void* user_data;
    
    /** 用户自定义数据大小 */
    size_t user_data_size;
    
} Stru_CN_ErrorContext_t;
```

#### 1.3.2 错误上下文创建函数

```c
/**
 * @brief 创建错误上下文
 * 
 * @param error_code 错误码
 * @param message 错误消息（可以为NULL）
 * @param filename 文件名（可以为NULL）
 * @param line 行号
 * @param column 列号
 * @param function 函数名（可以为NULL）
 * @param module 模块名（可以为NULL）
 * @return 错误上下文结构体
 */
Stru_CN_ErrorContext_t CN_error_create_context(
    Eum_CN_ErrorCode_t error_code,
    const char* message,
    const char* filename,
    size_t line,
    size_t column,
    const char* function,
    const char* module);

/**
 * @brief 创建简单的错误上下文（只有错误码和消息）
 * 
 * @param error_code 错误码
 * @param message 错误消息
 * @return 错误上下文结构体
 */
Stru_CN_ErrorContext_t CN_error_create_simple_context(
    Eum_CN_ErrorCode_t error_code,
    const char* message);

/**
 * @brief 创建带位置信息的错误上下文
 * 
 * @param error_code 错误码
 * @param message 错误消息
 * @param filename 文件名
 * @param line 行号
 * @param column 列号
 * @return 错误上下文结构体
 */
Stru_CN_ErrorContext_t CN_error_create_position_context(
    Eum_CN_ErrorCode_t error_code,
    const char* message,
    const char* filename,
    size_t line,
    size_t column);
```

#### 1.3.3 错误上下文操作函数

```c
/**
 * @brief 格式化错误上下文为字符串
 * 
 * @param context 错误上下文
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数（不包括终止符）
 */
size_t CN_error_format_context(
    const Stru_CN_ErrorContext_t* context,
    char* buffer,
    size_t buffer_size);

/**
 * @brief 获取错误上下文的默认消息
 * 
 * @param context 错误上下文
 * @return 默认消息字符串
 */
const char* CN_error_get_default_message(
    const Stru_CN_ErrorContext_t* context);

/**
 * @brief 检查错误上下文是否有效
 * 
 * @param context 错误上下文
 * @return true 如果有效，false 如果无效
 */
bool CN_error_is_context_valid(const Stru_CN_ErrorContext_t* context);

/**
 * @brief 清空错误上下文
 * 
 * @param context 错误上下文
 */
void CN_error_clear_context(Stru_CN_ErrorContext_t* context);

/**
 * @brief 复制错误上下文
 * 
 * @param dest 目标上下文
 * @param src 源上下文
 * @return true 如果成功，false 如果失败
 */
bool CN_error_copy_context(
    Stru_CN_ErrorContext_t* dest,
    const Stru_CN_ErrorContext_t* src);
```

### 1.4 CN_error_chain.h - 错误链

错误链头文件定义错误链结构体和相关操作函数。

#### 1.4.1 错误链结构体

```c
/**
 * @brief 错误链节点结构体
 * 
 * 表示错误链中的一个节点，包含错误上下文和指向下一个节点的指针。
 */
typedef struct Stru_CN_ErrorChainNode_t
{
    /** 错误上下文 */
    Stru_CN_ErrorContext_t context;
    
    /** 下一个错误节点 */
    struct Stru_CN_ErrorChainNode_t* next;
    
    /** 前一个错误节点 */
    struct Stru_CN_ErrorChainNode_t* prev;
    
} Stru_CN_ErrorChainNode_t;

/**
 * @brief 错误链结构体
 * 
 * 管理一系列相关的错误，支持错误链的构建和遍历。
 */
typedef struct Stru_CN_ErrorChain_t
{
    /** 链头节点 */
    Stru_CN_ErrorChainNode_t* head;
    
    /** 链尾节点 */
    Stru_CN_ErrorChainNode_t* tail;
    
    /** 节点数量 */
    size_t count;
    
    /** 最大节点数量（0表示无限制） */
    size_t max_nodes;
    
    /** 是否自动释放节点内存 */
    bool auto_free;
    
} Stru_CN_ErrorChain_t;
```

#### 1.4.2 错误链创建和销毁

```c
/**
 * @brief 创建错误链
 * 
 * @param max_nodes 最大节点数量（0表示无限制）
 * @param auto_free 是否自动释放节点内存
 * @return 错误链结构体
 */
Stru_CN_ErrorChain_t CN_error_chain_create(size_t max_nodes, bool auto_free);

/**
 * @brief 销毁错误链
 * 
 * @param chain 错误链
 */
void CN_error_chain_destroy(Stru_CN_ErrorChain_t* chain);

/**
 * @brief 清空错误链
 * 
 * @param chain 错误链
 */
void CN_error_chain_clear(Stru_CN_ErrorChain_t* chain);
```

#### 1.4.3 错误链操作函数

```c
/**
 * @brief 添加错误到错误链
 * 
 * @param chain 错误链
 * @param context 错误上下文
 * @return true 如果成功添加，false 如果失败
 */
bool CN_error_chain_add(
    Stru_CN_ErrorChain_t* chain,
    const Stru_CN_ErrorContext_t* context);

/**
 * @brief 从错误链创建并添加错误
 * 
 * @param chain 错误链
 * @param error_code 错误码
 * @param message 错误消息
 * @param filename 文件名
 * @param line 行号
 * @param column 列号
 * @param function 函数名
 * @param module 模块名
 * @return true 如果成功添加，false 如果失败
 */
bool CN_error_chain_add_new(
    Stru_CN_ErrorChain_t* chain,
    Eum_CN_ErrorCode_t error_code,
    const char* message,
    const char* filename,
    size_t line,
    size_t column,
    const char* function,
    const char* module);
```

#### 1.4.4 错误链查询函数

```c
/**
 * @brief 获取错误链中的第一个错误
 * 
 * @param chain 错误链
 * @return 第一个错误的上下文，如果链为空则返回NULL
 */
const Stru_CN_ErrorContext_t* CN_error_chain_get_first(
    const Stru_CN_ErrorChain_t* chain);

/**
 * @brief 获取错误链中的最后一个错误
 * 
 * @param chain 错误链
 * @return 最后一个错误的上下文，如果链为空则返回NULL
 */
const Stru_CN_ErrorContext_t* CN_error_chain_get_last(
    const Stru_CN_ErrorChain_t* chain);

/**
 * @brief 获取错误链中的错误数量
 * 
 * @param chain 错误链
 * @return 错误数量
 */
size_t CN_error_chain_get_count(const Stru_CN_ErrorChain_t* chain);

/**
 * @brief 检查错误链是否为空
 * 
 * @param chain 错误链
 * @return true 如果为空，false 如果不为空
 */
bool CN_error_chain_is_empty(const Stru_CN_ErrorChain_t* chain);

/**
 * @brief 检查错误链是否已满
 * 
 * @param chain 错误链
 * @return true 如果已满，false 如果未满
 */
bool CN_error_chain_is_full(const Stru_CN_ErrorChain_t* chain);
```

#### 1.4.5 错误链遍历和查找

```c
/**
 * @brief 遍历错误链
 * 
 * @param chain 错误链
 * @param callback 回调函数，接收错误上下文和用户数据
 * @param user_data 用户数据
 */
void CN_error_chain_foreach(
    const Stru_CN_ErrorChain_t* chain,
    void (*callback)(const Stru_CN_ErrorContext_t* context, void* user_data),
    void* user_data);

/**
 * @brief 查找特定错误码的错误
 * 
 * @param chain 错误链
 * @param error_code 要查找的错误码
 * @return 找到的第一个错误的上下文，如果未找到则返回NULL
 */
const Stru_CN_ErrorContext_t* CN_error_chain_find(
    const Stru_CN_ErrorChain_t* chain,
    Eum_CN_ErrorCode_t error_code);

/**
 * @brief 检查错误链中是否包含特定错误码
 * 
 * @param chain 错误链
 * @param error_code 要检查的错误码
 * @return true 如果包含，false 如果不包含
 */
bool CN_error_chain_contains(
    const Stru_CN_ErrorChain_t* chain,
    Eum_CN_ErrorCode_t error_code);
```

#### 1.4.6 错误链工具函数

```c
/**
 * @brief 合并两个错误链
 * 
 * @param dest 目标错误链
 * @param src 源错误链
 * @return true 如果成功合并，false 如果失败
 */
bool CN_error_chain_merge(
    Stru_CN_ErrorChain_t* dest,
    const Stru_CN_ErrorChain_t* src);

/**
 * @brief 格式化错误链为字符串
 * 
 * @param chain 错误链
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param separator 错误之间的分隔符
 * @return 实际写入的字符数（不包括终止符）
 */
size_t CN_error_chain_format(
    const Stru_CN_ErrorChain_t* chain,
    char* buffer,
    size_t buffer_size,
    const char* separator);

/**
 * @brief 获取错误链中最严重的错误
 * 
 * @param chain 错误链
 * @return 最严重的错误的上下文，如果链为空则返回NULL
 */
const Stru_CN_ErrorContext_t* CN_error_chain_get_most_severe(
    const Stru_CN_ErrorChain_t* chain);

/**
 * @brief 从错误链创建错误摘要
 * 
 * @param chain 错误链
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数（不包括终止符）
 */
size_t CN_error_chain_create_summary(
    const Stru_CN_ErrorChain_t* chain,
    char* buffer,
    size_t buffer_size);
```

## 2. 错误码分类参考

### 2.1 通用错误 (0x0000-0x0FFF)

| 错误码 | 值 | 描述 | 严重性 | 可恢复 |
|--------|-----|------|--------|--------|
| `Eum_CN_ERROR_SUCCESS` | 0x0000 | 成功，无错误 | 0 | 是 |
| `Eum_CN_ERROR_FAILURE` | 0
