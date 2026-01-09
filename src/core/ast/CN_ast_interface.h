/**
 * @file CN_ast_interface.h
 * @brief CN_Language 抽象语法树（AST）接口定义
 * 
 * 定义AST模块的抽象接口，支持AST节点的创建、操作、遍历和查询。
 * 遵循SOLID设计原则和分层架构，提供可扩展、可替换的AST实现。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_AST_INTERFACE_H
#define CN_AST_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_AstNodeInterface_t Stru_AstNodeInterface_t;
typedef struct Stru_AstBuilderInterface_t Stru_AstBuilderInterface_t;
typedef struct Stru_AstTraversalInterface_t Stru_AstTraversalInterface_t;
typedef struct Stru_AstQueryInterface_t Stru_AstQueryInterface_t;
typedef struct Stru_AstSerializerInterface_t Stru_AstSerializerInterface_t;

// ============================================================================
// AST节点类型枚举
// ============================================================================

/**
 * @brief AST节点类型枚举
 * 
 * 定义所有可能的AST节点类型，涵盖CN_Language的所有语法结构。
 */
typedef enum Eum_AstNodeType
{
    // ============================================
    // 程序结构
    // ============================================
    Eum_AST_PROGRAM,              ///< 程序根节点
    Eum_AST_MODULE,               ///< 模块定义
    Eum_AST_IMPORT,               ///< 导入语句
    
    // ============================================
    // 声明
    // ============================================
    Eum_AST_VARIABLE_DECL,        ///< 变量声明
    Eum_AST_FUNCTION_DECL,        ///< 函数声明
    Eum_AST_STRUCT_DECL,          ///< 结构体声明
    Eum_AST_ENUM_DECL,            ///< 枚举声明
    Eum_AST_CONSTANT_DECL,        ///< 常量声明
    Eum_AST_PARAMETER_DECL,       ///< 参数声明
    
    // ============================================
    // 语句
    // ============================================
    Eum_AST_EXPRESSION_STMT,      ///< 表达式语句
    Eum_AST_IF_STMT,              ///< if语句
    Eum_AST_WHILE_STMT,           ///< while语句
    Eum_AST_FOR_STMT,             ///< for语句
    Eum_AST_RETURN_STMT,          ///< return语句
    Eum_AST_BREAK_STMT,           ///< break语句
    Eum_AST_CONTINUE_STMT,        ///< continue语句
    Eum_AST_BLOCK_STMT,           ///< 代码块语句
    Eum_AST_SWITCH_STMT,          ///< switch语句
    Eum_AST_CASE_STMT,            ///< case语句
    Eum_AST_DEFAULT_STMT,         ///< default语句
    Eum_AST_TRY_STMT,             ///< try语句
    Eum_AST_CATCH_STMT,           ///< catch语句
    Eum_AST_THROW_STMT,           ///< throw语句
    Eum_AST_FINALLY_STMT,         ///< finally语句
    
    // ============================================
    // 表达式
    // ============================================
    Eum_AST_BINARY_EXPR,          ///< 二元表达式
    Eum_AST_UNARY_EXPR,           ///< 一元表达式
    Eum_AST_LITERAL_EXPR,         ///< 字面量表达式
    Eum_AST_IDENTIFIER_EXPR,      ///< 标识符表达式
    Eum_AST_CALL_EXPR,            ///< 函数调用表达式
    Eum_AST_INDEX_EXPR,           ///< 数组索引表达式
    Eum_AST_MEMBER_EXPR,          ///< 成员访问表达式
    Eum_AST_ASSIGN_EXPR,          ///< 赋值表达式
    Eum_AST_COMPOUND_ASSIGN_EXPR, ///< 复合赋值表达式
    Eum_AST_CAST_EXPR,            ///< 类型转换表达式
    Eum_AST_CONDITIONAL_EXPR,     ///< 条件表达式（三元运算符）
    Eum_AST_NEW_EXPR,             ///< 对象创建表达式
    Eum_AST_DELETE_EXPR,          ///< 对象销毁表达式
    
    // ============================================
    // 类型
    // ============================================
    Eum_AST_TYPE_NAME,            ///< 类型名称
    Eum_AST_ARRAY_TYPE,           ///< 数组类型
    Eum_AST_POINTER_TYPE,         ///< 指针类型
    Eum_AST_REFERENCE_TYPE,       ///< 引用类型
    Eum_AST_FUNCTION_TYPE,        ///< 函数类型
    
    // ============================================
    // 字面量
    // ============================================
    Eum_AST_INT_LITERAL,          ///< 整数字面量
    Eum_AST_FLOAT_LITERAL,        ///< 浮点数字面量
    Eum_AST_STRING_LITERAL,       ///< 字符串字面量
    Eum_AST_BOOL_LITERAL,         ///< 布尔字面量
    Eum_AST_ARRAY_LITERAL,        ///< 数组字面量
    Eum_AST_STRUCT_LITERAL,       ///< 结构体字面量
    Eum_AST_NULL_LITERAL,         ///< null字面量
    
    // ============================================
    // 错误处理
    // ============================================
    Eum_AST_ERROR_NODE,           ///< 错误节点（用于错误恢复）
    
    // ============================================
    // 特殊节点
    // ============================================
    Eum_AST_COMMENT,              ///< 注释节点
    Eum_AST_DIRECTIVE,            ///< 预处理指令节点
    
    Eum_AST_COUNT                 ///< AST节点类型总数（用于边界检查）
} Eum_AstNodeType;

// ============================================================================
// AST节点数据联合体
// ============================================================================

/**
 * @brief AST节点数据联合体
 * 
 * 根据节点类型存储相应的数据。
 */
typedef union Uni_AstNodeData_t
{
    // 字面量值
    long int_value;                   ///< 整数值
    double float_value;               ///< 浮点数值
    char* string_value;               ///< 字符串值
    bool bool_value;                  ///< 布尔值
    
    // 类型信息
    char* type_name;                  ///< 类型名称
    size_t array_size;                ///< 数组大小
    
    // 其他数据
    char* identifier;                 ///< 标识符名称
    int operator_type;                ///< 运算符类型
    
    // 通用指针
    void* custom_data;                ///< 自定义数据
} Uni_AstNodeData_t;

// ============================================================================
// AST节点位置信息
// ============================================================================

/**
 * @brief AST节点位置信息结构体
 */
typedef struct Stru_AstNodeLocation_t
{
    size_t line;                      ///< 行号（从1开始）
    size_t column;                    ///< 列号（从1开始）
    const char* file_name;            ///< 文件名
    size_t start_offset;              ///< 起始偏移量
    size_t end_offset;                ///< 结束偏移量
} Stru_AstNodeLocation_t;

// ============================================================================
// AST节点接口
// ============================================================================

/**
 * @brief AST节点接口
 * 
 * 表示抽象语法树中的一个节点，提供节点的基本操作。
 */
struct Stru_AstNodeInterface_t
{
    /**
     * @brief 获取节点类型
     * 
     * @param node 节点实例
     * @return Eum_AstNodeType 节点类型
     */
    Eum_AstNodeType (*get_type)(const Stru_AstNodeInterface_t* node);
    
    /**
     * @brief 获取节点位置信息
     * 
     * @param node 节点实例
     * @return const Stru_AstNodeLocation_t* 位置信息指针
     */
    const Stru_AstNodeLocation_t* (*get_location)(const Stru_AstNodeInterface_t* node);
    
    /**
     * @brief 获取节点数据
     * 
     * @param node 节点实例
     * @return const Uni_AstNodeData_t* 节点数据指针
     */
    const Uni_AstNodeData_t* (*get_data)(const Stru_AstNodeInterface_t* node);
    
    /**
     * @brief 设置节点数据
     * 
     * @param node 节点实例
     * @param data 节点数据
     * @return bool 设置成功返回true，否则返回false
     */
    bool (*set_data)(Stru_AstNodeInterface_t* node, const Uni_AstNodeData_t* data);
    
    /**
     * @brief 获取父节点
     * 
     * @param node 节点实例
     * @return Stru_AstNodeInterface_t* 父节点指针，无父节点返回NULL
     */
    Stru_AstNodeInterface_t* (*get_parent)(const Stru_AstNodeInterface_t* node);
    
    /**
     * @brief 设置父节点
     * 
     * @param node 节点实例
     * @param parent 父节点
     * @return bool 设置成功返回true，否则返回false
     */
    bool (*set_parent)(Stru_AstNodeInterface_t* node, Stru_AstNodeInterface_t* parent);
    
    /**
     * @brief 获取子节点数量
     * 
     * @param node 节点实例
     * @return size_t 子节点数量
     */
    size_t (*get_child_count)(const Stru_AstNodeInterface_t* node);
    
    /**
     * @brief 获取子节点
     * 
     * @param node 节点实例
     * @param index 子节点索引
     * @return Stru_AstNodeInterface_t* 子节点指针，索引无效返回NULL
     */
    Stru_AstNodeInterface_t* (*get_child)(const Stru_AstNodeInterface_t* node, size_t index);
    
    /**
     * @brief 添加子节点
     * 
     * @param node 节点实例
     * @param child 子节点
     * @return bool 添加成功返回true，否则返回false
     */
    bool (*add_child)(Stru_AstNodeInterface_t* node, Stru_AstNodeInterface_t* child);
    
    /**
     * @brief 移除子节点
     * 
     * @param node 节点实例
     * @param index 子节点索引
     * @return Stru_AstNodeInterface_t* 被移除的子节点指针，索引无效返回NULL
     */
    Stru_AstNodeInterface_t* (*remove_child)(Stru_AstNodeInterface_t* node, size_t index);
    
    /**
     * @brief 获取属性值
     * 
     * @param node 节点实例
     * @param key 属性键
     * @return void* 属性值，不存在返回NULL
     */
    void* (*get_attribute)(const Stru_AstNodeInterface_t* node, const char* key);
    
    /**
     * @brief 设置属性值
     * 
     * @param node 节点实例
     * @param key 属性键
     * @param value 属性值
     * @return bool 设置成功返回true，否则返回false
     */
    bool (*set_attribute)(Stru_AstNodeInterface_t* node, const char* key, void* value);
    
    /**
     * @brief 移除属性
     * 
     * @param node 节点实例
     * @param key 属性键
     * @return bool 移除成功返回true，否则返回false
     */
    bool (*remove_attribute)(Stru_AstNodeInterface_t* node, const char* key);
    
    /**
     * @brief 检查是否有属性
     * 
     * @param node 节点实例
     * @param key 属性键
     * @return bool 有属性返回true，否则返回false
     */
    bool (*has_attribute)(const Stru_AstNodeInterface_t* node, const char* key);
    
    /**
     * @brief 复制节点
     * 
     * @param node 节点实例
     * @return Stru_AstNodeInterface_t* 新复制的节点指针
     */
    Stru_AstNodeInterface_t* (*copy)(const Stru_AstNodeInterface_t* node);
    
    /**
     * @brief 销毁节点
     * 
     * @param node 节点实例
     */
    void (*destroy)(Stru_AstNodeInterface_t* node);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// AST构建器接口
// ============================================================================

/**
 * @brief AST构建器接口
 * 
 * 负责创建和构建AST节点，支持语法分析器的AST构建需求。
 */
struct Stru_AstBuilderInterface_t
{
    /**
     * @brief 初始化AST构建器
     * 
     * @param builder AST构建器实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_AstBuilderInterface_t* builder);
    
    /**
     * @brief 创建AST节点
     * 
     * @param builder AST构建器实例
     * @param type 节点类型
     * @param location 节点位置信息
     * @return Stru_AstNodeInterface_t* 新创建的AST节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*create_node)(Stru_AstBuilderInterface_t* builder,
                                           Eum_AstNodeType type,
                                           const Stru_AstNodeLocation_t* location);
    
    /**
     * @brief 创建字面量节点
     * 
     * @param builder AST构建器实例
     * @param type 字面量类型
     * @param location 节点位置信息
     * @param data 字面量数据
     * @return Stru_AstNodeInterface_t* 新创建的字面量节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*create_literal_node)(Stru_AstBuilderInterface_t* builder,
                                                   Eum_AstNodeType type,
                                                   const Stru_AstNodeLocation_t* location,
                                                   const Uni_AstNodeData_t* data);
    
    /**
     * @brief 创建标识符节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param identifier 标识符名称
     * @return Stru_AstNodeInterface_t* 新创建的标识符节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*create_identifier_node)(Stru_AstBuilderInterface_t* builder,
                                                      const Stru_AstNodeLocation_t* location,
                                                      const char* identifier);
    
    /**
     * @brief 创建类型节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param type_name 类型名称
     * @return Stru_AstNodeInterface_t* 新创建的类型节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*create_type_node)(Stru_AstBuilderInterface_t* builder,
                                                const Stru_AstNodeLocation_t* location,
                                                const char* type_name);
    
    /**
     * @brief 创建运算符节点
     * 
     * @param builder AST构建器实例
     * @param type 节点类型（二元或一元表达式）
     * @param location 节点位置信息
     * @param operator_type 运算符类型
     * @return Stru_AstNodeInterface_t* 新创建的运算符节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*create_operator_node)(Stru_AstBuilderInterface_t* builder,
                                                    Eum_AstNodeType type,
                                                    const Stru_AstNodeLocation_t* location,
                                                    int operator_type);
    
    /**
     * @brief 构建程序节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param module_name 模块名称
     * @param declarations 声明节点数组
     * @param declaration_count 声明数量
     * @return Stru_AstNodeInterface_t* 新创建的程序节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_program_node)(Stru_AstBuilderInterface_t* builder,
                                                  const Stru_AstNodeLocation_t* location,
                                                  const char* module_name,
                                                  Stru_AstNodeInterface_t** declarations,
                                                  size_t declaration_count);
    
    /**
     * @brief 构建函数声明节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param function_name 函数名称
     * @param return_type 返回类型节点
     * @param parameters 参数节点数组
     * @param parameter_count 参数数量
     * @param body 函数体节点
     * @return Stru_AstNodeInterface_t* 新创建的函数声明节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_function_declaration)(Stru_AstBuilderInterface_t* builder,
                                                          const Stru_AstNodeLocation_t* location,
                                                          const char* function_name,
                                                          Stru_AstNodeInterface_t* return_type,
                                                          Stru_AstNodeInterface_t** parameters,
                                                          size_t parameter_count,
                                                          Stru_AstNodeInterface_t* body);
    
    /**
     * @brief 构建变量声明节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param variable_name 变量名称
     * @param type 类型节点
     * @param initializer 初始化表达式节点（可选）
     * @return Stru_AstNodeInterface_t* 新创建的变量声明节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_variable_declaration)(Stru_AstBuilderInterface_t* builder,
                                                          const Stru_AstNodeLocation_t* location,
                                                          const char* variable_name,
                                                          Stru_AstNodeInterface_t* type,
                                                          Stru_AstNodeInterface_t* initializer);
    
    /**
     * @brief 构建表达式语句节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param expression 表达式节点
     * @return Stru_AstNodeInterface_t* 新创建的表达式语句节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_expression_statement)(Stru_AstBuilderInterface_t* builder,
                                                          const Stru_AstNodeLocation_t* location,
                                                          Stru_AstNodeInterface_t* expression);
    
    /**
     * @brief 构建if语句节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param condition 条件表达式节点
     * @param then_branch then分支节点
     * @param else_branch else分支节点（可选）
     * @return Stru_AstNodeInterface_t* 新创建的if语句节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_if_statement)(Stru_AstBuilderInterface_t* builder,
                                                  const Stru_AstNodeLocation_t* location,
                                                  Stru_AstNodeInterface_t* condition,
                                                  Stru_AstNodeInterface_t* then_branch,
                                                  Stru_AstNodeInterface_t* else_branch);
    
    /**
     * @brief 构建while语句节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param condition 条件表达式节点
     * @param body 循环体节点
     * @return Stru_AstNodeInterface_t* 新创建的while语句节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_while_statement)(Stru_AstBuilderInterface_t* builder,
                                                     const Stru_AstNodeLocation_t* location,
                                                     Stru_AstNodeInterface_t* condition,
                                                     Stru_AstNodeInterface_t* body);
    
    /**
     * @brief 构建for语句节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param init 初始化表达式节点（可选）
     * @param condition 条件表达式节点（可选）
     * @param update 更新表达式节点（可选）
     * @param body 循环体节点
     * @return Stru_AstNodeInterface_t* 新创建的for语句节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_for_statement)(Stru_AstBuilderInterface_t* builder,
                                                   const Stru_AstNodeLocation_t* location,
                                                   Stru_AstNodeInterface_t* init,
                                                   Stru_AstNodeInterface_t* condition,
                                                   Stru_AstNodeInterface_t* update,
                                                   Stru_AstNodeInterface_t* body);
    
    /**
     * @brief 构建return语句节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param expression 返回表达式节点（可选）
     * @return Stru_AstNodeInterface_t* 新创建的return语句节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_return_statement)(Stru_AstBuilderInterface_t* builder,
                                                      const Stru_AstNodeLocation_t* location,
                                                      Stru_AstNodeInterface_t* expression);
    
    /**
     * @brief 构建二元表达式节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param operator_type 运算符类型
     * @param left 左操作数节点
     * @param right 右操作数节点
     * @return Stru_AstNodeInterface_t* 新创建的二元表达式节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_binary_expression)(Stru_AstBuilderInterface_t* builder,
                                                       const Stru_AstNodeLocation_t* location,
                                                       int operator_type,
                                                       Stru_AstNodeInterface_t* left,
                                                       Stru_AstNodeInterface_t* right);
    
    /**
     * @brief 构建一元表达式节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param operator_type 运算符类型
     * @param operand 操作数节点
     * @return Stru_AstNodeInterface_t* 新创建的一元表达式节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_unary_expression)(Stru_AstBuilderInterface_t* builder,
                                                      const Stru_AstNodeLocation_t* location,
                                                      int operator_type,
                                                      Stru_AstNodeInterface_t* operand);
    
    /**
     * @brief 构建函数调用表达式节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param function 函数表达式节点
     * @param arguments 参数节点数组
     * @param argument_count 参数数量
     * @return Stru_AstNodeInterface_t* 新创建的函数调用表达式节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_call_expression)(Stru_AstBuilderInterface_t* builder,
                                                     const Stru_AstNodeLocation_t* location,
                                                     Stru_AstNodeInterface_t* function,
                                                     Stru_AstNodeInterface_t** arguments,
                                                     size_t argument_count);
    
    /**
     * @brief 构建try语句节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param try_block try块节点
     * @param catch_clauses catch子句节点数组
     * @param catch_clause_count catch子句数量
     * @param finally_block finally块节点（可选）
     * @return Stru_AstNodeInterface_t* 新创建的try语句节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_try_statement)(Stru_AstBuilderInterface_t* builder,
                                                   const Stru_AstNodeLocation_t* location,
                                                   Stru_AstNodeInterface_t* try_block,
                                                   Stru_AstNodeInterface_t** catch_clauses,
                                                   size_t catch_clause_count,
                                                   Stru_AstNodeInterface_t* finally_block);
    
    /**
     * @brief 构建catch语句节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param exception_type 异常类型节点（可选）
     * @param exception_name 异常名称（可选）
     * @param catch_block catch块节点
     * @return Stru_AstNodeInterface_t* 新创建的catch语句节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_catch_statement)(Stru_AstBuilderInterface_t* builder,
                                                     const Stru_AstNodeLocation_t* location,
                                                     Stru_AstNodeInterface_t* exception_type,
                                                     const char* exception_name,
                                                     Stru_AstNodeInterface_t* catch_block);
    
    /**
     * @brief 构建throw语句节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param expression 抛出表达式节点（可选）
     * @return Stru_AstNodeInterface_t* 新创建的throw语句节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_throw_statement)(Stru_AstBuilderInterface_t* builder,
                                                     const Stru_AstNodeLocation_t* location,
                                                     Stru_AstNodeInterface_t* expression);
    
    /**
     * @brief 构建finally语句节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param finally_block finally块节点
     * @return Stru_AstNodeInterface_t* 新创建的finally语句节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_finally_statement)(Stru_AstBuilderInterface_t* builder,
                                                       const Stru_AstNodeLocation_t* location,
                                                       Stru_AstNodeInterface_t* finally_block);
    
    /**
     * @brief 构建switch语句节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param expression switch表达式节点
     * @param cases case语句节点数组
     * @param case_count case语句数量
     * @param default_case default语句节点（可选）
     * @return Stru_AstNodeInterface_t* 新创建的switch语句节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_switch_statement)(Stru_AstBuilderInterface_t* builder,
                                                      const Stru_AstNodeLocation_t* location,
                                                      Stru_AstNodeInterface_t* expression,
                                                      Stru_AstNodeInterface_t** cases,
                                                      size_t case_count,
                                                      Stru_AstNodeInterface_t* default_case);
    
    /**
     * @brief 构建case语句节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param case_expression case表达式节点
     * @param case_body case体节点
     * @return Stru_AstNodeInterface_t* 新创建的case语句节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_case_statement)(Stru_AstBuilderInterface_t* builder,
                                                    const Stru_AstNodeLocation_t* location,
                                                    Stru_AstNodeInterface_t* case_expression,
                                                    Stru_AstNodeInterface_t* case_body);
    
    /**
     * @brief 构建default语句节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param default_body default体节点
     * @return Stru_AstNodeInterface_t* 新创建的default语句节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_default_statement)(Stru_AstBuilderInterface_t* builder,
                                                       const Stru_AstNodeLocation_t* location,
                                                       Stru_AstNodeInterface_t* default_body);
    
    /**
     * @brief 构建数组字面量节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param elements 数组元素节点数组
     * @param element_count 数组元素数量
     * @return Stru_AstNodeInterface_t* 新创建的数组字面量节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_array_literal)(Stru_AstBuilderInterface_t* builder,
                                                   const Stru_AstNodeLocation_t* location,
                                                   Stru_AstNodeInterface_t** elements,
                                                   size_t element_count);
    
    /**
     * @brief 构建结构体字面量节点
     * 
     * @param builder AST构建器实例
     * @param location 节点位置信息
     * @param struct_type_name 结构体类型名称（可选）
     * @param members 结构体成员节点数组
     * @param member_count 结构体成员数量
     * @return Stru_AstNodeInterface_t* 新创建的结构体字面量节点指针，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*build_struct_literal)(Stru_AstBuilderInterface_t* builder,
                                                    const Stru_AstNodeLocation_t* location,
                                                    const char* struct_type_name,
                                                    Stru_AstNodeInterface_t** members,
                                                    size_t member_count);
    
    /**
     * @brief 销毁AST构建器
     * 
     * @param builder AST构建器实例
     */
    void (*destroy)(Stru_AstBuilderInterface_t* builder);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// AST遍历接口
// ============================================================================

/**
 * @brief AST遍历接口
 * 
 * 提供AST树的遍历功能，支持深度优先、广度优先等遍历方式。
 */
struct Stru_AstTraversalInterface_t
{
    /**
     * @brief 初始化AST遍历器
     * 
     * @param traversal AST遍历器实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_AstTraversalInterface_t* traversal);
    
    /**
     * @brief 深度优先遍历
     * 
     * @param traversal AST遍历器实例
     * @param root 根节点
     * @param pre_order 前序遍历回调函数（可选）
     * @param in_order 中序遍历回调函数（可选）
     * @param post_order 后序遍历回调函数（可选）
     * @param context 上下文参数
     */
    void (*depth_first_traversal)(Stru_AstTraversalInterface_t* traversal,
                                 Stru_AstNodeInterface_t* root,
                                 void (*pre_order)(Stru_AstNodeInterface_t*, void*),
                                 void (*in_order)(Stru_AstNodeInterface_t*, void*),
                                 void (*post_order)(Stru_AstNodeInterface_t*, void*),
                                 void* context);
    
    /**
     * @brief 广度优先遍历
     * 
     * @param traversal AST遍历器实例
     * @param root 根节点
     * @param visitor 访问者回调函数
     * @param context 上下文参数
     */
    void (*breadth_first_traversal)(Stru_AstTraversalInterface_t* traversal,
                                   Stru_AstNodeInterface_t* root,
                                   void (*visitor)(Stru_AstNodeInterface_t*, void*),
                                   void* context);
    
    /**
     * @brief 查找满足条件的节点
     * 
     * @param traversal AST遍历器实例
     * @param root 根节点
     * @param predicate 谓词函数
     * @param context 上下文参数
     * @return Stru_AstNodeInterface_t** 匹配的节点数组，NULL表示无匹配
     */
    Stru_AstNodeInterface_t** (*find_nodes)(Stru_AstTraversalInterface_t* traversal,
                                           Stru_AstNodeInterface_t* root,
                                           bool (*predicate)(Stru_AstNodeInterface_t*, void*),
                                           void* context);
    
    /**
     * @brief 获取匹配节点数量
     * 
     * @param traversal AST遍历器实例
     * @return size_t 匹配节点数量
     */
    size_t (*get_match_count)(Stru_AstTraversalInterface_t* traversal);
    
    /**
     * @brief 销毁AST遍历器
     * 
     * @param traversal AST遍历器实例
     */
    void (*destroy)(Stru_AstTraversalInterface_t* traversal);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// AST查询接口
// ============================================================================

/**
 * @brief AST查询接口
 * 
 * 提供AST树的查询功能，支持节点类型统计、属性查询等。
 */
struct Stru_AstQueryInterface_t
{
    /**
     * @brief 初始化AST查询器
     * 
     * @param query AST查询器实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_AstQueryInterface_t* query);
    
    /**
     * @brief 统计节点类型
     * 
     * @param query AST查询器实例
     * @param root 根节点
     * @param type_counts 输出参数：类型统计数组
     * @param max_types 最大类型数量
     * @return size_t 实际统计的类型数量
     */
    size_t (*count_node_types)(Stru_AstQueryInterface_t* query,
                              Stru_AstNodeInterface_t* root,
                              size_t* type_counts,
                              size_t max_types);
    
    /**
     * @brief 查找具有特定属性的节点
     * 
     * @param query AST查询器实例
     * @param root 根节点
     * @param key 属性键
     * @param value 属性值
     * @return Stru_AstNodeInterface_t** 匹配的节点数组，NULL表示无匹配
     */
    Stru_AstNodeInterface_t** (*find_nodes_with_attribute)(Stru_AstQueryInterface_t* query,
                                                          Stru_AstNodeInterface_t* root,
                                                          const char* key,
                                                          void* value);
    
    /**
     * @brief 获取节点深度
     * 
     * @param query AST查询器实例
     * @param node 节点
     * @return size_t 节点深度（根节点为0）
     */
    size_t (*get_node_depth)(Stru_AstQueryInterface_t* query,
                            Stru_AstNodeInterface_t* node);
    
    /**
     * @brief 获取子树大小
     * 
     * @param query AST查询器实例
     * @param node 节点
     * @return size_t 子树节点数量（包括自身）
     */
    size_t (*get_subtree_size)(Stru_AstQueryInterface_t* query,
                              Stru_AstNodeInterface_t* node);
    
    /**
     * @brief 验证AST结构
     * 
     * @param query AST查询器实例
     * @param root 根节点
     * @return bool AST结构有效返回true，否则返回false
     */
    bool (*validate_ast_structure)(Stru_AstQueryInterface_t* query,
                                  Stru_AstNodeInterface_t* root);
    
    /**
     * @brief 销毁AST查询器
     * 
     * @param query AST查询器实例
     */
    void (*destroy)(Stru_AstQueryInterface_t* query);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// AST序列化接口
// ============================================================================

/**
 * @brief AST序列化接口
 * 
 * 提供AST树的序列化和反序列化功能，支持JSON、XML等格式。
 */
struct Stru_AstSerializerInterface_t
{
    /**
     * @brief 初始化AST序列化器
     * 
     * @param serializer AST序列化器实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_AstSerializerInterface_t* serializer);
    
    /**
     * @brief 序列化AST到JSON
     * 
     * @param serializer AST序列化器实例
     * @param root 根节点
     * @param buffer 输出缓冲区
     * @param buffer_size 缓冲区大小
     * @return size_t 序列化的字节数（不包括终止空字符）
     */
    size_t (*serialize_to_json)(Stru_AstSerializerInterface_t* serializer,
                               Stru_AstNodeInterface_t* root,
                               char* buffer,
                               size_t buffer_size);
    
    /**
     * @brief 从JSON反序列化AST
     * 
     * @param serializer AST序列化器实例
     * @param json JSON字符串
     * @return Stru_AstNodeInterface_t* 反序列化的AST根节点，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*deserialize_from_json)(Stru_AstSerializerInterface_t* serializer,
                                                     const char* json);
    
    /**
     * @brief 序列化AST到XML
     * 
     * @param serializer AST序列化器实例
     * @param root 根节点
     * @param buffer 输出缓冲区
     * @param buffer_size 缓冲区大小
     * @return size_t 序列化的字节数（不包括终止空字符）
     */
    size_t (*serialize_to_xml)(Stru_AstSerializerInterface_t* serializer,
                              Stru_AstNodeInterface_t* root,
                              char* buffer,
                              size_t buffer_size);
    
    /**
     * @brief 从XML反序列化AST
     * 
     * @param serializer AST序列化器实例
     * @param xml XML字符串
     * @return Stru_AstNodeInterface_t* 反序列化的AST根节点，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*deserialize_from_xml)(Stru_AstSerializerInterface_t* serializer,
                                                    const char* xml);
    
    /**
     * @brief 序列化AST到二进制格式
     * 
     * @param serializer AST序列化器实例
     * @param root 根节点
     * @param buffer 输出缓冲区
     * @param buffer_size 缓冲区大小
     * @return size_t 序列化的字节数
     */
    size_t (*serialize_to_binary)(Stru_AstSerializerInterface_t* serializer,
                                 Stru_AstNodeInterface_t* root,
                                 void* buffer,
                                 size_t buffer_size);
    
    /**
     * @brief 从二进制格式反序列化AST
     * 
     * @param serializer AST序列化器实例
     * @param buffer 二进制数据缓冲区
     * @param buffer_size 缓冲区大小
     * @return Stru_AstNodeInterface_t* 反序列化的AST根节点，失败返回NULL
     */
    Stru_AstNodeInterface_t* (*deserialize_from_binary)(Stru_AstSerializerInterface_t* serializer,
                                                       const void* buffer,
                                                       size_t buffer_size);
    
    /**
     * @brief 销毁AST序列化器
     * 
     * @param serializer AST序列化器实例
     */
    void (*destroy)(Stru_AstSerializerInterface_t* serializer);
    
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
 * @brief 创建AST节点接口实例
 * 
 * @return Stru_AstNodeInterface_t* AST节点接口实例，失败返回NULL
 */
Stru_AstNodeInterface_t* F_create_ast_node_interface(void);

/**
 * @brief 创建AST构建器接口实例
 * 
 * @return Stru_AstBuilderInterface_t* AST构建器接口实例，失败返回NULL
 */
Stru_AstBuilderInterface_t* F_create_ast_builder_interface(void);

/**
 * @brief 创建AST遍历接口实例
 * 
 * @return Stru_AstTraversalInterface_t* AST遍历接口实例，失败返回NULL
 */
Stru_AstTraversalInterface_t* F_create_ast_traversal_interface(void);

/**
 * @brief 创建AST查询接口实例
 * 
 * @return Stru_AstQueryInterface_t* AST查询接口实例，失败返回NULL
 */
Stru_AstQueryInterface_t* F_create_ast_query_interface(void);

/**
 * @brief 创建AST序列化接口实例
 * 
 * @return Stru_AstSerializerInterface_t* AST序列化接口实例，失败返回NULL
 */
Stru_AstSerializerInterface_t* F_create_ast_serializer_interface(void);

/**
 * @brief 销毁AST节点接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_ast_node_interface(Stru_AstNodeInterface_t* interface);

/**
 * @brief 销毁AST构建器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_ast_builder_interface(Stru_AstBuilderInterface_t* interface);

/**
 * @brief 销毁AST遍历接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_ast_traversal_interface(Stru_AstTraversalInterface_t* interface);

/**
 * @brief 销毁AST查询接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_ast_query_interface(Stru_AstQueryInterface_t* interface);

/**
 * @brief 销毁AST序列化接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_ast_serializer_interface(Stru_AstSerializerInterface_t* interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_AST_INTERFACE_H */
