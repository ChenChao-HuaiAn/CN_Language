/******************************************************************************
 * 文件名: CN_interpreter_debug.c
 * 功能: CN_Language 字节码解释器调试功能模块
 * 
 * 负责解释器的调试功能，包括错误处理、断点管理、栈跟踪等。
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
 * @brief 确保错误数组容量（内部函数声明）
 */
bool F_ensure_error_capacity(Stru_BytecodeInterpreter_t* interpreter, size_t required_capacity);

/**
 * @brief 获取解释器错误信息
 */
void F_get_interpreter_errors(const Stru_BytecodeInterpreter_t* interpreter,
                              char*** errors, size_t* error_count)
{
    if (!interpreter || !errors || !error_count) {
        return;
    }
    
    *errors = NULL;
    *error_count = 0;
    
    if (interpreter->error_count == 0) {
        return;
    }
    
    // 分配错误信息数组
    char** error_array = (char**)malloc(interpreter->error_count * sizeof(char*));
    if (!error_array) {
        return;
    }
    
    // 复制错误信息
    for (size_t i = 0; i < interpreter->error_count; i++) {
        if (interpreter->error_messages[i]) {
            error_array[i] = strdup(interpreter->error_messages[i]);
        } else {
            error_array[i] = NULL;
        }
    }
    
    *errors = error_array;
    *error_count = interpreter->error_count;
}

/**
 * @brief 清除解释器错误
 */
void F_clear_interpreter_errors(Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter) {
        return;
    }
    
    for (size_t i = 0; i < interpreter->error_count; i++) {
        if (interpreter->error_messages[i]) {
            free(interpreter->error_messages[i]);
            interpreter->error_messages[i] = NULL;
        }
    }
    
    interpreter->error_count = 0;
    interpreter->has_errors = false;
}

/**
 * @brief 设置断点
 */
bool F_set_breakpoint(Stru_BytecodeInterpreter_t* interpreter,
                      uint32_t instruction_offset)
{
    if (!interpreter || !interpreter->program) {
        return false;
    }
    
    if (instruction_offset >= interpreter->program->instruction_count) {
        F_add_error_internal(interpreter, "断点偏移 %u 超出程序范围", instruction_offset);
        return false;
    }
    
    // 检查断点是否已存在
    for (size_t i = 0; i < interpreter->breakpoint_count; i++) {
        if (interpreter->breakpoints[i] == instruction_offset) {
            return true; // 断点已存在
        }
    }
    
    // 扩展断点数组
    size_t new_count = interpreter->breakpoint_count + 1;
    uint32_t* new_breakpoints = (uint32_t*)realloc(interpreter->breakpoints, 
                                                   new_count * sizeof(uint32_t));
    if (!new_breakpoints) {
        F_add_error_internal(interpreter, "无法分配断点内存");
        return false;
    }
    
    interpreter->breakpoints = new_breakpoints;
    interpreter->breakpoints[interpreter->breakpoint_count] = instruction_offset;
    interpreter->breakpoint_count = new_count;
    
    return true;
}

/**
 * @brief 清除断点
 */
bool F_clear_breakpoint(Stru_BytecodeInterpreter_t* interpreter,
                        uint32_t instruction_offset)
{
    if (!interpreter || !interpreter->breakpoints) {
        return false;
    }
    
    // 查找断点
    size_t index = 0;
    bool found = false;
    for (size_t i = 0; i < interpreter->breakpoint_count; i++) {
        if (interpreter->breakpoints[i] == instruction_offset) {
            index = i;
            found = true;
            break;
        }
    }
    
    if (!found) {
        return false; // 断点不存在
    }
    
    // 移动后续断点
    for (size_t i = index; i < interpreter->breakpoint_count - 1; i++) {
        interpreter->breakpoints[i] = interpreter->breakpoints[i + 1];
    }
    
    interpreter->breakpoint_count--;
    
    // 如果数组为空，释放内存
    if (interpreter->breakpoint_count == 0) {
        free(interpreter->breakpoints);
        interpreter->breakpoints = NULL;
    } else {
        // 缩小数组
        uint32_t* new_breakpoints = (uint32_t*)realloc(interpreter->breakpoints,
                                                       interpreter->breakpoint_count * sizeof(uint32_t));
        if (new_breakpoints) {
            interpreter->breakpoints = new_breakpoints;
        }
    }
    
    return true;
}

/**
 * @brief 清除所有断点
 */
void F_clear_all_breakpoints(Stru_BytecodeInterpreter_t* interpreter)
{
    if (!interpreter) {
        return;
    }
    
    if (interpreter->breakpoints) {
        free(interpreter->breakpoints);
        interpreter->breakpoints = NULL;
    }
    
    interpreter->breakpoint_count = 0;
}

/**
 * @brief 获取当前栈帧信息
 */
bool F_get_current_stack_frame(const Stru_BytecodeInterpreter_t* interpreter,
                               size_t frame_index,
                               const char** function_name,
                               uint32_t* line_number,
                               uint32_t* instruction_offset)
{
    if (!interpreter || !interpreter->program) {
        return false;
    }
    
    // 简化实现：返回当前指令的信息
    if (function_name) {
        *function_name = interpreter->program->entry_point;
    }
    
    if (line_number && interpreter->program_counter < interpreter->program->instruction_count) {
        *line_number = interpreter->program->instructions[interpreter->program_counter].line;
    }
    
    if (instruction_offset) {
        *instruction_offset = interpreter->program_counter;
    }
    
    return true;
}

/**
 * @brief 获取栈跟踪
 */
bool F_get_stack_trace(const Stru_BytecodeInterpreter_t* interpreter,
                       void*** frames, size_t* frame_count)
{
    if (!interpreter || !frames || !frame_count) {
        return false;
    }
    
    // 简化实现：返回空栈跟踪
    *frames = NULL;
    *frame_count = 0;
    return true;
}

/**
 * @brief 添加错误信息（内部实现）
 */
void F_add_error_internal(Stru_BytecodeInterpreter_t* interpreter, const char* format, ...)
{
    if (!interpreter || !format) {
        return;
    }
    
    // 确保有足够的容量
    if (!F_ensure_error_capacity(interpreter, interpreter->error_count + 1)) {
        return;
    }
    
    // 格式化错误消息
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // 分配并存储错误消息
    interpreter->error_messages[interpreter->error_count] = strdup(buffer);
    if (interpreter->error_messages[interpreter->error_count]) {
        interpreter->error_count++;
        interpreter->has_errors = true;
    }
}
