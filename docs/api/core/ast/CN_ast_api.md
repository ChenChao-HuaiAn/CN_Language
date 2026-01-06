# CN_Language 抽象语法树（AST）模块 API 文档

## 概述

抽象语法树（AST）模块是CN_Language编译器的核心数据结构，用于表示程序的语法结构。本模块采用分层架构和SOLID设计原则，提供高度模块化、可扩展的AST实现。

### 主要功能

1. **AST节点定义**：定义所有CN语言语法结构的节点类型
2. **抽象接口设计**：基于接口的模块化设计，支持可替换实现
3. **节点创建和管理**：创建、复制、销毁AST节点
4. **树结构操作**：添加、获取、移除子节点
5. **节点属性管理**：设置和获取节点的自定义属性
6. **数据操作**：设置和获取节点的类型化数据
7. **树遍历和查询**：多种遍历算法和查询功能
8. **序列化支持**：JSON、XML、二进制格式的序列化和反序列化
9. **验证和调试**：验证AST结构，格式化输出

### 架构设计

- **设计模式**：接口隔离、依赖倒置、策略模式
- **模块化程度**：每个功能领域独立接口和实现
- **扩展性**：通过接口实现支持功能扩展
- **测试友好**：接口支持模拟测试

### 架构位置

- **所属层级**：核心层（Core Layer）
- **依赖模块**：基础设施层（内存管理、容器）、词法分析器（令牌）
- **被依赖模块**：语法分析器、语义分析器、代码生成器、优化器

## 核心接口

### 接口概览

AST模块采用接口隔离原则，将不同功能分解为独立的接口：

1. **`Stru_AstNodeInterface_t`**：AST节点基本操作接口
2. **`Stru_AstBuilderInterface_t`**：AST构建器接口
3. **`Stru_AstTraversalInterface_t`**：AST遍历器接口
4. **`Stru_AstQueryInterface_t`**：AST查询器接口
5. **`Stru_AstSerializerInterface_t`**：AST序列化器接口

### 工厂函数

所有接口实例通过工厂函数创建：

```c
// 创建接口实例
Stru_AstNodeInterface_t* F_create_ast_node_interface(void);
Stru_AstBuilderInterface_t* F_create_ast_builder_interface(void);
Stru_AstTraversalInterface_t* F_create_ast_traversal_interface(void);
Stru_AstQueryInterface_t* F_create_ast_query_interface(void);
Stru_AstSerializerInterface_t* F_create_ast_serializer_interface(void);

// 销毁接口实例
void F_destroy_ast_node_interface(Stru_AstNodeInterface_t* interface);
void F_destroy_ast_builder_interface(Stru_AstBuilderInterface_t* interface);
void F_destroy_ast_traversal_interface(Stru_AstTraversalInterface_t* interface);
void F_destroy_ast_query_interface(Stru_AstQueryInterface_t* interface);
void F_destroy_ast_serializer_interface(Stru_AstSerializerInterface_t* interface);
```

## 核心数据结构

### Eum_AstNodeType

AST节点类型枚举，定义了CN_Language支持的所有语法结构：

```c
typedef enum Eum_AstNodeType {
    // ============================================
    // 程序结构
    // ============================================
    Eum_AST_PROGRAM,              ///< 程序根节点
    Eum_AST_MODULE,               ///< 模块定义
    Eum_AST_IMPORT,               ///< 导入语句
    
    // ============================================
    // 声明
    // ============================================
    Eum_AST_VARIABLE_DECL,        ///< 变量声明
    Eum_AST_FUNCTION_DECL,        ///< 函数声明
    Eum_AST_STRUCT_DECL,          ///< 结构体声明
    Eum_AST_ENUM_DECL,            ///< 枚举声明
    Eum_AST_CONSTANT_DECL,        ///< 常量声明
    Eum_AST_PARAMETER_DECL,       ///< 参数声明
    
    // ============================================
    // 语句
    // ============================================
    Eum_AST_EXPRESSION_STMT,      ///< 表达式语句
    Eum_AST_IF_STMT,              ///< if语句
    Eum_AST_WHILE_STMT,           ///< while语句
    Eum_AST_FOR_STMT,             ///< for语句
    Eum_AST_RETURN_STMT,          ///< return语句
    Eum_AST_BREAK_STMT,           ///< break语句
    Eum_AST_CONTINUE_STMT,        ///< continue语句
    Eum_AST_BLOCK_STMT,           ///< 代码块语句
    Eum_AST_SWITCH_STMT,          ///< switch语句
    Eum_AST_CASE_STMT,            ///< case语句
    Eum_AST_DEFAULT_STMT,         ///< default语句
    
    // ============================================
    // 表达式
    // ============================================
    Eum_AST_BINARY_EXPR,          ///< 二元表达式
    Eum_AST_UNARY_EXPR,           ///< 一元表达式
    Eum_AST_LITERAL_EXPR,         ///< 字面量表达式
    Eum_AST_IDENTIFIER_EXPR,      ///< 标识符表达式
    Eum_AST_CALL_EXPR,            ///< 函数调用表达式
    Eum_AST_INDEX_EXPR,           ///< 数组索引表达式
    Eum_AST_MEMBER_EXPR,          ///< 成员访问表达式
    Eum_AST_ASSIGN_EXPR,          ///< 赋值表达式
    Eum_AST_COMPOUND_ASSIGN_EXPR, ///< 复合赋值表达式
    Eum_AST_CAST_EXPR,            ///< 类型转换表达式
    Eum_AST_CONDITIONAL_EXPR,     ///< 条件表达式（三元运算符）
    Eum_AST_NEW_EXPR,             ///< 对象创建表达式
    Eum_AST_DELETE_EXPR,          ///< 对象销毁表达式
    
    // ============================================
    // 类型
    // ============================================
    Eum_AST_TYPE_NAME,            ///< 类型名称
    Eum_AST_ARRAY_TYPE,           ///< 数组类型
    Eum_AST_POINTER_TYPE,         ///< 指针类型
    Eum_AST_REFERENCE_TYPE,       ///< 引用类型
    Eum_AST_FUNCTION_TYPE,        ///< 函数类型
    
    // ============================================
    // 字面量
    // ============================================
    Eum_AST_INT_LITERAL,          ///< 整数字面量
    Eum_AST_FLOAT_LITERAL,        ///< 浮点数字面量
    Eum_AST_STRING_LITERAL,       ///< 字符串字面量
    Eum_AST_BOOL_LITERAL,         ///< 布尔字面量
    Eum_AST_ARRAY_LITERAL,        ///< 数组字面量
    Eum_AST_STRUCT_LITERAL,       ///< 结构体字面量
    Eum_AST_NULL_LITERAL,         ///< null字面量
    
    // ============================================
    // 错误处理
    // ============================================
    Eum_AST_ERROR_NODE,           ///< 错误节点（用于错误恢复）
    
    // ============================================
    // 特殊节点
    // ============================================
    Eum_AST_COMMENT,              ///< 注释节点
    Eum_AST_DIRECTIVE,            ///< 预处理指令节点
    
    Eum_AST_COUNT                 ///< AST节点类型总数（用于边界检查）
} Eum_AstNodeType;
```

### Uni_AstNodeData_t

AST节点数据联合体，根据节点类型存储相应的数据：

```c
typedef union Uni_AstNodeData_t {
    // 字面量值
    long int_value;                   ///< 整数值
    double float_value;               ///< 浮点数值
    char* string_value;               ///< 字符串值
    bool bool_value;                  ///< 布尔值
    
    // 类型信息
    char* type_name;                  ///< 类型名称
    size_t array_size;                ///< 数组大小
    
    // 其他数据
    char* identifier;                 ///< 标识符名称
    int operator_type;                ///< 运算符类型
    
    // 通用指针
    void* custom_data;                ///< 自定义数据
} Uni_AstNodeData_t;
```

### Stru_AstNodeLocation_t

AST节点位置信息结构体：

```c
typedef struct Stru_AstNodeLocation_t {
    size_t line;                      ///< 行号（从1开始）
    size_t column;                    ///< 列号（从1开始）
    const char* file_name;            ///< 文件名
    size_t start_offset;              ///< 起始偏移量
    size_t end_offset;                ///< 结束偏移量
} Stru_AstNodeLocation_t;
```

## 接口详细说明

### Stru_AstNodeInterface_t

AST节点接口，表示抽象语法树中的一个节点：

#### 主要方法

**节点信息获取**
- `get_type()`: 获取节点类型
- `get_location()`: 获取节点位置信息
- `get_data()`: 获取节点数据

**树结构操作**
- `get_parent()`: 获取父节点
- `set_parent()`: 设置父节点
- `get_child_count()`: 获取子节点数量
- `get_child()`: 获取子节点
- `add_child()`: 添加子节点
- `remove_child()`: 移除子节点

**属性管理**
- `get_attribute()`: 获取属性值
- `set_attribute()`: 设置属性值
- `remove_attribute()`: 移除属性
- `has_attribute()`: 检查是否有属性

**生命周期管理**
- `copy()`: 复制节点（深拷贝）
- `destroy()`: 销毁节点

### Stru_AstBuilderInterface_t

AST构建器接口，负责创建和构建AST节点：

#### 主要方法

**初始化**
- `initialize()`: 初始化AST构建器

**节点创建**
- `create_node()`: 创建通用AST节点
- `create_literal_node()`: 创建字面量节点
- `create_identifier_node()`: 创建标识符节点
- `create_type_node()`: 创建类型节点
- `create_operator_node()`: 创建运算符节点

**复杂节点构建**
- `build_program_node()`: 构建程序节点
- `build_function_declaration()`: 构建函数声明节点
- `build_variable_declaration()`: 构建变量声明节点
- `build_expression_statement()`: 构建表达式语句节点
- `build_if_statement()`: 构建if语句节点
- `build_while_statement()`: 构建while语句节点
- `build_for_statement()`: 构建for语句节点
- `build_return_statement()`: 构建return语句节点
- `build_binary_expression()`: 构建二元表达式节点
- `build_unary_expression()`: 构建一元表达式节点
- `build_call_expression()`: 构建函数调用表达式节点

**资源管理**
- `destroy()`: 销毁AST构建器

### Stru_AstTraversalInterface_t

AST遍历器接口，提供AST树的遍历功能：

#### 主要方法

**初始化**
- `initialize()`: 初始化AST遍历器

**遍历算法**
- `depth_first_traversal()`: 深度优先遍历（支持前序、中序、后序）
- `breadth_first_traversal()`: 广度优先遍历

**节点查找**
- `find_nodes()`: 查找满足条件的节点
- `get_match_count()`: 获取匹配节点数量

**资源管理**
- `destroy()`: 销毁AST遍历器

### Stru_AstQueryInterface_t

AST查询器接口，提供AST树的查询功能：

#### 主要方法

**初始化**
- `initialize()`: 初始化AST查询器

**统计查询**
- `count_node_types()`: 统计节点类型
- `find_nodes_with_attribute()`: 查找具有特定属性的节点

**结构查询**
- `get_node_depth()`: 获取节点深度
- `get_subtree_size()`: 获取子树大小

**验证**
- `validate_ast_structure()`: 验证AST结构

**资源管理**
- `destroy()`: 销毁AST查询器

### Stru_AstSerializerInterface_t

AST序列化器接口，提供AST树的序列化和反序列化功能：

#### 主要方法

**初始化**
- `initialize()`: 初始化AST序列化器

**序列化**
- `serialize_to_json()`: 序列化AST到JSON
- `serialize_to_xml()`: 序列化AST到XML
- `serialize_to_binary()`: 序列化AST到二进制格式

**反序列化**
- `deserialize_from_json()`: 从JSON反序列化AST
- `deserialize_from_xml()`: 从XML反序列化AST
- `deserialize_from_binary()`: 从二进制格式反序列化AST

**资源管理**
- `destroy()`: 销毁AST序列化器

## 使用示例

### 示例1：使用接口创建AST

```c
#include "src/core/ast/CN_ast_interface.h"

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

// 设置自定义属性
assignment->set_attribute(assignment, "优化级别", (void*)2);
assignment->set_attribute(assignment, "是否需要括号", (void*)true);

// 获取节点信息
Eum_AstNodeType type = assignment->get_type(assignment);
const Stru_AstNodeLocation_t* loc = assignment->get_location(assignment);
printf("节点类型: %d, 位置: 行%zu, 列%zu\n", type, loc->line, loc->column);

// 清理资源
identifier->destroy(identifier);
literal->destroy(literal);
assignment->destroy(assignment);
builder->destroy(builder);
```

### 示例2：遍历和查询AST

```c
#include "src/core/ast/CN_ast_interface.h"

// 创建遍历器
Stru_AstTraversalInterface_t* traversal = F_create_ast_traversal_interface();
traversal->initialize(traversal);

// 创建查询器
Stru_AstQueryInterface_t* query = F_create_ast_query_interface();
query->initialize(query);

// 谓词函数：查找所有变量声明
bool find_variable_decls(Stru_AstNodeInterface_t* node, void* context) {
    (void)context;
    return node->get_type(node) == Eum_AST_VARIABLE_DECL;
}

// 访问者函数：打印节点信息
void print_node_info(Stru_AstNodeInterface_t* node, void* context) {
    (void)context;
    const Stru_AstNodeLocation_t* loc = node->get_location(node);
    printf("节点在行%zu, 列%zu\n", loc->line, loc->column);
}

// 假设已经有一个AST树
Stru_AstNodeInterface_t* ast_root = ...;

// 深度优先遍历
printf("深度优先遍历：\n");
traversal->depth_first_traversal(traversal, ast_root, print_node_info, NULL, NULL, NULL);

// 查找所有变量声明
printf("\n查找变量声明：\n");
Stru_AstNodeInterface_t** var_decls = traversal->find_nodes(traversal, ast_root, find_variable_decls, NULL);
size_t match_count = traversal->get_match_count(traversal);

for (size_t i = 0; i < match_count; i++) {
    Stru_AstNodeInterface_t* var_decl = var_decls[i];
    const Uni_AstNodeData_t* data = var_decl->get_data(var_decl);
    printf("变量声明: %s\n", data->identifier);
}

// 统计节点类型
printf("\n节点类型统计：\n");
size_t type_counts[Eum_AST_COUNT] = {0};
size_t count = query->count_node_types(query, ast_root, type_counts, Eum_AST_COUNT);

for (size_t i = 0; i < count; i++) {
    if (type_counts[i] > 0) {
        printf("类型 %zu: %zu 个节点\n", i, type_counts[i]);
    }
}

// 验证AST结构
if (query->validate_ast_structure(query, ast_root)) {
    printf("\nAST结构有效\n");
} else {
    printf("\nAST结构无效\n");
}

// 清理资源
traversal->destroy(traversal);
query->destroy(query);
```

### 示例3：序列化和反序列化

```c
#include "src/core/ast/CN_ast_interface.h"

// 创建序列化器
Stru_AstSerializerInterface_t* serializer = F_create_ast_serializer_interface();
serializer->initialize(serializer);

// 假设已经
