/**
 * @file CN_memory_debug_tools_dump.c
 * @brief 内存转储模块实现
 * 
 * 实现了内存内容的转储和格式化输出功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include "CN_memory_debug_tools_dump.h"
#include "../core/CN_memory_debug_tools_core.h"
#include "../utils/CN_memory_debug_tools_utils.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief 转储内存内容
 */
void F_dump_memory(const void* address, size_t size, 
                  const Stru_MemoryDumpOptions_t* options)
{
    if (!F_check_debug_tools_initialized() || address == NULL || size == 0)
    {
        return;
    }
    
    // 使用默认选项
    Stru_MemoryDumpOptions_t default_options = {
        .show_address = true,
        .show_hex = true,
        .show_ascii = true,
        .show_offset = true,
        .bytes_per_line = 16,
        .max_bytes = size,
        .label = "内存转储"
    };
    
    const Stru_MemoryDumpOptions_t* opts = 
        (options != NULL) ? options : &default_options;
    
    // 限制转储大小
    size_t dump_size = (opts->max_bytes > 0 && opts->max_bytes < size) ? 
                       opts->max_bytes : size;
    
    // 打印标签
    if (opts->label != NULL)
    {
        F_debug_output("=== %s ===", opts->label);
    }
    
    // 打印地址信息
    if (opts->show_address)
    {
        F_debug_output("地址: %p, 大小: %zu 字节", address, dump_size);
    }
    
    const uint8_t* data = (const uint8_t*)address;
    size_t bytes_per_line = (opts->bytes_per_line > 0) ? opts->bytes_per_line : 16;
    
    // 逐行转储
    for (size_t offset = 0; offset < dump_size; offset += bytes_per_line)
    {
        size_t line_size = (dump_size - offset < bytes_per_line) ? 
                          (dump_size - offset) : bytes_per_line;
        
        char line_buffer[256];
        char* ptr = line_buffer;
        size_t remaining = sizeof(line_buffer);
        
        // 添加偏移量
        if (opts->show_offset)
        {
            int written = snprintf(ptr, remaining, "%08zX: ", offset);
            if (written > 0 && (size_t)written < remaining)
            {
                ptr += written;
                remaining -= written;
            }
        }
        
        // 添加十六进制表示
        if (opts->show_hex)
        {
            for (size_t i = 0; i < line_size; i++)
            {
                if (i > 0 && i % 8 == 0)
                {
                    // 每8个字节添加一个空格
                    if (remaining > 1)
                    {
                        *ptr++ = ' ';
                        remaining--;
                    }
                }
                
                if (remaining > 2)
                {
                    F_byte_to_hex(ptr, data[offset + i]);
                    ptr += 2;
                    remaining -= 2;
                    
                    if (i < line_size - 1)
                    {
                        *ptr++ = ' ';
                        remaining--;
                    }
                }
            }
            
            // 填充对齐
            if (line_size < bytes_per_line)
            {
                size_t padding = (bytes_per_line - line_size) * 3;
                if (line_size % 8 != 0)
                {
                    padding++; // 额外的空格
                }
                
                if (padding < remaining)
                {
                    memset(ptr, ' ', padding);
                    ptr += padding;
                    remaining -= padding;
                }
            }
        }
        
        // 添加ASCII表示
        if (opts->show_ascii)
        {
            if (remaining > 3)
            {
                *ptr++ = ' ';
                *ptr++ = '|';
                *ptr++ = ' ';
                remaining -= 3;
            }
            
            for (size_t i = 0; i < line_size && remaining > 1; i++)
            {
                char c = (char)data[offset + i];
                *ptr++ = F_is_printable_char(c) ? c : '.';
                remaining--;
            }
        }
        
        // 结束字符串
        if (remaining > 0)
        {
            *ptr = '\0';
        }
        
        // 输出行
        F_debug_output("%s", line_buffer);
    }
    
    F_debug_output("=== 转储完成 ===");
}

/**
 * @brief 转储内存内容到字符串
 */
size_t F_dump_memory_to_string(const void* address, size_t size,
                              const Stru_MemoryDumpOptions_t* options,
                              char* buffer, size_t buffer_size)
{
    if (!F_check_debug_tools_initialized() || address == NULL || size == 0 || 
        buffer == NULL || buffer_size == 0)
    {
        return 0;
    }
    
    // 使用默认选项
    Stru_MemoryDumpOptions_t default_options = {
        .show_address = false,
        .show_hex = true,
        .show_ascii = true,
        .show_offset = true,
        .bytes_per_line = 16,
        .max_bytes = size,
        .label = NULL
    };
    
    const Stru_MemoryDumpOptions_t* opts = 
        (options != NULL) ? options : &default_options;
    
    // 限制转储大小
    size_t dump_size = (opts->max_bytes > 0 && opts->max_bytes < size) ? 
                       opts->max_bytes : size;
    
    char* ptr = buffer;
    size_t remaining = buffer_size;
    size_t total_written = 0;
    
    // 打印标签
    if (opts->label != NULL && remaining > 0)
    {
        int written = snprintf(ptr, remaining, "=== %s ===\n", opts->label);
        if (written > 0 && (size_t)written < remaining)
        {
            ptr += written;
            remaining -= written;
            total_written += written;
        }
    }
    
    // 打印地址信息
    if (opts->show_address && remaining > 0)
    {
        int written = snprintf(ptr, remaining, "地址: %p, 大小: %zu 字节\n", 
                              address, dump_size);
        if (written > 0 && (size_t)written < remaining)
        {
            ptr += written;
            remaining -= written;
            total_written += written;
        }
    }
    
    const uint8_t* data = (const uint8_t*)address;
    size_t bytes_per_line = (opts->bytes_per_line > 0) ? opts->bytes_per_line : 16;
    
    // 逐行转储
    for (size_t offset = 0; offset < dump_size && remaining > 0; offset += bytes_per_line)
    {
        size_t line_size = (dump_size - offset < bytes_per_line) ? 
                          (dump_size - offset) : bytes_per_line;
        
        char line_buffer[128];
        char* line_ptr = line_buffer;
        size_t line_remaining = sizeof(line_buffer);
        
        // 添加偏移量
        if (opts->show_offset)
        {
            int written = snprintf(line_ptr, line_remaining, "%08zX: ", offset);
            if (written > 0 && (size_t)written < line_remaining)
            {
                line_ptr += written;
                line_remaining -= written;
            }
        }
        
        // 添加十六进制表示
        if (opts->show_hex)
        {
            for (size_t i = 0; i < line_size; i++)
            {
                if (i > 0 && i % 8 == 0 && line_remaining > 1)
                {
                    *line_ptr++ = ' ';
                    line_remaining--;
                }
                
                if (line_remaining > 2)
                {
                    F_byte_to_hex(line_ptr, data[offset + i]);
                    line_ptr += 2;
                    line_remaining -= 2;
                    
                    if (i < line_size - 1 && line_remaining > 1)
                    {
                        *line_ptr++ = ' ';
                        line_remaining--;
                    }
                }
            }
            
            // 填充对齐
            if (line_size < bytes_per_line)
            {
                size_t padding = (bytes_per_line - line_size) * 3;
                if (line_size % 8 != 0)
                {
                    padding++; // 额外的空格
                }
                
                if (padding < line_remaining)
                {
                    memset(line_ptr, ' ', padding);
                    line_ptr += padding;
                    line_remaining -= padding;
                }
            }
        }
        
        // 添加ASCII表示
        if (opts->show_ascii)
        {
            if (line_remaining > 3)
            {
                *line_ptr++ = ' ';
                *line_ptr++ = '|';
                *line_ptr++ = ' ';
                line_remaining -= 3;
            }
            
            for (size_t i = 0; i < line_size && line_remaining > 1; i++)
            {
                char c = (char)data[offset + i];
                *line_ptr++ = F_is_printable_char(c) ? c : '.';
                line_remaining--;
            }
        }
        
        // 结束行
        if (line_remaining > 1)
        {
            *line_ptr++ = '\n';
            *line_ptr = '\0';
        }
        else if (line_remaining > 0)
        {
            *line_ptr = '\0';
        }
        
        // 复制到输出缓冲区
        size_t line_len = strlen(line_buffer);
        if (line_len < remaining)
        {
            strcpy(ptr, line_buffer);
            ptr += line_len;
            remaining -= line_len;
            total_written += line_len;
        }
        else
        {
            // 缓冲区不足
            break;
        }
    }
    
    return total_written;
}

/**
 * @brief 获取默认转储选项
 */
Stru_MemoryDumpOptions_t F_get_default_dump_options(void)
{
    Stru_MemoryDumpOptions_t options = {
        .show_address = true,
        .show_hex = true,
        .show_ascii = true,
        .show_offset = true,
        .bytes_per_line = 16,
        .max_bytes = 0, // 0表示不限制
        .label = "内存转储"
    };
    
    return options;
}
