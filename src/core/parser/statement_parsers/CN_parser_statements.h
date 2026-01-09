/**
 * @file CN_parser_statements.h
 * @brief CN_Language 语句解析模块
 * 
 * 语句解析功能模块，负责解析各种类型的语句。
 * 包括表达式语句、条件语句、循环语句、控制语句等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_PARSER_STATEMENTS_H
#define CN_PARSER_STATEMENTS_H

#include "../CN_parser_interface.h"

// ============================================
// 语句解析函数声明
// ============================================

/**
 * @brief 解析语句
 * 
 * 解析单个语句，返回语句AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 语句AST节点，NULL表示错误或结束
 */
Stru_AstNode_t* F_parse_statement(Stru_ParserInterface_t* interface);

/**
 * @brief 解析表达式语句
 * 
 * 解析表达式语句，如：表达式;。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 表达式语句AST节点
 */
Stru_AstNode_t* F_parse_expression_statement(Stru_ParserInterface_t* interface);

/**
 * @brief 解析代码块语句
 * 
 * 解析代码块语句，如：{ 语句1; 语句2; }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 代码块语句AST节点
 */
Stru_AstNode_t* F_parse_block_statement(Stru_ParserInterface_t* interface);

/**
 * @brief 解析条件语句
 * 
 * 解析条件语句，如：如果 (条件) { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 条件语句AST节点
 */
Stru_AstNode_t* F_parse_if_statement(Stru_ParserInterface_t* interface);

/**
 * @brief 解析循环语句
 * 
 * 解析循环语句，如：当 (条件) { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 循环语句AST节点
 */
Stru_AstNode_t* F_parse_while_statement(Stru_ParserInterface_t* interface);

/**
 * @brief 解析for循环语句
 * 
 * 解析for循环语句，如：循环 (初始化; 条件; 更新) { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* for循环语句AST节点
 */
Stru_AstNode_t* F_parse_for_statement(Stru_ParserInterface_t* interface);

/**
 * @brief 解析中断语句
 * 
 * 解析中断语句，如：中断;。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 中断语句AST节点
 */
Stru_AstNode_t* F_parse_break_statement(Stru_ParserInterface_t* interface);

/**
 * @brief 解析继续语句
 * 
 * 解析继续语句，如：继续;。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 继续语句AST节点
 */
Stru_AstNode_t* F_parse_continue_statement(Stru_ParserInterface_t* interface);

/**
 * @brief 解析返回语句
 * 
 * 解析返回语句，如：返回 值;。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 返回语句AST节点
 */
Stru_AstNode_t* F_parse_return_statement(Stru_ParserInterface_t* interface);

/**
 * @brief 解析空语句
 * 
 * 解析空语句，如：;。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 空语句AST节点
 */
Stru_AstNode_t* F_parse_empty_statement(Stru_ParserInterface_t* interface);

/**
 * @brief 解析变量声明语句
 * 
 * 解析变量声明语句，如：变量 名称 = 值;。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 变量声明语句AST节点
 */
Stru_AstNode_t* F_parse_variable_declaration_statement(Stru_ParserInterface_t* interface);

/**
 * @brief 解析语句列表
 * 
 * 解析代码块中的语句列表。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_DynamicArray_t* 语句节点数组
 */
Stru_DynamicArray_t* F_parse_statement_list(Stru_ParserInterface_t* interface);

#endif // CN_PARSER_STATEMENTS_H
