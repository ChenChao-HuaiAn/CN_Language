/**
 * @file test_error_reporter.c
 * @brief 语义错误报告器模块单元测试
 
 * 本文件包含语义错误报告器模块的单元测试，测试错误报告器的各项功能。
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
#include "../../../../src/core/semantic/error_reporter/CN_error_reporter.h"

// ============================================================================
// 测试辅助函数
// ============================================================================

/**
 * @brief 创建测试错误
 */
static Stru_SemanticError_t create_test_error(Eum_SemanticErrorType type, 
                                             const char* message, 
                                             size_t line, size_t column)
{
    Stru_SemanticError_t error;
    error.type = type;
    error.message = message;
    error.line = line;
    error.column = column;
    error.file_name = "test.cn";
    error.related_node = NULL;
    error.extra_data = NULL;
    
    return error;
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
 * @brief 测试错误报告器创建和销毁
 */
static bool test_error_reporter_create_destroy(void)
{
    printf("测试错误报告器创建和销毁:\n");
    
    // 测试1: 创建错误报告器
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    bool test1_passed = (error_reporter != NULL);
    print_test_result("创建错误报告器", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
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
    
    // 测试3: 销毁错误报告器
    F_destroy_semantic_error_reporter_interface(error_reporter);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁错误报告器", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试错误报告器初始化
 */
static bool test_error_reporter_initialization(void)
{
    printf("\n测试错误报告器初始化:\n");
    
    // 创建错误报告器
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    if (error_reporter == NULL) {
        return false;
    }
    
    // 测试1: 初始化成功（无限制）
    bool init_result = error_reporter->initialize(error_reporter, 0);
    bool test1_passed = init_result;
    print_test_result("初始化成功（无限制）", test1_passed);
    
    // 测试2: 重复初始化失败
    bool repeat_init = error_reporter->initialize(error_reporter, 0);
    bool test2_passed = !repeat_init; // 应该失败
    print_test_result("重复初始化（应该失败）", test2_passed);
    
    // 测试3: 带限制的初始化
    Stru_SemanticErrorReporterInterface_t* limited_reporter = 
        F_create_semantic_error_reporter_interface();
    if (limited_reporter == NULL) {
        F_destroy_semantic_error_reporter_interface(error_reporter);
        return false;
    }
    
    init_result = limited_reporter->initialize(limited_reporter, 10);
    bool test3_passed = init_result;
    print_test_result("带限制的初始化", test3_passed);
    
    // 测试4: 无效参数初始化
    bool null_init = error_reporter->initialize(NULL, 0);
    bool test4_passed = !null_init; // 应该失败
    print_test_result("NULL报告器初始化（应该失败）", test4_passed);
    
    // 清理
    F_destroy_semantic_error_reporter_interface(error_reporter);
    F_destroy_semantic_error_reporter_interface(limited_reporter);
    
    return test1_passed && test2_passed && test3_passed && test4_passed;
}

/**
 * @brief 测试错误报告
 */
static bool test_error_reporting(void)
{
    printf("\n测试错误报告:\n");
    
    // 创建错误报告器
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    if (error_reporter == NULL) {
        return false;
    }
    
    // 初始化
    bool init_result = error_reporter->initialize(error_reporter, 0);
    if (!init_result) {
        F_destroy_semantic_error_reporter_interface(error_reporter);
        return false;
    }
    
    // 创建测试错误
    Stru_SemanticError_t error = create_test_error(
        Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL, 
        "未定义符号 'x'", 
        10, 5);
    
    // 测试1: 报告错误
    bool report_result = error_reporter->report_error(error_reporter, &error);
    bool test1_passed = report_result;
    print_test_result("报告错误", test1_passed);
    
    // 测试2: 验证错误数量
    size_t error_count = error_reporter->get_error_count(error_reporter);
    bool test2_passed = (error_count == 1);
    print_test_result("验证错误数量", test2_passed);
    
    // 测试3: 验证是否有错误
    bool has_errors = error_reporter->has_errors(error_reporter);
    bool test3_passed = has_errors;
    print_test_result("验证是否有错误", test3_passed);
    
    // 测试4: 报告警告
    Stru_SemanticError_t warning = create_test_error(
        Eum_SEMANTIC_ERROR_UNINITIALIZED_VARIABLE,
        "变量 'y' 可能未初始化",
        15, 8);
    
    bool warning_result = error_reporter->report_warning(error_reporter, &warning);
    bool test4_passed = warning_result;
    print_test_result("报告警告", test4_passed);
    
    // 测试5: 验证警告数量
    size_t warning_count = error_reporter->get_warning_count(error_reporter);
    bool test5_passed = (warning_count == 1);
    print_test_result("验证警告数量", test5_passed);
    
    // 测试6: 验证是否有警告
    bool has_warnings = error_reporter->has_warnings(error_reporter);
    bool test6_passed = has_warnings;
    print_test_result("验证是否有警告", test6_passed);
    
    // 测试7: 无效参数报告错误
    bool null_report = error_reporter->report_error(NULL, &error);
    bool test7_passed = !null_report; // 应该失败
    print_test_result("NULL报告器报告错误（应该失败）", test7_passed);
    
    // 测试8: 无效错误参数
    bool null_error = error_reporter->report_error(error_reporter, NULL);
    bool test8_passed = !null_error; // 应该失败
    print_test_result("NULL错误参数报告（应该失败）", test8_passed);
    
    // 清理
    F_destroy_semantic_error_reporter_interface(error_reporter);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && 
           test5_passed && test6_passed && test7_passed && test8_passed;
}

/**
 * @brief 测试错误查询
 */
static bool test_error_querying(void)
{
    printf("\n测试错误查询:\n");
    
    // 创建错误报告器
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    if (error_reporter == NULL) {
        return false;
    }
    
    // 初始化
    bool init_result = error_reporter->initialize(error_reporter, 0);
    if (!init_result) {
        F_destroy_semantic_error_reporter_interface(error_reporter);
        return false;
    }
    
    // 报告多个错误
    bool all_errors_reported = true;
    for (int i = 0; i < 5; i++)
    {
        char message[64];
        snprintf(message, sizeof(message), "错误 %d", i);
        
        Stru_SemanticError_t error = create_test_error(
            Eum_SEMANTIC_ERROR_TYPE_MISMATCH,
            message,
            i * 10 + 1, i * 5 + 1);
        
        bool result = error_reporter->report_error(error_reporter, &error);
        if (!result) {
            all_errors_reported = false;
            break;
        }
    }
    
    bool test1_passed = all_errors_reported;
    print_test_result("报告多个错误", test1_passed);
    
    // 报告多个警告
    bool all_warnings_reported = true;
    for (int i = 0; i < 3; i++)
    {
        char message[64];
        snprintf(message, sizeof(message), "警告 %d", i);
        
        Stru_SemanticError_t warning = create_test_error(
            Eum_SEMANTIC_ERROR_UNREACHABLE_CODE,
            message,
            i * 20 + 1, i * 10 + 1);
        
        bool result = error_reporter->report_warning(error_reporter, &warning);
        if (!result) {
            all_warnings_reported = false;
            break;
        }
    }
    
    bool test2_passed = all_warnings_reported;
    print_test_result("报告多个警告", test2_passed);
    
    // 测试3: 验证错误总数
    size_t error_count = error_reporter->get_error_count(error_reporter);
    bool test3_passed = (error_count == 5);
    print_test_result("验证错误总数", test3_passed);
    
    // 测试4: 验证警告总数
    size_t warning_count = error_reporter->get_warning_count(error_reporter);
    bool test4_passed = (warning_count == 3);
    print_test_result("验证警告总数", test4_passed);
    
    // 测试5: 获取所有错误
    Stru_SemanticError_t errors[10];
    size_t retrieved_errors = error_reporter->get_all_errors(
        error_reporter, errors, 10);
    bool test5_passed = (retrieved_errors == 5);
    print_test_result("获取所有错误", test5_passed);
    
    // 测试6: 验证错误内容
    bool errors_content_valid = true;
    for (size_t i = 0; i < retrieved_errors; i++)
    {
        if (errors[i].type != Eum_SEMANTIC_ERROR_TYPE_MISMATCH ||
            errors[i].line != i * 10 + 1 ||
            errors[i].column != i * 5 + 1) {
            errors_content_valid = false;
            break;
        }
    }
    bool test6_passed = errors_content_valid;
    print_test_result("验证错误内容", test6_passed);
    
    // 测试7: 获取所有警告
    Stru_SemanticError_t warnings[10];
    size_t retrieved_warnings = error_reporter->get_all_warnings(
        error_reporter, warnings, 10);
    bool test7_passed = (retrieved_warnings == 3);
    print_test_result("获取所有警告", test7_passed);
    
    // 测试8: 部分获取错误
    Stru_SemanticError_t partial_errors[2];
    size_t partial_retrieved = error_reporter->get_all_errors(
        error_reporter, partial_errors, 2);
    bool test8_passed = (partial_retrieved == 2);
    print_test_result("部分获取错误", test8_passed);
    
    // 测试9: 无效参数获取错误
    size_t null_result = error_reporter->get_all_errors(NULL, errors, 10);
    bool test9_passed = (null_result == 0);
    print_test_result("NULL报告器获取错误", test9_passed);
    
    // 测试10: 无效缓冲区获取错误
    null_result = error_reporter->get_all_errors(error_reporter, NULL, 10);
    bool test10_passed = (null_result == 0);
    print_test_result("NULL缓冲区获取错误", test10_passed);
    
    // 清理
    F_destroy_semantic_error_reporter_interface(error_reporter);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && 
           test5_passed && test6_passed && test7_passed && test8_passed && 
           test9_passed && test10_passed;
}

/**
 * @brief 测试错误限制
 */
static bool test_error_limits(void)
{
    printf("\n测试错误限制:\n");
    
    // 创建带限制的错误报告器
    Stru_SemanticErrorReporterInterface_t* limited_reporter = 
        F_create_semantic_error_reporter_interface();
    if (limited_reporter == NULL) {
        return false;
    }
    
    // 初始化带限制
    bool init_result = limited_reporter->initialize(limited_reporter, 3);
    if (!init_result) {
        F_destroy_semantic_error_reporter_interface(limited_reporter);
        return false;
    }
    
    // 测试1: 报告错误直到达到限制
    bool errors_within_limit = true;
    bool errors_beyond_limit = true;
    for (int i = 0; i < 5; i++)
    {
        char message[64];
        snprintf(message, sizeof(message), "限制测试错误 %d", i);
        
        Stru_SemanticError_t error = create_test_error(
            Eum_SEMANTIC_ERROR_REDEFINED_SYMBOL,
            message,
            i + 1, 1);
        
        bool result = limited_reporter->report_error(limited_reporter, &error);
        
        // 前3个应该成功，后2个应该失败
        if (i < 3) {
            if (!result) errors_within_limit = false;
        } else {
            if (result) errors_beyond_limit = false;
        }
    }
    
    bool test1_passed = errors_within_limit;
    print_test_result("限制内错误报告", test1_passed);
    
    bool test2_passed = errors_beyond_limit;
    print_test_result("超限错误报告（应该失败）", test2_passed);
    
    // 测试3: 验证错误数量不超过限制
    size_t error_count = limited_reporter->get_error_count(limited_reporter);
    bool test3_passed = (error_count == 3);
    print_test_result("验证错误数量不超过限制", test3_passed);
    
    // 测试4: 清空后应该可以继续报告
    limited_reporter->clear_all(limited_reporter);
    error_count = limited_reporter->get_error_count(limited_reporter);
    bool test4_passed = (error_count == 0);
    print_test_result("清空后错误数量为0", test4_passed);
    
    // 测试5: 再次报告应该成功
    Stru_SemanticError_t error = create_test_error(
        Eum_SEMANTIC_ERROR_INVALID_OPERATION,
        "清空后测试",
        1, 1);
    
    bool result = limited_reporter->report_error(limited_reporter, &error);
    bool test5_passed = result;
    print_test_result("清空后再次报告错误", test5_passed);
    
    // 测试6: 验证清空后错误数量
    error_count = limited_reporter->get_error_count(limited_reporter);
    bool test6_passed = (error_count == 1);
    print_test_result("验证清空后错误数量", test6_passed);
    
    // 清理
    F_destroy_semantic_error_reporter_interface(limited_reporter);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && 
           test5_passed && test6_passed;
}

/**
 * @brief 测试错误回调
 */
static bool test_error_callback(void)
{
    printf("\n测试错误回调:\n");
    
    // 创建错误报告器
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    if (error_reporter == NULL) {
        return false;
    }
    
    // 初始化
    bool init_result = error_reporter->initialize(error_reporter, 0);
    if (!init_result) {
        F_destroy_semantic_error_reporter_interface(error_reporter);
        return false;
    }
    
    // 回调测试变量
    static int callback_called = 0;
    static Stru_SemanticError_t last_error;
    
    // 定义回调函数
    void error_callback(const Stru_SemanticError_t* error, void* user_data)
    {
        (void)user_data;
        callback_called++;
        if (error != NULL)
        {
            last_error = *error;
        }
    }
    
    // 测试1: 设置回调
    error_reporter->set_error_callback(error_reporter, error_callback, NULL);
    bool test1_passed = true; // 设置回调应该成功
    print_test_result("设置错误回调", test1_passed);
    
    // 测试2: 报告错误，应该触发回调
    Stru_SemanticError_t error = create_test_error(
        Eum_SEMANTIC_ERROR_MISSING_RETURN,
        "函数缺少返回语句",
        25, 10);
    
    bool report_result = error_reporter->report_error(error_reporter, &error);
    bool test2_passed = report_result;
    print_test_result("报告错误触发回调", test2_passed);
    
    // 测试3: 验证回调被调用
    bool test3_passed = (callback_called == 1);
    print_test_result("验证回调被调用", test3_passed);
    
    // 测试4: 验证回调接收到的错误信息
    bool test4_passed = (last_error.type == Eum_SEMANTIC_ERROR_MISSING_RETURN &&
                        strcmp(last_error.message, "函数缺少返回语句") == 0 &&
                        last_error.line == 25 &&
                        last_error.column == 10);
    print_test_result("验证回调接收到的错误信息", test4_passed);
    
    // 测试5: 报告警告，不应该触发错误回调
    Stru_SemanticError_t warning = create_test_error(
        Eum_SEMANTIC_ERROR_INVALID_ARGUMENTS,
        "参数无效",
        30, 15);
    
    report_result = error_reporter->report_warning(error_reporter, &warning);
    bool test5_passed = report_result;
    print_test_result("报告警告", test5_passed);
    
    // 测试6: 回调次数不应增加
    bool test6_passed = (callback_called == 1);
    print_test_result("警告不应触发错误回调", test6_passed);
    
    // 测试7: 清空回调
    error_reporter->set_error_callback(error_reporter, NULL, NULL);
    callback_called = 0;
    
    // 再次报告错误，回调不应被调用
    report_result = error_reporter->report_error(error_reporter, &error);
    bool test7_passed = report_result;
    print_test_result("清空回调后报告错误", test7_passed);
    
    // 测试8: 验证回调未被调用
    bool test8_passed = (callback_called == 0);
    print_test_result("验证清空回调后回调未被调用", test8_passed);
    
    // 清理
    F_destroy_semantic_error_reporter_interface(error_reporter);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && 
           test5_passed && test6_passed && test7_passed && test8_passed;
}

/**
 * @brief 测试错误报告器边界情况
 */
static bool test_error_reporter_edge_cases(void)
{
    printf("\n测试错误报告器边界情况:\n");
    
    // 创建错误报告器
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    if (error_reporter == NULL) {
        return false;
    }
    
    // 测试1: 未初始化时的操作
    size_t error_count = error_reporter->get_error_count(error_reporter);
    bool test1_passed = (error_count == 0);
    print_test_result("未初始化时获取错误数量", test1_passed);
    
    size_t warning_count = error_reporter->get_warning_count(error_reporter);
    bool test2_passed = (warning_count == 0);
    print_test_result("未初始化时获取警告数量", test2_passed);
    
    bool has_errors = error_reporter->has_errors(error_reporter);
    bool test3_passed = !has_errors;
    print_test_result("未初始化时是否有错误", test3_passed);
    
    bool has_warnings = error_reporter->has_warnings(error_reporter);
    bool test4_passed = !has_warnings;
    print_test_result("未初始化时是否有警告", test4_passed);
    
    // 测试5: 清空未初始化的报告器
    error_reporter->clear_all(error_reporter);
    bool test5_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("清空未初始化的报告器", test5_passed);
    
    // 测试6: 验证所有错误类型枚举值
    bool test6_passed = (Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL == 0 &&
                        Eum_SEMANTIC_ERROR_REDEFINED_SYMBOL == 1 &&
                        Eum_SEMANTIC_ERROR_TYPE_MISMATCH == 2 &&
                        Eum_SEMANTIC_ERROR_INVALID_OPERATION == 3 &&
                        Eum_SEMANTIC_ERROR_INVALID_ARGUMENTS == 4 &&
                        Eum_SEMANTIC_ERROR_UNREACHABLE_CODE == 5 &&
                        Eum_SEMANTIC_ERROR_MISSING_RETURN == 6 &&
                        Eum_SEMANTIC_ERROR_INVALID_RETURN_TYPE == 7 &&
                        Eum_SEMANTIC_ERROR_UNINITIALIZED_VARIABLE == 8 &&
                        Eum_SEMANTIC_ERROR_CONST_ASSIGNMENT == 9 &&
                        Eum_SEMANTIC_ERROR_SCOPE_VIOLATION == 10 &&
                        Eum_SEMANTIC_ERROR_CIRCULAR_DEPENDENCY == 11 &&
                        Eum_SEMANTIC_ERROR_INVALID_IMPORT == 12 &&
                        Eum_SEMANTIC_ERROR_INVALID_EXPORT == 13);
    print_test_result("验证所有错误类型枚举值", test6_passed);
    
    // 清理
    F_destroy_semantic_error_reporter_interface(error_reporter);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && 
           test5_passed && test6_passed;
}

// ============================================================================
// 主测试函数
// ============================================================================

/**
 * @brief 错误报告器模块主测试函数
 * 
 * 运行所有错误报告器测试用例，并输出测试结果。
 * 
 * @return int 测试结果：0表示所有测试通过，非0表示有测试失败
 */
static int test_error_reporter_main(void)
{
    printf("========================================\n");
    printf("语义错误报告器模块单元测试\n");
    printf("========================================\n\n");
    
    int passed_count = 0;
    int total_count = 7; // 总共有7个测试用例
    
    // 运行测试用例1: 错误报告器创建和销毁
    printf("运行测试: 错误报告器创建和销毁\n");
    bool result1 = test_error_reporter_create_destroy();
    if (result1) {
        passed_count++;
    }
    printf("\n");
    
    // 运行测试用例2: 错误报告器初始化
    printf("运行测试: 错误报告器初始化\n");
    bool result2 = test_error_reporter_initialization();
    if (result2) {
        passed_count++;
    }
    printf("\n");
    
    // 运行测试用例3: 错误报告
    printf("运行测试: 错误报告\n");
    bool result3 = test_error_reporting();
    if (result3) {
        passed_count++;
    }
    printf("\n");
    
    // 运行测试用例4: 错误查询
    printf("运行测试: 错误查询\n");
    bool result4 = test_error_querying();
    if (result4) {
        passed_count++;
    }
    printf("\n");
    
    // 运行测试用例5: 错误限制
    printf("运行测试: 错误限制\n");
    bool result5 = test_error_limits();
    if (result5) {
        passed_count++;
    }
    printf("\n");
    
    // 运行测试用例6: 错误回调
    printf("运行测试: 错误回调\n");
    bool result6 = test_error_callback();
    if (result6) {
        passed_count++;
    }
    printf("\n");
    
    // 运行测试用例7: 错误报告器边界情况
    printf("运行测试: 错误报告器边界情况\n");
    bool result7 = test_error_reporter_edge_cases();
    if (result7) {
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
 * @brief 运行所有错误报告器测试
 */
int main(void)
{
    return test_error_reporter_main();
}
