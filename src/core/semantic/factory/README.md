# CN_Language 语义分析工厂模块

## 概述

工厂模块是CN_Language语义分析系统的入口点，提供统一的工厂函数来创建所有语义分析组件。本模块遵循依赖注入和工厂模式，简化语义分析系统的创建和配置。

## 功能特性

- **统一创建接口**：提供统一的工厂函数创建所有语义分析组件
- **依赖管理**：自动管理组件间的依赖关系
- **配置简化**：简化复杂系统的配置过程
- **内存管理**：确保正确的内存管理和资源释放
- **错误处理**：提供统一的错误处理机制

## 接口定义

### 工厂函数

工厂模块提供以下工厂函数：

```c
// 创建符号表接口实例
Stru_SymbolTableInterface_t* F_create_symbol_table_interface(void);

// 创建作用域管理器接口实例
Stru_ScopeManagerInterface_t* F_create_scope_manager_interface(void);

// 创建类型检查器接口实例
Stru_TypeCheckerInterface_t* F_create_type_checker_interface(void);

// 创建错误报告器接口实例
Stru_SemanticErrorReporterInterface_t* F_create_semantic_error_reporter_interface(void);

// 创建语义分析器接口实例
Stru_SemanticAnalyzerInterface_t* F_create_semantic_analyzer_interface(
    Stru_SemanticErrorReporterInterface_t* error_reporter);

// 创建完整的语义分析系统
Stru_SemanticAnalyzerInterface_t* F_create_complete_semantic_analyzer_system(size_t max_errors);
```

### 销毁函数

每个创建函数都有对应的销毁函数：

```c
// 销毁符号表接口实例
void F_destroy_symbol_table_interface(Stru_SymbolTableInterface_t* interface);

// 销毁作用域管理器接口实例
void F_destroy_scope_manager_interface(Stru_ScopeManagerInterface_t* interface);

// 销毁类型检查器接口实例
void F_destroy_type_checker_interface(Stru_TypeCheckerInterface_t* interface);

// 销毁错误报告器接口实例
void F_destroy_semantic_error_reporter_interface(
    Stru_SemanticErrorReporterInterface_t* interface);

// 销毁语义分析器接口实例
void F_destroy_semantic_analyzer_interface(Stru_SemanticAnalyzerInterface_t* interface);
```

## 使用示例

### 基本使用

```c
#include "CN_semantic_factory.h"

int main() {
    // 创建完整的语义分析系统（最大100个错误）
    Stru_SemanticAnalyzerInterface_t* analyzer = 
        F_create_complete_semantic_analyzer_system(100);
    
    if (!analyzer) {
        printf("创建语义分析系统失败\n");
        return 1;
    }
    
    printf("语义分析系统创建成功\n");
    
    // 使用语义分析器...
    
    // 销毁语义分析系统
    F_destroy_semantic_analyzer_interface(analyzer);
    
    return 0;
}
```

### 手动创建组件

```c
void manual_component_creation(void) {
    // 创建错误报告器
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    
    if (!error_reporter) {
        printf("创建错误报告器失败\n");
        return;
    }
    
    // 初始化错误报告器
    if (!error_reporter->initialize(error_reporter, (void*)50)) {
        F_destroy_semantic_error_reporter_interface(error_reporter);
        printf("初始化错误报告器失败\n");
        return;
    }
    
    // 创建符号表
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    if (!symbol_table) {
        F_destroy_semantic_error_reporter_interface(error_reporter);
        printf("创建符号表失败\n");
        return;
    }
    
    // 初始化符号表
    if (!symbol_table->initialize(symbol_table, NULL)) {
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        printf("初始化符号表失败\n");
        return;
    }
    
    // 创建作用域管理器
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    if (!scope_manager) {
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        printf("创建作用域管理器失败\n");
        return;
    }
    
    // 初始化作用域管理器
    if (!scope_manager->initialize(scope_manager, NULL)) {
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        printf("初始化作用域管理器失败\n");
        return;
    }
    
    // 创建类型检查器
    Stru_TypeCheckerInterface_t* type_checker = F_create_type_checker_interface();
    if (!type_checker) {
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        printf("创建类型检查器失败\n");
        return;
    }
    
    // 初始化类型检查器
    if (!type_checker->initialize(type_checker, NULL)) {
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        printf("初始化类型检查器失败\n");
        return;
    }
    
    // 创建语义分析器
    Stru_SemanticAnalyzerInterface_t* analyzer = 
        F_create_semantic_analyzer_interface(error_reporter);
    
    if (!analyzer) {
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        printf("创建语义分析器失败\n");
        return;
    }
    
    // 配置语义分析器
    // analyzer->set_symbol_table(analyzer, symbol_table);
    // analyzer->set_scope_manager(analyzer, scope_manager);
    // analyzer->set_type_checker(analyzer, type_checker);
    
    printf("所有组件创建成功\n");
    
    // 使用组件...
    
    // 销毁所有组件（注意销毁顺序）
    F_destroy_semantic_analyzer_interface(analyzer);
    F_destroy_type_checker_interface(type_checker);
    F_destroy_scope_manager_interface(scope_manager);
    F_destroy_symbol_table_interface(symbol_table);
    F_destroy_semantic_error_reporter_interface(error_reporter);
}
```

### 配置自定义组件

```c
void custom_component_configuration(void) {
    // 创建自定义配置的错误报告器
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    
    // 自定义配置
    typedef struct {
        size_t max_errors;
        bool enable_warnings;
        void* user_data;
    } CustomConfig;
    
    CustomConfig config = {
        .max_errors = 200,
        .enable_warnings = true,
        .user_data = NULL
    };
    
    if (error_reporter->initialize(error_reporter, &config)) {
        printf("自定义错误报告器配置成功\n");
    }
    
    // 创建使用自定义错误报告器的语义分析器
    Stru_SemanticAnalyzerInterface_t* analyzer = 
        F_create_semantic_analyzer_interface(error_reporter);
    
    if (analyzer) {
        printf("使用自定义配置的语义分析器创建成功\n");
        
        // 使用分析器...
        
        F_destroy_semantic_analyzer_interface(analyzer);
    }
    
    F_destroy_semantic_error_reporter_interface(error_reporter);
}
```

## 实现细节

### 工厂模式实现

工厂模块使用简单的工厂模式实现：

```c
// 符号表工厂函数实现
Stru_SymbolTableInterface_t* F_create_symbol_table_interface(void) {
    Stru_SymbolTableInterface_t* interface = 
        (Stru_SymbolTableInterface_t*)malloc(sizeof(Stru_SymbolTableInterface_t));
    
    if (!interface) {
        return NULL;
    }
    
    // 设置接口函数指针
    interface->initialize = F_symbol_table_initialize;
    interface->insert_symbol = F_symbol_table_insert_symbol;
    interface->lookup_symbol = F_symbol_table_lookup_symbol;
    interface->remove_symbol = F_symbol_table_remove_symbol;
    interface->update_symbol = F_symbol_table_update_symbol;
    interface->get_symbol_count = F_symbol_table_get_symbol_count;
    interface->set_parent_scope = F_symbol_table_set_parent_scope;
    interface->get_parent_scope = F_symbol_table_get_parent_scope;
    interface->destroy = F_symbol_table_destroy;
    
    // 分配私有数据
    interface->private_data = malloc(sizeof(Stru_SymbolTablePrivateData_t));
    if (!interface->private_data) {
        free(interface);
        return NULL;
    }
    
    // 初始化私有数据
    memset(interface->private_data, 0, sizeof(Stru_SymbolTablePrivateData_t));
    
    return interface;
}
```

### 依赖管理

工厂模块自动管理组件间的依赖关系：

1. **错误报告器**：独立组件，不依赖其他组件
2. **符号表**：独立组件，可设置父作用域
3. **作用域管理器**：依赖符号表（每个作用域一个符号表）
4. **类型检查器**：依赖作用域管理器和类型系统
5. **语义分析器**：依赖所有其他组件

### 内存管理

工厂模块确保正确的内存管理：

1. **创建时分配**：分配接口结构和私有数据
2. **初始化时配置**：根据配置初始化私有数据
3. **销毁时释放**：释放所有分配的内存
4. **引用计数**：管理类型系统的引用计数

## 配置选项

### 错误报告器配置

```c
// 通过初始化参数配置
void* config = (void*)100; // 最大错误数量

// 或使用配置结构
typedef struct {
    size_t max_errors;
    bool store_call_stack;
} ErrorReporterConfig;
```

### 符号表配置

```c
typedef struct {
    size_t initial_capacity;
    void* (*malloc_func)(size_t);
    void (*free_func)(void*);
} SymbolTableConfig;
```

### 作用域管理器配置

```c
typedef struct {
    size_t initial_stack_capacity;
    Stru_SymbolTableInterface_t* (*create_symbol_table_func)(void);
} ScopeManagerConfig;
```

### 类型检查器配置

```c
typedef struct {
    Stru_TypeSystem_t* type_system;
    bool strict_mode;
    bool enable_warnings;
} TypeCheckerConfig;
```

## 错误处理

工厂模块提供以下错误处理机制：

1. **内存分配失败**：返回NULL
2. **初始化失败**：返回NULL或false
3. **无效参数**：返回NULL或false
4. **依赖循环**：检测并避免依赖循环

## 测试策略

### 单元测试

```c
// 测试工厂函数创建
void test_factory_creation(void) {
    // 测试符号表创建
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    TEST_ASSERT_NOT_NULL(symbol_table);
    TEST_ASSERT_TRUE(symbol_table->initialize(symbol_table, NULL));
    F_destroy_symbol_table_interface(symbol_table);
    
    // 测试错误报告器创建
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    TEST_ASSERT_NOT_NULL(error_reporter);
    TEST_ASSERT_TRUE(error_reporter->initialize(error_reporter, (void*)10));
    F_destroy_semantic_error_reporter_interface(error_reporter);
    
    // 测试完整系统创建
    Stru_SemanticAnalyzerInterface_t* analyzer = 
        F_create_complete_semantic_analyzer_system(50);
    TEST_ASSERT_NOT_NULL(analyzer);
    F_destroy_semantic_analyzer_interface(analyzer);
}
```

### 集成测试

- 测试组件间依赖关系
- 测试内存泄漏
- 测试错误恢复
- 测试配置选项

## 性能优化建议

1. **对象池**：重用组件实例，减少创建开销
2. **延迟初始化**：延迟初始化直到第一次使用
3. **配置缓存**：缓存常用配置，减少配置开销
4. **批量创建**：支持批量创建相关组件

## 扩展指南

### 添加新的工厂函数

1. 实现新的组件接口
2. 添加对应的工厂函数
3. 实现销毁函数
4. 更新工厂模块头文件
5. 添加测试用例

### 自定义工厂实现

1. 实现自定义的工厂函数
2. 管理自定义的依赖关系
3. 提供自定义的配置选项
4. 确保内存管理正确

## 相关模块

- **符号表模块**：工厂创建符号表实例
- **作用域管理器**：工厂创建作用域管理器实例
- **类型检查器**：工厂创建类型检查器实例
- **错误报告器**：工厂创建错误报告器实例
- **语义分析器**：工厂创建语义分析器实例

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本 |
| 2.0.0 | 2026-01-09 | 模块化重构 |
| 2.0.1 | 2026-01-10 | 修复递归调用问题 |

## 维护信息

- **最后更新**：2026年1月10日
- **维护者**：CN_Language语义分析团队
- **架构版本**：2.0.0
- **兼容性**：向后兼容所有版本

## 许可证

本模块遵循MIT许可证。详细信息请参阅项目根目录下的LICENSE文件。
