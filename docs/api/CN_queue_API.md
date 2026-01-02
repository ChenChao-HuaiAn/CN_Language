# CN_Queue API 文档

## 概述

CN_Queue 模块提供队列数据结构的实现，支持多种实现方式、线程安全选项和循环队列优化。本文档详细描述所有公共API函数的使用方法和参数说明。

## 数据类型

### 队列实现类型枚举
```c
typedef enum {
    CN_QUEUE_ARRAY,           // 数组实现
    CN_QUEUE_LINKED_LIST,     // 链表实现
    CN_QUEUE_CIRCULAR_ARRAY   // 循环数组实现（优化）
} CN_QueueImplementationType;
```

### 线程安全选项枚举
```c
typedef enum {
    CN_THREAD_SAFE_DISABLED,  // 非线程安全
    CN_THREAD_SAFE_ENABLED    // 线程安全
} CN_ThreadSafeOption;
```

### 队列句柄（不透明类型）
```c
typedef struct Stru_CN_Queue_t CN_Queue;
```

### 队列迭代器句柄
```c
typedef struct Stru_CN_QueueIterator_t CN_QueueIterator;
```

## 队列管理函数

### CN_queue_create
```c
CN_Queue* CN_queue_create(
    CN_QueueImplementationType impl_type,
    CN_ThreadSafeOption thread_safe,
    size_t initial_capacity
);
```

**功能**：创建新的队列实例。

**参数**：
- `impl_type`：队列实现类型
- `thread_safe`：线程安全选项
- `initial_capacity`：初始容量（对数组和循环数组实现有效）

**返回值**：
- 成功：返回队列句柄
- 失败：返回NULL

**示例**：
```c
CN_Queue* queue = CN_queue_create(CN_QUEUE_CIRCULAR_ARRAY, CN_THREAD_SAFE_ENABLED, 100);
```

### CN_queue_destroy
```c
void CN_queue_destroy(CN_Queue* queue);
```

**功能**：销毁队列实例，释放所有相关资源。

**参数**：
- `queue`：要销毁的队列句柄

**注意**：传入NULL是安全的，函数会直接返回。

## 基本操作函数

### CN_queue_enqueue
```c
bool CN_queue_enqueue(CN_Queue* queue, const void* data, size_t size);
```

**功能**：将元素加入队尾。

**参数**：
- `queue`：队列句柄
- `data`：要加入的数据指针
- `size`：数据大小（字节）

**返回值**：
- 成功：返回true
- 失败：返回false

**示例**：
```c
int value = 42;
bool success = CN_queue_enqueue(queue, &value, sizeof(int));
```

### CN_queue_dequeue
```c
void* CN_queue_dequeue(CN_Queue* queue, size_t* out_size);
```

**功能**：从队首移除元素。

**参数**：
- `queue`：队列句柄
- `out_size`：输出参数，接收移除数据的大小（可为NULL）

**返回值**：
- 成功：返回指向移除数据的指针（需要调用者释放）
- 失败或队列为空：返回NULL

**注意**：调用者负责使用`cn_free()`释放返回的数据。

### CN_queue_peek_front
```c
const void* CN_queue_peek_front(const CN_Queue* queue, size_t* out_size);
```

**功能**：查看队首元素但不移除。

**参数**：
- `queue`：队列句柄
- `out_size`：输出参数，接收数据大小（可为NULL）

**返回值**：
- 成功：返回指向队首数据的常量指针
- 失败或队列为空：返回NULL

**注意**：返回的指针在队列结构被修改前有效。

### CN_queue_peek_rear
```c
const void* CN_queue_peek_rear(const CN_Queue* queue, size_t* out_size);
```

**功能**：查看队尾元素。

**参数**：
- `queue`：队列句柄
- `out_size`：输出参数，接收数据大小（可为NULL）

**返回值**：
- 成功：返回指向队尾数据的常量指针
- 失败或队列为空：返回NULL

### CN_queue_is_empty
```c
bool CN_queue_is_empty(const CN_Queue* queue);
```

**功能**：检查队列是否为空。

**参数**：
- `queue`：队列句柄

**返回值**：
- 队列为空：返回true
- 队列不为空：返回false

### CN_queue_size
```c
size_t CN_queue_size(const CN_Queue* queue);
```

**功能**：获取队列中元素数量。

**参数**：
- `queue`：队列句柄

**返回值**：队列中元素的数量。

## 高级操作函数

### CN_queue_clear
```c
void CN_queue_clear(CN_Queue* queue);
```

**功能**：清空队列中所有元素。

**参数**：
- `queue`：队列句柄

### CN_queue_copy
```c
CN_Queue* CN_queue_copy(const CN_Queue* src);
```

**功能**：创建队列的深拷贝。

**参数**：
- `src`：源队列句柄

**返回值**：
- 成功：返回新队列句柄
- 失败：返回NULL

### CN_queue_merge
```c
bool CN_queue_merge(CN_Queue* dest, const CN_Queue* src);
```

**功能**：将源队列的所有元素合并到目标队列。

**参数**：
- `dest`：目标队列句柄
- `src`：源队列句柄

**返回值**：
- 成功：返回true
- 失败：返回false

### CN_queue_reverse
```c
void CN_queue_reverse(CN_Queue* queue);
```

**功能**：反转队列中元素的顺序。

**参数**：
- `queue`：队列句柄

### CN_queue_sort
```c
void CN_queue_sort(CN_Queue* queue, int (*compare)(const void*, const void*));
```

**功能**：对队列中元素进行排序。

**参数**：
- `queue`：队列句柄
- `compare`：比较函数指针，与qsort兼容

### CN_queue_find
```c
void* CN_queue_find(const CN_Queue* queue, const void* data, size_t size, size_t* out_size);
```

**功能**：在队列中查找指定元素。

**参数**：
- `queue`：队列句柄
- `data`：要查找的数据指针
- `size`：数据大小
- `out_size`：输出参数，接收找到数据的大小（可为NULL）

**返回值**：
- 找到：返回指向数据的指针（需要调用者释放）
- 未找到：返回NULL

### CN_queue_contains
```c
bool CN_queue_contains(const CN_Queue* queue, const void* data, size_t size);
```

**功能**：检查队列是否包含指定元素。

**参数**：
- `queue`：队列句柄
- `data`：要检查的数据指针
- `size`：数据大小

**返回值**：
- 包含：返回true
- 不包含：返回false

## 批量操作函数

### CN_queue_enqueue_multiple
```c
bool CN_queue_enqueue_multiple(CN_Queue* queue, const void* data_array, size_t element_size, size_t count);
```

**功能**：批量加入多个元素。

**参数**：
- `queue`：队列句柄
- `data_array`：数据数组指针
- `element_size`：每个元素的大小
- `count`：元素数量

**返回值**：
- 成功：返回true
- 失败：返回false

### CN_queue_dequeue_multiple
```c
size_t CN_queue_dequeue_multiple(CN_Queue* queue, void** out_data_array, size_t max_count);
```

**功能**：批量移除多个元素。

**参数**：
- `queue`：队列句柄
- `out_data_array`：输出参数，接收数据指针数组
- `max_count`：最大移除数量

**返回值**：实际移除的元素数量。

## 循环队列特定函数

### CN_queue_is_full
```c
bool CN_queue_is_full(const CN_Queue* queue);
```

**功能**：检查队列是否已满（仅对循环数组实现有效）。

**参数**：
- `queue`：队列句柄

**返回值**：
- 队列已满：返回true
- 队列未满或不是循环数组实现：返回false

### CN_queue_get_capacity
```c
size_t CN_queue_get_capacity(const CN_Queue* queue);
```

**功能**：获取队列的容量。

**参数**：
- `queue`：队列句柄

**返回值**：队列的容量。

### CN_queue_reserve
```c
bool CN_queue_reserve(CN_Queue* queue, size_t new_capacity);
```

**功能**：预留队列容量。

**参数**：
- `queue`：队列句柄
- `new_capacity`：新的容量

**返回值**：
- 成功：返回true
- 失败：返回false

### CN_queue_shrink_to_fit
```c
bool CN_queue_shrink_to_fit(CN_Queue* queue);
```

**功能**：收缩队列内存到合适大小。

**参数**：
- `queue`：队列句柄

**返回值**：
- 成功：返回true
- 失败：返回false

## 迭代器函数

### CN_queue_iterator_create
```c
CN_QueueIterator* CN_queue_iterator_create(const CN_Queue* queue);
```

**功能**：创建队列迭代器。

**参数**：
- `queue`：队列句柄

**返回值**：
- 成功：返回迭代器句柄
- 失败：返回NULL

### CN_queue_iterator_next
```c
void* CN_queue_iterator_next(CN_QueueIterator* iterator, size_t* out_size);
```

**功能**：获取迭代器的下一个元素。

**参数**：
- `iterator`：迭代器句柄
- `out_size`：输出参数，接收数据大小（可为NULL）

**返回值**：
- 成功：返回指向数据的指针（需要调用者释放）
- 没有更多元素：返回NULL

### CN_queue_iterator_has_next
```c
bool CN_queue_iterator_has_next(const CN_QueueIterator* iterator);
```

**功能**：检查迭代器是否还有下一个元素。

**参数**：
- `iterator`：迭代器句柄

**返回值**：
- 有下一个元素：返回true
- 没有下一个元素：返回false

### CN_queue_iterator_destroy
```c
void CN_queue_iterator_destroy(CN_QueueIterator* iterator);
```

**功能**：销毁迭代器。

**参数**：
- `iterator`：迭代器句柄

## 工具函数

### CN_queue_dump
```c
void CN_queue_dump(const CN_Queue* queue, void (*print_func)(const void*, size_t));
```

**功能**：调试输出队列内容。

**参数**：
- `queue`：队列句柄
- `print_func`：打印函数指针

### CN_queue_validate
```c
bool CN_queue_validate(const CN_Queue* queue);
```

**功能**：验证队列的完整性。

**参数**：
- `queue`：队列句柄

**返回值**：
- 队列有效：返回true
- 队列无效：返回false

### CN_queue_get_implementation_type
```c
CN_QueueImplementationType CN_queue_get_implementation_type(const CN_Queue* queue);
```

**功能**：获取队列的实现类型。

**参数**：
- `queue`：队列句柄

**返回值**：队列的实现类型。

### CN_queue_is_thread_safe
```c
bool CN_queue_is_thread_safe(const CN_Queue* queue);
```

**功能**：检查队列是否启用了线程安全。

**参数**：
- `queue`：队列句柄

**返回值**：
- 线程安全启用：返回true
- 线程安全禁用：返回false

## 线程安全操作函数

### CN_queue_lock
```c
void CN_queue_lock(CN_Queue* queue);
```

**功能**：手动锁定队列（仅当线程安全启用时有效）。

**参数**：
- `queue`：队列句柄

### CN_queue_unlock
```c
void CN_queue_unlock(CN_Queue* queue);
```

**功能**：手动解锁队列（仅当线程安全启用时有效）。

**参数**：
- `queue`：队列句柄

### CN_queue_try_lock
```c
bool CN_queue_try_lock(CN_Queue* queue);
```

**功能**：尝试锁定队列（非阻塞）。

**参数**：
- `queue`：队列句柄

**返回值**：
- 锁定成功：返回true
- 锁定失败：返回false

## 属性查询函数

### CN_queue_get_implementation_type
```c
CN_QueueImplementationType CN_queue_get_implementation_type(const CN_Queue* queue);
```

**功能**：获取队列的实现类型。

**参数**：
- `queue`：队列句柄

**返回值**：队列的实现类型。

### CN_queue_is_thread_safe
```c
bool CN_queue_is_thread_safe(const CN_Queue* queue);
```

**功能**：检查队列是否启用了线程安全。

**参数**：
- `queue`：队列句柄

**返回值**：
- 线程安全启用：返回true
- 线程安全禁用：返回false

## 错误处理

所有函数都遵循以下错误处理模式：
1. 无效参数：返回错误值（false、NULL等）
2. 内存不足：返回错误值，通过日志系统输出错误信息
3. 线程安全错误：返回错误值，通过日志系统输出错误信息
4. 容量不足（仅循环数组）：返回错误值，通过日志系统输出错误信息

## 内存管理约定

1. **创建函数**：返回的句柄需要调用者使用对应的销毁函数释放
2. **数据返回函数**：返回的数据指针需要调用者使用`cn_free()`释放
3. **迭代器**：需要调用者使用对应的销毁函数释放

## 使用示例

### 完整示例
```c
#include "CN_queue.h"
#include <stdio.h>

void print_int(const void* data, size_t size) {
    if (size == sizeof(int)) {
        printf("%d ", *(const int*)data);
    }
}

int main() {
    // 创建队列
    CN_Queue* queue = CN_queue_create(CN_QUEUE_CIRCULAR_ARRAY, CN_THREAD_SAFE_ENABLED, 10);
    if (!queue) {
        fprintf(stderr, "创建队列失败\n");
        return 1;
    }
    
    // 加入元素
    for (int i = 0; i < 5; i++) {
        if (!CN_queue_enqueue(queue, &i, sizeof(int))) {
            fprintf(stderr, "加入元素失败\n");
            CN_queue_destroy(queue);
            return 1;
        }
    }
    
    // 查看队列大小
    printf("队列大小: %zu\n", CN_queue_size(queue));
    
    // 检查队列是否已满（仅循环数组）
    if (CN_queue_is_full(queue)) {
        printf("队列已满\n");
    }
    
    // 移除元素
    while (!CN_queue_is_empty(queue)) {
        size_t size;
        int* data = (int*)CN_queue_dequeue(queue, &size);
        if (data) {
            printf("移除: %d\n", *data);
            cn_free(data);
        }
    }
    
    // 销毁队列
    CN_queue_destroy(queue);
    return 0;
}
```

## 循环队列优化说明

循环数组实现（CN_QUEUE_CIRCULAR_ARRAY）采用了以下优化：

1. **避免数据移动**：使用头尾指针循环利用数组空间，出队时不需要移动后续元素
2. **高效内存利用**：固定容量但完全利用，无碎片
3. **缓存友好**：数据连续存储，提高缓存命中率
4. **O(1)时间复杂度**：入队和出队操作都是常数时间

### 循环队列使用建议
```c
// 创建时指定合适的容量
CN_Queue* queue = CN_queue_create(CN_QUEUE_CIRCULAR_ARRAY, CN_THREAD_SAFE_DISABLED, 1000);

// 检查队列状态
if (CN_queue_is_full(queue)) {
    // 处理队列满的情况
    printf("队列已满，需要扩容或等待\n");
}

// 获取容量信息
size_t capacity = CN_queue_get_capacity(queue);
size_t size = CN_queue_size(queue);
printf("队列利用率: %.2f%%\n", (float)size / capacity * 100);
```

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-02 | 初始版本，包含所有基本和高级功能，循环队列优化 |

## 相关文档

- [CN_Queue 模块 README](../src/infrastructure/containers/queue/README.md)
- [CN_Language 架构设计原则](../architecture/架构设计原则.md)
- [CN_Stack API 文档](./CN_stack_API.md)
