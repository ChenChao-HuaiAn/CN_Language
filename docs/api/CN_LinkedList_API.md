# CN_linked_list 模块 API 文档

## 概述

本文档详细描述了CN_linked_list模块的所有公共接口。CN_linked_list模块提供了类型安全的链表容器，支持单向链表和双向链表两种变体，具有完整的操作API和迭代器支持。

## 架构位置

CN_linked_list模块位于基础设施层（Infrastructure Layer），为上层模块提供基础数据管理功能。遵循项目分层架构设计原则，只依赖C标准库和操作系统API。

## 数据结构

### 枚举类型

#### `enum Eum_CN_LinkedListType_t`
链表类型枚举，定义了CN_linked_list支持的链表类型。

```c
typedef enum Eum_CN_LinkedListType_t
{
    Eum_LINKED_LIST_SINGLY = 0,    /**< 单向链表 */
    Eum_LINKED_LIST_DOUBLY = 1     /**< 双向链表 */
} Eum_CN_LinkedListType_t;
```

### 结构体类型

#### `struct Stru_CN_LinkedList_t`
链表结构体（不透明类型）。外部代码只能通过指针访问，内部实现细节被隐藏。

```c
typedef struct Stru_CN_LinkedList_t Stru_CN_LinkedList_t;
```

#### `struct Stru_CN_ListNode_t`
链表节点结构体（不透明类型）。外部代码不能直接访问节点内部。

```c
typedef struct Stru_CN_ListNode_t Stru_CN_ListNode_t;
```

#### `struct Stru_CN_LinkedListIterator_t`
链表迭代器结构体，用于安全遍历链表元素。

```c
typedef struct Stru_CN_LinkedListIterator_t
{
    Stru_CN_LinkedList_t* list;        /**< 关联的链表 */
    Stru_CN_ListNode_t* current_node;  /**< 当前节点 */
    size_t current_index;              /**< 当前索引 */
} Stru_CN_LinkedListIterator_t;
```

### 函数指针类型

#### `CN_LinkedListCompareFunc`
链表比较函数类型，用于自定义类型的元素比较。

```c
typedef int (*CN_LinkedListCompareFunc)(const void* elem1, const void* elem2);
```

**参数**：
- `elem1`: 第一个元素
- `elem2`: 第二个元素

**返回值**：
- `<0`: elem1 < elem2
- `0`: elem1 == elem2
- `>0`: elem1 > elem2

#### `CN_LinkedListFreeFunc`
链表元素释放函数类型，用于自定义类型的资源释放。

```c
typedef void (*CN_LinkedListFreeFunc)(void* element);
```

**参数**：
- `element`: 要释放的元素

#### `CN_LinkedListCopyFunc`
链表元素复制函数类型，用于自定义类型的深度复制。

```c
typedef void* (*CN_LinkedListCopyFunc)(const void* src);
```

**参数**：
- `src`: 源元素

**返回值**：新分配的元素副本

## 函数接口

### 链表创建和销毁

#### `CN_linked_list_create`
创建链表。

```c
Stru_CN_LinkedList_t* CN_linked_list_create(Eum_CN_LinkedListType_t list_type,
                                            size_t element_size);
```

**参数**：
- `list_type`: 链表类型（单向或双向）
- `element_size`: 元素大小（字节）

**返回值**：新创建的链表指针，失败返回NULL

**注意**：
- 对于简单数据类型，使用此函数创建链表
- 调用者负责使用`CN_linked_list_destroy`释放返回的链表

#### `CN_linked_list_create_custom`
创建带自定义函数的链表，支持复杂自定义类型。

```c
Stru_CN_LinkedList_t* CN_linked_list_create_custom(
    Eum_CN_LinkedListType_t list_type,
    size_t element_size,
    CN_LinkedListCompareFunc compare_func,
    CN_LinkedListFreeFunc free_func,
    CN_LinkedListCopyFunc copy_func);
```

**参数**：
- `list_type`: 链表类型（单向或双向）
- `element_size`: 元素大小（字节）
- `compare_func`: 比较函数（可为NULL）
- `free_func`: 释放函数（可为NULL）
- `copy_func`: 复制函数（可为NULL）

**返回值**：新创建的链表指针，失败返回NULL

**注意**：
- 如果提供`free_func`，链表销毁时会自动调用它释放每个元素
- 如果提供`copy_func`，链表复制时会使用它进行深度复制
- 如果提供`compare_func`，搜索时会使用它

#### `CN_linked_list_destroy`
销毁链表，释放所有分配的内存。

```c
void CN_linked_list_destroy(Stru_CN_LinkedList_t* list);
```

**参数**：
- `list`: 要销毁的链表指针

**注意**：
- 如果链表创建时指定了`free_func`，会调用它释放每个元素
- 此函数会释放链表内部分配的所有内存
- 可以安全地传递NULL参数

#### `CN_linked_list_clear`
清空链表（移除所有元素），但不释放链表本身。

```c
void CN_linked_list_clear(Stru_CN_LinkedList_t* list);
```

**参数**：
- `list`: 要清空的链表

**注意**：
- 如果设置了`free_func`，会调用它释放每个元素
- 链表结构保持不变，可以重用

### 链表属性查询

#### `CN_linked_list_length`
获取链表长度（元素数量）。

```c
size_t CN_linked_list_length(const Stru_CN_LinkedList_t* list);
```

**参数**：
- `list`: 链表

**返回值**：链表长度，如果list为NULL返回0

#### `CN_linked_list_is_empty`
检查链表是否为空。

```c
bool CN_linked_list_is_empty(const Stru_CN_LinkedList_t* list);
```

**参数**：
- `list`: 链表

**返回值**：如果链表为空或为NULL返回true，否则返回false

#### `CN_linked_list_type`
获取链表类型。

```c
Eum_CN_LinkedListType_t CN_linked_list_type(const Stru_CN_LinkedList_t* list);
```

**参数**：
- `list`: 链表

**返回值**：链表类型，如果list为NULL返回Eum_LINKED_LIST_SINGLY

#### `CN_linked_list_element_size`
获取元素大小（字节）。

```c
size_t CN_linked_list_element_size(const Stru_CN_LinkedList_t* list);
```

**参数**：
- `list`: 链表

**返回值**：元素大小，如果list为NULL返回0

### 元素访问和修改

#### `CN_linked_list_first`
获取第一个元素。

```c
void* CN_linked_list_first(const Stru_CN_LinkedList_t* list);
```

**参数**：
- `list`: 链表

**返回值**：第一个元素的指针，链表为空返回NULL

**注意**：
- 返回的指针指向链表内部存储，不要释放它
- 通过返回的指针修改元素是安全的

#### `CN_linked_list_last`
获取最后一个元素。

```c
void* CN_linked_list_last(const Stru_CN_LinkedList_t* list);
```

**参数**：
- `list`: 链表

**返回值**：最后一个元素的指针，链表为空返回NULL

#### `CN_linked_list_get`
获取指定位置的元素。

```c
void* CN_linked_list_get(const Stru_CN_LinkedList_t* list, size_t index);
```

**参数**：
- `list`: 链表
- `index`: 元素索引（0-based）

**返回值**：元素的指针，索引无效或list为NULL返回NULL

**注意**：
- 索引从0开始
- 对于单向链表，访问中间元素需要O(n)时间

#### `CN_linked_list_set`
设置指定位置的元素值。

```c
bool CN_linked_list_set(Stru_CN_LinkedList_t* list, size_t index, 
                        const void* element);
```

**参数**：
- `list`: 链表
- `index`: 元素索引
- `element`: 元素值指针

**返回值**：设置成功返回true，失败返回false

**注意**：
- 如果设置了`free_func`，会先释放旧元素
- 新元素值会被复制到链表中

#### `CN_linked_list_prepend`
在链表头部添加元素。

```c
bool CN_linked_list_prepend(Stru_CN_LinkedList_t* list, const void* element);
```

**参数**：
- `list`: 链表
- `element`: 要添加的元素

**返回值**：添加成功返回true，失败返回false

**注意**：
- 时间复杂度：O(1)
- 元素值会被复制到链表中

#### `CN_linked_list_append`
在链表尾部添加元素。

```c
bool CN_linked_list_append(Stru_CN_LinkedList_t* list, const void* element);
```

**参数**：
- `list`: 链表
- `element`: 要添加的元素

**返回值**：添加成功返回true，失败返回false

**注意**：
- 对于单向链表，时间复杂度：O(n)（需要遍历到尾部）
- 对于双向链表，时间复杂度：O(1)（有尾指针）
- 元素值会被复制到链表中

#### `CN_linked_list_insert`
在指定位置插入元素。

```c
bool CN_linked_list_insert(Stru_CN_LinkedList_t* list, size_t index, 
                           const void* element);
```

**参数**：
- `list`: 链表
- `index`: 插入位置（0-based）
- `element`: 要插入的元素

**返回值**：插入成功返回true，失败返回false

**注意**：
- 如果index等于0，效果与`CN_linked_list_prepend`相同
- 如果index等于链表长度，效果与`CN_linked_list_append`相同
- 时间复杂度：O(n)（需要遍历到插入位置）

#### `CN_linked_list_remove_first`
移除链表头部元素。

```c
bool CN_linked_list_remove_first(Stru_CN_LinkedList_t* list, void* element);
```

**参数**：
- `list`: 链表
- `element`: 输出参数，接收移除的元素值（可为NULL）

**返回值**：移除成功返回true，失败返回false

**注意**：
- 如果element不为NULL，元素值会被复制到提供的缓冲区
- 如果设置了`free_func`，会调用它释放被移除的元素
- 时间复杂度：O(1)

#### `CN_linked_list_remove_last`
移除链表尾部元素。

```c
bool CN_linked_list_remove_last(Stru_CN_LinkedList_t* list, void* element);
```

**参数**：
- `list`: 链表
- `element`: 输出参数，接收移除的元素值（可为NULL）

**返回值**：移除成功返回true，失败返回false

**注意**：
- 对于单向链表，时间复杂度：O(n)（需要遍历到倒数第二个节点）
- 对于双向链表，时间复杂度：O(1)

#### `CN_linked_list_remove`
移除指定位置的元素。

```c
bool CN_linked_list_remove(Stru_CN_LinkedList_t* list, size_t index, 
                           void* element);
```

**参数**：
- `list`: 链表
- `index`: 要移除的元素索引
- `element`: 输出参数，接收移除的元素值（可为NULL）

**返回值**：移除成功返回true，失败返回false

**注意**：
- 如果设置了`free_func`，会调用它释放被移除的元素
- 时间复杂度：O(n)（需要遍历到指定位置）

### 链表操作

#### `CN_linked_list_copy`
复制链表，创建新的链表副本。

```c
Stru_CN_LinkedList_t* CN_linked_list_copy(const Stru_CN_LinkedList_t* src);
```

**参数**：
- `src`: 源链表

**返回值**：新创建的链表副本，失败返回NULL

**注意**：
- 如果源链表设置了`copy_func`，会使用它进行深度复制
- 否则进行浅复制（内存拷贝）
- 调用者负责销毁返回的副本

#### `CN_linked_list_concat`
连接两个链表，创建新的链表。

```c
Stru_CN_LinkedList_t* CN_linked_list_concat(const Stru_CN_LinkedList_t* list1,
                                            const Stru_CN_LinkedList_t* list2);
```

**参数**：
- `list1`: 第一个链表
- `list2`: 第二个链表

**返回值**：新创建的连接后的链表，失败返回NULL

**注意**：
- 两个链表的元素类型和大小必须相同
- 自定义函数（compare_func、free_func、copy_func）也必须相同
- 调用者负责销毁返回的链表

#### `CN_linked_list_reverse`
反转链表。

```c
bool CN_linked_list_reverse(Stru_CN_LinkedList_t* list);
```

**参数**：
- `list`: 链表

**返回值**：反转成功返回true，失败返回false

**注意**：
- 反转操作在原地进行，不创建新链表
- 时间复杂度：O(n)

#### `CN_linked_list_find`
查找元素，返回第一个匹配的索引。

```c
size_t CN_linked_list_find(const Stru_CN_LinkedList_t* list, const void* element);
```

**参数**：
- `list`: 链表
- `element`: 要查找的元素

**返回值**：元素索引，未找到返回SIZE_MAX

**注意**：
- 如果链表设置了`compare_func`，使用它进行比较
- 否则使用memcmp进行内存比较
- 时间复杂度：O(n)

#### `CN_linked_list_find_custom`
使用自定义比较函数查找元素。

```c
size_t CN_linked_list_find_custom(const Stru_CN_LinkedList_t* list, 
                                  const void* element,
                                  CN_LinkedListCompareFunc compare_func);
```

**参数**：
- `list`: 链表
- `element`: 要查找的元素
- `compare_func`: 自定义比较函数

**返回值**：元素索引，未找到返回SIZE_MAX

#### `CN_linked_list_contains`
检查链表是否包含元素。

```c
bool CN_linked_list_contains(const Stru_CN_LinkedList_t* list, const void* element);
```

**参数**：
- `list`: 链表
- `element`: 要检查的元素

**返回值**：如果包含返回true，否则返回false

### 迭代器支持

#### `CN_linked_list_iterator_create`
创建链表迭代器。

```c
Stru_CN_LinkedListIterator_t* CN_linked_list_iterator_create(
    Stru_CN_LinkedList_t* list);
```

**参数**：
- `list`: 链表

**返回值**：新创建的迭代器，失败返回NULL

**注意**：
- 迭代器初始位置在第一个元素之前
- 调用者负责销毁返回的迭代器

#### `CN_linked_list_iterator_destroy`
销毁迭代器。

```c
void CN_linked_list_iterator_destroy(Stru_CN_LinkedListIterator_t* iterator);
```

**参数**：
- `iterator`: 要销毁的迭代器

#### `CN_linked_list_iterator_reset`
重置迭代器到开始位置。

```c
void CN_linked_list_iterator_reset(Stru_CN_LinkedListIterator_t* iterator);
```

**参数**：
- `iterator`: 迭代器

#### `CN_linked_list_iterator_has_next`
检查迭代器是否有下一个元素。

```c
bool CN_linked_list_iterator_has_next(const Stru_CN_LinkedListIterator_t* iterator);
```

**参数**：
- `iterator`: 迭代器

**返回值**：如果有下一个元素返回true，否则返回false

#### `CN_linked_list_iterator_next`
获取下一个元素，并移动迭代器位置。

```c
void* CN_linked_list_iterator_next(Stru_CN_LinkedListIterator_t* iterator);
```

**参数**：
- `iterator`: 迭代器

**返回值**：下一个元素的指针，没有更多元素返回NULL

#### `CN_linked_list_iterator_current`
获取当前元素，不移动迭代器位置。

```c
void* CN_linked_list_iterator_current(const Stru_CN_LinkedListIterator_t* iterator);
```

**参数**：
- `iterator`: 迭代器

**返回值**：当前元素的指针

#### `CN_linked_list_iterator_has_prev`
检查迭代器是否有前一个元素（仅双向链表）。

```c
bool CN_linked_list_iterator_has_prev(const Stru_CN_LinkedListIterator_t* iterator);
```

**参数**：
- `iterator`: 迭代器

**返回值**：如果有前一个元素返回true，否则返回false

**注意**：
- 对于单向链表，此函数总是返回false
- 对于双向链表，支持向前遍历

#### `CN_linked_list_iterator_prev`
获取前一个元素（仅双向链表）。

```c
void* CN_linked_list_iterator_prev(Stru_CN_LinkedListIterator_t* iterator);
```

**参数**：
- `iterator`: 迭代器

**返回值**：前一个元素的指针，没有更多元素返回NULL

### 工具函数

#### `CN_linked_list_equal`
比较两个链表是否相等。

```c
bool CN_linked_list_equal(const Stru_CN_LinkedList_t* list1, 
                          const Stru_CN_LinkedList_t* list2);
```

**参数**：
- `list1`: 第一个链表
- `list2`: 第二个链表

**返回值**：如果链表相等返回true，否则返回false

**注意**：
- 两个链表的元素类型、大小和长度必须相同
- 如果设置了`compare_func`，使用它进行比较
- 否则使用memcmp进行内存比较

#### `CN_linked_list_dump`
转储链表信息到控制台（调试用）。

```c
void CN_linked_list_dump(const Stru_CN_LinkedList_t* list);
```

**参数**：
- `list`: 链表

**注意**：
- 输出链表类型、长度、元素大小等信息
- 主要用于调试目的

## 错误处理

所有函数都遵循以下错误处理原则：

1. **内存分配失败**：返回NULL或false
2. **无效参数**：返回适当的错误值（如SIZE_MAX、false等）
3. **边界检查**：进行必要的参数验证
4. **资源管理**：谁创建谁销毁，或使用引用计数

### 常见错误返回值
- 指针返回函数：失败返回NULL
- 布尔返回函数：失败返回false
- 大小返回函数：失败返回SIZE_MAX
- 枚举返回函数：失败返回Eum_LINKED_LIST_SINGLY

## 线程安全性

CN_linked_list模块的函数不是线程安全的。如果需要在多线程环境中使用，调用者需要提供适当的同步机制。

## 内存管理

### 分配策略
1. **节点分配**：每个元素分配一个节点
2. **批量操作**：支持批量添加和复制
3. **内存重用**：清空链表后可以重用已分配节点

### 内存所有权
1. **链表所有权**：谁创建谁销毁（使用`CN_linked_list_destroy`）
2. **元素所有权**：
   - 简单类型：链表管理内存
   - 自定义类型：如果提供`free_func`，链表会调用它
3. **迭代器所有权**：谁创建谁销毁（使用`CN_linked_list_iterator_destroy`）

### 内存安全
- 所有内存访问都进行边界检查
- 防止空指针解引用
- 支持自定义内存释放函数

## 性能特性

### 时间复杂度
- **头部访问**：O(1)
- **尾部访问**：
  - 单向链表：O(n)
  - 双向链表：O(1)
- **随机访问**：O(n)
- **头部插入/删除**：O(1)
- **尾部插入/删除**：
  - 单向链表：O(n)
  - 双向链表：O(1)
- **中间插入/删除**：O(n)
- **搜索**：O(n)

### 空间复杂度
- **存储开销**：n * (element_size + 节点开销)
- **节点开销**：
  - 单向链表：1个指针
  - 双向链表：2个指针
- **额外开销**：少量元数据（类型、长度等）

### 优化特性
1. **头部操作优化**：头部插入/删除非常高效
2. **双向链表优化**：支持双向遍历和高效尾部操作
3. **迭代器优化**：迭代访问避免重复边界检查
4. **自定义函数**：支持复杂类型的深度复制和释放

## 使用示例

### 示例1：创建和使用整数链表
```c
#include "CN_linked_list.h"
#include <stdio.h>

void example_int_linked_list(void)
{
    // 创建双向链表，存储整数
    Stru_CN_LinkedList_t* list = CN_linked_list_create(
        Eum_LINKED_LIST_DOUBLY, sizeof(int));
    
    if (list == NULL) {
        printf("创建链表失败\n");
        return;
    }
    
    // 添加元素
    for (int i = 1; i <= 10; i++) {
        if (!CN_linked_list_append(list, &i)) {
            printf("添加元素 %d 失败\n", i);
        }
    }
    
    // 访问元素
    printf("链表长度: %zu\n", CN_linked_list_length(list));
    
    // 在头部添加元素
    int head_value = 0;
    CN_linked_list_prepend(list, &head_value);
    
    // 在中间插入元素
    int middle_value = 99;
    CN_linked_list_insert(list, 5, &middle_value);
    
    // 遍历链表
    for (size_t i = 0; i < CN_linked_list_length(list); i++) {
        int* value = (int*)CN_linked_list_get(list, i);
        if (value != NULL) {
            printf("list[%zu] = %d\n", i, *value);
        }
    }
    
    // 销毁链表
    CN_linked_list_destroy(list);
}
```

### 示例2：使用自定义类型链表
```c
#include "CN_linked_list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    int id;
    char name[50];
    float salary;
} Employee;

// 自定义比较函数（按ID比较）
int compare_employees(const void* a, const void* b)
{
    const Employee* e1 = (const Employee*)a;
    const Employee* e2 = (const Employee*)b;
    return e1->id - e2->id;
}

// 自定义复制函数
void* copy_employee(const void* src)
{
    const Employee* e = (const Employee*)src;
    Employee* copy = malloc(sizeof(Employee));
    if (copy != NULL) {
        copy->id = e->id;
        strcpy(copy->name, e->name);
        copy->salary = e->salary;
    }
    return copy;
}

// 自定义释放函数
void free_employee(void* element)
{
    free(element);
}

void example_custom_linked_list(void)
{
    // 创建带自定义函数的单向链表
    Stru_CN_LinkedList_t* employees = CN_linked_list_create_custom(
        Eum_LINKED_LIST_SINGLY,
        sizeof(Employee),
        compare_employees,
        free_employee,
        copy_employee);
    
    if (employees == NULL) {
        printf("创建自定义链表失败\n");
        return;
    }
    
    // 添加员工
    Employee e1 = {101, "张三", 8500.0};
    Employee e2 = {102, "李四", 9200.0};
    Employee e3 = {103, "王五", 7800.0};
    
    CN_linked_list_append(employees, &e1);
    CN_linked_list_append(employees, &e2);
    CN_linked_list_append(employees, &e3);
    
    printf("员工数量: %zu\n", CN_linked_list_length(employees));
    
    // 查找员工
    Employee search_key = {102, "", 0};
    size_t index = CN_linked_list_find(employees, &search_key);
    if (index != SIZE_MAX) {
        Employee* found = (Employee*)CN_linked_list_get(employees, index);
        printf("找到员工: ID=%d, 姓名=%s, 工资=%.1f\n", 
               found->id, found->name, found->salary);
    }
    
    // 使用迭代器遍历
    Stru_CN_LinkedListIterator_t* iter = CN_linked_list_iterator_create(employees);
    if (iter != NULL) {
        printf("员工列表:\n");
        while (CN_linked_list_iterator_has_next(iter)) {
            Employee* e = (Employee*)CN_linked_list_iterator_next(iter);
            printf("  ID=%d, 姓名=%s, 工资=%.1f\n", e->id, e->name, e->salary);
        }
        CN_linked_list_iterator_destroy(iter);
    }
    
    // 销毁链表
    CN_linked_list_destroy(employees);
}
```

### 示例3：链表操作
```c
#include "CN_linked_list.h"
#include <stdio.h>

void example_linked_list_operations(void)
{
    // 创建两个链表
    Stru_CN_LinkedList_t* list1 = CN_linked_list_create(
        Eum_LINKED_LIST_SINGLY, sizeof(int));
    Stru_CN_LinkedList_t* list2 = CN_linked_list_create(
        Eum_LINKED_LIST_SINGLY, sizeof(int));
    
    // 填充数据
    for (int i = 1; i <= 5; i++) {
        CN_linked_list_append(list1, &i);
    }
    for (int i = 6; i <= 10; i++) {
        CN_linked_list_append(list2, &i);
    }
    
    // 连接链表
    Stru_CN_LinkedList_t* combined = CN_linked_list_concat(list1, list2);
    if (combined != NULL) {
        printf("连接后链表长度: %zu\n", CN_linked_list_length(combined));
        
        // 反转链表
        CN_linked_list_reverse(combined);
        printf("反转后第一个元素: %d\n", 
               *(int*)CN_linked_list_first(combined));
        
        CN_linked_list_destroy(combined);
    }
    
    // 链表复制
    Stru_CN_LinkedList_t* copy = CN_linked_list_copy(list1);
    if (copy != NULL) {
        printf("链表副本长度: %zu\n", CN_linked_list_length(copy));
        
        // 比较链表
        if (CN_linked_list_equal(list1, copy)) {
            printf("链表副本与原始链表相等\n");
        }
        
        CN_linked_list_destroy(copy);
    }
    
    // 清理
    CN_linked_list_destroy(list1);
    CN_linked_list_destroy(list2);
}
```

### 示例4：双向链表和迭代器
```c
#include "CN_linked_list.h"
#include <stdio.h>

void example_doubly_linked_list(void)
{
    // 创建双向链表
    Stru_CN_LinkedList_t* list = CN_linked_list_create(
        Eum_LINKED_LIST_DOUBLY, sizeof(int));
    
    // 添加元素
    for (int i = 1; i <= 5; i++) {
        CN_linked_list_append(list, &i);
    }
    
    // 正向遍历
    Stru_CN_LinkedListIterator_t* iter = CN_linked_list_iterator_create(list);
    if (iter != NULL) {
        printf("正向遍历:\n");
        while (CN_linked_list_iterator_has_next(iter)) {
            int* value = (int*)CN_linked_list_iterator_next(iter);
            printf("  %d", *value);
        }
        printf("\n");
        
        // 反向遍历（双向链表支持）
        printf("反向遍历:\n");
        // 先移动到最后一个元素
        while (CN_linked_list_iterator_has_next(iter)) {
            CN_linked_list_iterator_next(iter);
        }
        
        // 然后向前遍历
        while (CN_linked_list_iterator_has_prev(iter)) {
            int* value = (int*)CN_linked_list_iterator_prev(iter);
            printf("  %d", *value);
        }
        printf("\n");
        
        CN_linked_list_iterator_destroy(iter);
    }
    
    // 测试头部和尾部操作
    int head_value = 100;
    CN_linked_list_prepend(list, &head_value);
    
    int tail_value = 200;
    CN_linked_list_append(list, &tail_value);
    
    printf("更新后链表长度: %zu\n", CN_linked_list_length(list));
    printf("第一个元素: %d\n", *(int*)CN_linked_list_first(list));
    printf("最后一个元素: %d\n", *(int*)CN_linked_list_last(list));
    
    // 清理
    CN_linked_list_destroy(list);
}
```

## 版本历史

### v1.0.0 (2026-01-02)
- 初始版本
- 支持单向链表和双向链表
- 实现完整的链表操作功能
- 提供迭代器支持（双向链表支持反向遍历）
- 实现搜索、反转、连接、复制等操作

### v1.1.0 (2026-01-02)
- 模块化重构，遵循单一职责原则
- 分解为多个单一职责的源文件
- 完善错误处理和内存管理
- 添加完整的API文档

## 相关文档

- [README.md](../../src/infrastructure/containers/linked_list/README.md) - 模块概述和使用说明
- [CN_linked_list.h](../../src/infrastructure/containers/linked_list/CN_linked_list.h) - 公共接口头文件
- [test_linked_list.c](../../tests/infrastructure/containers/test_linked_list.c) - 测试程序示例
- [架构设计原则](../../architecture/架构设计原则.md) - 项目架构设计原则

## 许可证

MIT许可证
