# Stack Utils 模块

## 概述

Stack Utils 模块提供了一系列高级工具函数，用于扩展栈数据结构的实用功能。这些函数建立在栈核心模块的基础上，提供了批量操作、数据转换、查找比较等高级功能，使栈的使用更加方便和高效。

## 功能特性

- **批量操作**：支持批量压入和弹出多个元素
- **栈复制**：创建栈的深拷贝和浅拷贝
- **栈比较**：比较两个栈的内容是否相等
- **元素查找**：在栈中查找特定元素
- **栈反转**：反转栈中元素的顺序
- **栈统计**：统计栈中满足条件的元素数量
- **栈转换**：将栈转换为数组或其他数据结构

## API 接口

### 批量操作函数
- `bool F_stack_push_batch(Stru_Stack_t* stack, const void* items, size_t count)`
  - 批量压入多个元素
  - 参数：stack - 栈对象，items - 元素数组指针，count - 元素数量
  - 返回：成功返回true，失败返回false

- `bool F_stack_pop_batch(Stru_Stack_t* stack, void* output, size_t count)`
  - 批量弹出多个元素
  - 参数：stack - 栈对象，output - 输出缓冲区，count - 要弹出的元素数量
  - 返回：成功返回true，失败返回false

### 栈复制函数
- `Stru_Stack_t* F_stack_copy_shallow(Stru_Stack_t* stack)`
  - 创建栈的浅拷贝（共享数据）
  - 参数：stack - 源栈对象
  - 返回：新栈指针，失败返回NULL

- `Stru_Stack_t* F_stack_copy_deep(Stru_Stack_t* stack)`
  - 创建栈的深拷贝（复制数据）
  - 参数：stack - 源栈对象
  - 返回：新栈指针，失败返回NULL

### 栈比较函数
- `bool F_stack_equals(Stru_Stack_t* stack1, Stru_Stack_t* stack2, bool (*compare_func)(const void*, const void*))`
  - 比较两个栈是否相等
  - 参数：stack1, stack2 - 要比较的栈，compare_func - 元素比较函数
  - 返回：相等返回true，否则返回false

### 元素查找函数
- `void* F_stack_find(Stru_Stack_t* stack, const void* target, bool (*compare_func)(const void*, const void*))`
  - 在栈中查找元素
  - 参数：stack - 栈对象，target - 要查找的目标，compare_func - 比较函数
  - 返回：找到的元素指针，未找到返回NULL

- `size_t F_stack_find_all(Stru_Stack_t* stack, const void* target, bool (*compare_func)(const void*, const void*), size_t* positions, size_t max_positions)`
  - 查找所有匹配的元素
  - 参数：stack - 栈对象，target - 目标，compare_func - 比较函数，positions - 位置数组，max_positions - 最大位置数
  - 返回：找到的元素数量

### 栈操作函数
- `bool F_stack_reverse(Stru_Stack_t* stack)`
  - 反转栈中元素的顺序
  - 参数：stack - 栈对象
  - 返回：成功返回true，失败返回false

- `size_t F_stack_count_if(Stru_Stack_t* stack, bool (*predicate)(const void*))`
  - 统计满足条件的元素数量
  - 参数：stack - 栈对象，predicate - 条件判断函数
  - 返回：满足条件的元素数量

### 栈转换函数
- `void* F_stack_to_array(Stru_Stack_t* stack, size_t* array_size)`
  - 将栈转换为数组
  - 参数：stack - 栈对象，array_size - 输出数组大小
  - 返回：新分配的数组指针，失败返回NULL

- `bool F_stack_from_array(Stru_Stack_t* stack, const void* array, size_t array_size)`
  - 从数组创建栈
  - 参数：stack - 栈对象，array - 源数组，array_size - 数组大小
  - 返回：成功返回true，失败返回false

## 使用示例

```c
#include "CN_stack_utils.h"

// 创建栈
Stru_Stack_t* stack1 = F_stack_create(sizeof(int), 10);

// 批量压入数据
int data[] = {1, 2, 3, 4, 5};
F_stack_push_batch(stack1, data, 5);

// 创建深拷贝
Stru_Stack_t* stack2 = F_stack_copy_deep(stack1);

// 比较两个栈
bool equal = F_stack_equals(stack1, stack2, NULL);
printf("栈相等: %s\n", equal ? "是" : "否");

// 查找元素
int target = 3;
int* found = (int*)F_stack_find(stack1, &target, NULL);
if (found) {
    printf("找到元素: %d\n", *found);
}

// 反转栈
F_stack_reverse(stack1);

// 转换为数组
size_t array_size;
int* array = (int*)F_stack_to_array(stack1, &array_size);
if (array) {
    printf("栈转换为数组，大小: %zu\n", array_size);
    free(array);
}

// 清理
F_stack_destroy(stack1);
F_stack_destroy(stack2);
```

## 性能特点

- **时间复杂度**：
  - 批量操作：O(n)，n为元素数量
  - 栈复制：深拷贝O(n)，浅拷贝O(1)
  - 栈比较：O(n)
  - 元素查找：O(n)
  - 栈反转：O(n)
  - 栈转换：O(n)

- **空间复杂度**：
  - 批量操作：O(1) 额外空间（原地操作）
  - 深拷贝：O(n) 额外空间
  - 浅拷贝：O(1) 额外空间
  - 其他操作：O(1) 到 O(n) 不等

## 设计原则

1. **单一职责原则**：工具模块只提供辅助功能，不涉及栈的核心操作
2. **开闭原则**：通过函数指针参数支持自定义比较和条件判断
3. **接口隔离原则**：每个工具函数专注于单一功能
4. **依赖倒置原则**：工具函数依赖于栈的抽象接口

## 注意事项

1. 批量操作函数需要确保有足够的栈容量
2. 深拷贝会复制所有数据，调用者需要负责释放内存
3. 查找和比较函数可能需要自定义比较函数
4. 栈转换函数分配的内存需要调用者释放
5. 工具函数不检查栈对象的有效性，调用者需要确保参数有效

## 测试覆盖

- 批量操作测试：验证批量压入和弹出的正确性
- 复制功能测试：验证浅拷贝和深拷贝的行为
- 比较功能测试：验证栈相等性比较
- 查找功能测试：验证元素查找功能
- 反转功能测试：验证栈反转操作
- 转换功能测试：验证栈与数组的相互转换
- 边界条件测试：空栈、单元素栈、满栈的情况
- 错误处理测试：无效参数、内存不足等异常情况

## 相关模块

- [Stack Core](../stack_core/README.md)：栈核心功能模块
- [Stack Iterator](../stack_iterator/README.md)：栈迭代器模块
- [Main Stack](../README.md)：栈模块主文档
