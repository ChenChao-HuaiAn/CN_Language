# 链表模块 (Linked List Module)

## 概述

链表模块提供了单向链表数据结构的实现。链表支持高效的插入和删除操作，特别适合频繁修改的场景。

## 特性

- **高效插入删除**：在头部和尾部插入删除操作时间复杂度为O(1)
- **动态大小**：支持动态调整链表大小
- **类型安全**：通过元素大小参数确保类型安全
- **内存管理**：自动管理内存分配和释放
- **遍历支持**：支持前向遍历和回调函数遍历

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

## API 接口

### 创建和销毁

- `F_create_linked_list(size_t item_size)` - 创建链表
- `F_destroy_linked_list(Stru_LinkedList_t* list)` - 销毁链表

### 插入操作

- `F_linked_list_prepend(Stru_LinkedList_t* list, void* data)` - 在头部插入
- `F_linked_list_append(Stru_LinkedList_t* list, void* data)` - 在尾部插入
- `F_linked_list_insert(Stru_LinkedList_t* list, void* data, size_t position)` - 在指定位置插入

### 删除操作

- `F_linked_list_remove_first(Stru_LinkedList_t* list, void* out_data)` - 移除头部元素
- `F_linked_list_remove_last(Stru_LinkedList_t* list, void* out_data)` - 移除尾部元素
- `F_linked_list_remove_at(Stru_LinkedList_t* list, size_t position, void* out_data)` - 移除指定位置元素

### 查询操作

- `F_linked_list_length(Stru_LinkedList_t* list)` - 获取链表长度
- `F_linked_list_get(Stru_LinkedList_t* list, size_t position)` - 获取指定位置元素
- `F_linked_list_find(Stru_LinkedList_t* list, void* data, int (*compare)(void*, void*))` - 查找元素位置
- `F_linked_list_is_empty(Stru_LinkedList_t* list)` - 检查链表是否为空

### 链表操作

- `F_linked_list_clear(Stru_LinkedList_t* list)` - 清空链表
- `F_linked_list_foreach(Stru_LinkedList_t* list, void (*callback)(void* data, void* user_data), void* user_data)` - 遍历链表

## 使用示例

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

## 性能特征

- **时间复杂度**：
  - 访问：O(n)
  - 插入（头部/尾部）：O(1)
  - 插入（中间）：O(n)
  - 删除（头部）：O(1)
  - 删除（尾部）：O(n)（单向链表需要遍历）
  - 删除（中间）：O(n)
  - 搜索：O(n)

- **空间复杂度**：O(n)

## 内存管理

链表模块负责管理以下内存：
1. 链表结构体本身
2. 所有节点结构体
3. 每个节点的数据

用户只需要提供要存储的数据，模块会自动处理内存分配和释放。

## 错误处理

所有函数都包含错误检查：
- 无效参数返回适当错误值（NULL、false等）
- 内存分配失败返回错误
- 位置越界返回错误

## 线程安全

当前实现不是线程安全的。如果需要在多线程环境中使用，需要外部同步机制。

## 依赖关系

- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`, `stdint.h`
- 无其他项目模块依赖

## 测试

模块包含完整的单元测试，覆盖所有API接口和边界条件。测试覆盖率目标：
- 语句覆盖率：≥90%
- 分支覆盖率：≥80%
- 函数覆盖率：100%

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本链表功能 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
