# CN_Stack API 文档

## 概述

CN_Stack 模块提供栈数据结构的实现，支持多种实现方式和线程安全选项。本文档详细描述所有公共API函数的使用方法和参数说明。

## 数据类型

### 栈实现类型枚举
```c
typedef enum {
    CN_STACK_ARRAY,           // 数组实现
    CN_STACK_LINKED_LIST,     // 链表实现
    CN_STACK_CIRCULAR_ARRAY   // 循环数组实现
} CN_StackImplementationType;
```

### 线程安全选项枚举
```c
typedef enum {
    CN_THREAD_SAFE_DISABLED,  // 非线程安全
    CN_THREAD_SAFE_ENABLED    // 线程安全
} CN_ThreadSafeOption;
```

### 栈句柄（不透明类型）
```c
typedef struct Stru_CN_Stack_t CN_Stack;
```

### 栈迭代器句柄
```c
typedef struct Stru_CN_StackIterator_t CN_StackIterator;
```

## 栈管理函数

### CN_stack_create
```c
CN_Stack* CN_stack_create(
    CN_StackImplementationType impl_type,
    CN_ThreadSafeOption thread_safe,
    size_t initial_capacity
);
```

**功能**：创建新的栈实例。

**参数**：
- `impl_type`：栈实现类型
- `thread_safe`：线程安全选项
- `initial_capacity`：初始容量（仅对数组和循环数组实现有效）

**返回值**：
- 成功：返回栈句柄
- 失败：返回NULL

**示例**：
```c
CN_Stack* stack = CN_stack_create(CN_STACK_ARRAY, CN_THREAD_SAFE_ENABLED, 100);
```

### CN_stack_destroy
```c
void CN_stack_destroy(CN_Stack* stack);
```

**功能**：销毁栈实例，释放所有相关资源。

**参数**：
- `stack`：要销毁的栈句柄

**注意**：传入NULL是安全的，函数会直接返回。

## 基本操作函数

### CN_stack_push
```c
bool CN_stack_push(CN_Stack* stack, const void* data, size_t size);
```

**功能**：将元素压入栈顶。

**参数**：
- `stack`：栈句柄
- `data`：要压入的数据指针
- `size`：数据大小（字节）

**返回值**：
- 成功：返回true
- 失败：返回false

**示例**：
```c
int value = 42;
bool success = CN_stack_push(stack, &value, sizeof(int));
```

### CN_stack_pop
```c
void* CN_stack_pop(CN_Stack* stack, size_t* out_size);
```

**功能**：从栈顶弹出元素。

**参数**：
- `stack`：栈句柄
- `out_size`：输出参数，接收弹出数据的大小（可为NULL）

**返回值**：
- 成功：返回指向弹出数据的指针（需要调用者释放）
- 失败或栈为空：返回NULL

**注意**：调用者负责使用`cn_free()`释放返回的数据。

### CN_stack_peek
```c
const void* CN_stack_peek(const CN_Stack* stack, size_t* out_size);
```

**功能**：查看栈顶元素但不弹出。

**参数**：
- `stack`：栈句柄
- `out_size`：输出参数，接收数据大小（可为NULL）

**返回值**：
- 成功：返回指向栈顶数据的常量指针
- 失败或栈为空：返回NULL

**注意**：返回的指针在栈结构被修改前有效。

### CN_stack_is_empty
```c
bool CN_stack_is_empty(const CN_Stack* stack);
```

**功能**：检查栈是否为空。

**参数**：
- `stack`：栈句柄

**返回值**：
- 栈为空：返回true
- 栈不为空：返回false

### CN_stack_size
```c
size_t CN_stack_size(const CN_Stack* stack);
```

**功能**：获取栈中元素数量。

**参数**：
- `stack`：栈句柄

**返回值**：栈中元素的数量。

## 高级操作函数

### CN_stack_clear
```c
void CN_stack_clear(CN_Stack* stack);
```

**功能**：清空栈中所有元素。

**参数**：
- `stack`：栈句柄

### CN_stack_copy
```c
CN_Stack* CN_stack_copy(const CN_Stack* src);
```

**功能**：创建栈的深拷贝。

**参数**：
- `src`：源栈句柄

**返回值**：
- 成功：返回新栈句柄
- 失败：返回NULL

### CN_stack_merge
```c
bool CN_stack_merge(CN_Stack* dest, const CN_Stack* src);
```

**功能**：将源栈的所有元素合并到目标栈。

**参数**：
- `dest`：目标栈句柄
- `src`：源栈句柄

**返回值**：
- 成功：返回true
- 失败：返回false

### CN_stack_reverse
```c
void CN_stack_reverse(CN_Stack* stack);
```

**功能**：反转栈中元素的顺序。

**参数**：
- `stack`：栈句柄

### CN_stack_sort
```c
void CN_stack_sort(CN_Stack* stack, int (*compare)(const void*, const void*));
```

**功能**：对栈中元素进行排序。

**参数**：
- `stack`：栈句柄
- `compare`：比较函数指针，与qsort兼容

### CN_stack_find
```c
void* CN_stack_find(const CN_Stack* stack, const void* data, size_t size, size_t* out_size);
```

**功能**：在栈中查找指定元素。

**参数**：
- `stack`：栈句柄
- `data`：要查找的数据指针
- `size`：数据大小
- `out_size`：输出参数，接收找到数据的大小（可为NULL）

**返回值**：
- 找到：返回指向数据的指针（需要调用者释放）
- 未找到：返回NULL

### CN_stack_contains
```c
bool CN_stack_contains(const CN_Stack* stack, const void* data, size_t size);
```

**功能**：检查栈是否包含指定元素。

**参数**：
- `stack`：栈句柄
- `data`：要检查的数据指针
- `size`：数据大小

**返回值**：
- 包含：返回true
- 不包含：返回false

## 批量操作函数

### CN_stack_push_multiple
```c
bool CN_stack_push_multiple(CN_Stack* stack, const void* data_array, size_t element_size, size_t count);
```

**功能**：批量压入多个元素。

**参数**：
- `stack`：栈句柄
- `data_array`：数据数组指针
- `element_size`：每个元素的大小
- `count`：元素数量

**返回值**：
- 成功：返回true
- 失败：返回false

### CN_stack_pop_multiple
```c
size_t CN_stack_pop_multiple(CN_Stack* stack, void** out_data_array, size_t max_count);
```

**功能**：批量弹出多个元素。

**参数**：
- `stack`：栈句柄
- `out_data_array`：输出参数，接收数据指针数组
- `max_count`：最大弹出数量

**返回值**：实际弹出的元素数量。

## 迭代器函数

### CN_stack_iterator_create
```c
CN_StackIterator* CN_stack_iterator_create(const CN_Stack* stack);
```

**功能**：创建栈迭代器。

**参数**：
- `stack`：栈句柄

**返回值**：
- 成功：返回迭代器句柄
- 失败：返回NULL

### CN_stack_iterator_next
```c
void* CN_stack_iterator_next(CN_StackIterator* iterator, size_t* out_size);
```

**功能**：获取迭代器的下一个元素。

**参数**：
- `iterator`：迭代器句柄
- `out_size`：输出参数，接收数据大小（可为NULL）

**返回值**：
- 成功：返回指向数据的指针（需要调用者释放）
- 没有更多元素：返回NULL

### CN_stack_iterator_has_next
```c
bool CN_stack_iterator_has_next(const CN_StackIterator* iterator);
```

**功能**：检查迭代器是否还有下一个元素。

**参数**：
- `iterator`：迭代器句柄

**返回值**：
- 有下一个元素：返回true
- 没有下一个元素：返回false

### CN_stack_iterator_destroy
```c
void CN_stack_iterator_destroy(CN_StackIterator* iterator);
```

**功能**：销毁迭代器。

**参数**：
- `iterator`：迭代器句柄

## 工具函数

### CN_stack_dump
```c
void CN_stack_dump(const CN_Stack* stack, void (*print_func)(const void*, size_t));
```

**功能**：调试输出栈内容。

**参数**：
- `stack`：栈句柄
- `print_func`：打印函数指针

### CN_stack_validate
```c
bool CN_stack_validate(const CN_Stack* stack);
```

**功能**：验证栈的完整性。

**参数**：
- `stack`：栈句柄

**返回值**：
- 栈有效：返回true
- 栈无效：返回false

### CN_stack_get_capacity
```c
size_t CN_stack_get_capacity(const CN_Stack* stack);
```

**功能**：获取栈的当前容量。

**参数**：
- `stack`：栈句柄

**返回值**：栈的容量。

### CN_stack_shrink_to_fit
```c
bool CN_stack_shrink_to_fit(CN_Stack* stack);
```

**功能**：收缩栈内存到合适大小。

**参数**：
- `stack`：栈句柄

**返回值**：
- 成功：返回true
- 失败：返回false

## 线程安全操作函数

### CN_stack_lock
```c
void CN_stack_lock(CN_Stack* stack);
```

**功能**：手动锁定栈（仅当线程安全启用时有效）。

**参数**：
- `stack`：栈句柄

### CN_stack_unlock
```c
void CN_stack_unlock(CN_Stack* stack);
```

**功能**：手动解锁栈（仅当线程安全启用时有效）。

**参数**：
- `stack`：栈句柄

### CN_stack_try_lock
```c
bool CN_stack_try_lock(CN_Stack* stack);
```

**功能**：尝试锁定栈（非阻塞）。

**参数**：
- `stack`：栈句柄

**返回值**：
- 锁定成功：返回true
- 锁定失败：返回false

## 属性查询函数

### CN_stack_get_implementation_type
```c
CN_StackImplementationType CN_stack_get_implementation_type(const CN_Stack* stack);
```

**功能**：获取栈的实现类型。

**参数**：
- `stack`：栈句柄

**返回值**：栈的实现类型。

### CN_stack_is_thread_safe
```c
bool CN_stack_is_thread_safe(const CN_Stack* stack);
```

**功能**：检查栈是否启用了线程安全。

**参数**：
- `stack`：栈句柄

**返回值**：
- 线程安全启用：返回true
- 线程安全禁用：返回false

## 错误处理

所有函数都遵循以下错误处理模式：
1. 无效参数：返回错误值（false、NULL等）
2. 内存不足：返回错误值，通过日志系统输出错误信息
3. 线程安全错误：返回错误值，通过日志系统输出错误信息

## 内存管理约定

1. **创建函数**：返回的句柄需要调用者使用对应的销毁函数释放
2. **数据返回函数**：返回的数据指针需要调用者使用`cn_free()`释放
3. **迭代器**：需要调用者使用对应的销毁函数释放

## 使用示例

### 完整示例
```c
#include "CN_stack.h"
#include <stdio.h>

void print_int(const void* data, size_t size) {
    if (size == sizeof(int)) {
        printf("%d ", *(const int*)data);
    }
}

int main() {
    // 创建栈
    CN_Stack* stack = CN_stack_create(CN_STACK_ARRAY, CN_THREAD_SAFE_ENABLED, 10);
    if (!stack) {
        fprintf(stderr, "创建栈失败\n");
        return 1;
    }
    
    // 压入元素
    for (int i = 0; i < 5; i++) {
        if (!CN_stack_push(stack, &i, sizeof(int))) {
            fprintf(stderr, "压入元素失败\n");
            CN_stack_destroy(stack);
            return 1;
        }
    }
    
    // 查看栈大小
    printf("栈大小: %zu\n", CN_stack_size(stack));
    
    // 弹出元素
    while (!CN_stack_is_empty(stack)) {
        size_t size;
        int* data = (int*)CN_stack_pop(stack, &size);
        if (data) {
            printf("弹出: %d\n", *data);
            cn_free(data);
        }
    }
    
    // 销毁栈
    CN_stack_destroy(stack);
    return 0;
}
```

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-02 | 初始版本，包含所有基本和高级功能 |

## 相关文档

- [CN_Stack 模块 README](../src/infrastructure/containers/stack/README.md)
- [CN_Language 架构设计原则](../architecture/架构设计原则.md)
