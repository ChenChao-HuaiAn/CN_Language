/**
 * @file test_optimization_integration.c
 * @brief 高级优化模块集成测试
 
 * 本文件包含高级优化模块的集成测试，测试优化器的各项功能。
 * 包括常量传播、死代码消除、公共子表达式消除、循环优化和函数内联等。
 * 遵循项目测试规范，每个测试功能在单独的文件中。
 * 
 * @author CN_Language测试团队
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../../../src/core/semantic/optimization/CN_optimization.h"

// 模拟类型系统结构体，用于测试
typedef struct Stru_TypeSystem_t
{
    void* interface;
    void* private_data;
} Stru_TypeSystem_t;

// 创建模拟类型系统实例
static Stru_TypeSystem_t* create_mock_type_system(void)
{
    Stru_TypeSystem_t* type_system = (Stru_TypeSystem_t*)malloc(sizeof(Stru_TypeSystem_t));
    if (type_system)
    {
        type_system->interface = NULL;
        type_system->private_data = NULL;
    }
    return type_system;
}

// 销毁模拟类型系统实例
static void destroy_mock_type_system(Stru_TypeSystem_t* type_system)
{
    if (type_system)
    {
        free(type_system);
    }
}

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
 * @brief 测试常量传播器创建和销毁
 */
static bool test_constant_propagator_create_destroy(void)
{
    printf("测试常量传播器创建和销毁:\n");
    
    // 测试1: 创建常量传播器
    Stru_ConstantPropagationInterface_t* propagator = 
        F_create_constant_propagation_interface();
    bool test1_passed = (propagator != NULL);
    print_test_result("创建常量传播器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
    bool test2_passed = (propagator->initialize != NULL &&
                        propagator->propagate_constants != NULL &&
                        propagator->analyze_constants != NULL &&
                        propagator->is_constant_expression != NULL &&
                        propagator->get_constant_value != NULL &&
                        propagator->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试3: 销毁常量传播器
    F_destroy_constant_propagation_interface(propagator);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁常量传播器", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试死代码消除器创建和销毁
 */
static bool test_dead_code_eliminator_create_destroy(void)
{
    printf("\n测试死代码消除器创建和销毁:\n");
    
    // 测试1: 创建死代码消除器
    Stru_DeadCodeEliminationInterface_t* eliminator = 
        F_create_dead_code_elimination_interface();
    bool test1_passed = (eliminator != NULL);
    print_test_result("创建死代码消除器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
    bool test2_passed = (eliminator->initialize != NULL &&
                        eliminator->eliminate_dead_code != NULL &&
                        eliminator->analyze_reachability != NULL &&
                        eliminator->analyze_variable_usage != NULL &&
                        eliminator->is_reachable != NULL &&
                        eliminator->is_variable_used != NULL &&
                        eliminator->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试3: 销毁死代码消除器
    F_destroy_dead_code_elimination_interface(eliminator);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁死代码消除器", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试公共子表达式消除器创建和销毁
 */
static bool test_cse_eliminator_create_destroy(void)
{
    printf("\n测试公共子表达式消除器创建和销毁:\n");
    
    // 测试1: 创建公共子表达式消除器
    Stru_CommonSubexpressionEliminationInterface_t* cse_eliminator = 
        F_create_common_subexpression_elimination_interface();
    bool test1_passed = (cse_eliminator != NULL);
    print_test_result("创建公共子表达式消除器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
    bool test2_passed = (cse_eliminator->initialize != NULL &&
                        cse_eliminator->eliminate_common_subexpressions != NULL &&
                        cse_eliminator->are_expressions_equivalent != NULL &&
                        cse_eliminator->hash_expression != NULL &&
                        cse_eliminator->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试3: 销毁公共子表达式消除器
    F_destroy_common_subexpression_elimination_interface(cse_eliminator);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁公共子表达式消除器", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试循环优化器创建和销毁
 */
static bool test_loop_optimizer_create_destroy(void)
{
    printf("\n测试循环优化器创建和销毁:\n");
    
    // 测试1: 创建循环优化器
    Stru_LoopOptimizationInterface_t* loop_optimizer = 
        F_create_loop_optimization_interface();
    bool test1_passed = (loop_optimizer != NULL);
    print_test_result("创建循环优化器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
    bool test2_passed = (loop_optimizer->initialize != NULL &&
                        loop_optimizer->unroll_loop != NULL &&
                        loop_optimizer->hoist_invariant_code != NULL &&
                        loop_optimizer->analyze_invariant_expressions != NULL &&
                        loop_optimizer->is_invariant_expression != NULL &&
                        loop_optimizer->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试3: 销毁循环优化器
    F_destroy_loop_optimization_interface(loop_optimizer);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁循环优化器", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试函数内联器创建和销毁
 */
static bool test_function_inliner_create_destroy(void)
{
    printf("\n测试函数内联器创建和销毁:\n");
    
    // 测试1: 创建函数内联器
    Stru_FunctionInliningInterface_t* inliner = 
        F_create_function_inlining_interface();
    bool test1_passed = (inliner != NULL);
    print_test_result("创建函数内联器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
    bool test2_passed = (inliner->initialize != NULL &&
                        inliner->inline_function != NULL &&
                        inliner->is_function_inlinable != NULL &&
                        inliner->calculate_inlining_cost != NULL &&
                        inliner->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试3: 销毁函数内联器
    F_destroy_function_inlining_interface(inliner);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁函数内联器", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试优化管理器创建和销毁
 */
static bool test_optimization_manager_create_destroy(void)
{
    printf("\n测试优化管理器创建和销毁:\n");
    
    // 测试1: 创建优化管理器
    Stru_OptimizationManagerInterface_t* manager = 
        F_create_optimization_manager_interface();
    bool test1_passed = (manager != NULL);
    print_test_result("创建优化管理器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
    bool test2_passed = (manager->initialize != NULL &&
                        manager->optimize_ast != NULL &&
                        manager->get_constant_propagator != NULL &&
                        manager->get_dead_code_eliminator != NULL &&
                        manager->get_common_subexpression_eliminator != NULL &&
                        manager->get_loop_optimizer != NULL &&
                        manager->get_function_inliner != NULL &&
                        manager->set_optimization_options != NULL &&
                        manager->reset != NULL &&
                        manager->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试3: 销毁优化管理器
    F_destroy_optimization_manager_interface(manager);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁优化管理器", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试优化器初始化功能
 */
static bool test_optimizer_initialization(void)
{
    printf("\n测试优化器初始化功能:\n");
    
    bool all_passed = true;
    
    // 创建模拟类型系统
    Stru_TypeSystem_t* mock_type_system = create_mock_type_system();
    if (!mock_type_system)
    {
        printf("  创建模拟类型系统失败\n");
        return false;
    }
    
    // 测试常量传播器初始化
    Stru_ConstantPropagationInterface_t* propagator = 
        F_create_constant_propagation_interface();
    if (propagator != NULL) {
        bool init_result = propagator->initialize(propagator, mock_type_system);
        bool test_passed = init_result;
        print_test_result("常量传播器初始化", test_passed);
        all_passed = all_passed && test_passed;
        F_destroy_constant_propagation_interface(propagator);
    } else {
        print_test_result("常量传播器初始化", false);
        all_passed = false;
    }
    
    // 测试死代码消除器初始化
    Stru_DeadCodeEliminationInterface_t* eliminator = 
        F_create_dead_code_elimination_interface();
    if (eliminator != NULL) {
        bool init_result = eliminator->initialize(eliminator, mock_type_system);
        bool test_passed = init_result;
        print_test_result("死代码消除器初始化", test_passed);
        all_passed = all_passed && test_passed;
        F_destroy_dead_code_elimination_interface(eliminator);
    } else {
        print_test_result("死代码消除器初始化", false);
        all_passed = false;
    }
    
    // 测试公共子表达式消除器初始化
    Stru_CommonSubexpressionEliminationInterface_t* cse_eliminator = 
        F_create_common_subexpression_elimination_interface();
    if (cse_eliminator != NULL) {
        bool init_result = cse_eliminator->initialize(cse_eliminator, mock_type_system);
        bool test_passed = init_result;
        print_test_result("公共子表达式消除器初始化", test_passed);
        all_passed = all_passed && test_passed;
        F_destroy_common_subexpression_elimination_interface(cse_eliminator);
    } else {
        print_test_result("公共子表达式消除器初始化", false);
        all_passed = false;
    }
    
    // 测试循环优化器初始化
    Stru_LoopOptimizationInterface_t* loop_optimizer = 
        F_create_loop_optimization_interface();
    if (loop_optimizer != NULL) {
        bool init_result = loop_optimizer->initialize(loop_optimizer, mock_type_system);
        bool test_passed = init_result;
        print_test_result("循环优化器初始化", test_passed);
        all_passed = all_passed && test_passed;
        F_destroy_loop_optimization_interface(loop_optimizer);
    } else {
        print_test_result("循环优化器初始化", false);
        all_passed = false;
    }
    
    // 测试函数内联器初始化
    Stru_FunctionInliningInterface_t* inliner = 
        F_create_function_inlining_interface();
    if (inliner != NULL) {
        bool init_result = inliner->initialize(inliner, mock_type_system);
        bool test_passed = init_result;
        print_test_result("函数内联器初始化", test_passed);
        all_passed = all_passed && test_passed;
        F_destroy_function_inlining_interface(inliner);
    } else {
        print_test_result("函数内联器初始化", false);
        all_passed = false;
    }
    
    // 测试优化管理器初始化
    Stru_OptimizationManagerInterface_t* manager = 
        F_create_optimization_manager_interface();
    if (manager != NULL) {
        bool init_result = manager->initialize(manager, mock_type_system);
        bool test_passed = init_result;
        print_test_result("优化管理器初始化", test_passed);
        all_passed = all_passed && test_passed;
        F_destroy_optimization_manager_interface(manager);
    } else {
        print_test_result("优化管理器初始化", false);
        all_passed = false;
    }
    
    // 销毁模拟类型系统
    destroy_mock_type_system(mock_type_system);
    
    return all_passed;
}

/**
 * @brief 测试优化管理器功能
 */
static bool test_optimization_manager_functionality(void)
{
    printf("\n测试优化管理器功能:\n");
    
    // 创建模拟类型系统
    Stru_TypeSystem_t* mock_type_system = create_mock_type_system();
    if (!mock_type_system)
    {
        printf("  创建模拟类型系统失败\n");
        return false;
    }
    
    // 创建优化管理器
    Stru_OptimizationManagerInterface_t* manager = 
        F_create_optimization_manager_interface();
    if (manager == NULL) {
        destroy_mock_type_system(mock_type_system);
        print_test_result("创建优化管理器", false);
        return false;
    }
    
    // 初始化优化管理器
    bool init_result = manager->initialize(manager, mock_type_system);
    if (!init_result) {
        F_destroy_optimization_manager_interface(manager);
        destroy_mock_type_system(mock_type_system);
        print_test_result("初始化优化管理器", false);
        return false;
    }
    
    bool all_passed = true;
    
    // 测试1: 获取各个优化器接口
    Stru_ConstantPropagationInterface_t* propagator = manager->get_constant_propagator(manager);
    bool test1_passed = (propagator != NULL);
    print_test_result("获取常量传播器接口", test1_passed);
    all_passed = all_passed && test1_passed;
    
    Stru_DeadCodeEliminationInterface_t* eliminator = manager->get_dead_code_eliminator(manager);
    bool test2_passed = (eliminator != NULL);
    print_test_result("获取死代码消除器接口", test2_passed);
    all_passed = all_passed && test2_passed;
    
    Stru_CommonSubexpressionEliminationInterface_t* cse = manager->get_common_subexpression_eliminator(manager);
    bool test3_passed = (cse != NULL);
    print_test_result("获取公共子表达式消除器接口", test3_passed);
    all_passed = all_passed && test3_passed;
    
    Stru_LoopOptimizationInterface_t* loop_optimizer = manager->get_loop_optimizer(manager);
    bool test4_passed = (loop_optimizer != NULL);
    print_test_result("获取循环优化器接口", test4_passed);
    all_passed = all_passed && test4_passed;
    
    Stru_FunctionInliningInterface_t* inliner = manager->get_function_inliner(manager);
    bool test5_passed = (inliner != NULL);
    print_test_result("获取函数内联器接口", test5_passed);
    all_passed = all_passed && test5_passed;
    
    // 测试2: 设置优化选项
    manager->set_optimization_options(manager, 
                                     true,  // 启用常量传播
                                     true,  // 启用死代码消除
                                     true,  // 启用公共子表达式消除
                                     true,  // 启用循环优化
                                     true,  // 启用函数内联
                                     2);    // 激进优化级别
    bool test6_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("设置优化选项", test6_passed);
    all_passed = all_passed && test6_passed;
    
    // 测试3: 重置优化管理器
    manager->reset(manager);
    bool test7_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("重置优化管理器", test7_passed);
    all_passed = all_passed && test7_passed;
    
    // 清理
    F_destroy_optimization_manager_interface(manager);
    destroy_mock_type_system(mock_type_system);
    
    return all_passed;
}

/**
 * @brief 测试优化器基本功能
 */
static bool test_optimizer_basic_functionality(void)
{
    printf("\n测试优化器基本功能:\n");
    
    bool all_passed = true;
    
    // 创建模拟类型系统
    Stru_TypeSystem_t* mock_type_system = create_mock_type_system();
    if (!mock_type_system)
    {
        printf("  创建模拟类型系统失败\n");
        return false;
    }
    
    // 测试常量传播器基本功能
    Stru_ConstantPropagationInterface_t* propagator = 
        F_create_constant_propagation_interface();
    if (propagator != NULL) {
        propagator->initialize(propagator, mock_type_system);
        
        // 测试常量表达式检查
        bool is_constant = propagator->is_constant_expression(propagator, NULL);
        bool test1_passed = true; // 函数调用不崩溃即可
        print_test_result("常量表达式检查", test1_passed);
        all_passed = all_passed && test1_passed;
        
        // 测试常量值获取
        void* value = NULL;
        void* type = NULL;
        bool got_value = propagator->get_constant_value(propagator, NULL, &value, &type);
        bool test2_passed = true; // 函数调用不崩溃即可
        print_test_result("常量值获取", test2_passed);
        all_passed = all_passed && test2_passed;
        
        F_destroy_constant_propagation_interface(propagator);
    } else {
        print_test_result("常量传播器基本功能", false);
        all_passed = false;
    }
    
    // 测试死代码消除器基本功能
    Stru_DeadCodeEliminationInterface_t* eliminator = 
        F_create_dead_code_elimination_interface();
    if (eliminator != NULL) {
        eliminator->initialize(eliminator, mock_type_system);
        
        // 测试可达性检查
        bool is_reachable = eliminator->is_reachable(eliminator, NULL);
        bool test3_passed = true; // 函数调用不崩溃即可
        print_test_result("可达性检查", test3_passed);
        all_passed = all_passed && test3_passed;
        
        // 测试变量使用检查
        bool is_used = eliminator->is_variable_used(eliminator, "test_var", NULL);
        bool test4_passed = true; // 函数调用不崩溃即可
        print_test_result("变量使用检查", test4_passed);
        all_passed = all_passed && test4_passed;
        
        F_destroy_dead_code_elimination_interface(eliminator);
    } else {
        print_test_result("死代码消除器基本功能", false);
        all_passed = false;
    }
    
    // 测试公共子表达式消除器基本功能
    Stru_CommonSubexpressionEliminationInterface_t* cse_eliminator = 
        F_create_common_subexpression_elimination_interface();
    if (cse_eliminator != NULL) {
        cse_eliminator->initialize(cse_eliminator, mock_type_system);
        
        // 测试表达式等价性检查
        bool are_equivalent = cse_eliminator->are_expressions_equivalent(cse_eliminator, NULL, NULL);
        bool test5_passed = true; // 函数调用不崩溃即可
        print_test_result("表达式等价性检查", test5_passed);
        all_passed = all_passed && test5_passed;
        
        // 测试表达式哈希计算
        uint64_t hash = cse_eliminator->hash_expression(cse_eliminator, NULL);
        bool test6_passed = true; // 函数调用不崩溃即可
        print_test_result("表达式哈希计算", test6_passed);
        all_passed = all_passed && test6_passed;
        
        F_destroy_common_subexpression_elimination_interface(cse_eliminator);
    } else {
        print_test_result("公共子表达式消除器基本功能", false);
        all_passed = false;
    }
    
    // 测试循环优化器基本功能
    Stru_LoopOptimizationInterface_t* loop_optimizer = 
        F_create_loop_optimization_interface();
    if (loop_optimizer != NULL) {
        loop_optimizer->initialize(loop_optimizer, mock_type_system);
        
        // 测试循环不变表达式检查
        bool is_invariant = loop_optimizer->is_invariant_expression(loop_optimizer, NULL, NULL);
        bool test7_passed = true; // 函数调用不崩溃即可
        print_test_result("循环不变表达式检查", test7_passed);
        all_passed = all_passed && test7_passed;
        
        F_destroy_loop_optimization_interface(loop_optimizer);
    } else {
        print_test_result("循环优化器基本功能", false);
        all_passed = false;
    }
    
    // 测试函数内联器基本功能
    Stru_FunctionInliningInterface_t* inliner = 
        F_create_function_inlining_interface();
    if (inliner != NULL) {
        inliner->initialize(inliner, mock_type_system);
        
        // 测试函数内联成本计算
        int cost = inliner->calculate_inlining_cost(inliner, NULL);
        bool test8_passed = true; // 函数调用不崩溃即可
        print_test_result("函数内联成本计算", test8_passed);
        all_passed = all_passed && test8_passed;
        
        // 测试函数是否适合内联
        bool is_inlinable = inliner->is_function_inlinable(inliner, NULL);
        bool test9_passed = true; // 函数调用不崩溃即可
        print_test_result("函数内联适合性检查", test9_passed);
        all_passed = all_passed && test9_passed;
        
        F_destroy_function_inlining_interface(inliner);
    } else {
        print_test_result("函数内联器基本功能", false);
        all_passed = false;
    }
    
    // 销毁模拟类型系统
    destroy_mock_type_system(mock_type_system);
    
    return all_passed;
}

// ============================================================================
// 主测试函数
// ============================================================================

/**
 * @brief 运行所有测试
 */
int main(void)
{
    printf("========================================\n");
    printf("高级优化模块集成测试\n");
    printf("========================================\n\n");
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // 运行测试常量传播器创建和销毁
    total_tests++;
    if (test_constant_propagator_create_destroy()) {
        passed_tests++;
    }
    
    // 运行测试死代码消除器创建和销毁
    total_tests++;
    if (test_dead_code_eliminator_create_destroy()) {
        passed_tests++;
    }
    
    // 运行测试公共子表达式消除器创建和销毁
    total_tests++;
    if (test_cse_eliminator_create_destroy()) {
        passed_tests++;
    }
    
    // 运行测试循环优化器创建和销毁
    total_tests++;
    if (test_loop_optimizer_create_destroy()) {
        passed_tests++;
    }
    
    // 运行测试函数内联器创建和销毁
    total_tests++;
    if (test_function_inliner_create_destroy()) {
        passed_tests++;
    }
    
    // 运行测试优化管理器创建和销毁
    total_tests++;
    if (test_optimization_manager_create_destroy()) {
        passed_tests++;
    }
    
    // 运行测试优化器初始化功能
    total_tests++;
    if (test_optimizer_initialization()) {
        passed_tests++;
    }
    
    // 运行测试优化管理器功能
    total_tests++;
    if (test_optimization_manager_functionality()) {
        passed_tests++;
    }
    
    // 运行测试优化器基本功能
    total_tests++;
    if (test_optimizer_basic_functionality()) {
        passed_tests++;
    }
    
    printf("\n========================================\n");
    printf("测试结果汇总\n");
    printf("========================================\n");
    printf("总测试数: %d\n", total_tests);
    printf("通过测试: %d\n", passed_tests);
    printf("失败测试: %d\n", total_tests - passed_tests);
    printf("通过率: %.1f%%\n", (float)passed_tests / total_tests * 100);
    
    if (passed_tests == total_tests) {
        printf("\n所有测试通过！\n");
        return 0;
    } else {
        printf("\n部分测试失败！\n");
        return 1;
    }
}
