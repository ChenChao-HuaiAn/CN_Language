/**
 * @file CN_parser_syntax_sugar.h
 * @brief CN_Language 语法糖支持模块
 * 
 * 语法糖支持模块，负责将语法糖转换为标准的语法结构。
 * 包括简写赋值、范围循环、列表推导、空值合并、可选链等语法糖的转换。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_PARSER_SYNTAX_SUGAR_H
#define CN_PARSER_SYNTAX_SUGAR_H

#include "../CN_parser_interface.h"
#include "../../ast/CN_ast.h"
#include "../../token/CN_token_types.h"

// ============================================
// 语法糖类型枚举
// ============================================

/**
 * @brief 语法糖类型枚举
 * 
 * 定义支持的语法糖类型。
 */
typedef enum Eum_SyntaxSugarType {
    Eum_SUGAR_COMPOUND_ASSIGNMENT,     ///< 复合赋值语法糖 (a += b)
    Eum_SUGAR_INCREMENT_DECREMENT,     ///< 自增自减语法糖 (a++, a--)
    Eum_SUGAR_RANGE_LOOP,              ///< 范围循环语法糖 (循环 i 在 范围(1, 10))
    Eum_SUGAR_LIST_COMPREHENSION,      ///< 列表推导语法糖 ([x*2 对于 x 在 列表])
    Eum_SUGAR_NULL_COALESCING,         ///< 空值合并语法糖 (a ?? b)
    Eum_SUGAR_OPTIONAL_CHAINING,       ///< 可选链语法糖 (a?.b)
    Eum_SUGAR_SHORTHAND_IF,            ///< 简写if语法糖 (条件 ? 值 : 值)
    Eum_SUGAR_SHORTHAND_FUNCTION,      ///< 简写函数语法糖 (参数 => 表达式)
    Eum_SUGAR_NONE                     ///< 无语法糖
} Eum_SyntaxSugarType;

// ============================================
// 语法糖转换函数声明
// ============================================

/**
 * @brief 检测语法糖类型
 * 
 * 检测AST节点是否包含语法糖。
 * 
 * @param node AST节点
 * @return Eum_SyntaxSugarType 语法糖类型
 */
Eum_SyntaxSugarType F_detect_syntax_sugar(Stru_AstNode_t* node);

/**
 * @brief 转换复合赋值语法糖
 * 
 * 将复合赋值语法糖转换为标准赋值表达式。
 * 
 * @param interface 语法分析器接口指针
 * @param node 复合赋值AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_compound_assignment(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);

/**
 * @brief 转换自增自减语法糖
 * 
 * 将自增自减语法糖转换为标准赋值表达式。
 * 
 * @param interface 语法分析器接口指针
 * @param node 自增自减AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_increment_decrement(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);

/**
 * @brief 转换范围循环语法糖
 * 
 * 将范围循环语法糖转换为标准for循环。
 * 
 * @param interface 语法分析器接口指针
 * @param node 范围循环AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_range_loop(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);

/**
 * @brief 转换列表推导语法糖
 * 
 * 将列表推导语法糖转换为标准循环和列表构建。
 * 
 * @param interface 语法分析器接口指针
 * @param node 列表推导AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_list_comprehension(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);

/**
 * @brief 转换空值合并语法糖
 * 
 * 将空值合并语法糖转换为条件表达式。
 * 
 * @param interface 语法分析器接口指针
 * @param node 空值合并AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_null_coalescing(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);

/**
 * @brief 转换可选链语法糖
 * 
 * 将可选链语法糖转换为条件成员访问。
 * 
 * @param interface 语法分析器接口指针
 * @param node 可选链AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_optional_chaining(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);

/**
 * @brief 转换简写if语法糖
 * 
 * 将简写if语法糖转换为标准条件表达式。
 * 
 * @param interface 语法分析器接口指针
 * @param node 简写ifAST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_shorthand_if(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);

/**
 * @brief 转换简写函数语法糖
 * 
 * 将简写函数语法糖转换为标准函数表达式。
 * 
 * @param interface 语法分析器接口指针
 * @param node 简写函数AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_transform_shorthand_function(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);

/**
 * @brief 应用语法糖转换
 * 
 * 检测并应用所有语法糖转换。
 * 
 * @param interface 语法分析器接口指针
 * @param node AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_apply_syntax_sugar_transformations(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);

/**
 * @brief 递归应用语法糖转换
 * 
 * 递归遍历AST树，应用所有语法糖转换。
 * 
 * @param interface 语法分析器接口指针
 * @param node AST节点
 * @return Stru_AstNode_t* 转换后的AST节点
 */
Stru_AstNode_t* F_apply_syntax_sugar_recursive(Stru_ParserInterface_t* interface, Stru_AstNode_t* node);

#endif // CN_PARSER_SYNTAX_SUGAR_H
