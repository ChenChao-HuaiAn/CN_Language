/******************************************************************************
 * 文件名: CN_cfg_builder.h
 * 功能: CN_Language 控制流图构建器接口定义
 * 
 * 定义控制流图构建器接口，用于从IR构建控制流图。
 * 包括基本块划分算法、控制流图构建算法等。
 * 
 * 遵循项目架构规范，采用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_CFG_BUILDER_H
#define CN_CFG_BUILDER_H

#include <stddef.h>
#include <stdbool.h>
#include "../../CN_ir_interface.h"
#include "CN_cfg_control_flow_graph.h"

// ============================================================================
// 类型定义
// ============================================================================

/**
 * @brief 基本块划分算法类型枚举
 * 
 * 定义支持的基本块划分算法类型。
 */
typedef enum {
    Eum_BASIC_BLOCK_PARTITION_SIMPLE,   ///< 简单划分算法
    Eum_BASIC_BLOCK_PARTITION_ADVANCED, ///< 高级划分算法
    Eum_BASIC_BLOCK_PARTITION_OPTIMIZED ///< 优化划分算法
} Eum_BasicBlockPartitionAlgorithm;

/**
 * @brief 控制流图构建选项结构体
 * 
 * 定义控制流图构建的配置选项。
 */
typedef struct {
    Eum_BasicBlockPartitionAlgorithm partition_algorithm; ///< 基本块划分算法
    bool compute_dominators;           ///< 是否计算支配关系
    bool compute_postorder;            ///< 是否计算后序遍历
    bool identify_loops;               ///< 是否识别循环
    bool compute_data_flow;            ///< 是否计算数据流分析
    bool validate_cfg;                 ///< 是否验证控制流图
    bool optimize_cfg;                 ///< 是否优化控制流图
    int optimization_level;            ///< 优化级别
} Stru_CfgBuildOptions_t;

/**
 * @brief 控制流图构建结果结构体
 * 
 * 定义控制流图构建的结果。
 */
typedef struct {
    bool success;                      ///< 构建是否成功
    Stru_ControlFlowGraph_t* cfg;      ///< 构建的控制流图
    char* errors;                      ///< 错误信息
    char* warnings;                    ///< 警告信息
    size_t basic_block_count;          ///< 生成的基本块数量
    size_t instruction_count;          ///< 处理的指令数量
    double build_time;                 ///< 构建时间（秒）
} Stru_CfgBuildResult_t;

/**
 * @brief 控制流图构建器接口结构体
 * 
 * 定义控制流图构建器接口。
 */
typedef struct Stru_CfgBuilderInterface_t {
    // 基本信息
    const char* (*get_name)(void);
    const char* (*get_version)(void);
    
    // 构建器配置
    bool (*set_options)(Stru_CfgBuildOptions_t* options);
    Stru_CfgBuildOptions_t* (*get_options)(void);
    
    // 控制流图构建
    Stru_CfgBuildResult_t* (*build_from_ir)(void* ir);
    Stru_CfgBuildResult_t* (*build_from_instructions)(Stru_IrInstruction_t** instructions, 
                                                      size_t instruction_count);
    
    // 基本块划分
    Stru_BasicBlock_t** (*partition_basic_blocks)(Stru_IrInstruction_t** instructions,
                                                  size_t instruction_count,
                                                  size_t* block_count);
    
    // 控制流分析
    bool (*compute_dominators)(Stru_ControlFlowGraph_t* cfg);
    bool (*compute_postorder)(Stru_ControlFlowGraph_t* cfg);
    bool (*identify_loops)(Stru_ControlFlowGraph_t* cfg);
    
    // 数据流分析
    bool (*compute_reaching_definitions)(Stru_ControlFlowGraph_t* cfg);
    bool (*compute_live_variables)(Stru_ControlFlowGraph_t* cfg);
    bool (*compute_available_expressions)(Stru_ControlFlowGraph_t* cfg);
    
    // 控制流图优化
    bool (*optimize_cfg)(Stru_ControlFlowGraph_t* cfg, int optimization_level);
    bool (*remove_unreachable_blocks)(Stru_ControlFlowGraph_t* cfg);
    bool (*merge_blocks)(Stru_ControlFlowGraph_t* cfg);
    
    // 验证
    bool (*validate_cfg)(const Stru_ControlFlowGraph_t* cfg, char** error_message);
    
    // 序列化
    const char* (*serialize_cfg)(const Stru_ControlFlowGraph_t* cfg);
    bool (*save_cfg_to_file)(const Stru_ControlFlowGraph_t* cfg, const char* filename);
    
    // 资源管理
    void (*destroy_build_result)(Stru_CfgBuildResult_t* result);
    void (*destroy)(void);
    
    // 内部状态
    void* internal_state;
} Stru_CfgBuilderInterface_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

/**
 * @brief 创建控制流图构建器接口实例
 * 
 * 创建新的控制流图构建器接口实例。
 * 
 * @return Stru_CfgBuilderInterface_t* 控制流图构建器接口实例，失败返回NULL
 */
Stru_CfgBuilderInterface_t* F_create_cfg_builder_interface(void);

/**
 * @brief 销毁控制流图构建器接口实例
 * 
 * 销毁控制流图构建器接口实例及其所有资源。
 * 
 * @param builder 控制流图构建器接口实例
 */
void F_destroy_cfg_builder_interface(Stru_CfgBuilderInterface_t* builder);

// ============================================================================
// 工具函数声明
// ============================================================================

/**
 * @brief 创建默认控制流图构建选项
 * 
 * 创建默认的控制流图构建选项。
 * 
 * @return Stru_CfgBuildOptions_t* 默认构建选项，失败返回NULL
 */
Stru_CfgBuildOptions_t* F_create_default_cfg_build_options(void);

/**
 * @brief 销毁控制流图构建选项
 * 
 * 销毁控制流图构建选项。
 * 
 * @param options 控制流图构建选项
 */
void F_destroy_cfg_build_options(Stru_CfgBuildOptions_t* options);

/**
 * @brief 创建控制流图构建结果
 * 
 * 创建新的控制流图构建结果。
 * 
 * @return Stru_CfgBuildResult_t* 新创建的构建结果，失败返回NULL
 */
Stru_CfgBuildResult_t* F_create_cfg_build_result(void);

/**
 * @brief 设置控制流图构建结果错误
 * 
 * 设置控制流图构建结果的错误信息。
 * 
 * @param result 控制流图构建结果
 * @param error_message 错误信息
 */
void F_set_cfg_build_result_error(Stru_CfgBuildResult_t* result, const char* error_message);

/**
 * @brief 设置控制流图构建结果警告
 * 
 * 设置控制流图构建结果的警告信息。
 * 
 * @param result 控制流图构建结果
 * @param warning_message 警告信息
 */
void F_set_cfg_build_result_warning(Stru_CfgBuildResult_t* result, const char* warning_message);

/**
 * @brief 检查IR是否可构建控制流图
 * 
 * 检查IR是否可以构建控制流图。
 * 
 * @param ir IR实例
 * @param error_message 输出参数，错误信息
 * @return bool 可构建返回true，不可构建返回false
 */
bool F_can_build_cfg_from_ir(void* ir, char** error_message);

/**
 * @brief 估算控制流图构建资源需求
 * 
 * 估算将IR转换为控制流图所需的资源。
 * 
 * @param ir IR实例
 * @param estimated_memory 输出参数，估算的内存需求（字节）
 * @param estimated_time 输出参数，估算的时间需求（秒）
 * @return bool 成功返回true，失败返回false
 */
bool F_estimate_cfg_build_resources(void* ir, 
                                    size_t* estimated_memory, 
                                    double* estimated_time);

// ============================================================================
// 基本块划分算法函数
// ============================================================================

/**
 * @brief 简单基本块划分算法
 * 
 * 使用简单算法划分基本块。
 * 
 * @param instructions 指令数组
 * @param instruction_count 指令数量
 * @param block_count 输出参数，生成的基本块数量
 * @return Stru_BasicBlock_t** 基本块数组，失败返回NULL
 */
Stru_BasicBlock_t** F_partition_basic_blocks_simple(Stru_IrInstruction_t** instructions,
                                                    size_t instruction_count,
                                                    size_t* block_count);

/**
 * @brief 高级基本块划分算法
 * 
 * 使用高级算法划分基本块，考虑更多优化因素。
 * 
 * @param instructions 指令数组
 * @param instruction_count 指令数量
 * @param block_count 输出参数，生成的基本块数量
 * @return Stru_BasicBlock_t** 基本块数组，失败返回NULL
 */
Stru_BasicBlock_t** F_partition_basic_blocks_advanced(Stru_IrInstruction_t** instructions,
                                                      size_t instruction_count,
                                                      size_t* block_count);

/**
 * @brief 优化基本块划分算法
 * 
 * 使用优化算法划分基本块，考虑性能优化。
 * 
 * @param instructions 指令数组
 * @param instruction_count 指令数量
 * @param block_count 输出参数，生成的基本块数量
 * @return Stru_BasicBlock_t** 基本块数组，失败返回NULL
 */
Stru_BasicBlock_t** F_partition_basic_blocks_optimized(Stru_IrInstruction_t** instructions,
                                                       size_t instruction_count,
                                                       size_t* block_count);

// ============================================================================
// 控制流分析算法函数
// ============================================================================

/**
 * @brief 计算支配关系
 * 
 * 计算控制流图的支配关系。
 * 
 * @param cfg 控制流图
 * @return bool 成功返回true，失败返回false
 */
bool F_compute_dominators(Stru_ControlFlowGraph_t* cfg);

/**
 * @brief 计算后序遍历
 * 
 * 计算控制流图的后序遍历顺序。
 * 
 * @param cfg 控制流图
 * @return bool 成功返回true，失败返回false
 */
bool F_compute_postorder(Stru_ControlFlowGraph_t* cfg);

/**
 * @brief 识别循环
 * 
 * 识别控制流图中的循环结构。
 * 
 * @param cfg 控制流图
 * @return bool 成功返回true，失败返回false
 */
bool F_identify_loops(Stru_ControlFlowGraph_t* cfg);

// ============================================================================
// 数据流分析算法函数
// ============================================================================

/**
 * @brief 计算到达定义
 * 
 * 计算控制流图的到达定义分析。
 * 
 * @param cfg 控制流图
 * @return bool 成功返回true，失败返回false
 */
bool F_compute_reaching_definitions(Stru_ControlFlowGraph_t* cfg);

/**
 * @brief 计算活跃变量
 * 
 * 计算控制流图的活跃变量分析。
 * 
 * @param cfg 控制流图
 * @return bool 成功返回true，失败返回false
 */
bool F_compute_live_variables(Stru_ControlFlowGraph_t* cfg);

/**
 * @brief 计算可用表达式
 * 
 * 计算控制流图的可用表达式分析。
 * 
 * @param cfg 控制流图
 * @return bool 成功返回true，失败返回false
 */
bool F_compute_available_expressions(Stru_ControlFlowGraph_t* cfg);

// ============================================================================
// 控制流图优化函数
// ============================================================================

/**
 * @brief 优化控制流图
 * 
 * 优化控制流图，应用各种优化技术。
 * 
 * @param cfg 控制流图
 * @param optimization_level 优化级别
 * @return bool 成功返回true，失败返回false
 */
bool F_optimize_control_flow_graph(Stru_ControlFlowGraph_t* cfg, int optimization_level);

/**
 * @brief 移除不可达基本块
 * 
 * 移除控制流图中不可达的基本块。
 * 
 * @param cfg 控制流图
 * @return bool 成功返回true，失败返回false
 */
bool F_remove_unreachable_blocks(Stru_ControlFlowGraph_t* cfg);

/**
 * @brief 合并基本块
 * 
 * 合并控制流图中可合并的基本块。
 * 
 * @param cfg 控制流图
 * @return bool 成功返回true，失败返回false
 */
bool F_merge_basic_blocks(Stru_ControlFlowGraph_t* cfg);

// ============================================================================
// 验证函数
// ============================================================================

/**
 * @brief 验证控制流图
 * 
 * 验证控制流图的完整性。
 * 
 * @param cfg 控制流图
 * @param error_message 输出参数，错误信息
 * @return bool 有效返回true，无效返回false
 */
bool F_validate_control_flow_graph_comprehensive(const Stru_ControlFlowGraph_t* cfg, 
                                                 char** error_message);

// ============================================================================
// 版本信息函数
// ============================================================================

/**
 * @brief 获取控制流图构建器版本信息
 * 
 * 获取控制流图构建器的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 */
void F_get_cfg_builder_version(int* major, int* minor, int* patch);

/**
 * @brief 获取控制流图构建器版本字符串
 * 
 * 获取控制流图构建器的版本字符串。
 * 
 * @return const char* 版本字符串
 */
const char* F_get_cfg_builder_version_string(void);

#endif /* CN_CFG_BUILDER_H */
