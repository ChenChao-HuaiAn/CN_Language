# 内存调试模块 API 文档

## 概述

本文档描述了内存调试模块的API接口。内存调试模块提供了全面的内存调试和诊断功能，包括内存泄漏检测、错误检测、使用分析和调试工具。

## 模块化架构

内存调试模块采用模块化设计，遵循单一职责原则和SOLID设计原则。模块分为多个子模块，每个子模块专注于特定功能领域。

### 模块结构图
```
src/infrastructure/memory/debug/
├── CN_memory_debug.h              # 主头文件（向后兼容）
├── CN_memory_debug_new.c          # 新实现（薄包装层）
├── README.md                      # 主模块文档
├── interface/                     # 接口定义
│   ├── CN_memory_debug_interface.h
│   ├── CN_memory_debug_factory.h
│   └── README.md
├── implementation/                # 核心实现
│   ├── CN_memory_debug_private.h/.c
│   ├── CN_memory_debug_core.h/.c
│   └── README.md
├── leak_detection/               # 泄漏检测子模块
│   ├── CN_memory_leak_detection.h/.c
│   └── README.md
├── error_detection/              # 错误检测子模块
│   ├── CN_memory_error_detection.h/.c
│   └── README.md
├── analysis/                     # 分析功能子模块
│   ├── CN_memory_analysis.h/.c
│   └── README.md
└── tools/                        # 调试工具子模块
    ├── CN_memory_debug_tools.h/.c
    └── README.md
```

### 1. 主接口 (`CN_memory_debug.h`)
主头文件，提供向后兼容的接口，包含所有子模块的接口。

### 2. 新实现 (`CN_memory_debug_new.c`)
模块化实现的薄包装层，将旧接口映射到新模块化实现。

### 3. 抽象接口 (`interface/CN_memory_debug_interface.h`)
定义了内存调试器的抽象接口，遵循SOLID设计原则。

### 4. 工厂函数 (`interface/CN_memory_debug_factory.h`)
提供了创建和销毁内存调试器实例的工厂函数。

### 5. 核心实现 (`implementation/`)
包含私有数据结构和核心接口函数实现。

### 6. 泄漏检测 (`leak_detection/CN_memory_leak_detection.h/.c`)
专门处理内存泄漏检测的功能。

### 7. 错误检测 (`error_detection/CN_memory_error_detection.h/.c`)
专门处理内存错误检测的功能。

### 8. 分析功能 (`analysis/CN_memory_analysis.h/.c`)
专门处理内存使用分析的功能。

### 9. 调试工具 (`tools/CN_memory_debug_tools.h/.c`)
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

**描述：** 创建一个具有默认配置的内存调试器实例。使用模块化实现，组合了泄漏检测、错误检测、分析和调试工具子模块。

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

**描述：** 创建一个具有指定配置的内存调试器实例。根据配置动态组合所需的子模块。

#### F_destroy_memory_debugger
销毁内存调试器。

**原型：**
```c
void F_destroy_memory_debugger(Stru_MemoryDebuggerInterface_t* debugger);
```

**参数：**
- `debugger`: 要销毁的调试器

**返回值：** 无

**描述：** 销毁内存调试器实例，释放相关资源。会正确销毁所有子模块。

#### F_get_memory_debugger_interface
获取内存调试器接口（向后兼容）。

**原型：**
```c
Stru_MemoryDebuggerInterface_t* F_get_memory_debugger_interface(void);
```

**参数：** 无

**返回值：** 调试器接口指针

**描述：** 获取内存调试器接口，用于向后兼容旧代码。返回一个全局共享的调试器实例。

#### F_create_memory_debugger_from_components
从组件创建内存调试器（高级API）。

**原型：**
```c
Stru_MemoryDebuggerInterface_t* F_create_memory_debugger_from_components(
    Stru_LeakDetectorContext_t* leak_detector,
    Stru_ErrorDetectorContext_t* error_detector,
    Stru_AnalyzerContext_t* analyzer,
    Stru_DebugToolsContext_t* debug_tools);
```

**参数：**
- `leak_detector`: 泄漏检测器组件（可为NULL）
- `error_detector`: 错误检测器组件（可为NULL）
- `analyzer`: 分析器组件（可为NULL）
- `debug_tools`: 调试工具组件（可为NULL）

**返回值：** 调试器接口指针，失败返回NULL

**描述：** 从现有组件创建内存调试器，允许高级用户自定义组件组合。

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

模块提供了完整的向后兼容性支持：

1. **旧接口兼容**：`Stru_MemoryDebugInterface_t` 仍然可用，通过 `CN_memory_debug_new.c` 中的薄包装层映射到新实现
2. **新接口增强**：`Stru_MemoryDebuggerInterface_t` 提供了更多功能和更好的模块化设计
3. **工厂函数支持**：工厂函数同时支持新旧接口的创建
4. **平滑迁移**：现有代码无需修改即可继续工作，同时可以逐步迁移到新接口
5. **二进制兼容**：模块化重构保持了二进制兼容性，现有应用程序无需重新编译

### 迁移指南
1. 新项目应直接使用 `Stru_MemoryDebuggerInterface_t` 接口
2. 现有项目可以继续使用旧接口，或逐步迁移到新接口
3. 可以使用 `F_get_memory_debugger_interface()` 获取新接口实例
4. 所有新功能仅在新接口中可用

## 错误处理

所有函数都遵循以下错误处理模式：

1. 创建函数失败时返回NULL
2. 初始化函数失败时返回false
3. 验证函数失败时返回false
4. 其他函数通常不返回错误状态，但会记录错误信息

## 性能考虑

1. **模块化开销**：模块化设计增加了少量间接调用开销，但提高了可维护性和可测试性
2. **按需加载**：可以按需启用特定功能以减少开销
3. **调用栈跟踪**：调用栈跟踪有显著性能影响，建议在调试时启用
4. **内存开销**：调试功能会增加内存使用，包括：
   - 泄漏检测：记录每个分配的元数据
   - 错误检测：添加保护区域和校验和
   - 分析功能：收集统计信息和性能数据
5. **生产环境**：生产环境建议禁用所有调试功能，或仅启用必要的监控功能
6. **配置优化**：可以通过配置调整平衡功能性和性能

### 性能优化建议
1. 在开发阶段启用完整调试功能
2. 在测试阶段根据需求选择性启用功能
3. 在生产阶段禁用所有调试功能，或仅启用轻量级监控
4. 使用采样分析而不是连续跟踪来减少开销

## 平台支持

模块设计为跨平台，支持以下平台：

### 支持平台
- **Windows**: Windows 7及以上版本
- **Linux**: 主流Linux发行版（Ubuntu, CentOS, Fedora等）
- **macOS**: macOS 10.12及以上版本

### 平台特定功能
1. **调用栈跟踪**：需要平台特定实现
   - Windows: 使用DbgHelp API
   - Linux/macOS: 使用libunwind或backtrace
2. **内存保护**：使用平台特定的内存保护机制
3. **线程安全**：使用平台特定的线程同步原语

### 编译要求
- **C编译器**: GCC 4.8+, Clang 3.5+, MSVC 2015+
- **标准库**: C11标准库
- **可选依赖**: libunwind（用于高级调用栈跟踪）

### 构建配置
模块支持多种构建配置：
- **Debug**: 启用所有调试功能，包含完整符号信息
- **Release**: 禁用调试功能，优化性能
- **Profile**: 启用性能分析功能

## 版本信息

### 模块版本
- **当前版本**: 2.0.0
- **架构版本**: 2.0.0
- **发布日期**: 2026-01-08
- **兼容性**: 向后兼容版本1.x

### 变更历史
#### 版本 2.0.0 (2026-01-08)
- **重大重构**: 从单体架构重构为模块化架构
- **新增功能**: 添加了错误检测、分析和调试工具子模块
- **性能改进**: 优化了内存使用和性能
- **文档完善**: 为所有子模块添加了完整文档
- **测试增强**: 实现了模块化测试架构

#### 版本 1.0.0 (2025-12-01)
- **初始版本**: 基本内存调试功能
- **核心功能**: 泄漏检测和基本错误检查
- **基础架构**: 单体实现，包含所有功能

### 维护信息
- **维护者**: CN_Language架构委员会
- **支持周期**: 长期支持（LTS）
- **更新策略**: 语义化版本控制
- **问题跟踪**: 通过GitHub Issues

### 未来规划
1. **性能分析**: 添加更详细的性能分析功能
2. **可视化工具**: 开发内存使用可视化工具
3. **集成测试**: 增强集成测试覆盖
4. **多语言绑定**: 提供Python、JavaScript等语言绑定
