/**
 * @file CN_memory_debug_tools_utils.h
 * @brief 工具函数模块
 * 
 * 提供内存调试工具的各种辅助函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_DEBUG_TOOLS_UTILS_H
#define CN_MEMORY_DEBUG_TOOLS_UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 将字节转换为十六进制字符串
 * 
 * 将单个字节转换为两位十六进制字符串。
 * 
 * @param hex 输出缓冲区（至少3字节）
 * @param byte 要转换的字节
 */
void F_byte_to_hex(char* hex, uint8_t byte);

/**
 * @brief 检查字符是否可打印
 * 
 * 检查字符是否为可打印ASCII字符。
 * 
 * @param c 要检查的字符
 * @return bool 可打印返回true，否则返回false
 */
bool F_is_printable_char(char c);

/**
 * @brief 获取内存信息字符串
 * 
 * 获取内存地址的基本信息字符串。
 * 
 * @param address 内存地址
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return size_t 实际写入的字符数（不包括终止符）
 */
size_t F_get_memory_info_string(const void* address,
                               char* buffer, size_t buffer_size);

/**
 * @brief 格式化内存地址
 * 
 * 格式化内存地址为字符串。
 * 
 * @param address 内存地址
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return size_t 实际写入的字符数（不包括终止符）
 */
size_t F_format_memory_address(const void* address,
                              char* buffer, size_t buffer_size);

/**
 * @brief 计算内存对齐
 * 
 * 计算内存地址的对齐情况。
 * 
 * @param address 内存地址
 * @param alignment 期望的对齐值
 * @return bool 对齐正确返回true，否则返回false
 */
bool F_check_memory_alignment(const void* address, size_t alignment);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_DEBUG_TOOLS_UTILS_H
