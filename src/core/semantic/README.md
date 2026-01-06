# CN_Language 语义分析模块

## 概述

语义分析模块是CN_Language项目的核心组件之一，负责对抽象语法树（AST）进行语义验证和分析。本模块采用分层架构和SOLID设计原则，提供可扩展、可替换的语义分析实现。

## 模块结构

```
src/core/semantic/
├── CN_semantic_interface.h      # 语义分析接口定义
├── CN_semantic_interface.c      # 语义分析接口实现
└── README.md                    # 本文件
```

## 接口设计

语义分析模块包含以下主要接口：

### 1. Stru_SymbolTableInterface_t
符号表管理接口，提供：
- 符号的添加、查找和删除
- 作用域链管理
- 符号信息存储（类型、值、作用域等）
- 符号表遍历和查询

### 2. Stru_ScopeManagerInterface_t
作用域管理器接口，提供：
- 作用域的创建和销毁
- 作用域栈管理
- 作用域层次遍历
- 作用域内符号查找

### 3. Stru_TypeCheckerInterface_t
类型检查器接口，提供：
- 表达式类型推断
- 类型兼容性检查
- 函数参数类型验证
- 类型转换规则应用

### 4. Stru_SemanticErrorReporterInterface_t
语义错误报告接口，提供：
- 错误信息的收集和报告
- 错误严重性分级
- 错误位置跟踪
- 错误回调机制

### 5. Stru_SemanticAnalyzerInterface_t
主语义分析器接口，整合：
- 符号表管理
- 作用域分析
- 类型检查
- 错误报告
- 提供完整的语义分析功能

## 使用示例

### 基本使用

```c
#include "CN_semantic_interface.h"

int main() {
    // 创建语义分析器
    Stru_SemanticAnalyzerInterface_t* analyzer = F_create_semantic_analyzer_interface();
    
    // 初始化语义分析器
    if (!analyzer->initialize(analyzer, "{\"max_errors\": 100, \"strict_mode\": true}")) {
        // 处理初始化失败
        return 1;
    }
    
    // 获取符号表接口
    Stru_SymbolTableInterface_t* symbol_table = analyzer->get_symbol_table(analyzer);
    
    // 添加符号
    Stru_SymbolInfo_t symbol = {
        .name = "变量名",
        .type = "整数",
        .scope_level = 0,
        .is_constant = false,
        .value = NULL
    };
    
    symbol_table->add_symbol(symbol_table, &symbol);
    
    // 进行语义分析
    Stru_ASTNode_t* ast_root = get_ast_root(); // 假设有AST根节点
    bool success = analyzer->analyze_ast(analyzer, ast_root);
    
    if (!success) {
        // 获取错误信息
        Stru_SemanticErrorReporterInterface_t* error_reporter = analyzer->get_error_reporter(analyzer);
        uint32_t error_count = error_reporter->get_error_count(error_reporter);
        
        for (uint32_t i = 0; i < error_count; i++) {
            Stru_SemanticError_t error;
            error_reporter->get_error(error_reporter, i, &error);
            printf("错误 %u: %s (行: %u, 列: %u)\n", 
                   i, error.message, error.line, error.column);
        }
    }
    
    // 销毁语义分析器
    analyzer->destroy(analyzer);
    
    return 0;
}
```

### 类型检查示例

```c
#include "CN_semantic_interface.h"
#include <stdio.h>

void type_check_example(Stru_SemanticAnalyzerInterface_t* analyzer) {
    // 获取类型检查器接口
    Stru_TypeCheckerInterface_t* type_checker = analyzer->get_type_checker(analyzer);
    
    // 创建类型描述符
    Stru_TypeDescriptor_t type1 = {
        .type_name = "整数",
        .type_category = Eum_TYPE_CATEGORY_PRIMITIVE,
        .size = 4,
        .is_signed = true
    };
    
    Stru_TypeDescriptor_t type2 = {
        .type_name = "浮点数",
        .type_category = Eum_TYPE_CATEGORY_PRIMITIVE,
        .size = 8,
        .is_signed = true
    };
    
    // 检查类型兼容性
    bool compatible = type_checker->check_type_compatibility(type_checker, &type1, &type2);
    
    if (compatible) {
        printf("类型兼容\n");
    } else {
        printf("类型不兼容\n");
    }
    
    // 推断表达式类型
    Stru_ASTNode_t* expression = create_expression_node(); // 假设有表达式节点
    Stru_TypeDescriptor_t inferred_type;
    
    if (type_checker->infer_expression_type(type_checker, expression, &inferred_type)) {
        printf("表达式类型: %s\n", inferred_type.type_name);
    }
}
```

### 作用域管理示例

```c
#include "CN_semantic_interface.h"

void scope_management_example(Stru_SemanticAnalyzerInterface_t* analyzer) {
    // 获取作用域管理器接口
    Stru_ScopeManagerInterface_t* scope_manager = analyzer->get_scope_manager(analyzer);
    
    // 进入新作用域
    scope_manager->enter_scope(scope_manager, "函数作用域");
    
    // 在当前作用域添加符号
    Stru_SymbolTableInterface_t* symbol_table = analyzer->get_symbol_table(analyzer);
    
    Stru_SymbolInfo_t local_var = {
        .name = "局部变量",
        .type = "整数",
        .scope_level = scope_manager->get_current_scope_level(scope_manager),
        .is_constant = false,
        .value = NULL
    };
    
    symbol_table->add_symbol(symbol_table, &local_var);
    
    // 查找符号（在当前作用域和父作用域中）
    Stru_SymbolInfo_t found_symbol;
    if (symbol_table->lookup_symbol(symbol_table, "局部变量", &found_symbol)) {
        printf("找到符号: %s, 类型: %s\n", found_symbol.name, found_symbol.type);
    }
    
    // 离开作用域
    scope_manager->leave_scope(scope_manager);
}
```

## 设计原则

### 1. 单一职责原则
- 每个接口只负责一个功能领域
- 实现文件不超过500行
- 函数不超过50行

### 2. 开闭原则
- 通过抽象接口支持扩展
- 新的类型检查策略、符号表实现或错误报告机制可以通过实现相应接口来添加

### 3. 依赖倒置原则
- 高层模块定义接口
- 低层模块实现接口
- 通过工厂函数创建实例

### 4. 接口隔离原则
- 为不同客户端提供专用接口
- 避免"胖接口"
- 通过接口组合提供灵活的功能集

### 5. 里氏替换原则
- 接口实现可以无缝替换
- 所有实现遵守相同的接口契约
- 通过单元测试验证替换性

## 配置选项

语义分析器支持以下配置选项（JSON格式）：

```json
{
    "max_errors": 100,               // 最大错误数量
    "strict_mode": true,             // 严格模式（禁止隐式类型转换）
    "enable_warnings": true,         // 是否启用警告
    "type_inference": true,          // 是否启用类型推断
    "scope_tracking": true,          // 是否启用作用域跟踪
    "symbol_table_size": 1024,       // 符号表初始大小
    "error_callback": null           // 错误回调函数（可选）
}
```

## 性能考虑

### 内存使用
- 每个接口实例约80-160字节
- 符号表内存取决于符号数量
- 作用域管理器使用栈结构，内存开销小

### 分析性能
- 符号查找：O(log n) 使用哈希表
- 类型检查：O(n) 取决于AST大小
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
- 测试每个接口函数
- 测试错误处理
- 测试边界条件
- 测试类型兼容性规则

### 集成测试
- 测试接口组合使用
- 测试配置选项
- 测试AST分析流程
- 测试错误报告机制

### 端到端测试
- 测试完整程序语义分析
- 测试复杂类型系统
- 测试作用域嵌套
- 测试错误恢复

## 相关文档

- [API文档](../../docs/api/core/semantic/CN_semantic_interface.md)
- [架构设计文档](../../../docs/architecture/001-中文编程语言CN_Language开发规划.md)
- [编码规范](../../../docs/specifications/CN_Language项目 技术规范和编码标准.md)
- [AST模块文档](../ast/README.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本发布 |
| 1.0.1 | 2026-01-06 | 修复符号表查找错误 |
| 1.1.0 | 2026-01-06 | 添加作用域管理器接口 |

## 维护信息

- **最后更新**：2026年1月6日
- **维护者**：CN_Language语义分析团队
- **联系方式**：semantic-team@cn-language.org

## 许可证

本模块遵循MIT许可证。详细信息请参阅项目根目录下的LICENSE文件。
