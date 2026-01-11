/******************************************************************************
 * 文件名: CN_ssa_converter_function.c
 * 功能: CN_Language SSA转换器函数转换
 * 
 * 实现SSA转换器的函数转换功能。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_ssa_converter_internal.h"

// ============================================================================
// AST转换函数实现 - 函数部分
// ============================================================================

/**
 * @brief 转换函数
 */
bool F_convert_function(Stru_SsaConverterContext_t* context, Stru_AstNode_t* func)
{
    if (context == NULL || func == NULL) {
        return false;
    }
    
    // 获取函数名
    const char* func_name = F_ast_get_identifier(func);
    if (func_name == NULL) {
        F_add_error(context, "函数缺少名称");
        return false;
    }
    
    // 创建SSA函数
    Stru_SsaFunction_t* ssa_func = F_create_ssa_function(func_name);
    if (ssa_func == NULL) {
        F_add_error(context, "创建SSA函数失败");
        return false;
    }
    
    // 添加到SSA数据
    if (!F_ssa_data_add_function(context->ssa_data, ssa_func)) {
        F_add_error(context, "添加函数到SSA数据失败");
        F_destroy_ssa_function(ssa_func);
        return false;
    }
    
    // 设置当前函数
    context->current_function = ssa_func;
    
    // 创建入口基本块
    Stru_SsaBasicBlock_t* entry_block = F_create_ssa_basic_block("entry");
    if (entry_block == NULL) {
        F_add_error(context, "创建函数入口基本块失败");
        return false;
    }
    
    // 添加到函数
    if (!F_ssa_function_add_block(ssa_func, entry_block)) {
        F_add_error(context, "添加入口基本块到函数失败");
        F_destroy_ssa_basic_block(entry_block);
        return false;
    }
    
    // 设置当前基本块
    context->current_block = entry_block;
    
    // 转换函数体
    // TODO: 获取函数体节点并转换
    // 目前假设函数体是最后一个子节点
    size_t child_count = F_ast_get_child_count(func);
    if (child_count > 0) {
        // 最后一个子节点是函数体
        Stru_AstNode_t* body_node = F_ast_get_child(func, child_count - 1);
        if (!F_convert_ast_node(context, body_node)) {
            return false;
        }
    }
    
    // 如果没有返回语句，添加隐式返回
    // TODO: 检查基本块是否以返回指令结束
    
    return true;
}
