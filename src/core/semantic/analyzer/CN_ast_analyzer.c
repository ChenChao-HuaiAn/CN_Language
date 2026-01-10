/**
 * @file CN_ast_analyzer.c
 * @brief AST遍历和分析模块实现
 
 * 本文件实现了AST遍历和分析模块的功能，提供完整的AST遍历和分析功能。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_ast_analyzer.h"
#include "../symbol_table/CN_symbol_table.h"
#include "../scope_manager/CN_scope_manager.h"
#include "../type_checker/CN_type_checker.h"
#include "../error_reporter/CN_error_reporter.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// AST分析器私有数据结构
// ============================================================================

/**
 * @brief AST分析器私有数据
 */
typedef struct Stru_AstAnalyzerData_t
{
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer;  ///< 语义分析器接口
    Stru_AstAnalysisContext_t current_context;            ///< 当前分析上下文
    void* user_data;                                      ///< 用户自定义数据
} Stru_AstAnalyzerData_t;

// ============================================================================
// 辅助函数声明
// ============================================================================

static bool analyze_node_by_type(Stru_AstAnalyzerInterface_t* analyzer,
                                Stru_AstNodeInterface_t* node,
                                Stru_AstAnalysisContext_t* context);

static bool analyze_children(Stru_AstAnalyzerInterface_t* analyzer,
                            Stru_AstNodeInterface_t* node,
                            Stru_AstAnalysisContext_t* context);

// ============================================================================
// AST分析器接口实现函数
// ============================================================================

/**
 * @brief AST分析器初始化函数
 */
static bool ast_analyzer_initialize(Stru_AstAnalyzerInterface_t* analyzer,
                                   Stru_SemanticAnalyzerInterface_t* semantic_analyzer)
{
    if (analyzer == NULL || analyzer->private_data != NULL)
    {
        return false;
    }
    
    Stru_AstAnalyzerData_t* data = 
        (Stru_AstAnalyzerData_t*)malloc(sizeof(Stru_AstAnalyzerData_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->semantic_analyzer = semantic_analyzer;
    data->current_context.semantic_analyzer = semantic_analyzer;
    data->current_context.current_node = NULL;
    data->current_context.depth = 0;
    data->current_context.user_data = NULL;
    data->user_data = NULL;
    
    analyzer->private_data = data;
    return true;
}

/**
 * @brief 分析AST节点
 */
static bool ast_analyzer_analyze_ast(Stru_AstAnalyzerInterface_t* analyzer,
                                    Stru_AstNodeInterface_t* ast_node)
{
    if (analyzer == NULL || analyzer->private_data == NULL || ast_node == NULL)
    {
        return false;
    }
    
    Stru_AstAnalyzerData_t* data = (Stru_AstAnalyzerData_t*)analyzer->private_data;
    data->current_context.current_node = ast_node;
    data->current_context.depth = 0;
    
    // 根据节点类型进行分析
    return analyze_node_by_type(analyzer, ast_node, &data->current_context);
}

/**
 * @brief 分析程序节点
 */
static bool ast_analyzer_analyze_program(Stru_AstAnalyzerInterface_t* analyzer,
                                        Stru_AstNodeInterface_t* node,
                                        Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return false;
    }
    
    // 获取语义分析器组件
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer = context->semantic_analyzer;
    if (semantic_analyzer == NULL)
    {
        return false;
    }
    
    // 进入全局作用域
    Stru_ScopeManagerInterface_t* scope_manager = 
        semantic_analyzer->get_scope_manager(semantic_analyzer);
    if (scope_manager != NULL)
    {
        Stru_SymbolTableInterface_t* global_scope = 
            scope_manager->get_global_scope(scope_manager);
        if (global_scope != NULL)
        {
            scope_manager->enter_scope(scope_manager, global_scope);
        }
    }
    
    // 分析所有子节点（声明）
    bool success = analyze_children(analyzer, node, context);
    
    // 退出全局作用域
    if (scope_manager != NULL)
    {
        scope_manager->exit_scope(scope_manager);
    }
    
    return success;
}

/**
 * @brief 分析模块节点
 */
static bool ast_analyzer_analyze_module(Stru_AstAnalyzerInterface_t* analyzer,
                                       Stru_AstNodeInterface_t* node,
                                       Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return false;
    }
    
    // 模块节点分析：分析所有声明
    return analyze_children(analyzer, node, context);
}

/**
 * @brief 分析声明节点
 */
static bool ast_analyzer_analyze_declaration(Stru_AstAnalyzerInterface_t* analyzer,
                                            Stru_AstNodeInterface_t* node,
                                            Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return false;
    }
    
    // 根据声明类型调用相应的分析函数
    Eum_AstNodeType node_type = node->get_type(node);
    
    switch (node_type)
    {
        case Eum_AST_VARIABLE_DECL:
            return analyzer->analyze_variable_declaration(analyzer, node, context);
            
        case Eum_AST_FUNCTION_DECL:
            return analyzer->analyze_function_declaration(analyzer, node, context);
            
        case Eum_AST_STRUCT_DECL:
            return analyzer->analyze_struct_declaration(analyzer, node, context);
            
        default:
            // 对于其他类型的声明，分析其子节点
            return analyze_children(analyzer, node, context);
    }
}

/**
 * @brief 分析变量声明节点
 */
static bool ast_analyzer_analyze_variable_declaration(Stru_AstAnalyzerInterface_t* analyzer,
                                                     Stru_AstNodeInterface_t* node,
                                                     Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return false;
    }
    
    // 获取语义分析器组件
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer = context->semantic_analyzer;
    if (semantic_analyzer == NULL)
    {
        return false;
    }
    
    // 获取变量名称
    const char* variable_name = NULL;
    const Uni_AstNodeData_t* node_data = node->get_data(node);
    if (node_data != NULL)
    {
        variable_name = node_data->identifier;
    }
    
    if (variable_name == NULL)
    {
        // 报告错误：变量名称缺失
        Stru_SemanticErrorReporterInterface_t* error_reporter = 
            semantic_analyzer->get_error_reporter(semantic_analyzer);
        if (error_reporter != NULL)
        {
            Stru_SemanticError_t error;
            error.type = Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL;
            error.message = "变量声明缺少名称";
            error.line = 0;
            error.column = 0;
            error.file_name = NULL;
            error.related_node = node;
            error.extra_data = NULL;
            
            error_reporter->report_error(error_reporter, &error);
        }
        return false;
    }
    
    // 检查变量是否已声明
    Stru_SymbolTableInterface_t* symbol_table = 
        semantic_analyzer->get_symbol_table(semantic_analyzer);
    Stru_ScopeManagerInterface_t* scope_manager = 
        semantic_analyzer->get_scope_manager(semantic_analyzer);
    
    if (symbol_table != NULL && scope_manager != NULL)
    {
        Stru_SymbolTableInterface_t* current_scope = scope_manager->get_current_scope(scope_manager);
        if (current_scope != NULL)
        {
            Stru_SymbolInfo_t* existing_symbol = 
                symbol_table->lookup_symbol(symbol_table, variable_name, true);
            
            if (existing_symbol != NULL)
            {
                // 报告错误：变量重复声明
                Stru_SemanticErrorReporterInterface_t* error_reporter = 
                    semantic_analyzer->get_error_reporter(semantic_analyzer);
                if (error_reporter != NULL)
                {
                    Stru_SemanticError_t error;
                    error.type = Eum_SEMANTIC_ERROR_REDEFINED_SYMBOL;
                    error.message = "变量重复声明";
                    error.line = 0;
                    error.column = 0;
                    error.file_name = NULL;
                    error.related_node = node;
                    error.extra_data = NULL;
                    
                    error_reporter->report_error(error_reporter, &error);
                }
                return false;
            }
        }
    }
    
    // 分析初始化表达式（如果有）
    size_t child_count = node->get_child_count(node);
    if (child_count > 0)
    {
        // 假设最后一个子节点是初始化表达式
        Stru_AstNodeInterface_t* init_expr = node->get_child(node, child_count - 1);
        if (init_expr != NULL)
        {
            void* expr_type = analyzer->analyze_expression(analyzer, init_expr, context);
            if (expr_type == NULL)
            {
                // 表达式分析失败
                return false;
            }
        }
    }
    
    // 分析类型信息（如果有）
    if (child_count > 1)
    {
        // 假设第一个子节点是类型节点
        Stru_AstNodeInterface_t* type_node = node->get_child(node, 0);
        if (type_node != NULL)
        {
            // 这里可以添加类型分析逻辑
            (void)type_node; // 暂时不使用
        }
    }
    
    // 将变量添加到符号表
    if (symbol_table != NULL)
    {
        Stru_SymbolInfo_t symbol_info;
        memset(&symbol_info, 0, sizeof(Stru_SymbolInfo_t));
        symbol_info.name = variable_name;
        symbol_info.type = Eum_SYMBOL_VARIABLE;
        symbol_info.type_info = NULL;
        symbol_info.line = 0;
        symbol_info.column = 0;
        symbol_info.is_exported = false;
        symbol_info.is_initialized = (child_count > 0);
        symbol_info.is_constant = false;
        symbol_info.extra_data = NULL;
        
        bool inserted = symbol_table->insert_symbol(symbol_table, &symbol_info);
        if (!inserted)
        {
            // 报告错误：符号插入失败
            Stru_SemanticErrorReporterInterface_t* error_reporter = 
                semantic_analyzer->get_error_reporter(semantic_analyzer);
            if (error_reporter != NULL)
            {
                Stru_SemanticError_t error;
                error.type = Eum_SEMANTIC_ERROR_REDEFINED_SYMBOL;
                error.message = "无法将变量插入符号表";
                error.line = 0;
                error.column = 0;
                error.file_name = NULL;
                error.related_node = node;
                error.extra_data = NULL;
                
                error_reporter->report_error(error_reporter, &error);
            }
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 分析函数声明节点
 */
static bool ast_analyzer_analyze_function_declaration(Stru_AstAnalyzerInterface_t* analyzer,
                                                     Stru_AstNodeInterface_t* node,
                                                     Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return false;
    }
    
    // 获取语义分析器组件
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer = context->semantic_analyzer;
    if (semantic_analyzer == NULL)
    {
        return false;
    }
    
    // 获取函数名称
    const char* function_name = NULL;
    const Uni_AstNodeData_t* node_data = node->get_data(node);
    if (node_data != NULL)
    {
        function_name = node_data->identifier;
    }
    
    if (function_name == NULL)
    {
        // 报告错误：函数名称缺失
        Stru_SemanticErrorReporterInterface_t* error_reporter = 
            semantic_analyzer->get_error_reporter(semantic_analyzer);
        if (error_reporter != NULL)
        {
            Stru_SemanticError_t error;
            error.type = Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL;
            error.message = "函数声明缺少名称";
            error.line = 0;
            error.column = 0;
            error.file_name = NULL;
            error.related_node = node;
            error.extra_data = NULL;
            
            error_reporter->report_error(error_reporter, &error);
        }
        return false;
    }
    
    // 检查函数是否已声明
    Stru_SymbolTableInterface_t* symbol_table = 
        semantic_analyzer->get_symbol_table(semantic_analyzer);
    Stru_ScopeManagerInterface_t* scope_manager = 
        semantic_analyzer->get_scope_manager(semantic_analyzer);
    
    if (symbol_table != NULL && scope_manager != NULL)
    {
        Stru_SymbolTableInterface_t* current_scope = scope_manager->get_current_scope(scope_manager);
        if (current_scope != NULL)
        {
            Stru_SymbolInfo_t* existing_symbol = 
                symbol_table->lookup_symbol(symbol_table, function_name, true);
            
            if (existing_symbol != NULL)
            {
                // 报告错误：函数重复声明
                Stru_SemanticErrorReporterInterface_t* error_reporter = 
                    semantic_analyzer->get_error_reporter(semantic_analyzer);
                if (error_reporter != NULL)
                {
                    Stru_SemanticError_t error;
                    error.type = Eum_SEMANTIC_ERROR_REDEFINED_SYMBOL;
                    error.message = "函数重复声明";
                    error.line = 0;
                    error.column = 0;
                    error.file_name = NULL;
                    error.related_node = node;
                    error.extra_data = NULL;
                    
                    error_reporter->report_error(error_reporter, &error);
                }
                return false;
            }
        }
    }
    
    // 进入函数作用域
    if (scope_manager != NULL)
    {
        scope_manager->enter_scope(scope_manager, NULL);
    }
    
    // 分析参数和函数体
    bool success = analyze_children(analyzer, node, context);
    
    // 退出函数作用域
    if (scope_manager != NULL)
    {
        scope_manager->exit_scope(scope_manager);
    }
    
    // 将函数添加到符号表
    if (success && symbol_table != NULL)
    {
        Stru_SymbolInfo_t symbol_info;
        memset(&symbol_info, 0, sizeof(Stru_SymbolInfo_t));
        symbol_info.name = function_name;
        symbol_info.type = Eum_SYMBOL_FUNCTION;
        symbol_info.type_info = NULL;
        symbol_info.line = 0;
        symbol_info.column = 0;
        symbol_info.is_exported = false;
        symbol_info.is_initialized = true;
        symbol_info.is_constant = false;
        symbol_info.extra_data = NULL;
        
        bool inserted = symbol_table->insert_symbol(symbol_table, &symbol_info);
        if (!inserted)
        {
            // 报告错误：符号插入失败
            Stru_SemanticErrorReporterInterface_t* error_reporter = 
                semantic_analyzer->get_error_reporter(semantic_analyzer);
            if (error_reporter != NULL)
            {
                Stru_SemanticError_t error;
                error.type = Eum_SEMANTIC_ERROR_REDEFINED_SYMBOL;
                error.message = "无法将函数插入符号表";
                error.line = 0;
                error.column = 0;
                error.file_name = NULL;
                error.related_node = node;
                error.extra_data = NULL;
                
                error_reporter->report_error(error_reporter, &error);
            }
            return false;
        }
    }
    
    return success;
}

/**
 * @brief 分析结构体声明节点
 */
static bool ast_analyzer_analyze_struct_declaration(Stru_AstAnalyzerInterface_t* analyzer,
                                                   Stru_AstNodeInterface_t* node,
                                                   Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return false;
    }
    
    // 获取结构体名称
    const char* struct_name = NULL;
    const Uni_AstNodeData_t* node_data = node->get_data(node);
    if (node_data != NULL)
    {
        struct_name = node_data->identifier;
    }
    
    if (struct_name == NULL)
    {
        // 报告错误：结构体名称缺失
        Stru_SemanticErrorReporterInterface_t* error_reporter = 
            context->semantic_analyzer->get_error_reporter(context->semantic_analyzer);
        if (error_reporter != NULL)
        {
            Stru_SemanticError_t error;
            error.type = Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL;
            error.message = "结构体声明缺少名称";
            error.line = 0;
            error.column = 0;
            error.file_name = NULL;
            error.related_node = node;
            error.extra_data = NULL;
            
            error_reporter->report_error(error_reporter, &error);
        }
        return false;
    }
    
    // 进入结构体作用域
    Stru_ScopeManagerInterface_t* scope_manager = 
        context->semantic_analyzer->get_scope_manager(context->semantic_analyzer);
    if (scope_manager != NULL)
    {
        scope_manager->enter_scope(scope_manager, NULL);
    }
    
    // 分析结构体成员
    bool success = analyze_children(analyzer, node, context);
    
    // 退出结构体作用域
    if (scope_manager != NULL)
    {
        scope_manager->exit_scope(scope_manager);
    }
    
    // 将结构体添加到符号表
    if (success)
    {
        Stru_SymbolTableInterface_t* symbol_table = 
            context->semantic_analyzer->get_symbol_table(context->semantic_analyzer);
        if (symbol_table != NULL)
        {
            Stru_SymbolInfo_t symbol_info;
            memset(&symbol_info, 0, sizeof(Stru_SymbolInfo_t));
            symbol_info.name = struct_name;
            symbol_info.type = Eum_SYMBOL_STRUCT;
            symbol_info.type_info = NULL;
            symbol_info.line = 0;
            symbol_info.column = 0;
            symbol_info.is_exported = false;
            symbol_info.is_initialized = true;
            symbol_info.is_constant = false;
            symbol_info.extra_data = NULL;
            
            bool inserted = symbol_table->insert_symbol(symbol_table, &symbol_info);
            if (!inserted)
            {
                // 报告错误：符号插入失败
                Stru_SemanticErrorReporterInterface_t* error_reporter = 
                    context->semantic_analyzer->get_error_reporter(context->semantic_analyzer);
                if (error_reporter != NULL)
                {
                    Stru_SemanticError_t error;
                    error.type = Eum_SEMANTIC_ERROR_REDEFINED_SYMBOL;
                    error.message = "无法将结构体插入符号表";
                    error.line = 0;
                    error.column = 0;
                    error.file_name = NULL;
                    error.related_node = node;
                    error.extra_data = NULL;
                    
                    error_reporter->report_error(error_reporter, &error);
                }
                return false;
            }
        }
    }
    
    return success;
}

/**
 * @brief 分析语句节点
 */
static bool ast_analyzer_analyze_statement(Stru_AstAnalyzerInterface_t* analyzer,
                                          Stru_AstNodeInterface_t* node,
                                          Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return false;
    }
    
    // 根据语句类型调用相应的分析函数
    Eum_AstNodeType node_type = node->get_type(node);
    
    switch (node_type)
    {
        case Eum_AST_EXPRESSION_STMT:
            return analyzer->analyze_expression_statement(analyzer, node, context);
            
        case Eum_AST_IF_STMT:
            return analyzer->analyze_if_statement(analyzer, node, context);
            
        case Eum_AST_WHILE_STMT:
            return analyzer->analyze_while_statement(analyzer, node, context);
            
        case Eum_AST_FOR_STMT:
            return analyzer->analyze_for_statement(analyzer, node, context);
            
        case Eum_AST_RETURN_STMT:
            return analyzer->analyze_return_statement(analyzer, node, context);
            
        default:
            // 对于其他类型的语句，分析其子节点
            return analyze_children(analyzer, node, context);
    }
}

/**
 * @brief 分析表达式语句节点
 */
static bool ast_analyzer_analyze_expression_statement(Stru_AstAnalyzerInterface_t* analyzer,
                                                     Stru_AstNodeInterface_t* node,
                                                     Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return false;
    }
    
    // 分析表达式
    size_t child_count = node->get_child_count(node);
    if (child_count > 0)
    {
        Stru_AstNodeInterface_t* expr_node = node->get_child(node, 0);
        if (expr_node != NULL)
        {
            void* expr_type = analyzer->analyze_expression(analyzer, expr_node, context);
            if (expr_type == NULL)
            {
                // 表达式分析失败
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief 分析if语句节点
 */
static bool ast_analyzer_analyze_if_statement(Stru_AstAnalyzerInterface_t* analyzer,
                                             Stru_AstNodeInterface_t* node,
                                             Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return false;
    }
    
    // 分析条件表达式
    size_t child_count = node->get_child_count(node);
    if (child_count > 0)
    {
        Stru_AstNodeInterface_t* condition_node = node->get_child(node, 0);
        if (condition_node != NULL)
        {
            void* condition_type = analyzer->analyze_expression(analyzer, condition_node, context);
            if (condition_type == NULL)
            {
                // 条件表达式分析失败
                return false;
            }
        }
    }
    
    // 分析then分支
    if (child_count > 1)
    {
        Stru_AstNodeInterface_t* then_node = node->get_child(node, 1);
        if (then_node != NULL)
        {
            bool then_success = analyzer->analyze_statement(analyzer, then_node, context);
            if (!then_success)
            {
                return false;
            }
        }
    }
    
    // 分析else分支（如果有）
    if (child_count > 2)
    {
        Stru_AstNodeInterface_t* else_node = node->get_child(node, 2);
        if (else_node != NULL)
        {
            bool else_success = analyzer->analyze_statement(analyzer, else_node, context);
            if (!else_success)
            {
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief 分析while语句节点
 */
static bool ast_analyzer_analyze_while_statement(Stru_AstAnalyzerInterface_t* analyzer,
                                                Stru_AstNodeInterface_t* node,
                                                Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return false;
    }
    
    // 分析条件表达式
    size_t child_count = node->get_child_count(node);
    if (child_count > 0)
    {
        Stru_AstNodeInterface_t* condition_node = node->get_child(node, 0);
        if (condition_node != NULL)
        {
            void* condition_type = analyzer->analyze_expression(analyzer, condition_node, context);
            if (condition_type == NULL)
            {
                // 条件表达式分析失败
                return false;
            }
        }
    }
    
    // 分析循环体
    if (child_count > 1)
    {
        Stru_AstNodeInterface_t* body_node = node->get_child(node, 1);
        if (body_node != NULL)
        {
            bool body_success = analyzer->analyze_statement(analyzer, body_node, context);
            if (!body_success)
            {
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief 分析for语句节点
 */
static bool ast_analyzer_analyze_for_statement(Stru_AstAnalyzerInterface_t* analyzer,
                                              Stru_AstNodeInterface_t* node,
                                              Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return false;
    }
    
    // 分析初始化表达式（如果有）
    size_t child_count = node->get_child_count(node);
    if (child_count > 0)
    {
        Stru_AstNodeInterface_t* init_node = node->get_child(node, 0);
        if (init_node != NULL)
        {
            void* init_type = analyzer->analyze_expression(analyzer, init_node, context);
            if (init_type == NULL)
            {
                // 初始化表达式分析失败
                return false;
            }
        }
    }
    
    // 分析条件表达式（如果有）
    if (child_count > 1)
    {
        Stru_AstNodeInterface_t* condition_node = node->get_child(node, 1);
        if (condition_node != NULL)
        {
            void* condition_type = analyzer->analyze_expression(analyzer, condition_node, context);
            if (condition_type == NULL)
            {
                // 条件表达式分析失败
                return false;
            }
        }
    }
    
    // 分析更新表达式（如果有）
    if (child_count > 2)
    {
        Stru_AstNodeInterface_t* update_node = node->get_child(node, 2);
        if (update_node != NULL)
        {
            void* update_type = analyzer->analyze_expression(analyzer, update_node, context);
            if (update_type == NULL)
            {
                // 更新表达式分析失败
                return false;
            }
        }
    }
    
    // 分析循环体
    if (child_count > 3)
    {
        Stru_AstNodeInterface_t* body_node = node->get_child(node, 3);
        if (body_node != NULL)
        {
            bool body_success = analyzer->analyze_statement(analyzer, body_node, context);
            if (!body_success)
            {
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief 分析return语句节点
 */
static bool ast_analyzer_analyze_return_statement(Stru_AstAnalyzerInterface_t* analyzer,
                                                 Stru_AstNodeInterface_t* node,
                                                 Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return false;
    }
    
    // 分析返回表达式（如果有）
    size_t child_count = node->get_child_count(node);
    if (child_count > 0)
    {
        Stru_AstNodeInterface_t* expr_node = node->get_child(node, 0);
        if (expr_node != NULL)
        {
            void* expr_type = analyzer->analyze_expression(analyzer, expr_node, context);
            if (expr_type == NULL)
            {
                // 表达式分析失败
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief 分析表达式节点
 */
static void* ast_analyzer_analyze_expression(Stru_AstAnalyzerInterface_t* analyzer,
                                            Stru_AstNodeInterface_t* node,
                                            Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return NULL;
    }
    
    // 根据表达式类型调用相应的分析函数
    Eum_AstNodeType node_type = node->get_type(node);
    
    switch (node_type)
    {
        case Eum_AST_BINARY_EXPR:
            return analyzer->analyze_binary_expression(analyzer, node, context);
            
        case Eum_AST_UNARY_EXPR:
            return analyzer->analyze_unary_expression(analyzer, node, context);
            
        case Eum_AST_LITERAL_EXPR:
            return analyzer->analyze_literal_expression(analyzer, node, context);
            
        case Eum_AST_IDENTIFIER_EXPR:
            return analyzer->analyze_identifier_expression(analyzer, node, context);
            
        case Eum_AST_CALL_EXPR:
            return analyzer->analyze_call_expression(analyzer, node, context);
            
        case Eum_AST_ASSIGN_EXPR:
            return analyzer->analyze_assignment_expression(analyzer, node, context);
            
        case Eum_AST_CAST_EXPR:
            return analyzer->analyze_cast_expression(analyzer, node, context);
            
        case Eum_AST_CONDITIONAL_EXPR:
            return analyzer->analyze_conditional_expression(analyzer, node, context);
            
        default:
            // 对于其他类型的表达式，返回NULL（暂不支持）
            return NULL;
    }
}

/**
 * @brief 分析二元表达式节点
 */
static void* ast_analyzer_analyze_binary_expression(Stru_AstAnalyzerInterface_t* analyzer,
                                                   Stru_AstNodeInterface_t* node,
                                                   Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return NULL;
    }
    
    // 获取语义分析器组件
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer = context->semantic_analyzer;
    if (semantic_analyzer == NULL)
    {
        return NULL;
    }
    
    // 分析左操作数
    size_t child_count = node->get_child_count(node);
    if (child_count > 0)
    {
        Stru_AstNodeInterface_t* left_node = node->get_child(node, 0);
        if (left_node != NULL)
        {
            void* left_type = analyzer->analyze_expression(analyzer, left_node, context);
            if (left_type == NULL)
            {
                return NULL;
            }
        }
    }
    
    // 分析右操作数
    if (child_count > 1)
    {
        Stru_AstNodeInterface_t* right_node = node->get_child(node, 1);
        if (right_node != NULL)
        {
            void* right_type = analyzer->analyze_expression(analyzer, right_node, context);
            if (right_type == NULL)
            {
                return NULL;
            }
        }
    }
    
    // 这里应该进行类型检查，但暂时返回一个占位符
    return (void*)1;
}

/**
 * @brief 分析一元表达式节点
 */
static void* ast_analyzer_analyze_unary_expression(Stru_AstAnalyzerInterface_t* analyzer,
                                                  Stru_AstNodeInterface_t* node,
                                                  Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return NULL;
    }
    
    // 分析操作数
    size_t child_count = node->get_child_count(node);
    if (child_count > 0)
    {
        Stru_AstNodeInterface_t* operand_node = node->get_child(node, 0);
        if (operand_node != NULL)
        {
            void* operand_type = analyzer->analyze_expression(analyzer, operand_node, context);
            if (operand_type == NULL)
            {
                return NULL;
            }
        }
    }
    
    // 这里应该进行类型检查，但暂时返回一个占位符
    return (void*)1;
}

/**
 * @brief 分析字面量表达式节点
 */
static void* ast_analyzer_analyze_literal_expression(Stru_AstAnalyzerInterface_t* analyzer,
                                                    Stru_AstNodeInterface_t* node,
                                                    Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return NULL;
    }
    
    // 根据字面量类型返回相应的类型信息
    Eum_AstNodeType node_type = node->get_type(node);
    
    switch (node_type)
    {
        case Eum_AST_INT_LITERAL:
            // 返回整数类型
            return (void*)1;
            
        case Eum_AST_FLOAT_LITERAL:
            // 返回浮点数类型
            return (void*)2;
            
        case Eum_AST_STRING_LITERAL:
            // 返回字符串类型
            return (void*)3;
            
        case Eum_AST_BOOL_LITERAL:
            // 返回布尔类型
            return (void*)4;
            
        default:
            return NULL;
    }
}

/**
 * @brief 分析标识符表达式节点
 */
static void* ast_analyzer_analyze_identifier_expression(Stru_AstAnalyzerInterface_t* analyzer,
                                                       Stru_AstNodeInterface_t* node,
                                                       Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return NULL;
    }
    
    // 获取语义分析器组件
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer = context->semantic_analyzer;
    if (semantic_analyzer == NULL)
    {
        return NULL;
    }
    
    // 获取标识符名称
    const char* identifier_name = NULL;
    const Uni_AstNodeData_t* node_data = node->get_data(node);
    if (node_data != NULL)
    {
        identifier_name = node_data->identifier;
    }
    
    if (identifier_name == NULL)
    {
        // 报告错误：标识符名称缺失
        Stru_SemanticErrorReporterInterface_t* error_reporter = 
            semantic_analyzer->get_error_reporter(semantic_analyzer);
        if (error_reporter != NULL)
        {
            Stru_SemanticError_t error;
            error.type = Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL;
            error.message = "标识符缺少名称";
            error.line = 0;
            error.column = 0;
            error.file_name = NULL;
            error.related_node = node;
            error.extra_data = NULL;
            
            error_reporter->report_error(error_reporter, &error);
        }
        return NULL;
    }
    
    // 在符号表中查找标识符
    Stru_SymbolTableInterface_t* symbol_table = 
        semantic_analyzer->get_symbol_table(semantic_analyzer);
    Stru_ScopeManagerInterface_t* scope_manager = 
        semantic_analyzer->get_scope_manager(semantic_analyzer);
    
    if (symbol_table != NULL && scope_manager != NULL)
    {
        Stru_SymbolTableInterface_t* current_scope = scope_manager->get_current_scope(scope_manager);
        if (current_scope != NULL)
        {
            Stru_SymbolInfo_t* symbol_info = 
                symbol_table->lookup_symbol(symbol_table, identifier_name, true);
            
            if (symbol_info == NULL)
            {
                // 报告错误：未定义标识符
                Stru_SemanticErrorReporterInterface_t* error_reporter = 
                    semantic_analyzer->get_error_reporter(semantic_analyzer);
                if (error_reporter != NULL)
                {
                    Stru_SemanticError_t error;
                    error.type = Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL;
                    error.message = "未定义的标识符";
                    error.line = 0;
                    error.column = 0;
                    error.file_name = NULL;
                    error.related_node = node;
                    error.extra_data = NULL;
                    
                    error_reporter->report_error(error_reporter, &error);
                }
                return NULL;
            }
            
            // 返回符号的类型信息
            return symbol_info->type_info;
        }
    }
    
    return NULL;
}

/**
 * @brief 分析函数调用表达式节点
 */
static void* ast_analyzer_analyze_call_expression(Stru_AstAnalyzerInterface_t* analyzer,
                                                 Stru_AstNodeInterface_t* node,
                                                 Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return NULL;
    }
    
    // 获取语义分析器组件
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer = context->semantic_analyzer;
    if (semantic_analyzer == NULL)
    {
        return NULL;
    }
    
    // 分析函数表达式
    size_t child_count = node->get_child_count(node);
    if (child_count == 0)
    {
        // 报告错误：函数调用缺少函数表达式
        Stru_SemanticErrorReporterInterface_t* error_reporter = 
            semantic_analyzer->get_error_reporter(semantic_analyzer);
        if (error_reporter != NULL)
        {
            Stru_SemanticError_t error;
            error.type = Eum_SEMANTIC_ERROR_INVALID_OPERATION;
            error.message = "函数调用缺少函数表达式";
            error.line = 0;
            error.column = 0;
            error.file_name = NULL;
            error.related_node = node;
            error.extra_data = NULL;
            
            error_reporter->report_error(error_reporter, &error);
        }
        return NULL;
    }
    
    Stru_AstNodeInterface_t* function_node = node->get_child(node, 0);
    if (function_node == NULL)
    {
        return NULL;
    }
    
    void* function_type = analyzer->analyze_expression(analyzer, function_node, context);
    if (function_type == NULL)
    {
        return NULL;
    }
    
    // 分析参数
    for (size_t i = 1; i < child_count; i++)
    {
        Stru_AstNodeInterface_t* arg_node = node->get_child(node, i);
        if (arg_node != NULL)
        {
            void* arg_type = analyzer->analyze_expression(analyzer, arg_node, context);
            if (arg_type == NULL)
            {
                // 参数分析失败
                return NULL;
            }
        }
    }
    
    // 这里应该进行函数调用类型检查，但暂时返回一个占位符
    // 实际实现中应该检查函数类型、参数类型匹配等
    return (void*)1;
}

/**
 * @brief 分析赋值表达式节点
 */
static void* ast_analyzer_analyze_assignment_expression(Stru_AstAnalyzerInterface_t* analyzer,
                                                       Stru_AstNodeInterface_t* node,
                                                       Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return NULL;
    }
    
    // 获取语义分析器组件
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer = context->semantic_analyzer;
    if (semantic_analyzer == NULL)
    {
        return NULL;
    }
    
    // 分析左值表达式
    size_t child_count = node->get_child_count(node);
    if (child_count > 0)
    {
        Stru_AstNodeInterface_t* left_node = node->get_child(node, 0);
        if (left_node != NULL)
        {
            void* left_type = analyzer->analyze_expression(analyzer, left_node, context);
            if (left_type == NULL)
            {
                return NULL;
            }
        }
    }
    
    // 分析右值表达式
    if (child_count > 1)
    {
        Stru_AstNodeInterface_t* right_node = node->get_child(node, 1);
        if (right_node != NULL)
        {
            void* right_type = analyzer->analyze_expression(analyzer, right_node, context);
            if (right_type == NULL)
            {
                return NULL;
            }
        }
    }
    
    // 这里应该进行赋值类型检查，但暂时返回一个占位符
    // 实际实现中应该检查左值是否可赋值、类型是否兼容等
    return (void*)1;
}

/**
 * @brief 分析类型转换表达式节点
 */
static void* ast_analyzer_analyze_cast_expression(Stru_AstAnalyzerInterface_t* analyzer,
                                                 Stru_AstNodeInterface_t* node,
                                                 Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return NULL;
    }
    
    // 获取语义分析器组件
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer = context->semantic_analyzer;
    if (semantic_analyzer == NULL)
    {
        return NULL;
    }
    
    // 分析目标类型
    size_t child_count = node->get_child_count(node);
    if (child_count > 0)
    {
        Stru_AstNodeInterface_t* target_type_node = node->get_child(node, 0);
        if (target_type_node != NULL)
        {
            // 这里可以添加目标类型分析逻辑
            (void)target_type_node; // 暂时不使用
        }
    }
    
    // 分析源表达式
    if (child_count > 1)
    {
        Stru_AstNodeInterface_t* source_expr_node = node->get_child(node, 1);
        if (source_expr_node != NULL)
        {
            void* source_type = analyzer->analyze_expression(analyzer, source_expr_node, context);
            if (source_type == NULL)
            {
                return NULL;
            }
        }
    }
    
    // 这里应该进行类型转换检查，但暂时返回一个占位符
    // 实际实现中应该检查类型转换是否合法
    return (void*)1;
}

/**
 * @brief 分析条件表达式节点
 */
static void* ast_analyzer_analyze_conditional_expression(Stru_AstAnalyzerInterface_t* analyzer,
                                                        Stru_AstNodeInterface_t* node,
                                                        Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return NULL;
    }
    
    // 获取语义分析器组件
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer = context->semantic_analyzer;
    if (semantic_analyzer == NULL)
    {
        return NULL;
    }
    
    // 分析条件表达式
    size_t child_count = node->get_child_count(node);
    if (child_count > 0)
    {
        Stru_AstNodeInterface_t* condition_node = node->get_child(node, 0);
        if (condition_node != NULL)
        {
            void* condition_type = analyzer->analyze_expression(analyzer, condition_node, context);
            if (condition_type == NULL)
            {
                return NULL;
            }
        }
    }
    
    // 分析then表达式
    if (child_count > 1)
    {
        Stru_AstNodeInterface_t* then_node = node->get_child(node, 1);
        if (then_node != NULL)
        {
            void* then_type = analyzer->analyze_expression(analyzer, then_node, context);
            if (then_type == NULL)
            {
                return NULL;
            }
        }
    }
    
    // 分析else表达式
    if (child_count > 2)
    {
        Stru_AstNodeInterface_t* else_node = node->get_child(node, 2);
        if (else_node != NULL)
        {
            void* else_type = analyzer->analyze_expression(analyzer, else_node, context);
            if (else_type == NULL)
            {
                return NULL;
            }
        }
    }
    
    // 这里应该进行条件表达式类型检查，但暂时返回一个占位符
    // 实际实现中应该检查条件是否为布尔类型，then和else类型是否兼容
    return (void*)1;
}

/**
 * @brief 根据节点类型分析节点
 */
static bool analyze_node_by_type(Stru_AstAnalyzerInterface_t* analyzer,
                                Stru_AstNodeInterface_t* node,
                                Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return false;
    }
    
    // 更新上下文中的当前节点
    context->current_node = node;
    context->depth++;
    
    // 根据节点类型调用相应的分析函数
    Eum_AstNodeType node_type = node->get_type(node);
    
    switch (node_type)
    {
        case Eum_AST_PROGRAM:
            return analyzer->analyze_program(analyzer, node, context);
            
        case Eum_AST_MODULE:
            return analyzer->analyze_module(analyzer, node, context);
            
        // 声明类型
        case Eum_AST_VARIABLE_DECL:
        case Eum_AST_FUNCTION_DECL:
        case Eum_AST_STRUCT_DECL:
        case Eum_AST_ENUM_DECL:
        case Eum_AST_CONSTANT_DECL:
        case Eum_AST_PARAMETER_DECL:
            return analyzer->analyze_declaration(analyzer, node, context);
            
        // 语句类型
        case Eum_AST_EXPRESSION_STMT:
        case Eum_AST_IF_STMT:
        case Eum_AST_WHILE_STMT:
        case Eum_AST_FOR_STMT:
        case Eum_AST_RETURN_STMT:
        case Eum_AST_BREAK_STMT:
        case Eum_AST_CONTINUE_STMT:
        case Eum_AST_BLOCK_STMT:
        case Eum_AST_SWITCH_STMT:
        case Eum_AST_CASE_STMT:
        case Eum_AST_DEFAULT_STMT:
        case Eum_AST_TRY_STMT:
        case Eum_AST_CATCH_STMT:
        case Eum_AST_THROW_STMT:
        case Eum_AST_FINALLY_STMT:
            return analyzer->analyze_statement(analyzer, node, context);
            
        // 表达式类型
        case Eum_AST_BINARY_EXPR:
        case Eum_AST_UNARY_EXPR:
        case Eum_AST_LITERAL_EXPR:
        case Eum_AST_IDENTIFIER_EXPR:
        case Eum_AST_CALL_EXPR:
        case Eum_AST_INDEX_EXPR:
        case Eum_AST_MEMBER_EXPR:
        case Eum_AST_ASSIGN_EXPR:
        case Eum_AST_COMPOUND_ASSIGN_EXPR:
        case Eum_AST_CAST_EXPR:
        case Eum_AST_CONDITIONAL_EXPR:
        case Eum_AST_NEW_EXPR:
        case Eum_AST_DELETE_EXPR:
            return (analyzer->analyze_expression(analyzer, node, context) != NULL);
            
        default:
            // 对于其他类型的节点，分析其子节点
            return analyze_children(analyzer, node, context);
    }
}

/**
 * @brief 分析节点的所有子节点
 */
static bool analyze_children(Stru_AstAnalyzerInterface_t* analyzer,
                            Stru_AstNodeInterface_t* node,
                            Stru_AstAnalysisContext_t* context)
{
    if (analyzer == NULL || node == NULL || context == NULL)
    {
        return false;
    }
    
    bool success = true;
    size_t child_count = node->get_child_count(node);
    
    for (size_t i = 0; i < child_count; i++)
    {
        Stru_AstNodeInterface_t* child_node = node->get_child(node, i);
        if (child_node != NULL)
        {
            bool child_success = analyze_node_by_type(analyzer, child_node, context);
            if (!child_success)
            {
                success = false;
            }
        }
    }
    
    return success;
}

/**
 * @brief 销毁AST分析器
 */
static void ast_analyzer_destroy(Stru_AstAnalyzerInterface_t* analyzer)
{
    if (analyzer == NULL || analyzer->private_data == NULL)
    {
        return;
    }
    
    Stru_AstAnalyzerData_t* data = (Stru_AstAnalyzerData_t*)analyzer->private_data;
    free(data);
    analyzer->private_data = NULL;
}

// ============================================================================
// 工厂函数实现
// ============================================================================

/**
 * @brief 创建AST分析器接口实例
 */
Stru_AstAnalyzerInterface_t* F_create_ast_analyzer_interface(void)
{
    Stru_AstAnalyzerInterface_t* analyzer = 
        (Stru_AstAnalyzerInterface_t*)malloc(sizeof(Stru_AstAnalyzerInterface_t));
    if (analyzer == NULL)
    {
        return NULL;
    }
    
    // 初始化接口函数指针
    analyzer->initialize = ast_analyzer_initialize;
    analyzer->analyze_ast = ast_analyzer_analyze_ast;
    analyzer->analyze_program = ast_analyzer_analyze_program;
    analyzer->analyze_module = ast_analyzer_analyze_module;
    analyzer->analyze_declaration = ast_analyzer_analyze_declaration;
    analyzer->analyze_variable_declaration = ast_analyzer_analyze_variable_declaration;
    analyzer->analyze_function_declaration = ast_analyzer_analyze_function_declaration;
    analyzer->analyze_struct_declaration = ast_analyzer_analyze_struct_declaration;
    analyzer->analyze_statement = ast_analyzer_analyze_statement;
    analyzer->analyze_expression_statement = ast_analyzer_analyze_expression_statement;
    analyzer->analyze_if_statement = ast_analyzer_analyze_if_statement;
    analyzer->analyze_while_statement = ast_analyzer_analyze_while_statement;
    analyzer->analyze_for_statement = ast_analyzer_analyze_for_statement;
    analyzer->analyze_return_statement = ast_analyzer_analyze_return_statement;
    analyzer->analyze_expression = ast_analyzer_analyze_expression;
    analyzer->analyze_binary_expression = ast_analyzer_analyze_binary_expression;
    analyzer->analyze_unary_expression = ast_analyzer_analyze_unary_expression;
    analyzer->analyze_literal_expression = ast_analyzer_analyze_literal_expression;
    analyzer->analyze_identifier_expression = ast_analyzer_analyze_identifier_expression;
    analyzer->analyze_call_expression = ast_analyzer_analyze_call_expression;
    analyzer->analyze_assignment_expression = ast_analyzer_analyze_assignment_expression;
    analyzer->analyze_cast_expression = ast_analyzer_analyze_cast_expression;
    analyzer->analyze_conditional_expression = ast_analyzer_analyze_conditional_expression;
    analyzer->destroy = ast_analyzer_destroy;
    
    // 初始化私有数据
    analyzer->private_data = NULL;
    
    return analyzer;
}

/**
 * @brief 销毁AST分析器接口实例
 */
void F_destroy_ast_analyzer_interface(Stru_AstAnalyzerInterface_t* interface)
{
    if (interface == NULL)
    {
        return;
    }
    
    // 调用销毁函数
    if (interface->destroy != NULL)
    {
        interface->destroy(interface);
    }
    
    free(interface);
}
