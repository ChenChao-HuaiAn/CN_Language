/******************************************************************************
 * 文件名: CN_bytecode_interpreter_adapter.h
 * 功能: CN_Language 字节码解释器适配器模块头文件
 * 
 * 定义字节码解释器适配器的公共接口，将现有的字节码解释器适配到执行引擎接口。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language架构委员会
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_BYTECODE_INTERPRETER_ADAPTER_H
#define CN_BYTECODE_INTERPRETER_ADAPTER_H

#include "../CN_execution_engine_interface.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 字节码解释器适配器接口函数
// ============================================================================

/**
 * @brief 创建字节码执行引擎接口实例
 * 
 * 创建并返回一个新的字节码执行引擎接口实例，该实例使用现有的字节码解释器。
 * 
 * @return Stru_ExecutionEngineInterface_t* 字节码执行引擎接口实例
 */
Stru_ExecutionEngineInterface_t* F_create_bytecode_execution_engine(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_BYTECODE_INTERPRETER_ADAPTER_H */
