/**
 * @file CN_constant_folding.h
 * @brief 常量折叠和表达式简化模块头文件
 
 * 本文件定义了常量折叠和表达式简化模块的接口和数据结构，负责在编译时
 * 计算常量表达式、简化表达式结构、优化AST。遵循SOLID设计原则和分层架构。
 * 
 * 常量折叠功能包括：
 * 1. 算术运算的常量折叠（+、-、*、/、%等）
 * 2. 逻辑运算的常量折叠（&&、||、!等）
 * 3. 比较运算的常量折叠（==、!=、<、>、<=、>=等）
 * 4. 位运算的常量折叠（&、|、^、~、<<、>>等）
 * 5. 条件表达式的常量折叠（?:）
 * 
 * 表达式简化功能包括：
 * 1. 代数恒等式简化（x+0 → x, x*1 → x等）
 * 2. 逻辑表达式简化（true && x → x, false || x → x等）
 * 3. 冗余操作消除（!!x → x, (x) → x等）
 * 4. 常量传播（将常量值传播到使用处）
 * 5. 死代码消除（移除不可达的代码分支）
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_CONSTANT_FOLDING_H
#define CN_CONSTANT_FOLDING_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_ConstantFoldingInterface_t Stru_ConstantFoldingInterface_t;
typedef struct Stru_ExpressionSimplifierInterface_t Stru_ExpressionSimplifierInterface_t;
typedef struct Stru_ConstantFoldingManagerInterface_t Stru_ConstantFoldingManagerInterface_t;
typedef struct Stru_AstNodeInterface_t Stru_AstNodeInterface_t;
typedef struct Stru_TypeDescriptor_t Stru_TypeDescriptor_t;
typedef struct Stru_TypeSystem_t Stru_TypeSystem_t;

// ============================================================================
// 常量折叠接口
// ============================================================================

/**
 * @brief 常量折叠接口
 
 * 负责在编译时计算常量表达式，将常量表达式替换为计算结果。
 */
struct Stru_ConstantFoldingInterface_t
{
    /**
     * @brief 初始化常量折叠器
     * 
     * @param folder 常量折叠器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_ConstantFoldingInterface_t* folder,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 折叠常量表达式
     * 
     * @param folder 常量折叠器实例
     * @param ast_node AST表达式节点
     * @return Stru_AstNodeInterface_t* 折叠后的AST节点，如果无法折叠则返回原节点
     */
    Stru_AstNodeInterface_t* (*fold_constant_expression)(Stru_ConstantFoldingInterface_t* folder,
                                                        Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 检查表达式是否为常量
     * 
     * @param folder 常量折叠器实例
     * @param ast_node AST表达式节点
     * @return bool 是常量表达式返回true，否则返回false
     */
    bool (*is_constant_expression)(Stru_ConstantFoldingInterface_t* folder,
                                  const Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 尝试获取常量表达式的值
     * 
     * @param folder 常量折叠器实例
     * @param ast_node AST表达式节点
     * @param value 输出参数：常量值（根据类型不同使用不同的联合成员）
     * @param type 输出参数：常量类型
     * @return bool 成功获取常量值返回true，否则返回false
     */
    bool (*try_get_constant_value)(Stru_ConstantFoldingInterface_t* folder,
                                  const Stru_AstNodeInterface_t* ast_node,
                                  void* value,
                                  Stru_TypeDescriptor_t** type);
    
    /**
     * @brief 折叠二元运算表达式
     * 
     * @param folder 常量折叠器实例
     * @param operator_type 运算符类型
     * @param left 左操作数节点
     * @param right 右操作数节点
     * @return Stru_AstNodeInterface_t* 折叠后的AST节点
     */
    Stru_AstNodeInterface_t* (*fold_binary_expression)(Stru_ConstantFoldingInterface_t* folder,
                                                      int operator_type,
                                                      Stru_AstNodeInterface_t* left,
                                                      Stru_AstNodeInterface_t* right);
    
    /**
     * @brief 折叠一元运算表达式
     * 
     * @param folder 常量折叠器实例
     * @param operator_type 运算符类型
     * @param operand 操作数节点
     * @return Stru_AstNodeInterface_t* 折叠后的AST节点
     */
    Stru_AstNodeInterface_t* (*fold_unary_expression)(Stru_ConstantFoldingInterface_t* folder,
                                                     int operator_type,
                                                     Stru_AstNodeInterface_t* operand);
    
    /**
     * @brief 折叠条件表达式
     * 
     * @param folder 常量折叠器实例
     * @param condition 条件表达式节点
     * @param then_expr then表达式节点
     * @param else_expr else表达式节点
     * @return Stru_AstNodeInterface_t* 折叠后的AST节点
     */
    Stru_AstNodeInterface_t* (*fold_conditional_expression)(Stru_ConstantFoldingInterface_t* folder,
                                                           Stru_AstNodeInterface_t* condition,
                                                           Stru_AstNodeInterface_t* then_expr,
                                                           Stru_AstNodeInterface_t* else_expr);
    
    /**
     * @brief 折叠函数调用表达式
     * 
     * @param folder 常量折叠器实例
     * @param function 函数表达式节点
     * @param arguments 参数节点数组
     * @param argument_count 参数数量
     * @return Stru_AstNodeInterface_t* 折叠后的AST节点
     */
    Stru_AstNodeInterface_t* (*fold_call_expression)(Stru_ConstantFoldingInterface_t* folder,
                                                    Stru_AstNodeInterface_t* function,
                                                    Stru_AstNodeInterface_t** arguments,
                                                    size_t argument_count);
    
    /**
     * @brief 设置是否启用激进折叠
     * 
     * @param folder 常量折叠器实例
     * @param aggressive 是否启用激进折叠（可能影响浮点数精度）
     */
    void (*set_aggressive_folding)(Stru_ConstantFoldingInterface_t* folder,
                                  bool aggressive);
    
    /**
     * @brief 重置常量折叠器状态
     * 
     * @param folder 常量折叠器实例
     */
    void (*reset)(Stru_ConstantFoldingInterface_t* folder);
    
    /**
     * @brief 销毁常量折叠器
     * 
     * @param folder 常量折叠器实例
     */
    void (*destroy)(Stru_ConstantFoldingInterface_t* folder);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 表达式简化接口
// ============================================================================

/**
 * @brief 表达式简化接口
 
 * 负责简化表达式结构，应用代数恒等式和逻辑简化规则。
 */
struct Stru_ExpressionSimplifierInterface_t
{
    /**
     * @brief 初始化表达式简化器
     * 
     * @param simplifier 表达式简化器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_ExpressionSimplifierInterface_t* simplifier,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 简化表达式
     * 
     * @param simplifier 表达式简化器实例
     * @param ast_node AST表达式节点
     * @return Stru_AstNodeInterface_t* 简化后的AST节点
     */
    Stru_AstNodeInterface_t* (*simplify_expression)(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                   Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 应用代数恒等式简化
     * 
     * @param simplifier 表达式简化器实例
     * @param ast_node AST表达式节点
     * @return Stru_AstNodeInterface_t* 简化后的AST节点
     */
    Stru_AstNodeInterface_t* (*apply_algebraic_identities)(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                          Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 应用逻辑简化规则
     * 
     * @param simplifier 表达式简化器实例
     * @param ast_node AST表达式节点
     * @return Stru_AstNodeInterface_t* 简化后的AST节点
     */
    Stru_AstNodeInterface_t* (*apply_logical_simplifications)(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                             Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 消除冗余操作
     * 
     * @param simplifier 表达式简化器实例
     * @param ast_node AST表达式节点
     * @return Stru_AstNodeInterface_t* 简化后的AST节点
     */
    Stru_AstNodeInterface_t* (*eliminate_redundant_operations)(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                              Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 执行常量传播
     * 
     * @param simplifier 表达式简化器实例
     * @param ast_node AST表达式节点
     * @param constant_values 常量值映射
     * @return Stru_AstNodeInterface_t* 传播后的AST节点
     */
    Stru_AstNodeInterface_t* (*propagate_constants)(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                   Stru_AstNodeInterface_t* ast_node,
                                                   void* constant_values);
    
    /**
     * @brief 消除死代码
     * 
     * @param simplifier 表达式简化器实例
     * @param ast_node AST语句节点
     * @return Stru_AstNodeInterface_t* 消除死代码后的AST节点
     */
    Stru_AstNodeInterface_t* (*eliminate_dead_code)(Stru_ExpressionSimplifierInterface_t* simplifier,
                                                   Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 设置简化级别
     * 
     * @param simplifier 表达式简化器实例
     * @param level 简化级别（0-基本，1-中等，2-激进）
     */
    void (*set_simplification_level)(Stru_ExpressionSimplifierInterface_t* simplifier,
                                    int level);
    
    /**
     * @brief 重置表达式简化器状态
     * 
     * @param simplifier 表达式简化器实例
     */
    void (*reset)(Stru_ExpressionSimplifierInterface_t* simplifier);
    
    /**
     * @brief 销毁表达式简化器
     * 
     * @param simplifier 表达式简化器实例
     */
    void (*destroy)(Stru_ExpressionSimplifierInterface_t* simplifier);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 常量折叠和表达式简化管理器接口
// ============================================================================

/**
 * @brief 常量折叠和表达式简化管理器接口
 
 * 整合常量折叠和表达式简化功能，提供统一的优化接口。
 */
typedef struct Stru_ConstantFoldingManagerInterface_t
{
    /**
     * @brief 初始化管理器
     * 
     * @param manager 管理器实例
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_ConstantFoldingManagerInterface_t* manager,
                      Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 优化AST节点
     * 
     * @param manager 管理器实例
     * @param ast_node AST节点
     * @return Stru_AstNodeInterface_t* 优化后的AST节点
     */
    Stru_AstNodeInterface_t* (*optimize_ast)(Stru_ConstantFoldingManagerInterface_t* manager,
                                            Stru_AstNodeInterface_t* ast_node);
    
    /**
     * @brief 获取常量折叠器接口
     * 
     * @param manager 管理器实例
     * @return Stru_ConstantFoldingInterface_t* 常量折叠器接口
     */
    Stru_ConstantFoldingInterface_t* (*get_constant_folder)(Stru_ConstantFoldingManagerInterface_t* manager);
    
    /**
     * @brief 获取表达式简化器接口
     * 
     * @param manager 管理器实例
     * @return Stru_ExpressionSimplifierInterface_t* 表达式简化器接口
     */
    Stru_ExpressionSimplifierInterface_t* (*get_expression_simplifier)(Stru_ConstantFoldingManagerInterface_t* manager);
    
    /**
     * @brief 设置优化选项
     * 
     * @param manager 管理器实例
     * @param enable_folding 是否启用常量折叠
     * @param enable_simplification 是否启用表达式简化
     * @param simplification_level 简化级别
     */
    void (*set_optimization_options)(Stru_ConstantFoldingManagerInterface_t* manager,
                                    bool enable_folding,
                                    bool enable_simplification,
                                    int simplification_level);
    
    /**
     * @brief 重置管理器状态
     * 
     * @param manager 管理器实例
     */
    void (*reset)(Stru_ConstantFoldingManagerInterface_t* manager);
    
    /**
     * @brief 销毁管理器
     * 
     * @param manager 管理器实例
     */
    void (*destroy)(Stru_ConstantFoldingManagerInterface_t* manager);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
} Stru_ConstantFoldingManagerInterface_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建常量折叠器接口实例
 * 
 * @return Stru_ConstantFoldingInterface_t* 常量折叠器接口实例，失败返回NULL
 */
Stru_ConstantFoldingInterface_t* F_create_constant_folding_interface(void);

/**
 * @brief 创建表达式简化器接口实例
 * 
 * @return Stru_ExpressionSimplifierInterface_t* 表达式简化器接口实例，失败返回NULL
 */
Stru_ExpressionSimplifierInterface_t* F_create_expression_simplifier_interface(void);

/**
 * @brief 创建常量折叠和表达式简化管理器接口实例
 * 
 * @return Stru_ConstantFoldingManagerInterface_t* 管理器接口实例，失败返回NULL
 */
Stru_ConstantFoldingManagerInterface_t* F_create_constant_folding_manager_interface(void);

/**
 * @brief 销毁常量折叠器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_constant_folding_interface(Stru_ConstantFoldingInterface_t* interface);

/**
 * @brief 销毁表达式简化器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_expression_simplifier_interface(Stru_ExpressionSimplifierInterface_t* interface);

/**
 * @brief 销毁常量折叠和表达式简化管理器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_constant_folding_manager_interface(Stru_ConstantFoldingManagerInterface_t* interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_CONSTANT_FOLDING_H */
