/**
 * @file CN_memory_debug_tools_validation.h
 * @brief 内存验证模块
 * 
 * 负责内存验证、模式检查和完整性验证。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_DEBUG_TOOLS_VALIDATION_H
#define CN_MEMORY_DEBUG_TOOLS_VALIDATION_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 验证内存块完整性
 * 
 * 验证指定内存区域的完整性。
 * 
 * @param address 内存地址
 * @param size 内存大小
 * @param pattern 验证模式（可选）
 * @param pattern_size 模式大小
 * @return bool 验证成功返回true，否则返回false
 */
bool F_validate_memory_block(const void* address, size_t size,
                            const void* pattern, size_t pattern_size);

/**
 * @brief 填充内存模式
 * 
 * 使用指定模式填充内存区域。
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
 * @return bool 匹配成功返回true，否则返回false
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
 * @return uint32_t 校验和值
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
 * @return int 比较结果：0表示相等，正数表示addr1>addr2，负数表示addr1<addr2
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

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_DEBUG_TOOLS_VALIDATION_H
