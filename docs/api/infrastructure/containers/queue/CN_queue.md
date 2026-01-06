# CN_queue API 文档

## 概述

`CN_queue` 模块提供了队列数据结构的实现。队列是一种先进先出（FIFO）的数据结构，支持在队尾插入元素，在队头移除元素。

## 头文件

```c
#include "infrastructure/containers/queue/CN_queue.h"
```

## 数据结构

### Stru_Queue_t

队列的主要结构体。

```c
typedef struct Stru_Queue_t
{
    void** items;           /**< 元素指针数组 */
    size_t capacity;        /**< 当前分配的容量 */
    size_t front;           /**< 队头位置 */
    size_t rear;            /**< 队尾位置（下一个空闲位置） */
    size_t size;            /**< 队列中元素数量 */
    size_t item_size;       /**< 每个元素的大小（字节） */
} Stru_Queue_t;
```

**字段说明：**

- `items`: 指向元素指针数组的指针
- `capacity`: 队列当前分配的容量（可容纳的最大元素数）
- `front`: 队头位置，表示下一个要移除的元素位置
- `rear`: 队尾位置，表示下一个元素将插入的位置
- `size`: 队列中当前元素的数量
- `item_size`: 每个元素的大小（字节）

## 函数参考

### F_create_queue

```c
Stru_Queue_t* F_create_queue(size_t item_size);
```

创建并初始化一个新的队列。

**参数：**
- `item_size`: 每个元素的大小（字节）

**返回值：**
- 成功：指向新创建的队列的指针
- 失败：NULL（内存分配失败或item_size为0）

### F_destroy_queue

```c
void F_destroy_queue(Stru_Queue_t* queue);
```

销毁队列，释放所有相关内存。

**参数：**
- `queue`: 要销毁的队列指针

### F_queue_enqueue

```c
bool F_queue_enqueue(Stru_Queue_t* queue, void* item);
```

将元素加入队尾。

**参数：**
- `queue`: 队列指针
- `item`: 要加入的元素指针

**返回值：**
- 成功：true
- 失败：false（参数无效或内存分配失败）

### F_queue_dequeue

```c
bool F_queue_dequeue(Stru_Queue_t* queue, void* out_item);
```

从队头移除元素。

**参数：**
- `queue`: 队列指针
- `out_item`: 输出参数，用于接收移除的元素（可为NULL）

**返回值：**
- 成功：true
- 失败：false（队列为空）

### F_queue_peek

```c
void* F_queue_peek(Stru_Queue_t* queue);
```

查看队头元素但不移除。

**参数：**
- `queue`: 队列指针

**返回值：**
- 成功：指向队头元素的指针
- 失败：NULL（队列为空）

### F_queue_size

```c
size_t F_queue_size(Stru_Queue_t* queue);
```

获取队列中元素的数量。

**参数：**
- `queue`: 队列指针

**返回值：**
- 队列大小，如果`queue`为NULL返回0

### F_queue_is_empty

```c
bool F_queue_is_empty(Stru_Queue_t* queue);
```

检查队列是否不包含任何元素。

**参数：**
- `queue`: 队列指针

**返回值：**
- 队列为空返回true，否则返回false

### F_queue_is_full

```c
bool F_queue_is_full(Stru_Queue_t* queue);
```

检查队列是否已满。

**参数：**
- `queue`: 队列指针

**返回值：**
- 队列已满返回true，否则返回false

### F_queue_clear

```c
void F_queue_clear(Stru_Queue_t* queue);
```

清空队列中的所有元素，但不释放队列本身。

**参数：**
- `queue`: 队列指针

## 配置宏

### CN_QUEUE_INITIAL_CAPACITY
```c
#define CN_QUEUE_INITIAL_CAPACITY 16
```
队列的初始容量。

### CN_QUEUE_GROWTH_FACTOR
```c
#define CN_QUEUE_GROWTH_FACTOR 2
```
队列的扩容因子。

## 使用示例

```c
#include "infrastructure/containers/queue/CN_queue.h"
#include <stdio.h>

int main() {
    // 创建队列
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    
    // 入队
    for (int i = 0; i < 5; i++) {
        F_queue_enqueue(queue, &i);
    }
    
    // 查看队列大小
    printf("队列大小: %zu\n", F_queue_size(queue));
    
    // 查看队头元素
    int* front = (int*)F_queue_peek(queue);
    if (front != NULL) {
        printf("队头元素: %d\n", *front);
    }
    
    // 出队
    int dequeued;
    while (F_queue_dequeue(queue, &dequeued)) {
        printf("出队: %d\n", dequeued);
    }
    
    // 清理
    F_destroy_queue(queue);
    return 0;
}
```

## 性能特征

### 时间复杂度
- 入队：平均O(1)，最坏O(n)（需要扩容）
- 出队：O(1)
- 查看队头：O(1)
- 获取大小：O(1)

### 空间复杂度
- O(n)，其中n是队列容量

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本 |

## 相关文档

- [队列模块README](../src/infrastructure/containers/queue/README.md)
- [CN_Language项目架构文档](../../../../architecture/001-中文编程语言CN_Language开发规划.md)
