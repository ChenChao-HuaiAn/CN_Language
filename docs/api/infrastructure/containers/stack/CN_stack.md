# 栈模块 API 文档

## 概述

栈模块提供了完整、模块化的栈数据结构实现。栈是一种后进先出（LIFO）的数据结构，支持在栈顶进行压栈和弹栈操作。

## 模块结构

栈模块分为三个子模块：

1. **核心模块** (`stack_core/`) - 基本栈操作
2. **迭代器模块** (`stack_iterator/`) - 栈遍历功能
3. **工具模块** (`stack_utils/`) - 高级工具功能

## 核心模块 API

### 数据结构

#### `Stru_Stack_t`
栈的主要结构体。

```c
typedef struct Stru_Stack_t
{
    void** items;           /**< 元素指针数组 */
    size_t capacity;        /**< 当前分配的容量 */
    size_t top;             /**< 栈顶位置（下一个空闲位置） */
    size_t item_size;       /**< 每个元素的大小（字节） */
} Stru_Stack_t;
```

### 创建和销毁

#### `F_create_stack`
```c
Stru_Stack_t* F_create_stack(size_t item_size);
```
创建栈。

**参数**：
- `item_size`：每个元素的大小（字节）

**返回值**：
- 成功：指向新创建的栈的指针
- 失败：NULL

**时间复杂度**：O(1)

#### `F_destroy_stack`
```c
void F_destroy_stack(Stru_Stack_t* stack);
```
销毁栈。

**参数**：
- `stack`：要销毁的栈指针

**时间复杂度**：O(n)，其中n是栈大小

### 栈操作

#### `F_stack_push`
```c
bool F_stack_push(Stru_Stack_t* stack, void* item);
```
压栈。

**参数**：
- `stack`：栈指针
- `item`：要压入的元素指针

**返回值**：
- 成功：true
- 失败：false

**时间复杂度**：平均O(1)，最坏O(n)（需要扩容）

#### `F_stack_pop`
```c
bool F_stack_pop(Stru_Stack_t* stack, void* out_item);
```
弹栈。

**参数**：
- `stack`：栈指针
- `out_item`：输出参数，用于接收弹出的元素（可为NULL）

**返回值**：
- 成功：true
- 失败：false（栈为空）

**时间复杂度**：O(1)

#### `F_stack_peek`
```c
void* F_stack_peek(Stru_Stack_t* stack);
```
查看栈顶元素。

**参数**：
- `stack`：栈指针

**返回值**：
- 成功：指向栈顶元素的指针
- 失败：NULL（栈为空）

**时间复杂度**：O(1)

### 查询操作

#### `F_stack_size`
```c
size_t F_stack_size(Stru_Stack_t* stack);
```
获取栈大小。

**参数**：
- `stack`：栈指针

**返回值**：栈中元素的数量

**时间复杂度**：O(1)

#### `F_stack_is_empty`
```c
bool F_stack_is_empty(Stru_Stack_t* stack);
```
检查栈是否为空。

**参数**：
- `stack`：栈指针

**返回值**：
- 栈为空：true
- 栈不为空：false

**时间复杂度**：O(1)

#### `F_stack_is_full`
```c
bool F_stack_is_full(Stru_Stack_t* stack);
```
检查栈是否已满。

**参数**：
- `stack`：栈指针

**返回值**：
- 栈已满：true
- 栈未满：false

**时间复杂度**：O(1)

#### `F_stack_capacity`
```c
size_t F_stack_capacity(Stru_Stack_t* stack);
```
获取栈容量。

**参数**：
- `stack`：栈指针

**返回值**：栈当前分配的容量

**时间复杂度**：O(1)

### 栈管理

#### `F_stack_clear`
```c
void F_stack_clear(Stru_Stack_t* stack);
```
清空栈。

**参数**：
- `stack`：栈指针

**时间复杂度**：O(n)，其中n是栈大小

#### `F_stack_reserve`
```c
bool F_stack_reserve(Stru_Stack_t* stack, size_t new_capacity);
```
调整栈容量。

**参数**：
- `stack`：栈指针
- `new_capacity`：新的容量

**返回值**：
- 成功：true
- 失败：false

**时间复杂度**：O(n)，其中n是栈大小

## 迭代器模块 API

### 数据结构

#### `Eum_StackTraversalDirection`
栈遍历方向枚举。

```c
typedef enum Eum_StackTraversalDirection
{
    Eum_STACK_TRAVERSAL_TOP_TO_BOTTOM = 0,  /**< 从栈顶到栈底（LIFO顺序） */
    Eum_STACK_TRAVERSAL_BOTTOM_TO_TOP = 1   /**< 从栈底到栈顶（FIFO顺序） */
} Eum_StackTraversalDirection;
```

#### `Stru_StackIterator_t`
栈迭代器结构体。

```c
typedef struct Stru_StackIterator_t
{
    Stru_Stack_t* stack;                    /**< 要遍历的栈 */
    size_t current_index;                   /**< 当前遍历位置 */
    size_t visited_count;                   /**< 已访问元素数量 */
    Eum_StackTraversalDirection direction;  /**< 遍历方向 */
} Stru_StackIterator_t;
```

### 迭代器管理

#### `F_create_stack_iterator`
```c
Stru_StackIterator_t* F_create_stack_iterator(Stru_Stack_t* stack);
```
创建栈迭代器。

**参数**：
- `stack`：要遍历的栈

**返回值**：
- 成功：指向新创建的迭代器的指针
- 失败：NULL

#### `F_create_stack_iterator_with_direction`
```c
Stru_StackIterator_t* F_create_stack_iterator_with_direction(
    Stru_Stack_t* stack, 
    Eum_StackTraversalDirection direction);
```
创建带方向的栈迭代器。

**参数**：
- `stack`：要遍历的栈
- `direction`：遍历方向

**返回值**：
- 成功：指向新创建的迭代器的指针
- 失败：NULL

#### `F_destroy_stack_iterator`
```c
void F_destroy_stack_iterator(Stru_StackIterator_t* iterator);
```
销毁迭代器。

**参数**：
- `iterator`：要销毁的迭代器指针

### 遍历操作

#### `F_stack_iterator_has_next`
```c
bool F_stack_iterator_has_next(Stru_StackIterator_t* iterator);
```
检查是否有下一个元素。

**参数**：
- `iterator`：迭代器指针

**返回值**：
- 有下一个元素：true
- 没有下一个元素：false

#### `F_stack_iterator_next`
```c
bool F_stack_iterator_next(Stru_StackIterator_t* iterator, void* out_item);
```
获取下一个元素。

**参数**：
- `iterator`：迭代器指针
- `out_item`：输出参数，用于接收元素（可为NULL）

**返回值**：
- 成功：true
- 失败：false

#### `F_stack_iterator_current`
```c
void* F_stack_iterator_current(Stru_StackIterator_t* iterator);
```
获取当前元素。

**参数**：
- `iterator`：迭代器指针

**返回值**：
- 成功：指向当前元素的指针
- 失败：NULL

#### `F_stack_iterator_reset`
```c
void F_stack_iterator_reset(Stru_StackIterator_t* iterator);
```
重置迭代器。

**参数**：
- `iterator`：迭代器指针

#### `F_stack_iterator_set_direction`
```c
void F_stack_iterator_set_direction(
    Stru_StackIterator_t* iterator, 
    Eum_StackTraversalDirection direction);
```
设置遍历方向。

**参数**：
- `iterator`：迭代器指针
- `direction`：新的遍历方向

### 状态查询

#### `F_stack_iterator_visited_count`
```c
size_t F_stack_iterator_visited_count(Stru_StackIterator_t* iterator);
```
获取已访问元素数量。

**参数**：
- `iterator`：迭代器指针

**返回值**：已访问元素数量

#### `F_stack_iterator_remaining_count`
```c
size_t F_stack_iterator_remaining_count(Stru_StackIterator_t* iterator);
```
获取剩余元素数量。

**参数**：
- `iterator`：迭代器指针

**返回值**：剩余元素数量

#### `F_stack_iterator_get_direction`
```c
Eum_StackTraversalDirection F_stack_iterator_get_direction(
    Stru_StackIterator_t* iterator);
```
获取遍历方向。

**参数**：
- `iterator`：迭代器指针

**返回值**：遍历方向

## 工具模块 API

### 栈操作

#### `F_stack_copy`
```c
Stru_Stack_t* F_stack_copy(Stru_Stack_t* src_stack);
```
复制栈。

**参数**：
- `src_stack`：源栈指针

**返回值**：
- 成功：指向新创建的栈副本的指针
- 失败：NULL

**时间复杂度**：O(n)，其中n是栈大小

#### `F_stack_equals`
```c
bool F_stack_equals(
    Stru_Stack_t* stack1, 
    Stru_Stack_t* stack2, 
    int (*compare_func)(const void*, const void*));
```
比较栈。

**参数**：
- `stack1`：第一个栈指针
- `stack2`：第二个栈指针
- `compare_func`：比较函数，用于比较元素

**返回值**：
- 栈相等：true
- 栈不相等：false

**时间复杂度**：O(n)，其中n是栈大小

#### `F_stack_push_batch`
```c
size_t F_stack_push_batch(Stru_Stack_t* stack, void** items, size_t count);
```
批量压栈。

**参数**：
- `stack`：栈指针
- `items`：元素指针数组
- `count`：元素数量

**返回值**：成功压入的元素数量

**时间复杂度**：O(n)，其中n是元素数量

#### `F_stack_pop_batch`
```c
size_t F_stack_pop_batch(Stru_Stack_t* stack, void** out_items, size_t max_count);
```
批量弹栈。

**参数**：
- `stack`：栈指针
- `out_items`：输出参数，用于接收弹出的元素数组（可为NULL）
- `max_count`：最大弹出数量

**返回值**：实际弹出的元素数量

**时间复杂度**：O(n)，其中n是弹出数量

### 转换操作

#### `F_stack_to_array`
```c
size_t F_stack_to_array(Stru_Stack_t* stack, void** out_array);
```
将栈转换为数组。

**参数**：
- `stack`：栈指针
- `out_array`：输出参数，用于接收数组（必须预先分配足够内存）

**返回值**：成功复制的元素数量

**时间复杂度**：O(n)，其中n是栈大小

#### `F_stack_from_array`
```c
Stru_Stack_t* F_stack_from_array(void** items, size_t count, size_t item_size);
```
从数组创建栈。

**参数**：
- `items`：元素指针数组
- `count`：元素数量
- `item_size`：每个元素的大小（字节）

**返回值**：
- 成功：指向新创建的栈的指针
- 失败：NULL

**时间复杂度**：O(n)，其中n是元素数量

### 查找操作

#### `F_stack_find`
```c
int F_stack_find(
    Stru_Stack_t* stack, 
    void* item, 
    int (*compare_func)(const void*, const void*));
```
查找元素。

**参数**：
- `stack`：栈指针
- `item`：要查找的元素指针
- `compare_func`：比较函数，用于比较元素

**返回值**：
- 成功：元素在栈中的位置（从栈底开始计数）
- 失败：-1（未找到）

**时间复杂度**：O(n)，其中n是栈大小

#### `F_stack_contains`
```c
bool F_stack_contains(
    Stru_Stack_t* stack, 
    void* item, 
    int (*compare_func)(const void*, const void*));
```
检查是否包含元素。

**参数**：
- `stack`：栈指针
- `item`：要检查的元素指针
- `compare_func`：比较函数，用于比较元素

**返回值**：
- 包含元素：true
- 不包含元素：false

**时间复杂度**：O(n)，其中n是栈大小

#### `F_stack_find_if`
```c
void* F_stack_find_if(
    Stru_Stack_t* stack, 
    bool (*predicate)(const void*, void*), 
    void* context);
```
查找满足条件的元素。

**参数**：
- `stack`：栈指针
- `predicate`：谓词函数，返回true表示元素满足条件
- `context`：上下文指针，传递给谓词函数

**返回值**：
- 成功：指向满足条件的元素的指针
- 失败：NULL（未找到）

**时间复杂度**：O(n)，其中n是栈大小

### 栈操作工具

#### `F_stack_reverse`
```c
bool F_stack_reverse(Stru_Stack_t* stack);
```
反转栈。

**参数**：
- `stack`：栈指针

**返回值**：
- 成功：true
- 失败：false

**时间复杂度**：O(n)，其中n是栈大小

#### `F_stack_min`
```c
bool F_stack_min(
    Stru_Stack_t* stack, 
    int (*compare_func)(const void*, const void*), 
    void* out_item);
```
获取栈中最小元素。

**参数**：
- `stack`：栈指针
- `compare_func`：比较函数，用于比较元素
- `out_item`：输出参数，用于接收最小元素

**返回值**：
- 成功：true
- 失败：false（栈为空）

**时间复杂度**：O(n)，其中n是栈大小

#### `F_stack_max`
```c
bool F_stack_max(
    Stru_Stack_t* stack, 
    int (*compare_func)(const void*, const void*), 
    void* out_item);
```
获取栈中最大元素。

**参数**：
- `stack`：栈指针
- `compare_func`：比较函数，用于比较元素
- `out_item`：输出参数，用于接收最大元素

**返回值**：
- 成功：true
- 失败：false（栈为空）

**时间复杂度**：O(n)，其中n是栈大小

## 模块管理 API

#### `F_stack_get_version`
```c
void F_stack_get_version(int* major, int* minor, int* patch);
```
获取栈模块版本。

**参数**：
- `major`：输出参数，主版本号
- `minor`：输出参数，次版本号
- `patch`：输出参数，修订号

#### `F_stack_get_version_string`
```c
const char* F_stack_get_version_string(void);
```
获取栈模块版本字符串。

**返回值**：版本字符串

#### `F_stack_module_init`
```c
bool F_stack_module_init(void);
```
初始化栈模块。

**返回值**：
- 成功：true
- 失败：false

#### `F_stack_module_cleanup`
```c
void F_stack_module_cleanup(void);
```
清理栈模块。

## 错误处理

所有函数都包含错误检查：
1. **参数验证**：检查NULL指针和无效参数
2. **内存分配检查**：检查malloc/realloc是否成功
3. **边界检查**：检查栈空、栈满等边界条件

错误通过返回值表示：
- 返回指针的函数：失败返回NULL
- 返回布尔值的函数：失败返回false
- 返回大小的函数：失败返回0或适当的值

## 线程安全

当前实现不是线程安全的。如果需要在多线程环境中使用，需要外部同步机制。

## 内存管理

栈模块负责管理以下内存：
1. 栈结构体本身
2. 元素指针数组
3. 每个元素的实际数据

用户只需要提供要存储的数据，模块会自动处理内存分配和释放。

## 性能特征

### 时间复杂度
- 压栈：平均O(1)，最坏O(n)（需要扩容）
- 弹栈：O(1)
- 查看栈顶：O(1)
- 获取大小：O(1)
- 遍历：O(n)
- 批量操作：O(n)
- 查找操作：O(n)

### 空间复杂度
- O(n)，其中n是栈容量

## 使用示例

参考栈模块README.md中的使用示例。

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本栈功能 |
| 2.0.0 | 2026-01-07 | 模块化重构，增加迭代器和工具模块 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
