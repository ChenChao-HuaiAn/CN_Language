# Stack Iterator 模块

## 概述

Stack Iterator 模块为栈数据结构提供了灵活的遍历功能，支持多种遍历方向。该模块实现了迭代器模式，允许用户以不同的顺序访问栈中的元素，而无需了解栈的内部实现细节。

## 功能特性

- **双向遍历支持**：支持从栈顶到栈底（LIFO顺序）和从栈底到栈顶（FIFO顺序）两种遍历方向
- **安全迭代**：迭代器在遍历过程中检查栈状态，防止越界访问
- **状态保持**：迭代器独立于栈对象，可以同时存在多个迭代器
- **错误处理**：提供完整的错误检查和状态报告机制

## 数据结构

### 遍历方向枚举
```c
typedef enum Eum_StackTraversalDirection {
    Eum_STACK_TRAVERSAL_TOP_TO_BOTTOM,  // 从栈顶到栈底（默认LIFO顺序）
    Eum_STACK_TRAVERSAL_BOTTOM_TO_TOP   // 从栈底到栈顶（FIFO顺序）
} Eum_StackTraversalDirection;
```

### 迭代器结构
```c
typedef struct Stru_StackIterator_t {
    Stru_Stack_t* stack;                    // 指向栈对象的指针
    size_t current_index;                   // 当前遍历位置
    Eum_StackTraversalDirection direction;  // 遍历方向
    bool is_valid;                          // 迭代器有效性标志
} Stru_StackIterator_t;
```

## API 接口

### 迭代器创建和销毁
- `Stru_StackIterator_t* F_stack_iterator_create(Stru_Stack_t* stack, Eum_StackTraversalDirection direction)`
  - 创建栈迭代器
  - 参数：stack - 要遍历的栈对象，direction - 遍历方向
  - 返回：新创建的迭代器指针，失败返回NULL

- `void F_stack_iterator_destroy(Stru_StackIterator_t* iterator)`
  - 销毁迭代器并释放资源

### 迭代器操作
- `void* F_stack_iterator_next(Stru_StackIterator_t* iterator)`
  - 获取下一个元素
  - 返回：指向下一个元素的指针，遍历完成返回NULL

- `void* F_stack_iterator_current(Stru_StackIterator_t* iterator)`
  - 获取当前元素
  - 返回：指向当前元素的指针，无效状态返回NULL

- `bool F_stack_iterator_has_next(Stru_StackIterator_t* iterator)`
  - 检查是否还有下一个元素
  - 返回：true表示还有元素，false表示遍历完成

- `void F_stack_iterator_reset(Stru_StackIterator_t* iterator)`
  - 重置迭代器到起始位置

### 状态查询
- `bool F_stack_iterator_is_valid(Stru_StackIterator_t* iterator)`
  - 检查迭代器是否有效
  - 返回：true表示有效，false表示无效

- `size_t F_stack_iterator_get_position(Stru_StackIterator_t* iterator)`
  - 获取当前遍历位置
  - 返回：当前位置索引

## 使用示例

```c
#include "CN_stack_iterator.h"

// 创建栈
Stru_Stack_t* stack = F_stack_create(sizeof(int), 10);

// 压入一些数据
for (int i = 0; i < 5; i++) {
    F_stack_push(stack, &i);
}

// 创建从栈顶到栈底的迭代器
Stru_StackIterator_t* iterator = F_stack_iterator_create(
    stack, 
    Eum_STACK_TRAVERSAL_TOP_TO_BOTTOM
);

// 遍历栈
while (F_stack_iterator_has_next(iterator)) {
    int* value = (int*)F_stack_iterator_next(iterator);
    if (value) {
        printf("元素: %d\n", *value);
    }
}

// 清理
F_stack_iterator_destroy(iterator);
F_stack_destroy(stack);
```

## 性能特点

- **时间复杂度**：
  - 创建迭代器：O(1)
  - 获取下一个元素：O(1)
  - 重置迭代器：O(1)
  - 销毁迭代器：O(1)

- **空间复杂度**：
  - 迭代器本身：O(1) 额外空间
  - 不复制栈数据，只保存引用

## 设计原则

1. **单一职责原则**：迭代器模块只负责遍历功能，不涉及栈的核心操作
2. **开闭原则**：通过遍历方向枚举支持扩展新的遍历方式
3. **接口隔离原则**：提供最小化的必要接口，避免功能冗余
4. **依赖倒置原则**：迭代器依赖于栈的抽象接口，而非具体实现

## 注意事项

1. 迭代器创建后，如果栈被修改（压入/弹出元素），迭代器可能进入无效状态
2. 使用 `F_stack_iterator_is_valid()` 检查迭代器状态
3. 迭代器不持有栈的所有权，栈对象必须在迭代器生命周期内保持有效
4. 多线程环境下需要外部同步机制

## 测试覆盖

- 遍历方向测试：验证两种遍历方向的正确性
- 边界条件测试：空栈、单元素栈、满栈的遍历
- 状态验证测试：迭代器有效性检查
- 错误处理测试：无效参数、空指针等异常情况

## 相关模块

- [Stack Core](../stack_core/README.md)：栈核心功能模块
- [Stack Utils](../stack_utils/README.md)：栈工具函数模块
- [Main Stack](../README.md)：栈模块主文档
