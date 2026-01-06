# 动态数组模块 (Dynamic Array Module)

## 概述

动态数组模块提供了一种可以自动调整大小的数组数据结构。它支持在数组末尾高效地添加和删除元素，当数组满时会自动扩容。

## 特性

- **自动扩容**：当数组满时自动扩容为原来的2倍
- **随机访问**：支持O(1)时间的随机访问
- **动态大小**：支持动态调整数组大小
- **类型安全**：通过元素大小参数确保类型安全
- **内存管理**：自动管理内存分配和释放

## 数据结构

### Stru_DynamicArray_t

动态数组的主要结构体：

```c
typedef struct Stru_DynamicArray_t
{
    void** items;           // 元素指针数组
    size_t capacity;        // 当前分配的容量
    size_t length;          // 当前元素数量
    size_t item_size;       // 每个元素的大小（字节）
} Stru_DynamicArray_t;
```

### Stru_DynamicArrayInterface_t

动态数组抽象接口，支持不同的实现和测试桩：

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

动态数组错误码枚举：

```c
enum Eum_DynamicArrayError
{
    Eum_DYNAMIC_ARRAY_SUCCESS = 0,          // 操作成功
    Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER,   // 空指针错误
    Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY,  // 内存不足错误
    Eum_DYNAMIC_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS, // 索引越界错误
    Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE,   // 无效大小错误
    Eum_DYNAMIC_ARRAY_ERROR_ITEM_NOT_FOUND, // 元素未找到错误
    Eum_DYNAMIC_ARRAY_ERROR_INTERNAL        // 内部错误
};
```

## API 接口

### 基础API（向后兼容）

#### 创建和销毁
- `F_create_dynamic_array(size_t item_size)` - 创建动态数组
- `F_destroy_dynamic_array(Stru_DynamicArray_t* array)` - 销毁动态数组

#### 元素操作
- `F_dynamic_array_push(Stru_DynamicArray_t* array, const void* item)` - 添加元素
- `F_dynamic_array_get(Stru_DynamicArray_t* array, size_t index)` - 获取元素
- `F_dynamic_array_set(Stru_DynamicArray_t* array, size_t index, const void* item)` - 设置元素
- `F_dynamic_array_remove(Stru_DynamicArray_t* array, size_t index)` - 移除元素

#### 查询操作
- `F_dynamic_array_length(Stru_DynamicArray_t* array)` - 获取数组长度
- `F_dynamic_array_capacity(Stru_DynamicArray_t* array)` - 获取数组容量
- `F_dynamic_array_is_empty(Stru_DynamicArray_t* array)` - 检查数组是否为空

#### 数组操作
- `F_dynamic_array_clear(Stru_DynamicArray_t* array)` - 清空数组
- `F_dynamic_array_resize(Stru_DynamicArray_t* array, size_t new_capacity)` - 调整数组容量
- `F_dynamic_array_foreach(Stru_DynamicArray_t* array, void (*callback)(void* item, void* user_data), void* user_data)` - 遍历数组

### 扩展API（带错误码）

#### 创建和销毁
- `F_create_dynamic_array_ex(size_t item_size, size_t initial_capacity, enum Eum_DynamicArrayError* error_code)` - 创建动态数组（带错误码）
- `F_destroy_dynamic_array` 函数已支持错误处理

#### 元素操作
- `F_dynamic_array_push_ex(Stru_DynamicArray_t* array, const void* item, enum Eum_DynamicArrayError* error_code)` - 添加元素（带错误码）
- `F_dynamic_array_get_ex(Stru_DynamicArray_t* array, size_t index, enum Eum_DynamicArrayError* error_code)` - 获取元素（带错误码）
- `F_dynamic_array_set_ex(Stru_DynamicArray_t* array, size_t index, const void* item, enum Eum_DynamicArrayError* error_code)` - 设置元素（带错误码）
- `F_dynamic_array_remove_ex(Stru_DynamicArray_t* array, size_t index, enum Eum_DynamicArrayError* error_code)` - 移除元素（带错误码）

#### 查询操作
- `F_dynamic_array_length_ex(Stru_DynamicArray_t* array, enum Eum_DynamicArrayError* error_code)` - 获取数组长度（带错误码）
- `F_dynamic_array_capacity_ex(Stru_DynamicArray_t* array, enum Eum_DynamicArrayError* error_code)` - 获取数组容量（带错误码）
- `F_dynamic_array_is_empty_ex(Stru_DynamicArray_t* array, enum Eum_DynamicArrayError* error_code)` - 检查数组是否为空（带错误码）

#### 数组操作
- `F_dynamic_array_clear_ex(Stru_DynamicArray_t* array, enum Eum_DynamicArrayError* error_code)` - 清空数组（带错误码）
- `F_dynamic_array_resize_ex(Stru_DynamicArray_t* array, size_t new_capacity, enum Eum_DynamicArrayError* error_code)` - 调整数组容量（带错误码）
- `F_dynamic_array_find(Stru_DynamicArray_t* array, const void* item, F_DynamicArrayCompare_t compare)` - 查找元素
- `F_dynamic_array_find_ex(Stru_DynamicArray_t* array, const void* item, F_DynamicArrayCompare_t compare, enum Eum_DynamicArrayError* error_code)` - 查找元素（带错误码）
- `F_dynamic_array_push_batch(Stru_DynamicArray_t* array, const void* items, size_t count)` - 批量添加元素
- `F_dynamic_array_push_batch_ex(Stru_DynamicArray_t* array, const void* items, size_t count, enum Eum_DynamicArrayError* error_code)` - 批量添加元素（带错误码）
- `F_dynamic_array_foreach_ex(Stru_DynamicArray_t* array, F_DynamicArrayIterator_t iterator, void* user_data, enum Eum_DynamicArrayError* error_code)` - 遍历数组（带索引和错误码）

#### 内存管理
- `F_dynamic_array_get_memory_stats(Stru_DynamicArray_t* array, size_t* total_bytes, size_t* used_bytes)` - 获取内存使用统计
- `F_dynamic_array_get_memory_stats_ex(Stru_DynamicArray_t* array, size_t* total_bytes, size_t* used_bytes, enum Eum_DynamicArrayError* error_code)` - 获取内存使用统计（带错误码）
- `F_dynamic_array_shrink_to_fit(Stru_DynamicArray_t* array)` - 压缩数组（释放未使用的内存）
- `F_dynamic_array_shrink_to_fit_ex(Stru_DynamicArray_t* array, enum Eum_DynamicArrayError* error_code)` - 压缩数组（带错误码）

### 抽象接口API

- `F_get_default_dynamic_array_interface(void)` - 获取默认动态数组接口
- `Stru_DynamicArrayInterface_t` - 动态数组抽象接口结构体

## 使用示例

```c
#include "CN_dynamic_array.h"
#include <stdio.h>

// 定义示例结构体
typedef struct {
    int id;
    char name[32];
} Person;

// 打印回调函数
void print_person(void* item, void* user_data) {
    Person* p = (Person*)item;
    printf("ID: %d, Name: %s\n", p->id, p->name);
}

int main() {
    // 创建动态数组
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(Person));
    if (array == NULL) {
        printf("创建动态数组失败\n");
        return 1;
    }
    
    // 添加元素
    Person p1 = {1, "张三"};
    Person p2 = {2, "李四"};
    Person p3 = {3, "王五"};
    
    F_dynamic_array_push(array, &p1);
    F_dynamic_array_push(array, &p2);
    F_dynamic_array_push(array, &p3);
    
    // 获取元素
    Person* retrieved = (Person*)F_dynamic_array_get(array, 1);
    if (retrieved != NULL) {
        printf("获取到的元素: ID=%d, Name=%s\n", retrieved->id, retrieved->name);
    }
    
    // 遍历数组
    printf("所有人员:\n");
    F_dynamic_array_foreach(array, print_person, NULL);
    
    // 获取数组信息
    printf("数组长度: %zu\n", F_dynamic_array_length(array));
    printf("数组容量: %zu\n", F_dynamic_array_capacity(array));
    
    // 销毁数组
    F_destroy_dynamic_array(array);
    
    return 0;
}
```

## 性能特征

- **时间复杂度**：
  - 访问：O(1)
  - 插入（末尾）：平均O(1)，最坏O(n)（需要扩容）
  - 删除：O(n)（需要移动元素）
  - 搜索：O(n)

- **空间复杂度**：O(n)

## 内存管理

动态数组模块负责管理以下内存：
1. 动态数组结构体本身
2. 元素指针数组
3. 每个元素的实际数据

用户只需要提供要存储的数据，模块会自动处理内存分配和释放。

## 错误处理

动态数组模块提供了两种错误处理机制：

### 基础错误处理
所有基础API函数都包含错误检查：
- 无效参数返回适当错误值（NULL、false、0等）
- 内存分配失败返回错误
- 索引越界返回错误

### 扩展错误处理（推荐）
扩展API函数（带_ex后缀）提供详细的错误码机制：
- 使用`enum Eum_DynamicArrayError`枚举返回具体错误类型
- 支持错误码输出参数，可获取详细错误信息
- 包含更全面的错误检查，如空指针、无效大小、内部错误等

### 错误码说明
- `Eum_DYNAMIC_ARRAY_SUCCESS` - 操作成功
- `Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER` - 空指针错误
- `Eum_DYNAMIC_ARRAY_ERROR_OUT_OF_MEMORY` - 内存不足错误
- `Eum_DYNAMIC_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS` - 索引越界错误
- `Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE` - 无效大小错误
- `Eum_DYNAMIC_ARRAY_ERROR_ITEM_NOT_FOUND` - 元素未找到错误
- `Eum_DYNAMIC_ARRAY_ERROR_INTERNAL` - 内部错误

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

## 线程安全

当前实现不是线程安全的。如果需要在多线程环境中使用，需要外部同步机制。

## 配置参数

可以通过修改以下宏定义调整动态数组的行为：

```c
#define CN_DYNAMIC_ARRAY_INITIAL_CAPACITY 16    // 初始容量
#define CN_DYNAMIC_ARRAY_GROWTH_FACTOR 2        // 扩容因子
```

## 依赖关系

### 编译时依赖
- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`
- 项目内部文件：
  - `CN_dynamic_array_interface.h` - 抽象接口定义
  - `CN_dynamic_array.h` - 主头文件

### 架构依赖
- 基础设施层：本模块属于基础设施层
- 依赖规则：只能依赖C标准库和操作系统API，不能依赖项目其他层
- 被依赖：可被核心层和应用层依赖

### 接口依赖
- 抽象接口模式：通过`Stru_DynamicArrayInterface_t`提供抽象接口
- 依赖注入：支持通过接口指针进行依赖注入
- 测试桩：接口设计支持测试桩实现

## 测试

模块包含完整的单元测试，覆盖所有API接口和边界条件。测试覆盖率目标：
- 语句覆盖率：≥90%
- 分支覆盖率：≥80%
- 函数覆盖率：100%

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本动态数组功能 |
| 2.0.0 | 2026-01-06 | 重构版本，添加抽象接口、扩展API和错误处理机制 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
