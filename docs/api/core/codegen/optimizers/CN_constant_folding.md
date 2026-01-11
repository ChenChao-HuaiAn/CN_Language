# CN_Constant_Folding API 文档

## 概述

常量折叠（Constant Folding）是CN_Language编译器的基础优化算法之一，用于在编译时计算常量表达式，从而减少运行时开销。本模块实现了完整的常量折叠优化功能。

## 设计原则

1. **编译时计算**：在编译阶段计算常量表达式，避免运行时计算
2. **类型安全**：确保常量计算过程中的类型安全性
3. **错误处理**：正确处理常量计算中的错误情况
4. **性能优化**：高效遍历AST并应用常量折叠

## 核心数据结构

### 常量表达式类型

```c
/**
 * 常量表达式类型枚举
 */
enum Eum_ConstantType {
    Eum_CONST_INTEGER,      // 整数常量
    Eum_CONST_FLOAT,        // 浮点数常量
    Eum_CONST_BOOLEAN,      // 布尔常量
    Eum_CONST_STRING,       // 字符串常量
    Eum_CONST_NULL          // 空常量
};
```

### 常量折叠上下文

```c
/**
 * 常量折叠上下文结构
 */
typedef struct Stru_ConstantFoldingContext_t {
    size_t folded_count;     // 已折叠的常量数量
    size_t error_count;      // 错误数量
    Stru_MemoryContext_t* memory_ctx;  // 内存上下文
    Stru_ErrorReporter_t* error_reporter;  // 错误报告器
} Stru_ConstantFoldingContext_t;
```

## API 函数

### 主要接口函数

#### `apply_constant_folding`

```c
/**
 * @brief 应用常量折叠优化
 * 
 * 对AST应用常量折叠优化，在编译时计算常量表达式。
 * 
 * @param ast AST根节点
 * @param context 优化上下文
 * @return bool 优化是否成功应用
 */
bool apply_constant_folding(Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);
```

**参数说明：**
- `ast`: AST抽象语法树的根节点
- `context`: 优化上下文，包含内存管理、错误报告等

**返回值：**
- `true`: 优化成功应用
- `false`: 优化失败

**使用示例：**
```c
Stru_OptimizationContext_t context = create_optimization_context();
Stru_AstNode_t* ast = parse_source_code(source_code);

if (apply_constant_folding(ast, &context)) {
    printf("常量折叠优化成功应用\n");
} else {
    printf("常量折叠优化失败\n");
}
```

#### `is_constant_expression`

```c
/**
 * @brief 检查表达式是否为常量表达式
 * 
 * 检查给定的AST节点是否表示常量表达式。
 * 
 * @param node AST节点
 * @return bool 如果是常量表达式返回true，否则返回false
 */
bool is_constant_expression(Stru_AstNode_t* node);
```

**参数说明：**
- `node`: 要检查的AST节点

**返回值：**
- `true`: 节点是常量表达式
- `false`: 节点不是常量表达式

**使用示例：**
```c
Stru_AstNode_t* expr = create_literal_node(Eum_AST_INTEGER_LITERAL, "42");
if (is_constant_expression(expr)) {
    printf("这是一个常量表达式\n");
}
```

#### `evaluate_constant_expression`

```c
/**
 * @brief 评估常量表达式
 * 
 * 评估常量表达式并返回其值。
 * 
 * @param node 常量表达式AST节点
 * @param context 优化上下文
 * @return Stru_AstNode_t* 表示常量值的AST节点，失败返回NULL
 */
Stru_AstNode_t* evaluate_constant_expression(Stru_AstNode_t* node, Stru_OptimizationContext_t* context);
```

**参数说明：**
- `node`: 常量表达式AST节点
- `context`: 优化上下文

**返回值：**
- 成功：表示常量值的AST节点
- 失败：NULL

**使用示例：**
```c
// 创建表达式: 2 + 3 * 4
Stru_AstNode_t* expr = create_binary_expression(
    Eum_AST_ADD,
    create_literal_node(Eum_AST_INTEGER_LITERAL, "2"),
    create_binary_expression(
        Eum_AST_MULTIPLY,
        create_literal_node(Eum_AST_INTEGER_LITERAL, "3"),
        create_literal_node(Eum_AST_INTEGER_LITERAL, "4")
    )
);

Stru_AstNode_t* result = evaluate_constant_expression(expr, &context);
if (result != NULL) {
    printf("表达式计算结果: %s\n", result->value);
}
```

### 辅助函数

#### `fold_binary_expression`

```c
/**
 * @brief 折叠二元表达式
 * 
 * 折叠二元常量表达式。
 * 
 * @param node 二元表达式AST节点
 * @param context 优化上下文
 * @return Stru_AstNode_t* 折叠后的AST节点，失败返回NULL
 */
Stru_AstNode_t* fold_binary_expression(Stru_AstNode_t* node, Stru_OptimizationContext_t* context);
```

**支持的二元操作：**
- 算术运算：`+`, `-`, `*`, `/`, `%`
- 比较运算：`==`, `!=`, `<`, `>`, `<=`, `>=`
- 逻辑运算：`&&`, `||`
- 位运算：`&`, `|`, `^`, `<<`, `>>`

#### `fold_unary_expression`

```c
/**
 * @brief 折叠一元表达式
 * 
 * 折叠一元常量表达式。
 * 
 * @param node 一元表达式AST节点
 * @param context 优化上下文
 * @return Stru_AstNode_t* 折叠后的AST节点，失败返回NULL
 */
Stru_AstNode_t* fold_unary_expression(Stru_AstNode_t* node, Stru_OptimizationContext_t* context);
```

**支持的一元操作：**
- 算术负号：`-`
- 逻辑非：`!`
- 位非：`~`
- 类型转换

### 统计函数

#### `get_constant_folding_stats`

```c
/**
 * @brief 获取常量折叠统计信息
 * 
 * 获取常量折叠优化的统计信息。
 * 
 * @param context 优化上下文
 * @param folded_count 输出参数，已折叠的常量数量
 * @param error_count 输出参数，错误数量
 */
void get_constant_folding_stats(Stru_OptimizationContext_t* context, 
                                size_t* folded_count, size_t* error_count);
```

**使用示例：**
```c
size_t folded_count, error_count;
get_constant_folding_stats(&context, &folded_count, &error_count);
printf("常量折叠统计: 已折叠 %zu 个常量，错误 %zu 个\n", folded_count, error_count);
```

### 测试函数

#### `test_constant_folding`

```c
/**
 * @brief 测试常量折叠算法
 * 
 * 运行常量折叠算法的测试用例。
 */
void test_constant_folding(void);
```

**测试用例包括：**
1. 基本算术运算折叠
2. 复杂表达式折叠
3. 类型转换折叠
4. 错误处理测试
5. 边界条件测试

## 算法实现细节

### 常量折叠流程

1. **AST遍历**：深度优先遍历AST树
2. **常量检测**：识别常量表达式节点
3. **表达式评估**：计算常量表达式的值
4. **节点替换**：用计算结果替换原表达式
5. **错误处理**：处理计算过程中的错误

### 支持的常量类型

1. **整数常量**：支持所有整数运算
2. **浮点数常量**：支持浮点运算
3. **布尔常量**：支持逻辑运算
4. **字符串常量**：支持字符串连接
5. **空常量**：支持空值处理

### 优化策略

1. **惰性计算**：只在需要时计算常量表达式
2. **缓存结果**：缓存已计算的常量表达式结果
3. **增量更新**：支持AST的增量更新
4. **错误恢复**：优雅处理计算错误

## 使用示例

### 示例1：基本常量折叠

```c
#include "CN_constant_folding.h"

int main() {
    // 创建优化上下文
    Stru_OptimizationContext_t context;
    initialize_optimization_context(&context);
    
    // 创建AST: 2 + 3 * 4
    Stru_AstNode_t* expr = create_binary_expression(
        Eum_AST_ADD,
        create_literal_node(Eum_AST_INTEGER_LITERAL, "2"),
        create_binary_expression(
            Eum_AST_MULTIPLY,
            create_literal_node(Eum_AST_INTEGER_LITERAL, "3"),
            create_literal_node(Eum_AST_INTEGER_LITERAL, "4")
        )
    );
    
    // 应用常量折叠
    if (apply_constant_folding(expr, &context)) {
        // 表达式应被折叠为 14
        printf("常量折叠成功\n");
    }
    
    // 清理资源
    destroy_ast_node(expr);
    cleanup_optimization_context(&context);
    
    return 0;
}
```

### 示例2：复杂表达式折叠

```c
// 折叠复杂表达式: (10 + 20) * (30 - 15) / 5
Stru_AstNode_t* complex_expr = create_binary_expression(
    Eum_AST_DIVIDE,
    create_binary_expression(
        Eum_AST_MULTIPLY,
        create_binary_expression(
            Eum_AST_ADD,
            create_literal_node(Eum_AST_INTEGER_LITERAL, "10"),
            create_literal_node(Eum_AST_INTEGER_LITERAL, "20")
        ),
        create_binary_expression(
            Eum_AST_SUBTRACT,
            create_literal_node(Eum_AST_INTEGER_LITERAL, "30"),
            create_literal_node(Eum_AST_INTEGER_LITERAL, "15")
        )
    ),
    create_literal_node(Eum_AST_INTEGER_LITERAL, "5")
);

// 应用常量折叠后，表达式应被折叠为 90
```

## 性能考虑

1. **时间复杂度**：O(n)，其中n是AST节点数量
2. **空间复杂度**：O(h)，其中h是AST的最大深度
3. **内存使用**：使用内存上下文管理临时对象
4. **缓存策略**：实现表达式结果缓存以提高性能

## 错误处理

### 常见错误类型

1. **除零错误**：处理除法运算中的除零情况
2. **溢出错误**：处理整数和浮点数溢出
3. **类型错误**：处理不兼容的类型操作
4. **语法错误**：处理无效的AST结构

### 错误报告

所有错误通过优化上下文中的错误报告器进行报告，支持：
- 错误代码
- 错误消息
- 错误位置
- 错误严重程度

## 集成指南

### 与优化器框架集成

常量折叠模块通过标准优化器接口与CN_Language优化器框架集成：

```c
// 注册常量折叠优化器
Stru_OptimizerInterface_t constant_folding_optimizer = {
    .name = "constant_folding",
    .description = "常量折叠优化器",
    .apply = apply_constant_folding,
    .test = test_constant_folding,
    .get_stats = get_constant_folding_stats
};

// 注册到优化器管理器
register_optimizer(&constant_folding_optimizer);
```

### 构建配置

在Makefile中添加构建配置：

```makefile
# 常量折叠模块
CN_CONSTANT_FOLDING_SRC = src/core/codegen/optimizers/basic_optimizer/CN_constant_folding.c
CN_CONSTANT_FOLDING_OBJ = $(BUILD_DIR)/core/codegen/optimizers/basic_optimizer/CN_constant_folding.o

$(CN_CONSTANT_FOLDING_OBJ): $(CN_CONSTANT_FOLDING_SRC)
	$(CC) $(CFLAGS) -c $< -o $@
```

## 测试覆盖率

常量折叠模块的测试覆盖率目标：
- 语句覆盖率：≥90%
- 分支覆盖率：≥85%
- 函数覆盖率：≥95%

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-10 | 初始版本，实现基本常量折叠功能 |
| 1.1.0 | 2026-01-11 | 添加复杂表达式支持和错误处理 |

## 相关文档

1. [CN_Language优化器接口文档](../CN_optimizer_interface.md)
2. [AST数据结构文档](../../ast/CN_ast_api.md)
3. [基础优化器README](../../../../src/core/codegen/optimizers/basic_optimizer/README.md)

---

*文档最后更新: 2026年1月11日*
*CN_Language开发团队*
