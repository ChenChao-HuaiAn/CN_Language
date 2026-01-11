/******************************************************************************
 * 文件名: CN_ssa_converter_main.c
 * 功能: CN_Language SSA转换器主文件
 * 
 * 实现SSA转换器的公共接口函数。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_ssa_converter_internal.h"

// ============================================================================
// 公共接口函数实现
// ============================================================================

/**
 * @brief 从AST转换为SSA
 */
Stru_SsaData_t* F_convert_ast_to_ssa(Stru_AstNode_t* ast)
{
    if (ast == NULL) {
        return NULL;
    }
    
    // 创建转换器上下文
    Stru_SsaConverterContext_t* context = F_create_converter_context();
    if (context == NULL) {
        return NULL;
    }
    
    // 创建SSA数据
    context->ssa_data = F_create_ssa_data();
    if (context->ssa_data == NULL) {
        F_destroy_converter_context(context);
        return NULL;
    }
    
    // 转换AST根节点
    if (!F_convert_ast_node(context, ast)) {
        // 转换失败，复制错误信息
        if (context->error_count > 0) {
            for (size_t i = 0; i < context->error_count; i++) {
                F_ssa_data_add_error(context->ssa_data, context->errors[i]);
            }
        } else {
            F_ssa_data_add_error(context->ssa_data, "AST到SSA转换失败");
        }
        
        F_destroy_converter_context(context);
        F_destroy_ssa_data(context->ssa_data);
        return NULL;
    }
    
    // 插入φ函数
    if (!F_insert_phi_functions(context->ssa_data)) {
        F_ssa_data_add_error(context->ssa_data, "φ函数插入失败");
        // 继续返回数据，但标记为有错误
    }
    
    // 清理上下文，但保留SSA数据
    Stru_SsaData_t* result = context->ssa_data;
    F_destroy_converter_context(context);
    
    return result;
}

/**
 * @brief 从TAC转换为SSA
 */
Stru_SsaData_t* F_convert_tac_to_ssa(const void* tac_data)
{
    // TODO: 实现TAC到SSA的转换
    // 目前返回NULL，表示未实现
    return NULL;
}
