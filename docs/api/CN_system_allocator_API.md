# CN_Language 系统分配器 API 文档

## 概述

系统分配器（System Allocator）是CN_Language内存管理模块的基础组件，为整个系统提供标准的内存分配服务。本模块包装标准库的malloc/free/realloc函数，提供额外的统计、调试和安全功能，是其他高级分配器（如区域分配器、对象池分配器）的基础。

## 设计理念

### 核心特性
- **标准兼容**：完全兼容标准库内存分配接口
- **统计监控**：详细的分配统计和性能监控
- **调试支持**：强大的调试功能，包括泄漏检测和堆验证
- **安全增强**：内存边界检查、魔术字验证、分配跟踪
- **配置灵活**：丰富的配置选项，适应不同使用场景

### 适用场景
- 通用内存分配需求
- 需要详细统计和监控的内存管理
- 调试和开发阶段的内存问题排查
- 作为其他高级分配器的底层实现

## 数据结构

### 配置结构体

```c
/**
 * @brief 系统分配器配置选项
 * 
 * 用于配置系统分配器的行为和性能参数。
 * 系统分配器是标准库malloc/free/realloc的包装器，
 * 提供额外的统计、调试和安全功能。
 */
typedef struct Stru_CN_SystemConfig_t
{
    bool enable_statistics;       /**< 是否启用统计信息 */
    bool enable_debug;            /**< 是否启用调试模式 */
    bool zero_on_alloc;           /**< 分配时是否清零内存 */
    bool fill_on_alloc;           /**< 分配时是否填充特定模式（调试用） */
    bool fill_on_free;            /**< 释放时是否填充特定模式（调试用） */
    bool track_allocations;       /**< 是否跟踪分配信息（文件、行号） */
    bool detect_leaks;            /**< 是否检测内存泄漏 */
    bool validate_heap;           /**< 是否验证堆完整性 */
    size_t max_single_alloc;      /**< 单次分配最大大小（0表示无限制） */
    size_t max_total_alloc;       /**< 总分配最大大小（0表示无限制） */
} Stru_CN_SystemConfig_t;
```

### 默认配置

```c
/**
 * @brief 默认系统分配器配置
 * 
 * 提供合理的默认配置值。
 */
#define CN_SYSTEM_CONFIG_DEFAULT \
    { \
        .enable_statistics = true,    /* 启用统计 */ \
        .enable_debug = false,        /* 禁用调试 */ \
        .zero_on_alloc = false,       /* 不清零内存 */ \
        .fill_on_alloc = false,       /* 不填充模式 */ \
        .fill_on_free = false,        /* 不填充模式 */ \
        .track_allocations = true,    /* 跟踪分配信息 */ \
        .detect_leaks = true,         /* 检测泄漏 */ \
        .validate_heap = false,       /* 不验证堆 */ \
        .max_single_alloc = 0,        /* 无单次分配限制 */ \
        .max_total_alloc = 0          /* 无总分配限制 */ \
    }
```

### 统计信息结构体

```c
/**
 * @brief 系统分配器统计信息
 * 
 * 记录系统分配器的运行统计信息，用于性能监控和调试。
 */
typedef struct Stru_CN_SystemStats_t
{
    size_t total_allocated;       /**< 总分配字节数 */
    size_t total_freed;           /**< 总释放字节数 */
    size_t current_usage;         /**< 当前使用字节数 */
    size_t peak_usage;            /**< 峰值使用字节数 */
    size_t allocation_count;      /**< 分配次数 */
    size_t free_count;            /**< 释放次数 */
    size_t realloc_count;         /**< 重新分配次数 */
    size_t calloc_count;          /**< 清零分配次数 */
    size_t allocation_failures;   /**< 分配失败次数 */
    size_t memory_overhead;       /**< 内存开销（元数据等） */
} Stru_CN_SystemStats_t;
```

### 分配信息结构体

```c
/**
 * @brief 分配信息记录
 * 
 * 用于跟踪每次分配的信息，便于调试和泄漏检测。
 */
typedef struct Stru_CN_AllocationInfo_t
{
    void* address;                /**< 分配的内存地址 */
    size_t size;                  /**< 分配的大小 */
    const char* file;             /**< 分配所在的文件 */
    int line;                     /**< 分配所在的行号 */
    uint64_t timestamp;           /**< 分配时间戳 */
    const char* purpose;          /**< 分配目的（可选） */
} Stru_CN_AllocationInfo_t;
```

## API 函数

### 分配器管理

#### CN_system_create
```c
Stru_CN_SystemAllocator_t* CN_system_create(const Stru_CN_SystemConfig_t* config);
```
创建系统分配器。

**参数：**
- `config`：系统分配器配置

**返回值：**
- 成功：系统分配器句柄
- 失败：NULL

**示例：**
```c
Stru_CN_SystemConfig_t config = CN_SYSTEM_CONFIG_DEFAULT;
config.enable_debug = true;           // 启用调试模式
config.detect_leaks = true;           // 启用泄漏检测
config.max_single_alloc = 1024 * 1024; // 限制单次分配最大1MB

Stru_CN_SystemAllocator_t* allocator = CN_system_create(&config);
if (allocator == NULL) {
    // 处理错误
}
```

#### CN_system_destroy
```c
void CN_system_destroy(Stru_CN_SystemAllocator_t* allocator);
```
销毁系统分配器，释放所有相关资源。

**参数：**
- `allocator`：系统分配器句柄

**示例：**
```c
CN_system_destroy(allocator);
```

### 内存分配

#### CN_system_alloc
```c
void* CN_system_alloc(Stru_CN_SystemAllocator_t* allocator, size_t size, 
                      const char* file, int line, const char* purpose);
```
从系统分配器分配内存。

**参数：**
- `allocator`：系统分配器句柄
- `size`：要分配的内存大小（字节）
- `file`：调用文件名（用于调试，可为NULL）
- `line`：调用行号（用于调试）
- `purpose`：分配目的描述（可选，可为NULL）

**返回值：**
- 成功：分配的内存指针
- 失败：NULL

**示例：**
```c
void* ptr = CN_system_alloc(allocator, 256, __FILE__, __LINE__, "临时缓冲区");
if (ptr == NULL) {
    // 处理分配失败
}
```

#### CN_system_alloc_aligned
```c
void* CN_system_alloc_aligned(Stru_CN_SystemAllocator_t* allocator, size_t size, 
                              size_t alignment, const char* file, int line, 
                              const char* purpose);
```
从系统分配器分配对齐内存。

**参数：**
- `allocator`：系统分配器句柄
- `size`：要分配的内存大小（字节）
- `alignment`：对齐要求（必须是2的幂）
- `file`：调用文件名（用于调试，可为NULL）
- `line`：调用行号（用于调试）
- `purpose`：分配目的描述（可选，可为NULL）

**返回值：**
- 成功：分配的内存指针
- 失败：NULL

**示例：**
```c
// 分配256字节，32字节对齐
void* ptr = CN_system_alloc_aligned(allocator, 256, 32, __FILE__, __LINE__, "对齐缓冲区");
```

#### CN_system_free
```c
void CN_system_free(Stru_CN_SystemAllocator_t* allocator, void* ptr, 
                    const char* file, int line);
```
释放系统分配器分配的内存。

**参数：**
- `allocator`：系统分配器句柄
- `ptr`：要释放的内存指针
- `file`：调用文件名（用于调试，可为NULL）
- `line`：调用行号（用于调试）

**示例：**
```c
CN_system_free(allocator, ptr, __FILE__, __LINE__);
```

#### CN_system_realloc
```c
void* CN_system_realloc(Stru_CN_SystemAllocator_t* allocator, void* ptr, 
                        size_t new_size, const char* file, int line, 
                        const char* purpose);
```
重新分配系统分配器分配的内存。

**参数：**
- `allocator`：系统分配器句柄
- `ptr`：原内存指针
- `new_size`：新的内存大小
- `file`：调用文件名（用于调试，可为NULL）
- `line`：调用行号（用于调试）
- `purpose`：重新分配目的描述（可选，可为NULL）

**返回值：**
- 成功：重新分配的内存指针
- 失败：NULL

**示例：**
```c
// 将缓冲区扩展到512字节
void* new_ptr = CN_system_realloc(allocator, ptr, 512, __FILE__, __LINE__, "扩展缓冲区");
if (new_ptr == NULL) {
    // 处理重新分配失败
}
```

#### CN_system_calloc
```c
void* CN_system_calloc(Stru_CN_SystemAllocator_t* allocator, size_t count, 
                       size_t size, const char* file, int line, 
                       const char* purpose);
```
分配并清零内存。

**参数：**
- `allocator`：系统分配器句柄
- `count`：元素数量
- `size`：每个元素大小
- `file`：调用文件名（用于调试，可为NULL）
- `line`：调用行号（用于调试）
- `purpose`：分配目的描述（可选，可为NULL）

**返回值：**
- 成功：分配的内存指针
- 失败：NULL

**示例：**
```c
// 分配10个int大小的内存并清零
int* array = (int*)CN_system_calloc(allocator, 10, sizeof(int), __FILE__, __LINE__, "整数数组");
```

### 查询接口

#### CN_system_get_config
```c
bool CN_system_get_config(Stru_CN_SystemAllocator_t* allocator, 
                          Stru_CN_SystemConfig_t* config);
```
获取系统分配器配置。

**参数：**
- `allocator`：系统分配器句柄
- `config`：输出配置信息

**返回值：**
- 成功：true
- 失败：false

**示例：**
```c
Stru_CN_SystemConfig_t config;
if (CN_system_get_config(allocator, &config)) {
    printf("调试模式: %s\n", config.enable_debug ? "启用" : "禁用");
    printf("泄漏检测: %s\n", config.detect_leaks ? "启用" : "禁用");
}
```

#### CN_system_get_stats
```c
bool CN_system_get_stats(Stru_CN_SystemAllocator_t* allocator, 
                         Stru_CN_SystemStats_t* stats);
```
获取系统分配器统计信息。

**参数：**
- `allocator`：系统分配器句柄
- `stats`：输出统计信息

**返回值：**
- 成功：true
- 失败：false（统计未启用或参数无效）

**示例：**
```c
Stru_CN_SystemStats_t stats;
if (CN_system_get_stats(allocator, &stats)) {
    printf("当前使用: %zu 字节\n", stats.current_usage);
    printf("峰值使用: %zu 字节\n", stats.peak_usage);
    printf("分配次数: %zu\n", stats.allocation_count);
}
```

#### CN_system_reset_stats
```c
void CN_system_reset_stats(Stru_CN_SystemAllocator_t* allocator);
```
重置系统分配器统计信息。

**参数：**
- `allocator`：系统分配器句柄

**示例：**
```c
CN_system_reset_stats(allocator);
```

#### CN_system_check_leaks
```c
bool CN_system_check_leaks(Stru_CN_SystemAllocator_t* allocator);
```
检查内存泄漏。

**参数：**
- `allocator`：系统分配器句柄

**返回值：**
- 如果检测到内存泄漏返回true，否则返回false

**示例：**
```c
if (CN_system_check_leaks(allocator)) {
    printf("警告：检测到内存泄漏\n");
    CN_system_dump_leaks(allocator);
}
```

#### CN_system_current_usage
```c
size_t CN_system_current_usage(Stru_CN_SystemAllocator_t* allocator);
```
获取当前内存使用量。

**参数：**
- `allocator`：系统分配器句柄

**返回值：**
- 当前使用字节数

**示例：**
```c
size_t usage = CN_system_current_usage(allocator);
printf("当前内存使用: %.2f MB\n", usage / (1024.0 * 1024.0));
```

#### CN_system_peak_usage
```c
size_t CN_system_peak_usage(Stru_CN_SystemAllocator_t* allocator);
```
获取峰值内存使用量。

**参数：**
- `allocator`：系统分配器句柄

**返回值：**
- 峰值使用字节数

**示例：**
```c
size_t peak = CN_system_peak_usage(allocator);
printf("峰值内存使用: %.2f MB\n", peak / (1024.0 * 1024.0));
```

### 调试接口

#### CN_system_enable_debug
```c
void CN_system_enable_debug(Stru_CN_SystemAllocator_t* allocator, bool enable);
```
启用/禁用调试模式。

**参数：**
- `allocator`：系统分配器句柄
- `enable`：是否启用调试

**示例：**
```c
// 启用调试模式
CN_system_enable_debug(allocator, true);

// 执行一些操作...

// 禁用调试模式
CN_system_enable_debug(allocator, false);
```

#### CN_system_validate_heap
```c
bool CN_system_validate_heap(Stru_CN_SystemAllocator_t* allocator);
```
验证堆完整性。

**参数：**
- `allocator`：系统分配器句柄

**返回值：**
- 如果堆完整返回true，否则返回false

**示例：**
```c
if (!CN_system_validate_heap(allocator)) {
    printf("堆完整性验证失败\n");
    CN_system_dump(allocator);
}
```

#### CN_system_dump
```c
void CN_system_dump(Stru_CN_SystemAllocator_t* allocator);
```
转储系统分配器状态到标准输出。

**参数：**
- `allocator`：系统分配器句柄

**示例：**
```c
// 调试时输出分配器状态
CN_system_dump(allocator);
```

#### CN_system_dump_leaks
```c
void CN_system_dump_leaks(Stru_CN_SystemAllocator_t* allocator);
```
转储所有未释放的分配信息。

**参数：**
- `allocator`：系统分配器句柄

**示例：**
```c
// 程序退出前检查泄漏
if (CN_system_check_leaks(allocator)) {
    printf("发现内存泄漏：\n");
    CN_system_dump_leaks(allocator);
}
```

#### CN_system_set_debug_callback
```c
void CN_system_set_debug_callback(Stru_CN_SystemAllocator_t* allocator, 
                                  CN_SystemDebugCallback_t callback, 
                                  void* user_data);
```
设置系统分配器调试回调。

**参数：**
- `allocator`：系统分配器句柄
- `callback`：调试回调函数
- `user_data`：用户数据

**回调函数类型：**
```c
typedef void (*CN_SystemDebugCallback_t)(const char* message, void* user_data);
```

**示例：**
```c
void debug_callback(const char* message, void* user_data) {
    printf("[SYSTEM_ALLOCATOR_DEBUG] %s\n", message);
}

CN_system_set_debug_callback(allocator, debug_callback, NULL);
```

### 辅助函数

#### CN_system_get_default
```c
Stru_CN_SystemAllocator_t* CN_system_get_default(void);
```
获取默认系统分配器（全局单例）。

**注意：** 这个函数返回全局默认系统分配器，适合大多数使用场景。如果需要自定义配置，请使用CN_system_create创建独立的分配器。

**返回值：**
- 默认系统分配器句柄

**示例：**
```c
// 获取默认系统分配器
Stru_CN_SystemAllocator_t* default_allocator = CN_system_get_default();

// 使用默认分配器分配内存
void* ptr = CN_system_alloc(default_allocator, 1024, __FILE__, __LINE__, "默认分配");
```

#### CN_system_release_default
```c
void CN_system_release_default(void);
```
释放默认系统分配器。

**注意：** 在程序退出时调用，确保所有资源被正确释放。

**示例：**
```c
// 程序退出前释放默认分配器
CN_system_release_default();
```

## 使用示例

### 基本使用
```c
#include "CN_system_allocator.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    // 1. 创建配置
    Stru_CN_SystemConfig_t config = CN_SYSTEM_CONFIG_DEFAULT;
    config.enable_debug = true;           // 启用调试
    config.detect_leaks = true;           // 启用泄漏检测
    
    // 2. 创建系统分配器
    Stru_CN_SystemAllocator_t* allocator = CN_system_create(&config);
    if (allocator == NULL) {
        fprintf(stderr, "系统分配器创建失败\n");
        return 1;
    }
    
    // 3. 分配内存
    void* buffer1 = CN_system_alloc(allocator, 256, __FILE__, __LINE__, "缓冲区1");
    void* buffer2 = CN_system_alloc_aligned(allocator, 512, 32, __FILE__, __LINE__, "对齐缓冲区");
    
    if (buffer1 == NULL || buffer2 == NULL) {
        fprintf(stderr, "内存分配失败\n");
        CN_system_destroy(allocator);
        return 1;
    }
    
    // 4. 使用内存
    memset(buffer1, 0xAA, 256);
    memset(buffer2, 0xBB, 512);
    
    // 5. 重新分配内存
    buffer1 = CN_system_realloc(allocator, buffer1, 1024, __FILE__, __LINE__, "扩展缓冲区");
    if (buffer1 == NULL) {
        fprintf(stderr, "内存重新分配失败\n");
        CN_system_free(allocator, buffer2, __FILE__, __LINE__);
        CN_system_destroy(allocator);
        return 1;
    }
    
    // 6. 查看统计信息
    Stru_CN_SystemStats_t stats;
    if (CN_system_get_stats(allocator, &stats)) {
        printf("当前内存使用: %zu 字节\n", stats.current_usage);
        printf("峰值内存使用: %zu 字节\n", stats.peak_usage);
        printf("分配次数: %zu\n", stats.allocation_count);
    }
    
    // 7. 检查泄漏
    if (CN_system_check_leaks(allocator)) {
        printf("警告：检测到内存泄漏\n");
        CN_system_dump_leaks(allocator);
    }
    
    // 8. 释放内存
    CN_system_free(allocator, buffer1, __FILE__, __LINE__);
    CN_system_free(allocator, buffer2, __FILE__, __LINE__);
    
    // 9. 再次检查泄漏（应该没有）
    if (CN_system_check_leaks(allocator)) {
        printf("错误：仍有内存泄漏\n");
    } else {
        printf("内存管理正常，无泄漏\n");
    }
    
    // 10. 销毁分配器
    CN_system_destroy(allocator);
    
    return 0;
}
```

### 使用默认分配器
```c
#include "CN_system_allocator.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    // 1. 获取默认系统分配器
    Stru_CN_SystemAllocator_t* default_allocator = CN_system_get_default();
    
    // 2. 分配内存（使用默认分配器）
    int* numbers = (int*)CN_system_alloc(default_allocator, 
                                        10 * sizeof(int), 
                                        __FILE__, __LINE__, 
                                        "整数数组");
    
    if (numbers == NULL) {
        fprintf(stderr, "内存分配失败\n");
        return 1;
    }
    
    // 3. 使用内存
    for (int i = 0; i < 10; i++) {
        numbers[i] = i * i;
    }
    
    // 4. 打印数组
    printf("平方数: ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    // 5. 释放内存
    CN_system_free(default_allocator, numbers, __FILE__, __LINE__);
    
    // 6. 程序退出前释放默认分配器
    CN_system_release_default();
    
    return 0;
}
```

### 调试模式使用
```c
#include "CN_system_allocator.h"
#include <stdio.h>

// 调试回调函数
void debug_callback(const char* message, void* user_data) {
    printf("[DEBUG] %s\n", message);
}

int main(void) {
    // 1. 创建配置（启用所有调试功能）
    Stru_CN_SystemConfig_t config = CN_SYSTEM_CONFIG_DEFAULT;
    config.enable_debug = true;
    config.enable_statistics = true;
    config.track_allocations = true;
    config.detect_leaks = true;
    config.validate_heap = true;
    config.fill_on_alloc = true;   // 分配时填充模式
    config.fill_on_free = true;    // 释放时填充模式
    
    // 2. 创建系统分配器
    Stru_CN_SystemAllocator_t* allocator = CN_system_create(&config);
    if (allocator == NULL) {
        fprintf(stderr, "系统分配器创建失败\n");
        return 1;
    }
    
    // 3. 设置调试回调
    CN_system_set_debug_callback(allocator, debug_callback, NULL);
    
    // 4. 启用调试模式
    CN_system_enable_debug(allocator, true);
    
    // 5. 执行一些分配操作
    void* ptr1 = CN_system_alloc(allocator, 100, __FILE__, __LINE__, "测试分配1");
    void* ptr2 = CN_system_calloc(allocator, 5, sizeof(double), __FILE__, __LINE__, "双精度数组");
    void* ptr3 = CN_system_alloc_aligned(allocator, 256, 64, __FILE__, __LINE__, "64字节对齐缓冲区");
    
    // 6. 验证堆完整性
    if (!CN_system_validate_heap(allocator)) {
        printf("堆完整性验证失败\n");
    }
    
    // 7. 转储分配器状态
    printf("\n=== 分配器状态转储 ===\n");
    CN_system_dump(allocator);
    
    // 8. 释放内存（故意漏掉一个）
    CN_system_free(allocator, ptr1, __FILE__, __LINE__);
    CN_system_free(allocator, ptr3, __FILE__, __LINE__);
    // 注意：故意不释放ptr2以测试泄漏检测
    
    // 9. 检查泄漏
    if (CN_system_check_leaks(allocator)) {
        printf("\n=== 检测到内存泄漏 ===\n");
        CN_system_dump_leaks(allocator);
    }
    
    // 10. 重置统计信息
    CN_system_reset_stats(allocator);
    
    // 11. 销毁分配器
    CN_system_destroy(allocator);
    
    return 0;
}
```

### 集成到其他模块
```c
#include "CN_system_allocator.h"

// 自定义数据结构
typedef struct Stru_CN_CustomData_t {
    int id;
    char* name;
    double* values;
    size_t value_count;
} Stru_CN_CustomData_t;

// 使用系统分配器创建自定义数据
Stru_CN_CustomData_t* create_custom_data(Stru_CN_SystemAllocator_t* allocator, 
                                         int id, const char* name, 
                                         size_t value_count) {
    // 分配主结构体
    Stru_CN_CustomData_t* data = (Stru_CN_CustomData_t*)CN_system_alloc(
        allocator, sizeof(Stru_CN_CustomData_t), __FILE__, __LINE__, "自定义数据结构");
    
    if (data == NULL) {
        return NULL;
    }
    
    // 初始化字段
    data->id = id;
    
    // 分配名称字符串
    size_t name_len = strlen(name) + 1;
    data->name = (char*)CN_system_alloc(allocator, name_len, __FILE__, __LINE__, "名称字符串");
    if (data->name == NULL) {
        CN_system_free(allocator, data, __FILE__, __LINE__);
        return NULL;
    }
    strcpy(data->name, name);
    
    // 分配值数组
    data->value_count = value_count;
    data->values = (double*)CN_system_calloc(allocator, value_count, sizeof(double), 
                                            __FILE__, __LINE__, "双精度值数组");
    if (data->values == NULL) {
        CN_system_free(allocator, data->name, __FILE__, __LINE__);
        CN_system_free(allocator, data, __FILE__, __LINE__);
        return NULL;
    }
    
    return data;
}

// 使用系统分配器释放自定义数据
void destroy_custom_data(Stru_CN_SystemAllocator_t* allocator, 
                         Stru_CN_CustomData_t* data) {
    if (data == NULL) {
        return;
    }
    
    // 按分配顺序逆序释放
    if (data->values != NULL) {
        CN_system_free(allocator, data->values, __FILE__, __LINE__);
    }
    
    if (data->name != NULL) {
        CN_system_free(allocator, data->name, __FILE__, __LINE__);
    }
    
    CN_system_free(allocator, data, __FILE__, __LINE__);
}

// 使用示例
int example_usage(void) {
    // 获取默认分配器
    Stru_CN_SystemAllocator_t* allocator = CN_system_get_default();
    
    // 创建自定义数据
    Stru_CN_CustomData_t* data = create_custom_data(allocator, 1, "测试数据", 10);
    if (data == NULL) {
        fprintf(stderr, "创建自定义数据失败\n");
        return 1;
    }
    
    // 使用数据
    for (size_t i = 0; i < data->value_count; i++) {
        data->values[i] = i * 1.5;
    }
    
    // 打印数据
    printf("数据ID: %d\n", data->id);
    printf("数据名称: %s\n", data->name);
    printf("值: ");
    for (size_t i = 0; i < data->value_count; i++) {
        printf("%.2f ", data->values[i]);
    }
    printf("\n");
    
    // 释放数据
    destroy_custom_data(allocator, data);
    
    return 0;
}
```

## 性能注意事项

### 1. 配置优化
- **生产环境**：禁用调试功能（`enable_debug = false`）以提高性能
- **内存限制**：根据应用需求设置合理的`max_single_alloc`和`max_total_alloc`
- **统计开销**：如果不需要统计信息，可设置`enable_statistics = false`

### 2. 使用建议
- **批量分配**：尽量减少小内存块的频繁分配和释放
- **对齐要求**：根据数据类型使用合适的对齐分配
- **泄漏检测**：开发阶段启用泄漏检测，生产环境可禁用

### 3. 线程安全
当前版本的系统分配器不是线程安全的。如果需要在多线程环境中使用，建议：
- 每个线程使用独立的分配器实例
- 在分配器外部加锁
- 考虑实现线程安全的分配器版本

## 错误处理

### 常见错误
1. **分配失败**：返回NULL，检查系统内存是否充足
2. **配置无效**：创建分配器失败，检查配置参数
3. **堆损坏**：验证堆完整性失败，检查内存越界访问

### 调试技巧
1. 启用调试模式获取详细日志
2. 使用泄漏检测功能定位未释放的内存
3. 设置调试回调实时监控分配行为
4. 定期验证堆完整性，及早发现问题

## 兼容性说明

### 标准库兼容性
系统分配器完全兼容标准库的内存分配函数：
- `CN_system_alloc` 对应 `malloc`
- `CN_system_free` 对应 `free`
- `CN_system_realloc` 对应 `realloc`
- `CN_system_calloc` 对应 `calloc`

### 平台支持
- Windows (MSVC/MinGW)
- Linux (GCC/Clang)
- macOS (Clang)

### 编译选项
建议在编译时启用以下选项以获得最佳性能：
- `-O2` 或 `-O3` 优化级别
- `-DNDEBUG` 禁用调试断言（生产环境）
- `-fno-builtin-malloc` 等（如果需要）

## 版本历史

### v1.0.0 (2026-01-02)
- 初始版本发布
- 完整的系统分配器实现
- 支持统计、调试、泄漏检测功能
- 提供默认分配器单例
- 完整的API文档

### 未来计划
- 线程安全版本
- 内存池优化
- 性能分析工具集成
- 更多平台支持

## 相关文档

- [CN_Language内存管理架构](../architecture/内存管理架构.md)
- [区域分配器API文档](CN_arena_allocator_API.md)
- [对象池分配器API文档](CN_object_pool_API.md)（待实现）
- [调试分配器API文档](CN_debug_allocator_API.md)（待实现）

## 贡献指南

欢迎为系统分配器模块贡献代码。请遵循以下步骤：

1. 阅读并理解项目架构设计原则
2. 确保代码符合单一职责原则（文件≤500行，函数≤50行）
3. 添加完整的单元测试
4. 更新相关文档
5. 提交Pull Request

## 许可证

本模块采用MIT许可证。详见项目根目录的LICENSE文件。

## 联系方式

如有问题或建议，请联系：
- 项目维护者：CN_Language开发团队
- 问题跟踪：GitHub Issues
- 文档更新：提交Pull Request
