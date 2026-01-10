/**
 * @file test_error_recovery_integration.c
 * @brief 详细的错误报告和恢复机制模块集成测试
 
 * 本文件包含详细的错误报告和恢复机制模块的集成测试，测试错误报告器、
 * 错误恢复器、错误上下文管理器和错误统计器的各项功能。
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
#include <time.h>
#include "../../../../src/core/semantic/error_recovery/CN_error_recovery.h"

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

/**
 * @brief 创建测试用的详细错误信息
 */
static Stru_DetailedError_t create_test_error(void)
{
    Stru_DetailedError_t error = {0};
    
    // 基本错误信息
    error.error_code = 1001;
    error.error_type = "语法错误";
    error.error_message = "缺少分号";
    error.severity = Eum_ERROR_SEVERITY_ERROR;
    
    // 位置信息
    error.line = 42;
    error.column = 15;
    error.file_name = "test.cn";
    error.function_name = "main";
    error.module_name = "测试模块";
    
    // 上下文信息
    error.source_snippet = "变量 x = 5";
    error.surrounding_context = "在函数main中，第42行";
    error.ast_node = NULL;
    error.symbol_info = NULL;
    
    // 时间信息
    error.timestamp = time(NULL);
    error.process_id = 12345;
    error.thread_id = 67890;
    
    // 修复建议
    error.suggested_fix = "在行末添加分号";
    error.documentation_link = "https://docs.cn-language.org/syntax";
    
    // 调用栈信息
    error.call_stack = NULL;
    error.call_stack_depth = 0;
    
    // 额外数据
    error.extra_data = NULL;
    error.extra_data_size = 0;
    
    return error;
}

/**
 * @brief 创建测试用的错误上下文
 */
static Stru_ErrorContext_t create_test_context(void)
{
    Stru_ErrorContext_t context = {0};
    
    // 编译状态
    context.compilation_phase = "语义分析";
    context.current_pass = "类型检查";
    
    // 符号表状态
    context.symbol_table_state = NULL;
    context.scope_stack_state = NULL;
    
    // 类型系统状态
    context.type_system_state = NULL;
    
    // 内存状态
    context.memory_usage = 1024 * 1024; // 1MB
    context.heap_allocations = 100;
    
    // 性能指标
    context.processing_time_ms = 500;
    context.processed_nodes = 1000;
    
    // 环境信息
    context.platform = "Windows 10";
    context.compiler_version = "CN_Language 1.0.0";
    context.language_version = "CN 1.0";
    
    return context;
}

// ============================================================================
// 测试用例
// ============================================================================

/**
 * @brief 测试详细的错误报告器创建和销毁
 */
static bool test_detailed_error_reporter_create_destroy(void)
{
    printf("测试详细的错误报告器创建和销毁:\n");
    
    // 测试1: 创建详细的错误报告器
    Stru_DetailedErrorReporterInterface_t* reporter = 
        F_create_detailed_error_reporter_interface();
    bool test1_passed = (reporter != NULL);
    print_test_result("创建详细的错误报告器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
    bool test2_passed = (reporter->initialize != NULL &&
                        reporter->report_detailed_error != NULL &&
                        reporter->get_error_details != NULL &&
                        reporter->format_error_report != NULL &&
                        reporter->export_all_errors != NULL &&
                        reporter->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试3: 销毁详细的错误报告器
    F_destroy_detailed_error_reporter_interface(reporter);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁详细的错误报告器", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试错误恢复器创建和销毁
 */
static bool test_error_recovery_create_destroy(void)
{
    printf("\n测试错误恢复器创建和销毁:\n");
    
    // 测试1: 创建错误恢复器
    Stru_ErrorRecoveryInterface_t* recovery = 
        F_create_error_recovery_interface();
    bool test1_passed = (recovery != NULL);
    print_test_result("创建错误恢复器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
    bool test2_passed = (recovery->initialize != NULL &&
                        recovery->register_recovery_strategy != NULL &&
                        recovery->perform_recovery != NULL &&
                        recovery->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试3: 销毁错误恢复器
    F_destroy_error_recovery_interface(recovery);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁错误恢复器", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试错误上下文管理器创建和销毁
 */
static bool test_error_context_create_destroy(void)
{
    printf("\n测试错误上下文管理器创建和销毁:\n");
    
    // 测试1: 创建错误上下文管理器
    Stru_ErrorContextInterface_t* context = 
        F_create_error_context_interface();
    bool test1_passed = (context != NULL);
    print_test_result("创建错误上下文管理器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
    bool test2_passed = (context->initialize != NULL &&
                        context->capture_context != NULL &&
                        context->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试3: 销毁错误上下文管理器
    F_destroy_error_context_interface(context);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁错误上下文管理器", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试错误统计器创建和销毁
 */
static bool test_error_statistics_create_destroy(void)
{
    printf("\n测试错误统计器创建和销毁:\n");
    
    // 测试1: 创建错误统计器
    Stru_ErrorStatisticsInterface_t* statistics = 
        F_create_error_statistics_interface();
    bool test1_passed = (statistics != NULL);
    print_test_result("创建错误统计器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
    bool test2_passed = (statistics->initialize != NULL &&
                        statistics->add_error != NULL &&
                        statistics->get_error_frequency != NULL &&
                        statistics->get_error_distribution != NULL &&
                        statistics->get_error_trend != NULL &&
                        statistics->destroy != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试3: 销毁错误统计器
    F_destroy_error_statistics_interface(statistics);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁错误统计器", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试错误报告器初始化功能
 */
static bool test_error_reporter_initialization(void)
{
    printf("\n测试错误报告器初始化功能:\n");
    
    bool all_passed = true;
    
    // 测试详细的错误报告器初始化
    Stru_DetailedErrorReporterInterface_t* reporter = 
        F_create_detailed_error_reporter_interface();
    if (reporter != NULL) {
        bool init_result = reporter->initialize(reporter, 100, true);
        bool test_passed = init_result;
        print_test_result("详细的错误报告器初始化", test_passed);
        all_passed = all_passed && test_passed;
        F_destroy_detailed_error_reporter_interface(reporter);
    } else {
        print_test_result("详细的错误报告器初始化", false);
        all_passed = false;
    }
    
    // 测试错误恢复器初始化
    Stru_ErrorRecoveryInterface_t* recovery = 
        F_create_error_recovery_interface();
    if (recovery != NULL) {
        bool init_result = recovery->initialize(recovery, Eum_RECOVERY_STRATEGY_SKIP);
        bool test_passed = init_result;
        print_test_result("错误恢复器初始化", test_passed);
        all_passed = all_passed && test_passed;
        F_destroy_error_recovery_interface(recovery);
    } else {
        print_test_result("错误恢复器初始化", false);
        all_passed = false;
    }
    
    // 测试错误上下文管理器初始化
    Stru_ErrorContextInterface_t* context = 
        F_create_error_context_interface();
    if (context != NULL) {
        bool init_result = context->initialize(context, true);
        bool test_passed = init_result;
        print_test_result("错误上下文管理器初始化", test_passed);
        all_passed = all_passed && test_passed;
        F_destroy_error_context_interface(context);
    } else {
        print_test_result("错误上下文管理器初始化", false);
        all_passed = false;
    }
    
    // 测试错误统计器初始化
    Stru_ErrorStatisticsInterface_t* statistics = 
        F_create_error_statistics_interface();
    if (statistics != NULL) {
        bool init_result = statistics->initialize(statistics);
        bool test_passed = init_result;
        print_test_result("错误统计器初始化", test_passed);
        all_passed = all_passed && test_passed;
        F_destroy_error_statistics_interface(statistics);
    } else {
        print_test_result("错误统计器初始化", false);
        all_passed = false;
    }
    
    return all_passed;
}

/**
 * @brief 测试错误报告功能
 */
static bool test_error_reporting_functionality(void)
{
    printf("\n测试错误报告功能:\n");
    
    // 创建详细的错误报告器
    Stru_DetailedErrorReporterInterface_t* reporter = 
        F_create_detailed_error_reporter_interface();
    if (reporter == NULL) {
        print_test_result("创建详细的错误报告器", false);
        return false;
    }
    
    // 初始化错误报告器
    bool init_result = reporter->initialize(reporter, 10, false);
    if (!init_result) {
        F_destroy_detailed_error_reporter_interface(reporter);
        print_test_result("初始化错误报告器", false);
        return false;
    }
    
    bool all_passed = true;
    
    // 测试1: 报告错误
    Stru_DetailedError_t error = create_test_error();
    Stru_ErrorContext_t context = create_test_context();
    
    bool report_result = reporter->report_detailed_error(reporter, &error, &context);
    bool test1_passed = report_result;
    print_test_result("报告详细错误", test1_passed);
    all_passed = all_passed && test1_passed;
    
    // 测试2: 获取错误详情
    Stru_DetailedError_t retrieved_error = {0};
    Stru_ErrorContext_t retrieved_context = {0};
    
    bool get_result = reporter->get_error_details(reporter, 0, &retrieved_error, &retrieved_context);
    bool test2_passed = get_result;
    print_test_result("获取错误详情", test2_passed);
    all_passed = all_passed && test2_passed;
    
    // 测试3: 格式化错误报告
    char buffer[1024];
    size_t formatted_length = reporter->format_error_report(reporter, 0, Eum_ERROR_FORMAT_TEXT, buffer, sizeof(buffer));
    bool test3_passed = (formatted_length > 0);
    print_test_result("格式化错误报告", test3_passed);
    all_passed = all_passed && test3_passed;
    
    // 清理
    F_destroy_detailed_error_reporter_interface(reporter);
    
    return all_passed;
}

/**
 * @brief 测试错误恢复功能
 */
static bool test_error_recovery_functionality(void)
{
    printf("\n测试错误恢复功能:\n");
    
    // 创建错误恢复器
    Stru_ErrorRecoveryInterface_t* recovery = 
        F_create_error_recovery_interface();
    if (recovery == NULL) {
        print_test_result("创建错误恢复器", false);
        return false;
    }
    
    // 初始化错误恢复器
    bool init_result = recovery->initialize(recovery, Eum_RECOVERY_STRATEGY_SKIP);
    if (!init_result) {
        F_destroy_error_recovery_interface(recovery);
        print_test_result("初始化错误恢复器", false);
        return false;
    }
    
    bool all_passed = true;
    
    // 测试1: 注册恢复策略
    Stru_ErrorRecoveryStrategy_t strategy = {0};
    strategy.strategy = Eum_RECOVERY_STRATEGY_REPAIR;
    strategy.strategy_name = "修复策略";
    strategy.description = "尝试修复错误";
    strategy.max_retries = 3;
    strategy.retry_delay_ms = 100;
    strategy.allow_partial_recovery = true;
    strategy.recovery_action = NULL;
    strategy.recovery_context = NULL;
    
    bool register_result = recovery->register_recovery_strategy(recovery, "语法错误", &strategy);
    bool test1_passed = register_result;
    print_test_result("注册恢复策略", test1_passed);
    all_passed = all_passed && test1_passed;
    
    // 测试2: 执行错误恢复
    Stru_DetailedError_t error = create_test_error();
    Stru_ErrorContext_t context = create_test_context();
    void* recovery_result = NULL;
    
    bool perform_result = recovery->perform_recovery(recovery, &error, &context, &recovery_result);
    bool test2_passed = perform_result;
    print_test_result("执行错误恢复", test2_passed);
    all_passed = all_passed && test2_passed;
    
    // 清理
    F_destroy_error_recovery_interface(recovery);
    
    return all_passed;
}

/**
 * @brief 测试错误上下文捕获功能
 */
static bool test_error_context_capture(void)
{
    printf("\n测试错误上下文捕获功能:\n");
    
    // 创建错误上下文管理器
    Stru_ErrorContextInterface_t* context_manager = 
        F_create_error_context_interface();
    if (context_manager == NULL) {
        print_test_result("创建错误上下文管理器", false);
        return false;
    }
    
    // 初始化错误上下文管理器
    bool init_result = context_manager->initialize(context_manager, true);
    if (!init_result) {
        F_destroy_error_context_interface(context_manager);
        print_test_result("初始化错误上下文管理器", false);
        return false;
    }
    
    bool all_passed = true;
    
    // 测试1: 捕获上下文
    Stru_ErrorContext_t captured_context = {0};
    bool capture_result = context_manager->capture_context(context_manager, "语义分析", "类型检查", &captured_context);
    bool test1_passed = capture_result;
    print_test_result("捕获错误上下文", test1_passed);
    all_passed = all_passed && test1_passed;
    
    // 清理
    F_destroy_error_context_interface(context_manager);
    
    return all_passed;
}

/**
 * @brief 测试错误统计功能
 */
static bool test_error_statistics_functionality(void)
{
    printf("\n测试错误统计功能:\n");
    
    // 创建错误统计器
    Stru_ErrorStatisticsInterface_t* statistics = 
        F_create_error_statistics_interface();
    if (statistics == NULL) {
        print_test_result("创建错误统计器", false);
        return false;
    }
    
    // 初始化错误统计器
    bool init_result = statistics->initialize(statistics);
    if (!init_result) {
        F_destroy_error_statistics_interface(statistics);
        print_test_result("初始化错误统计器", false);
        return false;
    }
    
    bool all_passed = true;
    
    // 测试1: 添加错误到统计
    Stru_DetailedError_t error = create_test_error();
    Stru_ErrorContext_t context = create_test_context();
    
    bool add_result = statistics->add_error(statistics, &error, &context);
    bool test1_passed = add_result;
    print_test_result("添加错误到统计", test1_passed);
    all_passed = all_passed && test1_passed;
    
    // 测试2: 获取错误频率
    void* error_type_counts = NULL;
    size_t type_count = statistics->get_error_frequency(statistics, &error_type_counts, 10);
    bool test2_passed = (type_count >= 0);
    print_test_result("获取错误频率", test2_passed);
    all_passed = all_passed && test2_passed;
    
    // 测试3: 获取错误分布
    void* file_distribution = NULL;
    void* module_distribution = NULL;
    void* severity_distribution = NULL;
    bool distribution_result = statistics->get_error_distribution(statistics, &file_distribution, &module_distribution, &severity_distribution);
    bool test3_passed = distribution_result;
    print_test_result("获取错误分布", test3_passed);
    all_passed = all_passed && test3_passed;
    
    // 测试4: 获取错误趋势
    void* error_trend = NULL;
    size_t trend_count = statistics->get_error_trend(statistics, 300, &error_trend, 5); // 300秒（5分钟）的时间周期
    bool test4_passed = (trend_count >= 0);
    print_test_result("获取错误趋势", test4_passed);
    all_passed = all_passed && test4_passed;
    
    // 清理
    F_destroy_error_statistics_interface(statistics);
    
    return all_passed;
}

// ============================================================================
// 主测试函数
// ============================================================================

/**
 * @brief 主函数 - 运行所有集成测试
 */
int main(void)
{
    printf("========================================\n");
    printf("详细的错误报告和恢复机制模块集成测试\n");
    printf("========================================\n\n");
    
    int passed_tests = 0;
    int total_tests = 9;
    
    // 运行所有测试
    if (test_detailed_error_reporter_create_destroy()) {
        passed_tests++;
    }
    
    if (test_error_recovery_create_destroy()) {
        passed_tests++;
    }
    
    if (test_error_context_create_destroy()) {
        passed_tests++;
    }
    
    if (test_error_statistics_create_destroy()) {
        passed_tests++;
    }
    
    if (test_error_reporter_initialization()) {
        passed_tests++;
    }
    
    if (test_error_reporting_functionality()) {
        passed_tests++;
    }
    
    if (test_error_recovery_functionality()) {
        passed_tests++;
    }
    
    if (test_error_context_capture()) {
        passed_tests++;
    }
    
    if (test_error_statistics_functionality()) {
        passed_tests++;
    }
    
    // 打印测试摘要
    printf("\n========================================\n");
    printf("测试摘要:\n");
    printf("  通过测试: %d/%d\n", passed_tests, total_tests);
    printf("  成功率: %.1f%%\n", (float)passed_tests / total_tests * 100);
    printf("========================================\n");
    
    if (passed_tests == total_tests) {
        printf("所有测试通过！\n");
        return 0;
    } else {
        printf("部分测试失败。\n");
        return 1;
    }
}
