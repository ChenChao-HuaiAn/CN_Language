/******************************************************************************
 * 文件名: CN_ssa_converter_expression.c
 * 功能: CN_Language SSA转换器表达式转换
 
 * 实现SSA转换器的表达式转换功能。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_ssa_converter_internal.h"

// ============================================================================
// AST转换函数实现 - 表达式部分
// ============================================================================

/**
 * @brief 转换AST节点
 */
bool F_convert_ast_node(Stru_SsaConverterContext_t* context, Stru_AstNode_t* node)
{
    if (context == NULL || node == NULL) {
        return false;
    }
    
    // 获取内部接口
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(node);
    if (interface == NULL) {
        F_add_error(context, "获取AST节点接口失败");
        return false;
    }
    
    // 获取节点类型
    Eum_AstNodeType node_type = interface->get_type(interface);
    
    // 根据节点类型进行转换
    switch (node_type) {
        case Eum_AST_FUNCTION_DECL:
            return F_convert_function(context, node);
            
        case Eum_AST_BINARY_EXPR:
        case Eum_AST_UNARY_EXPR:
        case Eum_AST_LITERAL_EXPR:
        case Eum_AST_IDENTIFIER_EXPR:
        case Eum_AST_CALL_EXPR:
            return F_convert_expression(context, node);
            
        case Eum_AST_EXPRESSION_STMT:
        case Eum_AST_RETURN_STMT:
        case Eum_AST_IF_STMT:
        case Eum_AST_WHILE_STMT:
        case Eum_AST_FOR_STMT:
            return F_convert_statement(context, node);
            
        case Eum_AST_BLOCK_STMT:
            // 代码块语句：依次转换所有子语句
            for (size_t i = 0; i < interface->get_child_count(interface); i++) {
                Stru_AstNodeInterface_t* child_interface = interface->get_child(interface, i);
                if (child_interface == NULL) {
                    continue;
                }
                
                // 将接口转换为兼容层节点
                Stru_AstNode_t child_node = { child_interface, NULL };
                if (!F_convert_ast_node(context, &child_node)) {
                    return false;
                }
            }
            return true;
            
        default:
            F_add_error(context, "不支持的AST节点类型");
            return false;
    }
}

/**
 * @brief 转换表达式
 */
bool F_convert_expression(Stru_SsaConverterContext_t* context, Stru_AstNode_t* expr)
{
    if (context == NULL || expr == NULL || context->current_block == NULL) {
        return false;
    }
    
    // 获取内部接口
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(expr);
    if (interface == NULL) {
        F_add_error(context, "获取AST节点接口失败");
        return false;
    }
    
    // 获取节点类型
    Eum_AstNodeType node_type = interface->get_type(interface);
    
    // 根据表达式类型进行转换
    switch (node_type) {
        case Eum_AST_BINARY_EXPR: {
            // 二元表达式: left op right
            if (interface->get_child_count(interface) < 2) {
                F_add_error(context, "二元表达式缺少操作数");
                return false;
            }
            
            // 转换左操作数
            Stru_AstNodeInterface_t* left_interface = interface->get_child(interface, 0);
            Stru_AstNode_t left_node = { left_interface, NULL };
            if (!F_convert_expression(context, &left_node)) {
                return false;
            }
            Stru_SsaVariable_t* left_var = F_get_or_create_variable(context, "temp", false);
            
            // 转换右操作数
            Stru_AstNodeInterface_t* right_interface = interface->get_child(interface, 1);
            Stru_AstNode_t right_node = { right_interface, NULL };
            if (!F_convert_expression(context, &right_node)) {
                return false;
            }
            Stru_SsaVariable_t* right_var = F_get_or_create_variable(context, "temp", false);
            
            // 创建临时变量作为结果
            Stru_SsaVariable_t* result_var = F_create_temporary_variable(context);
            if (result_var == NULL) {
                return false;
            }
            
            // 根据操作符创建对应的SSA指令
            Eum_SsaOpcode opcode;
            const Uni_AstNodeData_t* data = interface->get_data(interface);
            if (data == NULL) {
                F_add_error(context, "获取二元表达式数据失败");
                return false;
            }
            
            // 假设operator_type字段存储操作符类型
            int operator_type = data->operator_type;
            
            // 根据操作符类型映射到SSA操作码
            switch (operator_type) {
                case 1:  // 假设1表示加法
                    opcode = Eum_SSA_OP_ADD;
                    break;
                case 2:  // 假设2表示减法
                    opcode = Eum_SSA_OP_SUB;
                    break;
                case 3:  // 假设3表示乘法
                    opcode = Eum_SSA_OP_MUL;
                    break;
                case 4:  // 假设4表示除法
                    opcode = Eum_SSA_OP_DIV;
                    break;
                case 5:  // 假设5表示取模
                    opcode = Eum_SSA_OP_MOD;
                    break;
                case 6:  // 假设6表示等于
                    opcode = Eum_SSA_OP_EQ;
                    break;
                case 7:  // 假设7表示不等于
                    opcode = Eum_SSA_OP_NE;
                    break;
                case 8:  // 假设8表示小于
                    opcode = Eum_SSA_OP_LT;
                    break;
                case 9:  // 假设9表示小于等于
                    opcode = Eum_SSA_OP_LE;
                    break;
                case 10: // 假设10表示大于
                    opcode = Eum_SSA_OP_GT;
                    break;
                case 11: // 假设11表示大于等于
                    opcode = Eum_SSA_OP_GE;
                    break;
                case 12: // 假设12表示逻辑与
                    opcode = Eum_SSA_OP_AND;
                    break;
                case 13: // 假设13表示逻辑或
                    opcode = Eum_SSA_OP_OR;
                    break;
                default:
                    F_add_error(context, "不支持的二元操作符");
                    return false;
            }
            
            // 获取位置信息
            const Stru_AstNodeLocation_t* location = interface->get_location(interface);
            size_t line = location ? location->line : 0;
            size_t column = location ? location->column : 0;
            
            // 创建二元操作指令
            Stru_SsaInstruction_t* instr = F_create_binary_op_instruction(
                opcode, result_var, left_var, right_var, line, column);
            if (instr == NULL) {
                F_add_error(context, "创建二元操作指令失败");
                return false;
            }
            
            // 将指令添加到当前基本块
            if (!F_ssa_basic_block_add_instruction(context->current_block, instr)) {
                F_add_error(context, "添加指令到基本块失败");
                return false;
            }
            
            // 将结果变量存储到临时变量映射中
            // TODO: 实现临时变量映射
            return true;
        }
        
        case Eum_AST_UNARY_EXPR: {
            // 一元表达式: op operand
            if (interface->get_child_count(interface) < 1) {
                F_add_error(context, "一元表达式缺少操作数");
                return false;
            }
            
            // 转换操作数
            Stru_AstNodeInterface_t* operand_interface = interface->get_child(interface, 0);
            Stru_AstNode_t operand_node = { operand_interface, NULL };
            if (!F_convert_expression(context, &operand_node)) {
                return false;
            }
            Stru_SsaVariable_t* operand_var = F_get_or_create_variable(context, "temp", false);
            
            // 创建临时变量作为结果
            Stru_SsaVariable_t* result_var = F_create_temporary_variable(context);
            if (result_var == NULL) {
                return false;
            }
            
            // 根据操作符创建对应的SSA指令
            Eum_SsaOpcode opcode;
            const Uni_AstNodeData_t* data = interface->get_data(interface);
            if (data == NULL) {
                F_add_error(context, "获取一元表达式数据失败");
                return false;
            }
            
            // 假设operator_type字段存储操作符类型
            int operator_type = data->operator_type;
            
            // 根据操作符类型映射到SSA操作码
            switch (operator_type) {
                case 14: // 假设14表示取负
                    opcode = Eum_SSA_OP_NEG;
                    break;
                case 15: // 假设15表示逻辑非
                    opcode = Eum_SSA_OP_NOT;
                    break;
                default:
                    F_add_error(context, "不支持的一元操作符");
                    return false;
            }
            
            // 获取位置信息
            const Stru_AstNodeLocation_t* location = interface->get_location(interface);
            size_t line = location ? location->line : 0;
            size_t column = location ? location->column : 0;
            
            // 创建一元操作指令
            Stru_SsaInstruction_t* instr = F_create_unary_op_instruction(
                opcode, result_var, operand_var, line, column);
            if (instr == NULL) {
                F_add_error(context, "创建一元操作指令失败");
                return false;
            }
            
            // 将指令添加到当前基本块
            if (!F_ssa_basic_block_add_instruction(context->current_block, instr)) {
                F_add_error(context, "添加指令到基本块失败");
                return false;
            }
            
            return true;
        }
        
        case Eum_AST_IDENTIFIER_EXPR: {
            // 标识符表达式: 获取变量值
            const Uni_AstNodeData_t* data = interface->get_data(interface);
            if (data == NULL || data->identifier == NULL) {
                F_add_error(context, "获取标识符数据失败");
                return false;
            }
            
            const char* var_name = data->identifier;
            Stru_SsaVariable_t* var = F_get_or_create_variable(context, var_name, false);
            if (var == NULL) {
                F_add_error(context, "变量未定义");
                return false;
            }
            
            // 将变量存储到临时变量映射中
            // TODO: 实现临时变量映射
            return true;
        }
        
        case Eum_AST_INT_LITERAL:
        case Eum_AST_FLOAT_LITERAL:
        case Eum_AST_STRING_LITERAL:
        case Eum_AST_BOOL_LITERAL: {
            // 字面量表达式: 创建常量
            const Uni_AstNodeData_t* data = interface->get_data(interface);
            if (data == NULL) {
                F_add_error(context, "获取字面量数据失败");
                return false;
            }
            
            // 创建临时变量作为结果
            Stru_SsaVariable_t* result_var = F_create_temporary_variable(context);
            if (result_var == NULL) {
                return false;
            }
            
            // 获取位置信息
            const Stru_AstNodeLocation_t* location = interface->get_location(interface);
            size_t line = location ? location->line : 0;
            size_t column = location ? location->column : 0;
            
            // 创建常量指令
            Stru_SsaInstruction_t* instr = F_create_ssa_instruction(
                Eum_SSA_OP_COPY, result_var, line, column);
            if (instr == NULL) {
                F_add_error(context, "创建常量指令失败");
                return false;
            }
            
            // 设置常量值（这里简化处理，实际需要根据字面量类型处理）
            // 注意：这里需要根据实际需求实现常量值的存储
            // 目前只是创建一个空指令，实际使用时需要填充操作数
            
            // 将指令添加到当前基本块
            if (!F_ssa_basic_block_add_instruction(context->current_block, instr)) {
                F_add_error(context, "添加指令到基本块失败");
                return false;
            }
            
            return true;
        }
        
        case Eum_AST_CALL_EXPR: {
            // 函数调用表达式
            if (interface->get_child_count(interface) < 1) {
                F_add_error(context, "函数调用缺少函数名");
                return false;
            }
            
            // 获取函数名（第一个子节点应该是标识符表达式）
            Stru_AstNodeInterface_t* func_name_interface = interface->get_child(interface, 0);
            if (func_name_interface == NULL) {
                F_add_error(context, "获取函数名节点失败");
                return false;
            }
            
            // 检查节点类型
            if (func_name_interface->get_type(func_name_interface) != Eum_AST_IDENTIFIER_EXPR) {
                F_add_error(context, "函数调用需要标识符作为函数名");
                return false;
            }
            
            const Uni_AstNodeData_t* func_data = func_name_interface->get_data(func_name_interface);
            if (func_data == NULL || func_data->identifier == NULL) {
                F_add_error(context, "获取函数名数据失败");
                return false;
            }
            
            const char* func_name = func_data->identifier;
            
            // 创建临时变量作为结果
            Stru_SsaVariable_t* result_var = F_create_temporary_variable(context);
            if (result_var == NULL) {
                return false;
            }
            
            // 获取位置信息
            const Stru_AstNodeLocation_t* location = interface->get_location(interface);
            size_t line = location ? location->line : 0;
            size_t column = location ? location->column : 0;
            
            // 创建函数调用指令
            Stru_SsaInstruction_t* instr = F_create_ssa_instruction(
                Eum_SSA_OP_CALL, result_var, line, column);
            if (instr == NULL) {
                F_add_error(context, "创建函数调用指令失败");
                return false;
            }
            
            // 设置函数名（这里简化处理，实际需要处理参数）
            // 注意：这里需要根据实际需求实现函数调用参数的传递
            // 目前只是创建一个空指令，实际使用时需要填充操作数
            
            // 将指令添加到当前基本块
            if (!F_ssa_basic_block_add_instruction(context->current_block, instr)) {
                F_add_error(context, "添加指令到基本块失败");
                return false;
            }
            
            return true;
        }
        
        default:
            F_add_error(context, "不支持的表达式类型");
            return false;
    }
}
