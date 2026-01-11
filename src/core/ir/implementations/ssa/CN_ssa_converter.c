/******************************************************************************
 * 文件名: CN_ssa_converter.c
 * 功能: CN_Language 静态单赋值形式（SSA）转换器主文件
 * 
 * SSA转换器的主文件，包含公共接口函数。
 * 注意：此文件已被拆分为多个子文件，具体实现位于：
 * - CN_ssa_converter_main.c: 公共接口函数
 * - CN_ssa_converter_context.c: 转换器上下文管理
 * - CN_ssa_converter_variable.c: 变量版本映射表管理
 * - CN_ssa_converter_expression.c: 表达式转换
 * - CN_ssa_converter_statement.c: 基本语句转换
 * - CN_ssa_converter_statement_control.c: 控制流语句转换
 * - CN_ssa_converter_function.c: 函数转换
 * - CN_ssa_converter_instruction.c: 指令创建辅助函数
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 *   - 2026年1月11日: 拆分为多个文件，此文件保留为公共接口
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_ssa_interface.h"

// ============================================================================
// 公共接口函数声明
// ============================================================================

/**
 * @brief 从AST转换为SSA
 */
Stru_SsaData_t* F_convert_ast_to_ssa(Stru_AstNode_t* ast);

/**
 * @brief 从TAC转换为SSA
 */
Stru_SsaData_t* F_convert_tac_to_ssa(const void* tac_data);

/**
 * @brief 插入φ函数
 */
bool F_insert_phi_functions(Stru_SsaData_t* ssa_data);

// ============================================================================
// 文件结束
// ============================================================================
