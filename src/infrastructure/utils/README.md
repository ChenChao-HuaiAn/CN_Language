# 工具函数库

## 概述

工具函数库是CN_Language项目的基础设施层核心组件，提供字符串处理、数学函数、错误处理和日志记录等通用功能。模块遵循SOLID设计原则，提供安全、高效的函数实现。

## 设计原则

1. **单一职责原则**：每个函数只负责一个明确的功能
2. **安全性原则**：所有函数都包含边界检查和错误处理
3. **一致性原则**：函数命名和参数顺序保持一致
4. **可扩展原则**：通过接口支持功能扩展

## 模块结构

```
src/infrastructure/utils/
├── CN_utils_interface.h      # 工具函数接口定义
├── CN_utils_string.c         # 字符串处理函数实现
├── CN_utils_math_error.c     # 数学函数和错误处理实现
└── README.md                 # 模块文档
```

## 核心功能

### 1. 字符串处理函数

提供安全的字符串操作函数，所有函数都包含边界检查：

```c
// 安全字符串复制
char* cn_strcpy(char* dest, const char* src, size_t max_len);

// 安全字符串连接
char* cn_strcat(char* dest, const char* src, size_t max_len);

// 字符串比较
int cn_strcmp(const char* str1, const char* str2);
int cn_strncmp(const char* str1, const char* str2, size_t n);

// 字符串查找
char* cn_strchr(const char* str, int ch);
char* cn_strstr(const char* haystack, const char* needle);

// 字符串长度
size_t cn_strlen(const char* str);
```

### 2. 内存操作函数

补充内存管理模块的功能：

```c
// 安全内存移动
void* cn_memmove(void* dest, const void* src, size_t size);

// 内存查找
void* cn_memchr(const void* ptr, int value, size_t size);
```

### 3. 数学函数

提供基本的数学运算：

```c
// 绝对值
int cn_abs(int x);
long cn_labs(long x);
long long cn_llabs(long long x);

// 除法运算
int cn_div(int numer, int denom, int* rem);

// 数值操作
int cn_min(int a, int b);
int cn_max(int a, int b);
int cn_clamp(int value, int min, int max);
```

### 4. 错误处理框架

提供统一的错误管理：

```c
// 错误代码枚举
typedef enum Eum_ErrorCode_t {
    Eum_ERROR_NONE = 0,
    Eum_ERROR_INVALID_ARGUMENT,
    Eum_ERROR_OUT_OF_MEMORY,
    // ... 更多错误代码
} Eum_ErrorCode_t;

// 错误信息结构
typedef struct Stru_ErrorInfo_t {
    Eum_ErrorCode_t code;
    const char* message;
    const char* file;
    int line;
    struct Stru_ErrorInfo_t* cause;
} Stru_ErrorInfo_t;

// 错误操作函数
void cn_set_error(Eum_ErrorCode_t code, const char* message,
                  const char* file, int line, Stru_ErrorInfo_t* cause);
Stru_ErrorInfo_t* cn_get_error(void);
void cn_clear_error(void);
```

### 5. 日志系统

提供灵活的日志记录功能：

```c
// 日志级别
typedef enum Eum_LogLevel_t {
    Eum_LOG_LEVEL_TRACE = 0,
    Eum_LOG_LEVEL_DEBUG,
    Eum_LOG_LEVEL_INFO,
    Eum_LOG_LEVEL_WARNING,
    Eum_LOG_LEVEL_ERROR,
    Eum_LOG_LEVEL_FATAL
} Eum_LogLevel_t;

// 日志函数
void cn_log(Eum_LogLevel_t level, const char* file, int line,
            const char* format, ...);

// 日志配置
void cn_set_log_level(Eum_LogLevel_t level);
void cn_set_log_callback(void (*callback)(Eum_LogLevel_t level,
                                          const char* file, int line,
                                          const char* message));
```

### 6. 断言和检查宏

提供开发辅助宏：

```c
// 断言（仅在调试模式下有效）
CN_ASSERT(condition);

// 条件检查并设置错误
CN_CHECK(condition, error_code, error_message);
CN_CHECK_RETURN(condition, error_code, error_message, return_value);
```

## 使用示例

### 字符串处理

```c
#include "CN_utils_interface.h"

// 安全字符串复制
char buffer[100];
const char* source = "Hello, World!";

if (cn_strcpy(buffer, source, sizeof(buffer)) != NULL) {
    printf("Copied: %s\n", buffer);
}

// 安全字符串连接
char path[256] = "/home/user/";
const char* filename = "document.txt";

if (cn_strcat(path, filename, sizeof(path)) != NULL) {
    printf("Full path: %s\n", path);
}

// 字符串查找
const char* text = "The quick brown fox jumps over the lazy dog";
char* found = cn_strstr(text, "fox");
if (found != NULL) {
    printf("Found 'fox' at position: %ld\n", found - text);
}
```

### 错误处理

```c
#include "CN_utils_interface.h"

bool process_file(const char* filename) {
    // 检查参数
    CN_CHECK(filename != NULL, Eum_ERROR_INVALID_ARGUMENT,
             "Filename cannot be NULL");
    
    // 尝试打开文件
    FILE* file = fopen(filename, "r");
    CN_CHECK_RETURN(file != NULL, Eum_ERROR_FILE_NOT_FOUND,
                   "Failed to open file", false);
    
    // 处理文件
    // ...
    
    fclose(file);
    return true;
}

int main() {
    if (!process_file("data.txt")) {
        Stru_ErrorInfo_t* error = cn_get_error();
        if (error != NULL) {
            fprintf(stderr, "Error %d: %s at %s:%d\n",
                   error->code, error->message,
                   error->file, error->line);
            cn_clear_error();
        }
        return 1;
    }
    
    return 0;
}
```

### 日志记录

```c
#include "CN_utils_interface.h"

// 设置日志级别
cn_set_log_level(Eum_LOG_LEVEL_DEBUG);

// 记录日志
cn_log(Eum_LOG_LEVEL_INFO, __FILE__, __LINE__,
       "Application started, version: %s", "1.0.0");

cn_log(Eum_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
       "Processing item %d of %d", 5, 100);

// 自定义日志回调
void my_log_callback(Eum_LogLevel_t level,
                     const char* file, int line,
                     const char* message) {
    // 将日志写入文件或发送到远程服务器
    FILE* log_file = fopen("app.log", "a");
    if (log_file != NULL) {
        fprintf(log_file, "[%d] %s:%d: %s\n",
                level, file, line, message);
        fclose(log_file);
    }
}

// 设置自定义日志回调
cn_set_log_callback(my_log_callback);
```

### 数学函数

```c
#include "CN_utils_interface.h"

// 绝对值
int negative = -42;
int positive = cn_abs(negative);  // 42

// 最小值和最大值
int a = 10, b = 20;
int min_val = cn_min(a, b);  // 10
int max_val = cn_max(a, b);  // 20

// 限制值在范围内
int value = 150;
int clamped = cn_clamp(value, 0, 100);  // 100

// 除法运算
int quotient, remainder;
quotient = cn_div(17, 5, &remainder);  // quotient=3, remainder=2
```

## 线程安全

- 错误处理函数使用线程局部存储，每个线程有独立的错误状态
- 字符串和数学函数是线程安全的（纯函数）
- 日志系统配置是全局的，但日志记录函数本身是线程安全的

## 性能考虑

1. **字符串函数**：包含边界检查，性能略低于标准C库函数
2. **错误处理**：错误信息动态分配，频繁错误可能影响性能
3. **日志系统**：日志级别检查减少不必要的格式化开销

## 错误处理最佳实践

1. **及时清理错误**：使用`cn_clear_error()`清除不再需要的错误
2. **错误链**：使用`cause`参数创建错误链，保留完整的错误上下文
3. **资源管理**：错误发生时确保释放已分配的资源

```c
bool complex_operation() {
    void* resource1 = NULL;
    void* resource2 = NULL;
    
    resource1 = cn_malloc(1024);
    if (resource1 == NULL) {
        cn_set_error(Eum_ERROR_OUT_OF_MEMORY,
                    "Failed to allocate resource1",
                    __FILE__, __LINE__, NULL);
        return false;
    }
    
    resource2 = cn_malloc(2048);
    if (resource2 == NULL) {
        cn_set_error(Eum_ERROR_OUT_OF_MEMORY,
                    "Failed to allocate resource2",
                    __FILE__, __LINE__, cn_get_error());
        cn_free(resource1);  // 清理已分配的资源
        return false;
    }
    
    // 使用资源
    // ...
    
    // 清理资源
    cn_free(resource1);
    cn_free(resource2);
    
    return true;
}
```

## 扩展指南

要添加新的工具函数：

1. 在`CN_utils_interface.h`中声明函数
2. 在相应的实现文件中实现函数
3. 添加单元测试
4. 更新文档

示例：添加字符串分割函数

```c
// 在接口文件中声明
char** cn_strsplit(const char* str, char delimiter, size_t* count);

// 在实现文件中实现
char** cn_strsplit(const char* str, char delimiter, size_t* count) {
    // 实现字符串分割
}
```

## 相关文档

- [API文档](../../docs/api/infrastructure/utils/API.md)
- [设计文档](../../docs/design/infrastructure/utils/DESIGN.md)
- [测试文档](../../tests/unit/infrastructure/utils/README.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现字符串处理、数学函数、错误处理和日志系统 |

## 维护者

CN_Language架构委员会
