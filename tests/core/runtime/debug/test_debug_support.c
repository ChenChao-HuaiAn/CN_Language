/**
 * @file test_debug_support.c
 * @brief 调试支持模块单元测试
 
 * 本文件包含调试支持模块的单元测试，测试调试支持的各项功能。
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
#include "../../../../src/core/runtime/debug/CN_debug_support_interface.h"
#include "../../../../src/core/runtime/execution/CN_execution_engine_interface.h"
#include "../../../../src/core/runtime/memory/CN_runtime_memory_interface.h"

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
 * @brief 创建测试用的执行引擎接口
 */
static Stru_ExecutionEngineInterface_t* create_test_execution_engine(void)
{
    // 创建内存管理接口
    Stru_RuntimeMemoryInterface_t* memory = F_create_runtime_memory_interface();
    if (memory == NULL) {
        return NULL;
    }
    
    if (!memory->initialize(memory, 1024 * 1024, false)) {
        memory->destroy(memory);
        return NULL;
    }
    
    // 创建执行引擎接口
    Stru_ExecutionEngineInterface_t* engine = F_create_execution_engine_interface();
    if (engine == NULL) {
        memory->destroy(memory);
        return NULL;
    }
    
    if (!engine->initialize(engine, memory, false)) {
        engine->destroy(engine);
        memory->destroy(memory);
        return NULL;
    }
    
    return engine;
}

// ============================================================================
// 测试用例
// ============================================================================

/**
 * @brief 测试调试支持接口创建和销毁
 */
static bool test_debug_support_create_destroy(void)
{
    printf("测试调试支持接口创建和销毁:\n");
    
    // 测试1: 创建调试支持接口
    Stru_DebugSupportInterface_t* debugger = F_create_debug_support_interface();
    bool test1_passed = (debugger != NULL);
    print_test_result("创建调试支持接口", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 创建执行引擎接口用于初始化
    Stru_ExecutionEngineInterface_t* engine = create_test_execution_engine();
    bool test2_passed = (engine != NULL);
    print_test_result("创建测试执行引擎接口", test2_passed);
    
    if (!test2_passed) {
        debugger->destroy(debugger);
        return false;
    }
    
    // 测试3: 初始化调试支持接口
    bool init_result = debugger->initialize(debugger, engine);
    bool test3_passed = init_result;
    print_test_result("初始化调试支持接口", test3_passed);
    
    // 测试4: 销毁调试支持接口
    debugger->destroy(debugger);
    bool test4_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁调试支持接口", test4_passed);
    
    // 清理执行引擎接口
    engine->destroy(engine);
    
    return test1_passed && test2_passed && test3_passed && test4_passed;
}

/**
 * @brief 测试断点管理功能
 */
static bool test_breakpoint_management(void)
{
    printf("\n测试断点管理功能:\n");
    
    // 创建调试支持接口和执行引擎接口
    Stru_DebugSupportInterface_t* debugger = F_create_debug_support_interface();
    if (debugger == NULL) {
        return false;
    }
    
    Stru_ExecutionEngineInterface_t* engine = create_test_execution_engine();
    if (engine == NULL) {
        debugger->destroy(debugger);
        return false;
    }
    
    // 初始化调试支持接口
    if (!debugger->initialize(debugger, engine)) {
        engine->destroy(engine);
        debugger->destroy(debugger);
        return false;
    }
    
    // 测试1: 设置断点
    Stru_Breakpoint_t breakpoint = {
        .module_name = "test_module.cn",
        .function_name = "test_function",
        .line_number = 42,
        .instruction_offset = 0,
        .enabled = true,
        .user_data = NULL
    };
    
    bool set_result = debugger->set_breakpoint(debugger, &breakpoint);
    bool test1_passed = set_result;
    print_test_result("设置断点", test1_passed);
    
    // 测试2: 清除断点
    bool clear_result = debugger->clear_breakpoint(debugger, 0); // 假设断点ID为0
    bool test2_passed = clear_result;
    print_test_result("清除断点", test2_passed);
    
    // 测试3: 设置无效断点
    Stru_Breakpoint_t invalid_breakpoint = {
        .module_name = NULL,
        .function_name = NULL,
        .line_number = 0,
        .instruction_offset = 0,
        .enabled = true,
        .user_data = NULL
    };
    
    bool invalid_set_result = debugger->set_breakpoint(debugger, &invalid_breakpoint);
    bool test3_passed = !invalid_set_result; // 应该失败
    print_test_result("设置无效断点（应该失败）", test3_passed);
    
    // 清理
    debugger->destroy(debugger);
    engine->destroy(engine);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试执行控制功能
 */
static bool test_execution_control(void)
{
    printf("\n测试执行控制功能:\n");
    
    // 创建调试支持接口和执行引擎接口
    Stru_DebugSupportInterface_t* debugger = F_create_debug_support_interface();
    if (debugger == NULL) {
        return false;
    }
    
    Stru_ExecutionEngineInterface_t* engine = create_test_execution_engine();
    if (engine == NULL) {
        debugger->destroy(debugger);
        return false;
    }
    
    // 初始化调试支持接口
    if (!debugger->initialize(debugger, engine)) {
        engine->destroy(engine);
        debugger->destroy(debugger);
        return false;
    }
    
    // 测试1: 单步执行
    bool step_result = debugger->step(debugger, 0); // 步入
    bool test1_passed = step_result;
    print_test_result("单步执行（步入）", test1_passed);
    
    // 测试2: 继续执行
    bool continue_result = debugger->continue_execution(debugger);
    bool test2_passed = continue_result;
    print_test_result("继续执行", test2_passed);
    
    // 测试3: 暂停执行
    bool pause_result = debugger->pause_execution(debugger);
    bool test3_passed = pause_result;
    print_test_result("暂停执行", test3_passed);
    
    // 清理
    debugger->destroy(debugger);
    engine->destroy(engine);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试状态检查功能
 */
static bool test_state_inspection(void)
{
    printf("\n测试状态检查功能:\n");
    
    // 创建调试支持接口和执行引擎接口
    Stru_DebugSupportInterface_t* debugger = F_create_debug_support_interface();
    if (debugger == NULL) {
        return false;
    }
    
    Stru_ExecutionEngineInterface_t* engine = create_test_execution_engine();
    if (engine == NULL) {
        debugger->destroy(debugger);
        return false;
    }
    
    // 初始化调试支持接口
    if (!debugger->initialize(debugger, engine)) {
        engine->destroy(engine);
        debugger->destroy(debugger);
        return false;
    }
    
    // 测试1: 获取调用栈
    void* call_stack[32];
    int stack_depth = debugger->get_call_stack(debugger, call_stack, 32);
    bool test1_passed = (stack_depth >= 0);
    print_test_result("获取调用栈", test1_passed);
    
    // 测试2: 检查变量
    char value_buffer[256];
    bool inspect_result = debugger->inspect_variable(debugger, "test_var", value_buffer, sizeof(value_buffer));
    bool test2_passed = !inspect_result; // 对于不存在的变量，应该失败
    print_test_result("检查不存在的变量（应该失败）", test2_passed);
    
    // 测试3: 设置变量
    bool set_result = debugger->set_variable(debugger, "test_var", "test_value");
    bool test3_passed = !set_result; // 对于不存在的变量，应该失败
    print_test_result("设置不存在的变量（应该失败）", test3_passed);
    
    // 清理
    debugger->destroy(debugger);
    engine->destroy(engine);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试事件处理功能
 */
static bool test_event_handling(void)
{
    printf("\n测试事件处理功能:\n");
    
    // 创建调试支持接口和执行引擎接口
    Stru_DebugSupportInterface_t* debugger = F_create_debug_support_interface();
    if (debugger == NULL) {
        return false;
    }
    
    Stru_ExecutionEngineInterface_t* engine = create_test_execution_engine();
    if (engine == NULL) {
        debugger->destroy(debugger);
        return false;
    }
    
    // 初始化调试支持接口
    if (!debugger->initialize(debugger, engine)) {
        engine->destroy(engine);
        debugger->destroy(debugger);
        return false;
    }
    
    // 测试1: 获取下一个事件（无事件，应该超时）
    Stru_DebugEvent_t event;
    bool get_event_result = debugger->get_next_event(debugger, &event, 100); // 100ms超时
    bool test1_passed = !get_event_result; // 应该超时返回false
    print_test_result("获取事件（应该超时）", test1_passed);
    
    // 测试2: 获取下一个事件（使用0超时）
    bool get_event_zero_result = debugger->get_next_event(debugger, &event, 0);
    bool test2_passed = !get_event_zero_result; // 应该立即返回false
    print_test_result("获取事件（0超时）", test2_passed);
    
    // 清理
    debugger->destroy(debugger);
    engine->destroy(engine);
    
    return test1_passed && test2_passed;
}

// ============================================================================
// 主测试函数
// ============================================================================

/**
 * @brief 运行所有调试支持测试
 */
int main(void)
{
    printf("========================================\n");
    printf("调试支持模块单元测试\n");
    printf("========================================\n\n");
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // 运行所有测试用例
    bool (*test_functions[])(void) = {
        test_debug_support_create_destroy,
        test_breakpoint_management,
        test_execution_control,
        test_state_inspection,
        test_event_handling
    };
    
    const char* test_names[] = {
        "调试支持接口创建和销毁",
        "断点管理功能",
        "执行控制功能",
        "状态检查功能",
        "事件处理功能"
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
