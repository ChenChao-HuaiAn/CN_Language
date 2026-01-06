# 队列模块 (Queue Module)

## 概述

队列模块提供了队列数据结构的实现。队列是一种先进先出（FIFO）的数据结构，支持在队尾入队、在队头出队操作。

## 特性

- **先进先出**：最先入队的元素最先出队
- **高效操作**：入队和出队操作时间复杂度为O(1)
- **循环数组**：使用循环数组实现，避免数据移动
- **动态扩容**：自动调整队列容量以适应更多元素
- **类型安全**：通过元素大小参数确保类型安全

## 数据结构

### Stru_Queue_t

队列的主要结构体：

```c
typedef struct Stru_Queue_t
{
    void** items;           // 元素指针数组
    size_t capacity;        // 当前分配的容量
    size_t front;           // 队头位置
    size_t rear;            // 队尾位置（下一个空闲位置）
    size_t size;            // 队列中元素数量
    size_t item_size;       // 每个元素的大小（字节）
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

### 队列管理

- `F_queue_clear(Stru_Queue_t* queue)` - 清空队列

## 使用示例

```c
#include "CN_queue.h"
#include <stdio.h>

int main() {
    // 创建队列
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    if (queue == NULL) {
        printf("创建队列失败\n");
        return 1;
    }
    
    // 入队操作
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        if (!F_queue_enqueue(queue, &values[i])) {
            printf("入队失败: %d\n", values[i]);
        }
    }
    
    printf("队列大小: %zu\n", F_queue_size(queue));
    printf("队列是否为空: %s\n", F_queue_is_empty(queue) ? "是" : "否");
    printf("队列是否已满: %s\n", F_queue_is_full(queue) ? "是" : "否");
    
    // 查看队头元素
    int* front = (int*)F_queue_peek(queue);
    if (front != NULL) {
        printf("队头元素: %d\n", *front);
    }
    
    // 出队操作
    printf("出队顺序（先进先出）:\n");
    while (!F_queue_is_empty(queue)) {
        int dequeued;
        if (F_queue_dequeue(queue, &dequeued)) {
            printf("出队: %d\n", dequeued);
        }
    }
    
    // 再次检查队列状态
    printf("出队后队列大小: %zu\n", F_queue_size(queue));
    printf("出队后队列是否为空: %s\n", F_queue_is_empty(queue) ? "是" : "否");
    
    // 清理
    F_destroy_queue(queue);
    
    return 0;
}
```

## 典型应用场景

### 1. 任务调度
```c
// 操作系统任务调度
// 新任务入队，调度器从队头取出任务执行
```

### 2. 消息队列
```c
// 进程间通信
// 发送进程将消息入队，接收进程从队头取出消息处理
```

### 3. 广度优先搜索
```c
// 图的广度优先搜索算法
// 使用队列存储待访问的节点
```

### 4. 打印队列
```c
// 打印机任务管理
// 打印任务按提交顺序入队，打印机按顺序处理
```

### 5. 缓冲区管理
```c
// 生产者-消费者问题
// 生产者将数据入队，消费者从队头取出数据
```

## 性能特征

- **时间复杂度**：
  - 入队：平均O(1)，最坏O(n)（需要扩容）
  - 出队：O(1)
  - 查看队头：O(1)
  - 获取大小：O(1)

- **空间复杂度**：O(n)，其中n是队列容量

## 循环数组实现

队列使用循环数组实现，具有以下优点：
1. **高效内存使用**：重复利用数组空间
2. **避免数据移动**：出队时不需要移动所有元素
3. **常数时间操作**：入队和出队都是O(1)操作

### 循环数组原理

```
初始状态:
front = 0, rear = 0, size = 0
[ ][ ][ ][ ][ ][ ]

入队A, B, C后:
front = 0, rear = 3, size = 3
[A][B][C][ ][ ][ ]

出队A后:
front = 1, rear = 3, size = 2
[ ][B][C][ ][ ][ ]

入队D, E, F后（循环利用）:
front = 1, rear = 0, size = 5
[F][B][C][D][E][ ]
```

## 内存管理

队列模块负责管理以下内存：
1. 队列结构体本身
2. 元素指针数组
3. 每个元素的实际数据

用户只需要提供要存储的数据，模块会自动处理内存分配和释放。

## 错误处理

所有函数都包含错误检查：
- 无效参数返回适当错误值（NULL、false等）
- 内存分配失败返回错误
- 队列空时出队返回false
- 队列满时入队返回false（除非自动扩容）

## 线程安全

当前实现不是线程安全的。如果需要在多线程环境中使用，需要外部同步机制。

## 配置参数

可以通过修改初始容量来优化性能：
- 如果知道队列的最大大小，可以设置合适的初始容量避免扩容
- 默认初始容量为16个元素

## 依赖关系

- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`
- 无其他项目模块依赖

## 测试

模块包含完整的单元测试，覆盖所有API接口和边界条件。测试覆盖率目标：
- 语句覆盖率：≥90%
- 分支覆盖率：≥80%
- 函数覆盖率：100%

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本队列功能 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
