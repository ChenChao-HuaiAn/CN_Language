/******************************************************************************
 * 文件名: CN_bytecode_executor.c
 * 功能: CN_Language 字节码执行器模块
 * 
 * 负责字节码的实际执行，包括指令解码、栈操作、函数调用等。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * 作者: CN_Language架构委员会
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_bytecode_executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

// ============================================================================
// 内部常量定义
// ============================================================================

#define CN_EXECUTOR_MAX_STACK_DEPTH 65536
#define CN_EXECUTOR_MAX_CALL_DEPTH 256
#define CN_EXECUTOR_MAX_LOCAL_VARS 1024

// ============================================================================
// 执行器私有数据结构
// ============================================================================

/**
 * @brief 执行器状态
 */
typedef struct Stru_ExecutorState_t {
    uint32_t program_counter;      ///< 程序计数器
    uint32_t stack_pointer;        ///< 栈指针
    uint32_t frame_pointer;        ///< 帧指针
    uint32_t call_depth;           ///< 调用深度
    bool running;                  ///< 是否正在运行
    bool error;                    ///< 是否有错误
    char error_message[256];       ///< 错误消息
} Stru_ExecutorState_t;

/**
 * @brief 执行器私有数据
 */
typedef struct Stru_BytecodeExecutorData_t {
    Stru_ExecutorState_t state;            ///< 执行器状态
    uint8_t* code_memory;                  ///< 代码内存
    size_t code_size;                      ///< 代码大小
    void* stack_memory;                    ///< 栈内存
    size_t stack_size;                     ///< 栈大小
    void* local_vars;                      ///< 局部变量
    void** call_stack;                     ///< 调用栈
    uint32_t* return_addresses;            ///< 返回地址数组
    void* user_data;                       ///< 用户数据
} Stru_BytecodeExecutorData_t;

// ============================================================================
// 内部辅助函数声明
// ============================================================================

static bool F_validate_execution_state(const Stru_BytecodeExecutorData_t* executor);
static uint8_t F_fetch_bytecode(const Stru_BytecodeExecutorData_t* executor, uint32_t offset);
static void F_set_error(Stru_BytecodeExecutorData_t* executor, const char* format, ...);

// ============================================================================
// 公共函数实现
// ============================================================================

/**
 * @brief 创建字节码执行器
 */
Stru_BytecodeExecutor_t* F_create_bytecode_executor(size_t stack_size)
{
    if (stack_size == 0) {
        stack_size = CN_EXECUTOR_MAX_STACK_DEPTH;
    }
    
    // 分配执行器
    Stru_BytecodeExecutor_t* executor = (Stru_BytecodeExecutor_t*)malloc(sizeof(Stru_BytecodeExecutor_t));
    if (!executor) {
        return NULL;
    }
    
    memset(executor, 0, sizeof(Stru_BytecodeExecutor_t));
    
    // 分配私有数据
    executor->private_data = malloc(sizeof(Stru_BytecodeExecutorData_t));
    if (!executor->private_data) {
        free(executor);
        return NULL;
    }
    
    Stru_BytecodeExecutorData_t* data = (Stru_BytecodeExecutorData_t*)executor->private_data;
    memset(data, 0, sizeof(Stru_BytecodeExecutorData_t));
    
    // 初始化状态
    data->state.program_counter = 0;
    data->state.stack_pointer = 0;
    data->state.frame_pointer = 0;
    data->state.call_depth = 0;
    data->state.running = false;
    data->state.error = false;
    data->state.error_message[0] = '\0';
    
    // 分配栈内存
    data->stack_memory = malloc(stack_size);
    if (!data->stack_memory) {
        free(data);
        free(executor);
        return NULL;
    }
    
    data->stack_size = stack_size;
    
    // 分配调用栈
    data->call_stack = malloc(CN_EXECUTOR_MAX_CALL_DEPTH * sizeof(void*));
    data->return_addresses = malloc(CN_EXECUTOR_MAX_CALL_DEPTH * sizeof(uint32_t));
    if (!data->call_stack || !data->return_addresses) {
        if (data->call_stack) free(data->call_stack);
        if (data->return_addresses) free(data->return_addresses);
        free(data->stack_memory);
        free(data);
        free(executor);
        return NULL;
    }
    
    // 分配局部变量
    data->local_vars = malloc(CN_EXECUTOR_MAX_LOCAL_VARS * sizeof(void*));
    if (!data->local_vars) {
        free(data->return_addresses);
        free(data->call_stack);
        free(data->stack_memory);
        free(data);
        free(executor);
        return NULL;
    }
    
    return executor;
}

/**
 * @brief 销毁字节码执行器
 */
void F_destroy_bytecode_executor(Stru_BytecodeExecutor_t* executor)
{
    if (!executor) {
        return;
    }
    
    if (executor->private_data) {
        Stru_BytecodeExecutorData_t* data = (Stru_BytecodeExecutorData_t*)executor->private_data;
        
        // 释放代码内存
        if (data->code_memory) {
            free(data->code_memory);
        }
        
        // 释放栈内存
        if (data->stack_memory) {
            free(data->stack_memory);
        }
        
        // 释放调用栈
        if (data->call_stack) {
            free(data->call_stack);
        }
        
        if (data->return_addresses) {
            free(data->return_addresses);
        }
        
        // 释放局部变量
        if (data->local_vars) {
            free(data->local_vars);
        }
        
        free(data);
    }
    
    free(executor);
}

/**
 * @brief 加载字节码到执行器
 */
bool F_load_bytecode(Stru_BytecodeExecutor_t* executor, 
                    const uint8_t* bytecode, size_t bytecode_size)
{
    if (!executor || !executor->private_data || !bytecode || bytecode_size == 0) {
        return false;
    }
    
    Stru_BytecodeExecutorData_t* data = (Stru_BytecodeExecutorData_t*)executor->private_data;
    
    // 释放旧的代码内存
    if (data->code_memory) {
        free(data->code_memory);
    }
    
    // 分配新的代码内存
    data->code_memory = (uint8_t*)malloc(bytecode_size);
    if (!data->code_memory) {
        return false;
    }
    
    // 复制字节码
    memcpy(data->code_memory, bytecode, bytecode_size);
    data->code_size = bytecode_size;
    
    // 重置执行器状态
    data->state.program_counter = 0;
    data->state.stack_pointer = 0;
    data->state.frame_pointer = 0;
    data->state.call_depth = 0;
    data->state.running = false;
    data->state.error = false;
    data->state.error_message[0] = '\0';
    
    return true;
}

/**
 * @brief 执行字节码
 */
bool F_execute_bytecode(Stru_BytecodeExecutor_t* executor, uint32_t max_instructions)
{
    if (!executor || !executor->private_data) {
        return false;
    }
    
    Stru_BytecodeExecutorData_t* data = (Stru_BytecodeExecutorData_t*)executor->private_data;
    
    // 验证执行状态
    if (!F_validate_execution_state(data)) {
        return false;
    }
    
    // 设置运行状态
    data->state.running = true;
    data->state.error = false;
    
    // 执行循环
    uint32_t instruction_count = 0;
    while (data->state.running && !data->state.error) {
        // 检查指令限制
        if (max_instructions > 0 && instruction_count >= max_instructions) {
            F_set_error(data, "达到最大指令执行限制: %u", max_instructions);
            break;
        }
        
        // 检查程序计数器范围
        if (data->state.program_counter >= data->code_size) {
            F_set_error(data, "程序计数器越界: %u >= %zu", 
                       data->state.program_counter, data->code_size);
            break;
        }
        
        // 获取操作码
        uint8_t opcode = F_fetch_bytecode(data, data->state.program_counter);
        
        // 执行指令
        // 简化实现：只处理几个基本指令
        switch (opcode) {
            case 0x00: // NOP
                data->state.program_counter++;
                break;
                
            case 0x01: // HALT
                data->state.running = false;
                break;
                
            case 0x02: // PUSH
                // 简化：压入一个固定值
                if (data->state.stack_pointer < data->stack_size) {
                    uint8_t* stack = (uint8_t*)data->stack_memory;
                    stack[data->state.stack_pointer] = 0x42; // 示例值
                    data->state.stack_pointer++;
                    data->state.program_counter++;
                } else {
                    F_set_error(data, "栈溢出");
                }
                break;
                
            case 0x03: // POP
                if (data->state.stack_pointer > 0) {
                    data->state.stack_pointer--;
                    data->state.program_counter++;
                } else {
                    F_set_error(data, "栈下溢");
                }
                break;
                
            default:
                F_set_error(data, "未知操作码: 0x%02X", opcode);
                break;
        }
        
        instruction_count++;
    }
    
    return !data->state.error;
}

/**
 * @brief 获取执行器状态
 */
void F_get_executor_state(const Stru_BytecodeExecutor_t* executor,
                         uint32_t* program_counter,
                         uint32_t* stack_pointer,
                         uint32_t* frame_pointer,
                         bool* running,
                         bool* error)
{
    if (!executor || !executor->private_data) {
        return;
    }
    
    Stru_BytecodeExecutorData_t* data = (Stru_BytecodeExecutorData_t*)executor->private_data;
    
    if (program_counter) *program_counter = data->state.program_counter;
    if (stack_pointer) *stack_pointer = data->state.stack_pointer;
    if (frame_pointer) *frame_pointer = data->state.frame_pointer;
    if (running) *running = data->state.running;
    if (error) *error = data->state.error;
}

/**
 * @brief 获取错误信息
 */
const char* F_get_executor_error(const Stru_BytecodeExecutor_t* executor)
{
    if (!executor || !executor->private_data) {
        return "无效的执行器";
    }
    
    Stru_BytecodeExecutorData_t* data = (Stru_BytecodeExecutorData_t*)executor->private_data;
    return data->state.error_message;
}

/**
 * @brief 重置执行器
 */
void F_reset_executor(Stru_BytecodeExecutor_t* executor)
{
    if (!executor || !executor->private_data) {
        return;
    }
    
    Stru_BytecodeExecutorData_t* data = (Stru_BytecodeExecutorData_t*)executor->private_data;
    
    // 重置状态
    data->state.program_counter = 0;
    data->state.stack_pointer = 0;
    data->state.frame_pointer = 0;
    data->state.call_depth = 0;
    data->state.running = false;
    data->state.error = false;
    data->state.error_message[0] = '\0';
    
    // 清空栈内存
    if (data->stack_memory && data->stack_size > 0) {
        memset(data->stack_memory, 0, data->stack_size);
    }
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 验证执行状态
 */
static bool F_validate_execution_state(const Stru_BytecodeExecutorData_t* executor)
{
    if (!executor) {
        return false;
    }
    
    if (!executor->code_memory || executor->code_size == 0) {
        return false;
    }
    
    if (!executor->stack_memory || executor->stack_size == 0) {
        return false;
    }
    
    return true;
}

/**
 * @brief 获取字节码
 */
static uint8_t F_fetch_bytecode(const Stru_BytecodeExecutorData_t* executor, uint32_t offset)
{
    if (!executor || !executor->code_memory || offset >= executor->code_size) {
        return 0;
    }
    
    return executor->code_memory[offset];
}

/**
 * @brief 设置错误信息
 */
static void F_set_error(Stru_BytecodeExecutorData_t* executor, const char* format, ...)
{
    if (!executor || !format) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    vsnprintf(executor->state.error_message, 
              sizeof(executor->state.error_message), 
              format, args);
    va_end(args);
    
    executor->state.error = true;
    executor->state.running = false;
}
