# CN_stack API 文档

## 概述

`CN_stack` 模块提供了栈数据结构的实现。栈是一种后进先出（LIFO）的数据结构，支持在栈顶进行高效的插入和删除操作。

## 头文件

```c
#include "infrastructure/containers/stack/CN_stack.h"
```

## 数据结构

### Stru_Stack_t

栈的主要结构体。

```c
typedef struct Stru_Stack_t
{
    void** items;           /**< 元素指针数组 */
    size_t capacity;        /**< 当前分配的容量 */
    size_t top;             /**< 栈顶位置（下一个空闲位置） */
    size_t item_size;       /**< 每个元素的大小（字节） */
} Stru_Stack_t;
```

**字段说明：**

- `items`: 指向元素指针数组的指针
- `capacity`: 栈当前分配的容量（可容纳的最大元素数）
- `top`: 栈顶位置，表示下一个元素将插入的位置，也等于栈中元素的数量
- `item_size`: 每个元素的大小（字节）

## 函数参考

### F_create_stack

```c
Stru_Stack_t* F_create_stack(size_t item_size);
```

创建并初始化一个新的栈。

**参数：**
- `item_size`: 每个元素的大小（字节）

**返回值：**
- 成功：指向新创建的栈的指针
- 失败：NULL（内存分配失败或item_size为0）

### F_destroy_stack

```c
void F_destroy_stack(Stru_Stack_t* stack);
```

销毁栈，释放所有相关内存。

**参数：**
- `stack`: 要销毁的栈指针

### F_stack_push

```c
bool F_stack_push(Stru_Stack_t* stack, void* item);
```

将元素压入栈顶。

**参数：**
- `stack`: 栈指针
- `item`: 要压入的元素指针

**返回值：**
- 成功：true
- 失败：false（参数无效或内存分配失败）

### F_stack_pop

```c
bool F_stack_pop(Stru_Stack_t* stack, void* out_item);
```

从栈顶弹出元素。

**参数：**
- `stack`: 栈指针
- `out_item`: 输出参数，用于接收弹出的元素（可为NULL）

**返回值：**
- 成功：true
- 失败：false（栈为空）

### F_stack_peek

```c
void* F_stack_peek(Stru_Stack_t* stack);
```

查看栈顶元素但不弹出。

**参数：**
- `stack`: 栈指针

**返回值：**
- 成功：指向栈顶元素的指针
- 失败：NULL（栈为空）

### F_stack_size

```c
size_t F_stack_size(Stru_Stack_t* stack);
```

获取栈中元素的数量。

**参数：**
- `stack`: 栈指针

**返回值：**
- 栈大小，如果`stack`为NULL返回0

### F_stack_is_empty

```c
bool F_stack_is_empty(Stru_Stack_t* stack);
```

检查栈是否不包含任何元素。

**参数：**
- `stack`: 栈指针

**返回值：**
- 栈为空返回true，否则返回false

### F_stack_clear

```c
void F_stack_clear(Stru_Stack_t* stack);
```

清空栈中的所有元素，但不释放栈本身。

**参数：**
- `stack`: 栈指针

## 配置宏

### CN_STACK_INITIAL_CAPACITY
```c
#define CN_STACK_INITIAL_CAPACITY 16
```
栈的初始容量。

### CN_STACK_GROWTH_FACTOR
```c
#define CN_STACK_GROWTH_FACTOR 2
```
栈的扩容因子。

## 使用示例

```c
#include "infrastructure/containers/stack/CN_stack.h"
#include <stdio.h>

int main() {
    // 创建栈
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    
    // 压栈
    for (int i = 0; i < 5; i++) {
        F_stack_push(stack, &i);
    }
    
    // 查看栈大小
    printf("栈大小: %zu\n", F_stack_size(stack));
    
    // 查看栈顶元素
    int* top = (int*)F_stack_peek(stack);
    if (top != NULL) {
        printf("栈顶元素: %d\n", *top);
    }
    
    // 弹栈
    int popped;
    while (F_stack_pop(stack, &popped)) {
        printf("弹出: %d\n", popped);
    }
    
    // 清理
    F_destroy_stack(stack);
    return 0;
}
```

## 性能特征

### 时间复杂度
- 压栈：平均O(1)，最坏O(n)（需要扩容）
- 弹栈：O(1)
- 查看栈顶：O(1)
- 获取大小：O(1)

### 空间复杂度
- O(n)，其中n是栈容量

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本 |

## 相关文档

- [栈模块README](../src/infrastructure/containers/stack/README.md)
- [CN_Language项目架构文档](../../../../architecture/001-中文编程语言CN_Language开发规划.md)
