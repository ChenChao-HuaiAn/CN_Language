# CN_Stack 模块

## 概述

CN_Stack 模块是 CN_Language 项目基础设施层中的栈容器实现。它提供了三种不同的栈实现方式，支持线程安全选项，并遵循项目的分层架构和 SOLID 设计原则。

## 设计原则

1. **单一职责原则**：每个文件功能单一，不超过500行代码
2. **开闭原则**：通过接口支持扩展，对修改封闭
3. **接口隔离原则**：为不同客户端提供专用接口
4. **依赖倒置原则**：高层模块定义接口，低层模块实现接口

## 文件结构

```
src/infrastructure/containers/stack/
├── CN_stack.h          # 公共接口定义
├── CN_stack_internal.h # 内部结构定义
├── CN_stack.c          # 主实现文件
├── CN_stack_api.c      # 公共API实现
└── README.md           # 本文档
```

## 实现类型

模块提供三种栈实现：

1. **数组实现 (CN_STACK_ARRAY)**：
   - 基于动态数组
   - 内存连续，缓存友好
   - 自动扩容机制

2. **链表实现 (CN_STACK_LINKED_LIST)**：
   - 基于单向链表
   - 动态内存分配
   - 无固定容量限制

3. **循环数组实现 (CN_STACK_CIRCULAR_ARRAY)**：
   - 基于循环数组
   - 避免数据移动
   - 固定容量但高效利用

## 线程安全

模块支持线程安全选项：
- **非线程安全 (CN_THREAD_SAFE_DISABLED)**：默认选项，性能最优
- **线程安全 (CN_THREAD_SAFE_ENABLED)**：使用平台特定的同步机制
  - Windows：临界区 (CRITICAL_SECTION)
  - POSIX：互斥锁 (pthread_mutex_t)

## 主要功能

### 基本操作
- `CN_stack_create()`：创建栈实例
- `CN_stack_destroy()`：销毁栈实例
- `CN_stack_push()`：元素入栈
- `CN_stack_pop()`：元素出栈
- `CN_stack_peek()`：查看栈顶元素
- `CN_stack_is_empty()`：检查栈是否为空
- `CN_stack_size()`：获取栈大小

### 高级功能
- `CN_stack_clear()`：清空栈
- `CN_stack_copy()`：复制栈
- `CN_stack_merge()`：合并栈
- `CN_stack_reverse()`：反转栈
- `CN_stack_sort()`：排序栈元素
- `CN_stack_find()`：查找元素
- `CN_stack_contains()`：检查是否包含元素

### 批量操作
- `CN_stack_push_multiple()`：批量入栈
- `CN_stack_pop_multiple()`：批量出栈

### 迭代器支持
- `CN_stack_iterator_create()`：创建迭代器
- `CN_stack_iterator_next()`：获取下一个元素
- `CN_stack_iterator_has_next()`：检查是否有下一个元素
- `CN_stack_iterator_destroy()`：销毁迭代器

### 工具函数
- `CN_stack_dump()`：调试输出栈内容
- `CN_stack_validate()`：验证栈完整性
- `CN_stack_get_capacity()`：获取栈容量
- `CN_stack_shrink_to_fit()`：收缩内存到合适大小

## 使用示例

### 创建栈
```c
#include "CN_stack.h"

// 创建数组实现的栈，启用线程安全
CN_Stack* stack = CN_stack_create(CN_STACK_ARRAY, CN_THREAD_SAFE_ENABLED, 100);
if (!stack) {
    // 处理错误
}
```

### 基本操作
```c
// 入栈
int value1 = 42;
CN_stack_push(stack, &value1, sizeof(int));

// 出栈
int popped_value;
size_t size;
void* data = CN_stack_pop(stack, &size);
if (data) {
    memcpy(&popped_value, data, size);
    cn_free(data);
}

// 查看栈顶
void* top = CN_stack_peek(stack, &size);
```

### 迭代器使用
```c
CN_StackIterator* iter = CN_stack_iterator_create(stack);
while (CN_stack_iterator_has_next(iter)) {
    void* data = CN_stack_iterator_next(iter, &size);
    // 处理数据
}
CN_stack_iterator_destroy(iter);
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

1. **时间复杂度**：
   - 入栈/出栈：O(1) 平均情况
   - 查找：O(n)
   - 排序：O(n log n)

2. **空间复杂度**：
   - 数组实现：O(n)
   - 链表实现：O(n) + 指针开销

## 测试

模块包含完整的单元测试，位于：
```
tests/infrastructure/containers/test_stack.c
```

测试覆盖：
- 基本功能测试
- 不同实现类型测试
- 线程安全测试
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

- v1.0.0 (2026-01-02)：初始版本，实现三种栈类型和线程安全

## 许可证

MIT License - 详见项目根目录 LICENSE 文件

## 作者

CN_Language 项目团队
