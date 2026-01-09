/**
 * @file CN_parser_utils.h
 * @brief CN_Language 语法分析器工具模块
 * 
 * 语法分析器工具函数模块，提供各种辅助函数。
 * 包括令牌处理、AST操作、类型检查等工具函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_PARSER_UTILS_H
#define CN_PARSER_UTILS_H

#include "../CN_parser_interface.h"
#include "../../token/CN_token_types.h"
#include "../../ast/CN_ast.h"

// ============================================
// 令牌工具函数声明
// ============================================

/**
 * @brief 检查令牌是否为关键字
 * 
 * 检查令牌类型是否为关键字。
 * 
 * @param token_type 令牌类型
 * @return bool 如果是关键字返回true，否则返回false
 */
bool F_is_keyword_token(Eum_TokenType token_type);

/**
 * @brief 检查令牌是否为运算符
 * 
 * 检查令牌类型是否为运算符。
 * 
 * @param token_type 令牌类型
 * @return bool 如果是运算符返回true，否则返回false
 */
bool F_is_operator_token(Eum_TokenType token_type);

/**
 * @brief 检查令牌是否为分隔符
 * 
 * 检查令牌类型是否为分隔符。
 * 
 * @param token_type 令牌类型
 * @return bool 如果是分隔符返回true，否则返回false
 */
bool F_is_delimiter_token(Eum_TokenType token_type);

/**
 * @brief 检查令牌是否为字面量
 * 
 * 检查令牌类型是否为字面量。
 * 
 * @param token_type 令牌类型
 * @return bool 如果是字面量返回true，否则返回false
 */
bool F_is_literal_token(Eum_TokenType token_type);

/**
 * @brief 检查令牌是否为类型关键字
 * 
 * 检查令牌类型是否为类型关键字。
 * 
 * @param token_type 令牌类型
 * @return bool 如果是类型关键字返回true，否则返回false
 */
bool F_is_type_keyword(Eum_TokenType token_type);

/**
 * @brief 获取运算符优先级
 * 
 * 获取运算符的优先级。
 * 
 * @param token_type 运算符令牌类型
 * @return int 运算符优先级，值越大优先级越高
 */
int F_get_operator_precedence(Eum_TokenType token_type);

/**
 * @brief 检查运算符是否为右结合
 * 
 * 检查运算符是否为右结合。
 * 
 * @param token_type 运算符令牌类型
 * @return bool 如果是右结合返回true，否则返回false
 */
bool F_is_right_associative(Eum_TokenType token_type);

// ============================================
// AST工具函数声明
// ============================================

/**
 * @brief 创建二元表达式节点
 * 
 * 创建二元表达式AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param operator_token 运算符令牌
 * @param left 左表达式节点
 * @param right 右表达式节点
 * @return Stru_AstNode_t* 二元表达式AST节点
 */
Stru_AstNode_t* F_create_binary_expression_node(Stru_ParserInterface_t* interface,
                                               Stru_Token_t* operator_token,
                                               Stru_AstNode_t* left,
                                               Stru_AstNode_t* right);

/**
 * @brief 创建一元表达式节点
 * 
 * 创建一元表达式AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param operator_token 运算符令牌
 * @param operand 操作数表达式节点
 * @return Stru_AstNode_t* 一元表达式AST节点
 */
Stru_AstNode_t* F_create_unary_expression_node(Stru_ParserInterface_t* interface,
                                              Stru_Token_t* operator_token,
                                              Stru_AstNode_t* operand);

/**
 * @brief 创建复合赋值表达式节点
 * 
 * 创建复合赋值表达式AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param operator_token 复合赋值运算符令牌
 * @param left 左表达式节点
 * @param right 右表达式节点
 * @return Stru_AstNode_t* 复合赋值表达式AST节点
 */
Stru_AstNode_t* F_create_compound_assignment_node(Stru_ParserInterface_t* interface,
                                                 Stru_Token_t* operator_token,
                                                 Stru_AstNode_t* left,
                                                 Stru_AstNode_t* right);

/**
 * @brief 创建字面量节点
 * 
 * 创建字面量AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param token 字面量令牌
 * @return Stru_AstNode_t* 字面量AST节点
 */
Stru_AstNode_t* F_create_literal_node(Stru_ParserInterface_t* interface,
                                     Stru_Token_t* token);

/**
 * @brief 创建标识符节点
 * 
 * 创建标识符AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param token 标识符令牌
 * @return Stru_AstNode_t* 标识符AST节点
 */
Stru_AstNode_t* F_create_identifier_node(Stru_ParserInterface_t* interface,
                                        Stru_Token_t* token);

/**
 * @brief 创建函数调用节点
 * 
 * 创建函数调用AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param function_name 函数名令牌
 * @param arguments 参数表达式数组
 * @return Stru_AstNode_t* 函数调用AST节点
 */
Stru_AstNode_t* F_create_function_call_node(Stru_ParserInterface_t* interface,
                                           Stru_Token_t* function_name,
                                           Stru_DynamicArray_t* arguments);

/**
 * @brief 创建变量声明节点
 * 
 * 创建变量声明AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param keyword_token 关键字令牌（如"变量"）
 * @param identifier_token 标识符令牌
 * @param type_token 类型令牌（可选）
 * @param initializer 初始化表达式节点（可选）
 * @return Stru_AstNode_t* 变量声明AST节点
 */
Stru_AstNode_t* F_create_variable_declaration_node(Stru_ParserInterface_t* interface,
                                                  Stru_Token_t* keyword_token,
                                                  Stru_Token_t* identifier_token,
                                                  Stru_Token_t* type_token,
                                                  Stru_AstNode_t* initializer);

/**
 * @brief 创建函数声明节点
 * 
 * 创建函数声明AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param keyword_token 关键字令牌（如"函数"）
 * @param identifier_token 函数名令牌
 * @param parameters 参数声明数组
 * @param return_type 返回类型节点（可选）
 * @param body 函数体语句节点
 * @return Stru_AstNode_t* 函数声明AST节点
 */
Stru_AstNode_t* F_create_function_declaration_node(Stru_ParserInterface_t* interface,
                                                  Stru_Token_t* keyword_token,
                                                  Stru_Token_t* identifier_token,
                                                  Stru_DynamicArray_t* parameters,
                                                  Stru_AstNode_t* return_type,
                                                  Stru_AstNode_t* body);

/**
 * @brief 创建类型转换表达式节点
 * 
 * 创建类型转换表达式AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param cast_token 类型转换令牌（通常是左括号）
 * @param type_node 目标类型节点
 * @param operand 要转换的表达式节点
 * @return Stru_AstNode_t* 类型转换表达式AST节点
 */
Stru_AstNode_t* F_create_cast_expression_node(Stru_ParserInterface_t* interface,
                                             Stru_Token_t* cast_token,
                                             Stru_AstNode_t* type_node,
                                             Stru_AstNode_t* operand);

/**
 * @brief 创建成员访问表达式节点
 * 
 * 创建成员访问表达式AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param dot_token 点运算符令牌
 * @param object 对象表达式节点
 * @param member_name 成员名称令牌
 * @return Stru_AstNode_t* 成员访问表达式AST节点
 */
Stru_AstNode_t* F_create_member_access_node(Stru_ParserInterface_t* interface,
                                           Stru_Token_t* dot_token,
                                           Stru_AstNode_t* object,
                                           Stru_Token_t* member_name);

/**
 * @brief 创建数组索引表达式节点
 * 
 * 创建数组索引表达式AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param lbracket_token 左方括号令牌
 * @param array 数组表达式节点
 * @param index 索引表达式节点
 * @param rbracket_token 右方括号令牌（可选）
 * @return Stru_AstNode_t* 数组索引表达式AST节点
 */
Stru_AstNode_t* F_create_array_index_node(Stru_ParserInterface_t* interface,
                                         Stru_Token_t* lbracket_token,
                                         Stru_AstNode_t* array,
                                         Stru_AstNode_t* index,
                                         Stru_Token_t* rbracket_token);

/**
 * @brief 创建条件表达式节点（三元运算符）
 * 
 * 创建条件表达式AST节点（三元运算符）。
 * 
 * @param interface 语法分析器接口指针
 * @param question_token 问号运算符令牌
 * @param condition 条件表达式节点
 * @param true_expr 真值表达式节点
 * @param false_expr 假值表达式节点
 * @param colon_token 冒号运算符令牌（可选）
 * @return Stru_AstNode_t* 条件表达式AST节点
 */
Stru_AstNode_t* F_create_conditional_expression_node(Stru_ParserInterface_t* interface,
                                                    Stru_Token_t* question_token,
                                                    Stru_AstNode_t* condition,
                                                    Stru_AstNode_t* true_expr,
                                                    Stru_AstNode_t* false_expr,
                                                    Stru_Token_t* colon_token);

/**
 * @brief 创建对象创建表达式节点
 * 
 * 创建对象创建表达式AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param new_token "新"关键字令牌
 * @param type_token 类型名称令牌
 * @param arguments 参数表达式数组（可选）
 * @return Stru_AstNode_t* 对象创建表达式AST节点
 */
Stru_AstNode_t* F_create_new_expression_node(Stru_ParserInterface_t* interface,
                                            Stru_Token_t* new_token,
                                            Stru_Token_t* type_token,
                                            Stru_DynamicArray_t* arguments);

/**
 * @brief 创建对象销毁表达式节点
 * 
 * 创建对象销毁表达式AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param delete_token "删除"关键字令牌
 * @param object_expression 对象表达式节点
 * @return Stru_AstNode_t* 对象销毁表达式AST节点
 */
Stru_AstNode_t* F_create_delete_expression_node(Stru_ParserInterface_t* interface,
                                               Stru_Token_t* delete_token,
                                               Stru_AstNode_t* object_expression);

/**
 * @brief 创建switch语句节点
 * 
 * 创建switch语句AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param switch_token "选择"关键字令牌
 * @param expression switch表达式节点
 * @param cases case语句节点数组
 * @param case_count case语句数量
 * @param default_case default语句节点（可选）
 * @return Stru_AstNode_t* switch语句AST节点
 */
Stru_AstNode_t* F_create_switch_statement_node(Stru_ParserInterface_t* interface,
                                              Stru_Token_t* switch_token,
                                              Stru_AstNode_t* expression,
                                              Stru_DynamicArray_t* cases,
                                              Stru_AstNode_t* default_case);

/**
 * @brief 创建case语句节点
 * 
 * 创建case语句AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param case_token "情况"关键字令牌
 * @param case_expression case表达式节点
 * @param case_body case体节点
 * @return Stru_AstNode_t* case语句AST节点
 */
Stru_AstNode_t* F_create_case_statement_node(Stru_ParserInterface_t* interface,
                                            Stru_Token_t* case_token,
                                            Stru_AstNode_t* case_expression,
                                            Stru_AstNode_t* case_body);

/**
 * @brief 创建default语句节点
 * 
 * 创建default语句AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param default_token "默认"关键字令牌
 * @param default_body default体节点
 * @return Stru_AstNode_t* default语句AST节点
 */
Stru_AstNode_t* F_create_default_statement_node(Stru_ParserInterface_t* interface,
                                               Stru_Token_t* default_token,
                                               Stru_AstNode_t* default_body);

/**
 * @brief 创建数组字面量节点
 * 
 * 创建数组字面量AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param lbracket_token 左方括号令牌
 * @param elements 数组元素节点数组
 * @param element_count 数组元素数量
 * @param rbracket_token 右方括号令牌（可选）
 * @return Stru_AstNode_t* 数组字面量AST节点
 */
Stru_AstNode_t* F_create_array_literal_node(Stru_ParserInterface_t* interface,
                                           Stru_Token_t* lbracket_token,
                                           Stru_DynamicArray_t* elements,
                                           Stru_Token_t* rbracket_token);

/**
 * @brief 创建结构体字面量节点
 * 
 * 创建结构体字面量AST节点。
 * 
 * @param interface 语法分析器接口指针
 * @param lbrace_token 左花括号令牌
 * @param struct_type_name 结构体类型名称（可选）
 * @param members 结构体成员节点数组
 * @param member_count 结构体成员数量
 * @param rbrace_token 右花括号令牌（可选）
 * @return Stru_AstNode_t* 结构体字面量AST节点
 */
Stru_AstNode_t* F_create_struct_literal_node(Stru_ParserInterface_t* interface,
                                            Stru_Token_t* lbrace_token,
                                            const char* struct_type_name,
                                            Stru_DynamicArray_t* members,
                                            Stru_Token_t* rbrace_token);

// ============================================
// 类型工具函数声明
// ============================================

/**
 * @brief 检查类型兼容性
 * 
 * 检查两种类型是否兼容。
 * 
 * @param type1 类型1
 * @param type2 类型2
 * @return bool 如果类型兼容返回true，否则返回false
 */
bool F_check_type_compatibility(Stru_AstNode_t* type1, Stru_AstNode_t* type2);

/**
 * @brief 获取类型名称
 * 
 * 获取类型节点的类型名称。
 * 
 * @param type_node 类型节点
 * @return const char* 类型名称
 */
const char* F_get_type_name(Stru_AstNode_t* type_node);

/**
 * @brief 检查是否为数值类型
 * 
 * 检查类型是否为数值类型（整数或浮点数）。
 * 
 * @param type_node 类型节点
 * @return bool 如果是数值类型返回true，否则返回false
 */
bool F_is_numeric_type(Stru_AstNode_t* type_node);

/**
 * @brief 检查是否为布尔类型
 * 
 * 检查类型是否为布尔类型。
 * 
 * @param type_node 类型节点
 * @return bool 如果是布尔类型返回true，否则返回false
 */
bool F_is_boolean_type(Stru_AstNode_t* type_node);

/**
 * @brief 检查是否为字符串类型
 * 
 * 检查类型是否为字符串类型。
 * 
 * @param type_node 类型节点
 * @return bool 如果是字符串类型返回true，否则返回false
 */
bool F_is_string_type(Stru_AstNode_t* type_node);

/**
 * @brief 检查是否为数组类型
 * 
 * 检查类型是否为数组类型。
 * 
 * @param type_node 类型节点
 * @return bool 如果是数组类型返回true，否则返回false
 */
bool F_is_array_type(Stru_AstNode_t* type_node);

/**
 * @brief 检查是否为指针类型
 * 
 * 检查类型是否为指针类型。
 * 
 * @param type_node 类型节点
 * @return bool 如果是指针类型返回true，否则返回false
 */
bool F_is_pointer_type(Stru_AstNode_t* type_node);

// ============================================
// 其他工具函数声明
// ============================================

/**
 * @brief 复制字符串
 * 
 * 复制字符串，返回新分配的字符串。
 * 
 * @param str 要复制的字符串
 * @return char* 新分配的字符串
 */
char* F_copy_string(const char* str);

/**
 * @brief 格式化错误信息
 * 
 * 格式化错误信息字符串。
 * 
 * @param format 格式字符串
 * @param ... 可变参数
 * @return char* 格式化后的字符串
 */
char* F_format_error_message(const char* format, ...);

/**
 * @brief 获取当前位置信息
 * 
 * 获取语法分析器当前位置信息。
 * 
 * @param interface 语法分析器接口指针
 * @param line 输出行号
 * @param column 输出列号
 */
void F_get_current_position(Stru_ParserInterface_t* interface, size_t* line, size_t* column);

/**
 * @brief 检查是否到达文件结尾
 * 
 * 检查是否到达文件结尾。
 * 
 * @param interface 语法分析器接口指针
 * @return bool 如果到达文件结尾返回true，否则返回false
 */
bool F_is_at_end_of_file(Stru_ParserInterface_t* interface);

#endif // CN_PARSER_UTILS_H
