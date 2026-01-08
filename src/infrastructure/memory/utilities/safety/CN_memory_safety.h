/**
 * @file CN_memory_safety.h
 * @brief 内存安全函数接口
 * 
 * 提供安全的内存操作函数，包括安全清零、内存验证和模式初始化。
 * 遵循防御性编程原则，所有函数都进行参数验证。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_SAFETY_H
#define CN_MEMORY_SAFETY_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 内存安全接口结构体
 * 
 * 定义了内存安全的核心功能，支持安全的内存操作和验证。
 */
typedef struct Stru_MemorySafetyInterface_t Stru_MemorySafetyInterface_t;

/**
 * @brief 内存安全接口
 */
struct Stru_MemorySafetyInterface_t
{
    /**
     * @brief 安全清零内存
     * 
     * 将内存块清零，防止编译器优化。
     * 用于清零敏感数据，如密码、密钥等。
     * 
     * @param ptr 内存指针
     * @param n 要清零的字节数
     */
    void (*secure_zero)(void* ptr, size_t n);
    
    /**
     * @brief 验证内存范围
     * 
     * 验证内存范围是否有效（非NULL且大小合理）。
     * 注意：此函数不保证内存可访问性。
     * 
     * @param ptr 内存指针
     * @param n 内存大小
     * @return bool 验证通过返回true，否则返回false
     */
    bool (*validate_range)(const void* ptr, size_t n);
    
    /**
     * @brief 使用模式初始化内存
     * 
     * 使用指定模式初始化内存块。
     * 用于测试和调试，确保内存被正确初始化。
     * 
     * @param ptr 内存指针
     * @param n 内存大小
     * @param pattern 初始化模式（0-255）
     */
    void (*initialize_with_pattern)(void* ptr, size_t n, uint8_t pattern);
    
    /**
     * @brief 检查内存是否已初始化
     * 
     * 检查内存块是否已初始化（非随机值）。
     * 通过检查内存内容是否包含特定模式来判断。
     * 
     * @param ptr 内存指针
     * @param n 内存大小
     * @param pattern 预期的初始化模式
     * @return bool 已初始化返回true，否则返回false
     */
    bool (*check_initialized)(const void* ptr, size_t n, uint8_t pattern);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

/**
 * @brief 创建内存安全接口实例
 * 
 * 创建默认的内存安全接口实例。
 * 
 * @return Stru_MemorySafetyInterface_t* 内存安全接口指针
 */
Stru_MemorySafetyInterface_t* F_create_memory_safety(void);

/**
 * @brief 获取全局内存安全接口实例
 * 
 * 返回全局的内存安全接口单例实例。
 * 如果尚未创建，则创建新的实例。
 * 
 * @return Stru_MemorySafetyInterface_t* 全局内存安全接口实例
 */
Stru_MemorySafetyInterface_t* F_get_global_memory_safety(void);

/**
 * @brief 销毁内存安全接口实例
 * 
 * 释放内存安全接口实例占用的资源。
 * 
 * @param safety 要销毁的内存安全接口
 */
void F_destroy_memory_safety(Stru_MemorySafetyInterface_t* safety);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_SAFETY_H
