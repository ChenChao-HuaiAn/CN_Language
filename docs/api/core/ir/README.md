# IR模块API文档

## 概述

IR（中间代码表示）模块是CN_Language编译器的核心组件之一，负责将抽象语法树（AST）转换为中间代码表示。该模块支持多种IR形式，包括三地址码（TAC）、静态单赋值形式（SSA）等，为后续的优化和代码生成提供统一的中间表示。

### 主要特性

- **多种IR类型支持**：支持TAC、SSA、HIR、MIR、LIR等多种中间代码表示
- **模块化设计**：采用接口模式，实现模块间解耦
- **可扩展性**：支持新的IR类型和指令类型扩展
- **序列化支持**：支持IR的序列化和反序列化
- **优化集成**：提供优化算法接口

### 架构设计

IR模块遵循CN_Language项目的分层架构设计，属于核心层的一部分。它通过抽象接口与AST模块和代码生成模块交互，确保模块间的松耦合。

## 核心API

### 数据类型

#### 枚举类型

```c
/**
 * @brief IR类型枚举
 * 
 * 定义支持的中间代码表示类型。
 */
typedef enum {
    Eum_IR_TYPE_TAC,     ///< 三地址码（Three-Address Code）
    Eum_IR_TYPE_SSA,     ///< 静态单赋值形式（Static Single Assignment）
    Eum_IR_TYPE_HIR,     ///< 高级IR（High-Level IR）
    Eum_IR_TYPE_MIR,     ///< 中级IR（Medium-Level IR）
    Eum_IR_TYPE_LIR      ///< 低级IR（Low-Level IR）
} Eum_IrType;

/**
 * @brief IR指令类型枚举（通用）
 * 
 * 定义通用的IR指令类型。
 */
typedef enum {
    Eum_IR_INSTR_ASSIGN,         ///< 赋值指令
    Eum_IR_INSTR_BINARY_OP,      ///< 二元运算指令
    Eum_IR_INSTR_UNARY_OP,       ///< 一元运算指令
    Eum_IR_INSTR_LOAD,           ///< 加载指令
    Eum_IR_INSTR_STORE,          ///< 存储指令
    Eum_IR_INSTR_CALL,           ///< 函数调用指令
    Eum_IR_INSTR_RETURN,         ///< 返回指令
    Eum_IR_INSTR_BRANCH,         ///< 条件分支指令
    Eum_IR_INSTR_JUMP,           ///< 无条件跳转指令
    Eum_IR_INSTR_PHI,            ///< φ函数指令（SSA）
    Eum_IR_INSTR_LABEL,          ///< 标签指令
    Eum_IR_INSTR_FUNCTION,       ///< 函数定义指令
    Eum_IR_INSTR_PARAM,          ///< 参数指令
    Eum_IR_INSTR_ALLOCA,         ///< 栈分配指令
    Eum_IR_INSTR_GET_ELEMENT_PTR ///< 获取元素指针指令
} Eum_IrInstructionType;
```

#### 结构体类型

```c
/**
 * @brief IR指令结构体（通用）
 * 
 * 通用的IR指令结构体，包含所有IR类型共有的信息。
 */
typedef struct {
    Eum_IrInstructionType type;  ///< 指令类型
    char* opcode;                ///< 操作码字符串
    char** operands;             ///< 操作数数组
    size_t operand_count;        ///< 操作数数量
    size_t line;                 ///< 源代码行号
    size_t column;               ///< 源代码列号
    void* extra_data;            ///< 额外数据（类型特定）
} Stru_IrInstruction_t;

/**
 * @brief IR接口结构体
 * 
 * 定义中间代码表示的核心接口。
 */
typedef struct Stru_IrInterface_t {
    // 基本信息
    const char* (*get_name)(void);
    const char* (*get_version)(void);
    Eum_IrType (*get_type)(void);
    
    // IR构建
    void* (*create_from_ast)(Stru_AstNode_t* ast);
    void* (*create_empty)(void);
    
    // IR操作
    bool (*add_instruction)(void* ir, Stru_IrInstruction_t* instr);
    bool (*remove_instruction)(void* ir, size_t index);
    Stru_IrInstruction_t* (*get_instruction)(void* ir, size_t index);
    size_t (*get_instruction_count)(void* ir);
    
    // IR查询
    bool (*has_errors)(void* ir);
    const char* (*get_errors)(void* ir);
    void (*clear_errors)(void* ir);
    
    // IR转换
    void* (*convert_to_lower)(void* ir);
    void* (*convert_to_higher)(void* ir);
    void* (*convert_to_type)(void* ir, Eum_IrType target_type);
    
    // IR优化
    bool (*apply_optimization)(void* ir, int optimization_type);
    bool (*apply_all_optimizations)(void* ir, int optimization_level);
    
    // 序列化/反序列化
    const char* (*serialize)(void* ir);
    void* (*deserialize)(const char* data);
    
    // 验证
    bool (*validate)(void* ir);
    
    // 资源管理
    void (*destroy)(void* ir);
    
    // 内部状态
    void* internal_state;
} Stru_IrInterface_t;
```

### 工厂函数

#### F_create_ir_interface

```c
/**
 * @brief 创建IR接口实例
 * 
 * 根据指定的IR类型创建相应的IR接口实例。
 * 
 * @param type IR类型
 * @return Stru_IrInterface_t* IR接口实例，失败返回NULL
 */
Stru_IrInterface_t* F_create_ir_interface(Eum_IrType type);
```

**参数：**
- `type`：IR类型（Eum_IrType枚举值）

**返回值：**
- 成功：IR接口实例指针
- 失败：NULL

**示例：**
```c
Stru_IrInterface_t* ir_interface = F_create_ir_interface(Eum_IR_TYPE_TAC);
if (ir_interface == NULL) {
    // 处理错误
}
```

#### F_destroy_ir_interface

```c
/**
 * @brief 销毁IR接口实例
 * 
 * 销毁IR接口实例及其所有资源。
 * 
 * @param interface IR接口实例
 */
void F_destroy_ir_interface(Stru_IrInterface_t* interface);
```

**参数：**
- `interface`：要销毁的IR接口实例

**注意：** 调用此函数后，接口指针将不再有效。

#### F_get_supported_ir_types

```c
/**
 * @brief 获取支持的IR类型
 * 
 * 获取当前实现支持的所有IR类型。
 * 
 * @param types 输出参数，IR类型数组
 * @param count 输出参数，IR类型数量
 * @return bool 成功返回true，失败返回false
 */
bool F_get_supported_ir_types(Eum_IrType** types, size_t* count);
```

**参数：**
- `types`：输出参数，指向IR类型数组的指针
- `count`：输出参数，IR类型数量

**返回值：**
- 成功：true
- 失败：false

**注意：** 调用者负责释放返回的数组内存。

#### F_is_ir_type_supported

```c
/**
 * @brief 检查IR类型是否支持
 * 
 * 检查指定的IR类型是否被支持。
 * 
 * @param type IR类型
 * @return bool 支持返回true，不支持返回false
 */
bool F_is_ir_type_supported(Eum_IrType type);
```

**参数：**
- `type`：要检查的IR类型

**返回值：**
- 支持：true
- 不支持：false

### 工具函数

#### F_ir_type_to_string

```c
/**
 * @brief IR类型转字符串
 * 
 * 将IR类型转换为可读的字符串表示。
 * 
 * @param type IR类型
 * @return const char* 类型字符串表示
 */
const char* F_ir_type_to_string(Eum_IrType type);
```

**参数：**
- `type`：IR类型

**返回值：**
- 类型字符串表示

#### F_ir_instruction_type_to_string

```c
/**
 * @brief IR指令类型转字符串
 * 
 * 将IR指令类型转换为可读的字符串表示。
 * 
 * @param type IR指令类型
 * @return const char* 指令类型字符串表示
 */
const char* F_ir_instruction_type_to_string(Eum_IrInstructionType type);
```

**参数：**
- `type`：IR指令类型

**返回值：**
- 指令类型字符串表示

#### F_create_ir_instruction

```c
/**
 * @brief 创建IR指令
 * 
 * 创建新的IR指令。
 * 
 * @param type 指令类型
 * @param opcode 操作码
 * @param line 源代码行号
 * @param column 源代码列号
 * @return Stru_IrInstruction_t* 新创建的IR指令，失败返回NULL
 */
Stru_IrInstruction_t* F_create_ir_instruction(Eum_IrInstructionType type, 
                                              const char* opcode,
                                              size_t line, size_t column);
```

**参数：**
- `type`：指令类型
- `opcode`：操作码字符串
- `line`：源代码行号
- `column`：源代码列号

**返回值：**
- 成功：新创建的IR指令指针
- 失败：NULL

#### F_destroy_ir_instruction

```c
/**
 * @brief 销毁IR指令
 * 
 * 销毁IR指令及其所有资源。
 * 
 * @param instruction IR指令
 */
void F_destroy_ir_instruction(Stru_IrInstruction_t* instruction);
```

**参数：**
- `instruction`：要销毁的IR指令

#### F_ir_instruction_add_operand

```c
/**
 * @brief 添加操作数到IR指令
 * 
 * 向IR指令添加操作数。
 * 
 * @param instruction IR指令
 * @param operand 操作数字符串
 * @return bool 成功返回true，失败返回false
 */
bool F_ir_instruction_add_operand(Stru_IrInstruction_t* instruction, const char* operand);
```

**参数：**
- `instruction`：IR指令
- `operand`：操作数字符串

**返回值：**
- 成功：true
- 失败：false

#### F_ir_instruction_get_operand

```c
/**
 * @brief 获取IR指令的操作数
 * 
 * 获取IR指令的指定操作数。
 * 
 * @param instruction IR指令
 * @param index 操作数索引
 * @return const char* 操作数字符串，索引无效返回NULL
 */
const char* F_ir_instruction_get_operand(const Stru_IrInstruction_t* instruction, size_t index);
```

**参数：**
- `instruction`：IR指令
- `index`：操作数索引（0-based）

**返回值：**
- 成功：操作数字符串
- 失败：NULL（索引无效）

#### F_ir_instruction_set_extra_data

```c
/**
 * @brief 设置IR指令的额外数据
 * 
 * 设置IR指令的额外数据。
 * 
 * @param instruction IR指令
 * @param extra_data 额外数据
 */
void F_ir_instruction_set_extra_data(Stru_IrInstruction_t* instruction, void* extra_data);
```

**参数：**
- `instruction`：IR指令
- `extra_data`：额外数据指针

#### F_ir_instruction_get_extra_data

```c
/**
 * @brief 获取IR指令的额外数据
 * 
 * 获取IR指令的额外数据。
 * 
 * @param instruction IR指令
 * @return void* 额外数据
 */
void* F_ir_instruction_get_extra_data(const Stru_IrInstruction_t* instruction);
```

**参数：**
- `instruction`：IR指令

**返回值：**
- 额外数据指针

### 版本信息函数

#### F_get_ir_module_version

```c
/**
 * @brief 获取IR模块版本信息
 * 
 * 获取IR模块的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 */
void F_get_ir_module_version(int* major, int* minor, int* patch);
```

**参数：**
- `major`：输出参数，主版本号
- `minor`：输出参数，次版本号
- `patch`：输出参数，修订号

#### F_get_ir_module_version_string

```c
/**
 * @brief 获取IR模块版本字符串
 * 
 * 获取IR模块的版本字符串。
 * 
 * @return const char* 版本字符串
 */
const char* F_get_ir_module_version_string(void);
```

**返回值：**
- 版本字符串

## 接口方法

### 基本信息方法

#### get_name

```c
const char* (*get_name)(void);
```

获取IR接口的名称。

**返回值：**
- IR接口名称字符串

#### get_version

```c
const char* (*get_version)(void);
```

获取IR接口的版本。

**返回值：**
- 版本字符串

#### get_type

```c
Eum_IrType (*get_type)(void);
```

获取IR接口的类型。

**返回值：**
- IR类型

### IR构建方法

#### create_from_ast

```c
void* (*create_from_ast)(Stru_AstNode_t* ast);
```

从AST创建IR。

**参数：**
- `ast`：抽象语法树根节点

**返回值：**
- 成功：IR对象指针
- 失败：NULL

#### create_empty

```c
void* (*create_empty)(void);
```

创建空的IR对象。

**返回值：**
- 成功：空的IR对象指针
- 失败：NULL

### IR操作方法

#### add_instruction

```c
bool (*add_instruction)(void* ir, Stru_IrInstruction_t* instr);
```

向IR添加指令。

**参数：**
- `ir`：IR对象
- `instr`：要添加的指令

**返回值：**
- 成功：true
- 失败：false

#### remove_instruction

```c
bool (*remove_instruction)(void* ir, size_t index);
```

从IR中移除指定索引的指令。

**参数：**
- `ir`：IR对象
- `index`：指令索引

**返回值：**
- 成功：true
- 失败：false

#### get_instruction

```c
Stru_IrInstruction_t* (*get_instruction)(void* ir, size_t index);
```

获取指定索引的指令。

**参数：**
- `ir`：IR对象
- `index`：指令索引

**返回值：**
- 成功：指令指针
- 失败：NULL

#### get_instruction_count

```c
size_t (*get_instruction_count)(void* ir);
```

获取IR中的指令数量。

**参数：**
- `ir`：IR对象

**返回值：**
- 指令数量

### IR查询方法

#### has_errors

```c
bool (*has_errors)(void* ir);
```

检查IR是否有错误。

**参数：**
- `ir`：IR对象

**返回值：**
- 有错误：true
- 无错误：false

#### get_errors

```c
const char* (*get_errors)(void* ir);
```

获取IR的错误信息。

**参数：**
- `ir`：IR对象

**返回值：**
- 错误信息字符串

#### clear_errors

```c
void (*clear_errors)(void* ir);
```

清除IR的错误信息。

**参数：**
- `ir`：IR对象

### IR转换方法

#### convert_to_lower

```c
void* (*convert_to_lower)(void* ir);
```

将IR转换为更低级别的表示。

**参数：**
- `ir`：源IR对象

**返回值：**
- 成功：转换后的IR对象指针
- 失败：NULL

#### convert_to_higher

```c
void* (*convert_to_higher)(void* ir);
```

将IR转换为更高级别的表示。

**参数：**
- `ir`：源IR对象

**返回值：**
- 成功：转换后的IR对象指针
- 失败：NULL

#### convert_to_type

```c
void* (*convert_to_type)(void* ir, Eum_IrType target_type);
```

将IR转换为指定类型的表示。

**参数：**
- `ir`：源IR对象
- `target_type`：目标IR类型

**返回值：**
- 成功：转换后的IR对象指针
- 失败：NULL

### IR优化方法

#### apply_optimization

```c
bool (*apply_optimization)(void* ir, int optimization_type);
```

应用指定的优化算法。

**参数：**
- `ir`：IR对象
- `optimization_type`：优化类型

**返回值：**
- 成功：true
- 失败：false

#### apply_all_optimizations

```c
bool (*apply_all_optimizations)(void* ir, int optimization_level);
```

应用所有可用的优化算法。

**参数：**
- `ir`：IR对象
- `optimization_level`：优化级别

**返回值：**
- 成功：true
- 失败：false

### 序列化/反序列化方法

#### serialize

```c
const char* (*serialize)(void* ir);
```

序列化IR对象。

**参数：**
- `ir`：IR对象

**返回值：**
- 序列化后的字符串

#### deserialize

```c
void* (*deserialize)(const char* data);
```

反序列化IR对象。

**参数：**
- `data`：序列化数据

**返回值：**
- 成功：IR对象指针
- 失败：NULL

### 验证方法

#### validate

```c
bool (*validate)(void* ir);
```

验证IR对象的有效性。

**参数：**
- `ir`：IR对象

**返回值：**
- 有效：true
- 无效：false

### 资源管理方法

#### destroy

```c
void (*destroy)(void* ir);
```

销毁IR对象及其所有资源。

**参数：**
- `ir`：IR对象

**注意：** 调用此函数后，IR对象指针将不再有效。

## 使用示例

### 示例1：创建TAC IR接口并构建IR

```c
#include "src/core/ir/CN_ir_interface.h"
#include "src/core/ast/CN_ast.h"

// 创建TAC IR接口
Stru_IrInterface_t* ir_interface = F_create_ir_interface(Eum_IR_TYPE_TAC);
if (ir_interface == NULL) {
    printf("无法创建IR接口\n");
    return;
}

// 从AST创建IR
Stru_AstNode_t* ast = /* 从解析器获取AST */;
void* ir = ir_interface->create_from_ast(ast);
if (ir == NULL) {
    printf("无法从AST创建IR\n");
    F_destroy_ir_interface(ir_interface);
    return;
}

// 检查是否有错误
if (ir_interface->has_errors(ir)) {
    printf("IR构建错误: %s\n", ir_interface->get_errors(ir));
    ir_interface->clear_errors(ir);
}

// 获取指令数量
size_t count = ir_interface->get_instruction_count(ir);
printf("IR包含 %zu 条指令\n", count);

// 遍历所有指令
for (size_t i = 0; i < count; i++) {
    Stru_IrInstruction_t* instr = ir_interface->get_instruction(ir, i);
    if (instr != NULL) {
        printf("指令 %zu: %s\n", i, F_ir_instruction_type_to_string(instr->type));
    }
}

// 清理资源
ir_interface->destroy(ir);
F_destroy_ir_interface(ir_interface);
```

### 示例2：创建自定义IR指令

```c
#include "src/core/ir/CN_ir_interface.h"

// 创建IR指令
Stru_IrInstruction_t* instr = F_create_ir_instruction(
    Eum_IR_INSTR_BINARY_OP,
    "add",
    10,  // 源代码行号
    5    // 源代码列号
);

if (instr != NULL) {
    // 添加操作数
    F_ir_instruction_add_operand(instr, "x");
    F_ir_instruction_add_operand(instr, "y");
    F_ir_instruction_add_operand(instr, "z");
    
    // 获取操作数
    for (size_t i = 0; i < instr->operand_count; i++) {
        const char* operand = F_ir_instruction_get_operand(instr, i);
        printf("操作数 %zu: %s\n", i, operand);
    }
    
    // 销毁指令
    F_destroy_ir_instruction(instr);
}
```

### 示例3：IR优化

```c
#include "src/core/ir/CN_ir_interface.h"

// 创建IR接口
Stru_IrInterface_t* ir_interface = F_create_ir_interface(Eum_IR_TYPE_TAC);
if (ir_interface == NULL) {
    return;
}

// 创建或获取IR对象
void* ir = /* 获取IR对象 */;

// 应用常量折叠优化
bool success = ir_interface->apply_optimization(ir, 1);  // 1表示常量折叠
if (success) {
    printf("常量折叠优化成功\n");
}

// 应用所有优化（优化级别2）
success = ir_interface->apply_all_optimizations(ir, 2);
if (success) {
    printf("所有优化应用成功\n");
}

// 清理资源
ir_interface->destroy(ir);
F_destroy_ir_interface(ir_interface);
```

### 示例4：IR序列化

```c
#include "src/core/ir/CN_ir_interface.h"

// 创建IR接口和对象
Stru_IrInterface_t* ir_interface = F_create_ir_interface(Eum_IR_TYPE_TAC);
void* ir = /* 获取IR对象 */;

// 序列化IR
const char* serialized = ir_interface->serialize(ir);
if (serialized != NULL) {
    printf("序列化数据:\n%s\n", serialized);
    
    // 保存到文件
    FILE* file = fopen("ir_data.txt", "w");
    if (file != NULL) {
        fprintf(file, "%s", serialized);
        fclose(file);
    }
}

// 反序列化
void* deserialized_ir = ir_interface->deserialize(serialized);
if (deserialized_ir != NULL) {
    printf("反序列化成功\n");
    ir_interface->destroy(deserialized_ir);
}

// 清理资源
ir_interface->destroy(ir);
F_destroy_ir_interface(ir_interface);
```

## 错误处理

IR模块使用统一的错误处理机制。所有可能失败的函数都返回错误指示（如NULL或false），并通过以下方式报告错误：

1. **接口级错误**：通过`has_errors`、`get_errors`和`clear_errors`方法处理
2. **函数级错误**：通过返回值指示（NULL表示失败，false表示失败等）

### 错误处理示例

```c
Stru_IrInterface_t* ir_interface = F_create_ir_interface(Eum_IR_TYPE_TAC);
if (ir_interface == NULL) {
    // 工厂函数失败
    printf("无法创建IR接口\n");
    return;
}

void* ir = ir_interface->create_from_ast(ast);
if (ir == NULL) {
    // 创建失败
    printf("无法创建IR对象\n");
    F_destroy_ir_interface(ir_interface);
    return;
}

if (ir_interface->has_errors(ir)) {
    // IR对象有错误
    const char* errors = ir_interface->get_errors(ir);
    printf("IR错误: %s\n", errors);
    ir_interface->clear_errors(ir);
}
```

## 性能考虑

1. **内存管理**：IR对象可能包含大量指令，建议及时销毁不再使用的对象
2. **序列化开销**：序列化大型IR对象可能消耗较多内存和时间
3. **优化成本**：某些优化算法（如全局优化）可能具有较高的时间复杂度
4. **缓存友好性**：频繁访问的IR指令应考虑缓存以提高性能

## 扩展指南

### 添加新的IR类型

1. 在`Eum_IrType`枚举中添加新的IR类型
2. 实现对应的IR接口工厂函数
3. 在`F_create_ir_interface`函数中添加对新类型的支持
4. 更新`F_get_supported_ir_types`和`F_is_ir_type_supported`函数

### 添加新的指令类型

1. 在`Eum_IrInstructionType`枚举中添加新的指令类型
2. 更新`F_ir_instruction_type_to_string`函数
3. 在IR实现中处理新的指令类型

### 实现新的优化算法

1. 在`apply_optimization`方法中添加对新优化类型的支持
2. 实现优化算法逻辑
3. 更新`apply_all_optimizations`方法以包含新优化

## 相关模块

- **AST模块**：提供抽象语法树，作为IR的输入
- **代码生成模块**：使用IR生成目标代码
- **优化器模块**：对IR进行优化
- **语义分析模块**：为IR构建提供类型信息

## 版本历史

- **1.0.0** (2026-01-10): 初始版本，支持TAC和SSA IR类型

## 许可证

本模块遵循MIT许可证。详见项目根目录下的LICENSE文件。

## 支持与反馈

如有问题或建议，请通过以下方式联系：
- 项目GitHub仓库: [CN_Language](https://github.com/ChenChao-HuaiAn/CN_Language)
- 电子邮件: 开发团队邮箱
