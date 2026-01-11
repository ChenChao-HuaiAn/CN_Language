# CN_Common_Subexpression_Elimination API 文档

## 概述

`CN_common_subexpr.h` 定义了CN_Language公共子表达式消除（CSE）优化算法的接口。该算法识别并消除重复的表达式计算，通过重用计算结果来提高代码效率。

## 文件信息

- **文件名**: `CN_common_subexpr.h`
- **位置**: `src/core/codegen/optimizers/basic_optimizer/`
- **版本**: 1.0.0
- **作者**: CN_Language架构委员会
- **创建日期**: 2026-01-11

## 包含文件

```c
#include "../../../ast/CN_ast.h"
#include "../CN_basic_optimizer.h"
```

## 枚举类型

### Eum_ExpressionType

表达式类型枚举，用于标识不同类型的表达式。

```c
typedef enum Eum_ExpressionType {
    Eum_EXPR_BINARY,          ///< 二元表达式
    Eum_EXPR_UNARY,           ///< 一元表达式
    Eum_EXPR_LITERAL,         ///< 字面量表达式
    Eum_EXPR_VARIABLE,        ///< 变量引用
    Eum_EXPR_FUNCTION_CALL,   ///< 函数调用
    Eum_EXPR_COMPLEX          ///< 复杂表达式
} Eum_ExpressionType;
```

### Eum_CSE_OptimizationLevel

CSE优化级别枚举，控制优化的激进程度。

```c
typedef enum Eum_CSE_OptimizationLevel {
    Eum_CSE_LEVEL_NONE = 0,   ///< 无CSE优化
    Eum_CSE_LEVEL_BASIC,      ///< 基本CSE（仅局部作用域）
    Eum_CSE_LEVEL_AGGRESSIVE, ///< 激进CSE（跨基本块）
    Eum_CSE_LEVEL_GLOBAL      ///< 全局CSE（跨函数）
} Eum_CSE_OptimizationLevel;
```

## 结构体类型

### Stru_ExpressionHash_t

表达式哈希结构体，用于快速比较表达式。

```c
typedef struct Stru_ExpressionHash_t {
    uint64_t hash_value;      ///< 哈希值
    size_t expression_size;   ///< 表达式大小（字节）
    Eum_ExpressionType type;  ///< 表达式类型
    const char* operator;     ///< 操作符（如果有）
    size_t child_count;       ///< 子表达式数量
    Stru_ExpressionHash_t** child_hashes; ///< 子表达式哈希数组
} Stru_ExpressionHash_t;
```

### Stru_ExpressionEntry_t

表达式条目结构体，存储在哈希表中。

```c
typedef struct Stru_ExpressionEntry_t {
    Stru_ExpressionHash_t* hash;      ///< 表达式哈希
    Stru_AstNode_t* expression;       ///< 原始表达式节点
    Stru_AstNode_t* replacement;      ///< 替换表达式（临时变量）
    size_t occurrence_count;          ///< 出现次数
    size_t first_occurrence_line;     ///< 首次出现行号
    bool is_pure;                     ///< 是否为纯表达式（无副作用）
    bool can_be_eliminated;           ///< 是否可以消除
} Stru_ExpressionEntry_t;
```

### Stru_CSE_Context_t

CSE上下文结构体，包含CSE优化的状态和配置。

```c
typedef struct Stru_CSE_Context_t {
    Stru_AstNode_t* ast;              ///< AST根节点
    Stru_OptimizationContext_t* opt_context; ///< 优化上下文
    Eum_CSE_OptimizationLevel level;  ///< CSE优化级别
    
    // 哈希表相关
    Stru_ExpressionEntry_t** hash_table; ///< 表达式哈希表
    size_t table_size;                ///< 哈希表大小
    size_t entry_count;               ///< 条目数量
    
    // 统计信息
    size_t expressions_found;         ///< 找到的表达式总数
    size_t duplicates_eliminated;     ///< 消除的重复表达式数
    size_t temp_variables_created;    ///< 创建的临时变量数
    size_t memory_saved;              ///< 节省的内存（估计值）
    
    // 配置选项
    bool aggressive_mode;             ///< 激进模式
    bool cross_block_optimization;    ///< 跨基本块优化
    bool preserve_side_effects;       ///< 保留副作用
    size_t min_occurrences;           ///< 最小出现次数阈值
    
    // 错误处理
    size_t error_count;               ///< 错误数量
    char** errors;                    ///< 错误信息数组
    size_t error_capacity;            ///< 错误数组容量
} Stru_CSE_Context_t;
```

## 函数接口

### 1. `apply_common_subexpr_elimination`

**功能：** 应用公共子表达式消除优化

**原型：**
```c
bool apply_common_subexpr_elimination(Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);
```

**参数：**
- `ast`: 抽象语法树根节点
- `context`: 优化上下文

**返回值：**
- 优化成功应用返回`true`，失败返回`false`

**算法描述：**
公共子表达式消除（CSE）是一种编译器优化技术，用于识别并消除重复的表达式计算。算法步骤：

1. **表达式识别**：遍历AST，识别所有可优化的表达式
2. **哈希计算**：为每个表达式计算哈希值，用于快速比较
3. **重复检测**：使用哈希表检测重复的表达式
4. **临时变量创建**：为重复表达式创建临时变量
5. **表达式替换**：用临时变量引用替换重复的表达式
6. **代码重构**：将临时变量赋值移动到合适的位置

**支持的表达式类型：**
- 算术表达式：`a + b`, `x * y`, `(a + b) * c`
- 比较表达式：`x > y`, `a == b`
- 逻辑表达式：`p && q`, `!flag`
- 位运算表达式：`x & mask`, `y << 2`

**优化示例：**
```c
// 优化前
result1 = (a + b) * c;
result2 = (a + b) * c;  // 重复计算

// 优化后
temp = (a + b) * c;
result1 = temp;
result2 = temp;
```

### 2. `create_cse_context`

**功能：** 创建CSE上下文

**原型：**
```c
Stru_CSE_Context_t* create_cse_context(Stru_AstNode_t* ast, Stru_OptimizationContext_t* opt_context);
```

**参数：**
- `ast`: AST根节点
- `opt_context`: 优化上下文

**返回值：**
- 成功：返回新创建的CSE上下文
- 失败：返回NULL

**说明：**
- 调用者负责在不再使用时调用`destroy_cse_context()`释放资源
- 上下文包含CSE优化的所有状态和配置

### 3. `destroy_cse_context`

**功能：** 销毁CSE上下文

**原型：**
```c
void destroy_cse_context(Stru_CSE_Context_t* context);
```

**参数：**
- `context`: CSE上下文

**说明：**
- 释放上下文及其所有相关资源
- 如果context为NULL，函数不执行任何操作

### 4. `compute_expression_hash`

**功能：** 计算表达式哈希值

**原型：**
```c
Stru_ExpressionHash_t* compute_expression_hash(Stru_AstNode_t* expression);
```

**参数：**
- `expression`: 表达式AST节点

**返回值：**
- 成功：返回表达式哈希结构体
- 失败：返回NULL

**哈希算法：**
使用递归哈希算法，考虑：
- 表达式类型和操作符
- 操作数的类型和值
- 子表达式的哈希值
- 表达式的大小和结构

### 5. `find_duplicate_expressions`

**功能：** 查找重复表达式

**原型：**
```c
size_t find_duplicate_expressions(Stru_CSE_Context_t* context, Stru_ExpressionEntry_t*** duplicates);
```

**参数：**
- `context`: CSE上下文
- `duplicates`: 输出参数，重复表达式条目数组

**返回值：**
- 重复表达式的数量

**说明：**
- 调用者负责释放返回的数组（但不释放数组中的条目）
- 数组按出现次数降序排序

### 6. `eliminate_duplicate_expression`

**功能：** 消除重复表达式

**原型：**
```c
bool eliminate_duplicate_expression(Stru_CSE_Context_t* context, Stru_ExpressionEntry_t* entry);
```

**参数：**
- `context`: CSE上下文
- `entry`: 表达式条目

**返回值：**
- 消除成功返回`true`，失败返回`false`

**消除策略：**
1. 创建临时变量存储表达式结果
2. 将临时变量赋值插入到第一个出现位置之前
3. 用临时变量引用替换所有重复出现
4. 更新控制流图和数据依赖关系

### 7. `get_cse_statistics`

**功能：** 获取CSE统计信息

**原型：**
```c
void get_cse_statistics(const Stru_CSE_Context_t* context, 
                        size_t* expressions_found,
                        size_t* duplicates_eliminated,
                        size_t* temp_variables_created,
                        size_t* memory_saved);
```

**参数：**
- `context`: CSE上下文
- `expressions_found`: 输出参数，找到的表达式总数
- `duplicates_eliminated`: 输出参数，消除的重复表达式数
- `temp_variables_created`: 输出参数，创建的临时变量数
- `memory_saved`: 输出参数，节省的内存（估计值）

### 8. `configure_cse_optimization`

**功能：** 配置CSE优化参数

**原型：**
```c
bool configure_cse_optimization(Stru_CSE_Context_t* context, 
                                Eum_CSE_OptimizationLevel level,
                                bool aggressive_mode,
                                size_t min_occurrences);
```

**参数：**
- `context`: CSE上下文
- `level`: CSE优化级别
- `aggressive_mode`: 是否启用激进模式
- `min_occurrences`: 最小出现次数阈值

**返回值：**
- 配置成功返回`true`，失败返回`false`

**配置选项说明：**
- **优化级别**：
  - `Eum_CSE_LEVEL_BASIC`: 仅优化同一基本块内的重复表达式
  - `Eum_CSE_LEVEL_AGGRESSIVE`: 优化跨基本块的重复表达式
  - `Eum_CSE_LEVEL_GLOBAL`: 优化整个函数内的重复表达式
- **最小出现次数**：只有出现次数超过此阈值的表达式才会被优化

### 9. `is_expression_eligible_for_cse`

**功能：** 检查表达式是否适合CSE优化

**原型：**
```c
bool is_expression_eligible_for_cse(Stru_AstNode_t* expression, const Stru_CSE_Context_t* context);
```

**参数：**
- `expression`: 表达式AST节点
- `context`: CSE上下文

**返回值：**
- 适合CSE优化返回`true`，否则返回`false`

**适合CSE的条件：**
1. 表达式是纯的（无副作用）
2. 表达式结果在优化范围内可重用
3. 表达式计算成本高于临时变量访问成本
4. 表达式出现次数超过最小阈值

### 10. `test_common_subexpr_elimination`

**功能：** 测试公共子表达式消除算法

**原型：**
```c
void test_common_subexpr_elimination(void);
```

**说明：**
- 运行CSE算法的测试用例
- 输出测试结果和性能统计

## 使用示例

### 基本使用

```c
#include "src/core/codegen/optimizers/basic_optimizer/CN_common_subexpr.h"

int main(void)
{
    // 创建AST和优化上下文
    Stru_AstNode_t* ast = ...;
    Stru_OptimizationContext_t* opt_context = ...;
    
    // 应用公共子表达式消除
    if (apply_common_subexpr_elimination(ast, opt_context)) {
        printf("公共子表达式消除优化成功应用\n");
        
        // 获取统计信息
        Stru_CSE_Context_t* cse_context = create_cse_context(ast, opt_context);
        if (cse_context) {
            size_t found, eliminated, temps, saved;
            get_cse_statistics(cse_context, &found, &eliminated, &temps, &saved);
            
            printf("找到 %zu 个表达式，消除了 %zu 个重复\n", found, eliminated);
            printf("创建了 %zu 个临时变量，节省了约 %zu 字节内存\n", temps, saved);
            
            destroy_cse_context(cse_context);
        }
    }
    
    return EXIT_SUCCESS;
}
```

### 高级配置

```c
#include "src/core/codegen/optimizers/basic_optimizer/CN_common_subexpr.h"

int main(void)
{
    // 创建AST和优化上下文
    Stru_AstNode_t* ast = ...;
    Stru_OptimizationContext_t* opt_context = ...;
    
    // 创建CSE上下文
    Stru_CSE_Context_t* cse_context = create_cse_context(ast, opt_context);
    if (!cse_context) {
        fprintf(stderr, "创建CSE上下文失败\n");
        return EXIT_FAILURE;
    }
    
    // 配置CSE优化
    if (!configure_cse_optimization(cse_context, 
                                    Eum_CSE_LEVEL_AGGRESSIVE,
                                    true,   // 激进模式
                                    2)) {   // 最小出现次数
        fprintf(stderr, "配置CSE优化失败\n");
        destroy_cse_context(cse_context);
        return EXIT_FAILURE;
    }
    
    // 查找重复表达式
    Stru_ExpressionEntry_t** duplicates = NULL;
    size_t duplicate_count = find_duplicate_expressions(cse_context, &duplicates);
    
    printf("找到 %zu 组重复表达式\n", duplicate_count);
    
    // 消除重复表达式
    size_t eliminated_count = 0;
    for (size_t i = 0; i < duplicate_count; i++) {
        if (eliminate_duplicate_expression(cse_context, duplicates[i])) {
            eliminated_count++;
        }
    }
    
    printf("成功消除了 %zu 组重复表达式\n", eliminated_count);
    
    // 清理资源
    if (duplicates) {
        free(duplicates);
    }
    destroy_cse_context(cse_context);
    
    return EXIT_SUCCESS;
}
```

### 集成到优化器插件

```c
#include "src/core/codegen/optimizers/basic_optimizer/CN_common_subexpr.h"
#include "src/core/codegen/CN_optimizer_interface.h"

// 在优化器插件中集成CSE
Stru_OptimizationResult_t* optimize_common_subexpr(Stru_OptimizerPluginInterface_t* interface,
                                                   Stru_OptimizationContext_t* context)
{
    Stru_OptimizationResult_t* result = F_create_optimization_result();
    if (!result) {
        return NULL;
    }
    
    result->success = false;
    
    if (!context || !context->ast) {
        add_error_to_result(result, "无效的优化上下文或AST");
        return result;
    }
    
    // 应用CSE优化
    if (apply_common_subexpr_elimination(context->ast, context)) {
        result->success = true;
        result->improvement_ratio = calculate_improvement_ratio(context);
        
        // 收集统计信息
        Stru_CSE_Context_t* cse_context = create_cse_context(context->ast, context);
        if (cse_context) {
            size_t found, eliminated, temps, saved;
            get_cse_statistics(cse_context, &found, &eliminated, &temps, &saved);
            
            // 将统计信息添加到结果中
            add_statistic_to_result(result, "expressions_found", found);
            add_statistic_to_result(result, "duplicates_eliminated", eliminated);
            add_statistic_to_result(result, "temp_variables_created", temps);
            add_statistic_to_result(result, "memory_saved_bytes", saved);
            
            destroy_cse_context(cse_context);
        }
    } else {
        add_error_to_result(result, "公共子表达式消除优化失败");
    }
    
    return result;
}
```

## 算法细节

### 哈希计算算法

表达式哈希使用递归算法计算：

```c
uint64_t compute_hash_recursive(Stru_AstNode_t* node, int depth) {
    if (!node || depth > MAX_DEPTH) {
        return DEFAULT_HASH;
    }
    
    uint64_t hash = FNV_OFFSET_BASIS;
    
    // 包含节点类型
    hash = fnv1a_hash(hash, (uint8_t*)&node->type, sizeof(node->type));
    
    // 包含操作符（如果有）
    if (node->operator) {
        hash = fnv1a_hash_string(hash, node->operator);
    }
    
    // 递归包含子节点
    for (int i = 0; i < node->child_count; i++) {
        uint64_t child_hash = compute_hash_recursive(node->children[i], depth + 1);
        hash = fnv1a_hash(hash, (uint8_t*)&child_hash, sizeof(child_hash));
    }
    
    return hash;
}
```

### 临时变量命名策略

CSE算法创建临时变量时使用以下命名策略：

```c
// 临时变量命名模式：_cse_temp_N
// 其中N是递增的数字
char* generate_temp_var_name(Stru_CSE_Context_t
