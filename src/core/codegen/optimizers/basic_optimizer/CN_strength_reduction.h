/******************************************************************************
 * 文件名: CN_strength_reduction.h
 * 功能: CN_Language 强度削减优化算法头文件
 * 
 * 定义强度削减优化算法的接口和数据结构。
 * 强度削减是用更高效的运算替换昂贵运算的优化技术。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_STRENGTH_REDUCTION_H
#define CN_STRENGTH_REDUCTION_H

#include "../../../ast/CN_ast.h"
#include "../../CN_optimizer_interface.h"
#include "CN_basic_optimizer.h"

// ============================================================================
// 常量定义
// ============================================================================

/**
 * @brief 最大模式数量
 */
#define SR_MAX_PATTERNS 32

/**
 * @brief 最大表达式长度
 */
#define SR_MAX_EXPR_LENGTH 128

// ============================================================================
// 数据结构定义
// ============================================================================

/**
 * @brief 强度削减模式类型枚举
 */
typedef enum Eum_StrengthReductionPattern {
    Eum_SR_MULTIPLY_BY_POWER_OF_TWO,     ///< 乘以2的幂 -> 左移
    Eum_SR_DIVIDE_BY_POWER_OF_TWO,       ///< 除以2的幂 -> 右移
    Eum_SR_MULTIPLY_BY_CONSTANT,         ///< 乘以常量 -> 移位和加法组合
    Eum_SR_DIVIDE_BY_CONSTANT,           ///< 除以常量 -> 乘法和移位
    Eum_SR_MODULO_POWER_OF_TWO,          ///< 模2的幂 -> 位与
    Eum_SR_MULTIPLY_BY_ZERO,             ///< 乘以0 -> 0
    Eum_SR_MULTIPLY_BY_ONE,              ///< 乘以1 -> 原值
    Eum_SR_ADD_ZERO,                     ///< 加0 -> 原值
    Eum_SR_SUBTRACT_ZERO,                ///< 减0 -> 原值
    Eum_SR_POWER_OF_TWO_MULTIPLY,        ///< 2的幂乘法优化
    Eum_SR_POWER_OF_TWO_DIVIDE,          ///< 2的幂除法优化
    Eum_SR_COMPARE_WITH_CONSTANT,        ///< 与常量比较优化
    Eum_SR_BOOLEAN_IDENTITY,             ///< 布尔恒等优化
    Eum_SR_ARITHMETIC_IDENTITY           ///< 算术恒等优化
} Eum_StrengthReductionPattern;

/**
 * @brief 强度削减模式结构体
 */
typedef struct Stru_SrPattern_t {
    Eum_StrengthReductionPattern type;   ///< 模式类型
    const char* description;             ///< 模式描述
    bool (*match)(Stru_AstNode_t* node); ///< 匹配函数
    Stru_AstNode_t* (*apply)(Stru_AstNode_t* node); ///< 应用函数
    bool is_safe;                        ///< 是否安全（不改变语义）
    int cost_reduction;                  ///< 成本减少（估计值）
} Stru_SrPattern_t;

/**
 * @brief 强度削减上下文结构体
 */
typedef struct Stru_SrContext_t {
    Stru_AstNode_t* ast;                 ///< AST根节点
    Stru_OptimizationContext_t* opt_context; ///< 优化上下文
    Stru_SrPattern_t* patterns;          ///< 模式数组
    size_t pattern_count;                ///< 模式数量
    size_t applied_count;                ///< 已应用的优化数量
    size_t error_count;                  ///< 错误数量
    char** errors;                       ///< 错误信息数组
    size_t error_capacity;               ///< 错误数组容量
    int total_cost_reduction;            ///< 总成本减少
} Stru_SrContext_t;

// ============================================================================
// 强度削减接口函数
// ============================================================================

/**
 * @brief 应用强度削减优化
 * 
 * 对AST应用强度削减优化，用更高效的运算替换昂贵的运算。
 * 
 * @param ast AST根节点
 * @param context 优化上下文
 * @return bool 优化是否成功应用
 */
bool apply_strength_reduction(Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);

/**
 * @brief 测试强度削减算法
 * 
 * 运行强度削减算法的测试用例。
 */
void test_strength_reduction(void);

/**
 * @brief 注册强度削减模式
 * 
 * 注册一个新的强度削减模式。
 * 
 * @param context 强度削减上下文
 * @param pattern 要注册的模式
 * @return bool 注册是否成功
 */
bool register_strength_reduction_pattern(Stru_SrContext_t* context, Stru_SrPattern_t pattern);

/**
 * @brief 检查表达式是否可优化
 * 
 * 检查表达式是否可以通过强度削减进行优化。
 * 
 * @param node 表达式AST节点
 * @param context 优化上下文
 * @return bool 如果可优化返回true，否则返回false
 */
bool is_expression_reducible(Stru_AstNode_t* node, Stru_OptimizationContext_t* context);

/**
 * @brief 应用强度削减到表达式
 * 
 * 对表达式应用强度削减优化。
 * 
 * @param node 表达式AST节点
 * @param context 优化上下文
 * @return Stru_AstNode_t* 优化后的表达式，如果未优化返回原节点
 */
Stru_AstNode_t* apply_strength_reduction_to_expression(Stru_AstNode_t* node, Stru_OptimizationContext_t* context);

/**
 * @brief 获取强度削减统计信息
 * 
 * 获取强度削减优化的统计信息。
 * 
 * @param context 优化上下文
 * @param applied_count 输出参数，已应用的优化数量
 * @param cost_reduction 输出参数，总成本减少
 * @param error_count 输出参数，错误数量
 */
void get_strength_reduction_stats(Stru_OptimizationContext_t* context, 
                                  size_t* applied_count, int* cost_reduction, size_t* error_count);

// ============================================================================
// 预定义模式匹配函数
// ============================================================================

/**
 * @brief 检查是否为乘以2的幂
 */
bool match_multiply_by_power_of_two(Stru_AstNode_t* node);

/**
 * @brief 检查是否为除以2的幂
 */
bool match_divide_by_power_of_two(Stru_AstNode_t* node);

/**
 * @brief 检查是否为乘以常量
 */
bool match_multiply_by_constant(Stru_AstNode_t* node);

/**
 * @brief 检查是否为除以常量
 */
bool match_divide_by_constant(Stru_AstNode_t* node);

/**
 * @brief 检查是否为模2的幂
 */
bool match_modulo_power_of_two(Stru_AstNode_t* node);

/**
 * @brief 检查是否为乘以0
 */
bool match_multiply_by_zero(Stru_AstNode_t* node);

/**
 * @brief 检查是否为乘以1
 */
bool match_multiply_by_one(Stru_AstNode_t* node);

/**
 * @brief 检查是否为加0
 */
bool match_add_zero(Stru_AstNode_t* node);

/**
 * @brief 检查是否为减0
 */
bool match_subtract_zero(Stru_AstNode_t* node);

// ============================================================================
// 预定义模式应用函数
// ============================================================================

/**
 * @brief 应用乘以2的幂优化
 */
Stru_AstNode_t* apply_multiply_by_power_of_two(Stru_AstNode_t* node);

/**
 * @brief 应用除以2的幂优化
 */
Stru_AstNode_t* apply_divide_by_power_of_two(Stru_AstNode_t* node);

/**
 * @brief 应用乘以常量优化
 */
Stru_AstNode_t* apply_multiply_by_constant(Stru_AstNode_t* node);

/**
 * @brief 应用除以常量优化
 */
Stru_AstNode_t* apply_divide_by_constant(Stru_AstNode_t* node);

/**
 * @brief 应用模2的幂优化
 */
Stru_AstNode_t* apply_modulo_power_of_two(Stru_AstNode_t* node);

/**
 * @brief 应用乘以0优化
 */
Stru_AstNode_t* apply_multiply_by_zero(Stru_AstNode_t* node);

/**
 * @brief 应用乘以1优化
 */
Stru_AstNode_t* apply_multiply_by_one(Stru_AstNode_t* node);

/**
 * @brief 应用加0优化
 */
Stru_AstNode_t* apply_add_zero(Stru_AstNode_t* node);

/**
 * @brief 应用减0优化
 */
Stru_AstNode_t* apply_subtract_zero(Stru_AstNode_t* node);

#endif /* CN_STRENGTH_REDUCTION_H */
