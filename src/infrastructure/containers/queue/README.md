# 队列模块 (Queue Module) - 版本 2.0.0

## 概述

队列模块提供了完整、模块化的队列数据结构实现。队列是一种先进先出（FIFO）的数据结构，支持在队尾入队、在队头出队操作。

本模块采用分层架构设计，分为三个子模块：
1. **核心模块** (`queue_core/`) - 基本队列操作
2. **迭代器模块** (`queue_iterator/`) - 队列遍历功能
3. **工具模块** (`queue_utils/`) - 高级工具功能

## 特性

- **模块化设计**：遵循单一职责原则，功能清晰分离
- **先进先出**：最先入队的元素最先出队
- **高效操作**：入队和出队操作时间复杂度为O(1)
- **循环数组**：使用循环数组实现，避免数据移动
- **动态扩容**：自动调整队列容量以适应更多元素
- **类型安全**：通过元素大小参数确保类型安全
- **完整迭代器**：支持顺序遍历队列元素
- **批量操作**：支持批量入队和出队
- **工具函数**：提供复制、比较、查找等高级功能

## 模块结构

```
queue/
├── CN_queue.h              # 主头文件（包含所有子模块）
├── README.md              # 模块文档（本文件）
├── queue_core/            # 核心模块
│   ├── CN_queue_core.h    # 核心头文件
│   ├── CN_queue_core.c    # 核心实现
│   └── README.md          # 核心模块文档
├── queue_iterator/        # 迭代器模块
│   ├── CN_queue_iterator.h # 迭代器头文件
│   ├── CN_queue_iterator.c # 迭代器实现
│   └── README.md          # 迭代器模块文档
└── queue_utils/           # 工具模块
    ├── CN_queue_utils.h   # 工具头文件
    ├── CN_queue_utils.c   # 工具实现
    └── README.md          # 工具模块文档
```

## 数据结构

### Stru_Queue_t (核心模块)

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

### Stru_QueueIterator_t (迭代器模块)

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

### 核心模块 API

#### 创建和销毁
- `F_create_queue(size_t item_size)` - 创建队列
- `F_destroy_queue(Stru_Queue_t* queue)` - 销毁队列

#### 队列操作
- `F_queue_enqueue(Stru_Queue_t* queue, void* item)` - 入队
- `F_queue_dequeue(Stru_Queue_t* queue, void* out_item)` - 出队
- `F_queue_peek(Stru_Queue_t* queue)` - 查看队头元素

#### 查询操作
- `F_queue_size(Stru_Queue_t* queue)` - 获取队列大小
- `F_queue_is_empty(Stru_Queue_t* queue)` - 检查队列是否为空
- `F_queue_is_full(Stru_Queue_t* queue)` - 检查队列是否已满
- `F_queue_capacity(Stru_Queue_t* queue)` - 获取队列容量

#### 队列管理
- `F_queue_clear(Stru_Queue_t* queue)` - 清空队列
- `F_queue_reserve(Stru_Queue_t* queue, size_t new_capacity)` - 调整队列容量

### 迭代器模块 API

#### 迭代器管理
- `F_create_queue_iterator(Stru_Queue_t* queue)` - 创建队列迭代器
- `F_destroy_queue_iterator(Stru_QueueIterator_t* iterator)` - 销毁迭代器

#### 遍历操作
- `F_queue_iterator_has_next(Stru_QueueIterator_t* iterator)` - 检查是否有下一个元素
- `F_queue_iterator_next(Stru_QueueIterator_t* iterator, void* out_item)` - 获取下一个元素
- `F_queue_iterator_current(Stru_QueueIterator_t* iterator)` - 获取当前元素
- `F_queue_iterator_reset(Stru_QueueIterator_t* iterator)` - 重置迭代器

#### 状态查询
- `F_queue_iterator_visited_count(Stru_QueueIterator_t* iterator)` - 获取已访问元素数量
- `F_queue_iterator_remaining_count(Stru_QueueIterator_t* iterator)` - 获取剩余元素数量

### 工具模块 API

#### 队列操作
- `F_queue_copy(Stru_Queue_t* src_queue)` - 复制队列
- `F_queue_equals(Stru_Queue_t* queue1, Stru_Queue_t* queue2, int (*compare_func)(const void*, const void*))` - 比较队列
- `F_queue_enqueue_batch(Stru_Queue_t* queue, void** items, size_t count)` - 批量入队
- `F_queue_dequeue_batch(Stru_Queue_t* queue, void** out_items, size_t max_count)` - 批量出队

#### 转换操作
- `F_queue_to_array(Stru_Queue_t* queue, void** out_array)` - 将队列转换为数组
- `F_queue_from_array(void** items, size_t count, size_t item_size)` - 从数组创建队列

#### 查找操作
- `F_queue_find(Stru_Queue_t* queue, void* item, int (*compare_func)(const void*, const void*))` - 查找元素
- `F_queue_contains(Stru_Queue_t* queue, void* item, int (*compare_func)(const void*, const void*))` - 检查是否包含元素

### 模块管理 API
- `F_queue_get_version(int* major, int* minor, int* patch)` - 获取模块版本
- `F_queue_get_version_string(void)` - 获取版本字符串
- `F_queue_module_init(void)` - 初始化模块
- `F_queue_module_cleanup(void)` - 清理模块

## 使用示例

### 基本使用示例

```c
#include "CN_queue.h"
#include <stdio.h>

int main() {
    // 初始化队列模块
    F_queue_module_init();
    
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
    printf("队列容量: %zu\n", F_queue_capacity(queue));
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
    F_queue_module_cleanup();
    
    return 0;
}
```

### 迭代器使用示例

```c
#include "CN_queue.h"
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
    
    printf("使用迭代器遍历队列:\n");
    while (F_queue_iterator_has_next(iterator)) {
        int value;
        F_queue_iterator_next(iterator, &value);
        printf("元素: %d\n", value);
    }
    
    printf("已访问元素数量: %zu\n", F_queue_iterator_visited_count(iterator));
    
    // 重置迭代器并再次遍历
    F_queue_iterator_reset(iterator);
    printf("\n重置后再次遍历:\n");
    while (F_queue_iterator_has_next(iterator)) {
        int value;
        F_queue_iterator_next(iterator, &value);
        printf("元素: %d\n", value);
    }
    
    // 清理
    F_destroy_queue_iterator(iterator);
    F_destroy_queue(queue);
    
    return 0;
}
```

### 工具函数使用示例

```c
#include "CN_queue.h"
#include <stdio.h>
#include <string.h>

// 自定义比较函数
int compare_strings(const void* a, const void* b) {
    return strcmp((const char*)a, (const char*)b);
}

int main() {
    // 创建字符串队列
    Stru_Queue_t* queue1 = F_create_queue(32); // 假设字符串最大长度为31
    
    // 批量入队
    const char* strings[] = {"Hello", "World", "Queue", "Example"};
    size_t count = sizeof(strings) / sizeof(strings[0]);
    
    F_queue_enqueue_batch(queue1, (void**)strings, count);
    
    // 复制队列
    Stru_Queue_t* queue2 = F_queue_copy(queue1);
    
    // 比较队列
    bool equal = F_queue_equals(queue1, queue2, compare_strings);
    printf("队列相等: %s\n", equal ? "是" : "否");
    
    // 查找元素
    const char* search = "World";
    int position = F_queue_find(queue1, (void*)search, compare_strings);
    printf("'%s' 在队列中的位置: %d\n", search, position);
    
    // 检查是否包含元素
    bool contains = F_queue_contains(queue1, (void*)"Example", compare_strings);
    printf("包含 'Example': %s\n", contains ? "是" : "否");
    
    // 批量出队
    char* dequeued[4];
    size_t dequeued_count = F_queue_dequeue_batch(queue1, (void**)dequeued, 4);
    printf("批量出队数量: %zu\n", dequeued_count);
    
    // 清理
    F_destroy_queue(queue1);
    F_destroy_queue(queue2);
    
    return 0;
}
```

## 典型应用场景

### 1. 任务调度系统
```c
// 操作系统任务调度
// 新任务入队，调度器从队头取出任务执行
// 使用迭代器可以监控所有待处理任务
```

### 2. 消息队列系统
```c
// 进程间通信或微服务通信
// 发送进程将消息入队，接收进程从队头取出消息处理
// 批量操作提高吞吐量，工具函数支持消息查找和过滤
```

### 3. 广度优先搜索算法
```c
// 图的广度优先搜索算法
// 使用队列存储待访问的节点
// 迭代器支持遍历已访问节点，工具函数支持节点查找
```

### 4. 打印队列管理系统
```c
// 打印机任务管理
// 打印任务按提交顺序入队，打印机按顺序处理
// 批量操作支持批量提交打印任务
```

### 5. 生产者-消费者缓冲区
```c
// 生产者-消费者问题
// 生产者将数据入队，消费者从队头取出数据
// 容量管理支持动态调整缓冲区大小
```

### 6. 网络数据包队列
```c
// 网络设备数据包缓冲
// 接收的数据包入队，处理引擎按顺序出队处理
// 批量操作提高处理效率，工具函数支持数据包查找
```

### 7. 事件处理

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

模块包含完整的单元测试，覆盖所有API接口和边界条件。测试文件位于`tests/infrastructure/containers/queue/`目录，包括：

- `test_queue_core.c` - 核心模块测试
- `test_queue_iterator.c` - 迭代器模块测试  
- `test_queue_utils.c` - 工具模块测试
- `test_queue_integration.c` - 集成测试
- `test_queue_main.c` - 独立测试运行器
- `test_queue_runner.c` - 模块化测试运行器

测试覆盖率目标：
- 语句覆盖率：≥90%
- 分支覆盖率：≥80%
- 函数覆盖率：100%

## 相关文档

### 子模块文档
- [队列核心模块文档](queue_core/README.md)
- [队列迭代器模块文档](queue_iterator/README.md)
- [队列工具模块文档](queue_utils/README.md)

### API文档
- [队列模块API文档](../../../docs/api/infrastructure/containers/queue/CN_queue.md)

### 架构文档
- [CN_Language项目架构文档](../../../docs/architecture/001-中文编程语言CN_Language开发规划.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本队列功能 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
