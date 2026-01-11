/******************************************************************************
 * 文件名: CN_ssa_converter_statement.c
 * 功能: CN_Language SSA转换器语句转换（基本语句）
 
 * 实现SSA转换器的基本语句转换功能。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_ssa_converter_internal.h"

// ============================================================================
// AST转换函数实现 - 语句部分（基本语句）
// ============================================================================

/**
 * @brief 转换语句
 */
bool F_convert_statement(Stru_SsaConverterContext_t* context, Stru_AstNode_t* stmt)
{
    if (context == NULL || stmt == NULL || context->current_block == NULL) {
        return false;
    }
    
    // 获取内部接口
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(stmt);
    if (interface == NULL) {
        F_add_error(context, "获取AST节点接口失败");
        return false;
    }
    
    // 获取节点类型
    Eum_AstNodeType node_type = interface->get_type(interface);
    
    // 根据语句类型进行转换
    switch (node_type) {
        case Eum_AST_EXPRESSION_STMT: {
            // 表达式语句: 转换表达式并丢弃结果
            if (interface->get_child_count(interface) < 1) {
                F_add_error(context, "表达式语句缺少表达式");
                return false;
            }
            
            // 获取表达式节点
            Stru_AstNodeInterface_t* expr_interface = interface->get_child(interface, 0);
            Stru_AstNode_t expr_node = { expr_interface, NULL };
            
            // 转换表达式
            if (!F_convert_expression(context, &expr_node)) {
                return false;
            }
            
            return true;
        }
        
        case Eum_AST_RETURN_STMT: {
            // 返回语句
            Stru_SsaVariable_t* return_var = NULL;
            
            if (interface->get_child_count(interface) > 0) {
                // 有返回值的返回语句
                Stru_AstNodeInterface_t* expr_interface = interface->get_child(interface, 0);
                Stru_AstNode_t expr_node = { expr_interface, NULL };
                
                if (!F_convert_expression(context, &expr_node)) {
                    return false;
                }
                
                // 获取返回值变量
                return_var = F_get_or_create_variable(context, "temp", false);
                if (return_var == NULL) {
                    F_add_error(context, "获取返回值变量失败");
                    return false;
                }
            }
            
            // 获取位置信息
            const Stru_AstNodeLocation_t* location = interface->get_location(interface);
            size_t line = location ? location->line : 0;
            size_t column = location ? location->column : 0;
            
            // 创建返回指令
            Stru_SsaInstruction_t* instr = F_create_ssa_instruction(
                Eum_SSA_OP_RETURN, return_var, line, column);
            if (instr == NULL) {
                F_add_error(context, "创建返回指令失败");
                return false;
            }
            
            // 将指令添加到当前基本块
            if (!F_ssa_basic_block_add_instruction(context->current_block, instr)) {
                F_add_error(context, "添加指令到基本块失败");
                return false;
            }
            
            return true;
        }
        
        case Eum_AST_IF_STMT:
        case Eum_AST_WHILE_STMT:
        case Eum_AST_FOR_STMT:
            // 控制流语句在另一个文件中处理
            F_add_error(context, "控制流语句应在控制流文件中处理");
            return false;
            
        default:
            // 尝试作为赋值表达式处理
            // 检查是否是赋值表达式（Eum_AST_ASSIGN_EXPR）
            if (node_type == Eum_AST_ASSIGN_EXPR) {
                // 赋值表达式: var = expr
                if (interface->get_child_count(interface) < 2) {
                    F_add_error(context, "赋值表达式缺少操作数");
                    return false;
                }
                
                // 获取左值节点（应该是标识符表达式）
                Stru_AstNodeInterface_t* left_interface = interface->get_child(interface, 0);
                if (left_interface == NULL) {
                    F_add_error(context, "获取左值节点失败");
                    return false;
                }
                
                // 检查左值节点类型
                if (left_interface->get_type(left_interface) != Eum_AST_IDENTIFIER_EXPR) {
                    F_add_error(context, "赋值表达式需要标识符作为左值");
                    return false;
                }
                
                // 获取变量名
                const Uni_AstNodeData_t* left_data = left_interface->get_data(left_interface);
                if (left_data == NULL || left_data->identifier == NULL) {
                    F_add_error(context, "获取变量名失败");
                    return false;
                }
                
                const char* var_name = left_data->identifier;
                
                // 转换右值表达式
                Stru_AstNodeInterface_t* right_interface = interface->get_child(interface, 1);
                Stru_AstNode_t right_node = { right_interface, NULL };
                if (!F_convert_expression(context, &right_node)) {
                    return false;
                }
                
                // 获取右值变量（假设存储在临时变量中）
                Stru_SsaVariable_t* value_var = F_get_or_create_variable(context, "temp", false);
                if (value_var == NULL) {
                    F_add_error(context, "获取右值变量失败");
                    return false;
                }
                
                // 获取或创建左值变量（创建新版本）
                Stru_SsaVariable_t* result_var = F_get_or_create_variable(context, var_name, true);
                if (result_var == NULL) {
                    F_add_error(context, "创建左值变量失败");
                    return false;
                }
                
                // 获取位置信息
                const Stru_AstNodeLocation_t* location = interface->get_location(interface);
                size_t line = location ? location->line : 0;
                size_t column = location ? location->column : 0;
                
                // 创建赋值指令
                Stru_SsaInstruction_t* instr = F_create_assignment_instruction(
                    result_var, value_var, line, column);
                if (instr == NULL) {
                    F_add_error(context, "创建赋值指令失败");
                    return false;
                }
                
                // 将指令添加到当前基本块
                if (!F_ssa_basic_block_add_instruction(context->current_block, instr)) {
                    F_add_error(context, "添加指令到基本块失败");
                    return false;
                }
                
                return true;
            }
            
            F_add_error(context, "不支持的语句类型");
            return false;
    }
}
