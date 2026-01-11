# CN_Bytecode_Instructions API 文档

## 概述

字节码指令集是CN_Language虚拟机的核心组件，定义了所有可执行的字节码操作码和指令格式。该指令集设计为栈式虚拟机指令集，支持高效的解释执行和跨平台兼容性。

## 版本信息

### 指令集版本

```c
#define CN_BYTECODE_VERSION_MAJOR 1
#define CN_BYTECODE_VERSION_MINOR 0
#define CN_BYTECODE_VERSION_PATCH 0
#define CN_BYTECODE_VERSION_STRING "1.0.0"
```

**说明**：
- **主版本号 (MAJOR)**：不兼容的指令集修改
- **次版本号 (MINOR)**：向下兼容的新指令添加
- **修订号 (PATCH)**：向下兼容的错误修正

### 版本兼容性

- 相同主版本号的指令集保持二进制兼容
- 不同主版本号之间可能需要转换工具
- 解释器支持向后兼容（新版本解释器可以执行旧版本字节码）

## 指令集架构

### 操作码范围分配

字节码操作码按照功能分组，便于指令解码和优化：

| 范围 | 指令类型 | 说明 |
|------|----------|------|
| 0x00-0x0F | 栈操作指令 | 栈的压入、弹出、复制、交换等操作 |
| 0x10-0x2F | 算术运算指令 | 加、减、乘、除、取模、位运算等 |
| 0x30-0x4F | 逻辑运算指令 | 逻辑与、或、非、比较运算等 |
| 0x50-0x6F | 控制流指令 | 跳转、函数调用、返回、异常处理等 |
| 0x70-0x8F | 内存访问指令 | 变量加载、存储、数组操作、对象操作等 |
| 0x90-0xAF | 函数调用指令 | 函数调用、方法调用、原生调用等 |
| 0xB0-0xCF | 类型转换指令 | 类型转换和检查 |
| 0xD0-0xEF | 特殊指令 | 调试、性能分析、垃圾回收等 |
| 0xF0-0xFF | 保留/扩展指令 | 保留用于未来扩展 |

## 数据结构

### Eum_BytecodeOpcode

```c
typedef enum Eum_BytecodeOpcode {
    // 栈操作指令 (0x00-0x0F)
    Eum_BC_PUSH_NULL = 0x00,
    Eum_BC_PUSH_TRUE = 0x01,
    Eum_BC_PUSH_FALSE = 0x02,
    Eum_BC_PUSH_INT8 = 0x03,
    Eum_BC_PUSH_INT16 = 0x04,
    Eum_BC_PUSH_INT32 = 0x05,
    Eum_BC_PUSH_INT64 = 0x06,
    Eum_BC_PUSH_FLOAT32 = 0x07,
    Eum_BC_PUSH_FLOAT64 = 0x08,
    Eum_BC_PUSH_STRING = 0x09,
    Eum_BC_POP = 0x0A,
    Eum_BC_DUP = 0x0B,
    Eum_BC_DUP2 = 0x0C,
    Eum_BC_SWAP = 0x0D,
    Eum_BC_ROT = 0x0E,
    Eum_BC_NOP = 0x0F,
    
    // 算术运算指令 (0x10-0x2F)
    Eum_BC_ADD = 0x10,
    Eum_BC_SUB = 0x11,
    Eum_BC_MUL = 0x12,
    Eum_BC_DIV = 0x13,
    Eum_BC_MOD = 0x14,
    Eum_BC_NEG = 0x15,
    Eum_BC_INC = 0x16,
    Eum_BC_DEC = 0x17,
    Eum_BC_ADD_IMM8 = 0x18,
    Eum_BC_SUB_IMM8 = 0x19,
    Eum_BC_MUL_IMM8 = 0x1A,
    Eum_BC_DIV_IMM8 = 0x1B,
    Eum_BC_MOD_IMM8 = 0x1C,
    Eum_BC_POW = 0x1D,
    Eum_BC_SHL = 0x1E,
    Eum_BC_SHR = 0x1F,
    Eum_BC_BIT_AND = 0x20,
    Eum_BC_BIT_OR = 0x21,
    Eum_BC_BIT_XOR = 0x22,
    Eum_BC_BIT_NOT = 0x23,
    Eum_BC_ABS = 0x24,
    Eum_BC_SIGN = 0x25,
    
    // 逻辑运算指令 (0x30-0x4F)
    Eum_BC_NOT = 0x30,
    Eum_BC_AND = 0x31,
    Eum_BC_OR = 0x32,
    Eum_BC_EQ = 0x33,
    Eum_BC_NE = 0x34,
    Eum_BC_LT = 0x35,
    Eum_BC_LE = 0x36,
    Eum_BC_GT = 0x37,
    Eum_BC_GE = 0x38,
    Eum_BC_IS_NULL = 0x39,
    Eum_BC_IS_TRUE = 0x3A,
    Eum_BC_IS_FALSE = 0x3B,
    Eum_BC_IS_ZERO = 0x3C,
    
    // 控制流指令 (0x50-0x6F)
    Eum_BC_JUMP = 0x50,
    Eum_BC_JUMP_IF_TRUE = 0x51,
    Eum_BC_JUMP_IF_FALSE = 0x52,
    Eum_BC_JUMP_IF_NULL = 0x53,
    Eum_BC_JUMP_IF_NOT_NULL = 0x54,
    Eum_BC_JUMP_IF_ZERO = 0x55,
    Eum_BC_JUMP_IF_NOT_ZERO = 0x56,
    Eum_BC_CALL = 0x57,
    Eum_BC_RETURN = 0x58,
    Eum_BC_RETURN_VOID = 0x59,
    Eum_BC_THROW = 0x5A,
    Eum_BC_TRY = 0x5B,
    Eum_BC_CATCH = 0x5C,
    Eum_BC_FINALLY = 0x5D,
    Eum_BC_END_TRY = 0x5E,
    
    // 内存访问指令 (0x70-0x8F)
    Eum_BC_LOAD_LOCAL = 0x70,
    Eum_BC_STORE_LOCAL = 0x71,
    Eum_BC_LOAD_GLOBAL = 0x72,
    Eum_BC_STORE_GLOBAL = 0x73,
    Eum_BC_LOAD_FIELD = 0x74,
    Eum_BC_STORE_FIELD = 0x75,
    Eum_BC_LOAD_ARRAY = 0x76,
    Eum_BC_STORE_ARRAY = 0x77,
    Eum_BC_LOAD_CONST = 0x78,
    Eum_BC_NEW_OBJECT = 0x79,
    Eum_BC_NEW_ARRAY = 0x7A,
    Eum_BC_ARRAY_LENGTH = 0x7B,
    Eum_BC_GET_TYPE = 0x7C,
    Eum_BC_CHECK_TYPE = 0x7D,
    Eum_BC_CAST = 0x7E,
    
    // 函数调用指令 (0x90-0xAF)
    Eum_BC_CALL_FUNC = 0x90,
    Eum_BC_CALL_METHOD = 0x91,
    Eum_BC_CALL_NATIVE = 0x92,
    Eum_BC_CALL_CONSTRUCTOR = 0x93,
    Eum_BC_TAIL_CALL = 0x94,
    Eum_BC_APPLY = 0x95,
    Eum_BC_BIND = 0x96,
    
    // 类型转换指令 (0xB0-0xCF)
    Eum_BC_TO_INT = 0xB0,
    Eum_BC_TO_FLOAT = 0xB1,
    Eum_BC_TO_STRING = 0xB2,
    Eum_BC_TO_BOOL = 0xB3,
    Eum_BC_INT_TO_FLOAT = 0xB4,
    Eum_BC_FLOAT_TO_INT = 0xB5,
    Eum_BC_INT_TO_STRING = 0xB6,
    Eum_BC_FLOAT_TO_STRING = 0xB7,
    Eum_BC_BOOL_TO_STRING = 0xB8,
    
    // 特殊指令 (0xD0-0xEF)
    Eum_BC_DEBUG = 0xD0,
    Eum_BC_BREAKPOINT = 0xD1,
    Eum_BC_PROFILE_START = 0xD2,
    Eum_BC_PROFILE_END = 0xD3,
    Eum_GC_COLLECT = 0xD4,
    Eum_BC_GET_STACK_TRACE = 0xD5,
    Eum_BC_GET_CURRENT_FUNCTION = 0xD6,
    Eum_BC_GET_CURRENT_LINE = 0xD7,
    
    // 保留/扩展指令 (0xF0-0xFF)
    Eum_BC_RESERVED_F0 = 0xF0,
    Eum_BC_RESERVED_F1 = 0xF1,
    Eum_BC_RESERVED_F2 = 0xF2,
    Eum_BC_RESERVED_F3 = 0xF3,
    Eum_BC_RESERVED_F4 = 0xF4,
    Eum_BC_RESERVED_F5 = 0xF5,
    Eum_BC_RESERVED_F6 = 0xF6,
    Eum_BC_RESERVED_F7 = 0xF7,
    Eum_BC_RESERVED_F8 = 0xF8,
    Eum_BC_RESERVED_F9 = 0xF9,
    Eum_BC_RESERVED_FA = 0xFA,
    Eum_BC_RESERVED_FB = 0xFB,
    Eum_BC_RESERVED_FC = 0xFC,
    Eum_BC_RESERVED_FD = 0xFD,
    Eum_BC_RESERVED_FE = 0xFE,
    Eum_BC_RESERVED_FF = 0xFF
} Eum_BytecodeOpcode;
```

### Stru_BytecodeInstruction_t

```c
typedef struct Stru_BytecodeInstruction_t {
    Eum_BytecodeOpcode opcode;   ///< 操作码
    uint8_t immediate_size;      ///< 立即数大小（字节）
    union {
        int8_t imm8;             ///< 8位立即数
        int16_t imm16;           ///< 16位立即数
        int32_t imm32;           ///< 32位立即数
        int64_t imm64;           ///< 64位立即数
        float imm_f32;           ///< 32位浮点数立即数
        double imm_f64;          ///< 64位浮点数立即数
        uint32_t imm_uint32;     ///< 32位无符号立即数
        void* imm_ptr;           ///< 指针立即数
    } immediate;                 ///< 立即数
    size_t line;                 ///< 源代码行号
    size_t column;               ///< 源代码列号
    const char* comment;         ///< 调试注释（可选）
} Stru_BytecodeInstruction_t;
```

**字段说明**：
- `opcode`：指令操作码，决定指令的类型和功能
- `immediate_size`：立即数的大小（0-8字节）
- `immediate`：立即数联合体，根据指令类型存储不同的立即数
- `line`：源代码行号，用于调试和错误报告
- `column`：源代码列号，用于精确错误定位
- `comment`：调试注释，可选字段，用于生成可读的字节码输出

## 指令格式

### 1. 无操作数指令

格式：`opcode`

示例：
- `Eum_BC_NOP` (0x0F)：空操作
- `Eum_BC_POP` (0x0A)：弹出栈顶元素
- `Eum_BC_DUP` (0x0B)：复制栈顶元素

### 2. 带立即数指令

格式：`opcode immediate`

示例：
- `Eum_BC_PUSH_INT8` (0x03)：压入8位整数，立即数大小1字节
- `Eum_BC_PUSH_INT32` (0x05)：压入32位整数，立即数大小4字节
- `Eum_BC_JUMP` (0x50)：跳转指令，立即数为跳转偏移量

### 3. 带索引指令

格式：`opcode index`

示例：
- `Eum_BC_LOAD_LOCAL` (0x70)：加载局部变量，索引为局部变量索引
- `Eum_BC_LOAD_CONST` (0x78)：加载常量，索引为常量池索引
- `Eum_BC_CALL_FUNC` (0x90)：调用函数，索引为函数表索引

## 指令语义

### 栈操作指令

| 指令 | 操作码 | 语义 | 栈变化 |
|------|--------|------|--------|
| PUSH_NULL | 0x00 | 压入空值到栈顶 | stack[++sp] = null |
| PUSH_TRUE | 0x01 | 压入真值到栈顶 | stack[++sp] = true |
| PUSH_FALSE | 0x02 | 压入假值到栈顶 | stack[++sp] = false |
| PUSH_INT8 | 0x03 | 压入8位整数 | stack[++sp] = imm8 |
| POP | 0x0A | 弹出栈顶元素 | sp-- |
| DUP | 0x0B | 复制栈顶元素 | stack[sp+1] = stack[sp]; sp++ |
| SWAP | 0x0D | 交换栈顶两个元素 | swap(stack[sp], stack[sp-1]) |

### 算术运算指令

| 指令 | 操作码 | 语义 | 栈变化 |
|------|--------|------|--------|
| ADD | 0x10 | 加法：b = a + b | stack[sp-1] = stack[sp-1] + stack[sp]; sp-- |
| SUB | 0x11 | 减法：b = a - b | stack[sp-1] = stack[sp-1] - stack[sp]; sp-- |
| MUL | 0x12 | 乘法：b = a * b | stack[sp-1] = stack[sp-1] * stack[sp]; sp-- |
| DIV | 0x13 | 除法：b = a / b | stack[sp-1] = stack[sp-1] / stack[sp]; sp-- |
| NEG | 0x15 | 取负：a = -a | stack[sp] = -stack[sp] |

### 控制流指令

| 指令 | 操作码 | 语义 | 说明 |
|------|--------|------|------|
| JUMP | 0x50 | 无条件跳转 | pc += immediate |
| JUMP_IF_TRUE | 0x51 | 如果为真则跳转 | if (stack[sp--]) pc += immediate |
| JUMP_IF_FALSE | 0x52 | 如果为假则跳转 | if (!stack[sp--]) pc += immediate |
| CALL | 0x57 | 函数调用 | 保存返回地址，跳转到函数入口 |
| RETURN | 0x58 | 从函数返回 | 恢复返回地址，返回调用者 |

## 使用示例

### 指令编码示例

```c
#include "CN_bytecode_instructions.h"

// 创建指令数组
Stru_BytecodeInstruction_t instructions[10];

// 压入整数42
instructions[0].opcode = Eum_BC_PUSH_INT32;
instructions[0].immediate_size = 4;
instructions[0].immediate.imm32 = 42;
instructions[0].line = 1;
instructions[0].column = 1;

// 压入整数23
instructions[1].opcode = Eum_BC_PUSH_INT32;
instructions[1].immediate_size = 4;
instructions[1].immediate.imm32 = 23;
instructions[1].line = 1;
instructions[1].column = 5;

// 执行加法
instructions[2].opcode = Eum_BC_ADD;
instructions[2].immediate_size = 0;
instructions[2].line = 1;
instructions[2].column = 9;

// 存储结果到局部变量0
instructions[3].opcode = Eum_BC_STORE_LOCAL;
instructions[3].immediate_size = 1;
instructions[3].immediate.imm8 = 0;  // 局部变量索引
instructions[3].line = 1;
instructions[3].column = 10;
```

### 指令解码示例
