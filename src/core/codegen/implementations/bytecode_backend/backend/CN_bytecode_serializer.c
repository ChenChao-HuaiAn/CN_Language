/******************************************************************************
 * 文件名: CN_bytecode_serializer.c
 * 功能: 字节码序列化器模块
 * 
 * 提供字节码程序的序列化和反序列化功能。
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

#ifndef CN_BYTECODE_MAGIC
#define CN_BYTECODE_MAGIC 0x434E4243  // "CNBC" in hex
#endif

// ============================================================================
// 公共函数实现
// ============================================================================

/**
 * @brief 序列化字节码程序
 */
bool F_serialize_bytecode_program_impl(const Stru_BytecodeProgram_t* program, 
                                      uint8_t** data, size_t* size)
{
    if (!program || !data || !size) {
        return false;
    }
    
    // 计算所需的总大小
    size_t total_size = 0;
    
    // 魔数 + 版本信息
    total_size += sizeof(uint32_t) * 2;
    
    // 程序头信息
    total_size += sizeof(uint32_t) * 6;  // instruction_count, constant_count, stack_size, heap_size, global_count, entry_point_offset
    
    // 指令数组
    total_size += program->instruction_count * sizeof(Stru_BytecodeInstruction_t);
    
    // 常量池
    for (size_t i = 0; i < program->constant_count; i++) {
        total_size += sizeof(uint32_t);  // 长度字段
        if (program->constant_pool[i]) {
            total_size += strlen(program->constant_pool[i]);
        }
    }
    
    // 行号表
    total_size += sizeof(uint32_t);  // line_table_size字段
    total_size += program->line_table_size * sizeof(Stru_LineTableEntry_t);
    
    // 调试符号表
    total_size += sizeof(uint32_t);  // debug_symbol_count字段
    for (size_t i = 0; i < program->debug_symbol_count; i++) {
        total_size += sizeof(uint32_t) * 2;  // offset + type
        total_size += sizeof(uint32_t);  // 名称长度字段
        if (program->debug_symbols[i].name) {
            total_size += strlen(program->debug_symbols[i].name);
        }
    }
    
    // 分配内存
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
        return false;
    }
    
    uint8_t* ptr = buffer;
    
    // 写入魔数
    uint32_t magic = CN_BYTECODE_MAGIC;
    memcpy(ptr, &magic, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    // 写入版本信息
    uint32_t version = (CN_BYTECODE_VERSION_MAJOR << 16) | 
                      (CN_BYTECODE_VERSION_MINOR << 8) | 
                      CN_BYTECODE_VERSION_PATCH;
    memcpy(ptr, &version, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    // 写入程序头信息
    uint32_t instruction_count_u32 = (uint32_t)program->instruction_count;
    uint32_t constant_count_u32 = (uint32_t)program->constant_count;
    uint32_t stack_size_u32 = (uint32_t)program->stack_size;
    uint32_t heap_size_u32 = (uint32_t)program->heap_size;
    uint32_t global_count_u32 = (uint32_t)program->global_count;
    
    memcpy(ptr, &instruction_count_u32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(ptr, &constant_count_u32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(ptr, &stack_size_u32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(ptr, &heap_size_u32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(ptr, &global_count_u32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(ptr, &program->entry_point_offset, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    // 写入指令数组
    if (program->instruction_count > 0) {
        size_t instructions_size = program->instruction_count * sizeof(Stru_BytecodeInstruction_t);
        memcpy(ptr, program->instructions, instructions_size);
        ptr += instructions_size;
    }
    
    // 写入常量池
    for (size_t i = 0; i < program->constant_count; i++) {
        if (program->constant_pool[i]) {
            uint32_t length = (uint32_t)strlen(program->constant_pool[i]);
            memcpy(ptr, &length, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            memcpy(ptr, program->constant_pool[i], length);
            ptr += length;
        } else {
            uint32_t length = 0;
            memcpy(ptr, &length, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
        }
    }
    
    // 写入行号表
    uint32_t line_table_size_u32 = (uint32_t)program->line_table_size;
    memcpy(ptr, &line_table_size_u32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    if (program->line_table_size > 0) {
        size_t line_table_size = program->line_table_size * sizeof(Stru_LineTableEntry_t);
        memcpy(ptr, program->line_table, line_table_size);
        ptr += line_table_size;
    }
    
    // 写入调试符号表
    uint32_t debug_symbol_count_u32 = (uint32_t)program->debug_symbol_count;
    memcpy(ptr, &debug_symbol_count_u32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    for (size_t i = 0; i < program->debug_symbol_count; i++) {
        const Stru_DebugSymbol_t* symbol = &program->debug_symbols[i];
        
        memcpy(ptr, &symbol->offset, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        memcpy(ptr, &symbol->type, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        
        if (symbol->name) {
            uint32_t name_len = (uint32_t)strlen(symbol->name);
            memcpy(ptr, &name_len, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            memcpy(ptr, symbol->name, name_len);
            ptr += name_len;
        } else {
            uint32_t zero = 0;
            memcpy(ptr, &zero, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
        }
    }
    
    // 设置输出参数
    *data = buffer;
    *size = total_size;
    
    return true;
}

/**
 * @brief 反序列化字节码程序
 */
Stru_BytecodeProgram_t* F_deserialize_bytecode_program_impl(const uint8_t* data, size_t size)
{
    if (!data || size == 0) {
        return NULL;
    }
    
    const uint8_t* ptr = data;
    const uint8_t* end = data + size;
    
    // 检查是否有足够的数据读取魔数
    if (ptr + sizeof(uint32_t) > end) {
        return NULL;
    }
    
    // 读取魔数
    uint32_t magic;
    memcpy(&magic, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    if (magic != CN_BYTECODE_MAGIC) {
        return NULL;
    }
    
    // 读取版本信息
    uint32_t version;
    memcpy(&version, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    // 检查版本兼容性
    uint32_t major = (version >> 16) & 0xFF;
    uint32_t minor = (version >> 8) & 0xFF;
    uint32_t patch = version & 0xFF;
    
    if (major != CN_BYTECODE_VERSION_MAJOR) {
        return NULL;  // 主版本不兼容
    }
    
    // 创建程序
    Stru_BytecodeProgram_t* program = F_create_bytecode_program();
    if (!program) {
        return NULL;
    }
    
    // 检查是否有足够的数据读取程序头信息
    if (ptr + sizeof(uint32_t) * 6 > end) {
        F_destroy_bytecode_program(program);
        return NULL;
    }
    
    // 读取程序头信息
    uint32_t instruction_count_u32, constant_count_u32, stack_size_u32, heap_size_u32, global_count_u32;
    
    memcpy(&instruction_count_u32, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(&constant_count_u32, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(&stack_size_u32, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(&heap_size_u32, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(&global_count_u32, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(&program->entry_point_offset, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    program->instruction_count = instruction_count_u32;
    program->constant_count = constant_count_u32;
    program->stack_size = stack_size_u32;
    program->heap_size = heap_size_u32;
    program->global_count = global_count_u32;
    
    // 分配指令数组
    program->instruction_capacity = program->instruction_count;
    program->instructions = (Stru_BytecodeInstruction_t*)malloc(
        program->instruction_count * sizeof(Stru_BytecodeInstruction_t));
    if (!program->instructions) {
        F_destroy_bytecode_program(program);
        return NULL;
    }
    
    // 读取指令数组
    if (program->instruction_count > 0) {
        size_t instructions_size = program->instruction_count * sizeof(Stru_BytecodeInstruction_t);
        if (ptr + instructions_size > end) {
            F_destroy_bytecode_program(program);
            return NULL;
        }
        memcpy(program->instructions, ptr, instructions_size);
        ptr += instructions_size;
    }
    
    // 分配常量池
    program->constant_capacity = program->constant_count;
    program->constant_pool = (char**)malloc(program->constant_count * sizeof(char*));
    if (!program->constant_pool) {
        F_destroy_bytecode_program(program);
        return NULL;
    }
    
    // 初始化常量池指针为NULL
    for (size_t i = 0; i < program->constant_count; i++) {
        program->constant_pool[i] = NULL;
    }
    
    // 读取常量池
    for (size_t i = 0; i < program->constant_count; i++) {
        if (ptr + sizeof(uint32_t) > end) {
            F_destroy_bytecode_program(program);
            return NULL;
        }
        
        uint32_t length;
        memcpy(&length, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        
        if (length > 0) {
            if (ptr + length > end) {
                F_destroy_bytecode_program(program);
                return NULL;
            }
            
            program->constant_pool[i] = (char*)malloc(length + 1);
            if (!program->constant_pool[i]) {
                F_destroy_bytecode_program(program);
                return NULL;
            }
            
            memcpy(program->constant_pool[i], ptr, length);
            ptr += length;
            program->constant_pool[i][length] = '\0';
        } else {
            program->constant_pool[i] = NULL;
        }
    }
    
    // 读取行号表
    if (ptr + sizeof(uint32_t) > end) {
        F_destroy_bytecode_program(program);
        return NULL;
    }
    
    uint32_t line_table_size;
    memcpy(&line_table_size, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    if (line_table_size > 0) {
        program->line_table_size = line_table_size;
        size_t line_table_bytes = line_table_size * sizeof(Stru_LineTableEntry_t);
        if (ptr + line_table_bytes > end) {
            F_destroy_bytecode_program(program);
            return NULL;
        }
        
        program->line_table = (Stru_LineTableEntry_t*)malloc(line_table_bytes);
        if (!program->line_table) {
            F_destroy_bytecode_program(program);
            return NULL;
        }
        
        memcpy(program->line_table, ptr, line_table_bytes);
        ptr += line_table_bytes;
    }
    
    // 读取调试符号表
    if (ptr + sizeof(uint32_t) > end) {
        F_destroy_bytecode_program(program);
        return NULL;
    }
    
    uint32_t debug_symbol_count;
    memcpy(&debug_symbol_count, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    if (debug_symbol_count > 0) {
        program->debug_symbol_count = debug_symbol_count;
        program->debug_symbols = (Stru_DebugSymbol_t*)malloc(
            debug_symbol_count * sizeof(Stru_DebugSymbol_t));
        if (!program->debug_symbols) {
            F_destroy_bytecode_program(program);
            return NULL;
        }
        
        for (size_t i = 0; i < debug_symbol_count; i++) {
            Stru_DebugSymbol_t* symbol = &program->debug_symbols[i];
            
            if (ptr + sizeof(uint32_t) * 2 > end) {
                F_destroy_bytecode_program(program);
                return NULL;
            }
            
            memcpy(&symbol->offset, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            memcpy(&symbol->type, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            
            uint32_t name_len;
            if (ptr + sizeof(uint32_t) > end) {
                F_destroy_bytecode_program(program);
                return NULL;
            }
            
            memcpy(&name_len, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            
            if (name_len > 0) {
                if (ptr + name_len > end) {
                    F_destroy_bytecode_program(program);
                    return NULL;
                }
                
                symbol->name = (char*)malloc(name_len + 1);
                if (!symbol->name) {
                    F_destroy_bytecode_program(program);
                    return NULL;
                }
                
                memcpy((void*)symbol->name, ptr, name_len);
                ptr += name_len;
                ((char*)symbol->name)[name_len] = '\0';
            } else {
                symbol->name = NULL;
            }
        }
    }
    
    return program;
}
