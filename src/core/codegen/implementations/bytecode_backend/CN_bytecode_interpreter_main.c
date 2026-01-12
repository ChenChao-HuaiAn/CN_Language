/******************************************************************************
 * 文件名: CN_bytecode_interpreter_main.c
 * 功能: CN_Language 字节码解释器主文件（精简版）
 * 
 * 整合所有解释器模块，提供完整的字节码解释器功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 此版本已精简，只保留核心调度功能，具体指令实现已移到专门模块。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 精简版本，从原1226行文件拆分，现在不超过500行
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_bytecode_interpreter.h"
#include "interpreter/CN_interpreter_core.h"
#include "interpreter/CN_interpreter_execution.h"
#include "interpreter/CN_interpreter_debug.h"
#include "interpreter/CN_interpreter_stack.h"
#include "interpreter/CN_interpreter_engine.h"
#include "interpreter/CN_interpreter_instructions.h"
#include "CN_bytecode_instructions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ============================================================================
// 外部函数声明（从其他模块导入）
// ============================================================================

// 从CN_interpreter_stack.c导入
extern bool F_push_value(Stru_BytecodeInterpreter_t* interpreter, Stru_BytecodeValue_t value);
extern Stru_BytecodeValue_t F_pop_value(Stru_BytecodeInterpreter_t* interpreter);
extern Stru_BytecodeValue_t F_peek_value(Stru_BytecodeInterpreter_t* interpreter, size_t offset);
extern bool F_ensure_stack_capacity(Stru_BytecodeInterpreter_t* interpreter, size_t required_capacity);
extern bool F_ensure_call_stack_capacity(Stru_BytecodeInterpreter_t* interpreter, size_t required_capacity);
extern bool F_ensure_error_capacity(Stru_BytecodeInterpreter_t* interpreter, size_t required_capacity);

// 从CN_interpreter_debug.c导入
extern void F_add_error_internal(Stru_BytecodeInterpreter_t* interpreter, const char* format, ...);
extern void F_get_interpreter_errors(const Stru_BytecodeInterpreter_t* interpreter,
                                     char*** errors, size_t* error_count);
extern void F_clear_interpreter_errors(Stru_BytecodeInterpreter_t* interpreter);
extern bool F_set_breakpoint(Stru_BytecodeInterpreter_t* interpreter,
                             uint32_t instruction_offset);
extern bool F_clear_breakpoint(Stru_BytecodeInterpreter_t* interpreter,
                               uint32_t instruction_offset);
extern void F_clear_all_breakpoints(Stru_BytecodeInterpreter_t* interpreter);
extern bool F_get_current_stack_frame(const Stru_BytecodeInterpreter_t* interpreter,
                                      size_t frame_index,
                                      const char** function_name,
                                      uint32_t* line_number,
                                      uint32_t* instruction_offset);
extern bool F_get_interpreter_stack_trace(const Stru_BytecodeInterpreter_t* interpreter,
                                          void*** frames, size_t* frame_count);

// 从CN_interpreter_engine.c导入
extern Stru_BytecodeValue_t F_execute_arithmetic_instruction(Stru_BytecodeInterpreter_t* interpreter, 
                                                            const Stru_BytecodeInstruction_t* instruction);
extern Stru_BytecodeValue_t F_execute_comparison_instruction(Stru_BytecodeInterpreter_t* interpreter, 
                                                            const Stru_BytecodeInstruction_t* instruction);
extern Stru_BytecodeValue_t F_execute_logical_instruction(Stru_BytecodeInterpreter_t* interpreter, 
                                                         const Stru_BytecodeInstruction_t* instruction);

// ============================================================================
// 静态辅助函数声明
// ============================================================================

static Stru_BytecodeValue_t F_execute_instruction(Stru_BytecodeInterpreter_t* interpreter, 
                                                 const Stru_BytecodeInstruction_t* instruction);

// ============================================================================
// 核心解释器函数实现
// ============================================================================

/**
 * @brief 执行指令（核心调度函数）
 */
static Stru_BytecodeValue_t F_execute_instruction(Stru_BytecodeInterpreter_t* interpreter, 
                                                 const Stru_BytecodeInstruction_t* instruction)
{
    Stru_BytecodeValue_t result;
    memset(&result, 0, sizeof(result));
    result.type = Eum_BCV_NULL;
    
    if (!interpreter || !instruction) {
        return result;
    }
    
    // 根据操作码分类执行指令
    switch (instruction->opcode) {
        // 基本操作指令
        case Eum_BC_NOP:
        case Eum_BC_LOAD_CONST:
        case Eum_BC_POP:
        case Eum_BC_PUSH_NULL:
        case Eum_BC_PUSH_TRUE:
        case Eum_BC_PUSH_FALSE:
        case Eum_BC_DUP:
        case Eum_BC_SWAP:
            return F_execute_basic_instruction(interpreter, instruction);
            
        // 算术指令 - 委托给CN_interpreter_engine.c
        case Eum_BC_ADD:
        case Eum_BC_SUB:
        case Eum_BC_MUL:
        case Eum_BC_DIV:
        case Eum_BC_MOD:
        case Eum_BC_NEG:
        case Eum_BC_INC:
        case Eum_BC_DEC:
            return F_execute_arithmetic_instruction(interpreter, instruction);
            
        // 比较指令 - 委托给CN_interpreter_engine.c
        case Eum_BC_EQ:
        case Eum_BC_NE:
        case Eum_BC_LT:
        case Eum_BC_GT:
        case Eum_BC_LE:
        case Eum_BC_GE:
            return F_execute_comparison_instruction(interpreter, instruction);
            
        // 逻辑指令 - 委托给CN_interpreter_engine.c
        case Eum_BC_NOT:
        case Eum_BC_AND:
        case Eum_BC_OR:
            return F_execute_logical_instruction(interpreter, instruction);
            
        // 控制流指令
        case Eum_BC_JUMP:
        case Eum_BC_JUMP_IF_TRUE:
        case Eum_BC_JUMP_IF_FALSE:
        case Eum_BC_CALL:
        case Eum_BC_RETURN:
        case Eum_BC_RETURN_VOID:
            return F_execute_control_flow_instruction(interpreter, instruction);
            
        // 变量操作指令
        case Eum_BC_LOAD_LOCAL:
        case Eum_BC_STORE_LOCAL:
        case Eum_BC_LOAD_GLOBAL:
        case Eum_BC_STORE_GLOBAL:
            return F_execute_variable_instruction(interpreter, instruction);
            
        // 类型转换指令
        case Eum_BC_TO_INT:
        case Eum_BC_TO_FLOAT:
        case Eum_BC_TO_STRING:
        case Eum_BC_TO_BOOL:
            return F_execute_conversion_instruction(interpreter, instruction);
            
        // 调试指令
        case Eum_BC_DEBUG:
        case Eum_BC_BREAKPOINT:
            return F_execute_debug_instruction(interpreter, instruction);
            
        default:
            // 未知操作码
            F_add_error_internal(interpreter, "未知操作码: 0x%02X", instruction->opcode);
            break;
    }
    
    return result;
}

/**
 * @brief 运行字节码程序
 */
bool F_run_bytecode_program(Stru_BytecodeInterpreter_t* interpreter, 
                           const Stru_BytecodeProgram_t* program)
{
    if (!interpreter || !program) {
        return false;
    }
    
    // 设置程序（需要类型转换，因为结构体字段是非const的）
    interpreter->program = (Stru_BytecodeProgram_t*)program;
    interpreter->program_counter = 0;
    interpreter->state = Eum_BCI_STATE_RUNNING;
    
    // 主执行循环
    while (interpreter->state == Eum_BCI_STATE_RUNNING && 
           interpreter->program_counter < program->instruction_count) {
        
        // 检查断点
        bool at_breakpoint = false;
        for (size_t i = 0; i < interpreter->breakpoint_count; i++) {
            if (interpreter->breakpoints[i] == interpreter->program_counter) {
                interpreter->state = Eum_BCI_STATE_BREAKPOINT;
                at_breakpoint = true;
                break;
            }
        }
        
        if (at_breakpoint) {
            break;
        }
        
        // 获取当前指令
        const Stru_BytecodeInstruction_t* instruction = &program->instructions[interpreter->program_counter];
        
        // 执行指令
        F_execute_instruction(interpreter, instruction);
        
        // 递增程序计数器（除非指令修改了它）
        interpreter->program_counter++;
    }
    
    return interpreter->state != Eum_BCI_STATE_ERROR;
}

/**
 * @brief 单步执行字节码程序
 */
bool F_step_bytecode_program(Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter || !interpreter->program) {
        return false;
    }
    
    if (interpreter->program_counter >= interpreter->program->instruction_count) {
        return false;
    }
    
    // 获取当前指令
    const Stru_BytecodeInstruction_t* instruction = &interpreter->program->instructions[interpreter->program_counter];
    
    // 执行指令
    F_execute_instruction(interpreter, instruction);
    
    // 递增程序计数器（除非指令修改了它）
    interpreter->program_counter++;
    
    return true;
}

/**
 * @brief 继续执行字节码程序（从断点继续）
 */
bool F_continue_bytecode_program(Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter || !interpreter->program) {
        return false;
    }
    
    // 如果当前在断点状态，切换到运行状态
    if (interpreter->state == Eum_BCI_STATE_BREAKPOINT) {
        interpreter->state = Eum_BCI_STATE_RUNNING;
    }
    
    // 继续执行
    return F_run_bytecode_program(interpreter, interpreter->program);
}

/**
 * @brief 暂停字节码程序执行
 */
bool F_pause_bytecode_program(Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter) {
        return false;
    }
    
    if (interpreter->state == Eum_BCI_STATE_RUNNING) {
        interpreter->state = Eum_BCI_STATE_PAUSED;
        return true;
    }
    
    return false;
}

/**
 * @brief 停止字节码程序执行
 */
bool F_stop_bytecode_program(Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter) {
        return false;
    }
    
    interpreter->state = Eum_BCI_STATE_TERMINATED;
    interpreter->program_counter = 0;
    return true;
}

/**
 * @brief 获取解释器性能分析数据
 */
void F_get_interpreter_profiling_data(const Stru_BytecodeInterpreter_t* interpreter,
                                      uint64_t* instruction_count,
                                      uint64_t* execution_time,
                                      size_t* memory_usage)
{
    if (!interpreter) {
        if (instruction_count) *instruction_count = 0;
        if (execution_time) *execution_time = 0;
        if (memory_usage) *memory_usage = 0;
        return;
    }
    
    // 使用现有的性能分析数据
    if (instruction_count) *instruction_count = interpreter->instruction_count;
    if (execution_time) *execution_time = interpreter->total_time;
    
    // 计算内存使用量
    size_t total_memory = 0;
    total_memory += interpreter->stack_capacity * sizeof(Stru_BytecodeValue_t);
    total_memory += interpreter->call_stack_capacity * sizeof(void*);
    total_memory += interpreter->error_capacity * sizeof(char*);
    total_memory += interpreter->breakpoint_count * sizeof(uint32_t);
    
    if (memory_usage) *memory_usage = total_memory;
}

/**
 * @brief 重置解释器性能分析数据
 */
void F_reset_interpreter_profiling_data(Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter) {
        return;
    }
    
    interpreter->instruction_count = 0;
    interpreter->total_time = 0;
    // 注意：stack_top和call_stack_depth是运行时状态，不重置
}
