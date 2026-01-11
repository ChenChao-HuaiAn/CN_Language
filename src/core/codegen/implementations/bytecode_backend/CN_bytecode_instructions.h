/******************************************************************************
 * 文件名: CN_bytecode_instructions.h
 * 功能: CN_Language 字节码指令集定义
 * 
 * 定义CN_Language虚拟机的字节码指令集，包括操作码、指令格式和语义。
 * 字节码设计为栈式虚拟机指令集，支持高效解释执行。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_BYTECODE_INSTRUCTIONS_H
#define CN_BYTECODE_INSTRUCTIONS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 字节码指令集版本
// ============================================================================

/**
 * @brief 字节码指令集版本
 * 
 * 定义字节码指令集的版本号，用于版本兼容性检查。
 */
#define CN_BYTECODE_VERSION_MAJOR 1
#define CN_BYTECODE_VERSION_MINOR 0
#define CN_BYTECODE_VERSION_PATCH 0
#define CN_BYTECODE_VERSION_STRING "1.0.0"

// ============================================================================
// 字节码操作码枚举
// ============================================================================

/**
 * @brief 字节码操作码枚举
 * 
 * 定义所有字节码指令的操作码。
 * 操作码范围分配：
 *   - 0x00-0x0F: 栈操作指令
 *   - 0x10-0x2F: 算术运算指令
 *   - 0x30-0x4F: 逻辑运算指令
 *   - 0x50-0x6F: 控制流指令
 *   - 0x70-0x8F: 内存访问指令
 *   - 0x90-0xAF: 函数调用指令
 *   - 0xB0-0xCF: 类型转换指令
 *   - 0xD0-0xEF: 特殊指令
 *   - 0xF0-0xFF: 保留/扩展指令
 */
typedef enum Eum_BytecodeOpcode {
    // ============================================
    // 栈操作指令 (0x00-0x0F)
    // ============================================
    Eum_BC_PUSH_NULL = 0x00,     ///< 压入空值到栈
    Eum_BC_PUSH_TRUE = 0x01,     ///< 压入真值到栈
    Eum_BC_PUSH_FALSE = 0x02,    ///< 压入假值到栈
    Eum_BC_PUSH_INT8 = 0x03,     ///< 压入8位整数到栈（带1字节立即数）
    Eum_BC_PUSH_INT16 = 0x04,    ///< 压入16位整数到栈（带2字节立即数）
    Eum_BC_PUSH_INT32 = 0x05,    ///< 压入32位整数到栈（带4字节立即数）
    Eum_BC_PUSH_INT64 = 0x06,    ///< 压入64位整数到栈（带8字节立即数）
    Eum_BC_PUSH_FLOAT32 = 0x07,  ///< 压入32位浮点数到栈（带4字节立即数）
    Eum_BC_PUSH_FLOAT64 = 0x08,  ///< 压入64位浮点数到栈（带8字节立即数）
    Eum_BC_PUSH_STRING = 0x09,   ///< 压入字符串到栈（带字符串索引）
    Eum_BC_POP = 0x0A,           ///< 弹出栈顶元素
    Eum_BC_DUP = 0x0B,           ///< 复制栈顶元素
    Eum_BC_DUP2 = 0x0C,          ///< 复制栈顶两个元素
    Eum_BC_SWAP = 0x0D,          ///< 交换栈顶两个元素
    Eum_BC_ROT = 0x0E,           ///< 旋转栈顶三个元素
    Eum_BC_NOP = 0x0F,           ///< 空操作
    
    // ============================================
    // 算术运算指令 (0x10-0x2F)
    // ============================================
    Eum_BC_ADD = 0x10,           ///< 加法: b = a + b
    Eum_BC_SUB = 0x11,           ///< 减法: b = a - b
    Eum_BC_MUL = 0x12,           ///< 乘法: b = a * b
    Eum_BC_DIV = 0x13,           ///< 除法: b = a / b
    Eum_BC_MOD = 0x14,           ///< 取模: b = a % b
    Eum_BC_NEG = 0x15,           ///< 取负: a = -a
    Eum_BC_INC = 0x16,           ///< 自增: a = a + 1
    Eum_BC_DEC = 0x17,           ///< 自减: a = a - 1
    Eum_BC_ADD_IMM8 = 0x18,      ///< 加立即数（8位）: a = a + imm
    Eum_BC_SUB_IMM8 = 0x19,      ///< 减立即数（8位）: a = a - imm
    Eum_BC_MUL_IMM8 = 0x1A,      ///< 乘立即数（8位）: a = a * imm
    Eum_BC_DIV_IMM8 = 0x1B,      ///< 除立即数（8位）: a = a / imm
    Eum_BC_MOD_IMM8 = 0x1C,      ///< 取模立即数（8位）: a = a % imm
    Eum_BC_POW = 0x1D,           ///< 幂运算: b = a ^ b
    Eum_BC_SHL = 0x1E,           ///< 左移: b = a << b
    Eum_BC_SHR = 0x1F,           ///< 右移: b = a >> b
    Eum_BC_BIT_AND = 0x20,       ///< 位与: b = a & b
    Eum_BC_BIT_OR = 0x21,        ///< 位或: b = a | b
    Eum_BC_BIT_XOR = 0x22,       ///< 位异或: b = a ^ b
    Eum_BC_BIT_NOT = 0x23,       ///< 位非: a = ~a
    Eum_BC_ABS = 0x24,           ///< 绝对值: a = abs(a)
    Eum_BC_SIGN = 0x25,          ///< 符号函数: a = sign(a)
    
    // ============================================
    // 逻辑运算指令 (0x30-0x4F)
    // ============================================
    Eum_BC_NOT = 0x30,           ///< 逻辑非: a = !a
    Eum_BC_AND = 0x31,           ///< 逻辑与: b = a && b
    Eum_BC_OR = 0x32,            ///< 逻辑或: b = a || b
    Eum_BC_EQ = 0x33,            ///< 等于: b = a == b
    Eum_BC_NE = 0x34,            ///< 不等于: b = a != b
    Eum_BC_LT = 0x35,            ///< 小于: b = a < b
    Eum_BC_LE = 0x36,            ///< 小于等于: b = a <= b
    Eum_BC_GT = 0x37,            ///< 大于: b = a > b
    Eum_BC_GE = 0x38,            ///< 大于等于: b = a >= b
    Eum_BC_IS_NULL = 0x39,       ///< 是否为null: a = (a == null)
    Eum_BC_IS_TRUE = 0x3A,       ///< 是否为true: a = (a == true)
    Eum_BC_IS_FALSE = 0x3B,      ///< 是否为false: a = (a == false)
    Eum_BC_IS_ZERO = 0x3C,       ///< 是否为零: a = (a == 0)
    
    // ============================================
    // 控制流指令 (0x50-0x6F)
    // ============================================
    Eum_BC_JUMP = 0x50,          ///< 无条件跳转
    Eum_BC_JUMP_IF_TRUE = 0x51,  ///< 如果为真则跳转
    Eum_BC_JUMP_IF_FALSE = 0x52, ///< 如果为假则跳转
    Eum_BC_JUMP_IF_NULL = 0x53,  ///< 如果为null则跳转
    Eum_BC_JUMP_IF_NOT_NULL = 0x54, ///< 如果不为null则跳转
    Eum_BC_JUMP_IF_ZERO = 0x55,  ///< 如果为零则跳转
    Eum_BC_JUMP_IF_NOT_ZERO = 0x56, ///< 如果不为零则跳转
    Eum_BC_CALL = 0x57,          ///< 函数调用
    Eum_BC_RETURN = 0x58,        ///< 从函数返回
    Eum_BC_RETURN_VOID = 0x59,   ///< 从函数返回（无返回值）
    Eum_BC_THROW = 0x5A,         ///< 抛出异常
    Eum_BC_TRY = 0x5B,           ///< 异常处理开始
    Eum_BC_CATCH = 0x5C,         ///< 异常捕获
    Eum_BC_FINALLY = 0x5D,       ///< 异常处理结束
    Eum_BC_END_TRY = 0x5E,       ///< 异常处理块结束
    
    // ============================================
    // 内存访问指令 (0x70-0x8F)
    // ============================================
    Eum_BC_LOAD_LOCAL = 0x70,    ///< 加载局部变量
    Eum_BC_STORE_LOCAL = 0x71,   ///< 存储到局部变量
    Eum_BC_LOAD_GLOBAL = 0x72,   ///< 加载全局变量
    Eum_BC_STORE_GLOBAL = 0x73,  ///< 存储到全局变量
    Eum_BC_LOAD_FIELD = 0x74,    ///< 加载对象字段
    Eum_BC_STORE_FIELD = 0x75,   ///< 存储到对象字段
    Eum_BC_LOAD_ARRAY = 0x76,    ///< 加载数组元素
    Eum_BC_STORE_ARRAY = 0x77,   ///< 存储到数组元素
    Eum_BC_LOAD_CONST = 0x78,    ///< 加载常量
    Eum_BC_NEW_OBJECT = 0x79,    ///< 创建新对象
    Eum_BC_NEW_ARRAY = 0x7A,     ///< 创建新数组
    Eum_BC_ARRAY_LENGTH = 0x7B,  ///< 获取数组长度
    Eum_BC_GET_TYPE = 0x7C,      ///< 获取值的类型
    Eum_BC_CHECK_TYPE = 0x7D,    ///< 检查类型
    Eum_BC_CAST = 0x7E,          ///< 类型转换
    
    // ============================================
    // 函数调用指令 (0x90-0xAF)
    // ============================================
    Eum_BC_CALL_FUNC = 0x90,     ///< 调用函数（带参数数量）
    Eum_BC_CALL_METHOD = 0x91,   ///< 调用方法（带参数数量）
    Eum_BC_CALL_NATIVE = 0x92,   ///< 调用原生函数
    Eum_BC_CALL_CONSTRUCTOR = 0x93, ///< 调用构造函数
    Eum_BC_TAIL_CALL = 0x94,     ///< 尾调用优化
    Eum_BC_APPLY = 0x95,         ///< 应用函数（参数在数组中）
    Eum_BC_BIND = 0x96,          ///< 绑定函数参数
    
    // ============================================
    // 类型转换指令 (0xB0-0xCF)
    // ============================================
    Eum_BC_TO_INT = 0xB0,        ///< 转换为整数
    Eum_BC_TO_FLOAT = 0xB1,      ///< 转换为浮点数
    Eum_BC_TO_STRING = 0xB2,     ///< 转换为字符串
    Eum_BC_TO_BOOL = 0xB3,       ///< 转换为布尔值
    Eum_BC_INT_TO_FLOAT = 0xB4,  ///< 整数转浮点数
    Eum_BC_FLOAT_TO_INT = 0xB5,  ///< 浮点数转整数
    Eum_BC_INT_TO_STRING = 0xB6, ///< 整数转字符串
    Eum_BC_FLOAT_TO_STRING = 0xB7, ///< 浮点数转字符串
    Eum_BC_BOOL_TO_STRING = 0xB8, ///< 布尔值转字符串
    
    // ============================================
    // 特殊指令 (0xD0-0xEF)
    // ============================================
    Eum_BC_DEBUG = 0xD0,         ///< 调试指令
    Eum_BC_BREAKPOINT = 0xD1,    ///< 断点指令
    Eum_BC_PROFILE_START = 0xD2, ///< 开始性能分析
    Eum_BC_PROFILE_END = 0xD3,   ///< 结束性能分析
    Eum_GC_COLLECT = 0xD4,       ///< 触发垃圾回收
    Eum_BC_GET_STACK_TRACE = 0xD5, ///< 获取调用栈
    Eum_BC_GET_CURRENT_FUNCTION = 0xD6, ///< 获取当前函数
    Eum_BC_GET_CURRENT_LINE = 0xD7, ///< 获取当前行号
    
    // ============================================
    // 保留/扩展指令 (0xF0-0xFF)
    // ============================================
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

// ============================================================================
// 字节码指令结构体
// ============================================================================

/**
 * @brief 字节码指令结构体
 * 
 * 表示单个字节码指令，包含操作码和可能的立即数。
 */
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

#ifdef __cplusplus
}
#endif

#endif /* CN_BYTECODE_INSTRUCTIONS_H */
