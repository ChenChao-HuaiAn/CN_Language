# 链表模块 (Linked List Module)

## 概述

链表模块提供了单向链表数据结构的实现，采用模块化设计，遵循单一职责原则。链表支持高效的插入和删除操作，特别适合频繁修改的场景。

## 模块结构

链表模块采用模块化设计，分为以下子模块：

```
src/infrastructure/containers/list/
├── CN_linked_list.h          # 主头文件（包含所有接口）
├── CN_linked_list.c          # 主实现文件（基础操作）
├── CN_linked_list_iterator.h # 迭代器接口
├── CN_linked_list_iterator.c # 迭代器实现
├── CN_linked_list_sort.h     # 排序接口
├── CN_linked_list_sort.c     # 排序实现
└── README.md                 # 模块文档
```

### 模块职责

1. **基础链表模块** (`CN_linked_list.[hc]`)：提供链表的基本操作，如创建、销毁、插入、删除、查询等。
2. **迭代器模块** (`CN_linked_list_iterator.[hc]`)：提供安全的链表遍历接口，支持在遍历过程中插入和删除元素。
3. **排序模块** (`CN_linked_list_sort.[hc]`)：提供多种排序算法，包括冒泡排序、插入排序、归并排序等。

## 特性

- **模块化设计**：遵循单一职责原则，每个模块功能明确
- **高效插入删除**：在头部和尾部插入删除操作时间复杂度为O(1)
- **动态大小**：支持动态调整链表大小
- **类型安全**：通过元素大小参数确保类型安全
- **内存管理**：自动管理内存分配和释放
- **遍历支持**：支持前向遍历、回调函数遍历和迭代器遍历
- **排序功能**：支持多种排序算法
- **错误处理**：完善的错误检查和返回值

## 数据结构

### Stru_LinkedList_t

链表的主要结构体：

```c
typedef struct Stru_LinkedList_t
{
    Stru_ListNode_t* head;      // 链表头节点
    Stru_ListNode_t* tail;      // 链表尾节点
    size_t length;              // 链表长度
    size_t item_size;           // 每个元素的大小（字节）
} Stru_LinkedList_t;
```

### Stru_ListNode_t

链表节点结构体：

```c
typedef struct Stru_ListNode_t
{
    void* data;                 // 节点数据指针
    struct Stru_ListNode_t* next; // 指向下一个节点的指针
} Stru_ListNode_t;
```

### Stru_LinkedListIterator_t

链表迭代器结构体：

```c
typedef struct Stru_LinkedListIterator_t
{
    Stru_LinkedList_t* list;    // 关联的链表
    Stru_ListNode_t* current;   // 当前节点
    size_t position;            // 当前位置
    bool is_valid;              // 迭代器是否有效
} Stru_LinkedListIterator_t;
```

## API 接口

### 基础链表操作

#### 创建和销毁
- `F_create_linked_list(size_t item_size)` - 创建链表
- `F_destroy_linked_list(Stru_LinkedList_t* list)` - 销毁链表

#### 插入操作
- `F_linked_list_prepend(Stru_LinkedList_t* list, void* data)` - 在头部插入
- `F_linked_list_append(Stru_LinkedList_t* list, void* data)` - 在尾部插入
- `F_linked_list_insert(Stru_LinkedList_t* list, void* data, size_t position)` - 在指定位置插入

#### 删除操作
- `F_linked_list_remove_first(Stru_LinkedList_t* list, void* out_data)` - 移除头部元素
- `F_linked_list_remove_last(Stru_LinkedList_t* list, void* out_data)` - 移除尾部元素
- `F_linked_list_remove_at(Stru_LinkedList_t* list, size_t position, void* out_data)` - 移除指定位置元素

#### 查询操作
- `F_linked_list_length(Stru_LinkedList_t* list)` - 获取链表长度
- `F_linked_list_get(Stru_LinkedList_t* list, size_t position)` - 获取指定位置元素
- `F_linked_list_find(Stru_LinkedList_t* list, void* data, int (*compare)(void*, void*))` - 查找元素位置
- `F_linked_list_is_empty(Stru_LinkedList_t* list)` - 检查链表是否为空

#### 链表操作
- `F_linked_list_clear(Stru_LinkedList_t* list)` - 清空链表
- `F_linked_list_foreach(Stru_LinkedList_t* list, void (*callback)(void* data, void* user_data), void* user_data)` - 遍历链表

### 迭代器操作

#### 创建和销毁
- `F_create_linked_list_iterator(Stru_LinkedList_t* list)` - 创建链表迭代器
- `F_destroy_linked_list_iterator(Stru_LinkedListIterator_t* iterator)` - 销毁迭代器

#### 遍历操作
- `F_linked_list_iterator_reset(Stru_LinkedListIterator_t* iterator)` - 重置迭代器到链表头部
- `F_linked_list_iterator_has_next(Stru_LinkedListIterator_t* iterator)` - 检查是否有下一个元素
- `F_linked_list_iterator_next(Stru_LinkedListIterator_t* iterator)` - 获取下一个元素
- `F_linked_list_iterator_current(Stru_LinkedListIterator_t* iterator)` - 获取当前元素
- `F_linked_list_iterator_position(Stru_LinkedListIterator_t* iterator)` - 获取当前位置

#### 修改操作
- `F_linked_list_iterator_insert(Stru_LinkedListIterator_t* iterator, void* data)` - 在当前位置插入元素
- `F_linked_list_iterator_remove(Stru_LinkedListIterator_t* iterator, void* out_data)` - 移除当前元素

#### 工具函数
- `F_linked_list_iterator_is_valid(Stru_LinkedListIterator_t* iterator)` - 检查迭代器是否有效
- `F_linked_list_find_iterator(Stru_LinkedList_t* list, void* data, int (*compare)(void*, void*))` - 查找元素并创建迭代器

### 排序操作

#### 排序算法
- `F_linked_list_sort(Stru_LinkedList_t* list, int (*compare)(void*, void*), Eum_SortAlgorithm_t algorithm, Eum_SortDirection_t direction)` - 使用指定算法排序链表
- `F_linked_list_bubble_sort(Stru_LinkedList_t* list, int (*compare)(void*, void*), bool ascending)` - 冒泡排序
- `F_linked_list_insertion_sort(Stru_LinkedList_t* list, int (*compare)(void*, void*), bool ascending)` - 插入排序
- `F_linked_list_merge_sort(Stru_LinkedList_t* list, int (*compare)(void*, void*), bool ascending)` - 归并排序

#### 工具函数
- `F_linked_list_is_sorted(Stru_LinkedList_t* list, int (*compare)(void*, void*), bool ascending)` - 检查链表是否已排序
- `F_linked_list_reverse(Stru_LinkedList_t* list)` - 反转链表

## 使用示例

### 基础使用

```c
#include "CN_linked_list.h"
#include <stdio.h>
#include <string.h>

// 比较函数
int compare_int(void* a, void* b) {
    int* ia = (int*)a;
    int* ib = (int*)b;
    return (*ia == *ib) ? 0 : (*ia > *ib ? 1 : -1);
}

// 打印回调函数
void print_int(void* data, void* user_data) {
    int* value = (int*)data;
    printf("%d ", *value);
}

int main() {
    // 创建链表
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    if (list == NULL) {
        printf("创建链表失败\n");
        return 1;
    }
    
    // 插入元素
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        F_linked_list_append(list, &values[i]);
    }
    
    // 在头部插入
    int head_value = 5;
    F_linked_list_prepend(list, &head_value);
    
    // 在位置2插入
    int insert_value = 25;
    F_linked_list_insert(list, &insert_value, 2);
    
    // 遍历链表
    printf("链表元素: ");
    F_linked_list_foreach(list, print_int, NULL);
    printf("\n");
    
    // 查找元素
    int search_value = 30;
    int64_t pos = F_linked_list_find(list, &search_value, compare_int);
    if (pos >= 0) {
        printf("元素 %d 在位置 %lld\n", search_value, pos);
    }
    
    // 获取元素
    int* retrieved = (int*)F_linked_list_get(list, 3);
    if (retrieved != NULL) {
        printf("位置3的元素: %d\n", *retrieved);
    }
    
    // 移除元素
    int removed_value;
    F_linked_list_remove_first(list, &removed_value);
    printf("移除的头部元素: %d\n", removed_value);
    
    // 获取链表信息
    printf("链表长度: %zu\n", F_linked_list_length(list));
    printf("链表是否为空: %s\n", F_linked_list_is_empty(list) ? "是" : "否");
    
    // 清理
    F_destroy_linked_list(list);
    
    return 0;
}
```

### 使用迭代器

```c
#include "CN_linked_list.h"
#include <stdio.h>

int main() {
    // 创建链表并添加元素
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    for (int i = 0; i < 10; i++) {
        F_linked_list_append(list, &i);
    }
    
    // 创建迭代器
    Stru_LinkedListIterator_t* iter = F_create_linked_list_iterator(list);
    
    // 使用迭代器遍历链表
    printf("使用迭代器遍历:\n");
    while (F_linked_list_iterator_is_valid(iter)) {
        int* value = (int*)F_linked_list_iterator_current(iter);
        if (value != NULL) {
            printf("位置 %lld: 值 %d\n", 
                   F_linked_list_iterator_position(iter), *value);
        }
        F_linked_list_iterator_next(iter);
    }
    
    // 重置迭代器
    F_linked_list_iterator_reset(iter);
    
    // 在迭代器位置插入元素
    int new_value = 99;
    F_linked_list_iterator_insert(iter, &new_value);
    
    // 再次遍历显示插入后的结果
    printf("\n插入元素后:\n");
    F_linked_list_iterator_reset(iter);
    while (F_linked_list_iterator_is_valid(iter)) {
        int* value = (int*)F_linked_list_iterator_current(iter);
        if (value != NULL) {
            printf("位置 %lld: 值 %d\n", 
                   F_linked_list_iterator_position(iter), *value);
        }
        F_linked_list_iterator_next(iter);
    }
    
    // 销毁迭代器和链表
    F_destroy_linked_list_iterator(iter);
    F_destroy_linked_list(list);
    
    return 0;
}
```

### 使用排序功能

```c
#include "CN_linked_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 比较函数
int compare_int(void* a, void* b) {
    int* ia = (int*)a;
    int* ib = (int*)b;
    return (*ia - *ib);
}

// 打印链表
void print_list(Stru_LinkedList_t* list) {
    printf("链表元素: ");
    for (size_t i = 0; i < F_linked_list_length(list); i++) {
        int* value = (int*)F_linked_list_get(list, i);
        if (value != NULL) {
            printf("%d ", *value);
        }
    }
    printf("\n");
}

int main() {
    // 创建链表
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    if (list == NULL) {
        printf("创建链表失败\n");
        return 1;
    }
    
    // 添加随机元素
    srand(time(NULL));
    for (int i = 0; i < 15; i++) {
        int value = rand() % 100;
        F_linked_list_append(list, &value);
    }
    
    printf("原始链表:\n");
    print_list(list);
    
    // 检查是否已排序
    bool is_sorted = F_linked_list_is_sorted(list, compare_int, true);
    printf("链表是否已排序(升序): %s\n", is_sorted ? "是" : "否");
    
    // 使用冒泡排序
    bool success = F_linked_list_bubble_sort(list, compare_int, true);
    if (success) {
        printf("\n冒泡排序后(升序):\n");
        print_list(list);
    }
    
    // 使用归并排序（降序）
    success = F_linked_list_merge_sort(list, compare_int, false);
    if (success) {
        printf("\n归并排序后(降序):\n");
        print_list(list);
    }
    
    // 反转链表
    success = F_linked_list_reverse(list);
    if (success) {
        printf("\n反转链表后:\n");
        print_list(list);
    }
    
    // 使用通用排序接口
    success = F_linked_list_sort(list, compare_int, Eum_SORT_INSERTION, Eum_SORT_ASCENDING);
    if (success) {
        printf("\n插入排序后(升序):\n");
        print_list(list);
    }
    
    // 清理
    F_destroy_linked_list(list);
    
    return 0;
}
```

## 性能特征

### 时间复杂度

#### 基础操作
- **访问**：O(n)
- **插入（头部/尾部）**：O(1)
- **插入（中间）**：O(n)
- **删除（头部）**：O(1)
- **删除（尾部）**：O(n)（单向链表需要遍历）
- **删除（中间）**：O(n)
- **搜索**：O(n)

#### 排序算法
- **冒泡排序**：O(n²)
- **插入排序**：O(n²)
- **归并排序**：O(n log n)

### 空间复杂度
- **基础链表**：O(n)
- **排序算法**：
  - 冒泡排序：O(1)
  - 插入排序：O(1)
  - 归并排序：O(log n)（递归栈空间）

## 内存管理

链表模块负责管理以下内存：
1. 链表结构体本身
2. 所有节点结构体
3. 每个节点的数据
4. 迭代器结构体
5. 排序算法中的临时缓冲区

用户只需要：
1. 调用`F_create_linked_list`创建链表
2. 提供要存储的数据
3. 调用`F_destroy_linked_list`销毁链表

## 错误处理

所有函数都包含错误检查：
- 无效参数返回适当错误值（NULL、false等）
- 内存分配失败返回错误
- 位置越界返回错误
- 迭代器无效状态检查

## 线程安全

当前实现不是线程安全的。如果需要在多线程环境中使用，需要外部同步机制。

## 依赖关系

- **C标准库**：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`, `stdint.h`, `time.h`（仅示例）
- **项目内部**：无其他模块依赖

## 测试

模块包含完整的单元测试，覆盖所有API接口和边界条件。测试文件位于`tests/infrastructure/containers/list/`目录。

### 测试覆盖率目标
- **语句覆盖率**：≥90%
- **分支覆盖率**：≥80%
- **函数覆盖率**：100%

### 测试文件结构
```
tests/infrastructure/containers/list/
├── test_linked_list_basic.c     # 基础功能测试
├── test_linked_list_iterator.c  # 迭代器测试
├── test_linked_list_sort.c      # 排序功能测试
└── test_linked_list_edge.c      # 边界条件测试
```

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本链表功能 |
| 1.1.0 | 2026-01-07 | 模块化重构，添加迭代器和排序功能 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
