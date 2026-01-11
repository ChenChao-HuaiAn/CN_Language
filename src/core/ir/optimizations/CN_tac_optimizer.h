/******************************************************************************
 * 文件名: CN_tac_optimizer.h
 * 功能: CN_Language 三地址码优化器接口定义
 
 * 定义三地址码（TAC）的优化器接口和实现。
 * 支持TAC级别的各种优化算法，包括常量折叠、死代码消除等。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_TAC_OPTIMIZER_H
#define CN_TAC_OPTIMIZER_H

#include "../CN_ir_optimizer.h"
#include "../implementations/tac/CN_tac_interface.h"
#include "../../../infrastructure/containers/hash/CN_hash_table.h"

// ============================================================================
// TAC优化器私有数据结构
// ============================================================================

/**
 * @brief TAC常量折叠器私有数据
 * 
 * 存储常量折叠器的内部状态。
 */
typedef struct {
    Stru_HashTable_t* constant_values;   ///< 常量值映射表
    Stru_HashTable_t* expression_cache;  ///< 表达式缓存
    bool aggressive_folding;             ///< 是否启用激进折叠
    size_t max_cache_size;               ///< 最大缓存大小
    size_t cache_hits;                   ///< 缓存命中次数
    size_t cache_misses;                 ///< 缓存未命中次数
} TacConstantFoldingPrivateData;

/**
 * @brief TAC死代码消除器私有数据
 * 
 * 存储死代码消除器的内部状态。
 */
typedef struct {
    Stru_HashTable_t* used_variables;    ///< 使用的变量集合
    Stru_HashTable_t* defined_variables; ///< 定义的变量集合
    Stru_HashTable_t* reachable_code;    ///< 可达代码集合
    bool analyze_control_flow;           ///< 是否分析控制流
    bool remove_unused_vars;             ///< 是否移除未使用变量
    size_t removed_instructions;         ///< 移除的指令数量
} TacDeadCodeEliminationPrivateData;

/**
 * @brief TAC公共子表达式消除器私有数据
 * 
 * 存储公共子表达式消除器的内部状态。
 */
typedef struct {
    Stru_HashTable_t* expression_table;  ///< 表达式哈希表
    Stru_HashTable_t* value_numbering;   ///< 值编号表
    size_t next_value_number;            ///< 下一个值编号
    bool enable_global_cse;              ///< 是否启用全局CSE
    size_t eliminated_expressions;       ///< 消除的表达式数量
} TacCommonSubexpressionPrivateData;

/**
 * @brief TAC强度削减器私有数据
 * 
 * 存储强度削减器的内部状态。
 */
typedef struct {
    Stru_HashTable_t* strength_patterns; ///< 强度削减模式表
    Stru_HashTable_t* induction_vars;    ///< 归纳变量表
    bool enable_loop_strength_reduction; ///< 是否启用循环强度削减
    size_t reduced_operations;           ///< 减少的操作数量
} TacStrengthReductionPrivateData;

/**
 * @brief TAC优化器私有数据
 * 
 * 存储TAC优化器的完整内部状态。
 */
typedef struct {
    // 优化器组件
    TacConstantFoldingPrivateData* constant_folder;      ///< 常量折叠器
    TacDeadCodeEliminationPrivateData* dead_code_eliminator; ///< 死代码消除器
    TacCommonSubexpressionPrivateData* cse_eliminator;   ///< 公共子表达式消除器
    TacStrengthReductionPrivateData* strength_reducer;   ///< 强度削减器
    
    // 统计信息
    Stru_IrOptimizationStats_t* stats;                   ///< 优化统计信息
    Stru_IrOptimizationConfig_t* config;                 ///< 优化配置
    
    // 状态标志
    bool initialized;                                    ///< 是否已初始化
    bool optimization_in_progress;                       ///< 优化是否进行中
    
    // 性能监控
    double total_optimization_time;                      ///< 总优化时间
    size_t total_optimizations_applied;                  ///< 应用的总优化次数
} TacOptimizerPrivateData;

// ============================================================================
// TAC优化器接口函数声明
// ============================================================================

/**
 * @brief 创建TAC优化器接口实例
 * 
 * 创建TAC（三地址码）的优化器接口实例。
 * 
 * @return Stru_IrOptimizerInterface_t* TAC优化器接口实例
 */
Stru_IrOptimizerInterface_t* F_create_tac_optimizer_interface(void);

/**
 * @brief 销毁TAC优化器接口实例
 * 
 * 销毁TAC优化器接口实例及其所有资源。
 * 
 * @param optimizer TAC优化器接口实例
 */
void F_destroy_tac_optimizer_interface(Stru_IrOptimizerInterface_t* optimizer);

// ============================================================================
// TAC特定优化函数声明
// ============================================================================

/**
 * @brief 应用TAC常量折叠
 * 
 * 对TAC IR应用常量折叠优化。
 * 
 * @param tac_data TAC数据
 * @param config 优化配置
 * @return Stru_IrOptimizationResult_t* 优化结果
 */
Stru_IrOptimizationResult_t* F_apply_tac_constant_folding(Stru_TacData_t* tac_data,
                                                         const Stru_IrOptimizationConfig_t* config);

/**
 * @brief 应用TAC死代码消除
 * 
 * 对TAC IR应用死代码消除优化。
 * 
 * @param tac_data TAC数据
 * @param config 优化配置
 * @return Stru_IrOptimizationResult_t* 优化结果
 */
Stru_IrOptimizationResult_t* F_apply_tac_dead_code_elimination(Stru_TacData_t* tac_data,
                                                              const Stru_IrOptimizationConfig_t* config);

/**
 * @brief 应用TAC公共子表达式消除
 * 
 * 对TAC IR应用公共子表达式消除优化。
 * 
 * @param tac_data TAC数据
 * @param config 优化配置
 * @return Stru_IrOptimizationResult_t* 优化结果
 */
Stru_IrOptimizationResult_t* F_apply_tac_common_subexpression_elimination(Stru_TacData_t* tac_data,
                                                                         const Stru_IrOptimizationConfig_t* config);

/**
 * @brief 应用TAC强度削减
 * 
 * 对TAC IR应用强度削减优化。
 * 
 * @param tac_data TAC数据
 * @param config 优化配置
 * @return Stru_IrOptimizationResult_t* 优化结果
 */
Stru_IrOptimizationResult_t* F_apply_tac_strength_reduction(Stru_TacData_t* tac_data,
                                                           const Stru_IrOptimizationConfig_t* config);

/**
 * @brief 应用TAC窥孔优化
 * 
 * 对TAC IR应用窥孔优化。
 * 
 * @param tac_data TAC数据
 * @param config 优化配置
 * @return Stru_IrOptimizationResult_t* 优化结果
 */
Stru_IrOptimizationResult_t* F_apply_tac_peephole_optimization(Stru_TacData_t* tac_data,
                                                              const Stru_IrOptimizationConfig_t* config);

/**
 * @brief 分析TAC优化潜力
 * 
 * 分析TAC IR的优化潜力，识别可优化的区域。
 * 
 * @param tac_data TAC数据
 * @param potential_areas 输出参数，潜在优化区域
 * @param area_count 输出参数，区域数量
 * @return bool 成功返回true，失败返回false
 */
bool F_analyze_tac_optimization_potential(Stru_TacData_t* tac_data,
                                         void** potential_areas,
                                         size_t* area_count);

/**
 * @brief 验证TAC优化正确性（TAC特定版本）
 * 
 * 验证TAC优化前后的正确性。
 * 
 * @param original_tac 原始TAC数据
 * @param optimized_tac 优化后TAC数据
 * @return bool 优化正确返回true，否则返回false
 */
bool F_validate_tac_optimization_specific(const Stru_TacData_t* original_tac,
                                         const Stru_TacData_t* optimized_tac);

/**
 * @brief 计算TAC指令统计信息
 * 
 * 计算TAC指令的统计信息。
 * 
 * @param tac_data TAC数据
 * @param stats 输出参数，统计信息
 * @return bool 成功返回true，失败返回false
 */
bool F_calculate_tac_instruction_stats(const Stru_TacData_t* tac_data,
                                      Stru_IrOptimizationStats_t* stats);

/**
 * @brief 优化TAC指令序列
 * 
 * 优化TAC指令序列，应用多种优化。
 * 
 * @param instructions TAC指令数组
 * @param count 指令数量
 * @param config 优化配置
 * @param optimized_count 输出参数，优化后指令数量
 * @return Stru_TacInstruction_t** 优化后的指令数组
 */
Stru_TacInstruction_t** F_optimize_tac_instruction_sequence(Stru_TacInstruction_t** instructions,
                                                           size_t count,
                                                           const Stru_IrOptimizationConfig_t* config,
                                                           size_t* optimized_count);

// ============================================================================
// TAC优化工具函数声明
// ============================================================================

/**
 * @brief 检查TAC指令是否为常量表达式
 * 
 * 检查TAC指令是否表示常量表达式。
 * 
 * @param instruction TAC指令
 * @return bool 是常量表达式返回true，否则返回false
 */
bool F_is_tac_instruction_constant(const Stru_TacInstruction_t* instruction);

/**
 * @brief 折叠常量TAC指令
 * 
 * 折叠常量TAC指令，计算其结果。
 * 
 * @param instruction TAC指令
 * @param result 输出参数，结果字符串
 * @return bool 成功折叠返回true，否则返回false
 */
bool F_fold_constant_tac_instruction(const Stru_TacInstruction_t* instruction,
                                    char** result);

/**
 * @brief 检查TAC指令是否死代码
 * 
 * 检查TAC指令是否为死代码。
 * 
 * @param instruction TAC指令
 * @param used_vars 使用的变量集合
 * @param defined_vars 定义的变量集合
 * @return bool 是死代码返回true，否则返回false
 */
bool F_is_tac_instruction_dead_code(const Stru_TacInstruction_t* instruction,
                                   const Stru_HashTable_t* used_vars,
                                   const Stru_HashTable_t* defined_vars);

/**
 * @brief 计算TAC表达式哈希值
 * 
 * 计算TAC表达式的哈希值，用于公共子表达式消除。
 * 
 * @param instruction TAC指令
 * @return uint64_t 表达式哈希值
 */
uint64_t F_hash_tac_expression(const Stru_TacInstruction_t* instruction);

/**
 * @brief 检查TAC表达式是否等价
 * 
 * 检查两个TAC表达式是否等价。
 * 
 * @param instr1 第一个TAC指令
 * @param instr2 第二个TAC指令
 * @return bool 表达式等价返回true，否则返回false
 */
bool F_are_tac_expressions_equivalent(const Stru_TacInstruction_t* instr1,
                                     const Stru_TacInstruction_t* instr2);

/**
 * @brief 应用TAC强度削减模式
 * 
 * 对TAC指令应用强度削减模式。
 * 
 * @param instruction TAC指令
 * @param reduced_instr 输出参数，削减后的指令
 * @return bool 成功应用返回true，否则返回false
 */
bool F_apply_tac_strength_reduction_pattern(const Stru_TacInstruction_t* instruction,
                                           Stru_TacInstruction_t** reduced_instr);

/**
 * @brief 应用TAC窥孔优化模式
 * 
 * 对TAC指令序列应用窥孔优化模式。
 * 
 * @param instructions TAC指令数组
 * @param count 指令数量
 * @param start_index 起始索引
 * @param optimized 输出参数，是否优化
 * @return size_t 优化的指令数量
 */
size_t F_apply_tac_peephole_pattern(Stru_TacInstruction_t** instructions,
                                   size_t count,
                                   size_t start_index,
                                   bool* optimized);

// ============================================================================
// TAC优化器接口函数声明
// ============================================================================

/**
 * @brief 应用TAC优化
 * 
 * 对TAC IR应用指定的优化类型。
 * 
 * @param ir_data TAC IR数据
 * @param optimization_type 优化类型
 * @return Stru_IrOptimizationResult_t* 优化结果
 */
Stru_IrOptimizationResult_t* F_apply_tac_optimization(void* ir_data,
                                                     Eum_IrOptimizationType optimization_type);

/**
 * @brief 应用TAC优化级别
 * 
 * 对TAC IR应用指定级别的优化。
 * 
 * @param ir_data TAC IR数据
 * @param level 优化级别
 * @return Stru_IrOptimizationResult_t* 优化结果
 */
Stru_IrOptimizationResult_t* F_apply_tac_optimization_level(void* ir_data,
                                                           Eum_IrOptimizationLevel level);

/**
 * @brief 应用所有TAC优化
 * 
 * 对TAC IR应用所有可用的优化。
 * 
 * @param ir_data TAC IR数据
 * @return Stru_IrOptimizationResult_t* 优化结果
 */
Stru_IrOptimizationResult_t* F_apply_all_tac_optimizations(void* ir_data);

/**
 * @brief 分析TAC IR
 * 
 * 分析TAC IR，收集优化统计信息。
 * 
 * @param ir_data TAC IR数据
 * @param stats 输出参数，优化统计信息
 * @return bool 成功返回true，失败返回false
 */
bool F_analyze_tac_ir(void* ir_data,
                     Stru_IrOptimizationStats_t* stats);

/**
 * @brief 检查是否可以应用TAC优化
 * 
 * 检查是否可以对TAC IR应用指定的优化类型。
 * 
 * @param ir_data TAC IR数据
 * @param optimization_type 优化类型
 * @return bool 可以应用返回true，否则返回false
 */
bool F_can_apply_tac_optimization(void* ir_data,
                                 Eum_IrOptimizationType optimization_type);

/**
 * @brief 估计TAC改进
 * 
 * 估计对TAC IR应用指定优化类型的改进效果。
 * 
 * @param ir_data TAC IR数据
 * @param optimization_type 优化类型
 * @return double 改进比例（0.0-1.0）
 */
double F_estimate_tac_improvement(void* ir_data,
                                 Eum_IrOptimizationType optimization_type);

/**
 * @brief 获取TAC优化器统计信息
 * 
 * 获取TAC优化器的统计信息。
 * 
 * @return Stru_IrOptimizationStats_t* 优化统计信息
 */
Stru_IrOptimizationStats_t* F_get_tac_optimizer_statistics(void);

/**
 * @brief 重置TAC优化器统计信息
 * 
 * 重置TAC优化器的统计信息。
 * 
 * @return bool 成功返回true，失败返回false
 */
bool F_reset_tac_optimizer_statistics(void);

/**
 * @brief 生成TAC优化报告
 * 
 * 生成TAC优化报告到指定文件。
 * 
 * @param filename 文件名
 * @return bool 成功返回true，失败返回false
 */
bool F_generate_tac_optimization_report(const char* filename);

/**
 * @brief 验证TAC优化
 * 
 * 验证TAC优化的正确性。
 * 
 * @param original_ir 原始IR
 * @param optimized_ir 优化后IR
 * @return bool 验证通过返回true，否则返回false
 */
bool F_validate_tac_optimization(void* original_ir,
                                void* optimized_ir);

/**
 * @brief 测试TAC优化正确性
 * 
 * 测试指定优化类型的正确性。
 * 
 * @param opt_type 优化类型
 * @return bool 测试通过返回true，否则返回false
 */
bool F_test_tac_optimization_correctness(Eum_IrOptimizationType opt_type);

/**
 * @brief 销毁TAC优化结果
 * 
 * 销毁TAC优化结果。
 * 
 * @param result 优化结果
 */
void F_destroy_tac_optimization_result(Stru_IrOptimizationResult_t* result);

/**
 * @brief 销毁TAC优化统计信息
 * 
 * 销毁TAC优化统计信息。
 * 
 * @param stats 优化统计信息
 */
void F_destroy_tac_optimization_stats(Stru_IrOptimizationStats_t* stats);

/**
 * @brief 创建TAC默认配置
 * 
 * 创建TAC优化器的默认配置。
 * 
 * @return Stru_IrOptimizationConfig_t* 默认配置
 */
Stru_IrOptimizationConfig_t* F_create_tac_default_config(void);

// ============================================================================
// TAC优化器版本信息函数
// ============================================================================

/**
 * @brief 获取TAC优化器版本信息
 * 
 * 获取TAC优化器的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 */
void F_get_tac_optimizer_version(int* major, int* minor, int* patch);

/**
 * @brief 获取TAC优化器版本字符串
 * 
 * 获取TAC优化器的版本字符串。
 * 
 * @return const char* 版本字符串
 */
const char* F_get_tac_optimizer_version_string(void);

#endif /* CN_TAC_OPTIMIZER_H */
