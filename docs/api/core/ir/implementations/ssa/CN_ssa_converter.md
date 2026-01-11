# SSA转换器 API 文档

## 概述

SSA转换器模块负责将抽象语法树（AST）转换为静态单赋值形式（SSA）。该模块提供了两种实现：
1. **完整版转换器**（CN_ssa_converter.c）：功能完整的AST到SSA转换器
2. **简化版转换器**（CN_ssa_converter_simple.c）：遵循单一职责原则的简化实现

SSA转换器的主要任务包括：
- 遍历AST并生成SSA指令
- 管理变量版本
- 构建控制流图
- 插入φ函数

## 模块架构

### 转换器上下文
转换器使用上下文结构体来跟踪转换过程中的状态：

```c
typedef struct {
    Stru_SsaData_t* ssa_data;              ///< 输出的SSA数据
    Stru_SsaFunction_t* current_function;  ///< 当前正在转换的函数
    Stru_SsaBasicBlock_t* current_block;   ///< 当前基本块
    int next_temp_id;                      ///< 下一个临时变量ID
    char** errors;                         ///< 错误信息数组
    size_t error_count;                    ///< 错误数量
} Stru_SsaConverterContext_t;
```

### 转换流程
1. **初始化**：创建转换器上下文和SSA数据结构
2. **AST遍历**：递归遍历AST节点，生成SSA指令
3. **变量版本管理**：为每个变量分配版本号
4. **控制流构建**：构建基本块和控制流关系
5. **φ函数插入**：在控制流汇合点插入φ函数
6. **错误处理**：收集和报告转换过程中的错误

## 函数接口

### 主要转换函数

#### F_convert_ast_to_ssa
将抽象语法树（AST）转换为静态单赋值形式（SSA）。

```c
Stru_SsaData_t* F_convert_ast_to_ssa(Stru_AstNode_t* ast);
```

**参数：**
- `ast`: AST根节点

**返回值：**
- 成功：返回转换后的SSA数据
- 失败：返回NULL

**说明：**
- 使用完整版转换器实现
- 转换后的SSA数据需要调用`F_destroy_ssa_data`释放
- 转换失败时，SSA数据中会包含错误信息

#### F_convert_ast_to_ssa_simple
将抽象语法树（AST）转换为静态单赋值形式（SSA） - 简化版本。

```c
Stru_SsaData_t* F_convert_ast_to_ssa_simple(Stru_AstNode_t* ast);
```

**参数：**
- `ast`: AST根节点

**返回值：**
- 成功：返回转换后的SSA数据
- 失败：返回NULL

**说明：**
- 使用简化版转换器实现，遵循单一职责原则
- 支持基本的AST节点转换
- 适合学习和测试用途

#### F_insert_phi_functions
在控制流图的汇合点插入φ函数。

```c
bool F_insert_phi_functions(Stru_SsaData_t* ssa_data);
```

**参数：**
- `ssa_data`: SSA数据

**返回值：**
- 成功：返回true
- 失败：返回false

**说明：**
- 基于支配边界算法插入φ函数
- 需要先构建控制流图和支配关系
- 插入的φ函数放在基本块的开头

#### F_insert_phi_functions_simple
在控制流图的汇合点插入φ函数 - 简化版本。

```c
bool F_insert_phi_functions_simple(Stru_SsaData_t* ssa_data);
```

**参数：**
- `ssa_data`: SSA数据

**返回值：**
- 成功：返回true
- 失败：返回false

**说明：**
- 简化版的φ函数插入算法
- 主要用于演示和测试
- 完整实现需要依赖控制流图分析

### 辅助函数

#### F_create_converter_context
创建转换器上下文（内部函数）。

```c
static Stru_SsaConverterContext_t* F_create_converter_context(void);
```

**返回值：**
- 成功：返回新创建的转换器上下文
- 失败：返回NULL

**说明：**
- 内部函数，不直接对外暴露
- 用于管理转换过程中的状态

#### F_destroy_converter_context
销毁转换器上下文（内部函数）。

```c
static void F_destroy_converter_context(Stru_SsaConverterContext_t* context);
```

**参数：**
- `context`: 要销毁的转换器上下文

**说明：**
- 内部函数，不直接对外暴露
- 释放上下文占用的所有资源

#### F_add_error
添加错误信息到转换器上下文（内部函数）。

```c
static bool F_add_error(Stru_SsaConverterContext_t* context, const char* error);
```

**参数：**
- `context`: 转换器上下文
- `error`: 错误信息

**返回值：**
- 成功：返回true
- 失败：返回false

**说明：**
- 内部函数，不直接对外暴露
- 错误信息会被复制到上下文中

#### F_create_temporary_variable
创建临时变量（内部函数）。

```c
static Stru_SsaVariable_t* F_create_temporary_variable(Stru_SsaConverterContext_t* context);
```

**参数：**
- `context`: 转换器上下文

**返回值：**
- 成功：返回新创建的临时变量
- 失败：返回NULL

**说明：**
- 内部函数，不直接对外暴露
- 临时变量名格式：`t0`, `t1`, `t2`, ...
- 临时变量总是创建新版本

#### F_convert_ast_node_simple
转换AST节点 - 简化版本（内部函数）。

```c
static bool F_convert_ast_node_simple(Stru_SsaConverterContext_t* context, Stru_AstNode_t* node);
```

**参数：**
- `context`: 转换器上下文
- `node`: AST节点

**返回值：**
- 成功：返回true
- 失败：返回false

**说明：**
- 内部函数，不直接对外暴露
- 支持以下AST节点类型：
  - `Eum_AST_FUNCTION_DECL`: 函数声明
  - `Eum_AST_BLOCK_STMT`: 代码块语句
  - `Eum_AST_EXPRESSION_STMT`: 表达式语句
  - `Eum_AST_RETURN_STMT`: 返回语句
  - `Eum_AST_VARIABLE_DECL`: 变量声明

## AST节点转换规则

### 函数声明（Eum_AST_FUNCTION_DECL）
1. 创建SSA函数
2. 创建入口基本块
3. 设置当前函数和当前基本块
4. 转换函数体

### 代码块语句（Eum_AST_BLOCK_STMT）
1. 依次转换所有子语句
2. 保持语句顺序

### 表达式语句（Eum_AST_EXPRESSION_STMT）
1. 转换表达式
2. 生成相应的SSA指令
3. 丢弃表达式结果（除非需要）

### 返回语句（Eum_AST_RETURN_STMT）
1. 转换返回值表达式
2. 创建返回指令
3. 将指令添加到当前基本块

### 变量声明（Eum_AST_VARIABLE_DECL）
1. 创建SSA变量（版本0）
2. 如果有初始化表达式，创建赋值指令
3. 将指令添加到当前基本块

## φ函数插入算法

### 算法概述
φ函数插入算法基于支配边界（dominance frontier）理论：
1. **计算支配关系**：确定每个基本块的支配者
2. **计算支配边界**：确定需要插入φ函数的位置
3. **插入φ函数**：在支配边界的基本块中插入φ函数
4. **重命名变量**：更新变量引用以使用正确的版本

### 简化版算法
简化版算法提供基本的φ函数插入框架：
1. 遍历所有函数
2. 标记需要实现完整算法
3. 返回成功（用于测试）

### 完整版算法（TODO）
完整版算法需要实现以下步骤：
1. 构建控制流图（CFG）
2. 计算支配树
3. 计算支配边界
4. 为每个变量确定插入点
5. 插入φ函数并重命名变量

## 错误处理

转换器使用多层错误处理机制：
1. **上下文级错误**：转换过程中收集的错误信息
2. **SSA数据级错误**：最终输出到SSA数据的错误信息
3. **返回值错误**：函数返回NULL或false表示失败

错误处理流程：
1. 转换过程中遇到错误时，调用`F_add_error`记录错误
2. 转换结束时，将上下文中的错误复制到SSA数据
3. 调用者可以通过`F_ssa_data_get_errors`获取错误信息

## 使用示例

### 示例1：基本AST到SSA转换

```c
// 假设已有AST根节点
Stru_AstNode_t* ast_root = ...;

// 使用完整版转换器
Stru_SsaData_t* ssa_data = F_convert_ast_to_ssa(ast_root);
if (!ssa_data) {
    printf("AST到SSA转换失败\n");
    return;
}

// 检查转换结果
if (F_ssa_data_has_errors(ssa_data)) {
    const char* errors = F_ssa_data_get_errors(ssa_data);
    printf("转换过程中发现错误:\n%s\n", errors);
} else {
    printf("AST到SSA转换成功\n");
    size_t func_count = F_ssa_data_get_function_count(ssa_data);
    printf("转换了 %zu 个函数\n", func_count);
}

// 清理
F_destroy_ssa_data(ssa_data);
```

### 示例2：使用简化版转换器

```c
// 使用简化版转换器
Stru_SsaData_t* ssa_data = F_convert_ast_to_ssa_simple(ast_root);
if (!ssa_data) {
    printf("简化版AST到SSA转换失败\n");
    return;
}

// 插入φ函数（简化版）
if (!F_insert_phi_functions_simple(ssa_data)) {
    printf("φ函数插入失败\n");
}

// 检查错误
if (F_ssa_data_has_errors(ssa_data)) {
    const char* errors = F_ssa_data_get_errors(ssa_data);
    printf("警告: %s\n", errors);
}

// 清理
F_destroy_ssa_data(ssa_data);
```

### 示例3：转换特定AST节点

```c
// 创建转换器上下文（仅用于演示）
Stru_SsaConverterContext_t* context = ...;
Stru_AstNode_t* node = ...;

// 转换函数声明节点
if (F_ast_get_node_type(node) == Eum_AST_FUNCTION_DECL) {
    if (!F_convert_ast_node_simple(context, node)) {
        printf("函数声明转换失败\n");
    }
}

// 转换变量声明节点
if (F_ast_get_node_type(node) == Eum_AST_VARIABLE_DECL) {
    if (!F_convert_ast_node_simple(context, node)) {
        printf("变量声明转换失败\n");
    }
}
```

## 性能考虑

1. **AST遍历**：使用递归遍历，注意栈深度限制
2. **变量版本管理**：使用高效的哈希表管理变量版本
3. **φ函数插入**：支配边界算法的时间复杂度接近O(n)
4. **内存使用**：及时释放不再需要的中间数据结构

## 限制和注意事项

### 简化版转换器的限制
1. 仅支持基本的AST节点类型
2. φ函数插入算法未完全实现
3. 不处理复杂的控制流结构（循环、条件嵌套）
4. 主要用于演示和测试

### 完整版转换器的要求
1. 需要完整的AST支持
2. 需要控制流图分析模块
3. 需要支配关系计算模块
4. 需要变量版本管理模块

### 通用注意事项
1. **AST兼容性**：转换器依赖于AST接口，AST结构变化会影响转换器
2. **错误恢复**：转换器尝试在遇到错误时继续处理，但可能产生不完整的SSA形式
3. **内存管理**：转换器创建大量临时对象，需要仔细管理内存
4. **线程安全**：转换器不是线程安全的，需要外部同步

## 版本历史

### 版本 1.0.0 (2026-01-11)
- 初始版本
- 实现完整版AST到SSA转换器框架
- 实现简化版AST到SSA转换器
- 提供φ函数插入算法框架
- 提供完整的错误处理机制
- 通过基本单元测试

### 版本 1.1.0 (计划中)
- 实现完整的φ函数插入算法
- 支持更多AST节点类型
- 优化性能和大规模AST处理
- 添加更多单元测试和集成测试

## 相关文档

- [SSA接口 API 文档](CN_ssa_interface.md)
- [SSA数据结构 API 文档](CN_ssa_data.md)
- [SSA模块 README](../README.md)
- [AST模块 API 文档](../../ast/CN_ast.md)
- [控制流图模块 API 文档](../cfg/CN_cfg_control_flow_graph.md)
