# CN_Language 作用域管理器模块 API 文档

## 概述

作用域管理器模块是CN_Language语义分析系统的核心组件之一，负责作用域的创建、管理和查询。本模块提供完整的作用域管理功能，支持作用域嵌套、作用域链查找、作用域信息查询等功能，为符号解析和类型检查提供作用域上下文。

## 接口定义

### Stru_ScopeManagerInterface_t

作用域管理器接口，负责作用域的创建和管理。

```c
typedef struct Stru_ScopeManagerInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_ScopeManagerInterface_t* scope_manager);
    
    // 作用域管理
    bool (*enter_scope)(Stru_ScopeManagerInterface_t* scope_manager, const char* scope_name);
    bool (*leave_scope)(Stru_ScopeManagerInterface_t* scope_manager);
    bool (*create_named_scope)(Stru_ScopeManagerInterface_t* scope_manager, const char* scope_name, uint32_t* scope_id);
    
    // 作用域查询
    uint32_t (*get_current_scope_level)(Stru_ScopeManagerInterface_t* scope_manager);
    const char* (*get_current_scope_name)(Stru_ScopeManagerInterface_t* scope_manager);
    bool (*get_scope_info)(Stru_ScopeManagerInterface_t* scope_manager, uint32_t scope_level, Stru_ScopeInfo_t* result);
    
    // 作用域层次
    bool (*is_in_scope)(Stru_ScopeManagerInterface_t* scope_manager, uint32_t scope_level);
    bool (*is_parent_scope)(Stru_ScopeManagerInterface_t* scope_manager, uint32_t parent_level, uint32_t child_level);
    uint32_t (*find_common_ancestor)(Stru_ScopeManagerInterface_t* scope_manager, uint32_t scope1, uint32_t scope2);
    
    // 作用域遍历
    bool (*iterate_scopes)(Stru_ScopeManagerInterface_t* scope_manager,
                          bool (*callback)(const Stru_ScopeInfo_t* scope, void* user_data),
                          void* user_data);
    
    // 资源管理
    void (*destroy)(Stru_ScopeManagerInterface_t* scope_manager);
    
    // 私有数据
    void* private_data;
} Stru_ScopeManagerInterface_t;
```

## 辅助数据结构

### Stru_ScopeInfo_t

作用域信息结构体，存储作用域的详细信息。

```c
typedef struct Stru_ScopeInfo_t {
    uint32_t scope_id;                      // 作用域ID
    char scope_name[CN_MAX_SCOPE_NAME_LENGTH]; // 作用域名称
    uint32_t parent_scope_id;               // 父作用域ID
    uint32_t depth;                         // 作用域深度
    size_t symbol_count;                    // 符号数量
    void* extra_data;                       // 额外数据（可选）
} Stru_ScopeInfo_t;
```

### Eum_ScopeType

作用域类型枚举。

```c
typedef enum Eum_ScopeType {
    Eum_SCOPE_TYPE_GLOBAL,                  // 全局作用域
    Eum_SCOPE_TYPE_FUNCTION,                // 函数作用域
    Eum_SCOPE_TYPE_BLOCK,                   // 块作用域
    Eum_SCOPE_TYPE_LOOP,                    // 循环作用域
    Eum_SCOPE_TYPE_CONDITIONAL,             // 条件作用域
    Eum_SCOPE_TYPE_CLASS,                   // 类作用域
    Eum_SCOPE_TYPE_NAMESPACE,               // 命名空间作用域
    Eum_SCOPE_TYPE_MODULE,                  // 模块作用域
    Eum_SCOPE_TYPE_UNKNOWN                  // 未知作用域
} Eum_ScopeType;
```

## API 参考

### F_create_scope_manager_interface

创建作用域管理器接口实例。

**函数签名：**
```c
Stru_ScopeManagerInterface_t* F_create_scope_manager_interface(void);
```

**返回值：**
- `Stru_ScopeManagerInterface_t*`：新创建的作用域管理器接口实例
- `NULL`：创建失败（内存不足）

**示例：**
```c
Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
if (scope_manager == NULL) {
    fprintf(stderr, "无法创建作用域管理器\n");
    return 1;
}

// 使用作用域管理器...

scope_manager->destroy(scope_manager);
```

### scope_manager->initialize

初始化作用域管理器。

**函数签名：**
```c
bool initialize(Stru_ScopeManagerInterface_t* scope_manager);
```

**参数：**
- `scope_manager`：作用域管理器接口指针

**返回值：**
- `true`：初始化成功
- `false`：初始化失败

### scope_manager->enter_scope

进入新的作用域。

**函数签名：**
```c
bool enter_scope(Stru_ScopeManagerInterface_t* scope_manager, const char* scope_name);
```

**参数：**
- `scope_manager`：作用域管理器接口指针
- `scope_name`：作用域名称（可为NULL表示匿名作用域）

**返回值：**
- `true`：进入作用域成功
- `false`：进入作用域失败

### scope_manager->leave_scope

离开当前作用域。

**函数签名：**
```c
bool leave_scope(Stru_ScopeManagerInterface_t* scope_manager);
```

**参数：**
- `scope_manager`：作用域管理器接口指针

**返回值：**
- `true`：离开作用域成功
- `false`：离开作用域失败（如已在最外层作用域）

### scope_manager->get_current_scope_level

获取当前作用域级别。

**函数签名：**
```c
uint32_t get_current_scope_level(Stru_ScopeManagerInterface_t* scope_manager);
```

**返回值：**
- `uint32_t`：当前作用域级别（0表示全局作用域）

### scope_manager->get_current_scope_name

获取当前作用域名称。

**函数签名：**
```c
const char* get_current_scope_name(Stru_ScopeManagerInterface_t* scope_manager);
```

**返回值：**
- `const char*`：当前作用域名称（可能为NULL表示匿名作用域）

### scope_manager->get_scope_info

获取指定作用域的信息。

**函数签名：**
```c
bool get_scope_info(Stru_ScopeManagerInterface_t* scope_manager, uint32_t scope_level, Stru_ScopeInfo_t* result);
```

**参数：**
- `scope_manager`：作用域管理器接口指针
- `scope_level`：作用域级别
- `result`：输出参数，存储作用域信息

**返回值：**
- `true`：获取成功
- `false`：获取失败（如作用域不存在）

### scope_manager->is_parent_scope

检查一个作用域是否是另一个作用域的父作用域。

**函数签名：**
```c
bool is_parent_scope(Stru_ScopeManagerInterface_t* scope_manager, uint32_t parent_level, uint32_t child_level);
```

**参数：**
- `scope_manager`：作用域管理器接口指针
- `parent_level`：可能的父作用域级别
- `child_level`：子作用域级别

**返回值：**
- `true`：parent_level是child_level的父作用域
- `false`：不是父作用域或参数无效

### scope_manager->iterate_scopes

遍历所有作用域。

**函数签名：**
```c
bool iterate_scopes(Stru_ScopeManagerInterface_t* scope_manager,
                   bool (*callback)(const Stru_ScopeInfo_t* scope, void* user_data),
                   void* user_data);
```

**参数：**
- `scope_manager`：作用域管理器接口指针
- `callback`：回调函数，对每个作用域调用
- `user_data`：传递给回调函数的用户数据

**返回值：**
- `true`：遍历完成
- `false`：遍历中断（回调函数返回false）

### scope_manager->destroy

销毁作用域管理器，释放所有资源。

**函数签名：**
```c
void destroy(Stru_ScopeManagerInterface_t* scope_manager);
```

## 使用示例

### 基本使用模式

```c
#include "CN_scope_manager_interface.h"

int main() {
    // 创建作用域管理器
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    if (scope_manager == NULL) {
        return 1;
    }
    
    // 初始化作用域管理器
    if (!scope_manager->initialize(scope_manager)) {
        scope_manager->destroy(scope_manager);
        return 1;
    }
    
    // 进入全局作用域
    scope_manager->enter_scope(scope_manager, "全局作用域");
    
    // 进入函数作用域
    scope_manager->enter_scope(scope_manager, "函数作用域");
    
    // 获取当前作用域信息
    uint32_t current_level = scope_manager->get_current_scope_level(scope_manager);
    const char* current_name = scope_manager->get_current_scope_name(scope_manager);
    printf("当前作用域: %s (级别: %u)\n", current_name, current_level);
    
    // 检查作用域关系
    bool in_global = scope_manager->is_in_scope(scope_manager, 0);
    bool is_parent = scope_manager->is_parent_scope(scope_manager, 0, 1);
    printf("在全局作用域中: %s\n", in_global ? "是" : "否");
    printf("全局是函数的父作用域: %s\n", is_parent ? "是" : "否");
    
    // 离开函数作用域
    scope_manager->leave_scope(scope_manager);
    
    // 离开全局作用域
    scope_manager->leave_scope(scope_manager);
    
    // 清理
    scope_manager->destroy(scope_manager);
    
    return 0;
}
```

### 作用域嵌套示例

```c
void scope_nesting_example(Stru_ScopeManagerInterface_t* scope_manager) {
    // 进入全局作用域
    scope_manager->enter_scope(scope_manager, "全局");
    
    // 进入函数作用域
    scope_manager->enter_scope(scope_manager, "函数main");
    
    // 进入条件语句作用域
    scope_manager->enter_scope(scope_manager, "if语句");
    
    // 进入循环作用域
    scope_manager->enter_scope(scope_manager, "for循环");
    
    // 获取当前作用域链
    printf("当前作用域链:\n");
    scope_manager->iterate_scopes(scope_manager, scope_callback, NULL);
    
    // 逐级离开作用域
    scope_manager->leave_scope(scope_manager); // 离开for循环
    scope_manager->leave_scope(scope_manager); // 离开if语句
    scope_manager->leave_scope(scope_manager); // 离开函数main
    scope_manager->leave_scope(scope_manager); // 离开全局
}

bool scope_callback(const Stru_ScopeInfo_t* scope, void* user_data) {
    printf("  作用域: %s (ID: %u, 深度: %u)\n", 
           scope->scope_name, scope->scope_id, scope->depth);
    return true;
}
```

### 作用域查询示例

```c
void scope_query_example(Stru_ScopeManagerInterface_t* scope_manager) {
    // 创建多个作用域
    scope_manager->enter_scope(scope_manager, "作用域A");
    uint32_t scope_a = scope_manager->get_current_scope_level(scope_manager);
    
    scope_manager->enter_scope(scope_manager, "作用域B");
    uint32_t scope_b = scope_manager->get_current_scope_level(scope_manager);
    
    scope_manager->enter_scope(scope_manager, "作用域C");
    uint32_t scope_c = scope_manager->get_current_scope_level(scope_manager);
    
    // 查询作用域信息
    Stru_ScopeInfo_t info;
    if (scope_manager->get_scope_info(scope_manager, scope_b, &info)) {
        printf("作用域B信息:\n");
        printf("  名称: %s\n", info.scope_name);
        printf("  ID: %u\n", info.scope_id);
        printf("  父作用域ID: %u\n", info.parent_scope_id);
        printf("  深度: %u\n", info.depth);
    }
    
    // 查找共同祖先
    uint32_t common_ancestor = scope_manager->find_common_ancestor(scope_manager, scope_b, scope_c);
    printf("作用域B和C的共同祖先级别: %u\n", common_ancestor);
    
    // 检查作用域关系
    bool b_is_parent_of_c = scope_manager->is_parent_scope(scope_manager, scope_b, scope_c);
    bool a_is_parent_of_c = scope_manager->is_parent_scope(scope_manager, scope_a, scope_c);
    printf("B是C的父作用域: %s\n", b_is_parent_of_c ? "是" : "否");
    printf("A是C的父作用域: %s\n", a_is_parent_of_c ? "是" : "否");
    
    // 清理
    scope_manager->leave_scope(scope_manager); // 离开C
    scope_manager->leave_scope(scope_manager); // 离开B
    scope_manager->leave_scope(scope_manager); // 离开A
}
```

### 作用域与符号表集成示例

```c
void scope_symbol_integration_example(Stru_ScopeManagerInterface_t* scope_manager, 
                                     Stru_SymbolTableInterface_t* symbol_table) {
    // 进入全局作用域
    scope_manager->enter_scope(scope_manager, "全局");
    uint32_t global_scope = scope_manager->get_current_scope_level(scope_manager);
    
    // 在全局作用域添加符号
    Stru_SymbolInfo_t global_symbol = {
        .name = "全局变量",
        .type_name = "整数",
        .scope_level = global_scope,
        .is_constant = false,
        .is_initialized = true
    };
    symbol_table->add_symbol(symbol_table, &global_symbol);
    
    // 进入函数作用域
    scope_manager->enter_scope(scope_manager, "函数作用域");
    uint32_t function_scope = scope_manager->get_current_scope_level(scope_manager);
    
    // 在函数作用域添加符号
    Stru_SymbolInfo_t local_symbol = {
        .name = "局部变量",
        .type_name = "浮点数",
        .scope_level = function_scope,
        .is_constant = false,
        .is_initialized = true
    };
    symbol_table->add_symbol(symbol_table, &local_symbol);
    
    // 查找符号（考虑作用域）
    Stru_SymbolInfo_t found_symbol;
    
    // 在函数作用域中查找全局符号
    if (symbol_table->lookup_symbol_in_scope(symbol_table, "全局变量", function_scope, &found_symbol)) {
        printf("在函数作用域中找到全局符号: %s\n", found_symbol.name);
    }
    
    // 离开作用域
    scope_manager->leave_scope(scope_manager); // 离开函数作用域
    scope_manager->leave_scope(scope_manager); // 离开全局作用域
}
```

## 错误处理

### 错误类型

作用域管理器操作可能遇到以下类型的错误：

1. **内存错误**：内存分配失败
2. **参数错误**：无效的参数（如NULL指针）
3. **作用域错误**：作用域不存在、作用域嵌套错误
4. **状态错误**：无效的操作顺序（如离开不存在的父作用域）
5. **容量错误**：超出作用域管理器容量

### 错误处理策略

1. **返回值检查**：所有函数都返回bool表示成功/失败
2. **状态验证**：验证操作前的状态是否有效
3. **资源清理**：失败时自动清理已分配的资源
4. **错误恢复**：提供错误恢复机制（如作用域回滚）

## 性能考虑

### 时间复杂度

1. **进入作用域**：O(1)
2. **离开作用域**：O(1)
3. **获取当前作用域**：O(1)
4. **获取作用域信息**：O(1) 平均，O(n) 最坏情况
5. **作用域关系检查**：O(d)，其中d为作用域深度差
6. **作用域遍历**：O(n)，其中n为作用域数量

### 空间复杂度

1. **作用域存储**：每个作用域约32-64字节
2. **作用域栈**：每个栈帧约8-16字节
3. **作用域信息缓存**：额外内存用于缓存作用域信息

### 优化建议

1. **作用域池**：重用作用域对象减少内存分配
2. **作用域ID映射**：使用数组快速访问作用域信息
3. **作用域路径压缩**：压缩作用域路径表示
4. **作用域缓存**：缓存常用作用域查询结果
5. **批量操作**：支持批量作用域创建和销毁

## 线程安全性

### 线程安全级别

作用域管理器接口是**非线程安全**的。多个线程不能同时访问同一个作用域管理器实例。

### 线程安全使用模式

```c
// 每个线程创建自己的作用域管理器实例
void* scope_manager_thread(void* arg) {
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    scope_manager->initialize(scope_manager);
    
    // ... 使用作用域管理器 ...
    
    scope_manager->destroy(scope_manager);
    return NULL;
}

// 或者使用线程局部存储
__thread Stru_ScopeManagerInterface_t* thread_local_scope_manager = NULL;

Stru_ScopeManagerInterface_t* get_thread_scope_manager() {
    if (thread_local_scope_manager == NULL) {
        thread_local_scope_manager = F_create_scope_manager_interface();
        thread_local_scope_manager->initialize(thread_local_scope_manager);
    }
    return thread_local_scope_manager;
}
```

## 扩展指南

### 添加新的作用域类型

1. 扩展`Eum_ScopeType`枚举
2. 更新作用域创建函数支持新类型
3. 提供类型特定的作用域管理功能
4. 更新作用域查询
