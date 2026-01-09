# CN_Language 语法糖支持模块

## 概述

语法糖支持模块是CN_Language语法分析器的核心组件之一，专门负责将语法糖（syntactic sugar）转换为标准的语法结构。语法糖是指那些使代码更易读、更简洁的语法特性，它们在语义上等价于更冗长的标准语法。

本模块支持多种语法糖转换，包括复合赋值、自增自减、范围循环、列表推导、空值合并、可选链、简写if和简写函数等。

## 模块功能

### 支持的语法糖类型

1. **复合赋值语法糖** (`+=`, `-=`, `*=`, `/=`, `%=`)
   - 转换示例：`a += b` → `a = a + b`

2. **自增自减语法糖** (`++`, `--`)
   - 转换示例：`a++` → `a = a + 1`

3. **范围循环语法糖** (`循环 i 在 范围(1, 10)`)
   - 转换示例：`循环 i 在 范围(1, 10)` → 标准for循环

4. **列表推导语法糖** (`[x*2 对于 x 在 列表]`)
   - 转换示例：`[x*2 对于 x 在 列表]` → 循环构建列表

5. **空值合并语法糖** (`??`)
   - 转换示例：`a ?? b` → `a 不为空 ? a : b`

6. **可选链语法糖** (`?.`)
   - 转换示例：`a?.b` → `a 不为空 ? a.b : 空`

7. **简写if语法糖** (`条件 ? 值1 : 值2`)
   - 转换示例：`条件 ? 值1 : 值2` → 标准条件表达式

8. **简写函数语法糖** (`参数 => 表达式`)
   - 转换示例：`x => x*2` → 标准函数表达式

## 文件结构

### 主要文件

1. **CN_parser_syntax_sugar.h** - 语法糖支持头文件
   - 定义语法糖类型枚举
   - 声明所有语法糖转换函数

2. **CN_parser_syntax_sugar.c** - 语法糖支持源文件
   - 实现所有语法糖转换函数
   - 实现语法糖检测和递归转换

### 依赖关系

- 语法分析器接口 (`../CN_parser_interface.h`)
- 抽象语法树模块 (`../../ast/CN_ast.h`)
- 令牌模块 (`../../token/CN_token_types.h`)
- 基础设施层内存管理 (`../../../infrastructure/memory/CN_memory_interface.h`)
- 基础设施层工具 (`../../../infrastructure/utils/CN_utils_interface.h`)

## 核心函数

### 语法糖检测

#### `F_detect_syntax_sugar()`
检测AST节点是否包含语法糖，返回语法糖类型。

### 语法糖转换函数

#### `F_transform_compound_assignment()`
将复合赋值语法糖转换为标准赋值表达式。

#### `F_transform_increment_decrement()`
将自增自减语法糖转换为标准赋值表达式。

#### `F_transform_range_loop()`
将范围循环语法糖转换为标准for循环。

#### `F_transform_list_comprehension()`
将列表推导语法糖转换为标准循环和列表构建。

#### `F_transform_null_coalescing()`
将空值合并语法糖转换为条件表达式。

#### `F_transform_optional_chaining()`
将可选链语法糖转换为条件成员访问。

#### `F_transform_shorthand_if()`
将简写if语法糖转换为标准条件表达式。

#### `F_transform_shorthand_function()`
将简写函数语法糖转换为标准函数表达式。

### 语法糖应用函数

#### `F_apply_syntax_sugar_transformations()`
检测并应用所有语法糖转换到单个节点。

#### `F_apply_syntax_sugar_recursive()`
递归遍历AST树，应用所有语法糖转换。

## 设计原理

### 语法糖检测机制

语法糖检测基于AST节点类型和属性：
1. 检查节点类型（如复合赋值表达式、一元表达式等）
2. 检查节点属性（如特殊标记）
3. 检查操作符类型（如自增自减操作符）

### 转换策略

1. **语义等价转换** - 确保转换后的代码与原始语法糖语义完全等价
2. **递归转换** - 支持嵌套语法糖的转换
3. **增量转换** - 支持部分转换，允许混合使用语法糖和标准语法

### 错误处理

1. **安全转换** - 在转换失败时返回原始节点
2. **类型检查** - 在转换前验证节点类型
3. **资源管理** - 正确处理内存分配和释放

## 使用示例

### 基本用法

```c
#include "syntax_sugar/CN_parser_syntax_sugar.h"

// 创建语法分析器接口
Stru_ParserInterface_t* parser = F_create_parser_interface();
parser->initialize(parser, lexer);

// 解析代码（包含语法糖）
Stru_AstNode_t* ast = parser->parse_program(parser);

// 应用语法糖转换
Stru_AstNode_t* transformed_ast = F_apply_syntax_sugar_recursive(parser, ast);

// 使用转换后的AST
// ...

// 清理资源
parser->destroy(parser);
```

### 检测语法糖类型

```c
// 检测AST节点的语法糖类型
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

### 应用特定语法糖转换

```c
// 只转换复合赋值语法糖
if (F_detect_syntax_sugar(node) == Eum_SUGAR_COMPOUND_ASSIGNMENT) {
    Stru_AstNode_t* transformed = F_transform_compound_assignment(parser, node);
    if (transformed != node) {
        printf("成功转换复合赋值语法糖\n");
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

## 性能考虑

### 转换开销

1. **AST遍历** - 递归遍历AST树需要额外时间
2. **节点创建** - 转换过程中可能创建新的AST节点
3. **内存使用** - 转换后的AST可能占用更多内存

### 优化策略

1. **惰性转换** - 只在需要时进行转换
2. **缓存结果** - 缓存已转换的节点
3. **批量转换** - 批量处理多个语法糖

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

## 测试

### 单元测试

- 测试每种语法糖的检测
- 测试每种语法糖的转换
- 测试嵌套语法糖的转换
- 测试错误处理

### 测试用例示例

```c
// 测试复合赋值转换
TEST_ASSERT_EQUAL(Eum_SUGAR_COMPOUND_ASSIGNMENT, F_detect_syntax_sugar(compound_assign_node));
TEST_ASSERT_NOT_NULL(F_transform_compound_assignment(parser, compound_assign_node));

// 测试递归转换
TEST_ASSERT_NOT_NULL(F_apply_syntax_sugar_recursive(parser, ast_with_sugar));
```

## 维护信息

### 版本历史

- **1.0.0** (2026-01-09): 初始版本，支持8种语法糖转换

### 维护者

CN_Language架构委员会

### 许可证

MIT许可证
