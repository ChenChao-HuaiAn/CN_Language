# CN_dynamic_array API 文档

## 概述

`CN_dynamic_array` 模块提供了动态数组数据结构的实现。动态数组是一种可以自动调整大小的数组，支持在末尾高效添加元素，当数组满时会自动扩容。

### 模块化架构

动态数组模块已重构为模块化结构，遵循单一职责原则：

1. **核心功能模块** (`CN_dynamic_array_core.h/.c`) - 创建、销毁、基本操作
2. **操作功能模块** (`CN_dynamic_array_operations.h/.c`) - 高级操作（查找、批量操作、迭代等）
3. **工具函数模块** (`CN_dynamic_array_utils.h/.c`) - 错误处理、验证、辅助函数
4. **主头文件** (`CN_dynamic_array.h`) - 统一接口和向后兼容
5. **接口实现** (`CN_dynamic_array_interface_impl.c`) - 抽象接口实现
6. **向后兼容包装** (`CN_dynamic_array.c`) - 确保现有代码继续工作

**版本：** 2.1.0  
**日期：** 2026-01-06  
**变更：** 模块化重构，将大型实现文件拆分为功能专注的小模块，符合单一职责原则

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

### Stru_DynamicArrayInterface_t

动态数组抽象接口，支持不同的实现和测试桩。

```c
typedef struct Stru_DynamicArrayInterface_t
{
    // 创建和销毁
    void* (*create)(size_t item_size, size_t initial_capacity, enum Eum_DynamicArrayError* error_code);
    void (*destroy)(void* array, enum Eum_DynamicArrayError* error_code);
    
    // 基本操作
    bool (*push)(void* array, const void* item, enum Eum_DynamicArrayError* error_code);
    void* (*get)(void* array, size_t index, enum Eum_DynamicArrayError* error_code);
    bool (*set)(void* array, size_t index, const void* item, enum Eum_DynamicArrayError* error_code);
    bool (*remove)(void* array, size_t index, enum Eum_DynamicArrayError* error_code);
    
    // 查询操作
    size_t (*length)(void* array, enum Eum_DynamicArrayError* error_code);
    size_t (*capacity)(void* array, enum Eum_DynamicArrayError* error_code);
    bool (*is_empty)(void* array, enum Eum_DynamicArrayError* error_code);
    
    // 数组操作
    bool (*clear)(void* array, enum Eum_DynamicArrayError* error_code);
    bool (*resize)(void* array, size_t new_capacity, enum Eum_DynamicArrayError* error_code);
    size_t (*find)(void* array, const void* item, F_DynamicArrayCompare_t compare, 
                   enum Eum_DynamicArrayError* error_code);
    bool (*push_batch)(void* array, const void* items, size_t count, 
                       enum Eum_DynamicArrayError* error_code);
    
    // 迭代操作
    bool (*foreach)(void* array, F_DynamicArrayIterator_t iterator, void* user_data,
                    enum Eum_DynamicArrayError* error_code);
    
    // 内存管理
    bool (*get_memory_stats)(void* array, size_t* total_bytes, size_t* used_bytes,
                             enum Eum_DynamicArrayError* error_code);
    bool (*shrink_to_fit)(void* array, enum Eum_DynamicArrayError* error_code);
} Stru_DynamicArrayInterface_t;
```

### Eum_DynamicArrayError

动态数组错误码枚举。

```c
enum Eum_DynamicArrayError
{
    Eum_DYNAMIC_ARRAY_SUCCESS = 0,          /**< 操作成功 */
    Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER,   /**< 空指针错误 */
    Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY,  /**< 内存不足错误 */
    Eum_DYNAMIC_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS, /**< 索引越界错误 */
    Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE,   /**< 无效大小错误 */
    Eum_DYNAMIC_ARRAY_ERROR_ITEM_NOT_FOUND, /**< 元素未找到错误 */
    Eum_DYNAMIC_ARRAY_ERROR_INTERNAL        /**< 内部错误 */
};
```

## 函数参考

### 基础API（向后兼容）

#### F_create_dynamic_array

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

#### F_destroy_dynamic_array

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

#### F_dynamic_array_push

```c
bool F_dynamic_array_push(Stru_DynamicArray_t* array, const void* item);
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

#### F_dynamic_array_get

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

#### F_dynamic_array_set

```c
bool F_dynamic_array_set(Stru_DynamicArray_t* array, size_t index, const void* item);
```

设置指定索引处的元素。

**参数：**
- `array`: 动态数组指针
- `index`: 元素索引
- `item`: 新元素指针

**返回值：**
- 成功：true
- 失败：false（参数无效或索引越界）

**示例：**
```c
Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
int value1 = 42;
int value2 = 100;
F_dynamic_array_push(array, &value1);
bool success = F_dynamic_array_set(array, 0, &value2);
```

#### F_dynamic_array_remove

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

#### F_dynamic_array_length

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

#### F_dynamic_array_capacity

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

#### F_dynamic_array_is_empty

```c
bool F_dynamic_array_is_empty(Stru_DynamicArray_t* array);
```

检查动态数组是否为空（长度为0）。

**参数：**
- `array`: 动态数组指针

**返回值：**
- 如果数组为空返回true，否则返回false

**示例：**
```c
Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
bool empty = F_dynamic_array_is_empty(array); // 应该返回true
```

#### F_dynamic_array_clear

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

#### F_dynamic_array_resize

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

#### F_dynamic_array_foreach

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

### 扩展API（带错误码）

#### F_create_dynamic_array_ex

```c
Stru_DynamicArray_t* F_create_dynamic_array_ex(size_t item_size, size_t initial_capacity, 
                                               enum Eum_DynamicArrayError* error_code);
```

创建动态数组（带错误码）。

**参数：**
- `item_size`: 每个元素的大小（字节）
- `initial_capacity`: 初始容量（0表示使用默认值）
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：指向新创建的动态数组的指针
- 失败：NULL（可通过error_code获取具体错误）

**错误码：**
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: item_size为0
- `Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY`: 内存分配失败
- `Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE`: 无效的大小参数

**示例：**
```c
enum Eum_DynamicArrayError error_code;
Stru_DynamicArray_t* array = F_create_dynamic_array_ex(sizeof(int), 10, &error_code);
if (array == NULL) {
    printf("创建失败，错误码: %d\n", error_code);
}
```

#### F_dynamic_array_push_ex

```c
bool F_dynamic_array_push_ex(Stru_DynamicArray_t* array, const void* item, 
                             enum Eum_DynamicArrayError* error_code);
```

向动态数组添加元素（带错误码）。

**参数：**
- `array`: 动态数组指针
- `item`: 要添加的元素指针
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：true
- 失败：false（可通过error_code获取具体错误）

**错误码：**
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: array或item为NULL
- `Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY`: 内存分配失败

**示例：**
```c
enum Eum_DynamicArrayError error_code;
int value = 42;
if (!F_dynamic_array_push_ex(array, &value, &error_code)) {
    printf("添加失败，错误码: %d\n", error_code);
}
```

#### F_dynamic_array_get_ex

```c
void* F_dynamic_array_get_ex(Stru_DynamicArray_t* array, size_t index, 
                             enum Eum_DynamicArrayError* error_code);
```

从动态数组获取元素（带错误码）。

**参数：**
- `array`: 动态数组指针
- `index`: 元素索引（0-based）
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：指向元素的指针
- 失败：NULL（可通过error_code获取具体错误）

**错误码：**
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: array为NULL
- `Eum_DYNAMIC_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS`: 索引越界

**示例：**
```c
enum Eum_DynamicArrayError error_code;
int* value = (int*)F_dynamic_array_get_ex(array, 0, &error_code);
if (value == NULL && error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
    printf("获取失败，错误码: %d\n", error_code);
}
```

#### F_dynamic_array_set_ex

```c
bool F_dynamic_array_set_ex(Stru_DynamicArray_t* array, size_t index, const void* item,
                            enum Eum_DynamicArrayError* error_code);
```

设置动态数组元素（带错误码）。

**参数：**
- `array`: 动态数组指针
- `index`: 元素索引
- `item`: 新元素指针
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：true
- 失败：false（可通过error_code获取具体错误）

**错误码：**
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: array或item为NULL
- `Eum_DYNAMIC_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS`: 索引越界

**示例：**
```c
enum Eum_DynamicArrayError error_code;
int new_value = 100;
if (!F_dynamic_array_set_ex(array, 0, &new_value, &error_code)) {
    printf("设置失败，错误码: %d\n", error_code);
}
```

#### F_dynamic_array_remove_ex

```c
bool F_dynamic_array_remove_ex(Stru_DynamicArray_t* array, size_t index,
                               enum Eum_DynamicArrayError* error_code);
```

从动态数组移除元素（带错误码）。

**参数：**
- `array`: 动态数组指针
- `index`: 要移除的元素索引
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：true
- 失败：false（可通过error_code获取具体错误）

**错误码：**
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: array为NULL
- `Eum_DYNAMIC_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS`: 索引越界

**示例：**
```c
enum Eum_DynamicArrayError error_code;
if (!F_dynamic_array_remove_ex(array, 2, &error_code)) {
    printf("移除失败，错误码: %d\n", error_code);
}
```

#### F_dynamic_array_length_ex

```c
size_t F_dynamic_array_length_ex(Stru_DynamicArray_t* array, 
                                 enum Eum_DynamicArrayError* error_code);
```

获取动态数组长度（带错误码）。

**参数：**
- `array`: 动态数组指针
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：数组长度
- 失败：0（可通过error_code获取具体错误）

**错误码：**
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: array为NULL

**示例：**
```c
enum Eum_DynamicArrayError error_code;
size_t len = F_dynamic_array_length_ex(array, &error_code);
if (error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
    printf("获取长度失败，错误码: %d\n", error_code);
}
```

#### F_dynamic_array_capacity_ex

```c
size_t F_dynamic_array_capacity_ex(Stru_DynamicArray_t* array,
                                   enum Eum_DynamicArrayError* error_code);
```

获取动态数组容量（带错误码）。

**参数：**
- `array`: 动态数组指针
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：数组容量
- 失败：0（可通过error_code获取具体错误）

**错误码：**
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: array为NULL

**示例：**
```c
enum Eum_DynamicArrayError error_code;
size_t cap = F_dynamic_array_capacity_ex(array, &error_code);
if (error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
    printf("获取容量失败，错误码: %d\n", error_code);
}
```

#### F_dynamic_array_is_empty_ex

```c
bool F_dynamic_array_is_empty_ex(Stru_DynamicArray_t* array,
                                 enum Eum_DynamicArrayError* error_code);
```

检查动态数组是否为空（带错误码）。

**参数：**
- `array`: 动态数组指针
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：true（数组为空）或false（数组不为空）
- 失败：false（可通过error_code获取具体错误）

**错误码：**
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: array为NULL

**示例：**
```c
enum Eum_DynamicArrayError error_code;
bool empty = F_dynamic_array_is_empty_ex(array, &error_code);
if (error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
    printf("检查是否为空失败，错误码: %d\n", error_code);
}
```

#### F_dynamic_array_clear_ex

```c
bool F_dynamic_array_clear_ex(Stru_DynamicArray_t* array,
                              enum Eum_DynamicArrayError* error_code);
```

清空动态数组（带错误码）。

**参数：**
- `array`: 动态数组指针
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：true
- 失败：false（可通过error_code获取具体错误）

**错误码：**
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: array为NULL

**示例：**
```c
enum Eum_DynamicArrayError error_code;
if (!F_dynamic_array_clear_ex(array, &error_code)) {
    printf("清空数组失败，错误码: %d\n", error_code);
}
```

#### F_dynamic_array_resize_ex

```c
bool F_dynamic_array_resize_ex(Stru_DynamicArray_t* array, size_t new_capacity,
                               enum Eum_DynamicArrayError* error_code);
```

调整动态数组容量（带错误码）。

**参数：**
- `array`: 动态数组指针
- `new_capacity`: 新的容量大小
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：true
- 失败：false（可通过error_code获取具体错误）

**错误码：**
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: array为NULL
- `Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY`: 内存分配失败
- `Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE`: 无效的容量大小

**示例：**
```c
enum Eum_DynamicArrayError error_code;
if (!F_dynamic_array_resize_ex(array, 100, &error_code)) {
    printf("调整容量失败，错误码: %d\n", error_code);
}
```

#### F_dynamic_array_find

```c
size_t F_dynamic_array_find(Stru_DynamicArray_t* array, const void* item,
                            F_DynamicArrayCompare_t compare);
```

查找动态数组元素。

**参数：**
- `array`: 动态数组指针
- `item`: 要查找的元素指针
- `compare`: 比较函数（NULL表示直接比较内存）

**返回值：**
- 成功：元素索引
- 未找到：SIZE_MAX

**注意：**
- 比较函数应返回：<0表示item1<item2，=0表示相等，>0表示item1>item2

**示例：**
```c
int compare_int(const void* a, const void* b) {
    int int_a = *(const int*)a;
    int int_b = *(const int*)b;
    return int_a - int_b;
}

int target = 42;
size_t index = F_dynamic_array_find(array, &target, compare_int);
if (index != SIZE_MAX) {
    printf("找到元素，索引: %zu\n", index);
}
```

#### F_dynamic_array_find_ex

```c
size_t F_dynamic_array_find_ex(Stru_DynamicArray_t* array, const void* item,
                               F_DynamicArrayCompare_t compare,
                               enum Eum_DynamicArrayError* error_code);
```

查找动态数组元素（带错误码）。

**参数：**
- `array`: 动态数组指针
- `item`: 要查找的元素指针
- `compare`: 比较函数（NULL表示直接比较内存）
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：元素索引
- 未找到：SIZE_MAX（error_code为Eum_DYNAMIC_ARRAY_ERROR_ITEM_NOT_FOUND）

**错误码：**
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: array或item为NULL
- `Eum_DYNAMIC_ARRAY_ERROR_ITEM_NOT_FOUND`: 元素未找到

**示例：**
```c
enum Eum_DynamicArrayError error_code;
int target = 42;
size_t index = F_dynamic_array_find_ex(array, &target, compare_int, &error_code);
if (error_code == Eum_DYNAMIC_ARRAY_ERROR_ITEM_NOT_FOUND) {
    printf("元素未找到\n");
} else if (error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
    printf("查找失败，错误码: %d\n", error_code);
}
```

#### F_dynamic_array_push_batch

```c
bool F_dynamic_array_push_batch(Stru_DynamicArray_t* array, const void* items, size_t count);
```

批量添加元素到动态数组。

**参数：**
- `array`: 动态数组指针
- `items`: 元素数组指针
- `count`: 元素数量

**返回值：**
- 成功：true
- 失败：false（参数无效或内存分配失败）

**注意：**
- 批量添加比逐个添加更高效
- 会一次性分配足够的内存

**示例：**
```c
int values[] = {1, 2, 3, 4, 5};
bool success = F_dynamic_array_push_batch(array, values, 5);
```

#### F_dynamic_array_push_batch_ex

```c
bool F_dynamic_array_push_batch_ex(Stru_DynamicArray_t* array, const void* items, size_t count,
                                   enum Eum_DynamicArrayError* error_code);
```

批量添加元素到动态数组（带错误码）。

**参数：**
- `array`: 动态数组指针
- `items`: 元素数组指针
- `count`: 元素数量
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：true
- 失败：false（可通过error_code获取具体错误）

**错误码：**
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: array或items为NULL
- `Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY`: 内存分配失败
- `Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE`: count为0

**示例：**
```c
enum Eum_DynamicArrayError error_code;
int values[] = {1, 2, 3, 4, 5};
if (!F_dynamic_array_push_batch_ex(array, values, 5, &error_code)) {
    printf("批量添加失败，错误码: %d\n", error_code);
}
```

#### F_dynamic_array_foreach_ex

```c
bool F_dynamic_array_foreach_ex(Stru_DynamicArray_t* array, 
                               F_DynamicArrayIterator_t iterator, void* user_data,
                               enum Eum_DynamicArrayError* error_code);
```

遍历动态数组（带索引和错误码）。

**参数：**
- `array`: 动态数组指针
- `iterator`: 迭代器回调函数
- `user_data`: 用户数据
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：true（所有元素都处理完成）
- 失败：false（可通过error_code获取具体错误，或迭代器返回false停止）

**注意：**
- 迭代器回调函数接收元素指针、索引和用户数据
- 迭代器返回true继续迭代，返回false停止迭代

**示例：**
```c
bool print_with_index(void* item, size_t index, void* user_data) {
    int* value = (int*)item;
    printf("[%zu] = %d\n", index, *value);
    return true; // 继续迭代
}

enum Eum_DynamicArrayError error_code;
if (!F_dynamic_array_foreach_ex(array, print_with_index, NULL, &error_code)) {
    printf("遍历失败，错误码: %d\n", error_code);
}
```

#### F_dynamic_array_get_memory_stats

```c
bool F_dynamic_array_get_memory_stats(Stru_DynamicArray_t* array, 
                                      size_t* total_bytes, size_t* used_bytes);
```

获取动态数组内存使用统计。

**参数：**
- `array`: 动态数组指针
- `total_bytes`: 输出参数，总字节数（可为NULL）
- `used_bytes`: 输出参数，已使用字节数（可为NULL）

**返回值：**
- 成功：true
- 失败：false（参数无效）

**注意：**
- 总字节数包括数组结构体、指针数组和元素数据
- 已使用字节数包括实际存储的元素数据

**示例：**
```c
size_t total, used;
if (F_dynamic_array_get_memory_stats(array, &total, &used)) {
    printf("总内存: %zu 字节，已使用: %zu 字节\n", total, used);
}
```

#### F_dynamic_array_get_memory_stats_ex

```c
bool F_dynamic_array_get_memory_stats_ex(Stru_DynamicArray_t* array, 
                                         size_t* total_bytes, size_t* used_bytes,
                                         enum Eum_DynamicArrayError* error_code);
```

获取动态数组内存使用统计（带错误码）。

**参数：**
- `array`: 动态数组指针
- `total_bytes`: 输出参数，总字节数（可为NULL）
- `used_bytes`: 输出参数，已使用字节数（可为NULL）
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：true
- 失败：false（可通过error_code获取具体错误）

**错误码：**
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: array为NULL

**示例：**
```c
enum Eum_DynamicArrayError error_code;
size_t total, used;
if (!F_dynamic_array_get_memory_stats_ex(array, &total, &used, &error_code)) {
    printf("获取内存统计失败，错误码: %d\n", error_code);
}
```

#### F_dynamic_array_shrink_to_fit

```c
bool F_dynamic_array_shrink_to_fit(Stru_DynamicArray_t* array);
```

压缩动态数组（释放未使用的内存）。

**参数：**
- `array`: 动态数组指针

**返回值：**
- 成功：true
- 失败：false（参数无效或内存分配失败）

**注意：**
- 压缩后容量等于长度
- 可以释放未使用的内存

**示例：**
```c
if (F_dynamic_array_shrink_to_fit(array)) {
    printf("压缩成功\n");
}
```

#### F_dynamic_array_shrink_to_fit_ex

```c
bool F_dynamic_array_shrink_to_fit_ex(Stru_DynamicArray_t* array,
                                      enum Eum_DynamicArrayError* error_code);
```

压缩动态数组（带错误码）。

**参数：**
- `array`: 动态数组指针
- `error_code`: 输出参数，错误码（可为NULL）

**返回值：**
- 成功：true
- 失败：false（可通过error_code获取具体错误）

**错误码：**
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: array为NULL
- `Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY`: 内存分配失败

**示例：**
```c
enum Eum_DynamicArrayError error_code;
if (!F_dynamic_array_shrink_to_fit_ex(array, &error_code)) {
    printf("压缩失败，错误码: %d\n", error_code);
}
```

#### F_get_default_dynamic_array_interface

```c
const Stru_DynamicArrayInterface_t* F_get_default_dynamic_array_interface(void);
```

获取默认动态数组接口。

**返回值：**
- 指向默认动态数组接口的指针

**注意：**
- 接口提供了所有动态数组操作的抽象
- 支持依赖注入和测试桩

**示例：**
```c
const Stru_DynamicArrayInterface_t* interface = F_get_default_dynamic_array_interface();
if (interface != NULL) {
    enum Eum_DynamicArrayError error_code;
    void* array = interface->create(sizeof(int), 10, &error_code);
    // 使用接口操作数组
    interface->destroy(array, &error_code);
}
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

动态数组模块提供了两种错误处理机制：

### 基础错误处理
所有基础API函数都包含错误检查，无效参数会返回适当的错误值：
- `NULL`: 用于返回指针的函数
- `false`: 用于返回布尔值的函数
- `0`: 用于返回大小的函数

### 扩展错误处理（推荐）
扩展API函数（带_ex后缀）提供详细的错误码机制：
- 使用`enum Eum_DynamicArrayError`枚举返回具体错误类型
- 支持错误码输出参数，可获取详细错误信息
- 包含更全面的错误检查，如空指针、无效大小、内部错误等

### 错误码说明
- `Eum_DYNAMIC_ARRAY_SUCCESS`: 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER`: 空指针错误
- `Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY`: 内存不足错误
- `Eum_DYNAMIC_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS`: 索引越界错误
- `Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE`: 无效大小错误
- `Eum_DYNAMIC_ARRAY_ERROR_ITEM_NOT_FOUND`: 元素未找到错误
- `Eum_DYNAMIC_ARRAY_ERROR_INTERNAL`: 内部错误

### 错误处理示例
```c
#include "CN_dynamic_array.h"
#include <stdio.h>

int main() {
    enum Eum_DynamicArrayError error_code;
    
    // 使用扩展API获取详细错误信息
    Stru_DynamicArray_t* array = F_create_dynamic_array_ex(sizeof(int), 10, &error_code);
    if (array == NULL) {
        printf("创建数组失败，错误码: %d\n", error_code);
        return 1;
    }
    
    // 添加元素并检查错误
    int value = 42;
    if (!F_dynamic_array_push_ex(array, &value, &error_code)) {
        printf("添加元素失败，错误码: %d\n", error_code);
    }
    
    F_destroy_dynamic_array(array);
    return 0;
}
```

## 内存管理

动态数组模块管理以下内存：
1. 动态数组结构体本身
2. 元素指针数组
3. 每个元素的实际数据

用户只需要：
1. 调用`F_create_dynamic_array`或`F_create_dynamic_array_ex`创建数组
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

### 使用抽象接口

```c
#include "infrastructure/containers/array/CN_dynamic_array.h"
#include <stdio.h>

int main() {
    // 获取默认接口
    const Stru_DynamicArrayInterface_t* interface = F_get_default_dynamic_array_interface();
    if (interface == NULL) {
        printf("获取接口失败\n");
        return 1;
    }
    
    // 使用接口创建数组
    enum Eum_DynamicArrayError error_code;
    void* array = interface->create(sizeof(int), 10, &error_code);
    if (array == NULL) {
        printf("创建数组失败，错误码: %d\n", error_code);
        return 1;
    }
    
    // 使用接口添加元素
    int value = 42;
    if (!interface->push(array, &value, &error_code)) {
        printf("添加元素失败，错误码: %d\n", error_code);
    }
    
    // 使用接口获取元素
    int* retrieved = (int*)interface->get(array, 0, &error_code);
    if (retrieved != NULL) {
        printf("获取到的值: %d\n", *retrieved);
    }
    
    // 使用接口销毁数组
    interface->destroy(array, &error_code);
    
    return 0;
}
```

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本动态数组功能 |
| 2.0.0 | 2026-01-06 | 重构版本，添加抽象接口、扩展API和错误处理机制 |
| 2.1.0 | 2026-01-06 | 模块化重构版本，将大型实现文件拆分为功能专注的小模块，符合单一职责原则 |

## 相关文档

- [动态数组模块README](../src/infrastructure/containers/array/README.md)
- [CN_Language项目架构文档](../../../../architecture/001-中文编程语言CN_Language开发规划.md)
