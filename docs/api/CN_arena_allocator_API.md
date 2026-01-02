# CN_Language 区域分配器 API 文档

## 概述

区域分配器（Arena Allocator）是CN_Language内存管理模块的重要组成部分，专门用于高效管理临时对象的内存分配。本模块提供高性能、低碎片的内存管理方案，特别适合编译器中间数据的管理。

## 设计理念

### 核心特性
- **线性分配**：在连续内存区域中线性分配内存
- **批量释放**：通过重置操作一次性释放所有内存
- **对齐支持**：支持任意对齐要求的分配
- **自动扩展**：区域不足时自动扩展
- **统计监控**：详细的运行统计信息

### 适用场景
- 编译器中间数据（AST节点、符号表等）
- 临时对象的批量分配和释放
- 阶段性的内存使用模式
- 需要高性能、低碎片的内存管理

## 数据结构

### 配置结构体

```c
/**
 * @brief 区域分配器配置选项
 */
typedef struct Stru_CN_ArenaConfig_t
{
    size_t initial_size;          /**< 初始区域大小（字节） */
    size_t max_size;              /**< 最大区域大小（0表示无限制） */
    size_t alignment;             /**< 内存对齐要求（字节） */
    bool auto_expand;             /**< 是否自动扩展区域 */
    size_t expand_increment;      /**< 自动扩展时的增量（字节） */
    bool zero_on_alloc;           /**< 分配时是否清零内存 */
    bool thread_safe;             /**< 是否线程安全 */
    bool enable_statistics;       /**< 是否启用统计信息 */
} Stru_CN_ArenaConfig_t;
```

### 统计信息结构体

```c
/**
 * @brief 区域分配器统计信息
 */
typedef struct Stru_CN_ArenaStats_t
{
    size_t total_allocated;       /**< 总分配字节数 */
    size_t total_freed;           /**< 总释放字节数（区域重置时） */
    size_t current_usage;         /**< 当前使用字节数 */
    size_t peak_usage;            /**< 峰值使用字节数 */
    size_t allocation_count;      /**< 分配次数 */
    size_t arena_expansions;      /**< 区域扩展次数 */
    size_t arena_resets;          /**< 区域重置次数 */
    size_t allocation_failures;   /**< 分配失败次数 */
    size_t wasted_space;          /**< 浪费的空间（对齐填充） */
} Stru_CN_ArenaStats_t;
```

### 默认配置

```c
/**
 * @brief 默认区域分配器配置
 */
#define CN_ARENA_CONFIG_DEFAULT \
    { \
        .initial_size = 65536,    /* 64KB初始大小 */ \
        .max_size = 0,            /* 无限制 */ \
        .alignment = 8,           /* 8字节对齐 */ \
        .auto_expand = true,      /* 允许自动扩展 */ \
        .expand_increment = 65536, /* 64KB扩展增量 */ \
        .zero_on_alloc = false,   /* 不清零内存 */ \
        .thread_safe = false,     /* 单线程模式 */ \
        .enable_statistics = true /* 启用统计 */ \
    }
```

## API 函数

### 分配器管理

#### CN_arena_create
```c
Stru_CN_ArenaAllocator_t* CN_arena_create(const Stru_CN_ArenaConfig_t* config);
```
创建区域分配器。

**参数：**
- `config`：区域分配器配置

**返回值：**
- 成功：区域分配器句柄
- 失败：NULL

**示例：**
```c
Stru_CN_ArenaConfig_t config = CN_ARENA_CONFIG_DEFAULT;
config.initial_size = 1024 * 1024;  // 1MB
config.max_size = 16 * 1024 * 1024; // 16MB

Stru_CN_ArenaAllocator_t* arena = CN_arena_create(&config);
if (arena == NULL) {
    // 处理错误
}
```

#### CN_arena_destroy
```c
void CN_arena_destroy(Stru_CN_ArenaAllocator_t* arena);
```
销毁区域分配器，释放所有相关资源。

**参数：**
- `arena`：区域分配器句柄

**示例：**
```c
CN_arena_destroy(arena);
```

### 内存分配

#### CN_arena_alloc
```c
void* CN_arena_alloc(Stru_CN_ArenaAllocator_t* arena, size_t size);
```
从区域分配器分配内存。

**参数：**
- `arena`：区域分配器句柄
- `size`：要分配的内存大小（字节）

**返回值：**
- 成功：分配的内存指针
- 失败：NULL

**示例：**
```c
void* ptr = CN_arena_alloc(arena, 256);
if (ptr == NULL) {
    // 处理分配失败
}
```

#### CN_arena_alloc_aligned
```c
void* CN_arena_alloc_aligned(Stru_CN_ArenaAllocator_t* arena, size_t size, size_t alignment);
```
从区域分配器分配对齐内存。

**参数：**
- `arena`：区域分配器句柄
- `size`：要分配的内存大小（字节）
- `alignment`：对齐要求（必须是2的幂）

**返回值：**
- 成功：分配的内存指针
- 失败：NULL

**示例：**
```c
// 分配256字节，32字节对齐
void* ptr = CN_arena_alloc_aligned(arena, 256, 32);
```

#### CN_arena_reset
```c
void CN_arena_reset(Stru_CN_ArenaAllocator_t* arena, bool zero_memory);
```
重置区域分配器，释放区域中所有分配的内存。

**参数：**
- `arena`：区域分配器句柄
- `zero_memory`：是否清零区域内存

**示例：**
```c
// 重置区域，不清零内存
CN_arena_reset(arena, false);

// 重置区域，并清零内存
CN_arena_reset(arena, true);
```

### 查询接口

#### CN_arena_get_config
```c
bool CN_arena_get_config(Stru_CN_ArenaAllocator_t* arena, Stru_CN_ArenaConfig_t* config);
```
获取区域分配器配置。

**参数：**
- `arena`：区域分配器句柄
- `config`：输出配置信息

**返回值：**
- 成功：true
- 失败：false

**示例：**
```c
Stru_CN_ArenaConfig_t config;
if (CN_arena_get_config(arena, &config)) {
    printf("初始大小: %zu\n", config.initial_size);
}
```

#### CN_arena_get_stats
```c
bool CN_arena_get_stats(Stru_CN_ArenaAllocator_t* arena, Stru_CN_ArenaStats_t* stats);
```
获取区域分配器统计信息。

**参数：**
- `arena`：区域分配器句柄
- `stats`：输出统计信息

**返回值：**
- 成功：true
- 失败：false（统计未启用或参数无效）

**示例：**
```c
Stru_CN_ArenaStats_t stats;
if (CN_arena_get_stats(arena, &stats)) {
    printf("使用率: %.2f%%\n", CN_arena_utilization(arena) * 100.0f);
    printf("分配次数: %zu\n", stats.allocation_count);
}
```

#### CN_arena_reset_stats
```c
void CN_arena_reset_stats(Stru_CN_ArenaAllocator_t* arena);
```
重置区域分配器统计信息。

**参数：**
- `arena`：区域分配器句柄

**示例：**
```c
CN_arena_reset_stats(arena);
```

#### CN_arena_utilization
```c
float CN_arena_utilization(Stru_CN_ArenaAllocator_t* arena);
```
获取区域分配器使用率。

**参数：**
- `arena`：区域分配器句柄

**返回值：**
- 使用率（0.0到1.0）
- 失败：-1.0

**示例：**
```c
float usage = CN_arena_utilization(arena);
if (usage > 0.8f) {
    printf("警告：区域使用率过高\n");
}
```

#### CN_arena_remaining
```c
size_t CN_arena_remaining(Stru_CN_ArenaAllocator_t* arena);
```
获取区域分配器剩余空间。

**参数：**
- `arena`：区域分配器句柄

**返回值：**
- 剩余空间字节数

**示例：**
```c
size_t remaining = CN_arena_remaining(arena);
if (remaining < 1024) {
    printf("警告：剩余空间不足1KB\n");
}
```

#### CN_arena_total_size
```c
size_t CN_arena_total_size(Stru_CN_ArenaAllocator_t* arena);
```
获取区域分配器总大小。

**参数：**
- `arena`：区域分配器句柄

**返回值：**
- 总大小字节数

**示例：**
```c
size_t total = CN_arena_total_size(arena);
size_t remaining = CN_arena_remaining(arena);
printf("总大小: %zu, 已使用: %zu, 剩余: %zu\n", 
       total, total - remaining, remaining);
```

### 批量操作

#### CN_arena_reserve
```c
bool CN_arena_reserve(Stru_CN_ArenaAllocator_t* arena, size_t size);
```
预分配区域空间，确保有足够的连续空间。

**参数：**
- `arena`：区域分配器句柄
- `size`：要预分配的空间大小

**返回值：**
- 成功：true
- 失败：false

**示例：**
```c
// 预分配1MB空间
if (!CN_arena_reserve(arena, 1024 * 1024)) {
    // 处理预分配失败
}
```

#### CN_arena_expand
```c
bool CN_arena_expand(Stru_CN_ArenaAllocator_t* arena, size_t additional_size);
```
手动扩展区域大小。

**参数：**
- `arena`：区域分配器句柄
- `additional_size`：要增加的大小

**返回值：**
- 成功：true
- 失败：false

**示例：**
```c
// 扩展512KB
if (CN_arena_expand(arena, 512 * 1024)) {
    printf("区域扩展成功\n");
}
```

#### CN_arena_shrink
```c
bool CN_arena_shrink(Stru_CN_ArenaAllocator_t* arena);
```
收缩区域，释放多余内存。

**参数：**
- `arena`：区域分配器句柄

**返回值：**
- 成功：true
- 失败：false

**示例：**
```c
// 收缩区域
if (CN_arena_shrink(arena)) {
    printf("区域收缩成功\n");
}
```

### 调试接口

#### CN_arena_validate
```c
bool CN_arena_validate(Stru_CN_ArenaAllocator_t* arena);
```
验证区域分配器完整性。

**参数：**
- `arena`：区域分配器句柄

**返回值：**
- 完整：true
- 不完整：false

**示例：**
```c
if (!CN_arena_validate(arena)) {
    printf("区域完整性验证失败\n");
    CN_arena_dump(arena);
}
```

#### CN_arena_dump
```c
void CN_arena_dump(Stru_CN_ArenaAllocator_t* arena);
```
转储区域分配器状态到标准输出。

**参数：**
- `arena`：区域分配器句柄

**示例：**
```c
// 调试时输出区域状态
CN_arena_dump(arena);
```

#### CN_arena_set_debug_callback
```c
void CN_arena_set_debug_callback(Stru_CN_ArenaAllocator_t* arena, 
                                 CN_ArenaDebugCallback_t callback, 
                                 void* user_data);
```
设置区域分配器调试回调。

**参数：**
- `arena`：区域分配器句柄
- `callback`：调试回调函数
- `user_data`：用户数据

**回调函数类型：**
```c
typedef void (*CN_ArenaDebugCallback_t)(const char* message, void* user_data);
```

**示例：**
```c
void debug_callback(const char* message, void* user_data) {
    printf("[ARENA_DEBUG] %s\n", message);
}

CN_arena_set_debug_callback(arena, debug_callback, NULL);
```

### 辅助函数

#### CN_arena_align_address
```c
static inline uintptr_t CN_arena_align_address(uintptr_t address, size_t alignment);
```
计算对齐后的地址。

**参数：**
- `address`：原始地址
- `alignment`：对齐要求（必须是2的幂）

**返回值：**
- 对齐后的地址

**示例：**
```c
uintptr_t addr = 0x1003;
uintptr_t aligned = CN_arena_align_address(addr, 8); // 返回 0x1008
```

#### CN_arena_padding_size
```c
static inline size_t CN_arena_padding_size(uintptr_t address, size_t alignment);
```
计算对齐填充大小。

**参数：**
- `address`：原始地址
- `alignment`：对齐要求（必须是2的幂）

**返回值：**
- 需要的填充字节数

**示例：**
```c
uintptr_t addr = 0x1003;
size_t padding = CN_arena_padding_size(addr, 8); // 返回 5
```

## 使用示例

### 基本使用
```c
#include "CN_arena_allocator.h"

int main(void) {
    // 1. 创建配置
    Stru_CN_ArenaConfig_t config = CN_ARENA_CONFIG_DEFAULT;
    config.initial_size = 1024 * 1024;  // 1MB
    config.max_size = 8 * 1024 * 1024;  // 8MB
    config.alignment = 16;              // 16字节对齐
    
    // 2. 创建区域分配器
    Stru_CN_ArenaAllocator_t* arena = CN_arena_create(&config);
    if (arena == NULL) {
        fprintf(stderr, "区域分配器创建失败\n");
        return 1;
    }
    
    // 3. 分配内存
    void* ptr1 = CN_arena_alloc(arena, 256);
    void* ptr2 = CN_arena_alloc_aligned(arena, 512, 32);
    
    if (ptr1 == NULL || ptr2 == NULL) {
        fprintf(stderr, "内存分配失败\n");
        CN_arena_destroy(arena);
        return 1;
    }
    
    // 4. 使用内存
    memset(ptr1, 0xAA, 256);
    memset(ptr2, 0xBB, 512);
    
    // 5. 查看统计信息
    Stru_CN_ArenaStats_t stats;
    if (CN_arena_get_stats(arena, &stats)) {
        printf("分配次数: %zu, 使用率: %.2f%%\n", 
               stats.allocation_count, CN_arena_utilization(arena) * 100.0f);
    }
    
    // 6. 重置区域（批量释放）
    CN_arena_reset(arena, true);
    
    // 7. 销毁区域分配器
    CN_arena_destroy(arena);
    
    return 0;
}
```

### 编译器中间数据管理
```c
#include "CN_arena_allocator.h"

// 编译器阶段使用不同的区域
typedef struct Stru_CN_CompilerArenas_t {
    Stru_CN_ArenaAllocator_t* lexer_arena;    // 词法分析阶段
    Stru_CN_ArenaAllocator_t* parser_arena;   // 语法分析阶段
    Stru_CN_ArenaAllocator_t* semantic_arena; // 语义分析阶段
} Stru_CN_CompilerArenas_t;

Stru_CN_CompilerArenas_t* create_compiler_arenas(void) {
    Stru_CN_CompilerArenas_t* arenas = malloc(sizeof(Stru_CN_CompilerArenas_t));
    if (arenas == NULL) return NULL;
    
    Stru_CN_ArenaConfig_t config = CN_ARENA_CONFIG_DEFAULT;
    config.initial_size = 512 * 1024;  // 512KB
    
    // 创建各阶段区域
    arenas->lexer_arena = CN_arena_create(&config);
    arenas->parser_arena = CN_arena_create(&config);
    arenas->semantic_arena = CN_arena_create(&config);
    
    if (!arenas->lexer_arena || !arenas->parser_arena || !arenas->semantic_arena) {
        // 清理已创建的区域
        if (arenas->lexer_arena) CN_arena_destroy(arenas->lexer_arena);
        if (arenas->parser_arena) CN_arena_destroy(arenas->parser_arena);
        if (arenas->semantic_arena) CN_arena_destroy(arenas->semantic_arena);
        free(arenas);
        return NULL;
    }
    
    return arenas;
}

void destroy_compiler_arenas(Stru_CN_CompilerArenas_t* arenas) {
    if (arenas == NULL) return;
    
    CN_arena_destroy(arenas->lexer_arena);
    CN_arena_destroy(arenas->parser_arena);
    CN_arena_destroy(arenas->semantic_arena);
    free(arenas);
}

// 在编译完成后重置所有区域
void reset_compiler_arenas(Stru_CN_CompilerArenas_t* arenas) {
    CN_arena_reset(arenas->lexer_arena, false);
    CN_arena_reset(arenas->parser_arena,
