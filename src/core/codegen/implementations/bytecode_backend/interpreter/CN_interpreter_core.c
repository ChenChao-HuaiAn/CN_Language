/******************************************************************************
 * 文件名: CN_interpreter_core.c
 * 功能: CN_Language 字节码解释器核心管理模块
 * 
 * 实现字节码解释器的核心管理功能，包括创建、销毁、重置和状态管理。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_interpreter_core.h"
#include "../CN_bytecode_interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// 内部常量定义
// ============================================================================

#define CN_INTERPRETER_DEFAULT_STACK_SIZE 65536      // 默认栈大小：64KB
#define CN_INTERPRETER_DEFAULT_HEAP_SIZE 1048576     // 默认堆大小：1MB
#define CN_INTERPRETER_MAX_CALL_DEPTH 256            // 最大调用深度
#define CN_INTERPRETER_MAX_INSTRUCTIONS 1000000      // 最大指令执行数

// ============================================================================
// 内部结构体定义
// ============================================================================

/**
 * @brief 调用栈帧结构体
 * 
 * 表示函数调用栈帧的内部结构。
 */
typedef struct Stru_CallFrame_t {
    uint32_t return_address;         ///< 返回地址（指令偏移）
    uint32_t base_pointer;           ///< 基址指针（栈索引）
    const char* function_name;       ///< 函数名
    uint32_t line_number;            ///< 行号
    void* local_variables;           ///< 局部变量
} Stru_CallFrame_t;

/**
 * @brief 内部状态结构体
 * 
 * 字节码解释器的内部状态，不对外暴露。
 */
typedef struct Stru_InternalState_t {
    // 内存管理
    void* heap_memory;               ///< 堆内存区域
    size_t heap_used;                ///< 已使用的堆内存
    size_t heap_capacity;            ///< 堆容量
    
    // 垃圾回收
    Stru_BytecodeValue_t** gc_roots; ///< GC根对象数组
    size_t gc_root_count;            ///< GC根对象数量
    size_t gc_root_capacity;         ///< GC根对象数组容量
    
    // 性能分析
    uint64_t* instruction_times;     ///< 指令执行时间统计
    size_t* instruction_counts;      ///< 指令执行次数统计
    
    // 调试信息
    bool tracing_enabled;            ///< 是否启用跟踪
    FILE* trace_file;                ///< 跟踪文件
} Stru_InternalState_t;

// ============================================================================
// 静态辅助函数声明
// ============================================================================

static Stru_InternalState_t* F_create_internal_state(const Stru_BytecodeInterpreterConfig_t* config);
static void F_destroy_internal_state(Stru_InternalState_t* state);
static uint64_t F_get_current_time_ms(void);

// ============================================================================
// 公共函数实现
// ============================================================================

/**
 * @brief 创建字节码解释器
 */
Stru_BytecodeInterpreter_t* F_create_bytecode_interpreter(
    const Stru_BytecodeInterpreterConfig_t* config)
{
    if (!config) {
        return NULL;
    }
    
    // 分配解释器内存
    Stru_BytecodeInterpreter_t* interpreter = (Stru_BytecodeInterpreter_t*)malloc(sizeof(Stru_BytecodeInterpreter_t));
    if (!interpreter) {
        return NULL;
    }
    
    // 初始化基本字段
    memset(interpreter, 0, sizeof(Stru_BytecodeInterpreter_t));
    interpreter->state = Eum_BCI_STATE_CREATED;
    
    // 创建内部状态
    interpreter->internal_state = F_create_internal_state(config);
    if (!interpreter->internal_state) {
        free(interpreter);
        return NULL;
    }
    
    // 初始化栈
    size_t stack_size = config->stack_size > 0 ? config->stack_size : CN_INTERPRETER_DEFAULT_STACK_SIZE;
    interpreter->stack = (Stru_BytecodeValue_t*)malloc(stack_size * sizeof(Stru_BytecodeValue_t));
    if (!interpreter->stack) {
        F_destroy_internal_state((Stru_InternalState_t*)interpreter->internal_state);
        free(interpreter);
        return NULL;
    }
    
    interpreter->stack_capacity = stack_size;
    interpreter->stack_top = 0;
    
    // 初始化调用栈
    size_t call_stack_size = config->max_call_depth > 0 ? config->max_call_depth : CN_INTERPRETER_MAX_CALL_DEPTH;
    interpreter->call_stack = (void**)malloc(call_stack_size * sizeof(void*));
    if (!interpreter->call_stack) {
        free(interpreter->stack);
        F_destroy_internal_state((Stru_InternalState_t*)interpreter->internal_state);
        free(interpreter);
        return NULL;
    }
    
    interpreter->call_stack_capacity = call_stack_size;
    interpreter->call_stack_depth = 0;
    
    // 初始化错误数组
    interpreter->error_capacity = 16;
    interpreter->error_messages = (char**)malloc(interpreter->error_capacity * sizeof(char*));
    if (!interpreter->error_messages) {
        free(interpreter->call_stack);
        free(interpreter->stack);
        F_destroy_internal_state((Stru_InternalState_t*)interpreter->internal_state);
        free(interpreter);
        return NULL;
    }
    
    interpreter->error_count = 0;
    
    // 初始化断点数组
    interpreter->breakpoint_count = 0;
    interpreter->breakpoints = NULL;
    interpreter->breakpoints_enabled = config->enable_debugging;
    
    // 初始化性能分析数据
    interpreter->instruction_count = 0;
    interpreter->start_time = 0;
    interpreter->total_time = 0;
    
    interpreter->state = Eum_BCI_STATE_INITIALIZED;
    return interpreter;
}

/**
 * @brief 销毁字节码解释器
 */
void F_destroy_bytecode_interpreter(Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter) {
        return;
    }
    
    // 释放程序（如果已加载）
    if (interpreter->program) {
        // 注意：这里不调用F_destroy_bytecode_program，因为程序可能由外部管理
        interpreter->program = NULL;
    }
    
    // 释放栈
    if (interpreter->stack) {
        // 释放栈上的所有值
        for (size_t i = 0; i < interpreter->stack_top; i++) {
            // 这里应该释放值的资源，但为了简化先不实现
        }
        free(interpreter->stack);
    }
    
    // 释放全局变量
    if (interpreter->globals) {
        free(interpreter->globals);
    }
    
    // 释放调用栈
    if (interpreter->call_stack) {
        free(interpreter->call_stack);
    }
    
    // 释放断点数组
    if (interpreter->breakpoints) {
        free(interpreter->breakpoints);
    }
    
    // 释放错误信息
    if (interpreter->error_messages) {
        for (size_t i = 0; i < interpreter->error_count; i++) {
            if (interpreter->error_messages[i]) {
                free(interpreter->error_messages[i]);
            }
        }
        free(interpreter->error_messages);
    }
    
    // 释放内部状态
    if (interpreter->internal_state) {
        F_destroy_internal_state((Stru_InternalState_t*)interpreter->internal_state);
    }
    
    // 释放解释器本身
    free(interpreter);
}

/**
 * @brief 重置字节码解释器
 */
void F_reset_bytecode_interpreter(Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter) {
        return;
    }
    
    // 重置状态
    interpreter->state = Eum_BCI_STATE_INITIALIZED;
    interpreter->has_errors = false;
    interpreter->has_warnings = false;
    
    // 重置程序计数器
    interpreter->program_counter = 0;
    interpreter->call_stack_depth = 0;
    
    // 重置栈
    interpreter->stack_top = 0;
    
    // 重置全局变量
    if (interpreter->globals) {
        for (size_t i = 0; i < interpreter->global_count; i++) {
            // 重置全局变量值
            interpreter->globals[i].type = Eum_BCV_NULL;
        }
    }
    
    // 重置性能分析数据
    interpreter->instruction_count = 0;
    interpreter->start_time = 0;
    interpreter->total_time = 0;
    
    // 清除错误
    // 注意：这里调用外部函数，需要在主文件中实现
    // F_clear_interpreter_errors(interpreter);
    
    // 清除断点
    // 注意：这里调用外部函数，需要在主文件中实现
    // F_clear_all_breakpoints(interpreter);
}

/**
 * @brief 获取解释器状态
 */
Eum_BytecodeInterpreterState F_get_interpreter_state(
    const Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter) {
        return Eum_BCI_STATE_ERROR;
    }
    
    return interpreter->state;
}

/**
 * @brief 检查解释器是否有错误
 */
bool F_interpreter_has_errors(const Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter) {
        return false;
    }
    
    return interpreter->has_errors;
}

/**
 * @brief 创建默认字节码解释器配置
 */
Stru_BytecodeInterpreterConfig_t F_create_default_bytecode_interpreter_config(void)
{
    Stru_BytecodeInterpreterConfig_t config;
    memset(&config, 0, sizeof(config));
    
    config.stack_size = CN_INTERPRETER_DEFAULT_STACK_SIZE;
    config.heap_size = CN_INTERPRETER_DEFAULT_HEAP_SIZE;
    config.max_call_depth = CN_INTERPRETER_MAX_CALL_DEPTH;
    config.max_instruction_count = CN_INTERPRETER_MAX_INSTRUCTIONS;
    config.enable_gc = true;
    config.enable_profiling = false;
    config.enable_debugging = true;
    config.enable_tracing = false;
    config.log_level = "warning";
    
    return config;
}

/**
 * @brief 获取字节码解释器版本信息
 */
void F_get_bytecode_interpreter_version(int* major, int* minor, int* patch)
{
    if (major) *major = 1;
    if (minor) *minor = 0;
    if (patch) *patch = 0;
}

/**
 * @brief 获取字节码解释器版本字符串
 */
const char* F_get_bytecode_interpreter_version_string(void)
{
    return "CN_Language Bytecode Interpreter v1.0.0";
}

// ============================================================================
// 静态辅助函数实现
// ============================================================================

/**
 * @brief 创建内部状态
 */
static Stru_InternalState_t* F_create_internal_state(const Stru_BytecodeInterpreterConfig_t* config)
{
    Stru_InternalState_t* state = (Stru_InternalState_t*)malloc(sizeof(Stru_InternalState_t));
    if (!state) {
        return NULL;
    }
    
    memset(state, 0, sizeof(Stru_InternalState_t));
    
    // 初始化堆内存
    size_t heap_size = config->heap_size > 0 ? config->heap_size : CN_INTERPRETER_DEFAULT_HEAP_SIZE;
    state->heap_memory = malloc(heap_size);
    if (!state->heap_memory) {
        free(state);
        return NULL;
    }
    
    state->heap_capacity = heap_size;
    state->heap_used = 0;
    
    // 初始化GC根数组
    state->gc_root_capacity = 16;
    state->gc_roots = (Stru_BytecodeValue_t**)malloc(state->gc_root_capacity * sizeof(Stru_BytecodeValue_t*));
    if (!state->gc_roots) {
        free(state->heap_memory);
        free(state);
        return NULL;
    }
    
    state->gc_root_count = 0;
    
    // 初始化性能分析数据
    state->instruction_times = NULL;
    state->instruction_counts = NULL;
    
    // 初始化调试信息
    state->tracing_enabled = config->enable_tracing;
    state->trace_file = NULL;
    
    if (state->tracing_enabled) {
        state->trace_file = fopen("bytecode_trace.log", "w");
    }
    
    return state;
}

/**
 * @brief 销毁内部状态
 */
static void F_destroy_internal_state(Stru_InternalState_t* state)
{
    if (!state) {
        return;
    }
    
    // 关闭跟踪文件
    if (state->trace_file) {
        fclose(state->trace_file);
    }
    
    // 释放堆内存
    if (state->heap_memory) {
        free(state->heap_memory);
    }
    
    // 释放GC根数组
    if (state->gc_roots) {
        free(state->gc_roots);
    }
    
    // 释放性能分析数据
    if (state->instruction_times) {
        free(state->instruction_times);
    }
    
    if (state->instruction_counts) {
        free(state->instruction_counts);
    }
    
    free(state);
}

/**
 * @brief 获取当前时间（毫秒）
 */
static uint64_t F_get_current_time_ms(void)
{
    // 简化实现：使用clock()函数
    return (uint64_t)(clock() * 1000 / CLOCKS_PER_SEC);
}
