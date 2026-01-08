# 内存分析模块

## 概述

内存分析模块提供了全面的内存使用分析和性能监控功能。该模块帮助开发者理解内存使用模式、识别性能瓶颈、分析内存碎片，从而优化内存使用和提升应用程序性能。

## 功能特性

### 1. 内存使用统计
- **分配统计**: 统计内存分配次数、大小、频率
- **使用趋势**: 分析内存使用随时间的变化趋势
- **峰值监控**: 监控内存使用的峰值和平均值

### 2. 内存碎片分析
- **碎片程度**: 计算内存碎片化百分比
- **空闲块分析**: 分析空闲内存块的分布和大小
- **碎片趋势**: 跟踪碎片化随时间的变化

### 3. 性能分析
- **分配性能**: 监控内存分配和释放的性能
- **延迟分析**: 分析内存操作的延迟分布
- **吞吐量**: 计算内存操作的吞吐量

### 4. 趋势分析
- **使用模式**: 识别内存使用的模式
- **预测分析**: 基于历史数据预测未来内存使用
- **异常检测**: 检测异常的内存使用模式

## 接口说明

### 数据结构

```c
// 内存分配统计
typedef struct Stru_MemoryAllocationStats_t
{
    uint64_t total_allocations;      // 总分配次数
    uint64_t total_deallocations;    // 总释放次数
    size_t peak_memory_usage;        // 峰值内存使用量（字节）
    size_t current_memory_usage;     // 当前内存使用量（字节）
    size_t total_allocated_bytes;    // 总分配字节数
    size_t total_freed_bytes;        // 总释放字节数
    size_t allocation_count;         // 当前活动分配数量
    size_t largest_allocation;       // 最大单次分配大小
    size_t smallest_allocation;      // 最小单次分配大小
    size_t average_allocation_size;  // 平均分配大小
} Stru_MemoryAllocationStats_t;

// 内存碎片信息
typedef struct Stru_MemoryFragmentationInfo_t
{
    size_t total_memory;             // 总内存
    size_t used_memory;              // 已使用内存
    size_t free_memory;              // 空闲内存
    size_t largest_free_block;       // 最大空闲块
    size_t free_block_count;         // 空闲块数量
    size_t used_block_count;         // 已使用块数量
    float fragmentation_percentage;  // 碎片化百分比
} Stru_MemoryFragmentationInfo_t;

// 性能指标
typedef struct Stru_PerformanceMetrics_t
{
    uint64_t allocation_time_ns;     // 分配总时间（纳秒）
    uint64_t deallocation_time_ns;   // 释放总时间（纳秒）
    uint64_t allocation_count;       // 分配次数
    uint64_t deallocation_count;     // 释放次数
    uint64_t average_allocation_time_ns;  // 平均分配时间（纳秒）
    uint64_t average_deallocation_time_ns; // 平均释放时间（纳秒）
    uint64_t peak_allocation_time_ns;     // 峰值分配时间（纳秒）
} Stru_PerformanceMetrics_t;
```

### 主要函数

#### 1. 创建内存分析器
```c
Stru_AnalyzerContext_t* F_create_memory_analyzer(
    bool enable_performance_tracking,
    bool enable_fragmentation_analysis,
    uint32_t sampling_interval_ms);
```

**参数：**
- `enable_performance_tracking`: 是否启用性能跟踪
- `enable_fragmentation_analysis`: 是否启用碎片分析
- `sampling_interval_ms`: 采样间隔（毫秒，0表示禁用采样）

**返回值：** 分析器上下文，失败返回NULL

#### 2. 销毁内存分析器
```c
void F_destroy_memory_analyzer(Stru_AnalyzerContext_t* analyzer);
```

#### 3. 记录内存分配
```c
void F_analyzer_record_allocation(Stru_AnalyzerContext_t* analyzer,
                        void* address, size_t size,
                        uint64_t allocation_time_ns);
```

#### 4. 记录内存释放
```c
void F_analyzer_record_deallocation(Stru_AnalyzerContext_t* analyzer,
                          void* address, uint64_t deallocation_time_ns);
```

#### 5. 获取内存分配统计
```c
void F_get_allocation_stats(Stru_AnalyzerContext_t* analyzer,
                           Stru_MemoryAllocationStats_t* stats);
```

#### 6. 获取内存碎片信息
```c
bool F_get_fragmentation_info(Stru_AnalyzerContext_t* analyzer,
                             Stru_MemoryFragmentationInfo_t* fragmentation_info);
```

#### 7. 获取性能指标
```c
void F_get_performance_metrics(Stru_AnalyzerContext_t* analyzer,
                              Stru_PerformanceMetrics_t* metrics);
```

#### 8. 生成内存使用报告
```c
void F_generate_memory_report(Stru_AnalyzerContext_t* analyzer,
                             char* report_buffer, size_t buffer_size);
```

## 使用示例

### 基本使用
```c
#include "CN_memory_analysis.h"
#include <stdio.h>
#include <time.h>

// 获取当前时间（纳秒）
uint64_t get_current_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

int main() {
    // 创建内存分析器（启用性能跟踪和碎片分析，采样间隔1秒）
    Stru_AnalyzerContext_t* analyzer = 
        F_create_memory_analyzer(true, true, 1000);
    
    if (!analyzer) {
        return 1;
    }
    
    // 开始性能采样
    F_start_performance_sampling(analyzer);
    
    // 模拟内存操作
    for (int i = 0; i < 100; i++) {
        uint64_t start_time = get_current_time_ns();
        
        // 分配内存
        void* ptr = malloc(i * 100);
        
        uint64_t alloc_time = get_current_time_ns() - start_time;
        
        if (ptr) {
            // 记录分配
            F_analyzer_record_allocation(analyzer, ptr, i * 100, alloc_time);
            
            // 使用内存
            memset(ptr, 0, i * 100);
            
            start_time = get_current_time_ns();
            
            // 释放内存
            free(ptr);
            
            uint64_t free_time = get_current_time_ns() - start_time;
            
            // 记录释放
            F_analyzer_record_deallocation(analyzer, ptr, free_time);
        }
    }
    
    // 停止性能采样
    F_stop_performance_sampling(analyzer);
    
    // 获取统计信息
    Stru_MemoryAllocationStats_t stats;
    F_get_allocation_stats(analyzer, &stats);
    
    printf("内存分配统计:\n");
    printf("  总分配次数: %llu\n", stats.total_allocations);
    printf("  总释放次数: %llu\n", stats.total_deallocations);
    printf("  峰值内存使用: %zu bytes\n", stats.peak_memory_usage);
    printf("  当前内存使用: %zu bytes\n", stats.current_memory_usage);
    printf("  平均分配大小: %zu bytes\n", stats.average_allocation_size);
    
    // 获取碎片信息
    Stru_MemoryFragmentationInfo_t frag_info;
    if (F_get_fragmentation_info(analyzer, &frag_info)) {
        printf("\n内存碎片信息:\n");
        printf("  总内存: %zu bytes\n", frag_info.total_memory);
        printf("  已使用内存: %zu bytes\n", frag_info.used_memory);
        printf("  空闲内存: %zu bytes\n", frag_info.free_memory);
        printf("  最大空闲块: %zu bytes\n", frag_info.largest_free_block);
        printf("  碎片化程度: %.2f%%\n", frag_info.fragmentation_percentage);
    }
    
    // 获取性能指标
    Stru_PerformanceMetrics_t perf_metrics;
    F_get_performance_metrics(analyzer, &perf_metrics);
    
    printf("\n性能指标:\n");
    printf("  平均分配时间: %llu ns\n", perf_metrics.average_allocation_time_ns);
    printf("  平均释放时间: %llu ns\n", perf_metrics.average_deallocation_time_ns);
    printf("  峰值分配时间: %llu ns\n", perf_metrics.peak_allocation_time_ns);
    
    // 生成详细报告
    char report[4096];
    F_generate_memory_report(analyzer, report, sizeof(report));
    printf("\n详细内存报告:\n%s\n", report);
    
    // 销毁分析器
    F_destroy_memory_analyzer(analyzer);
    
    return 0;
}
```

### 集成到内存分配器
```c
#include "CN_memory_analysis.h"
#include <time.h>

// 全局分析器实例
Stru_AnalyzerContext_t* g_memory_analyzer = NULL;

// 初始化内存分析
void init_memory_analysis() {
    g_memory_analyzer = F_create_memory_analyzer(true, true, 1000);
    if (g_memory_analyzer) {
        F_start_performance_sampling(g_memory_analyzer);
    }
}

// 包装malloc函数
void* analyzed_malloc(size_t size) {
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    void* ptr = malloc(size);
    
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    uint64_t alloc_time_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000 +
                            (end_time.tv_nsec - start_time.tv_nsec);
    
    if (ptr != NULL && g_memory_analyzer != NULL) {
        F_analyzer_record_allocation(g_memory_analyzer, ptr, size, alloc_time_ns);
    }
    
    return ptr;
}

// 包装free函数
void analyzed_free(void* ptr) {
    if (ptr == NULL) return;
    
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    free(ptr);
    
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    uint64_t free_time_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000 +
                           (end_time.tv_nsec - start_time.tv_nsec);
    
    if (g_memory_analyzer != NULL) {
        F_analyzer_record_deallocation(g_memory_analyzer, ptr, free_time_ns);
    }
}

// 生成分析报告
void generate_analysis_report() {
    if (g_memory_analyzer == NULL) return;
    
    char report[8192];
    F_generate_memory_report(g_memory_analyzer, report, sizeof(report));
    printf("内存分析报告:\n%s\n", report);
}

// 清理内存分析
void cleanup_memory_analysis() {
    if (g_memory_analyzer != NULL) {
        F_stop_performance_sampling(g_memory_analyzer);
        F_destroy_memory_analyzer(g_memory_analyzer);
        g_memory_analyzer = NULL;
    }
}
```

## 实现原理

### 1. 统计收集
- **实时监控**: 实时收集内存操作数据
- **采样机制**: 可配置的采样间隔减少开销
- **数据聚合**: 聚合数据生成统计信息

### 2. 碎片分析
- **内存映射**: 维护内存块的映射关系
- **空闲块跟踪**: 跟踪所有空闲内存块
- **碎片计算**: 基于空闲块分布计算碎片程度

### 3. 性能分析
- **时间测量**: 精确测量内存操作时间
- **统计计算**: 计算平均值、峰值等统计指标
- **趋势分析**: 分析性能随时间的变化趋势

### 4. 报告生成
- **数据格式化**: 将统计数据格式化为可读报告
- **图表生成**: 支持生成图表数据
- **导出功能**: 支持导出为多种格式

## 性能考虑

### 1. 内存开销
- 统计数据结构需要额外内存
- 采样数据存储需要内存
- 历史数据保留增加内存使用

### 2. 性能开销
- 时间测量增加操作延迟
- 数据收集消耗CPU时间
- 报告生成需要计算资源

### 3. 优化建议
- 调整采样间隔平衡精度和性能
- 选择性启用分析功能
- 定期清理历史数据

## 配置选项

### 1. 分析功能配置
- **性能跟踪**: 控制是否启用性能跟踪
- **碎片分析**: 控制是否启用碎片分析
- **趋势分析**: 控制是否启用趋势分析

### 2. 采样配置
- **采样间隔**: 控制数据采样的频率
- **采样深度**: 控制保留的历史数据量
- **采样策略**: 配置采样策略（固定间隔、自适应等）

### 3. 报告配置
- **报告格式**: 支持多种报告格式
- **详细程度**: 控制报告的详细程度
- **输出目标**: 配置报告输出目标（控制台、文件、网络等）

## 错误处理

### 1. 创建失败
- 内存不足时返回NULL
- 记录详细错误日志

### 2. 操作失败
- 无效参数时静默失败
- 记录警告信息

### 3. 资源管理
- 自动管理采样数据内存
- 资源耗尽时扩展容量
- 提供资源使用统计

## 平台支持

### 1. 支持平台
- Windows (MSVC, MinGW)
- Linux (GCC, Clang)
- macOS (Clang)

### 2. 平台特定功能
- **高精度计时**: 使用平台特定的高精度计时器
- **内存信息**: 获取平台特定的内存信息
- **性能计数器**: 使用平台性能计数器

## 测试覆盖

### 1. 单元测试
- 统计计算正确性测试
- 碎片分析算法测试
- 性能测量准确性测试

### 2. 集成测试
- 与内存分配器集成测试
- 长时间运行稳定性测试
- 多线程环境测试

### 3. 性能测试
- 分析器自身性能测试
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
- 时间测量模块
- 统计计算模块

### 2. 相关模块
- 内存泄漏检测模块
- 内存错误检测模块
- 调试工具模块

## 参考资料

1. [CN_Language项目技术规范](../docs/specifications/CN_Language项目%20技术规范和编码标准.md)
2. [内存调试模块API文档](../docs/api/infrastructure/memory/debug/API.md)
3. [内存管理架构设计](../../../docs/architecture/内存管理架构设计.md)

---

*最后更新: 2026-01-08*
*版本: 2.0.0*
*维护者: CN_Language架构委员会*
