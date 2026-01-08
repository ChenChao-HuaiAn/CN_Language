/**
 * @file CN_memory_safety.c
 * @brief 内存安全函数实现
 * 
 * 实现了安全的内存操作函数，包括安全清零、内存验证和模式初始化。
 * 遵循防御性编程原则，所有函数都进行参数验证。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_memory_safety.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief 内存安全接口实现结构体
 */
typedef struct
{
    Stru_MemorySafetyInterface_t interface;  ///< 接口定义
    // 可以添加私有数据字段
} MemorySafetyImpl;

/**
 * @brief 安全清零内存实现
 */
static void secure_zero_impl(void* ptr, size_t n)
{
    if (ptr == NULL || n == 0)
    {
        return;
    }
    
    // 使用volatile防止编译器优化
    volatile unsigned char* p = (volatile unsigned char*)ptr;
    while (n--)
    {
        *p++ = 0;
    }
}

/**
 * @brief 验证内存范围实现
 */
static bool validate_range_impl(const void* ptr, size_t n)
{
    // 检查指针是否为NULL
    if (ptr == NULL)
    {
        return false;
    }
    
    // 检查大小是否合理（防止整数溢出）
    if (n == 0 || n > SIZE_MAX / 2)
    {
        return false;
    }
    
    // 注意：这里无法真正验证内存是否可访问
    // 这只是一个基本的合理性检查
    return true;
}

/**
 * @brief 使用模式初始化内存实现
 */
static void initialize_with_pattern_impl(void* ptr, size_t n, uint8_t pattern)
{
    if (ptr == NULL || n == 0)
    {
        return;
    }
    
    memset(ptr, pattern, n);
}

/**
 * @brief 检查内存是否已初始化实现
 */
static bool check_initialized_impl(const void* ptr, size_t n, uint8_t pattern)
{
    if (ptr == NULL || n == 0)
    {
        return false;
    }
    
    const unsigned char* p = (const unsigned char*)ptr;
    for (size_t i = 0; i < n; i++)
    {
        if (p[i] != pattern)
        {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 创建内存安全接口实例
 */
Stru_MemorySafetyInterface_t* F_create_memory_safety(void)
{
    MemorySafetyImpl* impl = (MemorySafetyImpl*)malloc(sizeof(MemorySafetyImpl));
    if (impl == NULL)
    {
        return NULL;
    }
    
    // 初始化接口函数指针
    impl->interface.secure_zero = secure_zero_impl;
    impl->interface.validate_range = validate_range_impl;
    impl->interface.initialize_with_pattern = initialize_with_pattern_impl;
    impl->interface.check_initialized = check_initialized_impl;
    impl->interface.private_data = NULL;
    
    return (Stru_MemorySafetyInterface_t*)impl;
}

/**
 * @brief 全局内存安全接口实例
 */
static Stru_MemorySafetyInterface_t* g_global_memory_safety = NULL;

/**
 * @brief 获取全局内存安全接口实例
 */
Stru_MemorySafetyInterface_t* F_get_global_memory_safety(void)
{
    if (g_global_memory_safety == NULL)
    {
        g_global_memory_safety = F_create_memory_safety();
    }
    return g_global_memory_safety;
}

/**
 * @brief 销毁内存安全接口实例
 */
void F_destroy_memory_safety(Stru_MemorySafetyInterface_t* safety)
{
    if (safety != NULL)
    {
        free(safety);
        
        // 如果是全局实例，重置指针
        if (safety == g_global_memory_safety)
        {
            g_global_memory_safety = NULL;
        }
    }
}
