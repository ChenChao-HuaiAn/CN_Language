# CN_Language 哈希表容器模块

## 概述

哈希表（Hash Table）是CN_Language项目的基础数据结构之一，位于基础设施层的容器模块中。本模块提供了高效的键值对存储和检索功能，支持两种冲突解决策略：开放寻址法和链地址法。

## 特性

### 1. 两种冲突解决策略
- **开放寻址法（Open Addressing）**：使用二次探测解决冲突
- **链地址法（Separate Chaining）**：使用链表解决冲突

### 2. 动态扩容
- 自动根据负载因子调整容量（默认阈值0.75）
- 支持手动调整容量
- 使用质数作为容量以减少哈希冲突

### 3. 内存效率优化
- 预分配桶数组
- 支持自定义内存管理函数
- 惰性删除（仅开放寻址法）

### 4. 丰富的API
- 创建和销毁哈希表
- 插入、查找、更新、删除操作
- 迭代器支持
- 统计信息收集
- 复制和合并操作

### 5. 类型安全
- 支持任意类型的键和值
- 提供默认哈希函数（字符串、整数、指针）
- 支持自定义哈希和比较函数

## 文件结构

```
src/infrastructure/containers/hash_table/
├── CN_hash_table.h              # 公共接口头文件
├── CN_hash_table_internal.h     # 内部结构定义
├── CN_hash_table.c              # 主实现文件（创建、销毁、属性查询）
├── CN_hash_table_operations.c   # 操作实现文件（插入、查找、删除等）
├── CN_hash_table_utils.c        # 工具函数（调整大小、迭代器、统计等）
├── test_hash_table.c            # 单元测试文件
└── README.md                    # 本文档
```

## API 文档

### 数据结构

#### 哈希表结构体
```c
typedef struct Stru_CN_HashTable_t Stru_CN_HashTable_t;  // 不透明类型
```

#### 冲突解决策略枚举
```c
typedef enum Eum_CN_HashTableCollisionStrategy_t {
    Eum_HASH_TABLE_OPEN_ADDRESSING = 0,    // 开放寻址法
    Eum_HASH_TABLE_SEPARATE_CHAINING = 1   // 链地址法
} Eum_CN_HashTableCollisionStrategy_t;
```

#### 函数指针类型
```c
// 哈希函数
typedef size_t (*CN_HashFunc)(const void* key, size_t table_size);

// 键比较函数
typedef bool (*CN_KeyCompareFunc)(const void* key1, const void* key2);

// 内存管理函数
typedef void (*CN_KeyFreeFunc)(void* key);
typedef void (*CN_ValueFreeFunc)(void* value);
typedef void* (*CN_KeyCopyFunc)(const void* src);
typedef void* (*CN_ValueCopyFunc)(const void* src);
```

### 主要函数

#### 创建和销毁
```c
// 创建哈希表
Stru_CN_HashTable_t* CN_hash_table_create(
    size_t key_size, size_t value_size,
    size_t initial_capacity,
    Eum_CN_HashTableCollisionStrategy_t strategy);

// 创建带自定义函数的哈希表
Stru_CN_HashTable_t* CN_hash_table_create_custom(...);

// 销毁哈希表
void CN_hash_table_destroy(Stru_CN_HashTable_t* table);

// 清空哈希表
void CN_hash_table_clear(Stru_CN_HashTable_t* table);
```

#### 属性查询
```c
size_t CN_hash_table_size(const Stru_CN_HashTable_t* table);
size_t CN_hash_table_capacity(const Stru_CN_HashTable_t* table);
bool CN_hash_table_is_empty(const Stru_CN_HashTable_t* table);
double CN_hash_table_load_factor(const Stru_CN_HashTable_t* table);
```

#### 条目操作
```c
// 插入或更新键值对
bool CN_hash_table_put(Stru_CN_HashTable_t* table, 
                      const void* key, const void* value);

// 获取键对应的值
void* CN_hash_table_get(const Stru_CN_HashTable_t* table, const void* key);

// 检查键是否存在
bool CN_hash_table_contains(const Stru_CN_HashTable_t* table, const void* key);

// 移除键值对
bool CN_hash_table_remove(Stru_CN_HashTable_t* table, const void* key);
```

#### 迭代器
```c
// 创建迭代器
Stru_CN_HashTableIterator_t* CN_hash_table_iterator_create(
    Stru_CN_HashTable_t* table);

// 遍历哈希表
while (CN_hash_table_iterator_has_next(iterator)) {
    void* key;
    void* value;
    CN_hash_table_iterator_next(iterator, &key, &value);
    // 处理键值对
}
```

#### 工具函数
```c
// 调整容量
bool CN_hash_table_resize(Stru_CN_HashTable_t* table, size_t new_capacity);

// 自动调整容量
bool CN_hash_table_auto_resize(Stru_CN_HashTable_t* table, 
                              double load_factor_threshold);

// 复制哈希表
Stru_CN_HashTable_t* CN_hash_table_copy(const Stru_CN_HashTable_t* src);

// 合并两个哈希表
Stru_CN_HashTable_t* CN_hash_table_merge(const Stru_CN_HashTable_t* table1,
                                        const Stru_CN_HashTable_t* table2,
                                        bool overwrite);

// 获取统计信息
void CN_hash_table_stats(const Stru_CN_HashTable_t* table,
                        double* avg_probe_length,
                        size_t* max_probe_length,
                        size_t* empty_buckets);
```

## 使用示例

### 示例1：整数键值对
```c
#include "CN_hash_table.h"

// 创建哈希表（整数键，整数值）
Stru_CN_HashTable_t* table = CN_hash_table_create(
    sizeof(int), sizeof(int), 10, Eum_HASH_TABLE_OPEN_ADDRESSING);

// 插入数据
for (int i = 0; i < 100; i++) {
    int key = i;
    int value = i * 10;
    CN_hash_table_put(table, &key, &value);
}

// 查找数据
int key = 42;
int* value_ptr = (int*)CN_hash_table_get(table, &key);
if (value_ptr) {
    printf("值: %d\n", *value_ptr);  // 输出: 值: 420
}

// 清理
CN_hash_table_destroy(table);
```

### 示例2：字符串键值对
```c
#include "CN_hash_table.h"

// 创建哈希表（字符串键，整数值）
Stru_CN_HashTable_t* table = CN_hash_table_create_custom(
    sizeof(char*), sizeof(int), 10, Eum_HASH_TABLE_SEPARATE_CHAINING,
    CN_hash_table_default_string_hash, NULL, NULL, NULL, NULL, NULL);

// 插入数据
const char* name = "Alice";
int age = 30;
CN_hash_table_put(table, &name, &age);

// 查找数据
const char* search_key = "Alice";
int* age_ptr = (int*)CN_hash_table_get(table, &search_key);
if (age_ptr) {
    printf("年龄: %d\n", *age_ptr);  // 输出: 年龄: 30
}

// 清理
CN_hash_table_destroy(table);
```

### 示例3：使用迭代器
```c
#include "CN_hash_table.h"

// 创建哈希表并插入数据
Stru_CN_HashTable_t* table = CN_hash_table_create(
    sizeof(int), sizeof(int), 10, Eum_HASH_TABLE_OPEN_ADDRESSING);

for (int i = 0; i < 10; i++) {
    int key = i;
    int value = i * 100;
    CN_hash_table_put(table, &key, &value);
}

// 使用迭代器遍历
Stru_CN_HashTableIterator_t* iterator = CN_hash_table_iterator_create(table);
while (CN_hash_table_iterator_has_next(iterator)) {
    int* key_ptr;
    int* value_ptr;
    CN_hash_table_iterator_next(iterator, (void**)&key_ptr, (void**)&value_ptr);
    printf("键: %d, 值: %d\n", *key_ptr, *value_ptr);
}

CN_hash_table_iterator_destroy(iterator);
CN_hash_table_destroy(table);
```

## 性能特点

### 时间复杂度
- 平均情况：O(1) 的插入、查找、删除操作
- 最坏情况：O(n) 当所有键都哈希到同一个桶时

### 空间复杂度
- O(n + m)，其中n是元素数量，m是桶数量

### 负载因子
- 默认阈值：0.75
- 当负载因子超过阈值时自动扩容

## 设计决策

### 1. 二次探测 vs 线性探测
选择二次探测而非线性探测，因为：
- 减少聚集（clustering）现象
- 提供更好的缓存性能
- 公式：h(k, i) = (h1(k) + i²) % m

### 2. 惰性删除
在开放寻址法中采用惰性删除：
- 删除时标记为DELETED状态而非真正删除
- 插入时可重用DELETED位置
- 避免查找链断裂

### 3. 质数容量
使用质数作为哈希表容量：
- 减少哈希冲突
- 提高哈希函数分布均匀性
- 自动计算下一个质数

### 4. 自定义函数支持
提供完整的自定义函数支持：
- 哈希函数：支持字符串、整数、指针等类型
- 比较函数：支持自定义键比较逻辑
- 内存管理函数：支持自定义内存分配和释放

## 测试覆盖

### 单元测试
- 整数键值对测试
- 字符串键值对测试
- 性能测试（开放寻址法 vs 链地址法）
- 动态扩容测试
- 合并操作测试

### 性能基准
- 插入10000个元素的时间测量
- 平均探测长度统计
- 最大探测长度统计
- 空桶数量统计

## 依赖关系

### 内部依赖
- C标准库：stdlib.h, string.h, math.h, stdio.h
- 项目内部：无其他模块依赖

### 外部依赖
- 无第三方库依赖

## 编码规范

### 命名规范
- 结构体：`Stru_CN_HashTable_t`
- 枚举：`Eum_CN_HashTableCollisionStrategy_t`
- 函数：`CN_hash_table_create()`
- 常量：`Eum_HASH_TABLE_OPEN_ADDRESSING`

### 代码规范
- 每个函数不超过50行
- 每个文件不超过500行
- 完整的Doxygen注释
- 错误处理完善

## 扩展计划

### 短期改进
1. 添加线程安全支持
2. 优化内存分配策略
3. 添加更多哈希函数选项

### 长期规划
1. 支持并发访问
2. 添加持久化存储支持
3. 集成到CN_Language运行时系统

## 贡献指南

### 代码提交
1. 遵循项目编码规范
2. 添加相应的单元测试
3. 更新API文档
4. 运行现有测试确保兼容性

### 问题报告
1. 提供复现步骤
2. 包含环境信息
3. 描述预期行为和实际行为

## 许可证

本项目采用MIT许可证。详见项目根目录的LICENSE文件。

## 作者

CN_Language开发团队
- 创建日期：2026年1月
- 最后更新：2026年1月2日
- 版本：1.0.0
