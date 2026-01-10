# CN_Symbol_Table_Optimized API 文档

## 概述

优化版符号表模块提供高性能的符号表实现，使用哈希表、缓存机制和优化算法来提高符号查找和管理的性能。该模块是基础符号表模块的优化版本，提供相同的接口但具有更好的性能特性。

## 优化特性

### 1. 哈希表实现
- **FNV-1a哈希算法**：快速且分布均匀的哈希函数
- **开放寻址法**：解决哈希冲突，提高缓存局部性
- **动态扩容**：根据负载因子自动调整哈希表大小
- **惰性删除**：标记删除而非立即删除，提高性能

### 2. 缓存机制
- **最近访问缓存**：缓存最近访问的符号，减少哈希表查找
- **LRU策略**：使用最近最少使用策略管理缓存
- **预取优化**：预测性加载可能需要的符号

### 3. 批量操作优化
- **批量插入**：优化批量符号插入性能
- **批量查找**：支持批量符号查找
- **批量删除**：优化批量符号删除

### 4. 内存管理优化
- **内存池**：使用内存池减少内存分配开销
- **对象重用**：重用已释放的符号对象
- **紧凑存储**：减少内存碎片

## 模块结构

```
src/core/semantic/symbol_table/
├── CN_symbol_table.h           # 基础符号表接口定义
├── CN_symbol_table.c           # 基础符号表实现
├── CN_symbol_table_optimized.h # 优化版符号表接口定义
├── CN_symbol_table_optimized.c # 优化版符号表实现
└── README.md                   # 模块文档
```

## 接口定义

优化版符号表模块使用与基础符号表模块相同的接口（`Stru_SymbolTableInterface_t`），但通过不同的工厂函数创建优化实现。

### 符号表接口结构体

```c
typedef struct Stru_SymbolTableInterface_t
{
    // 初始化符号表
    bool (*initialize)(Stru_SymbolTableInterface_t* table);
    
    // 插入符号
    bool (*insert_symbol)(Stru_SymbolTableInterface_t* table, 
                         const Stru_SymbolInfo_t* symbol);
    
    // 查找符号
    Stru_SymbolInfo_t* (*lookup_symbol)(Stru_SymbolTableInterface_t* table,
                                       const char* name,
                                       bool search_parent);
    
    // 删除符号
    bool (*remove_symbol)(Stru_SymbolTableInterface_t* table,
                         const char* name);
    
    // 获取符号数量
    size_t (*get_symbol_count)(Stru_SymbolTableInterface_t* table);
    
    // 获取所有符号
    Stru_SymbolInfo_t** (*get_all_symbols)(Stru_SymbolTableInterface_t* table,
                                          size_t* count);
    
    // 清空符号表
    void (*clear)(Stru_SymbolTableInterface_t* table);
    
    // 销毁符号表
    void (*destroy)(Stru_SymbolTableInterface_t* table);
    
    // 私有数据
    void* private_data;
} Stru_SymbolTableInterface_t;
```

### 符号信息结构体

```c
typedef struct Stru_SymbolInfo_t
{
    const char* name;                    ///< 符号名称
    Eum_SymbolType type;                 ///< 符号类型
    void* type_info;                     ///< 类型信息
    size_t line;                         ///< 定义行号
    size_t column;                       ///< 定义列号
    bool is_exported;                    ///< 是否导出
    bool is_initialized;                 ///< 是否已初始化
    bool is_constant;                    ///< 是否为常量
    void* extra_data;                    ///< 额外数据
} Stru_SymbolInfo_t;
```

### 符号类型枚举

```c
typedef enum Eum_SymbolType_t
{
    Eum_SYMBOL_VARIABLE = 0,            ///< 变量符号
    Eum_SYMBOL_FUNCTION,                ///< 函数符号
    Eum_SYMBOL_STRUCT,                  ///< 结构体符号
    Eum_SYMBOL_ENUM,                    ///< 枚举符号
    Eum_SYMBOL_CONSTANT,                ///< 常量符号
    Eum_SYMBOL_PARAMETER,               ///< 参数符号
    Eum_SYMBOL_TYPE,                    ///< 类型符号
    Eum_SYMBOL_MODULE,                  ///< 模块符号
    Eum_SYMBOL_NAMESPACE,               ///< 命名空间符号
    Eum_SYMBOL_UNKNOWN                  ///< 未知符号类型
} Eum_SymbolType_t;
```

## API 函数

### 工厂函数

#### `F_create_symbol_table_interface_optimized`

```c
Stru_SymbolTableInterface_t* F_create_symbol_table_interface_optimized(void);
```

**功能**：创建优化版符号表接口实例。

**参数**：无

**返回值**：
- 成功：指向新创建的优化版符号表接口实例的指针
- 失败：NULL

**特点**：
- 使用哈希表实现，O(1)平均查找时间
- 内置缓存机制，提高重复访问性能
- 支持动态扩容，适应不同规模的符号表
- 内存管理优化，减少分配开销

**示例**：
```c
Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface_optimized();
if (symbol_table == NULL) {
    // 处理错误
}
```

#### `F_destroy_symbol_table_interface_optimized`

```c
void F_destroy_symbol_table_interface_optimized(Stru_SymbolTableInterface_t* interface);
```

**功能**：销毁优化版符号表接口实例。

**参数**：
- `interface`：要销毁的优化版符号表接口实例

**返回值**：无

**注意**：
- 会释放所有相关资源，包括哈希表、缓存和内存池
- 确保在销毁前不再使用该接口实例

**示例**：
```c
F_destroy_symbol_table_interface_optimized(symbol_table);
```

### 符号表接口函数

优化版符号表模块实现了与基础符号表模块相同的接口函数，但具有更好的性能特性。

#### `initialize`

```c
bool (*initialize)(Stru_SymbolTableInterface_t* table);
```

**功能**：初始化符号表。

**参数**：
- `table`：符号表接口实例

**返回值**：
- `true`：初始化成功
- `false`：初始化失败

**优化特性**：
- 延迟初始化哈希表，减少启动开销
- 预分配初始内存，减少运行时分配

#### `insert_symbol`

```c
bool (*insert_symbol)(Stru_SymbolTableInterface_t* table, 
                     const Stru_SymbolInfo_t* symbol);
```

**功能**：插入符号到符号表。

**参数**：
- `table`：符号表接口实例
- `symbol`：要插入的符号信息

**返回值**：
- `true`：插入成功
- `false`：插入失败（如符号已存在）

**优化特性**：
- 哈希表插入，O(1)平均时间复杂度
- 自动更新缓存
- 支持批量插入优化

#### `lookup_symbol`

```c
Stru_SymbolInfo_t* (*lookup_symbol)(Stru_SymbolTableInterface_t* table,
                                   const char* name,
                                   bool search_parent);
```

**功能**：在符号表中查找符号。

**参数**：
- `table`：符号表接口实例
- `name`：符号名称
- `search_parent`：是否在父作用域中查找

**返回值**：
- 成功：指向符号信息的指针
- 失败：NULL（符号不存在）

**优化特性**：
- 哈希表查找，O(1)平均时间复杂度
- 缓存最近访问的符号
- 支持批量查找优化

#### `remove_symbol`

```c
bool (*remove_symbol)(Stru_SymbolTableInterface_t* table,
                     const char* name);
```

**功能**：从符号表中删除符号。

**参数**：
- `table`：符号表接口实例
- `name`：要删除的符号名称

**返回值**：
- `true`：删除成功
- `false`：删除失败（如符号不存在）

**优化特性**：
- 惰性删除策略，提高性能
- 批量删除优化
- 自动清理缓存

#### `get_symbol_count`

```c
size_t (*get_symbol_count)(Stru_SymbolTableInterface_t* table);
```

**功能**：获取符号表中的符号数量。

**参数**：
- `table`：符号表接口实例

**返回值**：符号数量

**优化特性**：
- 维护计数器，O(1)时间复杂度
- 考虑惰性删除的符号

#### `get_all_symbols`

```c
Stru_SymbolInfo_t** (*get_all_symbols)(Stru_SymbolTableInterface_t* table,
                                      size_t* count);
```

**功能**：获取符号表中的所有符号。

**参数**：
- `table`：符号表接口实例
- `count`：输出参数，符号数量

**返回值**：
- 成功：指向符号信息数组的指针
- 失败：NULL

**注意**：
- 调用者负责释放返回的数组（但不释放数组中的符号信息）

**优化特性**：
- 批量获取，减少函数调用开销
- 内存布局优化，提高缓存命中率

#### `clear`

```c
void (*clear)(Stru_SymbolTableInterface_t* table);
```

**功能**：清空符号表。

**参数**：
- `table`：符号表接口实例

**返回值**：无

**优化特性**：
- 快速清空，重用内存
- 保留哈希表结构，避免重新分配

#### `destroy`

```c
void (*destroy)(Stru_SymbolTableInterface_t* table);
```

**功能**：销毁符号表。

**参数**：
- `table`：符号表接口实例

**返回值**：无

**优化特性**：
- 批量释放内存，减少系统调用
- 回收内存到内存池

## 使用示例

### 基本使用示例

```c
#include "CN_symbol_table_optimized.h"

// 创建优化版符号表
Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface_optimized();
if (symbol_table == NULL) {
    // 处理错误
    return;
}

// 初始化符号表
if (!symbol_table->initialize(symbol_table)) {
    // 处理错误
    F_destroy_symbol_table_interface_optimized(symbol_table);
    return;
}

// 插入符号
Stru_SymbolInfo_t symbol_info;
memset(&symbol_info, 0, sizeof(Stru_SymbolInfo_t));
symbol_info.name = "my_variable";
symbol_info.type = Eum_SYMBOL_VARIABLE;
symbol_info.line = 10;
symbol_info.column = 5;
symbol_info.is_initialized = true;

bool inserted = symbol_table->insert_symbol(symbol_table, &symbol_info);
if (!inserted) {
    // 处理插入失败
}

// 查找符号
Stru_SymbolInfo_t* found_symbol = symbol_table->lookup_symbol(symbol_table, "my_variable", false);
if (found_symbol != NULL) {
    // 使用找到的符号
    printf("Found symbol: %s at line %zu\n", found_symbol->name, found_symbol->line);
}

// 获取符号数量
size_t count = symbol_table->get_symbol_count(symbol_table);
printf("Symbol count: %zu\n", count);

// 销毁符号表
F_destroy_symbol_table_interface_optimized(symbol_table);
```

### 批量操作示例

```c
// 批量插入符号
const char* symbol_names[] = {"var1", "var2", "var3", "var4", "var5"};
size_t symbol_count = sizeof(symbol_names) / sizeof(symbol_names[0]);

for (size_t i = 0; i < symbol_count; i++) {
    Stru_SymbolInfo_t symbol;
    memset(&symbol, 0, sizeof(Stru_SymbolInfo_t));
    symbol.name = symbol_names[i];
    symbol.type = Eum_SYMBOL_VARIABLE;
    symbol.line = i + 1;
    symbol.column = 1;
    
    symbol_table->insert_symbol(symbol_table, &symbol);
}

// 批量查找符号
for (size_t i = 0; i < symbol_count; i++) {
    Stru_SymbolInfo_t* symbol = symbol_table->lookup_symbol(symbol_table, symbol_names[i], false);
    if (symbol != NULL) {
        printf("Found: %s\n", symbol->name);
    }
}
```

### 性能对比示例

```c
#include <time.h>

// 测试基础符号表性能
void test_basic_symbol_table_performance(void) {
    Stru_SymbolTableInterface_t* table = F_create_symbol_table_interface();
    table->initialize(table);
    
    clock_t start = clock();
    
    // 插入1000个符号
    for (int i = 0; i < 1000; i++) {
        char name[32];
        sprintf(name, "symbol_%d", i);
        
        Stru_SymbolInfo_t symbol;
        memset(&symbol, 0, sizeof(Stru_SymbolInfo_t));
        symbol.name = strdup(name);
        symbol.type = Eum_SYMBOL_VARIABLE;
        
        table->insert_symbol(table, &symbol);
        free((void*)symbol.name);
    }
    
    // 查找1000个符号
    for (int i = 0; i < 1000; i++) {
        char name[32];
        sprintf(name, "symbol_%d", i);
        table->lookup_symbol(table, name, false);
    }
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Basic symbol table: %.6f seconds\n", elapsed);
    
    F_destroy_symbol_table_interface(table);
}

// 测试优化版符号表性能
void test_optimized_symbol_table_performance(void) {
    Stru_SymbolTableInterface_t* table = F_create_symbol_table_interface_optimized();
    table->initialize(table);
    
    clock_t start = clock();
    
    // 插入1000个符号
    for (int i = 0; i < 1000; i++) {
        char name[32];
        sprintf(name, "symbol_%d", i);
        
        Stru_SymbolInfo_t symbol;
        memset(&symbol, 0, sizeof(Stru_SymbolInfo_t));
        symbol.name = strdup(name);
        symbol.type = Eum_SYMBOL_VARIABLE;
        
        table->insert_symbol(table, &symbol);
        free((void*)symbol.name);
    }
    
    // 查找1000个符号
    for (int i = 0; i < 1000; i++) {
        char name[32];
        sprintf(name, "symbol_%d", i);
        table->lookup_symbol(table, name, false);
    }
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Optimized symbol table: %.6f seconds\n", elapsed);
    
    F_destroy_symbol_table_interface_optimized(table);
}
```

## 性能基准

### 时间复杂度对比

| 操作 | 基础符号表 | 优化版符号表 |
|------|------------|--------------|
| 插入 | O(n) | O(1) 平均 |
| 查找 | O(n) | O(1) 平均 |
| 删除 | O(n) | O(1) 平均 |
| 清空 | O(n) | O(1) |
| 计数 | O(1) | O(1) |

### 内存使用对比

| 特性 | 基础符号表 | 优化版符号表 |
|------|------------|--------------|
| 存储结构 | 动态数组 | 哈希表 |
| 内存开销 | 较低 | 中等（哈希表开销） |
| 缓存友好 | 一般 | 优秀 |
| 内存碎片 | 较少 | 可控 |

### 适用场景

1. **使用基础符号表**：
   - 符号数量较少（< 100）
   - 内存受限环境
   - 简单应用场景
   - 不需要高性能查找

2. **使用优化版符号表**：
   - 符号数量较多（> 100）
   - 需要高性能查找
   - 频繁的符号操作
   - 大型项目或复杂应用

## 配置选项

优化版符号表支持以下配置选项（通过环境变量或编译时定义）：

```c
// 哈希表初始大小
#define CN_SYMBOL_TABLE_INITIAL_SIZE 64

// 哈希表负载因子阈值（触发扩容）
#define CN_SYMBOL_TABLE_LOAD_FACTOR 0.75

// 缓存大小
#define CN_SYMBOL_TABLE_CACHE_SIZE 16

// 内存池块大小
#define CN_SYMBOL_TABLE_MEMORY_POOL_BLOCK_SIZE 1024
```

## 注意事项

1. **线程安全**：当前实现不是线程安全的，需要在多线程环境中添加同步机制。
2. **内存管理**：符号名称需要由调用者管理，符号表不会复制或释放符号名称。
3. **性能权衡**：优化版符号表在内存使用和性能之间进行权衡，适合需要高性能的场景。
4. **兼容性**：优化版符号表与基础符号表接口完全兼容，可以无缝替换。

## 故障排除

### 常见问题

1. **内存泄漏**：确保调用`F_destroy_symbol_table_interface_optimized`释放资源。
2. **性能下降**：检查哈希表负载因子，可能需要调整初始大小。
3. **符号丢失**：确认符号名称的唯一性，避免哈希冲突导致的问题。

### 调试支持

启用调试模式可以输出详细的性能统计信息：

```c
#define CN_SYMBOL_TABLE_DEBUG 1
```

调试信息包括：
- 哈希表统计（负载因子、冲突次数等）
- 缓存命中率
-
