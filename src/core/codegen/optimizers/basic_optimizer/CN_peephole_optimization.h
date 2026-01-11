/******************************************************************************
 * 文件名: CN_peephole_optimization.h
 * 功能: CN_Language 窥孔优化算法头文件
 * 
 * 定义窥孔优化算法的接口和数据结构。
 * 窥孔优化是一种局部优化技术，检查一小段代码并用更高效的代码替换它。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_PEEPHOLE_OPTIMIZATION_H
#define CN_PEEPHOLE_OPTIMIZATION_H

#include "../../../ast/CN_ast.h"
#include "../../CN_optimizer_interface.h"
#include "CN_basic_optimizer.h"

// ============================================================================
// 常量定义
// ============================================================================

/**
 * @brief 最大窥孔模式数量
 */
#define PEEPHOLE_MAX_PATTERNS 64

/**
 * @brief 最大窗口大小（连续指令数）
 */
#define PEEPHOLE_WINDOW_SIZE 5

/**
 * @brief 最大模式长度
 */
#define PEEPHOLE_MAX_PATTERN_LENGTH 3

// ============================================================================
// 数据结构定义
// ============================================================================

/**
 * @brief 窥孔模式类型枚举
 */
typedef enum Eum_PeepholePatternType {
    Eum_PH_REDUNDANT_LOAD_STORE,        ///< 冗余加载/存储
    Eum_PH_REDUNDANT_MOVE,              ///< 冗余移动
    Eum_PH_DEAD_STORE,                  ///< 死存储
    Eum_PH_ALGEBRAIC_IDENTITY,          ///< 代数恒等式
    Eum_PH_CONSTANT_PROPAGATION,        ///< 常量传播
    Eum_PH_BRANCH_OPTIMIZATION,         ///< 分支优化
    Eum_PH_COMPARE_OPTIMIZATION,        ///< 比较优化
    Eum_PH_JUMP_OPTIMIZATION,           ///< 跳转优化
    Eum_PH_STACK_OPTIMIZATION,          ///< 栈优化
    Eum_PH_REGISTER_OPTIMIZATION        ///< 寄存器优化
} Eum_PeepholePatternType;

/**
 * @brief 窥孔模式结构体
 */
typedef struct Stru_PeepholePattern_t {
    Eum_PeepholePatternType type;       ///< 模式类型
    const char* description;            ///< 模式描述
    bool (*match)(Stru_AstNode_t** window, size_t window_size); ///< 匹配函数
    bool (*apply)(Stru_AstNode_t** window, size_t window_size, 
                  Stru_OptimizationContext_t* context); ///< 应用函数
    bool is_safe;                       ///< 是否安全（不改变语义）
    int cost_reduction;                 ///< 成本减少（估计值）
    size_t pattern_length;              ///< 模式长度（指令数）
} Stru_PeepholePattern_t;

/**
 * @brief 窥孔优化上下文结构体
 */
typedef struct Stru_PeepholeContext_t {
    Stru_AstNode_t* ast;                ///< AST根节点
    Stru_OptimizationContext_t* opt_context; ///< 优化上下文
    Stru_PeepholePattern_t* patterns;   ///< 模式数组
    size_t pattern_count;               ///< 模式数量
    size_t applied_count;               ///< 已应用的优化数量
    size_t error_count;                 ///< 错误数量
    char** errors;                      ///< 错误信息数组
    size_t error_capacity;              ///< 错误数组容量
    int total_cost_reduction;           ///< 总成本减少
    size_t window_size;                 ///< 当前窗口大小
} Stru_PeepholeContext_t;

// ============================================================================
// 窥孔优化接口函数
// ============================================================================

/**
 * @brief 应用窥孔优化
 * 
 * 对AST应用窥孔优化，用更高效的代码替换局部代码模式。
 * 
 * @param ast AST根节点
 * @param context 优化上下文
 * @return bool 优化是否成功应用
 */
bool apply_peephole_optimization(Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);

/**
 * @brief 测试窥孔优化算法
 * 
 * 运行窥孔优化算法的测试用例。
 */
void test_peephole_optimization(void);

/**
 * @brief 注册窥孔模式
 * 
 * 注册一个新的窥孔优化模式。
 * 
 * @param context 窥孔优化上下文
 * @param pattern 要注册的模式
 * @return bool 注册是否成功
 */
bool register_peephole_pattern(Stru_PeepholeContext_t* context, Stru_PeepholePattern_t pattern);

/**
 * @brief 检查代码窗口是否可优化
 * 
 * 检查代码窗口是否可以通过窥孔优化进行优化。
 * 
 * @param window 代码窗口（AST节点数组）
 * @param window_size 窗口大小
 * @param context 优化上下文
 * @return bool 如果可优化返回true，否则返回false
 */
bool is_window_optimizable(Stru_AstNode_t** window, size_t window_size, Stru_OptimizationContext_t* context);

/**
 * @brief 应用窥孔优化到代码窗口
 * 
 * 对代码窗口应用窥孔优化。
 * 
 * @param window 代码窗口（AST节点数组）
 * @param window_size 窗口大小
 * @param context 优化上下文
 * @return bool 优化是否成功应用
 */
bool apply_peephole_to_window(Stru_AstNode_t** window, size_t window_size, Stru_OptimizationContext_t* context);

/**
 * @brief 获取窥孔优化统计信息
 * 
 * 获取窥孔优化的统计信息。
 * 
 * @param context 优化上下文
 * @param applied_count 输出参数，已应用的优化数量
 * @param cost_reduction 输出参数，总成本减少
 * @param error_count 输出参数，错误数量
 */
void get_peephole_stats(Stru_OptimizationContext_t* context, 
                        size_t* applied_count, int* cost_reduction, size_t* error_count);

// ============================================================================
// 预定义模式匹配函数
// ============================================================================

/**
 * @brief 检查是否为冗余加载/存储模式
 */
bool match_redundant_load_store(Stru_AstNode_t** window, size_t window_size);

/**
 * @brief 检查是否为冗余移动模式
 */
bool match_redundant_move(Stru_AstNode_t** window, size_t window_size);

/**
 * @brief 检查是否为死存储模式
 */
bool match_dead_store(Stru_AstNode_t** window, size_t window_size);

/**
 * @brief 检查是否为代数恒等式模式
 */
bool match_algebraic_identity(Stru_AstNode_t** window, size_t window_size);

/**
 * @brief 检查是否为常量传播模式
 */
bool match_constant_propagation(Stru_AstNode_t** window, size_t window_size);

/**
 * @brief 检查是否为分支优化模式
 */
bool match_branch_optimization(Stru_AstNode_t** window, size_t window_size);

/**
 * @brief 检查是否为比较优化模式
 */
bool match_compare_optimization(Stru_AstNode_t** window, size_t window_size);

/**
 * @brief 检查是否为跳转优化模式
 */
bool match_jump_optimization(Stru_AstNode_t** window, size_t window_size);

// ============================================================================
// 预定义模式应用函数
// ============================================================================

/**
 * @brief 应用冗余加载/存储优化
 */
bool apply_redundant_load_store(Stru_AstNode_t** window, size_t window_size, Stru_OptimizationContext_t* context);

/**
 * @brief 应用冗余移动优化
 */
bool apply_redundant_move(Stru_AstNode_t** window, size_t window_size, Stru_OptimizationContext_t* context);

/**
 * @brief 应用死存储优化
 */
bool apply_dead_store(Stru_AstNode_t** window, size_t window_size, Stru_OptimizationContext_t* context);

/**
 * @brief 应用代数恒等式优化
 */
bool apply_algebraic_identity(Stru_AstNode_t** window, size_t window_size, Stru_OptimizationContext_t* context);

/**
 * @brief 应用常量传播优化
 */
bool apply_constant_propagation(Stru_AstNode_t** window, size_t window_size, Stru_OptimizationContext_t* context);

/**
 * @brief 应用分支优化
 */
bool apply_branch_optimization(Stru_AstNode_t** window, size_t window_size, Stru_OptimizationContext_t* context);

/**
 * @brief 应用比较优化
 */
bool apply_compare_optimization(Stru_AstNode_t** window, size_t window_size, Stru_OptimizationContext_t* context);

/**
 * @brief 应用跳转优化
 */
bool apply_jump_optimization(Stru_AstNode_t** window, size_t window_size, Stru_OptimizationContext_t* context);

#endif /* CN_PEEPHOLE_OPTIMIZATION_H */
