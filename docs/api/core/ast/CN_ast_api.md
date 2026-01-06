# CN_Language 抽象语法树（AST）模块 API 文档

## 概述

抽象语法树（AST）模块是CN_Language编译器的核心数据结构，用于表示程序的语法结构。AST是语法分析器的输出，也是语义分析器、代码生成器等后续阶段的输入。本模块提供了完整的AST节点定义、创建、操作和遍历功能。

### 主要功能

1. **AST节点定义**：定义所有CN语言语法结构的节点类型
2. **节点创建和管理**：创建、复制、销毁AST节点
3. **树结构操作**：添加、获取、移除子节点
4. **节点属性管理**：设置和获取节点的自定义属性
5. **数据操作**：设置和获取节点的类型化数据
6. **树遍历和查询**：遍历AST树，查找特定节点
7. **验证和调试**：验证AST结构，格式化输出

### 架构位置

- **所属层级**：核心层（Core Layer）
- **依赖模块**：基础设施层（内存管理、容器）、词法分析器（令牌）
- **被依赖模块**：语法分析器、语义分析器、代码生成器、优化器

## 核心数据结构

### Eum_AstNodeType

AST节点类型枚举，定义了CN_Language支持的所有语法结构：

```c
typedef enum Eum_AstNodeType {
    // 程序结构
    Eum_AST_PROGRAM,              // 程序根节点
    Eum_AST_MODULE,               // 模块定义
    Eum_AST_IMPORT,               // 导入语句
    
    // 声明
    Eum_AST_VARIABLE_DECL,        // 变量声明
    Eum_AST_FUNCTION_DECL,        // 函数声明
    Eum_AST_STRUCT_DECL,          // 结构体声明
    Eum_AST_ENUM_DECL,            // 枚举声明
    Eum_AST_CONSTANT_DECL,        // 常量声明
    Eum_AST_PARAMETER_DECL,       // 参数声明
    
    // 语句
    Eum_AST_EXPRESSION_STMT,      // 表达式语句
    Eum_AST_IF_STMT,              // if语句
    Eum_AST_WHILE_STMT,           // while语句
    Eum_AST_FOR_STMT,             // for语句
    Eum_AST_RETURN_STMT,          // return语句
    Eum_AST_BREAK_STMT,           // break语句
    Eum_AST_CONTINUE_STMT,        // continue语句
    Eum_AST_BLOCK_STMT,           // 代码块语句
    Eum_AST_SWITCH_STMT,          // switch语句
    Eum_AST_CASE_STMT,            // case语句
    Eum_AST_DEFAULT_STMT,         // default语句
    
    // 表达式
    Eum_AST_BINARY_EXPR,          // 二元表达式
    Eum_AST_UNARY_EXPR,           // 一元表达式
    Eum_AST_LITERAL_EXPR,         // 字面量表达式
    Eum_AST_IDENTIFIER_EXPR,      // 标识符表达式
    Eum_AST_CALL_EXPR,            // 函数调用表达式
    Eum_AST_INDEX_EXPR,           // 数组索引表达式
    Eum_AST_MEMBER_EXPR,          // 成员访问表达式
    Eum_AST_ASSIGN_EXPR,          // 赋值表达式
    Eum_AST_COMPOUND_ASSIGN_EXPR, // 复合赋值表达式
    Eum_AST_CAST_EXPR,            // 类型转换表达式
    Eum_AST_CONDITIONAL_EXPR,     // 条件表达式（三元运算符）
    Eum_AST_NEW_EXPR,             // 对象创建表达式
    Eum_AST_DELETE_EXPR,          // 对象销毁表达式
    
    // 类型
    Eum_AST_TYPE_NAME,            // 类型名称
    Eum_AST_ARRAY_TYPE,           // 数组类型
    Eum_AST_POINTER_TYPE,         // 指针类型
    Eum_AST_REFERENCE_TYPE,       // 引用类型
    Eum_AST_FUNCTION_TYPE,        // 函数类型
    
    // 字面量
    Eum_AST_INT_LITERAL,          // 整数字面量
    Eum_AST_FLOAT_LITERAL,        // 浮点数字面量
    Eum_AST_STRING_LITERAL,       // 字符串字面量
    Eum_AST_BOOL_LITERAL,         // 布尔字面量
    Eum_AST_ARRAY_LITERAL,        // 数组字面量
    Eum_AST_STRUCT_LITERAL,       // 结构体字面量
    Eum_AST_NULL_LITERAL,         // null字面量
    
    // 错误处理
    Eum_AST_ERROR_NODE,           // 错误节点（用于错误恢复）
    
    // 特殊节点
    Eum_AST_COMMENT,              // 注释节点
    Eum_AST_DIRECTIVE,            // 预处理指令节点
    
    Eum_AST_COUNT                 // AST节点类型总数（用于边界检查）
} Eum_AstNodeType;
```

### Stru_AstNode_t

AST节点结构体，表示抽象语法树中的一个节点：

```c
typedef struct Stru_AstNode_t {
    Eum_AstNodeType type;                 // 节点类型
    Stru_Token_t* token;                  // 关联的令牌（可选）
    size_t line;                          // 行号（从1开始）
    size_t column;                        // 列号（从1开始）
    
    Stru_DynamicArray_t* children;        // 子节点数组
    Stru_DynamicArray_t* attributes;      // 属性键值对数组
    
    // 节点数据联合体
    union {
        // 字面量值
        long int_value;                   // 整数值
        double float_value;               // 浮点数值
        char* string_value;               // 字符串值
        bool bool_value;                  // 布尔值
        
        // 类型信息
        char* type_name;                  // 类型名称
        size_t array_size;                // 数组大小
        
        // 其他数据
        char* identifier;                 // 标识符名称
        int operator_type;                // 运算符类型
    } data;
    
    struct Stru_AstNode_t* parent;        // 父节点指针（可选）
} Stru_AstNode_t;
```

## API函数参考

### AST节点创建和销毁

#### `F_create_ast_node`
- **功能**：创建AST节点
- **参数**：
  - `type`：节点类型
  - `token`：关联的令牌（可选）
  - `line`：行号
  - `column`：列号
- **返回值**：新创建的AST节点指针，失败返回NULL
- **内存管理**：调用者负责在不再使用时调用`F_destroy_ast_node`

#### `F_destroy_ast_node`
- **功能**：销毁AST节点
- **参数**：`node`：要销毁的AST节点指针
- **返回值**：无
- **注意**：会递归销毁所有子节点

#### `F_copy_ast_node`
- **功能**：复制AST节点
- **参数**：`node`：要复制的AST节点指针
- **返回值**：新复制的AST节点指针
- **注意**：执行深拷贝，包括所有子节点和属性

### AST节点操作

#### `F_ast_add_child`
- **功能**：向父节点添加子节点
- **参数**：
  - `parent`：父节点
  - `child`：子节点
- **返回值**：`true`表示添加成功，`false`表示失败
- **注意**：会自动设置子节点的`parent`指针

#### `F_ast_get_child_count`
- **功能**：获取子节点数量
- **参数**：`node`：节点
- **返回值**：子节点数量

#### `F_ast_get_child`
- **功能**：获取指定索引的子节点
- **参数**：
  - `node`：节点
  - `index`：子节点索引（0-based）
- **返回值**：子节点指针，索引无效返回NULL

#### `F_ast_remove_child`
- **功能**：移除指定索引的子节点
- **参数**：
  - `node`：节点
  - `index`：子节点索引
- **返回值**：被移除的子节点指针，索引无效返回NULL
- **注意**：只从父节点的子节点数组中移除，不销毁子节点

### AST节点属性操作

#### `F_ast_set_attribute`
- **功能**：设置节点属性
- **参数**：
  - `node`：节点
  - `key`：属性键（字符串）
  - `value`：属性值（任意指针）
- **返回值**：`true`表示设置成功，`false`表示失败
- **注意**：会复制键字符串，但不复制值

#### `F_ast_get_attribute`
- **功能**：获取节点属性
- **参数**：
  - `node`：节点
  - `key`：属性键
- **返回值**：属性值指针，不存在返回NULL

#### `F_ast_remove_attribute`
- **功能**：移除节点属性
- **参数**：
  - `node`：节点
  - `key`：属性键
- **返回值**：`true`表示移除成功，`false`表示失败

#### `F_ast_has_attribute`
- **功能**：检查节点是否有属性
- **参数**：
  - `node`：节点
  - `key`：属性键
- **返回值**：`true`表示有属性，`false`表示无属性

### AST节点数据操作

#### 设置函数

**`F_ast_set_int_value`**
- **功能**：设置整数值
- **参数**：`node`：节点，`value`：整数值

**`F_ast_set_float_value`**
- **功能**：设置浮点数值
- **参数**：`node`：节点，`value`：浮点数值

**`F_ast_set_string_value`**
- **功能**：设置字符串值
- **参数**：`node`：节点，`value`：字符串值
- **注意**：会复制字符串

**`F_ast_set_bool_value`**
- **功能**：设置布尔值
- **参数**：`node`：节点，`value`：布尔值

**`F_ast_set_identifier`**
- **功能**：设置标识符名称
- **参数**：`node`：节点，`name`：标识符名称
- **注意**：会复制字符串

**`F_ast_set_type_name`**
- **功能**：设置类型名称
- **参数**：`node`：节点，`type_name`：类型名称
- **注意**：会复制字符串

**`F_ast_set_array_size`**
- **功能**：设置数组大小
- **参数**：`node`：节点，`size`：数组大小

**`F_ast_set_operator_type`**
- **功能**：设置运算符类型
- **参数**：`node`：节点，`operator_type`：运算符类型

#### 获取函数

**`F_ast_get_int_value`**
- **功能**：获取整数值
- **参数**：`node`：节点
- **返回值**：整数值

**`F_ast_get_float_value`**
- **功能**：获取浮点数值
- **参数**：`node`：节点
- **返回值**：浮点数值

**`F_ast_get_string_value`**
- **功能**：获取字符串值
- **参数**：`node`：节点
- **返回值**：字符串值

**`F_ast_get_bool_value`**
- **功能**：获取布尔值
- **参数**：`node`：节点
- **返回值**：布尔值

**`F_ast_get_identifier`**
- **功能**：获取标识符名称
- **参数**：`node`：节点
- **返回值**：标识符名称

**`F_ast_get_type_name`**
- **功能**：获取类型名称
- **参数**：`node`：节点
- **返回值**：类型名称

**`F_ast_get_array_size`**
- **功能**：获取数组大小
- **参数**：`node`：节点
- **返回值**：数组大小

**`F_ast_get_operator_type`**
- **功能**：获取运算符类型
- **参数**：`node`：节点
- **返回值**：运算符类型

### AST遍历和查询

#### `F_ast_node_type_to_string`
- **功能**：AST节点类型转字符串
- **参数**：`type`：AST节点类型
- **返回值**：类型字符串表示

#### `F_ast_node_to_string`
- **功能**：打印AST节点信息
- **参数**：
  - `node`：AST节点
  - `buffer`：输出缓冲区
  - `buffer_size`：缓冲区大小
- **返回值**：写入的字符数（不包括终止空字符）

#### `F_ast_print_tree`
- **功能**：打印整个AST树
- **参数**：
  - `root`：AST根节点
  - `indent`：初始缩进级别
- **返回值**：无
- **输出格式**：
  ```
  Program
    FunctionDecl: main
      Parameter: argc
      Parameter: argv
      Block
        VariableDecl: x
          IntLiteral: 10
        Return
          Identifier: x
  ```

#### `F_ast_find_nodes`
- **功能**：在AST树中查找满足条件的节点
- **参数**：
  - `root`：根节点
  - `predicate`：谓词函数，返回true表示匹配
  - `context`：上下文参数（传递给谓词函数）
- **返回值**：匹配的节点数组，NULL表示无匹配
- **内存管理**：调用者负责销毁返回的数组

#### `F_ast_traverse`
- **功能**：遍历AST树并对每个节点执行操作
- **参数**：
  - `root`：根节点
  - `visitor`：访问者函数，对每个节点执行操作
  - `context`：上下文参数（传递给访问者函数）
- **返回值**：无
- **遍历顺序**：深度优先，先序遍历

### AST验证函数

#### `F_ast_validate_node`
- **功能**：验证AST节点的结构是否有效
- **参数**：`node`：要验证的节点
- **返回值**：`true`表示节点有效，`false`表示节点无效
- **验证内容**：
  - 节点类型是否有效
  - 必要字段是否设置
  - 子节点数量是否符合类型要求

#### `F_ast_validate_tree`
- **功能**：验证整个AST树的结构是否有效
- **参数**：`root`：根节点
- **返回值**：`true`表示AST树有效，`false`表示AST树无效
- **验证内容**：
  - 所有节点都有效
  - 树结构完整（无循环引用）
  - 类型特定的结构约束

## 使用示例

### 示例1：创建简单的AST

```c
#include "src/core/ast/CN_ast.h"

// 创建程序节点
Stru_AstNode_t* program = F_create_ast_node(Eum_AST_PROGRAM, NULL, 1, 1);

// 创建函数声明节点
Stru_AstNode_t* func_decl = F_create_ast_node(Eum_AST_FUNCTION_DECL, NULL, 2, 1);
F_ast_set_identifier(func_decl, "主函数");

// 创建参数节点
Stru_AstNode_t* param = F_create_ast_node(Eum_AST_PARAMETER_DECL, NULL, 2, 10);
F_ast_set_identifier(param, "参数");
F_ast_set_type_name(param, "字符串");

// 创建代码块节点
Stru_AstNode_t* block = F_create_ast_node(Eum_AST_BLOCK_STMT, NULL, 3, 5);

// 创建变量声明节点
Stru_AstNode_t* var_decl = F_create_ast_node(Eum_AST_VARIABLE_DECL, NULL, 4, 9);
F_ast_set_identifier(var_decl, "计数器");
F_ast_set_type_name(var_decl, "整数");

// 创建整数字面量节点
Stru_AstNode_t* int_literal = F_create_ast_node(Eum_AST_INT_LITERAL, NULL, 4, 18);
F_ast_set_int_value(int_literal, 0);

// 构建树结构
F_ast_add_child(func_decl, param);
F_ast_add_child(var_decl, int_literal);
F_ast_add_child(block, var_decl);
F_ast_add_child(func_decl, block);
F_ast_add_child(program, func_decl);

// 打印AST树
printf("生成的AST树：\n");
F_ast_print_tree(program, 0);

// 清理资源
F_destroy_ast_node(program);
```

### 示例2：遍历和查询AST

```c
#include "src/core/ast/CN_ast.h"

// 谓词函数：查找所有变量声明
bool find_variable_decls(const Stru_AstNode_t* node, void* context) {
    (void)context; // 未使用上下文
    return node->type == Eum_AST_VARIABLE_DECL;
}

// 访问者函数：打印节点信息
void print_node_info(Stru_AstNode_t* node, void* context) {
    (void)context; // 未使用上下文
    char buffer[256];
    F_ast_node_to_string(node, buffer, sizeof(buffer));
    printf("节点：%s\n", buffer);
}

int main() {
    // 假设已经有一个AST树
    Stru_AstNode_t* ast_root = ...; // 从语法分析器获取
    
    // 查找所有变量声明
    Stru_DynamicArray_t* var_decls = F_ast_find_nodes(ast_root, find_variable_decls, NULL);
    
    if (var_decls != NULL) {
        size_t count = var_decls->size;
        printf("找到 %zu 个变量声明：\n", count);
        
        for (size_t i = 0; i < count; i++) {
            Stru_AstNode_t* var_decl = (Stru_AstNode_t*)var_decls->get(var_decls, i);
            const char* name = F_ast_get_identifier(var_decl);
            const char* type = F_ast_get_type_name(var_decl);
            printf("  %s : %s\n", name, type);
        }
        
        // 销毁数组（但不销毁节点）
        var_decls->destroy(var_decls);
    }
    
    // 遍历整个AST树
    printf("\n遍历AST树：\n");
    F_ast_traverse(ast_root, print_node_info, NULL);
    
    return 0;
}
```

### 示例3：使用节点属性

```c
#include "src/core/ast/CN_ast.h"

// 创建表达式节点
Stru_AstNode_t* expr = F_create_ast_node(Eum_AST_BINARY_EXPR, NULL, 10, 5);
F_ast_set_operator_type(expr, Eum_TOKEN_PLUS); // 假设Eum_TOKEN_PLUS已定义

// 创建左操作数
Stru_AstNode_t* left = F_create_ast_node(Eum_AST_IDENTIFIER_EXPR, NULL, 10, 1);
F_ast_set_identifier(left, "x");

// 创建右操作数
Stru_AstNode_t* right = F_create_ast_node(Eum_AST_INT_LITERAL, NULL, 10, 9);
F_ast_set_int_value(right, 42);

// 构建表达式
F_ast_add_child(expr, left);
F_ast_add_child(expr, right);

// 设置自定义属性
F_ast_set_attribute(expr, "优化级别", (void*)2);
F_ast_set_attribute(expr, "是否需要括号", (void*)true);

// 获取属性
int optimization_level = (int)(intptr_t)F_ast_get_attribute(expr, "优化级别");
bool needs_parentheses = (bool)(intptr_t)F_ast_get_attribute(expr, "是否需要括号");

printf("优化级别：%d\n", optimization_level);
printf("需要括号：%s\n", needs_parentheses ? "是" : "否");

// 验证节点
if (F_ast_validate_node(expr)) {
    printf("表达式节点有效\n");
} else {
    printf("表达式节点无效\n");
}

// 清理资源
F_destroy_ast_node(expr);
```

## 节点类型说明

### 程序结构节点

#### `Eum_AST_PROGRAM`
- **描述**：程序根节点
- **子节点**：模块、函数声明、全局变量声明等
- **数据字段**：通常不使用

#### `Eum_AST_MODULE`
- **描述**：模块定义
- **子节点**：导入语句、模块级声明
- **数据字段**：`identifier`（模块名称）

#### `Eum_AST_IMPORT`
- **描述**：导入语句
- **子节点**：无
- **数据字段**：`string_value`（导入的模块路径）

### 声明节点

#### `Eum_AST_VARIABLE_DECL`
- **描述**：变量声明
- **子节点**：类型表达式、初始化表达式（可选）
- **数据字段**：`identifier`（变量名），`type_name`（类型名）

#### `Eum_AST_FUNCTION_DECL`
- **描述**：函数声明
- **子节点**：参数列表、返回类型、函数体
- **数据字段**：`identifier`（函数名）

#### `Eum_AST_STRUCT_DECL`
- **描述**：结构体声明
- **子节点**：成员声明列表
- **数据字段**：`identifier`（结构体名）

### 语句节点

#### `Eum_AST_EXPRESSION_STMT`
- **描述**：表达式语句
- **子节点**：一个表达式节点
- **数据字段**：无

#### `Eum_AST_IF_STMT`
- **描述**：if语句
- **子节点**：条件表达式、then分支、else分支（可选）
- **数据字段**：无

#### `Eum_AST_WHILE_STMT`
- **描述**：while语句
- **子节点**：条件表达式、循环体
- **数据字段**：无

### 表达式节点

#### `Eum_AST_BINARY_EXPR`
- **描述**：二元表达式
- **子节点**：左操作数、右操作数
- **数据字段**：`operator_type`（运算符类型）

#### `Eum_AST_UNARY_EXPR`
- **描述**：一元表达式
- **子节点**：操作数
- **数据字段**：`operator_type`（运算符类型）

#### `Eum_AST_CALL_EXPR`
- **描述**：函数调用表达式
- **子节点**：被调用表达式、参数列表
- **数据字段**：无

### 字面量节点

#### `Eum_AST_INT_LITERAL`
- **描述**：整数字面量
- **子节点**：无
- **数据字段**：`int_value`（整数值）

#### `Eum_AST_STRING_LITERAL`
- **描述**：字符串字面量
- **子节点**：无
- **数据字段**：`string_value`（字符串值）

#### `Eum_AST_BOOL_LITERAL`
- **描述**：布尔字面量
- **子节点**：无
- **数据字段**：`bool_value`（布尔值）

## 内存管理指南

### 所有权规则

1. **创建者负责销毁**：创建AST节点的代码负责在不再使用时销毁它
2. **子节点共享所有权**：当节点被添加到父节点时，父节点获得子节点的所有权
3. **属性值不复制**：属性值指针不被复制，调用者需要管理属性值的内存

### 最佳实践

1. **使用工厂模式**：通过语法分析器接口创建AST节点，而不是直接调用`F_create_ast_node`
2. **及时验证**：在关键阶段验证AST结构，及早发现错误
3. **合理使用属性**：使用属性存储额外的语义信息，但避免过度使用

### 常见错误

1. **内存泄漏**：忘记调用`F_destroy_ast_node`
2. **悬空指针**：在节点被销毁后继续使用其指针
3. **循环引用**：创建循环的父子关系，导致无法正确销毁

## 性能考虑

### 内存优化

1. **节点池**：对于频繁创建和销毁的节点类型，可以使用对象池
2. **字符串内化**：对于重复的标识符和字符串字面量，可以使用字符串内化技术
3. **数组预分配**：对于已知大小的子节点数组，可以预分配空间

### 遍历优化

1. **迭代遍历**：对于大型AST树，使用迭代遍历代替递归遍历
2. **缓存结果**：对于频繁查询的结果，可以缓存起来
3. **延迟计算**：对于昂贵的属性计算，可以延迟到需要时再计算

## 扩展性

### 自定义节点类型

AST模块支持通过以下方式扩展：

1. **预留类型范围**：`Eum_AST_COUNT`之后的枚举值可以用于自定义节点类型
2. **属性扩展**：通过节点属性机制添加自定义数据
3. **访问者模式**：通过`F_ast_traverse`函数支持自定义处理逻辑

### 序列化支持

AST节点支持序列化和反序列化：

1. **JSON序列化**：可以将AST转换为JSON格式进行存储或传输
2. **二进制序列化**：支持高效的二进制序列化格式
3. **文本格式化**：通过`F_ast_print_tree`函数生成可读的文本表示

## 测试指南

### 单元测试

AST模块包含以下单元测试：

1. **节点创建测试**：测试各种节点类型的创建和初始化
2. **树操作测试**：测试添加、获取、移除子节点的功能
3. **属性操作测试**：测试属性设置、获取、移除的功能
4. **数据操作测试**：测试各种数据类型的设置和获取
5. **遍历测试**：测试树遍历和节点查找功能
6. **验证测试**：测试节点和树的验证功能

### 集成测试

1. **与语法分析器集成**：测试从源代码生成AST的正确性
2. **与语义分析器集成**：测试AST在语义分析阶段的可用性
3. **与代码生成器集成**：测试AST在代码生成阶段的可用性

## 版本历史

### 版本 1.0.0 (2026-01-06)
- 初始版本发布
- 完整的AST节点类型定义
- 基本的节点创建和操作功能
- 树遍历和查询功能
- 节点验证功能
- 支持CN语言所有语法结构

### 版本 1.1.0 (计划中)
- 性能优化
- 序列化支持
- 更多查询和转换功能
- 改进的错误处理

## 相关文档

1. [语法分析器API文档](../parser/CN_parser_api.md)
2. [语法规范](../../specifications/CN_Language%20语法规范.md)
3. [词法分析器API文档](../lexer/CN_lexer_api.md)
4. [项目架构文档](../../architecture/001-中文编程语言CN_Language开发规划.md)

## 技术支持

如有问题或需要技术支持，请联系：
- 项目维护者：CN_Language架构委员会
- 问题跟踪：项目GitHub仓库
- 文档更新：docs/api/core/ast/目录

---
*文档最后更新：2026-01-06*
*版本：1.0.0*
