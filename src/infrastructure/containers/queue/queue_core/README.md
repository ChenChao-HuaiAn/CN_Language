# 队列核心模块 (Queue Core Module)

## 概述

队列核心模块提供了队列数据结构的基本操作实现。这是队列模块的基础部分，包含创建、销毁、入队、出队等核心功能。

## 功能特性

- **队列创建和销毁**：安全的队列生命周期管理
- **入队操作**：在队尾添加元素
- **出队操作**：从队头移除元素
- **队列查询**：查看队头元素、获取队列大小等
- **队列管理**：清空队列、调整容量等
- **错误处理**：完善的参数验证和错误处理

## 数据结构

### Stru_Queue_t

队列的主要结构体：

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

## API 接口

### 创建和销毁

- `F_create_queue(size_t item_size)` - 创建队列
- `F_destroy_queue(Stru_Queue_t* queue)` - 销毁队列

### 队列操作

- `F_queue_enqueue(Stru_Queue_t* queue, void* item)` - 入队
- `F_queue_dequeue(Stru_Queue_t* queue, void* out_item)` - 出队
- `F_queue_peek(Stru_Queue_t* queue)` - 查看队头元素

### 查询操作

- `F_queue_size(Stru_Queue_t* queue)` - 获取队列大小
- `F_queue_is_empty(Stru_Queue_t* queue)` - 检查队列是否为空
- `F_queue_is_full(Stru_Queue_t* queue)` - 检查队列是否已满
- `F_queue_capacity(Stru_Queue_t* queue)` - 获取队列容量

### 队列管理

- `F_queue_clear(Stru_Queue_t* queue)` - 清空队列
- `F_queue_reserve(Stru_Queue_t* queue, size_t new_capacity)` - 调整队列容量

## 实现细节

### 循环数组实现

队列使用循环数组实现，具有以下优点：
1. **高效内存使用**：重复利用数组空间
2. **避免数据移动**：出队时不需要移动所有元素
3. **常数时间操作**：入队和出队都是O(1)操作

### 动态扩容

当队列满时，会自动按照`CN_QUEUE_GROWTH_FACTOR`（默认为2）进行扩容。

### 内存管理

- 队列负责分配和释放元素内存
- 用户只需要提供要存储的数据
- 销毁队列时会自动释放所有相关内存

## 配置参数

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
#include "queue_core/CN_queue_core.h"
#include <stdio.h>

int main() {
    // 创建队列
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL) {
        printf("创建队列失败\n");
        return 1;
    }
    
    // 入队操作
    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        if (!F_queue_enqueue(queue, &values[i])) {
            printf("入队失败: %d\n", values[i]);
        }
    }
    
    // 查询操作
    printf("队列大小: %zu\n", F_queue_size(queue));
    printf("队列是否为空: %s\n", F_queue_is_empty(queue) ? "是" : "否");
    
    // 出队操作
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

## 错误处理

所有函数都包含错误检查：
- 无效参数返回适当错误值（NULL、false等）
- 内存分配失败返回错误
- 队列空时出队返回false
- 队列满时入队返回false（除非自动扩容）

## 依赖关系

- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`
- 无其他项目模块依赖

## 测试

核心模块包含完整的单元测试，覆盖所有API接口和边界条件。

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-07 | 初始版本 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
