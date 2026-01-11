/******************************************************************************
 * 文件名: CN_bytecode_formatter.c
 * 功能: 字节码格式化器模块
 * 
 * 提供字节码程序的格式化功能，将字节码程序转换为可读的文本格式。
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
// 公共函数实现
// ============================================================================

/**
 * @brief 格式化字节码程序为文本
 */
char* F_format_bytecode_program_impl(const Stru_BytecodeProgram_t* program)
{
    if (!program) {
        return NULL;
    }
    
    // 计算所需的总大小
    size_t total_size = 0;
    
    // 计算头部信息所需大小
    total_size += 256;  // 头部信息
    
    // 计算指令列表所需大小
    for (size_t i = 0; i < program->instruction_count; i++) {
        const Stru_BytecodeInstruction_t* inst = &program->instructions[i];
        total_size += 128;  // 每条指令的格式化字符串
        
        if (inst->comment) {
            total_size += strlen(inst->comment) + 10;  // 注释
        }
    }
    
    // 计算常量池所需大小
    total_size += 128;  // 常量池标题
    for (size_t i = 0; i < program->constant_count; i++) {
        if (program->constant_pool[i]) {
            total_size += strlen(program->constant_pool[i]) + 32;  // 常量条目
        }
    }
    
    // 计算调试信息所需大小
    if (program->debug_symbol_count > 0) {
        total_size += 128;  // 调试信息标题
        for (size_t i = 0; i < program->debug_symbol_count; i++) {
            const Stru_DebugSymbol_t* symbol = &program->debug_symbols[i];
            if (symbol->name) {
                total_size += strlen(symbol->name) + 32;
            }
        }
    }
    
    // 分配内存
    char* buffer = (char*)malloc(total_size);
    if (!buffer) {
        return NULL;
    }
    
    char* ptr = buffer;
    int written = 0;
    
    // 写入头部信息
    written = snprintf(ptr, total_size - (ptr - buffer),
        "CN_Language 字节码程序\n"
        "=====================\n"
        "指令数量: %zu\n"
        "常量数量: %zu\n"
        "栈大小: %zu 字节\n"
        "堆大小: %zu 字节\n"
        "全局变量数量: %zu\n"
        "入口点: %s (偏移: %u)\n"
        "行号表大小: %zu\n"
        "调试符号数量: %zu\n\n",
        program->instruction_count,
        program->constant_count,
        program->stack_size,
        program->heap_size,
        program->global_count,
        program->entry_point ? program->entry_point : "(无)",
        program->entry_point_offset,
        program->line_table_size,
        program->debug_symbol_count);
    
    if (written > 0) {
        ptr += written;
    }
    
    // 写入指令列表
    written = snprintf(ptr, total_size - (ptr - buffer), "指令列表:\n");
    if (written > 0) {
        ptr += written;
    }
    
    for (size_t i = 0; i < program->instruction_count; i++) {
        const Stru_BytecodeInstruction_t* inst = &program->instructions[i];
        
        // 格式化指令
        if (inst->comment) {
            written = snprintf(ptr, total_size - (ptr - buffer),
                "  [%04zu] 操作码: %3u  行: %3u  列: %3u  # %s\n",
                i, inst->opcode, inst->line, inst->column, inst->comment);
        } else {
            written = snprintf(ptr, total_size - (ptr - buffer),
                "  [%04zu] 操作码: %3u  行: %3u  列: %3u\n",
                i, inst->opcode, inst->line, inst->column);
        }
        
        if (written > 0) {
            ptr += written;
        }
    }
    
    written = snprintf(ptr, total_size - (ptr - buffer), "\n");
    if (written > 0) {
        ptr += written;
    }
    
    // 写入常量池
    written = snprintf(ptr, total_size - (ptr - buffer), "常量池:\n");
    if (written > 0) {
        ptr += written;
    }
    
    for (size_t i = 0; i < program->constant_count; i++) {
        if (program->constant_pool[i]) {
            written = snprintf(ptr, total_size - (ptr - buffer),
                "  [%04zu] \"%s\"\n", i, program->constant_pool[i]);
        } else {
            written = snprintf(ptr, total_size - (ptr - buffer),
                "  [%04zu] (空)\n", i);
        }
        
        if (written > 0) {
            ptr += written;
        }
    }
    
    written = snprintf(ptr, total_size - (ptr - buffer), "\n");
    if (written > 0) {
        ptr += written;
    }
    
    // 写入调试符号表（如果有）
    if (program->debug_symbol_count > 0) {
        written = snprintf(ptr, total_size - (ptr - buffer), "调试符号表:\n");
        if (written > 0) {
            ptr += written;
        }
        
        for (size_t i = 0; i < program->debug_symbol_count; i++) {
            const Stru_DebugSymbol_t* symbol = &program->debug_symbols[i];
            
            if (symbol->name) {
                written = snprintf(ptr, total_size - (ptr - buffer),
                    "  [%04zu] 偏移: %u  类型: %u  名称: %s\n",
                    i, symbol->offset, symbol->type, symbol->name);
            } else {
                written = snprintf(ptr, total_size - (ptr - buffer),
                    "  [%04zu] 偏移: %u  类型: %u  名称: (无名)\n",
                    i, symbol->offset, symbol->type);
            }
            
            if (written > 0) {
                ptr += written;
            }
        }
        
        written = snprintf(ptr, total_size - (ptr - buffer), "\n");
        if (written > 0) {
            ptr += written;
        }
    }
    
    // 写入行号表（如果有）
    if (program->line_table_size > 0 && program->line_table) {
        written = snprintf(ptr, total_size - (ptr - buffer), "行号表:\n");
        if (written > 0) {
            ptr += written;
        }
        
        // 这里简化处理，实际应该格式化行号表
        written = snprintf(ptr, total_size - (ptr - buffer),
            "  (包含 %zu 个行号条目)\n", program->line_table_size);
        if (written > 0) {
            ptr += written;
        }
        
        written = snprintf(ptr, total_size - (ptr - buffer), "\n");
        if (written > 0) {
            ptr += written;
        }
    }
    
    // 确保字符串以空字符结尾
    *ptr = '\0';
    
    return buffer;
}
