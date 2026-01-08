/**
 * @file CN_memory_debug_tools_utils.c
 * @brief 工具函数模块实现
 * 
 * 实现了内存调试工具的各种辅助函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include "CN_memory_debug_tools_utils.h"
#include "../core/CN_memory_debug_tools_core.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/**
 * @brief 将字节转换为十六进制字符串
 */
void F_byte_to_hex(char* hex, uint8_t byte)
{
    static const char* hex_digits = "0123456789ABCDEF";
    hex[0] = hex_digits[(byte >> 4) & 0x0F];
    hex[1] = hex_digits[byte & 0x0F];
}

/**
 * @brief 检查字符是否可打印
 */
bool F_is_printable_char(char c)
{
    return (c >= 32 && c <= 126);
}

/**
 * @brief 获取内存信息字符串
 */
size_t F_get_memory_info_string(const void* address,
                               char* buffer, size_t buffer_size)
{
    if (!F_check_debug_tools_initialized() || address == NULL || buffer == NULL || buffer_size == 0)
    {
        return 0;
    }
    
    // 简化实现：返回基本内存信息
    const char* info = 
        "内存信息:\n"
        "  地址: %p\n"
        "  对齐: 未知\n"
        "  可读: 是\n"
        "  可写: 是\n"
        "  可执行: 未知\n";
    
    size_t info_len = strlen(info) + 32; // 为地址预留空间
    if (info_len < buffer_size)
    {
        snprintf(buffer, buffer_size, info, address);
        return strlen(buffer);
    }
    else
    {
        size_t copy_len = buffer_size - 1;
        strncpy(buffer, info, copy_len);
        buffer[copy_len] = '\0';
        return copy_len;
    }
}

/**
 * @brief 格式化内存地址
 */
size_t F_format_memory_address(const void* address,
                              char* buffer, size_t buffer_size)
{
    if (!F_check_debug_tools_initialized() || address == NULL || buffer == NULL || buffer_size == 0)
    {
        return 0;
    }
    
    return snprintf(buffer, buffer_size, "%p", address);
}

/**
 * @brief 计算内存对齐
 */
bool F_check_memory_alignment(const void* address, size_t alignment)
{
    if (!F_check_debug_tools_initialized() || address == NULL || alignment == 0)
    {
        return false;
    }
    
    // 检查对齐是否为2的幂
    if ((alignment & (alignment - 1)) != 0)
    {
        return false;
    }
    
    // 检查地址是否对齐
    return ((uintptr_t)address & (alignment - 1)) == 0;
}
