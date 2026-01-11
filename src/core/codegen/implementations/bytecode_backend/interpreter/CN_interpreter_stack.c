/******************************************************************************
 * 文件名: CN_interpreter_stack.c
 * 功能: CN_Language 字节码解释器栈操作模块
 * 
 * 负责解释器的栈操作功能，包括值栈管理、调用栈管理等。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 从CN_bytecode_interpreter_main.c拆分出来
 * 版权: MIT许可证
 ******************************************************************************/

#include "../CN_bytecode_interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/**
 * @brief 添加错误信息（内部函数声明）
 */
void F_add_error_internal(Stru_BytecodeInterpreter_t* interpreter, const char* format, ...);

/**
 * @brief 确保栈容量
 */
bool F_ensure_stack_capacity(Stru_BytecodeInterpreter_t* interpreter, size_t required_capacity)
{
    if (!interpreter) {
        return false;
    }
    
    if (interpreter->stack_capacity >= required_capacity) {
        return true;
    }
    
    size_t new_capacity = interpreter->stack_capacity * 2;
    if (new_capacity < required_capacity) {
        new_capacity = required_capacity;
    }
    
    Stru_BytecodeValue_t* new_stack = (Stru_BytecodeValue_t*)realloc(interpreter->stack, 
                                                                     new_capacity * sizeof(Stru_BytecodeValue_t));
    if (!new_stack) {
        F_add_error_internal(interpreter, "无法扩展栈容量到 %zu", new_capacity);
        return false;
    }
    
    interpreter->stack = new_stack;
    interpreter->stack_capacity = new_capacity;
    return true;
}

/**
 * @brief 确保调用栈容量
 */
bool F_ensure_call_stack_capacity(Stru_BytecodeInterpreter_t* interpreter, size_t required_capacity)
{
    if (!interpreter) {
        return false;
    }
    
    if (interpreter->call_stack_capacity >= required_capacity) {
        return true;
    }
    
    size_t new_capacity = interpreter->call_stack_capacity * 2;
    if (new_capacity < required_capacity) {
        new_capacity = required_capacity;
    }
    
    void** new_call_stack = (void**)realloc(interpreter->call_stack, 
                                           new_capacity * sizeof(void*));
    if (!new_call_stack) {
        F_add_error_internal(interpreter, "无法扩展调用栈容量到 %zu", new_capacity);
        return false;
    }
    
    interpreter->call_stack = new_call_stack;
    interpreter->call_stack_capacity = new_capacity;
    return true;
}

/**
 * @brief 确保错误数组容量
 */
bool F_ensure_error_capacity(Stru_BytecodeInterpreter_t* interpreter, size_t required_capacity)
{
    if (!interpreter) {
        return false;
    }
    
    if (interpreter->error_capacity >= required_capacity) {
        return true;
    }
    
    size_t new_capacity = interpreter->error_capacity * 2;
    if (new_capacity < required_capacity) {
        new_capacity = required_capacity;
    }
    
    char** new_errors = (char**)realloc(interpreter->error_messages, 
                                       new_capacity * sizeof(char*));
    if (!new_errors) {
        return false;
    }
    
    interpreter->error_messages = new_errors;
    interpreter->error_capacity = new_capacity;
    return true;
}

/**
 * @brief 推送值到栈
 */
bool F_push_value(Stru_BytecodeInterpreter_t* interpreter, Stru_BytecodeValue_t value)
{
    if (!interpreter) {
        return false;
    }
    
    if (!F_ensure_stack_capacity(interpreter, interpreter->stack_top + 1)) {
        return false;
    }
    
    interpreter->stack[interpreter->stack_top] = value;
    interpreter->stack_top++;
    return true;
}

/**
 * @brief 从栈弹出值
 */
Stru_BytecodeValue_t F_pop_value(Stru_BytecodeInterpreter_t* interpreter)
{
    Stru_BytecodeValue_t result;
    memset(&result, 0, sizeof(result));
    result.type = Eum_BCV_NULL;
    
    if (!interpreter || interpreter->stack_top == 0) {
        return result;
    }
    
    interpreter->stack_top--;
    return interpreter->stack[interpreter->stack_top];
}

/**
 * @brief 查看栈中的值
 */
Stru_BytecodeValue_t F_peek_value(Stru_BytecodeInterpreter_t* interpreter, size_t offset)
{
    Stru_BytecodeValue_t result;
    memset(&result, 0, sizeof(result));
    result.type = Eum_BCV_NULL;
    
    if (!interpreter || offset >= interpreter->stack_top) {
        return result;
    }
    
    size_t index = interpreter->stack_top - offset - 1;
    return interpreter->stack[index];
}
