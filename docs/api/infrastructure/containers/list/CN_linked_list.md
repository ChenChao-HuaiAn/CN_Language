# CN_linked_list API 文档

## 概述

`CN_linked_list` 模块提供了单向链表数据结构的实现。链表支持高效的插入和删除操作，特别适合频繁修改的场景。

## 头文件

```c
#include "infrastructure/containers/list/CN_linked_list.h"
```

## 数据结构

### Stru_LinkedList_t

链表的主要结构体。

```c
typedef struct Stru_LinkedList_t
{
    Stru_ListNode_t* head;      /**< 链表头节点 */
    Stru_ListNode_t* tail;      /**< 链表尾节点 */
    size_t length;              /**< 链表长度 */
    size_t item_size;           /**< 每个元素的大小（字节） */
} Stru_LinkedList_t;
```

**字段说明：**

- `head`: 指向链表第一个节点的指针
- `tail`: 指向链表最后一个节点的指针
- `length`: 链表中元素的数量
- `item_size`: 每个元素的大小（字节）

### Stru_ListNode_t

链表节点结构体。

```c
typedef struct Stru_ListNode_t
{
    void* data;                 /**< 节点数据指针 */
    struct Stru_ListNode_t* next; /**< 指向下一个节点的指针 */
} Stru_ListNode_t;
```

**字段说明：**

- `data`: 指向节点数据的指针
- `next`: 指向下一个节点的指针

## 函数参考

### F_create_linked_list

```c
Stru_LinkedList_t* F_create_linked_list(size_t item_size);
```

创建并初始化一个新的链表。

**参数：**
- `item_size`: 每个元素的大小（字节）

**返回值：**
- 成功：指向新创建的链表的指针
- 失败：NULL（内存分配失败或item_size为0）

**示例：**
```c
// 创建存储int类型的链表
Stru_LinkedList_t* int_list = F_create_linked_list(sizeof(int));

// 创建存储自定义结构体的链表
typedef struct {
    int id;
    char name[32];
} Person;
Stru_LinkedList_t* person_list = F_create_linked_list(sizeof(Person));
```

### F_destroy_linked_list

```c
void F_destroy_linked_list(Stru_LinkedList_t* list);
```

销毁链表，释放所有相关内存。

**参数：**
- `list`: 要销毁的链表指针

**注意：**
- 如果`list`为NULL，函数不执行任何操作
- 函数会释放链表结构体、所有节点和节点数据

**示例：**
```c
Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
// ... 使用链表 ...
F_destroy_linked_list(list);
```

### F_linked_list_prepend

```c
bool F_linked_list_prepend(Stru_LinkedList_t* list, void* data);
```

在链表的头部插入一个新元素。

**参数：**
- `list`: 链表指针
- `data`: 要插入的数据指针

**返回值：**
- 成功：true
- 失败：false（参数无效或内存分配失败）

**时间复杂度：** O(1)

**示例：**
```c
Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
int value = 42;
bool success = F_linked_list_prepend(list, &value);
```

### F_linked_list_append

```c
bool F_linked_list_append(Stru_LinkedList_t* list, void* data);
```

在链表的尾部插入一个新元素。

**参数：**
- `list`: 链表指针
- `data`: 要插入的数据指针

**返回值：**
- 成功：true
- 失败：false（参数无效或内存分配失败）

**时间复杂度：** O(1)

**示例：**
```c
Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
int value = 42;
bool success = F_linked_list_append(list, &value);
```

### F_linked_list_insert

```c
bool F_linked_list_insert(Stru_LinkedList_t* list, void* data, size_t position);
```

在链表的指定位置插入一个新元素。

**参数：**
- `list`: 链表指针
- `data`: 要插入的数据指针
- `position`: 插入位置（0-based）

**返回值：**
- 成功：true
- 失败：false（参数无效、内存分配失败或位置无效）

**注意：**
- 如果`position`等于链表长度，则在尾部插入
- 如果`position`为0，则在头部插入

**时间复杂度：** O(n)，其中n是位置

**示例：**
```c
Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
int value = 42;
bool success = F_linked_list_insert(list, &value, 2); // 在位置2插入
```

### F_linked_list_remove_first

```c
bool F_linked_list_remove_first(Stru_LinkedList_t* list, void* out_data);
```

移除链表的头部元素。

**参数：**
- `list`: 链表指针
- `out_data`: 输出参数，用于接收移除的数据（可为NULL）

**返回值：**
- 成功：true
- 失败：false（链表为空）

**时间复杂度：** O(1)

**示例：**
```c
Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
// ... 添加一些元素 ...
int removed_value;
bool removed = F_linked_list_remove_first(list, &removed_value);
if (removed) {
    printf("移除的值: %d\n", removed_value);
}
```

### F_linked_list_remove_last

```c
bool F_linked_list_remove_last(Stru_LinkedList_t* list, void* out_data);
```

移除链表的尾部元素。

**参数：**
- `list`: 链表指针
- `out_data`: 输出参数，用于接收移除的数据（可为NULL）

**返回值：**
- 成功：true
- 失败：false（链表为空）

**时间复杂度：** O(n)，其中n是链表长度

**示例：**
```c
Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
// ... 添加一些元素 ...
int removed_value;
bool removed = F_linked_list_remove_last(list, &removed_value);
if (removed) {
    printf("移除的值: %d\n", removed_value);
}
```

### F_linked_list_remove_at

```c
bool F_linked_list_remove_at(Stru_LinkedList_t* list, size_t position, void* out_data);
```

移除链表中指定位置的元素。

**参数：**
- `list`: 链表指针
- `position`: 要移除的元素位置（0-based）
- `out_data`: 输出参数，用于接收移除的数据（可为NULL）

**返回值：**
- 成功：true
- 失败：false（参数无效或位置无效）

**时间复杂度：** O(n)，其中n是位置

**示例：**
```c
Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
// ... 添加一些元素 ...
int removed_value;
bool removed = F_linked_list_remove_at(list, 2, &removed_value); // 移除位置2的元素
```

### F_linked_list_length

```c
size_t F_linked_list_length(Stru_LinkedList_t* list);
```

获取链表中当前元素的数量。

**参数：**
- `list`: 链表指针

**返回值：**
- 链表长度，如果`list`为NULL返回0

**示例：**
```c
Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
// ... 添加一些元素 ...
size_t len = F_linked_list_length(list);
printf("链表长度: %zu\n", len);
```

### F_linked_list_get

```c
void* F_linked_list_get(Stru_LinkedList_t* list, size_t position);
```

获取链表中指定位置的元素。

**参数：**
- `list`: 链表指针
- `position`: 元素位置（0-based）

**返回值：**
- 成功：指向元素的指针
- 失败：NULL（参数无效或位置无效）

**时间复杂度：** O(n)，其中n是位置

**示例：**
```c
Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
// ... 添加一些元素 ...
int* value = (int*)F_linked_list_get(list, 2); // 获取位置2的元素
if (value != NULL) {
    printf("值: %d\n", *value);
}
```

### F_linked_list_find

```c
int64_t F_linked_list_find(Stru_LinkedList_t* list, void* data, 
                          int (*compare)(void*, void*));
```

查找指定元素在链表中的位置。

**参数：**
- `list`: 链表指针
- `data`: 要查找的数据指针
- `compare`: 比较函数，返回0表示相等

**返回值：**
- 成功：元素位置（0-based）
- 失败：-1（未找到或参数无效）

**时间复杂度：** O(n)，其中n是链表长度

**示例：**
```c
int compare_int(void* a, void* b) {
    int* ia = (int*)a;
    int* ib = (int*)b;
    return (*ia == *ib) ? 0 : 1;
}

Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
// ... 添加一些元素 ...
int search_value = 42;
int64_t pos = F_linked_list_find(list, &search_value, compare_int);
if (pos >= 0) {
    printf("找到值 %d 在位置 %lld\n", search_value, pos);
}
```

### F_linked_list_clear

```c
void F_linked_list_clear(Stru_LinkedList_t* list);
```

清空链表中的所有元素，但不释放链表本身。

**参数：**
- `list`: 链表指针

**注意：**
- 清空后链表长度变为0
- 会释放所有节点和节点数据

**示例：**
```c
Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
// ... 添加一些元素 ...
F_linked_list_clear(list); // 清空链表
```

### F_linked_list_foreach

```c
void F_linked_list_foreach(Stru_LinkedList_t* list,
                          void (*callback)(void* data, void* user_data),
                          void* user_data);
```

对链表中的每个元素执行指定的操作。

**参数：**
- `list`: 链表指针
- `callback`: 回调函数，接收元素指针和用户数据
- `user_data`: 传递给回调函数的用户数据

**注意：**
- 如果`list`或`callback`为NULL，函数不执行任何操作
- 回调函数不应该修改链表结构

**示例：**
```c
void print_int(void* data, void* user_data) {
    int* value = (int*)data;
    printf("值: %d\n", *value);
}

Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
// ... 添加一些元素 ...
F_linked_list_foreach(list, print_int, NULL);
```

### F_linked_list_is_empty

```c
bool F_linked_list_is_empty(Stru_LinkedList_t* list);
```

检查链表是否不包含任何元素。

**参数：**
- `list`: 链表指针

**返回值：**
- 链表为空返回true，否则返回false
- 如果`list`为NULL返回true

**示例：**
```c
Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
bool empty = F_linked_list_is_empty(list);
printf("链表是否为空: %s\n", empty ? "是" : "否");
```

## 错误处理

所有函数都包含错误检查，无效参数会返回适当的错误值：

- `NULL`: 用于返回指针的函数
- `false`: 用于返回布尔值的函数
- `0`: 用于返回大小的函数
- `-1`: 用于返回位置的函数

## 内存管理

链表模块管理以下内存：
1. 链表结构体本身
2. 所有节点结构体
3. 每个节点的数据

用户只需要：
1. 调用`F_create_linked_list`创建链表
2. 提供要存储的数据
3. 调用`F_destroy_linked_list`销毁链表

## 线程安全

当前实现不是线程安全的。如果需要在多线程环境中使用，需要外部同步机制。

## 性能特征

### 时间复杂度
- 访问：O(n)
- 插入（头部/尾部）：O(1)
- 插入（中间）：O(n)
- 删除（头部）：O(1)
- 删除（尾部）：O(n)
- 删除（中间）：O(n)
- 搜索：O(n)

### 空间复杂度
- O(n)，其中n是链表长度

## 使用示例

### 基本使用

```c
#include "infrastructure/containers/list/CN_linked_list.h"
#include <stdio.h>

int main() {
    // 创建链表
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    if (list == NULL) {
        printf("创建链表失败\n");
        return 1;
    }
    
    // 添加元素
    for (int i = 0; i < 10; i++) {
        F_linked_list_append(list, &i);
    }
    
    // 遍历元素
    printf("链表元素:\n");
    for (size_t i = 0; i < F_linked_list_length(list); i++) {
        int* value = (int*)F_linked_list_get(list, i);
        if (value != NULL) {
            printf("  [%zu] = %d\n", i, *value);
        }
    }
    
    // 清理
    F_destroy_linked_list(list);
    return 0;
}
```

### 存储自定义结构体

```c
#include "infrastructure/containers/list/CN_linked_list.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    int id;
    char name[32];
    float score;
} Student;

void print_student(void* data, void* user_data) {
    Student* s = (Student*)data;
    printf("学生: ID=%d, 姓名=%s, 分数=%.2f\n", 
           s->id, s->name, s->score);
}

int main() {
    Stru_LinkedList_t* students = F_create_linked_list(sizeof(Student));
    
    Student s1 = {1, "张三", 85.5};
    Student s2 = {2, "李四", 92.0};
    Student s3 = {3, "王五", 78.5};
    
    F_linked_list_append(students, &s1);
    F_linked_list_append(students, &s2);
    F_linked_list_append(students, &s3);
    
    printf("所有学生:\n");
    F_linked_list_foreach(students, print_student, NULL);
    
    F_destroy_linked_list(students);
    return 0;
}
```

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本 |

## 相关文档

- [链表模块README](../src/infrastructure/containers/list/README.md)
- [CN_Language项目架构文档](../../../../architecture/001-中文编程语言CN_Language开发规划.md)
