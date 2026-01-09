/**
 * @file CN_type_checker.h
 * @brief 类型检查器模块头文件
 
 * 本文件定义了类型检查器模块的接口和数据结构，负责检查表达式的类型兼容性、
 * 函数参数匹配、赋值类型检查等。遵循SOLID设计原则和分层架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_TYPE_CHECKER_H
#define CN_TYPE_CHECKER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_TypeCheckerInterface_t Stru_TypeCheckerInterface_t;
typedef struct Stru_ScopeManagerInterface_t Stru_ScopeManagerInterface_t;
typedef struct Stru_TypeCheckResult_t Stru_TypeCheckResult_t;

// ============================================================================
// 类型兼容性结果枚举
// ============================================================================

/**
 * @brief 类型兼容性结果枚举
 */
typedef enum Eum_TypeCompatibility
{
    Eum_TYPE_COMPATIBLE,        ///< 类型兼容
    Eum_TYPE_INCOMPATIBLE,      ///< 类型不兼容
    Eum_TYPE_CONVERTIBLE,       ///< 类型可转换
    Eum_TYPE_PROMOTABLE         ///< 类型可提升
} Eum_TypeCompatibility;

// ============================================================================
// 类型检查结果结构体
// ============================================================================

/**
 * @brief 类型检查结果结构体
 */
struct Stru_TypeCheckResult_t
{
    Eum_TypeCompatibility compatibility;  ///< 类型兼容性
    const char* error_message;            ///< 错误消息（如果不兼容）
    void* converted_type;                 ///< 转换后的类型信息（如果可转换）
    bool requires_explicit_cast;          ///< 是否需要显式类型转换
};

// ============================================================================
// 类型检查器接口
// ============================================================================

/**
 * @brief 类型检查器接口
 
 * 负责检查表达式的类型兼容性、函数参数匹配、赋值类型检查等。
 */
struct Stru_TypeCheckerInterface_t
{
    /**
     * @brief 初始化类型检查器
     * 
     * @param type_checker 类型检查器实例
     * @param scope_manager 作用域管理器
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_TypeCheckerInterface_t* type_checker,
                      Stru_ScopeManagerInterface_t* scope_manager);
    
    /**
     * @brief 检查二元表达式类型
     * 
     * @param type_checker 类型检查器实例
     * @param left_type 左操作数类型
     * @param right_type 右操作数类型
     * @param operator_type 运算符类型
     * @return Stru_TypeCheckResult_t 类型检查结果
     */
    Stru_TypeCheckResult_t (*check_binary_expression)(Stru_TypeCheckerInterface_t* type_checker,
                                                     void* left_type, void* right_type,
                                                     int operator_type);
    
    /**
     * @brief 检查一元表达式类型
     * 
     * @param type_checker 类型检查器实例
     * @param operand_type 操作数类型
     * @param operator_type 运算符类型
     * @return Stru_TypeCheckResult_t 类型检查结果
     */
    Stru_TypeCheckResult_t (*check_unary_expression)(Stru_TypeCheckerInterface_t* type_checker,
                                                    void* operand_type, int operator_type);
    
    /**
     * @brief 检查赋值类型
     * 
     * @param type_checker 类型检查器实例
     * @param target_type 目标类型
     * @param source_type 源类型
     * @param is_compound 是否为复合赋值
     * @return Stru_TypeCheckResult_t 类型检查结果
     */
    Stru_TypeCheckResult_t (*check_assignment)(Stru_TypeCheckerInterface_t* type_checker,
                                              void* target_type, void* source_type,
                                              bool is_compound);
    
    /**
     * @brief 检查函数调用类型
     * 
     * @param type_checker 类型检查器实例
     * @param function_type 函数类型
     * @param arg_types 参数类型数组
     * @param arg_count 参数数量
     * @return Stru_TypeCheckResult_t 类型检查结果
     */
    Stru_TypeCheckResult_t (*check_function_call)(Stru_TypeCheckerInterface_t* type_checker,
                                                 void* function_type,
                                                 void** arg_types, size_t arg_count);
    
    /**
     * @brief 检查类型转换
     * 
     * @param type_checker 类型检查器实例
     * @param from_type 源类型
     * @param to_type 目标类型
     * @param is_explicit 是否为显式转换
     * @return Stru_TypeCheckResult_t 类型检查结果
     */
    Stru_TypeCheckResult_t (*check_type_cast)(Stru_TypeCheckerInterface_t* type_checker,
                                             void* from_type, void* to_type,
                                             bool is_explicit);
    
    /**
     * @brief 获取表达式的推断类型
     * 
     * @param type_checker 类型检查器实例
     * @param ast_node AST节点
     * @return void* 推断的类型信息，失败返回NULL
     */
    void* (*infer_expression_type)(Stru_TypeCheckerInterface_t* type_checker,
                                  void* ast_node);
    
    /**
     * @brief 检查变量声明类型
     * 
     * @param type_checker 类型检查器实例
     * @param declared_type 声明的类型
     * @param initializer_type 初始化表达式类型
     * @return Stru_TypeCheckResult_t 类型检查结果
     */
    Stru_TypeCheckResult_t (*check_variable_declaration)(Stru_TypeCheckerInterface_t* type_checker,
                                                        void* declared_type,
                                                        void* initializer_type);
    
    /**
     * @brief 销毁类型检查器
     * 
     * @param type_checker 类型检查器实例
     */
    void (*destroy)(Stru_TypeCheckerInterface_t* type_checker);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 工厂函数声明
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建类型检查器接口实例
 * 
 * @return Stru_TypeCheckerInterface_t* 类型检查器接口实例，失败返回NULL
 */
Stru_TypeCheckerInterface_t* F_create_type_checker_interface(void);

/**
 * @brief 销毁类型检查器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_type_checker_interface(Stru_TypeCheckerInterface_t* interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_TYPE_CHECKER_H */
