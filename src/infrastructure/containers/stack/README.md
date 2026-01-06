# 栈模块 (Stack Module)

## 概述

栈模块提供了栈数据结构的实现。栈是一种后进先出（LIFO）的数据结构，支持在栈顶进行压栈和弹栈操作。

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
    void** items;           // 元素指针数组
    size_t capacity;        // 当前分配的容量
    size_t top;             // 栈顶位置（下一个空闲位置）
    size_t item_size;       // 每个元素的大小（字节）
} Stru_Stack_t;
```

## API 接口

### 创建和销毁

- `F_create_stack(size_t item_size)` - 创建栈
- `F_destroy_stack(Stru_Stack_t* stack)` - 销毁栈

### 栈操作

- `F_stack_push(Stru_Stack_t* stack, void* item)` - 压栈
- `F_stack_pop(Stru_Stack_t* stack, void* out_item)` - 弹栈
- `F_stack_peek(Stru_Stack_t* stack)` - 查看栈顶元素

### 查询操作

- `F_stack_size(Stru_Stack_t* stack)` - 获取栈大小
- `F_stack_is_empty(Stru_Stack_t* stack)` - 检查栈是否为空

### 栈管理

- `F_stack_clear(Stru_Stack_t* stack)` - 清空栈

## 使用示例

```c
#include "CN_stack.h"
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
    printf("栈是否为空: %s\n", F_stack_is_empty(stack) ? "是" : "否");
    
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
    
    return 0;
}
```

## 典型应用场景

### 1. 函数调用栈
```c
// 模拟函数调用
void function_a() {
    printf("进入函数A\n");
    // ... 函数A的逻辑 ...
}

void function_b() {
    printf("进入函数B\n");
    // ... 函数B的逻辑 ...
}

// 使用栈管理函数调用
```

### 2. 表达式求值
```c
// 后缀表达式求值示例
// 表达式: "3 4 + 5 *" 对应 (3 + 4) * 5
// 使用栈存储操作数
```

### 3. 括号匹配检查
```c
// 检查括号是否匹配
// 输入: "((a + b) * (c - d))"
// 使用栈存储左括号，遇到右括号时弹栈检查
```

### 4. 撤销操作
```c
// 文本编辑器中的撤销功能
// 每次编辑操作压栈，撤销时弹栈恢复之前状态
```

## 性能特征

- **时间复杂度**：
  - 压栈：平均O(1)，最坏O(n)（需要扩容）
  - 弹栈：O(1)
  - 查看栈顶：O(1)
  - 获取大小：O(1)

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

模块包含完整的单元测试，覆盖所有API接口和边界条件。测试覆盖率目标：
- 语句覆盖率：≥90%
- 分支覆盖率：≥80%
- 函数覆盖率：100%

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本栈功能 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
