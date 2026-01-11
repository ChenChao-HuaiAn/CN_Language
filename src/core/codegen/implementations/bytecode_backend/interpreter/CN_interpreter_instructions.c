/******************************************************************************
 * 文件名: CN_interpreter_instructions.c
 * 功能: CN_Language 字节码解释器指令执行模块
 * 
 * 实现指令执行相关的辅助函数，将大型指令执行函数拆分成更小的模块。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_interpreter_instructions.h"
#include "CN_interpreter_stack.h"
#include "CN_interpreter_debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 静态辅助函数声明
// ============================================================================

static Stru_BytecodeValue_t F_create_null_value(void);

// ============================================================================
// 辅助函数实现
// ============================================================================

/**
 * @brief 创建空值
 */
static Stru_BytecodeValue_t F_create_null_value(void)
{
    Stru_BytecodeValue_t value;
    memset(&value, 0, sizeof(value));
    value.type = Eum_BCV_NULL;
    return value;
}

/**
 * @brief 检查条件是否为真
 */
bool F_is_condition_true(const Stru_BytecodeValue_t* condition)
{
    if (!condition) {
        return false;
    }
    
    switch (condition->type) {
        case Eum_BCV_BOOL:
            return condition->data.bool_value;
        case Eum_BCV_INT32:
            return condition->data.int32_value != 0;
        case Eum_BCV_FLOAT32:
            return condition->data.float32_value != 0.0f;
        case Eum_BCV_STRING:
            return condition->data.string_value != NULL && 
                   condition->data.string_value[0] != '\0';
        default:
            return false;
    }
}

// ============================================================================
// 指令执行函数实现
// ============================================================================

/**
 * @brief 执行基本操作指令
 */
Stru_BytecodeValue_t F_execute_basic_instruction(
    Stru_BytecodeInterpreter_t* interpreter, 
    const Stru_BytecodeInstruction_t* instruction)
{
    Stru_BytecodeValue_t result = F_create_null_value();
    
    if (!interpreter || !instruction) {
        return result;
    }
    
    switch (instruction->opcode) {
        case Eum_BC_NOP:
            // 无操作
            break;
            
        case Eum_BC_LOAD_CONST:
            // 加载常量到栈
            if (instruction->immediate.imm_uint32 < interpreter->program->constant_count) {
                const char* constant = interpreter->program->constant_pool[instruction->immediate.imm_uint32];
                if (constant) {
                    result.type = Eum_BCV_STRING;
                    result.data.string_value = strdup(constant);
                    F_push_value(interpreter, result);
                }
            }
            break;
            
        case Eum_BC_POP:
            // 弹出栈顶值
            F_pop_value(interpreter);
            break;
            
        case Eum_BC_PUSH_NULL:
            // 压入空值
            result.type = Eum_BCV_NULL;
            F_push_value(interpreter, result);
            break;
            
        case Eum_BC_PUSH_TRUE:
            // 压入真值
            result.type = Eum_BCV_BOOL;
            result.data.bool_value = true;
            F_push_value(interpreter, result);
            break;
            
        case Eum_BC_PUSH_FALSE:
            // 压入假值
            result.type = Eum_BCV_BOOL;
            result.data.bool_value = false;
            F_push_value(interpreter, result);
            break;
            
        case Eum_BC_DUP:
            // 复制栈顶元素
            if (interpreter->stack_top > 0) {
                Stru_BytecodeValue_t top = F_peek_value(interpreter, 0);
                F_push_value(interpreter, top);
            }
            break;
            
        case Eum_BC_SWAP:
            // 交换栈顶两个元素
            if (interpreter->stack_top >= 2) {
                Stru_BytecodeValue_t b = F_pop_value(interpreter);
                Stru_BytecodeValue_t a = F_pop_value(interpreter);
                F_push_value(interpreter, b);
                F_push_value(interpreter, a);
            }
            break;
            
        default:
            // 未知操作码
            F_add_error_internal(interpreter, "未知基本操作码: 0x%02X", instruction->opcode);
            break;
    }
    
    return result;
}

/**
 * @brief 执行控制流指令
 */
Stru_BytecodeValue_t F_execute_control_flow_instruction(
    Stru_BytecodeInterpreter_t* interpreter, 
    const Stru_BytecodeInstruction_t* instruction)
{
    Stru_BytecodeValue_t result = F_create_null_value();
    
    if (!interpreter || !instruction) {
        return result;
    }
    
    switch (instruction->opcode) {
        case Eum_BC_JUMP:
            // 无条件跳转
            interpreter->program_counter = instruction->immediate.imm_uint32 - 1; // -1 因为主循环会加1
            break;
            
        case Eum_BC_JUMP_IF_TRUE:
            // 条件跳转（如果栈顶为真）
            if (interpreter->stack_top > 0) {
                Stru_BytecodeValue_t condition = F_pop_value(interpreter);
                if (F_is_condition_true(&condition)) {
                    interpreter->program_counter = instruction->immediate.imm_uint32 - 1; // -1 因为主循环会加1
                }
            }
            break;
            
        case Eum_BC_JUMP_IF_FALSE:
            // 条件跳转（如果栈顶为假）
            if (interpreter->stack_top > 0) {
                Stru_BytecodeValue_t condition = F_pop_value(interpreter);
                if (!F_is_condition_true(&condition)) {
                    interpreter->program_counter = instruction->immediate.imm_uint32 - 1; // -1 因为主循环会加1
                }
            }
            break;
            
        case Eum_BC_CALL:
            // 函数调用
            // 简化实现：记录返回地址
            if (F_ensure_call_stack_capacity(interpreter, interpreter->call_stack_depth + 1)) {
                interpreter->call_stack[interpreter->call_stack_depth] = (void*)(uintptr_t)(interpreter->program_counter + 1);
                interpreter->call_stack_depth++;
                interpreter->program_counter = instruction->immediate.imm_uint32 - 1; // -1 因为主循环会加1
            } else {
                F_add_error_internal(interpreter, "调用栈溢出");
            }
            break;
            
        case Eum_BC_RETURN:
            // 从函数返回
            if (interpreter->call_stack_depth > 0) {
                interpreter->call_stack_depth--;
                interpreter->program_counter = (uint32_t)(uintptr_t)interpreter->call_stack[interpreter->call_stack_depth] - 1; // -1 因为主循环会加1
            } else {
                F_add_error_internal(interpreter, "返回时调用栈为空");
            }
            break;
            
        case Eum_BC_RETURN_VOID:
            // 从函数返回（无返回值）
            if (interpreter->call_stack_depth > 0) {
                interpreter->call_stack_depth--;
                interpreter->program_counter = (uint32_t)(uintptr_t)interpreter->call_stack[interpreter->call_stack_depth] - 1; // -1 因为主循环会加1
            } else {
                F_add_error_internal(interpreter, "返回时调用栈为空");
            }
            break;
            
        default:
            // 未知操作码
            F_add_error_internal(interpreter, "未知控制流操作码: 0x%02X", instruction->opcode);
            break;
    }
    
    return result;
}

/**
 * @brief 执行变量操作指令
 */
Stru_BytecodeValue_t F_execute_variable_instruction(
    Stru_BytecodeInterpreter_t* interpreter, 
    const Stru_BytecodeInstruction_t* instruction)
{
    Stru_BytecodeValue_t result = F_create_null_value();
    
    if (!interpreter || !instruction) {
        return result;
    }
    
    switch (instruction->opcode) {
        case Eum_BC_LOAD_LOCAL:
            // 加载局部变量
            // 简化实现：从栈中加载
            if (instruction->immediate.imm_uint32 < interpreter->stack_top) {
                Stru_BytecodeValue_t value = F_peek_value(interpreter, instruction->immediate.imm_uint32);
                F_push_value(interpreter, value);
            } else {
                F_add_error_internal(interpreter, "局部变量索引 %u 超出范围", instruction->immediate.imm_uint32);
            }
            break;
            
        case Eum_BC_STORE_LOCAL:
            // 存储到局部变量
            // 简化实现：存储到栈中
            if (interpreter->stack_top > 0 && instruction->immediate.imm_uint32 < interpreter->stack_top) {
                Stru_BytecodeValue_t value = F_pop_value(interpreter);
                size_t index = interpreter->stack_top - instruction->immediate.imm_uint32 - 1;
                interpreter->stack[index] = value;
            } else {
                F_add_error_internal(interpreter, "局部变量索引 %u 超出范围", instruction->immediate.imm_uint32);
            }
            break;
            
        case Eum_BC_LOAD_GLOBAL:
            // 加载全局变量
            // 简化实现：从全局变量数组加载
            if (instruction->immediate.imm_uint32 < interpreter->global_count) {
                F_push_value(interpreter, interpreter->globals[instruction->immediate.imm_uint32]);
            } else {
                F_add_error_internal(interpreter, "全局变量索引 %u 超出范围", instruction->immediate.imm_uint32);
            }
            break;
            
        case Eum_BC_STORE_GLOBAL:
            // 存储到全局变量
            // 简化实现：存储到全局变量数组
            if (interpreter->stack_top > 0 && instruction->immediate.imm_uint32 < interpreter->global_count) {
                Stru_BytecodeValue_t value = F_pop_value(interpreter);
                interpreter->globals[instruction->immediate.imm_uint32] = value;
            } else {
                F_add_error_internal(interpreter, "全局变量索引 %u 超出范围", instruction->immediate.imm_uint32);
            }
            break;
            
        default:
            // 未知操作码
            F_add_error_internal(interpreter, "未知变量操作码: 0x%02X", instruction->opcode);
            break;
    }
    
    return result;
}

/**
 * @brief 执行类型转换指令
 */
Stru_BytecodeValue_t F_execute_conversion_instruction(
    Stru_BytecodeInterpreter_t* interpreter, 
    const Stru_BytecodeInstruction_t* instruction)
{
    Stru_BytecodeValue_t result = F_create_null_value();
    
    if (!interpreter || !instruction || interpreter->stack_top == 0) {
        return result;
    }
    
    Stru_BytecodeValue_t a = F_pop_value(interpreter);
    
    // 简化：只实现整数转换
    if (instruction->opcode == Eum_BC_TO_INT) {
        result.type = Eum_BCV_INT32;
        if (a.type == Eum_BCV_INT32) {
            result.data.int32_value = a.data.int32_value;
        } else if (a.type == Eum_BCV_FLOAT32) {
            result.data.int32_value = (int32_t)a.data.float32_value;
        } else if (a.type == Eum_BCV_BOOL) {
            result.data.int32_value = a.data.bool_value ? 1 : 0;
        } else {
            result.data.int32_value = 0;
        }
    } else {
        // 其他类型转换暂时返回0
        result.type = Eum_BCV_INT32;
        result.data.int32_value = 0;
    }
    
    F_push_value(interpreter, result);
    return result;
}

/**
 * @brief 执行调试指令
 */
Stru_BytecodeValue_t F_execute_debug_instruction(
    Stru_BytecodeInterpreter_t* interpreter, 
    const Stru_BytecodeInstruction_t* instruction)
{
    Stru_BytecodeValue_t result = F_create_null_value();
    
    if (!interpreter || !instruction) {
        return result;
    }
    
    switch (instruction->opcode) {
        case Eum_BC_DEBUG:
            // 调试指令
            // 简化实现：输出调试信息
            printf("[DEBUG] 程序计数器: %u, 栈顶: %zu\n", 
                   interpreter->program_counter, interpreter->stack_top);
            break;
            
        case Eum_BC_BREAKPOINT:
            // 断点指令
            // 简化实现：设置解释器状态为断点命中
            interpreter->state = Eum_BCI_STATE_BREAKPOINT;
            break;
            
        default:
            // 未知操作码
            F_add_error_internal(interpreter, "未知调试操作码: 0x%02X", instruction->opcode);
            break;
    }
    
    return result;
}
