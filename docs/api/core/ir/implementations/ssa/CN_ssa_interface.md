# SSA接口 API 文档

## 概述

静态单赋值形式（Static Single Assignment Form，SSA）是一种中间代码表示形式，其中每个变量只被赋值一次。SSA形式便于进行各种编译器优化，如常量传播、死代码消除和寄存器分配。

SSA接口定义了SSA特有的数据结构、操作码和转换函数，为编译器提供SSA形式的中间表示。

## 数据结构

### 枚举类型

#### Eum_SsaOpcode
SSA操作码枚举，定义SSA特有的操作码。

```c
typedef enum {
    Eum_SSA_OP_PHI,              ///< φ函数: x = φ(x1, x2, ..., xn)
    Eum_SSA_OP_COPY,             ///< 复制: x = y
    Eum_SSA_OP_ADD,              ///< 加法: x = y + z
    Eum_SSA_OP_SUB,              ///< 减法: x = y - z
    Eum_SSA_OP_MUL,              ///< 乘法: x = y * z
    Eum_SSA_OP_DIV,              ///< 除法: x = y / z
    Eum_SSA_OP_MOD,              ///< 取模: x = y % z
    Eum_SSA_OP_NEG,              ///< 取负: x = -y
    Eum_SSA_OP_NOT,              ///< 逻辑非: x = !y
    Eum_SSA_OP_AND,              ///< 逻辑与: x = y && z
    Eum_SSA_OP_OR,               ///< 逻辑或: x = y || z
    Eum_SSA_OP_EQ,               ///< 等于: x = y == z
    Eum_SSA_OP_NE,               ///< 不等于: x = y != z
    Eum_SSA_OP_LT,               ///< 小于: x = y < z
    Eum_SSA_OP_LE,               ///< 小于等于: x = y <= z
    Eum_SSA_OP_GT,               ///< 大于: x = y > z
    Eum_SSA_OP_GE,               ///< 大于等于: x = y >= z
    Eum_SSA_OP_BIT_AND,          ///< 位与: x = y & z
    Eum_SSA_OP_BIT_OR,           ///< 位或: x = y | z
    Eum_SSA_OP_BIT_XOR,          ///< 位异或: x = y ^ z
    Eum_SSA_OP_BIT_NOT,          ///< 位非: x = ~y
    Eum_SSA_OP_SHL,              ///< 左移: x = y << z
    Eum_SSA_OP_SHR,              ///< 右移: x = y >> z
    Eum_SSA_OP_LOAD,             ///< 加载: x = *y
    Eum_SSA_OP_STORE,            ///< 存储: *x = y
    Eum_SSA_OP_LOAD_ADDR,        ///< 加载地址: x = &y
    Eum_SSA_OP_CALL,             ///< 函数调用: x = call f(args...)
    Eum_SSA_OP_RETURN,           ///< 返回: return x
    Eum_SSA_OP_BRANCH,           ///< 条件分支: br cond, label1, label2
    Eum_SSA_OP_JUMP,             ///< 无条件跳转: jump label
    Eum_SSA_OP_LABEL,            ///< 标签: label:
    Eum_SSA_OP_FUNCTION,         ///< 函数定义: function f
    Eum_SSA_OP_END_FUNCTION,     ///< 函数结束: end_function
    Eum_SSA_OP_ALLOCA,           ///< 栈分配: x = alloca size
    Eum_SSA_OP_GET_ELEMENT_PTR   ///< 获取元素指针: x = &y[z]
} Eum_SsaOpcode;
```

### 结构体类型

#### Stru_SsaVariable_t
SSA变量结构体，包含版本信息。

```c
typedef struct {
    char* name;                 ///< 变量名（不含版本号）
    int version;                ///< 版本号（从0开始）
    char* full_name;            ///< 完整变量名（包含版本号）
    bool is_temporary;          ///< 是否为临时变量
    size_t definition_point;    ///< 定义点（指令索引）
    void* type_info;            ///< 类型信息
} Stru_SsaVariable_t;
```

#### Stru_PhiArgument_t
φ函数参数结构体，包含变量和来源基本块。

```c
typedef struct {
    Stru_SsaVariable_t* variable;   ///< 变量
    char* block_name;               ///< 来源基本块名称
    size_t block_index;             ///< 来源基本块索引
} Stru_PhiArgument_t;
```

#### Stru_SsaInstruction_t
SSA指令结构体。

```c
typedef struct {
    Eum_SsaOpcode opcode;           ///< SSA操作码
    Stru_SsaVariable_t* result;     ///< 结果变量（可为NULL）
    Stru_SsaVariable_t** operands;  ///< 操作数变量数组
    size_t operand_count;           ///< 操作数数量
    Stru_PhiArgument_t** phi_args;  ///< φ函数参数数组（仅用于φ指令）
    size_t phi_arg_count;           ///< φ函数参数数量
    char** labels;                  ///< 标签数组（用于跳转指令）
    size_t label_count;             ///< 标签数量
    size_t line;                    ///< 源代码行号
    size_t column;                  ///< 源代码列号
    void* extra_data;               ///< 额外数据
} Stru_SsaInstruction_t;
```

#### Stru_SsaBasicBlock_t
SSA基本块结构体，包含φ函数。

```c
typedef struct {
    char* name;                     ///< 基本块名称
    Stru_SsaInstruction_t** instructions;  ///< 指令数组
    size_t instruction_count;       ///< 指令数量
    Stru_SsaInstruction_t** phi_functions; ///< φ函数数组（基本块开头）
    size_t phi_count;               ///< φ函数数量
    
    // 控制流信息
    struct Stru_SsaBasicBlock_t** predecessors;  ///< 前驱基本块
    size_t predecessor_count;       ///< 前驱数量
    struct Stru_SsaBasicBlock_t** successors;    ///< 后继基本块
    size_t successor_count;         ///< 后继数量
    
    // 支配关系
    struct Stru_SsaBasicBlock_t** dominators;    ///< 支配者集合
    size_t dominator_count;         ///< 支配者数量
    struct Stru_SsaBasicBlock_t* immediate_dominator; ///< 直接支配者
    
    // 数据流信息
    void* in_set;                   ///< 到达定义入集
    void* out_set;                  ///< 到达定义出集
    void* def_set;                  ///< 定义集合
    void* use_set;                  ///< 使用集合
} Stru_SsaBasicBlock_t;
```

#### Stru_SsaFunction_t
SSA函数结构体。

```c
typedef struct {
    char* name;                     ///< 函数名
    Stru_SsaBasicBlock_t** blocks;  ///< 基本块数组
    size_t block_count;             ///< 基本块数量
    Stru_SsaBasicBlock_t* entry_block; ///< 入口基本块
    Stru_SsaBasicBlock_t* exit_block;  ///< 出口基本块
    
    // 变量信息
    Stru_SsaVariable_t** parameters; ///< 参数变量数组
    size_t param_count;             ///< 参数数量
    Stru_SsaVariable_t** local_vars; ///< 局部变量数组
    size_t local_var_count;         ///< 局部变量数量
    Stru_SsaVariable_t** temporaries; ///< 临时变量数组
    size_t temp_count;              ///< 临时变量数量
    
    // 控制流图
    void* cfg;                      ///< 控制流图
} Stru_SsaFunction_t;
```

#### Stru_SsaData_t
SSA数据结构体，包含SSA程序的所有信息。

```c
typedef struct {
    Stru_SsaFunction_t** functions; ///< 函数数组
    size_t function_count;          ///< 函数数量
    size_t capacity;                ///< 数组容量
    
    // 全局信息
    Stru_SsaVariable_t** global_vars; ///< 全局变量数组
    size_t global_var_count;       ///< 全局变量数量
    
    // 错误处理
    char* errors;                  ///< 错误信息
    bool has_errors;               ///< 是否有错误
    
    // 内部状态
    void* internal_state;          ///< 内部状态
} Stru_SsaData_t;
```

## 函数接口

### 工厂函数

#### F_create_ssa_interface
创建SSA接口实例。

```c
Stru_IrInterface_t* F_create_ssa_interface(void);
```

**返回值：**
- 成功：返回新创建的SSA接口实例
- 失败：返回NULL

**说明：**
- 创建的接口实例需要调用相应的销毁函数释放
- 接口实例实现了IR接口的所有方法

### 变量管理函数

#### F_create_ssa_variable
创建新的SSA变量。

```c
Stru_SsaVariable_t* F_create_ssa_variable(const char* name, int version, bool is_temporary);
```

**参数：**
- `name`: 变量名（不含版本号）
- `version`: 版本号（从0开始）
- `is_temporary`: 是否为临时变量

**返回值：**
- 成功：返回新创建的SSA变量
- 失败：返回NULL

**说明：**
- 创建的变量需要调用`F_destroy_ssa_variable`释放
- 变量名会被复制，调用者不需要保持字符串有效

#### F_destroy_ssa_variable
销毁SSA变量及其所有资源。

```c
void F_destroy_ssa_variable(Stru_SsaVariable_t* variable);
```

**参数：**
- `variable`: 要销毁的SSA变量

**说明：**
- 释放变量占用的所有内存
- 如果变量为NULL，函数不执行任何操作

#### F_ssa_variable_get_full_name
获取SSA变量的完整名称。

```c
const char* F_ssa_variable_get_full_name(const Stru_SsaVariable_t* variable);
```

**参数：**
- `variable`: SSA变量

**返回值：**
- 完整变量名（包含版本号）

**说明：**
- 返回的字符串由变量内部管理，调用者不应释放

### 指令管理函数

#### F_create_ssa_instruction
创建新的SSA指令。

```c
Stru_SsaInstruction_t* F_create_ssa_instruction(Eum_SsaOpcode opcode,
                                                Stru_SsaVariable_t* result,
                                                size_t line, size_t column);
```

**参数：**
- `opcode`: SSA操作码
- `result`: 结果变量（可为NULL）
- `line`: 源代码行号
- `column`: 源代码列号

**返回值：**
- 成功：返回新创建的SSA指令
- 失败：返回NULL

**说明：**
- 创建的指令需要调用`F_destroy_ssa_instruction`释放
- 结果变量的所有权不会转移给指令

#### F_destroy_ssa_instruction
销毁SSA指令及其所有资源。

```c
void F_destroy_ssa_instruction(Stru_SsaInstruction_t* instruction);
```

**参数：**
- `instruction`: 要销毁的SSA指令

**说明：**
- 释放指令占用的所有内存
- 如果指令为NULL，函数不执行任何操作

#### F_ssa_instruction_add_operand
添加操作数到SSA指令。

```c
bool F_ssa_instruction_add_operand(Stru_SsaInstruction_t* instruction, 
                                   Stru_SsaVariable_t* operand);
```

**参数：**
- `instruction`: SSA指令
- `operand`: 操作数变量

**返回值：**
- 成功：返回true
- 失败：返回false

**说明：**
- 操作数变量的所有权不会转移给指令
- 指令销毁时不会销毁操作数变量

#### F_ssa_instruction_add_phi_argument
添加φ参数到SSA指令。

```c
bool F_ssa_instruction_add_phi_argument(Stru_SsaInstruction_t* instruction,
                                        Stru_SsaVariable_t* variable,
                                        const char* block_name,
                                        size_t block_index);
```

**参数：**
- `instruction`: φ指令
- `variable`: 变量
- `block_name`: 来源基本块名称
- `block_index`: 来源基本块索引

**返回值：**
- 成功：返回true
- 失败：返回false

**说明：**
- 仅用于φ指令（opcode为Eum_SSA_OP_PHI）
- 变量和块名称的所有权不会转移给指令

#### F_ssa_instruction_add_label
添加标签到SSA指令。

```c
bool F_ssa_instruction_add_label(Stru_SsaInstruction_t* instruction, const char* label);
```

**参数：**
- `instruction`: SSA指令
- `label`: 标签

**返回值：**
- 成功：返回true
- 失败：返回false

**说明：**
- 主要用于跳转指令（BRANCH、JUMP）
- 标签字符串会被复制，调用者不需要保持字符串有效

### 操作码转换函数

#### F_ssa_opcode_to_string
将SSA操作码转换为可读的字符串表示。

```c
const char* F_ssa_opcode_to_string(Eum_SsaOpcode opcode);
```

**参数：**
- `opcode`: SSA操作码

**返回值：**
- 操作码字符串表示

**说明：**
- 返回的字符串是静态的，调用者不应释放
- 对于无效操作码，返回"UNKNOWN"

#### F_string_to_ssa_opcode
将字符串转换为SSA操作码。

```c
Eum_SsaOpcode F_string_to_ssa_opcode(const char* str);
```

**参数：**
- `str`: 操作码字符串

**返回值：**
- SSA操作码，无效返回Eum_SSA_OP_COPY

**说明：**
- 字符串比较不区分大小写
- 支持缩写形式（如"phi"、"add"等）

#### F_format_ssa_instruction
格式化SSA指令。

```c
char* F_format_ssa_instruction(const Stru_SsaInstruction_t* instruction);
```

**参数：**
- `instruction`: SSA指令

**返回值：**
- 格式化后的字符串，需要调用者释放

**说明：**
- 返回的字符串包含操作码、操作数和结果变量
- 对于φ指令，显示所有参数和来源基本块

### 数据管理函数

#### F_create_ssa_data
创建新的SSA数据结构体。

```c
Stru_SsaData_t* F_create_ssa_data(void);
```

**返回值：**
- 成功：返回新创建的SSA数据
- 失败：返回NULL

**说明：**
- 创建的SSA数据需要调用`F_destroy_ssa_data`释放
- 初始时没有函数和全局变量

#### F_destroy_ssa_data
销毁SSA数据及其所有资源。

```c
void F_destroy_ssa_data(Stru_SsaData_t* data);
```

**参数：**
- `data`: 要销毁的SSA数据

**说明：**
- 释放SSA数据占用的所有内存
- 同时销毁所有函数、基本块、指令和变量
- 如果数据为NULL，函数不执行任何操作

#### F_ssa_data_add_function
添加函数到SSA数据。

```c
bool F_ssa_data_add_function(Stru_SsaData_t* data, Stru_SsaFunction_t* function);
```

**参数：**
- `data`: SSA数据
- `function`: SSA函数

**返回值：**
- 成功：返回true
- 失败：返回false

**说明：**
- 函数的所有权转移给SSA数据
- SSA数据销毁时会自动销毁所有函数

#### F_ssa_data_get_function
从SSA数据获取指定索引的函数。

```c
Stru_SsaFunction_t* F_ssa_data_get_function(Stru_SsaData_t* data, size_t index);
```

**参数：**
- `data`: SSA数据
- `index`: 函数索引（0-based）

**返回值：**
- 成功：返回SSA函数指针
- 失败：返回NULL（索引越界）

#### F_ssa_data_get_function_count
获取SSA数据中的函数数量。

```c
size_t F_ssa_data_get_function_count(const Stru_SsaData_t* data);
```

**参数：**
- `data`: SSA数据

**返回值：**
- 函数数量

### 错误处理函数

#### F_ssa_data_add_error
向SSA数据添加错误信息。

```c
void F_ssa_data_add_error(Stru_SsaData_t* data, const char* error);
```

**参数：**
- `data`: SSA数据
- `error`: 错误信息

**说明：**
- 错误信息会被追加到现有错误信息后面
- 错误信息字符串会被复制

#### F_ssa_data_clear_errors
清除SSA数据中的所有错误信息。

```c
void F_ssa_data_clear_errors(Stru_SsaData_t* data);
```

**参数：**
- `data`: SSA数据

**说明：**
- 清除所有错误信息
- 将has_errors标志设置为false

#### F_ssa_data_has_errors
检查SSA数据是否有错误。

```c
bool F_ssa_data_has_errors(const Stru_SsaData_t* data);
```

**参数：**
- `data`: SSA数据

**返回值：**
- 有错误返回true，否则返回false

#### F_ssa_data_get_errors
获取SSA数据的错误信息。

```c
const char* F_ssa_data_get_errors(const Stru_SsaData_t* data);
```

**参数：**
- `data`: SSA数据

**返回值：**
- 错误信息，没有错误返回NULL

**说明：**
- 返回的字符串由SSA数据内部管理，调用者不应释放

### 转换函数

#### F_convert_tac_to_ssa
将三地址码（TAC）转换为静态单赋值形式（SSA）。

```c
Stru_SsaData_t* F_convert_tac_to_ssa(const void* tac_data);
```

**参数：**
- `tac_data`: TAC数据

**返回值：**
- 转换后的SSA数据，失败返回NULL

**说明：**
- 转换后的SSA数据需要调用`F_destroy_ssa_data`释放
- 转换过程包括变量版本管理和φ函数插入

#### F_convert_ast_to_ssa
将抽象语法树（AST）直接转换为静态单赋值形式（SSA）。

```c
Stru_SsaData_t* F_convert_ast_to_ssa(Stru_AstNode_t* ast);
```

**参数：**
- `ast`: AST根节点

**返回值：**
- 转换后的SSA数据，失败返回NULL

**说明：**
- 转换后的SSA数据需要调用`F_destroy_ssa_data`释放
- 直接转换避免了中间表示的开销

#### F_insert_phi_functions
在控制流图的汇合点插入φ函数。

```c
bool F_insert_phi_functions(Stru_SsaData_t* ssa_data);
```

**参数：**
- `ssa_data`: SSA数据

**返回值：**
- 成功返回true，失败返回false

**说明：**
- 基于支配边界算法插入φ函数
- 需要先构建控制流图和支配关系

## 使用示例

### 示例1：创建和使用SSA变量

```c
// 创建SSA变量
Stru_SsaVariable_t* var1 = F_create_ssa_variable("x", 0, false);
Stru_SsaVariable_t* var2 = F_create_ssa_variable("y", 1, false);
Stru_SsaVariable_t* temp = F_create_ssa_variable("t", 0, true);

if (!var1 || !var2 || !temp) {
    // 处理错误
    if (var1) F_destroy_ssa_variable(var1);
    if (var2) F_destroy_ssa_variable(var2);
    if (temp) F_destroy_ssa_variable(temp);
    return;
}

// 获取完整名称
const char* full_name = F_ssa_variable_get_full_name(var1);
printf("变量完整名称: %s\n", full_name);

// 清理
F_destroy_ssa_variable(var1);
F_destroy_ssa_variable(var2);
F_destroy_ssa_variable(temp);
```

### 示例2：创建和使用SSA指令

```c
// 创建变量
Stru_SsaVariable_t* result = F_create_ssa_variable("result", 0, false);
Stru_SsaVariable_t* op1 = F_create_ssa_variable("x", 0, false);
Stru_SsaVariable_t* op2 = F_create_ssa_variable("y", 0, false);

// 创建加法指令
Stru_SsaInstruction_t* instr = F_create_ssa_instruction(
    Eum_SSA_OP_ADD, result, 10, 5);

if (!instr) {
    // 处理错误
    F_destroy_ssa_variable(result);
    F_destroy_ssa_variable(op1);
    F_destroy_ssa_variable(op2);
    return;
}

// 添加操作数
if (!F_ssa_instruction_add_operand(instr, op1) ||
    !F_ssa_instruction_add_operand(instr, op2)) {
    // 处理错误
    F_destroy_ssa_instruction(instr);
    F_destroy_ssa_variable(result);
    F_destroy_ssa_variable(op1);
    F_destroy_ssa_variable(op2);
    return;
}

// 格式化指令
char* formatted = F_format_ssa_instruction(instr);
if (formatted) {
    printf("指令: %s\n", formatted);
    free(formatted);
}

// 清理
F_destroy_ssa_instruction(instr);
F_destroy_ssa_variable(result);
F_destroy_ssa_variable(op1);
F_destroy_ssa_variable(op2);
```

### 示例3：创建和使用SSA数据

```c
// 创建SSA数据
Stru_SsaData_t* ssa_data = F_create_ssa_data();
if (!ssa_data) {
    // 处理错误
    return;
}

// 添加错误信息
F_ssa_data_add_error(ssa_data, "测试错误信息");

// 检查错误
if (F_ssa_data_has_errors(ssa_data)) {
    const char* errors = F_ssa_data_get_errors(ssa_data);
    printf("发现错误: %s\n", errors);
}

// 清除错误
F_ssa_data_clear_errors(ssa_data);

// 清理
F_destroy_ssa_data(ssa_data);
```

### 示例4：转换AST到SSA

```c
// 假设已有AST根节点
Stru_AstNode_t* ast_root = ...;

// 转换为SSA
Stru_SsaData_t* ssa_data = F_convert_ast_to_ssa(ast_root);
if (!ssa_data) {
    printf("AST到SSA转换失败\n");
    return;
}

// 检查转换结果
if (F_ssa_data_has_errors(ssa_data)) {
    const char* errors = F_ssa_data_get_errors(ssa_data);
    printf("转换过程中发现错误: %s\n", errors);
} else {
    size_t func_count = F_ssa_data_get_function_count(ssa_data);
    printf("成功转换 %zu 个函数到SSA形式\n", func_count);
}

// 插入φ函数
if (!F_insert_phi_functions(ssa_data)) {
    printf("φ函数插入失败\n");
}

// 清理
F_destroy_ssa_data(ssa_data);
```

## 错误处理

所有函数都遵循统一的错误处理模式：
1. 返回布尔值的函数：失败时返回false
2. 返回指针的函数：失败时返回NULL
3. 需要错误信息的函数：通过SSA数据的错误机制处理

## 内存管理

SSA模块使用统一的内存管理接口（cn_malloc/cn_free）：
- 所有分配的内存都需要正确释放
- SSA数据销毁时会自动释放所有相关资源
- 调用者负责释放通过`F_format_ssa_instruction`返回的字符串

## 线程安全

SSA函数不是线程安全的。如果需要在多线程环境中使用，需要外部同步。

## 性能考虑

1. **变量版本管理**：使用高效的哈希表管理变量版本
2. **φ函数插入**：基于支配边界算法，时间复杂度接近O(n)
3. **指令格式化**：延迟计算完整变量名，减少字符串操作
4. **内存使用**：使用对象池优化频繁创建和销毁的场景

## 版本历史

### 版本 1.0.0 (2026-01-11)
- 初始版本
- 实现SSA数据结构和所有管理函数
- 提供AST到SSA的转换框架
- 提供φ函数插入算法框架
- 提供完整的API文档

## 相关文档

- [SSA数据结构 API 文档](CN_ssa_data.md)
- [SSA转换器 API 文档](CN_ssa_converter.md)
- [SSA模块 README](../README.md)
