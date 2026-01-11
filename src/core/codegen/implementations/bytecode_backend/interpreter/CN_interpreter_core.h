/******************************************************************************
 * 文件名: CN_interpreter_core.h
 * 功能: CN_Language 字节码解释器核心管理模块头文件
 * 
 * 定义字节码解释器核心管理模块的接口和数据结构。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_INTERPRETER_CORE_H
#define CN_INTERPRETER_CORE_H

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
 * @brief 创建字节码解释器
 */
Stru_BytecodeInterpreter_t* F_create_bytecode_interpreter(
    const Stru_BytecodeInterpreterConfig_t* config);

/**
 * @brief 销毁字节码解释器
 */
void F_destroy_bytecode_interpreter(Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 重置字节码解释器
 */
void F_reset_bytecode_interpreter(Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 获取解释器状态
 */
Eum_BytecodeInterpreterState F_get_interpreter_state(
    const Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 检查解释器是否有错误
 */
bool F_interpreter_has_errors(const Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 创建默认字节码解释器配置
 */
Stru_BytecodeInterpreterConfig_t F_create_default_bytecode_interpreter_config(void);

/**
 * @brief 获取字节码解释器版本信息
 */
void F_get_bytecode_interpreter_version(int* major, int* minor, int* patch);

/**
 * @brief 获取字节码解释器版本字符串
 */
const char* F_get_bytecode_interpreter_version_string(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_INTERPRETER_CORE_H */
