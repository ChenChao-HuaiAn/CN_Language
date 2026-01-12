/******************************************************************************
 * 文件名: CN_loop_analysis.h
 * 功能: CN_Language 循环分析模块
 * 
 * 提供循环分析功能，包括循环结构识别、循环信息收集等。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_LOOP_ANALYSIS_H
#define CN_LOOP_ANALYSIS_H

#include "../../CN_optimizer_interface.h"
#include "../CN_loop_optimizer.h"

/**
 * @brief 分析循环结构（内部实现）
 * 
 * 分析AST中的循环结构，收集循环信息。
 * 
 * @param ast AST根节点
 * @return Stru_DynamicArray_t* 循环分析信息数组，调用者负责销毁
 */
Stru_DynamicArray_t* F_analyze_loops_internal(Stru_AstNode_t* ast);

/**
 * @brief 创建循环分析信息
 * 
 * 创建并初始化循环分析信息结构体。
 * 
 * @param loop_id 循环ID
 * @param nesting_level 循环嵌套层级
 * @return Stru_LoopAnalysisInfo_t* 循环分析信息，调用者负责销毁
 */
Stru_LoopAnalysisInfo_t* F_create_loop_analysis_info(size_t loop_id, size_t nesting_level);

/**
 * @brief 销毁循环分析信息
 * 
 * 释放循环分析信息占用的资源。
 * 
 * @param info 要销毁的循环分析信息
 */
void F_destroy_loop_analysis_info(Stru_LoopAnalysisInfo_t* info);

/**
 * @brief 检查节点是否为循环节点
 * 
 * 检查AST节点是否为循环节点（for、while、do-while等）。
 * 
 * @param node AST节点
 * @return bool 是否为循环节点
 */
bool F_is_loop_node(Stru_AstNode_t* node);

/**
 * @brief 获取循环嵌套层级
 * 
 * 获取循环节点的嵌套层级。
 * 
 * @param node 循环节点
 * @return size_t 嵌套层级（0表示最外层）
 */
size_t F_get_loop_nesting_level(Stru_AstNode_t* node);

/**
 * @brief 分析循环依赖关系
 * 
 * 分析循环中的数据依赖关系。
 * 
 * @param loop_info 循环分析信息
 * @param ast 循环AST节点
 * @return bool 分析是否成功
 */
bool F_analyze_loop_dependencies(Stru_LoopAnalysisInfo_t* loop_info, Stru_AstNode_t* ast);

#endif /* CN_LOOP_ANALYSIS_H */
