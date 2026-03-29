/**
 * @file vtable_builder.h
 * @brief CN语言面向对象编程 - 虚函数表构建器
 * 
 * 本文件定义了CN语言虚函数表构建器的接口，包括：
 * - 虚函数表条目结构
 * - 虚函数表结构
 * - 虚函数表构建器
 * - vtable创建、合并、查询功能
 */

#ifndef CNLANG_SEMANTICS_VTABLE_BUILDER_H
#define CNLANG_SEMANTICS_VTABLE_BUILDER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <cnlang/frontend/ast/class_node.h>
#include <cnlang/support/diagnostics.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 前向声明
 * ============================================================================ */

struct CnInheritanceResolver;

/* ============================================================================
 * 虚函数表条目
 * ============================================================================ */

/**
 * @brief 虚函数表条目
 * 
 * 表示虚函数表中的一个条目，包含方法信息和偏移
 */
typedef struct CnVTableEntry {
    const char *method_name;      ///< 方法名
    size_t method_name_length;    ///< 方法名长度
    CnClassMember *method;        ///< 方法定义指针
    size_t offset;                ///< 在vtable中的偏移（索引）
    bool is_pure_virtual;         ///< 是否为纯虚函数
    bool is_override;             ///< 是否重写基类方法
    const char *defined_in_class; ///< 定义该方法的类名
    size_t defined_in_class_len;  ///< 定义该方法的类名长度
} CnVTableEntry;

/* ============================================================================
 * 虚函数表
 * ============================================================================ */

/**
 * @brief 虚函数表
 *
 * 表示一个类的虚函数表，包含所有虚函数的条目
 */
typedef struct CnVTable {
    const char *class_name;           ///< 所属类名
    size_t class_name_length;         ///< 类名长度
    CnVTableEntry *entries;           ///< 条目数组
    size_t entry_count;               ///< 条目数量
    size_t capacity;                  ///< 容量
    struct CnVTable *base_vtable;     ///< 基类vtable（用于继承）
    bool is_complete;                 ///< 是否完整（无纯虚函数）
    
    /* 多继承支持 */
    struct CnVTable **secondary_vtables;  ///< 次要基类vtable数组（多继承）
    size_t secondary_vtable_count;        ///< 次要基类vtable数量
} CnVTable;

/* ============================================================================
 * 虚函数表构建器
 * ============================================================================ */

/**
 * @brief 虚函数表构建器
 * 
 * 管理所有类的虚函数表构建过程
 */
typedef struct CnVTableBuilder {
    CnVTable *vtables;                ///< 所有vtable数组
    size_t vtable_count;              ///< vtable数量
    size_t vtable_capacity;           ///< vtable数组容量
    CnDiagnostics *diagnostics;       ///< 诊断信息收集器
} CnVTableBuilder;

/* ============================================================================
 * 创建和销毁函数
 * ============================================================================ */

/**
 * @brief 创建虚函数表构建器
 * 
 * @param diag 诊断信息收集器
 * @return CnVTableBuilder* 新创建的构建器，失败返回NULL
 */
CnVTableBuilder *cn_vtable_builder_create(CnDiagnostics *diag);

/**
 * @brief 销毁虚函数表构建器
 * 
 * @param builder 要销毁的构建器
 */
void cn_vtable_builder_destroy(CnVTableBuilder *builder);

/* ============================================================================
 * 虚函数表构建函数
 * ============================================================================ */

/**
 * @brief 为类构建虚函数表
 * 
 * 分析类的所有虚函数，构建完整的虚函数表
 * 
 * @param builder 构建器
 * @param class_decl 类声明AST节点
 * @return CnVTable* 构建的虚函数表，失败返回NULL
 */
CnVTable *cn_vtable_build_for_class(CnVTableBuilder *builder,
                                     CnAstClassDecl *class_decl);

/**
 * @brief 为类构建虚函数表（带继承解析）
 * 
 * 分析类的所有虚函数和基类虚函数，构建完整的虚函数表
 * 
 * @param builder 构建器
 * @param class_decl 类声明AST节点
 * @param inheritance_resolver 继承关系解析器（可为NULL）
 * @return CnVTable* 构建的虚函数表，失败返回NULL
 */
CnVTable *cn_vtable_build_for_class_ex(CnVTableBuilder *builder,
                                        CnAstClassDecl *class_decl,
                                        struct CnInheritanceResolver *inheritance_resolver);

/* ============================================================================
 * 虚函数表操作函数
 * ============================================================================ */

/**
 * @brief 创建空的虚函数表
 * 
 * @param class_name 类名
 * @param class_name_length 类名长度
 * @return CnVTable* 新创建的虚函数表，失败返回NULL
 */
CnVTable *cn_vtable_create(const char *class_name, size_t class_name_length);

/**
 * @brief 销毁虚函数表
 * 
 * @param vtable 要销毁的虚函数表
 */
void cn_vtable_destroy(CnVTable *vtable);

/**
 * @brief 添加虚函数条目到vtable
 * 
 * @param vtable 虚函数表
 * @param method 方法定义
 * @return bool 添加成功返回true，失败返回false
 */
bool cn_vtable_add_entry(CnVTable *vtable, CnClassMember *method);

/**
 * @brief 添加虚函数条目（带详细信息）
 * 
 * @param vtable 虚函数表
 * @param method 方法定义
 * @param defined_in_class 定义该方法的类名
 * @param defined_in_class_len 类名长度
 * @return bool 添加成功返回true，失败返回false
 */
bool cn_vtable_add_entry_ex(CnVTable *vtable, CnClassMember *method,
                            const char *defined_in_class, size_t defined_in_class_len);

/**
 * @brief 查找虚函数在vtable中的偏移
 * 
 * @param vtable 虚函数表
 * @param method_name 方法名
 * @param method_name_length 方法名长度
 * @return int 偏移（索引），未找到返回-1
 */
int cn_vtable_find_entry(CnVTable *vtable, const char *method_name, 
                         size_t method_name_length);

/**
 * @brief 合并基类vtable（用于继承）
 * 
 * 将基类的虚函数表合并到派生类的虚函数表中
 * 派生类可以重写基类的虚函数
 * 
 * @param derived 派生类vtable
 * @param base 基类vtable
 * @return bool 合并成功返回true，失败返回false
 */
bool cn_vtable_merge_base(CnVTable *derived, CnVTable *base);

/**
 * @brief 合并多个基类vtable（用于多继承）
 *
 * 将多个基类的虚函数表合并到派生类的虚函数表中
 * 处理方法名冲突，生成正确的vtable索引
 *
 * @param derived 派生类vtable
 * @param base_vtables 基类vtable数组
 * @param base_count 基类数量
 * @return bool 合并成功返回true，失败返回false
 */
bool cn_vtable_merge_multiple_bases(CnVTable *derived,
                                     CnVTable **base_vtables,
                                     size_t base_count);

/**
 * @brief 检测方法名冲突
 *
 * 检查多个基类中是否存在同名方法
 *
 * @param base_vtables 基类vtable数组
 * @param base_count 基类数量
 * @param conflict_name 输出参数，冲突的方法名（可选）
 * @param conflict_len 输出参数，冲突方法名长度（可选）
 * @return bool 存在冲突返回true
 */
bool cn_vtable_detect_method_conflict(CnVTable **base_vtables,
                                       size_t base_count,
                                       const char **conflict_name,
                                       size_t *conflict_len);

/**
 * @brief 获取方法在vtable中的偏移（支持多继承）
 *
 * 查找方法在vtable或其基类vtable中的偏移
 *
 * @param vtable 虚函数表
 * @param method_name 方法名
 * @param method_name_length 方法名长度
 * @param base_index 输出参数，基类索引（0表示主vtable，>0表示次要基类）
 * @return int 偏移（索引），未找到返回-1
 */
int cn_vtable_find_entry_multi(CnVTable *vtable,
                                const char *method_name,
                                size_t method_name_length,
                                size_t *base_index);

/* ============================================================================
 * 虚函数表查询函数
 * ============================================================================ */

/**
 * @brief 获取类的vtable
 * 
 * @param builder 构建器
 * @param class_name 类名
 * @param class_name_length 类名长度
 * @return CnVTable* 虚函数表，未找到返回NULL
 */
CnVTable *cn_vtable_builder_get_vtable(CnVTableBuilder *builder,
                                        const char *class_name,
                                        size_t class_name_length);

/**
 * @brief 检查类是否需要虚函数表
 * 
 * 判断类是否有虚函数或继承自含有虚函数的基类
 * 
 * @param class_decl 类声明
 * @return bool 需要vtable返回true，否则返回false
 */
bool cn_vtable_needs_vtable(CnAstClassDecl *class_decl);

/**
 * @brief 检查类是否为抽象类
 * 
 * 检查虚函数表中是否有未实现的纯虚函数
 * 
 * @param vtable 虚函数表
 * @return bool 是抽象类返回true，否则返回false
 */
bool cn_vtable_is_abstract(CnVTable *vtable);

/**
 * @brief 获取虚函数表中的纯虚函数数量
 * 
 * @param vtable 虚函数表
 * @return size_t 纯虚函数数量
 */
size_t cn_vtable_pure_virtual_count(CnVTable *vtable);

/**
 * @brief 打印虚函数表信息（用于调试）
 * 
 * @param vtable 虚函数表
 * @param out 输出文件
 */
void cn_vtable_print(CnVTable *vtable, FILE *out);

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_SEMANTICS_VTABLE_BUILDER_H */
