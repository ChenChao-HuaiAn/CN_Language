/******************************************************************************
 * 文件名: CN_bytecode_validator.h
 * 功能: 字节码验证器模块头文件
 * 
 * 声明字节码程序的验证功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_BYTECODE_VALIDATOR_H
#define CN_BYTECODE_VALIDATOR_H

#include "../CN_bytecode_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 公共函数声明
// ============================================================================

/**
 * @brief 验证字节码程序
 * 
 * @param program 要验证的字节码程序
 * @param errors 错误信息数组（输出参数，需要调用者释放）
 * @param error_count 错误数量（输出参数）
 * @return true 验证通过，false 验证失败
 */
bool F_validate_bytecode_program_impl(const Stru_BytecodeProgram_t* program, 
                                     char*** errors, size_t* error_count);

#ifdef __cplusplus
}
#endif

#endif /* CN_BYTECODE_VALIDATOR_H */
