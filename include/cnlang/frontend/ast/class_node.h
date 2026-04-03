/**
 * @file class_node.h
 * @brief CN语言面向对象编程 - 类AST节点定义
 * 
 * 本文件定义了CN语言面向对象编程相关的AST节点，包括：
 * - 类声明节点
 * - 接口声明节点
 * - 类成员节点
 * - 继承信息结构
 */

#ifndef CN_FRONTEND_AST_CLASS_NODE_H
#define CN_FRONTEND_AST_CLASS_NODE_H

#include <stddef.h>
#include <stdbool.h>
#include <cnlang/frontend/ast.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 枚举类型定义
 * ============================================================================ */

/**
 * @brief 访问级别枚举
 * 
 * 定义类成员的访问权限
 */
typedef enum CnAccessLevel {
    CN_ACCESS_PRIVATE,    ///< 私有 - 仅类内部可访问
    CN_ACCESS_PROTECTED,  ///< 保护 - 类内部及派生类可访问
    CN_ACCESS_PUBLIC      ///< 公开 - 任何地方都可访问
} CnAccessLevel;

/**
 * @brief 类成员类型枚举
 * 
 * 区分不同类型的类成员
 */
typedef enum CnMemberKind {
    CN_MEMBER_FIELD,       ///< 成员变量（字段）
    CN_MEMBER_METHOD,      ///< 成员函数（方法）
    CN_MEMBER_CONSTRUCTOR, ///< 构造函数
    CN_MEMBER_DESTRUCTOR   ///< 析构函数
} CnMemberKind;

/* ============================================================================
 * 前向声明
 * ============================================================================ */

struct CnType;
struct CnAstExpr;
struct CnAstBlockStmt;

/* ============================================================================
 * 数据结构定义
 * ============================================================================ */

/**
 * @brief 类成员结构
 * 
 * 表示类的一个成员（字段、方法、构造函数或析构函数）
 */
typedef struct CnClassMember {
    const char *name;               ///< 成员名称
    size_t name_length;             ///< 成员名称长度
    struct CnType *type;            ///< 成员类型（字段类型或方法返回类型）
    CnAccessLevel access;           ///< 访问级别
    CnMemberKind kind;              ///< 成员类型
    
    /* 成员属性标志 */
    bool is_static;                 ///< 是否静态成员
    bool is_virtual;                ///< 是否虚函数（仅方法有效）
    bool is_override;               ///< 是否重写基类方法（仅方法有效）
    bool is_pure_virtual;           ///< 是否纯虚函数（仅方法有效）
    bool is_const;                  ///< 是否常量成员（字段有效）
    
    /* 成员关联的AST节点 */
    struct CnAstExpr *init_expr;    ///< 初始化表达式（字段可用）
    struct CnAstBlockStmt *body;    ///< 函数体（方法、构造函数、析构函数有效）
    
    /* 函数参数（仅方法、构造函数有效） */
    CnAstParameter *parameters;     ///< 参数列表
    size_t parameter_count;         ///< 参数数量
    
    /* 初始化列表（仅构造函数有效） */
    struct CnAstStructFieldInit *initializer_list;  ///< 初始化列表
    size_t initializer_count;       ///< 初始化列表项数量
} CnClassMember;

/**
 * @brief 继承信息结构
 * 
 * 描述一个基类的继承方式
 */
typedef struct CnInheritanceInfo {
    const char *base_class_name;    ///< 基类名称
    size_t base_class_name_length;  ///< 基类名称长度
    bool is_virtual;                ///< 是否虚继承
    CnAccessLevel access;           ///< 继承访问级别
} CnInheritanceInfo;

/* 前向声明（接口实例化结构体） */
struct CnAstInterfaceInstantiation;

/**
 * @brief 类声明AST节点
 *
 * 表示一个完整的类定义
 * 支持实现带模板参数的接口，例如：类 数据项 : 实现 可比较<数据项> { ... }
 */
typedef struct CnAstClassDecl {
    CnAstStmt base;                 ///< 基础语句节点
    const char *name;               ///< 类名
    size_t name_length;             ///< 类名长度
    
    /* 继承信息 */
    CnInheritanceInfo *bases;       ///< 基类数组（支持多继承）
    size_t base_count;              ///< 基类数量
    
    /* 实现的接口列表（阶段17 - 支持接口模板参数） */
    struct CnAstInterfaceInstantiation **implemented_interfaces; ///< 实现的接口实例化数组
    size_t implemented_interface_count;  ///< 实现的接口数量
    
    /* 成员信息 */
    CnClassMember *members;         ///< 成员数组
    size_t member_count;            ///< 成员数量
    
    /* 类属性标志 */
    bool is_abstract;               ///< 是否抽象类
    bool is_interface;              ///< 是否接口（接口是一种特殊的抽象类）
    bool is_final;                  ///< 是否最终类（不可继承）
} CnAstClassDecl;

/**
 * @brief 接口声明AST节点
 *
 * 表示一个接口定义（纯抽象类）
 * 支持模板参数，例如：模板<T> 接口 可比较 { ... }
 */
typedef struct CnAstInterfaceDecl {
    CnAstStmt base;                 ///< 基础语句节点
    const char *name;               ///< 接口名
    size_t name_length;             ///< 接口名长度
    
    /* 模板参数（阶段17 - 接口模板参数支持） */
    CnAstTemplateParams *template_params;   ///< 模板参数列表（可为NULL）
    
    /* 接口方法（全部为纯虚函数） */
    CnClassMember *methods;         ///< 接口方法数组
    size_t method_count;            ///< 方法数量
    
    /* 接口继承 */
    CnInheritanceInfo *base_interfaces; ///< 基接口数组
    size_t base_interface_count;    ///< 基接口数量
} CnAstInterfaceDecl;

/**
 * @brief 接口实例化引用节点
 *
 * 用于类实现接口时指定模板参数
 * 例如：可比较<数据项>、映射<字符串, 整数>
 *
 * 语法：接口名<类型实参列表>
 */
typedef struct CnAstInterfaceInstantiation {
    const char *interface_name;         ///< 接口名称
    size_t interface_name_length;       ///< 接口名称长度
    
    /* 模板实参 */
    struct CnType **type_args;          ///< 类型实参数组
    size_t type_arg_count;              ///< 类型实参数量
    
    /* 源码位置 */
    int line;                           ///< 行号
    int column;                         ///< 列号
} CnAstInterfaceInstantiation;

/* ============================================================================
 * 创建函数
 * ============================================================================ */

/**
 * @brief 创建类声明节点
 * 
 * @param name 类名
 * @param name_length 类名长度
 * @return CnAstClassDecl* 新创建的类声明节点，失败返回NULL
 */
CnAstClassDecl *cn_ast_class_decl_create(const char *name, size_t name_length);

/**
 * @brief 创建接口声明节点
 * 
 * @param name 接口名
 * @param name_length 接口名长度
 * @return CnAstInterfaceDecl* 新创建的接口声明节点，失败返回NULL
 */
CnAstInterfaceDecl *cn_ast_interface_decl_create(const char *name, size_t name_length);

/**
 * @brief 创建接口实例化节点
 *
 * 用于类实现接口时指定模板参数
 *
 * @param interface_name 接口名称
 * @param interface_name_length 接口名称长度
 * @param line 源码行号
 * @param column 源码列号
 * @return CnAstInterfaceInstantiation* 新创建的接口实例化节点，失败返回NULL
 */
CnAstInterfaceInstantiation *cn_ast_interface_instantiation_create(
    const char *interface_name, size_t interface_name_length,
    int line, int column);

/**
 * @brief 向接口实例化添加类型实参
 *
 * @param instantiation 接口实例化节点
 * @param type_arg 类型实参
 * @return int 成功返回0，失败返回-1
 */
int cn_ast_interface_instantiation_add_type_arg(
    CnAstInterfaceInstantiation *instantiation, struct CnType *type_arg);

/**
 * @brief 创建类成员
 * 
 * @param name 成员名称
 * @param name_length 成员名称长度
 * @param kind 成员类型
 * @param access 访问级别
 * @return CnClassMember* 新创建的类成员，失败返回NULL
 */
CnClassMember *cn_ast_class_member_create(const char *name, size_t name_length,
                                          CnMemberKind kind, CnAccessLevel access);

/* ============================================================================
 * 销毁函数
 * ============================================================================ */

/**
 * @brief 销毁类成员
 * 
 * @param member 要销毁的类成员
 */
void cn_ast_class_member_destroy(CnClassMember *member);

/**
 * @brief 销毁类声明节点
 * 
 * @param class_decl 要销毁的类声明节点
 */
void cn_ast_class_decl_destroy(CnAstClassDecl *class_decl);

/**
 * @brief 销毁接口声明节点
 * 
 * @param interface_decl 要销毁的接口声明节点
 */
void cn_ast_interface_decl_destroy(CnAstInterfaceDecl *interface_decl);

/**
 * @brief 销毁接口实例化节点
 *
 * @param instantiation 要销毁的接口实例化节点
 */
void cn_ast_interface_instantiation_destroy(CnAstInterfaceInstantiation *instantiation);

/* ============================================================================
 * 操作函数
 * ============================================================================ */

/**
 * @brief 向类声明添加成员
 * 
 * @param class_decl 类声明节点
 * @param member 要添加的成员
 * @return int 成功返回0，失败返回-1
 */
int cn_ast_class_decl_add_member(CnAstClassDecl *class_decl, CnClassMember *member);

/**
 * @brief 向类声明添加基类
 * 
 * @param class_decl 类声明节点
 * @param base_class_name 基类名称
 * @param base_class_name_length 基类名称长度
 * @param access 继承访问级别
 * @param is_virtual 是否虚继承
 * @return int 成功返回0，失败返回-1
 */
int cn_ast_class_decl_add_base(CnAstClassDecl *class_decl,
                               const char *base_class_name, size_t base_class_name_length,
                               CnAccessLevel access, bool is_virtual);

/**
 * @brief 向类声明添加实现的接口（支持模板参数）
 *
 * @param class_decl 类声明节点
 * @param interface_inst 接口实例化节点（包含接口名和类型实参）
 * @return int 成功返回0，失败返回-1
 */
int cn_ast_class_decl_add_interface(CnAstClassDecl *class_decl,
                                    CnAstInterfaceInstantiation *interface_inst);

/**
 * @brief 向接口声明添加方法
 * 
 * @param interface_decl 接口声明节点
 * @param method 要添加的方法
 * @return int 成功返回0，失败返回-1
 */
int cn_ast_interface_decl_add_method(CnAstInterfaceDecl *interface_decl, CnClassMember *method);

/**
 * @brief 向接口声明添加基接口
 * 
 * @param interface_decl 接口声明节点
 * @param base_interface_name 基接口名称
 * @param base_interface_name_length 基接口名称长度
 * @return int 成功返回0，失败返回-1
 */
int cn_ast_interface_decl_add_base_interface(CnAstInterfaceDecl *interface_decl,
                                             const char *base_interface_name,
                                             size_t base_interface_name_length);

#ifdef __cplusplus
}
#endif

#endif /* CN_FRONTEND_AST_CLASS_NODE_H */
