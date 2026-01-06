# 工具函数库 API 文档

## 概述

工具函数库是CN_Language项目的基础设施层核心组件，提供字符串处理、数学函数、错误处理和日志记录等通用工具函数。本模块遵循SOLID设计原则，提供安全、可靠的工具函数。

## 设计原则

1. **单一职责原则**：每个函数只负责一个特定功能
2. **安全第一**：所有字符串函数都包含边界检查，防止缓冲区溢出
3. **线程安全**：错误处理使用线程局部存储，避免竞争条件
4. **可扩展性**：通过回调函数支持自定义日志输出

## 核心功能

### 字符串处理函数

提供安全的字符串操作函数，所有函数都包含边界检查。

### 数学函数

提供基本的数学运算函数。

### 错误处理框架

提供统一的错误管理和传播机制。

### 日志系统

提供灵活的日志记录功能，支持日志级别和自定义回调。

## API 函数

### 字符串处理函数

#### `cn_strcpy`

安全复制字符串。

```c
char* cn_strcpy(char* dest, const char* src, size_t max_len);
```

**参数**：
- `dest`：目标缓冲区
- `src`：源字符串
- `max_len`：最大复制长度（包括终止符）

**返回值**：目标字符串指针，失败返回NULL

**说明**：确保目标字符串以空字符结尾

#### `cn_strcat`

安全连接字符串。

```c
char* cn_strcat(char* dest, const char* src, size_t max_len);
```

**参数**：
- `dest`：目标缓冲区
- `src`：源字符串
- `max_len`：最大长度（包括终止符）

**返回值**：目标字符串指针，失败返回NULL

**说明**：检查目标缓冲区是否有足够空间

#### `cn_strcmp`

比较字符串。

```c
int cn_strcmp(const char* str1, const char* str2);
```

**参数**：
- `str1`：第一个字符串
- `str2`：第二个字符串

**返回值**：比较结果（0表示相等，<0表示str1<str2，>0表示str1>str2）

#### `cn_strncmp`

比较字符串（指定长度）。

```c
int cn_strncmp(const char* str1, const char* str2, size_t n);
```

**参数**：
- `str1`：第一个字符串
- `str2`：第二个字符串
- `n`：比较的最大字符数

**返回值**：比较结果（0表示相等）

#### `cn_strlen`

获取字符串长度。

```c
size_t cn_strlen(const char* str);
```

**参数**：
- `str`：字符串

**返回值**：字符串长度（不包括终止符），NULL返回0

#### `cn_strchr`

查找字符。

```c
char* cn_strchr(const char* str, int ch);
```

**参数**：
- `str`：字符串
- `ch`：要查找的字符

**返回值**：字符位置指针，未找到返回NULL

#### `cn_strstr`

查找子字符串。

```c
char* cn_strstr(const char* haystack, const char* needle);
```

**参数**：
- `haystack`：主字符串
- `needle`：子字符串

**返回值**：子字符串位置指针，未找到返回NULL

#### `cn_strncpy`

复制内存块到字符串。

```c
char* cn_strncpy(char* dest, const char* src, size_t n);
```

**参数**：
- `dest`：目标缓冲区
- `src`：源内存
- `n`：要复制的字节数

**返回值**：目标字符串指针

### 内存操作函数

#### `cn_memmove`

移动内存块。

```c
void* cn_memmove(void* dest, const void* src, size_t size);
```

**参数**：
- `dest`：目标内存
- `src`：源内存
- `size`：要移动的字节数

**返回值**：目标内存指针

**说明**：处理内存重叠的情况

#### `cn_memchr`

查找内存中的字节。

```c
void* cn_memchr(const void* ptr, int value, size_t size);
```

**参数**：
- `ptr`：内存指针
- `value`：要查找的值
- `size`：要搜索的字节数

**返回值**：找到的位置指针，未找到返回NULL

### 数学函数

#### `cn_abs`

绝对值（整数）。

```c
int cn_abs(int x);
```

**参数**：
- `x`：整数值

**返回值**：绝对值

#### `cn_labs`

绝对值（长整数）。

```c
long cn_labs(long x);
```

**参数**：
- `x`：长整数值

**返回值**：绝对值

#### `cn_llabs`

绝对值（长长整数）。

```c
long long cn_llabs(long long x);
```

**参数**：
- `x`：长长整数值

**返回值**：绝对值

#### `cn_div`

除法运算。

```c
int cn_div(int numer, int denom, int* rem);
```

**参数**：
- `numer`：被除数
- `denom`：除数
- `rem`：余数输出参数（可为NULL）

**返回值**：商，除数为0时返回0

#### `cn_min`

最小值。

```c
int cn_min(int a, int b);
```

**参数**：
- `a`：第一个值
- `b`：第二个值

**返回值**：最小值

#### `cn_max`

最大值。

```c
int cn_max(int a, int b);
```

**参数**：
- `a`：第一个值
- `b`：第二个值

**返回值**：最大值

#### `cn_clamp`

限制值在范围内。

```c
int cn_clamp(int value, int min, int max);
```

**参数**：
- `value`：要限制的值
- `min`：最小值
- `max`：最大值

**返回值**：限制后的值

### 错误处理函数

#### `cn_set_error`

设置当前线程的错误信息。

```c
void cn_set_error(Eum_ErrorCode_t code, const char* message,
                  const char* file, int line, Stru_ErrorInfo_t* cause);
```

**参数**：
- `code`：错误代码
- `message`：错误消息
- `file`：文件名
- `line`：行号
- `cause`：原因错误（可为NULL）

#### `cn_get_error`

获取当前线程的错误信息。

```c
Stru_ErrorInfo_t* cn_get_error(void);
```

**返回值**：错误信息指针，无错误返回NULL

#### `cn_clear_error`

清除当前线程的错误信息。

```c
void cn_clear_error(void);
```

#### `cn_create_error`

创建错误信息。

```c
Stru_ErrorInfo_t* cn_create_error(Eum_ErrorCode_t code, const char* message,
                                  const char* file, int line, Stru_ErrorInfo_t* cause);
```

**参数**：
- `code`：错误代码
- `message`：错误消息
- `file`：文件名
- `line`：行号
- `cause`：原因错误（可为NULL）

**返回值**：错误信息指针

#### `cn_destroy_error`

销毁错误信息。

```c
void cn_destroy_error(Stru_ErrorInfo_t* error);
```

**参数**：
- `error`：要销毁的错误信息

### 日志函数

#### `cn_log`

记录日志。

```c
void cn_log(Eum_LogLevel_t level, const char* file, int line,
            const char* format, ...);
```

**参数**：
- `level`：日志级别
- `file`：文件名
- `line`：行号
- `format`：格式字符串
- `...`：可变参数

#### `cn_set_log_level`

设置日志级别。

```c
void cn_set_log_level(Eum_LogLevel_t level);
```

**参数**：
- `level`：最小日志级别（低于此级别的日志将被忽略）

#### `cn_set_log_callback`

设置日志输出回调。

```c
void cn_set_log_callback(void (*callback)(Eum_LogLevel_t level,
                                          const char* file, int line,
                                          const char* message));
```

**参数**：
- `callback`：日志输出回调函数

### 错误代码枚举

```c
typedef enum Eum_ErrorCode_t
{
    Eum_ERROR_NONE = 0,              ///< 无错误
    Eum_ERROR_INVALID_ARGUMENT,      ///< 无效参数
    Eum_ERROR_OUT_OF_MEMORY,         ///< 内存不足
    Eum_ERROR_BUFFER_OVERFLOW,       ///< 缓冲区溢出
    Eum_ERROR_FILE_NOT_FOUND,        ///< 文件未找到
    Eum_ERROR_PERMISSION_DENIED,     ///< 权限被拒绝
    Eum_ERROR_IO_ERROR,              ///< IO错误
    Eum_ERROR_FORMAT_ERROR,          ///< 格式错误
    Eum_ERROR_SYNTAX_ERROR,          ///< 语法错误
    Eum_ERROR_RUNTIME_ERROR,         ///< 运行时错误
    Eum_ERROR_NOT_IMPLEMENTED,       ///< 未实现
    Eum_ERROR_INTERNAL_ERROR         ///< 内部错误
} Eum_ErrorCode_t;
```

### 日志级别枚举

```c
typedef enum Eum_LogLevel_t
{
    Eum_LOG_LEVEL_TRACE = 0,         ///< 跟踪级别（最详细）
    Eum_LOG_LEVEL_DEBUG,             ///< 调试级别
    Eum_LOG_LEVEL_INFO,              ///< 信息级别
    Eum_LOG_LEVEL_WARNING,           ///< 警告级别
    Eum_LOG_LEVEL_ERROR,             ///< 错误级别
    Eum_LOG_LEVEL_FATAL              ///< 致命级别（最严重）
} Eum_LogLevel_t;
```

### 错误信息结构

```c
typedef struct Stru_ErrorInfo_t
{
    Eum_ErrorCode_t code;            ///< 错误代码
    const char* message;             ///< 错误消息
    const char* file;                ///< 发生错误的文件
    int line;                        ///< 发生错误的行号
    struct Stru_ErrorInfo_t* cause;  ///< 原因错误（可为NULL）
} Stru_ErrorInfo_t;
```

### 开发辅助宏

#### `CN_ASSERT`

断言宏（在调试模式下有效）。

```c
#ifdef CN_DEBUG
#define CN_ASSERT(condition) ...
#else
#define CN_ASSERT(condition) ((void)0)
#endif
```

**参数**：
- `condition`：断言条件

#### `CN_CHECK`

检查条件并设置错误。

```c
#define CN_CHECK(condition, error_code, error_message) ...
```

**参数**：
- `condition`：条件
- `error_code`：错误代码（如果条件为假）
- `error_message`：错误消息（如果条件为假）

#### `CN_CHECK_RETURN`

检查条件并设置错误（返回指定值）。

```c
#define CN_CHECK_RETURN(condition, error_code, error_message, return_value) ...
```

**参数**：
- `condition`：条件
- `error_code`：错误代码（如果条件为假）
- `error_message`：错误消息（如果条件为假）
- `return_value`：返回值（如果条件为假）

## 使用示例

### 字符串处理

```c
#include "CN_utils_interface.h"

// 安全字符串复制
char buffer[64];
const char* source = "Hello, World!";
if (cn_strcpy(buffer, source, sizeof(buffer)) != NULL) {
    // 使用buffer
}

// 安全字符串连接
char greeting[32] = "Hello, ";
const char* name = "Alice";
if (cn_strcat(greeting, name, sizeof(greeting)) != NULL) {
    // greeting现在是"Hello, Alice"
}

// 字符串比较
int result = cn_strcmp("apple", "banana");
if (result < 0) {
    // "apple" < "banana"
}
```

### 数学运算

```c
#include "CN_utils_interface.h"

// 绝对值
int abs_value = cn_abs(-42);  // 42

// 最小值和最大值
int min_val = cn_min(10, 20);  // 10
int max_val = cn_max(10, 20);  // 20

// 限制值范围
int clamped = cn_clamp(150, 0, 100);  // 100
```

### 错误处理

```c
#include "CN_utils_interface.h"

// 设置错误
cn_set_error(Eum_ERROR_INVALID_ARGUMENT, 
             "Invalid parameter value",
             __FILE__, __LINE__, NULL);

// 获取错误
Stru_ErrorInfo_t* error = cn_get_error();
if (error != NULL) {
    printf("Error: %s (code: %d)\n", error->message, error->code);
    
    // 清除错误
    cn_clear_error();
}

// 使用CN_CHECK宏
bool process_data(const char* data) {
    CN_CHECK(data != NULL, Eum_ERROR_INVALID_ARGUMENT, "Data cannot be NULL");
    // 处理数据...
    return true;
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

// 设置自定义日志回调
void my_log_callback(Eum_LogLevel_t level, const char* file, 
                     int line, const char* message) {
    // 自定义日志处理
    printf("[MY_LOG] %s:%d: %s\n", file, line, message);
}

cn_set_log_callback(my_log_callback);
```

### 完整示例：安全的字符串处理

```c
#include "CN_utils_interface.h"
#include <stdio.h>

bool process_user_input(const char* input) {
    // 检查输入
    CN_CHECK(input != NULL, Eum_ERROR_INVALID_ARGUMENT, "Input cannot be NULL");
    
    // 安全处理字符串
    char buffer[256];
    if (cn_strcpy(buffer, input, sizeof(buffer)) == NULL) {
        cn_set_error(Eum_ERROR_BUFFER_OVERFLOW,
                    "Input too long for buffer",
                    __FILE__, __LINE__, NULL);
        return false;
    }
    
    // 处理数据
    cn_log(Eum_LOG_LEVEL_INFO, __FILE__, __LINE__,
           "Processing input: %s", buffer);
    
    return true;
}

int main() {
    // 设置日志级别
    cn_set_log_level(Eum_LOG_LEVEL_INFO);
    
    // 测试
    if (!process_user_input("Test input")) {
        Stru_ErrorInfo_t* error = cn_get_error();
        if (error != NULL) {
            cn_log(Eum_LOG_LEVEL_ERROR, error->file, error->line,
                   "Error: %s", error->message);
            cn_clear_error();
        }
    }
    
    return 0;
}
```

## 错误处理最佳实践

1. **及时清理错误**：使用`cn_clear_error()`清除不再需要的错误信息
2. **错误传播**：使用`cause`参数传递底层错误
3. **资源管理**：使用`CN_CHECK`宏确保资源正确释放
4. **日志记录**：结合错误处理和日志记录，便于调试

## 线程安全

- 错误处理使用线程局部存储，每个线程有独立的错误状态
- 字符串和数学函数是线程安全的（无共享状态）
- 日志系统需要外部同步（如果多个线程同时设置回调）

## 性能考虑

1. **边界检查**：安全字符串函数有轻微性能开销
2. **错误处理**：错误信息创建和销毁有内存分配开销
3. **日志记录**：日志级别检查避免不必要的格式化开销

## 扩展指南

要添加新的工具函数：

1. 在`CN_utils_interface.h`中声明函数
2. 在相应的`.c`文件中实现函数
3. 确保函数遵循安全原则（边界检查、NULL检查）
4. 在API文档中记录新函数

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，包含字符串处理、数学函数、错误处理和日志系统 |

## 相关文件

- `src/infrastructure/utils/CN_utils_interface.h` - 接口定义
- `src/infrastructure/utils/CN_utils_string.c` - 字符串处理实现
- `src/infrastructure/utils/CN_utils_math_error.c` - 数学和错误处理实现
- `src/infrastructure/utils/README.md` - 模块文档
