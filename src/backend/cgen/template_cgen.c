/**
 * @file template_cgen.c
 * @brief 模板代码生成实现
 *
 * 实现模板函数和模板结构体的单态化代码生成。
 * 将模板实例化后的AST节点转换为C代码。
 */

#include "cnlang/backend/cgen.h"
#include "cnlang/backend/template_cgen.h"
#include "cnlang/semantics/template.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/semantics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

/**
 * @brief 获取模板实例化后的C类型字符串
 *
 * 对于模板参数类型，使用映射表查找具体类型
 *
 * @param type 类型
 * @param type_map 类型映射表（可为NULL）
 * @return C类型字符串
 */
static const char *get_template_c_type_string(CnType *type, const CnTypeMap *type_map) {
    if (!type) return "void";
    
    // 如果是模板参数类型，进行替换
    if (cn_type_is_template_param(type) && type_map) {
        // 获取模板参数名
        const char *param_name = NULL;
        size_t param_name_len = 0;
        
        if (type->kind == CN_TYPE_STRUCT && type->as.struct_type.name) {
            param_name = type->as.struct_type.name;
            param_name_len = type->as.struct_type.name_length;
        }
        
        if (param_name) {
            CnType *concrete_type = cn_type_map_lookup(type_map, param_name, param_name_len);
            if (concrete_type) {
                return get_c_type_string(concrete_type);
            }
        }
    }
    
    // 非模板参数类型，直接返回C类型字符串
    return get_c_type_string(type);
}

/**
 * @brief 生成模板实例化后的函数名
 *
 * 格式：__cn_template_原名称_类型1_类型2_...
 *
 * @param mangled_name 已修饰的名称
 * @return C函数名（直接返回，因为已经是C兼容格式）
 */
static const char *get_template_instance_func_name(const char *mangled_name) {
    return mangled_name ? mangled_name : "__unknown_template";
}

/* ============================================================================
 * 模板函数代码生成
 * ============================================================================ */

/**
 * @brief 生成模板函数实例的参数列表
 *
 * @param ctx 代码生成上下文
 * @param func 实例化的函数声明
 * @param type_map 类型映射表
 */
static void cn_cgen_template_function_params(CnCCodeGenContext *ctx,
                                              CnAstFunctionDecl *func,
                                              const CnTypeMap *type_map) {
    if (!ctx || !func) return;
    
    FILE *out = ctx->output_file;
    
    if (func->parameter_count == 0) {
        fprintf(out, "void");
        return;
    }
    
    for (size_t i = 0; i < func->parameter_count; i++) {
        CnAstParameter *param = &func->parameters[i];
        
        // 获取参数类型（可能需要替换模板参数）
        const char *c_type = get_template_c_type_string(param->declared_type, type_map);
        
        fprintf(out, "%s %.*s", 
                c_type,
                (int)param->name_length, 
                param->name);
        
        if (i < func->parameter_count - 1) {
            fprintf(out, ", ");
        }
    }
}

/**
 * @brief 生成模板函数实例的函数体
 *
 * @param ctx 代码生成上下文
 * @param func 实例化的函数声明
 * @param type_map 类型映射表
 */
static void cn_cgen_template_function_body(CnCCodeGenContext *ctx,
                                            CnAstFunctionDecl *func,
                                            const CnTypeMap *type_map) {
    if (!ctx || !func || !func->body) return;
    
    FILE *out = ctx->output_file;
    
    // 生成函数体中的语句
    // 注意：这里简化处理，实际需要遍历AST生成代码
    // 完整实现需要IR生成阶段支持
    
    fprintf(out, "  /* 模板函数体 - 待完整实现 */\n");
    
    // 遍历函数体中的语句
    for (size_t i = 0; i < func->body->stmt_count; i++) {
        CnAstStmt *stmt = func->body->stmts[i];
        if (!stmt) continue;
        
        // 简化处理：生成占位注释
        fprintf(out, "  // 语句类型: %d\n", stmt->kind);
    }
}

/**
 * @brief 生成模板函数实例的C代码
 *
 * @param ctx 代码生成上下文
 * @param instance 模板实例化项
 * @return 成功返回true
 */
bool cn_cgen_template_function_instance(CnCCodeGenContext *ctx,
                                         const CnTemplateInstance *instance) {
    if (!ctx || !instance || !instance->instantiated_function) return false;
    
    FILE *out = ctx->output_file;
    CnAstFunctionDecl *func = instance->instantiated_function;
    
    // 构建类型映射表（用于生成函数体时替换类型）
    CnTypeMap *type_map = cn_type_map_new();
    if (!type_map) return false;
    
    // 注意：这里需要从实例化信息中获取模板参数映射
    // 简化处理：假设实例化后的函数已经完成类型替换
    
    // 生成函数注释
    fprintf(out, "/**\n");
    fprintf(out, " * @brief 模板函数实例化: %s\n", instance->mangled_name);
    fprintf(out, " * @tparam 原模板: %.*s\n", 
            (int)instance->template_name_length, 
            instance->template_name);
    fprintf(out, " */\n");
    
    // 生成返回类型
    const char *return_type = get_c_type_string(func->return_type);
    fprintf(out, "%s %s(", return_type, instance->mangled_name);
    
    // 生成参数列表
    cn_cgen_template_function_params(ctx, func, type_map);
    
    fprintf(out, ") {\n");
    
    // 生成函数体
    cn_cgen_template_function_body(ctx, func, type_map);
    
    fprintf(out, "}\n\n");
    
    cn_type_map_free(type_map);
    return true;
}

/* ============================================================================
 * 模板结构体代码生成
 * ============================================================================ */

/**
 * @brief 生成模板结构体实例的C代码
 *
 * @param ctx 代码生成上下文
 * @param instance 模板实例化项
 * @return 成功返回true
 */
bool cn_cgen_template_struct_instance(CnCCodeGenContext *ctx,
                                       const CnTemplateInstance *instance) {
    if (!ctx || !instance || !instance->instantiated_struct) return false;
    
    FILE *out = ctx->output_file;
    CnAstStructDecl *struct_decl = instance->instantiated_struct;
    
    // 构建类型映射表
    CnTypeMap *type_map = cn_type_map_new();
    if (!type_map) return false;
    
    // 生成结构体注释
    fprintf(out, "/**\n");
    fprintf(out, " * @brief 模板结构体实例化: %s\n", instance->mangled_name);
    fprintf(out, " * @tparam 原模板: %.*s\n",
            (int)instance->template_name_length,
            instance->template_name);
    fprintf(out, " */\n");
    
    // 生成结构体定义
    fprintf(out, "struct %s {\n", instance->mangled_name);
    
    // 生成字段
    for (size_t i = 0; i < struct_decl->field_count; i++) {
        CnAstStructField *field = &struct_decl->fields[i];
        
        // 获取字段类型（可能需要替换模板参数）
        const char *c_type = get_template_c_type_string(field->field_type, type_map);
        
        fprintf(out, "    %s %.*s;\n",
                c_type,
                (int)field->name_length,
                field->name);
    }
    
    fprintf(out, "};\n\n");
    
    cn_type_map_free(type_map);
    return true;
}

/* ============================================================================
 * 模板实例化批量代码生成
 * ============================================================================ */

/**
 * @brief 生成所有模板实例化的C代码
 *
 * 遍历模板缓存，生成所有已实例化的模板代码
 *
 * @param ctx 代码生成上下文
 * @param cache 模板实例化缓存
 * @return 成功生成的实例数量
 */
size_t cn_cgen_template_instances(CnCCodeGenContext *ctx,
                                   const CnTemplateCache *cache) {
    if (!ctx || !cache) return 0;
    
    size_t generated_count = 0;
    
    fprintf(ctx->output_file, "\n/* ============================================ */\n");
    fprintf(ctx->output_file, "/* 模板实例化代码 */\n");
    fprintf(ctx->output_file, "/* ============================================ */\n\n");
    
    for (size_t i = 0; i < cache->instance_count; i++) {
        CnTemplateInstance *instance = cache->instances[i];
        if (!instance) continue;
        
        bool success = false;
        
        if (instance->instantiated_function) {
            // 生成模板函数实例
            success = cn_cgen_template_function_instance(ctx, instance);
        } else if (instance->instantiated_struct) {
            // 生成模板结构体实例
            success = cn_cgen_template_struct_instance(ctx, instance);
        }
        
        if (success) {
            generated_count++;
        }
    }
    
    return generated_count;
}

/* ============================================================================
 * 模板实例化表达式处理
 * ============================================================================ */

/**
 * @brief 检查表达式是否为模板实例化调用
 *
 * @param expr 表达式节点
 * @return 是模板实例化返回true
 */
bool cn_cgen_is_template_instantiation(const CnAstExpr *expr) {
    if (!expr) return false;
    return expr->kind == CN_AST_EXPR_TEMPLATE_INSTANTIATION;
}

/**
 * @brief 生成模板实例化表达式的C代码
 *
 * 例如：最大值<整数>(1, 2) -> __cn_template_最大值_整数(1, 2)
 *
 * @param ctx 代码生成上下文
 * @param expr 模板实例化表达式
 * @param cache 模板缓存（用于查找实例化名称）
 * @return 成功返回true
 */
bool cn_cgen_template_instantiation_expr(CnCCodeGenContext *ctx,
                                          const CnAstExpr *expr,
                                          const CnTemplateCache *cache) {
    if (!ctx || !expr || expr->kind != CN_AST_EXPR_TEMPLATE_INSTANTIATION) {
        return false;
    }
    
    // 获取模板实例化表达式信息
    CnAstTemplateInstantiationExpr *inst_expr = &expr->as.template_inst;
    
    // 在缓存中查找对应的实例化
    if (cache && inst_expr->template_name) {
        CnTemplateInstance *instance = cn_template_cache_find(
            cache,
            inst_expr->template_name,
            inst_expr->template_name_length,
            inst_expr->type_args,
            inst_expr->type_arg_count
        );
        
        if (instance && instance->mangled_name) {
            // 找到实例化，输出修饰后的名称
            fprintf(ctx->output_file, "%s", instance->mangled_name);
            return true;
        }
    }
    
    // 未找到实例化，生成名称修饰
    char *mangled_name = cn_template_mangle_name(
        inst_expr->template_name,
        inst_expr->template_name_length,
        inst_expr->type_args,
        inst_expr->type_arg_count
    );
    
    if (mangled_name) {
        fprintf(ctx->output_file, "%s", mangled_name);
        free(mangled_name);
        return true;
    }
    
    // 回退：输出原始模板名
    fprintf(ctx->output_file, "%.*s",
            (int)inst_expr->template_name_length,
            inst_expr->template_name);
    return true;
}

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
                                         const CnTemplateCache *cache) {
    if (!expr || expr->kind != CN_AST_EXPR_TEMPLATE_INSTANTIATION) {
        return NULL;
    }
    
    CnAstTemplateInstantiationExpr *inst_expr = &expr->as.template_inst;
    
    // 在缓存中查找
    if (cache && inst_expr->template_name) {
        CnTemplateInstance *instance = cn_template_cache_find(
            cache,
            inst_expr->template_name,
            inst_expr->template_name_length,
            inst_expr->type_args,
            inst_expr->type_arg_count
        );
        
        if (instance && instance->mangled_name) {
            return strdup(instance->mangled_name);
        }
    }
    
    // 生成名称修饰
    return cn_template_mangle_name(
        inst_expr->template_name,
        inst_expr->template_name_length,
        inst_expr->type_args,
        inst_expr->type_arg_count
    );
}
