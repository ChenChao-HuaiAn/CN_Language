# 哈希表模块 (Hash Table Module)

## 概述

哈希表模块提供了哈希表数据结构的实现。哈希表支持高效的键值对查找、插入和删除操作，平均时间复杂度为O(1)。

## 特性

- **高效查找**：平均O(1)时间复杂度的查找操作
- **动态扩容**：当负载因子过高时自动扩容
- **冲突解决**：使用链地址法解决哈希冲突
- **类型安全**：支持自定义键值类型
- **内存管理**：自动管理内存分配和释放

## 数据结构

### Stru_HashTable_t

哈希表的主要结构体：

```c
typedef struct Stru_HashTable_t
{
    Stru_HashEntry_t** buckets; // 桶数组
    size_t capacity;            // 哈希表容量（桶的数量）
    size_t size;                // 哈希表中条目数量
    size_t key_size;            // 键的大小（字节）
    size_t value_size;          // 值的大小（字节）
    uint64_t (*hash_function)(void* key); // 哈希函数
    int (*compare_function)(void* key1, void* key2); // 键比较函数
} Stru_HashTable_t;
```

### Stru_HashEntry_t

哈希表条目结构体：

```c
typedef struct Stru_HashEntry_t
{
    void* key;                  // 键指针
    void* value;                // 值指针
    struct Stru_HashEntry_t* next; // 下一个条目（解决哈希冲突）
} Stru_HashEntry_t;
```

## API 接口

### 创建和销毁

- `F_create_hash_table(size_t key_size, size_t value_size, uint64_t (*hash_function)(void* key), int (*compare_function)(void* key1, void* key2), size_t initial_capacity)` - 创建哈希表
- `F_destroy_hash_table(Stru_HashTable_t* table)` - 销毁哈希表

### 内置哈希函数

- `F_string_hash_function(void* key)` - 字符串哈希函数
- `F_int_hash_function(void* key)` - 整数哈希函数
- `F_string_compare_function(void* key1, void* key2)` - 字符串比较函数
- `F_int_compare_function(void* key1, void* key2)` - 整数比较函数

### 基本操作

- `F_hash_table_put(Stru_HashTable_t* table, void* key, void* value)` - 插入键值对
- `F_hash_table_get(Stru_HashTable_t* table, void* key)` - 获取值
- `F_hash_table_remove(Stru_HashTable_t* table, void* key)` - 删除键值对
- `F_hash_table_contains(Stru_HashTable_t* table, void* key)` - 检查键是否存在

### 查询操作

- `F_hash_table_size(Stru_HashTable_t* table)` - 获取哈希表大小
- `F_hash_table_capacity(Stru_HashTable_t* table)` - 获取哈希表容量

### 哈希表操作

- `F_hash_table_clear(Stru_HashTable_t* table)` - 清空哈希表
- `F_hash_table_resize(Stru_HashTable_t* table, size_t new_capacity)` - 调整哈希表容量
- `F_hash_table_foreach(Stru_HashTable_t* table, void (*callback)(void* key, void* value, void* user_data), void* user_data)` - 遍历哈希表

## 使用示例

```c
#include "CN_hash_table.h"
#include <stdio.h>
#include <string.h>

// 打印回调函数
void print_entry(void* key, void* value, void* user_data) {
    char* k = (char*)key;
    int* v = (int*)value;
    printf("键: %s, 值: %d\n", k, *v);
}

int main() {
    // 创建哈希表（字符串键，整数值）
    Stru_HashTable_t* table = F_create_hash_table(
        sizeof(char[32]),  // 键大小：32字符的字符串
        sizeof(int),       // 值大小：整数
        F_string_hash_function,
        F_string_compare_function,
        16                 // 初始容量
    );
    
    if (table == NULL) {
        printf("创建哈希表失败\n");
        return 1;
    }
    
    // 插入键值对
    char key1[] = "apple";
    int value1 = 100;
    F_hash_table_put(table, key1, &value1);
    
    char key2[] = "banana";
    int value2 = 200;
    F_hash_table_put(table, key2, &value2);
    
    char key3[] = "orange";
    int value3 = 150;
    F_hash_table_put(table, key3, &value3);
    
    // 获取值
    int* retrieved = (int*)F_hash_table_get(table, "banana");
    if (retrieved != NULL) {
        printf("banana的值: %d\n", *retrieved);
    }
    
    // 检查键是否存在
    bool exists = F_hash_table_contains(table, "apple");
    printf("apple存在: %s\n", exists ? "是" : "否");
    
    // 遍历哈希表
    printf("所有条目:\n");
    F_hash_table_foreach(table, print_entry, NULL);
    
    // 获取哈希表信息
    printf("哈希表大小: %zu\n", F_hash_table_size(table));
    printf("哈希表容量: %zu\n", F_hash_table_capacity(table));
    
    // 删除条目
    F_hash_table_remove(table, "orange");
    printf("删除orange后大小: %zu\n", F_hash_table_size(table));
    
    // 清理
    F_destroy_hash_table(table);
    
    return 0;
}
```

## 性能特征

- **时间复杂度**（平均情况）：
  - 插入：O(1)
  - 查找：O(1)
  - 删除：O(1)
  - 遍历：O(n)

- **时间复杂度**（最坏情况）：
  - 所有操作：O(n)（当所有键都哈希到同一个桶时）

- **空间复杂度**：O(n + m)，其中n是条目数，m是桶数

## 内存管理

哈希表模块负责管理以下内存：
1. 哈希表结构体本身
2. 桶数组
3. 所有条目结构体
4. 键和值的数据

用户只需要提供要存储的键和值，模块会自动处理内存分配和释放。

## 错误处理

所有函数都包含错误检查：
- 无效参数返回适当错误值（NULL、false等）
- 内存分配失败返回错误
- 哈希函数或比较函数为NULL返回错误

## 线程安全

当前实现不是线程安全的。如果需要在多线程环境中使用，需要外部同步机制。

## 配置参数

可以通过以下方式调整哈希表的行为：
1. 选择合适的初始容量以减少扩容次数
2. 提供自定义的哈希函数以获得更好的分布
3. 提供自定义的比较函数以支持复杂键类型

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
| 1.0.0 | 2026-01-06 | 初始版本，实现基本哈希表功能 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
