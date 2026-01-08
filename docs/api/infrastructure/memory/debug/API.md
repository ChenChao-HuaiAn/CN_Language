# 内存调试模块 API 文档

## 概述

本文档描述了内存调试模块的API接口。内存调试模块提供了全面的内存调试和诊断功能，包括内存泄漏检测、错误检测、使用分析和调试工具。

## 模块结构

### 1. 主接口 (`CN_memory_debug.h`)
主头文件，包含所有子模块的接口。

### 2. 抽象接口 (`interface/CN_memory_debug_interface.h`)
定义了内存调试器的抽象接口，遵循SOLID设计原则。

### 3. 工厂函数 (`interface/CN_memory_debug_factory.h`)
提供了创建和销毁内存调试器实例的工厂函数。

### 4. 泄漏检测 (`leak_detection/CN_memory_leak_detection.h`)
专门处理内存泄漏检测的功能。

### 5. 错误检测 (`error_detection/CN_memory_error_detection.h`)
专门处理内存错误检测的功能。

### 6. 分析功能 (`analysis/CN_memory_analysis.h`)
专门处理内存使用分析的功能。

### 7. 调试工具 (`tools/CN_memory_debug_tools.h`)
提供各种内存调试工具。

## API 参考

### 抽象接口 API

#### Stru_MemoryDebuggerInterface_t
内存调试器抽象接口结构体。

**成员函数：**

| 函数 | 描述 | 参数 | 返回值 |
|------|------|------|--------|
| `initialize` | 初始化内存调试器 | `debugger`: 调试器实例 | `bool`: 成功返回true |
| `cleanup` | 清理内存调试器资源 | `debugger`: 调试器实例 | `void` |
| `enable_monitoring` | 启用或禁用内存监控 | `debugger`: 调试器实例, `enable`: 是否启用 | `void` |
| `is_monitoring_enabled` | 检查内存监控是否启用 | `debugger`: 调试器实例 | `bool`: 启用返回true |
| `get_leak_count` | 获取内存泄漏数量 | `debugger`: 调试器实例 | `size_t`: 泄漏数量 |
| `report_leaks` | 报告内存泄漏 | `debugger`: 调试器实例 | `void` |
| `clear_leak_records` | 清除泄漏记录 | `debugger`: 调试器实例 | `void` |
| `enable_overflow_check` | 启用或禁用缓冲区溢出检查 | `debugger`: 调试器实例, `enable`: 是否启用 | `void` |
| `enable_double_free_check` | 启用或禁用双重释放检查 | `debugger`: 调试器实例, `enable`: 是否启用 | `void` |
| `enable_uninitialized_check` | 启用或禁用未初始化内存检查 | `debugger`: 调试器实例, `enable`: 是否启用 | `void` |
| `validate_pointer` | 验证指针有效性 | `debugger`: 调试器实例, `ptr`: 要验证的指针 | `bool`: 有效返回true |
| `validate_memory_range` | 验证内存范围有效性 | `debugger`: 调试器实例, `ptr`: 内存起始指针, `size`: 内存大小 | `bool`: 有效返回true |
| `check_all_allocations` | 检查所有活动内存分配 | `debugger`: 调试器实例 | `void` |
| `dump_memory_info` | 转储内存信息 | `debugger`: 调试器实例 | `void` |
| `dump_allocation_stats` | 转储分配统计信息 | `debugger`: 调试器实例 | `void` |
| `set_debug_output` | 设置调试输出函数 | `debugger`: 调试器实例, `output_func`: 输出函数指针 | `void` |
| `enable_stack_trace` | 启用或禁用调用栈跟踪 | `debugger`: 调试器实例, `enable`: 是否启用 | `void` |
| `get_stack_trace_depth` | 获取调用栈跟踪深度 | `debugger`: 调试器实例 | `size_t`: 跟踪深度 |
| `set_stack_trace_depth` | 设置调用栈跟踪深度 | `debugger`: 调试器实例, `depth`: 跟踪深度 | `void` |
| `get_total_allocations` | 获取总分配次数 | `debugger`: 调试器实例 | `uint64_t`: 总分配次数 |
| `get_total_deallocations` | 获取总释放次数 | `debugger`: 调试器实例 | `uint64_t`: 总释放次数 |
| `get_peak_memory_usage` | 获取峰值内存使用量 | `debugger`: 调试器实例 | `size_t`: 峰值内存使用量 |
| `get_current_memory_usage` | 获取当前内存使用量 | `debugger`: 调试器实例 | `size_t`: 当前内存使用量 |

### 工厂函数 API

#### F_create_default_memory_debugger
创建默认内存调试器。

**原型：**
```c
Stru_MemoryDebuggerInterface_t* F_create_default_memory_debugger(void);
```

**参数：** 无

**返回值：** 调试器接口指针，失败返回NULL

**描述：** 创建一个具有默认配置的内存调试器实例。

#### F_create_configured_memory_debugger
创建具有特定配置的内存调试器。

**原型：**
```c
Stru_MemoryDebuggerInterface_t* F_create_configured_memory_debugger(
    bool enable_leak_detection,
    bool enable_overflow_check,
    bool enable_double_free_check,
    size_t stack_trace_depth);
```

**参数：**
- `enable_leak_detection`: 是否启用内存泄漏检测
- `enable_overflow_check`: 是否启用缓冲区溢出检查
- `enable_double_free_check`: 是否启用双重释放检查
- `stack_trace_depth`: 调用栈跟踪深度（0表示禁用）

**返回值：** 调试器接口指针，失败返回NULL

**描述：** 创建一个具有指定配置的内存调试器实例。

#### F_destroy_memory_debugger
销毁内存调试器。

**原型：**
```c
void F_destroy_memory_debugger(Stru_MemoryDebuggerInterface_t* debugger);
```

**参数：**
- `debugger`: 要销毁的调试器

**返回值：** 无

**描述：** 销毁内存调试器实例，释放相关资源。

#### F_get_memory_debugger_interface
获取内存调试器接口（向后兼容）。

**原型：**
```c
Stru_MemoryDebuggerInterface_t* F_get_memory_debugger_interface(void);
```

**参数：** 无

**返回值：** 调试器接口指针

**描述：** 获取内存调试器接口，用于向后兼容旧代码。

### 泄漏检测 API

#### F_create_leak_detector
创建泄漏检测器。

**原型：**
```c
Stru_LeakDetectorContext_t* F_create_leak_detector(bool enable_stack_trace, size_t max_stack_depth);
```

#### F_destroy_leak_detector
销毁泄漏检测器。

**原型：**
```c
void F_destroy_leak_detector(Stru_LeakDetectorContext_t* detector);
```

#### F_record_allocation
记录内存分配。

**原型：**
```c
void F_record_allocation(Stru_LeakDetectorContext_t* detector,
                        void* address, size_t size,
                        const char* file, size_t line,
                        const char* function);
```

#### F_record_deallocation
记录内存释放。

**原型：**
```c
void F_record_deallocation(Stru_LeakDetectorContext_t* detector, void* address);
```

#### F_check_leaks
检查内存泄漏。

**原型：**
```c
size_t F_check_leaks(Stru_LeakDetectorContext_t* detector);
```

### 错误检测 API

#### F_create_error_detector
创建错误检测器。

**原型：**
```c
Stru_ErrorDetectorContext_t* F_create_error_detector(
    bool enable_overflow_check,
    bool enable_double_free_check,
    bool enable_uninitialized_check,
    size_t guard_zone_size);
```

#### F_destroy_error_detector
销毁错误检测器。

**原型：**
```c
void F_destroy_error_detector(Stru_ErrorDetectorContext_t* detector);
```

#### F_validate_pointer
验证指针有效性。

**原型：**
```c
bool F_validate_pointer(Stru_ErrorDetectorContext_t* detector, const void* ptr);
```

#### F_validate_memory_range
验证内存范围有效性。

**原型：**
```c
bool F_validate_memory_range(Stru_ErrorDetectorContext_t* detector,
                            const void* ptr, size_t size);
```

### 分析功能 API

#### F_create_memory_analyzer
创建内存分析器。

**原型：**
```c
Stru_AnalyzerContext_t* F_create_memory_analyzer(
    bool enable_performance_tracking,
    bool enable_fragmentation_analysis,
    uint32_t sampling_interval_ms);
```

#### F_destroy_memory_analyzer
销毁内存分析器。

**原型：**
```c
void F_destroy_memory_analyzer(Stru_AnalyzerContext_t* analyzer);
```

#### F_get_allocation_stats
获取内存分配统计。

**原型：**
```c
void F_get_allocation_stats(Stru_AnalyzerContext_t* analyzer,
                           Stru_MemoryAllocationStats_t* stats);
```

#### F_get_performance_metrics
获取性能指标。

**原型：**
```c
void F_get_performance_metrics(Stru_AnalyzerContext_t* analyzer,
                              Stru_PerformanceMetrics_t* metrics);
```

### 调试工具 API

#### F_initialize_debug_tools
初始化调试工具。

**原型：**
```c
bool F_initialize_debug_tools(void);
```

#### F_cleanup_debug_tools
清理调试工具。

**原型：**
```c
void F_cleanup_debug_tools(void);
```

#### F_dump_memory
转储内存内容。

**原型：**
```c
void F_dump_memory(const void* address, size_t size, 
                  const Stru_MemoryDumpOptions_t* options);
```

#### F_get_stack_trace
获取调用栈跟踪。

**原型：**
```c
size_t F_get_stack_trace(const Stru_StackTraceOptions_t* options,
                        char* buffer, size_t buffer_size);
```

## 使用示例

### 示例1：基本使用
```c
#include "CN_memory_debug.h"

int main() {
    // 创建默认调试器
    Stru_MemoryDebuggerInterface_t* debugger = F_create_default_memory_debugger();
    if (!debugger) {
        return 1;
    }
    
    // 初始化
    if (!debugger->initialize(debugger)) {
        F_destroy_memory_debugger(debugger);
        return 1;
    }
    
    // 启用监控
    debugger->enable_monitoring(debugger, true);
    
    // 启用各种检查
    debugger->enable_overflow_check(debugger, true);
    debugger->enable_double_free_check(debugger, true);
    
    // 进行内存操作...
    void* ptr = malloc(100);
    // 使用内存...
    free(ptr);
    
    // 检查泄漏
    size_t leaks = debugger->get_leak_count(debugger);
    if (leaks > 0) {
        debugger->report_leaks(debugger);
    }
    
    // 清理
    debugger->cleanup(debugger);
    F_destroy_memory_debugger(debugger);
    
    return 0;
}
```

### 示例2：使用特定配置
```c
#include "CN_memory_debug.h"

int main() {
    // 创建配置的调试器
    Stru_MemoryDebuggerInterface_t* debugger = F_create_configured_memory_debugger(
        true,   // 启用泄漏检测
        true,   // 启用溢出检查
        true,   // 启用双重释放检查
        10      // 调用栈跟踪深度
    );
    
    // ... 使用调试器 ...
    
    F_destroy_memory_debugger(debugger);
    return 0;
}
```

## 向后兼容性

模块提供了向后兼容性支持：

1. 旧接口 `Stru_MemoryDebugInterface_t` 仍然可用
2. 新接口 `Stru_MemoryDebuggerInterface_t` 提供了更多功能
3. 工厂函数支持新旧接口的创建

## 错误处理

所有函数都遵循以下错误处理模式：

1. 创建函数失败时返回NULL
2. 初始化函数失败时返回false
3. 验证函数失败时返回false
4. 其他函数通常不返回错误状态，但会记录错误信息

## 性能考虑

1. 调试功能有性能开销，生产环境建议禁用
2. 可以按需启用特定功能以减少开销
3. 调用栈跟踪有显著性能影响

## 平台支持

模块设计为跨平台，但某些功能（如调用栈跟踪）可能需要平台特定实现。

## 版本信息

- **当前版本**: 2.0.0
- **发布日期**: 2026-01-08
- **兼容性**: 向后兼容版本1.x
- **维护者**: CN_Language架构委员会
