/******************************************************************************
 * 文件名: CN_bytecode_generator.h
 * 功能: 字节码生成器模块头文件
 * 
 * 声明从AST和IR生成字节码的功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_BYTECODE_GENERATOR_H
#define CN_BYTECODE_GENERATOR_H

#include "../CN_bytecode_backend.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 公共函数声明
// ============================================================================

/**
 * @brief 从AST生成字节码
 */
Stru_BytecodeProgram_t* F_generate_bytecode_from_ast_impl(Stru_AstNode_t* ast, 
                                                         const Stru_BytecodeBackendConfig_t* config);

/**
 * @brief 从IR生成字节码
 */
Stru_BytecodeProgram_t* F_generate_bytecode_from_ir_impl(void* ir, 
                                                        const Stru_BytecodeBackendConfig_t* config);

/**
 * @brief 获取字节码后端版本信息
 */
void F_get_bytecode_backend_version_impl(int* major, int* minor, int* patch);

/**
 * @brief 获取字节码后端版本字符串
 */
const char* F_get_bytecode_backend_version_string_impl(void);

/**
 * @brief 检查字节码后端是否支持特定功能
 */
bool F_bytecode_backend_supports_feature_impl(const char* feature);

/**
 * @brief 配置字节码后端选项
 */
bool F_configure_bytecode_backend_impl(const char* option, const char* value);

/**
 * @brief 创建字节码后端代码生成器接口
 */
Stru_CodeGeneratorInterface_t* F_create_bytecode_backend_interface_impl(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_BYTECODE_GENERATOR_H */
