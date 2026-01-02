# CN_linked_list 模块

## 概述

CN_linked_list 是 CN_Language 项目的基础设施层容器模块，提供了单向链表和双向链表的实现。该模块遵循项目的 SOLID 设计原则和架构规范，确保代码的可维护性、可扩展性和可测试性。

## 特性

- 支持单向链表和双向链表两种类型
- 类型安全的泛型设计（通过元素大小参数）
- 完整的迭代器支持
- 自定义比较、复制和释放函数
- 内存安全，无内存泄漏
- 完整的错误处理
- 符合项目编码规范（每个函数不超过50行，每个文件不超过500行）

## 接口说明

### 数据结构

#### 链表类型枚举
```c
typedef enum Eum_CN_LinkedListType_t
{
    Eum_LINKED_LIST_SINGLY = 0,    /**< 单向链表 */
    Eum_LINKED_LIST_DOUBLY = 1     /**< 双向链表 */
} Eum_CN_LinkedListType_t;
```

#### 链表结构体（不透明类型）
```c
typedef struct Stru_CN_LinkedList_t Stru_CN_LinkedList_t;
```

#### 链表节点结构体（不透明类型）
```c
typedef struct Stru_CN_ListNode_t Stru_CN_ListNode_t;
```

#### 链表迭代器结构体
```c
typedef struct Stru_CN_LinkedListIterator_t
{
    Stru_CN_LinkedList_t* list;        /**< 关联的链表 */
    Stru_CN_ListNode_t* current_node;  /**< 当前节点 */
    size_t current_index;              /**< 当前索引 */
} Stru_CN_LinkedListIterator_t;
```

### 函数指针类型

#### 比较函数
```c
typedef int (*CN_LinkedListCompareFunc)(const void* elem1, const void* elem2);
```

#### 释放函数
```c
typedef void (*CN_LinkedListFreeFunc)(void* element);
```

#### 复制函数
```c
typedef void* (*CN_LinkedListCopyFunc)(const void* src);
```

### 主要API函数

#### 创建和销毁
- `CN_linked_list_create()` - 创建链表
- `CN_linked_list_create_custom()` - 创建带自定义函数的链表
- `CN_linked_list_destroy()` - 销毁链表
- `CN_linked_list_clear()` - 清空链表

#### 属性查询
- `CN_linked_list_length()` - 获取链表长度
- `CN_linked_list_is_empty()` - 检查链表是否为空
- `CN_linked_list_type()` - 获取链表类型
- `CN_linked_list_element_size()` - 获取元素大小

#### 元素访问
- `CN_linked_list_first()` - 获取第一个元素
- `CN_linked_list_last()` - 获取最后一个元素
- `CN_linked_list_get()` - 获取指定位置的元素
- `CN_linked_list_set()` - 设置指定位置的元素值

#### 元素操作
- `CN_linked_list_prepend()` - 在头部添加元素
- `CN_linked_list_append()` - 在尾部添加元素
- `CN_linked_list_insert()` - 在指定位置插入元素
- `CN_linked_list_remove_first()` - 移除头部元素
- `CN_linked_list_remove_last()` - 移除尾部元素
- `CN_linked_list_remove()` - 移除指定位置的元素

#### 链表操作
- `CN_linked_list_copy()` - 复制链表
- `CN_linked_list_concat()` - 连接两个链表
- `CN_linked_list_reverse()` - 反转链表
- `CN_linked_list_find()` - 查找元素
- `CN_linked_list_find_custom()` - 使用自定义比较函数查找元素
- `CN_linked_list_contains()` - 检查链表是否包含元素
- `CN_linked_list_equal()` - 比较两个链表是否相等

#### 迭代器支持
- `CN_linked_list_iterator_create()` - 创建迭代器
- `CN_linked_list_iterator_destroy()` - 销毁迭代器
- `CN_linked_list_iterator_reset()` - 重置迭代器
- `CN_linked_list_iterator_has_next()` - 检查是否有下一个元素
- `CN_linked_list_iterator_next()` - 获取下一个元素
- `CN_linked_list_iterator_current()` - 获取当前元素
- `CN_linked_list_iterator_has_prev()` - 检查是否有前一个元素（仅双向链表）
- `CN_linked_list_iterator_prev()` - 获取前一个元素（仅双向链表）

#### 工具函数
- `CN_linked_list_dump()` - 转储链表信息到控制台（调试用）

## 使用示例

### 基本使用

```c
#include "CN_linked_list.h"
#include <stdio.h>

int main(void)
{
    // 创建单向链表，存储整数
    Stru_CN_LinkedList_t* list = CN_linked_list_create(
        Eum_LINKED_LIST_SINGLY, sizeof(int));
    
    if (list == NULL)
    {
        printf("无法创建链表\n");
        return 1;
    }
    
    // 添加元素
    for (int i = 1; i <= 5; i++)
    {
        CN_linked_list_append(list, &i);
    }
    
    // 遍历链表
    printf("链表元素: ");
    for (size_t i = 0; i < CN_linked_list_length(list); i++)
    {
        int* value = (int*)CN_linked_list_get(list, i);
        if (value != NULL)
        {
            printf("%d ", *value);
        }
    }
    printf("\n");
    
    // 使用迭代器遍历
    Stru_CN_LinkedListIterator_t* iter = CN_linked_list_iterator_create(list);
    if (iter != NULL)
    {
        printf("使用迭代器遍历: ");
        while (CN_linked_list_iterator_has_next(iter))
        {
            int* value = (int*)CN_linked_list_iterator_next(iter);
            if (value != NULL)
            {
                printf("%d ", *value);
            }
        }
        printf("\n");
        CN_linked_list_iterator_destroy(iter);
    }
    
    // 清理
    CN_linked_list_destroy(list);
    return 0;
}
```

### 自定义类型使用

```c
#include "CN_linked_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 自定义类型
typedef struct
{
    int id;
    char name[32];
    float salary;
} Employee;

// 自定义比较函数
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
    if (copy != NULL)
    {
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

int main(void)
{
    // 创建带自定义函数的链表
    Stru_CN_LinkedList_t* employees = CN_linked_list_create_custom(
        Eum_LINKED_LIST_SINGLY,
        sizeof(Employee),
        compare_employees,
        free_employee,
        copy_employee);
    
    if (employees == NULL)
    {
        printf("无法创建链表\n");
        return 1;
    }
    
    // 添加员工
    Employee e1 = {1001, "张三", 5000.0};
    Employee e2 = {1002, "李四", 6000.0};
    Employee e3 = {1003, "王五", 5500.0};
    
    CN_linked_list_append(employees, &e1);
    CN_linked_list_append(employees, &e2);
    CN_linked_list_append(employees, &e3);
    
    // 查找员工
    Employee search_key = {1002, "", 0};
    size_t index = CN_linked_list_find(employees, &search_key);
    if (index != SIZE_MAX)
    {
        Employee* found = (Employee*)CN_linked_list_get(employees, index);
        printf("找到员工: ID=%d, 姓名=%s, 工资=%.1f\n", 
               found->id, found->name, found->salary);
    }
    
    // 复制链表（会使用自定义复制函数）
    Stru_CN_LinkedList_t* copy = CN_linked_list_copy(employees);
    if (copy != NULL)
    {
        printf("链表复制成功，副本长度: %zu\n", CN_linked_list_length(copy));
        CN_linked_list_destroy(copy);
    }
    
    // 清理
    CN_linked_list_destroy(employees);
    return 0;
}
```

### 双向链表使用

```c
#include "CN_linked_list.h"
#include <stdio.h>

int main(void)
{
    // 创建双向链表
    Stru_CN_LinkedList_t* list = CN_linked_list_create(
        Eum_LINKED_LIST_DOUBLY, sizeof(double));
    
    if (list == NULL)
    {
        printf("无法创建链表\n");
        return 1;
    }
    
    // 添加元素
    for (double i = 1.0; i <= 5.0; i += 1.0)
    {
        CN_linked_list_append(list, &i);
    }
    
    // 双向遍历
    printf("正向遍历: ");
    for (size_t i = 0; i < CN_linked_list_length(list); i++)
    {
        double* value = (double*)CN_linked_list_get(list, i);
        if (value != NULL)
        {
            printf("%.1f ", *value);
        }
    }
    printf("\n");
    
    // 使用迭代器双向遍历
    Stru_CN_LinkedListIterator_t* iter = CN_linked_list_iterator_create(list);
    if (iter != NULL)
    {
        // 正向遍历
        printf("迭代器正向遍历: ");
        while (CN_linked_list_iterator_has_next(iter))
        {
            double* value = (double*)CN_linked_list_iterator_next(iter);
            if (value != NULL)
            {
                printf("%.1f ", *value);
            }
        }
        printf("\n");
        
        // 反向遍历
        printf("迭代器反向遍历: ");
        while (CN_linked_list_iterator_has_prev(iter))
        {
            double* value = (double*)CN_linked_list_iterator_prev(iter);
            if (value != NULL)
            {
                printf("%.1f ", *value);
            }
        }
        printf("\n");
        
        CN_linked_list_iterator_destroy(iter);
    }
    
    // 反转链表
    CN_linked_list_reverse(list);
    printf("反转后: ");
    for (size_t i = 0; i < CN_linked_list_length(list); i++)
    {
        double* value = (double*)CN_linked_list_get(list, i);
        if (value != NULL)
        {
            printf("%.1f ", *value);
        }
    }
    printf("\n");
    
    // 清理
    CN_linked_list_destroy(list);
    return 0;
}
```

## 文件结构

```
src/infrastructure/containers/linked_list/
├── CN_linked_list.h              # 公共接口头文件
├── CN_linked_list_internal.h     # 内部结构定义（仅供模块内部使用）
├── CN_linked_list.c              # 创建、销毁和基本操作实现
├── CN_linked_list_operations.c   # 元素访问和修改操作实现
├── CN_linked_list_iterator.c     # 迭代器实现
├── CN_linked_list_utils.c        # 工具函数实现
└── README.md                     # 本文档
```

## 设计原则

### 单一职责原则
- 每个源文件负责一个明确的功能领域
- 每个函数不超过50行代码
- 每个源文件不超过500行代码

### 开闭原则
- 通过接口支持扩展
- 自定义函数支持灵活的行为定制

### 依赖倒置原则
- 高层模块（应用层）定义接口
- 低层模块（基础设施层）实现接口

### 内存安全
- 所有分配的内存都有对应的释放
- 支持自定义内存管理函数
- 边界检查和错误处理

## 测试

模块包含完整的单元测试，位于 `tests/infrastructure/containers/test_linked_list.c`。测试覆盖了：

1. 单向链表基本操作
2. 双向链表基本操作
3. 链表高级操作（复制、连接、反转等）
4. 链表迭代器
5. 自定义函数支持
6. 边界情况和错误处理

运行测试：
```bash
cd tests/infrastructure/containers
gcc -I../../../src/infrastructure/containers/linked_list \
    -I../../../src/infrastructure/containers/array \
    test_linked_list.c \
    ../../../src/infrastructure/containers/linked_list/CN_linked_list.c \
    ../../../src/infrastructure/containers/linked_list/CN_linked_list_operations.c \
    ../../../src/infrastructure/containers/linked_list/CN_linked_list_iterator.c \
    ../../../src/infrastructure/containers/linked_list/CN_linked_list_utils.c \
    -o test_linked_list
./test_linked_list
```

## 性能考虑

- **时间复杂度**：
  - 访问：O(n) 最坏情况，O(n/2) 平均情况（双向链表）
  - 插入/删除：O(1) 在头部/尾部，O(n) 在中间
  - 查找：O(n)

- **空间复杂度**：
  - 每个节点需要额外存储指针（单向：1个，双向：2个）
  - 内存开销：节点结构体 + 元素数据

- **优化**：
  - 双向链表支持从尾部反向遍历，提高平均访问速度
  - 迭代器缓存当前节点，减少重复遍历
  - 批量操作优化

## 注意事项

1. 链表不适合随机访问频繁的场景，考虑使用数组
2. 自定义释放函数负责释放元素内部的动态内存
3. 自定义复制函数应返回深拷贝
4. 迭代器在链表修改后可能失效，需要重新创建或重置
5. 线程安全：当前实现不是线程安全的，需要在外部同步

## 版本历史

- 2026-01-02: 初始版本，实现单向和双向链表

## 许可证

MIT 许可证
