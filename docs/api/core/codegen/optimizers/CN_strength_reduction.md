# CN_Strength_Reduction API 文档

## 概述

`CN_strength_reduction.h` 定义了CN_Language强度削减优化算法的接口。该算法用更高效的运算替换昂贵的运算，例如用移位操作替换乘除操作，从而提高代码执行效率。

## 文件信息

- **文件名**: `CN_strength_reduction.h`
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

### Eum_StrengthReductionPattern

强度削减模式枚举，定义支持的优化模式。

```c
typedef enum Eum_StrengthReductionPattern {
    Eum_SR_MULTIPLY_BY_POWER_OF_TWO = 0,   ///< 乘以2的幂次方
    Eum_SR_DIVIDE_BY_POWER_OF_TWO,         ///< 除以2的幂次方
    Eum_SR_MULTIPLY_BY_CONSTANT,           ///< 乘以常数
    Eum_SR_DIVIDE_BY_CONSTANT,             ///< 除以常数
    Eum_SR_MODULO_BY_POWER_OF_TWO,         ///< 对2的幂次方取模
    Eum_SR_POWER_OF_TWO_MULTIPLY,          ///< 2的幂次方乘法
    Eum_SR_IDENTITY_OPERATIONS,            ///< 恒等操作
    Eum_SR_ZERO_OPERATIONS,                ///< 零操作
    Eum_SR_ONE_OPERATIONS,                 ///< 一操作
    Eum_SR_COMPARE_WITH_CONSTANT,          ///< 与常数比较
    Eum_SR_BITWISE_OPTIMIZATIONS,          ///< 位运算优化
    Eum_SR_ARITHMETIC_IDENTITIES,          ///< 算术恒等式
    Eum_SR_LOGICAL_IDENTITIES,             ///< 逻辑恒等式
    Eum_SR_CUSTOM_PATTERN                  ///< 自定义模式
} Eum_StrengthReductionPattern;
```

### Eum_SR_OptimizationLevel

强度削减优化级别枚举，控制优化的激进程度。

```c
typedef enum Eum_SR_OptimizationLevel {
    Eum_SR_LEVEL_NONE = 0,          ///< 无强度削减优化
    Eum_SR_LEVEL_SAFE,              ///< 安全优化（仅明显优化）
    Eum_SR_LEVEL_MODERATE,          ///< 适度优化（包括常见模式）
    Eum_SR_LEVEL_AGGRESSIVE,        ///< 激进优化（包括所有已知模式）
    Eum_SR_LEVEL_EXPERIMENTAL       ///< 实验性优化（可能改变语义）
} Eum_SR_OptimizationLevel;
```

## 结构体类型

### Stru_StrengthReductionPattern_t

强度削减模式结构体，定义具体的优化模式。

```c
typedef struct Stru_StrengthReductionPattern_t {
    Eum_StrengthReductionPattern type;     ///< 模式类型
    const char* description;               ///< 模式描述
    const char* original_pattern;          ///< 原始模式（字符串表示）
    const char* optimized_pattern;         ///< 优化后模式（字符串表示）
    bool (*match_function)(Stru_AstNode_t* node); ///< 匹配函数
    Stru_AstNode_t* (*apply_function)(Stru_AstNode_t* node); ///< 应用函数
    bool is_safe;                          ///< 是否安全（不改变语义）
    double improvement_ratio;              ///< 改进比例估计
    size_t min_occurrences;                ///< 最小出现次数阈值
} Stru_StrengthReductionPattern_t;
```

### Stru_StrengthReductionContext_t

强度削减上下文结构体，包含SR优化的状态和配置。

```c
typedef struct Stru_StrengthReductionContext_t {
    Stru_AstNode_t* ast;                   ///< AST根节点
    Stru_OptimizationContext_t* opt_context; ///< 优化上下文
    Eum_SR_OptimizationLevel level;        ///< SR优化级别
    
    // 模式匹配相关
    Stru_StrengthReductionPattern_t** patterns; ///< 可用模式数组
    size_t pattern_count;                  ///< 模式数量
    size_t pattern_capacity;               ///< 模式数组容量
    
    // 统计信息
    size_t patterns_matched;               ///< 匹配的模式数量
    size_t optimizations_applied;          ///< 应用的优化数量
    size_t operations_saved;               ///< 节省的操作数（估计）
    size_t cycles_saved;                   ///< 节省的时钟周期（估计）
    
    // 配置选项
    bool enable_aggressive_optimizations;  ///< 启用激进优化
    bool preserve_precision;               ///< 保留精度（浮点运算）
    bool consider_platform_specific;       ///< 考虑平台特定优化
    size_t max_pattern_depth;              ///< 最大模式匹配深度
    
    // 错误处理
    size_t error_count;                    ///< 错误数量
    char** errors;                         ///< 错误信息数组
    size_t error_capacity;                 ///< 错误数组容量
} Stru_StrengthReductionContext_t;
```

## 函数接口

### 1. `apply_strength_reduction`

**功能：** 应用强度削减优化

**原型：**
```c
bool apply_strength_reduction(Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);
```

**参数：**
- `ast`: 抽象语法树根节点
- `context`: 优化上下文

**返回值：**
- 优化成功应用返回`true`，失败返回`false`

**算法描述：**
强度削减是一种编译器优化技术，用更高效（通常更廉价）的运算替换昂贵的运算。算法步骤：

1. **模式识别**：遍历AST，识别可优化的运算模式
2. **安全性检查**：验证优化不会改变程序语义
3. **模式应用**：用更高效的运算替换原始运算
4. **结果验证**：验证优化后的正确性

**支持的优化模式：**

#### 乘法优化
- `x * 2` → `x << 1`
- `x * 4` → `x << 2`
- `x * 8` → `x << 3`
- `x * 16` → `x << 4`

#### 除法优化
- `x / 2` → `x >> 1` (仅适用于无符号整数或已知非负数)
- `x / 4` → `x >> 2`
- `x / 8` → `x >> 3`

#### 取模优化
- `x % 2` → `x & 1`
- `x % 4` → `x & 3`
- `x % 8` → `x & 7`

#### 常数乘法优化
- `x * 3` → `(x << 1) + x`
- `x * 5` → `(x << 2) + x`
- `x * 6` → `(x << 2) + (x << 1)`
- `x * 7` → `(x << 3) - x`
- `x * 9` → `(x << 3) + x`
- `x * 10` → `(x << 3) + (x << 1)`

#### 恒等操作优化
- `x * 1` → `x`
- `x / 1` → `x`
- `x + 0` → `x`
- `x - 0` → `x`
- `x * 0` → `0`
- `0 / x` → `0` (x ≠ 0)
- `x & x` → `x`
- `x | x` → `x`
- `x ^ x` → `0`

#### 比较优化
- `x < 0` → 根据x的类型优化
- `x >= 0` → 根据x的类型优化

### 2. `create_strength_reduction_context`

**功能：** 创建强度削减上下文

**原型：**
```c
Stru_StrengthReductionContext_t* create_strength_reduction_context(
    Stru_AstNode_t* ast, 
    Stru_OptimizationContext_t* opt_context);
```

**参数：**
- `ast`: AST根节点
- `opt_context`: 优化上下文

**返回值：**
- 成功：返回新创建的SR上下文
- 失败：返回NULL

**说明：**
- 调用者负责在不再使用时调用`destroy_strength_reduction_context()`释放资源
- 上下文根据优化级别预加载相应的优化模式

### 3. `destroy_strength_reduction_context`

**功能：** 销毁强度削减上下文

**原型：**
```c
void destroy_strength_reduction_context(Stru_StrengthReductionContext_t* context);
```

**参数：**
- `context`: SR上下文

**说明：**
- 释放上下文及其所有相关资源
- 如果context为NULL，函数不执行任何操作

### 4. `register_strength_reduction_pattern`

**功能：** 注册强度削减模式

**原型：**
```c
bool register_strength_reduction_pattern(
    Stru_StrengthReductionContext_t* context,
    const Stru_StrengthReductionPattern_t* pattern);
```

**参数：**
- `context`: SR上下文
- `pattern`: 要注册的模式

**返回值：**
- 注册成功返回`true`，失败返回`false`

**说明：**
- 允许动态添加自定义优化模式
- 模式按优先级排序，高优先级模式先尝试

### 5. `find_optimizable_operations`

**功能：** 查找可优化的操作

**原型：**
```c
size_t find_optimizable_operations(
    Stru_StrengthReductionContext_t* context,
    Stru_AstNode_t*** operations);
```

**参数：**
- `context`: SR上下文
- `operations`: 输出参数，可优化操作节点数组

**返回值：**
- 可优化操作的数量

**说明：**
- 调用者负责释放返回的数组（但不释放数组中的节点）
- 数组按优化潜力降序排序

### 6. `apply_pattern_to_operation`

**功能：** 对操作应用优化模式

**原型：**
```c
bool apply_pattern_to_operation(
    Stru_StrengthReductionContext_t* context,
    Stru_AstNode_t* operation,
    const Stru_StrengthReductionPattern_t* pattern);
```

**参数：**
- `context`: SR上下文
- `operation`: 要优化的操作节点
- `pattern`: 要应用的优化模式

**返回值：**
- 优化成功应用返回`true`，失败返回`false`

### 7. `get_strength_reduction_statistics`

**功能：** 获取强度削减统计信息

**原型：**
```c
void get_strength_reduction_statistics(
    const Stru_StrengthReductionContext_t* context,
    size_t* patterns_matched,
    size_t* optimizations_applied,
    size_t* operations_saved,
    size_t* cycles_saved);
```

**参数：**
- `context`: SR上下文
- `patterns_matched`: 输出参数，匹配的模式数量
- `optimizations_applied`: 输出参数，应用的优化数量
- `operations_saved`: 输出参数，节省的操作数
- `cycles_saved`: 输出参数，节省的时钟周期

### 8. `configure_strength_reduction`

**功能：** 配置强度削减优化参数

**原型：**
```c
bool configure_strength_reduction(
    Stru_StrengthReductionContext_t* context,
    Eum_SR_OptimizationLevel level,
    bool enable_aggressive_optimizations,
    bool preserve_precision);
```

**参数：**
- `context`: SR上下文
- `level`: SR优化级别
- `enable_aggressive_optimizations`: 是否启用激进优化
- `preserve_precision`: 是否保留精度（浮点运算）

**返回值：**
- 配置成功返回`true`，失败返回`false`

### 9. `is_power_of_two`

**功能：** 检查数字是否为2的幂次方

**原型：**
```c
bool is_power_of_two(long value);
```

**参数：**
- `value`: 要检查的数值

**返回值：**
- 如果是2的幂次方返回`true`，否则返回`false`

**算法：**
```c
bool is_power_of_two(long value) {
    return value > 0 && (value & (value - 1)) == 0;
}
```

### 10. `log2_power_of_two`

**功能：** 计算2的幂次方的对数（以2为底）

**原型：**
```c
int log2_power_of_two(long value);
```

**参数：**
- `value`: 2的幂次方数值

**返回值：**
- 对数结果，如果value不是2的幂次方返回-1

### 11. `test_strength_reduction`

**功能：** 测试强度削减算法

**原型：**
```c
void test_strength_reduction(void);
```

**说明：**
- 运行SR算法的测试用例
- 输出测试结果和性能统计

## 使用示例

### 基本使用

```c
#include "src/core/codegen/optimizers/basic_optimizer/CN_strength_reduction.h"

int main(void)
{
    // 创建AST和优化上下文
    Stru_AstNode_t* ast = ...;
    Stru_OptimizationContext_t* opt_context = ...;
    
    // 应用强度削减优化
    if (apply_strength_reduction(ast, opt_context)) {
        printf("强度削减优化成功应用\n");
        
        // 获取统计信息
        Stru_StrengthReductionContext_t* sr_context = 
            create_strength_reduction_context(ast, opt_context);
        if (sr_context) {
            size_t matched, applied, ops_saved, cycles_saved;
            get_strength_reduction_statistics(sr_context, 
                &matched, &applied, &ops_saved, &cycles_saved);
            
            printf("匹配了 %zu 个模式，应用了 %zu 个优化\n", matched, applied);
            printf("节省了约 %zu 个操作，%zu 个时钟周期\n", ops_saved, cycles_saved);
            
            destroy_strength_reduction_context(sr_context);
        }
    }
    
    return EXIT_SUCCESS;
}
```

### 高级配置

```c
#include "src/core/codegen/optimizers/basic_optimizer/CN_strength_reduction.h"

int main(void)
{
    // 创建AST和优化上下文
    Stru_AstNode_t* ast = ...;
    Stru_OptimizationContext_t* opt_context = ...;
    
    // 创建SR上下文
    Stru_StrengthReductionContext_t* sr_context = 
        create_strength_reduction_context(ast, opt_context);
    if (!sr_context) {
        fprintf(stderr, "创建SR上下文失败\n");
        return EXIT_FAILURE;
    }
    
    // 配置SR优化
    if (!configure_strength_reduction(sr_context,
                                      Eum_SR_LEVEL_AGGRESSIVE,
                                      true,   // 启用激进优化
                                      true)) { // 保留精度
        fprintf(stderr, "配置SR优化失败\n");
        destroy_strength_reduction_context(sr_context);
        return EXIT_FAILURE;
    }
    
    // 查找可优化操作
    Stru_AstNode_t** operations = NULL;
    size_t operation_count = find_optimizable_operations(sr_context, &operations);
    
    printf("找到 %zu 个可优化操作\n", operation_count);
    
    // 应用优化
    size_t optimized_count = 0;
    for (size_t i = 0; i < operation_count; i++) {
        // 尝试所有注册的模式
        for (size_t p = 0; p < sr_context->pattern_count; p++) {
            Stru_StrengthReductionPattern_t* pattern = sr_context->patterns[p];
            if (pattern->match_function(operations[i])) {
                if (apply_pattern_to_operation(sr_context, operations[i], pattern)) {
                    optimized_count++;
                    break; // 一个操作只需应用一个模式
                }
            }
        }
    }
    
    printf("成功优化了 %zu 个操作\n", optimized_count);
    
    // 清理资源
    if (operations) {
        free(operations);
    }
    destroy_strength_reduction_context(sr_context);
    
    return EXIT_SUCCESS;
}
```

### 自定义优化模式

```c
#include "src/core/codegen/optimizers/basic_optimizer/CN_strength_reduction.h"

// 自定义匹配函数：检查是否为乘以15
bool match_multiply_by_15(Stru_AstNode_t* node) {
    if (!node || node->type != Eum_AST_BINARY_EXPR) {
        return false;
    }
    
    // 检查操作符是否为乘法
    if (strcmp(node->operator, "*") != 0) {
        return false;
    }
    
    // 检查右操作数是否为常数15
    Stru_AstNode_t* right = get_ast_child(node, 1);
    if (!right || right->type != Eum_AST_INT_LITERAL) {
        return false;
    }
    
    long value = F_ast_get_int_value(right);
    return value == 15;
}

// 自定义应用函数：将 x * 15 优化为 (x << 4) - x
Stru_AstNode_t* apply_multiply_by_15(Stru_AstNode_t* node) {
    // 获取左操作数
    Stru_AstNode_t* left = get_ast_child(node, 0);
    
    // 创建移位操作：x << 4
