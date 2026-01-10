# AST遍历和分析模块

## 概述

本模块实现了CN_Language编译器的AST遍历和分析功能，是语义分析器的核心组件。主要负责符号解析和绑定、AST节点类型分析、作用域管理等功能。模块采用分层架构设计，遵循SOLID设计原则，提供高度模块化和可扩展的接口。

## 功能特性

### 符号解析和绑定
1. **符号解析**：在符号表中查找标识符，验证符号的存在性和可访问性
2. **符号绑定**：将声明的符号（变量、函数、结构体等）绑定到符号表
3. **作用域管理**：管理符号的作用域，支持嵌套作用域
4. **重复声明检测**：检测同一作用域内的重复符号声明
5. **未定义符号检测**：检测使用未定义的符号

### AST节点分析
1. **程序节点分析**：分析整个程序结构
2. **声明节点分析**：分析变量、函数、结构体等声明
3. **语句节点分析**：分析各种语句类型
4. **表达式节点分析**：分析各种表达式类型
5. **类型分析**：分析表达式的类型信息

### 错误检测和报告
1. **语义错误检测**：检测各种语义错误
2. **错误位置跟踪**：记录错误发生的具体位置
3. **错误分类**：对错误进行分类和分级
4. **错误过滤**：支持按类型、位置等条件过滤错误

## 模块结构

### 头文件
- `CN_ast_analyzer.h`：定义AST分析器接口和数据结构
- `CN_semantic_analyzer.h`：定义语义分析器接口和数据结构

### 源文件
- `CN_ast_analyzer.c`：实现AST分析器功能
- `CN_semantic_analyzer.c`：实现语义分析器功能

### 数据结构
1. **AST分析器接口**：
   - `Stru_AstAnalyzerInterface_t`：AST分析器接口
   - `Stru_AstAnalysisContext_t`：AST分析上下文

2. **语义分析器接口**：
   - `Stru_SemanticAnalyzerInterface_t`：语义分析器接口

3. **分析结果**：
   - `Eum_AstNodeType`：AST节点类型枚举
   - `Eum_SemanticErrorType`：语义错误类型枚举

## 接口说明

### AST分析器接口

```c
// 初始化AST分析器
bool initialize(Stru_AstAnalyzerInterface_t* analyzer,
               Stru_SemanticAnalyzerInterface_t* semantic_analyzer);

// 分析AST节点
bool analyze_ast(Stru_AstAnalyzerInterface_t* analyzer,
                Stru_AstNodeInterface_t* ast_node);

// 分析程序节点
bool analyze_program(Stru_AstAnalyzerInterface_t* analyzer,
                    Stru_AstNodeInterface_t* node,
                    Stru_AstAnalysisContext_t* context);

// 分析变量声明节点
bool analyze_variable_declaration(Stru_AstAnalyzerInterface_t* analyzer,
                                 Stru_AstNodeInterface_t* node,
                                 Stru_AstAnalysisContext_t* context);

// 分析函数声明节点
bool analyze_function_declaration(Stru_AstAnalyzerInterface_t* analyzer,
                                 Stru_AstNodeInterface_t* node,
                                 Stru_AstAnalysisContext_t* context);

// 分析结构体声明节点
bool analyze_struct_declaration(Stru_AstAnalyzerInterface_t* analyzer,
                               Stru_AstNodeInterface_t* node,
                               Stru_AstAnalysisContext_t* context);

// 分析标识符表达式节点（符号解析）
void* analyze_identifier_expression(Stru_AstAnalyzerInterface_t* analyzer,
                                   Stru_AstNodeInterface_t* node,
                                   Stru_AstAnalysisContext_t* context);

// 销毁AST分析器
void destroy(Stru_AstAnalyzerInterface_t* analyzer);
```

### 语义分析器接口

```c
// 获取符号表接口
Stru_SymbolTableInterface_t* get_symbol_table(
    Stru_SemanticAnalyzerInterface_t* analyzer);

// 获取作用域管理器接口
Stru_ScopeManagerInterface_t* get_scope_manager(
    Stru_SemanticAnalyzerInterface_t* analyzer);

// 获取错误报告器接口
Stru_SemanticErrorReporterInterface_t* get_error_reporter(
    Stru_SemanticAnalyzerInterface_t* analyzer);

// 获取类型检查器接口
Stru_TypeCheckerInterface_t* get_type_checker(
    Stru_SemanticAnalyzerInterface_t* analyzer);
```

## 使用示例

```c
// 创建语义分析器系统
Stru_SemanticAnalyzerInterface_t* semantic_analyzer = 
    F_create_complete_semantic_analyzer_system_optimized(100);

// 创建AST分析器
Stru_AstAnalyzerInterface_t* ast_analyzer = F_create_ast_analyzer_interface();

// 初始化AST分析器
bool init_success = ast_analyzer->initialize(ast_analyzer, semantic_analyzer);
if (!init_success) {
    // 处理初始化失败
    return;
}

// 分析AST
bool analysis_success = ast_analyzer->analyze_ast(ast_analyzer, ast_root);
if (!analysis_success) {
    // 处理分析失败
    // 可以获取错误信息
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        semantic_analyzer->get_error_reporter(semantic_analyzer);
    size_t error_count = error_reporter->get_error_count(error_reporter);
    
    // 过滤和分类错误
    size_t filtered_count = error_reporter->filter_errors(
        error_reporter, Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL, 0, 100, "test.cn");
    
    // 获取错误严重性
    for (size_t i = 0; i < filtered_count; i++) {
        Stru_SemanticError_t* error = error_reporter->get_error(error_reporter, i);
        Eum_ErrorSeverity severity = error_reporter->get_error_severity(
            error_reporter, error->type);
        // 处理错误
    }
}

// 清理资源
ast_analyzer->destroy(ast_analyzer);
F_destroy_ast_analyzer_interface(ast_analyzer);
F_destroy_semantic_analyzer_interface(semantic_analyzer);
```

## 符号解析和绑定算法

### 符号绑定流程
1. **变量声明绑定**：
   - 检查变量名称是否合法
   - 检查同一作用域内是否已存在同名符号
   - 分析变量类型和初始化表达式
   - 将变量信息插入符号表
   - 设置符号的作用域和生命周期信息

2. **函数声明绑定**：
   - 检查函数名称是否合法
   - 检查同一作用域内是否已存在同名符号
   - 进入函数作用域
   - 分析函数参数和返回类型
   - 分析函数体
   - 退出函数作用域
   - 将函数信息插入符号表

3. **结构体声明绑定**：
   - 检查结构体名称是否合法
   - 检查同一作用域内是否已存在同名符号
   - 进入结构体作用域
   - 分析结构体成员
   - 退出结构体作用域
   - 将结构体信息插入符号表

### 符号解析流程
1. **标识符解析**：
   - 获取标识符名称
   - 在当前作用域中查找符号
   - 如果未找到，在父作用域中查找
   - 如果找到，返回符号的类型信息
   - 如果未找到，报告未定义符号错误

2. **作用域链查找**：
   - 从当前作用域开始查找
   - 沿作用域链向上查找
   - 支持跨作用域的符号访问
   - 遵循作用域嵌套规则

## 错误检测

### 符号相关错误
1. **未定义符号错误**：使用未声明的标识符
2. **重复声明错误**：在同一作用域内重复声明同名符号
3. **作用域违规错误**：违反作用域嵌套规则
4. **生命周期错误**：变量在生命周期外被使用

### 类型相关错误
1. **类型不匹配错误**：表达式类型不匹配
2. **无效操作错误**：对不支持的类型进行操作
3. **参数不匹配错误**：函数调用参数不匹配
4. **返回类型错误**：返回语句类型与函数声明不匹配

### 控制流错误
1. **不可达代码错误**：永远无法执行到的代码
2. **缺少返回语句错误**：函数可能没有返回值
3. **死代码错误**：执行后不会影响程序状态的代码

## 性能优化

### 符号表优化
1. **哈希表实现**：使用FNV-1a哈希算法和开放寻址法
2. **动态扩容**：根据负载因子自动调整哈希表大小
3. **缓存机制**：使用LRU缓存最近访问的符号
4. **作用域局部性优化**：优化同一作用域内的符号查找

### 分析过程优化
1. **增量分析**：只分析变更的部分
2. **结果缓存**：缓存分析结果，避免重复计算
3. **并行分析**：支持并行分析不同的AST子树
4. **延迟分析**：延迟非必要的分析操作

## 测试策略

### 单元测试
1. **符号绑定测试**：验证符号绑定的正确性
2. **符号解析测试**：验证符号解析的准确性
3. **错误检测测试**：验证各种错误的正确检测
4. **作用域测试**：验证作用域管理的正确性

### 集成测试
1. **完整程序分析测试**：验证整个分析流程
2. **复杂作用域测试**：验证嵌套作用域的处理
3. **性能测试**：验证分析性能
4. **内存测试**：验证内存使用情况

## 依赖关系

### 内部依赖
- `src/core/semantic/symbol_table/`：符号表管理
- `src/core/semantic/scope_manager/`：作用域管理
- `src/core/semantic/type_checker/`：类型检查
- `src/core/semantic/error_reporter/`：错误报告
- `src/core/semantic/flow_analysis/`：控制流和数据流分析
- `src/core/semantic/constant_folding/`：常量折叠
- `src/core/semantic/factory/`：工厂模式创建

### 外部依赖
- `src/core/ast/`：AST接口
- C标准库：`stdlib.h`，`string.h`，`stdbool.h`

## 设计原则

### 单一职责原则
- 每个函数不超过50行代码
- 每个文件不超过500行代码
- AST分析器只负责AST遍历和分析
- 语义分析器只负责协调各个组件

### 开闭原则
- 通过接口支持扩展
- 支持添加新的AST节点类型分析
- 支持不同的分析策略

### 依赖倒置原则
- 高层模块定义接口
- 低层模块实现接口
- 通过依赖注入管理组件依赖

### 接口隔离原则
- 为不同客户端提供专用接口
- AST分析器接口与语义分析器接口分离
- 最小化接口依赖

### 里氏替换原则
- 接口实现完全遵守契约
- 子类可无缝替换父类
- 通过单元测试验证替换性

## 扩展性

### 新的AST节点类型
可以通过实现相应的分析函数来支持新的AST节点类型，无需修改现有代码。

### 自定义分析策略
支持通过配置选择不同的分析策略和优化级别。

### 插件架构
支持通过插件添加自定义的分析功能。

## 维护说明

### 代码规范
- 遵循项目编码标准
- 使用统一的命名约定（Stru_前缀表示结构体，F_前缀表示函数）
- 添加充分的注释和文档
- 每个函数前添加函数说明注释

### 版本管理
- 使用语义化版本控制
- 保持向后兼容性
- 明确记录变更历史

### 问题跟踪
- 记录已知问题和限制
- 跟踪性能瓶颈
- 收集用户反馈

## 参考资料

1. 《编译原理》（龙书）：语义分析的基本理论
2. 《现代编译原理》：语义分析的实际实现
3. CN_Language项目架构文档：项目整体架构设计
4. CN_Language语法规范：语言语法定义

## 版本历史

### v1.0.0 (2026-01-10)
- 初始版本
- 实现AST遍历和分析的基本框架
- 实现符号解析和绑定功能
- 提供完整的接口定义和基本实现
- 集成错误检测和报告功能

### 未来计划
- 实现更完整的类型分析
- 添加更多的语义检查
- 提高分析性能和准确性
- 支持更多的优化功能

---

*文档版本：1.0.0*
*创建日期：2026-01-10*
*维护者：CN_Language架构委员会*
