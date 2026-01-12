/******************************************************************************
 * 文件名: CN_interpreter_debug.h
 * 功能: CN_Language 字节码解释器调试功能模块头文件
 * 
 * 定义字节码解释器调试功能模块的接口和数据结构。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_INTERPRETER_DEBUG_H
#define CN_INTERPRETER_DEBUG_H

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
 * @brief 获取解释器错误信息
 */
void F_get_interpreter_errors(const Stru_BytecodeInterpreter_t* interpreter,
                              char*** errors, size_t* error_count);

/**
 * @brief 清除解释器错误
 */
void F_clear_interpreter_errors(Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 设置断点
 */
bool F_set_breakpoint(Stru_BytecodeInterpreter_t* interpreter,
                      uint32_t instruction_offset);

/**
 * @brief 清除断点
 */
bool F_clear_breakpoint(Stru_BytecodeInterpreter_t* interpreter,
                        uint32_t instruction_offset);

/**
 * @brief 清除所有断点
 */
void F_clear_all_breakpoints(Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 获取当前栈帧信息
 */
bool F_get_current_stack_frame(const Stru_BytecodeInterpreter_t* interpreter,
                               size_t frame_index,
                               const char** function_name,
                               uint32_t* line_number,
                               uint32_t* instruction_offset);

/**
 * @brief 获取栈跟踪
 */
bool F_get_interpreter_stack_trace(const Stru_BytecodeInterpreter_t* interpreter,
                                   void*** frames, size_t* frame_count);

/**
 * @brief 添加错误信息（内部实现）
 */
void F_add_error_internal(Stru_BytecodeInterpreter_t* interpreter, const char* format, ...);

/**
 * @brief 确保错误数组容量（内部函数）
 */
bool F_ensure_error_capacity(Stru_BytecodeInterpreter_t* interpreter, size_t required_capacity);

#ifdef __cplusplus
}
#endif

#endif /* CN_INTERPRETER_DEBUG_H */
