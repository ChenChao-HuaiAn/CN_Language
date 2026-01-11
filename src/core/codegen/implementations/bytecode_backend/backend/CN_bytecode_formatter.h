/******************************************************************************
 * 文件名: CN_bytecode_formatter.h
 * 功能: 字节码格式化器模块头文件
 * 
 * 声明字节码程序的格式化功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_BYTECODE_FORMATTER_H
#define CN_BYTECODE_FORMATTER_H

#include "../CN_bytecode_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 公共函数声明
// ============================================================================

/**
 * @brief 格式化字节码程序为文本
 * 
 * @param program 要格式化的字节码程序
 * @return char* 格式化后的文本字符串（需要调用者释放）
 */
char* F_format_bytecode_program_impl(const Stru_BytecodeProgram_t* program);

#ifdef __cplusplus
}
#endif

#endif /* CN_BYTECODE_FORMATTER_H */
