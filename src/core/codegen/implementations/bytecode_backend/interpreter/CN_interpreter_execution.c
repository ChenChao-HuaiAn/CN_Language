/******************************************************************************
 * 文件名: CN_interpreter_execution.c
 * 功能: CN_Language 字节码解释器程序执行控制模块
 * 
 * 实现字节码解释器的程序执行控制功能，包括加载、执行、单步、暂停、继续和停止。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_interpreter_execution.h"
#include "CN_interpreter_core.h"
#include "../CN_bytecode_interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ============================================================================
// 静态辅助函数声明
// ============================================================================

static bool F_check_interpreter_state_for_execution(const Stru_BytecodeInterpreter_t* interpreter, 
                                                   const char* operation);
static void F_add_error(Stru_BytecodeInterpreter_t* interpreter, const char* format, ...);

// ============================================================================
// 公共函数实现
// ============================================================================

/**
 * @brief 加载字节码程序
 */
bool F_load_bytecode_program(Stru_BytecodeInterpreter_t* interpreter,
                             Stru_BytecodeProgram_t* program)
{
    if (!interpreter || !program) {
        return false;
    }
    
    if (interpreter->state != Eum_BCI_STATE_INITIALIZED && 
        interpreter->state != Eum_BCI_STATE_FINISHED &&
        interpreter->state != Eum_BCI_STATE_ERROR) {
        F_add_error(interpreter, "解释器状态 %d 不允许加载程序", interpreter->state);
        return false;
    }
    
    // 重置解释器
    F_reset_bytecode_interpreter(interpreter);
    
    // 设置程序
    interpreter->program = program;
    
    // 初始化全局变量
    if (program->global_count > 0) {
        interpreter->globals = (Stru_BytecodeValue_t*)calloc(program->global_count, sizeof(Stru_BytecodeValue_t));
        if (!interpreter->globals) {
            F_add_error(interpreter, "无法分配全局变量内存");
            interpreter->program = NULL;
            return false;
        }
        interpreter->global_count = program->global_count;
    }
    
    // 重置程序计数器
    interpreter->program_counter = program->entry_point_offset;
    
    // 清除错误
    // 注意：这里调用外部函数，需要在主文件中实现
    // F_clear_interpreter_errors(interpreter);
    
    return true;
}

/**
 * @brief 执行字节码程序
 */
bool F_execute_bytecode_program(Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter || !interpreter->program) {
        return false;
    }
    
    if (!F_check_interpreter_state_for_execution(interpreter, "执行")) {
        return false;
    }
    
    // 设置状态
    interpreter->state = Eum_BCI_STATE_RUNNING;
    interpreter->start_time = 0; // 将在执行循环中设置
    interpreter->instruction_count = 0;
    
    // 注意：实际的执行循环需要在主文件中实现，因为它需要调用指令执行模块
    // 这里只进行状态检查和设置
    
    return true;
}

/**
 * @brief 单步执行字节码程序
 */
bool F_step_bytecode_program(Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter || !interpreter->program) {
        return false;
    }
    
    if (interpreter->state != Eum_BCI_STATE_PAUSED &&
        interpreter->state != Eum_BCI_STATE_BREAKPOINT &&
        interpreter->state != Eum_BCI_STATE_STEPPING) {
        F_add_error(interpreter, "解释器状态 %d 不允许单步执行", interpreter->state);
        return false;
    }
    
    // 设置状态
    interpreter->state = Eum_BCI_STATE_STEPPING;
    
    // 注意：实际的单步执行需要在主文件中实现
    
    return true;
}

/**
 * @brief 继续执行字节码程序
 */
bool F_continue_bytecode_program(Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter || !interpreter->program) {
        return false;
    }
    
    if (interpreter->state != Eum_BCI_STATE_PAUSED &&
        interpreter->state != Eum_BCI_STATE_BREAKPOINT) {
        F_add_error(interpreter, "解释器状态 %d 不允许继续执行", interpreter->state);
        return false;
    }
    
    // 继续执行
    return F_execute_bytecode_program(interpreter);
}

/**
 * @brief 暂停字节码程序执行
 */
bool F_pause_bytecode_program(Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter) {
        return false;
    }
    
    if (interpreter->state != Eum_BCI_STATE_RUNNING) {
        F_add_error(interpreter, "解释器状态 %d 不允许暂停", interpreter->state);
        return false;
    }
    
    interpreter->state = Eum_BCI_STATE_PAUSED;
    return true;
}

/**
 * @brief 停止字节码程序执行
 */
bool F_stop_bytecode_program(Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter) {
        return false;
    }
    
    if (interpreter->state != Eum_BCI_STATE_RUNNING &&
        interpreter->state != Eum_BCI_STATE_PAUSED &&
        interpreter->state != Eum_BCI_STATE_STEPPING &&
        interpreter->state != Eum_BCI_STATE_BREAKPOINT) {
        F_add_error(interpreter, "解释器状态 %d 不允许停止", interpreter->state);
        return false;
    }
    
    interpreter->state = Eum_BCI_STATE_TERMINATED;
    return true;
}

/**
 * @brief 获取性能分析数据
 */
void F_get_profiling_data(const Stru_BytecodeInterpreter_t* interpreter,
                          uint64_t* instruction_count,
                          uint64_t* execution_time,
                          size_t* memory_usage)
{
    if (!interpreter) {
        return;
    }
    
    if (instruction_count) {
        *instruction_count = interpreter->instruction_count;
    }
    
    if (execution_time) {
        *execution_time = interpreter->total_time;
    }
    
    if (memory_usage) {
        // 计算内存使用量
        size_t usage = 0;
        usage += interpreter->stack_capacity * sizeof(Stru_BytecodeValue_t);
        usage += interpreter->global_count * sizeof(Stru_BytecodeValue_t);
        usage += interpreter->call_stack_capacity * sizeof(void*);
        usage += interpreter->error_capacity * sizeof(char*);
        usage += interpreter->breakpoint_count * sizeof(uint32_t);
        
        *memory_usage = usage;
    }
}

// ============================================================================
// 静态辅助函数实现
// ============================================================================

/**
 * @brief 检查解释器状态是否允许执行
 */
static bool F_check_interpreter_state_for_execution(const Stru_BytecodeInterpreter_t* interpreter, 
                                                   const char* operation)
{
    if (!interpreter) {
        return false;
    }
    
    if (interpreter->state != Eum_BCI_STATE_INITIALIZED &&
        interpreter->state != Eum_BCI_STATE_PAUSED) {
        F_add_error((Stru_BytecodeInterpreter_t*)interpreter, 
                   "解释器状态 %d 不允许%s程序", interpreter->state, operation);
        return false;
    }
    
    return true;
}

/**
 * @brief 添加错误信息
 */
static void F_add_error(Stru_BytecodeInterpreter_t* interpreter, const char* format, ...)
{
    if (!interpreter || !format) {
        return;
    }
    
    // 简化实现：直接打印错误信息
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // 在实际实现中，这里应该将错误信息添加到解释器的错误数组中
    printf("错误: %s\n", buffer);
    interpreter->has_errors = true;
}
