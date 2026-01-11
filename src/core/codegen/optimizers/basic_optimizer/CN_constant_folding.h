/******************************************************************************
 * 文件名: CN_constant_folding.h
 * 功能: CN_Language 常量折叠优化算法头文件
 * 
 * 定义常量折叠优化算法的接口和数据结构。
 * 常量折叠是在编译时计算常量表达式的优化技术。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月10日
 * 修改历史:
 *   - 2026年1月10日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_CONSTANT_FOLDING_H
#define CN_CONSTANT_FOLDING_H

#include "../../../ast/CN_ast.h"
#include "../../CN_optimizer_interface.h"
#include "CN_basic_optimizer.h"

// ============================================================================
// 常量折叠接口函数
// ============================================================================

/**
 * @brief 应用常量折叠优化
 * 
 * 对AST应用常量折叠优化，在编译时计算常量表达式。
 * 
 * @param ast AST根节点
 * @param context 优化上下文
 * @return bool 优化是否成功应用
 */
bool apply_constant_folding(Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);

/**
 * @brief 测试常量折叠算法
 * 
 * 运行常量折叠算法的测试用例。
 */
void test_constant_folding(void);

/**
 * @brief 检查表达式是否为常量表达式
 * 
 * 检查给定的AST节点是否表示常量表达式。
 * 
 * @param node AST节点
 * @return bool 如果是常量表达式返回true，否则返回false
 */
bool is_constant_expression(Stru_AstNode_t* node);

/**
 * @brief 评估常量表达式
 * 
 * 评估常量表达式并返回其值。
 * 
 * @param node 常量表达式AST节点
 * @param context 优化上下文
 * @return Stru_AstNode_t* 表示常量值的AST节点，失败返回NULL
 */
Stru_AstNode_t* evaluate_constant_expression(Stru_AstNode_t* node, Stru_OptimizationContext_t* context);

/**
 * @brief 折叠二元表达式
 * 
 * 折叠二元常量表达式。
 * 
 * @param node 二元表达式AST节点
 * @param context 优化上下文
 * @return Stru_AstNode_t* 折叠后的AST节点，失败返回NULL
 */
Stru_AstNode_t* fold_binary_expression(Stru_AstNode_t* node, Stru_OptimizationContext_t* context);

/**
 * @brief 折叠一元表达式
 * 
 * 折叠一元常量表达式。
 * 
 * @param node 一元表达式AST节点
 * @param context 优化上下文
 * @return Stru_AstNode_t* 折叠后的AST节点，失败返回NULL
 */
Stru_AstNode_t* fold_unary_expression(Stru_AstNode_t* node, Stru_OptimizationContext_t* context);

/**
 * @brief 获取常量折叠统计信息
 * 
 * 获取常量折叠优化的统计信息。
 * 
 * @param context 优化上下文
 * @param folded_count 输出参数，已折叠的常量数量
 * @param error_count 输出参数，错误数量
 */
void get_constant_folding_stats(Stru_OptimizationContext_t* context, 
                                size_t* folded_count, size_t* error_count);

#endif /* CN_CONSTANT_FOLDING_H */
