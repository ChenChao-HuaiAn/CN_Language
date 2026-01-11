/******************************************************************************
 * 文件名: CN_interpreter_engine.h
 * 功能: CN_Language 字节码解释器指令执行引擎头文件
 * 
 * 定义字节码解释器指令执行引擎的接口和数据结构。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_INTERPRETER_ENGINE_H
#define CN_INTERPRETER_ENGINE_H

#include "../CN_bytecode_interpreter.h"
#include "../CN_bytecode_instructions.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 公共函数声明
// ============================================================================

/**
 * @brief 执行算术指令
 */
Stru_BytecodeValue_t F_execute_arithmetic_instruction(Stru_BytecodeInterpreter_t* interpreter, 
                                                     const Stru_BytecodeInstruction_t* instruction);

/**
 * @brief 执行比较指令
 */
Stru_BytecodeValue_t F_execute_comparison_instruction(Stru_BytecodeInterpreter_t* interpreter, 
                                                     const Stru_BytecodeInstruction_t* instruction);

/**
 * @brief 执行逻辑指令
 */
Stru_BytecodeValue_t F_execute_logical_instruction(Stru_BytecodeInterpreter_t* interpreter, 
                                                  const Stru_BytecodeInstruction_t* instruction);

#ifdef __cplusplus
}
#endif

#endif /* CN_INTERPRETER_ENGINE_H */
