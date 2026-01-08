# 内存调试工具模块 API 文档

## 概述

内存调试工具模块是内存调试系统的重要组成部分，提供了各种内存调试和诊断工具函数。该模块采用模块化设计，遵循单一职责原则，将功能分解为多个专注的子模块。

## 模块架构

### 模块化设计
内存调试工具模块采用分层模块化设计，每个子模块专注于特定功能领域：

```
src/infrastructure/memory/debug/tools/
├── CN_memory_debug_tools.h              # 主头文件（向后兼容）
├── CN_memory_debug_tools.c              # 主实现文件（薄包装层）
├── README.md                            # 主模块文档
├── core/                                # 核心管理模块
│   ├── CN_memory_debug_tools_core.h/.c
│   └── README.md
├── dump/                                # 内存转储模块
│   ├── CN_memory_debug_tools_dump.h/.c
│   └── README.md
├── validation/                          # 内存验证模块
│   ├── CN_memory_debug_tools_validation.h/.c
│   └── README.md
├── stacktrace/                          # 调用栈跟踪模块
│   ├── CN_memory_debug_tools_stacktrace.h/.c
│   └── README.md
└── utils/                               # 工具函数模块
    ├── CN_memory_debug_tools_utils.h/.c
    └── README.md
```

### 设计原则
1. **单一职责原则**: 每个子模块只负责一个功能领域
2. **接口隔离原则**: 为不同客户端提供专用接口
3. **依赖倒置原则**: 高层模块定义接口，低层模块实现接口
4. **开闭原则**: 对扩展开放，对修改封闭

## 核心管理模块 API

### 数据结构

#### DebugOutputCallback
调试输出回调函数类型。

**原型：**
```c
typedef void (*DebugOutputCallback)(const char* message);
```

**描述：** 用于自定义调试输出的回调函数类型。

### 主要函数

#### F_initialize_debug_tools
初始化调试工具模块。

**原型：**
```c
bool F_initialize_debug_tools(void);
```

**参数：** 无

**返回值：** 初始化成功返回true，失败返回false

**描述：** 初始化调试工具模块，必须在调用其他调试工具函数之前调用。分配全局状态结构，设置默认配置。

#### F_cleanup_debug_tools
清理调试工具模块。

**原型：**
```c
void F_cleanup_debug_tools(void);
```

**参数：** 无

**返回值：** 无

**描述：** 清理调试工具模块，释放所有资源。应在应用程序退出时调用。

#### F_check_debug_tools_initialized
检查调试工具是否已初始化。

**原型：**
```c
bool F_check_debug_tools_initialized(void);
```

**参数：** 无

**返回值：** 已初始化返回true，未初始化返回false

**描述：** 检查调试工具模块的初始化状态。

#### F_is_verbose_debugging_enabled
检查是否启用了详细调试模式。

**原型：**
```c
bool F_is_verbose_debugging_enabled(void);
```

**参数：** 无

**返回值：** 启用详细模式返回true，否则返回false

**描述：** 检查详细调试模式的启用状态。

#### F_set_debug_output_callback
设置调试输出回调函数。

**原型：**
```c
void F_set_debug_output_callback(DebugOutputCallback callback);
```

**参数：**
- `callback`: 调试输出回调函数指针，如果为NULL则使用默认输出

**返回值：** 无

**描述：** 设置自定义调试输出回调函数。默认输出到标准错误。

#### F_enable_verbose_debugging
启用或禁用详细调试模式。

**原型：**
```c
void F_enable_verbose_debugging(bool enable);
```

**参数：**
- `enable`: true启用详细模式，false禁用详细模式

**返回值：** 无

**描述：** 控制调试输出的详细程度。启用后会产生更多调试信息。

#### F_set_debug_output_file
设置调试输出文件。

**原型：**
```c
bool F_set_debug_output_file(const char* filename);
```

**参数：**
- `filename`: 输出文件名，如果为NULL则关闭文件输出

**返回值：** 设置成功返回true，失败返回false

**描述：** 将调试输出重定向到指定文件。如果文件已存在，会追加到文件末尾。

## 内存转储模块 API

### 数据结构

#### Stru_MemoryDumpOptions_t
内存转储选项结构体。

**定义：**
```c
typedef struct Stru_MemoryDumpOptions_t
{
    bool show_address;           // 是否显示地址（默认true）
    bool show_hex;               // 是否显示十六进制（默认true）
    bool show_ascii;             // 是否显示ASCII字符（默认true）
    bool show_offset;            // 是否显示偏移量（默认true）
    size_t bytes_per_line;       // 每行字节数（默认16）
    size_t max_bytes;            // 最大转储字节数（0表示无限制，默认0）
    const char* label;           // 转储标签（可选，默认NULL）
} Stru_MemoryDumpOptions_t;
```

### 主要函数

#### F_dump_memory
转储内存内容到调试输出。

**原型：**
```c
void F_dump_memory(const void* address, size_t size, 
                  const Stru_MemoryDumpOptions_t* options);
```

**参数：**
- `address`: 内存起始地址
- `size`: 内存大小（字节）
- `options`: 转储选项，如果为NULL则使用默认选项

**返回值：** 无

**描述：** 将内存内容以十六进制和ASCII格式转储到调试输出系统。输出可以通过调试输出回调或文件进行重定向。

#### F_dump_memory_to_string
将内存转储到字符串缓冲区。

**原型：**
```c
size_t F_dump_memory_to_string(const void* address, size_t size,
                              const Stru_MemoryDumpOptions_t* options,
                              char* buffer, size_t buffer_size);
```

**参数：**
- `address`: 内存起始地址
- `size`: 内存大小（字节）
- `options`: 转储选项，如果为NULL则使用默认选项
- `buffer`: 输出缓冲区
- `buffer_size`: 缓冲区大小

**返回值：** 实际写入的字符数（不包括终止空字符）

**描述：** 将内存转储结果保存到字符串缓冲区。如果缓冲区不足，会截断输出但保证以空字符结尾。

#### F_get_default_dump_options
获取默认的内存转储选项。

**原型：**
```c
Stru_MemoryDumpOptions_t F_get_default_dump_options(void);
```

**参数：** 无

**返回值：** 默认的内存转储选项结构

**描述：** 返回包含合理默认值的转储选项结构，可以直接使用或修改。

## 内存验证模块 API

### 主要函数

#### F_validate_memory_block
验证内存块的完整性。

**原型：**
```c
bool F_validate_memory_block(const void* address, size_t size,
                            const void* pattern, size_t pattern_size);
```

**参数：**
- `address`: 内存起始地址
- `size`: 内存大小（字节）
- `pattern`: 验证模式，如果为NULL则不进行模式检查
- `pattern_size`: 模式大小（字节）

**返回值：** 验证通过返回true，失败返回false

**描述：** 验证内存块的可访问性和完整性。可以选择检查内存是否匹配指定模式。

#### F_fill_memory_pattern
用指定模式填充内存区域。

**原型：**
```c
void F_fill_memory_pattern(void* address, size_t size,
                          const void* pattern, size_t pattern_size);
```

**参数：**
- `address`: 内存起始地址
- `size`: 内存大小（字节）
- `pattern`: 填充模式
- `pattern_size`: 模式大小（字节）

**返回值：** 无

**描述：** 用指定模式重复填充整个内存区域。常用于内存初始化和测试。

#### F_check_memory_pattern
检查内存是否匹配指定模式。

**原型：**
```c
bool F_check_memory_pattern(const void* address, size_t size,
                           const void* pattern, size_t pattern_size);
```

**参数：**
- `address`: 内存起始地址
- `size`: 内存大小（字节）
- `pattern`: 检查模式
- `pattern_size`: 模式大小（字节）

**返回值：** 完全匹配返回true，否则返回false

**描述：** 检查内存区域是否完全匹配指定模式。用于验证内存内容一致性。

#### F_find_memory_pattern
在内存中查找指定模式。

**原型：**
```c
void* F_find_memory_pattern(const void* address, size_t size,
                           const void* pattern, size_t pattern_size);
```

**参数：**
- `address`: 内存起始地址
- `size`: 内存大小（字节）
- `pattern`: 查找模式
- `pattern_size`: 模式大小（字节）

**返回值：** 找到模式返回首次出现的位置，未找到返回NULL

**描述：** 在内存区域中查找指定模式。返回首次匹配的位置指针。

#### F_calculate_memory_checksum
计算内存区域的校验和。

**原型：**
```c
uint32_t F_calculate_memory_checksum(const void* address, size_t size);
```

**参数：**
- `address`: 内存起始地址
- `size`: 内存大小（字节）

**返回值：** 32位校验和值

**描述：** 计算内存区域的简单加法校验和。用于检测内存损坏。

#### F_compare_memory_regions
比较两个内存区域。

**原型：**
```c
int F_compare_memory_regions(const void* addr1, const void* addr2, size_t size);
```

**参数：**
- `addr1`: 第一个内存区域起始地址
- `addr2`: 第二个内存区域起始地址
- `size`: 比较的大小（字节）

**返回值：** 相同返回0，不同返回第一个不同字节的偏移量+1

**描述：** 比较两个内存区域的内容。返回值范围：0表示完全相同，1-size表示第一个不同位置。

## 调用栈跟踪模块 API

### 数据结构

#### Stru_StackTraceOptions_t
调用栈跟踪选项结构体。

**定义：**
```c
typedef struct Stru_StackTraceOptions_t
{
    size_t max_depth;            // 最大跟踪深度（默认20）
    bool show_function_names;    // 是否显示函数名（默认true）
    bool show_file_names;        // 是否显示文件名（默认true）
    bool show_line_numbers;      // 是否显示行号（默认true）
    bool show_addresses;         // 是否显示地址（默认false）
} Stru_StackTraceOptions_t;
```

### 主要函数

#### F_get_stack_trace
获取格式化的调用栈跟踪信息。

**原型：**
```c
size_t F_get_stack_trace(const Stru_StackTraceOptions_t* options,
                        char* buffer, size_t buffer_size);
```

**参数：**
- `options`: 跟踪选项，如果为NULL则使用默认选项
- `buffer`: 输出缓冲区
- `buffer_size`: 缓冲区大小

**返回值：** 实际写入的字符数（不包括终止空字符）

**描述：** 获取当前调用栈的格式化信息。如果缓冲区不足，会截断输出但保证以空字符结尾。

#### F_get_stack_trace_addresses
获取调用栈地址序列。

**原型：**
```c
size_t F_get_stack_trace_addresses(void** addresses, size_t max_depth);
```

**参数：**
- `addresses`: 地址数组
- `max_depth`: 地址数组的最大容量

**返回值：** 实际获取的地址数量

**描述：** 获取当前调用栈的地址序列，不进行符号解析。返回的地址数量不超过max_depth。

#### F_get_default_stacktrace_options
获取默认的调用栈跟踪选项。

**原型：**
```c
Stru_StackTraceOptions_t F_get_default_stacktrace_options(void);
```

**参数：** 无

**返回值：** 默认的调用栈跟踪选项结构

**描述：** 返回包含合理默认值的调用栈跟踪选项结构，可以直接使用或修改。

## 工具函数模块 API

### 主要函数

#### F_byte_to_hex
将单个字节转换为两位十六进制字符串。

**原型：**
```c
void F_byte_to_hex(uint8_t byte, char* buffer);
```

**参数：**
- `byte`: 要转换的字节
- `buffer`: 输出缓冲区（至少3字节，包含终止空字符）

**返回值：** 无

**描述：** 将字节转换为两位十六进制字符串，使用小写字母。输出格式如"ab"、"0f"等。

#### F_is_printable_char
检查字符是否为可打印ASCII字符。

**原型：**
```c
bool F_is_printable_char(char ch);
```

**参数：**
- `ch`: 要检查的字符

**返回值：** 可打印返回true，否则返回false

**描述：** 检查字符是否为可打印ASCII字符（范围：0x20-0x7E，即空格到波浪号）。

#### F_get_memory_info_string
获取内存信息的格式化字符串。

**原型：**
```c
size_t F_get_memory_info_string(const void* address,
                               char* buffer, size_t buffer_size);
```

**参数：**
- `address`: 内存地址
- `buffer`: 输出缓冲区
- `buffer_size`: 缓冲区大小

**返回值：** 实际写入的字符数（不包括终止空字符）

**描述：** 生成格式化的内存信息描述字符串，格式如"Address: 0xXXXX, Size: YYYY bytes"。

#### F_format_memory_address
格式化内存地址为字符串。

**原型：**
```c
size_t F_format_memory_address(const void* address,
                              char* buffer, size_t buffer_size);
```

**参数：**
- `address`: 内存地址
- `buffer`: 输出缓冲区
- `buffer_size`: 缓冲区大小

**返回值：** 实际写入的字符数（不包括终止空字符）

**描述：** 将内存地址格式化为标准字符串表示，格式如"0xXXXXXXXX"（32位）或"0xXXXXXXXXXXXXXXXX"（64位）。

#### F_check_memory_alignment
检查内存地址是否符合对齐要求。

**原型：**
```c
bool F_check_memory_alignment(const void* address, size_t alignment);
```

**参数：**
- `address`: 内存地址
- `alignment`: 对齐要求（必须是2的幂）

**返回值：** 对齐返回true，否则返回false

**描述：** 检查内存地址是否符合指定的对齐要求。对齐要求必须是2的幂（如4、8、16等）。

## 使用示例

### 示例1：基本内存调试
```c
#include "CN_memory_debug_tools.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        fprintf(stderr, "Failed to initialize debug tools\n");
        return 1;
    }
    
    // 设置调试输出到文件
    F_set_debug_output_file("debug_log.txt");
    
    // 分配测试内存
    size_t buffer_size = 256;
    uint8_t* buffer = (uint8_t*)malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        F_cleanup_debug_tools();
        return 1;
    }
    
    // 用模式填充内存
    uint8_t pattern[] = {0xDE, 0xAD, 0xBE, 0xEF};
    F_fill_memory_pattern(buffer, buffer_size, pattern, sizeof(pattern));
    
    // 转储内存内容
    Stru_MemoryDumpOptions_t dump_options = {
        .show_address = true,
        .show_hex = true,
        .show_ascii = true,
        .bytes_per_line = 16,
        .max_bytes = 64,
        .label = "Test Buffer"
    };
    
    printf("Memory dump:\n");
    F_dump_memory(buffer, buffer_size, &dump_options);
    
    // 验证内存完整性
    bool valid = F_validate_memory_block(buffer, buffer_size, pattern, sizeof(pattern));
    printf("Memory validation: %s\n", valid ? "PASS" : "FAIL");
    
    // 获取调用栈跟踪
    char stack_trace[1024];
    Stru_StackTraceOptions_t stack_options = {
        .max_depth = 10,
        .show_function_names = true,
        .show_file_names = true,
        .show_line_numbers = true
    };
    
    size_t trace_len = F_get_stack_trace(&stack_options, stack_trace, sizeof(stack_trace));
    if (trace_len > 0) {
        printf("\nStack trace:\n%s\n", stack_trace);
    }
    
    // 清理
    free(buffer);
    F_cleanup_debug_tools();
    
    return 0;
}
```

### 示例2：集成到错误处理
```c
#include "CN_memory_debug_tools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 增强的内存分配函数
void* debug_malloc(size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "Memory allocation failed at %s:%d, size=%zu",
                 file, line, size);
        
        // 记录错误和调用栈
        F_debug_output("%s", error_msg);
        
        char stack_trace[512];
        Stru_StackTraceOptions_t options = {
            .max_depth = 8,
            .show_function_names = true,
            .show_file_names = true,
            .show_line_numbers = true
        };
        
        size_t trace_len = F_get_stack_trace(&options, stack_trace, sizeof(stack_trace));
        if (trace_len > 0) {
            F_debug_output("Stack trace:\n%s", stack_trace);
        }
    }
    
    return ptr;
}

// 增强的内存释放函数
void debug_free(void* ptr, const char* file, int line) {
    if (ptr == NULL) {
        return;
    }
    
    // 在释放前验证内存
    if (F_check_debug_tools_initialized()) {
        // 可以在这里添加内存验证逻辑
        char info_buffer[128];
        size_t info_len = F_format_memory_address(ptr, info_buffer, sizeof(info_buffer));
        F_debug_output("Freeing memory at %s from %s:%d", info_buffer, file, line);
    }
    
    free(ptr);
}

// 使用示例
void example_usage(void) {
    // 确保调试工具已初始化
    if (!F_check_debug_tools_initialized()) {
        F_initialize_debug_tools();
    }
    
    // 启用详细调试
    F_enable_verbose_debugging(true);
    
    // 分配内存
    int* data = (int*)debug_malloc(100 * sizeof(int), __FILE__, __LINE__);
    if (!data) {
        return;
    }
    
    // 使用内存...
    for (int i = 0; i < 100; i++) {
        data[i] = i;
    }
    
    // 转储部分内存
    Stru_MemoryDumpOptions_t dump_opts = {
        .show_address = true,
        .show_hex = true,
        .show_ascii = false,  // 整数数据不需要ASCII
        .bytes_per_line = 16,
        .max_bytes = 32,
        .label = "Integer Array"
    };
    
    F_dump_memory(data, 32, &dump_opts);
    
    // 释放内存
    debug_free(data, __FILE__, __LINE__);
    
    // 清理（可选，通常在程序退出时调用）
    // F_cleanup_debug_tools();
}
