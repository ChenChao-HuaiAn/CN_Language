# 调试工具模块

## 概述

调试工具模块提供了各种内存调试工具函数，包括内存转储、调用栈跟踪、内存验证等实用功能。这些工具函数可以独立使用，也可以与内存调试器集成使用。

## 功能特性

### 1. 内存转储
- **格式化转储**: 以十六进制和ASCII格式转储内存内容
- **可配置选项**: 控制转储的详细程度和格式
- **字符串输出**: 支持将转储结果输出到字符串缓冲区

### 2. 调用栈跟踪
- **栈帧获取**: 获取当前调用栈的地址信息
- **符号解析**: 支持函数名、文件名、行号解析
- **格式化输出**: 生成格式化的调用栈跟踪信息

### 3. 内存验证
- **完整性检查**: 验证内存块的完整性
- **模式检查**: 检查内存是否匹配指定模式
- **校验和计算**: 计算内存区域的校验和

### 4. 内存操作
- **模式填充**: 用指定模式填充内存区域
- **内存比较**: 比较两个内存区域的内容
- **模式查找**: 在内存中查找指定模式

### 5. 调试输出
- **回调机制**: 支持自定义调试输出回调
- **文件输出**: 支持将调试输出重定向到文件
- **详细控制**: 控制调试输出的详细程度

## 接口说明

### 数据结构

#### 内存转储选项
```c
typedef struct Stru_MemoryDumpOptions_t
{
    bool show_address;           // 显示地址
    bool show_hex;               // 显示十六进制
    bool show_ascii;             // 显示ASCII字符
    bool show_offset;            // 显示偏移量
    size_t bytes_per_line;       // 每行字节数
    size_t max_bytes;            // 最大转储字节数
    const char* label;           // 转储标签
} Stru_MemoryDumpOptions_t;
```

#### 调用栈跟踪选项
```c
typedef struct Stru_StackTraceOptions_t
{
    size_t max_depth;            // 最大跟踪深度
    bool show_function_names;    // 显示函数名
    bool show_file_names;        // 显示文件名
    bool show_line_numbers;      // 显示行号
    bool show_addresses;         // 显示地址
} Stru_StackTraceOptions_t;
```

### 主要函数

#### 1. 初始化与清理
```c
bool F_initialize_debug_tools(void);
void F_cleanup_debug_tools(void);
```

#### 2. 调试输出控制
```c
void F_set_debug_output_callback(DebugOutputCallback callback);
void F_enable_verbose_debugging(bool enable);
bool F_set_debug_output_file(const char* filename);
```

#### 3. 内存转储
```c
void F_dump_memory(const void* address, size_t size, 
                  const Stru_MemoryDumpOptions_t* options);
size_t F_dump_memory_to_string(const void* address, size_t size,
                              const Stru_MemoryDumpOptions_t* options,
                              char* buffer, size_t buffer_size);
```

#### 4. 调用栈跟踪
```c
size_t F_get_stack_trace(const Stru_StackTraceOptions_t* options,
                        char* buffer, size_t buffer_size);
size_t F_get_stack_trace_addresses(void** addresses, size_t max_depth);
```

#### 5. 内存验证
```c
bool F_validate_memory_block(const void* address, size_t size,
                            const void* pattern, size_t pattern_size);
uint32_t F_calculate_memory_checksum(const void* address, size_t size);
```

#### 6. 内存操作
```c
void F_fill_memory_pattern(void* address, size_t size,
                          const void* pattern, size_t pattern_size);
bool F_check_memory_pattern(const void* address, size_t size,
                           const void* pattern, size_t pattern_size);
int F_compare_memory_regions(const void* addr1, const void* addr2, size_t size);
void* F_find_memory_pattern(const void* address, size_t size,
                           const void* pattern, size_t pattern_size);
```

#### 7. 内存信息
```c
size_t F_get_memory_info_string(const void* address,
                               char* buffer, size_t buffer_size);
```

## 使用示例

### 基本使用
```c
#include "CN_memory_debug_tools.h"
#include <stdio.h>

// 调试输出回调函数
void my_debug_output(const char* message) {
    printf("[DEBUG] %s\n", message);
}

int main() {
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        return 1;
    }
    
    // 设置调试输出回调
    F_set_debug_output_callback(my_debug_output);
    
    // 分配测试内存
    char buffer[256];
    for (int i = 0; i < sizeof(buffer); i++) {
        buffer[i] = i % 256;
    }
    
    // 配置内存转储选项
    Stru_MemoryDumpOptions_t dump_options = {
        .show_address = true,
        .show_hex = true,
        .show_ascii = true,
        .show_offset = true,
        .bytes_per_line = 16,
        .max_bytes = 64,
        .label = "Test Buffer"
    };
    
    // 转储内存
    printf("Memory dump:\n");
    F_dump_memory(buffer, sizeof(buffer), &dump_options);
    
    // 获取调用栈跟踪
    char stack_trace[1024];
    Stru_StackTraceOptions_t stack_options = {
        .max_depth = 10,
        .show_function_names = true,
        .show_file_names = true,
        .show_line_numbers = true,
        .show_addresses = false
    };
    
    size_t trace_len = F_get_stack_trace(&stack_options, stack_trace, sizeof(stack_trace));
    if (trace_len > 0) {
        printf("\nStack trace:\n%s\n", stack_trace);
    }
    
    // 验证内存完整性
    uint8_t pattern = 0xAA;
    bool valid = F_validate_memory_block(buffer, sizeof(buffer), &pattern, 1);
    printf("Memory validation: %s\n", valid ? "PASS" : "FAIL");
    
    // 计算校验和
    uint32_t checksum = F_calculate_memory_checksum(buffer, sizeof(buffer));
    printf("Memory checksum: 0x%08X\n", checksum);
    
    // 清理
    F_cleanup_debug_tools();
    
    return 0;
}
```

### 集成到调试器
```c
#include "CN_memory_debug_tools.h"
#include "CN_memory_debug.h"

// 自定义调试输出函数
void debugger_output_func(const char* message) {
    // 使用调试工具的输出功能
    char formatted[1024];
    snprintf(formatted, sizeof(formatted), "[DEBUGGER] %s", message);
    
    // 可以同时输出到控制台和文件
    printf("%s\n", formatted);
    
    // 或者使用调试工具的文件输出
    static bool file_initialized = false;
    if (!file_initialized) {
        F_set_debug_output_file("debug_log.txt");
        file_initialized = true;
    }
}

// 扩展调试器功能
void enhanced_memory_dump(Stru_MemoryDebuggerInterface_t* debugger, 
                         const void* address, size_t size) {
    Stru_MemoryDumpOptions_t options = {
        .show_address = true,
        .show_hex = true,
        .show_ascii = true,
        .bytes_per_line = 16,
        .max_bytes = size,
        .label = "Enhanced Dump"
    };
    
    char dump_buffer[4096];
    size_t dump_len = F_dump_memory_to_string(address, size, &options, 
                                             dump_buffer, sizeof(dump_buffer));
    
    if (dump_len > 0) {
        debugger->set_debug_output(debugger, debugger_output_func);
        // 输出转储结果
    }
}
```

## 实现说明

### 1. 平台兼容性
- 调用栈跟踪使用平台特定的API
- 内存操作使用标准C库函数
- 提供平台抽象层

### 2. 性能考虑
- 内存转储有性能开销
- 调用栈跟踪影响性能
- 生产环境建议禁用

### 3. 可配置性
- 通过选项结构控制功能
- 支持运行时配置
- 提供默认配置

## 注意事项

1. **性能影响**: 调试工具函数有性能开销，生产环境应谨慎使用
2. **平台依赖**: 调用栈跟踪功能依赖平台支持
3. **内存使用**: 某些函数需要临时缓冲区
4. **线程安全**: 当前实现假设单线程环境

## 扩展指南

### 1. 添加新工具
1. 在头文件中声明函数
2. 实现函数功能
3. 添加测试用例
4. 更新文档

### 2. 平台适配
1. 识别平台差异
2. 实现平台特定代码
3. 测试平台兼容性
4. 更新平台文档

### 3. 性能优化
1. 分析性能瓶颈
2. 优化关键路径
3. 添加性能测试
4. 验证功能正确性

## 相关模块

### 1. 依赖模块
- C标准库
- 平台特定库（调用栈跟踪）

### 2. 集成模块
- 内存调试器模块
- 错误处理模块
- 日志记录模块

---

*最后更新: 2026-01-08*
*版本: 2.0.0*
*维护者: CN_Language架构委员会*
