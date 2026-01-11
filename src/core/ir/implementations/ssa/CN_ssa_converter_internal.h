/******************************************************************************
 * 文件名: CN_ssa_converter_internal.h
 * 功能: CN_Language SSA转换器内部接口定义
 * 
 * 定义SSA转换器的内部数据结构和函数声明。
 * 这些接口仅供SSA转换器内部使用，不对外暴露。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_SSA_CONVERTER_INTERNAL_H
#define CN_SSA_CONVERTER_INTERNAL_H

#include "CN_ssa_interface.h"
#include "../../../infrastructure/memory/utilities/CN_memory_utilities.h"
#include "../../../infrastructure/containers/string/CN_string.h"
#include "../../ast/CN_ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部数据结构定义
// ============================================================================

/**
 * @brief 变量版本映射条目
 */
typedef struct {
    char* variable_name;        ///< 变量名（不含版本号）
    int current_version;        ///< 当前版本号
    Stru_SsaVariable_t* variable; ///< 对应的SSA变量
} Stru_VariableVersionEntry_t;

/**
 * @brief 变量版本映射表
 */
typedef struct {
    Stru_VariableVersionEntry_t** entries; ///< 条目数组
    size_t entry_count;                    ///< 条目数量
    size_t capacity;                       ///< 数组容量
} Stru_VariableVersionMap_t;

/**
 * @brief 转换器上下文
 */
typedef struct {
    Stru_SsaData_t* ssa_data;              ///< 输出的SSA数据
    Stru_SsaFunction_t* current_function;  ///< 当前正在转换的函数
    Stru_SsaBasicBlock_t* current_block;   ///< 当前基本块
    Stru_VariableVersionMap_t* var_map;    ///< 变量版本映射表
    int next_temp_id;                      ///< 下一个临时变量ID
    char** errors;                         ///< 错误信息数组
    size_t error_count;                    ///< 错误数量
} Stru_SsaConverterContext_t;

// ============================================================================
// 转换器上下文函数声明
// ============================================================================

Stru_SsaConverterContext_t* F_create_converter_context(void);
void F_destroy_converter_context(Stru_SsaConverterContext_t* context);
bool F_add_error(Stru_SsaConverterContext_t* context, const char* error);

// ============================================================================
// 变量版本映射表函数声明
// ============================================================================

Stru_VariableVersionMap_t* F_create_variable_version_map(void);
void F_destroy_variable_version_map(Stru_VariableVersionMap_t* map);
Stru_SsaVariable_t* F_get_or_create_variable(Stru_SsaConverterContext_t* context, 
                                           const char* name, bool create_new);
Stru_SsaVariable_t* F_create_temporary_variable(Stru_SsaConverterContext_t* context);

// ============================================================================
// AST转换函数声明
// ============================================================================

bool F_convert_ast_node(Stru_SsaConverterContext_t* context, Stru_AstNode_t* node);
bool F_convert_expression(Stru_SsaConverterContext_t* context, Stru_AstNode_t* expr);
bool F_convert_statement(Stru_SsaConverterContext_t* context, Stru_AstNode_t* stmt);
bool F_convert_function(Stru_SsaConverterContext_t* context, Stru_AstNode_t* func);

// ============================================================================
// 指令创建辅助函数声明
// ============================================================================

Stru_SsaInstruction_t* F_create_binary_op_instruction(Eum_SsaOpcode opcode,
                                                    Stru_SsaVariable_t* result,
                                                    Stru_SsaVariable_t* left,
                                                    Stru_SsaVariable_t* right,
                                                    size_t line, size_t column);
Stru_SsaInstruction_t* F_create_unary_op_instruction(Eum_SsaOpcode opcode,
                                                   Stru_SsaVariable_t* result,
                                                   Stru_SsaVariable_t* operand,
                                                   size_t line, size_t column);
Stru_SsaInstruction_t* F_create_assignment_instruction(Stru_SsaVariable_t* result,
                                                     Stru_SsaVariable_t* value,
                                                     size_t line, size_t column);

#endif /* CN_SSA_CONVERTER_INTERNAL_H */
