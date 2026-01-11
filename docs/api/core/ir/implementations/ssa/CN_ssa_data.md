# SSA数据结构 API 文档

## 概述

SSA数据结构模块实现了静态单赋值形式（SSA）的所有核心数据结构及其管理函数。该模块提供了创建、销毁和操作SSA变量、指令、基本块、函数和数据的完整接口。

## 模块结构

SSA数据结构模块包含以下层次结构：
1. **SSA变量**（Stru_SsaVariable_t）：表示SSA形式的变量，包含版本信息
2. **SSA指令**（Stru_SsaInstruction_t）：表示SSA形式的指令，包含操作码、操作数和结果
3. **SSA基本块**（Stru_SsaBasicBlock_t）：表示SSA形式的基本块，包含指令和φ函数
4. **SSA函数**（Stru_SsaFunction_t）：表示SSA形式的函数，包含基本块和变量信息
5. **SSA数据**（Stru_SsaData_t）：表示完整的SSA程序，包含所有函数和全局变量

## 函数接口

### SSA变量管理函数

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
- 变量名会被复制，调用者不需要保持字符串有效
- 版本号用于生成完整变量名（格式：name_version）
- 临时变量通常用于中间计算结果

#### F_destroy_ssa_variable
销毁SSA变量及其所有资源。

```c
void F_destroy_ssa_variable(Stru_SsaVariable_t* variable);
```

**参数：**
- `variable`: 要销毁的SSA变量

**说明：**
- 释放变量名和完整变量名占用的内存
- 如果变量为NULL，函数不执行任何操作
- 注意：type_info字段由调用者管理，这里不释放

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
- 如果变量为NULL，返回NULL

### SSA指令管理函数

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
- 行号和列号用于调试和错误报告

#### F_destroy_ssa_instruction
销毁SSA指令及其所有资源。

```c
void F_destroy_ssa_instruction(Stru_SsaInstruction_t* instruction);
```

**参数：**
- `instruction`: 要销毁的SSA指令

**说明：**
- 释放指令占用的所有内存，包括操作数数组、φ参数数组和标签数组
- 如果指令为NULL，函数不执行任何操作
- 注意：result变量、操作数变量和extra_data由调用者管理，这里不释放

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
- 支持动态调整操作数数组大小

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
- 块名称字符串会被复制

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
- 主要用于跳转指令（BRANCH、JUMP）和标签指令（LABEL）
- 标签字符串会被复制，调用者不需要保持字符串有效

### SSA基本块管理函数

#### F_create_ssa_basic_block
创建新的SSA基本块。

```c
Stru_SsaBasicBlock_t* F_create_ssa_basic_block(const char* name);
```

**参数：**
- `name`: 基本块名称

**返回值：**
- 成功：返回新创建的SSA基本块
- 失败：返回NULL

**说明：**
- 基本块名称会被复制
- 初始时没有指令、φ函数和控制流关系

#### F_destroy_ssa_basic_block
销毁SSA基本块及其所有资源。

```c
void F_destroy_ssa_basic_block(Stru_SsaBasicBlock_t* block);
```

**参数：**
- `block`: 要销毁的SSA基本块

**说明：**
- 释放基本块占用的所有内存，包括名称、指令数组和φ函数数组
- 同时销毁所有指令和φ函数
- 注意：前驱、后继、支配者数组只包含指针，不释放指向的对象

#### F_ssa_basic_block_add_instruction
添加指令到SSA基本块。

```c
bool F_ssa_basic_block_add_instruction(Stru_SsaBasicBlock_t* block, 
                                       Stru_SsaInstruction_t* instruction);
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
- 支持动态调整指令数组大小

#### F_ssa_basic_block_add_phi_function
添加φ函数到SSA基本块。

```c
bool F_ssa_basic_block_add_phi_function(Stru_SsaBasicBlock_t* block,
                                        Stru_SsaInstruction_t* phi_instruction);
```

**参数：**
- `block`: 目标基本块
- `phi_instruction`: φ函数指令

**返回值：**
- 成功：返回true
- 失败：返回false

**说明：**
- 仅接受φ指令（opcode为Eum_SSA_OP_PHI）
- φ函数指令的所有权转移给基本块
- φ函数通常放在基本块的开头

### SSA函数管理函数

#### F_create_ssa_function
创建新的SSA函数。

```c
Stru_SsaFunction_t* F_create_ssa_function(const char* name);
```

**参数：**
- `name`: 函数名

**返回值：**
- 成功：返回新创建的SSA函数
- 失败：返回NULL

**说明：**
- 函数名会被复制
- 初始时没有基本块、参数、局部变量和临时变量

#### F_destroy_ssa_function
销毁SSA函数及其所有资源。

```c
void F_destroy_ssa_function(Stru_SsaFunction_t* function);
```

**参数：**
- `function`: 要销毁的SSA函数

**说明：**
- 释放函数占用的所有内存，包括名称、基本块数组和变量数组
- 同时销毁所有基本块、参数变量、局部变量和临时变量
- 注意：cfg字段由专门的模块管理，这里不释放

#### F_ssa_function_add_block
添加基本块到SSA函数。

```c
bool F_ssa_function_add_block(Stru_SsaFunction_t* function, Stru_SsaBasicBlock_t* block);
```

**参数：**
- `function`: 目标函数
- `block`: 要添加的基本块

**返回值：**
- 成功：返回true
- 失败：返回false

**说明：**
- 基本块的所有权转移给函数
- 函数销毁时会自动销毁所有基本块
- 第一个添加的基本块被自动设置为入口块
- 支持动态调整基本块数组大小

### SSA数据管理函数

#### F_create_ssa_data
创建新的SSA数据结构体。

```c
Stru_SsaData_t* F_create_ssa_data(void);
```

**返回值：**
- 成功：返回新创建的SSA数据
- 失败：返回NULL

**说明：**
- 初始时没有函数、全局变量和错误信息
- 创建的SSA数据需要调用`F_destroy_ssa_data`释放

#### F_destroy_ssa_data
销毁SSA数据及其所有资源。

```c
void F_destroy_ssa_data(Stru_SsaData_t* data);
```

**参数：**
- `data`: 要销毁的SSA数据

**说明：**
- 释放SSA数据占用的所有内存，包括函数数组、全局变量数组和错误信息
- 同时销毁所有函数和全局变量
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
- 支持动态调整函数数组大小

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
- 多条错误信息用换行符分隔

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

### 工具函数

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
- 对于无效操作码，返回"unknown"

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
- 字符串比较区分大小写
- 支持所有标准SSA操作码

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
- 根据指令类型采用不同的格式化方式：
  - φ函数：`result = φ(arg1@block1, arg2@block2, ...)`
  - 分支指令：`br cond, label1, label2`
  - 跳转指令：`jump label`
  - 标签指令：`label:`
  - 返回指令：`return value` 或 `return`
  - 调用指令：`result = call function(args...)`
  - 通用指令：`result = opcode operand1, operand2, ...`
- 返回的字符串需要调用者使用`free`或`cn_free`释放

## 使用示例

### 示例1：创建SSA程序结构

```c
// 创建SSA数据
Stru_SsaData_t* ssa_data = F_create_ssa_data();
if (!ssa_data) {
    printf("创建SSA数据失败\n");
    return;
}

// 创建函数
Stru_SsaFunction_t* func = F_create_ssa_function("main");
if (!func) {
    printf("创建函数失败\n");
    F_destroy_ssa_data(ssa_data);
    return;
}

// 创建基本块
Stru_SsaBasicBlock_t* entry_block = F_create_ssa_basic_block("entry");
Stru_SsaBasicBlock_t* exit_block = F_create_ssa_basic_block("exit");
if (!entry_block || !exit_block) {
    printf("创建基本块失败\n");
    if (entry_block) F_destroy_ssa_basic_block(entry_block);
    if (exit_block) F_destroy_ssa_basic_block(exit_block);
    F_destroy_ssa_function(func);
    F_destroy_ssa_data(ssa_data);
    return;
}

// 添加到函数
if (!F_ssa_function_add_block(func, entry_block) ||
    !F_ssa_function_add_block(func, exit_block)) {
    printf("添加基本块失败\n");
    F_destroy_ssa_basic_block(entry_block);
    F_destroy_ssa_basic_block(exit_block);
    F_destroy_ssa_function(func);
    F_destroy_ssa_data(ssa_data);
    return;
}

// 设置出口块
func->exit_block = exit_block;

// 添加到SSA数据
if (!F_ssa_data_add_function(ssa_data, func)) {
    printf("添加函数失败\n");
    F_destroy_ssa_function(func);
    F_destroy_ssa_data(ssa_data);
    return;
}

printf("成功创建SSA程序结构\n");

// 清理
F_destroy_ssa_data(ssa_data);
```

### 示例2：创建SSA指令序列

```c
// 创建变量
Stru_SsaVariable_t* x = F_create_ssa_variable("x", 0, false);
Stru_SsaVariable_t* y = F_create_ssa_variable("y", 0, false);
Stru_SsaVariable_t* result = F_create_ssa_variable("result", 0, false);

// 创建基本块
Stru_SsaBasicBlock_t* block = F_create_ssa_basic_block("block1");

// 创建加法指令
Stru_SsaInstruction_t* add_instr = F_create_ssa_instruction(
    Eum_SSA_OP_ADD, result, 10, 5);
F_ssa_instruction_add_operand(add_instr, x);
F_ssa_instruction_add_operand(add_instr, y);

// 创建返回指令
Stru_SsaInstruction_t* ret_instr = F_create_ssa_instruction(
    Eum_SSA_OP_RETURN, NULL, 11, 1);
F_ssa_instruction_add_operand(ret_instr, result);

// 添加到基本块
F_ssa_basic_block_add_instruction(block, add_instr);
F_ssa_basic_block_add_instruction(block, ret_instr);

// 格式化并显示指令
char* add_str = F_format_ssa_instruction(add_instr);
char* ret_str = F_format_ssa_instruction(ret_instr);
if (add_str && ret_str) {
    printf("指令1: %s\n", add_str);
    printf("指令2: %s\n", ret_str);
}

// 清理
if (add_str) free(add_str);
if (ret_str) free(ret_str);
F_destroy_ssa_basic_block(block);
F_destroy_ssa_variable(x);
F_destroy_ssa_variable(y);
F_destroy_ssa_variable(result);
```

### 示例3：创建φ函数

```c
// 创建变量
Stru_SsaVariable_t* x1 = F_create_ssa_variable("x", 1, false);
Stru_SsaVariable_t* x2 = F_create_ssa_variable("x", 2, false);
Stru_SsaVariable_t* x3 = F_create_ssa_variable("x", 3, false);

// 创建φ指令
Stru_SsaInstruction_t* phi_instr = F_create_ssa_instruction(
    Eum_SSA_OP_PHI, x3, 20, 10);

// 添加φ参数
F_ssa_instruction_add_phi_argument(phi_instr, x1, "block1", 0);
F_ssa_instruction_add_phi_argument(phi_instr, x2, "block2", 1);

// 格式化φ函数
char* phi_str = F_format_ssa_instruction(phi_instr);
if (phi_str) {
    printf("φ函数: %s\n", phi_str);
    free(phi_str);
}

// 清理
F_destroy_ssa_instruction(phi_instr);
F_destroy_ssa_variable(x1);
F_destroy_ssa_variable(x2);
F_destroy_ssa_variable(x3);
```

### 示例4：错误处理

```c
// 创建SSA数据
Stru_SsaData_t* data = F_create_ssa_data();

// 添加多个错误
F_ssa_data_add_error(data, "语法错误: 第10行缺少分号");
F_ssa_data_add_error(data, "类型错误: 第15行类型不匹配");
F_ssa_data_add_error(data, "语义错误: 第20行未定义的变量");

// 检查和处理错误
if (F_ssa_data_has_errors(data)) {
    const char* errors = F_ssa_data_get_errors(data);
    printf("编译错误:\n%s\n", errors);
    
    // 清除错误并重试
    F_ssa_data_clear_errors(data);
    printf("错误已清除，has_errors: %s\n", 
           F_ssa_data_has_errors(data) ? "true" : "false");
}

// 清理
F_destroy_ssa_data(data);
```

## 内存管理

SSA数据结构模块使用统一的内存管理接口（cn_malloc/cn_free/cn_realloc/cn_strdup）：
- 所有分配的内存都需要正确释放
- 数据结构销毁时会自动释放所有相关资源
- 调用者负责释放通过`F_format_ssa_instruction`返回的字符串
- 注意所有权转移规则：
  - 添加到基本块的指令：所有权转移给基本块
  - 添加到函数的块：所有权转移给函数
  - 添加到SSA数据的函数：所有权转移给SSA数据

## 线程安全

SSA数据结构函数不是线程安全的。如果需要在多线程环境中使用，需要外部同步。

## 性能考虑

1. **动态数组管理**：使用动态数组和指数增长策略，减少重新分配次数
2. **字符串管理**：延迟计算完整变量名，减少不必要的字符串操作
3. **内存池**：考虑使用对象池优化频繁创建和销毁的场景
4. **缓存友好**：相关数据尽量连续存储，提高缓存命中率

## 错误处理

所有函数都遵循统一的错误处理模式：
1. 返回布尔值的函数：失败时返回false
2. 返回指针的函数：失败时返回NULL
3. 需要错误信息的函数：通过SSA数据的错误机制处理
4. 内存分配失败：返回NULL或false，不崩溃

## 版本历史

### 版本 1.0.0 (2026-01-11)
- 初始版本
- 实现所有SSA数据结构及其管理函数
- 提供完整的错误处理机制
- 提供格式化函数和工具函数
- 通过所有单元测试

## 相关文档

- [SSA接口 API 文档](CN_ssa_interface.md)
- [SSA转换器 API 文档](CN_ssa_converter.md)
- [SSA模块 README](../README.md)
