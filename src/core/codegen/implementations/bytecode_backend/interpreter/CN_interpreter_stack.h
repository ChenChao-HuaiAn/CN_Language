/******************************************************************************
 * 文件名: CN_interpreter_stack.h
 * 功能: CN_Language 字节码解释器栈操作模块头文件
 * 
 * 定义字节码解释器栈操作模块的接口和数据结构。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_INTERPRETER_STACK_H
#define CN_INTERPRETER_STACK_H

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
 * @brief 确保栈容量
 */
bool F_ensure_stack_capacity(Stru_BytecodeInterpreter_t* interpreter, size_t required_capacity);

/**
 * @brief 确保调用栈容量
 */
bool F_ensure_call_stack_capacity(Stru_BytecodeInterpreter_t* interpreter, size_t required_capacity);

/**
 * @brief 确保错误数组容量
 */
bool F_ensure_error_capacity(Stru_BytecodeInterpreter_t* interpreter, size_t required_capacity);

/**
 * @brief 推送值到栈
 */
bool F_push_value(Stru_BytecodeInterpreter_t* interpreter, Stru_BytecodeValue_t value);

/**
 * @brief 从栈弹出值
 */
Stru_BytecodeValue_t F_pop_value(Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 查看栈中的值
 */
Stru_BytecodeValue_t F_peek_value(Stru_BytecodeInterpreter_t* interpreter, size_t offset);

#ifdef __cplusplus
}
#endif

#endif /* CN_INTERPRETER_STACK_H */
