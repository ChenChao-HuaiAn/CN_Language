/******************************************************************************
 * 文件名: CN_bytecode_optimizer.h
 * 功能: 字节码优化器模块头文件
 * 
 * 声明字节码程序的优化功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_BYTECODE_OPTIMIZER_H
#define CN_BYTECODE_OPTIMIZER_H

#include "../CN_bytecode_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 公共函数声明
// ============================================================================

/**
 * @brief 优化字节码程序
 */
Stru_BytecodeProgram_t* F_optimize_bytecode_program_impl(Stru_BytecodeProgram_t* program, 
                                                        int optimization_level);

#ifdef __cplusplus
}
#endif

#endif /* CN_BYTECODE_OPTIMIZER_H */
