/******************************************************************************
 * 文件名: CN_loop_config.h
 * 功能: CN_Language 循环优化器配置模块
 * 
 * 提供循环优化器的配置管理功能，包括默认配置创建、配置参数设置、
 * 配置验证和配置选项获取等。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_LOOP_CONFIG_H
#define CN_LOOP_CONFIG_H

#include "../../CN_optimizer_interface.h"
#include "../CN_loop_optimizer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 配置类型枚举
 * 
 * 定义配置值的类型。
 */
typedef enum Eum_ConfigType {
    Eum_CONFIG_TYPE_INTEGER = 0,    ///< 整数类型
    Eum_CONFIG_TYPE_BOOLEAN,        ///< 布尔类型
    Eum_CONFIG_TYPE_STRING,         ///< 字符串类型
    Eum_CONFIG_TYPE_FLOAT,          ///< 浮点数类型
    Eum_CONFIG_TYPE_ENUM            ///< 枚举类型
} Eum_ConfigType;

/**
 * @brief 配置选项结构体
 * 
 * 描述一个配置选项的元数据。
 */
typedef struct Stru_ConfigOption_t {
    const char* key;                ///< 配置键名
    const char* description;        ///< 配置描述
    const char* default_value;      ///< 默认值
    Eum_ConfigType type;            ///< 配置类型
} Stru_ConfigOption_t;

/**
 * @brief 创建默认循环优化配置
 * 
 * @return Stru_LoopOptimizationConfig_t* 分配的配置结构体指针，失败返回NULL
 */
Stru_LoopOptimizationConfig_t* F_create_default_loop_optimization_config(void);

/**
 * @brief 销毁循环优化配置
 * 
 * @param config 要销毁的配置结构体指针
 */
void F_destroy_loop_optimization_config(Stru_LoopOptimizationConfig_t* config);

/**
 * @brief 配置循环优化器参数
 * 
 * @param config 配置结构体指针
 * @param key 配置键名
 * @param value 配置值
 * @return bool 配置成功返回true，失败返回false
 */
bool F_configure_loop_optimizer(Stru_LoopOptimizationConfig_t* config, const char* key, const char* value);

/**
 * @brief 获取循环优化器配置选项
 * 
 * @param config 配置结构体指针
 * @return Stru_DynamicArray_t* 配置选项数组，失败返回NULL
 */
Stru_DynamicArray_t* F_get_loop_optimizer_config_options(Stru_LoopOptimizationConfig_t* config);

/**
 * @brief 验证循环优化配置
 * 
 * @param config 配置结构体指针
 * @return bool 配置有效返回true，无效返回false
 */
bool F_validate_loop_optimization_config(const Stru_LoopOptimizationConfig_t* config);

/**
 * @brief 克隆循环优化配置
 * 
 * @param src 源配置结构体指针
 * @return Stru_LoopOptimizationConfig_t* 克隆的配置结构体指针，失败返回NULL
 */
Stru_LoopOptimizationConfig_t* F_clone_loop_optimization_config(const Stru_LoopOptimizationConfig_t* src);

/**
 * @brief 重置循环优化配置为默认值
 * 
 * @param config 配置结构体指针
 */
void F_reset_loop_optimization_config(Stru_LoopOptimizationConfig_t* config);

/**
 * @brief 获取配置值
 * 
 * @param config 配置结构体指针
 * @param key 配置键名
 * @return const char* 配置值，未找到返回NULL
 */
const char* F_get_loop_optimization_config_value(const Stru_LoopOptimizationConfig_t* config, const char* key);

/**
 * @brief 设置配置值
 * 
 * @param config 配置结构体指针
 * @param key 配置键名
 * @param value 配置值
 * @return bool 设置成功返回true，失败返回false
 */
bool F_set_loop_optimization_config_value(Stru_LoopOptimizationConfig_t* config, const char* key, const char* value);

#ifdef __cplusplus
}
#endif

#endif /* CN_LOOP_CONFIG_H */
