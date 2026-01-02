# CN_array 模块 API 文档

## 概述

本文档详细描述了CN_array模块的所有公共接口。CN_array模块提供了类型安全的动态数组容器，支持多种内置类型和自定义类型，具有自动扩容、完整操作API和迭代器支持。

## 架构位置

CN_array模块位于基础设施层（Infrastructure Layer），为上层模块提供基础数据管理功能。遵循项目分层架构设计原则，只依赖C标准库和操作系统API。

## 数据结构

### 枚举类型

#### `enum Eum_CN_ArrayElementType_t`
数组元素类型枚举，定义了CN_array支持的所有元素类型。

```c
typedef enum Eum_CN_ArrayElementType_t
{
    Eum_ARRAY_ELEMENT_INT8 = 0,     /**< 8位有符号整数 */
    Eum_ARRAY_ELEMENT_UINT8 = 1,    /**< 8位无符号整数 */
    Eum_ARRAY_ELEMENT_INT16 = 2,    /**< 16位有符号整数 */
    Eum_ARRAY_ELEMENT_UINT16 = 3,   /**< 16位无符号整数 */
    Eum_ARRAY_ELEMENT_INT32 = 4,    /**< 32位有符号整数 */
    Eum_ARRAY_ELEMENT_UINT32 = 5,   /**< 32位无符号整数 */
    Eum_ARRAY_ELEMENT_INT64 = 6,    /**< 64位有符号整数 */
    Eum_ARRAY_ELEMENT_UINT64 = 7,   /**< 64位无符号整数 */
    Eum_ARRAY_ELEMENT_FLOAT = 8,    /**< 单精度浮点数 */
    Eum_ARRAY_ELEMENT_DOUBLE = 9,   /**< 双精度浮点数 */
    Eum_ARRAY_ELEMENT_BOOL = 10,    /**< 布尔值 */
    Eum_ARRAY_ELEMENT_CHAR = 11,    /**< 字符 */
    Eum_ARRAY_ELEMENT_POINTER = 12, /**< 指针 */
    Eum_ARRAY_ELEMENT_CUSTOM = 13   /**< 自定义类型 */
} Eum_CN_ArrayElementType_t;
```

### 结构体类型

#### `struct Stru_CN_Array_t`
动态数组结构体（不透明类型）。外部代码只能通过指针访问，内部实现细节被隐藏。

```c
typedef struct Stru_CN_Array_t Stru_CN_Array_t;
```

#### `struct Stru_CN_ArrayIterator_t`
数组迭代器结构体，用于安全遍历数组元素。

```c
typedef struct Stru_CN_ArrayIterator_t
{
    Stru_CN_Array_t* array;        /**< 关联的数组 */
    size_t current_index;          /**< 当前索引 */
    void* current_element;         /**< 当前元素指针 */
} Stru_CN_ArrayIterator_t;
```

### 函数指针类型

#### `CN_ArrayCompareFunc`
数组比较函数类型，用于自定义类型的元素比较。

```c
typedef int (*CN_ArrayCompareFunc)(const void* elem1, const void* elem2);
```

**参数**：
- `elem1`: 第一个元素
- `elem2`: 第二个元素

**返回值**：
- `<0`: elem1 < elem2
- `0`: elem1 == elem2
- `>0`: elem1 > elem2

#### `CN_ArrayFreeFunc`
数组元素释放函数类型，用于自定义类型的资源释放。

```c
typedef void (*CN_ArrayFreeFunc)(void* element);
```

**参数**：
- `element`: 要释放的元素

#### `CN_ArrayCopyFunc`
数组元素复制函数类型，用于自定义类型的深度复制。

```c
typedef void* (*CN_ArrayCopyFunc)(const void* src);
```

**参数**：
- `src`: 源元素

**返回值**：新分配的元素副本

## 函数接口

### 数组创建和销毁

#### `CN_array_create`
创建标准类型的动态数组。

```c
Stru_CN_Array_t* CN_array_create(Eum_CN_ArrayElementType_t element_type,
                                 size_t element_size, size_t initial_capacity);
```

**参数**：
- `element_type`: 元素类型枚举值
- `element_size`: 元素大小（字节），对自定义类型必需
- `initial_capacity`: 初始容量（元素数量）

**返回值**：新创建的数组指针，失败返回NULL

**注意**：
- 对于内置类型（如Eum_ARRAY_ELEMENT_INT32），element_size参数会被忽略
- 对于自定义类型（Eum_ARRAY_ELEMENT_CUSTOM），必须提供正确的element_size
- 调用者负责使用`CN_array_destroy`释放返回的数组

#### `CN_array_create_custom`
创建带自定义函数的数组，支持复杂自定义类型。

```c
Stru_CN_Array_t* CN_array_create_custom(size_t element_size,
                                        size_t initial_capacity,
                                        CN_ArrayCompareFunc compare_func,
                                        CN_ArrayFreeFunc free_func,
                                        CN_ArrayCopyFunc copy_func);
```

**参数**：
- `element_size`: 元素大小（字节）
- `initial_capacity`: 初始容量
- `compare_func`: 比较函数（可为NULL）
- `free_func`: 释放函数（可为NULL）
- `copy_func`: 复制函数（可为NULL）

**返回值**：新创建的数组指针，失败返回NULL

**注意**：
- 如果提供`free_func`，数组销毁时会自动调用它释放每个元素
- 如果提供`copy_func`，数组复制时会使用它进行深度复制
- 如果提供`compare_func`，搜索和排序时会使用它

#### `CN_array_destroy`
销毁数组，释放所有分配的内存。

```c
void CN_array_destroy(Stru_CN_Array_t* array);
```

**参数**：
- `array`: 要销毁的数组指针

**注意**：
- 如果数组创建时指定了`free_func`，会调用它释放每个元素
- 此函数会释放数组内部分配的所有内存
- 可以安全地传递NULL参数

#### `CN_array_clear`
清空数组（移除所有元素），但不释放数组本身。

```c
void CN_array_clear(Stru_CN_Array_t* array);
```

**参数**：
- `array`: 要清空的数组

**注意**：
- 如果设置了`free_func`，会调用它释放每个元素
- 数组容量保持不变，可以重用

### 数组属性查询

#### `CN_array_length`
获取数组长度（元素数量）。

```c
size_t CN_array_length(const Stru_CN_Array_t* array);
```

**参数**：
- `array`: 数组

**返回值**：数组长度，如果array为NULL返回0

#### `CN_array_capacity`
获取数组容量（可容纳的元素数量）。

```c
size_t CN_array_capacity(const Stru_CN_Array_t* array);
```

**参数**：
- `array`: 数组

**返回值**：数组容量，如果array为NULL返回0

#### `CN_array_is_empty`
检查数组是否为空。

```c
bool CN_array_is_empty(const Stru_CN_Array_t* array);
```

**参数**：
- `array`: 数组

**返回值**：如果数组为空或为NULL返回true，否则返回false

#### `CN_array_element_size`
获取元素大小（字节）。

```c
size_t CN_array_element_size(const Stru_CN_Array_t* array);
```

**参数**：
- `array`: 数组

**返回值**：元素大小，如果array为NULL返回0

#### `CN_array_element_type`
获取元素类型。

```c
Eum_CN_ArrayElementType_t CN_array_element_type(const Stru_CN_Array_t* array);
```

**参数**：
- `array`: 数组

**返回值**：元素类型，如果array为NULL返回Eum_ARRAY_ELEMENT_CUSTOM

### 元素访问和修改

#### `CN_array_get`
获取元素指针。

```c
void* CN_array_get(const Stru_CN_Array_t* array, size_t index);
```

**参数**：
- `array`: 数组
- `index`: 元素索引（0-based）

**返回值**：元素指针，索引无效或array为NULL返回NULL

**注意**：
- 返回的指针指向数组内部存储，不要释放它
- 通过返回的指针修改元素是安全的

#### `CN_array_set`
设置元素值。

```c
bool CN_array_set(Stru_CN_Array_t* array, size_t index, const void* element);
```

**参数**：
- `array`: 数组
- `index`: 元素索引
- `element`: 元素值指针

**返回值**：设置成功返回true，失败返回false

**注意**：
- 如果设置了`free_func`，会先释放旧元素
- 新元素值会被复制到数组中

#### `CN_array_append`
在末尾添加元素。

```c
bool CN_array_append(Stru_CN_Array_t* array, const void* element);
```

**参数**：
- `array`: 数组
- `element`: 要添加的元素

**返回值**：添加成功返回true，失败返回false

**注意**：
- 如果数组容量不足，会自动扩容
- 元素值会被复制到数组中

#### `CN_array_insert`
在指定位置插入元素。

```c
bool CN_array_insert(Stru_CN_Array_t* array, size_t index, const void* element);
```

**参数**：
- `array`: 数组
- `index`: 插入位置（0-based）
- `element`: 要插入的元素

**返回值**：插入成功返回true，失败返回false

**注意**：
- 如果index等于数组长度，效果与`CN_array_append`相同
- 插入位置之后的元素会向后移动
- 如果数组容量不足，会自动扩容

#### `CN_array_remove`
移除指定位置的元素。

```c
bool CN_array_remove(Stru_CN_Array_t* array, size_t index);
```

**参数**：
- `array`: 数组
- `index`: 要移除的元素索引

**返回值**：移除成功返回true，失败返回false

**注意**：
- 如果设置了`free_func`，会调用它释放被移除的元素
- 移除位置之后的元素会向前移动

#### `CN_array_pop`
移除并返回最后一个元素。

```c
bool CN_array_pop(Stru_CN_Array_t* array, void* element);
```

**参数**：
- `array`: 数组
- `element`: 输出参数，接收移除的元素值（可为NULL）

**返回值**：移除成功返回true，失败返回false

**注意**：
- 如果element不为NULL，元素值会被复制到提供的缓冲区
- 如果设置了`free_func`，会调用它释放被移除的元素

### 数组容量管理

#### `CN_array_ensure_capacity`
确保数组有足够容量。

```c
bool CN_array_ensure_capacity(Stru_CN_Array_t* array, size_t min_capacity);
```

**参数**：
- `array`: 数组
- `min_capacity`: 最小容量要求

**返回值**：调整成功返回true，失败返回false

**注意**：
- 如果当前容量小于min_capacity，会扩容到至少min_capacity
- 扩容策略：按1.5倍增长，但至少满足min_capacity

#### `CN_array_shrink_to_fit`
缩小数组容量以匹配长度，减少内存占用。

```c
bool CN_array_shrink_to_fit(Stru_CN_Array_t* array);
```

**参数**：
- `array`: 数组

**返回值**：调整成功返回true，失败返回false

**注意**：
- 如果数组为空，会释放所有存储空间
- 如果容量大于长度，会重新分配内存到精确大小

### 数组操作

#### `CN_array_copy`
复制数组，创建新的数组副本。

```c
Stru_CN_Array_t* CN_array_copy(const Stru_CN_Array_t* src);
```

**参数**：
- `src`: 源数组

**返回值**：新创建的数组副本，失败返回NULL

**注意**：
- 如果源数组设置了`copy_func`，会使用它进行深度复制
- 否则进行浅复制（内存拷贝）
- 调用者负责销毁返回的副本

#### `CN_array_concat`
连接两个数组，创建新的数组。

```c
Stru_CN_Array_t* CN_array_concat(const Stru_CN_Array_t* array1,
                                 const Stru_CN_Array_t* array2);
```

**参数**：
- `array1`: 第一个数组
- `array2`: 第二个数组

**返回值**：新创建的连接后的数组，失败返回NULL

**注意**：
- 两个数组的元素类型和大小必须相同
- 自定义函数（compare_func、free_func、copy_func）也必须相同
- 调用者负责销毁返回的数组

#### `CN_array_slice`
获取子数组，创建新的数组。

```c
Stru_CN_Array_t* CN_array_slice(const Stru_CN_Array_t* array,
                                size_t start, size_t end);
```

**参数**：
- `array`: 源数组
- `start`: 起始索引（包含）
- `end`: 结束索引（不包含）

**返回值**：新创建的子数组，失败返回NULL

**注意**：
- 如果start等于end，返回空数组
- 范围：[start, end)
- 调用者负责销毁返回的子数组

### 搜索和排序

#### `CN_array_find`
查找元素，返回第一个匹配的索引。

```c
size_t CN_array_find(const Stru_CN_Array_t* array, const void* element);
```

**参数**：
- `array`: 数组
- `element`: 要查找的元素

**返回值**：元素索引，未找到返回SIZE_MAX

**注意**：
- 如果数组设置了`compare_func`，使用它进行比较
- 否则使用memcmp进行内存比较

#### `CN_array_find_custom`
使用自定义比较函数查找元素。

```c
size_t CN_array_find_custom(const Stru_CN_Array_t* array, const void* element,
                            CN_ArrayCompareFunc compare_func);
```

**参数**：
- `array`: 数组
- `element`: 要查找的元素
- `compare_func`: 自定义比较函数

**返回值**：元素索引，未找到返回SIZE_MAX

#### `CN_array_contains`
检查数组是否包含元素。

```c
bool CN_array_contains(const Stru_CN_Array_t* array, const void* element);
```

**参数**：
- `array`: 数组
- `element`: 要检查的元素

**返回值**：如果包含返回true，否则返回false

#### `CN_array_sort`
排序数组。

```c
void CN_array_sort(Stru_CN_Array_t* array);
```

**参数**：
- `array`: 数组

**注意**：
- 如果数组设置了`compare_func`，使用它进行比较
- 否则对于内置类型使用适当的比较函数
- 使用快速排序算法

#### `CN_array_sort_custom`
使用自定义比较函数排序数组。

```c
void CN_array_sort_custom(Stru_CN_Array_t* array, CN_ArrayCompareFunc compare_func);
```

**参数**：
- `array`: 数组
- `compare_func`: 自定义比较函数

#### `CN_array_reverse`
反转数组元素顺序。

```c
void CN_array_reverse(Stru_CN_Array_t* array);
```

**参数**：
- `array`: 数组

### 迭代器支持

#### `CN_array_iterator_create`
创建数组迭代器。

```c
Stru_CN_ArrayIterator_t* CN_array_iterator_create(Stru_CN_Array_t* array);
```

**参数**：
- `array`: 数组

**返回值**：新创建的迭代器，失败返回NULL

**注意**：
- 迭代器初始位置在第一个元素之前
- 调用者负责销毁返回的迭代器

#### `CN_array_iterator_destroy`
销毁迭代器。

```c
void CN_array_iterator_destroy(Stru_CN_ArrayIterator_t* iterator);
```

**参数**：
- `iterator`: 要销毁的迭代器

#### `CN_array_iterator_reset`
重置迭代器到开始位置。

```c
void CN_array_iterator_reset(Stru_CN_ArrayIterator_t* iterator);
```

**参数**：
- `iterator`: 迭代器

#### `CN_array_iterator_has_next`
检查迭代器是否有下一个元素。

```c
bool CN_array_iterator_has_next(const Stru_CN_ArrayIterator_t* iterator);
```

**参数**：
- `iterator`: 迭代器

**返回值**：如果有下一个元素返回true，否则返回false

#### `CN_array_iterator_next`
获取下一个元素，并移动迭代器位置。

```c
void* CN_array_iterator_next(Stru_CN_ArrayIterator_t* iterator);
```

**参数**：
- `iterator`: 迭代器

**返回值**：下一个元素的指针，没有更多元素返回NULL

#### `CN_array_iterator_current`
获取当前元素，不移动迭代器位置。

```c
void* CN_array_iterator_current(const Stru_CN_ArrayIterator_t* iterator);
```

**参数**：
- `iterator`: 迭代器

**返回值**：当前元素的指针

### 工具函数

#### `CN_array_swap`
交换两个元素。

```c
bool CN_array_swap(Stru_CN_Array_t* array, size_t index1, size_t index2);
```

**参数**：
- `array`: 数组
- `index1`: 第一个元素索引
- `index2`: 第二个元素索引

**返回值**：交换成功返回true，失败返回false

#### `CN_array_fill`
填充数组，用指定元素填充前count个位置。

```c
bool CN_array_fill(Stru_CN_Array_t* array, const void* element, size_t count);
```

**参数**：
- `array`: 数组
- `element`: 填充元素
- `count`: 填充数量

**返回值**：填充成功返回true，失败返回false

**注意**：
- 如果count大于数组长度，会自动扩容
- 如果设置了`free_func`，会先释放被覆盖的元素

#### `CN_array_equal`
比较两个数组是否相等。

```c
bool CN_array_equal(const Stru_CN_Array_t* array1, const Stru_CN_Array_t* array2);
```

**参数**：
- `array1`: 第一个数组
- `array2`: 第二个数组

**返回值**：如果数组相等返回true，否则返回false

**注意**：
- 两个数组的元素类型、大小和长度必须相同
- 如果设置了`compare_func`，使用它进行比较
- 否则使用memcmp进行内存比较

#### `CN_array_dump`
转储数组信息到控制台（调试用）。

```c
void CN_array_dump(const Stru_CN_Array_t* array);
```

**参数**：
- `array`: 数组

**注意**：
- 输出数组长度、容量、元素类型等信息
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
- 枚举返回函数：失败返回Eum_ARRAY_ELEMENT_CUSTOM

## 线程安全性

CN_array模块的函数不是线程安全的。如果需要在多线程环境中使用，调用者需要提供适当的同步机制。

## 内存管理

### 分配策略
1. **初始分配**：按initial_capacity分配
2. **动态扩容**：按1.5倍增长，但至少满足需求
3. **容量收缩**：支持手动收缩以减少内存占用

### 内存所有权
1. **数组所有权**：谁创建谁销毁（使用`CN_array_destroy`）
2. **元素所有权**：
   - 内置类型：数组管理内存
   - 自定义类型：如果提供`free_func`，数组会调用它
3. **迭代器所有权**：谁创建谁销毁（使用`CN_array_iterator_destroy`）

### 内存安全
- 所有内存访问都进行边界检查
- 防止缓冲区溢出
- 支持自定义内存释放函数

## 性能特性

### 时间复杂度
- **随机访问**：O(1)
- **末尾添加**：平均O(1)，最坏O(n)（需要扩容）
- **中间插入/删除**：O(n)
- **搜索**：O(n)（线性搜索）
- **排序**：O(n log n)（快速排序）

### 空间复杂度
- **存储开销**：capacity * element_size
- **额外开销**：少量元数据（类型、长度、容量等）
- **扩容开销**：临时需要2倍内存（复制期间）

### 优化特性
1. **批量操作**：支持批量填充和复制
2. **容量预分配**：支持预分配容量避免频繁扩容
3. **内存重用**：清空数组后可以重用已分配内存
4. **迭代器优化**：迭代访问避免重复边界检查

## 使用示例

### 示例1：创建和使用整数数组
```c
#include "CN_array.h"
#include <stdio.h>

void example_int_array(void)
{
    // 创建整数数组，初始容量10
    Stru_CN_Array_t* array = CN_array_create(Eum_ARRAY_ELEMENT_INT32, 
                                             sizeof(int), 10);
    if (array == NULL) {
        printf("创建数组失败\n");
        return;
    }
    
    // 添加元素
    for (int i = 1; i <= 15; i++) {
        if (!CN_array_append(array, &i)) {
            printf("添加元素 %d 失败\n", i);
        }
    }
    
    // 访问元素
    printf("数组长度: %zu\n", CN_array_length(array));
    for (size_t i = 0; i < CN_array_length(array); i++) {
        int* value = (int*)CN_array_get(array, i);
        if (value != NULL) {
            printf("array[%zu] = %d\n", i, *value);
        }
    }
    
    // 修改元素
    int new_value = 100;
    CN_array_set(array, 5, &new_value);
    
    // 销毁数组
    CN_array_destroy(array);
}
```

### 示例2：使用自定义类型数组
```c
#include "CN_array.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    int id;
    char name[50];
    float score;
} Student;

// 自定义比较函数（按ID比较）
int compare_students(const void* a, const void* b)
{
    const Student* s1 = (const Student*)a;
    const Student* s2 = (const Student*)b;
    return s1->id - s2->id;
}

// 自定义复制函数
void* copy_student(const void* src)
{
    const Student* s = (const Student*)src;
    Student* copy = malloc(sizeof(Student));
    if (copy != NULL) {
        copy->id = s->id;
        strcpy(copy->name, s->name);
        copy->score = s->score;
    }
    return copy;
}

// 自定义释放函数
void free_student(void* element)
{
    // Student结构体没有需要特别释放的资源
    // 这里只是演示自定义释放函数
    (void)element;
}

void example_custom_array(void)
{
    // 创建自定义类型数组
    Stru_CN_Array_t* students = CN_array_create_custom(
        sizeof(Student), 5, compare_students, free_student, copy_student);
    
    if (students == NULL) {
        printf("创建自定义数组失败\n");
        return;
    }
    
    // 添加学生
    Student s1 = {101, "张三", 85.5};
    Student s2 = {102, "李四", 92.0};
    Student s3 = {103, "王五", 78.5};
    
    CN_array_append(students, &s1);
    CN_array_append(students, &s2);
    CN_array_append(students, &s3);
    
    printf("学生数量: %zu\n", CN_array_length(students));
    
    // 查找学生
    Student search_key = {102, "", 0};
    size_t index = CN_array_find(students, &search_key);
    if (index != SIZE_MAX) {
        Student* found = (Student*)CN_array_get(students, index);
        printf("找到学生: ID=%d, 姓名=%s, 分数=%.1f\n", 
               found->id, found->name, found->score);
    }
    
    // 排序学生（按ID）
    CN_array_sort(students);
    
    // 使用迭代器遍历
    Stru_CN_ArrayIterator_t* iter = CN_array_iterator_create(students);
    if (iter != NULL) {
        printf("排序后的学生列表:\n");
        while (CN_array_iterator_has_next(iter)) {
            Student* s = (Student*)CN_array_iterator_next(iter);
            printf("  ID=%d, 姓名=%s, 分数=%.1f\n", s->id, s->name, s->score);
        }
        CN_array_iterator_destroy(iter);
    }
    
    // 销毁数组
    CN_array_destroy(students);
}
```

### 示例3：数组操作
```c
#include "CN_array.h"
#include <stdio.h>

void example_array_operations(void)
{
    // 创建两个数组
    Stru_CN_Array_t* arr1 = CN_array_create(Eum_ARRAY_ELEMENT_INT32, sizeof(int), 0);
    Stru_CN_Array_t* arr2 = CN_array_create(Eum_ARRAY_ELEMENT_INT32, sizeof(int), 0);
    
    // 填充数据
    for (int i = 1; i <= 5; i++) {
        CN_array_append(arr1, &i);
    }
    for (int i = 6; i <= 10; i++) {
        CN_array_append(arr2, &i);
    }
    
    // 连接数组
    Stru_CN_Array_t* combined = CN_array_concat(arr1, arr2);
    if (combined != NULL) {
        printf("连接后数组长度: %zu\n", CN_array_length(combined));
        
        // 获取子数组
        Stru_CN_Array_t* slice = CN_array_slice(combined, 3, 8);
        if (slice != NULL) {
            printf("子数组长度: %zu\n", CN_array_length(slice));
            CN_array_destroy(slice);
        }
        
        CN_array_destroy(combined);
    }
    
    // 数组复制
    Stru_CN_Array_t* copy = CN_array_copy(arr1);
    if (copy != NULL) {
        printf("数组副本长度: %zu\n", CN_array_length(copy));
        CN_array_destroy(copy);
    }
    
    // 清理
    CN_array_destroy(arr1);
    CN_array_destroy(arr2);
}
```

## 版本历史

### v1.0.0 (2026-01-02)
- 初始版本
- 支持多种内置类型和自定义类型
- 实现完整的动态数组功能
- 提供迭代器支持
- 实现搜索、排序、反转等操作

### v1.1.0 (2026-01-02)
- 模块化重构，遵循单一职责原则
- 分解为多个单一职责的源文件
- 完善错误处理和内存管理
- 添加完整的API文档

## 相关文档

- [README.md](../../src/infrastructure/containers/array/README.md) - 模块概述和使用说明
- [CN_array.h](../../src/infrastructure/containers/array/CN_array.h) - 公共接口头文件
- [test_array.c](../../src/infrastructure/containers/array/test_array.c) - 测试程序示例
- [架构设计原则](../../architecture/架构设计原则.md) - 项目架构设计原则

## 许可证

MIT许可证
