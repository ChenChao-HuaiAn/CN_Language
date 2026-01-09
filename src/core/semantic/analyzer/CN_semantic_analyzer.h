/**
 * @file CN_semantic_analyzer.h
 * @brief 主语义分析器模块头文件
 
 * 本文件定义了主语义分析器模块的接口和数据结构，负责整合所有语义分析组件，
 * 提供完整的语义分析功能。遵循SOLID设计原则和分层架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_SEMANTIC_ANALYZER_H
#define CN_SEMANTIC_ANALYZER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_SemanticAnalyzerInterface_t Stru_SemanticAnalyzerInterface_t;
typedef struct Stru_SymbolTableInterface_t Stru_SymbolTableInterface_t;
typedef struct Stru_ScopeManagerInterface_t Stru_ScopeManagerInterface_t;
typedef struct Stru_TypeCheckerInterface_t Stru_TypeCheckerInterface_t;
typedef struct Stru_SemanticErrorReporterInterface_t Stru_SemanticErrorReporterInterface_t;

// ============================================================================
// 主语义分析器接口
// ============================================================================

/**
 * @brief 语义分析器接口
 
 * 整合所有语义分析组件，提供完整的语义分析功能。
 */
struct Stru_SemanticAnalyzerInterface_t
{
    /**
     * @brief 初始化语义分析器
     * 
     * @param analyzer 语义分析器实例
     * @param symbol_table 符号表接口
     * @param scope_manager 作用域管理器接口
     * @param type_checker 类型检查器接口
     * @param error_reporter 错误报告器接口
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_SemanticAnalyzerInterface_t* analyzer,
                      Stru_SymbolTableInterface_t* symbol_table,
                      Stru_ScopeManagerInterface_t* scope_manager,
                      Stru_TypeCheckerInterface_t* type_checker,
                      Stru_SemanticErrorReporterInterface_t* error_reporter);
    
    /**
     * @brief 分析AST节点
     * 
     * @param analyzer 语义分析器实例
     * @param ast_node AST根节点
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_ast)(Stru_SemanticAnalyzerInterface_t* analyzer,
                       void* ast_node);
    
    /**
     * @brief 分析单个声明
     * 
     * @param analyzer 语义分析器实例
     * @param declaration_node 声明AST节点
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_declaration)(Stru_SemanticAnalyzerInterface_t* analyzer,
                               void* declaration_node);
    
    /**
     * @brief 分析单个语句
     * 
     * @param analyzer 语义分析器实例
     * @param statement_node 语句AST节点
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_statement)(Stru_SemanticAnalyzerInterface_t* analyzer,
                             void* statement_node);
    
    /**
     * @brief 分析单个表达式
     * 
     * @param analyzer 语义分析器实例
     * @param expression_node 表达式AST节点
     * @return void* 表达式的类型信息，失败返回NULL
     */
    void* (*analyze_expression)(Stru_SemanticAnalyzerInterface_t* analyzer,
                               void* expression_node);
    
    /**
     * @brief 获取符号表接口
     * 
     * @param analyzer 语义分析器实例
     * @return Stru_SymbolTableInterface_t* 符号表接口
     */
    Stru_SymbolTableInterface_t* (*get_symbol_table)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    /**
     * @brief 获取作用域管理器接口
     * 
     * @param analyzer 语义分析器实例
     * @return Stru_ScopeManagerInterface_t* 作用域管理器接口
     */
    Stru_ScopeManagerInterface_t* (*get_scope_manager)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    /**
     * @brief 获取类型检查器接口
     * 
     * @param analyzer 语义分析器实例
     * @return Stru_TypeCheckerInterface_t* 类型检查器接口
     */
    Stru_TypeCheckerInterface_t* (*get_type_checker)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    /**
     * @brief 获取错误报告器接口
     * 
     * @param analyzer 语义分析器实例
     * @return Stru_SemanticErrorReporterInterface_t* 错误报告器接口
     */
    Stru_SemanticErrorReporterInterface_t* (*get_error_reporter)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    /**
     * @brief 重置语义分析器状态
     * 
     * @param analyzer 语义分析器实例
     */
    void (*reset)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    /**
     * @brief 销毁语义分析器
     * 
     * @param analyzer 语义分析器实例
     */
    void (*destroy)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
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
 * @brief 创建语义分析器接口实例
 * 
 * @return Stru_SemanticAnalyzerInterface_t* 语义分析器接口实例，失败返回NULL
 */
Stru_SemanticAnalyzerInterface_t* F_create_semantic_analyzer_interface(void);

/**
 * @brief 销毁语义分析器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_semantic_analyzer_interface(Stru_SemanticAnalyzerInterface_t* interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_SEMANTIC_ANALYZER_H */
