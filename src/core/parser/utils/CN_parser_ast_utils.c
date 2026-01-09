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
 * @brief 创建switch语句节点
 */
Stru_AstNode_t* F_create_switch_statement_node(Stru_ParserInterface_t* interface,
                                              Stru_Token_t* switch_token,
                                              Stru_AstNode_t* expression,
                                              Stru_DynamicArray_t* cases,
                                              Stru_AstNode_t* default_case)
{
    if (interface == NULL || switch_token == NULL || expression == NULL) {
        return NULL;
    }
    
    // 检查是否为"选择"关键字
    if (switch_token->type != Eum_TOKEN_KEYWORD_SWITCH) {
        return NULL;
    }
    
    // 创建switch语句节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_SWITCH_STMT, 
                                                     switch_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 添加switch表达式节点
    if (!interface->add_child_node(interface, node, expression)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 添加case语句节点（如果有）
    if (cases != NULL) {
        size_t case_count = F_dynamic_array_length(cases);
        for (size_t i = 0; i < case_count; i++) {
            Stru_AstNode_t* case_node = *(Stru_AstNode_t**)F_dynamic_array_get(cases, i);
            if (case_node != NULL) {
                interface->add_child_node(interface, node, case_node);
            }
        }
    }
    
    // 添加default语句节点（如果有）
    if (default_case != NULL) {
        interface->add_child_node(interface, node, default_case);
    }
    
    // 设置switch语句属性
    interface->set_node_attribute(interface, node, "switch_keyword", "选择");
    
    return node;
}

/**
 * @brief 创建case语句节点
 */
Stru_AstNode_t* F_create_case_statement_node(Stru_ParserInterface_t* interface,
                                            Stru_Token_t* case_token,
                                            Stru_AstNode_t* case_expression,
                                            Stru_AstNode_t* case_body)
{
    if (interface == NULL || case_token == NULL || case_expression == NULL || case_body == NULL) {
        return NULL;
    }
    
    // 检查是否为"情况"关键字
    if (case_token->type != Eum_TOKEN_KEYWORD_CASE) {
        return NULL;
    }
    
    // 创建case语句节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_CASE_STMT, 
                                                     case_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 添加case表达式节点
    if (!interface->add_child_node(interface, node, case_expression)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 添加case体节点
    if (!interface->add_child_node(interface, node, case_body)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 设置case语句属性
    interface->set_node_attribute(interface, node, "case_keyword", "情况");
    
    return node;
}

/**
 * @brief 创建default语句节点
 */
Stru_AstNode_t* F_create_default_statement_node(Stru_ParserInterface_t* interface,
                                               Stru_Token_t* default_token,
                                               Stru_AstNode_t* default_body)
{
    if (interface == NULL || default_token == NULL || default_body == NULL) {
        return NULL;
    }
    
    // 检查是否为"默认"关键字
    if (default_token->type != Eum_TOKEN_KEYWORD_DEFAULT) {
        return NULL;
    }
    
    // 创建default语句节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_DEFAULT_STMT, 
                                                     default_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 添加default体节点
    if (!interface->add_child_node(interface, node, default_body)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 设置default语句属性
    interface->set_node_attribute(interface, node, "default_keyword", "默认");
    
    return node;
}

/**
 * @brief 创建数组字面量节点
 */
Stru_AstNode_t* F_create_array_literal_node(Stru_ParserInterface_t* interface,
                                           Stru_Token_t* lbracket_token,
                                           Stru_DynamicArray_t* elements,
                                           Stru_Token_t* rbracket_token)
{
    if (interface == NULL || lbracket_token == NULL) {
        return NULL;
    }
    
    // 创建数组字面量节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_ARRAY_LITERAL, 
                                                     lbracket_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 添加数组元素节点（如果有）
    if (elements != NULL) {
        size_t element_count = F_dynamic_array_length(elements);
        for (size_t i = 0; i < element_count; i++) {
            Stru_AstNode_t* element = *(Stru_AstNode_t**)F_dynamic_array_get(elements, i);
            if (element != NULL) {
                interface->add_child_node(interface, node, element);
            }
        }
    }
    
    // 设置数组字面量属性
    interface->set_node_attribute(interface, node, "array_literal", "[]");
    
    // 设置右括号令牌属性（如果有）
    if (rbracket_token != NULL) {
        interface->set_node_attribute(interface, node, "right_bracket_token", rbracket_token);
    }
    
    return node;
}

/**
 * @brief 创建结构体字面量节点
 */
Stru_AstNode_t* F_create_struct_literal_node(Stru_ParserInterface_t* interface,
                                            Stru_Token_t* lbrace_token,
                                            const char* struct_type_name,
                                            Stru_DynamicArray_t* members,
                                            Stru_Token_t* rbrace_token)
{
    if (interface == NULL || lbrace_token == NULL) {
        return NULL;
    }
    
    // 创建结构体字面量节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_STRUCT_LITERAL, 
                                                     lbrace_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 设置结构体类型名称属性（如果有）
    if (struct_type_name != NULL) {
        interface->set_node_attribute(interface, node, "struct_type_name", (void*)struct_type_name);
    }
    
    // 添加结构体成员节点（如果有）
    if (members != NULL) {
        size_t member_count = F_dynamic_array_length(members);
        for (size_t i = 0; i < member_count; i++) {
            Stru_AstNode_t* member = *(Stru_AstNode_t**)F_dynamic_array_get(members, i);
            if (member != NULL) {
                interface->add_child_node(interface, node, member);
            }
        }
    }
    
    // 设置结构体字面量属性
    interface->set_node_attribute(interface, node, "struct_literal", "{}");
    
    // 设置右花括号令牌属性（如果有）
    if (rbrace_token != NULL) {
        interface->set_node_attribute(interface, node, "right_brace_token", rbrace_token);
    }
    
    return node;
}

/**
 * @brief 创建对象创建表达式节点
 */
Stru_AstNode_t* F_create_new_expression_node(Stru_ParserInterface_t* interface,
                                            Stru_Token_t* new_token,
                                            Stru_Token_t* type_token,
                                            Stru_DynamicArray_t* arguments)
{
    if (interface == NULL || new_token == NULL || type_token == NULL) {
        return NULL;
    }
    
    // 检查是否为"新"关键字
    if (new_token->type != Eum_TOKEN_KEYWORD_NEW) {
        return NULL;
    }
    
    // 检查是否为标识符（类型名称）
    if (type_token->type != Eum_TOKEN_IDENTIFIER) {
        return NULL;
    }
    
    // 创建对象创建表达式节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_NEW_EXPR, 
                                                     new_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 设置类型名称属性
    if (type_token->lexeme != NULL) {
        interface->set_node_attribute(interface, node, "type_name", 
                                     (void*)type_token->lexeme);
    }
    
    // 添加参数节点（如果有）
    if (arguments != NULL) {
        size_t arg_count = F_dynamic_array_length(arguments);
        for (size_t i = 0; i < arg_count; i++) {
            Stru_AstNode_t* arg = *(Stru_AstNode_t**)F_dynamic_array_get(arguments, i);
            if (arg != NULL) {
                interface->add_child_node(interface, node, arg);
            }
        }
    }
    
    // 设置对象创建运算符属性
    interface->set_node_attribute(interface, node, "new_operator", "新");
    
    return node;
}

/**
 * @brief 创建对象销毁表达式节点
 */
Stru_AstNode_t* F_create_delete_expression_node(Stru_ParserInterface_t* interface,
                                               Stru_Token_t* delete_token,
                                               Stru_AstNode_t* object_expression)
{
    if (interface == NULL || delete_token == NULL || object_expression == NULL) {
        return NULL;
    }
    
    // 检查是否为"删除"关键字
    if (delete_token->type != Eum_TOKEN_KEYWORD_DELETE) {
        return NULL;
    }
    
    // 创建对象销毁表达式节点
    Stru_AstNode_t* node = interface->create_ast_node(interface, 
                                                     Eum_AST_DELETE_EXPR, 
                                                     delete_token);
    if (node == NULL) {
        return NULL;
    }
    
    // 添加对象表达式节点
    if (!interface->add_child_node(interface, node, object_expression)) {
        F_destroy_ast_node(node);
        return NULL;
    }
    
    // 设置对象销毁运算符属性
    interface->set_node_attribute(interface, node, "delete_operator", "删除");
    
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
