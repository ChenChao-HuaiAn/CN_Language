# CN_Language 调试分配器模块

## 概述

调试分配器是CN_Language项目内存管理基础设施的一部分，提供高级调试功能，包括边界检查、内存泄漏检测、双重释放检测和内存损坏检测。该模块主要用于开发和测试阶段，帮助开发者发现和修复内存相关错误。

## 特性

### 1. 边界检查
- **保护区域**：在每个分配的内存块前后添加特殊填充区域
- **缓冲区溢出检测**：检测写入超出分配内存末尾的操作
- **缓冲区下溢检测**：检测写入分配内存开始之前的操作
- **填充验证**：定期检查保护区域的完整性

### 2. 泄漏检测
- **分配跟踪**：记录每次分配的详细信息（文件、行号、时间戳、目的等）
- **泄漏报告**：程序退出时自动生成未释放内存的报告
- **实时监控**：支持运行时查询当前分配状态

### 3. 双重释放检测
- **魔术字验证**：使用特殊值标记内存块状态（正常、已释放、损坏）
- **释放验证**：检测对已释放内存的重复释放操作
- **状态跟踪**：维护每个内存块的完整生命周期信息

### 4. 内存损坏检测
- **完整性检查**：验证内存块头部和数据的完整性
- **堆验证**：对整个堆进行完整性扫描
- **损坏报告**：详细报告损坏位置和可能原因

### 5. 统计信息
- **分配统计**：跟踪总分配次数、总释放次数、当前使用量等
- **性能统计**：记录分配/释放操作的时间开销
- **错误统计**：统计检测到的各种错误类型和数量

## 架构设计

### 模块结构
```
src/infrastructure/memory/debug/
├── CN_debug_allocator.h      # 公共接口定义
├── CN_debug_internal.h       # 内部数据结构和常量
├── CN_debug_core.c           # 核心实现（创建/销毁/内部辅助）
├── CN_debug_operations.c     # 操作实现（分配/释放/重新分配）
├── CN_debug_utils.c          # 工具函数（查询/配置/验证）
└── README.md                 # 本文档
```

### 设计原则
1. **单一职责原则**：每个文件专注于特定功能领域
2. **接口隔离**：公共接口最小化，内部实现细节隐藏
3. **可配置性**：通过配置结构体灵活调整调试功能
4. **零生产开销**：通过条件编译在生产环境中禁用调试功能

### 内存布局
```
+----------------+----------------+----------------+----------------+
| 前保护区域     | 内存块头部     | 用户数据       | 后保护区域     |
| (8字节)        | (64字节)       | (请求大小)     | (8字节)        |
+----------------+----------------+----------------+----------------+
^                ^                ^                ^
|                |                |                |
低地址          头部开始         用户指针         高地址
```

## 快速开始

### 基本使用

```c
#include "CN_debug_allocator.h"

int main()
{
    // 创建调试分配器配置
    Stru_CN_DebugAllocatorConfig_t config = {
        .enable_boundary_check = true,
        .enable_leak_detection = true,
        .enable_double_free_check = true,
        .enable_corruption_check = true,
        .front_guard_size = 8,
        .rear_guard_size = 8,
        .track_file_info = true,
        .track_timestamp = true
    };
    
    // 创建调试分配器
    Stru_CN_DebugAllocator_t* allocator = F_debug_allocator_create(&config);
    
    // 使用分配器
    int* ptr = (int*)F_debug_allocator_malloc(allocator, 100, __FILE__, __LINE__, "测试分配");
    
    // 使用内存...
    
    // 释放内存
    F_debug_allocator_free(allocator, ptr);
    
    // 销毁分配器
    F_debug_allocator_destroy(allocator);
    
    return 0;
}
```

### 集成到CN_Language内存系统

```c
#include "CN_memory.h"

int main()
{
    // 初始化CN_Language内存系统（自动使用调试分配器）
    CN_memory_init();
    
    // 使用标准内存接口（自动获得调试功能）
    int* ptr = (int*)cn_malloc(100);
    
    // 使用内存...
    
    // 释放内存
    cn_free(ptr);
    
    // 关闭内存系统（生成泄漏报告）
    CN_memory_shutdown();
    
    return 0;
}
```

## 配置选项

### 调试功能开关
```c
typedef struct Stru_CN_DebugAllocatorConfig_t
{
    bool enable_boundary_check;     // 启用边界检查
    bool enable_leak_detection;     // 启用泄漏检测
    bool enable_double_free_check;  // 启用双重释放检测
    bool enable_corruption_check;   // 启用内存损坏检测
    
    size_t front_guard_size;        // 前保护区域大小（字节）
    size_t rear_guard_size;         // 后保护区域大小（字节）
    
    bool track_file_info;           // 跟踪文件信息（__FILE__, __LINE__）
    bool track_timestamp;           // 跟踪时间戳
    bool track_purpose;             // 跟踪分配目的
    
    const char* log_file;           // 日志文件路径（NULL=标准错误）
} Stru_CN_DebugAllocatorConfig_t;
```

### 默认配置
```c
#define CN_DEBUG_ALLOCATOR_DEFAULT_CONFIG \
{ \
    .enable_boundary_check = true, \
    .enable_leak_detection = true, \
    .enable_double_free_check = true, \
    .enable_corruption_check = true, \
    .front_guard_size = 8, \
    .rear_guard_size = 8, \
    .track_file_info = true, \
    .track_timestamp = true, \
    .track_purpose = true, \
    .log_file = NULL \
}
```

## API参考

详细API文档请参考：[CN_debug_allocator_API.md](../../../docs/api/CN_debug_allocator_API.md)

### 主要接口函数
- `F_debug_allocator_create()` - 创建调试分配器
- `F_debug_allocator_destroy()` - 销毁调试分配器
- `F_debug_allocator_malloc()` - 分配内存
- `F_debug_allocator_free()` - 释放内存
- `F_debug_allocator_calloc()` - 分配并清零内存
- `F_debug_allocator_realloc()` - 重新分配内存

### 工具函数
- `F_debug_allocator_validate_heap()` - 验证堆完整性
- `F_debug_allocator_dump_stats()` - 转储统计信息
- `F_debug_allocator_report_leaks()` - 报告内存泄漏
- `F_debug_allocator_get_config()` - 获取当前配置
- `F_debug_allocator_set_config()` - 更新配置

## 编译选项

### CN_DEBUG_MEMORY
控制调试分配器的编译开关。

**启用调试功能：**
```bash
gcc -DCN_DEBUG_MEMORY -g -o myapp main.c
```

**禁用调试功能（生产环境）：**
```bash
gcc -O2 -o myapp main.c
```

### 效果
- **定义CN_DEBUG_MEMORY时**：调试分配器功能完全启用
- **未定义CN_DEBUG_MEMORY时**：调试分配器被替换为系统分配器，无性能开销

## 使用示例

### 示例1：基本内存操作
```c
#include "CN_debug_allocator.h"

void test_basic_operations()
{
    Stru_CN_DebugAllocator_t* allocator = F_debug_allocator_create(NULL);
    
    // 分配内存
    int* array = (int*)F_debug_allocator_malloc(allocator, 
        sizeof(int) * 10, __FILE__, __LINE__, "整数数组");
    
    // 使用内存
    for (int i = 0; i < 10; i++) {
        array[i] = i * i;
    }
    
    // 重新分配内存
    array = (int*)F_debug_allocator_realloc(allocator, array, 
        sizeof(int) * 20, __FILE__, __LINE__, "扩展的整数数组");
    
    // 释放内存
    F_debug_allocator_free(allocator, array);
    
    // 验证堆完整性
    bool valid = F_debug_allocator_validate_heap(allocator);
    printf("堆完整性: %s\n", valid ? "正常" : "损坏");
    
    // 报告泄漏
    F_debug_allocator_report_leaks(allocator);
    
    F_debug_allocator_destroy(allocator);
}
```

### 示例2：边界检查
```c
#include "CN_debug_allocator.h"

void test_boundary_check()
{
    Stru_CN_DebugAllocatorConfig_t config = CN_DEBUG_ALLOCATOR_DEFAULT_CONFIG;
    config.enable_boundary_check = true;
    config.front_guard_size = 16;
    config.rear_guard_size = 16;
    
    Stru_CN_DebugAllocator_t* allocator = F_debug_allocator_create(&config);
    
    char* buffer = (char*)F_debug_allocator_malloc(allocator, 
        100, __FILE__, __LINE__, "测试缓冲区");
    
    // 正常使用
    strcpy(buffer, "正常数据");
    
    // 缓冲区溢出（会被检测到）
    // buffer[100] = 'X';  // 错误：写入超出分配范围
    
    // 缓冲区下溢（会被检测到）
    // buffer[-1] = 'Y';   // 错误：写入分配开始之前
    
    F_debug_allocator_free(allocator, buffer);
    F_debug_allocator_destroy(allocator);
}
```

### 示例3：泄漏检测
```c
#include "CN_debug_allocator.h"

void test_leak_detection()
{
    Stru_CN_DebugAllocator_t* allocator = F_debug_allocator_create(NULL);
    
    // 故意制造泄漏
    void* leaked1 = F_debug_allocator_malloc(allocator, 
        50, __FILE__, __LINE__, "泄漏的内存1");
    void* leaked2 = F_debug_allocator_malloc(allocator, 
        100, __FILE__, __LINE__, "泄漏的内存2");
    
    // 只释放一个
    F_debug_allocator_free(allocator, leaked1);
    
    // 报告泄漏（会显示leaked2未释放）
    F_debug_allocator_report_leaks(allocator);
    
    // 注意：这里故意不释放leaked2以演示泄漏检测
    F_debug_allocator_destroy(allocator);
}
```

## 性能考虑

### 内存开销
调试分配器相比普通分配器有额外的内存开销：
- **头部信息**：每个分配约64字节
- **保护区域**：前后各8字节（可配置）
- **跟踪数据**：文件信息、时间戳、目的字符串等

### 时间开销
- **分配/释放操作**：比系统分配器慢2-5倍
- **边界检查**：每次分配/释放时验证保护区域
- **统计更新**：维护各种计数器

### 生产环境建议
1. **开发阶段**：启用所有调试功能
2. **测试阶段**：启用边界检查和泄漏检测
3. **生产环境**：禁用所有调试功能（使用系统分配器）

## 错误处理

### 检测到的错误类型
1. **边界违规**：缓冲区溢出/下溢
2. **双重释放**：重复释放同一内存块
3. **内存泄漏**：分配后未释放
4. **内存损坏**：保护区域或头部数据被修改
5. **无效指针**：释放非分配器管理的内存

### 错误响应
1. **立即报告**：检测到错误时立即输出详细信息
2. **堆栈跟踪**：记录错误发生时的调用堆栈
3. **程序继续**：大多数错误不会终止程序（可配置）
4. **统计记录**：所有错误被记录到统计信息中

## 最佳实践

### 开发阶段
1. **始终启用调试分配器**：在开发环境中默认使用调试分配器
2. **定期检查泄漏**：在关键点调用`F_debug_allocator_report_leaks()`
3. **验证堆完整性**：在复杂操作前后验证堆状态
4. **使用有意义的目的字符串**：帮助识别泄漏来源

### 测试阶段
1. **压力测试**：测试大量分配/释放操作
2. **边界测试**：故意触发边界违规以验证检测功能
3. **泄漏测试**：验证泄漏检测的准确性
4. **性能测试**：评估调试功能对性能的影响

### 生产环境
1. **禁用调试功能**：通过条件编译移除调试代码
2. **保留基本检查**：可保留基本的边界检查（如果性能允许）
3. **监控内存使用**：使用轻量级监控替代完整调试

## 限制和约束

### 平台限制
1. **内存对齐**：保证至少8字节对齐
2. **线程安全**：基础实现提供基本线程安全
3. **最大分配大小**：受系统内存限制
4. **最小分配大小**：至少1字节

### 功能限制
1. **性能开销**：调试功能有显著性能开销
2. **内存开销**：每个分配有额外内存开销
3. **兼容性**：需要与CN_Language内存系统集成使用
4. **可移植性**：主要支持Windows和Linux平台

## 故障排除

### 常见问题

#### Q1：调试分配器检测到边界违规，但程序没有崩溃
A：这是正常行为。调试分配器检测到错误时会报告但不会立即崩溃，以便收集更多调试信息。可以通过配置使检测到错误时立即终止程序。

#### Q2：泄漏报告显示误报
A：确保所有分配都通过对应的释放函数释放。检查是否有内存被其他分配器管理。

#### Q3：性能下降明显
A：调试分配器有性能开销。在性能关键路径考虑禁用部分调试功能或使用生产环境配置。

#### Q4：内存使用量显著增加
A：调试分配器有内存开销。每个分配都有头部和保护区域。可以通过减少保护区域大小来降低开销。

### 调试技巧
1. **使用详细日志**：启用文件信息和时间戳跟踪
2. **定期验证堆**：在怀疑有问题的地方调用`F_debug_allocator_validate_heap()`
3. **分析统计信息**：使用`F_debug_allocator_dump_stats()`了解内存使用模式
4. **逐步启用功能**：先启用基本功能，逐步添加复杂检查

## 相关文档

- [CN_debug_allocator_API.md](../../../docs/api/CN_debug_allocator_API.md) - 完整API文档
- [CN_memory_API.md](../../../docs/api/CN_memory_API.md) - 内存系统API文档
- [架构设计原则.md](../../../docs/architecture/架构设计原则.md) - 项目架构原则
- [技术规范和编码标准.md](../../../docs/specifications/CN_Language项目%20技术规范和编码标准.md) - 编码标准

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-02 | 初始版本，包含边界检查、泄漏检测、双重释放检测 |
| 1.1.0 | 2026-01-02 | 添加内存损坏检测和统计功能 |
| 1.2.0 | 2026-01-02 | 集成到CN_Language内存系统 |

## 维护者

CN_Language项目内存管理团队

## 许可证

MIT许可证
