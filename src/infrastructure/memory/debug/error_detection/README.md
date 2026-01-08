# 内存错误检测模块

## 概述

内存错误检测模块提供了全面的内存错误检测功能，包括缓冲区溢出、双重释放、野指针访问等常见内存错误的检测。该模块在开发和测试阶段特别有用，可以帮助开发者发现和修复潜在的内存安全问题。

## 功能特性

### 1. 缓冲区溢出检测
- **上溢检测**: 检测写入超过分配内存末尾
- **下溢检测**: 检测写入在分配内存开始之前
- **哨兵值保护**: 使用特殊值标记内存边界

### 2. 双重释放检测
- **释放记录**: 记录所有已释放的内存块
- **重复检测**: 检测对同一内存块的多次释放
- **无效释放**: 检测对非分配地址的释放

### 3. 野指针检测
- **释放后使用**: 检测使用已释放的内存
- **未初始化使用**: 检测使用未初始化的内存
- **越界访问**: 检测访问超出分配范围

### 4. 内存完整性检查
- **内存填充**: 使用特定模式填充内存
- **完整性验证**: 定期验证内存完整性
- **损坏检测**: 检测内存损坏

## 接口说明

### 数据结构

```c
// 内存错误类型
typedef enum Eum_MemoryErrorType_t
{
    Eum_MEMORY_ERROR_NONE = 0,           // 无错误
    Eum_MEMORY_ERROR_OVERFLOW,           // 缓冲区溢出
    Eum_MEMORY_ERROR_UNDERFLOW,          // 缓冲区下溢
    Eum_MEMORY_ERROR_DOUBLE_FREE,        // 双重释放
    Eum_MEMORY_ERROR_INVALID_FREE,       // 无效释放（非分配地址）
    Eum_MEMORY_ERROR_USE_AFTER_FREE,     // 释放后使用
    Eum_MEMORY_ERROR_UNINITIALIZED_READ, // 未初始化读取
    Eum_MEMORY_ERROR_CORRUPTION,         // 内存损坏
    Eum_MEMORY_ERROR_ALIGNMENT,          // 对齐错误
    Eum_MEMORY_ERROR_OUT_OF_BOUNDS       // 越界访问
} Eum_MemoryErrorType_t;

// 内存错误信息
typedef struct Stru_MemoryErrorInfo_t
{
    Eum_MemoryErrorType_t error_type;    // 错误类型
    void* address;                       // 错误地址
    size_t size;                         // 相关大小
    const char* file;                    // 源文件名
    size_t line;                         // 行号
    const char* function;                // 函数名
    const char* description;             // 错误描述
} Stru_MemoryErrorInfo_t;
```

### 主要函数

#### 1. 创建错误检测器
```c
Stru_ErrorDetectorContext_t* F_create_error_detector(
    bool enable_overflow_check,
    bool enable_double_free_check,
    bool enable_uninitialized_check,
    size_t guard_zone_size);
```

**参数：**
- `enable_overflow_check`: 是否启用缓冲区溢出检查
- `enable_double_free_check`: 是否启用双重释放检查
- `enable_uninitialized_check`: 是否启用未初始化内存检查
- `guard_zone_size`: 保护区域大小（用于溢出检测）

**返回值：** 错误检测器上下文，失败返回NULL

#### 2. 销毁错误检测器
```c
void F_destroy_error_detector(Stru_ErrorDetectorContext_t* detector);
```

#### 3. 保护内存区域
```c
bool F_protect_memory_region(Stru_ErrorDetectorContext_t* detector,
                            void* address, size_t size,
                            const char* file, size_t line,
                            const char* function);
```

#### 4. 取消内存区域保护
```c
void F_unprotect_memory_region(Stru_ErrorDetectorContext_t* detector, void* address);
```

#### 5. 验证指针有效性
```c
bool F_validate_pointer(Stru_ErrorDetectorContext_t* detector, const void* ptr);
```

#### 6. 验证内存范围有效性
```c
bool F_validate_memory_range(Stru_ErrorDetectorContext_t* detector,
                            const void* ptr, size_t size);
```

#### 7. 检查所有活动内存分配
```c
size_t F_check_all_allocations(Stru_ErrorDetectorContext_t* detector);
```

**返回值：** 发现的错误数量

## 使用示例

### 基本使用
```c
#include "CN_memory_error_detection.h"

int main() {
    // 创建错误检测器（启用溢出和双重释放检查）
    Stru_ErrorDetectorContext_t* detector = 
        F_create_error_detector(true, true, false, 16);
    
    if (!detector) {
        return 1;
    }
    
    // 分配内存并保护
    void* memory = malloc(256);
    if (memory) {
        F_protect_memory_region(detector, memory, 256, __FILE__, __LINE__, __func__);
        
        // 正常使用内存
        memset(memory, 0, 256);
        
        // 故意制造缓冲区溢出（检测）
        char* buffer = (char*)memory;
        buffer[256] = 'X';  // 溢出！
        
        // 检查错误
        size_t error_count = F_check_all_allocations(detector);
        if (error_count > 0) {
            printf("发现 %zu 个内存错误\n", error_count);
            
            // 获取错误详情
            Stru_MemoryErrorInfo_t error_info;
            if (F_get_error_info(detector, 0, &error_info)) {
                printf("错误类型: %d\n", error_info.error_type);
                printf("错误地址: %p\n", error_info.address);
                printf("错误描述: %s\n", error_info.description);
            }
        }
        
        // 取消保护并释放
        F_unprotect_memory_region(detector, memory);
        free(memory);
    }
    
    // 测试双重释放
    void* ptr = malloc(100);
    if (ptr) {
        free(ptr);
        free(ptr);  // 双重释放！
    }
    
    // 销毁检测器
    F_destroy_error_detector(detector);
    
    return 0;
}
```

### 集成到内存分配器
```c
// 包装malloc函数
void* debug_malloc(size_t size, const char* file, size_t line, const char* function) {
    void* ptr = malloc(size);
    if (ptr != NULL && g_error_detector != NULL) {
        // 添加保护区域
        F_protect_memory_region(g_error_detector, ptr, size, file, line, function);
        
        // 记录分配
        F_error_detector_record_allocation(g_error_detector, ptr, size);
    }
    return ptr;
}

// 包装free函数
void debug_free(void* ptr) {
    if (ptr != NULL && g_error_detector != NULL) {
        // 检查双重释放
        if (!F_error_detector_record_deallocation(g_error_detector, ptr)) {
            printf("警告：检测到双重释放或无效释放！\n");
        }
        
        // 取消保护
        F_unprotect_memory_region(g_error_detector, ptr);
    }
    free(ptr);
}
```

## 实现原理

### 1. 缓冲区溢出检测
- **哨兵值**: 在分配内存前后添加特殊值
- **定期检查**: 定期验证哨兵值是否被修改
- **实时检测**: 在内存访问时检查边界

### 2. 双重释放检测
- **分配记录**: 维护所有活动分配的哈希表
- **释放验证**: 释放时检查是否在活动分配表中
- **已释放记录**: 维护已释放内存的列表

### 3. 野指针检测
- **释放后填充**: 释放后用特殊值填充内存
- **访问检查**: 访问时检查内存内容
- **延迟重用**: 延迟重用已释放的内存块

### 4. 未初始化内存检测
- **初始化填充**: 分配时用特殊值填充内存
- **读取检查**: 读取时检查是否为初始化值
- **写入跟踪**: 跟踪哪些内存区域已被写入

## 性能考虑

### 1. 内存开销
- 保护区域增加内存使用
- 分配记录需要额外内存
- 错误信息存储需要内存

### 2. 性能开销
- 边界检查增加访问时间
- 分配/释放操作更复杂
- 定期检查消耗CPU时间

### 3. 优化建议
- 生产环境选择性启用功能
- 使用采样减少性能影响
- 按需进行完整性检查

## 配置选项

### 1. 检测类型配置
- **溢出检测**: 控制是否启用溢出检测
- **双重释放检测**: 控制是否启用双重释放检测
- **未初始化检测**: 控制是否启用未初始化检测

### 2. 保护区域配置
- **保护区域大小**: 控制保护区域的大小
- **哨兵值模式**: 配置使用的哨兵值
- **检查频率**: 控制完整性检查的频率

### 3. 报告配置
- **错误报告级别**: 控制错误报告的详细程度
- **输出格式**: 支持多种错误输出格式
- **错误过滤**: 按类型过滤错误报告

## 错误处理

### 1. 创建失败
- 内存不足时返回NULL
- 记录详细错误日志

### 2. 操作失败
- 无效参数时返回false
- 记录警告信息

### 3. 错误检测
- 检测到错误时记录详细信息
- 支持错误回调函数
- 可配置错误处理策略

## 平台支持

### 1. 支持平台
- Windows (MSVC, MinGW)
- Linux (GCC, Clang)
- macOS (Clang)

### 2. 平台特定优化
- **内存保护**: 使用平台特定的内存保护机制
- **性能优化**: 平台特定的性能优化
- **调试集成**: 与平台调试器集成

## 测试覆盖

### 1. 单元测试
- 各种错误类型检测测试
- 边界条件测试
- 错误处理测试

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
- 内存泄漏检测模块
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
