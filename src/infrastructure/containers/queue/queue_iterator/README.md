# 队列迭代器模块 (Queue Iterator Module)

## 概述

队列迭代器模块提供了遍历队列元素的功能。迭代器允许顺序访问队列中的元素，支持前进遍历、重置、状态查询等操作。

## 功能特性

- **迭代器创建和销毁**：安全的迭代器生命周期管理
- **顺序遍历**：按队列顺序访问元素
- **状态查询**：获取已访问和剩余元素数量
- **迭代器重置**：将迭代器重置到队列开始位置
- **当前元素访问**：查看当前指向的元素而不移动迭代器

## 数据结构

### Stru_QueueIterator_t

队列迭代器结构体：

```c
typedef struct Stru_QueueIterator_t
{
    Stru_Queue_t* queue;    /**< 要遍历的队列 */
    size_t current_index;   /**< 当前遍历位置 */
    size_t visited_count;   /**< 已访问元素数量 */
} Stru_QueueIterator_t;
```

## API 接口

### 迭代器管理

- `F_create_queue_iterator(Stru_Queue_t* queue)` - 创建队列迭代器
- `F_destroy_queue_iterator(Stru_QueueIterator_t* iterator)` - 销毁迭代器

### 遍历操作

- `F_queue_iterator_has_next(Stru_QueueIterator_t* iterator)` - 检查是否有下一个元素
- `F_queue_iterator_next(Stru_QueueIterator_t* iterator, void* out_item)` - 获取下一个元素
- `F_queue_iterator_current(Stru_QueueIterator_t* iterator)` - 获取当前元素
- `F_queue_iterator_reset(Stru_QueueIterator_t* iterator)` - 重置迭代器

### 状态查询

- `F_queue_iterator_visited_count(Stru_QueueIterator_t* iterator)` - 获取已访问元素数量
- `F_queue_iterator_remaining_count(Stru_QueueIterator_t* iterator)` - 获取剩余元素数量

## 实现细节

### 迭代器状态管理

迭代器维护以下状态信息：
1. **当前遍历位置**：指向队列中下一个要访问的元素
2. **已访问计数**：记录已经访问过的元素数量
3. **队列引用**：指向要遍历的队列

### 遍历算法

迭代器按照队列的先进先出顺序遍历元素：
1. 从队头位置开始
2. 按顺序访问每个元素
3. 处理循环数组的环绕情况
4. 在遍历结束时停止

### 线程安全考虑

迭代器不是线程安全的。如果在遍历过程中队列被修改，迭代器的行为是未定义的。

## 使用示例

```c
#include "queue_iterator/CN_queue_iterator.h"
#include "queue_core/CN_queue_core.h"
#include <stdio.h>

int main() {
    // 创建队列并添加元素
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    int values[] = {10, 20, 30, 40, 50};
    
    for (int i = 0; i < 5; i++) {
        F_queue_enqueue(queue, &values[i]);
    }
    
    // 创建迭代器
    Stru_QueueIterator_t* iterator = F_create_queue_iterator(queue);
    if (iterator == NULL) {
        printf("创建迭代器失败\n");
        F_destroy_queue(queue);
        return 1;
    }
    
    // 使用迭代器遍历队列
    printf("使用迭代器遍历队列:\n");
    while (F_queue_iterator_has_next(iterator)) {
        int value;
        F_queue_iterator_next(iterator, &value);
        printf("元素: %d\n", value);
    }
    
    // 查询迭代器状态
    printf("已访问元素数量: %zu\n", F_queue_iterator_visited_count(iterator));
    printf("剩余元素数量: %zu\n", F_queue_iterator_remaining_count(iterator));
    
    // 重置迭代器并再次遍历
    F_queue_iterator_reset(iterator);
    printf("\n重置后再次遍历:\n");
    while (F_queue_iterator_has_next(iterator)) {
        int value;
        F_queue_iterator_next(iterator, &value);
        printf("元素: %d\n", value);
    }
    
    // 获取当前元素（遍历结束后应为NULL）
    void* current = F_queue_iterator_current(iterator);
    if (current == NULL) {
        printf("当前元素: NULL (遍历结束)\n");
    }
    
    // 清理
    F_destroy_queue_iterator(iterator);
    F_destroy_queue(queue);
    return 0;
}
```

## 性能特征

### 时间复杂度
- 创建迭代器：O(1)
- 销毁迭代器：O(1)
- 检查是否有下一个元素：O(1)
- 获取下一个元素：O(1)
- 重置迭代器：O(1)
- 状态查询：O(1)

### 空间复杂度
- O(1)，迭代器只存储少量状态信息

## 错误处理

所有函数都包含错误检查：
- 无效参数返回适当错误值（NULL、false等）
- 迭代器结束返回false
- 空队列返回适当错误值

## 使用场景

### 1. 队列监控
```c
// 监控队列中的所有任务
Stru_QueueIterator_t* monitor = F_create_queue_iterator(task_queue);
while (F_queue_iterator_has_next(monitor)) {
    Task task;
    F_queue_iterator_next(monitor, &task);
    log_task_status(task);
}
F_destroy_queue_iterator(monitor);
```

### 2. 队列处理
```c
// 处理队列中的所有消息
Stru_QueueIterator_t* processor = F_create_queue_iterator(message_queue);
while (F_queue_iterator_has_next(processor)) {
    Message msg;
    F_queue_iterator_next(processor, &msg);
    process_message(msg);
}
F_destroy_queue_iterator(processor);
```

### 3. 队列统计
```c
// 统计队列中的元素
Stru_QueueIterator_t* stats = F_create_queue_iterator(data_queue);
size_t total_size = 0;
while (F_queue_iterator_has_next(stats)) {
    DataItem item;
    F_queue_iterator_next(stats, &item);
    total_size += item.size;
}
printf("队列总大小: %zu\n", total_size);
F_destroy_queue_iterator(stats);
```

## 依赖关系

- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`
- 队列核心模块：`queue_core/CN_queue_core.h`

## 测试

迭代器模块包含完整的单元测试，覆盖所有API接口和边界条件。

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-07 | 初始版本 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
