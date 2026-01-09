/**
 * @file CN_parser_syntax_sugar.c
 * @brief CN_Language 语法糖支持模块实现
 * 
 * 语法糖支持模块实现，负责将语法糖转换为标准的语法结构。
 * 包括简写赋值、范围循环、列表推导、空值合并、可选链等语法糖的转换。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_syntax_sugar.h"
#include "../interface/CN_parser_interface_impl.h"
#include "../../ast/CN_ast.h"
#include "../../../infrastructure/memory/CN_memory_interface.h"
#include "../../../infrastructure/utils/CN_utils_interface.h"
#include <string.h>

// ============================================
// 辅助函数声明
// ============================================

/**
 * @brief 获取AST节点的类型
 * 
 * 通过内部接口获取AST节点的类型。
 * 
 * @param node AST节点
 * @return Eum_AstNodeType 节点类型，如果节点为NULL返回Eum_AST_ERROR_NODE
 */
static Eum_AstNodeType get_ast_node_type(const Stru_AstNode_t* node)
{
    if (node == NULL || node->interface == NULL) {
        return Eum_AST_ERROR_NODE;
    }
    
    return node->interface->get_type(node->interface);
}

/**
 * @brief 获取AST节点的运算符类型
 * 
 * 获取AST节点的运算符类型。
 * 
 * @param node AST节点
 * @return int 运算符类型，如果无法获取返回0
 */
static int get_ast_node_operator_type(const Stru_AstNode_t* node)
{
    if (node == NULL) {
        return 0;
    }
    
    return F_ast_get_operator_type(node);
}

/**
 * @brief 获取AST节点的字符串值
 * 
 * 获取AST节点的字符串值（用于标识符、类型名称等）。
 * 
 * @param node AST节点
 * @return const char* 字符串值，如果无法获取返回NULL
 */
static const char* get_ast_node_string_value(const Stru_AstNode_t* node)
{
    if (node == NULL) {
        return NULL;
    }
    
    // 尝试获取标识符
    const char* identifier = F_ast_get_identifier(node);
    if (identifier != NULL) {
        return identifier;
    }
    
    // 尝试获取类型名称
    const char* type_name = F_ast_get_type_name(node);
    if (type_name != NULL) {
        return type_name;
    }
    
    // 尝试获取字符串值
    return F_ast_get_string_value(node);
}

// ============================================
// 语法糖转换函数实现
// ============================================

/**
 * @brief 检测语法糖类型
 * 
 * 检测AST节点是否包含语法糖。
 * 
 * @param node AST节点
 * @return Eum_SyntaxSugarType 语法糖类型
 */
Eum_SyntaxSugarType F_detect_syntax_sugar(Stru_AstNode_t* node)
{
    if (node == NULL) {
        return Eum_SUGAR_NONE;
    }
    
    // 获取节点类型
    Eum_AstNodeType node_type = get_ast_node_type(node);
    
    // 检查节点类型
    switch (node_type) {
        case Eum_AST_COMPOUND_ASSIGN_EXPR:
            return Eum_SUGAR_COMPOUND_ASSIGNMENT;
            
        case Eum_AST_UNARY_EXPR:
            // 检查是否为自增自减
            {
                int operator_type = get_ast_node_operator_type(node);
                // 假设运算符类型1表示++，2表示--
                if (operator_type == 1 || operator_type == 2) {
                    return Eum_SUGAR_INCREMENT_DECREMENT;
                }
            }
            break;
            
        case Eum_AST_FOR_STMT:
            // 检查是否为范围循环
            // 这里需要检查for语句的结构
            // 简化实现：假设有特殊标记
            if (F_ast_has_attribute(node, "range_loop")) {
                return Eum_SUGAR_RANGE_LOOP;
            }
            break;
            
        case Eum_AST_ARRAY_LITERAL:
            // 检查是否为列表推导
            // 简化实现：假设有特殊标记
            if (F_ast_has_attribute(node, "list_comprehension")) {
                return Eum_SUGAR_LIST_COMPREHENSION;
            }
            break;
            
        case Eum_AST_BINARY_EXPR:
            // 检查是否为空值合并操作符
            {
                int operator_type = get_ast_node_operator_type(node);
                // 假设运算符类型100表示空值合并操作符??
                if (operator_type == 100) {
                    return Eum_SUGAR_NULL_COALESCING;
                }
            }
            break;
            
        case Eum_AST_MEMBER_EXPR:
            // 检查是否为可选链
            // 简化实现：假设有特殊标记
            if (F_ast_has_attribute(node, "optional_chaining")) {
                return Eum_SUGAR_OPTIONAL_CHAINING;
            }
            break;
            
        case Eum_AST_CONDITIONAL_EXPR:
            // 检查是否为简写if
            return Eum_SUGAR_SHORTHAND_IF;
            
        case Eum_AST_FUNCTION_DECL:
            // 检查是否为简写函数
            // 简化实现：假设有特殊标记
            if (F_ast_has_attribute(node, "shorthand_function")) {
                return Eum_SUGAR_SHORTHAND_FUNCTION;
            }
            break;
            
        default:
            break;
    }
    
    return Eum_SUGAR_NONE;
}

/**
 * @brief 转换复合赋值语法糖
 * 
 * 将复合赋值语法糖转换为标准赋值表达式。
 * 
 * @param interface 语法分析器接口指针
 * @param node 复合赋值AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_compound_assignment(Stru_ParserInterface_t* interface, Stru_AstNode_t* node)
{
    if (interface == NULL || node == NULL || get_ast_node_type(node) != Eum_AST_COMPOUND_ASSIGN_EXPR) {
        return node;
    }
    
    // 获取AST构建器接口
    Stru_AstBuilderInterface_t* ast_builder = F_ast_create_builder_interface();
    if (ast_builder == NULL) {
        return node;
    }
    
    // 获取左操作数和右操作数
    Stru_AstNode_t* left_operand = NULL;
    Stru_AstNode_t* right_operand = NULL;
    
    // 使用兼容层API获取子节点
    size_t child_count = F_ast_get_child_count(node);
    if (child_count >= 2) {
        left_operand = F_ast_get_child(node, 0);
        right_operand = F_ast_get_child(node, 1);
    }
    
    if (left_operand == NULL || right_operand == NULL) {
        ast_builder->destroy(ast_builder);
        return node;
    }
    
    // 获取操作符类型
    int operator_type = get_ast_node_operator_type(node);
    
    // 根据操作符类型确定对应的二元操作符类型
    int binary_operator_type = 0;
    switch (operator_type) {
        case 10: // +=
            binary_operator_type = 3; // +
            break;
        case 11: // -=
            binary_operator_type = 4; // -
            break;
        case 12: // *=
            binary_operator_type = 5; // *
            break;
        case 13: // /=
            binary_operator_type = 6; // /
            break;
        case 14: // %=
            binary_operator_type = 7; // %
            break;
        default:
            ast_builder->destroy(ast_builder);
            return node;
    }
    
    // 创建二元表达式：左操作数 操作符 右操作数
    // 注意：这里需要创建内部接口节点
    Stru_AstNodeInterface_t* left_interface = F_ast_get_internal_interface(left_operand);
    Stru_AstNodeInterface_t* right_interface = F_ast_get_internal_interface(right_operand);
    
    if (left_interface == NULL || right_interface == NULL) {
        ast_builder->destroy(ast_builder);
        return node;
    }
    
    // 创建位置信息（简化实现）
    Stru_AstNodeLocation_t location = {0};
    
    Stru_AstNodeInterface_t* binary_expr_interface = ast_builder->build_binary_expression(
        ast_builder, &location, binary_operator_type, left_interface, right_interface);
    
    if (binary_expr_interface == NULL) {
        ast_builder->destroy(ast_builder);
        return node;
    }
    
    // 创建赋值表达式：左操作数 = 二元表达式
    Stru_AstNodeInterface_t* assign_expr_interface = ast_builder->build_binary_expression(
        ast_builder, &location, 2, left_interface, binary_expr_interface); // 2表示赋值操作符=
    
    ast_builder->destroy(ast_builder);
    
    if (assign_expr_interface == NULL) {
        return node;
    }
    
    // 将接口节点转换为兼容层节点
    // 简化实现：返回原节点，因为转换需要更复杂的处理
    // 在实际实现中，这里需要创建新的兼容层节点
    return node;
}

/**
 * @brief 转换自增自减语法糖
 * 
 * 将自增自减语法糖转换为标准赋值表达式。
 * 
 * @param interface 语法分析器接口指针
 * @param node 自增自减AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_increment_decrement(Stru_ParserInterface_t* interface, Stru_AstNode_t* node)
{
    if (interface == NULL || node == NULL || get_ast_node_type(node) != Eum_AST_UNARY_EXPR) {
        return node;
    }
    
    // 获取操作符类型
    int operator_type = get_ast_node_operator_type(node);
    
    if (operator_type != 1 && operator_type != 2) { // 1表示++，2表示--
        return node;
    }
    
    // 简化实现：返回原节点
    // 在实际实现中，这里需要将自增自减转换为赋值表达式
    return node;
}

/**
 * @brief 转换范围循环语法糖
 * 
 * 将范围循环语法糖转换为标准for循环。
 * 
 * @param interface 语法分析器接口指针
 * @param node 范围循环AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_range_loop(Stru_ParserInterface_t* interface, Stru_AstNode_t* node)
{
    // 简化实现：返回原节点
    // 在实际实现中，这里会将范围循环转换为标准for循环
    return node;
}

/**
 * @brief 转换列表推导语法糖
 * 
 * 将列表推导语法糖转换为标准循环和列表构建。
 * 
 * @param interface 语法分析器接口指针
 * @param node 列表推导AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_list_comprehension(Stru_ParserInterface_t* interface, Stru_AstNode_t* node)
{
    // 简化实现：返回原节点
    // 在实际实现中，这里会将列表推导转换为标准循环和列表构建
    return node;
}

/**
 * @brief 转换空值合并语法糖
 * 
 * 将空值合并语法糖转换为条件表达式。
 * 
 * @param interface 语法分析器接口指针
 * @param node 空值合并AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_null_coalescing(Stru_ParserInterface_t* interface, Stru_AstNode_t* node)
{
    if (interface == NULL || node == NULL || get_ast_node_type(node) != Eum_AST_BINARY_EXPR) {
        return node;
    }
    
    // 获取操作符类型
    int operator_type = get_ast_node_operator_type(node);
    
    if (operator_type != 100) { // 100表示空值合并操作符??
        return node;
    }
    
    // 简化实现：返回原节点
    // 在实际实现中，这里会将空值合并转换为条件表达式
    return node;
}

/**
 * @brief 转换可选链语法糖
 * 
 * 将可选链语法糖转换为条件成员访问。
 * 
 * @param interface 语法分析器接口指针
 * @param node 可选链AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_optional_chaining(Stru_ParserInterface_t* interface, Stru_AstNode_t* node)
{
    // 简化实现：返回原节点
    // 在实际实现中，这里会将可选链转换为条件成员访问
    return node;
}

/**
 * @brief 转换简写if语法糖
 * 
 * 将简写if语法糖转换为标准条件表达式。
 * 
 * @param interface 语法分析器接口指针
 * @param node 简写ifAST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_shorthand_if(Stru_ParserInterface_t* interface, Stru_AstNode_t* node)
{
    // 简写if已经是条件表达式，不需要转换
    return node;
}

/**
 * @brief 转换简写函数语法糖
 * 
 * 将简写函数语法糖转换为标准函数表达式。
 * 
 * @param interface 语法分析器接口指针
 * @param node 简写函数AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_shorthand_function(Stru_ParserInterface_t* interface, Stru_AstNode_t* node)
{
    // 简化实现：返回原节点
    // 在实际实现中，这里会将简写函数转换为标准函数表达式
    return node;
}

/**
 * @brief 应用语法糖转换
 * 
 * 检测并应用所有语法糖转换。
 * 
 * @param interface 语法分析器接口指针
 * @param node AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_apply_syntax_sugar_transformations(Stru_ParserInterface_t* interface, Stru_AstNode_t* node)
{
    if (interface == NULL || node == NULL) {
        return node;
    }
    
    // 检测语法糖类型
    Eum_SyntaxSugarType sugar_type = F_detect_syntax_sugar(node);
    
    // 根据语法糖类型应用转换
    switch (sugar_type) {
        case Eum_SUGAR_COMPOUND_ASSIGNMENT:
            return F_transform_compound_assignment(interface, node);
            
        case Eum_SUGAR_INCREMENT_DECREMENT:
            return F_transform_increment_decrement(interface, node);
            
        case Eum_SUGAR_RANGE_LOOP:
            return F_transform_range_loop(interface, node);
            
        case Eum_SUGAR_LIST_COMPREHENSION:
            return F_transform_list_comprehension(interface, node);
            
        case Eum_SUGAR_NULL_COALESCING:
            return F_transform_null_coalescing(interface, node);
            
        case Eum_SUGAR_OPTIONAL_CHAINING:
            return F_transform_optional_chaining(interface, node);
            
        case Eum_SUGAR_SHORTHAND_IF:
            return F_transform_shorthand_if(interface, node);
            
        case Eum_SUGAR_SHORTHAND_FUNCTION:
            return F_transform_shorthand_function(interface, node);
            
        case Eum_SUGAR_NONE:
        default:
            return node;
    }
}

/**
 * @brief 递归应用语法糖转换
 * 
 * 递归遍历AST树，应用所有语法糖转换。
 * 
 * @param interface 语法分析器接口指针
 * @param node AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_apply_syntax_sugar_recursive(Stru_ParserInterface_t* interface, Stru_AstNode_t* node)
{
    if (interface == NULL || node == NULL) {
        return node;
    }
    
    // 首先递归处理子节点
    size_t child_count = F_ast_get_child_count(node);
    for (size_t i = 0; i < child_count; i++) {
        Stru_AstNode_t* child = F_ast_get_child(node, i);
        if (child != NULL) {
            Stru_AstNode_t* transformed_child = F_apply_syntax_sugar_recursive(interface, child);
            if (transformed_child != child && transformed_child != NULL) {
                // 移除原子节点，添加转换后的节点
                Stru_AstNode_t* removed_child = F_ast_remove_child(node, i);
                if (removed_child != NULL) {
                    // 添加转换后的节点
                    F_ast_add_child(node, transformed_child);
                    // 注意：由于我们移除了一个节点，索引需要调整
                    // 简化处理：重新获取子节点数量，因为结构可能已改变
                    child_count = F_ast_get_child_count(node);
                    i--; // 重新处理当前索引
                }
            }
        }
    }
    
    // 然后处理当前节点
    return F_apply_syntax_sugar_transformations(interface, node);
}
