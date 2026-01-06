# CN_hash_table API 文档

## 概述

`CN_hash_table` 模块提供了哈希表数据结构的实现。哈希表支持高效的键值对查找、插入和删除操作，使用链地址法解决哈希冲突。

## 头文件

```c
#include "infrastructure/containers/hash/CN_hash_table.h"
```

## 数据结构

### Stru_HashTable_t

哈希表的主要结构体。

```c
typedef struct Stru_HashTable_t
{
    Stru_HashEntry_t** buckets; /**< 桶数组 */
    size_t capacity;            /**< 哈希表容量（桶的数量） */
    size_t size;                /**< 哈希表中条目数量 */
    size_t key_size;            /**< 键的大小（字节） */
    size_t value_size;          /**< 值的大小（字节） */
    uint64_t (*hash_function)(void* key); /**< 哈希函数 */
    int (*compare_function)(void* key1, void* key2); /**< 键比较函数 */
} Stru_HashTable_t;
```

**字段说明：**

- `buckets`: 指向桶数组的指针，每个桶是一个哈希条目链表
- `capacity`: 哈希表的容量（桶的数量）
- `size`: 哈希表中键值对的数量
- `key_size`: 键的大小（字节）
- `value_size`: 值的大小（字节）
- `hash_function`: 哈希函数指针
- `compare_function`: 键比较函数指针

### Stru_HashEntry_t

哈希表条目结构体。

```c
typedef struct Stru_HashEntry_t
{
    void* key;                  /**< 键指针 */
    void* value;                /**< 值指针 */
    struct Stru_HashEntry_t* next; /**< 下一个条目（解决哈希冲突） */
} Stru_HashEntry_t;
```

**字段说明：**

- `key`: 指向键数据的指针
- `value`: 指向值数据的指针
- `next`: 指向下一个条目的指针（链地址法）

## 函数参考

### F_create_hash_table

```c
Stru_HashTable_t* F_create_hash_table(size_t key_size, size_t value_size,
                                     uint64_t (*hash_function)(void* key),
                                     int (*compare_function)(void* key1, void* key2),
                                     size_t initial_capacity);
```

创建并初始化一个新的哈希表。

**参数：**
- `key_size`: 键的大小（字节）
- `value_size`: 值的大小（字节）
- `hash_function`: 哈希函数
- `compare_function`: 键比较函数
- `initial_capacity`: 初始容量

**返回值：**
- 成功：指向新创建的哈希表的指针
- 失败：NULL（参数无效或内存分配失败）

### F_destroy_hash_table

```c
void F_destroy_hash_table(Stru_HashTable_t* table);
```

销毁哈希表，释放所有相关内存。

**参数：**
- `table`: 要销毁的哈希表指针

### F_hash_table_put

```c
bool F_hash_table_put(Stru_HashTable_t* table, void* key, void* value);
```

向哈希表中插入一个新的键值对。

**参数：**
- `table`: 哈希表指针
- `key`: 键指针
- `value`: 值指针

**返回值：**
- 成功：true
- 失败：false（参数无效或内存分配失败）

**注意：** 如果键已存在，会更新对应的值

### F_hash_table_get

```c
void* F_hash_table_get(Stru_HashTable_t* table, void* key);
```

根据键从哈希表中获取对应的值。

**参数：**
- `table`: 哈希表指针
- `key`: 键指针

**返回值：**
- 成功：指向值的指针
- 失败：NULL（键不存在或参数无效）

### F_hash_table_remove

```c
bool F_hash_table_remove(Stru_HashTable_t* table, void* key);
```

根据键从哈希表中删除对应的键值对。

**参数：**
- `table`: 哈希表指针
- `key`: 键指针

**返回值：**
- 成功：true
- 失败：false（键不存在或参数无效）

### F_hash_table_contains

```c
bool F_hash_table_contains(Stru_HashTable_t* table, void* key);
```

检查哈希表中是否存在指定的键。

**参数：**
- `table`: 哈希表指针
- `key`: 键指针

**返回值：**
- 键存在返回true，否则返回false

### F_hash_table_size

```c
size_t F_hash_table_size(Stru_HashTable_t* table);
```

获取哈希表中键值对的数量。

**参数：**
- `table`: 哈希表指针

**返回值：**
- 哈希表大小，如果`table`为NULL返回0

### F_hash_table_capacity

```c
size_t F_hash_table_capacity(Stru_HashTable_t* table);
```

获取哈希表的容量（桶的数量）。

**参数：**
- `table`: 哈希表指针

**返回值：**
- 哈希表容量，如果`table`为NULL返回0

### F_hash_table_clear

```c
void F_hash_table_clear(Stru_HashTable_t* table);
```

清空哈希表中的所有键值对，但不释放哈希表本身。

**参数：**
- `table`: 哈希表指针

### F_hash_table_resize

```c
bool F_hash_table_resize(Stru_HashTable_t* table, size_t new_capacity);
```

调整哈希表的容量到指定大小。

**参数：**
- `table`: 哈希表指针
- `new_capacity`: 新的容量大小

**返回值：**
- 成功：true
- 失败：false（参数无效或内存分配失败）

### F_hash_table_foreach

```c
void F_hash_table_foreach(Stru_HashTable_t* table,
                         void (*callback)(void* key, void* value, void* user_data),
                         void* user_data);
```

对哈希表中的每个键值对执行指定的操作。

**参数：**
- `table`: 哈希表指针
- `callback`: 回调函数，接收键指针、值指针和用户数据
- `user_data`: 传递给回调函数的用户数据

### F_string_hash_function

```c
uint64_t F_string_hash_function(void* key);
```

用于字符串键的哈希函数（djb2算法）。

**参数：**
- `key`: 字符串键指针

**返回值：** 哈希值

### F_string_compare_function

```c
int F_string_compare_function(void* key1, void* key2);
```

用于字符串键的比较函数。

**参数：**
- `key1`: 第一个字符串键指针
- `key2`: 第二个字符串键指针

**返回值：** 相等返回0，否则返回非0值

### F_int_hash_function

```c
uint64_t F_int_hash_function(void* key);
```

用于整数键的哈希函数。

**参数：**
- `key`: 整数键指针

**返回值：** 哈希值

### F_int_compare_function

```c
int F_int_compare_function(void* key1, void* key2);
```

用于整数键的比较函数。

**参数：**
- `key1`: 第一个整数键指针
- `key2`: 第二个整数键指针

**返回值：** 相等返回0，否则返回非0值

## 配置宏

### CN_HASH_TABLE_INITIAL_CAPACITY
```c
#define CN_HASH_TABLE_INITIAL_CAPACITY 16
```
哈希表的初始容量。

### CN_HASH_TABLE_LOAD_FACTOR
```c
#define CN_HASH_TABLE_LOAD_FACTOR 0.75
```
哈希表的负载因子阈值，当size/capacity >= 此值时扩容。

### CN_HASH_TABLE_GROWTH_FACTOR
```c
#define CN_HASH_TABLE_GROWTH_FACTOR 2
```
哈希表的扩容因子。

## 使用示例

```c
#include "infrastructure/containers/hash/CN_hash_table.h"
#include <stdio.h>
#include <string.h>

int main() {
    // 创建字符串键的哈希表
    Stru_HashTable_t* table = F_create_hash_table(
        sizeof(char*), sizeof(int),
        F_string_hash_function,
        F_string_compare_function,
        16
    );
    
    // 插入键值对
    char* key1 = "apple";
    int value1 = 10;
    F_hash_table_put(table, &key1, &value1);
    
    char* key2 = "banana";
    int value2 = 20;
    F_hash_table_put(table, &key2, &value2);
    
    // 获取值
    int* retrieved = (int*)F_hash_table_get(table, &key1);
    if (retrieved != NULL) {
        printf("apple: %d\n", *retrieved);
    }
    
    // 检查键是否存在
    char* key3 = "orange";
    bool exists = F_hash_table_contains(table, &key3);
    printf("orange exists: %s\n", exists ? "true" : "false");
    
    // 遍历哈希表
    void print_entry(void* key, void* value, void* user_data) {
        char** k = (char**)key;
        int* v = (int*)value;
        printf("Key: %s, Value: %d\n", *k, *v);
    }
    F_hash_table_foreach(table, print_entry, NULL);
    
    // 清理
    F_destroy_hash_table(table);
    return 0;
}
```

## 性能特征

### 时间复杂度（平均情况）
- 插入：O(1)
- 查找：O(1)
- 删除：O(1)
- 遍历：O(n)

### 时间复杂度（最坏情况）
- 所有操作：O(n)（当所有键都哈希到同一个桶时）

### 空间复杂度
- O(n + m)，其中n是键值对数量，m是桶的数量

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本 |

## 相关文档

- [哈希表模块README](../src/infrastructure/containers/hash/README.md)
- [CN_Language项目架构文档](../../../../architecture/001-中文编程语言CN_Language开发规划.md)
