/**
 * @file inheritance_resolver.h
 * @brief CN语言面向对象编程 - 继承关系解析器
 * 
 * 本文件定义了CN语言继承关系解析器的接口，包括：
 * - 继承层次结构节点
 * - 类注册与关系构建
 * - 循环继承检测
 * - 继承链遍历
 */

#ifndef CNLANG_SEMANTICS_INHERITANCE_RESOLVER_H
#define CNLANG_SEMANTICS_INHERITANCE_RESOLVER_H

#include <stdbool.h>
#include <stddef.h>
#include <cnlang/frontend/ast/class_node.h>
#include <cnlang/support/diagnostics.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 继承层次结构节点
 * ============================================================================ */

/**
 * @brief 继承层次结构节点
 * 
 * 表示继承层次中的一个类节点，包含指向派生类的引用
 */
typedef struct CnInheritanceNode {
    CnAstClassDecl *class_decl;              ///< 类声明AST节点
    struct CnInheritanceNode **derived_classes;  ///< 派生类列表
    size_t derived_count;                    ///< 派生类数量
    size_t derived_capacity;                 ///< 派生类列表容量
    int depth;                               ///< 继承深度（0表示根类）
    bool visited;                            ///< 遍历标记（用于循环检测）
} CnInheritanceNode;

/* ============================================================================
 * 继承关系解析器
 * ============================================================================ */

/**
 * @brief 继承关系解析器
 * 
 * 管理所有类的继承关系，提供查询和检测功能
 */
typedef struct CnInheritanceResolver {
    CnInheritanceNode *nodes;                ///< 所有类节点数组
    size_t node_count;                       ///< 节点数量
    size_t node_capacity;                    ///< 节点数组容量
    CnDiagnostics *diagnostics;              ///< 诊断信息收集器
} CnInheritanceResolver;

/* ============================================================================
 * 创建和销毁函数
 * ============================================================================ */

/**
 * @brief 创建继承关系解析器
 * 
 * @param diag 诊断信息收集器
 * @return CnInheritanceResolver* 新创建的解析器，失败返回NULL
 */
CnInheritanceResolver *cn_inheritance_resolver_create(CnDiagnostics *diag);

/**
 * @brief 销毁继承关系解析器
 * 
 * @param resolver 要销毁的解析器
 */
void cn_inheritance_resolver_destroy(CnInheritanceResolver *resolver);

/* ============================================================================
 * 类注册函数
 * ============================================================================ */

/**
 * @brief 注册类到解析器
 * 
 * 将类添加到解析器的类列表中，尚未建立继承关系
 * 
 * @param resolver 解析器
 * @param class_decl 类声明AST节点
 * @return bool 注册成功返回true，失败返回false
 */
bool cn_inheritance_resolver_register(CnInheritanceResolver *resolver,
                                       CnAstClassDecl *class_decl);

/* ============================================================================
 * 继承关系解析函数
 * ============================================================================ */

/**
 * @brief 解析所有继承关系
 * 
 * 遍历所有已注册的类，建立父子类关系
 * 
 * @param resolver 解析器
 * @return bool 解析成功返回true，有错误返回false
 */
bool cn_inheritance_resolver_resolve(CnInheritanceResolver *resolver);

/**
 * @brief 检测循环继承
 * 
 * 检查是否存在循环继承关系
 * 
 * @param resolver 解析器
 * @return bool 存在循环继承返回true，否则返回false
 */
bool cn_inheritance_resolver_check_circular(CnInheritanceResolver *resolver);

/* ============================================================================
 * 查询函数
 * ============================================================================ */

/**
 * @brief 获取类的继承层次节点
 * 
 * @param resolver 解析器
 * @param class_name 类名
 * @return CnInheritanceNode* 找到返回节点指针，未找到返回NULL
 */
CnInheritanceNode *cn_inheritance_resolver_get_node(CnInheritanceResolver *resolver,
                                                     const char *class_name);

/**
 * @brief 获取类的所有基类（包括间接基类）
 * 
 * @param resolver 解析器
 * @param class_decl 类声明
 * @param count 输出参数，返回基类数量
 * @return CnAstClassDecl** 基类数组，需要调用者释放，失败返回NULL
 */
CnAstClassDecl **cn_inheritance_resolver_get_all_bases(CnInheritanceResolver *resolver,
                                                        CnAstClassDecl *class_decl,
                                                        size_t *count);

/**
 * @brief 检查是否为派生类
 * 
 * 判断 derived 类是否直接或间接继承自 base 类
 * 
 * @param resolver 解析器
 * @param derived 派生类名
 * @param base 基类名
 * @return bool 是派生关系返回true，否则返回false
 */
bool cn_inheritance_resolver_is_derived_from(CnInheritanceResolver *resolver,
                                              const char *derived,
                                              const char *base);

/**
 * @brief 获取类的继承深度
 * 
 * @param resolver 解析器
 * @param class_name 类名
 * @return int 继承深度，未找到返回-1
 */
int cn_inheritance_resolver_get_depth(CnInheritanceResolver *resolver,
                                       const char *class_name);

/**
 * @brief 获取直接基类
 *
 * @param resolver 解析器
 * @param class_decl 类声明
 * @param count 输出参数，返回直接基类数量
 * @return CnAstClassDecl** 直接基类数组，需要调用者释放，失败返回NULL
 */
CnAstClassDecl **cn_inheritance_resolver_get_direct_bases(CnInheritanceResolver *resolver,
                                                           CnAstClassDecl *class_decl,
                                                           size_t *count);

/* ============================================================================
 * 多继承语义检查函数
 * ============================================================================ */

/**
 * @brief 多继承检查结果枚举
 */
typedef enum CnMultiInheritCheckResult {
    CN_MULTI_INHERIT_OK,                ///< 检查通过
    CN_MULTI_INHERIT_BASE_NOT_FOUND,    ///< 基类不存在
    CN_MULTI_INHERIT_CIRCULAR,          ///< 循环继承
    CN_MULTI_INHERIT_DUPLICATE_BASE,    ///< 重复基类
    CN_MULTI_INHERIT_PRIVATE_BASE,      ///< 私有基类不可继承
    CN_MULTI_INHERIT_FINAL_BASE         ///< 最终类不可继承
} CnMultiInheritCheckResult;

/**
 * @brief 检查所有基类是否存在
 *
 * 遍历类的所有基类，检查是否都在解析器中注册
 *
 * @param resolver 解析器
 * @param class_decl 类声明
 * @param missing_base_name 输出参数，缺失的基类名（可选）
 * @param missing_base_len 输出参数，缺失基类名长度（可选）
 * @return bool 所有基类都存在返回true
 */
bool cn_inheritance_resolver_check_bases_exist(CnInheritanceResolver *resolver,
                                                CnAstClassDecl *class_decl,
                                                const char **missing_base_name,
                                                size_t *missing_base_len);

/**
 * @brief 检查重复基类
 *
 * 检查类是否直接继承同一类两次
 *
 * @param resolver 解析器
 * @param class_decl 类声明
 * @param duplicate_name 输出参数，重复的基类名（可选）
 * @param duplicate_len 输出参数，重复基类名长度（可选）
 * @return bool 无重复基类返回true
 */
bool cn_inheritance_resolver_check_duplicate_bases(CnInheritanceResolver *resolver,
                                                    CnAstClassDecl *class_decl,
                                                    const char **duplicate_name,
                                                    size_t *duplicate_len);

/**
 * @brief 检查基类访问级别
 *
 * 检查是否尝试继承私有基类或最终类
 *
 * @param resolver 解析器
 * @param class_decl 类声明
 * @param invalid_base_name 输出参数，无效基类名（可选）
 * @param invalid_base_len 输出参数，无效基类名长度（可选）
 * @return bool 所有基类都可继承返回true
 */
bool cn_inheritance_resolver_check_base_access(CnInheritanceResolver *resolver,
                                                CnAstClassDecl *class_decl,
                                                const char **invalid_base_name,
                                                size_t *invalid_base_len);

/**
 * @brief 执行完整的多继承语义检查
 *
 * 执行所有多继承相关的语义检查，包括：
 * - 基类存在性检查
 * - 循环继承检查
 * - 重复基类检查
 * - 基类访问级别检查
 *
 * @param resolver 解析器
 * @param class_decl 类声明
 * @param error_base_name 输出参数，错误相关的基类名（可选）
 * @param error_base_len 输出参数，错误基类名长度（可选）
 * @return CnMultiInheritCheckResult 检查结果
 */
CnMultiInheritCheckResult cn_inheritance_resolver_check_multi_inherit(
    CnInheritanceResolver *resolver,
    CnAstClassDecl *class_decl,
    const char **error_base_name,
    size_t *error_base_len);

/* ============================================================================
 * 方法解析顺序（MRO）函数
 * ============================================================================ */

/**
 * @brief 方法解析顺序列表
 *
 * 存储类的方法解析顺序（C3线性化结果）
 */
typedef struct CnMroList {
    CnAstClassDecl **classes;     ///< 按MRO顺序排列的类数组
    size_t count;                 ///< 类数量
    size_t capacity;              ///< 容量
} CnMroList;

/**
 * @brief 创建MRO列表
 *
 * @param capacity 初始容量
 * @return CnMroList* 新创建的列表，失败返回NULL
 */
CnMroList *cn_mro_list_create(size_t capacity);

/**
 * @brief 销毁MRO列表
 *
 * @param list 要销毁的列表
 */
void cn_mro_list_destroy(CnMroList *list);

/**
 * @brief 计算类的方法解析顺序（C3线性化）
 *
 * 使用C3线性化算法计算类的方法解析顺序
 * 用于处理多继承中的方法查找顺序
 *
 * @param resolver 解析器
 * @param class_decl 类声明
 * @return CnMroList* MRO列表，需要调用者释放，失败返回NULL
 */
CnMroList *cn_inheritance_resolver_compute_mro(CnInheritanceResolver *resolver,
                                                CnAstClassDecl *class_decl);

/**
 * @brief 在MRO列表中查找方法
 *
 * 按照方法解析顺序查找方法定义
 *
 * @param mro MRO列表
 * @param method_name 方法名
 * @param method_name_len 方法名长度
 * @return CnClassMember* 找到的方法，未找到返回NULL
 */
CnClassMember *cn_mro_find_method(CnMroList *mro,
                                   const char *method_name,
                                   size_t method_name_len);

/* ============================================================================
 * 虚继承语义检查函数
 * ============================================================================ */

/**
 * @brief 虚继承检查结果枚举
 */
typedef enum CnVirtualInheritCheckResult {
    CN_VIRTUAL_INHERIT_OK,               ///< 检查通过
    CN_VIRTUAL_INHERIT_AMBIGUOUS_BASE,   ///< 歧义基类（菱形继承未使用虚继承）
    CN_VIRTUAL_INHERIT_CONFLICT_VIRTUAL, ///< 虚继承与非虚继承冲突
    CN_VIRTUAL_INHERIT_BASE_NOT_FOUND    ///< 基类不存在
} CnVirtualInheritCheckResult;

/**
 * @brief 检测菱形继承问题
 *
 * 检查是否存在菱形继承且未使用虚继承的情况
 * 例如：A -> B, A -> C, B -> D, C -> D（如果A不是虚基类则有问题）
 *
 * @param resolver 解析器
 * @param class_decl 类声明
 * @param ambiguous_base_name 输出参数，歧义基类名（可选）
 * @param ambiguous_base_len 输出参数，歧义基类名长度（可选）
 * @return CnVirtualInheritCheckResult 检查结果
 */
CnVirtualInheritCheckResult cn_inheritance_resolver_check_diamond(
    CnInheritanceResolver *resolver,
    CnAstClassDecl *class_decl,
    const char **ambiguous_base_name,
    size_t *ambiguous_base_len);

/**
 * @brief 检查虚继承一致性
 *
 * 确保同一基类在所有继承路径上要么都是虚继承，要么都不是
 *
 * @param resolver 解析器
 * @param class_decl 类声明
 * @param conflict_base_name 输出参数，冲突基类名（可选）
 * @param conflict_base_len 输出参数，冲突基类名长度（可选）
 * @return CnVirtualInheritCheckResult 检查结果
 */
CnVirtualInheritCheckResult cn_inheritance_resolver_check_virtual_consistency(
    CnInheritanceResolver *resolver,
    CnAstClassDecl *class_decl,
    const char **conflict_base_name,
    size_t *conflict_base_len);

/**
 * @brief 获取虚基类列表
 *
 * 收集类中所有虚继承的基类
 *
 * @param resolver 解析器
 * @param class_decl 类声明
 * @param count 输出参数，返回虚基类数量
 * @return CnAstClassDecl** 虚基类数组，需要调用者释放，失败返回NULL
 */
CnAstClassDecl **cn_inheritance_resolver_get_virtual_bases(
    CnInheritanceResolver *resolver,
    CnAstClassDecl *class_decl,
    size_t *count);

/**
 * @brief 检查基类是否为虚基类
 *
 * 判断指定基类是否通过虚继承方式继承
 *
 * @param class_decl 类声明
 * @param base_name 基类名
 * @param base_name_len 基类名长度
 * @return bool 是虚继承返回true
 */
bool cn_inheritance_is_virtual_base(const CnAstClassDecl *class_decl,
                                     const char *base_name,
                                     size_t base_name_len);

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_SEMANTICS_INHERITANCE_RESOLVER_H */
