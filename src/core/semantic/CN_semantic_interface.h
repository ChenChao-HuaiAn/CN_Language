/**
 * @file CN_semantic_interface.h
 * @brief 语义分析抽象接口定义
 
 * 本文件定义了CN_Language项目的语义分析接口，包括类型检查、符号表管理、
 * 作用域分析、语义错误报告等功能。遵循SOLID设计原则和分层架构，提供
 * 可扩展、可替换的语义分析实现。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_SEMANTIC_INTERFACE_H
#define CN_SEMANTIC_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_SemanticAnalyzerInterface_t Stru_SemanticAnalyzerInterface_t;
typedef struct Stru_TypeCheckerInterface_t Stru_TypeCheckerInterface_t;
typedef struct Stru_SymbolTableInterface_t Stru_SymbolTableInterface_t;
typedef struct Stru_ScopeManagerInterface_t Stru_ScopeManagerInterface_t;
typedef struct Stru_SemanticErrorReporterInterface_t Stru_SemanticErrorReporterInterface_t;

// ============================================================================
// 符号表接口
// ============================================================================

/**
 * @brief 符号类型枚举
 */
typedef enum Eum_SymbolType
{
    Eum_SYMBOL_VARIABLE,        ///< 变量符号
    Eum_SYMBOL_FUNCTION,        ///< 函数符号
    Eum_SYMBOL_STRUCT,          ///< 结构体符号
    Eum_SYMBOL_ENUM,            ///< 枚举符号
    Eum_SYMBOL_CONSTANT,        ///< 常量符号
    Eum_SYMBOL_PARAMETER,       ///< 参数符号
    Eum_SYMBOL_TYPE_ALIAS,      ///< 类型别名符号
    Eum_SYMBOL_MODULE,          ///< 模块符号
    Eum_SYMBOL_IMPORT,          ///< 导入符号
    Eum_SYMBOL_TEMPLATE         ///< 模板符号
} Eum_SymbolType;

/**
 * @brief 符号信息结构体
 */
typedef struct Stru_SymbolInfo_t
{
    const char* name;           ///< 符号名称
    Eum_SymbolType type;        ///< 符号类型
    void* type_info;            ///< 类型信息（具体类型由实现决定）
    size_t line;                ///< 定义行号
    size_t column;              ///< 定义列号
    bool is_exported;           ///< 是否导出（对外可见）
    bool is_initialized;        ///< 是否已初始化
    bool is_constant;           ///< 是否为常量
    void* extra_data;           ///< 额外数据（实现特定）
} Stru_SymbolInfo_t;

/**
 * @brief 符号表接口
 
 * 管理程序中的符号信息，支持符号的插入、查找、作用域管理等操作。
 */
struct Stru_SymbolTableInterface_t
{
    /**
     * @brief 初始化符号表
     * 
     * @param symbol_table 符号表实例
     * @param parent_scope 父作用域符号表（可为NULL）
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_SymbolTableInterface_t* symbol_table, 
                      Stru_SymbolTableInterface_t* parent_scope);
    
    /**
     * @brief 插入符号
     * 
     * @param symbol_table 符号表实例
     * @param symbol 符号信息
     * @return bool 插入成功返回true，否则返回false（如重复定义）
     */
    bool (*insert_symbol)(Stru_SymbolTableInterface_t* symbol_table, 
                         const Stru_SymbolInfo_t* symbol);
    
    /**
     * @brief 查找符号
     * 
     * @param symbol_table 符号表实例
     * @param name 符号名称
     * @param search_parent 是否在父作用域中查找
     * @return Stru_SymbolInfo_t* 符号信息指针，未找到返回NULL
     */
    Stru_SymbolInfo_t* (*lookup_symbol)(Stru_SymbolTableInterface_t* symbol_table,
                                       const char* name, bool search_parent);
    
    /**
     * @brief 删除符号
     * 
     * @param symbol_table 符号表实例
     * @param name 符号名称
     * @return bool 删除成功返回true，否则返回false
     */
    bool (*remove_symbol)(Stru_SymbolTableInterface_t* symbol_table,
                         const char* name);
    
    /**
     * @brief 获取符号数量
     * 
     * @param symbol_table 符号表实例
     * @return size_t 符号数量
     */
    size_t (*get_symbol_count)(Stru_SymbolTableInterface_t* symbol_table);
    
    /**
     * @brief 获取所有符号
     * 
     * @param symbol_table 符号表实例
     * @param symbols 输出参数：符号数组
     * @param max_symbols 最大符号数量
     * @return size_t 实际获取的符号数量
     */
    size_t (*get_all_symbols)(Stru_SymbolTableInterface_t* symbol_table,
                             Stru_SymbolInfo_t* symbols, size_t max_symbols);
    
    /**
     * @brief 清空符号表
     * 
     * @param symbol_table 符号表实例
     */
    void (*clear)(Stru_SymbolTableInterface_t* symbol_table);
    
    /**
     * @brief 销毁符号表
     * 
     * @param symbol_table 符号表实例
     */
    void (*destroy)(Stru_SymbolTableInterface_t* symbol_table);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 作用域管理器接口
// ============================================================================

/**
 * @brief 作用域类型枚举
 */
typedef enum Eum_ScopeType
{
    Eum_SCOPE_GLOBAL,           ///< 全局作用域
    Eum_SCOPE_FUNCTION,         ///< 函数作用域
    Eum_SCOPE_BLOCK,            ///< 块作用域
    Eum_SCOPE_STRUCT,           ///< 结构体作用域
    Eum_SCOPE_LOOP,             ///< 循环作用域
    Eum_SCOPE_CONDITIONAL,      ///< 条件作用域
    Eum_SCOPE_TRY_CATCH         ///< try-catch作用域
} Eum_ScopeType;

/**
 * @brief 作用域信息结构体
 */
typedef struct Stru_ScopeInfo_t
{
    Eum_ScopeType type;         ///< 作用域类型
    size_t depth;               ///< 作用域深度（0为全局作用域）
    const char* name;           ///< 作用域名称（如函数名）
    Stru_SymbolTableInterface_t* symbol_table;  ///< 关联的符号表
    void* extra_data;           ///< 额外数据（实现特定）
} Stru_ScopeInfo_t;

/**
 * @brief 作用域管理器接口
 
 * 管理程序中的作用域层次结构，支持作用域的创建、进入、退出等操作。
 */
struct Stru_ScopeManagerInterface_t
{
    /**
     * @brief 初始化作用域管理器
     * 
     * @param scope_manager 作用域管理器实例
     * @param global_symbol_table 全局符号表
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_ScopeManagerInterface_t* scope_manager,
                      Stru_SymbolTableInterface_t* global_symbol_table);
    
    /**
     * @brief 创建新作用域
     * 
     * @param scope_manager 作用域管理器实例
     * @param type 作用域类型
     * @param name 作用域名称（可选）
     * @return Stru_SymbolTableInterface_t* 新作用域的符号表，失败返回NULL
     */
    Stru_SymbolTableInterface_t* (*create_scope)(Stru_ScopeManagerInterface_t* scope_manager,
                                                Eum_ScopeType type, const char* name);
    
    /**
     * @brief 进入作用域
     * 
     * @param scope_manager 作用域管理器实例
     * @param symbol_table 要进入的作用域符号表
     * @return bool 进入成功返回true，否则返回false
     */
    bool (*enter_scope)(Stru_ScopeManagerInterface_t* scope_manager,
                       Stru_SymbolTableInterface_t* symbol_table);
    
    /**
     * @brief 退出当前作用域
     * 
     * @param scope_manager 作用域管理器实例
     * @return Stru_SymbolTableInterface_t* 退出后的当前作用域符号表
     */
    Stru_SymbolTableInterface_t* (*exit_scope)(Stru_ScopeManagerInterface_t* scope_manager);
    
    /**
     * @brief 获取当前作用域
     * 
     * @param scope_manager 作用域管理器实例
     * @return Stru_SymbolTableInterface_t* 当前作用域符号表
     */
    Stru_SymbolTableInterface_t* (*get_current_scope)(Stru_ScopeManagerInterface_t* scope_manager);
    
    /**
     * @brief 获取全局作用域
     * 
     * @param scope_manager 作用域管理器实例
     * @return Stru_SymbolTableInterface_t* 全局作用域符号表
     */
    Stru_SymbolTableInterface_t* (*get_global_scope)(Stru_ScopeManagerInterface_t* scope_manager);
    
    /**
     * @brief 获取作用域深度
     * 
     * @param scope_manager 作用域管理器实例
     * @return size_t 当前作用域深度
     */
    size_t (*get_scope_depth)(Stru_ScopeManagerInterface_t* scope_manager);
    
    /**
     * @brief 查找符号（在当前作用域链中）
     * 
     * @param scope_manager 作用域管理器实例
     * @param name 符号名称
     * @return Stru_SymbolInfo_t* 符号信息指针，未找到返回NULL
     */
    Stru_SymbolInfo_t* (*lookup_symbol_in_scope_chain)(Stru_ScopeManagerInterface_t* scope_manager,
                                                      const char* name);
    
    /**
     * @brief 销毁作用域管理器
     * 
     * @param scope_manager 作用域管理器实例
     */
    void (*destroy)(Stru_ScopeManagerInterface_t* scope_manager);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 类型检查器接口
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

/**
 * @brief 类型检查结果结构体
 */
typedef struct Stru_TypeCheckResult_t
{
    Eum_TypeCompatibility compatibility;  ///< 类型兼容性
    const char* error_message;            ///< 错误消息（如果不兼容）
    void* converted_type;                 ///< 转换后的类型信息（如果可转换）
    bool requires_explicit_cast;          ///< 是否需要显式类型转换
} Stru_TypeCheckResult_t;

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
// 语义错误报告接口
// ============================================================================

/**
 * @brief 语义错误类型枚举
 */
typedef enum Eum_SemanticErrorType
{
    Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL,      ///< 未定义符号
    Eum_SEMANTIC_ERROR_REDEFINED_SYMBOL,      ///< 重复定义符号
    Eum_SEMANTIC_ERROR_TYPE_MISMATCH,         ///< 类型不匹配
    Eum_SEMANTIC_ERROR_INVALID_OPERATION,     ///< 无效操作
    Eum_SEMANTIC_ERROR_INVALID_ARGUMENTS,     ///< 无效参数
    Eum_SEMANTIC_ERROR_UNREACHABLE_CODE,      ///< 不可达代码
    Eum_SEMANTIC_ERROR_MISSING_RETURN,        ///< 缺少返回语句
    Eum_SEMANTIC_ERROR_INVALID_RETURN_TYPE,   ///< 无效返回类型
    Eum_SEMANTIC_ERROR_UNINITIALIZED_VARIABLE,///< 未初始化变量
    Eum_SEMANTIC_ERROR_CONST_ASSIGNMENT,      ///< 常量赋值
    Eum_SEMANTIC_ERROR_SCOPE_VIOLATION,       ///< 作用域违规
    Eum_SEMANTIC_ERROR_CIRCULAR_DEPENDENCY,   ///< 循环依赖
    Eum_SEMANTIC_ERROR_INVALID_IMPORT,        ///< 无效导入
    Eum_SEMANTIC_ERROR_INVALID_EXPORT         ///< 无效导出
} Eum_SemanticErrorType;

/**
 * @brief 语义错误信息结构体
 */
typedef struct Stru_SemanticError_t
{
    Eum_SemanticErrorType type;      ///< 错误类型
    const char* message;             ///< 错误消息
    size_t line;                     ///< 错误行号
    size_t column;                   ///< 错误列号
    const char* file_name;           ///< 文件名
    void* related_node;              ///< 相关AST节点（可选）
    void* extra_data;                ///< 额外数据（实现特定）
} Stru_SemanticError_t;

/**
 * @brief 语义错误报告接口
 
 * 负责收集、管理和报告语义分析过程中发现的错误。
 */
struct Stru_SemanticErrorReporterInterface_t
{
    /**
     * @brief 初始化错误报告器
     * 
     * @param reporter 错误报告器实例
     * @param max_errors 最大错误数量限制（0表示无限制）
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_SemanticErrorReporterInterface_t* reporter,
                      size_t max_errors);
    
    /**
     * @brief 报告语义错误
     * 
     * @param reporter 错误报告器实例
     * @param error 错误信息
     * @return bool 报告成功返回true，否则返回false（如达到错误上限）
     */
    bool (*report_error)(Stru_SemanticErrorReporterInterface_t* reporter,
                        const Stru_SemanticError_t* error);
    
    /**
     * @brief 报告语义警告
     * 
     * @param reporter 错误报告器实例
     * @param error 警告信息（使用错误结构体）
     * @return bool 报告成功返回true，否则返回false
     */
    bool (*report_warning)(Stru_SemanticErrorReporterInterface_t* reporter,
                          const Stru_SemanticError_t* error);
    
    /**
     * @brief 获取错误数量
     * 
     * @param reporter 错误报告器实例
     * @return size_t 错误数量
     */
    size_t (*get_error_count)(Stru_SemanticErrorReporterInterface_t* reporter);
    
    /**
     * @brief 获取警告数量
     * 
     * @param reporter 错误报告器实例
     * @return size_t 警告数量
     */
    size_t (*get_warning_count)(Stru_SemanticErrorReporterInterface_t* reporter);
    
    /**
     * @brief 获取所有错误
     * 
     * @param reporter 错误报告器实例
     * @param errors 输出参数：错误数组
     * @param max_errors 最大错误数量
     * @return size_t 实际获取的错误数量
     */
    size_t (*get_all_errors)(Stru_SemanticErrorReporterInterface_t* reporter,
                            Stru_SemanticError_t* errors, size_t max_errors);
    
    /**
     * @brief 获取所有警告
     * 
     * @param reporter 错误报告器实例
     * @param warnings 输出参数：警告数组
     * @param max_warnings 最大警告数量
     * @return size_t 实际获取的警告数量
     */
    size_t (*get_all_warnings)(Stru_SemanticErrorReporterInterface_t* reporter,
                              Stru_SemanticError_t* warnings, size_t max_warnings);
    
    /**
     * @brief 清空所有错误和警告
     * 
     * @param reporter 错误报告器实例
     */
    void (*clear_all)(Stru_SemanticErrorReporterInterface_t* reporter);
    
    /**
     * @brief 检查是否有错误
     * 
     * @param reporter 错误报告器实例
     * @return bool 有错误返回true，否则返回false
     */
    bool (*has_errors)(Stru_SemanticErrorReporterInterface_t* reporter);
    
    /**
     * @brief 检查是否有警告
     * 
     * @param reporter 错误报告器实例
     * @return bool 有警告返回true，否则返回false
     */
    bool (*has_warnings)(Stru_SemanticErrorReporterInterface_t* reporter);
    
    /**
     * @brief 设置错误处理回调
     * 
     * @param reporter 错误报告器实例
     * @param callback 回调函数
     * @param user_data 用户数据
     */
    void (*set_error_callback)(Stru_SemanticErrorReporterInterface_t* reporter,
                              void (*callback)(const Stru_SemanticError_t* error, void* user_data),
                              void* user_data);
    
    /**
     * @brief 销毁错误报告器
     * 
     * @param reporter 错误报告器实例
     */
    void (*destroy)(Stru_SemanticErrorReporterInterface_t* reporter);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 主语义分析器接口
// ============================================================================

/**
 * @brief 语义分析器接口
 
 * 整合所有语义分析组件，提供完整的语义分析功能。
 */
struct Stru_SemanticAnalyzerInterface_t
{
    /**
     * @brief 初始化语义分析器
     * 
     * @param analyzer 语义分析器实例
     * @param symbol_table 符号表接口
     * @param scope_manager 作用域管理器接口
     * @param type_checker 类型检查器接口
     * @param error_reporter 错误报告器接口
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_SemanticAnalyzerInterface_t* analyzer,
                      Stru_SymbolTableInterface_t* symbol_table,
                      Stru_ScopeManagerInterface_t* scope_manager,
                      Stru_TypeCheckerInterface_t* type_checker,
                      Stru_SemanticErrorReporterInterface_t* error_reporter);
    
    /**
     * @brief 分析AST节点
     * 
     * @param analyzer 语义分析器实例
     * @param ast_node AST根节点
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_ast)(Stru_SemanticAnalyzerInterface_t* analyzer,
                       void* ast_node);
    
    /**
     * @brief 分析单个声明
     * 
     * @param analyzer 语义分析器实例
     * @param declaration_node 声明AST节点
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_declaration)(Stru_SemanticAnalyzerInterface_t* analyzer,
                               void* declaration_node);
    
    /**
     * @brief 分析单个语句
     * 
     * @param analyzer 语义分析器实例
     * @param statement_node 语句AST节点
     * @return bool 分析成功返回true，否则返回false
     */
    bool (*analyze_statement)(Stru_SemanticAnalyzerInterface_t* analyzer,
                             void* statement_node);
    
    /**
     * @brief 分析单个表达式
     * 
     * @param analyzer 语义分析器实例
     * @param expression_node 表达式AST节点
     * @return void* 表达式的类型信息，失败返回NULL
     */
    void* (*analyze_expression)(Stru_SemanticAnalyzerInterface_t* analyzer,
                               void* expression_node);
    
    /**
     * @brief 获取符号表接口
     * 
     * @param analyzer 语义分析器实例
     * @return Stru_SymbolTableInterface_t* 符号表接口
     */
    Stru_SymbolTableInterface_t* (*get_symbol_table)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    /**
     * @brief 获取作用域管理器接口
     * 
     * @param analyzer 语义分析器实例
     * @return Stru_ScopeManagerInterface_t* 作用域管理器接口
     */
    Stru_ScopeManagerInterface_t* (*get_scope_manager)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    /**
     * @brief 获取类型检查器接口
     * 
     * @param analyzer 语义分析器实例
     * @return Stru_TypeCheckerInterface_t* 类型检查器接口
     */
    Stru_TypeCheckerInterface_t* (*get_type_checker)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    /**
     * @brief 获取错误报告器接口
     * 
     * @param analyzer 语义分析器实例
     * @return Stru_SemanticErrorReporterInterface_t* 错误报告器接口
     */
    Stru_SemanticErrorReporterInterface_t* (*get_error_reporter)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    /**
     * @brief 重置语义分析器状态
     * 
     * @param analyzer 语义分析器实例
     */
    void (*reset)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    /**
     * @brief 销毁语义分析器
     * 
     * @param analyzer 语义分析器实例
     */
    void (*destroy)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
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
 * @brief 创建符号表接口实例
 * 
 * @return Stru_SymbolTableInterface_t* 符号表接口实例，失败返回NULL
 */
Stru_SymbolTableInterface_t* F_create_symbol_table_interface(void);

/**
 * @brief 创建作用域管理器接口实例
 * 
 * @return Stru_ScopeManagerInterface_t* 作用域管理器接口实例，失败返回NULL
 */
Stru_ScopeManagerInterface_t* F_create_scope_manager_interface(void);

/**
 * @brief 创建类型检查器接口实例
 * 
 * @return Stru_TypeCheckerInterface_t* 类型检查器接口实例，失败返回NULL
 */
Stru_TypeCheckerInterface_t* F_create_type_checker_interface(void);

/**
 * @brief 创建语义错误报告器接口实例
 * 
 * @return Stru_SemanticErrorReporterInterface_t* 错误报告器接口实例，失败返回NULL
 */
Stru_SemanticErrorReporterInterface_t* F_create_semantic_error_reporter_interface(void);

/**
 * @brief 创建语义分析器接口实例
 * 
 * @return Stru_SemanticAnalyzerInterface_t* 语义分析器接口实例，失败返回NULL
 */
Stru_SemanticAnalyzerInterface_t* F_create_semantic_analyzer_interface(void);

/**
 * @brief 销毁符号表接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_symbol_table_interface(Stru_SymbolTableInterface_t* interface);

/**
 * @brief 销毁作用域管理器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_scope_manager_interface(Stru_ScopeManagerInterface_t* interface);

/**
 * @brief 销毁类型检查器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_type_checker_interface(Stru_TypeCheckerInterface_t* interface);

/**
 * @brief 销毁语义错误报告器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_semantic_error_reporter_interface(Stru_SemanticErrorReporterInterface_t* interface);

/**
 * @brief 销毁语义分析器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_semantic_analyzer_interface(Stru_SemanticAnalyzerInterface_t* interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_SEMANTIC_INTERFACE_H */
