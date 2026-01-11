# 基本块数据结构 API 文档

## 概述

基本块（Basic Block）是控制流图的基本构建单元，表示一段顺序执行的指令序列。基本块具有以下特点：
- 只有一个入口点（第一条指令）
- 只有一个出口点（最后一条指令）
- 内部没有分支或跳转指令

## 数据结构

### 枚举类型

#### Eum_BasicBlockType
基本块类型枚举，定义基本块的类型。

```c
enum Eum_BasicBlockType {
    Eum_BASIC_BLOCK_NORMAL = 0,      // 普通基本块
    Eum_BASIC_BLOCK_ENTRY = 1,       // 入口基本块
    Eum_BASIC_BLOCK_EXIT = 2,        // 出口基本块
    Eum_BASIC_BLOCK_LOOP_HEADER = 3, // 循环头基本块
    Eum_BASIC_BLOCK_LOOP_LATCH = 4,  // 循环闭包基本块
    Eum_BASIC_BLOCK_COUNT = 5        // 枚举值数量
};
```

### 结构体类型

#### Stru_BasicBlock_t
基本块结构体，表示一个基本块。

```c
typedef struct Stru_BasicBlock_t {
    char* name;                      // 基本块名称
    Eum_BasicBlockType type;         // 基本块类型
    size_t id;                       // 基本块ID
    
    // 指令管理
    Stru_IrInstruction_t** instructions;  // 指令数组
    size_t instruction_count;        // 指令数量
    size_t instruction_capacity;     // 指令数组容量
    
    // 控制流关系
    Stru_BasicBlock_t** predecessors;     // 前驱基本块数组
    size_t predecessor_count;        // 前驱数量
    size_t predecessor_capacity;     // 前驱数组容量
    
    Stru_BasicBlock_t** successors;       // 后继基本块数组
    size_t successor_count;         // 后继数量
    size_t successor_capacity;      // 后继数组容量
    
    // 支配关系
    Stru_BasicBlock_t** dominators;       // 支配者数组
    size_t dominator_count;         // 支配者数量
    size_t dominator_capacity;      // 支配者数组容量
    
    Stru_BasicBlock_t** dominated;        // 被支配者数组
    size_t dominated_count;         // 被支配者数量
    size_t dominated_capacity;      // 被支配者数组容量
    
    // 数据流信息
    void* reaching_definitions;     // 到达定义信息
    void* live_variables;           // 活跃变量信息
    void* available_expressions;    // 可用表达式信息
    
    // 循环信息
    bool is_in_loop;                // 是否在循环中
    size_t loop_depth;              // 循环深度
    Stru_BasicBlock_t* loop_header; // 循环头基本块
    
    // 统计信息
    size_t execution_count;         // 执行次数（用于性能分析）
    double execution_probability;   // 执行概率
    
    // 用户数据
    void* user_data;                // 用户自定义数据
} Stru_BasicBlock_t;
```

## 函数接口

### 工厂函数

#### F_create_basic_block
创建新的基本块。

```c
Stru_BasicBlock_t* F_create_basic_block(const char* name, Eum_BasicBlockType type, size_t id);
```

**参数：**
- `name`: 基本块名称（可为NULL）
- `type`: 基本块类型
- `id`: 基本块ID

**返回值：**
- 成功：返回新创建的基本块指针
- 失败：返回NULL

**说明：**
- 创建的基本块需要调用`F_destroy_basic_block`释放
- 如果名称为NULL，会生成默认名称

#### F_destroy_basic_block
销毁基本块及其所有资源。

```c
void F_destroy_basic_block(Stru_BasicBlock_t* block);
```

**参数：**
- `block`: 要销毁的基本块

**说明：**
- 释放基本块占用的所有内存
- 如果基本块为NULL，函数不执行任何操作

### 指令管理函数

#### F_basic_block_add_instruction
添加指令到基本块。

```c
bool F_basic_block_add_instruction(Stru_BasicBlock_t* block, Stru_IrInstruction_t* instruction);
```

**参数：**
- `block`: 目标基本块
- `instruction`: 要添加的指令

**返回值：**
- 成功：返回true
- 失败：返回false

**说明：**
- 指令的所有权转移给基本块
- 基本块销毁时会自动销毁所有指令

#### F_basic_block_get_instruction
获取基本块中的指令。

```c
Stru_IrInstruction_t* F_basic_block_get_instruction(const Stru_BasicBlock_t* block, size_t index);
```

**参数：**
- `block`: 基本块
- `index`: 指令索引（0-based）

**返回值：**
- 成功：返回指令指针
- 失败：返回NULL（索引越界）

#### F_basic_block_remove_instruction
从基本块中移除指令。

```c
bool F_basic_block_remove_instruction(Stru_BasicBlock_t* block, size_t index);
```

**参数：**
- `block`: 基本块
- `index`: 要移除的指令索引

**返回值：**
- 成功：返回true
- 失败：返回false（索引越界）

**说明：**
- 移除的指令不会被销毁，需要调用者负责销毁

#### F_basic_block_get_instruction_count
获取基本块中的指令数量。

```c
size_t F_basic_block_get_instruction_count(const Stru_BasicBlock_t* block);
```

**参数：**
- `block`: 基本块

**返回值：**
- 指令数量

### 控制流关系函数

#### F_basic_block_add_predecessor
添加前驱基本块。

```c
bool F_basic_block_add_predecessor(Stru_BasicBlock_t* block, Stru_BasicBlock_t* predecessor);
```

**参数：**
- `block`: 目标基本块
- `predecessor`: 前驱基本块

**返回值：**
- 成功：返回true
- 失败：返回false

**说明：**
- 同时会在前驱基本块中添加后继关系

#### F_basic_block_add_successor
添加后继基本块。

```c
bool F_basic_block_add_successor(Stru_BasicBlock_t* block, Stru_BasicBlock_t* successor);
```

**参数：**
- `block`: 目标基本块
- `successor`: 后继基本块

**返回值：**
- 成功：返回true
- 失败：返回false

**说明：**
- 同时会在后继基本块中添加前驱关系

#### F_basic_block_get_predecessor
获取前驱基本块。

```c
Stru_BasicBlock_t* F_basic_block_get_predecessor(const Stru_BasicBlock_t* block, size_t index);
```

**参数：**
- `block`: 基本块
- `index`: 前驱索引（0-based）

**返回值：**
- 成功：返回前驱基本块指针
- 失败：返回NULL（索引越界）

#### F_basic_block_get_successor
获取后继基本块。

```c
Stru_BasicBlock_t* F_basic_block_get_successor(const Stru_BasicBlock_t* block, size_t index);
```

**参数：**
- `block`: 基本块
- `index`: 后继索引（0-based）

**返回值：**
- 成功：返回后继基本块指针
- 失败：返回NULL（索引越界）

#### F_basic_block_get_predecessor_count
获取前驱数量。

```c
size_t F_basic_block_get_predecessor_count(const Stru_BasicBlock_t* block);
```

**参数：**
- `block`: 基本块

**返回值：**
- 前驱数量

#### F_basic_block_get_successor_count
获取后继数量。

```c
size_t F_basic_block_get_successor_count(const Stru_BasicBlock_t* block);
```

**参数：**
- `block`: 基本块

**返回值：**
- 后继数量

### 支配关系函数

#### F_basic_block_add_dominator
添加支配者基本块。

```c
bool F_basic_block_add_dominator(Stru_BasicBlock_t* block, Stru_BasicBlock_t* dominator);
```

**参数：**
- `block`: 目标基本块
- `dominator`: 支配者基本块

**返回值：**
- 成功：返回true
- 失败：返回false

#### F_basic_block_add_dominated
添加被支配者基本块。

```c
bool F_basic_block_add_dominated(Stru_BasicBlock_t* block, Stru_BasicBlock_t* dominated);
```

**参数：**
- `block`: 目标基本块
- `dominated`: 被支配者基本块

**返回值：**
- 成功：返回true
- 失败：返回false

#### F_basic_block_is_dominated_by
检查基本块是否被指定基本块支配。

```c
bool F_basic_block_is_dominated_by(const Stru_BasicBlock_t* block, const Stru_BasicBlock_t* dominator);
```

**参数：**
- `block`: 目标基本块
- `dominator`: 支配者基本块

**返回值：**
- 如果被支配：返回true
- 否则：返回false

#### F_basic_block_is_dominator_of
检查基本块是否支配指定基本块。

```c
bool F_basic_block_is_dominator_of(const Stru_BasicBlock_t* block, const Stru_BasicBlock_t* dominated);
```

**参数：**
- `block`: 目标基本块
- `dominated`: 被支配者基本块

**返回值：**
- 如果支配：返回true
- 否则：返回false

### 查询函数

#### F_basic_block_get_name
获取基本块名称。

```c
const char* F_basic_block_get_name(const Stru_BasicBlock_t* block);
```

**参数：**
- `block`: 基本块

**返回值：**
- 基本块名称

#### F_basic_block_get_type
获取基本块类型。

```c
Eum_BasicBlockType F_basic_block_get_type(const Stru_BasicBlock_t* block);
```

**参数：**
- `block`: 基本块

**返回值：**
- 基本块类型

#### F_basic_block_get_id
获取基本块ID。

```c
size_t F_basic_block_get_id(const Stru_BasicBlock_t* block);
```

**参数：**
- `block`: 基本块

**返回值：**
- 基本块ID

#### F_basic_block_is_empty
检查基本块是否为空（没有指令）。

```c
bool F_basic_block_is_empty(const Stru_BasicBlock_t* block);
```

**参数：**
- `block`: 基本块

**返回值：**
- 如果为空：返回true
- 否则：返回false

#### F_basic_block_has_branch
检查基本块是否有分支指令。

```c
bool F_basic_block_has_branch(const Stru_BasicBlock_t* block);
```

**参数：**
- `block`: 基本块

**返回值：**
- 如果有分支指令：返回true
- 否则：返回false

#### F_basic_block_has_loop
检查基本块是否在循环中。

```c
bool F_basic_block_has_loop(const Stru_BasicBlock_t* block);
```

**参数：**
- `block`: 基本块

**返回值：**
- 如果在循环中：返回true
- 否则：返回false

### 工具函数

#### F_basic_block_clear_instructions
清除基本块中的所有指令。

```c
void F_basic_block_clear_instructions(Stru_BasicBlock_t* block);
```

**参数：**
- `block`: 基本块

**说明：**
- 清除所有指令，但不销毁基本块本身

#### F_basic_block_clear_control_flow
清除基本块的所有控制流关系。

```c
void F_basic_block_clear_control_flow(Stru_BasicBlock_t* block);
```

**参数：**
- `block`: 基本块

**说明：**
- 清除所有前驱和后继关系

#### F_basic_block_clear_dominance
清除基本块的所有支配关系。

```c
void F_basic_block_clear_dominance(Stru_BasicBlock_t* block);
```

**参数：**
- `block`: 基本块

**说明：**
- 清除所有支配者和被支配者关系

#### F_basic_block_validate
验证基本块的完整性。

```c
bool F_basic_block_validate(const Stru_BasicBlock_t* block, char** error_message);
```

**参数：**
- `block`: 要验证的基本块
- `error_message`: 输出参数，错误信息

**返回值：**
- 如果有效：返回true
- 如果无效：返回false，并设置错误信息

**说明：**
- 验证基本块的数据结构完整性
- 验证控制流关系的一致性

#### F_basic_block_to_string
将基本块转换为字符串表示。

```c
char* F_basic_block_to_string(const Stru_BasicBlock_t* block);
```

**参数：**
- `block`: 基本块

**返回值：**
- 字符串表示，需要调用者释放

**说明：**
- 返回的字符串包含基本块名称、类型、ID和指令数量

## 使用示例

### 示例1：创建和使用基本块

```c
// 创建基本块
Stru_BasicBlock_t* block = F_create_basic_block("main_block", Eum_BASIC_BLOCK_NORMAL, 1);
if (!block) {
    // 处理错误
    return;
}

// 添加指令
Stru_IrInstruction_t* instr1 = create_ir_instruction(...);
Stru_IrInstruction_t* instr2 = create_ir_instruction(...);

if (!F_basic_block_add_instruction(block, instr1) ||
    !F_basic_block_add_instruction(block, instr2)) {
    // 处理错误
    F_destroy_basic_block(block);
    return;
}

// 获取指令
Stru_IrInstruction_t* retrieved = F_basic_block_get_instruction(block, 0);
if (retrieved != instr1) {
    // 错误处理
}

// 获取指令数量
size_t count = F_basic_block_get_instruction_count(block);
printf("基本块有 %zu 条指令\n", count);

// 清理
F_destroy_basic_block(block);
```

### 示例2：管理控制流关系

```c
// 创建基本块
Stru_BasicBlock_t* block1 = F_create_basic_block("block1", Eum_BASIC_BLOCK_NORMAL, 1);
Stru_BasicBlock_t* block2 = F_create_basic_block("block2", Eum_BASIC_BLOCK_NORMAL, 2);
Stru_BasicBlock_t* block3 = F_create_basic_block("block3", Eum_BASIC_BLOCK_NORMAL, 3);

// 建立控制流关系：block1 -> block2, block1 -> block3
F_basic_block_add_successor(block1, block2);
F_basic_block_add_successor(block1, block3);

// 验证关系
size_t successor_count = F_basic_block_get_successor_count(block1);
printf("block1 有 %zu 个后继\n", successor_count);

Stru_BasicBlock_t* successor = F_basic_block_get_successor(block1, 0);
if (successor == block2) {
    printf("第一个后继是 block2\n");
}

// 清理
F_destroy_basic_block(block1);
F_destroy_basic_block(block2);
F_destroy_basic_block(block3);
```

### 示例3：验证基本块

```c
Stru_BasicBlock_t* block = F_create_basic_block("test_block", Eum_BASIC_BLOCK_NORMAL, 1);

// 验证基本块
char* error_message = NULL;
if (!F_basic_block_validate(block, &error_message)) {
    printf("基本块验证失败: %s\n", error_message);
    if (error_message) {
        free(error_message);
    }
    F_destroy_basic_block(block);
    return;
}

// 转换为字符串
char* str = F_basic_block_to_string(block);
if (str) {
    printf("基本块信息: %s\n", str);
    free(str);
}

F_destroy_basic_block(block);
```

## 错误处理

所有函数都遵循统一的错误处理模式：
1. 返回布尔值的函数：失败时返回false
2. 返回指针的函数：失败时返回NULL
3. 需要错误信息的函数：通过输出参数返回错误信息

## 内存管理

基本块模块使用统一的内存管理接口（cn_malloc/cn_free）：
- 所有分配的内存都需要正确释放
- 基本块销毁时会自动释放所有相关资源
- 调用者负责释放通过`F_basic_block_to_string`返回的字符串

## 线程安全

基本块函数不是线程安全的。如果需要在多线程环境中使用，需要外部同步。

## 性能考虑

1. **指令管理**：使用动态数组管理指令，支持高效添加和访问
2. **控制流关系**：使用动态数组管理前驱和后继关系
3. **支配关系**：支配关系计算需要调用专门的算法函数
4. **内存使用**：使用对象池优化频繁创建和销毁的场景

## 版本历史

### 版本 1.0.0 (2026-01-11)
- 初始版本
- 实现基本块数据结构和所有管理函数
- 提供完整的API文档
- 通过所有单元测试

## 相关文档

- [控制流图数据结构 API 文档](CN_cfg_control_flow_graph.md)
- [控制流图构建器 API 文档](CN_cfg_builder.md)
- [控制流图模块 README](../README.md)
