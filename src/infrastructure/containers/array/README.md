# 动态数组模块 (Dynamic Array Module)

## 概述

动态数组模块提供了一种可以自动调整大小的数组数据结构。它支持在数组末尾高效地添加和删除元素，当数组满时会自动扩容。

### 模块化重构 (版本 2.0.0+)

动态数组模块已重构为模块化结构，遵循单一职责原则和项目架构规范：

1. **核心功能模块** (`CN_dynamic_array_core.h/.c`) - 创建、销毁、基本操作
2. **操作功能模块** (`CN_dynamic_array_operations.h/.c`) - 高级操作（查找、批量操作、迭代等）
3. **工具函数模块** (`CN_dynamic_array_utils.h/.c`) - 错误处理、验证、辅助函数
4. **主头文件** (`CN_dynamic_array.h`) - 统一接口和向后兼容
5. **接口实现** (`CN_dynamic_array_interface_impl.c`) - 抽象接口实现
6. **向后兼容包装** (`CN_dynamic_array.c`) - 确保现有代码继续工作

这种模块化设计确保：
- 每个.c文件不超过500行（符合单一职责原则）
- 每个函数不超过50行
- 模块间依赖清晰，符合分层架构
- 易于测试和维护

## 特性

- **自动扩容**：当数组满时自动扩容为原来的2倍
- **随机访问**：支持O(1)时间的随机访问
- **动态大小**：支持动态调整数组大小
- **类型安全**：通过元素大小参数确保类型安全
- **内存管理**：自动管理内存分配和释放

## 文件结构

```
src/infrastructure/containers/array/
├── CN_dynamic_array.h              # 主头文件（统一接口）
├── CN_dynamic_array.c              # 向后兼容包装
├── CN_dynamic_array_interface.h    # 抽象接口定义
├── CN_dynamic_array_interface_impl.c # 接口实现
├── CN_dynamic_array_core.h         # 核心功能头文件
├── CN_dynamic_array_core.c         # 核心功能实现
├── CN_dynamic_array_operations.h   # 操作功能头文件
├── CN_dynamic_array_operations.c   # 操作功能实现
├── CN_dynamic_array_utils.h        # 工具函数头文件
├── CN_dynamic_array_utils.c        # 工具函数实现
├── README.md                       # 模块文档
└── ... (测试文件等)
```

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

### 高级操作API（新增功能）

#### 排序操作
- `F_dynamic_array_sort(Stru_DynamicArray_t* array, F_DynamicArrayCompare_t compare)` - 排序数组
- `F_dynamic_array_sort_ex(Stru_DynamicArray_t* array, F_DynamicArrayCompare_t compare, enum Eum_DynamicArrayError* error_code)` - 排序数组（带错误码）

#### 反转操作
- `F_dynamic_array_reverse(Stru_DynamicArray_t* array)` - 反转数组
- `F_dynamic_array_reverse_ex(Stru_DynamicArray_t* array, enum Eum_DynamicArrayError* error_code)` - 反转数组（带错误码）

#### 映射操作
- `F_dynamic_array_map(Stru_DynamicArray_t* array, void (*mapper)(void* item, void* user_data), void* user_data)` - 映射数组（对每个元素应用函数）
- `F_dynamic_array_map_ex(Stru_DynamicArray_t* array, void (*mapper)(void* item, void* user_data), void* user_data, enum Eum_DynamicArrayError* error_code)` - 映射数组（带错误码）

#### 过滤操作
- `F_dynamic_array_filter(Stru_DynamicArray_t* array, bool (*filter)(void* item, void* user_data), void* user_data)` - 过滤数组（根据条件保留元素）
- `F_dynamic_array_filter_ex(Stru_DynamicArray_t* array, bool (*filter)(void* item, void* user_data), void* user_data, enum Eum_DynamicArrayError* error_code)` - 过滤数组（带错误码）

#### 切片操作
- `F_dynamic_array_slice(Stru_DynamicArray_t* array, size_t start, size_t end)` - 获取子数组（切片）
- `F_dynamic_array_slice_ex(Stru_DynamicArray_t* array, size_t start, size_t end, enum Eum_DynamicArrayError* error_code)` - 获取子数组（带错误码）

### 抽象接口API

- `F_get_default_dynamic_array_interface(void)` - 获取默认动态数组接口
- `Stru_DynamicArrayInterface_t` - 动态数组抽象接口结构体（包含所有高级操作）

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

## 模块依赖关系

### 编译时依赖
- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`, `limits.h`
- 项目内部文件：
  - `CN_dynamic_array_interface.h` - 抽象接口定义
  - `CN_dynamic_array.h` - 主头文件（包含所有子模块）
  - `CN_dynamic_array_core.h` - 核心功能
  - `CN_dynamic_array_operations.h` - 操作功能
  - `CN_dynamic_array_utils.h` - 工具函数

### 模块间依赖关系
```
CN_dynamic_array.c (向后兼容包装)
    ├── CN_dynamic_array.h
    │   ├── CN_dynamic_array_core.h
    │   ├── CN_dynamic_array_operations.h
    │   └── CN_dynamic_array_utils.h
    └── CN_dynamic_array_interface.h

CN_dynamic_array_interface_impl.c (接口实现)
    ├── CN_dynamic_array.h
    └── 调用各模块的_ex函数

各功能模块独立编译，通过主头文件统一暴露接口
```

### 架构依赖
- **基础设施层**：本模块属于基础设施层
- **依赖规则**：只能依赖C标准库和操作系统API，不能依赖项目其他层
- **被依赖**：可被核心层和应用层依赖
- **模块化原则**：遵循单一职责原则，每个模块功能专注

### 接口依赖
- **抽象接口模式**：通过`Stru_DynamicArrayInterface_t`提供抽象接口
- **依赖注入**：支持通过接口指针进行依赖注入
- **测试桩**：接口设计支持测试桩实现
- **向后兼容**：所有现有API保持兼容，内部调用模块化实现

## 测试

模块包含完整的模块化单元测试，覆盖所有API接口、错误处理、边界条件和性能特征。测试文件位于 `tests/infrastructure/containers/array/` 目录。

### 模块化测试架构

动态数组测试已重构为模块化结构，遵循单一职责原则：

1. **测试公共模块** (`test_dynamic_array_common.h/.c`) - 共享的测试辅助函数和工具
2. **独立测试模块** - 每个测试功能在单独的文件中实现
3. **测试运行器** (`test_runner_unit.c`) - 单元测试运行器
4. **测试声明头文件** (`test_dynamic_array_all.h`) - 所有测试函数声明
5. **Makefile构建系统** - 自动化编译和运行测试

### 测试文件结构

```
tests/infrastructure/containers/array/
├── unit/                              # 单元测试目录
│   ├── test_dynamic_array_common.h    # 测试公共头文件
│   ├── test_dynamic_array_common.c    # 测试公共实现
│   ├── test_dynamic_array_create_destroy.c      # 创建和销毁测试
│   ├── test_dynamic_array_basic_operations.c    # 基本操作测试
│   ├── test_dynamic_array_query_operations.c    # 查询操作测试
│   ├── test_dynamic_array_array_operations.c    # 数组操作测试
│   ├── test_dynamic_array_iteration_operations.c # 迭代操作测试
│   ├── test_dynamic_array_memory_management.c   # 内存管理测试
│   ├── test_dynamic_array_error_handling.c      # 错误处理测试
│   ├── test_dynamic_array_abstract_interface.c  # 抽象接口测试
│   ├── test_dynamic_array_edge_cases.c          # 边界情况测试
│   ├── test_dynamic_array_performance.c         # 性能测试
│   ├── test_dynamic_array_all.h                 # 所有测试函数声明
│   └── test_runner_unit.c                       # 单元测试运行器
├── Makefile                           # 测试构建脚本
└── test_dynamic_array.c               # 向后兼容的旧测试文件
```

### 测试覆盖范围

测试套件包含10个独立的测试模块，每个模块专注于单一测试功能：

1. **test_dynamic_array_create_destroy** - 测试创建和销毁功能
2. **test_dynamic_array_basic_operations** - 测试基本操作功能（push、get、set、remove）
3. **test_dynamic_array_query_operations** - 测试查询操作功能（length、capacity、is_empty）
4. **test_dynamic_array_array_operations** - 测试数组操作功能（clear、resize、find、push_batch）
5. **test_dynamic_array_iteration_operations** - 测试迭代操作功能（foreach）
6. **test_dynamic_array_memory_management** - 测试内存管理功能（get_memory_stats、shrink_to_fit）
7. **test_dynamic_array_error_handling** - 测试错误处理功能（所有_ex函数）
8. **test_dynamic_array_abstract_interface** - 测试抽象接口功能
9. **test_dynamic_array_edge_cases** - 测试边界情况（超大容量、零大小元素、重复添加移除等）
10. **test_dynamic_array_performance** - 测试性能特征

### 测试辅助函数

测试公共模块包含以下辅助函数：
- `compare_int` - 整数比较函数
- `print_int_iterator` - 带索引的整数打印迭代器
- `print_int_callback` - 整数打印回调函数
- `stop_at_third` - 在第三个元素停止的迭代器
- `run_all_dynamic_array_tests` - 运行所有测试并统计结果

### 测试覆盖率目标
- 语句覆盖率：≥90%
- 分支覆盖率：≥80%
- 函数覆盖率：100%

### 运行测试

#### 使用Makefile编译和运行

```bash
# 进入测试目录
cd tests/infrastructure/containers/array

# 编译并运行测试
make test

# 或者单独编译
make build

# 清理编译文件
make clean
```

#### 手动编译和运行

```bash
# 编译测试
gcc -o test_runner_unit tests/infrastructure/containers/array/unit/test_runner_unit.c \
    tests/infrastructure/containers/array/unit/test_dynamic_array_common.c \
    tests/infrastructure/containers/array/unit/test_dynamic_array_create_destroy.c \
    tests/infrastructure/containers/array/unit/test_dynamic_array_basic_operations.c \
    tests/infrastructure/containers/array/unit/test_dynamic_array_query_operations.c \
    tests/infrastructure/containers/array/unit/test_dynamic_array_array_operations.c \
    tests/infrastructure/containers/array/unit/test_dynamic_array_iteration_operations.c \
    tests/infrastructure/containers/array/unit/test_dynamic_array_memory_management.c \
    tests/infrastructure/containers/array/unit/test_dynamic_array_error_handling.c \
    tests/infrastructure/containers/array/unit/test_dynamic_array_abstract_interface.c \
    tests/infrastructure/containers/array/unit/test_dynamic_array_edge_cases.c \
    tests/infrastructure/containers/array/unit/test_dynamic_array_performance.c \
    src/infrastructure/containers/array/CN_dynamic_array.c \
    src/infrastructure/containers/array/CN_dynamic_array_core.c \
    src/infrastructure/containers/array/CN_dynamic_array_operations.c \
    src/infrastructure/containers/array/CN_dynamic_array_utils.c \
    src/infrastructure/containers/array/CN_dynamic_array_interface_impl.c \
    -I. -I./src -I./tests -std=c99 -Wall -Wextra

# 运行测试
./test_runner_unit
```

### 测试输出示例

```
=== 动态数组模块测试套件 ===

[1/10] test_dynamic_array_create_and_destroy... 通过
[2/10] test_dynamic_array_basic_operations... 通过
[3/10] test_dynamic_array_query_operations... 通过
[4/10] test_dynamic_array_array_operations... 通过
[5/10] test_dynamic_array_iteration_operations... 通过
[6/10] test_dynamic_array_memory_management... 通过
[7/10] test_dynamic_array_error_handling... 通过
[8/10] test_dynamic_array_abstract_interface... 通过
[9/10] test_dynamic_array_edge_cases... 通过
[10/10] test_dynamic_array_performance... 通过

=== 测试结果 ===
总测试数: 10
通过: 10
失败: 0
通过率: 100.00%
```

### 测试设计原则

1. **模块化测试设计**：每个测试功能在单独的文件中实现，符合单一职责原则
2. **全面覆盖**：覆盖所有API接口、错误处理、边界条件和性能特征
3. **独立测试**：每个测试函数独立运行，不依赖其他测试的状态
4. **清晰的断言**：使用明确的断言检查预期行为
5. **错误处理验证**：验证所有错误码和边界条件
6. **性能基准**：建立关键操作的性能基准
7. **自动化构建**：使用Makefile自动化编译和测试过程
8. **向后兼容**：保持旧测试文件的兼容性，便于逐步迁移

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本动态数组功能 |
| 2.0.0 | 2026-01-06 | 重构版本，添加抽象接口、扩展API和错误处理机制 |
| 2.1.0 | 2026-01-06 | 模块化重构版本，将大型实现文件拆分为功能专注的小模块，符合单一职责原则 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
