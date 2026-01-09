/**
 * @file test_symbol_table.c
 * @brief 符号表模块单元测试
 
 * 本文件包含符号表模块的单元测试，测试符号表的各项功能。
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
#include "../../../../src/core/semantic/symbol_table/CN_symbol_table.h"

// ============================================================================
// 测试辅助函数
// ============================================================================

/**
 * @brief 创建测试符号
 */
static Stru_SymbolInfo_t create_test_symbol(const char* name, Eum_SymbolType type)
{
    Stru_SymbolInfo_t symbol;
    symbol.name = name;
    symbol.type = type;
    symbol.type_info = NULL;
    symbol.line = 1;
    symbol.column = 1;
    symbol.is_exported = false;
    symbol.is_initialized = false;
    symbol.is_constant = false;
    symbol.extra_data = NULL;
    
    return symbol;
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
 * @brief 测试符号表创建和销毁
 */
static bool test_symbol_table_create_destroy(void)
{
    printf("测试符号表创建和销毁:\n");
    
    // 测试1: 创建符号表
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    bool test1_passed = (symbol_table != NULL);
    print_test_result("创建符号表", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 初始化符号表
    bool init_result = symbol_table->initialize(symbol_table, NULL);
    bool test2_passed = init_result;
    print_test_result("初始化符号表", test2_passed);
    
    // 测试3: 销毁符号表
    F_destroy_symbol_table_interface(symbol_table);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁符号表", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试符号插入和查找
 */
static bool test_symbol_insert_lookup(void)
{
    printf("\n测试符号插入和查找:\n");
    
    // 创建符号表
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    if (symbol_table == NULL) {
        return false;
    }
    
    symbol_table->initialize(symbol_table, NULL);
    
    // 测试1: 插入符号
    Stru_SymbolInfo_t symbol1 = create_test_symbol("variable1", Eum_SYMBOL_VARIABLE);
    bool insert_result = symbol_table->insert_symbol(symbol_table, &symbol1);
    bool test1_passed = insert_result;
    print_test_result("插入符号", test1_passed);
    
    // 测试2: 查找符号（当前作用域）
    Stru_SymbolInfo_t* found = symbol_table->lookup_symbol(symbol_table, "variable1", false);
    bool test2_passed = (found != NULL && strcmp(found->name, "variable1") == 0);
    print_test_result("查找符号（当前作用域）", test2_passed);
    
    // 测试3: 查找不存在的符号
    Stru_SymbolInfo_t* not_found = symbol_table->lookup_symbol(symbol_table, "nonexistent", false);
    bool test3_passed = (not_found == NULL);
    print_test_result("查找不存在的符号", test3_passed);
    
    // 测试4: 插入重复符号
    Stru_SymbolInfo_t symbol2 = create_test_symbol("variable1", Eum_SYMBOL_VARIABLE);
    bool duplicate_result = symbol_table->insert_symbol(symbol_table, &symbol2);
    bool test4_passed = !duplicate_result; // 应该失败
    print_test_result("插入重复符号（应该失败）", test4_passed);
    
    // 清理
    F_destroy_symbol_table_interface(symbol_table);
    
    return test1_passed && test2_passed && test3_passed && test4_passed;
}

/**
 * @brief 测试符号删除
 */
static bool test_symbol_remove(void)
{
    printf("\n测试符号删除:\n");
    
    // 创建符号表
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    if (symbol_table == NULL) {
        return false;
    }
    
    symbol_table->initialize(symbol_table, NULL);
    
    // 插入测试符号
    Stru_SymbolInfo_t symbol = create_test_symbol("to_remove", Eum_SYMBOL_VARIABLE);
    symbol_table->insert_symbol(symbol_table, &symbol);
    
    // 测试1: 删除存在的符号
    bool remove_result = symbol_table->remove_symbol(symbol_table, "to_remove");
    bool test1_passed = remove_result;
    print_test_result("删除存在的符号", test1_passed);
    
    // 验证符号已被删除
    Stru_SymbolInfo_t* found = symbol_table->lookup_symbol(symbol_table, "to_remove", false);
    bool test2_passed = (found == NULL);
    print_test_result("验证符号已被删除", test2_passed);
    
    // 测试3: 删除不存在的符号
    bool remove_nonexistent = symbol_table->remove_symbol(symbol_table, "nonexistent");
    bool test3_passed = !remove_nonexistent; // 应该失败
    print_test_result("删除不存在的符号（应该失败）", test3_passed);
    
    // 清理
    F_destroy_symbol_table_interface(symbol_table);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试符号数量统计
 */
static bool test_symbol_count(void)
{
    printf("\n测试符号数量统计:\n");
    
    // 创建符号表
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    if (symbol_table == NULL) {
        return false;
    }
    
    symbol_table->initialize(symbol_table, NULL);
    
    // 测试1: 空符号表
    size_t initial_count = symbol_table->get_symbol_count(symbol_table);
    bool test1_passed = (initial_count == 0);
    print_test_result("空符号表数量为0", test1_passed);
    
    // 插入一些符号
    Stru_SymbolInfo_t symbol1 = create_test_symbol("var1", Eum_SYMBOL_VARIABLE);
    Stru_SymbolInfo_t symbol2 = create_test_symbol("var2", Eum_SYMBOL_VARIABLE);
    Stru_SymbolInfo_t symbol3 = create_test_symbol("func1", Eum_SYMBOL_FUNCTION);
    
    symbol_table->insert_symbol(symbol_table, &symbol1);
    symbol_table->insert_symbol(symbol_table, &symbol2);
    symbol_table->insert_symbol(symbol_table, &symbol3);
    
    // 测试2: 插入后数量
    size_t after_insert_count = symbol_table->get_symbol_count(symbol_table);
    bool test2_passed = (after_insert_count == 3);
    print_test_result("插入3个符号后数量为3", test2_passed);
    
    // 删除一个符号
    symbol_table->remove_symbol(symbol_table, "var2");
    
    // 测试3: 删除后数量
    size_t after_remove_count = symbol_table->get_symbol_count(symbol_table);
    bool test3_passed = (after_remove_count == 2);
    print_test_result("删除1个符号后数量为2", test3_passed);
    
    // 清理
    F_destroy_symbol_table_interface(symbol_table);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试符号表清空
 */
static bool test_symbol_table_clear(void)
{
    printf("\n测试符号表清空:\n");
    
    // 创建符号表
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    if (symbol_table == NULL) {
        return false;
    }
    
    symbol_table->initialize(symbol_table, NULL);
    
    // 插入一些符号
    Stru_SymbolInfo_t symbol1 = create_test_symbol("var1", Eum_SYMBOL_VARIABLE);
    Stru_SymbolInfo_t symbol2 = create_test_symbol("var2", Eum_SYMBOL_VARIABLE);
    
    symbol_table->insert_symbol(symbol_table, &symbol1);
    symbol_table->insert_symbol(symbol_table, &symbol2);
    
    // 验证插入成功
    size_t before_clear_count = symbol_table->get_symbol_count(symbol_table);
    bool test1_passed = (before_clear_count == 2);
    print_test_result("清空前有2个符号", test1_passed);
    
    // 清空符号表
    symbol_table->clear(symbol_table);
    
    // 测试2: 清空后数量
    size_t after_clear_count = symbol_table->get_symbol_count(symbol_table);
    bool test2_passed = (after_clear_count == 0);
    print_test_result("清空后数量为0", test2_passed);
    
    // 测试3: 清空后查找
    Stru_SymbolInfo_t* found = symbol_table->lookup_symbol(symbol_table, "var1", false);
    bool test3_passed = (found == NULL);
    print_test_result("清空后查找不到符号", test3_passed);
    
    // 清理
    F_destroy_symbol_table_interface(symbol_table);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试父作用域查找
 */
static bool test_parent_scope_lookup(void)
{
    printf("\n测试父作用域查找:\n");
    
    // 创建父符号表
    Stru_SymbolTableInterface_t* parent_table = F_create_symbol_table_interface();
    if (parent_table == NULL) {
        return false;
    }
    parent_table->initialize(parent_table, NULL);
    
    // 在父符号表中插入符号
    Stru_SymbolInfo_t parent_symbol = create_test_symbol("parent_var", Eum_SYMBOL_VARIABLE);
    parent_table->insert_symbol(parent_table, &parent_symbol);
    
    // 创建子符号表（指定父符号表）
    Stru_SymbolTableInterface_t* child_table = F_create_symbol_table_interface();
    if (child_table == NULL) {
        F_destroy_symbol_table_interface(parent_table);
        return false;
    }
    child_table->initialize(child_table, parent_table);
    
    // 在子符号表中插入符号
    Stru_SymbolInfo_t child_symbol = create_test_symbol("child_var", Eum_SYMBOL_VARIABLE);
    child_table->insert_symbol(child_table, &child_symbol);
    
    // 测试1: 在子作用域中查找子符号
    Stru_SymbolInfo_t* found_child = child_table->lookup_symbol(child_table, "child_var", false);
    bool test1_passed = (found_child != NULL && strcmp(found_child->name, "child_var") == 0);
    print_test_result("在子作用域中查找子符号", test1_passed);
    
    // 测试2: 在子作用域中查找父符号（不搜索父作用域）
    Stru_SymbolInfo_t* found_parent_no_search = child_table->lookup_symbol(child_table, "parent_var", false);
    bool test2_passed = (found_parent_no_search == NULL);
    print_test_result("在子作用域中查找父符号（不搜索父作用域）", test2_passed);
    
    // 测试3: 在子作用域中查找父符号（搜索父作用域）
    Stru_SymbolInfo_t* found_parent_with_search = child_table->lookup_symbol(child_table, "parent_var", true);
    bool test3_passed = (found_parent_with_search != NULL && strcmp(found_parent_with_search->name, "parent_var") == 0);
    print_test_result("在子作用域中查找父符号（搜索父作用域）", test3_passed);
    
    // 清理
    F_destroy_symbol_table_interface(child_table);
    F_destroy_symbol_table_interface(parent_table);
    
    return test1_passed && test2_passed && test3_passed;
}

// ============================================================================
// 主测试函数
// ============================================================================

/**
 * @brief 运行所有符号表测试
 */
int main(void)
{
    printf("========================================\n");
    printf("符号表模块单元测试\n");
    printf("========================================\n\n");
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // 运行所有测试用例
    bool (*test_functions[])(void) = {
        test_symbol_table_create_destroy,
        test_symbol_insert_lookup,
        test_symbol_remove,
        test_symbol_count,
        test_symbol_table_clear,
        test_parent_scope_lookup
    };
    
    const char* test_names[] = {
        "符号表创建和销毁",
        "符号插入和查找",
        "符号删除",
        "符号数量统计",
        "符号表清空",
        "父作用域查找"
    };
    
    size_t num_tests = sizeof(test_functions) / sizeof(test_functions[0]);
    
    for (size_t i = 0; i < num_tests; i++) {
        printf("\n测试用例: %s\n", test_names[i]);
        printf("----------------------------------------\n");
        
        bool result = test_functions[i]();
        total_tests++;
        if (result) {
            passed_tests++;
        }
        
        printf("结果: %s\n\n", result ? "通过" : "失败");
    }
    
    // 打印测试摘要
    printf("========================================\n");
    printf("测试摘要\n");
    printf("========================================\n");
    printf("总测试用例: %d\n", total_tests);
    printf("通过: %d\n", passed_tests);
    printf("失败: %d\n", total_tests - passed_tests);
    printf("通过率: %.1f%%\n", total_tests > 0 ? (passed_tests * 100.0 / total_tests) : 0.0);
    
    return (passed_tests == total_tests) ? 0 : 1;
}
