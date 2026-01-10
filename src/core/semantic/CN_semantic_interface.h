/**
 * @file CN_semantic_interface.h
 * @brief 语义分析抽象接口定义
 
 * 本文件定义了CN_Language项目的语义分析接口，包括类型检查、符号表管理、
 * 作用域分析、语义错误报告等功能。遵循SOLID设计原则和分层架构，提供
 * 可扩展、可替换的语义分析实现。
 * 
 * 注意：本文件现在只包含各个子模块的头文件，避免重复定义接口。
 * 所有具体的接口定义都在各自的子模块头文件中。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_SEMANTIC_INTERFACE_H
#define CN_SEMANTIC_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 包含各个子模块的头文件
#include "symbol_table/CN_symbol_table.h"
#include "scope_manager/CN_scope_manager.h"
#include "type_checker/CN_type_checker.h"
#include "error_reporter/CN_error_reporter.h"
#include "analyzer/CN_semantic_analyzer.h"
#include "constant_folding/CN_constant_folding.h"
#include "factory/CN_semantic_factory.h"
#include "symbol_attributes/CN_symbol_attributes.h"

// ============================================================================
// 前向声明（为了向后兼容）
// ============================================================================

// 这些前向声明已经包含在子模块头文件中，这里提供是为了向后兼容
typedef struct Stru_SemanticAnalyzerInterface_t Stru_SemanticAnalyzerInterface_t;
typedef struct Stru_TypeCheckerInterface_t Stru_TypeCheckerInterface_t;
typedef struct Stru_SymbolTableInterface_t Stru_SymbolTableInterface_t;
typedef struct Stru_ScopeManagerInterface_t Stru_ScopeManagerInterface_t;
typedef struct Stru_SemanticErrorReporterInterface_t Stru_SemanticErrorReporterInterface_t;
typedef struct Stru_ConstantFoldingInterface_t Stru_ConstantFoldingInterface_t;
typedef struct Stru_ExpressionSimplifierInterface_t Stru_ExpressionSimplifierInterface_t;
typedef struct Stru_ConstantFoldingManagerInterface_t Stru_ConstantFoldingManagerInterface_t;

// ============================================================================
// 工厂函数声明（为了向后兼容）
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建符号表接口实例
 * 
 * @return Stru_SymbolTableInterface_t* 符号表接口实例，失败返回NULL
 */
Stru_SymbolTableInterface_t* F_create_symbol_table_interface(void);

/**
 * @brief 创建作用域管理器接口实例
 * 
 * @return Stru_ScopeManagerInterface_t* 作用域管理器接口实例，失败返回NULL
 */
Stru_ScopeManagerInterface_t* F_create_scope_manager_interface(void);

/**
 * @brief 创建类型检查器接口实例
 * 
 * @return Stru_TypeCheckerInterface_t* 类型检查器接口实例，失败返回NULL
 */
Stru_TypeCheckerInterface_t* F_create_type_checker_interface(void);

/**
 * @brief 创建语义错误报告器接口实例
 * 
 * @return Stru_SemanticErrorReporterInterface_t* 错误报告器接口实例，失败返回NULL
 */
Stru_SemanticErrorReporterInterface_t* F_create_semantic_error_reporter_interface(void);

/**
 * @brief 创建语义分析器接口实例
 * 
 * @return Stru_SemanticAnalyzerInterface_t* 语义分析器接口实例，失败返回NULL
 */
Stru_SemanticAnalyzerInterface_t* F_create_semantic_analyzer_interface(void);

/**
 * @brief 创建常量折叠器接口实例
 * 
 * @return Stru_ConstantFoldingInterface_t* 常量折叠器接口实例，失败返回NULL
 */
Stru_ConstantFoldingInterface_t* F_create_constant_folding_interface(void);

/**
 * @brief 创建表达式简化器接口实例
 * 
 * @return Stru_ExpressionSimplifierInterface_t* 表达式简化器接口实例，失败返回NULL
 */
Stru_ExpressionSimplifierInterface_t* F_create_expression_simplifier_interface(void);

/**
 * @brief 创建常量折叠和表达式简化管理器接口实例
 * 
 * @return Stru_ConstantFoldingManagerInterface_t* 管理器接口实例，失败返回NULL
 */
Stru_ConstantFoldingManagerInterface_t* F_create_constant_folding_manager_interface(void);

/**
 * @brief 销毁符号表接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_symbol_table_interface(Stru_SymbolTableInterface_t* interface);

/**
 * @brief 销毁作用域管理器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_scope_manager_interface(Stru_ScopeManagerInterface_t* interface);

/**
 * @brief 销毁类型检查器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_type_checker_interface(Stru_TypeCheckerInterface_t* interface);

/**
 * @brief 销毁语义错误报告器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_semantic_error_reporter_interface(Stru_SemanticErrorReporterInterface_t* interface);

/**
 * @brief 销毁语义分析器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_semantic_analyzer_interface(Stru_SemanticAnalyzerInterface_t* interface);

/**
 * @brief 销毁常量折叠器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_constant_folding_interface(Stru_ConstantFoldingInterface_t* interface);

/**
 * @brief 销毁表达式简化器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_expression_simplifier_interface(Stru_ExpressionSimplifierInterface_t* interface);

/**
 * @brief 销毁常量折叠和表达式简化管理器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_constant_folding_manager_interface(Stru_ConstantFoldingManagerInterface_t* interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_SEMANTIC_INTERFACE_H */
