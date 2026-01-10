/**
 * @file CN_semantic_factory.c
 * @brief 语义分析工厂模块实现
 
 * 本文件实现了语义分析工厂模块的功能，提供创建完整语义分析系统的工厂函数。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * 注意：工厂模块提供高级工厂函数，直接调用各个子模块的实现函数。
 * 各个组件的创建函数由各自的模块提供。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_semantic_factory.h"
#include "../symbol_table/CN_symbol_table.h"
#include "../symbol_table/CN_symbol_table_optimized.h"
#include "../scope_manager/CN_scope_manager.h"
#include "../type_checker/CN_type_checker.h"
#include "../error_reporter/CN_error_reporter.h"
#include "../analyzer/CN_semantic_analyzer.h"
#include <stdlib.h>

// ============================================================================
// 高级工厂函数实现
// ============================================================================

/**
 * @brief 创建完整的语义分析器系统
 */
Stru_SemanticAnalyzerInterface_t* F_create_complete_semantic_analyzer_system(size_t max_errors)
{
    // 创建所有组件
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    if (symbol_table == NULL)
    {
        return NULL;
    }
    
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    if (scope_manager == NULL)
    {
        F_destroy_symbol_table_interface(symbol_table);
        return NULL;
    }
    
    Stru_TypeCheckerInterface_t* type_checker = F_create_type_checker_interface();
    if (type_checker == NULL)
    {
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        return NULL;
    }
    
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    if (error_reporter == NULL)
    {
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        return NULL;
    }
    
    Stru_SemanticAnalyzerInterface_t* analyzer = F_create_semantic_analyzer_interface();
    if (analyzer == NULL)
    {
        F_destroy_semantic_error_reporter_interface(error_reporter);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        return NULL;
    }
    
    // 初始化所有组件
    if (!symbol_table->initialize(symbol_table, NULL))
    {
        F_destroy_semantic_analyzer_interface(analyzer);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        return NULL;
    }
    
    if (!scope_manager->initialize(scope_manager, symbol_table))
    {
        F_destroy_semantic_analyzer_interface(analyzer);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        return NULL;
    }
    
    if (!type_checker->initialize(type_checker, scope_manager))
    {
        F_destroy_semantic_analyzer_interface(analyzer);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        return NULL;
    }
    
    if (!error_reporter->initialize(error_reporter, max_errors))
    {
        F_destroy_semantic_analyzer_interface(analyzer);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        return NULL;
    }
    
    // 初始化语义分析器
    if (!analyzer->initialize(analyzer, symbol_table, scope_manager, type_checker, error_reporter))
    {
        F_destroy_semantic_analyzer_interface(analyzer);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        return NULL;
    }
    
    return analyzer;
}

/**
 * @brief 创建完整的语义分析器系统（使用优化版符号表）
 */
Stru_SemanticAnalyzerInterface_t* F_create_complete_semantic_analyzer_system_optimized(size_t max_errors)
{
    // 创建所有组件（使用优化版符号表）
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface_optimized();
    if (symbol_table == NULL)
    {
        return NULL;
    }
    
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    if (scope_manager == NULL)
    {
        F_destroy_symbol_table_interface_optimized(symbol_table);
        return NULL;
    }
    
    Stru_TypeCheckerInterface_t* type_checker = F_create_type_checker_interface();
    if (type_checker == NULL)
    {
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface_optimized(symbol_table);
        return NULL;
    }
    
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    if (error_reporter == NULL)
    {
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface_optimized(symbol_table);
        return NULL;
    }
    
    Stru_SemanticAnalyzerInterface_t* analyzer = F_create_semantic_analyzer_interface();
    if (analyzer == NULL)
    {
        F_destroy_semantic_error_reporter_interface(error_reporter);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface_optimized(symbol_table);
        return NULL;
    }
    
    // 初始化所有组件
    if (!symbol_table->initialize(symbol_table, NULL))
    {
        F_destroy_semantic_analyzer_interface(analyzer);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface_optimized(symbol_table);
        return NULL;
    }
    
    if (!scope_manager->initialize(scope_manager, symbol_table))
    {
        F_destroy_semantic_analyzer_interface(analyzer);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface_optimized(symbol_table);
        return NULL;
    }
    
    if (!type_checker->initialize(type_checker, scope_manager))
    {
        F_destroy_semantic_analyzer_interface(analyzer);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface_optimized(symbol_table);
        return NULL;
    }
    
    if (!error_reporter->initialize(error_reporter, max_errors))
    {
        F_destroy_semantic_analyzer_interface(analyzer);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface_optimized(symbol_table);
        return NULL;
    }
    
    // 初始化语义分析器
    if (!analyzer->initialize(analyzer, symbol_table, scope_manager, type_checker, error_reporter))
    {
        F_destroy_semantic_analyzer_interface(analyzer);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface_optimized(symbol_table);
        return NULL;
    }
    
    return analyzer;
}
