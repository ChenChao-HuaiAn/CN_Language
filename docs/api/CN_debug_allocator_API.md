# CN_debug_allocator API 文档

## 概述

CN_debug_allocator是CN_Language项目的调试内存分配器模块，提供高级内存调试功能，包括边界检查、泄漏检测、双重释放检测和内存损坏检测。该模块遵循项目的分层架构原则，属于基础设施层的内存管理组件。

## 头文件引用

```c
#include "CN_debug_allocator.h"
```

## 数据结构

### Stru_CN_DebugAllocatorConfig_t
调试分配器配置结构体，用于配置调试功能和行为。

```c
typedef struct Stru_CN_DebugAllocatorConfig_t
{
    // 调试功能开关
    bool enable_boundary_check;     // 启用边界检查
    bool enable_leak_detection;     // 启用泄漏检测
    bool enable_double_free_check;  // 启用双重释放检测
    bool enable_corruption_check;   // 启用内存损坏检测
    
    // 保护区域配置
    size_t front_guard_size;        // 前保护区域大小（字节）
    size_t rear_guard_size;         // 后保护区域大小（字节）
    
    // 跟踪配置
    bool track_file_info;           // 跟踪文件信息（__FILE__, __LINE__）
    bool track_timestamp;           // 跟踪时间戳
    bool track_purpose;             // 跟踪分配目的
    
    // 日志配置
    const char* log_file;           // 日志文件路径（NULL=标准错误）
} Stru_CN_DebugAllocatorConfig_t;
```

### Stru_CN_DebugAllocatorStats_t
调试分配器统计信息结构体，记录分配器的运行统计。

```c
typedef struct Stru_CN_DebugAllocatorStats_t
{
    // 分配统计
    size_t total_allocations;       // 总分配次数
    size_t total_deallocations;     // 总释放次数
    size_t current_allocations;     // 当前活跃分配数
    size_t peak_allocations;        // 峰值活跃分配数
    
    // 内存使用统计
    size_t total_allocated_bytes;   // 总分配字节数
    size_t total_freed_bytes;       // 总释放字节数
    size_t current_used_bytes;      // 当前使用字节数
    size_t peak_used_bytes;         // 峰值使用字节数
    
    // 错误统计
    size_t boundary_violations;     // 边界违规次数
    size_t double_free_errors;      // 双重释放错误次数
    size_t corruption_errors;       // 内存损坏错误次数
    size_t invalid_pointer_errors;  // 无效指针错误次数
    
    // 性能统计
    uint64_t total_alloc_time_ns;   // 总分配时间（纳秒）
    uint64_t total_free_time_ns;    // 总释放时间（纳秒）
    uint64_t avg_alloc_time_ns;     // 平均分配时间（纳秒）
    uint64_t avg_free_time_ns;      // 平均释放时间（纳秒）
} Stru_CN_DebugAllocatorStats_t;
```

### Stru_CN_DebugAllocator_t
调试分配器句柄类型（不透明指针）。

```c
typedef struct Stru_CN_DebugAllocator_t Stru_CN_DebugAllocator_t;
```

### Stru_CN_AllocationInfo_t
分配信息结构体，记录单个内存分配的详细信息。

```c
typedef struct Stru_CN_AllocationInfo_t
{
    void* address;                  // 分配的内存地址
    size_t size;                    // 分配的大小（字节）
    size_t actual_size;             // 实际分配的大小（包括头部和保护区域）
    
    // 跟踪信息
    const char* file;               // 分配所在的源文件
    int line;                       // 分配所在的行号
    const char* purpose;            // 分配目的描述
    uint64_t timestamp;             // 分配时间戳（纳秒）
    
    // 状态信息
    bool is_freed;                  // 是否已释放
    uint64_t free_timestamp;        // 释放时间戳（如果已释放）
} Stru_CN_AllocationInfo_t;
```

## 常量定义

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

### 魔术字常量
```c
#define CN_DEBUG_ALLOCATOR_MAGIC_NORMAL   0xDEADBEEF  // 正常内存块魔术字
#define CN_DEBUG_ALLOCATOR_MAGIC_FREED    0xFREEDEAD  // 已释放内存块魔术字
#define CN_DEBUG_ALLOCATOR_MAGIC_CORRUPT  0xBADDC0DE  // 损坏内存块魔术字
#define CN_DEBUG_ALLOCATOR_GUARD_PATTERN  0xCAFEBABE  // 保护区域填充模式
```

### 错误码
```c
typedef enum Eum_DebugAllocatorError_t
{
    Eum_DEBUG_ALLOCATOR_SUCCESS = 0,           // 操作成功
    Eum_DEBUG_ALLOCATOR_INVALID_ARGUMENT = 1,  // 无效参数
    Eum_DEBUG_ALLOCATOR_OUT_OF_MEMORY = 2,     // 内存不足
    Eum_DEBUG_ALLOCATOR_CORRUPTION_DETECTED = 3, // 检测到内存损坏
    Eum_DEBUG_ALLOCATOR_DOUBLE_FREE = 4,       // 双重释放
    Eum_DEBUG_ALLOCATOR_INVALID_POINTER = 5,   // 无效指针
    Eum_DEBUG_ALLOCATOR_BOUNDARY_VIOLATION = 6 // 边界违规
} Eum_DebugAllocatorError_t;
```

## 核心接口函数

### F_debug_allocator_create
创建调试分配器实例。

**函数签名：**
```c
Stru_CN_DebugAllocator_t* F_debug_allocator_create(
    const Stru_CN_DebugAllocatorConfig_t* config);
```

**参数：**
- `config` - 分配器配置，传入NULL使用默认配置

**返回值：**
- 成功：返回调试分配器实例指针
- 失败：返回NULL

**说明：**
- 创建新的调试分配器实例
- 如果config为NULL，使用CN_DEBUG_ALLOCATOR_DEFAULT_CONFIG
- 初始化内部数据结构、统计信息和日志系统
- 分配失败时返回NULL并输出错误信息

**示例：**
```c
// 使用默认配置
Stru_CN_DebugAllocator_t* allocator1 = F_debug_allocator_create(NULL);

// 使用自定义配置
Stru_CN_DebugAllocatorConfig_t config = CN_DEBUG_ALLOCATOR_DEFAULT_CONFIG;
config.enable_boundary_check = true;
config.front_guard_size = 16;
Stru_CN_DebugAllocator_t* allocator2 = F_debug_allocator_create(&config);
```

### F_debug_allocator_destroy
销毁调试分配器实例。

**函数签名：**
```c
void F_debug_allocator_destroy(Stru_CN_DebugAllocator_t* allocator);
```

**参数：**
- `allocator` - 要销毁的调试分配器实例

**说明：**
- 销毁调试分配器实例及其所有资源
- 自动生成内存泄漏报告（如果启用泄漏检测）
- 验证堆完整性（如果启用损坏检测）
- 输出最终统计信息
- 释放所有内部数据结构

**示例：**
```c
Stru_CN_DebugAllocator_t* allocator = F_debug_allocator_create(NULL);
// ... 使用分配器 ...
F_debug_allocator_destroy(allocator);  // 自动清理并报告泄漏
```

## 内存操作函数

### F_debug_allocator_malloc
分配指定大小的内存。

**函数签名：**
```c
void* F_debug_allocator_malloc(
    Stru_CN_DebugAllocator_t* allocator,
    size_t size,
    const char* file,
    int line,
    const char* purpose);
```

**参数：**
- `allocator` - 调试分配器实例
- `size` - 要分配的内存大小（字节）
- `file` - 源文件名（通常使用__FILE__）
- `line` - 行号（通常使用__LINE__）
- `purpose` - 分配目的描述（可选）

**返回值：**
- 成功：返回分配的内存指针
- 失败：返回NULL

**说明：**
- 分配指定大小的内存
- 添加保护区域（如果启用边界检查）
- 记录分配信息（如果启用跟踪）
- 更新统计信息
- 验证保护区域完整性（如果启用边界检查）
- 内存对齐至少8字节

**示例：**
```c
int* ptr = (int*)F_debug_allocator_malloc(
    allocator, 
    sizeof(int) * 100,
    __FILE__, 
    __LINE__,
    "整数数组");
```

### F_debug_allocator_calloc
分配并清零指定数量和大小的内存。

**函数签名：**
```c
void* F_debug_allocator_calloc(
    Stru_CN_DebugAllocator_t* allocator,
    size_t num,
    size_t size,
    const char* file,
    int line,
    const char* purpose);
```

**参数：**
- `allocator` - 调试分配器实例
- `num` - 元素数量
- `size` - 每个元素的大小（字节）
- `file` - 源文件名
- `line` - 行号
- `purpose` - 分配目的描述

**返回值：**
- 成功：返回分配并清零的内存指针
- 失败：返回NULL

**说明：**
- 分配num * size字节的内存
- 将分配的内存清零
- 其他行为与F_debug_allocator_malloc相同

**示例：**
```c
int* zeros = (int*)F_debug_allocator_calloc(
    allocator,
    50,                     // 50个元素
    sizeof(int),            // 每个元素4字节
    __FILE__,
    __LINE__,
    "清零的整数数组");
```

### F_debug_allocator_realloc
重新分配内存块。

**函数签名：**
```c
void* F_debug_allocator_realloc(
    Stru_CN_DebugAllocator_t* allocator,
    void* ptr,
    size_t new_size,
    const char* file,
    int line,
    const char* purpose);
```

**参数：**
- `allocator` - 调试分配器实例
- `ptr` - 要重新分配的内存指针（可为NULL）
- `new_size` - 新的内存大小（字节）
- `file` - 源文件名
- `line` - 行号
- `purpose` - 重新分配目的描述

**返回值：**
- 成功：返回重新分配的内存指针
- 失败：返回NULL（原指针保持不变）

**说明：**
- 如果ptr为NULL，行为等同于F_debug_allocator_malloc
- 如果new_size为0，行为等同于F_debug_allocator_free（返回NULL）
- 验证原内存块的完整性（如果启用检查）
- 复制原数据到新内存块（如果缩小，可能丢失部分数据）
- 更新跟踪信息

**示例：**
```c
// 扩展数组
array = (int*)F_debug_allocator_realloc(
    allocator,
    array,                  // 原指针
    sizeof(int) * 200,      // 新大小
    __FILE__,
    __LINE__,
    "扩展的整数数组");
```

### F_debug_allocator_free
释放分配的内存。

**函数签名：**
```c
Eum_DebugAllocatorError_t F_debug_allocator_free(
    Stru_CN_DebugAllocator_t* allocator,
    void* ptr);
```

**参数：**
- `allocator` - 调试分配器实例
- `ptr` - 要释放的内存指针

**返回值：**
- 操作结果错误码

**说明：**
- 释放之前分配的内存
- 验证内存块完整性（如果启用检查）
- 检测双重释放（如果启用检查）
- 更新统计信息
- 标记内存块为已释放状态
- 保留释放信息用于泄漏报告

**示例：**
```c
Eum_DebugAllocatorError_t error = F_debug_allocator_free(allocator, ptr);
if (error != Eum_DEBUG_ALLOCATOR_SUCCESS) {
    printf("释放失败: %d\n", error);
}
```

### F_debug_allocator_aligned_alloc
分配对齐的内存。

**函数签名：**
```c
void* F_debug_allocator_aligned_alloc(
    Stru_CN_DebugAllocator_t* allocator,
    size_t alignment,
    size_t size,
    const char* file,
    int line,
    const char* purpose);
```

**参数：**
- `allocator` - 调试分配器实例
- `alignment` - 对齐要求（必须是2的幂）
- `size` - 要分配的内存大小
- `file` - 源文件名
- `line` - 行号
- `purpose` - 分配目的描述

**返回值：**
- 成功：返回对齐分配的内存指针
- 失败：返回NULL

**说明：**
- 分配指定对齐要求的内存
- 对齐值必须是2的幂
- 其他行为与F_debug_allocator_malloc相同

**示例：**
```c
// 分配16字节对齐的内存
void* aligned = F_debug_allocator_aligned_alloc(
    allocator,
    16,                     // 16字节对齐
    1024,                   // 1024字节
    __FILE__,
    __LINE__,
    "对齐的内存块");
```

## 查询和配置函数

### F_debug_allocator_get_config
获取当前配置。

**函数签名：**
```c
const Stru_CN_DebugAllocatorConfig_t* F_debug_allocator_get_config(
    const Stru_CN_DebugAllocator_t* allocator);
```

**参数：**
- `allocator` - 调试分配器实例

**返回值：**
- 当前配置的只读指针

**说明：**
- 返回分配器的当前配置
- 返回的指针是只读的，不应修改
- 可用于检查当前设置或创建新配置

**示例：**
```c
const Stru_CN_DebugAllocatorConfig_t* config = 
    F_debug_allocator_get_config(allocator);
printf("边界检查: %s\n", config->enable_boundary_check ? "启用" : "禁用");
```

### F_debug_allocator_set_config
更新配置。

**函数签名：**
```c
Eum_DebugAllocatorError_t F_debug_allocator_set_config(
    Stru_CN_DebugAllocator_t* allocator,
    const Stru_CN_DebugAllocatorConfig_t* config);
```

**参数：**
- `allocator` - 调试分配器实例
- `config` - 新的配置

**返回值：**
- 操作结果错误码

**说明：**
- 更新分配器的配置
- 某些配置更改可能需要重新初始化内部结构
- 不会影响已分配的内存块
- 验证配置的有效性

**示例：**
```c
Stru_CN_DebugAllocatorConfig_t new_config = *F_debug_allocator_get_config(allocator);
new_config.enable_boundary_check = false;  // 禁用边界检查以提高性能
Eum_DebugAllocatorError_t error = F_debug_allocator_set_config(allocator, &new_config);
```

### F_debug_allocator_get_stats
获取统计信息。

**函数签名：**
```c
const Stru_CN_DebugAllocatorStats_t* F_debug_allocator_get_stats(
    const Stru_CN_DebugAllocator_t* allocator);
```

**参数：**
- `allocator` - 调试分配器实例

**返回值：**
- 当前统计信息的只读指针

**说明：**
- 返回分配器的当前统计信息
- 统计信息实时更新
- 返回的指针是只读的，不应修改

**示例：**
```c
const Stru_CN_DebugAllocatorStats_t* stats = 
    F_debug_allocator_get_stats(allocator);
printf("当前使用内存: %zu 字节\n", stats->current_used_bytes);
printf("总分配次数: %zu\n", stats->total_allocations);
```

### F_debug_allocator_reset_stats
重置统计信息。

**函数签名：**
```c
void F_debug_allocator_reset_stats(Stru_CN_DebugAllocator_t* allocator);
```

**参数：**
- `allocator` - 调试分配器实例

**说明：**
- 重置所有统计计数器为零
- 不影响已分配的内存
- 可用于性能测试或阶段性统计

**示例：**
```c
// 测试前重置统计
F_debug_allocator_reset_stats(allocator);
// ... 执行测试 ...
const Stru_CN_DebugAllocatorStats_t* stats = F_debug_allocator_get_stats(allocator);
printf("测试期间分配次数: %zu\n", stats->total_allocations);
```

## 验证和调试函数

### F_debug_allocator_validate_heap
验证堆完整性。

**函数签名：**
```c
bool F_debug_allocator_validate_heap(const Stru_CN_DebugAllocator_t* allocator);
```

**参数：**
- `allocator` - 调试分配器实例

**返回值：**
- true: 堆完整性正常
- false: 检测到堆损坏

**说明：**
- 验证所有已分配内存块的完整性
- 检查保护区域（如果启用边界检查）
- 验证魔术字（如果启用损坏检测）
- 检测内存损坏和边界违规
- 输出详细的错误信息到日志

**示例：**
```c
bool heap_ok = F_debug_allocator_validate_heap(allocator);
if (!heap_ok) {
    printf("警告：检测到堆损坏\n");
}
```

### F_debug_allocator_validate_block
验证单个内存块的完整性。

**函数签名：**
```c
bool F_debug_allocator_validate_block(
    const Stru_CN_DebugAllocator_t* allocator,
    const void* ptr);
```

**参数：**
- `allocator` - 调试分配器实例
- `ptr` - 要验证的内存块指针

**返回值：**
- true: 内存块完整性正常
- false: 检测到内存块损坏

**说明：**
- 验证指定内存块的完整性
- 检查保护区域（如果启用边界检查）
- 验证魔术字和头部信息
- 检测缓冲区溢出/下溢
- 输出详细的错误信息到日志

**示例：**
```c
bool block_ok = F_debug_allocator_validate_block(allocator, ptr);
if (!block_ok) {
    printf("内存块损坏，地址: %p\n", ptr);
}
```

### F_debug_allocator_dump_stats
转储统计信息到日志。

**函数签名：**
```c
void F_debug_allocator_dump_stats(const Stru_CN_DebugAllocator_t* allocator);
```

**参数：**
- `allocator` - 调试分配器实例

**说明：**
- 输出详细的统计信息到日志
- 包括分配统计、内存使用统计、错误统计和性能统计
- 格式化输出，便于阅读和分析
- 可用于性能分析和调试

**示例：**
```c
// 在关键点转储统计信息
F_debug_allocator_dump_stats(allocator);
```

### F_debug_allocator_report_leaks
报告内存泄漏。

**函数签名：**
```c
void F_debug_allocator_report_leaks(const Stru_CN_DebugAllocator_t* allocator);
```

**参数：**
- `allocator` - 调试分配器实例

**说明：**
- 报告所有未释放的内存分配
- 输出详细的泄漏信息，包括地址、大小、分配位置等
- 按泄漏大小排序输出
- 计算总泄漏字节数
- 在F_debug_allocator_destroy中自动调用

**示例：**
```c
// 手动检查泄漏
F_debug_allocator_report_leaks(allocator);
```

### F_debug_allocator_get_allocation_info
获取指定内存块的分配信息。

**函数签名：**
```c
const Stru_CN_AllocationInfo_t* F_debug_allocator_get_allocation_info(
    const Stru_CN_DebugAllocator_t* allocator,
    const void* ptr);
```

**参数：**
- `allocator` - 调试分配器实例
- `ptr` - 内存块指针

**返回值：**
- 成功：返回分配信息的只读指针
- 失败：返回NULL（指针无效或未跟踪）

**说明：**
- 获取内存块的详细分配信息
- 包括大小、分配位置、时间戳、目的等
- 返回的指针是只读的，不应修改
- 需要启用相应的跟踪功能

**示例：**
```c
const Stru_CN_AllocationInfo_t* info = 
    F_debug_allocator_get_allocation_info(allocator, ptr);
if (info != NULL) {
    printf("分配大小: %zu 字节\n", info->size);
    printf("分配位置: %s:%d\n", info->file, info->line);
}
```

### F_debug_allocator_iterate_allocations
迭代所有活跃的内存分配。

**函数签名：**
```c
void F_debug_allocator_iterate_allocations(
    const Stru_CN_DebugAllocator_t* allocator,
    void (*callback)(const Stru_CN_AllocationInfo_t* info, void* user_data),
    void* user_data);
```

**参数：**
- `allocator` - 调试分配器实例
- `callback` - 回调函数，对每个分配调用
- `user_data` - 传递给回调函数的用户数据

**说明：**
- 遍历所有活跃的内存分配
- 对每个分配调用指定的回调函数
- 可用于自定义分析或报告
- 回调函数不应修改分配信息

**示例：**
```c
void print_allocation(const Stru_CN_AllocationInfo_t* info, void* user_data)
{
    printf("分配: %p, 大小: %zu, 文件: %s:%d\n",
           info->address, info->size, info->file, info->line);
}

// 打印所有活跃分配
F_debug_allocator_iterate_allocations(allocator, print_allocation, NULL);
```

### F_debug_allocator_set_log_level
设置日志级别。

**函数签名：**
```c
void F_debug_allocator_set_log_level(
    Stru_CN_DebugAllocator_t* allocator,
    int log_level);
```

**参数：**
- `allocator` - 调试分配器实例
- `log_level` - 日志级别（0=无日志，1=错误，2=警告，3=信息，4=调试）

**说明：**
- 控制调试分配器的日志输出详细程度
- 较高的日志级别会产生更多输出
- 默认级别为2（警告）

**示例：**
```c
// 启用详细日志
F_debug_allocator_set_log_level(allocator, 4);  // 调试级别
```

## 使用示例

### 完整示例程序

```c
/******************************************************************************
 * CN_debug_allocator使用示例
 ******************************************************************************/

#include "CN_debug_allocator.h"
#include <stdio.h>
#include <string.h>

int main()
{
    // 创建调试分配器（使用默认配置）
    Stru_CN_DebugAllocator_t* allocator = F_debug_allocator_create(NULL);
    if (allocator == NULL) {
        fprintf(stderr, "无法创建调试分配器\n");
        return 1;
    }
    
    // 设置详细日志
    F_debug_allocator_set_log_level(allocator, 3);  // 信息级别
    
    printf("=== 测试基本内存操作 ===\n");
    
    // 分配内存
    int* numbers = (int*)F_debug_allocator_malloc(
        allocator,
        sizeof(int) * 10,
        __FILE__,
        __LINE__,
        "整数数组");
    
    if (numbers == NULL) {
        fprintf(stderr, "内存分配失败\n");
        F_debug_allocator_destroy(allocator);
        return 1;
    }
    
    // 使用内存
    for (int i = 0; i < 10; i++) {
        numbers[i] = i * i;
    }
    
    // 验证内存块
    bool valid = F_debug_allocator_validate_block(allocator, numbers);
    printf("内存块验证: %s\n", valid ? "通过" : "失败");
    
    // 重新分配内存
    numbers = (int*)F_debug_allocator_realloc(
        allocator,
        numbers,
        sizeof(int) * 20,
        __FILE__,
        __LINE__,
        "扩展的整数数组");
    
    // 分配并清零内存
    char* buffer = (char*)F_debug_allocator_calloc(
        allocator,
        100,
        sizeof(char),
        __FILE__,
        __LINE__,
        "清零的缓冲区");
    
    strcpy(buffer, "Hello, Debug Allocator!");
    
    printf("=== 验证堆完整性 ===\n");
    
    // 验证整个堆
    bool heap_ok = F_debug_allocator_validate_heap(allocator);
    printf("堆完整性: %s\n", heap_ok ? "正常" : "损坏");
    
    printf("=== 显示统计信息 ===\n");
    
    // 转储统计信息
    F_debug_allocator_dump_stats(allocator);
    
    printf("=== 测试泄漏检测 ===\n");
    
    // 故意制造泄漏
    void* leaked = F_debug_allocator_malloc(
        allocator,
        50,
        __FILE__,
        __LINE__,
        "故意泄漏的内存");
    
    // 报告泄漏（会显示leaked未释放）
    F_debug_allocator_report_leaks(allocator);
    
    printf("=== 清理 ===\n");
    
    // 释放内存（除了故意泄漏的）
    F_debug_allocator_free(allocator, numbers);
    F_debug_allocator_free(allocator, buffer);
    
    // 销毁分配器（会自动报告泄漏）
    F_debug_allocator_destroy(allocator);
    
    return 0;
}
```

### 集成到现有项目

```c
#include "CN_debug_allocator.h"

// 全局调试分配器实例
static Stru_CN_DebugAllocator_t* g_debug_allocator = NULL;

void init_debug_memory_system()
{
    Stru_CN_DebugAllocatorConfig_t config = CN_DEBUG_ALLOCATOR_DEFAULT_CONFIG;
    
    // 根据编译配置调整
    #ifdef PRODUCTION_BUILD
    config.enable_boundary_check = false;
    config.enable_leak_detection = false;
    config.track_file_info = false;
    #endif
    
    g_debug_allocator = F_debug_allocator_create(&config);
}

void* debug_malloc(size_t size, const char* file, int line, const char* purpose)
{
    if (g_debug_allocator == NULL) {
        init_debug_memory_system();
    }
    
    return F_debug_allocator_malloc(g_debug_allocator, size, file, line, purpose);
}

void debug_free(void* ptr)
{
    if (g_debug_allocator != NULL && ptr != NULL) {
        F_debug_allocator_free(g_debug_allocator, ptr);
    }
}

void shutdown_debug_memory_system()
{
    if (g_debug_allocator != NULL) {
        // 报告最终泄漏
        F_debug_allocator_report_leaks(g_debug_allocator);
        
        // 显示最终统计
        F_debug_allocator_dump_stats(g_debug_allocator);
        
        F_debug_allocator_destroy(g_debug_allocator);
        g_debug_allocator = NULL;
    }
}

// 包装宏
#ifdef CN_DEBUG_MEMORY
#define DEBUG_MALLOC(size) debug_malloc(size, __FILE__, __LINE__, #size)
#define DEBUG_FREE(ptr) debug_free(ptr)
#else
#define DEBUG_MALLOC(size) malloc(size)
#define DEBUG_FREE(ptr) free(ptr)
#endif
```

## 编译选项

### CN_DEBUG_MEMORY
控制调试分配器的编译开关。

**启用调试功能：**
```bash
gcc -DCN_DEBUG_MEMORY -g -o myapp main.c src/infrastructure/memory/debug/*.c
```

**禁用调试功能（生产环境）：**
```bash
gcc -O2 -o myapp main.c
```

### 效果
- **定义CN_DEBUG_MEMORY时**：调试分配器功能完全启用，所有调试代码被编译
- **未定义CN_DEBUG_MEMORY时**：调试分配器函数被替换为系统分配器函数，无性能开销

### 条件编译示例

```c
// CN_debug_allocator.h中的条件编译
#ifdef CN_DEBUG_MEMORY

// 调试版本：使用完整的调试分配器
Stru_CN_DebugAllocator_t* F_debug_allocator_create(
    const Stru_CN_DebugAllocatorConfig_t* config);

#else

// 生产版本：简化为系统分配器包装
static inline void* F_debug_allocator_create(
    const Stru_CN_DebugAllocatorConfig_t* config)
{
    (void)config;  // 忽略参数
    return (Stru_CN_DebugAllocator_t*)1;  // 返回非NULL伪句柄
}

#endif
```

## 性能考虑

### 内存开销
调试分配器相比系统分配器有显著的内存开销：

| 组件 | 典型大小 | 说明 |
|------|----------|------|
| 内存块头部 | 64字节 | 包含魔术字、大小、跟踪信息等 |
| 前保护区域 | 8-16字节 | 检测缓冲区下溢 |
| 后保护区域 | 8-16字节 | 检测缓冲区溢出 |
| 跟踪数据 | 可变 | 文件信息、时间戳、目的字符串等 |
| **总开销** | **80-100+字节** | 每个分配 |

### 时间开销
调试操作会增加时间开销：

| 操作 | 相对开销 | 说明 |
|------|----------|------|
| 分配 | 2-5倍 | 添加保护区域、初始化头部、更新统计 |
| 释放 | 3-8倍 | 验证完整性、检查双重释放、更新统计 |
| 重新分配 | 3-6倍 | 验证原块、复制数据、更新跟踪 |
| 验证操作 | 额外开销 | 保护区域检查、魔术字验证等 |

### 优化建议
1. **开发阶段**：启用所有调试功能，关注正确性而非性能
2. **测试阶段**：根据测试类型调整配置（性能测试时禁用部分功能）
3. **生产环境**：通过条件编译完全禁用调试功能
4. **选择性启用**：对关键模块启用调试，其他模块使用系统分配器

## 错误处理

### 错误检测
调试分配器可以检测多种内存错误：

1. **边界违规**：写入保护区域
2. **双重释放**：重复释放同一内存块
3. **内存泄漏**：分配后未释放
4. **内存损坏**：魔术字被修改
5. **无效指针**：释放非分配器管理的内存
6. **使用已释放内存**：访问已标记为释放的内存

### 错误响应
检测到错误时的行为：

1. **立即报告**：输出详细的错误信息到日志
2. **堆栈信息**：记录错误发生时的调用堆栈（如果支持）
3. **统计记录**：更新错误统计计数器
4. **程序行为**：
   - 默认：报告错误但继续执行
   - 可配置：严重错误时终止程序
5. **错误恢复**：尽可能保持分配器状态稳定

### 错误信息格式
错误信息包含：
- 错误类型和描述
- 内存地址和大小
- 分配位置（文件、行号）
- 时间戳
- 相关统计信息

## 最佳实践

### 开发阶段
1. **始终启用调试分配器**：在开发环境中默认使用CN_DEBUG_MEMORY
2. **使用完整跟踪**：启用文件信息、时间戳和目的跟踪
3. **定期检查泄漏**：在关键点调用F_debug_allocator_report_leaks()
4. **验证堆完整性**：在复杂操作前后验证堆状态
5. **使用有意义的目的字符串**：帮助识别泄漏来源

### 测试阶段
1. **压力测试**：测试大量分配/释放操作下的稳定性
2. **边界测试**：故意触发边界违规以验证检测功能
3. **泄漏测试**：验证泄漏检测的准确性
4. **性能测试**：评估调试功能对性能的影响
5. **配置测试**：测试不同配置组合下的行为

### 生产环境
1. **条件编译**：通过CN_DEBUG_MEMORY开关禁用调试功能
2. **渐进部署**：先在小范围启用，验证稳定性后再扩大
3. **监控替代**：使用轻量级监控替代完整调试功能
4. **紧急启用**：保留在生产环境中临时启用调试的能力

### 集成建议
1. **统一接口**：通过包装函数或宏提供统一的内存接口
2. **配置管理**：集中管理分配器配置，便于调整
3. **日志集成**：将调试分配器日志集成到项目日志系统
4. **测试集成**：在单元测试和集成测试中使用调试分配器

## 限制和约束

### 平台限制
1. **内存对齐**：保证至少8字节对齐，可能不满足特定硬件要求
2. **线程安全**：基础实现提供基本线程安全，复杂场景需额外考虑
3. **最大分配大小**：受系统内存和内部数据结构限制
4. **最小分配大小**：至少1字节，但实际开销更大

### 功能限制
1. **性能开销**：调试功能有显著性能开销，不适合性能关键路径
2. **内存开销**：每个分配有额外内存开销，可能影响内存受限系统
3. **兼容性**：需要与项目的内存管理策略兼容
4. **可移植性**：主要支持Windows和Linux，其他平台可能需要适配

### 使用限制
1. **混合分配**：不应与其他分配器混合使用同一内存块
2. **长期运行**：长时间运行可能积累大量跟踪数据
3. **实时系统**：不适合硬实时系统（时间开销不可预测）
4. **嵌入式系统**：内存开销可能过大

## 故障排除

### 常见问题

#### Q1：调试分配器检测到边界违规，但程序没有崩溃
A：这是正常行为。调试分配器检测到错误时会报告但不会立即崩溃，以便收集更多调试信息。可以通过配置使检测到错误时立即终止程序。

#### Q2：泄漏报告显示误报
A：确保所有分配都通过对应的释放函数释放。检查是否有内存被其他分配器管理。如果使用多个分配器，确保正确跟踪每个分配。

#### Q3：性能下降明显
A：调试分配器有性能开销。在性能关键路径考虑禁用部分调试功能或使用生产环境配置。可以通过CN_DEBUG_MEMORY编译开关完全禁用调试功能。

#### Q4：内存使用量显著增加
A：调试分配器有内存开销。每个分配都有头部和保护区域。可以通过减少保护区域大小或禁用部分跟踪功能来降低开销。

#### Q5：调试分配器自身内存不足
A：调试分配器需要内存来管理跟踪数据。如果系统内存严重不足，调试分配器可能无法正常工作。考虑减少跟踪数据量或使用更简单的配置。

#### Q6：多线程环境下的问题
A：基础实现提供基本线程安全，但复杂多线程场景可能需要额外同步。确保正确使用分配器接口，避免竞态条件。

### 调试技巧

1. **使用详细日志**：启用文件信息和时间戳跟踪，设置较高的日志级别
2. **定期验证堆**：在怀疑有问题的地方调用`F_debug_allocator_validate_heap()`
3. **分析统计信息**：使用`F_debug_allocator_dump_stats()`了解内存使用模式
4. **逐步启用功能**：先启用基本功能，逐步添加复杂检查以定位问题
5. **使用目的字符串**：为每个分配提供有意义的描述，帮助识别泄漏来源
6. **隔离测试**：在最小化环境中复现问题，排除其他因素干扰

### 错误诊断步骤

1. **重现问题**：创建最小化测试用例复现问题
2. **启用完整调试**：确保所有调试功能都已启用
3. **检查日志**：查看调试分配器的日志输出
4. **验证堆完整性**：在关键点调用验证函数
5. **分析泄漏报告**：检查泄漏报告中的详细信息
6. **检查配置**：确保分配器配置符合预期
7. **简化环境**：排除其他库或框架的干扰

### 性能问题诊断

1. **基准测试**：建立性能基准，比较不同配置下的表现
2. **分析统计**：使用统计信息识别性能瓶颈
3. **配置调整**：根据性能需求调整配置选项
4. **选择性启用**：只对问题模块启用调试功能
5. **生产环境对比**：与生产环境配置对比性能差异

## 相关文档

### 项目文档
- [CN_Language项目 技术规范和编码标准.md](../../specifications/CN_Language项目%20技术规范和编码标准.md) - 项目编码标准
- [CN_Language项目 目录结构规范.md](../../specifications/CN_Language项目%20目录结构规范.md) - 项目目录结构
- [架构设计原则.md](../../architecture/架构设计原则.md) - 项目架构原则
- [模块依赖规范.md](../../architecture/模块依赖规范.md) - 模块依赖规范

### API文档
- [CN_memory_API.md](CN_memory_API.md) - 内存系统API文档
- [CN_system_allocator_API.md](CN_system_allocator_API.md) - 系统分配器API文档
- [CN_pool_allocator_API.md](CN_pool_allocator_API.md) - 池分配器API文档
- [CN_arena_allocator_API.md](CN_arena_allocator_API.md) - 区域分配器API文档

### 设计文档
- [中文编程语言CN_Language开发规划.md](../../design/中文编程语言CN_Language开发规划.md) - 项目开发规划
- [第一阶段：基础设施层重构详细实施计划（第1-3周）.md](../../design/第一阶段：基础设施层重构详细实施计划（第1-3周）.md) - 基础设施层实施计划

### 外部参考
- [C标准库内存管理函数](https://en.cppreference.com/w/c/memory) - C标准库内存函数参考
- [内存调试技术](https://en.wikipedia.org/wiki/Memory_debugger) - 内存调试技术概述
- [Valgrind工具集](http://valgrind.org/) - 著名内存调试工具

## 版本历史

### 版本 1.0.0 (2026-01-02)
- 初始版本发布
- 基本调试分配器框架
- 边界检查功能
- 泄漏检测功能
- 双重释放检测功能

### 版本 1.1.0 (2026-01-02)
- 添加内存损坏检测
- 增强统计信息功能
- 改进错误报告机制
- 添加配置管理接口

### 版本 1.2.0 (2026-01-02)
- 集成到CN_Language内存系统
- 添加条件编译支持
- 优化性能开销
- 完善API文档

### 版本 1.3.0 (计划中)
- 多线程增强支持
- 更高效的内存布局
- 远程调试支持
- 性能分析工具集成

## 维护者

CN_Language项目内存管理团队

### 核心贡献者
- 架构设计：CN_Language架构委员会
- 实现开发：基础设施层开发团队
- 测试验证：质量保证团队
- 文档编写：技术文档团队

### 联系方式
- 项目仓库：https://github.com/ChenChao-HuaiAn/CN_Language
- 问题反馈：通过GitHub Issues提交
- 讨论论坛：项目Wiki页面

## 许可证

MIT许可证

```
Copyright (c) 2026 CN_Language项目

特此免费授予任何获得本软件副本和相关文档文件（以下简称"软件"）的人士，
无限制地处理本软件的权利，包括但不限于使用、复制、修改、合并、发布、
分发、再许可和/或销售本软件的副本，并允许向其提供本软件的人士这样做，
但须符合以下条件：

上述版权声明和本许可声明应包含在本软件的所有副本或重要部分中。

本软件按"原样"提供，不提供任何形式的明示或暗示保证，包括但不限于
对适销性、特定用途适用性和非侵权性的保证。在任何情况下，作者或
版权持有人均不对因本软件或本软件的使用或其他交易而引起的任何索赔、
损害赔偿或其他责任承担责任，无论是在合同诉讼、侵权诉讼还是其他诉讼中。
```

## 致谢

感谢以下项目和资源的启发和参考：
- [Electric Fence](http://perens.com/FreeSoftware/ElectricFence/) - 边界检查分配器
- [dmalloc](http://dmalloc.com/) - 调试内存分配库
- [Valgrind](http://valgrind.org/) - 内存调试和分析工具
- [AddressSanitizer](https://github.com/google/sanitizers) - 地址消毒剂技术

## 未来计划

### 短期计划 (2026年Q1)
1. 性能优化和内存使用改进
2. 增强多线程支持
3. 添加更多调试功能
4. 完善测试覆盖

### 中期计划 (2026年Q2-Q3)
1. 远程调试和监控支持
2. 可视化分析工具
3. 与其他调试工具集成
4. 跨平台增强

### 长期计划 (2026年Q4及以后)
1. 机器学习辅助错误检测
2. 实时性能分析
3. 云原生调试支持
4. 自动化修复建议

---
*文档最后更新：2026年1月2日*
*API版本：1.2.0*
*架构版本：2.0.0*
