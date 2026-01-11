/******************************************************************************
 * 文件名: CN_common_subexpr.h
 * 功能: CN_Language 公共子表达式消除优化算法头文件
 * 
 * 定义公共子表达式消除优化算法的接口和数据结构。
 * 公共子表达式消除是识别并消除重复表达式计算的优化技术。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_COMMON_SUBEXPR_H
#define CN_COMMON_SUBEXPR_H

#include "../../../ast/CN_ast.h"
#include "../../CN_optimizer_interface.h"
#include "CN_basic_optimizer.h"

// ============================================================================
// 常量定义
// ============================================================================

/**
 * @brief 表达式哈希表最大容量
 */
#define CSE_MAX_HASH_TABLE_SIZE 1024

/**
 * @brief 最大表达式长度（用于哈希计算）
 */
#define CSE_MAX_EXPR_LENGTH 256

// ============================================================================
// 数据结构定义
// ============================================================================

/**
 * @brief 表达式哈希条目结构体
 */
typedef struct Stru_CseHashEntry_t {
    char* expression_hash;           ///< 表达式哈希值
    Stru_AstNode_t* expression;      ///< 表达式AST节点
    char* temp_var_name;             ///< 临时变量名
    size_t use_count;                ///< 使用次数
    struct Stru_CseHashEntry_t* next; ///< 下一个条目（用于链式哈希）
} Stru_CseHashEntry_t;

/**
 * @brief 表达式哈希表结构体
 */
typedef struct Stru_CseHashTable_t {
    Stru_CseHashEntry_t** buckets;   ///< 哈希桶数组
    size_t bucket_count;             ///< 桶数量
    size_t entry_count;              ///< 条目数量
} Stru_CseHashTable_t;

/**
 * @brief 公共子表达式消除上下文结构体
 */
typedef struct Stru_CseContext_t {
    Stru_AstNode_t* ast;             ///< AST根节点
    Stru_OptimizationContext_t* opt_context; ///< 优化上下文
    Stru_CseHashTable_t* hash_table; ///< 表达式哈希表
    size_t eliminated_count;         ///< 已消除的公共子表达式数量
    size_t temp_var_counter;         ///< 临时变量计数器
    char** errors;                   ///< 错误信息数组
    size_t error_count;              ///< 错误数量
    size_t error_capacity;           ///< 错误数组容量
} Stru_CseContext_t;

// ============================================================================
// 公共子表达式消除接口函数
// ============================================================================

/**
 * @brief 应用公共子表达式消除优化
 * 
 * 对AST应用公共子表达式消除优化，消除重复的表达式计算。
 * 
 * @param ast AST根节点
 * @param context 优化上下文
 * @return bool 优化是否成功应用
 */
bool apply_common_subexpression_elimination(Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);

/**
 * @brief 测试公共子表达式消除算法
 * 
 * 运行公共子表达式消除算法的测试用例。
 */
void test_common_subexpression_elimination(void);

/**
 * @brief 计算表达式哈希值
 * 
 * 计算AST表达式的哈希值，用于快速比较表达式是否相同。
 * 
 * @param node 表达式AST节点
 * @return char* 表达式哈希字符串，调用者负责释放
 */
char* compute_expression_hash(Stru_AstNode_t* node);

/**
 * @brief 检查表达式是否等价
 * 
 * 检查两个表达式AST节点是否等价（具有相同的结构和值）。
 * 
 * @param expr1 第一个表达式
 * @param expr2 第二个表达式
 * @return bool 如果表达式等价返回true，否则返回false
 */
bool are_expressions_equivalent(Stru_AstNode_t* expr1, Stru_AstNode_t* expr2);

/**
 * @brief 创建临时变量
 * 
 * 为公共子表达式创建临时变量。
 * 
 * @param context 公共子表达式消除上下文
 * @param expression 原始表达式
 * @return char* 临时变量名，调用者不应修改
 */
const char* create_temp_variable(Stru_CseContext_t* context, Stru_AstNode_t* expression);

/**
 * @brief 替换公共子表达式
 * 
 * 用临时变量替换公共子表达式。
 * 
 * @param context 公共子表达式消除上下文
 * @param node 要替换的表达式节点
 * @param temp_var_name 临时变量名
 * @return bool 替换是否成功
 */
bool replace_common_subexpression(Stru_CseContext_t* context, 
                                  Stru_AstNode_t* node, 
                                  const char* temp_var_name);

/**
 * @brief 获取公共子表达式消除统计信息
 * 
 * 获取公共子表达式消除优化的统计信息。
 * 
 * @param context 优化上下文
 * @param eliminated_count 输出参数，已消除的公共子表达式数量
 * @param error_count 输出参数，错误数量
 */
void get_common_subexpression_stats(Stru_OptimizationContext_t* context, 
                                    size_t* eliminated_count, size_t* error_count);

// ============================================================================
// 哈希表管理函数
// ============================================================================

/**
 * @brief 创建表达式哈希表
 * 
 * 创建并初始化表达式哈希表。
 * 
 * @param bucket_count 哈希桶数量
 * @return Stru_CseHashTable_t* 新创建的哈希表，调用者负责销毁
 */
Stru_CseHashTable_t* create_cse_hash_table(size_t bucket_count);

/**
 * @brief 销毁表达式哈希表
 * 
 * 销毁表达式哈希表及其所有条目。
 * 
 * @param table 要销毁的哈希表
 */
void destroy_cse_hash_table(Stru_CseHashTable_t* table);

/**
 * @brief 插入表达式到哈希表
 * 
 * 将表达式插入哈希表，如果已存在则增加使用计数。
 * 
 * @param table 哈希表
 * @param expression 表达式AST节点
 * @param hash 表达式哈希值
 * @return Stru_CseHashEntry_t* 哈希表条目，如果插入失败返回NULL
 */
Stru_CseHashEntry_t* insert_expression(Stru_CseHashTable_t* table, 
                                       Stru_AstNode_t* expression, 
                                       const char* hash);

/**
 * @brief 查找表达式在哈希表中
 * 
 * 在哈希表中查找表达式。
 * 
 * @param table 哈希表
 * @param hash 表达式哈希值
 * @return Stru_CseHashEntry_t* 找到的哈希表条目，如果未找到返回NULL
 */
Stru_CseHashEntry_t* find_expression(Stru_CseHashTable_t* table, const char* hash);

/**
 * @brief 从哈希表中移除表达式
 * 
 * 从哈希表中移除表达式。
 * 
 * @param table 哈希表
 * @param hash 表达式哈希值
 * @return bool 如果成功移除返回true，否则返回false
 */
bool remove_expression(Stru_CseHashTable_t* table, const char* hash);

#endif /* CN_COMMON_SUBEXPR_H */
