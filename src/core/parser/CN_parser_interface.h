/**
 * @file CN_parser_interface.h
 * @brief CN_Language 语法分析器抽象接口
 * 
 * 定义语法分析器的抽象接口，支持语法分析、AST构建和语法错误处理。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 */

#ifndef CN_PARSER_INTERFACE_H
#define CN_PARSER_INTERFACE_H

#include "../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../token/CN_token.h"
#include "../ast/CN_ast.h"

// 前向声明词法分析器接口
typedef struct Stru_LexerInterface_t Stru_LexerInterface_t;

/**
 * @brief 语法分析器抽象接口结构体
 * 
 * 定义语法分析器的完整接口，包含初始化、语法分析、AST构建、错误处理和资源管理功能。
 * 所有函数指针必须由具体实现提供。
 */
typedef struct Stru_ParserInterface_t {
    // ============================================
    // 初始化函数
    // ============================================
    
    /**
     * @brief 初始化语法分析器
     * 
     * 设置词法分析器接口，准备进行语法分析。
     * 
     * @param interface 语法分析器接口指针
     * @param lexer_interface 词法分析器接口指针
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool (*initialize)(struct Stru_ParserInterface_t* interface, Stru_LexerInterface_t* lexer_interface);
    
    // ============================================
    // 核心功能 - 语法分析
    // ============================================
    
    /**
     * @brief 解析程序
     * 
     * 解析整个程序，构建完整的AST。
     * 
     * @param interface 语法分析器接口指针
     * @return Stru_AstNode_t* 程序AST根节点，NULL表示错误
     */
    Stru_AstNode_t* (*parse_program)(struct Stru_ParserInterface_t* interface);
    
    /**
     * @brief 解析语句
     * 
     * 解析单个语句，返回语句AST节点。
     * 
     * @param interface 语法分析器接口指针
     * @return Stru_AstNode_t* 语句AST节点，NULL表示错误或结束
     */
    Stru_AstNode_t* (*parse_statement)(struct Stru_ParserInterface_t* interface);
    
    /**
     * @brief 解析表达式
     * 
     * 解析单个表达式，返回表达式AST节点。
     * 
     * @param interface 语法分析器接口指针
     * @return Stru_AstNode_t* 表达式AST节点，NULL表示错误
     */
    Stru_AstNode_t* (*parse_expression)(struct Stru_ParserInterface_t* interface);
    
    /**
     * @brief 解析声明
     * 
     * 解析变量、函数、结构体或模块声明。
     * 
     * @param interface 语法分析器接口指针
     * @return Stru_AstNode_t* 声明AST节点，NULL表示错误
     */
    Stru_AstNode_t* (*parse_declaration)(struct Stru_ParserInterface_t* interface);
    
    // ============================================
    // 核心功能 - AST构建
    // ============================================
    
    /**
     * @brief 创建AST节点
     * 
     * 创建指定类型的AST节点。
     * 
     * @param interface 语法分析器接口指针
     * @param type AST节点类型
     * @param token 关联的令牌（可选）
     * @return Stru_AstNode_t* 新创建的AST节点，NULL表示错误
     */
    Stru_AstNode_t* (*create_ast_node)(struct Stru_ParserInterface_t* interface, Eum_AstNodeType type, Stru_Token_t* token);
    
    /**
     * @brief 添加子节点
     * 
     * 向父AST节点添加子节点。
     * 
     * @param interface 语法分析器接口指针
     * @param parent 父节点
     * @param child 子节点
     * @return true 添加成功
     * @return false 添加失败
     */
    bool (*add_child_node)(struct Stru_ParserInterface_t* interface, Stru_AstNode_t* parent, Stru_AstNode_t* child);
    
    /**
     * @brief 设置节点属性
     * 
     * 设置AST节点的属性值。
     * 
     * @param interface 语法分析器接口指针
     * @param node AST节点
     * @param key 属性键
     * @param value 属性值
     * @return true 设置成功
     * @return false 设置失败
     */
    bool (*set_node_attribute)(struct Stru_ParserInterface_t* interface, Stru_AstNode_t* node, const char* key, void* value);
    
    /**
     * @brief 获取节点属性
     * 
     * 获取AST节点的属性值。
     * 
     * @param interface 语法分析器接口指针
     * @param node AST节点
     * @param key 属性键
     * @return void* 属性值，NULL表示不存在
     */
    void* (*get_node_attribute)(struct Stru_ParserInterface_t* interface, Stru_AstNode_t* node, const char* key);
    
    // ============================================
    // 错误处理
    // ============================================
    
    /**
     * @brief 报告语法错误
     * 
     * 报告语法分析过程中发现的错误。
     * 
     * @param interface 语法分析器接口指针
     * @param line 错误行号
     * @param column 错误列号
     * @param message 错误信息
     * @param token 出错的令牌（可选）
     */
    void (*report_error)(struct Stru_ParserInterface_t* interface, size_t line, size_t column, const char* message, Stru_Token_t* token);
    
    /**
     * @brief 检查是否有错误
     * 
     * 检查语法分析过程中是否发生了错误。
     * 
     * @param interface 语法分析器接口指针
     * @return true 有错误
     * @return false 没有错误
     */
    bool (*has_errors)(struct Stru_ParserInterface_t* interface);
    
    /**
     * @brief 获取错误列表
     * 
     * 获取所有语法错误的列表。
     * 
     * @param interface 语法分析器接口指针
     * @return Stru_DynamicArray_t* 错误信息数组，NULL表示无错误
     */
    Stru_DynamicArray_t* (*get_errors)(struct Stru_ParserInterface_t* interface);
    
    /**
     * @brief 清除错误
     * 
     * 清除所有已记录的语法错误。
     * 
     * @param interface 语法分析器接口指针
     */
    void (*clear_errors)(struct Stru_ParserInterface_t* interface);
    
    // ============================================
    // 状态查询
    // ============================================
    
    /**
     * @brief 获取当前令牌
     * 
     * 获取语法分析器当前正在处理的令牌。
     * 
     * @param interface 语法分析器接口指针
     * @return Stru_Token_t* 当前令牌，NULL表示结束
     */
    Stru_Token_t* (*get_current_token)(struct Stru_ParserInterface_t* interface);
    
    /**
     * @brief 获取前一个令牌
     * 
     * 获取语法分析器前一个处理的令牌。
     * 
     * @param interface 语法分析器接口指针
     * @return Stru_Token_t* 前一个令牌，NULL表示不存在
     */
    Stru_Token_t* (*get_previous_token)(struct Stru_ParserInterface_t* interface);
    
    /**
     * @brief 获取下一个令牌
     * 
     * 获取语法分析器下一个将要处理的令牌。
     * 
     * @param interface 语法分析器接口指针
     * @return Stru_Token_t* 下一个令牌，NULL表示结束
     */
    Stru_Token_t* (*get_next_token)(struct Stru_ParserInterface_t* interface);
    
    // ============================================
    // 资源管理
    // ============================================
    
    /**
     * @brief 重置语法分析器
     * 
     * 重置语法分析器到初始状态，可以重新分析相同的源代码。
     * 
     * @param interface 语法分析器接口指针
     */
    void (*reset)(struct Stru_ParserInterface_t* interface);
    
    /**
     * @brief 销毁语法分析器
     * 
     * 释放语法分析器占用的所有资源。
     * 
     * @param interface 语法分析器接口指针
     */
    void (*destroy)(struct Stru_ParserInterface_t* interface);
    
    // ============================================
    // 内部状态（不直接暴露）
    // ============================================
    
    /**
     * @brief 内部状态指针
     * 
     * 具体实现可以使用此指针存储内部状态。
     * 接口使用者不应直接访问此字段。
     */
    void* internal_state;
    
} Stru_ParserInterface_t;

// ============================================
// 工厂函数
// ============================================

/**
 * @brief 创建语法分析器接口实例
 * 
 * 创建并返回一个新的语法分析器接口实例。
 * 调用者负责在不再使用时调用destroy函数。
 * 
 * @return Stru_ParserInterface_t* 新创建的语法分析器接口实例
 */
Stru_ParserInterface_t* F_create_parser_interface(void);

#endif // CN_PARSER_INTERFACE_H
