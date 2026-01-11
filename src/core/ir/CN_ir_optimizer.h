/******************************************************************************
 * 文件名: CN_ir_optimizer.h
 * 功能: CN_Language 中间代码表示优化器接口定义
 
 * 定义中间代码表示的优化器接口，支持IR级别的各种优化算法。
 * 遵循项目架构规范，采用接口模式实现模块解耦。
 * 
 * 支持的优化类型：
 * 1. 常量折叠（Constant Folding）
 * 2. 死代码消除（Dead Code Elimination）
 * 3. 公共子表达式消除（Common Subexpression Elimination）
 * 4. 强度削减（Strength Reduction）
 * 5. 循环优化（Loop Optimization）
 * 6. 函数内联（Function Inlining）
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_IR_OPTIMIZER_H
#define CN_IR_OPTIMIZER_H

#include <stddef.h>
#include <stdbool.h>
#include "CN_ir_interface.h"

// ============================================================================
// 类型定义
// ============================================================================

/**
 * @brief 优化类型枚举
 * 
 * 定义支持的IR优化类型。
 */
typedef enum {
    Eum_IR_OPT_CONSTANT_FOLDING = 1,          ///< 常量折叠
    Eum_IR_OPT_DEAD_CODE_ELIMINATION = 2,     ///< 死代码消除
    Eum_IR_OPT_COMMON_SUBEXPRESSION = 3,      ///< 公共子表达式消除
    Eum_IR_OPT_STRENGTH_REDUCTION = 4,        ///< 强度削减
    Eum_IR_OPT_LOOP_INVARIANT_MOTION = 5,     ///< 循环不变代码外提
    Eum_IR_OPT_LOOP_UNROLLING = 6,            ///< 循环展开
    Eum_IR_OPT_FUNCTION_INLINING = 7,         ///< 函数内联
    Eum_IR_OPT_PEEPHOLE = 8,                  ///< 窥孔优化
    Eum_IR_OPT_REGISTER_ALLOCATION = 9,       ///< 寄存器分配
    Eum_IR_OPT_INSTRUCTION_SCHEDULING = 10    ///< 指令调度
} Eum_IrOptimizationType;

/**
 * @brief 优化级别枚举
 * 
 * 定义优化级别，控制优化的激进程度。
 */
typedef enum {
    Eum_IR_OPT_LEVEL_NONE = 0,    ///< 无优化
    Eum_IR_OPT_LEVEL_O1 = 1,      ///< 基本优化（常量折叠、死代码消除）
    Eum_IR_OPT_LEVEL_O2 = 2,      ///< 中级优化（包含O1，加上公共子表达式消除、强度削减）
    Eum_IR_OPT_LEVEL_O3 = 3,      ///< 高级优化（包含O2，加上循环优化、函数内联）
    Eum_IR_OPT_LEVEL_Os = 4,      ///< 代码大小优化
    Eum_IR_OPT_LEVEL_Oz = 5       ///< 激进代码大小优化
} Eum_IrOptimizationLevel;

/**
 * @brief 优化结果结构体
 * 
 * 存储优化操作的结果信息。
 */
typedef struct {
    bool success;                       ///< 优化是否成功
    void* optimized_ir;                 ///< 优化后的IR
    size_t original_instruction_count;  ///< 原始指令数量
    size_t optimized_instruction_count; ///< 优化后指令数量
    size_t removed_instructions;        ///< 移除的指令数量
    size_t added_instructions;          ///< 新增的指令数量
    double improvement_ratio;           ///< 改进比例（0.0-1.0）
    char* error_message;                ///< 错误信息
    char* warning_message;              ///< 警告信息
    double optimization_time;           ///< 优化时间（秒）
} Stru_IrOptimizationResult_t;

/**
 * @brief 优化统计信息结构体
 * 
 * 存储优化过程的统计信息。
 */
typedef struct {
    size_t total_optimizations_applied;     ///< 应用的总优化次数
    size_t successful_optimizations;        ///< 成功的优化次数
    size_t failed_optimizations;            ///< 失败的优化次数
    size_t total_instructions_removed;      ///< 移除的总指令数
    size_t total_instructions_added;        ///< 新增的总指令数
    double total_optimization_time;         ///< 总优化时间（秒）
    double average_improvement_ratio;       ///< 平均改进比例
    size_t* optimization_type_counts;       ///< 各优化类型应用次数
    size_t optimization_type_count_size;    ///< 优化类型计数数组大小
} Stru_IrOptimizationStats_t;

/**
 * @brief 优化配置结构体
 * 
 * 定义优化器的配置选项。
 */
typedef struct {
    bool enable_constant_folding;           ///< 是否启用常量折叠
    bool enable_dead_code_elimination;      ///< 是否启用死代码消除
    bool enable_common_subexpression;       ///< 是否启用公共子表达式消除
    bool enable_strength_reduction;         ///< 是否启用强度削减
    bool enable_loop_optimization;          ///< 是否启用循环优化
    bool enable_function_inlining;          ///< 是否启用函数内联
    bool enable_peephole_optimization;      ///< 是否启用窥孔优化
    int max_inlining_depth;                 ///< 最大内联深度
    int max_loop_unroll_factor;             ///< 最大循环展开因子
    bool aggressive_optimization;           ///< 是否启用激进优化
    bool preserve_debug_info;               ///< 是否保留调试信息
    bool generate_optimization_report;      ///< 是否生成优化报告
} Stru_IrOptimizationConfig_t;

/**
 * @brief IR优化器接口结构体
 * 
 * 定义中间代码表示的优化器接口。
 */
typedef struct Stru_IrOptimizerInterface_t {
    // 基本信息
    const char* (*get_name)(void);
    const char* (*get_version)(void);
    Eum_IrType (*get_supported_ir_type)(void);
    
    // 配置管理
    bool (*set_config)(Stru_IrOptimizationConfig_t* config);
    Stru_IrOptimizationConfig_t* (*get_config)(void);
    Stru_IrOptimizationConfig_t* (*create_default_config)(void);
    
    // 优化操作
    Stru_IrOptimizationResult_t* (*apply_optimization)(void* ir, Eum_IrOptimizationType opt_type);
    Stru_IrOptimizationResult_t* (*apply_optimization_level)(void* ir, Eum_IrOptimizationLevel level);
    Stru_IrOptimizationResult_t* (*apply_all_optimizations)(void* ir);
    
    // 优化分析
    bool (*analyze_ir)(void* ir, Stru_IrOptimizationStats_t* stats);
    bool (*can_apply_optimization)(void* ir, Eum_IrOptimizationType opt_type);
    double (*estimate_improvement)(void* ir, Eum_IrOptimizationType opt_type);
    
    // 统计信息
    Stru_IrOptimizationStats_t* (*get_statistics)(void);
    bool (*reset_statistics)(void);
    bool (*generate_report)(const char* filename);
    
    // 验证和测试
    bool (*validate_optimization)(void* original_ir, void* optimized_ir);
    bool (*test_optimization_correctness)(Eum_IrOptimizationType opt_type);
    
    // 资源管理
    void (*destroy_optimization_result)(Stru_IrOptimizationResult_t* result);
    void (*destroy_optimization_stats)(Stru_IrOptimizationStats_t* stats);
    void (*destroy)(void);
    
    // 内部状态
    void* internal_state;
} Stru_IrOptimizerInterface_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

/**
 * @brief 创建IR优化器接口实例
 * 
 * 根据指定的IR类型创建相应的IR优化器接口实例。
 * 
 * @param ir_type IR类型
 * @return Stru_IrOptimizerInterface_t* IR优化器接口实例，失败返回NULL
 */
Stru_IrOptimizerInterface_t* F_create_ir_optimizer_interface(Eum_IrType ir_type);

/**
 * @brief 销毁IR优化器接口实例
 * 
 * 销毁IR优化器接口实例及其所有资源。
 * 
 * @param optimizer IR优化器接口实例
 */
void F_destroy_ir_optimizer_interface(Stru_IrOptimizerInterface_t* optimizer);

// ============================================================================
// 工具函数声明
// ============================================================================

/**
 * @brief 创建默认优化配置
 * 
 * 创建默认的优化配置。
 * 
 * @return Stru_IrOptimizationConfig_t* 默认优化配置，失败返回NULL
 */
Stru_IrOptimizationConfig_t* F_create_default_ir_optimization_config(void);

/**
 * @brief 销毁优化配置
 * 
 * 销毁优化配置。
 * 
 * @param config 优化配置
 */
void F_destroy_ir_optimization_config(Stru_IrOptimizationConfig_t* config);

/**
 * @brief 创建优化结果
 * 
 * 创建新的优化结果。
 * 
 * @return Stru_IrOptimizationResult_t* 新创建的优化结果，失败返回NULL
 */
Stru_IrOptimizationResult_t* F_create_ir_optimization_result(void);

/**
 * @brief 设置优化结果错误
 * 
 * 设置优化结果的错误信息。
 * 
 * @param result 优化结果
 * @param error_message 错误信息
 */
void F_set_ir_optimization_result_error(Stru_IrOptimizationResult_t* result, const char* error_message);

/**
 * @brief 设置优化结果警告
 * 
 * 设置优化结果的警告信息。
 * 
 * @param result 优化结果
 * @param warning_message 警告信息
 */
void F_set_ir_optimization_result_warning(Stru_IrOptimizationResult_t* result, const char* warning_message);

/**
 * @brief 创建优化统计信息
 * 
 * 创建新的优化统计信息。
 * 
 * @return Stru_IrOptimizationStats_t* 新创建的优化统计信息，失败返回NULL
 */
Stru_IrOptimizationStats_t* F_create_ir_optimization_stats(void);

/**
 * @brief 更新优化统计信息
 * 
 * 根据优化结果更新统计信息。
 * 
 * @param stats 优化统计信息
 * @param result 优化结果
 * @param opt_type 优化类型
 * @return bool 成功返回true，失败返回false
 */
bool F_update_ir_optimization_stats(Stru_IrOptimizationStats_t* stats, 
                                   const Stru_IrOptimizationResult_t* result,
                                   Eum_IrOptimizationType opt_type);

/**
 * @brief 优化类型转字符串
 * 
 * 将优化类型转换为可读的字符串表示。
 * 
 * @param opt_type 优化类型
 * @return const char* 优化类型字符串表示
 */
const char* F_ir_optimization_type_to_string(Eum_IrOptimizationType opt_type);

/**
 * @brief 优化级别转字符串
 * 
 * 将优化级别转换为可读的字符串表示。
 * 
 * @param level 优化级别
 * @return const char* 优化级别字符串表示
 */
const char* F_ir_optimization_level_to_string(Eum_IrOptimizationLevel level);

/**
 * @brief 检查优化类型是否支持
 * 
 * 检查指定的优化类型是否被支持。
 * 
 * @param opt_type 优化类型
 * @return bool 支持返回true，不支持返回false
 */
bool F_is_ir_optimization_type_supported(Eum_IrOptimizationType opt_type);

/**
 * @brief 检查优化级别是否有效
 * 
 * 检查指定的优化级别是否有效。
 * 
 * @param level 优化级别
 * @return bool 有效返回true，无效返回false
 */
bool F_is_ir_optimization_level_valid(Eum_IrOptimizationLevel level);

/**
 * @brief 获取优化级别对应的优化类型
 * 
 * 获取指定优化级别对应的优化类型数组。
 * 
 * @param level 优化级别
 * @param types 输出参数，优化类型数组
 * @param count 输出参数，优化类型数量
 * @return bool 成功返回true，失败返回false
 */
bool F_get_optimizations_for_level(Eum_IrOptimizationLevel level,
                                  Eum_IrOptimizationType** types,
                                  size_t* count);

// ============================================================================
// 优化效果评估函数
// ============================================================================

/**
 * @brief 评估优化效果
 * 
 * 评估优化前后的效果，计算改进比例。
 * 
 * @param original_ir 原始IR
 * @param optimized_ir 优化后IR
 * @param stats 输出参数，统计信息
 * @return double 改进比例（0.0-1.0），失败返回-1.0
 */
double F_evaluate_optimization_effectiveness(void* original_ir,
                                            void* optimized_ir,
                                            Stru_IrOptimizationStats_t* stats);

/**
 * @brief 计算指令减少比例
 * 
 * 计算优化后指令减少的比例。
 * 
 * @param original_count 原始指令数量
 * @param optimized_count 优化后指令数量
 * @return double 减少比例（0.0-1.0）
 */
double F_calculate_instruction_reduction_ratio(size_t original_count,
                                              size_t optimized_count);

/**
 * @brief 分析优化潜力
 * 
 * 分析IR的优化潜力，识别可优化的区域。
 * 
 * @param ir IR实例
 * @param potential_areas 输出参数，潜在优化区域
 * @param area_count 输出参数，区域数量
 * @return bool 成功返回true，失败返回false
 */
bool F_analyze_optimization_potential(void* ir,
                                     void** potential_areas,
                                     size_t* area_count);

// ============================================================================
// 版本信息函数
// ============================================================================

/**
 * @brief 获取IR优化器版本信息
 * 
 * 获取IR优化器的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 */
void F_get_ir_optimizer_version(int* major, int* minor, int* patch);

/**
 * @brief 获取IR优化器版本字符串
 * 
 * 获取IR优化器的版本字符串。
 * 
 * @return const char* 版本字符串
 */
const char* F_get_ir_optimizer_version_string(void);

#endif /* CN_IR_OPTIMIZER_H */
