# 栈核心模块 (Stack Core Module)

## 概述

栈核心模块提供了栈数据结构的基本操作实现。这是栈模块的基础，负责栈的创建、销毁、压栈、弹栈等核心功能。

## 特性

- **后进先出**：最后压入的元素最先弹出
- **高效操作**：压栈和弹栈操作时间复杂度为O(1)
- **动态扩容**：自动调整栈容量以适应更多元素
- **类型安全**：通过元素大小参数确保类型安全
- **内存管理**：自动管理内存分配和释放

## 数据结构

### Stru_Stack_t

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

## API 接口

### 创建和销毁

#### `F_create_stack`
```c
Stru_Stack_t* F_create_stack(size_t item_size);
```
创建栈。分配并初始化一个新的栈。

**参数**：
- `item_size`：每个元素的大小（字节）

**返回值**：
- 成功：指向新创建的栈的指针
- 失败：NULL（如果item_size为0或内存分配失败）

#### `F_destroy_stack`
```c
void F_destroy_stack(Stru_Stack_t* stack);
```
销毁栈。释放栈占用的所有内存。

**参数**：
- `stack`：要销毁的栈指针

**注意**：如果stack为NULL，函数不执行任何操作

### 栈操作

#### `F_stack_push`
```c
bool F_stack_push(Stru_Stack_t* stack, void* item);
```
压栈。将元素压入栈顶。

**参数**：
- `stack`：栈指针
- `item`：要压入的元素指针

**返回值**：
- 成功：true
- 失败：false（如果stack或item为NULL，或内存分配失败）

**注意**：如果栈已满，会自动扩容

#### `F_stack_pop`
```c
bool F_stack_pop(Stru_Stack_t* stack, void* out_item);
```
弹栈。从栈顶弹出元素。

**参数**：
- `stack`：栈指针
- `out_item`：输出参数，用于接收弹出的元素（可为NULL）

**返回值**：
- 成功：true
- 失败：false（如果stack为NULL或栈为空）

#### `F_stack_peek`
```c
void* F_stack_peek(Stru_Stack_t* stack);
```
查看栈顶元素。查看栈顶元素但不弹出。

**参数**：
- `stack`：栈指针

**返回值**：
- 成功：指向栈顶元素的指针
- 失败：NULL（如果stack为NULL或栈为空）

### 查询操作

#### `F_stack_size`
```c
size_t F_stack_size(Stru_Stack_t* stack);
```
获取栈大小。返回栈中元素的数量。

**参数**：
- `stack`：栈指针

**返回值**：
- 栈大小（如果stack为NULL返回0）

#### `F_stack_is_empty`
```c
bool F_stack_is_empty(Stru_Stack_t* stack);
```
检查栈是否为空。检查栈是否不包含任何元素。

**参数**：
- `stack`：栈指针

**返回值**：
- 栈为空：true
- 栈不为空：false（如果stack为NULL返回true）

#### `F_stack_is_full`
```c
bool F_stack_is_full(Stru_Stack_t* stack);
```
检查栈是否已满。检查栈是否已达到当前容量。

**参数**：
- `stack`：栈指针

**返回值**：
- 栈已满：true
- 栈未满：false（如果stack为NULL返回false）

**注意**：栈满时压栈会自动扩容，所以此函数主要用于监控

#### `F_stack_capacity`
```c
size_t F_stack_capacity(Stru_Stack_t* stack);
```
获取栈容量。返回栈当前分配的容量。

**参数**：
- `stack`：栈指针

**返回值**：
- 栈容量（如果stack为NULL返回0）

### 栈管理

#### `F_stack_clear`
```c
void F_stack_clear(Stru_Stack_t* stack);
```
清空栈。移除栈中的所有元素，但不释放栈本身。

**参数**：
- `stack`：栈指针

**注意**：清空后栈大小变为0，但容量保持不变

#### `F_stack_reserve`
```c
bool F_stack_reserve(Stru_Stack_t* stack, size_t new_capacity);
```
调整栈容量。调整栈的容量。

**参数**：
- `stack`：栈指针
- `new_capacity`：新的容量

**返回值**：
- 成功：true
- 失败：false（如果stack为NULL，或new_capacity小于当前大小）

**注意**：如果new_capacity小于当前大小，函数返回false

## 使用示例

```c
#include "CN_stack_core.h"
#include <stdio.h>

int main() {
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
    
    // 调整容量
    if (F_stack_reserve(stack, 100)) {
        printf("容量调整成功，新容量: %zu\n", F_stack_capacity(stack));
    }
    
    // 弹栈操作
    printf("弹栈顺序（后进先出）:\n");
    while (!F_stack_is_empty(stack)) {
        int popped;
        if (F_stack_pop(stack, &popped)) {
            printf("弹出: %d\n", popped);
        }
    }
    
    // 清空栈
    F_stack_clear(stack);
    printf("清空后栈大小: %zu\n", F_stack_size(stack));
    
    // 清理
    F_destroy_stack(stack);
    
    return 0;
}
```

## 内部实现

### 动态数组实现

栈使用动态数组实现，具有以下特点：
1. **连续内存**：元素存储在连续的内存块中，缓存友好
2. **自动扩容**：当栈满时自动扩容，扩容因子为2
3. **高效访问**：通过索引直接访问元素，时间复杂度O(1)

### 内存管理

栈核心模块负责管理以下内存：
1. **栈结构体**：存储栈的元数据（容量、大小、元素大小等）
2. **元素指针数组**：存储指向每个元素的指针
3. **元素数据**：每个元素的实际数据

### 错误处理

所有函数都包含完整的错误检查：
1. **参数验证**：检查NULL指针和无效参数
2. **内存分配检查**：检查malloc/realloc是否成功
3. **边界检查**：检查栈空、栈满等边界条件

## 性能特征

- **时间复杂度**：
  - 压栈：平均O(1)，最坏O(n)（需要扩容）
  - 弹栈：O(1)
  - 查看栈顶：O(1)
  - 获取大小：O(1)
  - 清空栈：O(n)

- **空间复杂度**：O(n)，其中n是栈容量

## 配置参数

可以通过修改以下常量来调整栈的行为：

```c
// 默认初始容量
#define CN_STACK_INITIAL_CAPACITY 16

// 扩容因子
#define CN_STACK_GROWTH_FACTOR 2
```

## 依赖关系

- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`
- 无其他项目模块依赖

## 测试

核心模块包含完整的单元测试，覆盖所有API接口和边界条件。测试文件位于`tests/infrastructure/containers/stack/test_stack_core.c`。

测试覆盖率目标：
- 语句覆盖率：≥95%
- 分支覆盖率：≥85%
- 函数覆盖率：100%

## 相关文档

- [栈模块主文档](../README.md)
- [栈迭代器模块文档](../stack_iterator/README.md)
- [栈工具模块文档](../stack_utils/README.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本栈功能 |
| 2.0.0 | 2026-01-07 | 模块化重构，作为核心模块 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
