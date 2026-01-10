/**
 * @file CN_ast_analyzer.h
 * @brief AST遍历和分析模块头文件
 
 * 本文件定义了AST遍历和分析模块的接口和数据结构，负责实现
 * 主语义分析器的AST遍历和分析功能。遵循SOLID设计原则和分层架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_AST_ANALYZER_H
#define CN_AST_ANALYZER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "../CN_semantic_interface.h"
#include "../../ast/CN_ast_interface.h"

// ============================================================================
// 前向声明
// ============================================================================

typedef struct Stru_AstAnalyzerInterface_t Stru_AstAnalyzerInterface_t;
typedef struct Stru_SemanticAnalyzerInterface_t Stru_SemanticAnalyzerInterface_t;

// ============================================================================
// AST分析上下文结构体
// ============================================================================

/**
 * @brief AST分析上下文结构体
 * 
 * 包含AST分析过程中需要的所有上下文信息。
 */
typedef struct Stru_AstAnalysisContext_t
{
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer;  ///< 语义分析器接口
    Stru_AstNodeInterface_t* current_node;                ///< 当前分析的节点
    size_t depth;                                         ///< 当前分析深度
    void* user_data;                                      ///< 用户自定义数据
} Stru_AstAnalysisContext_t;

// ============================================================================
// AST分析器接口
// ============================================================================

/**
 * @brief AST分析器接口
 * 
 * 提供AST遍历和分析功能，支持不同类型的AST节点分析。
 */
struct Stru_AstAnalyzerInterface_t
{
    /**
     * @brief 初始化AST分析器
     * 
     * @param analyzer AST分析器实例
     * @param semantic_analyzer 语义分析器接口
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_AstAnalyzerInterface_t* analyzer,
                      Stru_SemanticAnalyzerInterface_t* semantic_analyzer);
    
    /**
     * @brief 分析AST节点
     * 
     * @param analyzer AST分析器实例
     * @param ast_node AST根节点
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_ast)(Stru_AstAnalyzerInterface_t* analyzer,
                       Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 分析程序节点
     * 
     * @param analyzer AST分析器实例
     * @param node 程序节点
     * @param context 分析上下文
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_program)(Stru_AstAnalyzerInterface_t* analyzer,
                           Stru_AstNodeInterface_t* node,
                           Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析模块节点
     * 
     * @param analyzer AST分析器实例
     * @param node 模块节点
     * @param context 分析上下文
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_module)(Stru_AstAnalyzerInterface_t* analyzer,
                          Stru_AstNodeInterface_t* node,
                          Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析声明节点
     * 
     * @param analyzer AST分析器实例
     * @param node 声明节点
     * @param context 分析上下文
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_declaration)(Stru_AstAnalyzerInterface_t* analyzer,
                               Stru_AstNodeInterface_t* node,
                               Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析变量声明节点
     * 
     * @param analyzer AST分析器实例
     * @param node 变量声明节点
     * @param context 分析上下文
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_variable_declaration)(Stru_AstAnalyzerInterface_t* analyzer,
                                        Stru_AstNodeInterface_t* node,
                                        Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析函数声明节点
     * 
     * @param analyzer AST分析器实例
     * @param node 函数声明节点
     * @param context 分析上下文
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_function_declaration)(Stru_AstAnalyzerInterface_t* analyzer,
                                        Stru_AstNodeInterface_t* node,
                                        Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析结构体声明节点
     * 
     * @param analyzer AST分析器实例
     * @param node 结构体声明节点
     * @param context 分析上下文
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_struct_declaration)(Stru_AstAnalyzerInterface_t* analyzer,
                                      Stru_AstNodeInterface_t* node,
                                      Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析语句节点
     * 
     * @param analyzer AST分析器实例
     * @param node 语句节点
     * @param context 分析上下文
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_statement)(Stru_AstAnalyzerInterface_t* analyzer,
                             Stru_AstNodeInterface_t* node,
                             Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析表达式语句节点
     * 
     * @param analyzer AST分析器实例
     * @param node 表达式语句节点
     * @param context 分析上下文
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_expression_statement)(Stru_AstAnalyzerInterface_t* analyzer,
                                        Stru_AstNodeInterface_t* node,
                                        Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析if语句节点
     * 
     * @param analyzer AST分析器实例
     * @param node if语句节点
     * @param context 分析上下文
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_if_statement)(Stru_AstAnalyzerInterface_t* analyzer,
                                Stru_AstNodeInterface_t* node,
                                Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析while语句节点
     * 
     * @param analyzer AST分析器实例
     * @param node while语句节点
     * @param context 分析上下文
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_while_statement)(Stru_AstAnalyzerInterface_t* analyzer,
                                   Stru_AstNodeInterface_t* node,
                                   Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析for语句节点
     * 
     * @param analyzer AST分析器实例
     * @param node for语句节点
     * @param context 分析上下文
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_for_statement)(Stru_AstAnalyzerInterface_t* analyzer,
                                 Stru_AstNodeInterface_t* node,
                                 Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析return语句节点
     * 
     * @param analyzer AST分析器实例
     * @param node return语句节点
     * @param context 分析上下文
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_return_statement)(Stru_AstAnalyzerInterface_t* analyzer,
                                    Stru_AstNodeInterface_t* node,
                                    Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析表达式节点
     * 
     * @param analyzer AST分析器实例
     * @param node 表达式节点
     * @param context 分析上下文
     * @return void* 表达式的类型信息，失败返回NULL
     */
    void* (*analyze_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                               Stru_AstNodeInterface_t* node,
                               Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析二元表达式节点
     * 
     * @param analyzer AST分析器实例
     * @param node 二元表达式节点
     * @param context 分析上下文
     * @return void* 表达式的类型信息，失败返回NULL
     */
    void* (*analyze_binary_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                      Stru_AstNodeInterface_t* node,
                                      Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析一元表达式节点
     * 
     * @param analyzer AST分析器实例
     * @param node 一元表达式节点
     * @param context 分析上下文
     * @return void* 表达式的类型信息，失败返回NULL
     */
    void* (*analyze_unary_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                     Stru_AstNodeInterface_t* node,
                                     Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析字面量表达式节点
     * 
     * @param analyzer AST分析器实例
     * @param node 字面量表达式节点
     * @param context 分析上下文
     * @return void* 表达式的类型信息，失败返回NULL
     */
    void* (*analyze_literal_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                       Stru_AstNodeInterface_t* node,
                                       Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析标识符表达式节点
     * 
     * @param analyzer AST分析器实例
     * @param node 标识符表达式节点
     * @param context 分析上下文
     * @return void* 表达式的类型信息，失败返回NULL
     */
    void* (*analyze_identifier_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                          Stru_AstNodeInterface_t* node,
                                          Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析函数调用表达式节点
     * 
     * @param analyzer AST分析器实例
     * @param node 函数调用表达式节点
     * @param context 分析上下文
     * @return void* 表达式的类型信息，失败返回NULL
     */
    void* (*analyze_call_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                    Stru_AstNodeInterface_t* node,
                                    Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析赋值表达式节点
     * 
     * @param analyzer AST分析器实例
     * @param node 赋值表达式节点
     * @param context 分析上下文
     * @return void* 表达式的类型信息，失败返回NULL
     */
    void* (*analyze_assignment_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                          Stru_AstNodeInterface_t* node,
                                          Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析类型转换表达式节点
     * 
     * @param analyzer AST分析器实例
     * @param node 类型转换表达式节点
     * @param context 分析上下文
     * @return void* 表达式的类型信息，失败返回NULL
     */
    void* (*analyze_cast_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                    Stru_AstNodeInterface_t* node,
                                    Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 分析条件表达式节点
     * 
     * @param analyzer AST分析器实例
     * @param node 条件表达式节点
     * @param context 分析上下文
     * @return void* 表达式的类型信息，失败返回NULL
     */
    void* (*analyze_conditional_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                           Stru_AstNodeInterface_t* node,
                                           Stru_AstAnalysisContext_t* context);
    
    /**
     * @brief 设置用户数据
     * 
     * @param analyzer AST分析器实例
     * @param user_data 用户数据
     */
    void (*set_user_data)(Stru_AstAnalyzerInterface_t* analyzer,
                         void* user_data);
    
    /**
     * @brief 获取用户数据
     * 
     * @param analyzer AST分析器实例
     * @return void* 用户数据
     */
    void* (*get_user_data)(Stru_AstAnalyzerInterface_t* analyzer);
    
    /**
     * @brief 重置AST分析器状态
     * 
     * @param analyzer AST分析器实例
     */
    void (*reset)(Stru_AstAnalyzerInterface_t* analyzer);
    
    /**
     * @brief 销毁AST分析器
     * 
     * @param analyzer AST分析器实例
     */
    void (*destroy)(Stru_AstAnalyzerInterface_t* analyzer);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 工厂函数声明
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建AST分析器接口实例
 * 
 * @return Stru_AstAnalyzerInterface_t* AST分析器接口实例，失败返回NULL
 */
Stru_AstAnalyzerInterface_t* F_create_ast_analyzer_interface(void);

/**
 * @brief 销毁AST分析器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_ast_analyzer_interface(Stru_AstAnalyzerInterface_t* interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_AST_ANALYZER_H */
