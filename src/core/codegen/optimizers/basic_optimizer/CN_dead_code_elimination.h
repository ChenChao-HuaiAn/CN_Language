/******************************************************************************
 * 文件名: CN_dead_code_elimination.h
 * 功能: CN_Language 死代码消除优化算法头文件
 * 
 * 定义死代码消除优化算法的接口和数据结构。
 * 死代码消除是移除不会执行的代码的优化技术。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月10日
 * 修改历史:
 *   - 2026年1月10日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_DEAD_CODE_ELIMINATION_H
#define CN_DEAD_CODE_ELIMINATION_H

#include "../../../ast/CN_ast.h"
#include "../../CN_optimizer_interface.h"
#include "CN_basic_optimizer.h"

// ============================================================================
// 死代码消除接口函数
// ============================================================================

/**
 * @brief 应用死代码消除优化
 * 
 * 对AST应用死代码消除优化，移除不会执行的代码。
 * 
 * @param ast AST根节点
 * @param context 优化上下文
 * @return bool 优化是否成功应用
 */
bool apply_dead_code_elimination(Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);

/**
 * @brief 测试死代码消除算法
 * 
 * 运行死代码消除算法的测试用例。
 */
void test_dead_code_elimination(void);

/**
 * @brief 构建控制流图
 * 
 * 从AST构建控制流图，用于死代码分析。
 * 
 * @param ast AST根节点
 * @return void* 控制流图句柄，调用者负责销毁
 */
void* build_control_flow_graph(Stru_AstNode_t* ast);

/**
 * @brief 销毁控制流图
 * 
 * 销毁控制流图及其所有资源。
 * 
 * @param cfg 控制流图句柄
 */
void destroy_control_flow_graph(void* cfg);

/**
 * @brief 分析代码可达性
 * 
 * 分析控制流图中哪些代码是可达的。
 * 
 * @param cfg 控制流图句柄
 * @return bool 分析是否成功
 */
bool analyze_reachability(void* cfg);

/**
 * @brief 分析活跃变量
 * 
 * 分析控制流图中的活跃变量。
 * 
 * @param cfg 控制流图句柄
 * @return bool 分析是否成功
 */
bool analyze_live_variables(void* cfg);

/**
 * @brief 移除不可达代码
 * 
 * 从AST中移除不可达的代码。
 * 
 * @param ast AST根节点
 * @param cfg 控制流图句柄
 * @return size_t 移除的代码数量
 */
size_t remove_unreachable_code(Stru_AstNode_t* ast, void* cfg);

/**
 * @brief 移除死赋值
 * 
 * 从AST中移除死的赋值语句。
 * 
 * @param ast AST根节点
 * @param cfg 控制流图句柄
 * @return size_t 移除的死赋值数量
 */
size_t remove_dead_assignments(Stru_AstNode_t* ast, void* cfg);

/**
 * @brief 移除未使用变量
 * 
 * 从AST中移除未使用的变量声明。
 * 
 * @param ast AST根节点
 * @param cfg 控制流图句柄
 * @return size_t 移除的未使用变量数量
 */
size_t remove_unused_variables(Stru_AstNode_t* ast, void* cfg);

/**
 * @brief 获取死代码消除统计信息
 * 
 * 获取死代码消除优化的统计信息。
 * 
 * @param context 优化上下文
 * @param removed_count 输出参数，已移除的代码数量
 * @param error_count 输出参数，错误数量
 */
void get_dead_code_elimination_stats(Stru_OptimizationContext_t* context, 
                                     size_t* removed_count, size_t* error_count);

#endif /* CN_DEAD_CODE_ELIMINATION_H */
