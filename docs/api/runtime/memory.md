# 运行时内存管理 API

**模块**: `cnlang/runtime/memory`  
**版本**: 1.0.0  
**头文件**: `include/cnlang/runtime/memory.h`

---

## 概述

CN_Language运行时内存管理模块提供了完整的堆内存分配、释放和管理功能，支持标准模式和freestanding模式。

### 特性

- ✅ 标准堆分配器（malloc/free）
- ✅ 对齐内存分配
- ✅ 内存池管理
- ✅ Freestanding模式支持
- ✅ 直接内存访问（MMIO）
- ✅ 内存统计和调试

---

## API参考

### 基础分配函数

#### cn_malloc

分配指定大小的内存块。

```c
void* cn_malloc(size_t size);
```

**参数**:
- `size`: 要分配的字节数

**返回值**:
- 成功: 指向分配内存的指针
- 失败: `NULL`

**示例**:
```c
int* array = (int*)cn_malloc(100 * sizeof(int));
if (array == NULL) {
    fprintf(stderr, "内存分配失败\n");
    return -1;
}
// 使用array
cn_free(array);
```

**注意事项**:
- 分配的内存未初始化，包含随机数据
- 在freestanding模式下，使用自定义分配器
- 必须使用`cn_free`释放内存

---

#### cn_calloc

分配并清零指定大小的内存块。

```c
void* cn_calloc(size_t count, size_t size);
```

**参数**:
- `count`: 元素个数
- `size`: 每个元素的字节数

**返回值**:
- 成功: 指向分配并清零的内存指针
- 失败: `NULL`

**示例**:
```c
int* array = (int*)cn_calloc(100, sizeof(int));
// array中所有元素已初始化为0
```

---

#### cn_realloc

调整已分配内存块的大小。

```c
void* cn_realloc(void* ptr, size_t new_size);
```

**参数**:
- `ptr`: 之前分配的内存指针（可以为NULL）
- `new_size`: 新的大小（字节）

**返回值**:
- 成功: 指向调整后内存的指针（可能与原指针不同）
- 失败: `NULL`（原内存块不变）

**示例**:
```c
int* array = (int*)cn_malloc(10 * sizeof(int));
// 需要更多空间
int* new_array = (int*)cn_realloc(array, 20 * sizeof(int));
if (new_array == NULL) {
    // 扩展失败，原array仍有效
    cn_free(array);
    return -1;
}
array = new_array;  // 使用新指针
```

**注意事项**:
- 如果`ptr`为NULL，行为等同于`cn_malloc`
- 如果`new_size`为0，行为等同于`cn_free`
- 新增的内存未初始化

---

#### cn_free

释放已分配的内存块。

```c
void cn_free(void* ptr);
```

**参数**:
- `ptr`: 要释放的内存指针（可以为NULL）

**示例**:
```c
void* ptr = cn_malloc(1024);
// 使用ptr
cn_free(ptr);
ptr = NULL;  // 良好实践：释放后置NULL
```

**注意事项**:
- 释放NULL指针是安全的（无操作）
- 不要释放未分配的指针
- 不要重复释放同一指针

---

### 对齐内存分配

#### cn_aligned_alloc

分配对齐的内存块。

```c
void* cn_aligned_alloc(size_t alignment, size_t size);
```

**参数**:
- `alignment`: 对齐字节数（必须是2的幂）
- `size`: 要分配的字节数

**返回值**:
- 成功: 对齐的内存指针
- 失败: `NULL`

**示例**:
```c
// 分配64字节对齐的缓冲区（用于SIMD）
void* buffer = cn_aligned_alloc(64, 1024);
if (buffer == NULL) {
    return -1;
}
// buffer地址保证是64的倍数
cn_aligned_free(buffer);
```

**应用场景**:
- SIMD指令优化
- 硬件DMA缓冲区
- Cache行对齐

---

#### cn_aligned_free

释放对齐分配的内存。

```c
void cn_aligned_free(void* ptr);
```

**参数**:
- `ptr`: 通过`cn_aligned_alloc`分配的指针

**注意事项**:
- 必须使用此函数释放对齐分配的内存
- 不要混用`cn_free`和`cn_aligned_free`

---

### 内存操作

#### cn_memcpy

复制内存块。

```c
void* cn_memcpy(void* dest, const void* src, size_t count);
```

**参数**:
- `dest`: 目标内存指针
- `src`: 源内存指针
- `count`: 复制的字节数

**返回值**:
- 返回`dest`

**示例**:
```c
int src[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
int dest[10];
cn_memcpy(dest, src, sizeof(src));
```

**注意事项**:
- 源和目标不能重叠（使用`cn_memmove`处理重叠）
- 目标区域必须足够大

---

#### cn_memmove

安全地复制可能重叠的内存块。

```c
void* cn_memmove(void* dest, const void* src, size_t count);
```

**参数**: 同`cn_memcpy`

**示例**:
```c
int array[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
// 向右移动元素（区域重叠）
cn_memmove(array + 2, array, 8 * sizeof(int));
```

---

#### cn_memset

用指定值填充内存块。

```c
void* cn_memset(void* dest, int value, size_t count);
```

**参数**:
- `dest`: 目标内存指针
- `value`: 填充值（转换为unsigned char）
- `count`: 填充的字节数

**返回值**:
- 返回`dest`

**示例**:
```c
int array[100];
cn_memset(array, 0, sizeof(array));  // 清零数组
```

---

#### cn_memcmp

比较两个内存块。

```c
int cn_memcmp(const void* ptr1, const void* ptr2, size_t count);
```

**参数**:
- `ptr1`: 第一个内存块
- `ptr2`: 第二个内存块
- `count`: 比较的字节数

**返回值**:
- `< 0`: ptr1 < ptr2
- `0`: ptr1 == ptr2
- `> 0`: ptr1 > ptr2

**示例**:
```c
if (cn_memcmp(buffer1, buffer2, 256) == 0) {
    printf("两个缓冲区内容相同\n");
}
```

---

### 内存池管理

#### cn_pool_create

创建内存池。

```c
CnMemoryPool* cn_pool_create(size_t block_size, size_t pool_size);
```

**参数**:
- `block_size`: 每个块的大小（字节）
- `pool_size`: 池的总大小（字节）

**返回值**:
- 成功: 内存池句柄
- 失败: `NULL`

**示例**:
```c
// 创建用于分配128字节对象的内存池
CnMemoryPool* pool = cn_pool_create(128, 64 * 1024);  // 64KB池
if (pool == NULL) {
    return -1;
}
```

**应用场景**:
- 频繁分配/释放固定大小对象
- 减少内存碎片
- 提高分配性能

---

#### cn_pool_alloc

从内存池分配块。

```c
void* cn_pool_alloc(CnMemoryPool* pool);
```

**参数**:
- `pool`: 内存池句柄

**返回值**:
- 成功: 指向分配块的指针
- 失败: `NULL`（池已满）

**示例**:
```c
typedef struct Node {
    int data;
    struct Node* next;
} Node;

CnMemoryPool* node_pool = cn_pool_create(sizeof(Node), 10000);

Node* node = (Node*)cn_pool_alloc(node_pool);
if (node != NULL) {
    node->data = 42;
    node->next = NULL;
}
```

---

#### cn_pool_free

将块返回内存池。

```c
void cn_pool_free(CnMemoryPool* pool, void* ptr);
```

**参数**:
- `pool`: 内存池句柄
- `ptr`: 要释放的块指针

---

#### cn_pool_destroy

销毁内存池。

```c
void cn_pool_destroy(CnMemoryPool* pool);
```

**参数**:
- `pool`: 内存池句柄

**注意事项**:
- 销毁池会释放所有内存，包括未释放的块
- 销毁后不要访问从池中分配的指针

---

### 直接内存访问（MMIO）

#### cn_mmio_read

从内存映射地址读取数据。

```c
uint32_t cn_mmio_read(volatile uint32_t* addr);
```

**参数**:
- `addr`: 内存映射地址

**返回值**:
- 读取的32位值

**示例**:
```c
// 读取硬件寄存器
volatile uint32_t* status_reg = (volatile uint32_t*)0x40000000;
uint32_t status = cn_mmio_read(status_reg);
```

**应用场景**:
- 读取硬件寄存器
- 访问设备内存
- 内核驱动开发

---

#### cn_mmio_write

写入内存映射地址。

```c
void cn_mmio_write(volatile uint32_t* addr, uint32_t value);
```

**参数**:
- `addr`: 内存映射地址
- `value`: 要写入的值

**示例**:
```c
// 配置硬件寄存器
volatile uint32_t* control_reg = (volatile uint32_t*)0x40000004;
cn_mmio_write(control_reg, 0x00000001);  // 使能设备
```

---

### 内存统计

#### cn_mem_get_stats

获取内存使用统计信息。

```c
CnMemoryStats cn_mem_get_stats(void);
```

**返回值**:
- 内存统计结构体

**CnMemoryStats结构**:
```c
typedef struct {
    size_t total_allocated;    // 总分配字节数
    size_t total_freed;         // 总释放字节数
    size_t current_usage;       // 当前使用字节数
    size_t peak_usage;          // 峰值使用字节数
    size_t alloc_count;         // 分配次数
    size_t free_count;          // 释放次数
} CnMemoryStats;
```

**示例**:
```c
CnMemoryStats stats = cn_mem_get_stats();
printf("当前内存使用: %zu bytes\n", stats.current_usage);
printf("峰值内存使用: %zu bytes\n", stats.peak_usage);
printf("分配次数: %zu\n", stats.alloc_count);
```

---

## Freestanding模式

在freestanding模式下（内核开发），内存管理有特殊考虑：

### 初始化

```c
// 提供堆内存区域
void cn_freestanding_heap_init(void* heap_start, size_t heap_size);
```

**示例**:
```c
// 内核启动时初始化堆
extern char _heap_start[];
extern char _heap_end[];
size_t heap_size = (size_t)(_heap_end - _heap_start);

cn_freestanding_heap_init(_heap_start, heap_size);

// 现在可以使用cn_malloc等函数
void* buffer = cn_malloc(4096);
```

### 限制

- 不依赖操作系统
- 需要手动提供堆内存
- 可能需要实现自定义分配策略

---

## 最佳实践

### 1. 检查分配结果

```c
void* ptr = cn_malloc(size);
if (ptr == NULL) {
    // 处理分配失败
    return -1;
}
```

### 2. 避免内存泄漏

```c
void* ptr = cn_malloc(1024);
// 使用ptr
cn_free(ptr);
ptr = NULL;  // 防止悬挂指针
```

### 3. 使用内存池优化性能

```c
// 频繁分配小对象时使用内存池
CnMemoryPool* pool = cn_pool_create(sizeof(MyStruct), 10000);
for (int i = 0; i < 1000; i++) {
    MyStruct* obj = (MyStruct*)cn_pool_alloc(pool);
    // 使用obj
}
cn_pool_destroy(pool);
```

### 4. 对齐敏感数据

```c
// SIMD操作需要对齐
float* vector = (float*)cn_aligned_alloc(16, 1024 * sizeof(float));
// 使用SIMD指令处理vector
cn_aligned_free(vector);
```

---

## 相关API

- [IO系统](io.md) - 文件和标准输入输出
- [同步原语](sync.md) - 线程安全的内存操作
- [集合类型](collections.md) - 高级数据结构

---

## 示例代码

完整示例请参考：
- `examples/system/memory/memory_manager_demo.cn`
- `examples/system/memory/allocator_example.c`
- `examples/os-kernel/os_kernel_demo.cn`

---

**文档版本**: 1.0  
**最后更新**: 2026-01-26  
**维护者**: CN_Language开发团队
