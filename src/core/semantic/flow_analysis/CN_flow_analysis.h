/**
 * @file CN_flow_analysis.h
 * @brief 控制流分析和数据流分析模块头文件
 
 * 本文件定义了控制流分析和数据流分析模块的接口和数据结构。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_FLOW_ANALYSIS_H
#define CN_FLOW_ANALYSIS_H

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// 前向声明
// ============================================================================

typedef struct Stru_AstNodeInterface_t Stru_AstNodeInterface_t;
typedef struct Stru_SemanticAnalyzerInterface_t Stru_SemanticAnalyzerInterface_t;
typedef struct Stru_FlowAnalysisInterface_t Stru_FlowAnalysisInterface_t;
typedef struct Stru_DataFlowAnalysisInterface_t Stru_DataFlowAnalysisInterface_t;

// ============================================================================
// 控制流分析结果枚举
// ============================================================================

/**
 * @brief 控制流分析结果枚举
 */
typedef enum Eum_FlowAnalysisResult_t
{
    Eum_FLOW_ANALYSIS_SUCCESS = 0,          ///< 分析成功
    Eum_FLOW_ANALYSIS_UNREACHABLE_CODE,     ///< 不可达代码
    Eum_FLOW_ANALYSIS_INFINITE_LOOP,        ///< 无限循环
    Eum_FLOW_ANALYSIS_MISSING_RETURN,       ///< 缺少返回语句
    Eum_FLOW_ANALYSIS_UNINITIALIZED_VAR,    ///< 未初始化变量
    Eum_FLOW_ANALYSIS_DEAD_CODE,            ///< 死代码
    Eum_FLOW_ANALYSIS_ERROR                 ///< 分析错误
} Eum_FlowAnalysisResult_t;

// ============================================================================
// 数据流分析结果枚举
// ============================================================================

/**
 * @brief 数据流分析结果枚举
 */
typedef enum Eum_DataFlowResult_t
{
    Eum_DATA_FLOW_SUCCESS = 0,              ///< 分析成功
    Eum_DATA_FLOW_UNINITIALIZED_USE,        ///< 未初始化使用
    Eum_DATA_FLOW_UNUSED_VARIABLE,          ///< 未使用变量
    Eum_DATA_FLOW_CONSTANT_PROPAGATION,     ///< 常量传播
    Eum_DATA_FLOW_DEAD_STORE,               ///< 死存储
    Eum_DATA_FLOW_ERROR                     ///< 分析错误
} Eum_DataFlowResult_t;

// ============================================================================
// 控制流图节点类型枚举
// ============================================================================

/**
 * @brief 控制流图节点类型枚举
 */
typedef enum Eum_CfgNodeType_t
{
    Eum_CFG_BASIC_BLOCK = 0,               ///< 基本块
    Eum_CFG_ENTRY,                         ///< 入口节点
    Eum_CFG_EXIT,                          ///< 出口节点
    Eum_CFG_CONDITIONAL,                   ///< 条件节点
    Eum_CFG_LOOP_HEADER,                   ///< 循环头节点
    Eum_CFG_LOOP_BODY,                     ///< 循环体节点
    Eum_CFG_LOOP_EXIT                      ///< 循环退出节点
} Eum_CfgNodeType_t;

// ============================================================================
// 控制流图节点结构体
// ============================================================================

/**
 * @brief 控制流图节点结构体
 */
typedef struct Stru_CfgNode_t
{
    Eum_CfgNodeType_t type;                ///< 节点类型
    size_t id;                             ///< 节点ID
    Stru_AstNodeInterface_t* ast_node;     ///< 对应的AST节点
    struct Stru_CfgNode_t** successors;    ///< 后继节点数组
    size_t successor_count;                ///< 后继节点数量
    struct Stru_CfgNode_t** predecessors;  ///< 前驱节点数组
    size_t predecessor_count;              ///< 前驱节点数量
    void* analysis_data;                   ///< 分析数据
} Stru_CfgNode_t;

// ============================================================================
// 控制流图结构体
// ============================================================================

/**
 * @brief 控制流图结构体
 */
typedef struct Stru_ControlFlowGraph_t
{
    Stru_CfgNode_t* entry_node;            ///< 入口节点
    Stru_CfgNode_t* exit_node;             ///< 出口节点
    Stru_CfgNode_t** nodes;                ///< 所有节点数组
    size_t node_count;                     ///< 节点数量
    size_t capacity;                       ///< 节点数组容量
} Stru_ControlFlowGraph_t;

// ============================================================================
// 数据流分析上下文结构体
// ============================================================================

/**
 * @brief 数据流分析上下文结构体
 */
typedef struct Stru_DataFlowContext_t
{
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer;  ///< 语义分析器
    Stru_ControlFlowGraph_t* cfg;                         ///< 控制流图
    void* user_data;                                      ///< 用户自定义数据
} Stru_DataFlowContext_t;

// ============================================================================
// 控制流分析接口结构体
// ============================================================================

/**
 * @brief 控制流分析接口结构体
 */
typedef struct Stru_FlowAnalysisInterface_t
{
    // 控制流图构建
    Stru_ControlFlowGraph_t* (*build_control_flow_graph)(
        Stru_FlowAnalysisInterface_t* analyzer,
        Stru_AstNodeInterface_t* ast_node);
    
    // 控制流分析
    Eum_FlowAnalysisResult_t (*analyze_control_flow)(
        Stru_FlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg);
    
    // 可达性分析
    bool (*is_reachable)(
        Stru_FlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_CfgNode_t* from,
        Stru_CfgNode_t* to);
    
    // 循环检测
    bool (*detect_loops)(
        Stru_FlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_CfgNode_t*** loop_headers,
        size_t* loop_count);
    
    // 销毁控制流图
    void (*destroy_control_flow_graph)(
        Stru_FlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg);
    
    // 私有数据
    void* private_data;
} Stru_FlowAnalysisInterface_t;

// ============================================================================
// 数据流分析接口结构体
// ============================================================================

/**
 * @brief 数据流分析接口结构体
 */
typedef struct Stru_DataFlowAnalysisInterface_t
{
    // 数据流分析
    Eum_DataFlowResult_t (*analyze_data_flow)(
        Stru_DataFlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_DataFlowContext_t* context);
    
    // 活跃变量分析
    bool (*analyze_live_variables)(
        Stru_DataFlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_DataFlowContext_t* context);
    
    // 到达定义分析
    bool (*analyze_reaching_definitions)(
        Stru_DataFlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_DataFlowContext_t* context);
    
    // 常量传播分析
    bool (*analyze_constant_propagation)(
        Stru_DataFlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_DataFlowContext_t* context);
    
    // 未初始化变量检测
    bool (*detect_uninitialized_variables)(
        Stru_DataFlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_DataFlowContext_t* context);
    
    // 未使用变量检测
    bool (*detect_unused_variables)(
        Stru_DataFlowAnalysisInterface_t* analyzer,
        Stru_ControlFlowGraph_t* cfg,
        Stru_DataFlowContext_t* context);
    
    // 私有数据
    void* private_data;
} Stru_DataFlowAnalysisInterface_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

/**
 * @brief 创建控制流分析接口实例
 */
Stru_FlowAnalysisInterface_t* F_create_flow_analysis_interface(void);

/**
 * @brief 销毁控制流分析接口实例
 */
void F_destroy_flow_analysis_interface(Stru_FlowAnalysisInterface_t* interface);

/**
 * @brief 创建数据流分析接口实例
 */
Stru_DataFlowAnalysisInterface_t* F_create_data_flow_analysis_interface(void);

/**
 * @brief 销毁数据流分析接口实例
 */
void F_destroy_data_flow_analysis_interface(Stru_DataFlowAnalysisInterface_t* interface);

#endif // CN_FLOW_ANALYSIS_H
