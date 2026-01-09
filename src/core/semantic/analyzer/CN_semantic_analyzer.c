/**
 * @file CN_semantic_analyzer.c
 * @brief 主语义分析器模块实现
 
 * 本文件实现了主语义分析器模块的功能，整合所有语义分析组件，
 * 提供完整的语义分析功能。遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_semantic_analyzer.h"
#include "../symbol_table/CN_symbol_table.h"
#include "../scope_manager/CN_scope_manager.h"
#include "../type_checker/CN_type_checker.h"
#include "../error_reporter/CN_error_reporter.h"
#include <stdlib.h>

// ============================================================================
// 语义分析器接口私有数据结构
// ============================================================================

/**
 * @brief 语义分析器接口私有数据
 */
typedef struct Stru_SemanticAnalyzerData_t
{
    Stru_SymbolTableInterface_t* symbol_table;          ///< 符号表接口
    Stru_ScopeManagerInterface_t* scope_manager;        ///< 作用域管理器接口
    Stru_TypeCheckerInterface_t* type_checker;          ///< 类型检查器接口
    Stru_SemanticErrorReporterInterface_t* error_reporter;  ///< 错误报告器接口
} Stru_SemanticAnalyzerData_t;

// ============================================================================
// 语义分析器接口实现函数
// ============================================================================

/**
 * @brief 语义分析器初始化函数
 */
static bool semantic_analyzer_initialize(Stru_SemanticAnalyzerInterface_t* analyzer,
                                        Stru_SymbolTableInterface_t* symbol_table,
                                        Stru_ScopeManagerInterface_t* scope_manager,
                                        Stru_TypeCheckerInterface_t* type_checker,
                                        Stru_SemanticErrorReporterInterface_t* error_reporter)
{
    if (analyzer == NULL || analyzer->private_data != NULL)
    {
        return false;
    }
    
    Stru_SemanticAnalyzerData_t* data = 
        (Stru_SemanticAnalyzerData_t*)malloc(sizeof(Stru_SemanticAnalyzerData_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->symbol_table = symbol_table;
    data->scope_manager = scope_manager;
    data->type_checker = type_checker;
    data->error_reporter = error_reporter;
    
    analyzer->private_data = data;
    return true;
}

/**
 * @brief 分析AST节点
 */
static bool semantic_analyzer_analyze_ast(Stru_SemanticAnalyzerInterface_t* analyzer,
                                         void* ast_node)
{
    (void)analyzer;
    (void)ast_node;
    
    // 桩实现：总是成功
    return true;
}

/**
 * @brief 分析单个声明
 */
static bool semantic_analyzer_analyze_declaration(Stru_SemanticAnalyzerInterface_t* analyzer,
                                                 void* declaration_node)
{
    (void)analyzer;
    (void)declaration_node;
    
    // 桩实现：总是成功
    return true;
}

/**
 * @brief 分析单个语句
 */
static bool semantic_analyzer_analyze_statement(Stru_SemanticAnalyzerInterface_t* analyzer,
                                               void* statement_node)
{
    (void)analyzer;
    (void)statement_node;
    
    // 桩实现：总是成功
    return true;
}

/**
 * @brief 分析单个表达式
 */
static void* semantic_analyzer_analyze_expression(Stru_SemanticAnalyzerInterface_t* analyzer,
                                                 void* expression_node)
{
    (void)analyzer;
    (void)expression_node;
    
    // 桩实现：返回NULL
    return NULL;
}

/**
 * @brief 获取符号表接口
 */
static Stru_SymbolTableInterface_t* semantic_analyzer_get_symbol_table(
    Stru_SemanticAnalyzerInterface_t* analyzer)
{
    if (analyzer == NULL || analyzer->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_SemanticAnalyzerData_t* data = (Stru_SemanticAnalyzerData_t*)analyzer->private_data;
    return data->symbol_table;
}

/**
 * @brief 获取作用域管理器接口
 */
static Stru_ScopeManagerInterface_t* semantic_analyzer_get_scope_manager(
    Stru_SemanticAnalyzerInterface_t* analyzer)
{
    if (analyzer == NULL || analyzer->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_SemanticAnalyzerData_t* data = (Stru_SemanticAnalyzerData_t*)analyzer->private_data;
    return data->scope_manager;
}

/**
 * @brief 获取类型检查器接口
 */
static Stru_TypeCheckerInterface_t* semantic_analyzer_get_type_checker(
    Stru_SemanticAnalyzerInterface_t* analyzer)
{
    if (analyzer == NULL || analyzer->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_SemanticAnalyzerData_t* data = (Stru_SemanticAnalyzerData_t*)analyzer->private_data;
    return data->type_checker;
}

/**
 * @brief 获取错误报告器接口
 */
static Stru_SemanticErrorReporterInterface_t* semantic_analyzer_get_error_reporter(
    Stru_SemanticAnalyzerInterface_t* analyzer)
{
    if (analyzer == NULL || analyzer->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_SemanticAnalyzerData_t* data = (Stru_SemanticAnalyzerData_t*)analyzer->private_data;
    return data->error_reporter;
}

/**
 * @brief 重置语义分析器状态
 */
static void semantic_analyzer_reset(Stru_SemanticAnalyzerInterface_t* analyzer)
{
    if (analyzer == NULL || analyzer->private_data == NULL)
    {
        return;
    }
    
    Stru_SemanticAnalyzerData_t* data = (Stru_SemanticAnalyzerData_t*)analyzer->private_data;
    
    // 重置所有组件
    if (data->symbol_table != NULL)
    {
        data->symbol_table->clear(data->symbol_table);
    }
    
    if (data->scope_manager != NULL)
    {
        // 重置作用域管理器到全局作用域
        while (data->scope_manager->get_scope_depth(data->scope_manager) > 1)
        {
            data->scope_manager->exit_scope(data->scope_manager);
        }
    }
    
    if (data->error_reporter != NULL)
    {
        data->error_reporter->clear_all(data->error_reporter);
    }
}

/**
 * @brief 销毁语义分析器
 */
static void semantic_analyzer_destroy(Stru_SemanticAnalyzerInterface_t* analyzer)
{
    if (analyzer == NULL)
    {
        return;
    }
    
    if (analyzer->private_data != NULL)
    {
        Stru_SemanticAnalyzerData_t* data = (Stru_SemanticAnalyzerData_t*)analyzer->private_data;
        
        // 注意：这里不销毁组件接口，因为它们可能被其他部分共享
        // 组件的销毁由创建者负责
        
        free(data);
        analyzer->private_data = NULL;
    }
}

// ============================================================================
// 工厂函数实现
// ============================================================================

/**
 * @brief 创建语义分析器接口实例
 */
Stru_SemanticAnalyzerInterface_t* F_create_semantic_analyzer_interface(void)
{
    Stru_SemanticAnalyzerInterface_t* interface = 
        (Stru_SemanticAnalyzerInterface_t*)malloc(sizeof(Stru_SemanticAnalyzerInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = semantic_analyzer_initialize;
    interface->analyze_ast = semantic_analyzer_analyze_ast;
    interface->analyze_declaration = semantic_analyzer_analyze_declaration;
    interface->analyze_statement = semantic_analyzer_analyze_statement;
    interface->analyze_expression = semantic_analyzer_analyze_expression;
    interface->get_symbol_table = semantic_analyzer_get_symbol_table;
    interface->get_scope_manager = semantic_analyzer_get_scope_manager;
    interface->get_type_checker = semantic_analyzer_get_type_checker;
    interface->get_error_reporter = semantic_analyzer_get_error_reporter;
    interface->reset = semantic_analyzer_reset;
    interface->destroy = semantic_analyzer_destroy;
    interface->private_data = NULL;
    
    return interface;
}

/**
 * @brief 销毁语义分析器接口实例
 */
void F_destroy_semantic_analyzer_interface(Stru_SemanticAnalyzerInterface_t* interface)
{
    if (interface == NULL)
    {
        return;
    }
    
    if (interface->destroy != NULL)
    {
        interface->destroy(interface);
    }
    
    free(interface);
}
