# CN_Language 声明解析模块

## 概述

声明解析模块是CN_Language语法分析器的核心组件之一，专门负责解析各种类型的声明。声明是CN_Language程序的组织单元，包括变量、函数、结构体、枚举等定义。本模块遵循项目架构规范，采用递归下降解析算法，实现高效、准确的声明解析。

## 模块功能

### 核心功能
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

## 文件结构

### 主要文件
1. **CN_parser_declarations.h** - 声明解析头文件
   - 声明所有声明解析函数
   - 定义声明解析接口

### 子模块文件
2. **CN_parser_declarations_main.c** - 声明解析主模块
   - 实现声明解析入口函数 `F_parse_declaration()`
   - 根据令牌类型分发到相应的子解析函数

3. **CN_parser_variable_declarations.c** - 变量声明解析模块
   - 实现变量声明解析函数 `F_parse_variable_declaration()`

4. **CN_parser_function_declarations.c** - 函数声明解析模块
   - 实现函数声明解析函数 `F_parse_function_declaration()`

5. **CN_parser_struct_declarations.c** - 结构体声明解析模块
   - 实现结构体声明解析函数 `F_parse_struct_declaration()`

6. **CN_parser_enum_declarations.c** - 枚举声明解析模块
   - 实现枚举声明解析函数 `F_parse_enum_declaration()`

7. **CN_parser_module_declarations.c** - 模块声明解析模块
   - 实现模块声明解析函数 `F_parse_module_declaration()`

8. **CN_parser_type_declarations.c** - 类型声明解析模块
   - 实现类型声明解析函数 `F_parse_type_declaration()`

9. **CN_parser_interface_declarations.c** - 接口声明解析模块
   - 实现接口声明解析函数 `F_parse_interface_declaration()`

10. **CN_parser_class_declarations.c** - 类声明解析模块
    - 实现类声明解析函数 `F_parse_class_declaration()`

11. **CN_parser_generic_declarations.c** - 泛型声明解析模块
    - 实现泛型声明解析函数 `F_parse_generic_declaration()`

12. **CN_parser_parameter_lists.c** - 参数列表解析模块
    - 实现参数列表解析函数 `F_parse_parameter_list()` 和 `F_parse_parameter()`

13. **CN_parser_type_expressions.c** - 类型表达式解析模块
    - 实现类型表达式解析函数 `F_parse_type_expression()`, `F_parse_basic_type()`, `F_parse_array_type()`, `F_parse_pointer_type()`

14. **CN_parser_member_lists.c** - 成员列表解析模块
    - 实现成员列表解析函数 `F_parse_struct_member_list()` 和 `F_parse_enum_member_list()`

### 依赖关系
- 语法分析器接口 (`../CN_parser_interface.h`)
- 语法错误处理 (`../CN_syntax_error.h`)
- 表达式解析模块 (`../expression_parsers/CN_parser_expressions.h`)
- 语句解析模块 (`../statement_parsers/CN_parser_statements.h`)
- 令牌模块 (`../../token/CN_token_types.h`)
- AST模块 (`../../ast/CN_ast.h`)
- 基础设施层容器 (`../../../infrastructure/containers/array/CN_dynamic_array.h`)

## 声明解析函数

### 主要解析函数

#### `F_parse_declaration()`
解析声明，根据当前令牌类型调用相应的子解析函数。

#### `F_parse_variable_declaration()`
解析变量声明，如：`变量 名称 = 值;`。

#### `F_parse_function_declaration()`
解析函数声明，如：`函数 名称(参数) { ... }`。

#### `F_parse_struct_declaration()`
解析结构体声明，如：`结构体 名称 { ... }`。

#### `F_parse_enum_declaration()`
解析枚举声明，如：`枚举 名称 { ... }`。

#### `F_parse_module_declaration()`
解析模块声明，如：`模块 名称 { ... }`。

#### `F_parse_type_declaration()`
解析类型声明，如：`类型 名称 = 类型表达式;`。

#### `F_parse_interface_declaration()`
解析接口声明，如：`接口 名称 { ... }`。

#### `F_parse_class_declaration()`
解析类声明，如：`类 名称 { ... }`。

#### `F_parse_generic_declaration()`
解析泛型声明，如：`泛型 名称<T> { ... }`。

### 辅助解析函数

#### `F_parse_parameter_list()`
解析函数声明的参数列表。

#### `F_parse_parameter()`
解析函数声明的单个参数。

#### `F_parse_type_expression()`
解析类型表达式，如：`整数[], 字符串指针`。

#### `F_parse_basic_type()`
解析基本类型，如：`整数, 浮点数, 字符串, 布尔值`。

#### `F_parse_array_type()`
解析数组类型，如：`整数[], 字符串[10]`。

#### `F_parse_pointer_type()`
解析指针类型，如：`整数指针, 字符串指针`。

#### `F_parse_struct_member_list()`
解析结构体成员列表。

#### `F_parse_enum_member_list()`
解析枚举成员列表。

## 声明语法

### 变量声明
```
变量 名称 = 值;
```

### 函数声明
```
函数 名称(参数1: 类型1, 参数2: 类型2) -> 返回类型 {
    // 函数体
}
```

### 结构体声明
```
结构体 名称 {
    成员1: 类型1;
    成员2: 类型2;
    ...
}
```

### 枚举声明
```
枚举 名称 {
    成员1 = 值1,
    成员2 = 值2,
    ...
}
```

### 模块声明
```
模块 名称 {
    // 模块内容
}
```

### 类型声明
```
类型 名称 = 类型表达式;
```

### 接口声明
```
接口 名称 {
    方法1(参数: 类型) -> 返回类型;
    方法2(参数: 类型) -> 返回类型;
    ...
}
```

### 类声明
```
类 名称 {
    公开 字段1: 类型;
    私有 字段2: 类型;
    静态 方法1(参数: 类型) -> 返回类型 { ... }
    虚拟 方法2(参数: 类型) -> 返回类型 { ... }
    ...
}
```

### 泛型声明
```
泛型 名称<T, U> {
    字段: T;
    方法(参数: U) -> T { ... }
    ...
}
```

## 使用示例

### 基本用法
```c
#include "declaration_parsers/CN_parser_declarations.h"

// 获取语法分析器接口
Stru_ParserInterface_t* parser = F_create_parser_interface();
parser->initialize(parser, lexer);

// 解析声明
Stru_AstNode_t* decl = F_parse_declaration(parser);

// 检查声明类型
if (decl != NULL) {
    switch (decl->type) {
        case Eum_AST_NODE_VARIABLE_DECL:
            // 处理变量声明
            break;
        case Eum_AST_NODE_FUNCTION_DECL:
            // 处理函数声明
            break;
        case Eum_AST_NODE_STRUCT_DECL:
            // 处理结构体声明
            break;
        case Eum_AST_NODE_INTERFACE_DECL:
            // 处理接口声明
            break;
        case Eum_AST_NODE_CLASS_DECL:
            // 处理类声明
            break;
        case Eum_AST_NODE_GENERIC_DECL:
            // 处理泛型声明
            break;
        // ... 其他声明类型
    }
}

// 清理资源
parser->destroy(parser);
```

### 解析复杂声明
```c
// 解析函数声明
Stru_AstNode_t* func_decl = F_parse_function_declaration(parser);

// 解析结构体声明
Stru_AstNode_t* struct_decl = F_parse_struct_declaration(parser);

// 解析类型声明
Stru_AstNode_t* type_decl = F_parse_type_declaration(parser);

// 解析接口声明
Stru_AstNode_t* interface_decl = F_parse_interface_declaration(parser);

// 解析类声明
Stru_AstNode_t* class_decl = F_parse_class_declaration(parser);

// 解析泛型声明
Stru_AstNode_t* generic_decl = F_parse_generic_declaration(parser);
```

## 设计原理

### 递归下降算法
声明解析模块使用递归下降算法：
1. `F_parse_declaration()` 根据当前令牌类型分发到相应的解析函数
2. 每个声明类型有专门的解析函数
3. 支持嵌套声明结构

### 类型系统
- 支持基本类型、数组类型、指针类型
- 支持类型表达式解析
- 支持类型别名

### 错误恢复
- 在解析失败时返回NULL
- 报告详细的错误信息
- 支持跳过错误声明继续解析

## 测试

### 单元测试
- 测试各种声明类型的解析
- 测试类型系统解析
- 测试错误处理

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
```

## 性能考虑

### 递归深度
- 声明解析可能涉及多层嵌套
- 最大递归深度受限于栈大小
- 复杂嵌套结构可能需要优化

### 内存使用
- 每个声明节点独立分配内存
- 支持声明节点重用
- 避免内存泄漏

## 扩展性

### 添加新的声明类型
1. 添加新的声明解析函数
2. 更新声明类型枚举
3. 在`F_parse_declaration()`中添加分发逻辑

### 自定义类型系统
1. 添加新的类型解析函数
2. 实现对应的AST构建逻辑
3. 更新语法规范

## 维护信息

### 版本历史
- **1.0.0** (2026-01-08): 初始版本，基于模块化架构设计
- **1.1.0** (2026-01-09): 新增接口、类和泛型声明解析功能

### 维护者
CN_Language架构委员会

### 许可证
MIT许可证
