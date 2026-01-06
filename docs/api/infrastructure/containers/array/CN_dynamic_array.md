# CN_dynamic_array API 文档

## 概述

`CN_dynamic_array` 模块提供了动态数组数据结构的实现。动态数组是一种可以自动调整大小的数组，支持在末尾高效添加元素，当数组满时会自动扩容。

## 头文件

```c
#include "infrastructure/containers/array/CN_dynamic_array.h"
```

## 数据结构

### Stru_DynamicArray_t

动态数组的主要结构体。

```c
typedef struct Stru_DynamicArray_t
{
    void** items;           /**< 元素指针数组 */
    size_t capacity;        /**< 当前分配的容量 */
    size_t length;          /**< 当前元素数量 */
    size_t item_size;       /**< 每个元素的大小（字节） */
} Stru_DynamicArray_t;
```

**字段说明：**

- `items`: 指向元素指针数组的指针
- `capacity`: 数组当前分配的容量（可容纳的最大元素数）
- `length`: 数组当前包含的元素数量
- `item_size`: 每个元素的大小（字节）

## 函数参考

### F_create_dynamic_array

```c
Stru_DynamicArray_t* F_create_dynamic_array(size_t item_size);
```

创建并初始化一个新的动态数组。

**参数：**
- `item_size`: 每个元素的大小（字节）

**返回值：**
- 成功：指向新创建的动态数组的指针
- 失败：NULL（内存分配失败或item_size为0）

**示例：**
```c
// 创建存储int类型的动态数组
Stru_DynamicArray_t* int_array = F_create_dynamic_array(sizeof(int));

// 创建存储自定义结构体的动态数组
typedef struct {
    int id;
    char name[32];
} Person;
Stru_DynamicArray_t* person_array = F_create_dynamic_array(sizeof(Person));
```

### F_destroy_dynamic_array

```c
void F_destroy_dynamic_array(Stru_DynamicArray_t* array);
```

销毁动态数组，释放所有相关内存。

**参数：**
- `array`: 要销毁的动态数组指针

**注意：**
- 如果`array`为NULL，函数不执行任何操作
- 函数会释放数组结构体、元素指针数组和所有元素数据

**示例：**
```c
Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
// ... 使用数组 ...
F_destroy_dynamic_array(array);
```

### F_dynamic_array_push

```c
bool F_dynamic_array_push(Stru_DynamicArray_t* array, void* item);
```

在动态数组的末尾添加一个新元素。

**参数：**
- `array`: 动态数组指针
- `item`: 要添加的元素指针

**返回值：**
- 成功：true
- 失败：false（参数无效或内存分配失败）

**注意：**
- 如果数组已满，会自动扩容为原来的2倍
- 函数会复制`item`指向的数据

**示例：**
```c
Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
int value = 42;
bool success = F_dynamic_array_push(array, &value);
```

### F_dynamic_array_get

```c
void* F_dynamic_array_get(Stru_DynamicArray_t* array, size_t index);
```

获取指定索引处的元素。

**参数：**
- `array`: 动态数组指针
- `index`: 元素索引（0-based）

**返回值：**
- 成功：指向元素的指针
- 失败：NULL（参数无效或索引越界）

**示例：**
```c
Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
int value = 42;
F_dynamic_array_push(array, &value);

int* retrieved = (int*)F_dynamic_array_get(array, 0);
if (retrieved != NULL) {
    printf("值: %d\n", *retrieved);
}
```

### F_dynamic_array_remove

```c
bool F_dynamic_array_remove(Stru_DynamicArray_t* array, size_t index);
```

移除指定索引处的元素。

**参数：**
- `array`: 动态数组指针
- `index`: 要移除的元素索引

**返回值：**
- 成功：true
- 失败：false（参数无效或索引越界）

**注意：**
- 移除元素后，后面的元素会向前移动
- 数组长度减1

**示例：**
```c
Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
// ... 添加一些元素 ...
bool removed = F_dynamic_array_remove(array, 2); // 移除索引2的元素
```

### F_dynamic_array_length

```c
size_t F_dynamic_array_length(Stru_DynamicArray_t* array);
```

获取动态数组中当前元素的数量。

**参数：**
- `array`: 动态数组指针

**返回值：**
- 数组长度，如果`array`为NULL返回0

**示例：**
```c
Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
// ... 添加一些元素 ...
size_t len = F_dynamic_array_length(array);
printf("数组长度: %zu\n", len);
```

### F_dynamic_array_capacity

```c
size_t F_dynamic_array_capacity(Stru_DynamicArray_t* array);
```

获取动态数组当前分配的容量。

**参数：**
- `array`: 动态数组指针

**返回值：**
- 数组容量，如果`array`为NULL返回0

**示例：**
```c
Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
size_t cap = F_dynamic_array_capacity(array);
printf("数组容量: %zu\n", cap);
```

### F_dynamic_array_clear

```c
void F_dynamic_array_clear(Stru_DynamicArray_t* array);
```

清空动态数组中的所有元素，但不释放数组本身。

**参数：**
- `array`: 动态数组指针

**注意：**
- 清空后数组长度变为0，但容量保持不变
- 会释放所有元素数据

**示例：**
```c
Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
// ... 添加一些元素 ...
F_dynamic_array_clear(array); // 清空数组
```

### F_dynamic_array_resize

```c
bool F_dynamic_array_resize(Stru_DynamicArray_t* array, size_t new_capacity);
```

调整动态数组的容量到指定大小。

**参数：**
- `array`: 动态数组指针
- `new_capacity`: 新的容量大小

**返回值：**
- 成功：true
- 失败：false（参数无效或内存分配失败）

**注意：**
- 如果`new_capacity`小于当前长度，会截断数组
- 截断时会释放被移除的元素

**示例：**
```c
Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
// ... 添加一些元素 ...
bool resized = F_dynamic_array_resize(array, 100); // 调整容量为100
```

### F_dynamic_array_foreach

```c
void F_dynamic_array_foreach(Stru_DynamicArray_t* array, 
                            void (*callback)(void* item, void* user_data),
                            void* user_data);
```

对动态数组中的每个元素执行指定的操作。

**参数：**
- `array`: 动态数组指针
- `callback`: 回调函数，接收元素指针和用户数据
- `user_data`: 传递给回调函数的用户数据

**注意：**
- 如果`array`或`callback`为NULL，函数不执行任何操作
- 回调函数不应该修改数组结构

**示例：**
```c
void print_int(void* item, void* user_data) {
    int* value = (int*)item;
    printf("值: %d\n", *value);
}

Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
// ... 添加一些元素 ...
F_dynamic_array_foreach(array, print_int, NULL);
```

## 配置宏

### CN_DYNAMIC_ARRAY_INITIAL_CAPACITY

```c
#define CN_DYNAMIC_ARRAY_INITIAL_CAPACITY 16
```

动态数组的初始容量。创建新数组时分配的初始元素数量。

### CN_DYNAMIC_ARRAY_GROWTH_FACTOR

```c
#define CN_DYNAMIC_ARRAY_GROWTH_FACTOR 2
```

动态数组的扩容因子。当数组满时，容量会乘以这个因子进行扩容。

## 错误处理

所有函数都包含错误检查，无效参数会返回适当的错误值：

- `NULL`: 用于返回指针的函数
- `false`: 用于返回布尔值的函数
- `0`: 用于返回大小的函数

## 内存管理

动态数组模块管理以下内存：
1. 动态数组结构体本身
2. 元素指针数组
3. 每个元素的实际数据

用户只需要：
1. 调用`F_create_dynamic_array`创建数组
2. 提供要存储的数据
3. 调用`F_destroy_dynamic_array`销毁数组

## 线程安全

当前实现不是线程安全的。如果需要在多线程环境中使用，需要外部同步机制。

## 性能特征

### 时间复杂度
- 访问：O(1)
- 插入（末尾）：平均O(1)，最坏O(n)（需要扩容）
- 删除：O(n)（需要移动元素）
- 搜索：O(n)

### 空间复杂度
- O(n)，其中n是数组容量

## 使用示例

### 基本使用

```c
#include "infrastructure/containers/array/CN_dynamic_array.h"
#include <stdio.h>

int main() {
    // 创建动态数组
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
    if (array == NULL) {
        printf("创建数组失败\n");
        return 1;
    }
    
    // 添加元素
    for (int i = 0; i < 10; i++) {
        F_dynamic_array_push(array, &i);
    }
    
    // 遍历元素
    printf("数组元素:\n");
    for (size_t i = 0; i < F_dynamic_array_length(array); i++) {
        int* value = (int*)F_dynamic_array_get(array, i);
        if (value != NULL) {
            printf("  [%zu] = %d\n", i, *value);
        }
    }
    
    // 清理
    F_destroy_dynamic_array(array);
    return 0;
}
```

### 存储自定义结构体

```c
#include "infrastructure/containers/array/CN_dynamic_array.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    int id;
    char name[32];
    float score;
} Student;

void print_student(void* item, void* user_data) {
    Student* s = (Student*)item;
    printf("学生: ID=%d, 姓名=%s, 分数=%.2f\n", 
           s->id, s->name, s->score);
}

int main() {
    Stru_DynamicArray_t* students = F_create_dynamic_array(sizeof(Student));
    
    Student s1 = {1, "张三", 85.5};
    Student s2 = {2, "李四", 92.0};
    Student s3 = {3, "王五", 78.5};
    
    F_dynamic_array_push(students, &s1);
    F_dynamic_array_push(students, &s2);
    F_dynamic_array_push(students, &s3);
    
    printf("所有学生:\n");
    F_dynamic_array_foreach(students, print_student, NULL);
    
    F_destroy_dynamic_array(students);
    return 0;
}
```

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本 |

## 相关文档

- [动态数组模块README](../src/infrastructure/containers/array/README.md)
- [CN_Language项目架构文档](../../../../architecture/001-中文编程语言CN_Language开发规划.md)
