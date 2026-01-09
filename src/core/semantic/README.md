# CN_Language 语义分析模块

## 概述

语义分析模块是CN_Language项目的核心组件之一，负责对抽象语法树（AST）进行语义验证和分析。本模块采用分层架构和SOLID设计原则，提供可扩展、可替换的语义分析实现。

## 模块结构（更新于2026-01-09）

```
src/core/semantic/
├── CN_semantic_interface.h          # 语义分析主接口文件（包含所有子模块头文件）
├── README.md                        # 本文件
├── analyzer/                        # 主语义分析器模块
│   ├── CN_semantic_analyzer.h       # 语义分析器接口定义
│   └── CN_semantic_analyzer.c       # 语义分析器实现
├── error_reporter/                  # 语义错误报告器模块
│   ├── CN_error_reporter.h          # 错误报告器接口定义
│   └── CN_error_reporter.c          # 错误报告器实现
├── factory/                         # 工厂模块
│   ├── CN_semantic_factory.h        # 工厂接口定义
│   └── CN_semantic_factory.c        # 工厂实现
├── scope_manager/                   # 作用域管理器模块
│   ├── CN_scope_manager.h           # 作用域管理器接口定义
│   └── CN_scope_manager.c           # 作用域管理器实现
├── symbol_table/                    # 符号表模块
│   ├── CN_symbol_table.h            # 符号表接口定义
│   └── CN_symbol_table.c            # 符号表实现
└── type_checker/                    # 类型检查器模块
    ├── CN_type_checker.h            # 类型检查器接口定义
    └── CN_type_checker.c            # 类型检查器实现
```

## 接口设计

语义分析模块包含以下主要接口：

### 1. Stru_SymbolTableInterface_t（符号表模块）
符号表管理接口，提供：
- 符号的添加、查找和删除
- 作用域链管理
- 符号信息存储（类型、值、作用域等）
- 符号表遍历和查询

### 2. Stru_ScopeManagerInterface_t（作用域管理器模块）
作用域管理器接口，提供：
- 作用域的创建和销毁
- 作用域栈管理
- 作用域层次遍历
- 作用域内符号查找

### 3. Stru_TypeCheckerInterface_t（类型检查器模块）
类型检查器接口，提供：
- 表达式类型推断
- 类型兼容性检查
- 函数参数类型验证
- 类型转换规则应用

### 4. Stru_SemanticErrorReporterInterface_t（错误报告器模块）
语义错误报告接口，提供：
- 错误信息的收集和报告
- 错误严重性分级
- 错误位置跟踪
- 错误回调机制

### 5. Stru_SemanticAnalyzerInterface_t（主语义分析器模块）
主语义分析器接口，整合：
- 符号表管理
- 作用域分析
- 类型检查
- 错误报告
- 提供完整的语义分析功能

### 6. 工厂模块
提供统一的创建接口：
- `F_create_symbol_table_interface()` - 创建符号表实例
- `F_create_scope_manager_interface()` - 创建作用域管理器实例
- `F_create_type_checker_interface()` - 创建类型检查器实例
- `F_create_semantic_error_reporter_interface()` - 创建错误报告器实例
- `F_create_semantic_analyzer_interface()` - 创建语义分析器实例
- `F_create_complete_semantic_analyzer_system()` - 创建完整的语义分析系统

## 使用示例

### 基本使用

```c
#include "src/core/semantic/CN_semantic_interface.h"

int main() {
    // 创建完整的语义分析器系统
    Stru_SemanticAnalyzerInterface_t* analyzer = 
        F_create_complete_semantic_analyzer_system(100); // 最大100个错误
    
    if (analyzer == NULL) {
        printf("创建语义分析器失败\n");
        return 1;
    }
    
    // 进行语义分析
    void* ast_root = get_ast_root(); // 假设有AST根节点
    bool success = analyzer->analyze_ast(analyzer, ast_root);
    
    if (!success) {
        // 获取错误信息
        Stru_SemanticErrorReporterInterface_t* error_reporter = 
            analyzer->get_error_reporter(analyzer);
        
        size_t error_count = error_reporter->get_error_count(error_reporter);
        printf("发现 %zu 个语义错误\n", error_count);
        
        // 可以获取具体错误信息
        Stru_SemanticError_t errors[10];
        size_t actual_count = error_reporter->get_all_errors(error_reporter, errors, 10);
        
        for (size_t i = 0; i < actual_count; i++) {
            printf("错误 %zu: %s (行: %zu, 列: %zu)\n", 
                   i, errors[i].message, errors[i].line, errors[i].column);
        }
    }
    
    // 销毁语义分析器
    F_destroy_semantic_analyzer_interface(analyzer);
    
    return 0;
}
```

### 单独使用符号表

```c
#include "src/core/semantic/symbol_table/CN_symbol_table.h"

void symbol_table_example(void) {
    // 创建符号表
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    
    if (symbol_table == NULL) {
        return;
    }
    
    // 初始化符号表
    if (!symbol_table->initialize(symbol_table, NULL)) {
        F_destroy_symbol_table_interface(symbol_table);
        return;
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
    if (found != NULL) {
        printf("找到符号: %s\n", found->name);
    }
    
    // 销毁符号表
    F_destroy_symbol_table_interface(symbol_table);
}
```

### 类型检查示例

```c
#include "src/core/semantic/type_checker/CN_type_checker.h"
#include "src/core/semantic/scope_manager/CN_scope_manager.h"

void type_check_example(void) {
    // 创建作用域管理器
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    if (scope_manager == NULL) {
        return;
    }
    
    // 创建类型检查器
    Stru_TypeCheckerInterface_t* type_checker = F_create_type_checker_interface();
    if (type_checker == NULL) {
        F_destroy_scope_manager_interface(scope_manager);
        return;
    }
    
    // 初始化类型检查器
    if (!type_checker->initialize(type_checker, scope_manager)) {
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        return;
    }
    
    // 检查二元表达式类型
    void* int_type = get_int_type(); // 假设有获取类型信息的函数
    void* float_type = get_float_type();
    
    Stru_TypeCheckResult_t result = type_checker->check_binary_expression(
        type_checker, int_type, float_type, OPERATOR_ADD);
    
    if (result.compatibility == Eum_TYPE_COMPATIBLE) {
        printf("类型兼容\n");
    } else if (result.compatibility == Eum_TYPE_CONVERTIBLE) {
        printf("类型可转换\n");
    } else {
        printf("类型不兼容: %s\n", result.error_message);
    }
    
    // 销毁资源
    F_destroy_type_checker_interface(type_checker);
    F_destroy_scope_manager_interface(scope_manager);
}
```

## 设计原则

### 1. 单一职责原则
- 每个模块只负责一个功能领域
- 每个.c文件不超过500行
- 每个函数不超过50行
- 每个头文件只暴露必要的接口

### 2. 开闭原则
- 通过抽象接口支持扩展
- 新的类型检查策略、符号表实现或错误报告机制可以通过实现相应接口来添加
- 模块对修改封闭，对扩展开放

### 3. 依赖倒置原则
- 高层模块定义接口
- 低层模块实现接口
- 通过工厂函数创建实例
- 通过依赖注入管理依赖关系

### 4. 接口隔离原则
- 为不同客户端提供专用接口
- 避免"胖接口"
- 通过接口组合提供灵活的功能集
- 模块间通过接口指针通信

### 5. 里氏替换原则
- 接口实现可以无缝替换
- 所有实现遵守相同的接口契约
- 通过单元测试验证替换性
- 子类可无缝替换父类

## 模块化优势

### 1. 解耦简单
- 每个模块独立编译和测试
- 模块间通过接口通信，减少直接依赖
- 易于替换实现

### 2. 可读性高
- 清晰的模块边界
- 每个文件功能单一
- 统一的命名规范

### 3. 易于测试
- 每个模块可独立测试
- 测试功能在单独的文件中
- 支持桩实现和模拟对象

### 4. 可维护性强
- 修改一个模块不影响其他模块
- 清晰的依赖关系
- 完善的文档

## 配置选项

语义分析器支持以下配置：

```c
// 通过工厂函数配置
Stru_SemanticAnalyzerInterface_t* analyzer = 
    F_create_complete_semantic_analyzer_system(100); // 最大100个错误
```

## 性能考虑

### 内存使用
- 每个接口实例内存开销小
- 符号表使用动态数组，内存效率高
- 作用域管理器使用栈结构，内存开销小

### 分析性能
- 符号查找：O(n) 线性查找（可优化为哈希表）
- 类型检查：O(1) 常量时间操作
- 作用域管理：O(1) 栈操作

### 线程安全
- 接口实例不是线程安全的
- 多线程使用需要外部同步
- 建议每个分析任务创建独立的分析器实例

## 扩展指南

### 添加新的类型系统
1. 扩展类型描述符结构
2. 实现新的类型检查规则
3. 更新类型兼容性检查函数
4. 提供类型转换函数

### 添加新的符号表实现
1. 创建新的符号表接口实现
2. 实现符号的添加、查找和删除
3. 支持作用域链查找
4. 提供工厂函数

### 添加新的错误报告机制
1. 扩展错误报告接口
2. 实现新的错误收集和报告策略
3. 支持错误过滤和分类
4. 保持向后兼容性

## 测试策略

### 单元测试
- 每个模块有独立的测试文件
- 测试每个接口函数
- 测试错误处理
- 测试边界条件

### 集成测试
- 测试模块间协作
- 测试工厂函数
- 测试完整分析流程
- 测试错误报告机制

### 端到端测试
- 测试完整程序语义分析
- 测试复杂类型系统
- 测试作用域嵌套
- 测试错误恢复

## 相关文档

- [API文档](../../docs/api/core/semantic/) - 各模块的API文档
- [架构设计文档](../../../docs/architecture/001-中文编程语言CN_Language开发规划.md)
- [编码规范](../../../docs/specifications/CN_Language项目 技术规范和编码标准.md)
- [目录结构规范](../../../docs/specifications/CN_Language项目 目录结构规范.md)
- [AST模块文档](../ast/README.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本发布 |
| 2.0.0 | 2026-01-09 | 模块化重构，拆分为多个子模块 |
| 2.0.1 | 2026-01-09 | 修复工厂模块递归调用问题 |

## 维护信息

- **最后更新**：2026年1月9日
- **维护者**：CN_Language语义分析团队
- **架构版本**：2.0.0
- **兼容性**：向后兼容1.0.0版本接口

## 许可证

本模块遵循MIT许可证。详细信息请参阅项目根目录下的LICENSE文件。
