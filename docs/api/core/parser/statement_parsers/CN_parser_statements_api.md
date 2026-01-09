# CN_Language 语句解析模块 API 文档

## 概述

语句解析模块是CN_Language语法分析器的核心组件之一，专门负责解析各种类型的语句。语句是CN_Language程序的基本执行单元，包括表达式语句、控制流语句、声明语句等。本模块采用递归下降解析算法，实现高效、准确的语句解析。

## 模块信息

- **模块名称**: 语句解析模块 (Statement Parsers)
- **所属层级**: 核心层 (Core Layer)
- **模块版本**: 1.1.0
- **最后更新**: 2026-01-09
- **维护者**: CN_Language架构委员会
- **许可证**: MIT

## 文件结构

### 头文件
- `src/core/parser/statement_parsers/CN_parser_statements.h` - 主头文件，包含所有语句解析函数的声明

### 实现文件
- `src/core/parser/statement_parsers/CN_parser_statements_main.c` - 语句解析主模块
- `src/core/parser/statement_parsers/CN_parser_block_statements.c` - 代码块和表达式语句解析模块
- `src/core/parser/statement_parsers/CN_parser_conditional_statements.c` - 条件语句解析模块
- `src/core/parser/statement_parsers/CN_parser_control_statements.c` - 控制语句解析模块
- `src/core/parser/statement_parsers/CN_parser_loop_statements.c` - 循环语句解析模块
- `src/core/parser/statement_parsers/CN_parser_switch_statements.c` - switch语句解析模块
- `src/core/parser/statement_parsers/CN_parser_exception_statements.c` - 异常处理语句解析模块

## API 函数参考

### 主要语句解析函数

#### `F_parse_statement`
```c
Stru_AstNode_t* F_parse_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析单个语句，根据当前令牌类型调用相应的子解析函数。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 语句AST节点指针
- 失败: NULL（表示错误或语句结束）

**示例**:
```c
Stru_AstNode_t* stmt = F_parse_statement(parser);
if (stmt != NULL) {
    // 处理语句
}
```

#### `F_parse_expression_statement`
```c
Stru_AstNode_t* F_parse_expression_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析表达式语句，如：`表达式;`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 表达式语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: x = 42;
Stru_AstNode_t* expr_stmt = F_parse_expression_statement(parser);
```

#### `F_parse_block_statement`
```c
Stru_AstNode_t* F_parse_block_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析代码块语句，如：`{ 语句1; 语句2; }`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 代码块语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: { x = 1; y = 2; }
Stru_AstNode_t* block = F_parse_block_statement(parser);
```

#### `F_parse_if_statement`
```c
Stru_AstNode_t* F_parse_if_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析条件语句，如：`如果 (条件) { ... }`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 条件语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: 如果 (x > 0) { 返回 真; }
Stru_AstNode_t* if_stmt = F_parse_if_statement(parser);
```

#### `F_parse_while_statement`
```c
Stru_AstNode_t* F_parse_while_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析循环语句，如：`当 (条件) { ... }`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 循环语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: 当 (i < 10) { i = i + 1; }
Stru_AstNode_t* while_stmt = F_parse_while_statement(parser);
```

#### `F_parse_for_statement`
```c
Stru_AstNode_t* F_parse_for_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析for循环语句，如：`循环 (初始化; 条件; 更新) { ... }`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: for循环语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: 循环 (i = 0; i < 10; i = i + 1) { 打印(i); }
Stru_AstNode_t* for_stmt = F_parse_for_statement(parser);
```

### 控制语句解析函数

#### `F_parse_break_statement`
```c
Stru_AstNode_t* F_parse_break_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析中断语句，如：`中断;`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 中断语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: 中断;
Stru_AstNode_t* break_stmt = F_parse_break_statement(parser);
```

#### `F_parse_continue_statement`
```c
Stru_AstNode_t* F_parse_continue_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析继续语句，如：`继续;`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 继续语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: 继续;
Stru_AstNode_t* continue_stmt = F_parse_continue_statement(parser);
```

#### `F_parse_return_statement`
```c
Stru_AstNode_t* F_parse_return_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析返回语句，如：`返回 值;`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 返回语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: 返回 x + y;
Stru_AstNode_t* return_stmt = F_parse_return_statement(parser);
```

#### `F_parse_empty_statement`
```c
Stru_AstNode_t* F_parse_empty_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析空语句，如：`;`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 空语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: ;
Stru_AstNode_t* empty_stmt = F_parse_empty_statement(parser);
```

### 声明语句解析函数

#### `F_parse_variable_declaration_statement`
```c
Stru_AstNode_t* F_parse_variable_declaration_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析变量声明语句，如：`变量 名称 = 值;`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 变量声明语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: 变量 x = 42;
Stru_AstNode_t* var_decl = F_parse_variable_declaration_statement(parser);
```

### 辅助解析函数

#### `F_parse_statement_list`
```c
Stru_DynamicArray_t* F_parse_statement_list(Stru_ParserInterface_t* interface);
```

**描述**: 解析代码块中的语句列表。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 语句节点数组指针
- 失败: NULL

**示例**:
```c
// 解析代码块中的语句列表
Stru_DynamicArray_t* stmt_list = F_parse_statement_list(parser);
```

### 高级语句解析函数

#### `F_parse_switch_statement`
```c
Stru_AstNode_t* F_parse_switch_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析switch语句，如：`选择 (表达式) { 情况 值: 语句; 默认: 语句; }`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: switch语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: 选择 (x) { 情况 1: 返回 "一"; 默认: 返回 "其他"; }
Stru_AstNode_t* switch_stmt = F_parse_switch_statement(parser);
```

#### `F_parse_case_statement`
```c
Stru_AstNode_t* F_parse_case_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析case语句，如：`情况 表达式: 语句`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: case语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: 情况 1: 返回 "一";
Stru_AstNode_t* case_stmt = F_parse_case_statement(parser);
```

#### `F_parse_default_statement`
```c
Stru_AstNode_t* F_parse_default_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析default语句，如：`默认: 语句`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: default语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: 默认: 返回 "其他";
Stru_AstNode_t* default_stmt = F_parse_default_statement(parser);
```

#### `F_parse_try_catch_statement`
```c
Stru_AstNode_t* F_parse_try_catch_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析try-catch语句，如：`尝试 { ... } 捕获 (异常类型) { ... } 最终块 { ... }`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: try-catch语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: 尝试 { 危险操作(); } 捕获 (异常 e) { 处理异常(e); }
Stru_AstNode_t* try_catch_stmt = F_parse_try_catch_statement(parser);
```

#### `F_parse_throw_statement`
```c
Stru_AstNode_t* F_parse_throw_statement(Stru_ParserInterface_t* interface);
```

**描述**: 解析throw语句，如：`抛出 异常表达式;`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: throw语句AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: 抛出 新 异常("错误发生");
Stru_AstNode_t* throw_stmt = F_parse_throw_statement(parser);
```

## 语句语法

### 表达式语句
```
表达式;
```

### 代码块语句
```
{
    语句1;
    语句2;
    ...
}
```

### 条件语句
```
如果 (条件) {
    // 条件为真时执行
}
```

```
如果 (条件) {
    // 条件为真时执行
} 否则 {
    // 条件为假时执行
}
```

### 循环语句
```
当 (条件) {
    // 条件为真时重复执行
}
```

```
循环 (初始化; 条件; 更新) {
    // 条件为真时重复执行
}
```

### 控制语句
```
中断;      // 退出当前循环
继续;      // 跳过当前循环迭代
返回 值;   // 从函数返回
```

### 变量声明语句
```
变量 名称 = 值;
```

### Switch语句
```
选择 (表达式) {
    情况 值1: 语句1;
    情况 值2: 语句2;
    默认: 语句3;
}
```

### 异常处理语句
```
尝试 {
    // 可能抛出异常的代码
} 捕获 (异常类型 变量名) {
    // 异常处理代码
} 最终块 {
    // 无论是否发生异常都会执行的代码
}
```

## 使用示例

### 基本用法
```c
#include "core/parser/statement_parsers/CN_parser_statements.h"

// 获取语法分析器接口
Stru_ParserInterface_t* parser = F_create_parser_interface();
parser->initialize(parser, lexer);

// 解析语句
Stru_AstNode_t* stmt = F_parse_statement(parser);

// 检查语句类型
if (stmt != NULL) {
    switch (stmt->type) {
        case Eum_AST_NODE_EXPRESSION_STMT:
            printf("表达式语句\n");
            break;
        case Eum_AST_NODE_IF_STMT:
            printf("条件语句\n");
            break;
        case Eum_AST_NODE_WHILE_STMT:
            printf("循环语句\n");
            break;
        case Eum_AST_NODE_FOR_STMT:
            printf("for循环语句\n");
            break;
        case Eum_AST_NODE_BREAK_STMT:
            printf("中断语句\n");
            break;
        case Eum_AST_NODE_RETURN_STMT:
            printf("返回语句\n");
            break;
        case Eum_AST_NODE_VARIABLE_DECL_STMT:
            printf("变量声明语句\n");
            break;
        default:
            printf("其他语句类型\n");
    }
}

// 清理资源
parser->destroy(parser);
```

### 解析复杂语句结构
```c
// 解析嵌套条件语句
Stru_AstNode_t* nested_if = F_parse_statement(parser);

// 解析循环中的控制语句
Stru_AstNode_t* loop_with_break = F_parse_statement(parser);

// 解析带异常处理的代码块
Stru_AstNode_t* try_catch_block = F_parse_statement(parser);

// 解析switch语句
Stru_AstNode_t* switch_stmt = F_parse_statement(parser);
```

### 错误处理示例
```c
Stru_AstNode_t* stmt = F_parse_statement(parser);
if (stmt == NULL) {
    // 获取错误信息
    Stru_SyntaxError_t* error = parser->get_last_error(parser);
    if (error != NULL) {
        printf("语句解析错误: %s\n", error->message);
        printf("位置: 行 %d, 列 %d\n", error->line, error->column);
    }
    
    // 尝试错误恢复
    parser->recover_from_error(parser);
    
    // 继续解析后续语句
    stmt = F_parse_statement(parser);
}
```

## 设计原理

### 递归下降算法
语句解析模块使用递归下降算法：
1. `F_parse_statement()` 根据当前令牌类型分发到相应的解析函数
2. 每个语句类型有专门的解析函数
3. 支持嵌套语句结构
4. 通过函数调用链实现复杂的控制流结构

### 错误恢复机制
- 在解析失败时返回NULL
- 报告详细的错误信息
- 支持跳过错误语句继续解析
- 与语法错误处理模块集成

### 控制流处理
- 正确处理条件语句的嵌套
- 支持循环语句的嵌套
- 处理控制语句的作用域
- 实现异常处理的控制流

## 性能考虑

### 递归深度
- 语句解析可能涉及多层嵌套
- 最大递归深度受限于栈大小
- 复杂嵌套结构可能需要优化

### 内存使用
- 每个语句节点独立分配内存
- 支持语句节点重用
- 避免内存泄漏
- 使用动态数组管理语句列表

## 测试

### 单元测试
语句解析模块的单元测试位于：
- `tests/core/parser/test_parser_all.c` - 包含语句解析测试

### 测试用例类型
1. **基本语句测试**: 测试各种基本语句类型的解析
2. **控制流测试**: 测试条件语句、循环语句的嵌套
3. **错误处理测试**: 测试语法错误的检测和恢复
4. **复杂结构测试**: 测试switch语句、异常处理等复杂结构

### 运行测试
```bash
cd tests
make test_parser_all
./test_parser_all
```

## 扩展性

### 添加新的语句类型
1. 在头文件中添加新的语句解析函数声明
2. 实现相应的解析函数
3. 在`F_parse_statement()`中添加分发逻辑
4. 添加对应的AST节点类型
5. 更新测试用例

### 自定义控制流结构
1. 添加新的控制流解析函数
2. 实现对应的AST构建逻辑
3. 更新语法规范
4. 添加相应的测试用例

##
