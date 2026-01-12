/******************************************************************************
 * 文件名: CN_loop_optimizer.h
 * 功能: CN_Language 循环优化器实现
 * 
 * 提供循环相关的优化算法实现，包括循环不变代码外提、循环展开、
 * 循环融合、循环向量化等优化。
 * 遵循项目架构规范，实现优化器插件接口。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_LOOP_OPTIMIZER_H
#define CN_LOOP_OPTIMIZER_H

#include "../../CN_optimizer_interface.h"

/**
 * @brief 循环类型枚举
 * 
 * 定义循环的类型。
 */
typedef enum Eum_LoopType {
    Eum_LOOP_TYPE_UNKNOWN = 0,             ///< 未知循环类型
    Eum_LOOP_TYPE_FOR,                     ///< for循环
    Eum_LOOP_TYPE_WHILE                    ///< while循环
} Eum_LoopType;

/**
 * @brief 循环优化类型枚举
 * 
 * 定义循环相关的优化类型。
 */
typedef enum Eum_LoopOptimizationType {
    Eum_LOOP_OPTIMIZE_NONE = 0,            ///< 无循环优化
    Eum_LOOP_OPTIMIZE_INVARIANT_CODE_MOTION, ///< 循环不变代码外提
    Eum_LOOP_OPTIMIZE_UNROLLING,           ///< 循环展开
    Eum_LOOP_OPTIMIZE_FUSION,              ///< 循环融合
    Eum_LOOP_OPTIMIZE_VECTORIZATION,       ///< 循环向量化
    Eum_LOOP_OPTIMIZE_PARALLELIZATION,     ///< 循环并行化
    Eum_LOOP_OPTIMIZE_TILING,              ///< 循环分块
    Eum_LOOP_OPTIMIZE_INTERCHANGE,         ///< 循环交换
    Eum_LOOP_OPTIMIZE_REVERSAL,            ///< 循环反转
    Eum_LOOP_OPTIMIZE_PEELING,             ///< 循环剥离
    Eum_LOOP_OPTIMIZE_JAMMING,             ///< 循环压紧
    Eum_LOOP_OPTIMIZE_SKEWING,             ///< 循环倾斜
    Eum_LOOP_OPTIMIZE_DISTRIBUTION         ///< 循环分布
} Eum_LoopOptimizationType;

/**
 * @brief 循环分析信息结构体
 * 
 * 包含循环分析的结果信息。
 */
typedef struct Stru_LoopAnalysisInfo_t {
    size_t loop_id;                        ///< 循环ID
    size_t depth;                          ///< 循环嵌套层级
    Eum_LoopType loop_type;                ///< 循环类型
    size_t body_size;                      ///< 循环体大小（语句数量）
    bool has_nested_loops;                 ///< 是否有嵌套循环
    bool has_data_dependencies;            ///< 是否有数据依赖
    bool has_vectorizable_operations;      ///< 是否有向量化支持的操作
    bool has_complex_condition;            ///< 是否有复杂条件
    bool can_be_optimized;                 ///< 是否可优化
    size_t estimated_iterations;           ///< 估计迭代次数
    size_t iteration_count;                ///< 迭代次数（如果可确定）
    bool is_countable;                     ///< 是否可计数循环
    bool is_infinite;                      ///< 是否无限循环
    bool has_side_effects;                 ///< 是否有副作用
    bool contains_function_calls;          ///< 是否包含函数调用
    bool contains_io_operations;           ///< 是否包含IO操作
    bool contains_memory_operations;       ///< 是否包含内存操作
    Stru_DynamicArray_t* loop_variables;   ///< 循环变量数组
    Stru_DynamicArray_t* invariant_expressions; ///< 循环不变表达式数组
    Stru_DynamicArray_t* dependencies;     ///< 依赖关系数组
} Stru_LoopAnalysisInfo_t;

/**
 * @brief 循环优化配置结构体
 * 
 * 包含循环优化的配置参数。
 */
typedef struct Stru_LoopOptimizationConfig_t {
    size_t max_unroll_factor;              ///< 最大展开因子
    size_t min_loop_iterations;            ///< 最小循环迭代次数
    size_t max_vector_width;               ///< 最大向量宽度
    size_t max_loop_depth;                 ///< 最大循环深度
    bool enable_aggressive_optimizations;  ///< 是否启用激进优化
    bool enable_parallelization;           ///< 是否启用并行化
    bool enable_memory_optimization;       ///< 是否启用内存优化
    bool preserve_semantics;               ///< 是否保持语义
    bool enable_profiling;                 ///< 是否启用性能分析
    double cost_threshold;                 ///< 成本阈值
} Stru_LoopOptimizationConfig_t;

/**
 * @brief 创建循环优化器插件接口实例
 * 
 * 创建并返回一个新的循环优化器插件接口实例。
 * 调用者负责在不再使用时调用destroy函数。
 * 
 * @return Stru_OptimizerPluginInterface_t* 新创建的循环优化器插件接口实例
 */
Stru_OptimizerPluginInterface_t* F_create_loop_optimizer_interface(void);

/**
 * @brief 获取循环优化器版本信息
 * 
 * 返回循环优化器的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 */
void F_get_loop_optimizer_version(int* major, int* minor, int* patch);

/**
 * @brief 获取循环优化器版本字符串
 * 
 * 返回循环优化器的版本字符串。
 * 
 * @return 版本字符串
 */
const char* F_get_loop_optimizer_version_string(void);

/**
 * @brief 检查是否支持特定循环优化类型
 * 
 * 检查循环优化器是否支持特定的循环优化类型。
 * 
 * @param loop_optimization_type 循环优化类型
 * @return bool 是否支持
 */
bool F_loop_optimizer_supports_optimization(Eum_LoopOptimizationType loop_optimization_type);

/**
 * @brief 获取循环优化器支持的优化类型
 * 
 * 获取循环优化器支持的所有优化类型。
 * 
 * @param count 输出参数，优化类型数量
 * @return Eum_OptimizationType* 优化类型数组，调用者不应修改
 */
const Eum_OptimizationType* F_get_loop_optimizer_supported_optimizations(size_t* count);

/**
 * @brief 分析循环结构
 * 
 * 分析AST中的循环结构，收集循环信息。
 * 
 * @param ast AST根节点
 * @return Stru_DynamicArray_t* 循环分析信息数组，调用者负责销毁
 */
Stru_DynamicArray_t* F_analyze_loops(Stru_AstNode_t* ast);

/**
 * @brief 应用循环不变代码外提
 * 
 * 将循环中不变的计算移到循环外部。
 * 
 * @param ast AST根节点
 * @param loop_info 循环分析信息
 * @return bool 优化是否成功应用
 */
bool F_apply_loop_invariant_code_motion(Stru_AstNode_t* ast, Stru_LoopAnalysisInfo_t* loop_info);

/**
 * @brief 应用循环展开
 * 
 * 将循环体复制多次，减少循环控制开销。
 * 
 * @param ast AST根节点
 * @param loop_info 循环分析信息
 * @param unroll_factor 展开因子
 * @return bool 优化是否成功应用
 */
bool F_apply_loop_unrolling(Stru_AstNode_t* ast, Stru_LoopAnalysisInfo_t* loop_info, size_t unroll_factor);

/**
 * @brief 应用循环融合
 * 
 * 将多个相邻的循环合并为一个循环。
 * 
 * @param ast AST根节点
 * @param loop_infos 循环分析信息数组
 * @param loop_count 循环数量
 * @return bool 优化是否成功应用
 */
bool F_apply_loop_fusion(Stru_AstNode_t* ast, Stru_LoopAnalysisInfo_t** loop_infos, size_t loop_count);

/**
 * @brief 应用循环向量化
 * 
 * 将循环转换为向量化操作。
 * 
 * @param ast AST根节点
 * @param loop_info 循环分析信息
 * @param vector_width 向量宽度
 * @return bool 优化是否成功应用
 */
bool F_apply_loop_vectorization(Stru_AstNode_t* ast, Stru_LoopAnalysisInfo_t* loop_info, size_t vector_width);

/**
 * @brief 创建默认循环优化配置
 * 
 * 创建并返回默认的循环优化配置。
 * 
 * @return Stru_LoopOptimizationConfig_t* 默认配置，调用者负责销毁
 */
Stru_LoopOptimizationConfig_t* F_create_default_loop_optimization_config(void);

/**
 * @brief 销毁循环优化配置
 * 
 * 释放循环优化配置占用的资源。
 * 
 * @param config 要销毁的配置
 */
void F_destroy_loop_optimization_config(Stru_LoopOptimizationConfig_t* config);

/**
 * @brief 循环优化类型转字符串
 * 
 * 将循环优化类型转换为可读的字符串表示。
 * 
 * @param type 循环优化类型
 * @return const char* 类型字符串表示
 */
const char* F_loop_optimization_type_to_string(Eum_LoopOptimizationType type);

#endif /* CN_LOOP_OPTIMIZER_H */
