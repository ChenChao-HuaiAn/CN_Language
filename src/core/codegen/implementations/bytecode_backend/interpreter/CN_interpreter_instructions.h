/******************************************************************************
 * 文件名: CN_interpreter_instructions.h
 * 功能: CN_Language 字节码解释器指令执行模块头文件
 * 
 * 定义指令执行相关的辅助函数，用于将大型指令执行函数拆分成更小的模块。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_INTERPRETER_INSTRUCTIONS_H
#define CN_INTERPRETER_INSTRUCTIONS_H

#include "../CN_bytecode_interpreter.h"
#include "../CN_bytecode_instructions.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 指令执行辅助函数声明
// ============================================================================

/**
 * @brief 执行基本操作指令
 */
Stru_BytecodeValue_t F_execute_basic_instruction(
    Stru_BytecodeInterpreter_t* interpreter, 
    const Stru_BytecodeInstruction_t* instruction);

/**
 * @brief 执行控制流指令
 */
Stru_BytecodeValue_t F_execute_control_flow_instruction(
    Stru_BytecodeInterpreter_t* interpreter, 
    const Stru_BytecodeInstruction_t* instruction);

/**
 * @brief 执行变量操作指令
 */
Stru_BytecodeValue_t F_execute_variable_instruction(
    Stru_BytecodeInterpreter_t* interpreter, 
    const Stru_BytecodeInstruction_t* instruction);

/**
 * @brief 执行类型转换指令
 */
Stru_BytecodeValue_t F_execute_conversion_instruction(
    Stru_BytecodeInterpreter_t* interpreter, 
    const Stru_BytecodeInstruction_t* instruction);

/**
 * @brief 执行调试指令
 */
Stru_BytecodeValue_t F_execute_debug_instruction(
    Stru_BytecodeInterpreter_t* interpreter, 
    const Stru_BytecodeInstruction_t* instruction);

/**
 * @brief 检查条件是否为真
 */
bool F_is_condition_true(const Stru_BytecodeValue_t* condition);

#ifdef __cplusplus
}
#endif

#endif /* CN_INTERPRETER_INSTRUCTIONS_H */
