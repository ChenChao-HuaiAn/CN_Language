/**
 * @file CN_parser_declarations.h
 * @brief CN_Language 声明解析模块
 * 
 * 声明解析功能模块，负责解析各种类型的声明。
 * 包括变量声明、函数声明、结构体声明、枚举声明等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_PARSER_DECLARATIONS_H
#define CN_PARSER_DECLARATIONS_H

#include "../CN_parser_interface.h"

// ============================================
// 声明解析函数声明
// ============================================

/**
 * @brief 解析声明
 * 
 * 解析变量、函数、结构体或模块声明。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 声明AST节点，NULL表示错误
 */
Stru_AstNode_t* F_parse_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析变量声明
 * 
 * 解析变量声明，如：变量 名称 = 值;。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 变量声明AST节点
 */
Stru_AstNode_t* F_parse_variable_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析函数声明
 * 
 * 解析函数声明，如：函数 名称(参数) { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 函数声明AST节点
 */
Stru_AstNode_t* F_parse_function_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析结构体声明
 * 
 * 解析结构体声明，如：结构体 名称 { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 结构体声明AST节点
 */
Stru_AstNode_t* F_parse_struct_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析枚举声明
 * 
 * 解析枚举声明，如：枚举 名称 { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 枚举声明AST节点
 */
Stru_AstNode_t* F_parse_enum_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析模块声明
 * 
 * 解析模块声明，如：模块 名称 { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 模块声明AST节点
 */
Stru_AstNode_t* F_parse_module_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析类型声明
 * 
 * 解析类型声明，如：类型 名称 = 类型表达式;。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 类型声明AST节点
 */
Stru_AstNode_t* F_parse_type_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析参数列表
 * 
 * 解析函数声明的参数列表。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_DynamicArray_t* 参数声明数组
 */
Stru_DynamicArray_t* F_parse_parameter_list(Stru_ParserInterface_t* interface);

/**
 * @brief 解析单个参数
 * 
 * 解析函数声明的单个参数。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 参数声明AST节点
 */
Stru_AstNode_t* F_parse_parameter(Stru_ParserInterface_t* interface);

/**
 * @brief 解析类型表达式
 * 
 * 解析类型表达式，如：整数[], 字符串指针。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 类型表达式AST节点
 */
Stru_AstNode_t* F_parse_type_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析基本类型
 * 
 * 解析基本类型，如：整数, 浮点数, 字符串, 布尔值。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 基本类型AST节点
 */
Stru_AstNode_t* F_parse_basic_type(Stru_ParserInterface_t* interface);

/**
 * @brief 解析数组类型
 * 
 * 解析数组类型，如：整数[], 字符串[10]。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 数组类型AST节点
 */
Stru_AstNode_t* F_parse_array_type(Stru_ParserInterface_t* interface);

/**
 * @brief 解析指针类型
 * 
 * 解析指针类型，如：整数指针, 字符串指针。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 指针类型AST节点
 */
Stru_AstNode_t* F_parse_pointer_type(Stru_ParserInterface_t* interface);

/**
 * @brief 解析结构体成员列表
 * 
 * 解析结构体成员列表。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_DynamicArray_t* 结构体成员数组
 */
Stru_DynamicArray_t* F_parse_struct_member_list(Stru_ParserInterface_t* interface);

/**
 * @brief 解析枚举成员列表
 * 
 * 解析枚举成员列表。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_DynamicArray_t* 枚举成员数组
 */
Stru_DynamicArray_t* F_parse_enum_member_list(Stru_ParserInterface_t* interface);

/**
 * @brief 解析接口声明
 * 
 * 解析接口声明，如：接口 名称 { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 接口声明AST节点
 */
Stru_AstNode_t* F_parse_interface_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析类声明
 * 
 * 解析类声明，如：类 名称 { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 类声明AST节点
 */
Stru_AstNode_t* F_parse_class_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析泛型声明
 * 
 * 解析泛型声明，如：泛型 名称<T> { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 泛型声明AST节点
 */
Stru_AstNode_t* F_parse_generic_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析常量声明
 * 
 * 解析常量声明，如：常量 名称 = 值;。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 常量声明AST节点
 */
Stru_AstNode_t* F_parse_constant_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析静态声明
 * 
 * 解析静态变量或静态函数声明。
 * 静态变量声明语法：静态 变量 名称 = 值;
 * 静态函数声明语法：静态 函数 名称(参数) { ... }
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 静态声明AST节点
 */
Stru_AstNode_t* F_parse_static_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析访问修饰符声明
 * 
 * 解析访问修饰符声明，如：公开 变量 名称; 私有 函数 名称() { ... } 保护 结构体 名称 { ... }
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 访问修饰符声明AST节点
 */
Stru_AstNode_t* F_parse_access_modifier_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析虚拟函数声明
 * 
 * 解析虚拟函数声明，如：虚拟 函数 名称(参数) { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 虚拟函数声明AST节点
 */
Stru_AstNode_t* F_parse_virtual_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析抽象函数声明
 * 
 * 解析抽象函数声明，如：抽象 函数 名称(参数);。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 抽象函数声明AST节点
 */
Stru_AstNode_t* F_parse_abstract_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析最终类声明
 * 
 * 解析最终类声明，如：最终 类 名称 { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 最终类声明AST节点
 */
Stru_AstNode_t* F_parse_final_class_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 解析模板声明
 * 
 * 解析模板声明，如：模板 名称<T> { ... }。
 * 模板声明类似于泛型声明，但可能支持更复杂的模板特性。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 模板声明AST节点
 */
Stru_AstNode_t* F_parse_template_declaration(Stru_ParserInterface_t* interface);

#endif // CN_PARSER_DECLARATIONS_H
