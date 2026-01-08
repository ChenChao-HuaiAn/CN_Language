# 调用栈跟踪模块

## 概述

调用栈跟踪模块提供了获取和分析函数调用栈的功能。该模块可以捕获当前执行位置的调用栈信息，包括函数地址、函数名、文件名和行号等，便于调试和错误分析。

## 功能特性

### 1. 调用栈捕获
- **栈帧获取**: 获取当前调用栈的地址序列
- **深度控制**: 控制跟踪的最大深度
- **平台抽象**: 提供跨平台的调用栈跟踪接口

### 2. 符号解析
- **函数名解析**: 将地址解析为函数名
- **文件名解析**: 获取源代码文件名
- **行号解析**: 获取源代码行号（如果可用）

### 3. 格式化输出
- **可读格式**: 生成易于阅读的调用栈信息
- **可配置格式**: 控制输出包含哪些信息
- **字符串输出**: 支持将调用栈保存到字符串缓冲区

### 4. 调试集成
- **错误追踪**: 记录错误发生时的调用栈
- **性能分析**: 用于性能分析和优化
- **日志增强**: 增强日志信息的可调试性

## 接口说明

### 数据结构

#### 调用栈跟踪选项
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

#### 1. 调用栈跟踪
```c
/**
 * @brief 获取格式化的调用栈跟踪信息
 * @param options 跟踪选项，如果为NULL则使用默认选项
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数（不包括终止空字符）
 * @note 如果缓冲区不足，会截断输出但保证以空字符结尾
 */
size_t F_get_stack_trace(const Stru_StackTraceOptions_t* options,
                        char* buffer, size_t buffer_size);

/**
 * @brief 获取调用栈地址序列
 * @param addresses 地址数组
 * @param max_depth 地址数组的最大容量
 * @return 实际获取的地址数量
 * @note 返回的地址数量不超过max_depth
 */
size_t F_get_stack_trace_addresses(void** addresses, size_t max_depth);
```

#### 2. 配置管理
```c
/**
 * @brief 获取默认的调用栈跟踪选项
 * @return 默认的调用栈跟踪选项结构
 * @note 返回的结构包含合理的默认值，可以直接使用或修改
 */
Stru_StackTraceOptions_t F_get_default_stacktrace_options(void);
```

## 使用示例

### 基本使用
```c
#include "CN_memory_debug_tools.h"
#include <stdio.h>

// 示例函数，用于演示调用栈
void function_c(void) {
    char stack_trace[2048];
    
    // 使用默认选项获取调用栈
    size_t trace_len = F_get_stack_trace(NULL, stack_trace, sizeof(stack_trace));
    
    if (trace_len > 0) {
        printf("Stack trace from function_c:\n%s\n", stack_trace);
    }
}

void function_b(void) {
    function_c();
}

void function_a(void) {
    function_b();
}

int main() {
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        return 1;
    }
    
    printf("Basic stack trace example:\n");
    function_a();
    
    // 自定义选项
    Stru_StackTraceOptions_t custom_options = {
        .max_depth = 10,
        .show_function_names = true,
        .show_file_names = true,
        .show_line_numbers = true,
        .show_addresses = true  // 显示地址
    };
    
    char custom_trace[1024];
    size_t custom_len = F_get_stack_trace(&custom_options, 
                                         custom_trace, sizeof(custom_trace));
    
    printf("\nCustom stack trace (with addresses):\n");
    if (custom_len > 0) {
        printf("%s\n", custom_trace);
    }
    
    // 获取地址序列
    void* addresses[20];
    size_t address_count = F_get_stack_trace_addresses(addresses, 20);
    
    printf("\nRaw addresses (%zu frames):\n", address_count);
    for (size_t i = 0; i < address_count; i++) {
        printf("  Frame %zu: %p\n", i, addresses[i]);
    }
    
    // 使用默认选项函数
    Stru_StackTraceOptions_t default_opts = F_get_default_stacktrace_options();
    default_opts.max_depth = 5;  // 只显示5层
    
    char short_trace[512];
    size_t short_len = F_get_stack_trace(&default_opts, short_trace, sizeof(short_trace));
    
    printf("\nShort stack trace (5 frames):\n");
    if (short_len > 0) {
        printf("%s\n", short_trace);
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    return 0;
}
```

### 错误处理集成
```c
#include "CN_memory_debug_tools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 增强的错误处理函数
void handle_error_with_stacktrace(const char* message, const char* file, int line) {
    char error_buffer[1024];
    char stack_buffer[2048];
    
    // 格式化错误信息
    snprintf(error_buffer, sizeof(error_buffer),
             "ERROR at %s:%d: %s\n", file, line, message);
    
    // 获取调用栈
    Stru_StackTraceOptions_t stack_options = {
        .max_depth = 15,
        .show_function_names = true,
        .show_file_names = true,
        .show_line_numbers = true,
        .show_addresses = false
    };
    
    size_t stack_len = F_get_stack_trace(&stack_options, stack_buffer, sizeof(stack_buffer));
    
    // 输出错误信息和调用栈
    fprintf(stderr, "%s\n", error_buffer);
    if (stack_len > 0) {
        fprintf(stderr, "Stack trace:\n%s\n", stack_buffer);
    }
    
    // 可以记录到日志文件或发送到监控系统
}

// 示例：内存分配错误处理
void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        handle_error_with_stacktrace("Memory allocation failed", __FILE__, __LINE__);
    }
    return ptr;
}

// 示例：使用安全内存分配
void process_data(void) {
    int* data = (int*)safe_malloc(100 * sizeof(int));
    if (data == NULL) {
        return;  // 错误已处理
    }
    
    // 使用数据...
    
    free(data);
}
```

### 性能分析集成
```c
#include "CN_memory_debug_tools.h"
#include <stdio.h>
#include <time.h>

// 性能分析结构
typedef struct Stru_PerformanceProfile_t
{
    clock_t start_time;
    clock_t end_time;
    char stack_trace[1024];
} Stru_PerformanceProfile_t;

// 开始性能分析
void start_profiling(Stru_PerformanceProfile_t* profile) {
    profile->start_time = clock();
    
    // 记录开始时的调用栈
    Stru_StackTraceOptions_t options = {
        .max_depth = 8,
        .show_function_names = true,
        .show_file_names = false,  // 性能分析通常不需要文件名
        .show_line_numbers = false,
        .show_addresses = false
    };
    
    F_get_stack_trace(&options, profile->stack_trace, sizeof(profile->stack_trace));
}

// 结束性能分析并报告
void end_profiling(Stru_PerformanceProfile_t* profile, const char* operation_name) {
    profile->end_time = clock();
    
    double elapsed_ms = (double)(profile->end_time - profile->start_time) * 1000.0 / CLOCKS_PER_SEC;
    
    printf("Performance profile for '%s':\n", operation_name);
    printf("  Time: %.2f ms\n", elapsed_ms);
    printf("  Call stack:\n%s\n", profile->stack_trace);
}

// 示例：分析函数性能
void expensive_operation(void) {
    Stru_PerformanceProfile_t profile;
    start_profiling(&profile);
    
    // 模拟耗时操作
    volatile int sum = 0;
    for (int i = 0; i < 1000000; i++) {
        sum += i;
    }
    
    end_profiling(&profile, "expensive_operation");
}
```

## 输出格式

### 默认输出格式示例
```
Stack trace (10 frames):
  #0 function_c at example.c:15
  #1 function_b at example.c:20
  #2 function_a at example.c:25
  #3 main at example.c:30
  #4 __libc_start_main at libc-start.c:308
  #5 _start at ??:0
```

### 带地址的输出格式
```
Stack trace (5 frames):
  #0 function_c (0x55a1b2c3d4e5) at example.c:15
  #1 function_b (0x55a1b2c3d510) at example.c:20
  #2 function_a (0x55a1b2c3d540) at example.c:25
  #3 main (0x55a1b2c3d570) at example.c:30
```

### 格式说明
1. **帧编号**: 从0开始的调用栈帧编号
2. **函数名**: 解析出的函数名（如果可用）
3. **地址**: 可选的函数地址（十六进制）
4. **文件名**: 源代码文件名（如果可用）
5. **行号**: 源代码行号（如果可用）

## 实现细节

### 1. 平台抽象层
1. **Linux/Unix**: 使用`backtrace()`和`backtrace_symbols()`
2. **Windows**: 使用`CaptureStackBackTrace()`和`SymFromAddr()`
3. **其他平台**: 提供最小实现或返回简化信息

### 2. 符号解析流程
1. 获取原始地址序列
2. 尝试解析每个地址的函数名
3. 尝试获取文件名和行号信息
4. 格式化输出结果

### 3. 性能优化
- 缓存符号解析结果
- 批量处理地址解析
- 提供轻量级地址获取函数

### 4. 错误处理
- 处理地址解析失败
- 处理缓冲区不足
- 处理平台不支持的情况

## 平台支持

### 1. 完全支持
- **Linux/glibc**: 完整的符号解析支持
- **macOS**: 通过`backtrace()`支持
- **Windows**: 通过DbgHelp库支持

### 2. 部分支持
- **嵌入式系统**: 可能只支持地址获取
- **无调试信息**: 只能显示地址，无法解析符号
- **特定架构**: 某些架构可能有限制

### 3. 编译要求
- **Linux**: 需要链接`-ldl`和`-rdynamic`选项
- **Windows**: 需要链接`DbgHelp.lib`
- **通用**: 建议包含调试信息（`-g`选项）

## 性能考虑

### 1. 跟踪开销
- 获取调用栈有显著性能开销
- 符号解析可能较慢（特别是第一次）
- 格式化输出需要字符串处理

### 2. 优化建议
- 生产环境谨慎使用调用栈跟踪
- 使用地址获取而非完整符号解析
- 限制跟踪深度和频率
- 缓存频繁使用的符号信息

### 3. 内存使用
- 符号解析可能使用动态内存
- 格式化输出需要缓冲区
- 平台库可能有内部缓存

## 扩展指南

### 1. 添加新平台支持
1. 实现平台特定的地址获取函数
2. 实现平台特定的符号解析函数
3. 添加平台检测和分发逻辑
4. 更新平台支持文档

### 2. 增强符号解析
1. 添加DWARF调试信息支持
2. 支持PDB文件（Windows）
3. 添加动态库符号解析
4. 支持内联函数展开

### 3. 添加过滤功能
1. 添加帧过滤（跳过库函数）
2. 添加模块过滤（只显示特定模块）
3. 添加深度动态调整
4. 添加时间戳集成

## 相关模块

### 1. 直接依赖
- 调试工具核心模块（用于输出和状态管理）
- 平台特定库（backtrace、DbgHelp等）

### 2. 被依赖模块
- 错误处理模块（用于错误追踪）
- 日志记录模块（用于增强日志）
- 性能分析模块（用于调用分析）

### 3. 协同模块
- 内存转储模块（结合内存状态）
- 内存验证模块（记录验证失败位置）

## 注意事项

1. **性能影响**: 调用栈跟踪显著影响性能，生产环境慎用
2. **符号可用性**: 需要编译时包含调试信息（`-g`）
3. **平台差异**: 不同平台的支持程度和输出格式不同
4. **异步安全**: 某些平台函数不是异步信号安全的

## 测试要点

### 1. 功能测试
- 测试各种跟踪深度
- 测试所有选项组合
- 测试字符串输出功能
- 测试地址获取函数

### 2. 平台测试
- 测试不同平台的兼容性
- 测试有无调试信息的情况
- 测试不同编译选项的影响
- 测试异常情况处理

### 3. 性能测试
- 测试调用栈获取性能
- 测试符号解析性能
- 测试不同深度的影响
- 测试内存使用情况

### 4. 集成测试
- 测试与错误处理的集成
- 测试在真实调试场景中的使用
- 测试与其他调试工具的协同工作
- 测试边界情况和错误恢复

---

*最后更新: 2026-01-08*
*版本: 2.0.0*
*维护者: CN_Language架构委员会*
