/**
 * @file CN_target_codegen_config.c
 * @brief CN_Language 目标代码生成器配置管理模块
 * 
 * 提供目标代码生成器配置的创建和管理功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-12
 * @version 1.0.0
 */

#include "../CN_target_codegen_interface.h"
#include <string.h>

/**
 * @brief 创建默认目标配置
 * 
 * 创建并返回默认的目标配置。
 * 
 * @return Stru_TargetConfig_t 默认目标配置
 */
Stru_TargetConfig_t F_create_default_target_config(void)
{
    Stru_TargetConfig_t config;
    memset(&config, 0, sizeof(Stru_TargetConfig_t));
    
    config.platform = Eum_TARGET_PLATFORM_GENERIC;
    config.os = Eum_TARGET_OS_GENERIC;
    config.abi = Eum_TARGET_ABI_GENERIC;
    config.format = Eum_TARGET_FORMAT_ASSEMBLY;
    config.code_type = Eum_TARGET_C;
    
    config.position_independent = false;
    config.stack_protector = false;
    config.omit_frame_pointer = false;
    config.optimize_for_size = false;
    config.optimize_for_speed = true;
    
    config.alignment = 8;
    config.stack_alignment = 16;
    config.red_zone_size = 128;
    
    config.defines = NULL;
    config.includes = NULL;
    config.libraries = NULL;
    config.lib_paths = NULL;
    
    return config;
}
