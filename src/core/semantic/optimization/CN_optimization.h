/**
 * @file CN_optimization.h
 * @brief 高级优化模块头文件
 
 * 本文件定义了高级优化模块的接口和数据结构，负责实现编译器优化策略，
 * 包括常量传播、死代码消除、公共子表达式消除等高级优化技术。
 * 遵循SOLID设计原则和分层架构。
 * 
 * 优化策略包括：
 * 1. 常量传播：将已知的常量值传播到使用处
 * 2. 死代码消除：移除不可达的代码分支和未使用的变量
 * 3. 公共子表达式消除：识别并重用重复的计算结果
 * 4. 循环优化：循环展开、循环不变代码外提等
 * 5. 函数内联：将小函数内联到调用处
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_OPTIMIZATION_H
#define CN_OPTIMIZATION_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_OptimizationInterface_t Stru_OptimizationInterface_t;
typedef struct Stru_ConstantPropagationInterface_t Stru_ConstantPropagationInterface_t;
typedef struct Stru_DeadCodeEliminationInterface_t Stru_DeadCodeEliminationInterface_t;
typedef struct Stru_CommonSubexpressionEliminationInterface_t Stru_CommonSubexpressionEliminationInterface_t;
typedef struct Stru_LoopOptimizationInterface_t Stru_LoopOptimizationInterface_t;
typedef struct Stru_FunctionInliningInterface_t Stru_FunctionInliningInterface_t;
typedef struct Stru_OptimizationManagerInterface_t Stru_OptimizationManagerInterface_t;
typedef struct Stru_AstNodeInterface_t Stru_AstNodeInterface_t;
typedef struct Stru_TypeSystem_t Stru_TypeSystem_t;
typedef struct Stru_SymbolTableInterface_t Stru_SymbolTableInterface_t;

// ============================================================================
// 常量传播接口
// ============================================================================

/**
 * @brief 常量传播接口
 
 * 负责将已知的常量值传播到表达式中，替换变量引用为常量值。
 */
struct Stru_ConstantPropagationInterface_t
{
    /**
     * @brief 初始化常量传播器
     * 
     * @param propagator 常量传播器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_ConstantPropagationInterface_t* propagator,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 执行常量传播
     * 
     * @param propagator 常量传播器实例
     * @param ast_node AST节点
     * @param constant_map 常量值映射
     * @return Stru_AstNodeInterface_t* 传播后的AST节点
     */
    Stru_AstNodeInterface_t* (*propagate_constants)(Stru_ConstantPropagationInterface_t* propagator,
                                                   Stru_AstNodeInterface_t* ast_node,
                                                   void* constant_map);
    
    /**
     * @brief 分析并收集常量值
     * 
     * @param propagator 常量传播器实例
     * @param ast_node AST节点
     * @param constant_map 输出参数：常量值映射
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_constants)(Stru_ConstantPropagationInterface_t* propagator,
                             Stru_AstNodeInterface_t* ast_node,
                             void** constant_map);
    
    /**
     * @brief 检查表达式是否为常量
     * 
     * @param propagator 常量传播器实例
     * @param ast_node AST表达式节点
     * @return bool 是常量表达式返回true，否则返回false
     */
    bool (*is_constant_expression)(Stru_ConstantPropagationInterface_t* propagator,
                                  const Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 获取表达式的常量值
     * 
     * @param propagator 常量传播器实例
     * @param ast_node AST表达式节点
     * @param value 输出参数：常量值
     * @param type 输出参数：常量类型
     * @return bool 成功获取常量值返回true，否则返回false
     */
    bool (*get_constant_value)(Stru_ConstantPropagationInterface_t* propagator,
                              const Stru_AstNodeInterface_t* ast_node,
                              void* value,
                              void** type);
    
    /**
     * @brief 销毁常量传播器
     * 
     * @param propagator 常量传播器实例
     */
    void (*destroy)(Stru_ConstantPropagationInterface_t* propagator);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 死代码消除接口
// ============================================================================

/**
 * @brief 死代码消除接口
 
 * 负责识别并移除不可达的代码分支和未使用的变量。
 */
struct Stru_DeadCodeEliminationInterface_t
{
    /**
     * @brief 初始化死代码消除器
     * 
     * @param eliminator 死代码消除器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_DeadCodeEliminationInterface_t* eliminator,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 执行死代码消除
     * 
     * @param eliminator 死代码消除器实例
     * @param ast_node AST节点
     * @return Stru_AstNodeInterface_t* 消除后的AST节点
     */
    Stru_AstNodeInterface_t* (*eliminate_dead_code)(Stru_DeadCodeEliminationInterface_t* eliminator,
                                                   Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 分析代码可达性
     * 
     * @param eliminator 死代码消除器实例
     * @param ast_node AST节点
     * @param reachable_map 输出参数：可达性映射
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_reachability)(Stru_DeadCodeEliminationInterface_t* eliminator,
                                Stru_AstNodeInterface_t* ast_node,
                                void** reachable_map);
    
    /**
     * @brief 分析变量使用情况
     * 
     * @param eliminator 死代码消除器实例
     * @param ast_node AST节点
     * @param used_vars 输出参数：使用的变量集合
     * @param defined_vars 输出参数：定义的变量集合
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_variable_usage)(Stru_DeadCodeEliminationInterface_t* eliminator,
                                  Stru_AstNodeInterface_t* ast_node,
                                  void** used_vars,
                                  void** defined_vars);
    
    /**
     * @brief 检查代码是否可达
     * 
     * @param eliminator 死代码消除器实例
     * @param ast_node AST节点
     * @return bool 代码可达返回true，否则返回false
     */
    bool (*is_reachable)(Stru_DeadCodeEliminationInterface_t* eliminator,
                        const Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 检查变量是否被使用
     * 
     * @param eliminator 死代码消除器实例
     * @param var_name 变量名
     * @param ast_node AST节点
     * @return bool 变量被使用返回true，否则返回false
     */
    bool (*is_variable_used)(Stru_DeadCodeEliminationInterface_t* eliminator,
                            const char* var_name,
                            const Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 销毁死代码消除器
     * 
     * @param eliminator 死代码消除器实例
     */
    void (*destroy)(Stru_DeadCodeEliminationInterface_t* eliminator);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 公共子表达式消除接口
// ============================================================================

/**
 * @brief 公共子表达式消除接口
 
 * 负责识别并重用重复的计算结果，减少冗余计算。
 */
struct Stru_CommonSubexpressionEliminationInterface_t
{
    /**
     * @brief 初始化公共子表达式消除器
     * 
     * @param eliminator 公共子表达式消除器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_CommonSubexpressionEliminationInterface_t* eliminator,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 执行公共子表达式消除
     * 
     * @param eliminator 公共子表达式消除器实例
     * @param ast_node AST节点
     * @return Stru_AstNodeInterface_t* 消除后的AST节点
     */
    Stru_AstNodeInterface_t* (*eliminate_common_subexpressions)(Stru_CommonSubexpressionEliminationInterface_t* eliminator,
                                                               Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 分析表达式等价性
     * 
     * @param eliminator 公共子表达式消除器实例
     * @param expr1 第一个表达式
     * @param expr2 第二个表达式
     * @return bool 表达式等价返回true，否则返回false
     */
    bool (*are_expressions_equivalent)(Stru_CommonSubexpressionEliminationInterface_t* eliminator,
                                      const Stru_AstNodeInterface_t* expr1,
                                      const Stru_AstNodeInterface_t* expr2);
    
    /**
     * @brief 计算表达式哈希值
     * 
     * @param eliminator 公共子表达式消除器实例
     * @param expr 表达式
     * @return uint64_t 表达式哈希值
     */
    uint64_t (*hash_expression)(Stru_CommonSubexpressionEliminationInterface_t* eliminator,
                               const Stru_AstNodeInterface_t* expr);
    
    /**
     * @brief 销毁公共子表达式消除器
     * 
     * @param eliminator 公共子表达式消除器实例
     */
    void (*destroy)(Stru_CommonSubexpressionEliminationInterface_t* eliminator);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 循环优化接口
// ============================================================================

/**
 * @brief 循环优化接口
 
 * 负责循环相关的优化，包括循环展开、循环不变代码外提等。
 */
struct Stru_LoopOptimizationInterface_t
{
    /**
     * @brief 初始化循环优化器
     * 
     * @param optimizer 循环优化器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_LoopOptimizationInterface_t* optimizer,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 执行循环展开
     * 
     * @param optimizer 循环优化器实例
     * @param loop_node 循环节点
     * @param unroll_factor 展开因子
     * @return Stru_AstNodeInterface_t* 展开后的AST节点
     */
    Stru_AstNodeInterface_t* (*unroll_loop)(Stru_LoopOptimizationInterface_t* optimizer,
                                           Stru_AstNodeInterface_t* loop_node,
                                           int unroll_factor);
    
    /**
     * @brief 执行循环不变代码外提
     * 
     * @param optimizer 循环优化器实例
     * @param loop_node 循环节点
     * @return Stru_AstNodeInterface_t* 优化后的AST节点
     */
    Stru_AstNodeInterface_t* (*hoist_invariant_code)(Stru_LoopOptimizationInterface_t* optimizer,
                                                    Stru_AstNodeInterface_t* loop_node);
    
    /**
     * @brief 分析循环不变表达式
     * 
     * @param optimizer 循环优化器实例
     * @param loop_node 循环节点
     * @param invariant_exprs 输出参数：不变表达式集合
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_invariant_expressions)(Stru_LoopOptimizationInterface_t* optimizer,
                                         Stru_AstNodeInterface_t* loop_node,
                                         void** invariant_exprs);
    
    /**
     * @brief 检查表达式是否为循环不变
     * 
     * @param optimizer 循环优化器实例
     * @param expr 表达式
     * @param loop_node 循环节点
     * @return bool 是循环不变表达式返回true，否则返回false
     */
    bool (*is_invariant_expression)(Stru_LoopOptimizationInterface_t* optimizer,
                                   const Stru_AstNodeInterface_t* expr,
                                   const Stru_AstNodeInterface_t* loop_node);
    
    /**
     * @brief 销毁循环优化器
     * 
     * @param optimizer 循环优化器实例
     */
    void (*destroy)(Stru_LoopOptimizationInterface_t* optimizer);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 函数内联接口
// ============================================================================

/**
 * @brief 函数内联接口
 
 * 负责将小函数内联到调用处，减少函数调用开销。
 */
struct Stru_FunctionInliningInterface_t
{
    /**
     * @brief 初始化函数内联器
     * 
     * @param inliner 函数内联器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_FunctionInliningInterface_t* inliner,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 执行函数内联
     * 
     * @param inliner 函数内联器实例
     * @param call_node 函数调用节点
     * @param function_node 函数定义节点
     * @return Stru_AstNodeInterface_t* 内联后的AST节点
     */
    Stru_AstNodeInterface_t* (*inline_function)(Stru_FunctionInliningInterface_t* inliner,
                                               Stru_AstNodeInterface_t* call_node,
                                               Stru_AstNodeInterface_t* function_node);
    
    /**
     * @brief 检查函数是否适合内联
     * 
     * @param inliner 函数内联器实例
     * @param function_node 函数定义节点
     * @return bool 函数适合内联返回true，否则返回false
     */
    bool (*is_function_inlinable)(Stru_FunctionInliningInterface_t* inliner,
                                 const Stru_AstNodeInterface_t* function_node);
    
    /**
     * @brief 计算函数内联成本
     * 
     * @param inliner 函数内联器实例
     * @param function_node 函数定义节点
     * @return int 内联成本（越小越适合内联）
     */
    int (*calculate_inlining_cost)(Stru_FunctionInliningInterface_t* inliner,
                                  const Stru_AstNodeInterface_t* function_node);
    
    /**
     * @brief 销毁函数内联器
     * 
     * @param inliner 函数内联器实例
     */
    void (*destroy)(Stru_FunctionInliningInterface_t* inliner);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 优化管理器接口
// ============================================================================

/**
 * @brief 优化管理器接口
 
 * 整合各种优化技术，提供统一的优化接口。
 */
typedef struct Stru_OptimizationManagerInterface_t
{
    /**
     * @brief 初始化优化管理器
     * 
     * @param manager 优化管理器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_OptimizationManagerInterface_t* manager,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 优化AST节点
     * 
     * @param manager 优化管理器实例
     * @param ast_node AST节点
     * @param optimization_level 优化级别（0-无优化，1-基本优化，2-激进优化）
     * @return Stru_AstNodeInterface_t* 优化后的AST节点
     */
    Stru_AstNodeInterface_t* (*optimize_ast)(Stru_OptimizationManagerInterface_t* manager,
                                            Stru_AstNodeInterface_t* ast_node,
                                            int optimization_level);
    
    /**
     * @brief 获取常量传播器接口
     * 
     * @param manager 优化管理器实例
     * @return Stru_ConstantPropagationInterface_t* 常量传播器接口
     */
    Stru_ConstantPropagationInterface_t* (*get_constant_propagator)(Stru_OptimizationManagerInterface_t* manager);
    
    /**
     * @brief 获取死代码消除器接口
     * 
     * @param manager 优化管理器实例
     * @return Stru_DeadCodeEliminationInterface_t* 死代码消除器接口
     */
    Stru_DeadCodeEliminationInterface_t* (*get_dead_code_eliminator)(Stru_OptimizationManagerInterface_t* manager);
    
    /**
     * @brief 获取公共子表达式消除器接口
     * 
     * @param manager 优化管理器实例
     * @return Stru_CommonSubexpressionEliminationInterface_t* 公共子表达式消除器接口
     */
    Stru_CommonSubexpressionEliminationInterface_t* (*get_common_subexpression_eliminator)(Stru_OptimizationManagerInterface_t* manager);
    
    /**
     * @brief 获取循环优化器接口
     * 
     * @param manager 优化管理器实例
     * @return Stru_LoopOptimizationInterface_t* 循环优化器接口
     */
    Stru_LoopOptimizationInterface_t* (*get_loop_optimizer)(Stru_OptimizationManagerInterface_t* manager);
    
    /**
     * @brief 获取函数内联器接口
     * 
     * @param manager 优化管理器实例
     * @return Stru_FunctionInliningInterface_t* 函数内联器接口
     */
    Stru_FunctionInliningInterface_t* (*get_function_inliner)(Stru_OptimizationManagerInterface_t* manager);
    
    /**
     * @brief 设置优化选项
     * 
     * @param manager 优化管理器实例
     * @param enable_constant_propagation 是否启用常量传播
     * @param enable_dead_code_elimination 是否启用死代码消除
     * @param enable_cse 是否启用公共子表达式消除
     * @param enable_loop_optimization 是否启用循环优化
     * @param enable_function_inlining 是否启用函数内联
     * @param optimization_level 优化级别
     */
    void (*set_optimization_options)(Stru_OptimizationManagerInterface_t* manager,
                                    bool enable_constant_propagation,
                                    bool enable_dead_code_elimination,
                                    bool enable_cse,
                                    bool enable_loop_optimization,
                                    bool enable_function_inlining,
                                    int optimization_level);
    
    /**
     * @brief 重置优化管理器状态
     * 
     * @param manager 优化管理器实例
     */
    void (*reset)(Stru_OptimizationManagerInterface_t* manager);
    
    /**
     * @brief 销毁优化管理器
     * 
     * @param manager 优化管理器实例
     */
    void (*destroy)(Stru_OptimizationManagerInterface_t* manager);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
} Stru_OptimizationManagerInterface_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建常量传播器接口实例
 * 
 * @return Stru_ConstantPropagationInterface_t* 常量传播器接口实例，失败返回NULL
 */
Stru_ConstantPropagationInterface_t* F_create_constant_propagation_interface(void);

/**
 * @brief 创建死代码消除器接口实例
 * 
 * @return Stru_DeadCodeEliminationInterface_t* 死代码消除器接口实例，失败返回NULL
 */
Stru_DeadCodeEliminationInterface_t* F_create_dead_code_elimination_interface(void);

/**
 * @brief 创建公共子表达式消除器接口实例
 * 
 * @return Stru_CommonSubexpressionEliminationInterface_t* 公共子表达式消除器接口实例，失败返回NULL
 */
Stru_CommonSubexpressionEliminationInterface_t* F_create_common_subexpression_elimination_interface(void);

/**
 * @brief 创建循环优化器接口实例
 * 
 * @return Stru_LoopOptimizationInterface_t* 循环优化器接口实例，失败返回NULL
 */
Stru_LoopOptimizationInterface_t* F_create_loop_optimization_interface(void);

/**
 * @brief 创建函数内联器接口实例
 * 
 * @return Stru_FunctionInliningInterface_t* 函数内联器接口实例，失败返回NULL
 */
Stru_FunctionInliningInterface_t* F_create_function_inlining_interface(void);

/**
 * @brief 创建优化管理器接口实例
 * 
 * @return Stru_OptimizationManagerInterface_t* 优化管理器接口实例，失败返回NULL
 */
Stru_OptimizationManagerInterface_t* F_create_optimization_manager_interface(void);

/**
 * @brief 销毁常量传播器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_constant_propagation_interface(Stru_ConstantPropagationInterface_t* interface);

/**
 * @brief 销毁死代码消除器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_dead_code_elimination_interface(Stru_DeadCodeEliminationInterface_t* interface);

/**
 * @brief 销毁公共子表达式消除器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_common_subexpression_elimination_interface(Stru_CommonSubexpressionEliminationInterface_t* interface);

/**
 * @brief 销毁循环优化器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_loop_optimization_interface(Stru_LoopOptimizationInterface_t* interface);

/**
 * @brief 销毁函数内联器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_function_inlining_interface(Stru_FunctionInliningInterface_t* interface);

/**
 * @brief 销毁优化管理器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_optimization_manager_interface(Stru_OptimizationManagerInterface_t* interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_OPTIMIZATION_H */
