# CN_Language 容器模块

## 概述

CN_Language容器模块提供了一系列基础数据结构实现，遵循项目分层架构设计原则。所有容器位于基础设施层，为上层模块提供基础数据管理功能。

## 模块结构

```
src/infrastructure/containers/
├── CN_array.h              # 动态数组公共接口
├── CN_array.c              # 动态数组核心实现
├── CN_array_operations.c   # 数组操作函数（复制、连接、切片等）
├── CN_array_search_sort.c  # 数组搜索和排序函数
├── CN_array_iterator.c     # 数组迭代器实现
├── CN_array_utils.c        # 数组工具函数
└── README.md               # 本文件
```

## 设计原则

### 1. 单一职责原则
- 每个.c文件不超过500行代码
- 每个函数不超过50行代码
- 每个文件专注于单一功能领域

### 2. 接口隔离原则
- 公共接口定义在CN_array.h中
- 内部实现细节对外隐藏
- 通过抽象接口提供功能

### 3. 依赖倒置原则
- 容器模块不依赖其他项目模块
- 只依赖C标准库和操作系统API
- 提供稳定的API供上层使用

## CN_array 动态数组

### 功能特性

1. **类型安全**：支持多种元素类型，包括内置类型和自定义类型
2. **动态扩容**：自动按需扩容，减少内存分配次数
3. **内存管理**：支持自定义内存释放和复制函数
4. **完整API**：提供完整的数组操作、搜索、排序功能
5. **迭代器支持**：提供安全的迭代访问机制

### 元素类型支持

```c
typedef enum Eum_CN_ArrayElementType_t
{
    Eum_ARRAY_ELEMENT_INT8 = 0,     // 8位有符号整数
    Eum_ARRAY_ELEMENT_UINT8 = 1,    // 8位无符号整数
    Eum_ARRAY_ELEMENT_INT16 = 2,    // 16位有符号整数
    Eum_ARRAY_ELEMENT_UINT16 = 3,   // 16位无符号整数
    Eum_ARRAY_ELEMENT_INT32 = 4,    // 32位有符号整数
    Eum_ARRAY_ELEMENT_UINT32 = 5,   // 32位无符号整数
    Eum_ARRAY_ELEMENT_INT64 = 6,    // 64位有符号整数
    Eum_ARRAY_ELEMENT_UINT64 = 7,   // 64位无符号整数
    Eum_ARRAY_ELEMENT_FLOAT = 8,    // 单精度浮点数
    Eum_ARRAY_ELEMENT_DOUBLE = 9,   // 双精度浮点数
    Eum_ARRAY_ELEMENT_BOOL = 10,    // 布尔值
    Eum_ARRAY_ELEMENT_CHAR = 11,    // 字符
    Eum_ARRAY_ELEMENT_POINTER = 12, // 指针
    Eum_ARRAY_ELEMENT_CUSTOM = 13   // 自定义类型
} Eum_CN_ArrayElementType_t;
```

### 核心API

#### 创建和销毁
```c
// 创建标准类型数组
Stru_CN_Array_t* CN_array_create(Eum_CN_ArrayElementType_t element_type,
                                 size_t element_size, size_t initial_capacity);

// 创建自定义类型数组
Stru_CN_Array_t* CN_array_create_custom(size_t element_size,
                                        size_t initial_capacity,
                                        CN_ArrayCompareFunc compare_func,
                                        CN_ArrayFreeFunc free_func,
                                        CN_ArrayCopyFunc copy_func);

// 销毁数组
void CN_array_destroy(Stru_CN_Array_t* array);
```

#### 元素访问
```c
// 获取元素指针
void* CN_array_get(const Stru_CN_Array_t* array, size_t index);

// 设置元素值
bool CN_array_set(Stru_CN_Array_t* array, size_t index, const void* element);

// 添加元素
bool CN_array_append(Stru_CN_Array_t* array, const void* element);
```

#### 数组操作
```c
// 复制数组
Stru_CN_Array_t* CN_array_copy(const Stru_CN_Array_t* src);

// 连接数组
Stru_CN_Array_t* CN_array_concat(const Stru_CN_Array_t* array1,
                                 const Stru_CN_Array_t* array2);

// 获取子数组
Stru_CN_Array_t* CN_array_slice(const Stru_CN_Array_t* array,
                                size_t start, size_t end);
```

#### 搜索和排序
```c
// 查找元素
size_t CN_array_find(const Stru_CN_Array_t* array, const void* element);

// 排序数组
void CN_array_sort(Stru_CN_Array_t* array);

// 反转数组
void CN_array_reverse(Stru_CN_Array_t* array);
```

#### 迭代器
```c
// 创建迭代器
Stru_CN_ArrayIterator_t* CN_array_iterator_create(Stru_CN_Array_t* array);

// 遍历数组
while (CN_array_iterator_has_next(iterator)) {
    void* element = CN_array_iterator_next(iterator);
    // 处理元素
}
```

### 使用示例

#### 示例1：创建和使用整数数组
```c
#include "CN_array.h"

void example_int_array(void)
{
    // 创建整数数组
    Stru_CN_Array_t* array = CN_array_create(Eum_ARRAY_ELEMENT_INT32, 
                                             sizeof(int), 10);
    
    // 添加元素
    int values[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) {
        CN_array_append(array, &values[i]);
    }
    
    // 访问元素
    int* elem = (int*)CN_array_get(array, 2);
    printf("Element at index 2: %d\n", *elem);
    
    // 遍历数组
    for (size_t i = 0; i < CN_array_length(array); i++) {
        int* value = (int*)CN_array_get(array, i);
        printf("array[%zu] = %d\n", i, *value);
    }
    
    // 销毁数组
    CN_array_destroy(array);
}
```

#### 示例2：使用自定义类型数组
```c
typedef struct {
    int id;
    char name[50];
    double score;
} Student;

int compare_students(const void* a, const void* b)
{
    const Student* s1 = (const Student*)a;
    const Student* s2 = (const Student*)b;
    return s1->id - s2->id;
}

void example_custom_array(void)
{
    // 创建自定义类型数组
    Stru_CN_Array_t* students = CN_array_create_custom(
        sizeof(Student), 10, compare_students, NULL, NULL);
    
    // 添加学生数据
    Student s1 = {101, "张三", 85.5};
    Student s2 = {102, "李四", 92.0};
    CN_array_append(students, &s1);
    CN_array_append(students, &s2);
    
    // 排序学生（按ID）
    CN_array_sort(students);
    
    // 查找学生
    Student search_key = {102, "", 0};
    size_t index = CN_array_find(students, &search_key);
    if (index != SIZE_MAX) {
        printf("找到学生，索引: %zu\n", index);
    }
    
    CN_array_destroy(students);
}
```

### 性能考虑

1. **时间复杂度**：
   - 随机访问：O(1)
   - 末尾添加：平均O(1)，最坏O(n)（需要扩容）
   - 中间插入/删除：O(n)
   - 搜索：O(n)（线性搜索）

2. **空间复杂度**：
   - 初始容量：可指定
   - 扩容策略：按1.5倍增长
   - 内存使用：capacity * element_size

3. **内存管理**：
   - 自动内存管理
   - 支持自定义释放函数
   - 可收缩容量以减少内存占用

### 错误处理

所有函数都提供明确的错误处理：
- 返回bool的函数：成功返回true，失败返回false
- 返回指针的函数：成功返回有效指针，失败返回NULL
- 返回size_t的函数：失败返回SIZE_MAX

### 线程安全

当前实现不是线程安全的。如果需要在多线程环境中使用，需要在外部添加同步机制。

### 测试覆盖率

模块包含完整的单元测试，确保：
- 语句覆盖率 ≥ 85%
- 分支覆盖率 ≥ 70%
- 函数覆盖率 ≥ 90%

### 版本历史

- v1.0.0 (2026-01-02): 初始版本，提供基本动态数组功能
- v1.1.0 (2026-01-02): 模块化重构，遵循单一职责原则

### 许可证

MIT许可证
