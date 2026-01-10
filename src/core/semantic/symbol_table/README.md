# CN_Language 符号表模块

## 概述

符号表模块是CN_Language语义分析系统的核心组件，负责管理程序中的符号（变量、函数、类型等）信息。本模块提供高效的符号存储、查找和管理功能，支持作用域链和符号信息维护。

## 功能特性

- **符号管理**：支持符号的添加、查找、更新和删除
- **作用域支持**：支持嵌套作用域和符号查找
- **符号信息**：存储符号的类型、位置、初始化状态等信息
- **高效查找**：支持作用域链查找和全局查找
- **内存管理**：自动管理符号内存，防止内存泄漏

## 接口定义

### 符号信息结构

```c
typedef struct Stru_SymbolInfo_t
{
    const char* name;                    ///< 符号名称
    Eum_SymbolType type;                 ///< 符号类型
    void* type_info;                     ///< 类型信息指针
    size_t line;                         ///< 符号定义行号
    size_t column;                       ///< 符号定义列号
    bool is_exported;                    ///< 是否导出符号
    bool is_initialized;                 ///< 是否已初始化
    bool is_constant;                    ///< 是否为常量
    void* extra_data;                    ///< 额外数据指针
} Stru_SymbolInfo_t;
```

### 符号表接口

```c
typedef struct Stru_SymbolTableInterface_t
{
    bool (*initialize)(Stru_SymbolTableInterface_t* self, void* config);
    bool (*insert_symbol)(Stru_SymbolTableInterface_t* self, const Stru_SymbolInfo_t* symbol);
    Stru_SymbolInfo_t* (*lookup_symbol)(Stru_SymbolTableInterface_t* self, 
                                       const char* name, bool search_parent);
    bool (*remove_symbol)(Stru_SymbolTableInterface_t* self, const char* name);
    bool (*update_symbol)(Stru_SymbolTableInterface_t* self, const char* name, 
                         const Stru_SymbolInfo_t* new_info);
    size_t (*get_symbol_count)(Stru_SymbolTableInterface_t* self);
    bool (*set_parent_scope)(Stru_SymbolTableInterface_t* self, 
                            Stru_SymbolTableInterface_t* parent);
    Stru_SymbolTableInterface_t* (*get_parent_scope)(Stru_SymbolTableInterface_t* self);
    void (*destroy)(Stru_SymbolTableInterface_t* self);
    void* private_data;
} Stru_SymbolTableInterface_t;
```

## 使用示例

### 基本使用

```c
#include "CN_symbol_table.h"

int main() {
    // 创建符号表实例
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    
    if (!symbol_table) {
        return 1;
    }
    
    // 初始化符号表
    if (!symbol_table->initialize(symbol_table, NULL)) {
        F_destroy_symbol_table_interface(symbol_table);
        return 1;
    }
    
    // 创建符号信息
    Stru_SymbolInfo_t symbol = {
        .name = "my_variable",
        .type = Eum_SYMBOL_VARIABLE,
        .type_info = NULL,
        .line = 10,
        .column = 5,
        .is_exported = false,
        .is_initialized = false,
        .is_constant = false,
        .extra_data = NULL
    };
    
    // 插入符号
    if (symbol_table->insert_symbol(symbol_table, &symbol)) {
        printf("符号插入成功\n");
    }
    
    // 查找符号
    Stru_SymbolInfo_t* found = symbol_table->lookup_symbol(symbol_table, "my_variable", false);
    if (found) {
        printf("找到符号: %s (行: %zu, 列: %zu)\n", found->name, found->line, found->column);
    }
    
    // 销毁符号表
    F_destroy_symbol_table_interface(symbol_table);
    
    return 0;
}
```

### 作用域链示例

```c
void scope_chain_example(void) {
    // 创建全局作用域符号表
    Stru_SymbolTableInterface_t* global_scope = F_create_symbol_table_interface();
    global_scope->initialize(global_scope, NULL);
    
    // 在全局作用域插入符号
    Stru_SymbolInfo_t global_var = {
        .name = "global_var",
        .type = Eum_SYMBOL_VARIABLE,
        .line = 1,
        .column = 1
    };
    global_scope->insert_symbol(global_scope, &global_var);
    
    // 创建局部作用域符号表
    Stru_SymbolTableInterface_t* local_scope = F_create_symbol_table_interface();
    local_scope->initialize(local_scope, NULL);
    
    // 设置父作用域
    local_scope->set_parent_scope(local_scope, global_scope);
    
    // 在局部作用域插入符号
    Stru_SymbolInfo_t local_var = {
        .name = "local_var",
        .type = Eum_SYMBOL_VARIABLE,
        .line = 10,
        .column = 5
    };
    local_scope->insert_symbol(local_scope, &local_var);
    
    // 在局部作用域查找全局符号
    Stru_SymbolInfo_t* found_global = local_scope->lookup_symbol(local_scope, "global_var", true);
    if (found_global) {
        printf("在局部作用域找到全局符号: %s\n", found_global->name);
    }
    
    // 销毁符号表
    F_destroy_symbol_table_interface(local_scope);
    F_destroy_symbol_table_interface(global_scope);
}
```

## 实现细节

### 数据结构

符号表使用动态数组存储符号信息，支持快速插入和遍历：

```c
typedef struct Stru_SymbolTablePrivateData_t
{
    Stru_SymbolInfo_t* symbols;          ///< 符号数组
    size_t capacity;                     ///< 数组容量
    size_t size;                         ///< 当前符号数量
    Stru_SymbolTableInterface_t* parent; ///< 父作用域符号表
} Stru_SymbolTablePrivateData_t;
```

### 算法复杂度

- **插入符号**：O(1) 平均情况（动态数组追加）
- **查找符号**：O(n) 最坏情况（线性查找）
- **删除符号**：O(n) 最坏情况（需要移动元素）
- **更新符号**：O(n) 最坏情况（需要查找符号）

### 内存管理

- 符号表自动管理符号内存
- 插入符号时复制符号信息
- 销毁符号表时释放所有相关内存
- 支持自定义内存分配器（通过配置参数）

## 配置选项

符号表支持以下配置选项：

```c
typedef struct Stru_SymbolTableConfig_t
{
    size_t initial_capacity;             ///< 初始容量
    void* (*malloc_func)(size_t);        ///< 内存分配函数
    void (*free_func)(void*);            ///< 内存释放函数
} Stru_SymbolTableConfig_t;
```

## 错误处理

符号表模块提供以下错误处理机制：

1. **内存分配失败**：返回NULL或false
2. **符号重复插入**：返回false，不覆盖现有符号
3. **符号未找到**：返回NULL
4. **无效参数**：返回false或NULL

## 测试策略

### 单元测试

```c
// 测试符号插入和查找
void test_symbol_insert_and_lookup(void) {
    Stru_SymbolTableInterface_t* table = F_create_symbol_table_interface();
    TEST_ASSERT_NOT_NULL(table);
    
    Stru_SymbolInfo_t symbol = {.name = "test", .type = Eum_SYMBOL_VARIABLE};
    TEST_ASSERT_TRUE(table->insert_symbol(table, &symbol));
    
    Stru_SymbolInfo_t* found = table->lookup_symbol(table, "test", false);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("test", found->name);
    
    F_destroy_symbol_table_interface(table);
}
```

### 集成测试

- 测试作用域链查找
- 测试符号更新和删除
- 测试内存管理
- 测试错误处理

## 性能优化建议

1. **使用哈希表**：对于大型符号表，可考虑使用哈希表实现
2. **符号缓存**：缓存常用符号查找结果
3. **批量操作**：支持批量符号插入和查找
4. **内存池**：使用内存池减少内存分配开销

## 扩展指南

### 添加新的符号类型

1. 在`Eum_SymbolType`枚举中添加新类型
2. 更新符号信息结构（如果需要）
3. 更新符号处理函数
4. 添加相应的测试用例

### 自定义符号表实现

1. 实现`Stru_SymbolTableInterface_t`接口
2. 提供工厂函数
3. 实现所有接口方法
4. 确保内存管理正确

## 相关模块

- **作用域管理器**：管理作用域层次结构
- **类型检查器**：使用符号表中的类型信息
- **语义分析器**：整合符号表进行语义分析

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本 |
| 2.0.0 | 2026-01-09 | 模块化重构 |
| 2.0.1 | 2026-01-10 | 完善文档和测试 |

## 维护信息

- **最后更新**：2026年1月10日
- **维护者**：CN_Language语义分析团队
- **架构版本**：2.0.0
- **兼容性**：向后兼容所有版本

## 许可证

本模块遵循MIT许可证。详细信息请参阅项目根目录下的LICENSE文件。
