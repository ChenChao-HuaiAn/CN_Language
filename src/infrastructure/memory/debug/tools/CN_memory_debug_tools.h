/**
 * @file CN_memory_debug_tools.h
 * @brief 内存调试工具模块
 * 
 * 本模块提供了各种内存调试工具，包括：
 * - 内存转储
 * - 内存验证
 * - 调试信息输出
 * - 调用栈跟踪
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_DEBUG_TOOLS_H
#define CN_MEMORY_DEBUG_TOOLS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

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
 * @brief 调用栈跟踪选项
 */
typedef struct Stru_StackTraceOptions_t
{
    size_t max_depth;            ///< 最大跟踪深度
    bool show_function_names;    ///< 显示函数名
    bool show_file_names;        ///< 显示文件名
    bool show_line_numbers;      ///< 显示行号
    bool show_addresses;         ///< 显示地址
} Stru_StackTraceOptions_t;

/**
 * @brief 调试输出回调函数类型
 */
typedef void (*DebugOutputCallback)(const char* message);

/**
 * @brief 初始化调试工具
 * 
 * 初始化调试工具模块。
 * 
 * @return bool 初始化成功返回true，否则返回false
 */
bool F_initialize_debug_tools(void);

/**
 * @brief 清理调试工具
 * 
 * 清理调试工具模块，释放相关资源。
 */
void F_cleanup_debug_tools(void);

/**
 * @brief 设置调试输出回调
 * 
 * 设置调试信息输出回调函数。
 * 
 * @param callback 回调函数
 */
void F_set_debug_output_callback(DebugOutputCallback callback);

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
 * @brief 获取调用栈跟踪
 * 
 * 获取当前调用栈跟踪。
 * 
 * @param options 跟踪选项
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return size_t 实际写入的字符数（不包括终止符）
 */
size_t F_get_stack_trace(const Stru_StackTraceOptions_t* options,
                        char* buffer, size_t buffer_size);

/**
 * @brief 获取调用栈跟踪数组
 * 
 * 获取当前调用栈跟踪的地址数组。
 * 
 * @param addresses 输出地址数组
 * @param max_depth 最大跟踪深度
 * @return size_t 实际获取的地址数量
 */
size_t F_get_stack_trace_addresses(void** addresses, size_t max_depth);

/**
 * @brief 验证内存块完整性
 * 
 * 验证内存块的完整性。
 * 
 * @param address 内存地址
 * @param size 内存大小
 * @param pattern 验证模式（可选）
 * @param pattern_size 模式大小
 * @return bool 验证通过返回true，否则返回false
 */
bool F_validate_memory_block(const void* address, size_t size,
                            const void* pattern, size_t pattern_size);

/**
 * @brief 填充内存模式
 * 
 * 用指定模式填充内存区域。
 * 
 * @param address 内存地址
 * @param size 内存大小
 * @param pattern 填充模式
 * @param pattern_size 模式大小
 */
void F_fill_memory_pattern(void* address, size_t size,
                          const void* pattern, size_t pattern_size);

/**
 * @brief 检查内存模式
 * 
 * 检查内存区域是否匹配指定模式。
 * 
 * @param address 内存地址
 * @param size 内存大小
 * @param pattern 检查模式
 * @param pattern_size 模式大小
 * @return bool 匹配返回true，否则返回false
 */
bool F_check_memory_pattern(const void* address, size_t size,
                           const void* pattern, size_t pattern_size);

/**
 * @brief 计算内存校验和
 * 
 * 计算内存区域的校验和。
 * 
 * @param address 内存地址
 * @param size 内存大小
 * @return uint32_t 校验和
 */
uint32_t F_calculate_memory_checksum(const void* address, size_t size);

/**
 * @brief 比较内存区域
 * 
 * 比较两个内存区域的内容。
 * 
 * @param addr1 第一个内存地址
 * @param addr2 第二个内存地址
 * @param size 比较大小
 * @return int 比较结果：0表示相等，负数表示addr1<addr2，正数表示addr1>addr2
 */
int F_compare_memory_regions(const void* addr1, const void* addr2, size_t size);

/**
 * @brief 查找内存模式
 * 
 * 在内存区域中查找指定模式。
 * 
 * @param address 内存地址
 * @param size 内存大小
 * @param pattern 查找模式
 * @param pattern_size 模式大小
 * @return void* 找到的位置，未找到返回NULL
 */
void* F_find_memory_pattern(const void* address, size_t size,
                           const void* pattern, size_t pattern_size);

/**
 * @brief 获取内存信息字符串
 * 
 * 获取内存信息的字符串表示。
 * 
 * @param address 内存地址
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return size_t 实际写入的字符数（不包括终止符）
 */
size_t F_get_memory_info_string(const void* address,
                               char* buffer, size_t buffer_size);

/**
 * @brief 启用详细调试输出
 * 
 * 启用或禁用详细的调试输出。
 * 
 * @param enable 是否启用
 */
void F_enable_verbose_debugging(bool enable);

/**
 * @brief 设置调试输出文件
 * 
 * 设置调试输出文件。
 * 
 * @param filename 文件名
 * @return bool 设置成功返回true，否则返回false
 */
bool F_set_debug_output_file(const char* filename);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_DEBUG_TOOLS_H
