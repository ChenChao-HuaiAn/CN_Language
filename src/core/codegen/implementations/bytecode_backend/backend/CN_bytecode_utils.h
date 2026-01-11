/******************************************************************************
 * 文件名: CN_bytecode_utils.h
 * 功能: 字节码后端工具函数模块头文件
 * 
 * 声明字节码后端的内部状态管理、配置验证和工具函数。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_BYTECODE_UTILS_H
#define CN_BYTECODE_UTILS_H

#include "../CN_bytecode_backend.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 内部类型声明
// ============================================================================

/**
 * @brief 字节码后端内部状态结构体（前向声明）
 */
typedef struct BytecodeBackendState BytecodeBackendState;

// ============================================================================
// 工具函数声明
// ============================================================================

/**
 * @brief 创建后端状态
 */
BytecodeBackendState* F_create_backend_state(void);

/**
 * @brief 销毁后端状态
 */
void F_destroy_backend_state(BytecodeBackendState* state);

/**
 * @brief 验证配置
 */
bool F_validate_config(const Stru_BytecodeBackendConfig_t* config);

/**
 * @brief 创建字节码程序
 */
Stru_BytecodeProgram_t* F_create_bytecode_program(void);

/**
 * @brief 确保指令容量
 */
bool F_ensure_instruction_capacity(Stru_BytecodeProgram_t* program, size_t needed);

/**
 * @brief 确保常量池容量
 */
bool F_ensure_constant_capacity(Stru_BytecodeProgram_t* program, size_t needed);

/**
 * @brief 添加常量到常量池
 */
uint32_t F_add_constant_to_pool(Stru_BytecodeProgram_t* program, const char* constant);

/**
 * @brief 添加指令到程序
 */
void F_add_instruction(Stru_BytecodeProgram_t* program, 
                       const Stru_BytecodeInstruction_t* instruction);

/**
 * @brief 销毁字节码程序
 */
void F_destroy_bytecode_program(Stru_BytecodeProgram_t* program);

/**
 * @brief 创建默认字节码后端配置
 */
Stru_BytecodeBackendConfig_t F_create_default_bytecode_backend_config(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_BYTECODE_UTILS_H */
