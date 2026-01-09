/**
 * @file CN_parser_ast_utils.c
 * @brief CN_Language 语法分析器AST工具模块实现
 * 
 * 语法分析器AST工具函数模块实现，提供AST节点创建和操作相关辅助函数。
 * 包括表达式节点、声明节点、字面量节点等的创建函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_utils.h"
#include "../interface/CN_parser_interface_impl.h"
#include "../../../infrastructure/memory/CN_memory_interface.h"
#include "../../../infrastructure/utils/CN_utils_interface.h"
#include <stdio.h>
#include <string.h>

// ============================================
// AST工具函数实现
// ============================================

/**
 * @brief 创建二元表达式节点
 */
Stru_AstNode_t* F_create_binary_expression_node(Stru_ParserInterface_t* interface,
                                               Stru_Token_t* operator_token,
                                               Stru_AstNode_t* left,
                                               Stru_AstNode_t* right)
{
    if (interface == NULL || operator_token == NULL || left == NULL || right == NULL) {
        return NULL;
    }
    
    // 创建二元表达式节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_BINARY_EXPR, 
                                                     operator_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 添加左操作数
    if (!interface->add_child_node(interface, node, left)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 添加右操作数
    if (!interface->add_child_node(interface, node, right)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 设置运算符类型属性
    char operator_type[32];
    snprintf(operator_type, sizeof(operator_type), "%d", operator_token->type);
    interface->set_node_attribute(interface, node, "operator_type", operator_type);
    
    return node;
}

/**
 * @brief 创建一元表达式节点
 */
Stru_AstNode_t* F_create_unary_expression_node(Stru_ParserInterface_t* interface,
                                              Stru_Token_t* operator_token,
                                              Stru_AstNode_t* operand)
{
    if (interface == NULL || operator_token == NULL || operand == NULL) {
        return NULL;
    }
    
    // 创建一元表达式节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_UNARY_EXPR, 
                                                     operator_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 添加操作数
    if (!interface->add_child_node(interface, node, operand)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 设置运算符类型属性
    char operator_type[32];
    snprintf(operator_type, sizeof(operator_type), "%d", operator_token->type);
    interface->set_node_attribute(interface, node, "operator_type", operator_type);
    
    return node;
}

/**
 * @brief 创建复合赋值表达式节点
 */
Stru_AstNode_t* F_create_compound_assignment_node(Stru_ParserInterface_t* interface,
                                                 Stru_Token_t* operator_token,
                                                 Stru_AstNode_t* left,
                                                 Stru_AstNode_t* right)
{
    if (interface == NULL || operator_token == NULL || left == NULL || right == NULL) {
        return NULL;
    }
    
    // 检查是否为有效的复合赋值运算符
    if (operator_token->type != Eum_TOKEN_OPERATOR_PLUS_ASSIGN &&
        operator_token->type != Eum_TOKEN_OPERATOR_MINUS_ASSIGN &&
        operator_token->type != Eum_TOKEN_OPERATOR_MULTIPLY_ASSIGN &&
        operator_token->type != Eum_TOKEN_OPERATOR_DIVIDE_ASSIGN &&
        operator_token->type != Eum_TOKEN_OPERATOR_MODULO_ASSIGN) {
        return NULL;
    }
    
    // 创建复合赋值表达式节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_COMPOUND_ASSIGN_EXPR, 
                                                     operator_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 添加左操作数
    if (!interface->add_child_node(interface, node, left)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 添加右操作数
    if (!interface->add_child_node(interface, node, right)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 设置运算符类型属性
    char operator_type[32];
    snprintf(operator_type, sizeof(operator_type), "%d", operator_token->type);
    interface->set_node_attribute(interface, node, "operator_type", operator_type);
    
    // 设置运算符名称属性
    const char* operator_name = NULL;
    switch (operator_token->type) {
        case Eum_TOKEN_OPERATOR_PLUS_ASSIGN:
            operator_name = "+=";
            break;
        case Eum_TOKEN_OPERATOR_MINUS_ASSIGN:
            operator_name = "-=";
            break;
        case Eum_TOKEN_OPERATOR_MULTIPLY_ASSIGN:
            operator_name = "*=";
            break;
        case Eum_TOKEN_OPERATOR_DIVIDE_ASSIGN:
            operator_name = "/=";
            break;
        case Eum_TOKEN_OPERATOR_MODULO_ASSIGN:
            operator_name = "%=";
            break;
        default:
            operator_name = "unknown";
            break;
    }
    interface->set_node_attribute(interface, node, "operator_name", (void*)operator_name);
    
    return node;
}

/**
 * @brief 创建字面量节点
 */
Stru_AstNode_t* F_create_literal_node(Stru_ParserInterface_t* interface,
                                     Stru_Token_t* token)
{
    if (interface == NULL || token == NULL) {
        return NULL;
    }
    
    // 根据令牌类型确定AST节点类型
    Eum_AstNodeType node_type = Eum_AST_LITERAL_EXPR;
    
    switch (token->type) {
        case Eum_TOKEN_LITERAL_INTEGER:
            node_type = Eum_AST_INT_LITERAL;
            break;
        case Eum_TOKEN_LITERAL_FLOAT:
            node_type = Eum_AST_FLOAT_LITERAL;
            break;
        case Eum_TOKEN_LITERAL_STRING:
            node_type = Eum_AST_STRING_LITERAL;
            break;
        case Eum_TOKEN_LITERAL_BOOLEAN:
        case Eum_TOKEN_KEYWORD_TRUE:
        case Eum_TOKEN_KEYWORD_FALSE:
            node_type = Eum_AST_BOOL_LITERAL;
            break;
        case Eum_TOKEN_KEYWORD_NULL:
            node_type = Eum_AST_NULL_LITERAL;
            break;
        default:
            // 如果不是字面量令牌，返回NULL
            return NULL;
    }
    
    // 创建字面量节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, node_type, token);
    if (node == NULL) {
        return NULL;
    }
    
    // 设置字面量值属性
    if (token->lexeme != NULL) {
        interface->set_node_attribute(interface, node, "value", (void*)token->lexeme);
    }
    
    return node;
}

/**
 * @brief 创建标识符节点
 */
Stru_AstNode_t* F_create_identifier_node(Stru_ParserInterface_t* interface,
                                        Stru_Token_t* token)
{
    if (interface == NULL || token == NULL || token->type != Eum_TOKEN_IDENTIFIER) {
        return NULL;
    }
    
    // 创建标识符节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_IDENTIFIER_EXPR, 
                                                     token);
    if (node == NULL) {
        return NULL;
    }
    
    // 设置标识符名称属性
    if (token->lexeme != NULL) {
        interface->set_node_attribute(interface, node, "name", (void*)token->lexeme);
    }
    
    return node;
}

/**
 * @brief 创建函数调用节点
 */
Stru_AstNode_t* F_create_function_call_node(Stru_ParserInterface_t* interface,
                                           Stru_Token_t* function_name,
                                           Stru_DynamicArray_t* arguments)
{
    if (interface == NULL || function_name == NULL) {
        return NULL;
    }
    
    // 创建函数调用节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_CALL_EXPR, 
                                                     function_name);
    if (node == NULL) {
        return NULL;
    }
    
    // 设置函数名称属性
    if (function_name->lexeme != NULL) {
        interface->set_node_attribute(interface, node, "function_name", 
                                     (void*)function_name->lexeme);
    }
    
    // 添加参数节点
    if (arguments != NULL) {
        size_t arg_count = F_dynamic_array_length(arguments);
        for (size_t i = 0; i < arg_count; i++) {
            Stru_AstNode_t* arg = *(Stru_AstNode_t**)F_dynamic_array_get(arguments, i);
            if (arg != NULL) {
                interface->add_child_node(interface, node, arg);
            }
        }
    }
    
    return node;
}

/**
 * @brief 创建变量声明节点
 */
Stru_AstNode_t* F_create_variable_declaration_node(Stru_ParserInterface_t* interface,
                                                  Stru_Token_t* keyword_token,
                                                  Stru_Token_t* identifier_token,
                                                  Stru_Token_t* type_token,
                                                  Stru_AstNode_t* initializer)
{
    if (interface == NULL || keyword_token == NULL || identifier_token == NULL) {
        return NULL;
    }
    
    // 创建变量声明节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_VARIABLE_DECL, 
                                                     keyword_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 设置变量名称属性
    if (identifier_token->lexeme != NULL) {
        interface->set_node_attribute(interface, node, "variable_name", 
                                     (void*)identifier_token->lexeme);
    }
    
    // 设置类型属性（如果有）
    if (type_token != NULL && type_token->lexeme != NULL) {
        interface->set_node_attribute(interface, node, "type_name", 
                                     (void*)type_token->lexeme);
    }
    
    // 添加初始化表达式（如果有）
    if (initializer != NULL) {
        interface->add_child_node(interface, node, initializer);
    }
    
    return node;
}

/**
 * @brief 创建函数声明节点
 */
Stru_AstNode_t* F_create_function_declaration_node(Stru_ParserInterface_t* interface,
                                                  Stru_Token_t* keyword_token,
                                                  Stru_Token_t* identifier_token,
                                                  Stru_DynamicArray_t* parameters,
                                                  Stru_AstNode_t* return_type,
                                                  Stru_AstNode_t* body)
{
    if (interface == NULL || keyword_token == NULL || identifier_token == NULL || body == NULL) {
        return NULL;
    }
    
    // 创建函数声明节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_FUNCTION_DECL, 
                                                     keyword_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 设置函数名称属性
    if (identifier_token->lexeme != NULL) {
        interface->set_node_attribute(interface, node, "function_name", 
                                     (void*)identifier_token->lexeme);
    }
    
    // 添加参数节点（如果有）
    if (parameters != NULL) {
        size_t param_count = F_dynamic_array_length(parameters);
        for (size_t i = 0; i < param_count; i++) {
            Stru_AstNode_t* param = *(Stru_AstNode_t**)F_dynamic_array_get(parameters, i);
            if (param != NULL) {
                interface->add_child_node(interface, node, param);
            }
        }
    }
    
    // 添加返回类型节点（如果有）
    if (return_type != NULL) {
        interface->add_child_node(interface, node, return_type);
    }
    
    // 添加函数体节点
    interface->add_child_node(interface, node, body);
    
    return node;
}

/**
 * @brief 创建类型转换表达式节点
 */
Stru_AstNode_t* F_create_cast_expression_node(Stru_ParserInterface_t* interface,
                                             Stru_Token_t* cast_token,
                                             Stru_AstNode_t* type_node,
                                             Stru_AstNode_t* operand)
{
    if (interface == NULL || cast_token == NULL || type_node == NULL || operand == NULL) {
        return NULL;
    }
    
    // 创建类型转换表达式节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_CAST_EXPR, 
                                                     cast_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 添加类型节点
    if (!interface->add_child_node(interface, node, type_node)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 添加操作数节点
    if (!interface->add_child_node(interface, node, operand)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 设置类型转换属性
    interface->set_node_attribute(interface, node, "cast_operator", "()");
    
    return node;
}

/**
 * @brief 创建成员访问表达式节点
 */
Stru_AstNode_t* F_create_member_access_node(Stru_ParserInterface_t* interface,
                                           Stru_Token_t* dot_token,
                                           Stru_AstNode_t* object,
                                           Stru_Token_t* member_name)
{
    if (interface == NULL || dot_token == NULL || object == NULL || member_name == NULL) {
        return NULL;
    }
    
    // 创建成员访问表达式节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_MEMBER_EXPR, 
                                                     dot_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 添加对象表达式节点
    if (!interface->add_child_node(interface, node, object)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 设置成员名称属性
    if (member_name->lexeme != NULL) {
        interface->set_node_attribute(interface, node, "member_name", 
                                     (void*)member_name->lexeme);
    }
    
    // 设置访问运算符属性
    interface->set_node_attribute(interface, node, "access_operator", ".");
    
    return node;
}

/**
 * @brief 创建数组索引表达式节点
 */
Stru_AstNode_t* F_create_array_index_node(Stru_ParserInterface_t* interface,
                                         Stru_Token_t* lbracket_token,
                                         Stru_AstNode_t* array,
                                         Stru_AstNode_t* index,
                                         Stru_Token_t* rbracket_token)
{
    if (interface == NULL || lbracket_token == NULL || array == NULL || index == NULL) {
        return NULL;
    }
    
    // 创建数组索引表达式节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_INDEX_EXPR, 
                                                     lbracket_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 添加数组表达式节点
    if (!interface->add_child_node(interface, node, array)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 添加索引表达式节点
    if (!interface->add_child_node(interface, node, index)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 设置索引运算符属性
    interface->set_node_attribute(interface, node, "index_operator", "[]");
    
    // 设置右括号令牌属性（如果有）
    if (rbracket_token != NULL) {
        interface->set_node_attribute(interface, node, "right_bracket_token", rbracket_token);
    }
    
    return node;
}

/**
 * @brief 创建条件表达式节点（三元运算符）
 */
Stru_AstNode_t* F_create_conditional_expression_node(Stru_ParserInterface_t* interface,
                                                    Stru_Token_t* question_token,
                                                    Stru_AstNode_t* condition,
                                                    Stru_AstNode_t* true_expr,
                                                    Stru_AstNode_t* false_expr,
                                                    Stru_Token_t* colon_token)
{
    if (interface == NULL || question_token == NULL || condition == NULL || 
        true_expr == NULL || false_expr == NULL) {
        return NULL;
    }
    
    // 创建条件表达式节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_CONDITIONAL_EXPR, 
                                                     question_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 添加条件表达式节点
    if (!interface->add_child_node(interface, node, condition)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 添加真值表达式节点
    if (!interface->add_child_node(interface, node, true_expr)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 添加假值表达式节点
    if (!interface->add_child_node(interface, node, false_expr)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 设置条件运算符属性
    interface->set_node_attribute(interface, node, "conditional_operator", "?:");
    
    // 设置冒号令牌属性（如果有）
    if (colon_token != NULL) {
        interface->set_node_attribute(interface, node, "colon_token", colon_token);
    }
    
    return node;
}
