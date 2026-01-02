# CN_memory API 文档

## 概述

`CN_memory` 模块提供统一的内存管理接口，支持多种分配器策略。本模块是CN_Language基础设施层的核心组件，遵循SOLID设计原则和项目架构规范。

## 文件结构

```
src/infrastructure/memory/
├── CN_memory.h      # 头文件 - 公共接口定义
├── CN_memory.c      # 源文件 - 实现代码
└── README.md        # 模块文档
```

## 数据类型

### 分配器接口结构体

```c
typedef struct Stru_CN_AllocatorInterface_t
{
    /**
     * @brief 分配内存
     * 
     * @param size 要分配的内存大小（字节）
     * @param file 调用文件名（用于调试）
     * @param line 调用行号（用于调试）
     * @return 分配的内存指针，失败返回NULL
     */
    void* (*allocate)(size_t size, const char* file, int line);
    
    /**
     * @brief 释放内存
     * 
     * @param ptr 要释放的内存指针
     * @param file 调用文件名（用于调试）
     * @param line 调用行号（用于调试）
     */
    void (*deallocate)(void* ptr, const char* file, int line);
    
    /**
     * @brief 重新分配内存
     * 
     * @param ptr 原内存指针
     * @param new_size 新的内存大小
     * @param file 调用文件名（用于调试）
     * @param line 调用行号（用于调试）
     * @return 重新分配的内存指针，失败返回NULL
     */
    void* (*reallocate)(void* ptr, size_t new_size, const char* file, int line);
    
} Stru_CN_AllocatorInterface_t;
```

### 分配器类型枚举

```c
typedef enum Eum_CN_AllocatorType_t
{
    Eum_ALLOCATOR_SYSTEM = 0,      /**< 系统分配器（包装malloc/free） */
    Eum_ALLOCATOR_DEBUG = 1,       /**< 调试分配器（边界检查、泄漏检测） */
    Eum_ALLOCATOR_POOL = 2,        /**< 对象池分配器（固定大小对象重用） */
    Eum_ALLOCATOR_ARENA = 3        /**< 区域分配器（批量分配，统一释放） */
} Eum_CN_AllocatorType_t;
```

### 内存统计信息结构体（内部使用）

```c
typedef struct Stru_CN_MemoryStats_t
{
    size_t total_allocated;        /**< 总分配字节数 */
    size_t total_freed;            /**< 总释放字节数 */
    size_t current_usage;          /**< 当前使用字节数 */
    size_t peak_usage;             /**< 峰值使用字节数 */
    size_t allocation_count;       /**< 分配次数 */
    size_t free_count;             /**< 释放次数 */
} Stru_CN_MemoryStats_t;
```

## 函数参考

### 初始化与关闭

#### `CN_memory_init`

```c
bool CN_memory_init(Eum_CN_AllocatorType_t allocator_type);
```

初始化内存管理系统。

**参数：**
- `allocator_type`：分配器类型

**返回值：**
- `true`：初始化成功
- `false`：初始化失败

**示例：**
```c
// 使用系统分配器初始化内存系统
if (!CN_memory_init(Eum_ALLOCATOR_SYSTEM)) {
    fprintf(stderr, "内存系统初始化失败\n");
    return EXIT_FAILURE;
}
```

#### `CN_memory_shutdown`

```c
void CN_memory_shutdown(void);
```

关闭内存管理系统，释放所有资源。

**示例：**
```c
// 程序结束时关闭内存系统
CN_memory_shutdown();
```

### 分配器管理

#### `cn_set_allocator`

```c
bool cn_set_allocator(Eum_CN_AllocatorType_t allocator_type);
```

设置当前分配器类型。

**参数：**
- `allocator_type`：新的分配器类型

**返回值：**
- `true`：设置成功
- `false`：设置失败

**示例：**
```c
// 切换到调试分配器
if (!cn_set_allocator(Eum_ALLOCATOR_DEBUG)) {
    fprintf(stderr, "切换分配器失败\n");
}
```

#### `cn_get_allocator_type`

```c
Eum_CN_AllocatorType_t cn_get_allocator_type(void);
```

获取当前分配器类型。

**返回值：**
当前分配器类型

**示例：**
```c
Eum_CN_AllocatorType_t type = cn_get_allocator_type();
printf("当前分配器类型: %d\n", type);
```

#### `cn_get_allocator`

```c
const Stru_CN_AllocatorInterface_t* cn_get_allocator(void);
```

获取当前分配器接口。

**返回值：**
当前分配器接口指针

**示例：**
```c
const Stru_CN_AllocatorInterface_t* allocator = cn_get_allocator();
void* memory = allocator->allocate(100, __FILE__, __LINE__);
```

### 内存分配函数

#### `cn_malloc`

```c
void* cn_malloc(size_t size);
```

分配指定大小的内存。

**参数：**
- `size`：要分配的内存大小（字节）

**返回值：**
- 成功：分配的内存指针
- 失败：NULL

**示例：**
```c
// 分配100字节内存
void* buffer = cn_malloc(100);
if (buffer == NULL) {
    fprintf(stderr, "内存分配失败\n");
    return;
}

// 使用内存
memset(buffer, 0, 100);

// 释放内存
cn_free(buffer);
```

#### `cn_free`

```c
void cn_free(void* ptr);
```

释放之前分配的内存。

**参数：**
- `ptr`：要释放的内存指针

**注意：**
- 如果ptr为NULL，函数什么都不做
- 不能释放非cn_malloc分配的内存

**示例：**
```c
void* data = cn_malloc(50);
// 使用数据...
cn_free(data);
data = NULL; // 避免悬空指针
```

#### `cn_realloc`

```c
void* cn_realloc(void* ptr, size_t new_size);
```

重新分配内存，改变已有内存块的大小。

**参数：**
- `ptr`：原内存指针（可以为NULL）
- `new_size`：新的内存大小

**返回值：**
- 成功：重新分配的内存指针
- 失败：NULL（原内存保持不变）

**示例：**
```c
// 初始分配
void* buffer = cn_malloc(50);

// 扩大内存
void* new_buffer = cn_realloc(buffer, 100);
if (new_buffer == NULL) {
    // 重新分配失败，原内存仍然有效
    cn_free(buffer);
} else {
    buffer = new_buffer; // 使用新指针
}

cn_free(buffer);
```

#### `cn_calloc`

```c
void* cn_calloc(size_t count, size_t size);
```

分配并清零内存。

**参数：**
- `count`：元素数量
- `size`：每个元素大小

**返回值：**
- 成功：分配并清零的内存指针
- 失败：NULL

**示例：**
```c
// 分配10个int并清零
int* numbers = (int*)cn_calloc(10, sizeof(int));
if (numbers == NULL) {
    fprintf(stderr, "内存分配失败\n");
    return;
}

// numbers数组已全部为0
for (int i = 0; i < 10; i++) {
    printf("%d ", numbers[i]); // 全部输出0
}

cn_free(numbers);
```

### 调试和统计函数

#### `cn_enable_debug`

```c
void cn_enable_debug(bool enable);
```

启用或禁用调试模式。

**参数：**
- `enable`：true启用调试，false禁用调试

**示例：**
```c
// 启用调试模式
cn_enable_debug(true);

// 现在所有分配都会输出调试信息
void* ptr = cn_malloc(100);
cn_free(ptr);

// 禁用调试模式
cn_enable_debug(false);
```

#### `cn_check_leaks`

```c
bool cn_check_leaks(void);
```

检查内存泄漏。

**返回值：**
- `true`：检测到内存泄漏
- `false`：没有内存泄漏

**示例：**
```c
// 程序结束前检查泄漏
if (cn_check_leaks()) {
    fprintf(stderr, "警告：检测到内存泄漏\n");
    cn_dump_stats(); // 显示详细统计
}
```

#### `cn_dump_stats`

```c
void cn_dump_stats(void);
```

转储内存统计信息到标准输出。

**示例：**
```c
// 分配一些内存
void* p1 = cn_malloc(100);
void* p2 = cn_malloc(200);

// 查看统计
cn_dump_stats();

// 输出示例：
// === 内存统计信息 ===
// 总分配字节数: 300
// 总释放字节数: 0
// 当前使用字节数: 300
// 峰值使用字节数: 300
// 分配次数: 2
// 释放次数: 0
// ========================

cn_free(p1);
cn_free(p2);
```

#### `cn_validate_heap`

```c
bool cn_validate_heap(void);
```

验证堆完整性（简化实现）。

**返回值：**
- `true`：堆完整
- `false`：堆损坏

**注意：**
当前为简化实现，总是返回true。完整实现应检查堆的完整性。

**示例：**
```c
if (!cn_validate_heap()) {
    fprintf(stderr, "堆完整性检查失败\n");
    abort();
}
```

## 分配器策略详解

### 系统分配器 (Eum_ALLOCATOR_SYSTEM)

**特点：**
- 直接包装标准库的malloc/free/realloc
- 性能与标准库相当
- 提供基本的内存统计
- 支持调试模式下的内存填充

**适用场景：**
- 通用内存分配
- 性能要求不苛刻的场景
- 需要与现有代码兼容的情况

### 调试分配器 (Eum_ALLOCATOR_DEBUG)

**特点：**
- 输出详细的分配/释放信息
- 包含文件名和行号
- 便于追踪内存问题
- 有额外性能开销

**适用场景：**
- 调试阶段
- 内存泄漏检测
- 性能分析

### 对象池分配器 (Eum_ALLOCATOR_POOL)

**特点：**
- 预分配固定大小的对象池
- 重用已释放的对象
- 减少内存碎片
- 提高小对象分配速度

**适用场景：**
- 频繁创建销毁的小对象
- 固定大小的数据结构
- 性能敏感的应用

### 区域分配器 (Eum_ALLOCATOR_ARENA)

**特点：**
- 一次性分配大块内存
- 从区域中分配小内存块
- 区域销毁时统一释放
- 减少系统调用

**适用场景：**
- 临时数据
- 同一生命周期的对象
- 解析器中间数据

## 使用模式

### 基本模式

```c
#include "CN_memory.h"

int main(void)
{
    // 1. 初始化内存系统
    if (!CN_memory_init(Eum_ALLOCATOR_SYSTEM)) {
        return EXIT_FAILURE;
    }
    
    // 2. 分配内存
    int* array = (int*)cn_malloc(100 * sizeof(int));
    if (array == NULL) {
        CN_memory_shutdown();
        return EXIT_FAILURE;
    }
    
    // 3. 使用内存
    for (int i = 0; i < 100; i++) {
        array[i] = i;
    }
    
    // 4. 释放内存
    cn_free(array);
    
    // 5. 关闭内存系统
    CN_memory_shutdown();
    
    return EXIT_SUCCESS;
}
```

### 调试模式

```c
#include "CN_memory.h"

void debug_example(void)
{
    // 使用调试分配器
    CN_memory_init(Eum_ALLOCATOR_DEBUG);
    cn_enable_debug(true);
    
    // 分配会输出调试信息
    void* p1 = cn_malloc(100);  // 输出: [DEBUG] 分配内存: 100 字节, 文件: example.c, 行: 10
    void* p2 = cn_calloc(5, 20); // 输出: [DEBUG] 分配内存: 100 字节, 文件: example.c, 行: 11
    
    cn_free(p1);  // 输出: [DEBUG] 释放内存: 地址: 0x..., 文件: example.c, 行: 14
    cn_free(p2);
    
    // 检查泄漏
    if (cn_check_leaks()) {
        printf("检测到内存泄漏\n");
        cn_dump_stats();
    }
    
    CN_memory_shutdown();
}
```

### 性能优化模式

```c
#include "CN_memory.h"

void performance_example(void)
{
    // 使用对象池分配器优化小对象分配
    CN_memory_init(Eum_ALLOCATOR_POOL);
    
    // 频繁分配释放小对象
    for (int i = 0; i < 10000; i++) {
        void* obj = cn_malloc(32);  // 从对象池获取
        // 使用对象...
        cn_free(obj);  // 返回对象池，不是真正释放
    }
    
    CN_memory_shutdown();
}
```

## 错误处理

### 内存分配失败

```c
void* ptr = cn_malloc(large_size);
if (ptr == NULL) {
    // 处理内存不足
    fprintf(stderr, "内存分配失败，请求大小: %zu\n", large_size);
    
    // 尝试释放一些内存后重试
    free_some_memory();
    ptr = cn_malloc(large_size);
    
    if (ptr == NULL) {
        // 仍然失败，采取降级策略或退出
        return ERROR_OUT_OF_MEMORY;
    }
}
```

### 内存泄漏检测

```c
// 在程序关键点检查泄漏
void critical_function(void)
{
    size_t start_usage = get_current_memory_usage();
    
    perform_operation();
    
    size_t end_usage = get_current_memory_usage();
    if (end_usage > start_usage) {
        fprintf(stderr, "可能的内存泄漏: 增加了 %zu 字节\n", 
                end_usage - start_usage);
        cn_dump_stats();
    }
}
```

## 最佳实践

1. **始终检查返回值**：cn_malloc/cn_realloc可能返回NULL
2. **避免悬空指针**：释放后立即将指针设为NULL
3. **谁分配谁释放**：保持内存所有权的清晰
4. **使用合适的分配器**：根据场景选择分配器类型
5. **定期检查泄漏**：在调试版本中启用泄漏检测
6. **批量操作**：使用区域分配器处理临时数据
7. **性能监控**：使用统计功能监控内存使用

## 兼容性说明

### 与标准库的兼容性

- `cn_malloc`/`cn_free` 可以替代 `malloc`/`free`
- 但不能混用（不要用free释放cn_malloc分配的内存）
- 提供了`cn_calloc`作为`calloc`的替代

### 线程安全性

当前实现不是线程安全的。如果需要在多线程环境中使用：
1. 每个线程使用独立的内存池
2. 或添加互斥锁保护
3. 或使用线程本地存储

### 平台依赖性

- 使用标准C库，平台无关
- 调试信息使用`__FILE__`和`__LINE__`宏
- 区域分配器实现是平台无关的

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-02 | 初始版本，统一内存管理接口 |
| 1.0.1 | 计划 | 修复已知问题，优化性能 |
| 1.1.0 | 计划 | 添加完整对象池和区域分配器 |

## 相关文档

- [内存管理模块README](../src/infrastructure/memory/README.md)
- [架构设计原则](../../architecture/架构设计原则.md)
- [编码规范](../../specifications/CN_Language项目%20技术规范和编码标准.md)

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件
