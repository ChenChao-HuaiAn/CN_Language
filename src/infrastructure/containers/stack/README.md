# 栈模块 (Stack Module) - 版本 2.0.0

## 概述

栈模块提供了完整、模块化的栈数据结构实现。栈是一种后进先出（LIFO）的数据结构，支持在栈顶进行压栈和弹栈操作。

本模块采用分层架构设计，分为三个子模块：
1. **核心模块** (`stack_core/`) - 基本栈操作
2. **迭代器模块** (`stack_iterator/`) - 栈遍历功能
3. **工具模块** (`stack_utils/`) - 高级工具功能

## 特性

- **模块化设计**：遵循单一职责原则，功能清晰分离
- **后进先出**：最后压入的元素最先弹出
- **高效操作**：压栈和弹栈操作时间复杂度为O(1)
- **动态数组**：使用动态数组实现，支持自动扩容
- **类型安全**：通过元素大小参数确保类型安全
- **完整迭代器**：支持从栈顶到栈底和从栈底到栈顶的遍历
- **批量操作**：支持批量压栈和弹栈
- **工具函数**：提供复制、比较、查找、反转等高级功能

## 模块结构

```
stack/
├── CN_stack.h              # 主头文件（包含所有子模块）
├── README.md              # 模块文档（本文件）
├── stack_core/            # 核心模块
│   ├── CN_stack_core.h    # 核心头文件
│   ├── CN_stack_core.c    # 核心实现
│   └── README.md          # 核心模块文档
├── stack_iterator/        # 迭代器模块
│   ├── CN_stack_iterator.h # 迭代器头文件
│   ├── CN_stack_iterator.c # 迭代器实现
│   └── README.md          # 迭代器模块文档
└── stack_utils/           # 工具模块
    ├── CN_stack_utils.h   # 工具头文件
    ├── CN_stack_utils.c   # 工具实现
    └── README.md          # 工具模块文档
```

## 数据结构

### Stru_Stack_t (核心模块)

栈的主要结构体：

```c
typedef struct Stru_Stack_t
{
    void** items;           /**< 元素指针数组 */
    size_t capacity;        /**< 当前分配的容量 */
    size_t top;             /**< 栈顶位置（下一个空闲位置） */
    size_t item_size;       /**< 每个元素的大小（字节） */
} Stru_Stack_t;
```

### Stru_StackIterator_t (迭代器模块)

栈迭代器结构体：

```c
typedef struct Stru_StackIterator_t
{
    Stru_Stack_t* stack;                    /**< 要遍历的栈 */
    size_t current_index;                   /**< 当前遍历位置 */
    size_t visited_count;                   /**< 已访问元素数量 */
    Eum_StackTraversalDirection direction;  /**< 遍历方向 */
} Stru_StackIterator_t;
```

## API 接口

### 核心模块 API

#### 创建和销毁
- `F_create_stack(size_t item_size)` - 创建栈
- `F_destroy_stack(Stru_Stack_t* stack)` - 销毁栈

#### 栈操作
- `F_stack_push(Stru_Stack_t* stack, void* item)` - 压栈
- `F_stack_pop(Stru_Stack_t* stack, void* out_item)` - 弹栈
- `F_stack_peek(Stru_Stack_t* stack)` - 查看栈顶元素

#### 查询操作
- `F_stack_size(Stru_Stack_t* stack)` - 获取栈大小
- `F_stack_is_empty(Stru_Stack_t* stack)` - 检查栈是否为空
- `F_stack_is_full(Stru_Stack_t* stack)` - 检查栈是否已满
- `F_stack_capacity(Stru_Stack_t* stack)` - 获取栈容量

#### 栈管理
- `F_stack_clear(Stru_Stack_t* stack)` - 清空栈
- `F_stack_reserve(Stru_Stack_t* stack, size_t new_capacity)` - 调整栈容量

### 迭代器模块 API

#### 迭代器管理
- `F_create_stack_iterator(Stru_Stack_t* stack)` - 创建栈迭代器
- `F_create_stack_iterator_with_direction(Stru_Stack_t* stack, Eum_StackTraversalDirection direction)` - 创建带方向的栈迭代器
- `F_destroy_stack_iterator(Stru_StackIterator_t* iterator)` - 销毁迭代器

#### 遍历操作
- `F_stack_iterator_has_next(Stru_StackIterator_t* iterator)` - 检查是否有下一个元素
- `F_stack_iterator_next(Stru_StackIterator_t* iterator, void* out_item)` - 获取下一个元素
- `F_stack_iterator_current(Stru_StackIterator_t* iterator)` - 获取当前元素
- `F_stack_iterator_reset(Stru_StackIterator_t* iterator)` - 重置迭代器
- `F_stack_iterator_set_direction(Stru_StackIterator_t* iterator, Eum_StackTraversalDirection direction)` - 设置遍历方向

#### 状态查询
- `F_stack_iterator_visited_count(Stru_StackIterator_t* iterator)` - 获取已访问元素数量
- `F_stack_iterator_remaining_count(Stru_StackIterator_t* iterator)` - 获取剩余元素数量
- `F_stack_iterator_get_direction(Stru_StackIterator_t* iterator)` - 获取遍历方向

### 工具模块 API

#### 栈操作
- `F_stack_copy(Stru_Stack_t* src_stack)` - 复制栈
- `F_stack_equals(Stru_Stack_t* stack1, Stru_Stack_t* stack2, int (*compare_func)(const void*, const void*))` - 比较栈
- `F_stack_push_batch(Stru_Stack_t* stack, void** items, size_t count)` - 批量压栈
- `F_stack_pop_batch(Stru_Stack_t* stack, void** out_items, size_t max_count)` - 批量弹栈

#### 转换操作
- `F_stack_to_array(Stru_Stack_t* stack, void** out_array)` - 将栈转换为数组
- `F_stack_from_array(void** items, size_t count, size_t item_size)` - 从数组创建栈

#### 查找操作
- `F_stack_find(Stru_Stack_t* stack, void* item, int (*compare_func)(const void*, const void*))` - 查找元素
- `F_stack_contains(Stru_Stack_t* stack, void* item, int (*compare_func)(const void*, const void*))` - 检查是否包含元素
- `F_stack_find_if(Stru_Stack_t* stack, bool (*predicate)(const void*, void*), void* context)` - 查找满足条件的元素

#### 栈操作工具
- `F_stack_reverse(Stru_Stack_t* stack)` - 反转栈
- `F_stack_min(Stru_Stack_t* stack, int (*compare_func)(const void*, const void*), void* out_item)` - 获取栈中最小元素
- `F_stack_max(Stru_Stack_t* stack, int (*compare_func)(const void*, const void*), void* out_item)` - 获取栈中最大元素

### 模块管理 API
- `F_stack_get_version(int* major, int* minor, int* patch)` - 获取模块版本
- `F_stack_get_version_string(void)` - 获取版本字符串
- `F_stack_module_init(void)` - 初始化模块
- `F_stack_module_cleanup(void)` - 清理模块

## 使用示例

### 基本使用示例

```c
#include "CN_stack.h"
#include <stdio.h>

int main() {
    // 初始化栈模块
    F_stack_module_init();
    
    // 创建栈
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    if (stack == NULL) {
        printf("创建栈失败\n");
        return 1;
    }
    
    // 压栈操作
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        if (!F_stack_push(stack, &values[i])) {
            printf("压栈失败: %d\n", values[i]);
        }
    }
    
    printf("栈大小: %zu\n", F_stack_size(stack));
    printf("栈容量: %zu\n", F_stack_capacity(stack));
    printf("栈是否为空: %s\n", F_stack_is_empty(stack) ? "是" : "否");
    printf("栈是否已满: %s\n", F_stack_is_full(stack) ? "是" : "否");
    
    // 查看栈顶元素
    int* top = (int*)F_stack_peek(stack);
    if (top != NULL) {
        printf("栈顶元素: %d\n", *top);
    }
    
    // 弹栈操作
    printf("弹栈顺序（后进先出）:\n");
    while (!F_stack_is_empty(stack)) {
        int popped;
        if (F_stack_pop(stack, &popped)) {
            printf("弹出: %d\n", popped);
        }
    }
    
    // 再次检查栈状态
    printf("弹栈后栈大小: %zu\n", F_stack_size(stack));
    printf("弹栈后栈是否为空: %s\n", F_stack_is_empty(stack) ? "是" : "否");
    
    // 清理
    F_destroy_stack(stack);
    F_stack_module_cleanup();
    
    return 0;
}
```

### 迭代器使用示例

```c
#include "CN_stack.h"
#include <stdio.h>

int main() {
    // 创建栈并添加元素
    Stru_Stack_t* stack = F_create_stack(sizeof(int));
    int values[] = {10, 20, 30, 40, 50};
    
    for (int i = 0; i < 5; i++) {
        F_stack_push(stack, &values[i]);
    }
    
    // 创建迭代器（默认从栈顶到栈底）
    Stru_StackIterator_t* iterator = F_create_stack_iterator(stack);
    
    printf("从栈顶到栈底遍历（LIFO顺序）:\n");
    while (F_stack_iterator_has_next(iterator)) {
        int value;
        F_stack_iterator_next(iterator, &value);
        printf("元素: %d\n", value);
    }
    
    // 设置遍历方向为从栈底到栈顶
    F_stack_iterator_set_direction(iterator, Eum_STACK_TRAVERSAL_BOTTOM_TO_TOP);
    F_stack_iterator_reset(iterator);
    
    printf("\n从栈底到栈顶遍历（FIFO顺序）:\n");
    while (F_stack_iterator_has_next(iterator)) {
        int value;
        F_stack_iterator_next(iterator, &value);
        printf("元素: %d\n", value);
    }
    
    printf("已访问元素数量: %zu\n", F_stack_iterator_visited_count(iterator));
    
    // 清理
    F_destroy_stack_iterator(iterator);
    F_destroy_stack(stack);
    
    return 0;
}
```

### 工具函数使用示例

```c
#include "CN_stack.h"
#include <stdio.h>
#include <string.h>

// 自定义比较函数
int compare_integers(const void* a, const void* b) {
    int int_a = *(const int*)a;
    int int_b = *(const int*)b;
    return int_a - int_b;
}

int main() {
    // 创建栈
    Stru_Stack_t* stack1 = F_create_stack(sizeof(int));
    
    // 批量压栈
    int values[] = {30, 10, 50, 20, 40};
    size_t count = sizeof(values) / sizeof(values[0]);
    
    F_stack_push_batch(stack1, (void**)values, count);
    
    // 复制栈
    Stru_Stack_t* stack2 = F_stack_copy(stack1);
    
    // 比较栈
    bool equal = F_stack_equals(stack1, stack2, compare_integers);
    printf("栈相等: %s\n", equal ? "是" : "否");
    
    // 查找元素
    int search_value = 20;
    int position = F_stack_find(stack1, &search_value, compare_integers);
    printf("值 %d 在栈中的位置: %d\n", search_value, position);
    
    // 获取最小元素
    int min_value;
    if (F_stack_min(stack1, compare_integers, &min_value)) {
        printf("最小元素: %d\n", min_value);
    }
    
    // 获取最大元素
    int max_value;
    if (F_stack_max(stack1, compare_integers, &max_value)) {
        printf("最大元素: %d\n", max_value);
    }
    
    // 反转栈
    F_stack_reverse(stack1);
    printf("\n反转后栈顶元素: %d\n", *(int*)F_stack_peek(stack1));
    
    // 清理
    F_destroy_stack(stack1);
    F_destroy_stack(stack2);
    
    return 0;
}
```

## 典型应用场景

### 1. 函数调用栈
```c
// 操作系统函数调用栈
// 函数调用时压栈，返回时弹栈
// 使用迭代器可以遍历调用栈进行调试
```

### 2. 表达式求值
```c
// 后缀表达式求值
// 表达式: "3 4 + 5 *" 对应 (3 + 4) * 5
// 使用栈存储操作数，工具函数支持批量操作
```

### 3. 括号匹配检查
```c
// 编译器语法检查
// 使用栈存储左括号，遇到右括号时弹栈检查
// 查找函数可用于定位不匹配的括号位置
```

### 4. 撤销操作
```c
// 文本编辑器中的撤销功能
// 每次编辑操作压栈，撤销时弹栈恢复之前状态
// 批量操作支持同时撤销多个操作
```

### 5. 深度优先搜索算法
```c
// 图的深度优先搜索算法
// 使用栈存储待访问的节点
// 迭代器支持遍历已访问节点，工具函数支持节点查找
```

### 6. 递归转迭代
```c
// 将递归算法转换为迭代算法
// 使用栈模拟递归调用栈
// 工具函数支持栈的复制和比较，便于调试
```

## 性能特征

- **时间复杂度**：
  - 压栈：平均O(1)，最坏O(n)（需要扩容）
  - 弹栈：O(1)
  - 查看栈顶：O(1)
  - 获取大小：O(1)
  - 遍历：O(n)

- **空间复杂度**：O(n)，其中n是栈容量

## 内存管理

栈模块负责管理以下内存：
1. 栈结构体本身
2. 元素指针数组
3. 每个元素的实际数据

用户只需要提供要存储的数据，模块会自动处理内存分配和释放。

## 错误处理

所有函数都包含错误检查：
- 无效参数返回适当错误值（NULL、false等）
- 内存分配失败返回错误
- 栈空时弹栈返回false
- 栈满时压栈会自动扩容，不会失败（除非内存不足）

## 线程安全

当前实现不是线程安全的。如果需要在多线程环境中使用，需要外部同步机制。

## 配置参数

可以通过修改初始容量来优化性能：
- 如果知道栈的最大大小，可以设置合适的初始容量避免扩容
- 默认初始容量为16个元素

## 依赖关系

- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`
- 无其他项目模块依赖

## 测试

模块包含完整的单元测试，覆盖所有API接口和边界条件。测试文件位于`tests/infrastructure/containers/stack/`目录，包括：

- `test_stack_core.c` - 核心模块测试
- `test_stack_iterator.c` - 迭代器模块测试  
- `test_stack_utils.c` - 工具模块测试
- `test_stack_integration.c` - 集成测试
- `test_stack_main.c` - 独立测试运行器
- `test_stack_runner.c` - 模块化测试运行器

测试覆盖率目标：
- 语句覆盖率：≥90%
- 分支覆盖率：≥80%
- 函数覆盖率：100%

## 相关文档

### 子模块文档
- [栈核心模块文档](stack_core/README.md)
- [栈迭代器模块文档](stack_iterator/README.md)
- [栈工具模块文档](stack_utils/README.md)

### API文档
- [栈模块API文档](../../../docs/api/infrastructure/containers/stack/CN_stack.md)

### 架构文档
- [CN_Language项目架构文档](../../../docs/architecture/001-中文编程语言CN_Language开发规划.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本栈功能 |
| 2.0.0 | 2026-01-07 | 模块化重构，增加迭代器和工具模块 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
