# CN_Dead_Code_Elimination API 文档

## 概述

死代码消除（Dead Code Elimination）是CN_Language编译器的基础优化算法之一，用于移除程序中不会执行的代码，从而减少程序大小和提高执行效率。本模块实现了完整的死代码消除优化功能。

## 设计原则

1. **安全性**：确保只移除确实不会执行的代码
2. **精确性**：使用控制流分析和活跃变量分析确保分析精度
3. **保守性**：在不确定时保留代码，避免误删
4. **性能优化**：高效分析控制流图并应用优化

## 核心数据结构

### 控制流图节点类型

```c
/**
 * 控制流图节点类型枚举
 */
enum Eum_CfgNodeType {
    Eum_CFG_BASIC_BLOCK,     // 基本块
    Eum_CFG_ENTRY,           // 入口节点
    Eum_CFG_EXIT,            // 出口节点
    Eum_CFG_CONDITIONAL,     // 条件分支节点
    Eum_CFG_LOOP_HEADER,     // 循环头节点
    Eum_CFG_LOOP_LATCH       // 循环闭锁节点
};
```

### 控制流图上下文

```c
/**
 * 控制流图上下文结构
 */
typedef struct Stru_ControlFlowGraphContext_t {
    Stru_HashTable_t* node_table;      // 节点哈希表
    Stru_Array_t* basic_blocks;        // 基本块数组
    Stru_Array_t* edges;               // 边数组
    Stru_MemoryContext_t* memory_ctx;  // 内存上下文
    Stru_ErrorReporter_t* error_reporter;  // 错误报告器
} Stru_ControlFlowGraphContext_t;
```

### 活跃变量分析上下文

```c
/**
 * 活跃变量分析上下文结构
 */
typedef struct Stru_LiveVariableContext_t {
    Stru_HashTable_t* live_in;         // 入口活跃变量集合
    Stru_HashTable_t* live_out;        // 出口活跃变量集合
    Stru_HashTable_t* def_set;         // 定义集合
    Stru_HashTable_t* use_set;         // 使用集合
    Stru_MemoryContext_t* memory_ctx;  // 内存上下文
} Stru_LiveVariableContext_t;
```

## API 函数

### 主要接口函数

#### `apply_dead_code_elimination`

```c
/**
 * @brief 应用死代码消除优化
 * 
 * 对AST应用死代码消除优化，移除不会执行的代码。
 * 
 * @param ast AST根节点
 * @param context 优化上下文
 * @return bool 优化是否成功应用
 */
bool apply_dead_code_elimination(Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);
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

if (apply_dead_code_elimination(ast, &context)) {
    printf("死代码消除优化成功应用\n");
} else {
    printf("死代码消除优化失败\n");
}
```

#### `build_control_flow_graph`

```c
/**
 * @brief 构建控制流图
 * 
 * 从AST构建控制流图，用于死代码分析。
 * 
 * @param ast AST根节点
 * @return void* 控制流图句柄，调用者负责销毁
 */
void* build_control_flow_graph(Stru_AstNode_t* ast);
```

**参数说明：**
- `ast`: AST抽象语法树的根节点

**返回值：**
- 成功：控制流图句柄
- 失败：NULL

**使用示例：**
```c
void* cfg = build_control_flow_graph(ast);
if (cfg != NULL) {
    printf("控制流图构建成功\n");
    // 使用控制流图进行分析
    destroy_control_flow_graph(cfg);
}
```

#### `destroy_control_flow_graph`

```c
/**
 * @brief 销毁控制流图
 * 
 * 销毁控制流图及其所有资源。
 * 
 * @param cfg 控制流图句柄
 */
void destroy_control_flow_graph(void* cfg);
```

**参数说明：**
- `cfg`: 控制流图句柄

### 分析函数

#### `analyze_reachability`

```c
/**
 * @brief 分析代码可达性
 * 
 * 分析控制流图中哪些代码是可达的。
 * 
 * @param cfg 控制流图句柄
 * @return bool 分析是否成功
 */
bool analyze_reachability(void* cfg);
```

**算法说明：**
1. 从入口节点开始深度优先搜索
2. 标记所有可达节点
3. 识别不可达的基本块
4. 返回可达性分析结果

#### `analyze_live_variables`

```c
/**
 * @brief 分析活跃变量
 * 
 * 分析控制流图中的活跃变量。
 * 
 * @param cfg 控制流图句柄
 * @return bool 分析是否成功
 */
bool analyze_live_variables(void* cfg);
```

**算法说明：**
1. 计算每个基本块的def和use集合
2. 迭代计算live-in和live-out集合
3. 直到达到不动点
4. 识别死变量（定义后不再使用）

### 代码移除函数

#### `remove_unreachable_code`

```c
/**
 * @brief 移除不可达代码
 * 
 * 从AST中移除不可达的代码。
 * 
 * @param ast AST根节点
 * @param cfg 控制流图句柄
 * @return size_t 移除的代码数量
 */
size_t remove_unreachable_code(Stru_AstNode_t* ast, void* cfg);
```

**移除的代码类型：**
1. 不可达的基本块
2. 不可达的函数
3. 不可达的条件分支
4. 不可达的循环

#### `remove_dead_assignments`

```c
/**
 * @brief 移除死赋值
 * 
 * 从AST中移除死的赋值语句。
 * 
 * @param ast AST根节点
 * @param cfg 控制流图句柄
 * @return size_t 移除的死赋值数量
 */
size_t remove_dead_assignments(Stru_AstNode_t* ast, void* cfg);
```

**移除条件：**
1. 变量赋值后不再使用
2. 变量重新赋值前不再使用
3. 临时变量不再使用

#### `remove_unused_variables`

```c
/**
 * @brief 移除未使用变量
 * 
 * 从AST中移除未使用的变量声明。
 * 
 * @param ast AST根节点
 * @param cfg 控制流图句柄
 * @return size_t 移除的未使用变量数量
 */
size_t remove_unused_variables(Stru_AstNode_t* ast, void* cfg);
```

**移除条件：**
1. 局部变量从未使用
2. 函数参数从未使用
3. 全局变量从未使用（如果可能）

### 统计函数

#### `get_dead_code_elimination_stats`

```c
/**
 * @brief 获取死代码消除统计信息
 * 
 * 获取死代码消除优化的统计信息。
 * 
 * @param context 优化上下文
 * @param removed_count 输出参数，已移除的代码数量
 * @param error_count 输出参数，错误数量
 */
void get_dead_code_elimination_stats(Stru_OptimizationContext_t* context, 
                                     size_t* removed_count, size_t* error_count);
```

**使用示例：**
```c
size_t removed_count, error_count;
get_dead_code_elimination_stats(&context, &removed_count, &error_count);
printf("死代码消除统计: 已移除 %zu 个代码块，错误 %zu 个\n", removed_count, error_count);
```

### 测试函数

#### `test_dead_code_elimination`

```c
/**
 * @brief 测试死代码消除算法
 * 
 * 运行死代码消除算法的测试用例。
 */
void test_dead_code_elimination(void);
```

**测试用例包括：**
1. 不可达代码消除测试
2. 死赋值消除测试
3. 未使用变量消除测试
4. 复杂控制流分析测试
5. 循环和条件分支测试

## 算法实现细节

### 死代码消除流程

1. **控制流图构建**：从AST构建控制流图
2. **可达性分析**：分析哪些代码是可达的
3. **活跃变量分析**：分析哪些变量是活跃的
4. **代码移除**：移除不可达代码和死代码
5. **AST更新**：更新AST以反映移除的代码

### 控制流图构建算法

1. **基本块识别**：识别程序中的基本块
2. **边建立**：建立基本块之间的控制流边
3. **特殊节点处理**：处理入口、出口、条件分支等特殊节点
4. **循环识别**：识别循环结构

### 活跃变量分析算法

1. **数据流方程**：使用标准数据流方程
2. **迭代求解**：迭代求解直到不动点
3. **集合操作**：使用高效的集合操作
4. **位向量优化**：使用位向量表示集合以提高性能

### 代码移除策略

1. **保守移除**：只在确定安全时移除代码
2. **增量更新**：支持AST的增量更新
3. **错误恢复**：优雅处理移除错误
4. **验证机制**：验证移除后的程序语义

## 使用示例

### 示例1：基本死代码消除

```c
#include "CN_dead_code_elimination.h"

int main() {
    // 创建优化上下文
    Stru_OptimizationContext_t context;
    initialize_optimization_context(&context);
    
    // 解析包含死代码的源程序
    const char* source_code = 
        "函数 测试() {\n"
        "  变量 x = 10;\n"
        "  变量 y = 20;\n"
        "  x = x + y;  // y不再使用，死赋值\n"
        "  返回 x;\n"
        "  打印(\"这行代码不可达\");  // 不可达代码\n"
        "}\n";
    
    Stru_AstNode_t* ast = parse_source_code(source_code);
    
    // 应用死代码消除
    if (apply_dead_code_elimination(ast, &context)) {
        printf("死代码消除成功应用\n");
        
        // 获取统计信息
        size_t removed_count, error_count;
        get_dead_code_elimination_stats(&context, &removed_count, &error_count);
        printf("移除 %zu 个死代码块，错误 %zu 个\n", removed_count, error_count);
    }
    
    // 清理资源
    destroy_ast_node(ast);
    cleanup_optimization_context(&context);
    
    return 0;
}
```

### 示例2：复杂控制流分析

```c
// 分析包含复杂控制流的程序
const char* complex_source = 
    "函数 复杂测试(条件) {\n"
    "  变量 x = 0;\n"
    "  变量 y = 0;\n"
    "  \n"
    "  如果 (条件) {\n"
    "    x = 10;\n"
    "    y = 20;\n"
    "  } 否则 {\n"
    "    x = 30;\n"
    "    // y 在这个分支未定义\n"
    "  }\n"
    "  \n"
    "  // 这里 y 可能未定义，但死代码消除会保守处理\n"
    "  返回 x;\n"
    "}\n";

// 应用死代码消除后：
// 1. 在"否则"分支中，y的赋值被识别为死代码（如果y在其他地方不再使用）
// 2. 但保守分析可能保留它，因为y可能在后续使用
```

### 示例3：循环中的死代码

```c
// 循环中的死代码示例
const char* loop_source = 
    "函数 循环测试() {\n"
    "  变量 i = 0;\n"
    "  变量 和 = 0;\n"
    "  变量 临时 = 0;  // 未使用的变量\n"
    "  \n"
    "  当 (i < 10) {\n"
    "    和 = 和 + i;\n"
    "    临时 = i * 2;  // 死赋值，临时不再使用\n"
    "    i = i + 1;\n"
    "  }\n"
    "  \n"
    "  返回 和;\n"
    "}\n";

// 应用死代码消除后：
// 1. 变量"临时"的声明被移除
// 2. 循环中的"临时 = i * 2"赋值被移除
// 3. 程序大小减少，执行效率提高
```

## 性能考虑

1. **时间复杂度**：O(n²) 最坏情况，O(n) 平均情况
2. **空间复杂度**：O(n)，其中n是AST节点数量
3. **内存使用**：使用内存上下文管理临时对象
4. **缓存策略**：实现分析结果缓存以提高性能

## 错误处理

### 常见错误类型

1. **控制流图构建错误**：处理无效的AST结构
2. **分析错误**：处理分析过程中的错误
3. **代码移除错误**：处理代码移除过程中的错误
4. **内存错误**：处理内存分配失败

### 错误报告

所有错误通过优化上下文中的错误报告器进行报告，支持：
- 错误代码
- 错误消息
- 错误位置
- 错误严重程度

## 集成指南

### 与优化器框架集成

死代码消除模块通过标准优化器接口与CN_Language优化器框架集成：

```c
// 注册死代码消除优化器
Stru_OptimizerInterface_t dead_code_elimination_optimizer = {
    .name = "dead_code_elimination",
    .description = "死代码消除优化器",
    .apply = apply_dead_code_elimination,
    .test = test_dead_code_elimination,
    .get_stats = get_dead_code_elimination_stats
};

// 注册到优化器管理器
register_optimizer(&dead_code_elimination_optimizer);
```

### 构建配置

在Makefile中添加构建配置：

```makefile
# 死代码消除模块
CN_DEAD_CODE_ELIMINATION_SRC = src/core/codegen/optimizers/basic_optimizer/CN_dead_code_elimination.c
CN_DEAD_CODE_ELIMINATION_OBJ = $(BUILD_DIR)/core/codegen/optimizers/basic_optimizer/CN_dead_code_elimination.o

$(CN_DEAD_CODE_ELIMINATION_OBJ): $(CN_DEAD_CODE_ELIMINATION_SRC)
	$(CC) $(CFLAGS) -c $< -o $@
```

## 测试覆盖率

死代码消除模块的测试覆盖率目标：
- 语句覆盖率：≥85%
- 分支覆盖率：≥80%
- 函数覆盖率：≥90%

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-10 | 初始版本，实现基本死代码消除功能 |
| 1.1.0 | 2026-01-11 | 添加控制流图分析和活跃变量分析 |

## 相关文档

1. [CN_Language优化器接口文档](../CN_optimizer_interface.md)
2. [控制流图数据结构文档](../../ir/implementations/cfg/CN_cfg_control_flow_graph.md)
3. [活跃变量分析文档](../../semantic/flow_analysis/CN_flow_analysis_api.md)
4. [基础优化器README](../../../../src/core/codegen/optimizers/basic_optimizer/README.md)

---

*文档最后更新: 2026年1月11日*
*CN_Language开发团队*
