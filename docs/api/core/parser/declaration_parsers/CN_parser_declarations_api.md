# CN_Language 声明解析模块 API 文档

## 概述

声明解析模块是CN_Language语法分析器的核心组件之一，专门负责解析各种类型的声明。声明是CN_Language程序的组织单元，包括变量、函数、结构体、枚举等定义。本模块遵循项目架构规范，采用递归下降解析算法，实现高效、准确的声明解析。

### 主要功能

1. **声明解析** - 解析各种类型的声明，构建声明AST节点
2. **类型系统支持** - 解析类型表达式和类型声明
3. **作用域管理** - 处理声明的可见性和作用域
4. **错误检测** - 检测声明语法错误

### 支持的声明类型

- 变量声明：定义变量及其初始值
- 函数声明：定义函数及其参数和函数体
- 结构体声明：定义结构体及其成员
- 枚举声明：定义枚举及其成员
- 模块声明：定义模块及其内容
- 类型声明：定义类型别名
- 接口声明：定义接口及其方法
- 类声明：定义类及其成员（字段和方法）
- 泛型声明：定义泛型类型及其参数

### 架构位置

- **所属层级**：核心层（Core Layer）
- **依赖模块**：语法分析器接口、表达式解析模块、语句解析模块、AST模块
- **被依赖模块**：语义分析器、代码生成器

## API 函数

### 主要声明解析函数

#### `F_parse_declaration`

**功能**：解析声明，根据当前令牌类型调用相应的子解析函数。

**函数原型**：
```c
Stru_AstNode_t* F_parse_declaration(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 声明AST节点，`NULL`表示错误或结束

**算法**：
1. 检查当前令牌类型
2. 根据令牌类型分发到相应的解析函数：
   - 变量关键字 → `F_parse_variable_declaration()`
   - 函数关键字 → `F_parse_function_declaration()`
   - 结构体关键字 → `F_parse_struct_declaration()`
   - 枚举关键字 → `F_parse_enum_declaration()`
   - 模块关键字 → `F_parse_module_declaration()`
   - 类型关键字 → `F_parse_type_declaration()`
   - 接口关键字 → `F_parse_interface_declaration()`
   - 类关键字 → `F_parse_class_declaration()`
   - 泛型关键字 → `F_parse_generic_declaration()`

#### `F_parse_variable_declaration`

**功能**：解析变量声明。

**函数原型**：
```c
Stru_AstNode_t* F_parse_variable_declaration(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 变量声明AST节点

**语法**：
```
变量 标识符 [: 类型] [= 初始值];
```

**示例**：
```cn
变量 计数器 = 0;
变量 名称: 字符串 = "张三";
变量 标志: 布尔值;
```

#### `F_parse_function_declaration`

**功能**：解析函数声明。

**函数原型**：
```c
Stru_AstNode_t* F_parse_function_declaration(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 函数声明AST节点

**语法**：
```
函数 标识符(参数列表) [: 返回类型] { 函数体 }
```

**示例**：
```cn
函数 加法(a: 整数, b: 整数): 整数 {
    返回 a + b;
}
```

#### `F_parse_struct_declaration`

**功能**：解析结构体声明。

**函数原型**：
```c
Stru_AstNode_t* F_parse_struct_declaration(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 结构体声明AST节点

**语法**：
```
结构体 标识符 { 成员列表 }
```

**示例**：
```cn
结构体 点 {
    x: 整数;
    y: 整数;
}
```

#### `F_parse_enum_declaration`

**功能**：解析枚举声明。

**函数原型**：
```c
Stru_AstNode_t* F_parse_enum_declaration(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 枚举声明AST节点

**语法**：
```
枚举 标识符 { 成员列表 }
```

**示例**：
```cn
枚举 颜色 {
    红,
    绿,
    蓝
}
```

#### `F_parse_module_declaration`

**功能**：解析模块声明。

**函数原型**：
```c
Stru_AstNode_t* F_parse_module_declaration(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 模块声明AST节点

**语法**：
```
模块 标识符 { 模块内容 }
```

**示例**：
```cn
模块 数学工具 {
    函数 平方(x: 整数): 整数 {
        返回 x * x;
    }
}
```

#### `F_parse_type_declaration`

**功能**：解析类型声明。

**函数原型**：
```c
Stru_AstNode_t* F_parse_type_declaration(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 类型声明AST节点

**语法**：
```
类型 标识符 = 类型表达式;
```

**示例**：
```cn
类型 字符串指针 = 字符串指针;
类型 整数数组 = 整数[];
```

#### `F_parse_interface_declaration`

**功能**：解析接口声明。

**函数原型**：
```c
Stru_AstNode_t* F_parse_interface_declaration(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 接口声明AST节点

**语法**：
```
接口 标识符 { 方法列表 }
```

**示例**：
```cn
接口 可打印 {
    打印(): 空;
    获取字符串表示(): 字符串;
}
```

#### `F_parse_class_declaration`

**功能**：解析类声明。

**函数原型**：
```c
Stru_AstNode_t* F_parse_class_declaration(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 类声明AST节点

**语法**：
```
类 标识符 [: 父类] { 成员列表 }
```

**示例**：
```cn
类 矩形 {
    宽度: 整数;
    高度: 整数;
    
    函数 计算面积(): 整数 {
        返回 宽度 * 高度;
    }
}
```

#### `F_parse_generic_declaration`

**功能**：解析泛型声明。

**函数原型**：
```c
Stru_AstNode_t* F_parse_generic_declaration(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 泛型声明AST节点

**语法**：
```
泛型 标识符<类型参数列表> { 成员列表 }
```

**示例**：
```cn
泛型 容器<T> {
    数据: T;
    
    函数 设置值(值: T): 空 {
        数据 = 值;
    }
    
    函数 获取值(): T {
        返回 数据;
    }
}
```

### 辅助解析函数

#### `F_parse_parameter_list`

**功能**：解析函数声明的参数列表。

**函数原型**：
```c
Stru_DynamicArray_t* F_parse_parameter_list(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 参数声明数组

**语法**：
```
(参数1: 类型1, 参数2: 类型2, ...)
```

#### `F_parse_parameter`

**功能**：解析函数声明的单个参数。

**函数原型**：
```c
Stru_AstNode_t* F_parse_parameter(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 参数声明AST节点

**语法**：
```
标识符: 类型
```

#### `F_parse_type_expression`

**功能**：解析类型表达式。

**函数原型**：
```c
Stru_AstNode_t* F_parse_type_expression(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 类型表达式AST节点

**支持的表达式**：
- 基本类型：`整数`, `浮点数`, `字符串`, `布尔值`
- 数组类型：`类型[]`, `类型[大小]`
- 指针类型：`类型指针`
- 复合类型：`(类型表达式)`

#### `F_parse_basic_type`

**功能**：解析基本类型。

**函数原型**：
```c
Stru_AstNode_t* F_parse_basic_type(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 基本类型AST节点

**支持的基本类型**：
- `整数` - 整数值
- `浮点数` - 浮点数值
- `字符串` - 字符串值
- `布尔值` - 布尔值
- `空` - 空类型

#### `F_parse_array_type`

**功能**：解析数组类型。

**函数原型**：
```c
Stru_AstNode_t* F_parse_array_type(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 数组类型AST节点

**语法**：
```
类型[]
类型[表达式]
```

#### `F_parse_pointer_type`

**功能**：解析指针类型。

**函数原型**：
```c
Stru_AstNode_t* F_parse_pointer_type(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 指针类型AST节点

**语法**：
```
类型指针
```

#### `F_parse_struct_member_list`

**功能**：解析结构体成员列表。

**函数原型**：
```c
Stru_DynamicArray_t* F_parse_struct_member_list(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 结构体成员数组

**语法**：
```
{ 成员1: 类型1; 成员2: 类型2; ... }
```

#### `F_parse_enum_member_list`

**功能**：解析枚举成员列表。

**函数原型**：
```c
Stru_DynamicArray_t* F_parse_enum_member_list(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 枚举成员数组

**语法**：
```
{ 成员1 [= 值1], 成员2 [= 值2], ... }
```

## 使用示例

### 示例1：基本声明解析

```c
#include "src/core/parser/declaration_parsers/CN_parser_declarations.h"

// 创建语法分析器
Stru_ParserInterface_t* parser = F_create_parser_interface();
parser->initialize(parser, lexer);

// 解析变量声明
Stru_AstNode_t* var_decl = F_parse_variable_declaration(parser);
if (var_decl != NULL) {
    printf("变量声明解析成功\n");
    
    // 获取变量信息
    const char* var_name = F_ast_get_identifier(var_decl);
    Stru_AstNode_t* var_type = F_ast_get_type_node(var_decl);
    Stru_AstNode_t* var_value = F_ast_get_initial_value(var_decl);
    
    printf("变量名: %s\n", var_name);
}

// 解析函数声明
Stru_AstNode_t* func_decl = F_parse_function_declaration(parser);
if (func_decl != NULL) {
    printf("函数声明解析成功\n");
}

// 清理资源
parser->destroy(parser);
```

### 示例2：解析复杂声明

```c
// 解析结构体声明
Stru_AstNode_t* struct_decl = F_parse_struct_declaration(parser);
if (struct_decl != NULL) {
    // 获取结构体成员
    Stru_DynamicArray_t* members = F_parse_struct_member_list(parser);
    if (members != NULL) {
        size_t member_count = F_dynamic_array_get_size(members);
        printf("结构体有 %zu 个成员\n", member_count);
    }
}

// 解析类型声明
Stru_AstNode_t* type_decl = F_parse_type_declaration(parser);
if (type_decl != NULL) {
    // 获取类型表达式
    Stru_AstNode_t* type_expr = F_parse_type_expression(parser);
    if (type_expr != NULL) {
        printf("类型声明解析成功\n");
    }
}
```

### 示例3：使用声明解析入口函数

```c
// 使用通用声明解析函数
Stru_AstNode_t* declaration = F_parse_declaration(parser);

if (declaration != NULL) {
    // 根据声明类型处理
    Eum_AstNodeType decl_type = F_ast_get_node_type(declaration);
    
    switch (decl_type) {
        case Eum_AST_NODE_VARIABLE_DECL:
            printf("变量声明\n");
            break;
            
        case Eum_AST_NODE_FUNCTION_DECL:
            printf("函数声明\n");
            break;
            
        case Eum_AST_NODE_STRUCT_DECL:
            printf("结构体声明\n");
            break;
            
        case Eum_AST_NODE_ENUM_DECL:
            printf("枚举声明\n");
            break;
            
        case Eum_AST_NODE_INTERFACE_DECL:
            printf("接口声明\n");
            break;
            
        case Eum_AST_NODE_CLASS_DECL:
            printf("类声明\n");
            break;
            
        case Eum_AST_NODE_GENERIC_DECL:
            printf("泛型声明\n");
            break;
            
        default:
            printf("未知声明类型\n");
            break;
    }
}
```

## 设计原理

### 递归下降算法

声明解析模块使用递归下降算法：

1. **入口函数分发**：`F_parse_declaration()`根据当前令牌类型分发到相应的解析函数
2. **专门解析函数**：每个声明类型有专门的解析函数
3. **嵌套结构支持**：支持嵌套声明结构（如结构体中的成员声明）

### 类型系统

1. **类型表达式解析**：支持复杂的类型表达式解析
2. **类型兼容性**：在解析时进行基本的类型检查
3. **类型别名**：支持类型别名定义

### 错误恢复

1. **错误检测**：检测声明语法错误
2. **错误报告**：生成详细的错误信息
3. **恢复机制**：在错误后尝试继续解析

## 性能考虑

### 递归深度

1. **嵌套声明**：声明解析可能涉及多层嵌套
2. **最大深度**：最大递归深度受限于栈大小
3. **优化策略**：复杂嵌套结构可能需要优化

### 内存使用

1. **节点分配**：每个声明节点独立分配内存
2. **数组管理**：参数列表和成员列表使用动态数组
3. **内存回收**：支持声明节点重用和内存回收

## 扩展性

### 添加新的声明类型

1. 添加新的声明解析函数
2. 更新声明类型枚举
3. 在`F_parse_declaration()`中添加分发逻辑
4. 实现对应的AST构建逻辑

### 自定义类型系统

1. 添加新的类型解析函数
2. 实现类型兼容性检查
3. 更新语法规范

## 测试指南

### 单元测试

声明解析模块包含完整的单元测试：

1. **声明解析测试**：
   - 测试各种声明类型的解析
   - 测试边界条件和错误情况

2. **类型系统测试**：
   - 测试类型表达式解析
   - 测试类型兼容性检查

3. **错误处理测试**：
   - 测试错误检测和报告
   - 测试错误恢复机制

### 测试用例示例

```c
// 测试变量声明
TEST_ASSERT_NOT_NULL(F_parse_variable_declaration(parser));

// 测试函数声明
TEST_ASSERT_NOT_NULL(F_parse_function_declaration(parser));

// 测试结构体声明
TEST_ASSERT_NOT_NULL(F_parse_struct_declaration(parser));

// 测试类型表达式
TEST_ASSERT_NOT_NULL(F_parse_type_expression(parser));

// 测试参数列表
TEST_ASSERT_NOT_NULL(F_parse_parameter_list(parser));
```

## 相关文档

1. [语法分析器主API文档](../CN_parser_api.md)
2. [表达式解析模块API文档](../expression_parsers/README.md)
3. [语句解析模块API文档](../statement_parsers/README.md)
4. [声明解析模块README](../../../../src/core/parser/declaration_parsers/README.md)

## 版本历史

### 版本 1.0.0 (2026-01-08)
- 初始版本发布
- 支持基本声明类型解析
- 完整的类型系统支持

### 版本 1
