/******************************************************************************
 * 文件名: CN_cfg_control_flow_graph.h
 * 功能: CN_Language 控制流图数据结构定义
 * 
 * 定义控制流图数据结构，包括基本块集合、入口出口块、
 * 支配树、循环信息等。
 * 
 * 遵循项目架构规范，采用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_CFG_CONTROL_FLOW_GRAPH_H
#define CN_CFG_CONTROL_FLOW_GRAPH_H

#include <stddef.h>
#include <stdbool.h>
#include "CN_cfg_basic_block.h"

// ============================================================================
// 类型定义
// ============================================================================

/**
 * @brief 控制流图结构体
 * 
 * 定义完整的控制流图，包含基本块集合、入口出口块、
 * 支配树、循环信息等。
 */
typedef struct Stru_ControlFlowGraph_t {
    // 基本信息
    char* name;                         ///< 控制流图名称（通常是函数名）
    size_t id;                          ///< 控制流图ID
    
    // 基本块集合
    Stru_BasicBlock_t** blocks;         ///< 基本块数组
    size_t block_count;                 ///< 基本块数量
    size_t block_capacity;              ///< 基本块数组容量
    
    // 特殊基本块
    Stru_BasicBlock_t* entry_block;     ///< 入口基本块
    Stru_BasicBlock_t* exit_block;      ///< 出口基本块
    
    // 支配树信息
    Stru_BasicBlock_t** postorder;      ///< 后序遍历顺序
    size_t postorder_count;             ///< 后序遍历数量
    
    Stru_BasicBlock_t** reverse_postorder; ///< 逆后序遍历顺序
    size_t reverse_postorder_count;     ///< 逆后序遍历数量
    
    // 循环信息
    Stru_BasicBlock_t** loop_headers;   ///< 循环头数组
    size_t loop_header_count;           ///< 循环头数量
    size_t loop_header_capacity;        ///< 循环头数组容量
    
    // 数据流分析信息
    void* reaching_definitions;         ///< 到达定义分析结果
    void* live_variables;               ///< 活跃变量分析结果
    void* available_expressions;        ///< 可用表达式分析结果
    
    // 统计信息
    size_t max_loop_depth;              ///< 最大循环嵌套深度
    size_t total_instructions;          ///< 总指令数
    size_t total_basic_blocks;          ///< 总基本块数（与block_count相同）
    
    // 内部状态
    void* internal_data;                ///< 内部数据（用于算法临时存储）
    bool analyzed;                      ///< 是否已分析
} Stru_ControlFlowGraph_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

/**
 * @brief 创建控制流图
 * 
 * 创建新的控制流图实例。
 * 
 * @param name 控制流图名称（可空，自动生成）
 * @param id 控制流图ID（如果为0，自动生成）
 * @return Stru_ControlFlowGraph_t* 新创建的控制流图，失败返回NULL
 */
Stru_ControlFlowGraph_t* F_create_control_flow_graph(const char* name, size_t id);

/**
 * @brief 销毁控制流图
 * 
 * 销毁控制流图及其所有资源。
 * 
 * @param cfg 控制流图
 */
void F_destroy_control_flow_graph(Stru_ControlFlowGraph_t* cfg);

/**
 * @brief 复制控制流图
 * 
 * 创建控制流图的深拷贝。
 * 
 * @param src 源控制流图
 * @return Stru_ControlFlowGraph_t* 新创建的控制流图副本，失败返回NULL
 */
Stru_ControlFlowGraph_t* F_copy_control_flow_graph(const Stru_ControlFlowGraph_t* src);

// ============================================================================
// 基本块管理函数
// ============================================================================

/**
 * @brief 添加基本块到控制流图
 * 
 * 向控制流图添加基本块。
 * 
 * @param cfg 控制流图
 * @param block 基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_add_basic_block(Stru_ControlFlowGraph_t* cfg, Stru_BasicBlock_t* block);

/**
 * @brief 从控制流图移除基本块
 * 
 * 从控制流图移除基本块。
 * 
 * @param cfg 控制流图
 * @param block 基本块
 * @return Stru_BasicBlock_t* 被移除的基本块，失败返回NULL
 */
Stru_BasicBlock_t* F_cfg_remove_basic_block(Stru_ControlFlowGraph_t* cfg, 
                                            Stru_BasicBlock_t* block);

/**
 * @brief 获取控制流图基本块
 * 
 * 获取控制流图的指定基本块。
 * 
 * @param cfg 控制流图
 * @param index 基本块索引
 * @return Stru_BasicBlock_t* 基本块，索引无效返回NULL
 */
Stru_BasicBlock_t* F_cfg_get_basic_block(const Stru_ControlFlowGraph_t* cfg, 
                                         size_t index);

/**
 * @brief 根据ID查找基本块
 * 
 * 根据基本块ID在控制流图中查找基本块。
 * 
 * @param cfg 控制流图
 * @param id 基本块ID
 * @return Stru_BasicBlock_t* 找到的基本块，未找到返回NULL
 */
Stru_BasicBlock_t* F_cfg_find_basic_block_by_id(const Stru_ControlFlowGraph_t* cfg, 
                                                size_t id);

/**
 * @brief 根据名称查找基本块
 * 
 * 根据基本块名称在控制流图中查找基本块。
 * 
 * @param cfg 控制流图
 * @param name 基本块名称
 * @return Stru_BasicBlock_t* 找到的基本块，未找到返回NULL
 */
Stru_BasicBlock_t* F_cfg_find_basic_block_by_name(const Stru_ControlFlowGraph_t* cfg, 
                                                  const char* name);

/**
 * @brief 清空控制流图基本块
 * 
 * 清空控制流图的所有基本块。
 * 
 * @param cfg 控制流图
 */
void F_cfg_clear_basic_blocks(Stru_ControlFlowGraph_t* cfg);

// ============================================================================
// 入口出口块管理函数
// ============================================================================

/**
 * @brief 设置入口基本块
 * 
 * 设置控制流图的入口基本块。
 * 
 * @param cfg 控制流图
 * @param entry_block 入口基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_set_entry_block(Stru_ControlFlowGraph_t* cfg, 
                           Stru_BasicBlock_t* entry_block);

/**
 * @brief 设置出口基本块
 * 
 * 设置控制流图的出口基本块。
 * 
 * @param cfg 控制流图
 * @param exit_block 出口基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_set_exit_block(Stru_ControlFlowGraph_t* cfg, 
                          Stru_BasicBlock_t* exit_block);

/**
 * @brief 获取入口基本块
 * 
 * 获取控制流图的入口基本块。
 * 
 * @param cfg 控制流图
 * @return Stru_BasicBlock_t* 入口基本块
 */
Stru_BasicBlock_t* F_cfg_get_entry_block(const Stru_ControlFlowGraph_t* cfg);

/**
 * @brief 获取出口基本块
 * 
 * 获取控制流图的出口基本块。
 * 
 * @param cfg 控制流图
 * @return Stru_BasicBlock_t* 出口基本块
 */
Stru_BasicBlock_t* F_cfg_get_exit_block(const Stru_ControlFlowGraph_t* cfg);

// ============================================================================
// 遍历顺序管理函数
// ============================================================================

/**
 * @brief 设置后序遍历顺序
 * 
 * 设置控制流图的后序遍历顺序。
 * 
 * @param cfg 控制流图
 * @param order 后序遍历顺序数组
 * @param count 数组元素数量
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_set_postorder(Stru_ControlFlowGraph_t* cfg, 
                         Stru_BasicBlock_t** order, size_t count);

/**
 * @brief 设置逆后序遍历顺序
 * 
 * 设置控制流图的逆后序遍历顺序。
 * 
 * @param cfg 控制流图
 * @param order 逆后序遍历顺序数组
 * @param count 数组元素数量
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_set_reverse_postorder(Stru_ControlFlowGraph_t* cfg, 
                                 Stru_BasicBlock_t** order, size_t count);

/**
 * @brief 获取后序遍历顺序
 * 
 * 获取控制流图的后序遍历顺序。
 * 
 * @param cfg 控制流图
 * @param count 输出参数，数组元素数量
 * @return Stru_BasicBlock_t** 后序遍历顺序数组
 */
Stru_BasicBlock_t** F_cfg_get_postorder(const Stru_ControlFlowGraph_t* cfg, 
                                        size_t* count);

/**
 * @brief 获取逆后序遍历顺序
 * 
 * 获取控制流图的逆后序遍历顺序。
 * 
 * @param cfg 控制流图
 * @param count 输出参数，数组元素数量
 * @return Stru_BasicBlock_t** 逆后序遍历顺序数组
 */
Stru_BasicBlock_t** F_cfg_get_reverse_postorder(const Stru_ControlFlowGraph_t* cfg, 
                                                size_t* count);

// ============================================================================
// 循环信息管理函数
// ============================================================================

/**
 * @brief 添加循环头
 * 
 * 向控制流图添加循环头。
 * 
 * @param cfg 控制流图
 * @param loop_header 循环头基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_add_loop_header(Stru_ControlFlowGraph_t* cfg, 
                           Stru_BasicBlock_t* loop_header);

/**
 * @brief 移除循环头
 * 
 * 从控制流图移除循环头。
 * 
 * @param cfg 控制流图
 * @param loop_header 循环头基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_remove_loop_header(Stru_ControlFlowGraph_t* cfg, 
                              Stru_BasicBlock_t* loop_header);

/**
 * @brief 获取循环头数组
 * 
 * 获取控制流图的循环头数组。
 * 
 * @param cfg 控制流图
 * @param count 输出参数，数组元素数量
 * @return Stru_BasicBlock_t** 循环头数组
 */
Stru_BasicBlock_t** F_cfg_get_loop_headers(const Stru_ControlFlowGraph_t* cfg, 
                                           size_t* count);

// ============================================================================
// 数据流分析信息管理函数
// ============================================================================

/**
 * @brief 设置到达定义分析结果
 * 
 * 设置控制流图的到达定义分析结果。
 * 
 * @param cfg 控制流图
 * @param reaching_definitions 到达定义分析结果
 */
void F_cfg_set_reaching_definitions(Stru_ControlFlowGraph_t* cfg, 
                                    void* reaching_definitions);

/**
 * @brief 设置活跃变量分析结果
 * 
 * 设置控制流图的活跃变量分析结果。
 * 
 * @param cfg 控制流图
 * @param live_variables 活跃变量分析结果
 */
void F_cfg_set_live_variables(Stru_ControlFlowGraph_t* cfg, 
                              void* live_variables);

/**
 * @brief 设置可用表达式分析结果
 * 
 * 设置控制流图的可用表达式分析结果。
 * 
 * @param cfg 控制流图
 * @param available_expressions 可用表达式分析结果
 */
void F_cfg_set_available_expressions(Stru_ControlFlowGraph_t* cfg, 
                                     void* available_expressions);

/**
 * @brief 获取到达定义分析结果
 * 
 * 获取控制流图的到达定义分析结果。
 * 
 * @param cfg 控制流图
 * @return void* 到达定义分析结果
 */
void* F_cfg_get_reaching_definitions(const Stru_ControlFlowGraph_t* cfg);

/**
 * @brief 获取活跃变量分析结果
 * 
 * 获取控制流图的活跃变量分析结果。
 * 
 * @param cfg 控制流图
 * @return void* 活跃变量分析结果
 */
void* F_cfg_get_live_variables(const Stru_ControlFlowGraph_t* cfg);

/**
 * @brief 获取可用表达式分析结果
 * 
 * 获取控制流图的可用表达式分析结果。
 * 
 * @param cfg 控制流图
 * @return void* 可用表达式分析结果
 */
void* F_cfg_get_available_expressions(const Stru_ControlFlowGraph_t* cfg);

// ============================================================================
// 查询函数
// ============================================================================

/**
 * @brief 获取控制流图名称
 * 
 * 获取控制流图的名称。
 * 
 * @param cfg 控制流图
 * @return const char* 控制流图名称
 */
const char* F_cfg_get_name(const Stru_ControlFlowGraph_t* cfg);

/**
 * @brief 设置控制流图名称
 * 
 * 设置控制流图的名称。
 * 
 * @param cfg 控制流图
 * @param name 新名称
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_set_name(Stru_ControlFlowGraph_t* cfg, const char* name);

/**
 * @brief 获取控制流图ID
 * 
 * 获取控制流图的ID。
 * 
 * @param cfg 控制流图
 * @return size_t 控制流图ID
 */
size_t F_cfg_get_id(const Stru_ControlFlowGraph_t* cfg);

/**
 * @brief 检查控制流图是否为空
 * 
 * 检查控制流图是否为空（无基本块）。
 * 
 * @param cfg 控制流图
 * @return bool 为空返回true，否则返回false
 */
bool F_cfg_is_empty(const Stru_ControlFlowGraph_t* cfg);

/**
 * @brief 检查控制流图是否已分析
 * 
 * 检查控制流图是否已分析。
 * 
 * @param cfg 控制流图
 * @return bool 已分析返回true，否则返回false
 */
bool F_cfg_is_analyzed(const Stru_ControlFlowGraph_t* cfg);

/**
 * @brief 设置控制流图分析状态
 * 
 * 设置控制流图的分析状态。
 * 
 * @param cfg 控制流图
 * @param analyzed 分析状态
 */
void F_cfg_set_analyzed(Stru_ControlFlowGraph_t* cfg, bool analyzed);

/**
 * @brief 获取控制流图统计信息
 * 
 * 获取控制流图的统计信息。
 * 
 * @param cfg 控制流图
 * @param max_loop_depth 输出参数，最大循环嵌套深度
 * @param total_instructions 输出参数，总指令数
 * @param total_basic_blocks 输出参数，总基本块数
 */
void F_cfg_get_statistics(const Stru_ControlFlowGraph_t* cfg,
                          size_t* max_loop_depth,
                          size_t* total_instructions,
                          size_t* total_basic_blocks);

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 生成控制流图名称
 * 
 * 根据控制流图ID生成默认名称。
 * 
 * @param id 控制流图ID
 * @return char* 生成的名称，调用者负责释放
 */
char* F_generate_cfg_name(size_t id);

/**
 * @brief 验证控制流图
 * 
 * 验证控制流图的完整性。
 * 
 * @param cfg 控制流图
 * @param error_message 输出参数，错误信息
 * @return bool 有效返回true，无效返回false
 */
bool F_validate_control_flow_graph(const Stru_ControlFlowGraph_t* cfg, 
                                   char** error_message);

/**
 * @brief 重置控制流图访问标记
 * 
 * 重置控制流图中所有基本块的访问标记。
 * 
 * @param cfg 控制流图
 */
void F_cfg_reset_visited_marks(Stru_ControlFlowGraph_t* cfg);

/**
 * @brief 计算控制流图统计信息
 * 
 * 计算控制流图的统计信息并更新内部状态。
 * 
 * @param cfg 控制流图
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_compute_statistics(Stru_ControlFlowGraph_t* cfg);

#endif /* CN_CFG_CONTROL_FLOW_GRAPH_H */
