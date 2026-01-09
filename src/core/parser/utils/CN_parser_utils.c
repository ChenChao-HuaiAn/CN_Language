/**
 * @file CN_parser_utils.c
 * @brief CN_Language 语法分析器工具模块实现
 * 
 * 语法分析器工具函数模块实现，提供各种辅助函数。
 * 包括令牌处理、AST操作、类型检查等工具函数。
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
#include <stdarg.h>
#include <string.h>

// ============================================
// 令牌工具函数实现
// ============================================

/**
 * @brief 检查令牌是否为关键字
 */
bool F_is_keyword_token(Eum_TokenType token_type) {
    // 检查是否在关键字范围内
    return (token_type >= Eum_TOKEN_KEYWORD_VAR && 
            token_type <= Eum_TOKEN_KEYWORD_FINALLY);
}

/**
 * @brief 检查令牌是否为运算符
 */
bool F_is_operator_token(Eum_TokenType token_type) {
    // 检查是否为运算符关键字
    if (token_type >= Eum_TOKEN_KEYWORD_ADD && 
        token_type <= Eum_TOKEN_KEYWORD_GREATER_EQUAL) {
        return true;
    }
    
    // 检查是否为符号运算符
    if (token_type >= Eum_TOKEN_OPERATOR_PLUS && 
        token_type <= Eum_TOKEN_OPERATOR_GREATER_EQUAL) {
        return true;
    }
    
    return false;
}

/**
 * @brief 检查令牌是否为分隔符
 */
bool F_is_delimiter_token(Eum_TokenType token_type) {
    return (token_type >= Eum_TOKEN_DELIMITER_COMMA && 
            token_type <= Eum_TOKEN_DELIMITER_RBRACKET);
}

/**
 * @brief 检查令牌是否为字面量
 */
bool F_is_literal_token(Eum_TokenType token_type) {
    return (token_type >= Eum_TOKEN_LITERAL_INTEGER && 
            token_type <= Eum_TOKEN_LITERAL_BOOLEAN) ||
           (token_type == Eum_TOKEN_KEYWORD_TRUE) ||
           (token_type == Eum_TOKEN_KEYWORD_FALSE) ||
           (token_type == Eum_TOKEN_KEYWORD_NULL);
}

/**
 * @brief 检查令牌是否为类型关键字
 */
bool F_is_type_keyword(Eum_TokenType token_type) {
    // 检查是否为类型声明关键字
    if (token_type >= Eum_TOKEN_KEYWORD_INT && 
        token_type <= Eum_TOKEN_KEYWORD_REFERENCE) {
        return true;
    }
    
    // 检查是否为类型关键字
    if (token_type >= Eum_TOKEN_KEYWORD_TYPE && 
        token_type <= Eum_TOKEN_KEYWORD_TEMPLATE) {
        return true;
    }
    
    return false;
}

/**
 * @brief 获取运算符优先级
 */
int F_get_operator_precedence(Eum_TokenType token_type) {
    switch (token_type) {
        // 赋值运算符（优先级1）
        case Eum_TOKEN_OPERATOR_ASSIGN:
            return 1;
            
        // 逻辑或运算符（优先级2）
        case Eum_TOKEN_KEYWORD_OR:
            return 2;
            
        // 逻辑与运算符（优先级3）
        case Eum_TOKEN_KEYWORD_AND:
        case Eum_TOKEN_KEYWORD_AND2:
            return 3;
            
        // 比较运算符（优先级4）
        case Eum_TOKEN_OPERATOR_EQUAL:
        case Eum_TOKEN_OPERATOR_NOT_EQUAL:
        case Eum_TOKEN_KEYWORD_EQUAL:
        case Eum_TOKEN_KEYWORD_NOT_EQUAL:
            return 4;
            
        // 关系运算符（优先级5）
        case Eum_TOKEN_OPERATOR_LESS:
        case Eum_TOKEN_OPERATOR_GREATER:
        case Eum_TOKEN_OPERATOR_LESS_EQUAL:
        case Eum_TOKEN_OPERATOR_GREATER_EQUAL:
        case Eum_TOKEN_KEYWORD_LESS:
        case Eum_TOKEN_KEYWORD_GREATER:
        case Eum_TOKEN_KEYWORD_LESS_EQUAL:
        case Eum_TOKEN_KEYWORD_GREATER_EQUAL:
            return 5;
            
        // 加减运算符（优先级6）
        case Eum_TOKEN_OPERATOR_PLUS:
        case Eum_TOKEN_OPERATOR_MINUS:
        case Eum_TOKEN_KEYWORD_ADD:
        case Eum_TOKEN_KEYWORD_SUBTRACT:
            return 6;
            
        // 乘除模运算符（优先级7）
        case Eum_TOKEN_OPERATOR_MULTIPLY:
        case Eum_TOKEN_OPERATOR_DIVIDE:
        case Eum_TOKEN_OPERATOR_MODULO:
        case Eum_TOKEN_KEYWORD_MULTIPLY:
        case Eum_TOKEN_KEYWORD_DIVIDE:
        case Eum_TOKEN_KEYWORD_MODULO:
            return 7;
            
        // 一元运算符（优先级8）
        case Eum_TOKEN_KEYWORD_NOT:
            return 8;
            
        // 括号、函数调用（优先级9）
        case Eum_TOKEN_DELIMITER_LPAREN:
        case Eum_TOKEN_DELIMITER_LBRACKET:
            return 9;
            
        default:
            return 0; // 不是运算符
    }
}

/**
 * @brief 检查运算符是否为右结合
 */
bool F_is_right_associative(Eum_TokenType token_type) {
    // 赋值运算符是右结合的
    return (token_type == Eum_TOKEN_OPERATOR_ASSIGN);
}

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
        size_t arg_count = F_dynamic_array_size(arguments);
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
        size_t param_count = F_dynamic_array_size(parameters);
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

// ============================================
// 类型工具函数实现
// ============================================

/**
 * @brief 检查类型兼容性
 */
bool F_check_type_compatibility(Stru_AstNode_t* type1, Stru_AstNode_t* type2) {
    if (type1 == NULL || type2 == NULL) {
        return false;
    }
    
    // 简单实现：如果类型节点相同或都是数值类型，则兼容
    // 实际实现需要更复杂的类型系统
    
    // 检查是否为相同类型
    if (type1->type == type2->type) {
        return true;
    }
    
    // 检查是否都是数值类型
    if (F_is_numeric_type(type1) && F_is_numeric_type(type2)) {
        return true;
    }
    
    return false;
}

/**
 * @brief 获取类型名称
 */
const char* F_get_type_name(Stru_AstNode_t* type_node) {
    if (type_node == NULL) {
        return "未知类型";
    }
    
    // 根据AST节点类型返回类型名称
    // 注意：这里使用了兼容层的类型常量，实际应该使用CN_ast_interface.h中的常量
    // 但由于兼容层可能没有定义这些常量，我们使用字符串表示
    switch (type_node->type) {
        case Eum_AST_TYPE_NAME:
            return "类型名称";
        case Eum_AST_ARRAY_TYPE:
            return "数组类型";
        case Eum_AST_POINTER_TYPE:
            return "指针类型";
        case Eum_AST_REFERENCE_TYPE:
            return "引用类型";
        case Eum_AST_FUNCTION_TYPE:
            return "函数类型";
        default:
            // 尝试从属性中获取类型名称
            const char* type_name = (const char*)F_ast_get_attribute(type_node, "type_name");
            if (type_name != NULL) {
                return type_name;
            }
            return "未知类型";
    }
}

/**
 * @brief 检查是否为数值类型
 */
bool F_is_numeric_type(Stru_AstNode_t* type_node) {
    if (type_node == NULL) {
        return false;
    }
    
    // 检查是否为数值类型（整数或浮点数）
    // 通过类型名称判断
    const char* type_name = F_get_type_name(type_node);
    return (strcmp(type_name, "整数") == 0 || strcmp(type_name, "小数")
