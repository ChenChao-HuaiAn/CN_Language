# CN_Queue 模块

## 概述

CN_Queue 模块是 CN_Language 项目基础设施层中的队列容器实现。它提供了三种不同的队列实现方式，支持线程安全选项，包含循环队列优化，并遵循项目的分层架构和 SOLID 设计原则。

## 设计原则

1. **单一职责原则**：每个文件功能单一，不超过500行代码
2. **开闭原则**：通过接口支持扩展，对修改封闭
3. **接口隔离原则**：为不同客户端提供专用接口
4. **依赖倒置原则**：高层模块定义接口，低层模块实现接口

## 文件结构

```
src/infrastructure/containers/queue/
├── CN_queue.h              # 公共接口定义
├── CN_queue_internal.h     # 内部结构定义
├── CN_queue.c              # 锁管理和通用函数
├── CN_queue_api.c          # 公共API实现
├── CN_queue_array.c        # 数组实现
├── CN_queue_list.c         # 链表实现
├── CN_queue_circular.c     # 循环数组实现
└── README.md               # 本文档
```

## 实现类型

模块提供三种队列实现：

1. **数组实现 (CN_QUEUE_ARRAY)**：
   - 基于动态数组
   - 内存连续，缓存友好
   - 自动扩容机制
   - 数据移动开销

2. **链表实现 (CN_QUEUE_LINKED_LIST)**：
   - 基于双向链表
   - 动态内存分配
   - 无固定容量限制
   - 高效的头尾操作

3. **循环数组实现 (CN_QUEUE_CIRCULAR_ARRAY)**：
   - 基于循环数组（优化实现）
   - 避免数据移动
   - 固定容量但高效利用
   - O(1) 时间复杂度的入队出队

## 线程安全

模块支持线程安全选项：
- **非线程安全 (CN_THREAD_SAFE_DISABLED)**：默认选项，性能最优
- **线程安全 (CN_THREAD_SAFE_ENABLED)**：使用平台特定的同步机制
  - Windows：临界区 (CRITICAL_SECTION)
  - POSIX：互斥锁 (pthread_mutex_t)

## 主要功能

### 基本操作
- `CN_queue_create()`：创建队列实例
- `CN_queue_destroy()`：销毁队列实例
- `CN_queue_enqueue()`：元素入队
- `CN_queue_dequeue()`：元素出队
- `CN_queue_peek_front()`：查看队首元素
- `CN_queue_peek_rear()`：查看队尾元素
- `CN_queue_is_empty()`：检查队列是否为空
- `CN_queue_size()`：获取队列大小

### 高级功能
- `CN_queue_clear()`：清空队列
- `CN_queue_copy()`：复制队列
- `CN_queue_merge()`：合并队列
- `CN_queue_reverse()`：反转队列
- `CN_queue_sort()`：排序队列元素
- `CN_queue_find()`：查找元素
- `CN_queue_contains()`：检查是否包含元素

### 批量操作
- `CN_queue_enqueue_multiple()`：批量入队
- `CN_queue_dequeue_multiple()`：批量出队

### 循环队列特定操作
- `CN_queue_is_full()`：检查队列是否已满（仅循环数组）
- `CN_queue_get_capacity()`：获取队列容量
- `CN_queue_reserve()`：预留容量
- `CN_queue_shrink_to_fit()`：收缩内存到合适大小

### 迭代器支持
- `CN_queue_iterator_create()`：创建迭代器
- `CN_queue_iterator_next()`：获取下一个元素
- `CN_queue_iterator_has_next()`：检查是否有下一个元素
- `CN_queue_iterator_destroy()`：销毁迭代器

### 工具函数
- `CN_queue_dump()`：调试输出队列内容
- `CN_queue_validate()`：验证队列完整性
- `CN_queue_get_implementation_type()`：获取实现类型
- `CN_queue_is_thread_safe()`：检查是否线程安全

## 使用示例

### 创建队列
```c
#include "CN_queue.h"

// 创建循环数组实现的队列，启用线程安全，容量100
CN_Queue* queue = CN_queue_create(CN_QUEUE_CIRCULAR_ARRAY, CN_THREAD_SAFE_ENABLED, 100);
if (!queue) {
    // 处理错误
}
```

### 基本操作
```c
// 入队
int value1 = 42;
CN_queue_enqueue(queue, &value1, sizeof(int));

// 出队
int dequeued_value;
size_t size;
void* data = CN_queue_dequeue(queue, &size);
if (data) {
    memcpy(&dequeued_value, data, size);
    cn_free(data);
}

// 查看队首
void* front = CN_queue_peek_front(queue, &size);
```

### 循环队列操作
```c
// 检查队列是否已满（仅循环数组）
if (CN_queue_is_full(queue)) {
    printf("队列已满\n");
}

// 获取队列容量
size_t capacity = CN_queue_get_capacity(queue);
```

### 迭代器使用
```c
CN_QueueIterator* iter = CN_queue_iterator_create(queue);
while (CN_queue_iterator_has_next(iter)) {
    void* data = CN_queue_iterator_next(iter, &size);
    // 处理数据
}
CN_queue_iterator_destroy(iter);
```

## 内存管理

模块使用项目统一的内存管理接口：
- `cn_malloc()`：内存分配
- `cn_free()`：内存释放
- `cn_calloc()`：清零内存分配
- `cn_realloc()`：内存重分配

## 错误处理

所有函数都返回明确的错误码或状态：
- 成功操作返回 `true` 或非空指针
- 失败操作返回 `false` 或 `NULL`
- 错误信息通过日志系统输出

## 性能特性

### 时间复杂度
1. **数组实现**：
   - 入队：O(1) 平均，O(n) 最坏（扩容时）
   - 出队：O(n)（需要移动数据）

2. **链表实现**：
   - 入队/出队：O(1)
   - 查找：O(n)

3. **循环数组实现**：
   - 入队/出队：O(1)
   - 空间利用率：接近100%

### 空间复杂度
- 数组实现：O(n)
- 链表实现：O(n) + 指针开销
- 循环数组实现：O(n) 固定

## 循环队列优化

循环数组实现采用了以下优化：
1. **避免数据移动**：使用头尾指针循环利用数组空间
2. **高效内存利用**：固定容量但完全利用
3. **缓存友好**：数据连续存储
4. **原子操作**：支持无锁实现选项

## 测试

模块包含完整的单元测试，位于：
```
tests/infrastructure/containers/test_queue.c
```

测试覆盖：
- 基本功能测试
- 不同实现类型测试
- 线程安全测试
- 循环队列优化测试
- 边界情况测试
- 性能测试

## 依赖关系

- 基础设施层：`memory` 模块（内存管理）
- C标准库：`stdlib.h`, `string.h`, `stdbool.h`
- 平台相关：Windows API 或 POSIX 线程

## 架构合规性

1. **分层架构**：属于基础设施层
2. **模块边界**：通过接口暴露功能，隐藏实现细节
3. **依赖方向**：只依赖C标准库和基础设施层其他模块
4. **编译独立**：可独立编译和测试

## 版本历史

- v1.0.0 (2026-01-02)：初始版本，实现三种队列类型、线程安全和循环队列优化

## 许可证

MIT License - 详见项目根目录 LICENSE 文件

## 作者

CN_Language 项目团队
