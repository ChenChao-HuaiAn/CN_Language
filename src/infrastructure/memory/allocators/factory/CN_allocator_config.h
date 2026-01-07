/**
 * @file CN_allocator_config.h
 * @brief 分配器配置结构定义
 * 
 * 定义了分配器配置结构和相关函数，用于配置各种分配器的行为。
 * 遵循项目编码标准和架构规范。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_ALLOCATOR_CONFIG_H
#define CN_ALLOCATOR_CONFIG_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 分配器配置结构
 * 
 * 用于配置分配器的各种参数，支持灵活的配置选项。
 */
typedef struct Stru_AllocatorConfig_t Stru_AllocatorConfig_t;

/**
 * @brief 分配器配置结构定义
 */
struct Stru_AllocatorConfig_t {
    /**
     * @brief 分配器名称（用于调试和日志）
     * 
     * 为分配器指定一个描述性名称，便于调试和日志记录。
     * 如果为NULL，将使用默认名称。
     */
    const char* name;
    
    /**
     * @brief 是否启用线程安全
     * 
     * 如果为true，分配器将提供线程安全保证。
     * 默认值为false，以提高单线程环境下的性能。
     */
    bool thread_safe;
    
    /**
     * @brief 是否启用统计信息收集
     * 
     * 如果为true，分配器将收集详细的统计信息。
     * 默认值为false，以减少内存开销。
     */
    bool enable_statistics;
    
    /**
     * @brief 是否启用调试功能
     * 
     * 如果为true，分配器将启用调试功能，如边界检查、泄漏检测等。
     * 默认值为false，以提高性能。
     */
    bool enable_debugging;
    
    /**
     * @brief 内存对齐要求
     * 
     * 指定内存分配的对齐要求。0表示使用分配器默认对齐。
     * 必须是2的幂次方。
     */
    size_t alignment;
    
    /**
     * @brief 最大分配大小限制
     * 
     * 指定单个分配的最大字节数。0表示无限制。
     * 用于防止意外的大内存分配。
     */
    size_t max_allocation_size;
    
    /**
     * @brief 分配失败回调函数
     * 
     * 当内存分配失败时调用的回调函数。
     * 可以为NULL，表示不处理分配失败。
     */
    void (*allocation_failure_callback)(size_t requested_size);
    
    /**
     * @brief 自定义配置数据
     * 
     * 指向自定义配置数据的指针，用于扩展配置选项。
     * 分配器工厂不管理此内存，由调用者负责生命周期。
     */
    void* custom_data;
    
    /**
     * @brief 自定义配置数据大小
     * 
     * 自定义配置数据的大小（字节数）。
     * 如果custom_data为NULL，此值应为0。
     */
    size_t custom_data_size;
};

/**
 * @brief 创建默认分配器配置
 * 
 * 创建并返回一个默认的分配器配置。
 * 调用者负责释放返回的配置对象。
 * 
 * @return Stru_AllocatorConfig_t* 默认配置对象，失败返回NULL
 */
Stru_AllocatorConfig_t* F_create_default_allocator_config(void);

/**
 * @brief 复制分配器配置
 * 
 * 创建配置对象的深拷贝。
 * 
 * @param src 源配置对象
 * @return Stru_AllocatorConfig_t* 新的配置对象，失败返回NULL
 */
Stru_AllocatorConfig_t* F_copy_allocator_config(const Stru_AllocatorConfig_t* src);

/**
 * @brief 释放分配器配置
 * 
 * 释放配置对象占用的内存。
 * 
 * @param config 要释放的配置对象
 */
void F_free_allocator_config(Stru_AllocatorConfig_t* config);

/**
 * @brief 验证分配器配置
 * 
 * 验证配置对象的有效性，检查参数是否在合理范围内。
 * 
 * @param config 要验证的配置对象
 * @return bool 配置有效返回true，否则返回false
 */
bool F_validate_allocator_config(const Stru_AllocatorConfig_t* config);

/**
 * @brief 设置分配器名称
 * 
 * 设置配置对象的分配器名称。
 * 函数会复制名称字符串。
 * 
 * @param config 配置对象
 * @param name 分配器名称
 * @return bool 成功返回true，失败返回false
 */
bool F_set_allocator_name(Stru_AllocatorConfig_t* config, const char* name);

/**
 * @brief 设置自定义配置数据
 * 
 * 设置配置对象的自定义数据。
 * 函数会复制数据内容。
 * 
 * @param config 配置对象
 * @param data 自定义数据指针
 * @param size 数据大小
 * @return bool 成功返回true，失败返回false
 */
bool F_set_custom_data(Stru_AllocatorConfig_t* config, const void* data, size_t size);

/**
 * @brief 获取配置摘要
 * 
 * 获取配置对象的文本摘要，用于调试和日志。
 * 
 * @param config 配置对象
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return bool 成功返回true，失败返回false
 */
bool F_get_config_summary(const Stru_AllocatorConfig_t* config, char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* CN_ALLOCATOR_CONFIG_H */
