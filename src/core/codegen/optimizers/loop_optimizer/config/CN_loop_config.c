/******************************************************************************
 * 文件名: CN_loop_config.c
 * 功能: CN_Language 循环优化器配置模块实现
 * 
 * 实现循环优化器的配置管理功能，包括默认配置创建、配置参数设置、
 * 配置验证和配置选项获取等。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_loop_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 默认配置值 */
#define DEFAULT_MAX_UNROLL_FACTOR 4
#define DEFAULT_MIN_LOOP_ITERATIONS 10
#define DEFAULT_VECTOR_WIDTH 4
#define DEFAULT_MAX_LOOP_DEPTH 3
#define DEFAULT_ENABLE_PARALLELIZATION true
#define DEFAULT_ENABLE_MEMORY_OPT true
#define DEFAULT_AGGRESSIVE_OPT false

/* 配置键名定义 */
static const char* CONFIG_KEY_MAX_UNROLL_FACTOR = "max_unroll_factor";
static const char* CONFIG_KEY_MIN_LOOP_ITERATIONS = "min_loop_iterations";
static const char* CONFIG_KEY_VECTOR_WIDTH = "vector_width";
static const char* CONFIG_KEY_MAX_LOOP_DEPTH = "max_loop_depth";
static const char* CONFIG_KEY_ENABLE_PARALLELIZATION = "enable_parallelization";
static const char* CONFIG_KEY_ENABLE_MEMORY_OPT = "enable_memory_optimization";
static const char* CONFIG_KEY_AGGRESSIVE_OPT = "aggressive_optimization";

/**
 * @brief 创建默认循环优化配置
 */
Stru_LoopOptimizationConfig_t* F_create_default_loop_optimization_config(void)
{
    Stru_LoopOptimizationConfig_t* config = 
        (Stru_LoopOptimizationConfig_t*)malloc(sizeof(Stru_LoopOptimizationConfig_t));
    if (!config) {
        return NULL;
    }
    
    /* 初始化默认值 */
    config->max_unroll_factor = DEFAULT_MAX_UNROLL_FACTOR;
    config->min_loop_iterations = DEFAULT_MIN_LOOP_ITERATIONS;
    config->max_vector_width = DEFAULT_VECTOR_WIDTH;
    config->max_loop_depth = DEFAULT_MAX_LOOP_DEPTH;
    config->enable_aggressive_optimizations = DEFAULT_AGGRESSIVE_OPT;
    config->enable_parallelization = DEFAULT_ENABLE_PARALLELIZATION;
    config->enable_memory_optimization = DEFAULT_ENABLE_MEMORY_OPT;
    config->preserve_semantics = true;
    config->enable_profiling = false;
    config->cost_threshold = 1.0;
    
    return config;
}

/**
 * @brief 销毁循环优化配置
 */
void F_destroy_loop_optimization_config(Stru_LoopOptimizationConfig_t* config)
{
    if (config) {
        free(config);
    }
}

/**
 * @brief 配置循环优化器参数
 */
bool F_configure_loop_optimizer(Stru_LoopOptimizationConfig_t* config, const char* key, const char* value)
{
    if (!config || !key || !value) {
        return false;
    }
    
    /* 根据键名设置配置值 */
    if (strcmp(key, CONFIG_KEY_MAX_UNROLL_FACTOR) == 0) {
        int max_unroll_factor = atoi(value);
        if (max_unroll_factor > 0 && max_unroll_factor <= 32) {
            config->max_unroll_factor = max_unroll_factor;
            return true;
        }
    }
    else if (strcmp(key, CONFIG_KEY_MIN_LOOP_ITERATIONS) == 0) {
        int min_loop_iterations = atoi(value);
        if (min_loop_iterations >= 0) {
            config->min_loop_iterations = min_loop_iterations;
            return true;
        }
    }
    else if (strcmp(key, CONFIG_KEY_VECTOR_WIDTH) == 0) {
        int vector_width = atoi(value);
        if (vector_width > 0 && vector_width <= 16) {
            config->max_vector_width = vector_width;
            return true;
        }
    }
    else if (strcmp(key, CONFIG_KEY_MAX_LOOP_DEPTH) == 0) {
        int max_loop_depth = atoi(value);
        if (max_loop_depth > 0 && max_loop_depth <= 10) {
            config->max_loop_depth = max_loop_depth;
            return true;
        }
    }
    else if (strcmp(key, CONFIG_KEY_ENABLE_PARALLELIZATION) == 0) {
        if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
            config->enable_parallelization = true;
            return true;
        }
        else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
            config->enable_parallelization = false;
            return true;
        }
    }
    else if (strcmp(key, CONFIG_KEY_ENABLE_MEMORY_OPT) == 0) {
        if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
            config->enable_memory_optimization = true;
            return true;
        }
        else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
            config->enable_memory_optimization = false;
            return true;
        }
    }
    else if (strcmp(key, CONFIG_KEY_AGGRESSIVE_OPT) == 0) {
        if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
            config->enable_aggressive_optimizations = true;
            return true;
        }
        else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
            config->enable_aggressive_optimizations = false;
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 获取循环优化器配置选项
 */
Stru_DynamicArray_t* F_get_loop_optimizer_config_options(Stru_LoopOptimizationConfig_t* config)
{
    if (!config) {
        return NULL;
    }
    
    /* 创建配置选项数组 */
    Stru_DynamicArray_t* options = F_create_dynamic_array(sizeof(Stru_ConfigOption_t));
    if (!options) {
        return NULL;
    }
    
    /* 添加配置选项 */
    Stru_ConfigOption_t option;
    
    /* 最大展开因子 */
    option.key = CONFIG_KEY_MAX_UNROLL_FACTOR;
    option.description = "循环最大展开因子 (1-32)";
    option.default_value = "4";
    option.type = Eum_CONFIG_TYPE_INTEGER;
    F_dynamic_array_push(options, &option);
    
    /* 最小循环迭代次数 */
    option.key = CONFIG_KEY_MIN_LOOP_ITERATIONS;
    option.description = "应用优化的最小循环迭代次数";
    option.default_value = "10";
    option.type = Eum_CONFIG_TYPE_INTEGER;
    F_dynamic_array_push(options, &option);
    
    /* 向量宽度 */
    option.key = CONFIG_KEY_VECTOR_WIDTH;
    option.description = "向量化宽度 (1-16)";
    option.default_value = "4";
    option.type = Eum_CONFIG_TYPE_INTEGER;
    F_dynamic_array_push(options, &option);
    
    /* 最大循环深度 */
    option.key = CONFIG_KEY_MAX_LOOP_DEPTH;
    option.description = "支持优化的最大循环嵌套深度";
    option.default_value = "3";
    option.type = Eum_CONFIG_TYPE_INTEGER;
    F_dynamic_array_push(options, &option);
    
    /* 启用并行化 */
    option.key = CONFIG_KEY_ENABLE_PARALLELIZATION;
    option.description = "启用循环并行化优化";
    option.default_value = "true";
    option.type = Eum_CONFIG_TYPE_BOOLEAN;
    F_dynamic_array_push(options, &option);
    
    /* 启用内存优化 */
    option.key = CONFIG_KEY_ENABLE_MEMORY_OPT;
    option.description = "启用内存访问优化";
    option.default_value = "true";
    option.type = Eum_CONFIG_TYPE_BOOLEAN;
    F_dynamic_array_push(options, &option);
    
    /* 激进优化 */
    option.key = CONFIG_KEY_AGGRESSIVE_OPT;
    option.description = "启用激进优化策略";
    option.default_value = "false";
    option.type = Eum_CONFIG_TYPE_BOOLEAN;
    F_dynamic_array_push(options, &option);
    
    return options;
}

/**
 * @brief 验证循环优化配置
 */
bool F_validate_loop_optimization_config(const Stru_LoopOptimizationConfig_t* config)
{
    if (!config) {
        return false;
    }
    
    /* 验证配置值范围 */
    if (config->max_unroll_factor < 1 || config->max_unroll_factor > 32) {
        return false;
    }
    
    if (config->min_loop_iterations < 0) {
        return false;
    }
    
    if (config->max_vector_width < 1 || config->max_vector_width > 16) {
        return false;
    }
    
    if (config->max_loop_depth < 1 || config->max_loop_depth > 10) {
        return false;
    }
    
    return true;
}

/**
 * @brief 克隆循环优化配置
 */
Stru_LoopOptimizationConfig_t* F_clone_loop_optimization_config(const Stru_LoopOptimizationConfig_t* src)
{
    if (!src) {
        return NULL;
    }
    
    Stru_LoopOptimizationConfig_t* dst = F_create_default_loop_optimization_config();
    if (!dst) {
        return NULL;
    }
    
    /* 复制所有字段 */
    dst->max_unroll_factor = src->max_unroll_factor;
    dst->min_loop_iterations = src->min_loop_iterations;
    dst->max_vector_width = src->max_vector_width;
    dst->max_loop_depth = src->max_loop_depth;
    dst->enable_aggressive_optimizations = src->enable_aggressive_optimizations;
    dst->enable_parallelization = src->enable_parallelization;
    dst->enable_memory_optimization = src->enable_memory_optimization;
    dst->preserve_semantics = src->preserve_semantics;
    dst->enable_profiling = src->enable_profiling;
    dst->cost_threshold = src->cost_threshold;
    
    return dst;
}

/**
 * @brief 重置循环优化配置为默认值
 */
void F_reset_loop_optimization_config(Stru_LoopOptimizationConfig_t* config)
{
    if (!config) {
        return;
    }
    
    config->max_unroll_factor = DEFAULT_MAX_UNROLL_FACTOR;
    config->min_loop_iterations = DEFAULT_MIN_LOOP_ITERATIONS;
    config->max_vector_width = DEFAULT_VECTOR_WIDTH;
    config->max_loop_depth = DEFAULT_MAX_LOOP_DEPTH;
    config->enable_aggressive_optimizations = DEFAULT_AGGRESSIVE_OPT;
    config->enable_parallelization = DEFAULT_ENABLE_PARALLELIZATION;
    config->enable_memory_optimization = DEFAULT_ENABLE_MEMORY_OPT;
    config->preserve_semantics = true;
    config->enable_profiling = false;
    config->cost_threshold = 1.0;
}

/**
 * @brief 获取配置值
 */
const char* F_get_loop_optimization_config_value(const Stru_LoopOptimizationConfig_t* config, const char* key)
{
    if (!config || !key) {
        return NULL;
    }
    
    static char buffer[32];
    
    if (strcmp(key, CONFIG_KEY_MAX_UNROLL_FACTOR) == 0) {
        snprintf(buffer, sizeof(buffer), "%d", config->max_unroll_factor);
        return buffer;
    }
    else if (strcmp(key, CONFIG_KEY_MIN_LOOP_ITERATIONS) == 0) {
        snprintf(buffer, sizeof(buffer), "%d", config->min_loop_iterations);
        return buffer;
    }
    else if (strcmp(key, CONFIG_KEY_VECTOR_WIDTH) == 0) {
        snprintf(buffer, sizeof(buffer), "%d", config->max_vector_width);
        return buffer;
    }
    else if (strcmp(key, CONFIG_KEY_MAX_LOOP_DEPTH) == 0) {
        snprintf(buffer, sizeof(buffer), "%d", config->max_loop_depth);
        return buffer;
    }
    else if (strcmp(key, CONFIG_KEY_ENABLE_PARALLELIZATION) == 0) {
        return config->enable_parallelization ? "true" : "false";
    }
    else if (strcmp(key, CONFIG_KEY_ENABLE_MEMORY_OPT) == 0) {
        return config->enable_memory_optimization ? "true" : "false";
    }
    else if (strcmp(key, CONFIG_KEY_AGGRESSIVE_OPT) == 0) {
        return config->enable_aggressive_optimizations ? "true" : "false";
    }
    
    return NULL;
}

/**
 * @brief 设置配置值
 */
bool F_set_loop_optimization_config_value(Stru_LoopOptimizationConfig_t* config, const char* key, const char* value)
{
    return F_configure_loop_optimizer(config, key, value);
}
