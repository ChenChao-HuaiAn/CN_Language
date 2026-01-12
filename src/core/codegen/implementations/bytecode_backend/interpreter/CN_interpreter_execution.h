/******************************************************************************
 * 文件名: CN_interpreter_execution.h
 * 功能: CN_Language 字节码解释器程序执行控制模块头文件
 * 
 * 定义字节码解释器程序执行控制模块的接口和数据结构。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_INTERPRETER_EXECUTION_H
#define CN_INTERPRETER_EXECUTION_H

#include "../CN_bytecode_interpreter.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 公共函数声明
// ============================================================================

/**
 * @brief 加载字节码程序
 */
bool F_load_bytecode_program(Stru_BytecodeInterpreter_t* interpreter,
                             Stru_BytecodeProgram_t* program);

/**
 * @brief 执行字节码程序
 */
bool F_execute_bytecode_program(Stru_BytecodeInterpreter_t* interpreter);


/**
 * @brief 获取性能分析数据
 */
void F_get_profiling_data(const Stru_BytecodeInterpreter_t* interpreter,
                          uint64_t* instruction_count,
                          uint64_t* execution_time,
                          size_t* memory_usage);

#ifdef __cplusplus
}
#endif

#endif /* CN_INTERPRETER_EXECUTION_H */
