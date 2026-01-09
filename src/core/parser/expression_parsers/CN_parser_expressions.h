/**
 * @file CN_parser_expressions.h
 * @brief CN_Language 表达式解析模块
 * 
 * 表达式解析功能模块，负责解析各种类型的表达式。
 * 包括字面量表达式、标识符表达式、二元表达式、一元表达式等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_PARSER_EXPRESSIONS_H
#define CN_PARSER_EXPRESSIONS_H

#include "../CN_parser_interface.h"

// ============================================
// 表达式解析函数声明
// ============================================

/**
 * @brief 解析表达式
 * 
 * 解析单个表达式，返回表达式AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 表达式AST节点，NULL表示错误
 */
Stru_AstNode_t* F_parse_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析赋值表达式
 * 
 * 解析赋值表达式，如：变量 = 值。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 赋值表达式AST节点
 */
Stru_AstNode_t* F_parse_assignment_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析逻辑或表达式
 * 
 * 解析逻辑或表达式，如：条件1 或 条件2。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 逻辑或表达式AST节点
 */
Stru_AstNode_t* F_parse_logical_or_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析逻辑与表达式
 * 
 * 解析逻辑与表达式，如：条件1 且 条件2。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 逻辑与表达式AST节点
 */
Stru_AstNode_t* F_parse_logical_and_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析相等性表达式
 * 
 * 解析相等性表达式，如：a == b, a != b。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 相等性表达式AST节点
 */
Stru_AstNode_t* F_parse_equality_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析关系表达式
 * 
 * 解析关系表达式，如：a < b, a >= b。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 关系表达式AST节点
 */
Stru_AstNode_t* F_parse_relational_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析加法表达式
 * 
 * 解析加法表达式，如：a + b, a - b。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 加法表达式AST节点
 */
Stru_AstNode_t* F_parse_additive_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析乘法表达式
 * 
 * 解析乘法表达式，如：a * b, a / b, a % b。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 乘法表达式AST节点
 */
Stru_AstNode_t* F_parse_multiplicative_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析一元表达式
 * 
 * 解析一元表达式，如：-a, 非 a。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 一元表达式AST节点
 */
Stru_AstNode_t* F_parse_unary_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析基本表达式
 * 
 * 解析基本表达式，包括字面量、标识符、括号表达式等。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 基本表达式AST节点
 */
Stru_AstNode_t* F_parse_primary_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析字面量表达式
 * 
 * 解析字面量表达式，如：42, 3.14, "字符串", 真, 假。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 字面量表达式AST节点
 */
Stru_AstNode_t* F_parse_literal_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析标识符表达式
 * 
 * 解析标识符表达式，如：变量名, 函数名。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 标识符表达式AST节点
 */
Stru_AstNode_t* F_parse_identifier_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析括号表达式
 * 
 * 解析括号表达式，如：(表达式)。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 括号表达式AST节点
 */
Stru_AstNode_t* F_parse_parenthesized_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析函数调用表达式
 * 
 * 解析函数调用表达式，如：函数名(参数1, 参数2)。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 函数调用表达式AST节点
 */
Stru_AstNode_t* F_parse_function_call_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析参数列表
 * 
 * 解析函数调用的参数列表。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_DynamicArray_t* 参数表达式数组
 */
Stru_DynamicArray_t* F_parse_argument_list(Stru_ParserInterface_t* interface);

/**
 * @brief 解析后缀表达式
 * 
 * 解析后缀表达式，包括成员访问和数组索引。
 * 
 * @param interface 语法分析器接口指针
 * @param base 基础表达式节点
 * @return Stru_AstNode_t* 后缀表达式AST节点
 */
Stru_AstNode_t* F_parse_postfix_expression(Stru_ParserInterface_t* interface,
                                          Stru_AstNode_t* base);

/**
 * @brief 解析条件表达式（三元运算符）
 * 
 * 解析条件表达式（三元运算符），如：条件 ? 真值 : 假值。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 条件表达式AST节点
 */
Stru_AstNode_t* F_parse_conditional_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析对象创建表达式
 * 
 * 解析对象创建表达式，如：新 类型()。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 对象创建表达式AST节点
 */
Stru_AstNode_t* F_parse_new_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析对象销毁表达式
 * 
 * 解析对象销毁表达式，如：删除 对象。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 对象销毁表达式AST节点
 */
Stru_AstNode_t* F_parse_delete_expression(Stru_ParserInterface_t* interface);

#endif // CN_PARSER_EXPRESSIONS_H
