/******************************************************************************
 * 文件名: CN_ir_interface.h
 * 功能: CN_Language 中间代码表示（IR）核心接口定义
 * 
 * 定义中间代码表示的核心接口和数据结构，支持多种IR形式。
 * 遵循项目架构规范，采用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月10日
 * 修改历史:
 *   - 2026年1月10日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_IR_INTERFACE_H
#define CN_IR_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include "../ast/CN_ast.h"

// ============================================================================
// 类型定义
// ============================================================================

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

// ============================================================================
// 工厂函数声明
// ============================================================================

/**
 * @brief 创建IR接口实例
 * 
 * 根据指定的IR类型创建相应的IR接口实例。
 * 
 * @param type IR类型
 * @return Stru_IrInterface_t* IR接口实例，失败返回NULL
 */
Stru_IrInterface_t* F_create_ir_interface(Eum_IrType type);

/**
 * @brief 销毁IR接口实例
 * 
 * 销毁IR接口实例及其所有资源。
 * 
 * @param interface IR接口实例
 */
void F_destroy_ir_interface(Stru_IrInterface_t* interface);

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

/**
 * @brief 检查IR类型是否支持
 * 
 * 检查指定的IR类型是否被支持。
 * 
 * @param type IR类型
 * @return bool 支持返回true，不支持返回false
 */
bool F_is_ir_type_supported(Eum_IrType type);

// ============================================================================
// 工具函数声明
// ============================================================================

/**
 * @brief IR类型转字符串
 * 
 * 将IR类型转换为可读的字符串表示。
 * 
 * @param type IR类型
 * @return const char* 类型字符串表示
 */
const char* F_ir_type_to_string(Eum_IrType type);

/**
 * @brief IR指令类型转字符串
 * 
 * 将IR指令类型转换为可读的字符串表示。
 * 
 * @param type IR指令类型
 * @return const char* 指令类型字符串表示
 */
const char* F_ir_instruction_type_to_string(Eum_IrInstructionType type);

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

/**
 * @brief 销毁IR指令
 * 
 * 销毁IR指令及其所有资源。
 * 
 * @param instruction IR指令
 */
void F_destroy_ir_instruction(Stru_IrInstruction_t* instruction);

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

/**
 * @brief 设置IR指令的额外数据
 * 
 * 设置IR指令的额外数据。
 * 
 * @param instruction IR指令
 * @param extra_data 额外数据
 */
void F_ir_instruction_set_extra_data(Stru_IrInstruction_t* instruction, void* extra_data);

/**
 * @brief 获取IR指令的额外数据
 * 
 * 获取IR指令的额外数据。
 * 
 * @param instruction IR指令
 * @return void* 额外数据
 */
void* F_ir_instruction_get_extra_data(const Stru_IrInstruction_t* instruction);

// ============================================================================
// 版本信息函数
// ============================================================================

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

/**
 * @brief 获取IR模块版本字符串
 * 
 * 获取IR模块的版本字符串。
 * 
 * @return const char* 版本字符串
 */
const char* F_get_ir_module_version_string(void);

#endif /* CN_IR_INTERFACE_H */
