# CN_Flow_Analysis API 文档

## 概述

控制流分析和数据流分析模块提供对CN_Language程序的控制流和数据流分析功能。该模块实现了以下核心功能：

1. **控制流分析**：构建控制流图（CFG），分析程序的控制流结构
2. **数据流分析**：分析程序中的数据流，包括活跃变量、到达定义、常量传播等
3. **优化检测**：检测不可达代码、未初始化变量、未使用变量等

## 模块结构

```
src/core/semantic/flow_analysis/
├── CN_flow_analysis.h      # 接口定义
├── CN_flow_analysis.c      # 实现文件
└── README.md               # 模块文档
```

## 接口定义

### 控制流分析结果枚举

```c
typedef enum Eum_FlowAnalysisResult_t
{
    Eum_FLOW_ANALYSIS_SUCCESS = 0,          ///< 分析成功
    Eum_FLOW_ANALYSIS_UNREACHABLE_CODE,     ///< 不可达代码
    Eum_FLOW_ANALYSIS_INFINITE_LOOP,        ///< 无限循环
    Eum_FLOW_ANALYSIS_MISSING_RETURN,       ///< 缺少返回语句
    Eum_FLOW_ANALYSIS_UNINITIALIZED_VAR,    ///< 未初始化变量
    Eum_FLOW_ANALYSIS_DEAD_CODE,            ///< 死代码
    Eum_FLOW_ANALYSIS_ERROR                 ///< 分析错误
} Eum_FlowAnalysisResult_t;
```

### 数据流分析结果枚举

```c
typedef enum Eum_DataFlowResult_t
{
    Eum_DATA_FLOW_SUCCESS = 0,              ///< 分析成功
    Eum_DATA_FLOW_UNINITIALIZED_USE,        ///< 未初始化使用
    Eum_DATA_FLOW_UNUSED_VARIABLE,          ///< 未使用变量
    Eum_DATA_FLOW_CONSTANT_PROPAGATION,     ///< 常量传播
    Eum_DATA_FLOW_DEAD_STORE,               ///< 死存储
    Eum_DATA_FLOW_ERROR                     ///< 分析错误
} Eum_DataFlowResult_t;
```

### 控制流图节点类型枚举

```c
typedef enum Eum_CfgNodeType_t
{
    Eum_CFG_BASIC_BLOCK = 0,               ///< 基本块
    Eum_CFG_ENTRY,                         ///< 入口节点
    Eum_CFG_EXIT,                          ///< 出口节点
    Eum_CFG_CONDITIONAL,                   ///< 条件节点
    Eum_CFG_LOOP_HEADER,                   ///< 循环头节点
    Eum_CFG_LOOP_BODY,                     ///< 循环体节点
    Eum_CFG_LOOP_EXIT                      ///< 循环退出节点
} Eum_CfgNodeType_t;
```

### 控制流图节点结构体

```c
typedef struct Stru_CfgNode_t
{
    Eum_CfgNodeType_t type;                ///< 节点类型
    size_t id;                             ///< 节点ID
    Stru_AstNodeInterface_t* ast_node;     ///< 对应的AST节点
    struct Stru_CfgNode_t** successors;    ///< 后继节点数组
    size_t successor_count;                ///< 后继节点数量
    struct Stru_CfgNode_t** predecessors;  ///< 前驱节点数组
    size_t predecessor_count;              ///< 前驱节点数量
    void* analysis_data;                   ///< 分析数据
} Stru_CfgNode_t;
```

### 控制流图结构体

```c
typedef struct Stru_ControlFlowGraph_t
{
    Stru_CfgNode_t* entry_node;            ///< 入口节点
    Stru_CfgNode_t* exit_node;             ///< 出口节点
    Stru_CfgNode_t** nodes;                ///< 所有节点数组
    size_t node_count;                     ///< 节点数量
    size_t capacity;                       ///< 节点数组容量
} Stru_ControlFlowGraph_t;
```

### 数据流分析上下文结构体

```c
typedef struct Stru_DataFlowContext_t
{
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer;  ///< 语义分析器
    Stru_ControlFlowGraph_t* cfg;                         ///< 控制流图
    void* user_data;                                      ///< 用户自定义数据
} Stru_DataFlowContext_t;
```

### 控制流分析接口结构体

```c
typedef struct Stru_FlowAnalysisInterface_t
{
    // 控制流图构建
    Stru_ControlFlowGraph_t* (*build_control_flow_graph)(
        Stru_FlowAnalysisInterface_t* analyzer,
        Stru_AstNodeInterface_t* ast_node);
    
    // 控制流分析
    Eum_FlowAnalysisResult_t (*analyze_control_flow)(
        Stru_FlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg);
    
    // 可达性分析
    bool (*is_reachable)(
        Stru_FlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_CfgNode_t* from,
        Stru_CfgNode_t* to);
    
    // 循环检测
    bool (*detect_loops)(
        Stru_FlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_CfgNode_t*** loop_headers,
        size_t* loop_count);
    
    // 销毁控制流图
    void (*destroy_control_flow_graph)(
        Stru_FlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg);
    
    // 私有数据
    void* private_data;
} Stru_FlowAnalysisInterface_t;
```

### 数据流分析接口结构体

```c
typedef struct Stru_DataFlowAnalysisInterface_t
{
    // 数据流分析
    Eum_DataFlowResult_t (*analyze_data_flow)(
        Stru_DataFlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_DataFlowContext_t* context);
    
    // 活跃变量分析
    bool (*analyze_live_variables)(
        Stru_DataFlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_DataFlowContext_t* context);
    
    // 到达定义分析
    bool (*analyze_reaching_definitions)(
        Stru_DataFlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_DataFlowContext_t* context);
    
    // 常量传播分析
    bool (*analyze_constant_propagation)(
        Stru_DataFlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_DataFlowContext_t* context);
    
    // 未初始化变量检测
    bool (*detect_uninitialized_variables)(
        Stru_DataFlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_DataFlowContext_t* context);
    
    // 未使用变量检测
    bool (*detect_unused_variables)(
        Stru_DataFlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_DataFlowContext_t* context);
    
    // 私有数据
    void* private_data;
} Stru_DataFlowAnalysisInterface_t;
```

## API 函数

### 工厂函数

#### `F_create_flow_analysis_interface`

```c
Stru_FlowAnalysisInterface_t* F_create_flow_analysis_interface(void);
```

**功能**：创建控制流分析接口实例。

**参数**：无

**返回值**：
- 成功：指向新创建的控制流分析接口实例的指针
- 失败：NULL

**示例**：
```c
Stru_FlowAnalysisInterface_t* flow_analyzer = F_create_flow_analysis_interface();
if (flow_analyzer == NULL) {
    // 处理错误
}
```

#### `F_destroy_flow_analysis_interface`

```c
void F_destroy_flow_analysis_interface(Stru_FlowAnalysisInterface_t* interface);
```

**功能**：销毁控制流分析接口实例。

**参数**：
- `interface`：要销毁的控制流分析接口实例

**返回值**：无

**示例**：
```c
F_destroy_flow_analysis_interface(flow_analyzer);
```

#### `F_create_data_flow_analysis_interface`

```c
Stru_DataFlowAnalysisInterface_t* F_create_data_flow_analysis_interface(void);
```

**功能**：创建数据流分析接口实例。

**参数**：无

**返回值**：
- 成功：指向新创建的数据流分析接口实例的指针
- 失败：NULL

**示例**：
```c
Stru_DataFlowAnalysisInterface_t* data_flow_analyzer = F_create_data_flow_analysis_interface();
if (data_flow_analyzer == NULL) {
    // 处理错误
}
```

#### `F_destroy_data_flow_analysis_interface`

```c
void F_destroy_data_flow_analysis_interface(Stru_DataFlowAnalysisInterface_t* interface);
```

**功能**：销毁数据流分析接口实例。

**参数**：
- `interface`：要销毁的数据流分析接口实例

**返回值**：无

**示例**：
```c
F_destroy_data_flow_analysis_interface(data_flow_analyzer);
```

### 控制流分析接口函数

#### `build_control_flow_graph`

```c
Stru_ControlFlowGraph_t* (*build_control_flow_graph)(
    Stru_FlowAnalysisInterface_t* analyzer,
    Stru_AstNodeInterface_t* ast_node);
```

**功能**：从AST节点构建控制流图。

**参数**：
- `analyzer`：控制流分析接口实例
- `ast_node`：要分析的AST节点

**返回值**：
- 成功：指向新创建的控制流图的指针
- 失败：NULL

**示例**：
```c
Stru_ControlFlowGraph_t* cfg = flow_analyzer->build_control_flow_graph(flow_analyzer, function_node);
if (cfg == NULL) {
    // 处理错误
}
```

#### `analyze_control_flow`

```c
Eum_FlowAnalysisResult_t (*analyze_control_flow)(
    Stru_FlowAnalysisInterface_t* analyzer,
    Stru_ControlFlowGraph_t* cfg);
```

**功能**：分析控制流图，检测控制流问题。

**参数**：
- `analyzer`：控制流分析接口实例
- `cfg`：要分析的控制流图

**返回值**：控制流分析结果枚举值

**示例**：
```c
Eum_FlowAnalysisResult_t result = flow_analyzer->analyze_control_flow(flow_analyzer, cfg);
switch (result) {
    case Eum_FLOW_ANALYSIS_SUCCESS:
        // 分析成功
        break;
    case Eum_FLOW_ANALYSIS_UNREACHABLE_CODE:
        // 检测到不可达代码
        break;
    // ... 其他情况
}
```

#### `is_reachable`

```c
bool (*is_reachable)(
    Stru_FlowAnalysisInterface_t* analyzer,
    Stru_ControlFlowGraph_t* cfg,
    Stru_CfgNode_t* from,
    Stru_CfgNode_t* to);
```

**功能**：检查从节点`from`到节点`to`是否可达。

**参数**：
- `analyzer`：控制流分析接口实例
- `cfg`：控制流图
- `from`：起始节点
- `to`：目标节点

**返回值**：
- `true`：可达
- `false`：不可达

**示例**：
```c
bool reachable = flow_analyzer->is_reachable(flow_analyzer, cfg, start_node, end_node);
if (!reachable) {
    // 报告不可达代码
}
```

#### `detect_loops`

```c
bool (*detect_loops)(
    Stru_FlowAnalysisInterface_t* analyzer,
    Stru_ControlFlowGraph_t* cfg,
    Stru_CfgNode_t*** loop_headers,
    size_t* loop_count);
```

**功能**：检测控制流图中的循环。

**参数**：
- `analyzer`：控制流分析接口实例
- `cfg`：控制流图
- `loop_headers`：输出参数，指向循环头节点数组的指针
- `loop_count`：输出参数，循环数量

**返回值**：
- `true`：检测成功
- `false`：检测失败

**示例**：
```c
Stru_CfgNode_t** loop_headers = NULL;
size_t loop_count = 0;
bool success = flow_analyzer->detect_loops(flow_analyzer, cfg, &loop_headers, &loop_count);
if (success) {
    for (size_t i = 0; i < loop_count; i++) {
        // 处理循环头节点
    }
    free(loop_headers);
}
```

#### `destroy_control_flow_graph`

```c
void (*destroy_control_flow_graph)(
    Stru_FlowAnalysisInterface_t* analyzer,
    Stru_ControlFlowGraph_t* cfg);
```

**功能**：销毁控制流图。

**参数**：
- `analyzer`：控制流分析接口实例
- `cfg`：要销毁的控制流图

**返回值**：无

**示例**：
```c
flow_analyzer->destroy_control_flow_graph(flow_analyzer, cfg);
```

### 数据流分析接口函数

#### `analyze_data_flow`

```c
Eum_DataFlowResult_t (*analyze_data_flow)(
    Stru_DataFlowAnalysisInterface_t* analyzer,
    Stru_ControlFlowGraph_t* cfg,
    Stru_DataFlowContext_t* context);
```

**功能**：执行完整的数据流分析。

**参数**：
- `analyzer`：数据流分析接口实例
- `cfg`：控制流图
- `context`：数据流分析上下文

**返回值**：数据流分析结果枚举值

**示例**：
```c
Stru_DataFlowContext_t context;
context.semantic_analyzer = semantic_analyzer;
context.cfg = cfg;
context.user_data = NULL;

Eum_DataFlowResult_t result = data_flow_analyzer->analyze_data_flow(data_flow_analyzer, cfg, &context);
```

#### `analyze_live_variables`

```c
bool (*analyze_live_variables)(
    Stru_DataFlowAnalysisInterface_t* analyzer,
    Stru_ControlFlowGraph_t* cfg,
    Stru_DataFlowContext_t* context);
```

**功能**：执行活跃变量分析。

**参数**：
- `analyzer`：数据流分析接口实例
- `cfg`：控制流图
- `context`：数据流分析上下文

**返回值**：
- `true`：分析成功
- `false`：分析失败

**示例**：
```c
bool success = data_flow_analyzer->analyze_live_variables(data_flow_analyzer, cfg, &context);
if (success) {
    // 活跃变量分析完成
}
```

#### `analyze_reaching_definitions`

```c
bool (*analyze_reaching_definitions)(
    Stru_DataFlowAnalysisInterface_t* analyzer,
    Stru_ControlFlowGraph_t* cfg,
    Stru_DataFlowContext_t* context);
```

**功能**：执行到达定义分析。

**参数**：
- `analyzer`：数据流分析接口实例
- `cfg`：控制流图
- `context`：数据流分析上下文

**返回值**：
- `true`：分析成功
- `false`：分析失败

**示例**：
```c
bool success = data_flow_analyzer->analyze_reaching_definitions(data_flow_analyzer, cfg, &context);
```

#### `analyze_constant_propagation`

```c
bool (*analyze_constant_propagation)(
    Stru_DataFlowAnalysisInterface_t* analyzer,
    Stru_ControlFlowGraph_t* cfg,
    Stru_DataFlowContext_t* context);
```

**功能**：执行常量传播分析。

**参数**：
- `analyzer`：数据流分析接口实例
- `cfg`：控制流图
- `context`：数据流分析上下文

**返回值**：
- `true`：分析成功
- `false`：分析失败

**示例**：
```c
bool success = data_flow_analyzer->analyze_constant_propagation(data_flow_analyzer, cfg, &context);
```

#### `detect_uninitialized_variables`

```c
bool (*detect_uninitialized_variables)(
    Stru_DataFlowAnalysisInterface_t* analyzer,
    Stru_ControlFlowGraph_t* cfg,
    Stru_DataFlowContext_t* context);
```

**功能**：检测未初始化变量。

**参数**：
- `analyzer`：数据流分析接口实例
- `cfg`：控制流图
- `context`：数据流分析上下文

**返回值**：
- `true`：检测成功
- `false`：检测失败

**示例**：
```c
bool success = data_flow_analyzer->detect_uninitialized_variables(data_flow_analyzer, cfg, &context);
if (success) {
    // 未初始化变量检测完成
}
```

#### `detect_unused_variables`

```c
bool (*detect_unused_variables)(
    Stru_DataFlowAnalysisInterface_t* analyzer,
    Stru_ControlFlowGraph_t* cfg,
    Stru_DataFlowContext_t* context);
```

**功能**：检测未使用变量。

**参数**：
- `analyzer`：数据流分析接口实例
- `cfg`：控制流图
- `context`：数据流分析上下文

**返回值**：
- `true`：检测成功
- `false`：检测失败

**示例**：
```c
bool success = data_flow_analyzer->detect_unused_variables(data_flow_analyzer, cfg, &context);
```

## 使用示例

### 完整控制流和数据流分析示例
