/**
 * @file test_virtual_machine.c
 * @brief 虚拟机模块单元测试
 
 * 本文件包含虚拟机模块的单元测试，测试虚拟机的各项功能。
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
#include "../../../../src/core/runtime/vm/CN_virtual_machine_interface.h"
#include "../../../../src/core/runtime/memory/CN_runtime_memory_interface.h"
#include "../../../../src/core/runtime/execution/CN_execution_engine_interface.h"
#include "../../../../src/core/runtime/debug/CN_debug_support_interface.h"

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
 * @brief 测试虚拟机接口创建和销毁
 */
static bool test_virtual_machine_create_destroy(void)
{
    printf("测试虚拟机接口创建和销毁:\n");
    
    // 测试1: 创建虚拟机接口
    Stru_VirtualMachineInterface_t* vm = F_create_virtual_machine_interface();
    bool test1_passed = (vm != NULL);
    print_test_result("创建虚拟机接口", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 初始化虚拟机接口
    bool init_result = vm->initialize(vm, 1024 * 1024, false, false); // 1MB堆，无GC，无JIT
    bool test2_passed = init_result;
    print_test_result("初始化虚拟机接口", test2_passed);
    
    // 测试3: 销毁虚拟机接口
    vm->destroy(vm);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁虚拟机接口", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试虚拟机重置功能
 */
static bool test_virtual_machine_reset(void)
{
    printf("\n测试虚拟机重置功能:\n");
    
    // 创建虚拟机接口
    Stru_VirtualMachineInterface_t* vm = F_create_virtual_machine_interface();
    if (vm == NULL) {
        return false;
    }
    
    // 初始化虚拟机接口
    if (!vm->initialize(vm, 1024 * 1024, false, false)) {
        vm->destroy(vm);
        return false;
    }
    
    // 测试1: 重置虚拟机
    vm->reset(vm);
    bool test1_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("重置虚拟机", test1_passed);
    
    // 测试2: 重置后可以重新初始化
    bool reinit_result = vm->initialize(vm, 2048 * 1024, true, true); // 2MB堆，启用GC，启用JIT
    bool test2_passed = reinit_result;
    print_test_result("重置后重新初始化", test2_passed);
    
    // 清理
    vm->destroy(vm);
    
    return test1_passed && test2_passed;
}

/**
 * @brief 测试虚拟机组件访问功能
 */
static bool test_virtual_machine_component_access(void)
{
    printf("\n测试虚拟机组件访问功能:\n");
    
    // 创建虚拟机接口
    Stru_VirtualMachineInterface_t* vm = F_create_virtual_machine_interface();
    if (vm == NULL) {
        return false;
    }
    
    // 初始化虚拟机接口
    if (!vm->initialize(vm, 1024 * 1024, false, false)) {
        vm->destroy(vm);
        return false;
    }
    
    // 测试1: 获取内存管理接口
    Stru_RuntimeMemoryInterface_t* memory = vm->get_memory_interface(vm);
    bool test1_passed = (memory != NULL);
    print_test_result("获取内存管理接口", test1_passed);
    
    // 测试2: 获取执行引擎接口
    Stru_ExecutionEngineInterface_t* engine = vm->get_execution_engine(vm);
    bool test2_passed = (engine != NULL);
    print_test_result("获取执行引擎接口", test2_passed);
    
    // 测试3: 获取调试支持接口
    Stru_DebugSupportInterface_t* debugger = vm->get_debug_support(vm);
    bool test3_passed = (debugger != NULL);
    print_test_result("获取调试支持接口", test3_passed);
    
    // 测试4: 检查组件是否可用
    bool test4_passed = false;
    if (memory != NULL && engine != NULL && debugger != NULL) {
        // 测试内存管理接口功能
        size_t total_heap = 0;
        memory->get_memory_stats(memory, &total_heap, NULL, NULL, NULL);
        bool test4a_passed = (total_heap == 1024 * 1024);
        print_test_result("内存管理接口功能正常", test4a_passed);
        
        // 测试执行引擎接口功能
        uint64_t instructions = 0;
        engine->get_execution_stats(engine, &instructions, NULL, NULL);
        bool test4b_passed = (instructions == 0);
        print_test_result("执行引擎接口功能正常", test4b_passed);
        
        // 测试调试支持接口功能
        void* call_stack[32];
        int stack_depth = debugger->get_call_stack(debugger, call_stack, 32);
        bool test4c_passed = (stack_depth >= 0);
        print_test_result("调试支持接口功能正常", test4c_passed);
        
        test4_passed = test4a_passed && test4b_passed && test4c_passed;
    } else {
        print_test_result("组件功能测试", test4_passed);
    }
    
    // 清理
    vm->destroy(vm);
    
    return test1_passed && test2_passed && test3_passed && test4_passed;
}

/**
 * @brief 测试虚拟机程序执行功能
 */
static bool test_virtual_machine_program_execution(void)
{
    printf("\n测试虚拟机程序执行功能:\n");
    
    // 创建虚拟机接口
    Stru_VirtualMachineInterface_t* vm = F_create_virtual_machine_interface();
    if (vm == NULL) {
        return false;
    }
    
    // 初始化虚拟机接口
    if (!vm->initialize(vm, 1024 * 1024, false, false)) {
        vm->destroy(vm);
        return false;
    }
    
    // 测试1: 加载并执行空字节码
    uint8_t empty_bytecode[] = {0x00, 0x01, 0x02, 0x03}; // 示例字节码
    void* argv[] = {"arg1", "arg2"};
    int exit_code = vm->load_and_execute(vm, empty_bytecode, sizeof(empty_bytecode), 
                                        "test_module.cn", 2, argv);
    
    bool test1_passed = (exit_code == 0); // 对于示例字节码，应该返回0
    print_test_result("加载并执行简单字节码", test1_passed);
    
    // 测试2: 加载并执行空字节码
    int empty_exit_code = vm->load_and_execute(vm, NULL, 0, "empty_module.cn", 0, NULL);
    bool test2_passed = (empty_exit_code != 0); // 应该失败
    print_test_result("加载并执行空字节码（应该失败）", test2_passed);
    
    // 测试3: 加载并执行大字节码
    uint8_t large_bytecode[1024];
    for (int i = 0; i < 1024; i++) {
        large_bytecode[i] = i % 256;
    }
    
    int large_exit_code = vm->load_and_execute(vm, large_bytecode, sizeof(large_bytecode),
                                              "large_module.cn", 0, NULL);
    bool test3_passed = (large_exit_code == 0); // 应该成功
    print_test_result("加载并执行大字节码", test3_passed);
    
    // 清理
    vm->destroy(vm);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试虚拟机不同配置
 */
static bool test_virtual_machine_configurations(void)
{
    printf("\n测试虚拟机不同配置:\n");
    
    // 测试1: 启用垃圾回收的配置
    Stru_VirtualMachineInterface_t* vm_gc = F_create_virtual_machine_interface();
    bool test1_passed = (vm_gc != NULL);
    
    if (test1_passed) {
        bool init_gc = vm_gc->initialize(vm_gc, 1024 * 1024, true, false); // 启用GC，禁用JIT
        test1_passed = init_gc;
        if (init_gc) {
            Stru_RuntimeMemoryInterface_t* memory = vm_gc->get_memory_interface(vm_gc);
            if (memory != NULL) {
                // 分配一些对象
                void* obj1 = memory->allocate_object(memory, 100, 1);
                void* obj2 = memory->allocate_object(memory, 200, 2);
                test1_passed = (obj1 != NULL && obj2 != NULL);
            }
        }
        vm_gc->destroy(vm_gc);
    }
    print_test_result("启用垃圾回收配置", test1_passed);
    
    // 测试2: 启用JIT的配置
    Stru_VirtualMachineInterface_t* vm_jit = F_create_virtual_machine_interface();
    bool test2_passed = (vm_jit != NULL);
    
    if (test2_passed) {
        bool init_jit = vm_jit->initialize(vm_jit, 1024 * 1024, false, true); // 禁用GC，启用JIT
        test2_passed = init_jit;
        vm_jit->destroy(vm_jit);
    }
    print_test_result("启用JIT编译配置", test2_passed);
    
    // 测试3: 启用GC和JIT的配置
    Stru_VirtualMachineInterface_t* vm_both = F_create_virtual_machine_interface();
    bool test3_passed = (vm_both != NULL);
    
    if (test3_passed) {
        bool init_both = vm_both->initialize(vm_both, 2048 * 1024, true, true); // 启用GC和JIT
        test3_passed = init_both;
        vm_both->destroy(vm_both);
    }
    print_test_result("启用GC和JIT配置", test3_passed);
    
    // 测试4: 小堆配置
    Stru_VirtualMachineInterface_t* vm_small = F_create_virtual_machine_interface();
    bool test4_passed = (vm_small != NULL);
    
    if (test4_passed) {
        bool init_small = vm_small->initialize(vm_small, 64 * 1024, false, false); // 64KB堆
        test4_passed = init_small;
        vm_small->destroy(vm_small);
    }
    print_test_result("小堆配置（64KB）", test4_passed);
    
    return test1_passed && test2_passed && test3_passed && test4_passed;
}

// ============================================================================
// 主测试函数
// ============================================================================

/**
 * @brief 运行所有虚拟机测试
 */
int main(void)
{
    printf("========================================\n");
    printf("虚拟机模块单元测试\n");
    printf("========================================\n\n");
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // 运行所有测试用例
    bool (*test_functions[])(void) = {
        test_virtual_machine_create_destroy,
        test_virtual_machine_reset,
        test_virtual_machine_component_access,
        test_virtual_machine_program_execution,
        test_virtual_machine_configurations
    };
    
    const char* test_names[] = {
        "虚拟机接口创建和销毁",
        "虚拟机重置功能",
        "虚拟机组件访问功能",
        "虚拟机程序执行功能",
        "虚拟机不同配置"
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
