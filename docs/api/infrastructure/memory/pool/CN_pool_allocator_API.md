# CN_pool_allocator API 文档

## 概述

`CN_pool_allocator` 模块提供高效的对象池内存管理功能，专门用于管理固定大小对象的内存分配和释放。本模块是CN_Language基础设施层的重要组成部分，遵循SOLID设计原则和项目架构规范。

## 文件结构

```
src/infrastructure/memory/
├── CN_pool_allocator.h           # 头文件 - 公共接口定义
├── CN_pool_allocator.c           # 源文件 - 实现代码
└── CN_pool_allocator_README.md   # 模块文档
```

## 数据类型

### 对象池配置结构体

```c
/**
 * @brief 对象池配置选项
 * 
 * 用于配置对象池的行为和性能参数。
 */
typedef struct Stru_CN_PoolConfig_t
{
    size_t object_size;           /**< 对象大小（字节） */
    size_t initial_capacity;      /**< 初始容量（对象数量） */
    size_t max_capacity;          /**< 最大容量（0表示无限制） */
    bool auto_expand;             /**< 是否自动扩展 */
    size_t expand_increment;      /**< 自动扩展时的增量 */
    bool thread_safe;             /**< 是否线程安全 */
    bool zero_on_alloc;           /**< 分配时是否清零内存 */
    bool zero_on_free;            /**< 释放时是否清零内存 */
} Stru_CN_PoolConfig_t;
```

#### 默认配置宏

```c
/**
 * @brief 默认对象池配置
 * 
 * 提供合理的默认配置值。
 */
#define CN_POOL_CONFIG_DEFAULT \
    { \
        .object_size = 0, \
        .initial_capacity = 64, \
        .max_capacity = 0, \
        .auto_expand = true, \
        .expand_increment = 64, \
        .thread_safe = false, \
        .zero_on_alloc = false, \
        .zero_on_free = false \
    }
```

### 对象池统计信息结构体

```c
/**
 * @brief 对象池统计信息
 * 
 * 记录对象池的运行统计信息，用于性能监控和调试。
 */
typedef struct Stru_CN_PoolStats_t
{
    size_t total_objects;         /**< 总对象数量（当前在池中+已分配） */
    size_t free_objects;          /**< 空闲对象数量 */
    size_t allocated_objects;     /**< 已分配对象数量 */
    size_t total_allocations;     /**< 总分配次数 */
    size_t total_deallocations;   /**< 总释放次数 */
    size_t pool_expansions;       /**< 池扩展次数 */
    size_t memory_usage;          /**< 内存使用量（字节） */
    size_t peak_memory_usage;     /**< 峰值内存使用量（字节） */
    size_t allocation_failures;   /**< 分配失败次数 */
} Stru_CN_PoolStats_t;
```

### 对象池句柄

```c
/**
 * @brief 对象池句柄
 * 
 * 不透明指针，代表一个对象池实例。
 */
typedef struct Stru_CN_PoolAllocator_t Stru_CN_PoolAllocator_t;
```

### 调试回调函数类型

```c
/**
 * @brief 对象池调试回调函数类型
 * 
 * @param message 调试消息
 * @param user_data 用户数据
 */
typedef void (*CN_PoolDebugCallback_t)(const char* message, void* user_data);
```

## 函数参考

### 对象池管理函数

#### `CN_pool_create`

```c
Stru_CN_PoolAllocator_t* CN_pool_create(const Stru_CN_PoolConfig_t* config);
```

创建对象池。

**参数：**
- `config`：对象池配置（不能为NULL）

**返回值：**
- 成功：对象池句柄
- 失败：NULL（配置无效或内存不足）

**前置条件：**
- `config` 不能为NULL
- `config->object_size` 必须大于0
- `config->initial_capacity` 必须大于0

**后置条件：**
- 返回的对象池已初始化并准备好使用
- 如果返回NULL，没有资源被分配

**示例：**
```c
Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
config.object_size = sizeof(MyStruct);
config.initial_capacity = 100;

Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
if (pool == NULL) {
    fprintf(stderr, "对象池创建失败\n");
    return;
}
```

#### `CN_pool_destroy`

```c
void CN_pool_destroy(Stru_CN_PoolAllocator_t* pool);
```

销毁对象池，释放所有资源。

**参数：**
- `pool`：对象池句柄

**注意：**
- 如果pool为NULL，函数什么都不做
- 销毁后，所有从该池分配的对象都不应再使用
- 函数会释放池管理的所有内存

**示例：**
```c
Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
// 使用对象池...
CN_pool_destroy(pool);
pool = NULL; // 避免悬空指针
```

#### `CN_pool_alloc`

```c
void* CN_pool_alloc(Stru_CN_PoolAllocator_t* pool);
```

从对象池分配一个对象。

**参数：**
- `pool`：对象池句柄

**返回值：**
- 成功：分配的对象指针
- 失败：NULL（池已满且无法扩展，或内存不足）

**注意：**
- 分配的对象大小由创建时的配置决定
- 如果启用了`zero_on_alloc`，分配的内存会被清零
- 对象可能来自空闲链表或新分配的内存块

**示例：**
```c
MyStruct* obj = (MyStruct*)CN_pool_alloc(pool);
if (obj == NULL) {
    fprintf(stderr, "对象分配失败\n");
    return;
}

// 使用对象
obj->value = 42;
```

#### `CN_pool_free`

```c
bool CN_pool_free(Stru_CN_PoolAllocator_t* pool, void* ptr);
```

释放对象到对象池。

**参数：**
- `pool`：对象池句柄
- `ptr`：要释放的对象指针

**返回值：**
- `true`：释放成功
- `false`：释放失败（指针不属于对象池、重复释放等）

**注意：**
- 如果ptr为NULL，函数返回false
- 如果启用了`zero_on_free`，释放的内存会被清零
- 释放的对象会被添加到空闲链表供重用
- 重复释放会被检测并返回false

**示例：**
```c
MyStruct* obj = (MyStruct*)CN_pool_alloc(pool);
// 使用对象...
if (!CN_pool_free(pool, obj)) {
    fprintf(stderr, "对象释放失败\n");
}
obj = NULL; // 避免悬空指针
```

#### `CN_pool_realloc`

```c
void* CN_pool_realloc(Stru_CN_PoolAllocator_t* pool, void* ptr, size_t new_size);
```

重新分配对象（对象池不支持真正的重新分配，提供兼容接口）。

**参数：**
- `pool`：对象池句柄
- `ptr`：原对象指针
- `new_size`：新大小（必须等于对象池对象大小）

**返回值：**
- 成功：重新分配的对象指针
- 失败：NULL

**注意：**
- 如果new_size等于对象大小，返回原指针
- 如果ptr为NULL，相当于分配新对象
- 否则分配新对象，复制数据，释放原对象
- 这不是真正的realloc，性能可能不如系统realloc

**示例：**
```c
void* ptr = CN_pool_alloc(pool);
// 需要改变大小（必须等于对象大小）
void* new_ptr = CN_pool_realloc(pool, ptr, pool_object_size);
if (new_ptr == NULL) {
    // 处理失败
}
```

### 对象池查询函数

#### `CN_pool_get_config`

```c
bool CN_pool_get_config(Stru_CN_PoolAllocator_t* pool, Stru_CN_PoolConfig_t* config);
```

获取对象池配置。

**参数：**
- `pool`：对象池句柄
- `config`：输出配置信息

**返回值：**
- `true`：获取成功
- `false`：获取失败（参数无效）

**示例：**
```c
Stru_CN_PoolConfig_t config;
if (CN_pool_get_config(pool, &config)) {
    printf("对象大小: %zu\n", config.object_size);
    printf("初始容量: %zu\n", config.initial_capacity);
}
```

#### `CN_pool_get_stats`

```c
bool CN_pool_get_stats(Stru_CN_PoolAllocator_t* pool, Stru_CN_PoolStats_t* stats);
```

获取对象池统计信息。

**参数：**
- `pool`：对象池句柄
- `stats`：输出统计信息

**返回值：**
- `true`：获取成功
- `false`：获取失败（参数无效）

**示例：**
```c
Stru_CN_PoolStats_t stats;
if (CN_pool_get_stats(pool, &stats)) {
    printf("总对象数: %zu\n", stats.total_objects);
    printf("已分配对象: %zu\n", stats.allocated_objects);
    printf("使用率: %.2f%%\n", 
           (float)stats.allocated_objects / stats.total_objects * 100.0f);
}
```

#### `CN_pool_reset_stats`

```c
void CN_pool_reset_stats(Stru_CN_PoolAllocator_t* pool);
```

重置对象池统计信息。

**参数：**
- `pool`：对象池句柄

**注意：**
- 重置计数统计，保留容量和内存使用统计
- 峰值内存使用量重置为当前内存使用量

**示例：**
```c
// 开始性能测试前重置统计
CN_pool_reset_stats(pool);
// 运行测试...
// 查看测试期间的统计
```

#### `CN_pool_contains`

```c
bool CN_pool_contains(Stru_CN_PoolAllocator_t* pool, const void* ptr);
```

检查对象指针是否属于对象池。

**参数：**
- `pool`：对象池句柄
- `ptr`：要检查的指针

**返回值：**
- `true`：指针属于对象池（已分配或空闲）
- `false`：指针不属于对象池或参数无效

**注意：**
- 检查指针是否在对象池的内存范围内
- 验证指针是否对齐到对象大小
- 检查指针是否已分配（不在空闲链表中）

**示例：**
```c
void* ptr = malloc(100);  // 系统分配
if (CN_pool_contains(pool, ptr)) {
    printf("指针属于对象池，应使用CN_pool_free释放\n");
} else {
    printf("指针不属于对象池，应使用free释放\n");
    free(ptr);
}
```

#### `CN_pool_utilization`

```c
float CN_pool_utilization(Stru_CN_PoolAllocator_t* pool);
```

获取对象池使用率。

**参数：**
- `pool`：对象池句柄

**返回值：**
- 成功：使用率（0.0到1.0）
- 失败：-1.0（参数无效或总对象数为0）

**示例：**
```c
float usage = CN_pool_utilization(pool);
if (usage >= 0.0f) {
    printf("对象池使用率: %.2f%%\n", usage * 100.0f);
    if (usage > 0.9f) {
        printf("警告：使用率过高，考虑增加容量\n");
    }
}
```

### 对象池批量操作函数

#### `CN_pool_prealloc`

```c
bool CN_pool_prealloc(Stru_CN_PoolAllocator_t* pool, size_t count);
```

预分配多个对象。

**参数：**
- `pool`：对象池句柄
- `count`：要预分配的对象数量

**返回值：**
- `true`：预分配成功
- `false`：预分配失败（参数无效或内存不足）

**注意：**
- 用于提高首次分配的性能
- 如果当前空闲对象不足，会自动扩展池
- 预分配的对象在空闲链表中

**示例：**
```c
// 预分配1000个对象，提高后续分配性能
if (!CN_pool_prealloc(pool, 1000)) {
    fprintf(stderr, "预分配失败\n");
}
```

#### `CN_pool_clear`

```c
bool CN_pool_clear(Stru_CN_PoolAllocator_t* pool);
```

清空对象池（释放所有空闲对象）。

**参数：**
- `pool`：对象池句柄

**返回值：**
- `true`：清空成功
- `false`：清空失败（参数无效）

**注意：**
- 清空空闲链表
- 重置所有内存块的位图
- 已分配的对象不受影响
- 主要用于重置池状态或调试

**示例：**
```c
// 重置对象池状态
CN_pool_clear(pool);
printf("对象池已清空，所有对象标记为空闲\n");
```

#### `CN_pool_shrink`

```c
bool CN_pool_shrink(Stru_CN_PoolAllocator_t* pool);
```

收缩对象池（释放多余内存）。

**参数：**
- `pool`：对象池句柄

**返回值：**
- `true`：收缩成功
- `false`：收缩失败（当前版本暂未实现）

**注意：**
- 当前版本为占位函数，实际未实现
- 计划在后续版本中实现
- 用于释放完全空闲的内存块

**示例：**
```c
// 尝试收缩对象池（当前版本无效）
if (!CN_pool_shrink(pool)) {
    printf("对象池收缩功能暂未实现\n");
}
```

### 对象池调试函数

#### `CN_pool_validate`

```c
bool CN_pool_validate(Stru_CN_PoolAllocator_t* pool);
```

验证对象池完整性。

**参数：**
- `pool`：对象池句柄

**返回值：**
- `true`：对象池完整
- `false`：对象池损坏或参数无效

**验证内容：**
1. 块指针有效性
2. 容量和使用计数一致性
3. 位图与实际使用情况匹配
4. 统计信息一致性
5. 空闲链表有效性

**示例：**
```c
// 定期验证对象池完整性
if (!CN_pool_validate(pool)) {
    fprintf(stderr, "对象池完整性验证失败！\n");
    CN_pool_dump(pool);  // 输出详细状态
    // 采取恢复措施或中止程序
}
```

#### `CN_pool_dump`

```c
void CN_pool_dump(Stru_CN_PoolAllocator_t* pool);
```

转储对象池状态到标准输出。

**参数：**
- `pool`：对象池句柄

**输出内容：**
1. 配置信息
2. 统计信息
3. 内存块信息
4. 使用率计算

**示例：**
```c
// 调试时输出对象池状态
printf("=== 对象池状态 ===\n");
CN_pool_dump(pool);
printf("==================\n");
```

#### `CN_pool_set_debug_callback`

```c
void CN_pool_set_debug_callback(Stru_CN_PoolAllocator_t* pool, 
                                CN_PoolDebugCallback_t callback, 
                                void* user_data);
```

设置对象池调试回调。

**参数：**
- `pool`：对象池句柄
- `callback`：调试回调函数
- `user_data`：用户数据（传递给回调函数）

**注意：**
- 回调函数接收调试消息和用户数据
- 可用于日志记录、调试输出等
- 设置为NULL禁用调试输出

**示例：**
```c
void my_debug_callback(const char* message, void* user_data) {
    FILE* log_file = (FILE*)user_data;
    fprintf(log_file, "[POOL] %s\n", message);
}

FILE* log_file = fopen("pool.log", "w");
CN_pool_set_debug_callback(pool, my_debug_callback, log_file);

// 使用对象池...
// 调试信息会写入日志文件

fclose(log_file);
```

## 使用模式

### 基本模式

```c
#include "CN_pool_allocator.h"

int main(void)
{
    // 1. 创建配置
    Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
    config.object_size = sizeof(MyData);
    config.initial_capacity = 100;
    config.auto_expand = true;
    
    // 2. 创建对象池
    Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
    if (pool == NULL) {
        return EXIT_FAILURE;
    }
    
    // 3. 预分配（可选）
    CN_pool_prealloc(pool, 50);
    
    // 4. 使用对象池
    MyData* data1 = (MyData*)CN_pool_alloc(pool);
    MyData* data2 = (MyData*)CN_pool_alloc(pool);
    
    if (data1 && data2) {
        data1->value = 1;
        data2->value = 2;
        
        // 使用数据...
    }
    
    // 5. 释放对象
    if (data1) CN_pool_free(pool, data1);
    if (data2) CN_pool_free(pool, data2);
    
    // 6. 查看统计
    Stru_CN_PoolStats_t stats;
    if (CN_pool_get_stats(pool, &stats)) {
        printf("总分配次数: %zu\n", stats.total_allocations);
        printf("内存使用量: %zu 字节\n", stats.memory_usage);
        printf("使用率: %.2f%%\n", CN_pool_utilization(pool) * 100.0f);
    }
    
    // 7. 销毁对象池
    CN_pool_destroy(pool);
    
    return EXIT_SUCCESS;
}
```

### 高级模式：性能优化

```c
#include "CN_pool_allocator.h"
#include <time.h>

// 性能测试结构体
typedef struct Stru_PerfTestData_t
{
    int id;
    double value;
    char name[32];
} Stru_PerfTestData_t;

void performance_test(void)
{
    // 配置高性能对象池
    Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
    config.object_size = sizeof(Stru_PerfTestData_t);
    config.initial_capacity = 10000;
    config.auto_expand = true;
    config.expand_increment = 5000;
    config.zero_on_alloc = false;  // 关闭清零以提高性能
    
    Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
    if (pool == NULL) return;
    
    // 预分配大量对象
    CN_pool_prealloc(pool, 10000);
    
    clock_t start = clock();
    
    // 分配和释放循环
    const int iterations = 100000;
    void* objects[1000];
    
    for (int i = 0; i < iterations; i++) {
        // 分配1000个对象
        for (int j = 0; j < 1000; j++) {
            objects[j] = CN_pool_alloc(pool);
            if (objects[j]) {
                Stru_PerfTestData_t* data = (Stru_PerfTestData_t*)objects[j];
                data->id = j;
                data->value = j * 1.5;
                snprintf(data->name, sizeof(data->name), "Object_%d", j);
            }
        }
        
        // 释放所有对象
        for (int j = 0; j < 1000; j++) {
            if (objects[j]) {
                CN_pool_free(pool, objects[j]);
            }
        }
    }
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("性能测试结果:\n");
    printf("迭代次数: %d\n", iterations);
    printf("每次迭代对象数: 1000\n");
    printf("总操作数: %d\n", iterations * 1000 * 2);  // 分配+释放
    printf("总时间: %.3f 秒\n", elapsed);
    printf("每秒操作数: %.0f\n", (iterations * 1000 * 2) / elapsed);
    
    // 输出统计信息
    Stru_CN_PoolStats_t stats;
    if (CN_pool_get_stats(pool, &stats)) {
        printf("\n对象池统计:\n");
        printf("总对象数: %zu\n", stats.total_objects);
        printf("已分配对象: %zu\n", stats.allocated_objects);
        printf("空闲对象: %zu\n", stats.free_objects);
        printf("池扩展次数: %zu\n", stats.pool_expansions);
        printf("分配失败次数: %zu\n", stats.allocation_failures);
    }
    
    CN_pool_destroy(pool);
}
```

### 调试模式

```c
#include "CN_pool_allocator.h"

// 自定义调试回调
void debug_callback(const char* message, void* user_data)
{
    FILE* log_file = (FILE*)user_data;
    fprintf(log_file, "[DEBUG] %s\n", message);
}

void debug_mode_example(void)
{
    // 创建日志文件
    FILE* log_file = fopen("pool_debug.log", "w");
    if (!log_file) return;
    
    // 创建带调试的对象池
    Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
    config.object_size = 64;
    config.initial_capacity = 10;
    config.auto_expand = true;
    
    Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
    if (!pool) {
        fclose(log_file);
        return;
    }
    
    // 设置调试回调
    CN_pool_set_debug_callback(pool, debug_callback, log_file);
    
    // 执行一些操作
    void* ptr1 = CN_pool_alloc(pool);
    void* ptr2 = CN_pool_alloc(pool);
    void* ptr3 = CN_pool_alloc(pool);
    
    // 故意制造一些错误
    CN_pool_free(pool, NULL);  // 释放NULL指针
    CN_pool_free(pool, ptr1);  // 正常释放
    CN_pool_free(pool, ptr1);  // 重复释放
    
    // 验证对象池完整性
    if (!CN_pool_validate(pool)) {
        fprintf(log_file, "对象池完整性验证失败！\n");
    }
    
    // 转储状态
    CN_pool_dump(pool);
    
    // 清理
    CN_pool_free(pool, ptr2);
    CN_pool_free(pool, ptr3);
    CN_pool_destroy(pool);
    fclose(log_file);
}
```

## 性能特性

### 优势
1. **减少内存碎片**：固定大小对象分配，避免内存碎片
2. **快速分配/释放**：O(1)时间复杂度，使用空闲链表
3. **缓存友好**：连续内存布局，提高缓存命中率
4. **可预测性能**：分配时间稳定，适合实时系统
5. **内存使用高效**：位图跟踪，内存开销小

### 适用场景
1. **频繁创建/销毁的小对象**
2. **固定大小的数据结构**
3. **游戏开发中的实体/组件系统**
4. **网络连接池**
5. **数据库连接池**
6. **线程池任务对象**

### 不适用场景
1. **变长数据**（使用块分配器或系统分配器）
2. **超大对象**（超过几KB）
3. **需要复杂内存布局的对象**

## 错误处理

### 常见错误码
对象池函数通过返回值指示错误：
- `NULL` 指针：分配失败或参数无效
- `false`：操作失败（如释放无效指针）
- `-1.0f`：无效的使用率计算

### 错误预防
1. **配置验证**：创建时验证配置参数
2. **边界检查**：分配时检查容量限制
3. **重复释放检测**：跟踪已释放对象
4. **内存范围验证**：检查指针是否属于对象池
5. **完整性验证**：定期验证内部数据结构

### 调试支持
1. **调试回调**：自定义调试输出
2. **状态转储**：详细输出内部状态
3. **统计信息**：性能监控和调优
4. **完整性检查**：自动检测数据损坏

## 集成指南

### 与CN_memory系统集成
对象池分配器已集成到CN_memory系统中，可以通过以下方式使用：

```c
#include "CN_memory.h"

// 使用全局对象池分配器
void* ptr = CN_pool_alloc(g_pool_allocator_instance);
// 或创建独立对象池
Stru_CN_PoolAllocator_t* my_pool = CN_pool_create(&config);
```

### 编译选项
在编译时可以通过以下宏控制功能：

```makefile
# 启用线程安全（需要额外的同步开销）
CFLAGS += -DCN_POOL_THREAD_SAFE

# 启用详细调试
CFLAGS += -DCN_POOL_DEBUG

# 启用性能统计
CFLAGS += -DCN_POOL_STATS
```

### 平台适配
对象池分配器设计为跨平台，但某些功能可能需要平台特定实现：

1. **内存对齐**：使用平台特定的对齐要求
2. **线程安全**：使用平台特定的同步原语
3. **内存分配**：使用CN_memory的统一接口

## 版本历史

### v1.0.0 (初始版本)
- 基本对象池功能：创建、销毁、分配、释放
- 配置驱动设计
- 统计信息收集
- 调试支持

### 计划功能
1. **内存压缩**：释放完全空闲的内存块
2. **分层对象池**：支持不同大小的对象
3. **性能优化**：SIMD加速位图操作
4. **监控集成**：与系统监控工具集成

## 相关文档

1. [CN_pool_allocator_README.md](../src/infrastructure/memory/CN_pool_allocator_README.md) - 模块详细文档
2. [CN_memory_API.md](./CN_memory_API.md) - 内存管理系统API
3. [架构设计原则.md](../../docs/architecture/架构设计原则.md) - 项目架构原则
4. [编码标准.md](../../docs/specifications/CN_Language项目%20技术规范和编码标准.md) - 编码规范

## 许可证

本模块遵循MIT许可证。详见项目根目录的LICENSE文件。

## 贡献指南

1. 遵循项目编码标准和架构原则
2. 每个函数不超过50行，每个文件不超过500行
3. 添加完整的文档注释
4. 编写单元测试验证功能
5. 更新相关文档

## 支持与反馈

如有问题或建议，请通过项目Issue系统提交。

---
*文档最后更新：2026-01-02*
*架构版本：2.0.0*
