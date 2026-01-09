/**
 * @file CN_parser_type_utils.c
 * @brief CN_Language 语法分析器类型工具模块实现
 * 
 * 语法分析器类型工具函数模块实现，提供类型检查和操作相关辅助函数。
 * 包括类型兼容性检查、类型名称获取等函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_utils.h"
#include "../../ast/CN_ast.h"
#include <string.h>

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
    
    // 获取节点类型
    Stru_AstNodeInterface_t* interface1 = F_ast_get_internal_interface(type1);
    Stru_AstNodeInterface_t* interface2 = F_ast_get_internal_interface(type2);
    
    if (interface1 == NULL || interface2 == NULL) {
        return false;
    }
    
    // 检查是否为相同类型
    if (interface1->get_type(interface1) == interface2->get_type(interface2)) {
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
    
    // 获取内部接口
    Stru_AstNodeInterface_t* interface = F_ast_get_internal_interface(type_node);
    if (interface == NULL) {
        return "未知类型";
    }
    
    // 根据AST节点类型返回类型名称
    Eum_AstNodeType node_type = interface->get_type(interface);
    
    switch (node_type) {
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
        default: {
            // 尝试从属性中获取类型名称
            const char* type_name = (const char*)F_ast_get_attribute(type_node, "type_name");
            if (type_name != NULL) {
                return type_name;
            }
            return "未知类型";
        }
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
    return (strcmp(type_name, "整数") == 0 || strcmp(type_name, "小数") == 0);
}

/**
 * @brief 检查是否为布尔类型
 */
bool F_is_boolean_type(Stru_AstNode_t* type_node) {
    if (type_node == NULL) {
        return false;
    }
    
    // 检查是否为布尔类型
    const char* type_name = F_get_type_name(type_node);
    return (strcmp(type_name, "布尔") == 0);
}

/**
 * @brief 检查是否为字符串类型
 */
bool F_is_string_type(Stru_AstNode_t* type_node) {
    if (type_node == NULL) {
        return false;
    }
    
    // 检查是否为字符串类型
    const char* type_name = F_get_type_name(type_node);
    return (strcmp(type_name, "字符串") == 0);
}

/**
 * @brief 检查是否为数组类型
 */
bool F_is_array_type(Stru_AstNode_t* type_node) {
    if (type_node == NULL) {
        return false;
    }
    
    // 检查是否为数组类型
    const char* type_name = F_get_type_name(type_node);
    return (strcmp(type_name, "数组类型") == 0);
}

/**
 * @brief 检查是否为指针类型
 */
bool F_is_pointer_type(Stru_AstNode_t* type_node) {
    if (type_node == NULL) {
        return false;
    }
    
    // 检查是否为指针类型
    const char* type_name = F_get_type_name(type_node);
    return (strcmp(type_name, "指针类型") == 0);
}
