/**
 * @file class_analyzer.h
 * @brief CN语言面向对象编程 - 类语义分析器
 * 
 * 本文件定义了CN语言类成员变量的语义分析功能，包括：
 * - 类型检查
 * - 符号表绑定
 * - 访问控制检查
 * - 成员名称冲突检测
 */

#ifndef CNLANG_SEMANTICS_CLASS_ANALYZER_H
#define CNLANG_SEMANTICS_CLASS_ANALYZER_H

#include <stdbool.h>
#include <stddef.h>
#include "cnlang/frontend/ast/class_node.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 类语义分析上下文
 * ============================================================================ */

/**
 * @brief 类语义分析上下文结构
 * 
 * 维护类分析过程中的状态信息
 */
typedef struct CnClassAnalyzerContext {
    CnSemScope *symbol_table;       ///< 当前符号表作用域
    CnAstClassDecl *current_class;  ///< 当前正在分析的类
    CnClassMember *current_member;  ///< 当前正在分析的成员
    CnAccessLevel current_access;   ///< 当前访问级别上下文
    CnDiagnostics *diagnostics;     ///< 诊断信息收集器
    struct CnAstProgram *program;   ///< 程序AST（用于查找基类）
} CnClassAnalyzerContext;

/* ============================================================================
 * 初始化和销毁函数
 * ============================================================================ */

/**
 * @brief 初始化类分析上下文
 *
 * @param ctx 要初始化的上下文指针
 * @param symbol_table 符号表作用域
 * @param diagnostics 诊断信息收集器
 * @param program 程序AST（用于查找基类）
 * @return bool 成功返回true
 */
bool cn_class_analyzer_context_init(CnClassAnalyzerContext *ctx,
                                     CnSemScope *symbol_table,
                                     CnDiagnostics *diagnostics,
                                     struct CnAstProgram *program);

/**
 * @brief 清理类分析上下文
 * 
 * @param ctx 要清理的上下文指针
 */
void cn_class_analyzer_context_cleanup(CnClassAnalyzerContext *ctx);

/* ============================================================================
 * 类分析主函数
 * ============================================================================ */

/**
 * @brief 分析类定义
 * 
 * 执行完整的类语义分析，包括：
 * - 检查类名是否有效
 * - 检查基类是否存在
 * - 分析所有成员
 * - 检查成员冲突
 * 
 * @param ctx 分析上下文
 * @param class_decl 类声明AST节点
 * @return bool 分析成功返回true，有错误返回false
 */
bool cn_analyze_class_decl(CnClassAnalyzerContext *ctx, CnAstClassDecl *class_decl);

/**
 * @brief 分析类成员变量
 * 
 * 执行成员变量的语义分析：
 * - 检查变量类型是否有效
 * - 检查变量名是否与类名冲突
 * - 检查变量名是否与其他成员冲突
 * - 将成员变量添加到符号表
 * 
 * @param ctx 分析上下文
 * @param member 成员节点
 * @return bool 分析成功返回true，有错误返回false
 */
bool cn_analyze_class_field(CnClassAnalyzerContext *ctx, CnClassMember *member);

/**
 * @brief 分析类成员函数
 * 
 * 执行成员函数的语义分析：
 * - 检查返回类型是否有效
 * - 检查参数类型是否有效
 * - 检查函数名是否与类名冲突（非构造函数）
 * - 检查重写函数的签名匹配
 * 
 * @param ctx 分析上下文
 * @param member 成员节点
 * @return bool 分析成功返回true，有错误返回false
 */
bool cn_analyze_class_method(CnClassAnalyzerContext *ctx, CnClassMember *member);

/**
 * @brief 分析构造函数
 *
 * 执行构造函数的语义分析：
 * - 检查构造函数名是否与类名相同
 * - 检查构造函数不能有返回类型
 * - 检查构造函数不能是静态的
 * - 检查初始化列表中的成员是否存在
 *
 * @param ctx 分析上下文
 * @param member 成员节点
 * @return bool 分析成功返回true，有错误返回false
 */
bool cn_analyze_constructor(CnClassAnalyzerContext *ctx, CnClassMember *member);

/**
 * @brief 检查构造函数不能有返回类型
 *
 * @param ctx 分析上下文
 * @param member 成员节点
 * @return bool 无返回类型返回true，有返回类型返回false
 */
bool cn_check_constructor_no_return(CnClassAnalyzerContext *ctx, CnClassMember *member);

/**
 * @brief 分析析构函数
 *
 * 执行析构函数的语义分析：
 * - 检查析构函数名是否与类名相同（带~前缀）
 * - 检查析构函数不能有返回类型
 * - 检查析构函数不能有参数
 * - 检查析构函数不能是静态的
 * - 检查析构函数不能是虚函数
 *
 * @param ctx 分析上下文
 * @param member 成员节点
 * @return bool 分析成功返回true，有错误返回false
 */
bool cn_analyze_destructor(CnClassAnalyzerContext *ctx, CnClassMember *member);

/**
 * @brief 检查析构函数不能有参数
 *
 * @param ctx 分析上下文
 * @param member 成员节点
 * @return bool 无参数返回true，有参数返回false
 */
bool cn_check_destructor_no_params(CnClassAnalyzerContext *ctx, CnClassMember *member);

/**
 * @brief 检查析构函数不能有返回类型
 *
 * @param ctx 分析上下文
 * @param member 成员节点
 * @return bool 无返回类型返回true，有返回类型返回false
 */
bool cn_check_destructor_no_return(CnClassAnalyzerContext *ctx, CnClassMember *member);

/**
 * @brief 检查初始化列表中的成员是否存在
 *
 * @param ctx 分析上下文
 * @param member 构造函数成员节点
 * @return bool 所有初始化成员都存在返回true，有错误返回false
 */
bool cn_check_initializer_list(CnClassAnalyzerContext *ctx, CnClassMember *member);

/**
 * @brief 检查初始化表达式类型是否匹配
 *
 * 验证初始化表达式的类型是否与字段类型兼容：
 * - 类型种类必须相同或可隐式转换
 * - 整数类型之间允许隐式转换
 * - 浮点类型允许从整数隐式转换
 *
 * @param ctx 分析上下文
 * @param member_name 成员名称
 * @param init_expr 初始化表达式
 * @return bool 类型匹配返回true，不匹配返回false
 */
bool cn_check_initializer_type(CnClassAnalyzerContext *ctx,
                               const char *member_name,
                               struct CnAstExpr *init_expr);

/**
 * @brief 检查是否有重复初始化
 *
 * 检查初始化列表中是否有重复的成员名
 *
 * @param ctx 分析上下文
 * @param member_names 成员名称数组
 * @param count 成员数量
 * @return bool 无重复返回true，有重复返回false
 */
bool cn_check_duplicate_initialization(CnClassAnalyzerContext *ctx,
                                       const char **member_names,
                                       size_t count);

/**
 * @brief 分析方法参数列表
 *
 * 检查每个参数的类型是否有效：
 * - 参数类型必须已定义
 * - 参数名不能重复
 * - 参数不能与成员变量同名
 *
 * @param ctx 分析上下文
 * @param parameters 参数数组
 * @param param_count 参数数量
 * @return bool 分析成功返回true，有错误返回false
 */
bool cn_analyze_method_params(CnClassAnalyzerContext *ctx,
                               CnAstParameter *parameters,
                               size_t param_count);

/**
 * @brief 检查方法返回类型
 *
 * 验证方法的返回类型是否有效：
 * - 返回类型必须已定义
 * - 构造函数和析构函数不能有返回类型
 *
 * @param ctx 分析上下文
 * @param member 成员节点
 * @return bool 返回类型有效返回true，有错误返回false
 */
bool cn_check_method_return_type(CnClassAnalyzerContext *ctx, CnClassMember *member);

/**
 * @brief 检查方法重写有效性
 *
 * 当is_override为true时，验证：
 * - 基类必须存在同名虚函数
 * - 方法签名必须与基类方法一致
 * - 访问级别不能比基类更严格
 *
 * @param ctx 分析上下文
 * @param member 成员节点
 * @return bool 重写有效返回true，有错误返回false
 */
bool cn_check_method_override(CnClassAnalyzerContext *ctx, CnClassMember *member);

/**
 * @brief 检查虚函数声明有效性
 *
 * 验证虚函数声明是否符合规则：
 * - 静态函数不能是虚函数
 * - 构造函数不能是虚函数
 * - 纯虚函数必须有虚函数标记
 *
 * @param ctx 分析上下文
 * @param member 成员节点
 * @return bool 虚函数声明有效返回true，有错误返回false
 */
bool cn_check_virtual_method(CnClassAnalyzerContext *ctx, CnClassMember *member);

/**
 * @brief 将方法添加到符号表
 *
 * 将方法作为函数符号添加到当前作用域：
 * - 设置符号类型为函数
 * - 记录参数信息
 * - 设置访问权限属性
 *
 * @param ctx 分析上下文
 * @param member 成员节点
 * @return bool 添加成功返回true，失败返回false
 */
bool cn_add_method_to_symbol_table(CnClassAnalyzerContext *ctx, CnClassMember *member);

/* ============================================================================
 * 访问控制检查
 * ============================================================================ */

/**
 * @brief 检查成员访问权限（完整实现）
 *
 * 验证是否可以在当前上下文访问指定成员：
 * - 私有成员：只能在声明它的类内部访问
 * - 保护成员：只能在类内部和派生类中访问
 * - 公开成员：可以在任何地方访问
 *
 * @param ctx 分析上下文
 * @param member 要检查的成员
 * @param accessing_class 尝试访问的类（可为NULL表示外部访问）
 * @return bool 允许访问返回true，拒绝访问返回false
 */
bool cn_check_member_access(CnClassAnalyzerContext *ctx,
                            CnClassMember *member,
                            CnAstClassDecl *accessing_class);

/**
 * @brief 检查是否可以从外部访问成员
 *
 * 只有公开成员可以从外部访问
 *
 * @param member 要检查的成员
 * @return bool 允许访问返回true
 */
bool cn_can_access_from_external(CnClassMember *member);

/**
 * @brief 检查是否可以从派生类访问成员
 *
 * 保护成员和公开成员可以从派生类访问
 *
 * @param member 要检查的成员
 * @param derived_class 派生类声明
 * @return bool 允许访问返回true
 */
bool cn_can_access_from_derived(CnClassMember *member, CnAstClassDecl *derived_class);

/**
 * @brief 检查是否可以从同一类访问成员
 *
 * 类内部可以访问所有成员
 *
 * @param member 要检查的成员
 * @param class_decl 类声明
 * @return bool 允许访问返回true
 */
bool cn_can_access_from_same_class(CnClassMember *member, CnAstClassDecl *class_decl);

/**
 * @brief 检查是否可以从派生类访问保护成员（旧接口，保持兼容）
 *
 * @param member 要检查的成员
 * @param derived_class 派生类
 * @return bool 允许访问返回true
 */
bool cn_check_protected_access(CnClassMember *member, CnAstClassDecl *derived_class);

/**
 * @brief 获取继承后的有效访问级别
 *
 * 根据继承方式和成员原有访问级别，计算派生类中的有效访问级别：
 * - 公开继承：保持基类成员的原有访问级别
 * - 保护继承：基类公开成员变为保护
 * - 私有继承：基类公开和保护成员变为私有
 *
 * @param member_access 成员原有的访问级别
 * @param inheritance_access 继承访问级别
 * @return CnAccessLevel 继承后的有效访问级别
 */
CnAccessLevel cn_get_effective_access(CnAccessLevel member_access,
                                       CnAccessLevel inheritance_access);

/* ============================================================================
 * 自身指针（this）检查
 * ============================================================================ */

/**
 * @brief 检查自身指针使用是否有效
 *
 * 验证自身指针是否在正确的上下文中使用：
 * - 必须在实例方法中使用（非静态方法）
 * - 不能在全局函数中使用
 * - 不能在静态方法中使用
 *
 * @param ctx 分析上下文
 * @param expr 表达式节点（自身指针表达式）
 * @return bool 使用有效返回true，无效返回false
 */
bool cn_check_this_usage(CnClassAnalyzerContext *ctx, struct CnAstExpr *expr);

/**
 * @brief 获取自身指针的类型
 *
 * 返回当前类类型的指针类型
 *
 * @param ctx 分析上下文
 * @return struct CnType* 自身指针的类型，失败返回NULL
 */
struct CnType *cn_get_this_type(CnClassAnalyzerContext *ctx);

/**
 * @brief 检查当前是否在静态方法中
 *
 * @param ctx 分析上下文
 * @return bool 在静态方法中返回true，否则返回false
 */
bool cn_is_in_static_method(CnClassAnalyzerContext *ctx);

/**
 * @brief 检查自身指针是否在静态方法中使用（错误情况）
 *
 * 如果在静态方法中使用自身指针，报告错误
 *
 * @param ctx 分析上下文
 * @return bool 在静态方法中使用返回true（错误），否则返回false
 */
bool cn_check_this_in_static_method(CnClassAnalyzerContext *ctx);

/* ============================================================================
 * 成员查找函数
 * ============================================================================ */

/**
 * @brief 在类中查找成员
 * 
 * @param class_decl 类声明节点
 * @param name 成员名称
 * @return CnClassMember* 找到的成员，未找到返回NULL
 */
CnClassMember *cn_find_class_member(CnAstClassDecl *class_decl, const char *name);

/**
 * @brief 在类及其基类中查找成员（支持继承链查找）
 * 
 * @param class_decl 类声明节点
 * @param name 成员名称
 * @param out_base_class 输出参数：找到成员的类（可为NULL）
 * @return CnClassMember* 找到的成员，未找到返回NULL
 */
CnClassMember *cn_find_class_member_in_hierarchy(CnAstClassDecl *class_decl,
                                                  const char *name,
                                                  CnAstClassDecl **out_base_class);

/**
 * @brief 查找类中的虚函数成员（递归查找基类）
 *
 * @param class_decl 类声明节点
 * @param name 成员名称
 * @param program 程序AST（用于查找基类）
 * @return CnClassMember* 找到的虚函数成员，未找到返回NULL
 */
CnClassMember *cn_find_virtual_member(CnAstClassDecl *class_decl,
                                       const char *name,
                                       struct CnAstProgram *program);

/**
 * @brief 查找类中的方法成员（不限于虚函数）
 *
 * 根据CN语言语法规范，"重写"关键字可以重写任何父类方法，
 * 不要求被重写的方法必须是虚函数。
 *
 * @param class_decl 类声明节点
 * @param name 成员名称
 * @return CnClassMember* 找到的方法成员，未找到返回NULL
 */
CnClassMember *cn_find_base_method_member(CnAstClassDecl *class_decl, const char *name);

/* ============================================================================
 * 冲突检测函数
 * ============================================================================ */

/**
 * @brief 检查成员名称冲突
 * 
 * 检查成员名称是否与以下内容冲突：
 * - 类名
 * - 已定义的其他成员
 * - 基类的私有成员
 * 
 * @param ctx 分析上下文
 * @param name 成员名称
 * @return bool 无冲突返回true，有冲突返回false
 */
bool cn_check_member_conflict(CnClassAnalyzerContext *ctx, const char *name);

/**
 * @brief 检查成员名称是否与类名冲突
 * 
 * @param class_decl 类声明节点
 * @param name 成员名称
 * @return bool 无冲突返回true，有冲突返回false
 */
bool cn_check_name_conflict_with_class(CnAstClassDecl *class_decl, const char *name);

/**
 * @brief 检查两个成员是否签名相同（用于重载检测）
 * 
 * @param member1 第一个成员
 * @param member2 第二个成员
 * @return bool 签名相同返回true
 */
bool cn_check_member_signature_conflict(CnClassMember *member1, CnClassMember *member2);

/* ============================================================================
 * 类型检查辅助函数
 * ============================================================================ */

/**
 * @brief 检查类型是否有效
 * 
 * @param ctx 分析上下文
 * @param type 要检查的类型
 * @return bool 类型有效返回true
 */
bool cn_validate_member_type(CnClassAnalyzerContext *ctx, struct CnType *type);

/**
 * @brief 检查类型是否可以在类成员中使用
 * 
 * 某些类型可能不允许作为成员类型（如不完整类型）
 * 
 * @param ctx 分析上下文
 * @param type 要检查的类型
 * @return bool 允许使用返回true
 */
bool cn_is_valid_member_type(CnClassAnalyzerContext *ctx, struct CnType *type);

/* ============================================================================
 * 符号表操作函数
 * ============================================================================ */

/**
 * @brief 将类成员添加到符号表
 * 
 * @param ctx 分析上下文
 * @param member 成员节点
 * @return bool 添加成功返回true
 */
bool cn_add_member_to_symbol_table(CnClassAnalyzerContext *ctx, CnClassMember *member);

/**
 * @brief 将类本身添加到符号表
 * 
 * @param ctx 分析上下文
 * @param class_decl 类声明节点
 * @return bool 添加成功返回true
 */
bool cn_add_class_to_symbol_table(CnClassAnalyzerContext *ctx, CnAstClassDecl *class_decl);

/* ============================================================================
 * 批量分析函数
 * ============================================================================ */

/**
 * @brief 分析程序中的所有类
 * 
 * 遍历程序中的所有类声明并执行语义分析
 * 
 * @param global_scope 全局作用域
 * @param program 程序AST根节点
 * @param diagnostics 诊断信息收集器
 * @return bool 所有类分析成功返回true
 */
bool cn_analyze_all_classes(CnSemScope *global_scope,
                            struct CnAstProgram *program,
                            CnDiagnostics *diagnostics);

/* ============================================================================
 * 继承语义检查函数
 * ============================================================================ */

/**
 * @brief 检查基类是否存在
 *
 * 在符号表中查找基类声明，验证基类是否已定义
 *
 * @param ctx 分析上下文
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @return bool 基类存在返回true
 */
bool cn_check_base_class_exists(CnClassAnalyzerContext *ctx,
                                 const char *base_name,
                                 size_t base_name_len);

/**
 * @brief 检查循环继承
 *
 * 检测类继承链中是否存在循环引用（A继承B，B继承A）
 *
 * @param ctx 分析上下文
 * @param class_decl 类声明节点
 * @return bool 无循环继承返回true，存在循环继承返回false
 */
bool cn_check_circular_inheritance(CnClassAnalyzerContext *ctx,
                                    CnAstClassDecl *class_decl);

/**
 * @brief 检查基类是否为最终类
 *
 * 最终类（is_final=true）不能被继承
 *
 * @param ctx 分析上下文
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @return bool 基类可被继承返回true，基类是最终类返回false
 */
bool cn_check_base_not_final(CnClassAnalyzerContext *ctx,
                              const char *base_name,
                              size_t base_name_len);

/**
 * @brief 分析类的继承关系
 *
 * 执行完整的继承语义检查：
 * - 检查基类是否存在
 * - 检查循环继承
 * - 检查基类是否为最终类
 *
 * @param ctx 分析上下文
 * @param class_decl 类声明节点
 * @return bool 检查通过返回true
 */
bool cn_analyze_inheritance(CnClassAnalyzerContext *ctx,
                             CnAstClassDecl *class_decl);

/* ============================================================================
 * 虚函数调用语义检查函数
 * ============================================================================ */

/**
 * @brief 检查虚函数调用是否有效
 *
 * 验证调用的方法是否存在且可访问
 *
 * @param ctx 分析上下文
 * @param class_decl 对象所属类的声明
 * @param method_name 方法名
 * @param method_name_len 方法名长度
 * @return bool 调用有效返回true
 */
bool cn_check_virtual_call_valid(CnClassAnalyzerContext *ctx,
                                  CnAstClassDecl *class_decl,
                                  const char *method_name,
                                  size_t method_name_len);

/**
 * @brief 检查虚函数调用的访问权限
 *
 * 验证调用者是否有权限访问该方法
 *
 * @param ctx 分析上下文
 * @param method 方法定义
 * @param calling_class 调用该方法所在的类（可为NULL表示全局调用）
 * @return bool 有访问权限返回true
 */
bool cn_check_virtual_call_access(CnClassAnalyzerContext *ctx,
                                   CnClassMember *method,
                                   CnAstClassDecl *calling_class);

/**
 * @brief 检查虚函数调用的参数数量
 *
 * 验证调用时提供的参数数量是否与方法定义匹配
 *
 * @param ctx 分析上下文
 * @param method 方法定义
 * @param arg_count 调用时提供的参数数量
 * @return bool 参数数量匹配返回true
 */
bool cn_check_virtual_call_args(CnClassAnalyzerContext *ctx,
                                 CnClassMember *method,
                                 size_t arg_count);

/**
 * @brief 检查是否调用了纯虚函数
 *
 * 纯虚函数不能被直接调用，必须在派生类中实现后才能调用
 *
 * @param ctx 分析上下文
 * @param method 方法定义
 * @return bool 可以调用返回true（非纯虚函数）
 */
bool cn_check_pure_virtual_call(CnClassAnalyzerContext *ctx,
                                 CnClassMember *method);

/**
 * @brief 执行完整的虚函数调用语义检查
 *
 * 包括：
 * - 方法存在性检查
 * - 访问权限检查
 * - 参数数量检查
 * - 纯虚函数检查
 *
 * @param ctx 分析上下文
 * @param class_decl 对象所属类的声明
 * @param method_name 方法名
 * @param method_name_len 方法名长度
 * @param arg_count 调用时提供的参数数量
 * @param calling_class 调用该方法所在的类（可为NULL）
 * @return bool 所有检查通过返回true
 */
bool cn_check_virtual_call(CnClassAnalyzerContext *ctx,
                            CnAstClassDecl *class_decl,
                            const char *method_name,
                            size_t method_name_len,
                            size_t arg_count,
                            CnAstClassDecl *calling_class);

/* ============================================================================
 * 抽象类语义检查函数
 * ============================================================================ */

/**
 * @brief 检查类是否为抽象类
 *
 * 检查类是否包含纯虚函数或被标记为抽象类
 *
 * @param class_decl 类声明节点
 * @return bool 是抽象类返回true
 */
bool cn_class_is_abstract(CnAstClassDecl *class_decl);

/**
 * @brief 检查类是否可以实例化
 *
 * 抽象类不能被实例化，必须检查：
 * - 类是否被标记为抽象类
 * - 类是否包含未实现的纯虚函数
 *
 * @param ctx 分析上下文
 * @param class_decl 类声明节点
 * @return bool 可以实例化返回true
 */
bool cn_check_class_instantiable(CnClassAnalyzerContext *ctx,
                                  CnAstClassDecl *class_decl);

/**
 * @brief 检查派生类是否实现了所有纯虚函数
 *
 * 派生类必须实现基类的所有纯虚函数才能实例化
 *
 * @param ctx 分析上下文
 * @param derived_class 派生类声明
 * @param base_class 基类声明
 * @return bool 所有纯虚函数都已实现返回true
 */
bool cn_check_pure_virtual_implementation(CnClassAnalyzerContext *ctx,
                                           CnAstClassDecl *derived_class,
                                           CnAstClassDecl *base_class);

/**
 * @brief 获取类中未实现的纯虚函数列表
 *
 * @param class_decl 类声明节点
 * @param out_names 输出纯虚函数名称数组（可为NULL）
 * @param out_count 输出纯虚函数数量
 * @return bool 成功返回true
 */
bool cn_get_unimplemented_pure_virtuals(CnAstClassDecl *class_decl,
                                         const char **out_names,
                                         size_t *out_count);

/* ============================================================================
 * 接口实现检查函数
 * ============================================================================ */

/**
 * @brief 检查类是否实现了所有接口方法
 *
 * 验证类实现了其声明的所有接口的方法：
 * - 遍历类实现的所有接口
 * - 检查每个接口的方法是否在类中实现
 * - 验证方法签名是否匹配
 *
 * @param ctx 分析上下文
 * @param class_decl 类声明节点
 * @param program 程序AST（用于查找接口定义）
 * @return bool 所有接口方法都已实现返回true
 */
bool cn_check_interface_implementation(CnClassAnalyzerContext *ctx,
                                        CnAstClassDecl *class_decl,
                                        struct CnAstProgram *program);

/**
 * @brief 检查类是否实现了指定接口的所有方法
 *
 * @param ctx 分析上下文
 * @param class_decl 类声明节点
 * @param interface_decl 接口声明节点
 * @return bool 所有方法都已实现返回true
 */
bool cn_check_interface_methods_implemented(CnClassAnalyzerContext *ctx,
                                             CnAstClassDecl *class_decl,
                                             struct CnAstInterfaceDecl *interface_decl);

/**
 * @brief 查找接口定义
 *
 * 在程序的接口列表中查找指定名称的接口
 *
 * @param program 程序AST
 * @param interface_name 接口名称
 * @param interface_name_length 接口名称长度
 * @return struct CnAstInterfaceDecl* 找到的接口声明，未找到返回NULL
 */
struct CnAstInterfaceDecl *cn_find_interface(struct CnAstProgram *program,
                                              const char *interface_name,
                                              size_t interface_name_length);

/**
 * @brief 检查接口方法签名是否匹配
 *
 * 验证类方法与接口方法签名是否一致：
 * - 返回类型必须相同
 * - 参数数量必须相同
 * - 参数类型必须兼容
 *
 * @param class_method 类方法
 * @param interface_method 接口方法
 * @return bool 签名匹配返回true
 */
bool cn_check_interface_method_signature(CnClassMember *class_method,
                                          CnClassMember *interface_method);

/* ============================================================================
 * 类对象布局计算（多继承支持）
 * ============================================================================ */

/**
 * @brief 类对象布局信息结构
 *
 * 存储类的内存布局信息，包括：
 * - 类的总大小
 * - vtable指针偏移量
 * - 各基类子对象的偏移量
 * - 各成员变量的偏移量
 */
typedef struct CnClassLayout {
    size_t total_size;              ///< 类总大小（字节）
    size_t vtable_offset;           ///< vtable指针偏移量（SIZE_MAX表示无vtable）
    bool has_vtable;                ///< 是否有虚函数表
    
    /* 基类布局信息 */
    size_t base_offsets[16];        ///< 基类子对象偏移量数组
    const char *base_names[16];     ///< 基类名称数组
    size_t base_name_lengths[16];   ///< 基类名称长度数组
    bool base_is_virtual[16];       ///< 基类是否虚继承标记
    size_t base_count;              ///< 基类数量
    
    /* 虚基类信息（用于虚继承） */
    bool has_virtual_base;          ///< 是否有虚基类
    size_t vbptr_offset;            ///< 虚基类指针偏移量（SIZE_MAX表示无vbptr）
    size_t vbtable_count;           ///< 虚基类表项数量
    const char *vbtable_names[16];  ///< 虚基类名称数组
    size_t vbtable_name_lengths[16];///< 虚基类名称长度数组
    size_t vbtable_offsets[16];     ///< 虚基类在最终对象中的偏移量
    
    /* 成员布局信息 */
    size_t member_offsets[64];      ///< 成员偏移量数组
    const char *member_names[64];   ///< 成员名称数组
    size_t member_name_lengths[64]; ///< 成员名称长度数组
    size_t member_count;            ///< 成员数量
    
    /* 对齐信息 */
    size_t alignment;               ///< 类对齐要求
} CnClassLayout;

/**
 * @brief 初始化类布局结构
 *
 * @param layout 要初始化的布局结构指针
 * @return bool 成功返回true
 */
bool cn_class_layout_init(CnClassLayout *layout);

/**
 * @brief 清理类布局结构
 *
 * @param layout 要清理的布局结构指针
 */
void cn_class_layout_cleanup(CnClassLayout *layout);

/**
 * @brief 计算类的对象布局
 *
 * 遍历类的继承层次和成员，计算：
 * - 各基类子对象的偏移量
 * - 各成员变量的偏移量
 * - 类的总大小和对齐要求
 *
 * @param layout 输出布局结构
 * @param class_decl 类声明节点
 * @param get_base_layout 获取基类布局的回调函数（可为NULL）
 * @param user_data 回调函数用户数据
 * @return bool 计算成功返回true
 */
bool cn_class_layout_calculate(CnClassLayout *layout,
                                CnAstClassDecl *class_decl,
                                CnClassLayout* (*get_base_layout)(const char *name, size_t name_len, void *user_data),
                                void *user_data);

/**
 * @brief 获取基类子对象的偏移量
 *
 * @param layout 类布局结构
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @return size_t 偏移量，未找到返回SIZE_MAX
 */
size_t cn_class_layout_get_base_offset(const CnClassLayout *layout,
                                        const char *base_name,
                                        size_t base_name_len);

/**
 * @brief 获取成员变量的偏移量
 *
 * @param layout 类布局结构
 * @param member_name 成员名称
 * @param member_name_len 成员名称长度
 * @return size_t 偏移量，未找到返回SIZE_MAX
 */
size_t cn_class_layout_get_member_offset(const CnClassLayout *layout,
                                          const char *member_name,
                                          size_t member_name_len);

/**
 * @brief 计算this指针调整量
 *
 * 当从派生类指针转换为基类指针时，需要调整this指针
 * 例如：C* -> B* 需要加上B在C中的偏移量
 *
 * @param derived_layout 派生类布局
 * @param base_name 目标基类名称
 * @param base_name_len 基类名称长度
 * @return size_t 调整量（字节）
 */
size_t cn_class_layout_compute_this_adjustment(const CnClassLayout *derived_layout,
                                                const char *base_name,
                                                size_t base_name_len);

/**
 * @brief 检查基类子对象是否共享（虚继承）
 *
 * @param class_decl 类声明节点
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @return bool 是虚继承返回true
 */
bool cn_class_is_virtual_base(const CnAstClassDecl *class_decl,
                               const char *base_name,
                               size_t base_name_len);

/**
 * @brief 获取类型的对齐要求
 *
 * @param type 类型指针
 * @return size_t 对齐要求（字节）
 */
size_t cn_type_get_alignment(const struct CnType *type);

/**
 * @brief 获取类型的大小
 *
 * @param type 类型指针
 * @return size_t 大小（字节）
 */
size_t cn_type_get_size(const struct CnType *type);

/**
 * @brief 对齐偏移量到指定边界
 *
 * @param offset 当前偏移量
 * @param alignment 对齐要求
 * @return size_t 对齐后的偏移量
 */
size_t cn_align_offset(size_t offset, size_t alignment);

/* ============================================================================
 * 菱形继承成员访问歧义检测函数
 * ============================================================================ */

/**
 * @brief 成员访问歧义检测结果
 */
typedef enum CnMemberAmbiguityResult {
    CN_MEMBER_AMBIGUITY_OK,           ///< 无歧义，成员唯一
    CN_MEMBER_AMBIGUITY_FOUND,        ///< 存在歧义，多个基类有同名成员
    CN_MEMBER_AMBIGUITY_NOT_FOUND     ///< 成员未找到
} CnMemberAmbiguityResult;

/**
 * @brief 检查成员访问是否存在歧义（菱形继承场景）
 *
 * 在菱形继承中，如果多个基类（非虚基类）有同名成员，访问时会产生歧义。
 * 此函数检查成员是否在多个非虚基类路径中出现。
 *
 * @param class_decl 类声明节点
 * @param member_name 成员名称
 * @param member_name_len 成员名称长度
 * @param ambiguous_class1 输出参数：第一个歧义基类名（可为NULL）
 * @param ambiguous_class2 输出参数：第二个歧义基类名（可为NULL）
 * @return CnMemberAmbiguityResult 检测结果
 */
CnMemberAmbiguityResult cn_check_member_access_ambiguity(
    CnAstClassDecl *class_decl,
    const char *member_name,
    size_t member_name_len,
    const char **ambiguous_class1,
    const char **ambiguous_class2);

/**
 * @brief 在继承链中查找成员的所有来源
 *
 * 查找成员在哪些基类中定义，用于歧义检测
 *
 * @param class_decl 类声明节点
 * @param member_name 成员名称
 * @param member_name_len 成员名称长度
 * @param sources 输出数组：存储找到的成员来源基类
 * @param source_count 输出参数：找到的来源数量
 * @param max_sources 最大来源数量
 * @return bool 找到至少一个来源返回true
 */
bool cn_find_member_sources_in_hierarchy(
    CnAstClassDecl *class_decl,
    const char *member_name,
    size_t member_name_len,
    CnAstClassDecl **sources,
    size_t *source_count,
    size_t max_sources);

/**
 * @brief 检查虚基类成员访问是否需要通过vbptr
 *
 * 如果成员来自虚基类，需要通过vbptr访问
 *
 * @param class_decl 类声明节点
 * @param member_name 成员名称
 * @param member_name_len 成员名称长度
 * @param virtual_base_name 输出参数：虚基类名称（可为NULL）
 * @param virtual_base_len 输出参数：虚基类名称长度（可为NULL）
 * @return bool 成员来自虚基类返回true
 */
bool cn_is_member_from_virtual_base(
    CnAstClassDecl *class_decl,
    const char *member_name,
    size_t member_name_len,
    const char **virtual_base_name,
    size_t *virtual_base_len);

/**
 * @brief 获取虚基类成员的访问路径
 *
 * 返回访问虚基类成员所需的vbptr偏移量信息
 *
 * @param class_decl 类声明节点
 * @param virtual_base_name 虚基类名称
 * @param virtual_base_len 虚基类名称长度
 * @param vbptr_offset 输出参数：vbptr在对象中的偏移量
 * @param vbase_offset 输出参数：虚基类在对象中的偏移量（运行时从vbtable获取）
 * @return bool 成功获取返回true
 */
bool cn_get_virtual_base_access_path(
    CnAstClassDecl *class_decl,
    const char *virtual_base_name,
    size_t virtual_base_len,
    size_t *vbptr_offset,
    size_t *vbase_offset);

/* ============================================================================
 * 抽象类继承检查增强函数
 * ============================================================================ */

/**
 * @brief 通过名称在符号表中查找类声明
 *
 * @param symbol_table 符号表
 * @param name 类名
 * @param name_len 类名长度
 * @return CnAstClassDecl* 找到的类声明，未找到返回NULL
 */
CnAstClassDecl *cn_find_class_decl_by_name(CnSemScope *symbol_table,
                                            const char *name,
                                            size_t name_len);

/**
 * @brief 检查派生类是否实现了基类的所有纯虚函数
 *
 * 遍历基类的所有纯虚函数，检查派生类是否都实现了
 *
 * @param ctx 分析上下文
 * @param derived_class 派生类声明
 * @param base_class 基类声明
 * @return bool 所有纯虚函数都已实现返回true
 */
bool cn_check_derived_implements_all_pure_virtuals(CnClassAnalyzerContext *ctx,
                                                    CnAstClassDecl *derived_class,
                                                    CnAstClassDecl *base_class);

/**
 * @brief 报告未实现的纯虚函数列表
 *
 * 生成详细的错误信息，列出所有未实现的纯虚函数名称
 *
 * @param ctx 分析上下文
 * @param derived_class 派生类声明
 * @param base_class 基类声明
 * @param unimplemented_names 未实现的纯虚函数名称数组
 * @param count 未实现的纯虚函数数量
 */
void cn_report_unimplemented_pure_virtuals(CnClassAnalyzerContext *ctx,
                                            CnAstClassDecl *derived_class,
                                            CnAstClassDecl *base_class,
                                            const char **unimplemented_names,
                                            size_t count);

/**
 * @brief 检查类是否可以实例化（增强版）
 *
 * 检查类及其继承链中的所有纯虚函数是否都已实现
 *
 * @param ctx 分析上下文
 * @param class_decl 类声明节点
 * @param program 程序AST（用于查找基类）
 * @return bool 可以实例化返回true
 */
bool cn_check_class_instantiable_ex(CnClassAnalyzerContext *ctx,
                                     CnAstClassDecl *class_decl,
                                     struct CnAstProgram *program);

/**
 * @brief 在程序中查找类声明
 *
 * @param program 程序AST
 * @param name 类名
 * @param name_len 类名长度
 * @return CnAstClassDecl* 找到的类声明，未找到返回NULL
 */
CnAstClassDecl *cn_find_class_in_program(struct CnAstProgram *program,
                                          const char *name,
                                          size_t name_len);

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_SEMANTICS_CLASS_ANALYZER_H */
