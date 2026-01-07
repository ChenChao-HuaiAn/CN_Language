/**
 * @file CN_allocator_config.c
 * @brief 分配器配置结构实现
 * 
 * 实现了分配器配置结构的相关函数，包括创建、复制、验证和释放配置。
 * 遵循项目编码标准和架构规范。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_allocator_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief 分配器配置私有数据结构
 */
typedef struct Stru_AllocatorConfigData_t {
    char* name_copy;           ///< 名称字符串的副本
    void* custom_data_copy;    ///< 自定义数据的副本
    size_t custom_data_size;   ///< 自定义数据大小
} Stru_AllocatorConfigData_t;

/**
 * @brief 创建默认分配器配置
 */
Stru_AllocatorConfig_t* F_create_default_allocator_config(void)
{
    Stru_AllocatorConfig_t* config = (Stru_AllocatorConfig_t*)malloc(sizeof(Stru_AllocatorConfig_t));
    if (config == NULL) {
        return NULL;
    }
    
    // 分配私有数据
    Stru_AllocatorConfigData_t* private_data = (Stru_AllocatorConfigData_t*)malloc(sizeof(Stru_AllocatorConfigData_t));
    if (private_data == NULL) {
        free(config);
        return NULL;
    }
    
    // 初始化私有数据
    private_data->name_copy = NULL;
    private_data->custom_data_copy = NULL;
    private_data->custom_data_size = 0;
    
    // 设置默认值
    config->name = NULL;  // 使用私有数据中的副本
    config->thread_safe = false;
    config->enable_statistics = false;
    config->enable_debugging = false;
    config->alignment = 0;  // 使用默认对齐
    config->max_allocation_size = 0;  // 无限制
    config->allocation_failure_callback = NULL;
    config->custom_data = NULL;
    config->custom_data_size = 0;
    
    // 将私有数据附加到配置
    config->custom_data = private_data;
    config->custom_data_size = sizeof(Stru_AllocatorConfigData_t);
    
    return config;
}

/**
 * @brief 复制分配器配置
 */
Stru_AllocatorConfig_t* F_copy_allocator_config(const Stru_AllocatorConfig_t* src)
{
    if (src == NULL) {
        return NULL;
    }
    
    Stru_AllocatorConfig_t* dst = F_create_default_allocator_config();
    if (dst == NULL) {
        return NULL;
    }
    
    // 复制基本字段
    dst->thread_safe = src->thread_safe;
    dst->enable_statistics = src->enable_statistics;
    dst->enable_debugging = src->enable_debugging;
    dst->alignment = src->alignment;
    dst->max_allocation_size = src->max_allocation_size;
    dst->allocation_failure_callback = src->allocation_failure_callback;
    
    // 复制名称
    if (src->name != NULL) {
        if (!F_set_allocator_name(dst, src->name)) {
            F_free_allocator_config(dst);
            return NULL;
        }
    }
    
    // 复制自定义数据
    if (src->custom_data != NULL && src->custom_data_size > 0) {
        if (!F_set_custom_data(dst, src->custom_data, src->custom_data_size)) {
            F_free_allocator_config(dst);
            return NULL;
        }
    }
    
    return dst;
}

/**
 * @brief 释放分配器配置
 */
void F_free_allocator_config(Stru_AllocatorConfig_t* config)
{
    if (config == NULL) {
        return;
    }
    
    // 释放私有数据
    if (config->custom_data != NULL && config->custom_data_size == sizeof(Stru_AllocatorConfigData_t)) {
        Stru_AllocatorConfigData_t* private_data = (Stru_AllocatorConfigData_t*)config->custom_data;
        
        // 释放名称副本
        if (private_data->name_copy != NULL) {
            free(private_data->name_copy);
        }
        
        // 释放自定义数据副本
        if (private_data->custom_data_copy != NULL) {
            free(private_data->custom_data_copy);
        }
        
        free(private_data);
    }
    
    free(config);
}

/**
 * @brief 验证分配器配置
 */
bool F_validate_allocator_config(const Stru_AllocatorConfig_t* config)
{
    if (config == NULL) {
        return false;
    }
    
    // 检查对齐要求
    if (config->alignment != 0) {
        // 对齐必须是2的幂
        if ((config->alignment & (config->alignment - 1)) != 0) {
            return false;
        }
        
        // 对齐不能太大（例如，不能超过系统页大小）
        if (config->alignment > 4096) {  // 假设最大对齐为4KB
            return false;
        }
    }
    
    // 检查最大分配大小
    if (config->max_allocation_size > 0) {
        // 最大分配大小应该合理
        if (config->max_allocation_size > (1024ULL * 1024 * 1024 * 1024)) {  // 1TB
            return false;
        }
    }
    
    // 检查自定义数据一致性
    if ((config->custom_data == NULL && config->custom_data_size > 0) ||
        (config->custom_data != NULL && config->custom_data_size == 0)) {
        return false;
    }
    
    return true;
}

/**
 * @brief 设置分配器名称
 */
bool F_set_allocator_name(Stru_AllocatorConfig_t* config, const char* name)
{
    if (config == NULL || name == NULL) {
        return false;
    }
    
    // 获取私有数据
    if (config->custom_data == NULL || config->custom_data_size != sizeof(Stru_AllocatorConfigData_t)) {
        return false;
    }
    
    Stru_AllocatorConfigData_t* private_data = (Stru_AllocatorConfigData_t*)config->custom_data;
    
    // 释放旧名称
    if (private_data->name_copy != NULL) {
        free(private_data->name_copy);
        private_data->name_copy = NULL;
    }
    
    // 复制新名称
    size_t name_len = strlen(name);
    private_data->name_copy = (char*)malloc(name_len + 1);
    if (private_data->name_copy == NULL) {
        return false;
    }
    
    strcpy(private_data->name_copy, name);
    config->name = private_data->name_copy;
    
    return true;
}

/**
 * @brief 设置自定义配置数据
 */
bool F_set_custom_data(Stru_AllocatorConfig_t* config, const void* data, size_t size)
{
    if (config == NULL || data == NULL || size == 0) {
        return false;
    }
    
    // 获取私有数据
    if (config->custom_data == NULL || config->custom_data_size != sizeof(Stru_AllocatorConfigData_t)) {
        return false;
    }
    
    Stru_AllocatorConfigData_t* private_data = (Stru_AllocatorConfigData_t*)config->custom_data;
    
    // 释放旧数据
    if (private_data->custom_data_copy != NULL) {
        free(private_data->custom_data_copy);
        private_data->custom_data_copy = NULL;
        private_data->custom_data_size = 0;
    }
    
    // 复制新数据
    private_data->custom_data_copy = malloc(size);
    if (private_data->custom_data_copy == NULL) {
        return false;
    }
    
    memcpy(private_data->custom_data_copy, data, size);
    private_data->custom_data_size = size;
    
    // 更新配置中的引用
    config->custom_data = private_data->custom_data_copy;
    config->custom_data_size = size;
    
    return true;
}

/**
 * @brief 获取配置摘要
 */
bool F_get_config_summary(const Stru_AllocatorConfig_t* config, char* buffer, size_t buffer_size)
{
    if (config == NULL || buffer == NULL || buffer_size == 0) {
        return false;
    }
    
    int written = snprintf(buffer, buffer_size,
        "Allocator Config: name='%s', thread_safe=%s, stats=%s, debug=%s, "
        "alignment=%zu, max_size=%zu",
        config->name ? config->name : "(default)",
        config->thread_safe ? "yes" : "no",
        config->enable_statistics ? "yes" : "no",
        config->enable_debugging ? "yes" : "no",
        config->alignment,
        config->max_allocation_size);
    
    return (written > 0 && (size_t)written < buffer_size);
}
