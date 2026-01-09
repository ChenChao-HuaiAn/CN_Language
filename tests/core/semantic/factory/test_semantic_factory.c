/**
 * @file test_semantic_factory.c
 * @brief 语义分析工厂模块测试文件
 
 * 本文件包含语义分析工厂模块的单元测试，使用项目自定义测试框架。
 * 测试内容包括工厂函数的创建、销毁、组件集成等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../../../src/core/semantic/factory/CN_semantic_factory.h"
#include "../../../../src/core/semantic/symbol_table/CN_symbol_table.h"
#include "../../../../src/core/semantic/scope_manager/CN_scope_manager.h"
#include "../../../../src/core/semantic/type_checker/CN_type_checker.h"
#include "../../../../src/core/semantic/error_reporter/CN_error_reporter.h"
#include "../../../../src/core/semantic/analyzer/CN_semantic_analyzer.h"

// ============================================================================
// 测试辅助函数
// ============================================================================

/**
 * @brief 打印测试结果
 */
static void print_test_result(const char* test_name, bool passed)
{
    printf("  %-40s %s\n", test_name, passed ? "[PASS]" : "[FAIL]");
}

// ============================================================================
// 测试用例
// ============================================================================

/**
 * @brief 测试符号表工厂函数
 */
static bool test_symbol_table_factory(void)
{
    printf("测试符号表工厂函数:\n");
    
    // 测试创建符号表
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    bool test1_passed = (symbol_table != NULL);
    print_test_result("创建符号表", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试接口函数指针
    bool test2_passed = (symbol_table->initialize != NULL &&
                        symbol_table->insert_symbol != NULL &&
                        symbol_table->lookup_symbol != NULL &&
                        symbol_table->remove_symbol != NULL &&
                        symbol_table->get_symbol_count != NULL &&
                        symbol_table->get_all_symbols != NULL &&
                        symbol_table->clear != NULL &&
                        symbol_table->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试初始化
    bool init_result = symbol_table->initialize(symbol_table, NULL);
    bool test3_passed = init_result;
    print_test_result("初始化", test3_passed);
    
    if (!test3_passed) {
        F_destroy_symbol_table_interface(symbol_table);
        return false;
    }
    
    // 测试基本功能
    Stru_SymbolInfo_t symbol_info;
    symbol_info.name = "test_var";
    symbol_info.type = Eum_SYMBOL_VARIABLE;
    symbol_info.type_info = NULL;
    symbol_info.line = 10;
    symbol_info.column = 5;
    symbol_info.is_exported = false;
    symbol_info.is_initialized = false;
    symbol_info.is_constant = false;
    symbol_info.extra_data = NULL;
    
    bool insert_result = symbol_table->insert_symbol(symbol_table, &symbol_info);
    bool test4_passed = insert_result;
    print_test_result("插入符号", test4_passed);
    
    size_t symbol_count = 0;
    if (insert_result) {
        symbol_count = symbol_table->get_symbol_count(symbol_table);
        bool test5_passed = (symbol_count == 1);
        print_test_result("获取符号数量", test5_passed);
    } else {
        bool test5_passed = false;
        print_test_result("获取符号数量（跳过）", test5_passed);
    }
    
    // 测试销毁
    F_destroy_symbol_table_interface(symbol_table);
    bool test6_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁符号表", test6_passed);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && test6_passed;
}

/**
 * @brief 测试作用域管理器工厂函数
 */
static bool test_scope_manager_factory(void)
{
    printf("\n测试作用域管理器工厂函数:\n");
    
    // 测试创建作用域管理器
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    bool test1_passed = (scope_manager != NULL);
    print_test_result("创建作用域管理器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试接口函数指针
    bool test2_passed = (scope_manager->initialize != NULL &&
                        scope_manager->create_scope != NULL &&
                        scope_manager->enter_scope != NULL &&
                        scope_manager->exit_scope != NULL &&
                        scope_manager->get_current_scope != NULL &&
                        scope_manager->get_global_scope != NULL &&
                        scope_manager->get_scope_depth != NULL &&
                        scope_manager->lookup_symbol_in_scope_chain != NULL &&
                        scope_manager->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 创建符号表用于初始化
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    bool test3_passed = (symbol_table != NULL);
    print_test_result("创建符号表用于初始化", test3_passed);
    
    if (!test3_passed) {
        F_destroy_scope_manager_interface(scope_manager);
        return false;
    }
    
    bool symbol_init = symbol_table->initialize(symbol_table, NULL);
    bool test4_passed = symbol_init;
    print_test_result("符号表初始化", test4_passed);
    
    if (!test4_passed) {
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_scope_manager_interface(scope_manager);
        return false;
    }
    
    // 测试初始化
    bool init_result = scope_manager->initialize(scope_manager, symbol_table);
    bool test5_passed = init_result;
    print_test_result("作用域管理器初始化", test5_passed);
    
    if (!test5_passed) {
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_scope_manager_interface(scope_manager);
        return false;
    }
    
    // 测试基本功能
    Stru_SymbolTableInterface_t* new_scope = 
        scope_manager->create_scope(scope_manager, Eum_SCOPE_FUNCTION, "test_func");
    bool test6_passed = (new_scope != NULL);
    print_test_result("创建新作用域", test6_passed);
    
    // 清理
    if (new_scope != NULL) {
        F_destroy_symbol_table_interface(new_scope);
    }
    F_destroy_scope_manager_interface(scope_manager);
    F_destroy_symbol_table_interface(symbol_table);
    
    bool test7_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("清理资源", test7_passed);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && 
           test5_passed && test6_passed && test7_passed;
}

/**
 * @brief 测试类型检查器工厂函数
 */
static bool test_type_checker_factory(void)
{
    printf("\n测试类型检查器工厂函数:\n");
    
    // 测试创建类型检查器
    Stru_TypeCheckerInterface_t* type_checker = F_create_type_checker_interface();
    bool test1_passed = (type_checker != NULL);
    print_test_result("创建类型检查器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试接口函数指针
    bool test2_passed = (type_checker->initialize != NULL &&
                        type_checker->check_binary_expression != NULL &&
                        type_checker->check_unary_expression != NULL &&
                        type_checker->check_assignment != NULL &&
                        type_checker->check_function_call != NULL &&
                        type_checker->check_type_cast != NULL &&
                        type_checker->infer_expression_type != NULL &&
                        type_checker->check_variable_declaration != NULL &&
                        type_checker->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 创建作用域管理器用于初始化
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    bool test3_passed = (scope_manager != NULL);
    print_test_result("创建作用域管理器", test3_passed);
    
    if (!test3_passed) {
        F_destroy_type_checker_interface(type_checker);
        return false;
    }
    
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    bool test4_passed = (symbol_table != NULL);
    print_test_result("创建符号表", test4_passed);
    
    if (!test4_passed) {
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_type_checker_interface(type_checker);
        return false;
    }
    
    bool symbol_init = symbol_table->initialize(symbol_table, NULL);
    bool test5_passed = symbol_init;
    print_test_result("符号表初始化", test5_passed);
    
    if (!test5_passed) {
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_type_checker_interface(type_checker);
        return false;
    }
    
    bool scope_init = scope_manager->initialize(scope_manager, symbol_table);
    bool test6_passed = scope_init;
    print_test_result("作用域管理器初始化", test6_passed);
    
    if (!test6_passed) {
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_type_checker_interface(type_checker);
        return false;
    }
    
    // 测试初始化
    bool init_result = type_checker->initialize(type_checker, scope_manager);
    bool test7_passed = init_result;
    print_test_result("类型检查器初始化", test7_passed);
    
    // 测试基本功能（桩实现）
    void* left_type = (void*)0x1234;
    void* right_type = (void*)0x5678;
    Stru_TypeCheckResult_t result = 
        type_checker->check_binary_expression(type_checker, left_type, right_type, 1);
    
    // 桩实现总是返回兼容
    bool test8_passed = (result.compatibility == Eum_TYPE_COMPATIBLE);
    print_test_result("检查二元表达式", test8_passed);
    
    // 清理
    F_destroy_type_checker_interface(type_checker);
    F_destroy_scope_manager_interface(scope_manager);
    F_destroy_symbol_table_interface(symbol_table);
    
    bool test9_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("清理资源", test9_passed);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && 
           test5_passed && test6_passed && test7_passed && test8_passed && test9_passed;
}

/**
 * @brief 测试错误报告器工厂函数
 */
static bool test_error_reporter_factory(void)
{
    printf("\n测试错误报告器工厂函数:\n");
    
    // 测试创建错误报告器
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    bool test1_passed = (error_reporter != NULL);
    print_test_result("创建错误报告器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试接口函数指针
    bool test2_passed = (error_reporter->initialize != NULL &&
                        error_reporter->report_error != NULL &&
                        error_reporter->report_warning != NULL &&
                        error_reporter->get_error_count != NULL &&
                        error_reporter->get_warning_count != NULL &&
                        error_reporter->get_all_errors != NULL &&
                        error_reporter->get_all_warnings != NULL &&
                        error_reporter->clear_all != NULL &&
                        error_reporter->has_errors != NULL &&
                        error_reporter->has_warnings != NULL &&
                        error_reporter->set_error_callback != NULL &&
                        error_reporter->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试初始化
    bool init_result = error_reporter->initialize(error_reporter, 0);
    bool test3_passed = init_result;
    print_test_result("初始化", test3_passed);
    
    if (!test3_passed) {
        F_destroy_semantic_error_reporter_interface(error_reporter);
        return false;
    }
    
    // 测试基本功能
    Stru_SemanticError_t error;
    error.type = Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL;
    error.message = "测试错误";
    error.line = 10;
    error.column = 5;
    error.file_name = "test.cn";
    error.related_node = NULL;
    error.extra_data = NULL;
    
    bool report_result = error_reporter->report_error(error_reporter, &error);
    bool test4_passed = report_result;
    print_test_result("报告错误", test4_passed);
    
    size_t error_count = 0;
    if (report_result) {
        error_count = error_reporter->get_error_count(error_reporter);
        bool test5_passed = (error_count == 1);
        print_test_result("获取错误数量", test5_passed);
    } else {
        bool test5_passed = false;
        print_test_result("获取错误数量（跳过）", test5_passed);
    }
    
    // 清理
    F_destroy_semantic_error_reporter_interface(error_reporter);
    bool test6_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁错误报告器", test6_passed);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && test6_passed;
}

/**
 * @brief 测试语义分析器工厂函数
 */
static bool test_semantic_analyzer_factory(void)
{
    printf("\n测试语义分析器工厂函数:\n");
    
    // 测试创建语义分析器
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer = 
        F_create_semantic_analyzer_interface();
    bool test1_passed = (semantic_analyzer != NULL);
    print_test_result("创建语义分析器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试接口函数指针
    bool test2_passed = (semantic_analyzer->initialize != NULL &&
                        semantic_analyzer->analyze_ast != NULL &&
                        semantic_analyzer->analyze_declaration != NULL &&
                        semantic_analyzer->analyze_statement != NULL &&
                        semantic_analyzer->analyze_expression != NULL &&
                        semantic_analyzer->get_symbol_table != NULL &&
                        semantic_analyzer->get_scope_manager != NULL &&
                        semantic_analyzer->get_type_checker != NULL &&
                        semantic_analyzer->get_error_reporter != NULL &&
                        semantic_analyzer->reset != NULL &&
                        semantic_analyzer->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 创建所有组件用于初始化
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    bool test3_passed = (symbol_table != NULL);
    print_test_result("创建符号表", test3_passed);
    
    if (!test3_passed) {
        F_destroy_semantic_analyzer_interface(semantic_analyzer);
        return false;
    }
    
    bool symbol_init = symbol_table->initialize(symbol_table, NULL);
    bool test4_passed = symbol_init;
    print_test_result("符号表初始化", test4_passed);
    
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    bool test5_passed = (scope_manager != NULL);
    print_test_result("创建作用域管理器", test5_passed);
    
    if (!test5_passed) {
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_semantic_analyzer_interface(semantic_analyzer);
        return false;
    }
    
    bool scope_init = scope_manager->initialize(scope_manager, symbol_table);
    bool test6_passed = scope_init;
    print_test_result("作用域管理器初始化", test6_passed);
    
    Stru_TypeCheckerInterface_t* type_checker = F_create_type_checker_interface();
    bool test7_passed = (type_checker != NULL);
    print_test_result("创建类型检查器", test7_passed);
    
    if (!test7_passed) {
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_semantic_analyzer_interface(semantic_analyzer);
        return false;
    }
    
    bool type_init = type_checker->initialize(type_checker, scope_manager);
    bool test8_passed = type_init;
    print_test_result("类型检查器初始化", test8_passed);
    
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    bool test9_passed = (error_reporter != NULL);
    print_test_result("创建错误报告器", test9_passed);
    
    if (!test9_passed) {
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_semantic_analyzer_interface(semantic_analyzer);
        return false;
    }
    
    bool error_init = error_reporter->initialize(error_reporter, 0);
    bool test10_passed = error_init;
    print_test_result("错误报告器初始化", test10_passed);
    
    if (!test10_passed) {
        F_destroy_semantic_error_reporter_interface(error_reporter);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_semantic_analyzer_interface(semantic_analyzer);
        return false;
    }
    
    // 测试初始化
    bool init_result = semantic_analyzer->initialize(
        semantic_analyzer, 
        symbol_table,
        scope_manager,
        type_checker,
        error_reporter);
    bool test11_passed = init_result;
    print_test_result("语义分析器初始化", test11_passed);
    
    if (!test11_passed) {
        F_destroy_semantic_error_reporter_interface(error_reporter);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_semantic_analyzer_interface(semantic_analyzer);
        return false;
    }
    
    // 测试基本功能
    void* ast_node = (void*)0x1234;
    bool analysis_result = semantic_analyzer->analyze_ast(semantic_analyzer, ast_node);
    bool test12_passed = analysis_result;
    print_test_result("AST分析", test12_passed);
    
    // 清理
    F_destroy_semantic_analyzer_interface(semantic_analyzer);
    F_destroy_semantic_error_reporter_interface(error_reporter);
    F_destroy_type_checker_interface(type_checker);
    F_destroy_scope_manager_interface(scope_manager);
    F_destroy_symbol_table_interface(symbol_table);
    
    bool test13_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("清理资源", test13_passed);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && 
           test5_passed && test6_passed && test7_passed && test8_passed && 
           test9_passed && test10_passed && test11_passed && test12_passed && test13_passed;
}

/**
 * @brief 测试完整语义分析器系统工厂函数
 */
static bool test_complete_semantic_analyzer_system_factory(void)
{
    printf("\n测试完整语义分析器系统工厂函数:\n");
    
    // 测试创建完整系统（无错误限制）
    Stru_SemanticAnalyzerInterface_t* complete_system = 
        F_create_complete_semantic_analyzer_system(0);
    bool test1_passed = (complete_system != NULL);
    print_test_result("创建完整系统（无限制）", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 验证系统已初始化
    Stru_SymbolTableInterface_t* symbol_table = 
        complete_system->get_symbol_table(complete_system);
    bool test2_passed = (symbol_table != NULL);
    print_test_result("获取符号表", test2_passed);
    
    Stru_ScopeManagerInterface_t* scope_manager = 
        complete_system->get_scope_manager(complete_system);
    bool test3_passed = (scope_manager != NULL);
    print_test_result("获取作用域管理器", test3_passed);
    
    Stru_TypeCheckerInterface_t* type_checker = 
        complete_system->get_type_checker(complete_system);
    bool test4_passed = (type_checker != NULL);
    print_test_result("获取类型检查器", test4_passed);
    
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        complete_system->get_error_reporter(complete_system);
    bool test5_passed = (error_reporter != NULL);
    print_test_result("获取错误报告器", test5_passed);
    
    // 测试系统功能
    void* ast_node = (void*)0x1234;
    bool analysis_result = complete_system->analyze_ast(complete_system, ast_node);
    bool test6_passed = analysis_result;
    print_test_result("系统AST分析", test6_passed);
    
    // 测试带错误限制的系统
    Stru_SemanticAnalyzerInterface_t* limited_system = 
        F_create_complete_semantic_analyzer_system(10);
    bool test7_passed = (limited_system != NULL);
    print_test_result("创建带限制系统", test7_passed);
    
    if (!test7_passed) {
        F_destroy_semantic_analyzer_interface(complete_system);
        return false;
    }
    
    // 验证错误报告器已正确配置限制
    Stru_SemanticErrorReporterInterface_t* limited_error_reporter = 
        limited_system->get_error_reporter(limited_system);
    bool test8_passed = (limited_error_reporter != NULL);
    print_test_result("获取带限制错误报告器", test8_passed);
    
    // 报告错误以测试限制
    bool test9_passed = true;
    for (int i = 0; i < 15; i++)
    {
        Stru_SemanticError_t error;
        error.type = Eum_SEMANTIC_ERROR_TYPE_MISMATCH;
        error.message = "测试错误";
        error.line = i + 1;
        error.column = 1;
        error.file_name = "test.cn";
        error.related_node = NULL;
        error.extra_data = NULL;
        
        bool report_result = limited_error_reporter->report_error(limited_error_reporter, &error);
        
        // 前10个应该成功，后5个应该失败
        if (i < 10)
        {
            if (!report_result) {
                test9_passed = false;
                break;
            }
        }
        else
        {
            if (report_result) {
                test9_passed = false;
                break;
            }
        }
    }
    print_test_result("错误限制功能", test9_passed);
    
    // 清理
    F_destroy_semantic_analyzer_interface(complete_system);
    F_destroy_semantic_analyzer_interface(limited_system);
    
    bool test10_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("清理系统资源", test10_passed);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && 
           test5_passed && test6_passed && test7_passed && test8_passed && 
           test9_passed && test10_passed;
}

/**
 * @brief 测试工厂函数边界情况
 */
static bool test_factory_edge_cases(void)
{
    printf("\n测试工厂函数边界情况:\n");
    
    // 测试多次创建和销毁
    bool test1_passed = true;
    const int NUM_INSTANCES = 5;
    for (int i = 0; i < NUM_INSTANCES; i++)
    {
        Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
        if (symbol_table == NULL) {
            test1_passed = false;
            break;
        }
        
        bool init_result = symbol_table->initialize(symbol_table, NULL);
        if (!init_result) {
            F_destroy_symbol_table_interface(symbol_table);
            test1_passed = false;
            break;
        }
        
        // 插入一些符号 - 使用字符串字面量避免内存问题
        const char* symbol_names[3] = {
            "symbol_a",
            "symbol_b", 
            "symbol_c"
        };
        
        for (int j = 0; j < 3; j++)
        {
            Stru_SymbolInfo_t symbol_info;
            symbol_info.name = symbol_names[j];
            symbol_info.type = Eum_SYMBOL_VARIABLE;
            symbol_info.type_info = NULL;
            symbol_info.line = j + 1;
            symbol_info.column = 1;
            symbol_info.is_exported = false;
            symbol_info.is_initialized = false;
            symbol_info.is_constant = false;
            symbol_info.extra_data = NULL;
            
            bool insert_result = symbol_table->insert_symbol(symbol_table, &symbol_info);
            if (!insert_result) {
                test1_passed = false;
                break;
            }
        }
        
        if (!test1_passed) {
            F_destroy_symbol_table_interface(symbol_table);
            break;
        }
        
        size_t symbol_count = symbol_table->get_symbol_count(symbol_table);
        if (symbol_count != 3) {
            test1_passed = false;
            F_destroy_symbol_table_interface(symbol_table);
            break;
        }
        
        F_destroy_symbol_table_interface(symbol_table);
    }
    print_test_result("多次创建和销毁", test1_passed);
    
    // 测试销毁NULL指针
    F_destroy_symbol_table_interface(NULL);
    F_destroy_scope_manager_interface(NULL);
    F_destroy_type_checker_interface(NULL);
    F_destroy_semantic_error_reporter_interface(NULL);
    F_destroy_semantic_analyzer_interface(NULL);
    bool test2_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁NULL指针", test2_passed);
    
    // 测试组件间的依赖关系
    bool test3_passed = true;
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    if (symbol_table == NULL) {
        test3_passed = false;
    }
    
    Stru_ScopeManagerInterface_t* scope_manager = NULL;
    if (test3_passed) {
        scope_manager = F_create_scope_manager_interface();
        if (scope_manager == NULL) {
            test3_passed = false;
        }
    }
    
    // 作用域管理器需要符号表进行初始化
    if (test3_passed) {
        bool scope_init = scope_manager->initialize(scope_manager, symbol_table);
        if (!scope_init) {
            test3_passed = false;
        }
    }
    
    // 类型检查器需要作用域管理器
    Stru_TypeCheckerInterface_t* type_checker = NULL;
    if (test3_passed) {
        type_checker = F_create_type_checker_interface();
        if (type_checker == NULL) {
            test3_passed = false;
        }
    }
    
    if (test3_passed) {
        bool type_init = type_checker->initialize(type_checker, scope_manager);
        if (!type_init) {
            test3_passed = false;
        }
    }
    
    // 清理
    if (type_checker != NULL) {
        F_destroy_type_checker_interface(type_checker);
    }
    if (scope_manager != NULL) {
        F_destroy_scope_manager_interface(scope_manager);
    }
    if (symbol_table != NULL) {
        F_destroy_symbol_table_interface(symbol_table);
    }
    
    print_test_result("组件依赖关系", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试工厂函数集成
 */
static bool test_factory_integration(void)
{
    printf("\n测试工厂函数集成:\n");
    
    // 创建完整系统
    Stru_SemanticAnalyzerInterface_t* system = 
        F_create_complete_semantic_analyzer_system(0);
    bool test1_passed = (system != NULL);
    print_test_result("创建完整系统", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 获取所有组件
    Stru_SymbolTableInterface_t* symbol_table = system->get_symbol_table(system);
    Stru_ScopeManagerInterface_t* scope_manager = system->get_scope_manager(system);
    Stru_TypeCheckerInterface_t* type_checker = system->get_type_checker(system);
    Stru_SemanticErrorReporterInterface_t* error_reporter = system->get_error_reporter(system);
    
    bool test2_passed = (symbol_table != NULL);
    print_test_result("获取符号表", test2_passed);
    
    bool test3_passed = (scope_manager != NULL);
    print_test_result("获取作用域管理器", test3_passed);
    
    bool test4_passed = (type_checker != NULL);
    print_test_result("获取类型检查器", test4_passed);
    
    bool test5_passed = (error_reporter != NULL);
    print_test_result("获取错误报告器", test5_passed);
    
    if (!test2_passed || !test3_passed || !test4_passed || !test5_passed) {
        F_destroy_semantic_analyzer_interface(system);
        return false;
    }
    
    // 测试组件集成功能
    // 1. 在符号表中插入符号
    Stru_SymbolInfo_t symbol_info;
    symbol_info.name = "global_var";
    symbol_info.type = Eum_SYMBOL_VARIABLE;
    symbol_info.type_info = NULL;
    symbol_info.line = 1;
    symbol_info.column = 1;
    symbol_info.is_exported = false;
    symbol_info.is_initialized = false;
    symbol_info.is_constant = false;
    symbol_info.extra_data = NULL;
    
    bool insert_result = symbol_table->insert_symbol(symbol_table, &symbol_info);
    bool test6_passed = insert_result;
    print_test_result("插入全局符号", test6_passed);
    
    // 2. 创建新作用域
    Stru_SymbolTableInterface_t* new_scope = 
        scope_manager->create_scope(scope_manager, Eum_SCOPE_FUNCTION, "test_func");
    bool test7_passed = (new_scope != NULL);
    print_test_result("创建新作用域", test7_passed);
    
    bool enter_result = false;
    if (test7_passed) {
        enter_result = scope_manager->enter_scope(scope_manager, new_scope);
        test7_passed = enter_result;
        print_test_result("进入新作用域", test7_passed);
    } else {
        print_test_result("进入新作用域（跳过）", false);
    }
    
    // 3. 在新作用域中插入符号
    bool local_insert = false;
    if (enter_result) {
        Stru_SymbolInfo_t local_symbol_info;
        local_symbol_info.name = "local_var";
        local_symbol_info.type = Eum_SYMBOL_VARIABLE;
        local_symbol_info.type_info = NULL;
        local_symbol_info.line = 2;
        local_symbol_info.column = 1;
        local_symbol_info.is_exported = false;
        local_symbol_info.is_initialized = false;
        local_symbol_info.is_constant = false;
        local_symbol_info.extra_data = NULL;
        
        local_insert = new_scope->insert_symbol(new_scope, &local_symbol_info);
        bool test8_passed = local_insert;
        print_test_result("插入局部符号", test8_passed);
    } else {
        print_test_result("插入局部符号（跳过）", false);
    }
    
    // 4. 测试类型检查
    void* left_type = (void*)0x1234;
    void* right_type = (void*)0x5678;
    Stru_TypeCheckResult_t type_result = 
        type_checker->check_binary_expression(type_checker, left_type, right_type, 1);
    bool test9_passed = (type_result.compatibility == Eum_TYPE_COMPATIBLE);
    print_test_result("类型检查", test9_passed);
    
    // 5. 测试错误报告
    Stru_SemanticError_t error;
    error.type = Eum_SEMANTIC_ERROR_TYPE_MISMATCH;
    error.message = "集成测试错误";
    error.line = 42;
    error.column = 7;
    error.file_name = "integration_test.cn";
    error.related_node = NULL;
    error.extra_data = NULL;
    
    bool report_result = error_reporter->report_error(error_reporter, &error);
    bool test10_passed = report_result;
    print_test_result("报告错误", test10_passed);
    
    size_t error_count = 0;
    if (report_result) {
        error_count = error_reporter->get_error_count(error_reporter);
        bool test11_passed = (error_count == 1);
        print_test_result("获取错误数量", test11_passed);
    } else {
        bool test11_passed = false;
        print_test_result("获取错误数量（跳过）", test11_passed);
    }
    
    // 6. 测试语义分析器功能
    void* ast_node = (void*)0x1234;
    bool analysis_result = system->analyze_ast(system, ast_node);
    bool test12_passed = analysis_result;
    print_test_result("系统AST分析", test12_passed);
    
    // 清理
    if (new_scope != NULL) {
        F_destroy_symbol_table_interface(new_scope);
    }
    F_destroy_semantic_analyzer_interface(system);
    
    bool test13_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("清理系统资源", test13_passed);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && 
           test5_passed && test6_passed && test7_passed && test9_passed && 
           test10_passed && test12_passed && test13_passed;
}

// ============================================================================
// 主测试函数
// ============================================================================

/**
 * @brief 主工厂测试函数
 * 
 * @return int 测试结果：0表示所有测试通过，非0表示有测试失败
 */
int test_semantic_factory_main(void)
{
    printf("========================================\n");
    printf("开始语义分析工厂模块测试\n");
    printf("========================================\n\n");
    
    int failed_tests = 0;
    
    // 运行所有测试
    if (!test_symbol_table_factory()) {
        failed_tests++;
    }
    
    if (!test_scope_manager_factory()) {
        failed_tests++;
    }
    
    if (!test_type_checker_factory()) {
        failed_tests++;
    }
    
    if (!test_error_reporter_factory()) {
        failed_tests++;
    }
    
    if (!test_semantic_analyzer_factory()) {
        failed_tests++;
    }
    
    if (!test_complete_semantic_analyzer_system_factory()) {
        failed_tests++;
    }
    
    if (!test_factory_edge_cases()) {
        failed_tests++;
    }
    
    if (!test_factory_integration()) {
        failed_tests++;
    }
    
    printf("\n========================================\n");
    printf("语义分析工厂模块测试完成\n");
    printf("========================================\n");
    
    if (failed_tests == 0) {
        printf("所有测试通过！\n");
        return 0;
    } else {
        printf("有 %d 个测试失败\n", failed_tests);
        return 1;
    }
}

/**
 * @brief 主函数（用于独立测试）
 */
int main(void)
{
    return test_semantic_factory_main();
}
