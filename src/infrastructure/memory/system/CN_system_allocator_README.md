# CN_Language 系统分配器模块

## 概述

系统分配器（System Allocator）是CN_Language内存管理模块的核心组件，提供对标准库malloc/free/realloc的安全包装和增强功能。本模块遵循SOLID设计原则和项目架构规范，提供安全、可监控、可调试的内存管理方案，适合通用内存分配场景。

## 设计理念

### 核心目标
- **安全性**：提供内存边界检查、泄漏检测和调试支持
- **可监控性**：详细的统计信息和运行时监控
- **兼容性**：与标准库完全兼容，可无缝替换
- **可配置性**：灵活的配置选项适应不同使用场景
- **高性能**：在保证安全的前提下最小化性能开销

### 架构原则
- **单一职责**：专注于系统内存分配的安全包装
- **接口隔离**：提供专用接口，避免功能混杂
- **依赖倒置**：通过抽象接口与上层模块交互
- **开闭原则**：支持扩展而不修改核心逻辑

## 功能特性

### 1. 核心功能
- **安全分配**：包装标准库分配函数，添加安全检查和统计
- **对齐支持**：支持任意对齐要求的内存分配
- **内存跟踪**：跟踪每次分配的信息（文件、行号、目的）
- **泄漏检测**：自动检测未释放的内存
- **统计收集**：收集详细的分配统计信息

### 2. 配置选项
- **统计启用**：是否启用统计信息收集
- **调试模式**：是否启用详细调试输出
- **内存清零**：分配时是否清零内存
- **模式填充**：调试模式下分配/释放时的内存填充
- **分配跟踪**：是否跟踪每次分配的信息
- **泄漏检测**：是否启用内存泄漏检测
- **堆验证**：是否验证堆完整性
- **大小限制**：单次分配和总分配的大小限制

### 3. 统计监控
- **使用统计**：分配次数、释放次数、重新分配次数
- **容量统计**：当前使用量、峰值使用量、总分配量
- **性能统计**：分配失败次数、内存开销
- **跟踪统计**：跟踪的分配数量、泄漏检测

### 4. 调试支持
- **调试输出**：详细的分配/释放调试信息
- **泄漏转储**：输出未释放的分配详细信息
- **状态转储**：输出完整的分配器状态
- **回调支持**：自定义调试信息输出回调
- **完整性验证**：验证内存块头部完整性

## 架构设计

### 模块结构
```
CN_system_allocator/
├── CN_system_allocator.h      # 公共接口定义
├── CN_system_internal.h       # 内部数据结构
├── CN_system_core.c           # 核心实现（创建、销毁、分配、释放）
├── CN_system_operations.c     # 高级操作和查询接口
├── CN_system_utils.c          # 工具函数和默认分配器
└── CN_system_allocator_README.md  # 模块文档
```

### 内部数据结构
```c
// 系统分配器内部结构
struct Stru_CN_SystemAllocator_t
{
    // 配置信息
    Stru_CN_SystemConfig_t config;
    
    // 统计信息
    Stru_CN_SystemStats_t stats;
    
    // 分配跟踪信息
    Stru_CN_AllocationInfo_t* allocations[CN_SYSTEM_MAX_TRACKED_ALLOCATIONS];
    size_t allocation_count;
    
    // 调试支持
    CN_SystemDebugCallback_t debug_callback;
    void* debug_user_data;
    
    // 线程安全
    void* mutex;  // 预留，当前版本不支持线程安全
};
```

### 内存管理策略
1. **头部信息**：每个分配都包含头部信息，记录大小、来源等元数据
2. **魔术字验证**：使用魔术字验证内存块完整性，防止重复释放
3. **跟踪记录**：可选地跟踪每次分配，便于调试和泄漏检测
4. **统计更新**：实时更新统计信息，监控内存使用情况
5. **调试填充**：调试模式下填充特定模式，便于检测内存错误

## 使用指南

### 基本使用
```c
#include "CN_system_allocator.h"

// 1. 创建系统分配器配置
Stru_CN_SystemConfig_t config = CN_SYSTEM_CONFIG_DEFAULT;
config.enable_statistics = true;    // 启用统计
config.track_allocations = true;    // 跟踪分配信息
config.detect_leaks = true;         // 检测泄漏
config.zero_on_alloc = false;       // 不清零内存（性能优先）

// 2. 创建系统分配器
Stru_CN_SystemAllocator_t* allocator = CN_system_create(&config);
if (allocator == NULL) {
    // 处理创建失败
    fprintf(stderr, "系统分配器创建失败\n");
    return;
}

// 3. 分配内存
void* ptr1 = CN_system_alloc(allocator, 256, __FILE__, __LINE__, "示例分配1");
void* ptr2 = CN_system_alloc_aligned(allocator, 512, 32, __FILE__, __LINE__, "对齐分配");

// 4. 重新分配内存
void* ptr3 = CN_system_realloc(allocator, ptr1, 1024, __FILE__, __LINE__, "重新分配");

// 5. 分配并清零内存
int* array = (int*)CN_system_calloc(allocator, 10, sizeof(int), __FILE__, __LINE__, "整数数组");

// 6. 使用内存
// ... 业务逻辑 ...

// 7. 释放内存
CN_system_free(allocator, ptr2, __FILE__, __LINE__);
CN_system_free(allocator, ptr3, __FILE__, __LINE__);
CN_system_free(allocator, array, __FILE__, __LINE__);

// 8. 检查泄漏
if (CN_system_check_leaks(allocator)) {
    printf("警告：检测到内存泄漏\n");
    CN_system_dump_leaks(allocator);
}

// 9. 查看统计信息
Stru_CN_SystemStats_t stats;
if (CN_system_get_stats(allocator, &stats)) {
    printf("当前使用: %zu 字节, 峰值使用: %zu 字节\n", 
           stats.current_usage, stats.peak_usage);
}

// 10. 销毁分配器
CN_system_destroy(allocator);
```

### 使用默认分配器（推荐）
```c
#include "CN_system_allocator.h"

// 获取默认系统分配器（全局单例）
Stru_CN_SystemAllocator_t* allocator = CN_system_get_default();

// 使用默认分配器分配内存
void* ptr = CN_system_alloc(allocator, 100, __FILE__, __LINE__, "测试分配");

// ... 使用内存 ...

// 释放内存
CN_system_free(allocator, ptr, __FILE__, __LINE__);

// 注意：不需要手动销毁默认分配器
// 程序退出时会自动清理并检查泄漏
```

### 高级功能
```c
// 启用调试模式
CN_system_enable_debug(allocator, true);

// 转储完整状态
CN_system_dump(allocator);

// 重置统计信息
CN_system_reset_stats(allocator);

// 设置调试回调
void debug_callback(const char* message, void* user_data) {
    printf("[SYSTEM_ALLOC] %s\n", message);
}
CN_system_set_debug_callback(allocator, debug_callback, NULL);

// 验证堆完整性（简化实现）
if (!CN_system_validate_heap(allocator)) {
    printf("堆完整性验证失败\n");
}
```

### 集成到CN_memory系统
```c
#include "CN_memory.h"

// 系统分配器是CN_memory的默认分配器
CN_memory_init(Eum_ALLOCATOR_SYSTEM);

// 现在所有cn_malloc/cn_free调用都使用系统分配器
void* ptr1 = cn_malloc(64);
void* ptr2 = cn_malloc(128);

// ... 使用内存 ...

cn_free(ptr1);
cn_free(ptr2);

// 检查泄漏
if (cn_check_leaks()) {
    printf("检测到内存泄漏\n");
    cn_dump_stats();
}

CN_memory_shutdown();
```

## 性能优化

### 1. 配置优化
```c
// 高性能配置（生产环境）
Stru_CN_SystemConfig_t perf_config = {
    .enable_statistics = true,      // 启用基本统计
    .enable_debug = false,          // 禁用调试输出
    .zero_on_alloc = false,         // 不清零内存
    .fill_on_alloc = false,         // 不填充模式
    .fill_on_free = false,          // 不填充模式
    .track_allocations = false,     // 不跟踪分配（性能优先）
    .detect_leaks = false,          // 不检测泄漏（性能优先）
    .validate_heap = false,         // 不验证堆
    .max_single_alloc = 0,          // 无单次分配限制
    .max_total_alloc = 0            // 无总分配限制
};

// 安全配置（调试环境）
Stru_CN_SystemConfig_t safe_config = {
    .enable_statistics = true,      // 启用统计
    .enable_debug = true,           // 启用调试
    .zero_on_alloc = true,          // 分配时清零（安全）
    .fill_on_alloc = true,          // 分配时填充模式
    .fill_on_free = true,           // 释放时填充模式
    .track_allocations = true,      // 跟踪分配信息
    .detect_leaks = true,           // 检测泄漏
    .validate_heap = true,          // 验证堆完整性
    .max_single_alloc = 16 * 1024 * 1024,  // 16MB单次分配限制
    .max_total_alloc = 256 * 1024 * 1024   // 256MB总分配限制
};
```

### 2. 使用模式建议
- **生产环境**：使用高性能配置，禁用调试和跟踪
- **测试环境**：使用安全配置，启用所有检查和跟踪
- **开发环境**：根据需求混合配置，平衡性能和安全性
- **内存敏感**：启用大小限制，防止内存失控

### 3. 性能监控
```c
// 定期监控性能
void monitor_system_performance(Stru_CN_SystemAllocator_t* allocator) {
    Stru_CN_SystemStats_t stats;
    if (!CN_system_get_stats(allocator, &stats)) {
        return;
    }
    
    printf("内存使用统计:\n");
    printf("  当前使用: %zu 字节\n", stats.current_usage);
    printf("  峰值使用: %zu 字节\n", stats.peak_usage);
    printf("  分配次数: %zu\n", stats.allocation_count);
    printf("  释放次数: %zu\n", stats.free_count);
    printf("  分配失败: %zu\n", stats.allocation_failures);
    
    // 计算使用率警告
    if (stats.current_usage > 100 * 1024 * 1024) {  // 超过100MB
        printf("警告：内存使用量超过100MB\n");
    }
    
    if (stats.allocation_failures > 0) {
        printf("警告：有分配失败，可能内存不足\n");
    }
    
    // 检查泄漏
    if (CN_system_check_leaks(allocator)) {
        printf("警告：检测到内存泄漏\n");
        CN_system_dump_leaks(allocator);
    }
}
```

## 错误处理

### 常见错误
```c
// 1. 创建失败
Stru_CN_SystemAllocator_t* allocator = CN_system_create(&config);
if (allocator == NULL) {
    // 可能原因：内存不足、无效配置
    fprintf(stderr, "系统分配器创建失败\n");
    return;
}

// 2. 分配失败
void* ptr = CN_system_alloc(allocator, size, __FILE__, __LINE__, "分配");
if (ptr == NULL) {
    // 可能原因：内存不足、大小超过限制
    fprintf(stderr, "内存分配失败，大小: %zu\n", size);
    
    // 获取统计信息了解情况
    Stru_CN_SystemStats_t stats;
    if (CN_system_get_stats(allocator, &stats)) {
        fprintf(stderr, "当前使用: %zu, 峰值: %zu, 失败次数: %zu\n",
                stats.current_usage, stats.peak_usage, stats.allocation_failures);
    }
    
    return;
}

// 3. 释放无效指针
CN_system_free(allocator, invalid_ptr, __FILE__, __LINE__);
// 系统分配器会检测无效指针并输出错误信息
// 但不会崩溃，保证程序继续运行

// 4. 配置错误
if (config.max_single_alloc > 0 && size > config.max_single_alloc) {
    fprintf(stderr, "分配大小超过单次分配限制: %zu > %zu\n",
            size, config.max_single_alloc);
    return;
}
```

### 调试技巧
```c
// 启用详细调试
void debug_callback(const char* message, void* user_data) {
    FILE* log_file = (FILE*)user_data;
    fprintf(log_file, "[%s] %s\n", get_timestamp(), message);
}

FILE* log_file = fopen("system_alloc_debug.log", "w");
CN_system_set_debug_callback(allocator, debug_callback, log_file);

// 定期检查状态
CN_system_dump(allocator);

// 程序退出前检查泄漏
if (CN_system_check_leaks(allocator)) {
    printf("程序退出前检测到内存泄漏\n");
    CN_system_dump_leaks(allocator);
}

fclose(log_file);
```

## 内存安全

### 安全特性
1. **头部验证**：每个分配都有头部信息，包含魔术字验证
2. **边界检查**：释放时验证指针有效性
3. **泄漏检测**：跟踪分配并检测未释放的内存
4. **模式填充**：调试模式下填充特定模式，便于检测内存错误
5. **大小限制**：可配置的单次和总分配大小限制

### 安全配置
```c
// 安全优先的配置
Stru_CN_SystemConfig_t safe_config = {
    .enable_statistics = true,
    .enable_debug = true,
    .zero_on_alloc = true,          // 防止信息泄漏
    .fill_on_alloc = true,          // 便于检测未初始化使用
    .fill_on_free = true,           // 便于检测释放后使用
    .track_allocations = true,      // 跟踪所有分配
    .detect_leaks = true,           // 检测泄漏
    .validate_heap = true,          // 验证堆
    .max_single_alloc = 8 * 1024 * 1024,   // 8MB单次限制
    .max_total_alloc = 128 * 1024 * 1024   // 128MB总限制
};
```

## 测试策略

### 单元测试
```c
// 测试创建和销毁
void test_create_destroy(void) {
    Stru_CN_SystemConfig_t config = CN_SYSTEM_CONFIG_DEFAULT;
    
    Stru_CN_SystemAllocator_t* allocator = CN_system_create(&config);
    assert(allocator != NULL);
    
    CN_system_destroy(allocator);
    printf("测试通过：创建和销毁\n");
}

// 测试基本分配和释放
void test_alloc_free(void) {
    Stru_CN_SystemConfig_t config = CN_SYSTEM_CONFIG_DEFAULT;
    config.track_allocations = true;
    
    Stru_CN_SystemAllocator_t* allocator = CN_system_create(&config);
    
    void* ptr = CN_system_alloc(allocator, 100, __FILE__, __LINE__, "测试分配");
    assert(ptr != NULL);
    
    CN_system_free(allocator, ptr, __FILE__, __LINE__);
    
    // 检查应无泄漏
    assert(!CN_system_check_leaks(allocator));
    
    CN_system_destroy(allocator);
    printf("测试通过：基本分配和释放\n");
}

// 测试calloc
void test_calloc(void) {
    Stru_CN_SystemAllocator_t* allocator = CN_system_get_default();
    
    int* array = (int*)CN_system_calloc(allocator, 10, sizeof(int), 
                                        __FILE__, __LINE__, "测试数组");
    assert(array != NULL);
    
    // 验证内存已清零
    for (int i = 0; i < 10; i++) {
        assert(array[i] == 0);
    }
    
    CN_system_free(allocator, array, __FILE__, __LINE__);
    printf("测试通过：calloc分配\n");
}
```

### 压力测试
```c
// 长时间运行测试
void stress_test(void) {
    Stru_CN_SystemConfig_t config = CN_SYSTEM_CONFIG_DEFAULT;
    config.track_allocations = true;
    config.max_total_alloc = 64 * 1024 * 1024;  // 64MB限制
    
    Stru_CN_SystemAllocator_t* allocator = CN_system_create(&config);
    
    // 运行多次分配释放循环
    for (int cycle = 0; cycle < 10000; cycle++) {
        // 分配一批对象
        void* objects[100];
        for (int i = 0; i < 100; i++) {
            size_t size = (cycle * 17 +
