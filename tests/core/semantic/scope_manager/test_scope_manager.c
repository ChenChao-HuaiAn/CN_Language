/**
 * @file test_scope_manager.c
 * @brief 作用域管理器模块单元测试
 
 * 本文件包含作用域管理器模块的单元测试，测试作用域管理器的各项功能。
 * 遵循项目测试规范，每个测试功能在单独的文件中。
 * 
 * @author CN_Language测试团队
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../../../src/core/semantic/scope_manager/CN_scope_manager.h"
#include "../../../../src/core/semantic/symbol_table/CN_symbol_table.h"

// ============================================================================
// 测试辅助函数
// ============================================================================

/**
 * @brief 创建测试符号表
 */
static Stru_SymbolTableInterface_t* create_test_symbol_table(void)
{
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    if (symbol_table == NULL) {
        return NULL;
    }
    
    bool init_result = symbol_table->initialize(symbol_table, NULL);
    if (!init_result) {
        F_destroy_symbol_table_interface(symbol_table);
        return NULL;
    }
    
    return symbol_table;
}

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
 * @brief 测试作用域管理器创建和销毁
 */
static bool test_scope_manager_create_destroy(void)
{
    printf("测试作用域管理器创建和销毁:\n");
    
    // 测试1: 创建作用域管理器
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    bool test1_passed = (scope_manager != NULL);
    print_test_result("创建作用域管理器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
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
    
    // 测试3: 销毁作用域管理器
    F_destroy_scope_manager_interface(scope_manager);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁作用域管理器", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试作用域管理器初始化
 */
static bool test_scope_manager_initialization(void)
{
    printf("\n测试作用域管理器初始化:\n");
    
    // 创建全局符号表
    Stru_SymbolTableInterface_t* global_symbol_table = create_test_symbol_table();
    if (global_symbol_table == NULL) {
        return false;
    }
    
    // 创建作用域管理器
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    if (scope_manager == NULL) {
        F_destroy_symbol_table_interface(global_symbol_table);
        return false;
    }
    
    // 测试1: 初始化成功
    bool init_result = scope_manager->initialize(scope_manager, global_symbol_table);
    bool test1_passed = init_result;
    print_test_result("初始化成功", test1_passed);
    
    // 测试2: 重复初始化失败
    bool repeat_init = scope_manager->initialize(scope_manager, global_symbol_table);
    bool test2_passed = !repeat_init; // 应该失败
    print_test_result("重复初始化（应该失败）", test2_passed);
    
    // 测试3: 无效参数初始化
    bool null_init = scope_manager->initialize(NULL, global_symbol_table);
    bool test3_passed = !null_init; // 应该失败
    print_test_result("NULL管理器初始化（应该失败）", test3_passed);
    
    // 清理
    F_destroy_scope_manager_interface(scope_manager);
    F_destroy_symbol_table_interface(global_symbol_table);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试作用域创建
 */
static bool test_scope_creation(void)
{
    printf("\n测试作用域创建:\n");
    
    // 创建全局符号表
    Stru_SymbolTableInterface_t* global_symbol_table = create_test_symbol_table();
    if (global_symbol_table == NULL) {
        return false;
    }
    
    // 创建作用域管理器并初始化
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    if (scope_manager == NULL) {
        F_destroy_symbol_table_interface(global_symbol_table);
        return false;
    }
    
    bool init_result = scope_manager->initialize(scope_manager, global_symbol_table);
    if (!init_result) {
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(global_symbol_table);
        return false;
    }
    
    // 测试1: 创建新作用域
    Stru_SymbolTableInterface_t* new_scope = 
        scope_manager->create_scope(scope_manager, Eum_SCOPE_FUNCTION, "test_function");
    bool test1_passed = (new_scope != NULL);
    print_test_result("创建新作用域", test1_passed);
    
    // 测试2: 无效参数创建作用域
    Stru_SymbolTableInterface_t* null_scope = 
        scope_manager->create_scope(NULL, Eum_SCOPE_FUNCTION, "test_function");
    bool test2_passed = (null_scope == NULL); // 应该失败
    print_test_result("NULL管理器创建作用域（应该失败）", test2_passed);
    
    // 清理
    if (new_scope != NULL) {
        F_destroy_symbol_table_interface(new_scope);
    }
    F_destroy_scope_manager_interface(scope_manager);
    F_destroy_symbol_table_interface(global_symbol_table);
    
    return test1_passed && test2_passed;
}

/**
 * @brief 测试作用域进入和退出
 */
static bool test_scope_enter_exit(void)
{
    printf("\n测试作用域进入和退出:\n");
    
    // 创建全局符号表
    Stru_SymbolTableInterface_t* global_symbol_table = create_test_symbol_table();
    if (global_symbol_table == NULL) {
        return false;
    }
    
    // 创建作用域管理器并初始化
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    if (scope_manager == NULL) {
        F_destroy_symbol_table_interface(global_symbol_table);
        return false;
    }
    
    bool init_result = scope_manager->initialize(scope_manager, global_symbol_table);
    if (!init_result) {
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(global_symbol_table);
        return false;
    }
    
    // 创建新作用域
    Stru_SymbolTableInterface_t* new_scope = 
        scope_manager->create_scope(scope_manager, Eum_SCOPE_FUNCTION, "test_function");
    if (new_scope == NULL) {
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(global_symbol_table);
        return false;
    }
    
    // 测试1: 进入作用域
    bool enter_result = scope_manager->enter_scope(scope_manager, new_scope);
    bool test1_passed = enter_result;
    print_test_result("进入作用域", test1_passed);
    
    // 测试2: 获取当前作用域
    Stru_SymbolTableInterface_t* current_scope = 
        scope_manager->get_current_scope(scope_manager);
    bool test2_passed = (current_scope == new_scope);
    print_test_result("获取当前作用域", test2_passed);
    
    // 测试3: 获取作用域深度
    size_t depth = scope_manager->get_scope_depth(scope_manager);
    bool test3_passed = (depth == 2); // 全局作用域 + 新作用域
    print_test_result("获取作用域深度", test3_passed);
    
    // 测试4: 退出作用域
    Stru_SymbolTableInterface_t* after_exit = 
        scope_manager->exit_scope(scope_manager);
    bool test4_passed = (after_exit == global_symbol_table);
    print_test_result("退出作用域", test4_passed);
    
    // 测试5: 退出后深度
    depth = scope_manager->get_scope_depth(scope_manager);
    bool test5_passed = (depth == 1); // 只剩全局作用域
    print_test_result("退出后作用域深度", test5_passed);
    
    // 清理
    F_destroy_symbol_table_interface(new_scope);
    F_destroy_scope_manager_interface(scope_manager);
    F_destroy_symbol_table_interface(global_symbol_table);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && test5_passed;
}

/**
 * @brief 测试作用域链查找
 */
static bool test_scope_chain_lookup(void)
{
    printf("\n测试作用域链查找:\n");
    
    // 创建全局符号表
    Stru_SymbolTableInterface_t* global_symbol_table = create_test_symbol_table();
    if (global_symbol_table == NULL) {
        return false;
    }
    
    // 创建作用域管理器并初始化
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    if (scope_manager == NULL) {
        F_destroy_symbol_table_interface(global_symbol_table);
        return false;
    }
    
    bool init_result = scope_manager->initialize(scope_manager, global_symbol_table);
    if (!init_result) {
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(global_symbol_table);
        return false;
    }
    
    // 在全局作用域插入符号
    Stru_SymbolInfo_t global_symbol;
    global_symbol.name = "global_var";
    global_symbol.type = Eum_SYMBOL_VARIABLE;
    global_symbol.type_info = NULL;
    global_symbol.line = 1;
    global_symbol.column = 1;
    global_symbol.is_exported = false;
    global_symbol.is_initialized = false;
    global_symbol.is_constant = false;
    global_symbol.extra_data = NULL;
    
    bool insert_global = global_symbol_table->insert_symbol(global_symbol_table, &global_symbol);
    if (!insert_global) {
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(global_symbol_table);
        return false;
    }
    
    // 创建函数作用域
    Stru_SymbolTableInterface_t* function_scope = 
        scope_manager->create_scope(scope_manager, Eum_SCOPE_FUNCTION, "func1");
    if (function_scope == NULL) {
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(global_symbol_table);
        return false;
    }
    
    // 进入函数作用域
    bool enter_result = scope_manager->enter_scope(scope_manager, function_scope);
    if (!enter_result) {
        F_destroy_symbol_table_interface(function_scope);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(global_symbol_table);
        return false;
    }
    
    // 在函数作用域插入符号
    Stru_SymbolInfo_t local_symbol;
    local_symbol.name = "local_var";
    local_symbol.type = Eum_SYMBOL_VARIABLE;
    local_symbol.type_info = NULL;
    local_symbol.line = 2;
    local_symbol.column = 5;
    local_symbol.is_exported = false;
    local_symbol.is_initialized = false;
    local_symbol.is_constant = false;
    local_symbol.extra_data = NULL;
    
    bool insert_local = function_scope->insert_symbol(function_scope, &local_symbol);
    if (!insert_local) {
        F_destroy_symbol_table_interface(function_scope);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(global_symbol_table);
        return false;
    }
    
    // 测试1: 在作用域链中查找全局符号
    Stru_SymbolInfo_t* found_global = 
        scope_manager->lookup_symbol_in_scope_chain(scope_manager, "global_var");
    bool test1_passed = (found_global != NULL && strcmp(found_global->name, "global_var") == 0);
    print_test_result("在作用域链中查找全局符号", test1_passed);
    
    // 测试2: 在作用域链中查找局部符号
    Stru_SymbolInfo_t* found_local = 
        scope_manager->lookup_symbol_in_scope_chain(scope_manager, "local_var");
    bool test2_passed = (found_local != NULL && strcmp(found_local->name, "local_var") == 0);
    print_test_result("在作用域链中查找局部符号", test2_passed);
    
    // 测试3: 查找不存在的符号
    Stru_SymbolInfo_t* not_found = 
        scope_manager->lookup_symbol_in_scope_chain(scope_manager, "nonexistent");
    bool test3_passed = (not_found == NULL);
    print_test_result("查找不存在的符号", test3_passed);
    
    // 测试4: 无效参数查找
    Stru_SymbolInfo_t* null_result = 
        scope_manager->lookup_symbol_in_scope_chain(NULL, "global_var");
    bool test4_passed = (null_result == NULL); // 应该失败
    print_test_result("NULL管理器查找（应该失败）", test4_passed);
    
    // 清理
    F_destroy_symbol_table_interface(function_scope);
    F_destroy_scope_manager_interface(scope_manager);
    F_destroy_symbol_table_interface(global_symbol_table);
    
    return test1_passed && test2_passed && test3_passed && test4_passed;
}

/**
 * @brief 测试作用域管理器边界情况
 */
static bool test_scope_manager_edge_cases(void)
{
    printf("\n测试作用域管理器边界情况:\n");
    
    // 创建全局符号表
    Stru_SymbolTableInterface_t* global_symbol_table = create_test_symbol_table();
    if (global_symbol_table == NULL) {
        return false;
    }
    
    // 创建作用域管理器
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    if (scope_manager == NULL) {
        F_destroy_symbol_table_interface(global_symbol_table);
        return false;
    }
    
    // 测试1: 未初始化时的操作
    Stru_SymbolTableInterface_t* current_scope = 
        scope_manager->get_current_scope(scope_manager);
    bool test1_passed = (current_scope == NULL);
    print_test_result("未初始化时获取当前作用域", test1_passed);
    
    Stru_SymbolTableInterface_t* global_scope = 
        scope_manager->get_global_scope(scope_manager);
    bool test2_passed = (global_scope == NULL);
    print_test_result("未初始化时获取全局作用域", test2_passed);
    
    size_t depth = scope_manager->get_scope_depth(scope_manager);
    bool test3_passed = (depth == 0);
    print_test_result("未初始化时获取作用域深度", test3_passed);
    
    Stru_SymbolTableInterface_t* exit_result = 
        scope_manager->exit_scope(scope_manager);
    bool test4_passed = (exit_result == NULL);
    print_test_result("未初始化时退出作用域", test4_passed);
    
    // 初始化作用域管理器
    bool init_result = scope_manager->initialize(scope_manager, global_symbol_table);
    if (!init_result) {
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_symbol_table_interface(global_symbol_table);
        return false;
    }
    
    // 测试5: 栈扩容测试
    bool test5_passed = true;
    for (int i = 0; i < 20; i++) {
        char scope_name[32];
        snprintf(scope_name, sizeof(scope_name), "scope_%d", i);
        
        Stru_SymbolTableInterface_t* new_scope = 
            scope_manager->create_scope(scope_manager, Eum_SCOPE_BLOCK, scope_name);
        if (new_scope == NULL) {
            test5_passed = false;
            break;
        }
        
        bool enter_result = scope_manager->enter_scope(scope_manager, new_scope);
        if (!enter_result) {
            F_destroy_symbol_table_interface(new_scope);
            test5_passed = false;
            break;
        }
        
        // 验证深度
        size_t current_depth = scope_manager->get_scope_depth(scope_manager);
        if (current_depth != (size_t)(i + 2)) { // 全局作用域 + i+1个新作用域
            test5_passed = false;
            F_destroy_symbol_table_interface(new_scope);
            break;
        }
    }
    print_test_result("栈扩容测试", test5_passed);
    
    // 逐步退出所有作用域
    bool test6_passed = true;
    for (int i = 19; i >= 0; i--) {
        Stru_SymbolTableInterface_t* exit_scope = 
            scope_manager->exit_scope(scope_manager);
        if (exit_scope == NULL) {
            test6_passed = false;
            break;
        }
    }
    print_test_result("逐步退出作用域", test6_passed);
    
    // 测试7: 退出过多作用域
    Stru_SymbolTableInterface_t* over_exit = 
        scope_manager->exit_scope(scope_manager);
    bool test7_passed = (over_exit == NULL); // 应该失败，因为已经退回到全局作用域
    print_test_result("退出过多作用域（应该失败）", test7_passed);
    
    // 测试8: 获取全局作用域
    Stru_SymbolTableInterface_t* global_scope_after_init =
        scope_manager->get_global_scope(scope_manager);
    bool test8_passed = (global_scope_after_init == global_symbol_table);
    print_test_result("获取全局作用域", test8_passed);
    
    // 清理
    F_destroy_scope_manager_interface(scope_manager);
    F_destroy_symbol_table_interface(global_symbol_table);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && 
           test5_passed && test6_passed && test7_passed && test8_passed;
}

// ============================================================================
// 主测试函数
// ============================================================================

/**
 * @brief 作用域管理器模块主测试函数
 * 
 * 运行所有作用域管理器测试用例，并输出测试结果。
 * 
 * @return int 测试结果：0表示所有测试通过，非0表示有测试失败
 */
static int test_scope_manager_main(void)
{
    printf("========================================\n");
    printf("作用域管理器模块单元测试\n");
    printf("========================================\n\n");
    
    int passed_count = 0;
    int total_count = 6; // 总共有6个测试用例
    
    // 运行测试用例1: 作用域管理器创建和销毁
    printf("运行测试: 作用域管理器创建和销毁\n");
    bool result1 = test_scope_manager_create_destroy();
    if (result1) {
        passed_count++;
    }
    printf("\n");
    
    // 运行测试用例2: 作用域管理器初始化
    printf("运行测试: 作用域管理器初始化\n");
    bool result2 = test_scope_manager_initialization();
    if (result2) {
        passed_count++;
    }
    printf("\n");
    
    // 运行测试用例3: 作用域创建
    printf("运行测试: 作用域创建\n");
    bool result3 = test_scope_creation();
    if (result3) {
        passed_count++;
    }
    printf("\n");
    
    // 运行测试用例4: 作用域进入和退出
    printf("运行测试: 作用域进入和退出\n");
    bool result4 = test_scope_enter_exit();
    if (result4) {
        passed_count++;
    }
    printf("\n");
    
    // 运行测试用例5: 作用域链查找
    printf("运行测试: 作用域链查找\n");
    bool result5 = test_scope_chain_lookup();
    if (result5) {
        passed_count++;
    }
    printf("\n");
    
    // 运行测试用例6: 作用域管理器边界情况
    printf("运行测试: 作用域管理器边界情况\n");
    bool result6 = test_scope_manager_edge_cases();
    if (result6) {
        passed_count++;
    }
    printf("\n");
    
    // 输出测试摘要
    printf("========================================\n");
    printf("测试摘要:\n");
    printf("  总测试用例数: %d\n", total_count);
    printf("  通过测试数: %d\n", passed_count);
    printf("  失败测试数: %d\n", total_count - passed_count);
    printf("  通过率: %.1f%%\n", (float)passed_count / total_count * 100);
    printf("========================================\n");
    
    // 返回测试结果
    if (passed_count == total_count) {
        printf("所有测试通过！\n");
        return 0;
    } else {
        printf("有测试失败！\n");
        return 1;
    }
}

/**
 * @brief 运行所有作用域管理器测试
 */
int main(void)
{
    return test_scope_manager_main();
}
