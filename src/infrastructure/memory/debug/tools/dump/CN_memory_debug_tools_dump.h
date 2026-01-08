/**
 * @file CN_memory_debug_tools_dump.h
 * @brief 内存转储模块
 * 
 * 负责内存内容的转储和格式化输出。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_DEBUG_TOOLS_DUMP_H
#define CN_MEMORY_DEBUG_TOOLS_DUMP_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 内存转储选项
 */
typedef struct Stru_MemoryDumpOptions_t
{
    bool show_address;           ///< 显示地址
    bool show_hex;               ///< 显示十六进制
    bool show_ascii;             ///< 显示ASCII字符
    bool show_offset;            ///< 显示偏移量
    size_t bytes_per_line;       ///< 每行字节数
    size_t max_bytes;            ///< 最大转储字节数
    const char* label;           ///< 转储标签
} Stru_MemoryDumpOptions_t;

/**
 * @brief 转储内存内容
 * 
 * 转储指定内存区域的内容。
 * 
 * @param address 内存地址
 * @param size 内存大小
 * @param options 转储选项
 */
void F_dump_memory(const void* address, size_t size, 
                  const Stru_MemoryDumpOptions_t* options);

/**
 * @brief 转储内存内容到字符串
 * 
 * 转储指定内存区域的内容到字符串缓冲区。
 * 
 * @param address 内存地址
 * @param size 内存大小
 * @param options 转储选项
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return size_t 实际写入的字符数（不包括终止符）
 */
size_t F_dump_memory_to_string(const void* address, size_t size,
                              const Stru_MemoryDumpOptions_t* options,
                              char* buffer, size_t buffer_size);

/**
 * @brief 获取默认转储选项
 * 
 * 获取默认的内存转储选项。
 * 
 * @return Stru_MemoryDumpOptions_t 默认转储选项
 */
Stru_MemoryDumpOptions_t F_get_default_dump_options(void);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_DEBUG_TOOLS_DUMP_H
