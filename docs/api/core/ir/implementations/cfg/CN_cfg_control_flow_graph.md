# 控制流图数据结构 API 文档

## 概述

控制流图（Control Flow Graph，CFG）是由基本块和它们之间的控制流边组成的有向图，表示程序的整体控制流结构。控制流图是编译器优化和分析的基础数据结构。

## 数据结构

### 枚举类型

#### Eum_CfgTraversalOrder
控制流图遍历顺序枚举。

```c
enum Eum_CfgTraversalOrder {
    Eum_CFG_TRAVERSAL_PREORDER = 0,     // 前序遍历
    Eum_CFG_TRAVERSAL_POSTORDER = 1,    // 后序遍历
    Eum_CFG_TRAVERSAL_REVERSE_POSTORDER = 2, // 逆后序遍历
    Eum_CFG_TRAVERSAL_BREADTH_FIRST = 3, // 广度优先遍历
    Eum_CFG_TRAVERSAL_DEPTH_FIRST = 4,  // 深度优先遍历
    Eum_CFG_TRAVERSAL_COUNT = 5         // 枚举值数量
};
```

### 结构体类型

#### Stru_ControlFlowGraph_t
控制流图结构体。

```c
typedef struct Stru_ControlFlowGraph_t {
    char* name;                      // 控制流图名称
    size_t id;                       // 控制流图ID
    
    // 基本块管理
    Stru_BasicBlock_t** blocks;      // 基本块数组
    size_t block_count;              // 基本块数量
    size_t block_capacity;           // 基本块数组容量
    
    // 特殊基本块
    Stru_BasicBlock_t* entry_block;  // 入口基本块
    Stru_BasicBlock_t* exit_block;   // 出口基本块
    
    // 遍历顺序
    Stru_BasicBlock_t** traversal_order[Eum_CFG_TRAVERSAL_COUNT]; // 各种遍历顺序
    size_t traversal_count[Eum_CFG_TRAVERSAL_COUNT]; // 各种遍历顺序的数量
    
    // 循环信息
    Stru_BasicBlock_t** loop_headers; // 循环头数组
    size_t loop_header_count;        // 循环头数量
    size_t loop_header_capacity;     // 循环头数组容量
    
    // 数据流分析信息
    void* reaching_definitions;      // 到达定义分析结果
    void* live_variables;            // 活跃变量分析结果
    void* available_expressions;     // 可用表达式分析结果
    
    // 统计信息
    size_t total_instructions;       // 总指令数量
    size_t max_loop_depth;           // 最大循环深度
    double average_branch_factor;    // 平均分支因子
    
    // 用户数据
    void* user_data;                 // 用户自定义数据
} Stru_ControlFlowGraph_t;
```

## 函数接口

### 工厂函数

#### F_create_control_flow_graph
创建新的控制流图。

```c
Stru_ControlFlowGraph_t* F_create_control_flow_graph(const char* name, size_t id);
```

**参数：**
- `name`: 控制流图名称（可为NULL）
- `id`: 控制流图ID

**返回值：**
- 成功：返回新创建的控制流图指针
- 失败：返回NULL

#### F_destroy_control_flow_graph
销毁控制流图及其所有资源。

```c
void F_destroy_control_flow_graph(Stru_ControlFlowGraph_t* cfg);
```

**参数：**
- `cfg`: 要销毁的控制流图

### 基本块管理函数

#### F_cfg_add_basic_block
添加基本块到控制流图。

```c
bool F_cfg_add_basic_block(Stru_ControlFlowGraph_t* cfg, Stru_BasicBlock_t* block);
```

**参数：**
- `cfg`: 控制流图
- `block`: 要添加的基本块

**返回值：**
- 成功：返回true
- 失败：返回false

#### F_cfg_get_basic_block
获取控制流图中的基本块。

```c
Stru_BasicBlock_t* F_cfg_get_basic_block(const Stru_ControlFlowGraph_t* cfg, size_t index);
```

**参数：**
- `cfg`: 控制流图
- `index`: 基本块索引

**返回值：**
- 成功：返回基本块指针
- 失败：返回NULL

#### F_cfg_get_basic_block_by_id
根据ID获取基本块。

```c
Stru_BasicBlock_t* F_cfg_get_basic_block_by_id(const Stru_ControlFlowGraph_t* cfg, size_t id);
```

**参数：**
- `cfg`: 控制流图
- `id`: 基本块ID

**返回值：**
- 成功：返回基本块指针
- 失败：返回NULL

#### F_cfg_get_basic_block_count
获取控制流图中的基本块数量。

```c
size_t F_cfg_get_basic_block_count(const Stru_ControlFlowGraph_t* cfg);
```

**参数：**
- `cfg`: 控制流图

**返回值：**
- 基本块数量

### 入口和出口块管理

#### F_cfg_set_entry_block
设置入口基本块。

```c
bool F_cfg_set_entry_block(Stru_ControlFlowGraph_t* cfg, Stru_BasicBlock_t* block);
```

**参数：**
- `cfg`: 控制流图
- `block`: 入口基本块

**返回值：**
- 成功：返回true
- 失败：返回false

#### F_cfg_set_exit_block
设置出口基本块。

```c
bool F_cfg_set_exit_block(Stru_ControlFlowGraph_t* cfg, Stru_BasicBlock_t* block);
```

**参数：**
- `cfg`: 控制流图
- `block`: 出口基本块

**返回值：**
- 成功：返回true
- 失败：返回false

#### F_cfg_get_entry_block
获取入口基本块。

```c
Stru_BasicBlock_t* F_cfg_get_entry_block(const Stru_ControlFlowGraph_t* cfg);
```

**参数：**
- `cfg`: 控制流图

**返回值：**
- 入口基本块指针

#### F_cfg_get_exit_block
获取出口基本块。

```c
Stru_BasicBlock_t* F_cfg_get_exit_block(const Stru_ControlFlowGraph_t* cfg);
```

**参数：**
- `cfg`: 控制流图

**返回值：**
- 出口基本块指针

### 遍历顺序管理

#### F_cfg_compute_traversal_order
计算控制流图的遍历顺序。

```c
bool F_cfg_compute_traversal_order(Stru_ControlFlowGraph_t* cfg, Eum_CfgTraversalOrder order);
```

**参数：**
- `cfg`: 控制流图
- `order`: 遍历顺序类型

**返回值：**
- 成功：返回true
- 失败：返回false

#### F_cfg_get_traversal_order
获取指定类型的遍历顺序。

```c
Stru_BasicBlock_t** F_cfg_get_traversal_order(const Stru_ControlFlowGraph_t* cfg, Eum_CfgTraversalOrder order, size_t* count);
```

**参数：**
- `cfg`: 控制流图
- `order`: 遍历顺序类型
- `count`: 输出参数，遍历顺序中的基本块数量

**返回值：**
- 基本块数组指针

### 循环信息管理

#### F_cfg_add_loop_header
添加循环头基本块。

```c
bool F_cfg_add_loop_header(Stru_ControlFlowGraph_t* cfg, Stru_BasicBlock_t* loop_header);
```

**参数：**
- `cfg`: 控制流图
- `loop_header`: 循环头基本块

**返回值：**
- 成功：返回true
- 失败：返回false

#### F_cfg_get_loop_header
获取循环头基本块。

```c
Stru_BasicBlock_t* F_cfg_get_loop_header(const Stru_ControlFlowGraph_t* cfg, size_t index);
```

**参数：**
- `cfg`: 控制流图
- `index`: 循环头索引

**返回值：**
- 循环头基本块指针

#### F_cfg_get_loop_header_count
获取循环头数量。

```c
size_t F_cfg_get_loop_header_count(const Stru_ControlFlowGraph_t* cfg);
```

**参数：**
- `cfg`: 控制流图

**返回值：**
- 循环头数量

### 查询函数

#### F_cfg_get_name
获取控制流图名称。

```c
const char* F_cfg_get_name(const Stru_ControlFlowGraph_t* cfg);
```

**参数：**
- `cfg`: 控制流图

**返回值：**
- 控制流图名称

#### F_cfg_get_id
获取控制流图ID。

```c
size_t F_cfg_get_id(const Stru_ControlFlowGraph_t* cfg);
```

**参数：**
- `cfg`: 控制流图

**返回值：**
- 控制流图ID

#### F_cfg_is_empty
检查控制流图是否为空。

```c
bool F_cfg_is_empty(const Stru_ControlFlowGraph_t* cfg);
```

**参数：**
- `cfg`: 控制流图

**返回值：**
- 如果为空：返回true
- 否则：返回false

#### F_cfg_has_loops
检查控制流图是否有循环。

```c
bool F_cfg_has_loops(const Stru_ControlFlowGraph_t* cfg);
```

**参数：**
- `cfg`: 控制流图

**返回值：**
- 如果有循环：返回true
- 否则：返回false

### 工具函数

#### F_cfg_validate
验证控制流图的完整性。

```c
bool F_cfg_validate(const Stru_ControlFlowGraph_t* cfg, char** error_message);
```

**参数：**
- `cfg`: 要验证的控制流图
- `error_message`: 输出参数，错误信息

**返回值：**
- 如果有效：返回true
- 如果无效：返回false

#### F_cfg_to_string
将控制流图转换为字符串表示。

```c
char* F_cfg_to_string(const Stru_ControlFlowGraph_t* cfg);
```

**参数：**
- `cfg`: 控制流图

**返回值：**
- 字符串表示

#### F_cfg_to_dot
将控制流图转换为DOT格式（Graphviz）。

```c
char* F_cfg_to_dot(const Stru_ControlFlowGraph_t* cfg);
```

**参数：**
- `cfg`: 控制流图

**返回值：**
- DOT格式字符串

## 使用示例

```c
// 创建控制流图
Stru_ControlFlowGraph_t* cfg = F_create_control_flow_graph("main_cfg", 1);

// 创建基本块
Stru_BasicBlock_t* entry = F_create_basic_block("entry", Eum_BASIC_BLOCK_ENTRY, 1);
Stru_BasicBlock_t* body = F_create_basic_block("body", Eum_BASIC_BLOCK_NORMAL, 2);
Stru_BasicBlock_t* exit = F_create_basic_block("exit", Eum_BASIC_BLOCK_EXIT, 3);

// 添加基本块到控制流图
F_cfg_add_basic_block(cfg, entry);
F_cfg_add_basic_block(cfg, body);
F_cfg_add_basic_block(cfg, exit);

// 设置入口和出口块
F_cfg_set_entry_block(cfg, entry);
F_cfg_set_exit_block(cfg, exit);

// 建立控制流关系
F_basic_block_add_successor(entry, body);
F_basic_block_add_successor(body, exit);

// 计算遍历顺序
F_cfg_compute_traversal_order(cfg, Eum_CFG_TRAVERSAL_POSTORDER);

// 验证控制流图
char* error = NULL;
if (!F_cfg_validate(cfg, &error)) {
    printf("验证失败: %s\n", error);
    free(error);
}

// 转换为DOT格式
char* dot = F_cfg_to_dot(cfg);
if (dot) {
    printf("DOT格式:\n%s\n", dot);
    free(dot);
}

// 清理
F_destroy_control_flow_graph(cfg);
```

## 相关文档

- [基本块数据结构 API 文档](CN_cfg_basic_block.md)
- [控制流图构建器 API 文档](CN_cfg_builder.md)
- [控制流图模块 README](../README.md)
