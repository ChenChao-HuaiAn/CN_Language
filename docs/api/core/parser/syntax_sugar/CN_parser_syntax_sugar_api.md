# CN_Language 语法糖支持模块 API 文档

## 概述

语法糖支持模块是CN_Language语法分析器的扩展组件，专门负责将语法糖（syntactic sugar）转换为标准的语法结构。语法糖是指那些使代码更易读、更简洁的语法特性，它们在语义上等价于更冗长的标准语法。

本模块支持多种语法糖转换，包括复合赋值、自增自减、范围循环、列表推导、空值合并、可选链、简写if和简写函数等。

### 主要功能

1. **语法糖检测** - 检测AST节点是否包含语法糖
2. **语法糖转换** - 将语法糖转换为标准语法结构
3. **递归转换** - 支持嵌套语法糖的转换
4. **语义保持** - 确保转换后的代码语义不变

### 架构位置

- **所属层级**：核心层（Core Layer）
- **依赖模块**：语法分析器接口、AST模块、基础设施层
- **被依赖模块**：语义分析器、代码生成器

## 核心接口

### 语法糖类型枚举

```c
typedef enum Eum_SyntaxSugarType {
    Eum_SUGAR_COMPOUND_ASSIGNMENT,     ///< 复合赋值语法糖 (a += b)
    Eum_SUGAR_INCREMENT_DECREMENT,     ///< 自增自减语法糖 (a++, a--)
    Eum_SUGAR_RANGE_LOOP,              ///< 范围循环语法糖 (循环 i 在 范围(1, 10))
    Eum_SUGAR_LIST_COMPREHENSION,      ///< 列表推导语法糖 ([x*2 对于 x 在 列表])
    Eum_SUGAR_NULL_COALESCING,         ///< 空值合并语法糖 (a ?? b)
    Eum_SUGAR_OPTIONAL_CHAINING,       ///< 可选链语法糖 (a?.b)
    Eum_SUGAR_SHORTHAND_IF,            ///< 简写if语法糖 (条件 ? 值 : 值)
    Eum_SUGAR_SHORTHAND_FUNCTION,      ///< 简写函数语法糖 (参数 => 表达式)
    Eum_SUGAR_NONE                     ///< 无语法糖
} Eum_SyntaxSugarType;
```

## API 函数

### 语法糖检测函数

#### `F_detect_syntax_sugar`

**功能**：检测AST节点是否包含语法糖。

**函数原型**：
```c
Eum_SyntaxSugarType F_detect_syntax_sugar(Stru_AstNode_t* node);
```

**参数**：
- `node`：要检测的AST节点

**返回值**：
- 检测到的语法糖类型，如果未检测到语法糖则返回`Eum_SUGAR_NONE`

**使用示例**：
```c
Stru_AstNode_t* ast_node = parser->parse_expression(parser);
Eum_SyntaxSugarType sugar_type = F_detect_syntax_sugar(ast_node);

switch (sugar_type) {
    case Eum_SUGAR_COMPOUND_ASSIGNMENT:
        printf("检测到复合赋值语法糖\n");
        break;
    case Eum_SUGAR_INCREMENT_DECREMENT:
        printf("检测到自增自减语法糖\n");
        break;
    // ... 其他语法糖类型
    default:
        printf("未检测到语法糖\n");
        break;
}
```

### 语法糖转换函数

#### `F_transform_compound_assignment`

**功能**：将复合赋值语法糖转换为标准赋值表达式。

**函数原型**：
```c
Stru_AstNode_t* F_transform_compound_assignment(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);
```

**参数**：
- `interface`：语法分析器接口指针
- `node`：复合赋值AST节点

**返回值**：
- 转换后的AST节点，如果转换失败则返回原始节点

**转换规则**：
- `a += b` → `a = a + b`
- `a -= b` → `a = a - b`
- `a *= b` → `a = a * b`
- `a /= b` → `a = a / b`
- `a %= b` → `a = a % b`

#### `F_transform_increment_decrement`

**功能**：将自增自减语法糖转换为标准赋值表达式。

**函数原型**：
```c
Stru_AstNode_t* F_transform_increment_decrement(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);
```

**参数**：
- `interface`：语法分析器接口指针
- `node`：自增自减AST节点

**返回值**：
- 转换后的AST节点，如果转换失败则返回原始节点

**转换规则**：
- `a++` → `a = a + 1`
- `a--` → `a = a - 1`

#### `F_transform_range_loop`

**功能**：将范围循环语法糖转换为标准for循环。

**函数原型**：
```c
Stru_AstNode_t* F_transform_range_loop(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);
```

**参数**：
- `interface`：语法分析器接口指针
- `node`：范围循环AST节点

**返回值**：
- 转换后的AST节点，如果转换失败则返回原始节点

**转换规则**：
- `循环 i 在 范围(1, 10)` → 标准for循环

#### `F_transform_list_comprehension`

**功能**：将列表推导语法糖转换为标准循环和列表构建。

**函数原型**：
```c
Stru_AstNode_t* F_transform_list_comprehension(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);
```

**参数**：
- `interface`：语法分析器接口指针
- `node`：列表推导AST节点

**返回值**：
- 转换后的AST节点，如果转换失败则返回原始节点

**转换规则**：
- `[x*2 对于 x 在 列表]` → 循环构建列表

#### `F_transform_null_coalescing`

**功能**：将空值合并语法糖转换为条件表达式。

**函数原型**：
```c
Stru_AstNode_t* F_transform_null_coalescing(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);
```

**参数**：
- `interface`：语法分析器接口指针
- `node`：空值合并AST节点

**返回值**：
- 转换后的AST节点，如果转换失败则返回原始节点

**转换规则**：
- `a ?? b` → `a 不为空 ? a : b`

#### `F_transform_optional_chaining`

**功能**：将可选链语法糖转换为条件成员访问。

**函数原型**：
```c
Stru_AstNode_t* F_transform_optional_chaining(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);
```

**参数**：
- `interface`：语法分析器接口指针
- `node`：可选链AST节点

**返回值**：
- 转换后的AST节点，如果转换失败则返回原始节点

**转换规则**：
- `a?.b` → `a 不为空 ? a.b : 空`

#### `F_transform_shorthand_if`

**功能**：将简写if语法糖转换为标准条件表达式。

**函数原型**：
```c
Stru_AstNode_t* F_transform_shorthand_if(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);
```

**参数**：
- `interface`：语法分析器接口指针
- `node`：简写ifAST节点

**返回值**：
- 转换后的AST节点，如果转换失败则返回原始节点

**转换规则**：
- `条件 ? 值1 : 值2` → 标准条件表达式

#### `F_transform_shorthand_function`

**功能**：将简写函数语法糖转换为标准函数表达式。

**函数原型**：
```c
Stru_AstNode_t* F_transform_shorthand_function(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);
```

**参数**：
- `interface`：语法分析器接口指针
- `node`：简写函数AST节点

**返回值**：
- 转换后的AST节点，如果转换失败则返回原始节点

**转换规则**：
- `参数 => 表达式` → 标准函数表达式

### 语法糖应用函数

#### `F_apply_syntax_sugar_transformations`

**功能**：检测并应用所有语法糖转换到单个节点。

**函数原型**：
```c
Stru_AstNode_t* F_apply_syntax_sugar_transformations(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);
```

**参数**：
- `interface`：语法分析器接口指针
- `node`：要转换的AST节点

**返回值**：
- 转换后的AST节点

**算法**：
1. 调用`F_detect_syntax_sugar()`检测语法糖类型
2. 根据语法糖类型调用相应的转换函数
3. 返回转换后的节点

#### `F_apply_syntax_sugar_recursive`

**功能**：递归遍历AST树，应用所有语法糖转换。

**函数原型**：
```c
Stru_AstNode_t* F_apply_syntax_sugar_recursive(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);
```

**参数**：
- `interface`：语法分析器接口指针
- `node`：AST根节点

**返回值**：
- 转换后的AST根节点

**算法**：
1. 递归处理所有子节点
2. 对每个节点应用语法糖转换
3. 返回转换后的AST树

## 使用示例

### 示例1：基本语法糖转换

```c
#include "src/core/parser/syntax_sugar/CN_parser_syntax_sugar.h"

// 创建语法分析器
Stru_ParserInterface_t* parser = F_create_parser_interface();
parser->initialize(parser, lexer);

// 解析包含语法糖的代码
Stru_AstNode_t* ast = parser->parse_program(parser);

// 应用语法糖转换
Stru_AstNode_t* transformed_ast = F_apply_syntax_sugar_recursive(parser, ast);

if (transformed_ast != ast) {
    printf("语法糖转换成功\n");
    
    // 检查转换后的AST
    // ...
}

// 清理资源
parser->destroy(parser);
```

### 示例2：检测特定语法糖

```c
// 检测复合赋值语法糖
Stru_AstNode_t* expr = parser->parse_expression(parser);
Eum_SyntaxSugarType sugar_type = F_detect_syntax_sugar(expr);

if (sugar_type == Eum_SUGAR_COMPOUND_ASSIGNMENT) {
    printf("检测到复合赋值语法糖\n");
    
    // 应用转换
    Stru_AstNode_t* transformed = F_transform_compound_assignment(parser, expr);
    if (transformed != expr) {
        printf("复合赋值语法糖转换成功\n");
    }
}
```

### 示例3：自定义转换流程

```c
// 自定义语法糖转换流程
void custom_syntax_sugar_transform(Stru_ParserInterface_t* parser, Stru_AstNode_t* node) {
    // 首先检测语法糖类型
    Eum_SyntaxSugarType sugar_type = F_detect_syntax_sugar(node);
    
    // 根据类型应用不同的转换策略
    switch (sugar_type) {
        case Eum_SUGAR_COMPOUND_ASSIGNMENT:
            // 只转换复合赋值，其他保持不变
            node = F_transform_compound_assignment(parser, node);
            break;
            
        case Eum_SUGAR_NULL_COALESCING:
            // 转换空值合并，并记录转换日志
            printf("转换空值合并语法糖\n");
            node = F_transform_null_coalescing(parser, node);
            break;
            
        default:
            // 其他语法糖保持原样
            break;
    }
    
    // 递归处理子节点
    size_t child_count = F_ast_get_child_count(node);
    for (size_t i = 0; i < child_count; i++) {
        Stru_AstNode_t* child = F_ast_get_child(node, i);
        if (child != NULL) {
            custom_syntax_sugar_transform(parser, child);
        }
    }
}
```

## 转换示例

### 复合赋值转换

**原始代码（语法糖）：**
```cn
变量 a = 5;
a += 3;
```

**转换后代码（标准语法）：**
```cn
变量 a = 5;
a = a + 3;
```

### 自增自减转换

**原始代码（语法糖）：**
```cn
变量 i = 0;
i++;
```

**转换后代码（标准语法）：**
```cn
变量 i = 0;
i = i + 1;
```

### 空值合并转换

**原始代码（语法糖）：**
```cn
变量 结果 = 输入 ?? "默认值";
```

**转换后代码（标准语法）：**
```cn
变量 结果 = 输入 不为空 ? 输入 : "默认值";
```

### 可选链转换

**原始代码（语法糖）：**
```cn
变量 值 = 对象?.属性?.子属性;
```

**转换后代码（标准语法）：**
```cn
变量 临时1 = 对象 不为空 ? 对象.属性 : 空;
变量 值 = 临时1 不为空 ? 临时1.子属性 : 空;
```

## 设计原理

### 语法糖检测机制

语法糖检测基于AST节点类型和属性：

1. **节点类型检查**：检查AST节点类型（如复合赋值表达式、一元表达式等）
2. **属性检查**：检查节点是否包含特殊标记属性
3. **操作符检查**：检查操作符类型（如自增自减操作符）

### 转换策略

1. **语义等价转换**：确保转换后的代码与原始语法糖语义完全等价
2. **递归转换**：支持嵌套语法糖的转换
3. **增量转换**：支持部分转换，允许混合使用语法糖和标准语法
4. **安全转换**：在转换失败时返回原始节点，避免破坏AST结构

### 错误处理

1. **安全转换**：在转换失败时返回原始节点
2. **类型检查**：在转换前验证节点类型
3. **资源管理**：正确处理内存分配和释放
4. **错误报告**：记录转换过程中的错误信息

## 性能考虑

### 转换开销

1. **AST遍历**：递归遍历AST树需要额外时间
2. **节点创建**：转换过程中可能创建新的AST节点
3. **内存使用**：转换后的AST可能占用更多内存

### 优化策略

1. **惰性转换**：只在需要时进行转换
2. **缓存结果**：缓存已转换的节点
3. **批量转换**：批量处理多个语法糖
4. **增量转换**：只转换发生变化的节点

## 扩展性

### 添加新的语法糖类型

1. 在`Eum_SyntaxSugarType`枚举中添加新类型
2. 实现对应的检测函数逻辑
3. 实现转换函数
4. 更新`F_apply_syntax_sugar_transformations()`函数

### 自定义转换规则

1. 实现自定义的转换函数
2. 注册到语法糖转换器
3. 配置转换优先级
4. 测试转换正确性

## 测试指南

### 单元测试

语法糖支持模块包含完整的单元测试：

1. **语法糖检测测试**：
   - 测试各种语法糖类型的检测
   - 测试边界条件和错误情况

2. **语法糖转换测试**：
   - 测试每种语法糖的转换正确性
   - 测试嵌套语法糖的转换
   - 测试错误处理

3. **性能测试**：
   - 测试转换性能
   - 测试内存使用情况

### 测试用例示例

```c
// 测试复合赋值转换
TEST_ASSERT_EQUAL(Eum_SUGAR_COMPOUND_ASSIGNMENT, F_detect_syntax_sugar(compound_assign_node));
TEST_ASSERT_NOT_NULL(F_transform_compound_assignment(parser, compound_assign_node));

// 测试递归转换
TEST_ASSERT_NOT_NULL(F_apply_syntax_sugar_recursive(parser, ast_with_sugar));

// 测试转换正确性
TEST_ASSERT_EQUAL_STRING("a = a + b", get_ast_as_string(transformed_node));
```

## 相关文档

1. [语法分析器主API文档](../CN_parser_api.md)
2. [AST模块API文档](../../ast/CN_ast_api.md)
3. [语法糖支持模块README](../../../../src/core/parser/syntax_sugar/README.md)

## 版本历史

### 版本 1.0.0 (2026-01-09)
- 初始版本发布
- 支持8种语法糖转换
- 完整的API文档
- 单元测试框架

### 版本 1.1.0 (计划中)
- 性能优化
- 支持更多语法糖类型
- 改进的错误处理
- 扩展的测试用例

## 技术支持

如有问题或需要技术支持，请联系：
- 项目维护者：CN_Language架构委员会
- 问题跟踪：项目GitHub仓库
- 文档更新：docs/api/core/parser/syntax_sugar/目录

---
*文档最后更新：2026-01-10*
*版本：1.0.0*
