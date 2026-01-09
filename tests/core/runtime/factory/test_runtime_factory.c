/**
 * @file test_runtime_factory.c
 * @brief 运行时工厂模块单元测试
 
 * 本文件包含运行时工厂模块的单元测试，测试主运行时接口的各项功能。
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
#include "../../../../src/core/runtime/factory/CN_runtime_factory.h"
#include "../../../../src/core/runtime/memory/CN_runtime_memory_interface.h"
#include "../../../../src/core/runtime/execution/CN_execution_engine_interface.h"
#include "../../../../src/core/runtime/debug/CN_debug_support_interface.h"
#include "../../../../src/core/runtime/vm/CN_virtual_machine_interface.h"

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
 * @brief 测试主运行时接口创建和销毁
 */
static bool test_runtime_interface_create_destroy(void)
{
    printf("测试主运行时接口创建和销毁:\n");
    
    // 测试1: 创建主运行时接口
    Stru_RuntimeInterface_t* runtime = F_create_runtime_interface();
    bool test1_passed = (runtime != NULL);
    print_test_result("创建主运行时接口", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 初始化主运行时接口
    bool init_result = runtime->initialize(runtime, NULL); // 使用默认配置
    bool test2_passed = init_result;
    print_test_result("初始化主运行时接口", test2_passed);
    
    // 测试3: 销毁主运行时接口
    runtime->destroy(runtime);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁主运行时接口", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试运行时接口重置功能
 */
static bool test_runtime_interface_reset(void)
{
    printf("\n测试运行时接口重置功能:\n");
    
    // 创建主运行时接口
    Stru_RuntimeInterface_t* runtime = F_create_runtime_interface();
    if (runtime == NULL) {
        return false;
    }
    
    // 初始化主运行时接口
    if (!runtime->initialize(runtime, NULL)) {
        runtime->destroy(runtime);
        return false;
    }
    
    // 测试1: 重置运行时接口
    runtime->reset(runtime);
    bool test1_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("重置运行时接口", test1_passed);
    
    // 测试2: 重置后可以重新初始化
    const char* config = "{\"heap_size\": 2048 * 1024, \"use_gc\": true, \"use_jit\": false}";
    bool reinit_result = runtime->initialize(runtime, config);
    bool test2_passed = reinit_result;
    print_test_result("重置后重新初始化", test2_passed);
    
    // 清理
    runtime->destroy(runtime);
    
    return test1_passed && test2_passed;
}

/**
 * @brief 测试运行时接口组件访问功能
 */
static bool test_runtime_interface_component_access(void)
{
    printf("\n测试运行时接口组件访问功能:\n");
    
    // 创建主运行时接口
    Stru_RuntimeInterface_t* runtime = F_create_runtime_interface();
    if (runtime == NULL) {
        return false;
    }
    
    // 初始化主运行时接口
    if (!runtime->initialize(runtime, NULL)) {
        runtime->destroy(runtime);
        return false;
    }
    
    // 测试1: 获取内存管理接口
    Stru_RuntimeMemoryInterface_t* memory = runtime->get_memory_interface(runtime);
    bool test1_passed = (memory != NULL);
    print_test_result("获取内存管理接口", test1_passed);
    
    // 测试2: 获取执行引擎接口
    Stru_ExecutionEngineInterface_t* engine = runtime->get_execution_engine(runtime);
    bool test2_passed = (engine != NULL);
    print_test_result("获取执行引擎接口", test2_passed);
    
    // 测试3: 获取调试支持接口
    Stru_DebugSupportInterface_t* debugger = runtime->get_debug_support(runtime);
    bool test3_passed = (debugger != NULL);
    print_test_result("获取调试支持接口", test3_passed);
    
    // 测试4: 获取虚拟机接口
    Stru_VirtualMachineInterface_t* vm = runtime->get_virtual_machine(runtime);
    bool test4_passed = (vm != NULL);
    print_test_result("获取虚拟机接口", test4_passed);
    
    // 测试5: 检查组件是否可用
    bool test5_passed = false;
    if (memory != NULL && engine != NULL && debugger != NULL && vm != NULL) {
        // 测试内存管理接口功能
        size_t total_heap = 0;
        memory->get_memory_stats(memory, &total_heap, NULL, NULL, NULL);
        bool test5a_passed = (total_heap > 0);
        print_test_result("内存管理接口功能正常", test5a_passed);
        
        // 测试执行引擎接口功能
        uint64_t instructions = 0;
        engine->get_execution_stats(engine, &instructions, NULL, NULL);
        bool test5b_passed = (instructions == 0);
        print_test_result("执行引擎接口功能正常", test5b_passed);
        
        // 测试调试支持接口功能
        void* call_stack[32];
        int stack_depth = debugger->get_call_stack(debugger, call_stack, 32);
        bool test5c_passed = (stack_depth >= 0);
        print_test_result("调试支持接口功能正常", test5c_passed);
        
        // 测试虚拟机接口功能 - 检查是否已经正确初始化
        // 虚拟机已经在工厂中被初始化，所以尝试再次初始化应该失败
        bool reinit_attempt = vm->initialize(vm, 1024 * 1024, false, false);
        bool test5d_passed = !reinit_attempt; // 重新初始化应该失败，因为已经初始化了
        print_test_result("虚拟机接口功能正常（已初始化）", test5d_passed);
        
        test5_passed = test5a_passed && test5b_passed && test5c_passed && test5d_passed;
    } else {
        print_test_result("组件功能测试", test5_passed);
    }
    
    // 清理
    runtime->destroy(runtime);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && test5_passed;
}

/**
 * @brief 测试运行时接口程序执行功能
 */
static bool test_runtime_interface_program_execution(void)
{
    printf("\n测试运行时接口程序执行功能:\n");
    
    // 创建主运行时接口
    Stru_RuntimeInterface_t* runtime = F_create_runtime_interface();
    if (runtime == NULL) {
        return false;
    }
    
    // 初始化主运行时接口
    if (!runtime->initialize(runtime, NULL)) {
        runtime->destroy(runtime);
        return false;
    }
    
    // 测试1: 执行简单字节码
    uint8_t simple_bytecode[] = {0x00, 0x01, 0x02, 0x03}; // 示例字节码
    char* argv[] = {"test_program", "arg1", "arg2"};
    int exit_code = runtime->execute_program(runtime, simple_bytecode, sizeof(simple_bytecode), 
                                            3, argv);
    
    bool test1_passed = (exit_code == 0); // 对于示例字节码，应该返回0
    print_test_result("执行简单字节码程序", test1_passed);
    
    // 测试2: 执行空字节码
    int empty_exit_code = runtime->execute_program(runtime, NULL, 0, 0, NULL);
    bool test2_passed = (empty_exit_code != 0); // 应该失败
    print_test_result("执行空字节码程序（应该失败）", test2_passed);
    
    // 测试3: 执行大字节码
    uint8_t large_bytecode[2048];
    for (int i = 0; i < 2048; i++) {
        large_bytecode[i] = i % 256;
    }
    
    int large_exit_code = runtime->execute_program(runtime, large_bytecode, sizeof(large_bytecode),
                                                  0, NULL);
    bool test3_passed = (large_exit_code == 0); // 应该成功
    print_test_result("执行大字节码程序", test3_passed);
    
    // 清理
    runtime->destroy(runtime);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试运行时接口不同配置
 */
static bool test_runtime_interface_configurations(void)
{
    printf("\n测试运行时接口不同配置:\n");
    
    // 测试1: 默认配置
    Stru_RuntimeInterface_t* runtime_default = F_create_runtime_interface();
    bool test1_passed = (runtime_default != NULL);
    
    if (test1_passed) {
        bool init_default = runtime_default->initialize(runtime_default, NULL);
        test1_passed = init_default;
        runtime_default->destroy(runtime_default);
    }
    print_test_result("默认配置", test1_passed);
    
    // 测试2: 启用垃圾回收的配置
    Stru_RuntimeInterface_t* runtime_gc = F_create_runtime_interface();
    bool test2_passed = (runtime_gc != NULL);
    
    if (test2_passed) {
        const char* gc_config = "{\"heap_size\": 1024 * 1024, \"use_gc\": true, \"use_jit\": false}";
        bool init_gc = runtime_gc->initialize(runtime_gc, gc_config);
        test2_passed = init_gc;
        runtime_gc->destroy(runtime_gc);
    }
    print_test_result("启用垃圾回收配置", test2_passed);
    
    // 测试3: 启用JIT的配置
    Stru_RuntimeInterface_t* runtime_jit = F_create_runtime_interface();
    bool test3_passed = (runtime_jit != NULL);
    
    if (test3_passed) {
        const char* jit_config = "{\"heap_size\": 1024 * 1024, \"use_gc\": false, \"use_jit\": true}";
        bool init_jit = runtime_jit->initialize(runtime_jit, jit_config);
        test3_passed = init_jit;
        runtime_jit->destroy(runtime_jit);
    }
    print_test_result("启用JIT编译配置", test3_passed);
    
    // 测试4: 启用调试的配置
    Stru_RuntimeInterface_t* runtime_debug = F_create_runtime_interface();
    bool test4_passed = (runtime_debug != NULL);
    
    if (test4_passed) {
        const char* debug_config = "{\"heap_size\": 1024 * 1024, \"use_gc\": false, \"use_jit\": false, \"debug_enabled\": true}";
        bool init_debug = runtime_debug->initialize(runtime_debug, debug_config);
        test4_passed = init_debug;
        runtime_debug->destroy(runtime_debug);
    }
    print_test_result("启用调试配置", test4_passed);
    
    // 测试5: 小堆配置
    Stru_RuntimeInterface_t* runtime_small = F_create_runtime_interface();
    bool test5_passed = (runtime_small != NULL);
    
    if (test5_passed) {
        const char* small_config = "{\"heap_size\": 64 * 1024, \"use_gc\": false, \"use_jit\": false}";
        bool init_small = runtime_small->initialize(runtime_small, small_config);
        test5_passed = init_small;
        runtime_small->destroy(runtime_small);
    }
    print_test_result("小堆配置（64KB）", test5_passed);
    
    return test1_passed && test2_passed && test3_passed && test4_passed && test5_passed;
}

// ============================================================================
// 主测试函数
// ============================================================================

/**
 * @brief 运行所有运行时工厂测试
 */
int main(void)
{
    printf("========================================\n");
    printf("运行时工厂模块单元测试\n");
    printf("========================================\n\n");
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // 运行所有测试用例
    bool (*test_functions[])(void) = {
        test_runtime_interface_create_destroy,
        test_runtime_interface_reset,
        test_runtime_interface_component_access,
        test_runtime_interface_program_execution,
        test_runtime_interface_configurations
    };
    
    const char* test_names[] = {
        "主运行时接口创建和销毁",
        "运行时接口重置功能",
        "运行时接口组件访问功能",
        "运行时接口程序执行功能",
        "运行时接口不同配置"
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
