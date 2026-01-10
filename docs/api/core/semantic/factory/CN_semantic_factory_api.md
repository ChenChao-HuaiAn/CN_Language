# CN_Language 语义分析器工厂模块 API 文档

## 概述

工厂模块是CN_Language语义分析系统的核心组件之一，负责创建和管理语义分析系统的各个组件实例。本模块提供完整的工厂函数，支持依赖注入、配置管理和资源管理，为语义分析系统提供灵活的组件创建和生命周期管理能力。

## 工厂函数定义

### F_create_semantic_analyzer_interface

创建主语义分析器接口实例。

**函数签名：**
```c
Stru_SemanticAnalyzerInterface_t* F_create_semantic_analyzer_interface(void);
```

**返回值：**
- `Stru_SemanticAnalyzerInterface_t*`：新创建的语义分析器接口实例
- `NULL`：创建失败（内存不足）

**示例：**
```c
Stru_SemanticAnalyzerInterface_t* analyzer = F_create_semantic_analyzer_interface();
if (analyzer == NULL) {
    fprintf(stderr, "无法创建语义分析器\n");
    return 1;
}

// 使用语义分析器...

analyzer->destroy(analyzer);
```

### F_create_symbol_table_interface

创建符号表接口实例。

**函数签名：**
```c
Stru_SymbolTableInterface_t* F_create_symbol_table_interface(void);
```

**返回值：**
- `Stru_SymbolTableInterface_t*`：新创建的符号表接口实例
- `NULL`：创建失败（内存不足）

### F_create_scope_manager_interface

创建作用域管理器接口实例。

**函数签名：**
```c
Stru_ScopeManagerInterface_t* F_create_scope_manager_interface(void);
```

**返回值：**
- `Stru_ScopeManagerInterface_t*`：新创建的作用域管理器接口实例
- `NULL`：创建失败（内存不足）

### F_create_type_checker_interface

创建类型检查器接口实例。

**函数签名：**
```c
Stru_TypeCheckerInterface_t* F_create_type_checker_interface(void);
```

**返回值：**
- `Stru_TypeCheckerInterface_t*`：新创建的类型检查器接口实例
- `NULL`：创建失败（内存不足）

### F_create_semantic_error_reporter_interface

创建语义错误报告接口实例。

**函数签名：**
```c
Stru_SemanticErrorReporterInterface_t* F_create_semantic_error_reporter_interface(void);
```

**返回值：**
- `Stru_SemanticErrorReporterInterface_t*`：新创建的语义错误报告接口实例
- `NULL`：创建失败（内存不足）

### F_create_complete_semantic_analyzer_system

创建完整的语义分析系统。

**函数签名：**
```c
Stru_SemanticAnalyzerInterface_t* F_create_complete_semantic_analyzer_system(const char* config);
```

**参数：**
- `config`：系统配置字符串（JSON格式），可为NULL使用默认配置

**返回值：**
- `Stru_SemanticAnalyzerInterface_t*`：新创建的完整语义分析系统
- `NULL`：创建失败

**配置示例：**
```json
{
    "analyzer": {
        "strict_mode": true,
        "enable_warnings": true,
        "type_inference": true
    },
    "symbol_table": {
        "initial_capacity": 1024,
        "enable_hash_table": true
    },
    "scope_manager": {
        "max_scopes": 100,
        "enable_scope_caching": true
    },
    "type_checker": {
        "type_rules_config": "{\"strict_mode\": true}",
        "enable_implicit_conversions": true
    },
    "error_reporter": {
        "max_errors": 100,
        "enable_callbacks": true
    }
}
```

## 销毁函数定义

### F_destroy_semantic_analyzer_interface

销毁语义分析器接口实例。

**函数签名：**
```c
void F_destroy_semantic_analyzer_interface(Stru_SemanticAnalyzerInterface_t* analyzer);
```

**参数：**
- `analyzer`：要销毁的语义分析器接口指针

### F_destroy_symbol_table_interface

销毁符号表接口实例。

**函数签名：**
```c
void F_destroy_symbol_table_interface(Stru_SymbolTableInterface_t* symbol_table);
```

**参数：**
- `symbol_table`：要销毁的符号表接口指针

### F_destroy_scope_manager_interface

销毁作用域管理器接口实例。

**函数签名：**
```c
void F_destroy_scope_manager_interface(Stru_ScopeManagerInterface_t* scope_manager);
```

**参数：**
- `scope_manager`：要销毁的作用域管理器接口指针

### F_destroy_type_checker_interface

销毁类型检查器接口实例。

**函数签名：**
```c
void F_destroy_type_checker_interface(Stru_TypeCheckerInterface_t* type_checker);
```

**参数：**
- `type_checker`：要销毁的类型检查器接口指针

### F_destroy_semantic_error_reporter_interface

销毁语义错误报告接口实例。

**函数签名：**
```c
void F_destroy_semantic_error_reporter_interface(Stru_SemanticErrorReporterInterface_t* error_reporter);
```

**参数：**
- `error_reporter`：要销毁的语义错误报告接口指针

### F_destroy_complete_semantic_analyzer_system

销毁完整的语义分析系统。

**函数签名：**
```c
void F_destroy_complete_semantic_analyzer_system(Stru_SemanticAnalyzerInterface_t* system);
```

**参数：**
- `system`：要销毁的完整语义分析系统指针

## 配置管理函数

### F_configure_semantic_analyzer

配置语义分析器。

**函数签名：**
```c
bool F_configure_semantic_analyzer(Stru_SemanticAnalyzerInterface_t* analyzer, const char* config);
```

**参数：**
- `analyzer`：语义分析器接口指针
- `config`：配置字符串（JSON格式）

**返回值：**
- `true`：配置成功
- `false`：配置失败

### F_get_semantic_analyzer_config

获取语义分析器配置。

**函数签名：**
```c
char* F_get_semantic_analyzer_config(Stru_SemanticAnalyzerInterface_t* analyzer);
```

**参数：**
- `analyzer`：语义分析器接口指针

**返回值：**
- `char*`：配置字符串（需要调用者释放）
- `NULL`：获取失败

## 组件访问函数

### F_get_symbol_table_from_analyzer

从语义分析器获取符号表接口。

**函数签名：**
```c
Stru_SymbolTableInterface_t* F_get_symbol_table_from_analyzer(Stru_SemanticAnalyzerInterface_t* analyzer);
```

**参数：**
- `analyzer`：语义分析器接口指针

**返回值：**
- `Stru_SymbolTableInterface_t*`：符号表接口指针
- `NULL`：获取失败

### F_get_scope_manager_from_analyzer

从语义分析器获取作用域管理器接口。

**函数签名：**
```c
Stru_ScopeManagerInterface_t* F_get_scope_manager_from_analyzer(Stru_SemanticAnalyzerInterface_t* analyzer);
```

**参数：**
- `analyzer`：语义分析器接口指针

**返回值：**
- `Stru_ScopeManagerInterface_t*`：作用域管理器接口指针
- `NULL`：获取失败

### F_get_type_checker_from_analyzer

从语义分析器获取类型检查器接口。

**函数签名：**
```c
Stru_TypeCheckerInterface_t* F_get_type_checker_from_analyzer(Stru_SemanticAnalyzerInterface_t* analyzer);
```

**参数：**
- `analyzer`：语义分析器接口指针

**返回值：**
- `Stru_TypeCheckerInterface_t*`：类型检查器接口指针
- `NULL`：获取失败

### F_get_error_reporter_from_analyzer

从语义分析器获取错误报告器接口。

**函数签名：**
```c
Stru_SemanticErrorReporterInterface_t* F_get_error_reporter_from_analyzer(Stru_SemanticAnalyzerInterface_t* analyzer);
```

**参数：**
- `analyzer`：语义分析器接口指针

**返回值：**
- `Stru_SemanticErrorReporterInterface_t*`：错误报告器接口指针
- `NULL`：获取失败

## 使用示例

### 基本使用模式

```c
#include "CN_semantic_factory.h"

int main() {
    // 创建完整的语义分析系统
    const char* config = "{\"analyzer\": {\"strict_mode\": true}}";
    Stru_SemanticAnalyzerInterface_t* analyzer = F_create_complete_semantic_analyzer_system(config);
    
    if (analyzer == NULL) {
        fprintf(stderr, "无法创建语义分析系统\n");
        return 1;
    }
    
    // 初始化语义分析器
    if (!analyzer->initialize(analyzer, NULL)) {
        F_destroy_complete_semantic_analyzer_system(analyzer);
        return 1;
    }
    
    // 使用语义分析器...
    
    // 清理
    F_destroy_complete_semantic_analyzer_system(analyzer);
    
    return 0;
}
```

### 手动创建和配置组件

```c
void manual_component_creation_example() {
    // 创建各个组件
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    Stru_TypeCheckerInterface_t* type_checker = F_create_type_checker_interface();
    Stru_SemanticErrorReporterInterface_t* error_reporter = F_create_semantic_error_reporter_interface();
    
    // 初始化各个组件
    if (symbol_table && scope_manager && type_checker && error_reporter) {
        symbol_table->initialize(symbol_table, 1024);
        scope_manager->initialize(scope_manager);
        
        const char* type_rules = "{\"strict_mode\": true}";
        type_checker->initialize(type_checker, type_rules);
        
        error_reporter->initialize(error_reporter, 100);
        
        // 创建语义分析器（假设有相关函数）
        Stru_SemanticAnalyzerInterface_t* analyzer = create_custom_analyzer(
            symbol_table, scope_manager, type_checker, error_reporter);
        
        if (analyzer) {
            // 使用自定义的语义分析器...
            
            // 清理（需要手动清理各个组件）
            analyzer->destroy(analyzer);
        }
    }
    
    // 清理各个组件
    if (symbol_table) F_destroy_symbol_table_interface(symbol_table);
    if (scope_manager) F_destroy_scope_manager_interface(scope_manager);
    if (type_checker) F_destroy_type_checker_interface(type_checker);
    if (error_reporter) F_destroy_semantic_error_reporter_interface(error_reporter);
}
```

### 依赖注入示例

```c
void dependency_injection_example() {
    // 创建自定义的符号表实现
    Stru_SymbolTableInterface_t* custom_symbol_table = create_custom_symbol_table_implementation();
    
    // 创建其他标准组件
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    Stru_TypeCheckerInterface_t* type_checker = F_create_type_checker_interface();
    Stru_SemanticErrorReporterInterface_t* error_reporter = F_create_semantic_error_reporter_interface();
    
    // 初始化组件
    if (custom_symbol_table && scope_manager && type_checker && error_reporter) {
        // 自定义符号表可能已经有不同的初始化方式
        initialize_custom_symbol_table(custom_symbol_table);
        
        scope_manager->initialize(scope_manager);
        
        const char* type_rules = "{\"custom_rules\": true}";
        type_checker->initialize(type_checker, type_rules);
        
        error_reporter->initialize(error_reporter, 100);
        
        // 创建使用自定义组件的语义分析器
        Stru_SemanticAnalyzerInterface_t* analyzer = create_analyzer_with_dependencies(
            custom_symbol_table, scope_manager, type_checker, error_reporter);
        
        if (analyzer) {
            printf("使用依赖注入创建的语义分析器\n");
            
            // 配置语义分析器
            const char* config = "{\"custom_mode\": true}";
            F_configure_semantic_analyzer(analyzer, config);
            
            // 使用语义分析器...
            
            // 清理
            analyzer->destroy(analyzer);
        }
    }
    
    // 清理（自定义组件需要自定义清理函数）
    destroy_custom_symbol_table(custom_symbol_table);
    if (scope_manager) F_destroy_scope_manager_interface(scope_manager);
    if (type_checker) F_destroy_type_checker_interface(type_checker);
    if (error_reporter) F_destroy_semantic_error_reporter_interface(error_reporter);
}
```

### 配置管理示例

```c
void configuration_management_example() {
    // 创建语义分析系统
    Stru_SemanticAnalyzerInterface_t* analyzer = F_create_complete_semantic_analyzer_system(NULL);
    
    if (analyzer == NULL) {
        return;
    }
    
    // 获取当前配置
    char* current_config = F_get_semantic_analyzer_config(analyzer);
    if (current_config) {
        printf("当前配置: %s\n", current_config);
        free(current_config);
    }
    
    // 更新配置
    const char* new_config = 
        "{"
        "  \"analyzer\": {"
        "    \"strict_mode\": false,"
        "    \"enable_warnings\": true"
        "  },"
        "  \"symbol_table\": {"
        "    \"initial_capacity\": 2048"
        "  }"
        "}";
    
    if (F_configure_semantic_analyzer(analyzer, new_config)) {
        printf("配置更新成功\n");
        
        // 获取更新后的配置
        char* updated_config = F_get_semantic_analyzer_config(analyzer);
        if (updated_config) {
            printf("更新后的配置: %s\n", updated_config);
            free(updated_config);
        }
    } else {
        printf("配置更新失败\n");
    }
    
    // 清理
    F_destroy_complete_semantic_analyzer_system(analyzer);
}
```

### 组件替换示例

```c
void component_replacement_example() {
    // 创建标准语义分析系统
    Stru_SemanticAnalyzerInterface_t* analyzer = F_create_complete_semantic_analyzer_system(NULL);
    
    if (analyzer == NULL) {
        return;
    }
    
    // 获取当前组件
    Stru_TypeCheckerInterface_t* original_type_checker = F_get_type_checker_from_analyzer(analyzer);
    
    if (original_type_checker) {
        printf("原始类型检查器: %p\n", (void*)original_type_checker);
        
        // 创建新的类型检查器实现
        Stru_TypeCheckerInterface_t* enhanced_type_checker = create_enhanced_type_checker();
        
        if (enhanced_type_checker) {
            // 替换类型检查器（假设有相关函数）
            if (replace_type_checker_in_analyzer(analyzer, enhanced_type_checker)) {
                printf("类型检查器替换成功\n");
                
                // 现在分析器使用增强的类型检查器
                Stru_TypeCheckerInterface_t* current_type_checker = F_get_type_checker_from_analyzer(analyzer);
                printf("当前类型检查器: %p\n", (void*)current_type_checker);
                
                // 清理原始类型检查器
                F_destroy_type_checker_interface(original_type_checker);
            } else {
                printf("类型检查器替换失败\n");
                F_destroy_type_checker_interface(enhanced_type_checker);
            }
        }
    }
    
    // 清理（注意：现在分析器中的类型检查器需要单独清理）
    F_destroy_complete_semantic_analyzer_system(analyzer);
}
```

## 错误处理

### 错误类型

工厂模块操作可能遇到以下类型的错误：

1. **内存错误**：内存分配失败
2. **参数错误**：无效的参数（如NULL指针）
3. **配置错误**：无效的配置字符串
4. **初始化错误**：组件初始化失败
5. **依赖错误**：组件依赖关系错误
6. **资源错误**：资源分配失败

### 错误处理策略

1. **返回值检查**：所有工厂函数返回NULL表示失败
2. **配置验证**：验证配置字符串的格式和内容
3. **资源清理**：失败时自动清理已分配的资源
4. **错误传播**：组件初始化错误传播到工厂函数
5. **安全销毁**：提供安全的销毁函数处理部分初始化的系统

## 性能考虑

### 创建性能

1. **完整系统创建**：O(n)，其中n为组件数量
2. **单个组件创建**：O(1)
3. **配置解析**：O(m)，其中m为配置字符串长度
4. **依赖注入**：O(d)，其中d为依赖关系深度

### 内存使用

1. **完整系统**：约1-2KB基础开销 + 各个组件内存
2. **配置管理**：额外内存存储配置字符串
3. **工厂缓存**：可选的组件实例缓存

### 优化建议

1. **组件池**：重用组件实例减少创建开销
2. **配置缓存**：缓存解析后的配置
3. **延迟初始化**：按需初始化组件
4. **批量创建**：支持批量组件创建
5. **内存池**：使用内存池减少内存分配开销

## 线程安全性

### 线程安全级别

工厂函数是**线程安全**的，可以同时从多个线程调用。但创建的组件实例本身可能不是线程安全的。

### 线程安全使用模式

```c
// 多个线程同时创建分析器
void* create_analyzer_thread(void* arg) {
    Stru_SemanticAnalyzerInterface_t*
