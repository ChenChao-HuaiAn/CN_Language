# CN_Language 作用域管理器模块

## 概述

作用域管理器模块负责管理程序中的作用域层次结构，支持作用域的创建、销毁、切换和查找。本模块与符号表模块紧密协作，为语义分析提供作用域管理功能。

## 功能特性

- **作用域管理**：支持作用域的创建、销毁和切换
- **作用域栈**：管理作用域嵌套层次
- **符号查找**：支持在当前作用域和父作用域中查找符号
- **作用域信息**：存储作用域类型、位置等信息
- **内存管理**：自动管理作用域内存，防止内存泄漏

## 接口定义

### 作用域信息结构

```c
typedef struct Stru_ScopeInfo_t
{
    const char* name;                    ///< 作用域名称（可选）
    Eum_ScopeType type;                  ///< 作用域类型
    size_t line;                         ///< 作用域开始行号
    size_t column;                       ///< 作用域开始列号
    Stru_SymbolTableInterface_t* symbol_table; ///< 作用域对应的符号表
    void* extra_data;                    ///< 额外数据指针
} Stru_ScopeInfo_t;
```

### 作用域管理器接口

```c
typedef struct Stru_ScopeManagerInterface_t
{
    bool (*initialize)(Stru_ScopeManagerInterface_t* self, void* config);
    bool (*enter_scope)(Stru_ScopeManagerInterface_t* self, const Stru_ScopeInfo_t* scope_info);
    bool (*exit_scope)(Stru_ScopeManagerInterface_t* self);
    Stru_ScopeInfo_t* (*get_current_scope)(Stru_ScopeManagerInterface_t* self);
    Stru_SymbolTableInterface_t* (*get_current_symbol_table)(Stru_ScopeManagerInterface_t* self);
    Stru_SymbolInfo_t* (*lookup_symbol)(Stru_ScopeManagerInterface_t* self, 
                                       const char* name, bool search_parent);
    bool (*insert_symbol)(Stru_ScopeManagerInterface_t* self, const Stru_SymbolInfo_t* symbol);
    size_t (*get_scope_depth)(Stru_ScopeManagerInterface_t* self);
    void (*destroy)(Stru_ScopeManagerInterface_t* self);
    void* private_data;
} Stru_ScopeManagerInterface_t;
```

## 使用示例

### 基本使用

```c
#include "CN_scope_manager.h"

int main() {
    // 创建作用域管理器实例
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    
    if (!scope_manager) {
        return 1;
    }
    
    // 初始化作用域管理器
    if (!scope_manager->initialize(scope_manager, NULL)) {
        F_destroy_scope_manager_interface(scope_manager);
        return 1;
    }
    
    // 进入全局作用域
    Stru_ScopeInfo_t global_scope = {
        .name = "global",
        .type = Eum_SCOPE_GLOBAL,
        .line = 1,
        .column = 1,
        .symbol_table = NULL,
        .extra_data = NULL
    };
    
    if (scope_manager->enter_scope(scope_manager, &global_scope)) {
        printf("进入全局作用域\n");
    }
    
    // 在全局作用域插入符号
    Stru_SymbolInfo_t global_var = {
        .name = "global_var",
        .type = Eum_SYMBOL_VARIABLE,
        .line = 1,
        .column = 1
    };
    
    if (scope_manager->insert_symbol(scope_manager, &global_var)) {
        printf("全局变量插入成功\n");
    }
    
    // 进入函数作用域
    Stru_ScopeInfo_t function_scope = {
        .name = "my_function",
        .type = Eum_SCOPE_FUNCTION,
        .line = 10,
        .column = 1
    };
    
    if (scope_manager->enter_scope(scope_manager, &function_scope)) {
        printf("进入函数作用域\n");
    }
    
    // 在函数作用域插入符号
    Stru_SymbolInfo_t local_var = {
        .name = "local_var",
        .type = Eum_SYMBOL_VARIABLE,
        .line = 11,
        .column = 5
    };
    
    if (scope_manager->insert_symbol(scope_manager, &local_var)) {
        printf("局部变量插入成功\n");
    }
    
    // 查找符号（在当前作用域）
    Stru_SymbolInfo_t* found_local = scope_manager->lookup_symbol(scope_manager, "local_var", false);
    if (found_local) {
        printf("找到局部符号: %s\n", found_local->name);
    }
    
    // 查找符号（在父作用域）
    Stru_SymbolInfo_t* found_global = scope_manager->lookup_symbol(scope_manager, "global_var", true);
    if (found_global) {
        printf("找到全局符号: %s\n", found_global->name);
    }
    
    // 退出函数作用域
    scope_manager->exit_scope(scope_manager);
    printf("退出函数作用域\n");
    
    // 获取当前作用域深度
    size_t depth = scope_manager->get_scope_depth(scope_manager);
    printf("当前作用域深度: %zu\n", depth);
    
    // 销毁作用域管理器
    F_destroy_scope_manager_interface(scope_manager);
    
    return 0;
}
```

### 嵌套作用域示例

```c
void nested_scope_example(void) {
    Stru_ScopeManagerInterface_t* manager = F_create_scope_manager_interface();
    manager->initialize(manager, NULL);
    
    // 进入全局作用域
    Stru_ScopeInfo_t global = {.name = "global", .type = Eum_SCOPE_GLOBAL};
    manager->enter_scope(manager, &global);
    
    // 进入函数作用域
    Stru_ScopeInfo_t function = {.name = "func", .type = Eum_SCOPE_FUNCTION};
    manager->enter_scope(manager, &function);
    
    // 进入块作用域
    Stru_ScopeInfo_t block = {.name = "block", .type = Eum_SCOPE_BLOCK};
    manager->enter_scope(manager, &block);
    
    // 在块作用域插入符号
    Stru_SymbolInfo_t block_var = {.name = "block_var", .type = Eum_SYMBOL_VARIABLE};
    manager->insert_symbol(manager, &block_var);
    
    // 逐级退出作用域
    manager->exit_scope(manager); // 退出块作用域
    manager->exit_scope(manager); // 退出函数作用域
    manager->exit_scope(manager); // 退出全局作用域
    
    F_destroy_scope_manager_interface(manager);
}
```

## 实现细节

### 数据结构

作用域管理器使用栈结构管理作用域层次：

```c
typedef struct Stru_ScopeManagerPrivateData_t
{
    Stru_ScopeInfo_t* scope_stack;       ///< 作用域栈
    size_t stack_capacity;               ///< 栈容量
    size_t stack_size;                   ///< 当前栈大小
    Stru_SymbolTableInterface_t* (*create_symbol_table)(void); ///< 符号表创建函数
} Stru_ScopeManagerPrivateData_t;
```

### 算法复杂度

- **进入作用域**：O(1)（栈压入操作）
- **退出作用域**：O(1)（栈弹出操作）
- **获取当前作用域**：O(1)（栈顶访问）
- **符号查找**：O(n) 最坏情况（需要遍历作用域链）
- **符号插入**：O(1)（在当前作用域插入）

### 内存管理

- 作用域管理器自动管理作用域内存
- 每个作用域关联一个独立的符号表
- 退出作用域时自动销毁对应的符号表
- 支持自定义内存分配器

## 配置选项

作用域管理器支持以下配置选项：

```c
typedef struct Stru_ScopeManagerConfig_t
{
    size_t initial_stack_capacity;       ///< 初始栈容量
    Stru_SymbolTableInterface_t* (*create_symbol_table_func)(void); ///< 符号表创建函数
    void* (*malloc_func)(size_t);        ///< 内存分配函数
    void (*free_func)(void*);            ///< 内存释放函数
} Stru_ScopeManagerConfig_t;
```

## 错误处理

作用域管理器模块提供以下错误处理机制：

1. **内存分配失败**：返回false或NULL
2. **栈溢出**：返回false，不创建新作用域
3. **栈下溢**：返回false，不退出作用域
4. **无效参数**：返回false或NULL
5. **符号重复插入**：返回false，不覆盖现有符号

## 测试策略

### 单元测试

```c
// 测试作用域进入和退出
void test_scope_enter_and_exit(void) {
    Stru_ScopeManagerInterface_t* manager = F_create_scope_manager_interface();
    TEST_ASSERT_NOT_NULL(manager);
    TEST_ASSERT_TRUE(manager->initialize(manager, NULL));
    
    Stru_ScopeInfo_t scope = {.name = "test", .type = Eum_SCOPE_GLOBAL};
    TEST_ASSERT_TRUE(manager->enter_scope(manager, &scope));
    TEST_ASSERT_EQUAL(1, manager->get_scope_depth(manager));
    
    TEST_ASSERT_TRUE(manager->exit_scope(manager));
    TEST_ASSERT_EQUAL(0, manager->get_scope_depth(manager));
    
    F_destroy_scope_manager_interface(manager);
}
```

### 集成测试

- 测试嵌套作用域管理
- 测试作用域链符号查找
- 测试作用域内存管理
- 测试错误恢复机制

## 性能优化建议

1. **作用域缓存**：缓存常用作用域查找结果
2. **符号表池**：重用符号表实例，减少内存分配
3. **批量操作**：支持批量符号插入和查找
4. **作用域预分配**：预分配作用域栈空间，减少动态分配

## 扩展指南

### 添加新的作用域类型

1. 在`Eum_ScopeType`枚举中添加新类型
2. 更新作用域信息结构（如果需要）
3. 更新作用域处理函数
4. 添加相应的测试用例

### 自定义作用域管理器实现

1. 实现`Stru_ScopeManagerInterface_t`接口
2. 提供工厂函数
3. 实现所有接口方法
4. 确保内存管理正确

## 相关模块

- **符号表模块**：每个作用域关联一个符号表
- **类型检查器**：使用作用域信息进行类型检查
- **语义分析器**：整合作用域管理器进行语义分析

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
