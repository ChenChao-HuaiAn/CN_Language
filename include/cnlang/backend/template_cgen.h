/**
 * @file template_cgen.h
 * @brief 模板代码生成接口定义
 *
 * 定义模板函数和模板结构体的单态化代码生成接口。
 * 
 * @version 1.0
 * @date 2026-03-29
 */

#ifndef CN_BACKEND_TEMPLATE_CGEN_H
#define CN_BACKEND_TEMPLATE_CGEN_H

#include "cnlang/backend/cgen.h"
#include "cnlang/semantics/template.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 模板函数代码生成
 * ============================================================================ */

/**
 * @brief 生成模板函数实例的C代码
 *
 * 将实例化后的模板函数转换为C函数定义。
 * 函数名使用修饰后的名称：__cn_template_原名称_类型
 *
 * @param ctx 代码生成上下文
 * @param instance 模板实例化项（包含实例化的函数声明）
 * @return 成功返回true，失败返回false
 */
bool cn_cgen_template_function_instance(CnCCodeGenContext *ctx,
                                         const CnTemplateInstance *instance);

/* ============================================================================
 * 模板结构体代码生成
 * ============================================================================ */

/**
 * @brief 生成模板结构体实例的C代码
 *
 * 将实例化后的模板结构体转换为C结构体定义。
 * 结构体名使用修饰后的名称：__cn_template_原名称_类型
 *
 * @param ctx 代码生成上下文
 * @param instance 模板实例化项（包含实例化的结构体声明）
 * @return 成功返回true，失败返回false
 */
bool cn_cgen_template_struct_instance(CnCCodeGenContext *ctx,
                                       const CnTemplateInstance *instance);

/* ============================================================================
 * 批量代码生成
 * ============================================================================ */

/**
 * @brief 生成所有模板实例化的C代码
 *
 * 遍历模板缓存，生成所有已实例化的模板函数和结构体代码。
 * 通常在生成普通代码之前调用，确保模板实例化代码可用。
 *
 * @param ctx 代码生成上下文
 * @param cache 模板实例化缓存
 * @return 成功生成的实例数量
 */
size_t cn_cgen_template_instances(CnCCodeGenContext *ctx,
                                   const CnTemplateCache *cache);

/* ============================================================================
 * 模板实例化表达式处理
 * ============================================================================ */

/**
 * @brief 检查表达式是否为模板实例化调用
 *
 * @param expr 表达式节点
 * @return 是模板实例化返回true
 */
bool cn_cgen_is_template_instantiation(const CnAstExpr *expr);

/**
 * @brief 生成模板实例化表达式的C代码
 *
 * 将模板实例化表达式转换为对实例化函数的调用。
 * 例如：最大值<整数>(1, 2) -> __cn_template_最大值_整数(1, 2)
 *
 * @param ctx 代码生成上下文
 * @param expr 模板实例化表达式
 * @param cache 模板缓存（用于查找实例化名称）
 * @return 成功返回true，失败返回false
 */
bool cn_cgen_template_instantiation_expr(CnCCodeGenContext *ctx,
                                          const CnAstExpr *expr,
                                          const CnTemplateCache *cache);

/**
 * @brief 获取模板实例化的修饰名称
 *
 * 用于在代码生成时获取模板实例化的C兼容名称
 *
 * @param expr 模板实例化表达式
 * @param cache 模板缓存（可为NULL）
 * @return 修饰后的名称字符串（需调用者释放），失败返回NULL
 */
char *cn_cgen_get_template_mangled_name(const CnAstExpr *expr,
                                         const CnTemplateCache *cache);

#ifdef __cplusplus
}
#endif

#endif /* CN_BACKEND_TEMPLATE_CGEN_H */
