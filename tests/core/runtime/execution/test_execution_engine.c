/**
 * @file test_execution_engine.c
 * @brief 执行引擎模块单元测试
 
 * 本文件包含执行引擎模块的单元测试，测试执行引擎的各项功能。
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
 * @brief 创建测试用的内存管理接口
 */
static Stru_RuntimeMemoryInterface_t* create_test_memory_interface(void)
{
    Stru_RuntimeMemoryInterface_t* memory = F_create_runtime_memory_interface();
    if (memory == NULL) {
        return NULL;
    }
    
    if (!memory->initialize(memory, 1024 * 1024, false)) {
        memory->destroy(memory);
        return NULL;
    }
    
    return memory;
}

// ============================================================================
// 测试用例
// ============================================================================

/**
 * @brief 测试执行引擎接口创建和销毁
 */
static bool test_execution_engine_create_destroy(void)
{
    printf("测试执行引擎接口创建和销毁:\n");
    
    // 测试1: 创建执行引擎接口
    Stru_ExecutionEngineInterface_t* engine = F_create_execution_engine_interface();
    bool test1_passed = (engine != NULL);
    print_test_result("创建执行引擎接口", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 创建内存管理接口用于初始化
    Stru_RuntimeMemoryInterface_t* memory = create_test_memory_interface();
    bool test2_passed = (memory != NULL);
    print_test_result("创建测试内存管理接口", test2_passed);
    
    if (!test2_passed) {
        engine->destroy(engine);
        return false;
    }
    
    // 测试3: 初始化执行引擎接口
    bool init_result = engine->initialize(engine, memory, false); // 禁用JIT
    bool test3_passed = init_result;
    print_test_result("初始化执行引擎接口", test3_passed);
    
    // 测试4: 销毁执行引擎接口
    engine->destroy(engine);
    bool test4_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁执行引擎接口", test4_passed);
    
    // 清理内存管理接口
    memory->destroy(memory);
    
    return test1_passed && test2_passed && test3_passed && test4_passed;
}

/**
 * @brief 测试执行引擎重置功能
 */
static bool test_execution_engine_reset(void)
{
    printf("\n测试执行引擎重置功能:\n");
    
    // 创建执行引擎和内存管理接口
    Stru_ExecutionEngineInterface_t* engine = F_create_execution_engine_interface();
    if (engine == NULL) {
        return false;
    }
    
    Stru_RuntimeMemoryInterface_t* memory = create_test_memory_interface();
    if (memory == NULL) {
        engine->destroy(engine);
        return false;
    }
    
    // 初始化执行引擎
    if (!engine->initialize(engine, memory, false)) {
        memory->destroy(memory);
        engine->destroy(engine);
        return false;
    }
    
    // 测试1: 重置执行引擎
    engine->reset(engine);
    bool test1_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("重置执行引擎", test1_passed);
    
    // 测试2: 重置后可以重新初始化
    bool reinit_result = engine->initialize(engine, memory, true); // 启用JIT
    bool test2_passed = reinit_result;
    print_test_result("重置后重新初始化", test2_passed);
    
    // 清理
    engine->destroy(engine);
    memory->destroy(memory);
    
    return test1_passed && test2_passed;
}

/**
 * @brief 测试执行统计功能
 */
static bool test_execution_statistics(void)
{
    printf("\n测试执行统计功能:\n");
    
    // 创建执行引擎和内存管理接口
    Stru_ExecutionEngineInterface_t* engine = F_create_execution_engine_interface();
    if (engine == NULL) {
        return false;
    }
    
    Stru_RuntimeMemoryInterface_t* memory = create_test_memory_interface();
    if (memory == NULL) {
        engine->destroy(engine);
        return false;
    }
    
    // 初始化执行引擎
    if (!engine->initialize(engine, memory, false)) {
        memory->destroy(memory);
        engine->destroy(engine);
        return false;
    }
    
    // 测试1: 获取执行统计
    uint64_t instructions_executed = 0;
    uint64_t execution_time_ms = 0;
    size_t memory_used = 0;
    
    engine->get_execution_stats(engine, &instructions_executed, &execution_time_ms, &memory_used);
    
    bool test1_passed = (instructions_executed == 0) && 
                       (execution_time_ms == 0) && 
                       (memory_used == 0);
    print_test_result("初始执行统计正确", test1_passed);
    
    // 测试2: 设置超时
    engine->set_timeout(engine, 1000); // 1秒超时
    bool test2_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("设置执行超时", test2_passed);
    
    // 清理
    engine->destroy(engine);
    memory->destroy(memory);
    
    return test1_passed && test2_passed;
}

/**
 * @brief 测试模块加载和卸载功能
 */
static bool test_module_load_unload(void)
{
    printf("\n测试模块加载和卸载功能:\n");
    
    // 创建执行引擎和内存管理接口
    Stru_ExecutionEngineInterface_t* engine = F_create_execution_engine_interface();
    if (engine == NULL) {
        return false;
    }
    
    Stru_RuntimeMemoryInterface_t* memory = create_test_memory_interface();
    if (memory == NULL) {
        engine->destroy(engine);
        return false;
    }
    
    // 初始化执行引擎
    if (!engine->initialize(engine, memory, false)) {
        memory->destroy(memory);
        engine->destroy(engine);
        return false;
    }
    
    // 测试1: 加载模块（使用空字节码）
    uint8_t empty_bytecode[] = {0x00, 0x01, 0x02, 0x03}; // 示例字节码
    void* module = engine->load_module(engine, empty_bytecode, sizeof(empty_bytecode), "test_module");
    
    bool test1_passed = (module != NULL);
    print_test_result("加载模块", test1_passed);
    
    // 测试2: 卸载模块
    if (module != NULL) {
        engine->unload_module(engine, module);
    }
    bool test2_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("卸载模块", test2_passed);
    
    // 测试3: 加载空模块
    void* empty_module = engine->load_module(engine, NULL, 0, "empty_module");
    bool test3_passed = (empty_module == NULL); // 应该失败
    print_test_result("加载空模块（应该失败）", test3_passed);
    
    // 清理
    engine->destroy(engine);
    memory->destroy(memory);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试字节码执行功能
 */
static bool test_bytecode_execution(void)
{
    printf("\n测试字节码执行功能:\n");
    
    // 创建执行引擎和内存管理接口
    Stru_ExecutionEngineInterface_t* engine = F_create_execution_engine_interface();
    if (engine == NULL) {
        return false;
    }
    
    Stru_RuntimeMemoryInterface_t* memory = create_test_memory_interface();
    if (memory == NULL) {
        engine->destroy(engine);
        return false;
    }
    
    // 初始化执行引擎
    if (!engine->initialize(engine, memory, false)) {
        memory->destroy(memory);
        engine->destroy(engine);
        return false;
    }
    
    // 测试1: 执行空字节码
    uint8_t simple_bytecode[] = {0x00, 0x01, 0x02, 0x03}; // 示例字节码
    void* result = engine->execute_bytecode(engine, simple_bytecode, sizeof(simple_bytecode), NULL);
    
    bool test1_passed = (result == NULL); // 对于示例字节码，应该返回NULL
    print_test_result("执行简单字节码", test1_passed);
    
    // 测试2: 执行空字节码
    void* empty_result = engine->execute_bytecode(engine, NULL, 0, NULL);
    bool test2_passed = (empty_result == NULL); // 应该失败
    print_test_result("执行空字节码（应该失败）", test2_passed);
    
    // 清理
    engine->destroy(engine);
    memory->destroy(memory);
    
    return test1_passed && test2_passed;
}

// ============================================================================
// 主测试函数
// ============================================================================

/**
 * @brief 运行所有执行引擎测试
 */
int main(void)
{
    printf("========================================\n");
    printf("执行引擎模块单元测试\n");
    printf("========================================\n\n");
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // 运行所有测试用例
    bool (*test_functions[])(void) = {
        test_execution_engine_create_destroy,
        test_execution_engine_reset,
        test_execution_statistics,
        test_module_load_unload,
        test_bytecode_execution
    };
    
    const char* test_names[] = {
        "执行引擎接口创建和销毁",
        "执行引擎重置功能",
        "执行统计功能",
        "模块加载和卸载功能",
        "字节码执行功能"
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
