# CN_queue API 文档 - 版本 2.0.0

## 概述

`CN_queue` 模块提供了完整、模块化的队列数据结构实现。队列是一种先进先出（FIFO）的数据结构，支持在队尾插入元素，在队头移除元素。

本模块采用分层架构设计，分为三个子模块：
1. **核心模块** - 基本队列操作
2. **迭代器模块** - 队列遍历功能
3. **工具模块** - 高级工具功能

## 头文件

```c
#include "infrastructure/containers/queue/CN_queue.h"
```

## 模块结构

```
queue/
├── CN_queue.h              # 主头文件（包含所有子模块）
├── queue_core/            # 核心模块
│   ├── CN_queue_core.h    # 核心头文件
│   └── CN_queue_core.c    # 核心实现
├── queue_iterator/        # 迭代器模块
│   ├── CN_queue_iterator.h # 迭代器头文件
│   └── CN_queue_iterator.c # 迭代器实现
└── queue_utils/           # 工具模块
    ├── CN_queue_utils.h   # 工具头文件
    └── CN_queue_utils.c   # 工具实现
```

## 数据结构

### Stru_Queue_t (核心模块)

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

### Stru_QueueIterator_t (迭代器模块)

队列迭代器结构体。

```c
typedef struct Stru_QueueIterator_t
{
    Stru_Queue_t* queue;    /**< 要遍历的队列 */
    size_t current_index;   /**< 当前遍历位置 */
    size_t visited_count;   /**< 已访问元素数量 */
} Stru_QueueIterator_t;
```

**字段说明：**

- `queue`: 要遍历的队列指针
- `current_index`: 当前遍历到的元素位置
- `visited_count`: 已经访问过的元素数量

## 函数参考

### 模块管理函数

#### F_queue_get_version
```c
void F_queue_get_version(int* major, int* minor, int* patch);
```
获取队列模块的版本信息。

**参数：**
- `major`: 输出参数，主版本号
- `minor`: 输出参数，次版本号  
- `patch`: 输出参数，修订号

#### F_queue_get_version_string
```c
const char* F_queue_get_version_string(void);
```
获取队列模块的版本字符串。

**返回值：**
- 版本字符串

#### F_queue_module_init
```c
bool F_queue_module_init(void);
```
初始化队列模块。

**返回值：**
- 初始化成功返回true，失败返回false

#### F_queue_module_cleanup
```c
void F_queue_module_cleanup(void);
```
清理队列模块。

### 核心模块函数

#### F_create_queue
```c
Stru_Queue_t* F_create_queue(size_t item_size);
```
创建并初始化一个新的队列。

**参数：**
- `item_size`: 每个元素的大小（字节）

**返回值：**
- 成功：指向新创建的队列的指针
- 失败：NULL（内存分配失败或item_size为0）

#### F_destroy_queue
```c
void F_destroy_queue(Stru_Queue_t* queue);
```
销毁队列，释放所有相关内存。

**参数：**
- `queue`: 要销毁的队列指针

#### F_queue_enqueue
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

#### F_queue_dequeue
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

#### F_queue_peek
```c
void* F_queue_peek(Stru_Queue_t* queue);
```
查看队头元素但不移除。

**参数：**
- `queue`: 队列指针

**返回值：**
- 成功：指向队头元素的指针
- 失败：NULL（队列为空）

#### F_queue_size
```c
size_t F_queue_size(Stru_Queue_t* queue);
```
获取队列中元素的数量。

**参数：**
- `queue`: 队列指针

**返回值：**
- 队列大小，如果`queue`为NULL返回0

#### F_queue_is_empty
```c
bool F_queue_is_empty(Stru_Queue_t* queue);
```
检查队列是否不包含任何元素。

**参数：**
- `queue`: 队列指针

**返回值：**
- 队列为空返回true，否则返回false

#### F_queue_is_full
```c
bool F_queue_is_full(Stru_Queue_t* queue);
```
检查队列是否已满。

**参数：**
- `queue`: 队列指针

**返回值：**
- 队列已满返回true，否则返回false

#### F_queue_clear
```c
void F_queue_clear(Stru_Queue_t* queue);
```
清空队列中的所有元素，但不释放队列本身。

**参数：**
- `queue`: 队列指针

#### F_queue_capacity
```c
size_t F_queue_capacity(Stru_Queue_t* queue);
```
获取队列当前分配的容量。

**参数：**
- `queue`: 队列指针

**返回值：**
- 队列容量，如果`queue`为NULL返回0

#### F_queue_reserve
```c
bool F_queue_reserve(Stru_Queue_t* queue, size_t new_capacity);
```
调整队列的容量。

**参数：**
- `queue`: 队列指针
- `new_capacity`: 新的容量

**返回值：**
- 调整成功返回true，失败返回false

### 迭代器模块函数

#### F_create_queue_iterator
```c
Stru_QueueIterator_t* F_create_queue_iterator(Stru_Queue_t* queue);
```
创建队列迭代器。

**参数：**
- `queue`: 要遍历的队列

**返回值：**
- 成功：指向新创建的迭代器的指针
- 失败：NULL

#### F_destroy_queue_iterator
```c
void F_destroy_queue_iterator(Stru_QueueIterator_t* iterator);
```
销毁队列迭代器。

**参数：**
- `iterator`: 要销毁的迭代器指针

#### F_queue_iterator_has_next
```c
bool F_queue_iterator_has_next(Stru_QueueIterator_t* iterator);
```
检查迭代器是否还有未遍历的元素。

**参数：**
- `iterator`: 迭代器指针

**返回值：**
- 还有下一个元素返回true，否则返回false

#### F_queue_iterator_next
```c
bool F_queue_iterator_next(Stru_QueueIterator_t* iterator, void* out_item);
```
获取迭代器的下一个元素。

**参数：**
- `iterator`: 迭代器指针
- `out_item`: 输出参数，用于接收元素（可为NULL）

**返回值：**
- 成功获取元素返回true，没有更多元素返回false

#### F_queue_iterator_current
```c
void* F_queue_iterator_current(Stru_QueueIterator_t* iterator);
```
获取迭代器当前指向的元素。

**参数：**
- `iterator`: 迭代器指针

**返回值：**
- 指向当前元素的指针，没有当前元素返回NULL

#### F_queue_iterator_reset
```c
void F_queue_iterator_reset(Stru_QueueIterator_t* iterator);
```
将迭代器重置到队列的开始位置。

**参数：**
- `iterator`: 要重置的迭代器指针

#### F_queue_iterator_visited_count
```c
size_t F_queue_iterator_visited_count(Stru_QueueIterator_t* iterator);
```
获取已访问元素数量。

**参数：**
- `iterator`: 迭代器指针

**返回值：**
- 已访问元素数量，如果`iterator`为NULL返回0

#### F_queue_iterator_remaining_count
```c
size_t F_queue_iterator_remaining_count(Stru_QueueIterator_t* iterator);
```
获取剩余元素数量。

**参数：**
- `iterator`: 迭代器指针

**返回值：**
- 剩余元素数量，如果`iterator`为NULL返回0

### 工具模块函数

#### F_queue_copy
```c
Stru_Queue_t* F_queue_copy(Stru_Queue_t* src_queue);
```
复制队列。

**参数：**
- `src_queue`: 源队列指针

**返回值：**
- 成功：指向新创建的队列副本的指针
- 失败：NULL

#### F_queue_equals
```c
bool F_queue_equals(Stru_Queue_t* queue1, Stru_Queue_t* queue2, 
                   int (*compare_func)(const void*, const void*));
```
比较两个队列是否相等。

**参数：**
- `queue1`: 第一个队列指针
- `queue2`: 第二个队列指针
- `compare_func`: 元素比较函数，如果为NULL则使用memcmp

**返回值：**
- 队列相等返回true，否则返回false

#### F_queue_enqueue_batch
```c
size_t F_queue_enqueue_batch(Stru_Queue_t* queue, void** items, size_t count);
```
批量入队。

**参数：**
- `queue`: 队列指针
- `items`: 元素数组指针
- `count`: 元素数量

**返回值：**
- 成功入队的元素数量

#### F_queue_dequeue_batch
```c
size_t F_queue_dequeue_batch(Stru_Queue_t* queue, void** out_items, size_t max_count);
```
批量出队。

**参数：**
- `queue`: 队列指针
- `out_items`: 输出数组指针（可为NULL）
- `max_count`: 最多出队的元素数量

**返回值：**
- 实际出队的元素数量

#### F_queue_to_array
```c
size_t F_queue_to_array(Stru_Queue_t* queue, void** out_array);
```
将队列转换为数组。

**参数：**
- `queue`: 队列指针
- `out_array`: 输出参数，指向分配的数组指针

**返回值：**
- 数组中的元素数量，失败返回0

#### F_queue_from_array
```c
Stru_Queue_t* F_queue_from_array(void** items, size_t count, size_t item_size);
```
从数组创建队列。

**参数：**
- `items`: 元素数组指针
- `count`: 元素数量
- `item_size`: 每个元素的大小（字节）

**返回值：**
- 成功：指向新创建的队列的指针
- 失败：NULL

#### F_queue_find
```c
int F_queue_find(Stru_Queue_t* queue, void* item, 
                int (*compare_func)(const void*, const void*));
```
查找队列中的元素。

**参数：**
- `queue`: 队列指针
- `item`: 要查找的元素指针
- `compare_func`: 元素比较函数，如果为NULL则使用memcmp

**返回值：**
- 找到的元素位置（从0开始），未找到返回-1

#### F_queue_contains
```c
bool F_queue_contains(Stru_Queue_t* queue, void* item, 
                     int (*compare_func)(const void*, const void*));
```
检查队列是否包含元素。

**参数：**
- `queue`: 队列指针
- `item`: 要检查的元素指针
- `compare_func`: 元素比较函数，如果为NULL则使用memcmp

**返回值：**
- 包含返回true，否则返回false

## 配置宏

### 核心模块配置

#### CN_QUEUE_INITIAL_CAPACITY
```c
#define CN_QUEUE_INITIAL_CAPACITY 16
```
队列的初始容量。

#### CN_QUEUE_GROWTH_FACTOR
```c
#define CN_QUEUE_GROWTH_FACTOR 2
```
队列的扩容因子。

### 模块版本配置

#### CN_QUEUE_VERSION_MAJOR
```c
#define CN_QUEUE_VERSION_MAJOR 2
```
队列模块主版本号。

#### CN_QUEUE_VERSION_MINOR
```c
#define CN_QUEUE_VERSION_MINOR 0
```
队列模块次版本号。

#### CN_QUEUE_VERSION_PATCH
```c
#define CN_QUEUE_VERSION_PATCH 0
```
队列模块修订号。

#### CN_QUEUE_VERSION_STRING
```c
#define CN_QUEUE_VERSION_STRING "2.0.0"
```
队列模块版本字符串。

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
