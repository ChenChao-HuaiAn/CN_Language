# CN_Language 符号表模块 API 文档

## 概述

符号表模块是CN_Language语义分析系统的核心组件之一，负责符号的存储、查找和管理。本模块采用面向接口的设计，提供完整的符号管理功能，支持作用域链查找、符号属性管理和符号统计等功能。

## 接口定义

### Stru_SymbolTableInterface_t

符号表管理接口，负责符号的存储和查找。

```c
typedef struct Stru_SymbolTableInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_SymbolTableInterface_t* symbol_table, size_t initial_capacity);
    
    // 符号管理
    bool (*add_symbol)(Stru_SymbolTableInterface_t* symbol_table, const Stru_SymbolInfo_t* symbol);
    bool (*remove_symbol)(Stru_SymbolTableInterface_t* symbol_table, const char* symbol_name);
    bool (*update_symbol)(Stru_SymbolTableInterface_t* symbol_table, const char* symbol_name, const Stru_SymbolInfo_t* new_info);
    
    // 符号查找
    bool (*lookup_symbol)(Stru_SymbolTableInterface_t* symbol_table, const char* symbol_name, Stru_SymbolInfo_t* result);
    bool (*lookup_symbol_in_scope)(Stru_SymbolTableInterface_t* symbol_table, const char* symbol_name, uint32_t scope_level, Stru_SymbolInfo_t* result);
    bool (*find_symbols_by_type)(Stru_SymbolTableInterface_t* symbol_table, const char* type_name, Stru_SymbolInfo_t* results, size_t max_results, size_t* found_count);
    
    // 作用域支持
    bool (*set_parent_scope)(Stru_SymbolTableInterface_t* symbol_table, Stru_SymbolTableInterface_t* parent_scope);
    uint32_t (*get_current_scope_level)(Stru_SymbolTableInterface_t* symbol_table);
    
    // 统计信息
    size_t (*get_symbol_count)(Stru_SymbolTableInterface_t* symbol_table);
    size_t (*get_memory_usage)(Stru_SymbolTableInterface_t* symbol_table);
    
    // 遍历功能
    bool (*iterate_symbols)(Stru_SymbolTableInterface_t* symbol_table, 
                           bool (*callback)(const Stru_SymbolInfo_t* symbol, void* user_data), 
                           void* user_data);
    
    // 资源管理
    void (*destroy)(Stru_SymbolTableInterface_t* symbol_table);
    
    // 私有数据
    void* private_data;
} Stru_SymbolTableInterface_t;
```

## 辅助数据结构

### Stru_SymbolInfo_t

符号信息结构体，存储符号的详细信息。

```c
typedef struct Stru_SymbolInfo_t {
    char name[CN_MAX_SYMBOL_NAME_LENGTH];  // 符号名称
    char type_name[CN_MAX_TYPE_NAME_LENGTH]; // 类型名称
    uint32_t scope_level;                   // 作用域级别
    bool is_constant;                       // 是否为常量
    bool is_initialized;                    // 是否已初始化
    void* value;                            // 符号值（可选）
    void* extra_data;                       // 额外数据（可选）
    uint64_t flags;                         // 标志位
} Stru_SymbolInfo_t;
```

### Eum_SymbolType

符号类型枚举（如果需要，根据实际实现定义）。

```c
typedef enum Eum_SymbolType {
    Eum_SYMBOL_TYPE_VARIABLE,              // 变量
    Eum_SYMBOL_TYPE_FUNCTION,              // 函数
    Eum_SYMBOL_TYPE_TYPE,                  // 类型
    Eum_SYMBOL_TYPE_CONSTANT,              // 常量
    Eum_SYMBOL_TYPE_PARAMETER,             // 参数
    Eum_SYMBOL_TYPE_TEMPLATE,              // 模板
    Eum_SYMBOL_TYPE_MODULE,                // 模块
    Eum_SYMBOL_TYPE_UNKNOWN                // 未知类型
} Eum_SymbolType;
```

## API 参考

### F_create_symbol_table_interface

创建符号表接口实例。

**函数签名：**
```c
Stru_SymbolTableInterface_t* F_create_symbol_table_interface(void);
```

**返回值：**
- `Stru_SymbolTableInterface_t*`：新创建的符号表接口实例
- `NULL`：创建失败（内存不足）

**示例：**
```c
Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
if (symbol_table == NULL) {
    fprintf(stderr, "无法创建符号表\n");
    return 1;
}

// 使用符号表...

symbol_table->destroy(symbol_table);
```

### symbol_table->initialize

初始化符号表。

**函数签名：**
```c
bool initialize(Stru_SymbolTableInterface_t* symbol_table, size_t initial_capacity);
```

**参数：**
- `symbol_table`：符号表接口指针
- `initial_capacity`：初始容量（符号数量）

**返回值：**
- `true`：初始化成功
- `false`：初始化失败

### symbol_table->add_symbol

添加符号到符号表。

**函数签名：**
```c
bool add_symbol(Stru_SymbolTableInterface_t* symbol_table, const Stru_SymbolInfo_t* symbol);
```

**参数：**
- `symbol_table`：符号表接口指针
- `symbol`：要添加的符号信息

**返回值：**
- `true`：添加成功
- `false`：添加失败（如符号已存在）

### symbol_table->lookup_symbol

查找符号。

**函数签名：**
```c
bool lookup_symbol(Stru_SymbolTableInterface_t* symbol_table, const char* symbol_name, Stru_SymbolInfo_t* result);
```

**参数：**
- `symbol_table`：符号表接口指针
- `symbol_name`：符号名称
- `result`：输出参数，存储找到的符号信息

**返回值：**
- `true`：找到符号
- `false`：未找到符号

### symbol_table->remove_symbol

从符号表中删除符号。

**函数签名：**
```c
bool remove_symbol(Stru_SymbolTableInterface_t* symbol_table, const char* symbol_name);
```

**参数：**
- `symbol_table`：符号表接口指针
- `symbol_name`：要删除的符号名称

**返回值：**
- `true`：删除成功
- `false`：删除失败（如符号不存在）

### symbol_table->iterate_symbols

遍历符号表中的所有符号。

**函数签名：**
```c
bool iterate_symbols(Stru_SymbolTableInterface_t* symbol_table, 
                     bool (*callback)(const Stru_SymbolInfo_t* symbol, void* user_data), 
                     void* user_data);
```

**参数：**
- `symbol_table`：符号表接口指针
- `callback`：回调函数，对每个符号调用
- `user_data`：传递给回调函数的用户数据

**返回值：**
- `true`：遍历完成
- `false`：遍历中断（回调函数返回false）

### symbol_table->get_symbol_count

获取符号表中的符号数量。

**函数签名：**
```c
size_t get_symbol_count(Stru_SymbolTableInterface_t* symbol_table);
```

**返回值：**
- `size_t`：符号数量

### symbol_table->destroy

销毁符号表，释放所有资源。

**函数签名：**
```c
void destroy(Stru_SymbolTableInterface_t* symbol_table);
```

## 使用示例

### 基本使用模式

```c
#include "CN_symbol_table_interface.h"

int main() {
    // 创建符号表
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    if (symbol_table == NULL) {
        return 1;
    }
    
    // 初始化符号表
    if (!symbol_table->initialize(symbol_table, 1024)) {
        symbol_table->destroy(symbol_table);
        return 1;
    }
    
    // 添加符号
    Stru_SymbolInfo_t symbol1 = {
        .name = "变量1",
        .type_name = "整数",
        .scope_level = 0,
        .is_constant = false,
        .is_initialized = true,
        .value = NULL,
        .extra_data = NULL,
        .flags = 0
    };
    
    Stru_SymbolInfo_t symbol2 = {
        .name = "常量1",
        .type_name = "浮点数",
        .scope_level = 0,
        .is_constant = true,
        .is_initialized = true,
        .value = NULL,
        .extra_data = NULL,
        .flags = 0
    };
    
    symbol_table->add_symbol(symbol_table, &symbol1);
    symbol_table->add_symbol(symbol_table, &symbol2);
    
    // 查找符号
    Stru_SymbolInfo_t found_symbol;
    if (symbol_table->lookup_symbol(symbol_table, "变量1", &found_symbol)) {
        printf("找到符号: %s, 类型: %s, 常量: %s\n", 
               found_symbol.name, found_symbol.type_name,
               found_symbol.is_constant ? "是" : "否");
    }
    
    // 遍历符号
    symbol_table->iterate_symbols(symbol_table, symbol_callback, NULL);
    
    // 获取统计信息
    size_t count = symbol_table->get_symbol_count(symbol_table);
    printf("符号数量: %zu\n", count);
    
    // 清理
    symbol_table->destroy(symbol_table);
    
    return 0;
}

bool symbol_callback(const Stru_SymbolInfo_t* symbol, void* user_data) {
    printf("符号: %s (类型: %s, 作用域: %u)\n", 
           symbol->name, symbol->type_name, symbol->scope_level);
    return true; // 继续遍历
}
```

### 作用域链查找示例

```c
void scope_chain_lookup_example() {
    // 创建全局符号表
    Stru_SymbolTableInterface_t* global_table = F_create_symbol_table_interface();
    global_table->initialize(global_table, 512);
    
    // 创建局部符号表
    Stru_SymbolTableInterface_t* local_table = F_create_symbol_table_interface();
    local_table->initialize(local_table, 256);
    
    // 设置父作用域
    local_table->set_parent_scope(local_table, global_table);
    
    // 在全局作用域添加符号
    Stru_SymbolInfo_t global_symbol = {
        .name = "全局变量",
        .type_name = "整数",
        .scope_level = 0,
        .is_constant = false,
        .is_initialized = true
    };
    global_table->add_symbol(global_table, &global_symbol);
    
    // 在局部作用域添加符号
    Stru_SymbolInfo_t local_symbol = {
        .name = "局部变量",
        .type_name = "浮点数",
        .scope_level = 1,
        .is_constant = false,
        .is_initialized = true
    };
    local_table->add_symbol(local_table, &local_symbol);
    
    // 在局部作用域查找全局符号（通过作用域链）
    Stru_SymbolInfo_t result;
    if (local_table->lookup_symbol(local_table, "全局变量", &result)) {
        printf("通过作用域链找到全局符号: %s\n", result.name);
    }
    
    // 清理
    local_table->destroy(local_table);
    global_table->destroy(global_table);
}
```

### 符号过滤和查询示例

```c
void symbol_filtering_example(Stru_SymbolTableInterface_t* symbol_table) {
    // 添加不同类型的符号
    Stru_SymbolInfo_t symbols[] = {
        {.name = "变量1", .type_name = "整数", .scope_level = 0},
        {.name = "变量2", .type_name = "浮点数", .scope_level = 0},
        {.name = "变量3", .type_name = "整数", .scope_level = 1},
        {.name = "变量4", .type_name = "字符串", .scope_level = 0},
        {.name = "变量5", .type_name = "整数", .scope_level = 0}
    };
    
    for (int i = 0; i < 5; i++) {
        symbol_table->add_symbol(symbol_table, &symbols[i]);
    }
    
    // 查找特定类型的符号
    Stru_SymbolInfo_t results[10];
    size_t found_count = 0;
    
    if (symbol_table->find_symbols_by_type(symbol_table, "整数", results, 10, &found_count)) {
        printf("找到 %zu 个整数类型的符号:\n", found_count);
        for (size_t i = 0; i < found_count; i++) {
            printf("  %s (作用域: %u)\n", results[i].name, results[i].scope_level);
        }
    }
    
    // 遍历并过滤符号
    symbol_table->iterate_symbols(symbol_table, filter_callback, NULL);
}

bool filter_callback(const Stru_SymbolInfo_t* symbol, void* user_data) {
    // 只显示作用域为0的符号
    if (symbol->scope_level == 0) {
        printf("全局符号: %s (类型: %s)\n", symbol->name, symbol->type_name);
    }
    return true;
}
```

## 错误处理

### 错误类型

符号表操作可能遇到以下类型的错误：

1. **内存错误**：内存分配失败
2. **参数错误**：无效的参数（如NULL指针）
3. **符号错误**：符号已存在、符号不存在
4. **容量错误**：超出符号表容量
5. **作用域错误**：无效的作用域操作

### 错误处理策略

1. **返回值检查**：所有函数都返回bool表示成功/失败
2. **错误恢复**：部分失败不会影响其他操作
3. **资源清理**：失败时自动清理已分配的资源
4. **错误信息**：通过错误报告器提供详细的错误信息

## 性能考虑

### 时间复杂度

1. **符号添加**：O(1) 平均，O(n) 最坏情况（哈希冲突）
2. **符号查找**：O(1) 平均，O(n) 最坏情况（哈希冲突）
3. **符号删除**：O(1) 平均，O(n) 最坏情况（哈希冲突）
4. **符号遍历**：O(n)，其中n为符号数量
5. **作用域链查找**：O(d)，其中d为作用域深度

### 空间复杂度

1. **符号存储**：每个符号约64-128字节
2. **哈希表开销**：额外20-30%的内存用于哈希表结构
3. **作用域链**：每个作用域链节点约16-32字节

### 优化建议

1. **合适的初始容量**：根据预期符号数量设置初始容量
2. **哈希函数选择**：使用高效的哈希函数减少冲突
3. **内存池**：使用内存池减少内存分配开销
4. **缓存优化**：缓存常用符号的查找结果
5. **批量操作**：支持批量符号添加和删除

## 线程安全性

### 线程安全级别

符号表接口是**非线程安全**的。多个线程不能同时访问同一个符号表实例。

### 线程安全使用模式

```c
// 每个线程创建自己的符号表实例
void* symbol_table_thread(void* arg) {
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    symbol_table->initialize(symbol_table, 1024);
    
    // ... 使用符号表 ...
    
    symbol_table->destroy(symbol_table);
    return NULL;
}

// 或者使用互斥锁保护共享实例
pthread_mutex_t symbol_table_mutex = PTHREAD_MUTEX_INITIALIZER;
Stru_SymbolTableInterface_t* shared_symbol_table = NULL;

void safe_symbol_operation(const char* symbol_name) {
    pthread_mutex_lock(&symbol_table_mutex);
    
    if (shared_symbol_table == NULL) {
        shared_symbol_table = F_create_symbol_table_interface();
        shared_symbol_table->initialize(shared_symbol_table, 1024);
    }
    
    Stru_SymbolInfo_t symbol;
    bool found = shared_symbol_table->lookup_symbol(shared_symbol_table, symbol_name, &symbol);
    
    pthread_mutex_unlock(&symbol_table_mutex);
}
```

## 扩展指南

### 添加新的符号属性

1. 扩展`Stru_SymbolInfo_t`结构体
2. 更新符号表操作函数
3. 提供新的查询和过滤功能
4. 保持向后兼容性

### 实现不同的存储策略

1. **哈希表实现**：提高查找性能
2. **树结构实现**：支持范围查询
3. **混合实现**：结合多种数据结构的优点
4. **持久化存储**：支持符号表保存和加载

### 集成其他模块

1. **与作用域管理器集成**：支持动态作用域管理
2. **与类型检查器集成**：支持类型相关的符号查询
3. **与错误报告器集成**：提供详细的符号错误信息
4. **与调试器集成**：支持符号调试信息

## 测试策略

### 单元测试

测试每个接口函数的正确性：
- 符号添加、查找、删除测试
- 作用域链查找测试
- 符号遍历测试
- 错误处理测试
- 内存泄漏测试

### 集成测试

测试符号表与其他模块的集成：
- 与作用域管理器集成测试
- 与类型检查器集成测试
- 与语义分析器集成测试
- 性能测试

### 性能测试

建立性能基准：
- 符号查找性能测试
- 符号添加性能测试
- 内存使用测试
- 并发访问测试

## 相关文档

- [符号表模块 README](../../../../../src/core/semantic/symbol_table/README.md)
- [语义分析器接口文档](../CN_semantic_interface.md)
- [架构设计文档](../../../../architecture/001-中文编程语言CN_Language开发规划.md)
- [编码规范](../../../../specifications/CN_Language项目 技术规范和编码标准.md)

## 修订历史

| 版本 | 日期 | 描述 |
|
