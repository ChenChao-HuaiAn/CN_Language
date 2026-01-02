# CN_hash_table 模块 API 文档

## 概述

本文档详细描述了CN_hash_table模块的所有公共接口。CN_hash_table模块提供了高性能的哈希表容器，支持开放寻址法和链地址法两种冲突解决策略，具有动态扩容、完整操作API和迭代器支持。

## 架构位置

CN_hash_table模块位于基础设施层（Infrastructure Layer），为上层模块提供基础数据管理功能。遵循项目分层架构设计原则，只依赖C标准库和操作系统API。

## 数据结构

### 枚举类型

#### `enum Eum_CN_HashTableCollisionStrategy_t`
哈希表冲突解决策略枚举，定义了CN_hash_table支持的冲突解决策略。

```c
typedef enum Eum_CN_HashTableCollisionStrategy_t
{
    Eum_HASH_TABLE_OPEN_ADDRESSING = 0,   /**< 开放寻址法（线性探测） */
    Eum_HASH_TABLE_SEPARATE_CHAINING = 1  /**< 链地址法（链表解决冲突） */
} Eum_CN_HashTableCollisionStrategy_t;
```

#### `enum Eum_CN_HashEntryStatus_t`
哈希表条目状态枚举，用于开放寻址法。

```c
typedef enum Eum_CN_HashEntryStatus_t
{
    Eum_HASH_ENTRY_EMPTY = 0,     /**< 空条目 */
    Eum_HASH_ENTRY_OCCUPIED = 1,  /**< 已占用条目 */
    Eum_HASH_ENTRY_DELETED = 2    /**< 已删除条目（惰性删除） */
} Eum_CN_HashEntryStatus_t;
```

### 结构体类型

#### `struct Stru_CN_HashTable_t`
哈希表结构体（不透明类型）。外部代码只能通过指针访问，内部实现细节被隐藏。

```c
typedef struct Stru_CN_HashTable_t Stru_CN_HashTable_t;
```

#### `struct Stru_CN_HashTableIterator_t`
哈希表迭代器结构体，用于安全遍历哈希表键值对。

```c
typedef struct Stru_CN_HashTableIterator_t
{
    Stru_CN_HashTable_t* table;   /**< 关联的哈希表 */
    size_t current_bucket;        /**< 当前桶索引 */
    size_t current_entry;         /**< 当前条目索引（链地址法） */
    void* current_key;            /**< 当前键指针 */
    void* current_value;          /**< 当前值指针 */
} Stru_CN_HashTableIterator_t;
```

### 函数指针类型

#### `CN_HashFunc`
哈希函数类型，用于计算键的哈希值。

```c
typedef size_t (*CN_HashFunc)(const void* key, size_t table_size);
```

**参数**：
- `key`: 键指针
- `table_size`: 哈希表大小

**返回值**：哈希值（0到table_size-1之间）

#### `CN_KeyCompareFunc`
键比较函数类型，用于比较两个键是否相等。

```c
typedef bool (*CN_KeyCompareFunc)(const void* key1, const void* key2);
```

**参数**：
- `key1`: 第一个键指针
- `key2`: 第二个键指针

**返回值**：如果键相等返回true，否则返回false

#### `CN_KeyFreeFunc`
键释放函数类型，用于自定义键的释放。

```c
typedef void (*CN_KeyFreeFunc)(void* key);
```

**参数**：
- `key`: 要释放的键

#### `CN_ValueFreeFunc`
值释放函数类型，用于自定义值的释放。

```c
typedef void (*CN_ValueFreeFunc)(void* value);
```

**参数**：
- `value`: 要释放的值

#### `CN_KeyCopyFunc`
键复制函数类型，用于自定义键的深度复制。

```c
typedef void* (*CN_KeyCopyFunc)(const void* key);
```

**参数**：
- `key`: 源键

**返回值**：新分配的键副本

#### `CN_ValueCopyFunc`
值复制函数类型，用于自定义值的深度复制。

```c
typedef void* (*CN_ValueCopyFunc)(const void* value);
```

**参数**：
- `value`: 源值

**返回值**：新分配的值副本

## 函数接口

### 哈希表创建和销毁

#### `CN_hash_table_create`
创建标准哈希表。

```c
Stru_CN_HashTable_t* CN_hash_table_create(size_t key_size, size_t value_size,
                                          size_t initial_capacity,
                                          Eum_CN_HashTableCollisionStrategy_t strategy);
```

**参数**：
- `key_size`: 键大小（字节）
- `value_size`: 值大小（字节）
- `initial_capacity`: 初始容量（元素数量）
- `strategy`: 冲突解决策略

**返回值**：新创建的哈希表指针，失败返回NULL

**注意**：
- 使用默认哈希函数和比较函数
- 容量会自动调整为质数以减少冲突
- 调用者负责使用`CN_hash_table_destroy`释放返回的哈希表

#### `CN_hash_table_create_custom`
创建带自定义函数的哈希表，支持复杂键值类型。

```c
Stru_CN_HashTable_t* CN_hash_table_create_custom(
    size_t key_size, size_t value_size,
    size_t initial_capacity,
    Eum_CN_HashTableCollisionStrategy_t strategy,
    CN_HashFunc hash_func,
    CN_KeyCompareFunc key_compare_func,
    CN_KeyFreeFunc key_free_func,
    CN_ValueFreeFunc value_free_func,
    CN_KeyCopyFunc key_copy_func,
    CN_ValueCopyFunc value_copy_func);
```

**参数**：
- `key_size`: 键大小（字节）
- `value_size`: 值大小（字节）
- `initial_capacity`: 初始容量
- `strategy`: 冲突解决策略
- `hash_func`: 哈希函数（可为NULL，使用默认）
- `key_compare_func`: 键比较函数（可为NULL，使用默认）
- `key_free_func`: 键释放函数（可为NULL，使用free）
- `value_free_func`: 值释放函数（可为NULL，使用free）
- `key_copy_func`: 键复制函数（可为NULL，使用memcpy）
- `value_copy_func`: 值复制函数（可为NULL，使用memcpy）

**返回值**：新创建的哈希表指针，失败返回NULL

**注意**：
- 如果提供`key_free_func`和`value_free_func`，哈希表销毁时会自动调用它们
- 如果提供`key_copy_func`和`value_copy_func`，哈希表复制时会使用它们进行深度复制

#### `CN_hash_table_destroy`
销毁哈希表，释放所有分配的内存。

```c
void CN_hash_table_destroy(Stru_CN_HashTable_t* table);
```

**参数**：
- `table`: 要销毁的哈希表指针

**注意**：
- 如果哈希表创建时指定了`key_free_func`和`value_free_func`，会调用它们释放每个键值对
- 此函数会释放哈希表内部分配的所有内存
- 可以安全地传递NULL参数

#### `CN_hash_table_clear`
清空哈希表（移除所有键值对），但不释放哈希表本身。

```c
void CN_hash_table_clear(Stru_CN_HashTable_t* table);
```

**参数**：
- `table`: 要清空的哈希表

**注意**：
- 如果设置了`key_free_func`和`value_free_func`，会调用它们释放每个键值对
- 哈希表容量保持不变，可以重用

### 哈希表属性查询

#### `CN_hash_table_size`
获取哈希表大小（键值对数量）。

```c
size_t CN_hash_table_size(const Stru_CN_HashTable_t* table);
```

**参数**：
- `table`: 哈希表

**返回值**：哈希表大小，如果table为NULL返回0

#### `CN_hash_table_capacity`
获取哈希表容量（桶数量）。

```c
size_t CN_hash_table_capacity(const Stru_CN_HashTable_t* table);
```

**参数**：
- `table`: 哈希表

**返回值**：哈希表容量，如果table为NULL返回0

#### `CN_hash_table_is_empty`
检查哈希表是否为空。

```c
bool CN_hash_table_is_empty(const Stru_CN_HashTable_t* table);
```

**参数**：
- `table`: 哈希表

**返回值**：如果哈希表为空或为NULL返回true，否则返回false

#### `CN_hash_table_load_factor`
获取哈希表负载因子。

```c
double CN_hash_table_load_factor(const Stru_CN_HashTable_t* table);
```

**参数**：
- `table`: 哈希表

**返回值**：负载因子（size/capacity），如果table为NULL或容量为0返回0.0

#### `CN_hash_table_strategy`
获取哈希表冲突解决策略。

```c
Eum_CN_HashTableCollisionStrategy_t CN_hash_table_strategy(const Stru_CN_HashTable_t* table);
```

**参数**：
- `table`: 哈希表

**返回值**：冲突解决策略，如果table为NULL返回Eum_HASH_TABLE_OPEN_ADDRESSING

#### `CN_hash_table_key_size`
获取键大小（字节）。

```c
size_t CN_hash_table_key_size(const Stru_CN_HashTable_t* table);
```

**参数**：
- `table`: 哈希表

**返回值**：键大小，如果table为NULL返回0

#### `CN_hash_table_value_size`
获取值大小（字节）。

```c
size_t CN_hash_table_value_size(const Stru_CN_HashTable_t* table);
```

**参数**：
- `table`: 哈希表

**返回值**：值大小，如果table为NULL返回0

### 键值对操作

#### `CN_hash_table_put`
插入或更新键值对。

```c
bool CN_hash_table_put(Stru_CN_HashTable_t* table, const void* key, const void* value);
```

**参数**：
- `table`: 哈希表
- `key`: 键指针
- `value`: 值指针

**返回值**：操作成功返回true，失败返回false

**注意**：
- 如果键已存在，更新对应的值
- 如果键不存在，插入新的键值对
- 如果负载因子超过0.75，会自动扩容
- 键值会被复制到哈希表中

#### `CN_hash_table_get`
获取键对应的值。

```c
void* CN_hash_table_get(const Stru_CN_HashTable_t* table, const void* key);
```

**参数**：
- `table`: 哈希表
- `key`: 键指针

**返回值**：值指针，键不存在或table为NULL返回NULL

**注意**：
- 返回的指针指向哈希表内部存储，不要释放它
- 通过返回的指针修改值是安全的

#### `CN_hash_table_contains`
检查哈希表是否包含指定键。

```c
bool CN_hash_table_contains(const Stru_CN_HashTable_t* table, const void* key);
```

**参数**：
- `table`: 哈希表
- `key`: 键指针

**返回值**：如果包含键返回true，否则返回false

#### `CN_hash_table_remove`
移除指定键的键值对。

```c
bool CN_hash_table_remove(Stru_CN_HashTable_t* table, const void* key);
```

**参数**：
- `table`: 哈希表
- `key`: 键指针

**返回值**：移除成功返回true，失败返回false

**注意**：
- 如果设置了`key_free_func`和`value_free_func`，会调用它们释放键值对
- 开放寻址法中采用惰性删除（标记为已删除）

### 键和值集合获取

#### `CN_hash_table_keys`
获取所有键的数组。

```c
size_t CN_hash_table_keys(const Stru_CN_HashTable_t* table, void*** keys);
```

**参数**：
- `table`: 哈希表
- `keys`: 输出参数，接收键数组指针

**返回值**：键数量，失败返回0

**注意**：
- 返回的键数组需要调用者释放
- 每个键都是新分配的副本
- 调用者负责释放键数组和每个键

#### `CN_hash_table_values`
获取所有值的数组。

```c
size_t CN_hash_table_values(const Stru_CN_HashTable_t* table, void*** values);
```

**参数**：
- `table`: 哈希表
- `values`: 输出参数，接收值数组指针

**返回值**：值数量，失败返回0

**注意**：
- 返回的值数组需要调用者释放
- 每个值都是新分配的副本
- 调用者负责释放值数组和每个值

### 哈希表容量管理

#### `CN_hash_table_resize`
调整哈希表容量。

```c
bool CN_hash_table_resize(Stru_CN_HashTable_t* table, size_t new_capacity);
```

**参数**：
- `table`: 哈希表
- `new_capacity`: 新容量

**返回值**：调整成功返回true，失败返回false

**注意**：
- 新容量会自动调整为质数
- 调整容量会触发重新哈希所有键值对
- 如果新容量小于当前容量，操作失败

#### `CN_hash_table_auto_resize`
自动调整哈希表容量（基于负载因子）。

```c
bool CN_hash_table_auto_resize(Stru_CN_HashTable_t* table, double load_factor_threshold);
```

**参数**：
- `table`: 哈希表
- `load_factor_threshold`: 负载因子阈值

**返回值**：如果进行了调整返回true，否则返回false

**注意**：
- 如果当前负载因子超过阈值，容量翻倍
- 默认阈值0.75

### 哈希表操作

#### `CN_hash_table_copy`
复制哈希表，创建新的哈希表副本。

```c
Stru_CN_HashTable_t* CN_hash_table_copy(const Stru_CN_HashTable_t* src);
```

**参数**：
- `src`: 源哈希表

**返回值**：新创建的哈希表副本，失败返回NULL

**注意**：
- 如果源哈希表设置了`key_copy_func`和`value_copy_func`，会使用它们进行深度复制
- 否则进行浅复制（内存拷贝）
- 调用者负责销毁返回的副本

#### `CN_hash_table_merge`
合并两个哈希表，创建新的哈希表。

```c
Stru_CN_HashTable_t* CN_hash_table_merge(const Stru_CN_HashTable_t* table1,
                                         const Stru_CN_HashTable_t* table2,
                                         bool overwrite);
```

**参数**：
- `table1`: 第一个哈希表
- `table2`: 第二个哈希表
- `overwrite`: 是否覆盖重复键的值

**返回值**：新创建的合并后的哈希表，失败返回NULL

**注意**：
- 两个哈希表的键大小、值大小和策略必须相同
- 如果overwrite为true，table2的值会覆盖table1的值
- 如果overwrite为false，保留table1的值
- 调用者负责销毁返回的哈希表

### 迭代器支持

#### `CN_hash_table_iterator_create`
创建哈希表迭代器。

```c
Stru_CN_HashTableIterator_t* CN_hash_table_iterator_create(Stru_CN_HashTable_t* table);
```

**参数**：
- `table`: 哈希表

**返回值**：新创建的迭代器，失败返回NULL

**注意**：
- 迭代器初始位置在第一个元素之前
- 调用者负责销毁返回的迭代器

#### `CN_hash_table_iterator_destroy`
销毁迭代器。

```c
void CN_hash_table_iterator_destroy(Stru_CN_HashTableIterator_t* iterator);
```

**参数**：
- `iterator`: 要销毁的迭代器

#### `CN_hash_table_iterator_reset`
重置迭代器到开始位置。

```c
void CN_hash_table_iterator_reset(Stru_CN_HashTableIterator_t* iterator);
```

**参数**：
- `iterator`: 迭代器

#### `CN_hash_table_iterator_has_next`
检查迭代器是否有下一个键值对。

```c
bool CN_hash_table_iterator_has_next(const Stru_CN_HashTableIterator_t* iterator);
```

**参数**：
- `iterator`: 迭代器

**返回值**：如果有下一个键值对返回true，否则返回false

#### `CN_hash_table_iterator_next`
获取下一个键值对，并移动迭代器位置。

```c
bool CN_hash_table_iterator_next(Stru_CN_HashTableIterator_t* iterator,
                                 void** key, void** value);
```

**参数**：
- `iterator`: 迭代器
- `key`: 输出参数，接收键指针（可为NULL）
- `value`: 输出参数，接收值指针（可为NULL）

**返回值**：获取成功返回true，没有更多键值对返回false

**注意**：
- 返回的键值指针指向哈希表内部存储，不要释放它们
- 通过返回的指针修改键值是安全的

#### `CN_hash_table_iterator_current`
获取当前键值对，不移动迭代器位置。

```c
bool CN_hash_table_iterator_current(const Stru_CN_HashTableIterator_t* iterator,
                                    void** key, void** value);
```

**参数**：
- `iterator`: 迭代器
- `key`: 输出参数，接收键指针（可为NULL）
- `value`: 输出参数，接收值指针（可为NULL）

**返回值**：获取成功返回true，当前没有键值对返回false

### 工具函数

#### `CN_hash_table_dump`
转储哈希表信息到控制台（调试用）。

```c
void CN_hash_table_dump(const Stru_CN_HashTable_t* table);
```

**参数**：
- `table`: 哈希表

**注意**：
- 输出哈希表大小、容量、负载因子、策略等信息
- 主要用于调试目的

#### `CN_hash_table_stats`
获取哈希表统计信息。

```c
void CN_hash_table_stats(const Stru_CN_HashTable_t* table,
                         double* avg_probe_length,
                         size_t* max_probe_length,
                         size_t* empty_buckets);
```

**参数**：
- `table`: 哈希表
- `avg_probe_length`: 输出参数，接收平均探测长度（可为NULL）
- `max_probe_length`: 输出参数，接收最大探测长度（可为NULL）
- `empty_buckets`: 输出参数，接收空桶数量（可为NULL）

**注意**：
- 平均探测长度：成功查找一个元素所需的平均探测次数
- 最大探测长度：成功查找一个元素所需的最大探测次数
- 空桶数量：未使用的桶数量

### 默认哈希函数

#### `CN_hash_table_default_string_hash`
默认字符串哈希函数。

```c
size_t CN_hash_table_default_string_hash(const void* key, size_t table_size);
```

**参数**：
- `key`: 字符串键指针（以null结尾的C字符串）
- `table_size`: 哈希表大小

**返回值**：哈希值（0到table_size-1之间）

**注意**：
- 使用djb2哈希算法
- 适用于以null结尾的C字符串

#### `CN_hash_table_default_int_hash`
默认整数哈希函数。

```c
size_t CN_hash_table_default_int_hash(const void* key, size_t table_size);
```

**参数**：
- `key`: 整数键指针（size_t类型）
- `table_size`: 哈希表大小

**返回值**：哈希值（0到table_size-1之间）

**注意**：
- 使用乘法哈希算法
- 适用于size_t类型的整数键

#### `CN_hash_table_default_ptr_hash`
默认指针哈希函数。

```c
size_t CN_hash_table_default_ptr_hash(const void* key, size_t table_size);
```

**参数**：
- `key`: 指针键
- `table_size`: 哈希表大小

**返回值**：哈希值（0到table_size-1之间）

**注意**：
- 直接将指针地址取模
- 适用于指针类型的键

## 错误处理

所有函数都遵循以下错误处理原则：

1. **内存分配失败**：返回NULL或false
2. **无效参数**：返回适当的错误值（如NULL、false等）
3. **边界检查**：进行必要的参数验证
4. **资源管理**：谁创建谁销毁，或使用引用计数

### 常见错误返回值
- 指针返回函数：失败返回NULL
- 布尔返回函数：失败返回false
- 大小返回函数：失败返回0
- 枚举返回函数：失败返回Eum_HASH_TABLE_OPEN_ADDRESSING

## 线程安全性

CN_hash_table模块的函数不是线程安全的。如果需要在多线程环境中使用，调用者需要提供适当的同步机制。

## 内存管理

### 分配策略
1. **初始分配**：按initial_capacity分配，自动调整为质数
2. **动态扩容**：负载因子超过阈值（默认0.75）时容量翻倍
3. **重新哈希**：扩容时重新哈希所有键值对

### 内存所有权
1. **哈希表所有权**：谁创建谁销毁（使用`CN_hash_table_destroy`）
2. **键值对所有权**：
   - 内置类型：哈希表管理内存
   - 自定义类型：如果提供`key_free_func`和`value_free_func`，哈希表会调用它们
3. **迭代器所有权**：谁创建谁销毁（使用`CN_hash_table_iterator_destroy`）

### 内存安全
- 所有内存访问都进行边界检查
- 防止缓冲区溢出
- 支持自定义内存释放函数

## 性能特性

### 时间复杂度
- **插入（平均）**：O(1)
- **插入（最坏）**：O(n)（需要扩容）
- **查找（平均）**：O(1)
- **查找（最坏）**：O(n)（所有键哈希冲突）
- **删除（平均）**：O(1)
- **删除（最坏）**：O(n)

### 空间复杂度
- **存储开销**：capacity * (key_size + value_size + 开销)
- **额外开销**：
  - 开放寻址法：每个条目需要状态标记
  - 链地址法：每个节点需要next指针
- **扩容开销**：临时需要2倍内存（重新哈希期间）

### 优化特性
1. **质数容量**：使用质数作为容量以减少冲突
2. **动态扩容**：基于负载因子自动调整容量
3. **惰性删除**：开放寻址法中标记删除而非立即删除
4. **预分配**：支持预分配容量避免频繁扩容

## 使用示例

### 示例1：创建和使用整数键值对哈希表
```c
#include "CN_hash_table.h"
#include <stdio.h>

void example_int_hash_table(void)
{
    // 创建哈希表（整数键，整数值），使用开放寻址法
    Stru_CN_HashTable_t* table = CN_hash_table_create(
        sizeof(int), sizeof(int), 10, Eum_HASH_TABLE_OPEN_ADDRESSING);
    
    if (table == NULL) {
        printf("创建哈希表失败\n");
        return;
    }
    
    printf("哈希表创建成功，容量: %zu\n", CN_hash_table_capacity(table));
    
    // 插入键值对
    for (int i = 0; i < 20; i++) {
        int key = i;
        int value = i * 10;
        if (CN_hash_table_put(table, &key, &value)) {
            printf("插入键值对: %d -> %d\n", key, value);
        } else {
            printf("插入键值对失败: %d -> %d\n", key, value);
        }
    }
    
    printf("哈希表大小: %zu, 负载因子: %.2f\n", 
           CN_hash_table_size(table), CN_hash_table_load_factor(table));
    
    // 查找值
    for (int i = 0; i < 25; i++) {
        int key = i;
        int* value_ptr = (int*)CN_hash_table_get(table, &key);
        if (value_ptr != NULL) {
            printf("查找键 %d: 值 = %d\n", key, *value_ptr);
        } else {
            printf("键 %d 不存在\n", key);
        }
    }
    
    // 删除键值对
    for (int i = 5; i < 10; i++) {
        int key = i;
        if (CN_hash_table_remove(table, &key)) {
            printf("删除键: %d\n", key);
        }
    }
    
    printf("删除后哈希表大小: %zu\n", CN_hash_table_size(table));
    
    // 使用迭代器遍历
    Stru_CN_HashTableIterator_t* iterator = CN_hash_table_iterator_create(table);
    if (iterator != NULL) {
        printf("使用迭代器遍历哈希表:\n");
        while (CN_hash_table_iterator_has_next(iterator)) {
            int* key_ptr;
            int* value_ptr;
            if (CN_hash_table_iterator_next(iterator, (void**)&key_ptr, (void**)&value_ptr)) {
                printf("  键=%d, 值=%d\n", *key_ptr, *value_ptr);
            }
        }
        CN_hash_table_iterator_destroy(iterator);
    }
    
    // 销毁哈希表
    CN_hash_table_destroy(table);
}
```

### 示例2：使用字符串键哈希表
```c
#include "CN_hash_table.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void example_string_hash_table(void)
{
    // 创建哈希表（字符串键，整数值），使用链地址法
    Stru_CN_HashTable_t* table = CN_hash_table_create_custom(
        sizeof(char*), sizeof(int), 10, Eum_HASH_TABLE_SEPARATE_CHAINING,
        CN_hash_table_default_string_hash, NULL, NULL, NULL, NULL, NULL);
    
    if (table == NULL) {
        printf("创建字符串哈希表失败\n");
        return;
    }
    
    // 插入字符串键值对
    const char* fruits[] = {"apple", "banana", "cherry", "date", "elderberry"};
    int quantities[] = {10, 20, 30, 40, 50};
    
    for (int i = 0; i < 5; i++) {
        if (CN_hash_table_put(table, &fruits[i], &quantities[i])) {
            printf("插入: %s -> %d\n", fruits[i], quantities[i]);
        }
    }
    
    // 查找值
    const char* search_key = "banana";
    int* quantity_ptr = (int*)CN_hash_table_get(table, &search_key);
    if (quantity_ptr != NULL) {
        printf("查找 %s: 数量 = %d\n", search_key, *quantity_ptr);
    }
    
    // 检查键是否存在
    const char* check_key = "grape";
    if (CN_hash_table_contains(table, &check_key)) {
        printf("%s 存在\n", check_key);
    } else {
        printf("%s 不存在\n", check_key);
    }
    
    // 获取所有键
    void** keys;
    size_t key_count = CN_hash_table_keys(table, &keys);
    if (key_count > 0) {
        printf("所有键 (%zu 个):\n", key_count);
        for (size_t i = 0; i < key_count; i++) {
            char* key = *(char**)keys[i];
            printf("  %s\n", key);
            free(keys[i]); // 释放键副本
        }
        free(keys); // 释放键数组
    }
    
    CN_hash_table_destroy(table);
}
```

### 示例3：使用自定义键值类型
```c
#include "CN_hash_table.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    int id;
    char name[50];
} Employee;

// 自定义哈希函数（基于ID）
size_t employee_hash(const void* key, size_t table_size)
{
    const Employee* emp = (const Employee*)key;
    // 简单哈希：ID取模
    return emp->id % table_size;
}

// 自定义键比较函数
bool employee_compare(const void* key1, const void* key2)
{
    const Employee* emp1 = (const Employee*)key1;
    const Employee* emp2 = (const Employee*)key2;
    return emp1->id == emp2->id;
}

// 自定义键释放函数
void employee_key_free(void* key)
{
    // Employee结构体没有需要特别释放的资源
    // 这里只是演示自定义释放函数
    (void)key;
}

// 自定义值释放函数
void employee_value_free(void* value)
{
    // 整数值不需要特别释放
    (void)value;
}

// 自定义键复制函数
void* employee_key_copy(const void* key)
{
    const Employee* src = (const Employee*)key;
    Employee* copy = malloc(sizeof(Employee));
    if (copy != NULL) {
        copy->id = src->id;
        strcpy(copy->name, src->name);
    }
    return copy;
}

// 自定义值复制函数
void* employee_value_copy(const void* value)
{
    int* copy = malloc(sizeof(int));
    if (copy != NULL) {
        *copy = *(int*)value;
    }
    return copy;
}

void example_custom_hash_table(void)
{
    // 创建自定义哈希表
    Stru_CN_HashTable_t* table = CN_hash_table_create_custom(
        sizeof(Employee), sizeof(int), 10, Eum_HASH_TABLE_OPEN_ADDRESSING,
        employee_hash, employee_compare, employee_key_free, employee_value_free,
        employee_key_copy, employee_value_copy);
    
    if (table == NULL) {
        printf("创建自定义哈希表失败\n");
        return;
    }
    
    // 插入员工数据
    Employee emp1 = {101, "张三"};
    Employee emp2 = {102, "李四"};
    Employee emp3 = {103, "王五"};
    
    int salary1 = 5000;
    int salary2 = 6000;
    int salary3 = 5500;
    
    CN_hash_table_put(table, &emp1, &salary1);
    CN_hash_table_put(table, &emp2, &salary2);
    CN_hash_table_put(table, &emp3, &salary3);
    
    printf("插入 %d 个员工数据\n", (int)CN_hash_table_size(table));
    
    // 查找员工工资
    Employee search_emp = {102, ""}; // 只使用ID进行查找
    int* salary_ptr = (int*)CN_hash_table_get(table, &search_emp);
    if (salary_ptr != NULL) {
        printf("员工ID %d 的工资: %d\n", search_emp.id, *salary_ptr);
    }
    
    // 更新员工工资
    int new_salary = 6500;
    if (CN_hash_table_put(table, &emp2, &new_salary)) {
        printf("更新员工ID %d 的工资为: %d\n", emp2.id, new_salary);
    }
    
    // 使用迭代器遍历
    Stru_CN_HashTableIterator_t* iterator = CN_hash_table_iterator_create(table);
    if (iterator != NULL) {
        printf("所有员工数据:\n");
        while (CN_hash_table_iterator_has_next(iterator)) {
            Employee* key_ptr;
            int* value_ptr;
            if (CN_hash_table_iterator_next(iterator, (void**)&key_ptr, (void**)&value_ptr)) {
                printf("  员工ID: %d, 姓名: %s, 工资: %d\n", 
                       key_ptr->id, key_ptr->name, *value_ptr);
            }
        }
        CN_hash_table_iterator_destroy(iterator);
    }
    
    // 获取统计信息
    double avg_probe_length;
    size_t max_probe_length;
    size_t empty_buckets;
    CN_hash_table_stats(table, &avg_probe_length, &max_probe_length, &empty_buckets);
    
    printf("统计信息: 平均探测长度=%.2f, 最大探测长度=%zu, 空桶数量=%zu\n",
           avg_probe_length, max_probe_length, empty_buckets);
    
    CN_hash_table_destroy(table);
}
```

### 示例4：哈希表合并
```c
#include "CN_hash_table.h"
#include <stdio.h>

void example_hash_table_merge(void)
{
    // 创建两个哈希表
    Stru_CN_HashTable_t* table1 = CN_hash_table_create(
        sizeof(int), sizeof(int), 5, Eum_HASH_TABLE_OPEN_ADDRESSING);
    Stru_CN_HashTable_t* table2 = CN_hash_table_create(
        sizeof(int), sizeof(int), 5, Eum_HASH_TABLE_OPEN_ADDRESSING);
    
    // 向第一个表插入数据
    for (int i = 0; i < 5; i++) {
        int key = i;
        int value = i * 10;
        CN_hash_table_put(table1, &key, &value);
    }
    
    // 向第二个表插入数据（部分重叠）
    for (int i = 3; i < 8; i++) {
        int key = i;
        int value = i * 20;
        CN_hash_table_put(table2, &key, &value);
    }
    
    printf("表1大小: %zu, 表2大小: %zu\n", 
           CN_hash_table_size(table1), CN_hash_table_size(table2));
    
    // 不覆盖合并
    Stru_CN_HashTable_t* merged_no_overwrite = CN_hash_table_merge(table1, table2, false);
    if (merged_no_overwrite != NULL) {
        printf("不覆盖合并后大小: %zu\n", CN_hash_table_size(merged_no_overwrite));
        
        // 检查重叠键的值（应该使用table1的值）
        for (int i = 3; i < 5; i++) {
            int key = i;
            int* value_ptr = (int*)CN_hash_table_get(merged_no_overwrite, &key);
            if (value_ptr != NULL) {
                printf("  键 %d 的值: %d (来自表1)\n", key, *value_ptr);
            }
        }
        CN_hash_table_destroy(merged_no_overwrite);
    }
    
    // 覆盖合并
    Stru_CN_HashTable_t* merged_overwrite = CN_hash_table_merge(table1, table2, true);
    if (merged_overwrite != NULL) {
        printf("覆盖合并后大小: %zu\n", CN_hash_table_size(merged_overwrite));
        
        // 检查重叠键的值（应该使用table2的值）
        for (int i = 3; i < 5; i++) {
            int key = i;
            int* value_ptr = (int*)CN_hash_table_get(merged_overwrite, &key);
            if (value_ptr != NULL) {
                printf("  键 %d 的值: %d (来自表2)\n", key, *value_ptr);
            }
        }
        CN_hash_table_destroy(merged_overwrite);
    }
    
    CN_hash_table_destroy(table1);
    CN_hash_table_destroy(table2);
}
```

## 版本历史

### v1.0.0 (2026-01-02)
- 初始版本
- 支持开放寻址法和链地址法两种冲突解决策略
- 实现完整的哈希表功能
- 提供迭代器支持
- 支持动态扩容和重新哈希
- 提供默认哈希函数（字符串、整数、指针）

### v1.0.1 (2026-01-02)
- 修复编译错误：内部辅助函数访问问题
- 修复测试失败：开放寻址法插入失败
- 改进错误处理和边界检查

### v1.1.0 (2026-01-02)
- 修复开放寻址法探测序列问题（二次探测改为线性探测）
- 修复迭代器重置函数中的错误
- 修复重新哈希期间的递归问题
- 改进测试覆盖率和性能基准测试
- 完善API文档和示例代码

## 相关文档

### 项目架构文档
- [架构设计原则](../../docs/architecture/架构设计原则.md) - 项目整体架构设计原则
- [架构决策记录](../../docs/architecture/架构决策记录.md) - 重要架构决策记录
- [模块依赖规范](../../docs/architecture/模块依赖规范.md) - 模块间依赖关系规范
- [实施路线图](../../docs/architecture/实施路线图.md) - 项目实施计划和时间表

### 技术规范文档
- [CN_Language项目 技术规范和编码标准](../../docs/specifications/CN_Language项目%20技术规范和编码标准.md) - 项目编码标准和规范
- [CN_Language项目 目录结构规范](../../docs/specifications/CN_Language项目%20目录结构规范.md) - 项目目录结构规范
- [CN_Language 语法规范](../../docs/specifications/CN_Language%20语法规范.md) - CN语言语法规范
- [CN_Language项目 构建方法和编译流程](../../docs/specifications/CN_Language项目%20构建方法和编译流程.md) - 项目构建和编译流程

### 设计文档
- [中文编程语言CN_Language开发规划](../../docs/design/中文编程语言CN_Language开发规划.md) - 项目整体开发规划
- [第一阶段：基础设施层重构详细实施计划（第1-3周）](../../docs/design/第一阶段：基础设施层重构详细实施计划（第1-3周）.md) - 基础设施层实施计划

### 模块文档
- [哈希表模块README](../../src/infrastructure/containers/hash_table/README.md) - 哈希表模块详细文档
- [哈希表单元测试](../../src/infrastructure/containers/hash_table/test_hash_table.c) - 哈希表单元测试代码

### API文档
- [CN_debug_API.md](../CN_debug_API.md) - 调试模块API文档

## 维护者

CN_hash_table模块由CN_Language项目基础设施层团队维护。

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件。

## 反馈和贡献

如有问题或建议，请通过项目GitHub仓库提交Issue或Pull Request。
