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

## API 接口

### 创建和销毁

- `F_create_dynamic_array(size_t item_size)` - 创建动态数组
- `F_destroy_dynamic_array(Stru_DynamicArray_t* array)` - 销毁动态数组

### 元素操作

- `F_dynamic_array_push(Stru_DynamicArray_t* array, void* item)` - 添加元素
- `F_dynamic_array_get(Stru_DynamicArray_t* array, size_t index)` - 获取元素
- `F_dynamic_array_remove(Stru_DynamicArray_t* array, size_t index)` - 移除元素

### 查询操作

- `F_dynamic_array_length(Stru_DynamicArray_t* array)` - 获取数组长度
- `F_dynamic_array_capacity(Stru_DynamicArray_t* array)` - 获取数组容量

### 数组操作

- `F_dynamic_array_clear(Stru_DynamicArray_t* array)` - 清空数组
- `F_dynamic_array_resize(Stru_DynamicArray_t* array, size_t new_capacity)` - 调整数组容量
- `F_dynamic_array_foreach(Stru_DynamicArray_t* array, void (*callback)(void* item, void* user_data), void* user_data)` - 遍历数组

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

所有函数都包含错误检查：
- 无效参数返回适当错误值（NULL、false等）
- 内存分配失败返回错误
- 索引越界返回错误

## 线程安全

当前实现不是线程安全的。如果需要在多线程环境中使用，需要外部同步机制。

## 配置参数

可以通过修改以下宏定义调整动态数组的行为：

```c
#define CN_DYNAMIC_ARRAY_INITIAL_CAPACITY 16    // 初始容量
#define CN_DYNAMIC_ARRAY_GROWTH_FACTOR 2        // 扩容因子
```

## 依赖关系

- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`
- 无其他项目模块依赖

## 测试

模块包含完整的单元测试，覆盖所有API接口和边界条件。测试覆盖率目标：
- 语句覆盖率：≥90%
- 分支覆盖率：≥80%
- 函数覆盖率：100%

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本动态数组功能 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
