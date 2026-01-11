/******************************************************************************
 * 文件名: CN_bytecode_utils.c
 * 功能: 字节码后端工具函数模块
 * 
 * 提供字节码后端的内部状态管理、配置验证和工具函数。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "../CN_bytecode_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ============================================================================
// 内部类型定义
// ============================================================================

/**
 * @brief 字节码后端内部状态结构体
 */
typedef struct {
    Stru_BytecodeBackendConfig_t config;        ///< 当前配置
    bool initialized;                           ///< 是否已初始化
    char* error_message;                        ///< 错误消息
    void* internal_data;                        ///< 内部数据
} BytecodeBackendState;

// ============================================================================
// 内部函数实现
// ============================================================================

/**
 * @brief 创建后端状态
 */
BytecodeBackendState* F_create_backend_state(void)
{
    BytecodeBackendState* state = (BytecodeBackendState*)malloc(sizeof(BytecodeBackendState));
    if (!state) {
        return NULL;
    }
    
    memset(state, 0, sizeof(BytecodeBackendState));
    state->initialized = false;
    state->error_message = NULL;
    state->internal_data = NULL;
    
    // 设置默认配置
    state->config = F_create_default_bytecode_backend_config();
    
    return state;
}

/**
 * @brief 销毁后端状态
 */
void F_destroy_backend_state(BytecodeBackendState* state)
{
    if (!state) {
        return;
    }
    
    if (state->error_message) {
        free(state->error_message);
    }
    
    if (state->internal_data) {
        free(state->internal_data);
    }
    
    free(state);
}

/**
 * @brief 验证配置
 */
bool F_validate_config(const Stru_BytecodeBackendConfig_t* config)
{
    if (!config) {
        return false;
    }
    
    // 验证优化级别
    if (config->optimization_level < 0 || config->optimization_level > 3) {
        return false;
    }
    
    // 验证栈大小
    if (config->stack_size == 0) {
        return false;
    }
    
    // 验证堆大小
    if (config->heap_size == 0) {
        return false;
    }
    
    // 验证输出格式
    if (config->output_format) {
        if (strcmp(config->output_format, "binary") != 0 &&
            strcmp(config->output_format, "text") != 0 &&
            strcmp(config->output_format, "hex") != 0) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 创建字节码程序
 */
Stru_BytecodeProgram_t* F_create_bytecode_program(void)
{
    Stru_BytecodeProgram_t* program = (Stru_BytecodeProgram_t*)malloc(sizeof(Stru_BytecodeProgram_t));
    if (!program) {
        return NULL;
    }
    
    memset(program, 0, sizeof(Stru_BytecodeProgram_t));
    
    // 初始化指令数组
    program->instruction_capacity = 64;
    program->instructions = (Stru_BytecodeInstruction_t*)malloc(
        program->instruction_capacity * sizeof(Stru_BytecodeInstruction_t));
    if (!program->instructions) {
        free(program);
        return NULL;
    }
    
    // 初始化常量池
    program->constant_capacity = 32;
    program->constant_pool = (char**)malloc(program->constant_capacity * sizeof(char*));
    if (!program->constant_pool) {
        free(program->instructions);
        free(program);
        return NULL;
    }
    
    // 初始化其他字段
    program->instruction_count = 0;
    program->constant_count = 0;
    program->line_table = NULL;
    program->line_table_size = 0;
    program->debug_symbols = NULL;
    program->debug_symbol_count = 0;
    program->stack_size = 4096;  // 默认栈大小
    program->heap_size = 65536;  // 默认堆大小
    program->global_count = 0;
    program->entry_point = NULL;
    program->entry_point_offset = 0;
    program->internal_data = NULL;
    
    return program;
}

/**
 * @brief 确保指令容量
 */
bool F_ensure_instruction_capacity(Stru_BytecodeProgram_t* program, size_t needed)
{
    if (!program || !program->instructions) {
        return false;
    }
    
    if (program->instruction_count + needed <= program->instruction_capacity) {
        return true;
    }
    
    // 计算新容量
    size_t new_capacity = program->instruction_capacity * 2;
    while (new_capacity < program->instruction_count + needed) {
        new_capacity *= 2;
    }
    
    // 重新分配内存
    Stru_BytecodeInstruction_t* new_instructions = (Stru_BytecodeInstruction_t*)realloc(
        program->instructions, new_capacity * sizeof(Stru_BytecodeInstruction_t));
    if (!new_instructions) {
        return false;
    }
    
    program->instructions = new_instructions;
    program->instruction_capacity = new_capacity;
    return true;
}

/**
 * @brief 确保常量池容量
 */
bool F_ensure_constant_capacity(Stru_BytecodeProgram_t* program, size_t needed)
{
    if (!program || !program->constant_pool) {
        return false;
    }
    
    if (program->constant_count + needed <= program->constant_capacity) {
        return true;
    }
    
    // 计算新容量
    size_t new_capacity = program->constant_capacity * 2;
    while (new_capacity < program->constant_count + needed) {
        new_capacity *= 2;
    }
    
    // 重新分配内存
    char** new_pool = (char**)realloc(program->constant_pool, new_capacity * sizeof(char*));
    if (!new_pool) {
        return false;
    }
    
    program->constant_pool = new_pool;
    program->constant_capacity = new_capacity;
    return true;
}

/**
 * @brief 添加常量到常量池
 */
uint32_t F_add_constant_to_pool(Stru_BytecodeProgram_t* program, const char* constant)
{
    if (!program || !constant) {
        return 0;
    }
    
    // 检查常量是否已存在
    for (size_t i = 0; i < program->constant_count; i++) {
        if (program->constant_pool[i] && strcmp(program->constant_pool[i], constant) == 0) {
            return (uint32_t)(i + 1);  // 返回索引+1（0表示无常量）
        }
    }
    
    // 确保有足够容量
    if (!F_ensure_constant_capacity(program, 1)) {
        return 0;
    }
    
    // 复制常量字符串
    char* constant_copy = strdup(constant);
    if (!constant_copy) {
        return 0;
    }
    
    // 添加到常量池
    program->constant_pool[program->constant_count] = constant_copy;
    program->constant_count++;
    
    return (uint32_t)program->constant_count;  // 返回新常量的索引
}

/**
 * @brief 添加指令到程序
 */
void F_add_instruction(Stru_BytecodeProgram_t* program, 
                       const Stru_BytecodeInstruction_t* instruction)
{
    if (!program || !instruction) {
        return;
    }
    
    // 确保有足够容量
    if (!F_ensure_instruction_capacity(program, 1)) {
        return;
    }
    
    // 复制指令
    memcpy(&program->instructions[program->instruction_count], 
           instruction, 
           sizeof(Stru_BytecodeInstruction_t));
    
    program->instruction_count++;
}

/**
 * @brief 销毁字节码程序
 */
void F_destroy_bytecode_program(Stru_BytecodeProgram_t* program)
{
    if (!program) {
        return;
    }
    
    // 释放指令数组
    if (program->instructions) {
        free(program->instructions);
    }
    
    // 释放常量池
    if (program->constant_pool) {
        for (size_t i = 0; i < program->constant_count; i++) {
            if (program->constant_pool[i]) {
                free(program->constant_pool[i]);
            }
        }
        free(program->constant_pool);
    }
    
    // 释放行号表
    if (program->line_table) {
        free(program->line_table);
    }
    
    // 释放调试符号表
    if (program->debug_symbols) {
        for (size_t i = 0; i < program->debug_symbol_count; i++) {
            if (program->debug_symbols[i].name) {
                free((void*)program->debug_symbols[i].name);
            }
        }
        free(program->debug_symbols);
    }
    
    // 释放内部数据
    if (program->internal_data) {
        free(program->internal_data);
    }
    
    // 释放程序结构体本身
    free(program);
}

/**
 * @brief 创建默认字节码后端配置
 */
Stru_BytecodeBackendConfig_t F_create_default_bytecode_backend_config(void)
{
    Stru_BytecodeBackendConfig_t config;
    memset(&config, 0, sizeof(config));
    
    // 设置默认值
    config.optimization_level = 1;  // 默认优化级别
    config.optimize_bytecode = true;
    config.include_debug_info = true;
    config.stack_size = 4096;       // 默认栈大小
    config.heap_size = 65536;       // 默认堆大小
    config.output_format = "binary";
    config.enable_profiling = false;
    config.enable_gc = false;
    
    return config;
}
