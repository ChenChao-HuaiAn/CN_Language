# 内存泄漏检测模块

## 概述

内存泄漏检测模块提供了实时内存泄漏检测功能，帮助开发者发现和修复内存泄漏问题。该模块记录所有内存分配和释放操作，并在程序结束时报告未释放的内存块。

## 功能特性

### 1. 实时监控
- 监控所有内存分配操作（malloc, calloc, realloc）
- 监控所有内存释放操作（free）
- 实时跟踪内存使用情况

### 2. 泄漏检测
- 自动检测未释放的内存块
- 提供详细的泄漏报告
- 支持按模块/文件统计泄漏

### 3. 调用栈跟踪
- 可选启用调用栈跟踪
- 记录分配点的调用栈信息
- 帮助定位泄漏源头

### 4. 统计信息
- 活动分配数量统计
- 总分配内存统计
- 泄漏数量统计

## 接口说明

### 数据结构

```c
// 内存分配记录
typedef struct Stru_MemoryAllocationRecord_t
{
    void* address;              // 分配的内存地址
    size_t size;                // 分配的大小
    const char* file;           // 分配所在的源文件
    size_t line;                // 分配所在的行号
    const char* function;       // 分配所在的函数名
    void* stack_trace[16];      // 调用栈跟踪（最多16层）
    size_t stack_depth;         // 调用栈深度
    struct Stru_MemoryAllocationRecord_t* next;  // 下一个记录
} Stru_MemoryAllocationRecord_t;
```

### 主要函数

#### 1. 创建泄漏检测器
```c
Stru_LeakDetectorContext_t* F_create_leak_detector(
    bool enable_stack_trace, 
    size_t max_stack_depth);
```

**参数：**
- `enable_stack_trace`: 是否启用调用栈跟踪
- `max_stack_depth`: 最大调用栈深度（如果启用栈跟踪）

**返回值：** 泄漏检测器上下文，失败返回NULL

#### 2. 销毁泄漏检测器
```c
void F_destroy_leak_detector(Stru_LeakDetectorContext_t* detector);
```

#### 3. 记录内存分配
```c
void F_record_allocation(Stru_LeakDetectorContext_t* detector,
                        void* address, size_t size,
                        const char* file, size_t line,
                        const char* function);
```

#### 4. 记录内存释放
```c
void F_record_deallocation(Stru_LeakDetectorContext_t* detector, void* address);
```

#### 5. 检查内存泄漏
```c
size_t F_check_leaks(Stru_LeakDetectorContext_t* detector);
```

**返回值：** 泄漏的内存块数量

#### 6. 生成泄漏报告
```c
void F_generate_leak_report(Stru_LeakDetectorContext_t* detector,
                           char* report_buffer, size_t buffer_size);
```

## 使用示例

### 基本使用
```c
#include "CN_memory_leak_detection.h"

int main() {
    // 创建泄漏检测器（启用栈跟踪，最大深度10）
    Stru_LeakDetectorContext_t* detector = 
        F_create_leak_detector(true, 10);
    
    if (!detector) {
        return 1;
    }
    
    // 记录内存分配
    void* ptr1 = malloc(100);
    F_record_allocation(detector, ptr1, 100, __FILE__, __LINE__, __func__);
    
    void* ptr2 = calloc(10, 20);
    F_record_allocation(detector, ptr2, 200, __FILE__, __LINE__, __func__);
    
    // 记录内存释放
    free(ptr1);
    F_record_deallocation(detector, ptr1);
    
    // 故意泄漏ptr2
    
    // 检查泄漏
    size_t leak_count = F_check_leaks(detector);
    if (leak_count > 0) {
        char report[4096];
        F_generate_leak_report(detector, report, sizeof(report));
        printf("发现内存泄漏:\n%s\n", report);
    }
    
    // 销毁检测器
    F_destroy_leak_detector(detector);
    
    return 0;
}
```

### 集成到内存分配器
```c
// 包装malloc函数
void* debug_malloc(size_t size, const char* file, size_t line, const char* function) {
    void* ptr = malloc(size);
    if (ptr != NULL && g_leak_detector != NULL) {
        F_record_allocation(g_leak_detector, ptr, size, file, line, function);
    }
    return ptr;
}

// 包装free函数
void debug_free(void* ptr) {
    if (ptr != NULL && g_leak_detector != NULL) {
        F_record_deallocation(g_leak_detector, ptr);
    }
    free(ptr);
}

// 使用宏简化调用
#define DEBUG_MALLOC(size) debug_malloc(size, __FILE__, __LINE__, __func__)
#define DEBUG_FREE(ptr) debug_free(ptr)
```

## 性能考虑

### 1. 内存开销
- 每个分配记录需要额外内存存储元数据
- 调用栈跟踪增加额外内存使用
- 哈希表用于快速查找增加开销

### 2. 性能开销
- 每次分配/释放都需要记录操作
- 调用栈跟踪有显著性能影响
- 泄漏检查需要遍历所有活动分配

### 3. 优化建议
- 生产环境禁用泄漏检测
- 按需启用调用栈跟踪
- 使用采样减少性能影响

## 配置选项

### 1. 栈跟踪配置
- **启用/禁用**: 控制是否记录调用栈
- **最大深度**: 限制调用栈记录深度
- **过滤函数**: 可配置过滤特定函数

### 2. 报告配置
- **详细程度**: 控制报告详细程度
- **输出格式**: 支持多种输出格式
- **过滤规则**: 按大小、文件等过滤报告

## 错误处理

### 1. 创建失败
- 内存不足时返回NULL
- 记录错误日志

### 2. 操作失败
- 无效参数时静默失败
- 记录警告信息

### 3. 资源耗尽
- 记录数量超过限制时扩展容量
- 调用栈深度超过限制时截断

## 平台支持

### 1. 支持平台
- Windows (MSVC, MinGW)
- Linux (GCC, Clang)
- macOS (Clang)

### 2. 平台特定功能
- **Windows**: 使用CaptureStackBackTrace
- **Linux**: 使用backtrace
- **macOS**: 使用backtrace

## 测试覆盖

### 1. 单元测试
- 基本分配/释放测试
- 泄漏检测测试
- 调用栈跟踪测试

### 2. 集成测试
- 与内存分配器集成测试
- 多线程环境测试
- 压力测试

### 3. 性能测试
- 性能基准测试
- 内存使用测试
- 扩展性测试

## 维护说明

### 1. 版本兼容性
- 保持向后兼容性
- 废弃接口提供迁移路径
- 版本号遵循语义化版本

### 2. 代码质量
- 遵循项目编码规范
- 高测试覆盖率（≥85%）
- 静态分析通过

### 3. 文档更新
- API变更时更新文档
- 示例代码保持最新
- 设计决策记录在案

## 相关模块

### 1. 依赖模块
- 基础数据结构模块
- 内存分配器模块
- 错误处理模块

### 2. 相关模块
- 内存错误检测模块
- 内存分析模块
- 调试工具模块

## 参考资料

1. [CN_Language项目技术规范](../docs/specifications/CN_Language项目%20技术规范和编码标准.md)
2. [内存调试模块API文档](../docs/api/infrastructure/memory/debug/API.md)
3. [内存管理架构设计](../../../docs/architecture/内存管理架构设计.md)

---

*最后更新: 2026-01-08*
*版本: 2.0.0*
*维护者: CN_Language架构委员会*
