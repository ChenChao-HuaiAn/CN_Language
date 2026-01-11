/******************************************************************************
 * 文件名: CN_bytecode_validator.c
 * 功能: 字节码验证器模块
 * 
 * 提供字节码程序的验证功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "../CN_bytecode_backend.h"
#include "CN_bytecode_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 常量定义
// ============================================================================

#ifndef Eum_BC_MAX_OPCODES
#define Eum_BC_MAX_OPCODES 256
#endif

#ifndef CN_MAX_CONSTANT_LENGTH
#define CN_MAX_CONSTANT_LENGTH 65535
#endif

// ============================================================================
// 公共函数实现
// ============================================================================

/**
 * @brief 验证字节码程序
 */
bool F_validate_bytecode_program_impl(const Stru_BytecodeProgram_t* program, 
                                     char*** errors, size_t* error_count)
{
    if (!program) {
        return false;
    }
    
    // 初始化错误输出参数
    if (errors) {
        *errors = NULL;
    }
    if (error_count) {
        *error_count = 0;
    }
    
    // 收集错误信息
    char** error_list = NULL;
    size_t error_list_size = 0;
    size_t error_list_capacity = 0;
    
    // 检查基本结构
    if (!program->instructions && program->instruction_count > 0) {
        // 添加错误信息
        if (errors) {
            // 扩展错误列表容量
            if (error_list_size >= error_list_capacity) {
                size_t new_capacity = error_list_capacity == 0 ? 4 : error_list_capacity * 2;
                char** new_list = (char**)realloc(error_list, new_capacity * sizeof(char*));
                if (!new_list) {
                    // 内存分配失败，清理并返回
                    for (size_t i = 0; i < error_list_size; i++) {
                        free(error_list[i]);
                    }
                    free(error_list);
                    return false;
                }
                error_list = new_list;
                error_list_capacity = new_capacity;
            }
            
            // 添加错误信息
            error_list[error_list_size] = strdup("指令数组为空，但指令数量大于0");
            if (error_list[error_list_size]) {
                error_list_size++;
            }
        }
        // 不立即返回，继续检查其他错误
    }
    
    if (!program->constant_pool && program->constant_count > 0) {
        // 添加错误信息
        if (errors) {
            // 扩展错误列表容量
            if (error_list_size >= error_list_capacity) {
                size_t new_capacity = error_list_capacity == 0 ? 4 : error_list_capacity * 2;
                char** new_list = (char**)realloc(error_list, new_capacity * sizeof(char*));
                if (!new_list) {
                    // 内存分配失败，清理并返回
                    for (size_t i = 0; i < error_list_size; i++) {
                        free(error_list[i]);
                    }
                    free(error_list);
                    return false;
                }
                error_list = new_list;
                error_list_capacity = new_capacity;
            }
            
            // 添加错误信息
            error_list[error_list_size] = strdup("常量池为空，但常量数量大于0");
            if (error_list[error_list_size]) {
                error_list_size++;
            }
        }
        // 不立即返回，继续检查其他错误
    }
    
    // 检查指令有效性
    for (size_t i = 0; i < program->instruction_count; i++) {
        const Stru_BytecodeInstruction_t* inst = &program->instructions[i];
        
        // 检查操作码是否有效
        if (inst->opcode >= Eum_BC_MAX_OPCODES) {
            // 添加错误信息
            if (errors) {
                // 扩展错误列表容量
                if (error_list_size >= error_list_capacity) {
                    size_t new_capacity = error_list_capacity == 0 ? 4 : error_list_capacity * 2;
                    char** new_list = (char**)realloc(error_list, new_capacity * sizeof(char*));
                    if (!new_list) {
                        // 内存分配失败，清理并返回
                        for (size_t i = 0; i < error_list_size; i++) {
                            free(error_list[i]);
                        }
                        free(error_list);
                        return false;
                    }
                    error_list = new_list;
                    error_list_capacity = new_capacity;
                }
                
                // 添加错误信息
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), 
                        "指令 %zu: 无效的操作码 %u (最大有效操作码: %u)", 
                        i, inst->opcode, Eum_BC_MAX_OPCODES - 1);
                error_list[error_list_size] = strdup(error_msg);
                if (error_list[error_list_size]) {
                    error_list_size++;
                }
            }
        }
        
        // 检查操作数是否在有效范围内
        // 这里可以根据具体指令类型进行更详细的检查
    }
    
    // 检查常量池
    for (size_t i = 0; i < program->constant_count; i++) {
        if (program->constant_pool[i] && strlen(program->constant_pool[i]) > CN_MAX_CONSTANT_LENGTH) {
            // 添加错误信息
            if (errors) {
                // 扩展错误列表容量
                if (error_list_size >= error_list_capacity) {
                    size_t new_capacity = error_list_capacity == 0 ? 4 : error_list_capacity * 2;
                    char** new_list = (char**)realloc(error_list, new_capacity * sizeof(char*));
                    if (!new_list) {
                        // 内存分配失败，清理并返回
                        for (size_t i = 0; i < error_list_size; i++) {
                            free(error_list[i]);
                        }
                        free(error_list);
                        return false;
                    }
                    error_list = new_list;
                    error_list_capacity = new_capacity;
                }
                
                // 添加错误信息
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), 
                        "常量 %zu: 长度 %zu 超过最大限制 %u", 
                        i, strlen(program->constant_pool[i]), CN_MAX_CONSTANT_LENGTH);
                error_list[error_list_size] = strdup(error_msg);
                if (error_list[error_list_size]) {
                    error_list_size++;
                }
            }
        }
    }
    
    // 检查行号表
    if (program->line_table_size > 0 && !program->line_table) {
        // 添加错误信息
        if (errors) {
            // 扩展错误列表容量
            if (error_list_size >= error_list_capacity) {
                size_t new_capacity = error_list_capacity == 0 ? 4 : error_list_capacity * 2;
                char** new_list = (char**)realloc(error_list, new_capacity * sizeof(char*));
                if (!new_list) {
                    // 内存分配失败，清理并返回
                    for (size_t i = 0; i < error_list_size; i++) {
                        free(error_list[i]);
                    }
                    free(error_list);
                    return false;
                }
                error_list = new_list;
                error_list_capacity = new_capacity;
            }
            
            // 添加错误信息
            error_list[error_list_size] = strdup("行号表大小为0，但行号表指针为空");
            if (error_list[error_list_size]) {
                error_list_size++;
            }
        }
    }
    
    // 检查调试符号表
    if (program->debug_symbol_count > 0 && !program->debug_symbols) {
        // 添加错误信息
        if (errors) {
            // 扩展错误列表容量
            if (error_list_size >= error_list_capacity) {
                size_t new_capacity = error_list_capacity == 0 ? 4 : error_list_capacity * 2;
                char** new_list = (char**)realloc(error_list, new_capacity * sizeof(char*));
                if (!new_list) {
                    // 内存分配失败，清理并返回
                    for (size_t i = 0; i < error_list_size; i++) {
                        free(error_list[i]);
                    }
                    free(error_list);
                    return false;
                }
                error_list = new_list;
                error_list_capacity = new_capacity;
            }
            
            // 添加错误信息
            error_list[error_list_size] = strdup("调试符号数量大于0，但调试符号表指针为空");
            if (error_list[error_list_size]) {
                error_list_size++;
            }
        }
    }
    
    // 设置输出参数
    if (errors) {
        *errors = error_list;
    } else {
        // 调用者不关心错误信息，释放内存
        for (size_t i = 0; i < error_list_size; i++) {
            free(error_list[i]);
        }
        free(error_list);
    }
    
    if (error_count) {
        *error_count = error_list_size;
    }
    
    // 如果没有错误，验证通过
    return error_list_size == 0;
}
