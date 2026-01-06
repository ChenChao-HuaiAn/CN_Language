# CN_Language AST模块

## 概述

AST（抽象语法树）模块是CN_Language编译器的核心组件之一，负责表示和操作程序的语法结构。本模块采用分层架构和SOLID设计原则，提供高度模块化、可扩展的AST实现。

## 设计原则

### 1. 单一职责原则
- 每个子模块只负责一个功能领域
- 每个.c文件不超过500行代码
- 每个函数不超过50行代码

### 2. 开闭原则
- 通过抽象接口支持扩展
- 模块对修改封闭，对扩展开放

### 3. 依赖倒置原则
- 高层模块定义接口
- 低层模块实现接口
- 通过依赖注入管理依赖关系

## 模块结构

### 核心接口 (`CN_ast_interface.h`)
定义AST模块的所有抽象接口，包括：
- `Stru_AstNodeInterface_t`: AST节点接口
- `Stru_AstBuilderInterface_t`: AST构建器接口
- `Stru_AstTraversalInterface_t`: AST遍历器接口
- `Stru_AstQueryInterface_t`: AST查询器接口
- `Stru_AstSerializerInterface_t`: AST序列化器接口

### 具体实现
- `CN_ast_node.c/h`: AST节点具体实现
- `CN_ast_builder.c/h`: AST构建器具体实现
- `CN_ast_traversal.c/h`: AST遍历器具体实现
- `CN_ast_query.c/h`: AST查询器具体实现
- `CN_ast_serializer.c/h`: AST序列化器具体实现

### 兼容层 (`CN_ast.h`)
提供向后兼容的API，包装新的接口实现，确保现有代码可以继续使用。

## 节点类型

AST支持以下节点类型：

### 程序结构
- `Eum_AST_PROGRAM`: 程序根节点
- `Eum_AST_MODULE`: 模块定义
- `Eum_AST_IMPORT`: 导入语句

### 声明
- `Eum_AST_VARIABLE_DECL`: 变量声明
- `Eum_AST_FUNCTION_DECL`: 函数声明
- `Eum_AST_STRUCT_DECL`: 结构体声明
- `Eum_AST_ENUM_DECL`: 枚举声明
- `Eum_AST_CONSTANT_DECL`: 常量声明
- `Eum_AST_PARAMETER_DECL`: 参数声明

### 语句
- `Eum_AST_EXPRESSION_STMT`: 表达式语句
- `Eum_AST_IF_STMT`: if语句
- `Eum_AST_WHILE_STMT`: while语句
- `Eum_AST_FOR_STMT`: for语句
- `Eum_AST_RETURN_STMT`: return语句
- `Eum_AST_BREAK_STMT`: break语句
- `Eum_AST_CONTINUE_STMT`: continue语句
- `Eum_AST_BLOCK_STMT`: 代码块语句
- `Eum_AST_SWITCH_STMT`: switch语句
- `Eum_AST_CASE_STMT`: case语句
- `Eum_AST_DEFAULT_STMT`: default语句

### 表达式
- `Eum_AST_BINARY_EXPR`: 二元表达式
- `Eum_AST_UNARY_EXPR`: 一元表达式
- `Eum_AST_LITERAL_EXPR`: 字面量表达式
- `Eum_AST_IDENTIFIER_EXPR`: 标识符表达式
- `Eum_AST_CALL_EXPR`: 函数调用表达式
- `Eum_AST_INDEX_EXPR`: 数组索引表达式
- `Eum_AST_MEMBER_EXPR`: 成员访问表达式
- `Eum_AST_ASSIGN_EXPR`: 赋值表达式
- `Eum_AST_COMPOUND_ASSIGN_EXPR`: 复合赋值表达式
- `Eum_AST_CAST_EXPR`: 类型转换表达式
- `Eum_AST_CONDITIONAL_EXPR`: 条件表达式（三元运算符）
- `Eum_AST_NEW_EXPR`: 对象创建表达式
- `Eum_AST_DELETE_EXPR`: 对象销毁表达式

### 类型
- `Eum_AST_TYPE_NAME`: 类型名称
- `Eum_AST_ARRAY_TYPE`: 数组类型
- `Eum_AST_POINTER_TYPE`: 指针类型
- `Eum_AST_REFERENCE_TYPE`: 引用类型
- `Eum_AST_FUNCTION_TYPE`: 函数类型

### 字面量
- `Eum_AST_INT_LITERAL`: 整数字面量
- `Eum_AST_FLOAT_LITERAL`: 浮点数字面量
- `Eum_AST_STRING_LITERAL`: 字符串字面量
- `Eum_AST_BOOL_LITERAL`: 布尔字面量
- `Eum_AST_ARRAY_LITERAL`: 数组字面量
- `Eum_AST_STRUCT_LITERAL`: 结构体字面量
- `Eum_AST_NULL_LITERAL`: null字面量

## 使用示例

### 创建AST节点
```c
#include "CN_ast_interface.h"

// 创建AST构建器
Stru_AstBuilderInterface_t* builder = F_create_ast_builder_interface();
builder->initialize(builder);

// 创建位置信息
Stru_AstNodeLocation_t location = {
    .line = 1,
    .column = 1,
    .file_name = "test.cn",
    .start_offset = 0,
    .end_offset = 10
};

// 创建标识符节点
Stru_AstNodeInterface_t* identifier = builder->create_identifier_node(builder, &location, "myVariable");

// 创建整数字面量节点
Uni_AstNodeData_t data = { .int_value = 42 };
Stru_AstNodeInterface_t* literal = builder->create_literal_node(builder, Eum_AST_INT_LITERAL, &location, &data);

// 构建赋值表达式
Stru_AstNodeInterface_t* assignment = builder->build_binary_expression(builder, &location, 
    OPERATOR_ASSIGN, identifier, literal);

// 清理资源
identifier->destroy(identifier);
literal->destroy(literal);
assignment->destroy(assignment);
builder->destroy(builder);
```

### 遍历AST树
```c
#include "CN_ast_interface.h"

void print_node(Stru_AstNodeInterface_t* node, void* context) {
    const Stru_AstNodeLocation_t* loc = node->get_location(node);
    printf("Node at line %zu, column %zu\n", loc->line, loc->column);
}

// 创建遍历器
Stru_AstTraversalInterface_t* traversal = F_create_ast_traversal_interface();
traversal->initialize(traversal);

// 深度优先遍历
traversal->depth_first_traversal(traversal, root, print_node, NULL, NULL, NULL);

// 清理资源
traversal->destroy(traversal);
```

## 依赖关系

- 基础设施层: `CN_dynamic_array.h` (动态数组)
- 核心层: `CN_token.h` (令牌定义)

## 测试策略

### 单元测试
- 测试每个接口函数的正确性
- 测试边界条件和错误处理
- 测试内存管理（无内存泄漏）

### 集成测试
- 测试模块间的协作
- 测试AST构建、遍历、查询的完整流程

### 性能测试
- 测试大规模AST的性能
- 测试内存使用情况

## 扩展指南

### 添加新的节点类型
1. 在 `CN_ast_interface.h` 的 `Eum_AstNodeType` 枚举中添加新类型
2. 在相应的构建器函数中支持新类型的创建
3. 在序列化器中支持新类型的序列化和反序列化

### 添加新的遍历算法
1. 实现新的遍历器接口
2. 注册到工厂函数中
3. 更新文档和测试

## 版本历史

- v1.0.0 (2026-01-06): 初始版本，基于接口的模块化设计
- v0.x.x: 旧版本，基于具体实现的简单设计

## 维护者

CN_Language架构委员会
