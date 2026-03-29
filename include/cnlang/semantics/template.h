/**
 * @file template.h
 * @brief CN语言模板实例化机制接口定义
 *
 * 本文件定义了模板实例化缓存、类型推导和类型替换的核心数据结构和接口。
 * 支持模板函数和模板结构体的实例化。
 *
 * @version 1.0
 * @date 2026-03-29
 */

#ifndef CN_SEMANTICS_TEMPLATE_H
#define CN_SEMANTICS_TEMPLATE_H

#include <stddef.h>
#include <stdbool.h>
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/semantics.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 类型映射表 - 用于模板参数到具体类型的映射
 * ============================================================================ */

/**
 * @brief 类型映射项
 *
 * 存储模板参数名到具体类型的映射关系
 */
typedef struct CnTypeMapEntry {
    const char *param_name;       // 模板参数名（如 "T"）
    size_t param_name_length;     // 参数名长度
    CnType *concrete_type;        // 具体类型
} CnTypeMapEntry;

/**
 * @brief 类型映射表
 *
 * 存储所有模板参数到具体类型的映射
 */
typedef struct CnTypeMap {
    CnTypeMapEntry *entries;      // 映射项数组
    size_t entry_count;           // 映射项数量
    size_t capacity;              // 容量
} CnTypeMap;

/* ============================================================================
 * 模板实例化缓存
 * ============================================================================ */

/**
 * @brief 模板实例化缓存项
 *
 * 存储一个已实例化的模板实例
 */
typedef struct CnTemplateInstance {
    char *mangled_name;           // 修饰后的唯一名称
    size_t mangled_name_length;   // 名称长度
    
    // 原模板信息
    const char *template_name;    // 原模板名称
    size_t template_name_length;  // 模板名称长度
    
    // 类型实参
    CnType **type_args;           // 类型实参数组
    size_t type_arg_count;        // 类型实参数量
    
    // 实例化结果（根据模板类型，只有一个非空）
    CnAstFunctionDecl *instantiated_function;  // 实例化的函数（函数模板）
    CnAstStructDecl *instantiated_struct;      // 实例化的结构体（结构体模板）
    CnType *instantiated_type;                 // 实例化的类型
} CnTemplateInstance;

/**
 * @brief 模板实例化缓存表
 *
 * 管理所有已实例化的模板，避免重复实例化
 */
typedef struct CnTemplateCache {
    CnTemplateInstance **instances;  // 实例数组
    size_t instance_count;           // 实例数量
    size_t capacity;                 // 容量
} CnTemplateCache;

/* ============================================================================
 * 模板注册表
 * ============================================================================ */

/**
 * @brief 模板注册表项
 *
 * 存储一个模板定义的信息
 */
typedef struct CnTemplateRegistryEntry {
    const char *name;             // 模板名称
    size_t name_length;           // 名称长度
    CnAstTemplateParams *params;  // 模板参数列表
    
    // 模板定义（根据类型，只有一个非空）
    CnAstTemplateFunctionDecl *template_func;   // 模板函数定义
    CnAstTemplateStructDecl *template_struct;   // 模板结构体定义
    
    bool is_function;             // 是否为函数模板
} CnTemplateRegistryEntry;

/**
 * @brief 模板注册表
 *
 * 存储程序中所有模板定义
 */
typedef struct CnTemplateRegistry {
    CnTemplateRegistryEntry *entries;  // 注册项数组
    size_t entry_count;                // 注册项数量
    size_t capacity;                   // 容量
} CnTemplateRegistry;

/* ============================================================================
 * 类型映射表接口
 * ============================================================================ */

/**
 * @brief 创建类型映射表
 * @return 新创建的类型映射表，失败返回NULL
 */
CnTypeMap *cn_type_map_new(void);

/**
 * @brief 释放类型映射表
 * @param map 要释放的类型映射表
 */
void cn_type_map_free(CnTypeMap *map);

/**
 * @brief 向映射表中添加类型映射
 * @param map 类型映射表
 * @param param_name 模板参数名
 * @param param_name_len 参数名长度
 * @param concrete_type 具体类型
 * @return 成功返回true，失败返回false
 */
bool cn_type_map_insert(CnTypeMap *map, 
                        const char *param_name, 
                        size_t param_name_len,
                        CnType *concrete_type);

/**
 * @brief 在映射表中查找类型
 * @param map 类型映射表
 * @param param_name 模板参数名
 * @param param_name_len 参数名长度
 * @return 找到返回具体类型，未找到返回NULL
 */
CnType *cn_type_map_lookup(const CnTypeMap *map,
                           const char *param_name,
                           size_t param_name_len);

/* ============================================================================
 * 模板实例化缓存接口
 * ============================================================================ */

/**
 * @brief 创建模板实例化缓存
 * @return 新创建的缓存，失败返回NULL
 */
CnTemplateCache *cn_template_cache_new(void);

/**
 * @brief 释放模板实例化缓存
 * @param cache 要释放的缓存
 */
void cn_template_cache_free(CnTemplateCache *cache);

/**
 * @brief 在缓存中查找已实例化的模板
 * @param cache 模板缓存
 * @param template_name 模板名称
 * @param template_name_len 名称长度
 * @param type_args 类型实参数组
 * @param type_arg_count 类型实参数量
 * @return 找到返回实例化项，未找到返回NULL
 */
CnTemplateInstance *cn_template_cache_find(const CnTemplateCache *cache,
                                           const char *template_name,
                                           size_t template_name_len,
                                           CnType **type_args,
                                           size_t type_arg_count);

/**
 * @brief 向缓存中添加新实例化
 * @param cache 模板缓存
 * @param instance 实例化项
 * @return 成功返回true，失败返回false
 */
bool cn_template_cache_add(CnTemplateCache *cache, CnTemplateInstance *instance);

/* ============================================================================
 * 模板注册表接口
 * ============================================================================ */

/**
 * @brief 创建模板注册表
 * @return 新创建的注册表，失败返回NULL
 */
CnTemplateRegistry *cn_template_registry_new(void);

/**
 * @brief 释放模板注册表
 * @param registry 要释放的注册表
 */
void cn_template_registry_free(CnTemplateRegistry *registry);

/**
 * @brief 注册模板函数
 * @param registry 模板注册表
 * @param template_func 模板函数声明
 * @return 成功返回true，失败返回false
 */
bool cn_template_registry_register_function(CnTemplateRegistry *registry,
                                            CnAstTemplateFunctionDecl *template_func);

/**
 * @brief 注册模板结构体
 * @param registry 模板注册表
 * @param template_struct 模板结构体声明
 * @return 成功返回true，失败返回false
 */
bool cn_template_registry_register_struct(CnTemplateRegistry *registry,
                                          CnAstTemplateStructDecl *template_struct);

/**
 * @brief 在注册表中查找模板
 * @param registry 模板注册表
 * @param name 模板名称
 * @param name_len 名称长度
 * @return 找到返回注册项，未找到返回NULL
 */
CnTemplateRegistryEntry *cn_template_registry_lookup(const CnTemplateRegistry *registry,
                                                     const char *name,
                                                     size_t name_len);

/* ============================================================================
 * 类型推导接口
 * ============================================================================ */

/**
 * @brief 从函数调用实参推导模板参数类型
 *
 * 例如：最大值(1, 2) -> 推导 T = 整数
 *
 * @param template_params 模板形参列表
 * @param func_params 函数形参列表
 * @param func_param_count 函数形参数量
 * @param call_args 函数调用实参表达式数组
 * @param call_arg_count 实参数量
 * @param[out] out_type_args 输出的类型实参数组（需调用者释放）
 * @return 推导成功返回true，失败返回false
 */
bool cn_template_deduce_types(CnAstTemplateParams *template_params,
                              CnAstParameter *func_params,
                              size_t func_param_count,
                              CnAstExpr **call_args,
                              size_t call_arg_count,
                              CnType ***out_type_args);

/**
 * @brief 从单个参数进行类型推导
 *
 * 用于模式匹配，如 T* 匹配 整数* -> T = 整数
 *
 * @param param_type 函数参数类型（可能包含模板参数）
 * @param arg_type 实参类型
 * @param type_map 类型映射表（输入/输出）
 * @return 推导成功返回true，失败返回false
 */
bool cn_template_deduce_from_param(CnType *param_type,
                                   CnType *arg_type,
                                   CnTypeMap *type_map);

/* ============================================================================
 * 类型替换接口
 * ============================================================================ */

/**
 * @brief 替换类型中的模板参数
 *
 * 例如：T* + {T -> 整数} -> 整数*
 *
 * @param type 原类型（可能包含模板参数）
 * @param type_map 类型映射表
 * @return 替换后的新类型，失败返回原类型
 */
CnType *cn_type_substitute(CnType *type, const CnTypeMap *type_map);

/**
 * @brief 替换表达式中的类型引用
 *
 * 遍历表达式树，替换所有模板参数类型引用
 *
 * @param expr 表达式节点
 * @param type_map 类型映射表
 * @return 成功返回true
 */
bool cn_type_substitute_expr(CnAstExpr *expr, const CnTypeMap *type_map);

/**
 * @brief 替换语句中的类型引用
 *
 * 遍历语句树，替换所有模板参数类型引用
 *
 * @param stmt 语句节点
 * @param type_map 类型映射表
 * @return 成功返回true
 */
bool cn_type_substitute_stmt(CnAstStmt *stmt, const CnTypeMap *type_map);

/**
 * @brief 替换语句块中的类型引用
 *
 * @param block 语句块
 * @param type_map 类型映射表
 * @return 成功返回true
 */
bool cn_type_substitute_block(CnAstBlockStmt *block, const CnTypeMap *type_map);

/* ============================================================================
 * 模板实例化接口
 * ============================================================================ */

/**
 * @brief 实例化模板函数
 *
 * @param template_func 模板函数声明
 * @param type_args 类型实参数组
 * @param type_arg_count 类型实参数量
 * @param cache 实例化缓存（用于检查是否已实例化）
 * @return 实例化的函数声明，失败返回NULL
 */
CnAstFunctionDecl *cn_template_instantiate_function(
    CnAstTemplateFunctionDecl *template_func,
    CnType **type_args,
    size_t type_arg_count,
    CnTemplateCache *cache);

/**
 * @brief 实例化模板结构体
 *
 * @param template_struct 模板结构体声明
 * @param type_args 类型实参数组
 * @param type_arg_count 类型实参数量
 * @param cache 实例化缓存（用于检查是否已实例化）
 * @return 实例化的结构体声明，失败返回NULL
 */
CnAstStructDecl *cn_template_instantiate_struct(
    CnAstTemplateStructDecl *template_struct,
    CnType **type_args,
    size_t type_arg_count,
    CnTemplateCache *cache);

/**
 * @brief 生成模板实例的唯一名称
 *
 * 格式：__cn_template_原名称_类型1_类型2_...
 * 例如：最大值<整数> -> __cn_template_最大值_整数
 *
 * @param template_name 模板名称
 * @param template_name_len 名称长度
 * @param type_args 类型实参数组
 * @param type_arg_count 类型实参数量
 * @return 生成的名称字符串（需调用者释放）
 */
char *cn_template_mangle_name(const char *template_name,
                              size_t template_name_len,
                              CnType **type_args,
                              size_t type_arg_count);

/**
 * @brief 获取类型的名称字符串
 *
 * @param type 类型
 * @return 类型名称字符串（不需释放）
 */
const char *cn_type_get_name(CnType *type);

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

/**
 * @brief 检查类型是否为模板参数类型
 *
 * @param type 类型
 * @return 是模板参数类型返回true
 */
bool cn_type_is_template_param(CnType *type);

/**
 * @brief 比较两个类型数组是否相等
 *
 * @param types1 类型数组1
 * @param count1 数组1长度
 * @param types2 类型数组2
 * @param count2 数组2长度
 * @return 相等返回true
 */
bool cn_type_array_equals(CnType **types1, size_t count1,
                          CnType **types2, size_t count2);

/**
 * @brief 复制类型数组
 *
 * @param types 源类型数组
 * @param count 数组长度
 * @return 新的类型数组（需调用者释放）
 */
CnType **cn_type_array_copy(CnType **types, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* CN_SEMANTICS_TEMPLATE_H */
