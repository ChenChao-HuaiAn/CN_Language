/**
 * @file class_cgen.c
 * @brief CN语言面向对象编程 - 类代码生成实现
 *
 * 本文件实现了CN语言类和接口的C代码生成功能，将类AST节点转换为C代码。
 */

#include "cnlang/backend/cgen/class_cgen.h"
#include "cnlang/backend/cgen.h"
#include "cnlang/frontend/ast/class_node.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/frontend/token.h"
#include "cnlang/semantics/vtable_builder.h"
#include "cnlang/semantics/class_analyzer.h"
#include "cnlang/semantics/template.h"
#include "cnlang/runtime/type_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 获取C类型字符串（从CnType）
 *
 * 注意：此函数声明在cgen.c中，这里通过extern引用
 */
extern const char *get_c_type_string(CnType *type);

/**
 * @brief 检查表达式是否为字符串类型
 *
 * 检查表达式本身的类型，或者对于二元表达式检查操作数类型
 */
static bool is_string_expr(CnAstExpr *expr) {
    if (!expr) return false;
    
    // 检查表达式本身的类型
    if (expr->type && expr->type->kind == CN_TYPE_STRING) {
        return true;
    }
    
    // 对于字符串字面量，直接返回true
    if (expr->kind == CN_AST_EXPR_STRING_LITERAL) {
        return true;
    }
    
    // 对于成员访问，检查成员类型
    if (expr->kind == CN_AST_EXPR_MEMBER_ACCESS) {
        // 成员访问的类型可能在 type 字段中
        if (expr->type && expr->type->kind == CN_TYPE_STRING) {
            return true;
        }
    }
    
    // 对于二元表达式（加法），递归检查是否为字符串拼接
    if (expr->kind == CN_AST_EXPR_BINARY && expr->as.binary.op == CN_AST_BINARY_OP_ADD) {
        // 如果任一操作数是字符串，则结果也是字符串
        if (is_string_expr(expr->as.binary.left) || is_string_expr(expr->as.binary.right)) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 检查二元表达式是否为字符串拼接
 */
static bool is_string_concat(CnAstExpr *expr) {
    if (!expr || expr->kind != CN_AST_EXPR_BINARY) return false;
    if (expr->as.binary.op != CN_AST_BINARY_OP_ADD) return false;
    
    // 检查表达式类型
    if (expr->type && expr->type->kind == CN_TYPE_STRING) {
        return true;
    }
    
    // 检查任一操作数是否为字符串（包括嵌套的字符串拼接）
    if (is_string_expr(expr->as.binary.left) || is_string_expr(expr->as.binary.right)) {
        return true;
    }
    
    return false;
}

/**
 * @brief 生成初始化表达式的C代码（简化版）
 *
 * 前向声明，实现在后面
 */
static void cgen_init_expr_simple(FILE *out, CnAstExpr *expr);

/**
 * @brief 检查标识符名称是否是当前方法的参数
 *
 * @param ctx 代码生成上下文
 * @param name 标识符名称
 * @param name_length 名称长度
 * @return true 如果是参数名，false 否则
 */
static bool is_parameter_name(CnCCodeGenContext *ctx, const char *name, size_t name_length) {
    if (!ctx || !ctx->current_method || !name) return false;
    
    CnClassMember *method = ctx->current_method;
    for (size_t i = 0; i < method->parameter_count; i++) {
        CnAstParameter *param = &method->parameters[i];
        if (param->name_length == name_length &&
            memcmp(param->name, name, name_length) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 在类层次结构中查找成员变量
 *
 * 检查成员是否属于当前类或其基类
 *
 * @param class_decl 类声明
 * @param member_name 成员名称
 * @param member_name_length 成员名称长度
 * @param out_base_class_name 输出：如果成员来自基类，返回基类名
 * @param out_base_class_name_length 输出：基类名长度
 * @return true 如果找到成员，false 否则
 */
static bool find_member_in_class_hierarchy(CnAstClassDecl *class_decl,
                                           const char *member_name, size_t member_name_length,
                                           const char **out_base_class_name, size_t *out_base_class_name_length) {
    if (!class_decl || !member_name) return false;
    
    // 首先在当前类中查找
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_FIELD && !member->is_static) {
            if (member->name_length == member_name_length &&
                memcmp(member->name, member_name, member_name_length) == 0) {
                // 成员在当前类中
                return true;
            }
        }
    }
    
    // 在基类中查找
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        // 注意：这里需要访问 program 来查找基类定义
        // 简化处理：假设成员来自第一个基类（单继承情况）
        // 对于多继承，需要更复杂的查找逻辑
        if (out_base_class_name && out_base_class_name_length) {
            *out_base_class_name = base_info->base_class_name;
            *out_base_class_name_length = base_info->base_class_name_length;
        }
        // 返回 true 表示成员来自基类
        return true;
    }
    
    return false;
}

/**
 * @brief 输出缩进空格
 */
static void print_indent(FILE *out, int indent) {
    for (int i = 0; i < indent; i++) {
        fprintf(out, "    ");
    }
}

/**
 * @brief 检查类是否有虚函数
 */
static bool class_has_virtual_methods(CnAstClassDecl *class_decl) {
    if (!class_decl) return false;
    
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD && member->is_virtual) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 检查类是否有虚函数或继承自含有虚函数的基类
 */
static bool class_needs_vtable(CnAstClassDecl *class_decl) {
    // 使用vtable_builder模块的函数
    return cn_vtable_needs_vtable(class_decl);
}

/**
 * @brief 生成成员变量的C类型声明
 *
 * 注意：静态成员变量不放在结构体中，单独生成全局变量
 */
static void cgen_member_field(FILE *out, CnClassMember *member, int indent) {
    if (!member || member->kind != CN_MEMBER_FIELD) return;
    
    // 静态成员变量不放在结构体中
    if (member->is_static) {
        return;
    }
    
    print_indent(out, indent);
    
    // 生成类型和名称
    const char *type_str = "int";  // 默认类型
    if (member->type) {
        type_str = get_c_type_string(member->type);
    }
    
    fprintf(out, "%s %.*s", type_str,
            (int)member->name_length, member->name);
    
    // 如果是常量且有初始值，可以在这里处理
    // 但C结构体不支持直接初始化，需要构造函数处理
    
    fprintf(out, ";\n");
}

/**
 * @brief 生成静态成员变量的全局变量声明
 *
 * 静态成员变量生成为全局变量，命名格式：类名_静态成员名
 * 例如：静态 整数 实例数; -> static int Counter_实例数;
 */
static void cgen_static_field_decl(FILE *out, CnAstClassDecl *class_decl,
                                    CnClassMember *member) {
    if (!out || !member || member->kind != CN_MEMBER_FIELD || !member->is_static) return;
    
    // 生成类型
    const char *type_str = "int";  // 默认类型
    if (member->type) {
        type_str = get_c_type_string(member->type);
    }
    
    // 静态成员变量命名：类名_成员名
    fprintf(out, "static %s %.*s_%.*s",
            type_str,
            (int)class_decl->name_length, class_decl->name,
            (int)member->name_length, member->name);
    
    // 如果有初始值，生成初始化
    if (member->init_expr) {
        fprintf(out, " = ");
        cgen_init_expr_simple(out, member->init_expr);
    }
    
    fprintf(out, ";\n");
}

/**
 * @brief 生成所有静态成员变量的声明
 */
static void cgen_static_fields(FILE *out, CnAstClassDecl *class_decl) {
    if (!out || !class_decl) return;
    
    bool has_static = false;
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_FIELD && member->is_static) {
            has_static = true;
            break;
        }
    }
    
    if (!has_static) return;
    
    fprintf(out, "/* 静态成员变量 */\n");
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_FIELD && member->is_static) {
            cgen_static_field_decl(out, class_decl, member);
        }
    }
    fprintf(out, "\n");
}

/**
 * @brief 生成构造函数的前向声明
 */
static void cgen_constructor_forward_decl(FILE *out, CnAstClassDecl *class_decl,
                                           CnClassMember *constructor) {
    if (!constructor || constructor->kind != CN_MEMBER_CONSTRUCTOR) return;
    
    // 构造函数返回类指针类型
    fprintf(out, "%.*s* %.*s_construct(",
            (int)class_decl->name_length, class_decl->name,
            (int)class_decl->name_length, class_decl->name);
    
    // 第一个参数是 self 指针（构造函数需要返回新对象）
    fprintf(out, "%.*s* self",
            (int)class_decl->name_length, class_decl->name);
    
    // 其他参数
    for (size_t i = 0; i < constructor->parameter_count; i++) {
        CnAstParameter *param = &constructor->parameters[i];
        const char *param_type = "int";
        if (param->declared_type) {
            param_type = get_c_type_string(param->declared_type);
        }
        fprintf(out, ", %s %.*s", param_type,
                (int)param->name_length, param->name);
    }
    
    fprintf(out, ");\n");
}

/**
 * @brief 生成析构函数的前向声明
 */
static void cgen_destructor_forward_decl(FILE *out, CnAstClassDecl *class_decl,
                                          CnClassMember *destructor) {
    if (!destructor || destructor->kind != CN_MEMBER_DESTRUCTOR) return;
    
    // 析构函数返回void，参数只有self指针
    fprintf(out, "void %.*s_destruct(%.*s* self);\n",
            (int)class_decl->name_length, class_decl->name,
            (int)class_decl->name_length, class_decl->name);
}

/**
 * @brief 生成成员函数的前向声明
 *
 * 静态方法不需要 self 指针参数
 */
static void cgen_method_forward_decl(FILE *out, CnAstClassDecl *class_decl,
                                       CnClassMember *method) {
    if (!method || method->kind != CN_MEMBER_METHOD) return;
    
    // 返回类型
    const char *return_type = "void";
    if (method->type) {
        return_type = get_c_type_string(method->type);
    }
    
    // 函数名：类名_方法名
    fprintf(out, "%s %.*s_%.*s(",
            return_type,
            (int)class_decl->name_length, class_decl->name,
            (int)method->name_length, method->name);
    
    // 静态方法不需要 self 指针参数
    if (!method->is_static) {
        // 参数列表：第一个参数是 self 指针
        fprintf(out, "struct %.*s* self",
                (int)class_decl->name_length, class_decl->name);
    }
    
    // 其他参数
    for (size_t i = 0; i < method->parameter_count; i++) {
        CnAstParameter *param = &method->parameters[i];
        const char *param_type = "int";
        if (param->declared_type) {
            param_type = get_c_type_string(param->declared_type);
        }
        // 非静态方法需要逗号分隔，静态方法第一个参数不需要逗号
        if (!method->is_static || i > 0) {
            fprintf(out, ", ");
        }
        fprintf(out, "%s %.*s", param_type,
                (int)param->name_length, param->name);
    }
    
    // 静态方法如果没有参数，需要添加 void
    if (method->is_static && method->parameter_count == 0) {
        fprintf(out, "void");
    }
    
    fprintf(out, ");\n");
}

/**
 * @brief 生成初始化表达式的C代码（简化版）
 *
 * @param out 输出文件
 * @param expr 表达式节点
 */
static void cgen_init_expr_simple(FILE *out, CnAstExpr *expr) {
    if (!out || !expr) {
        fprintf(out, "0");
        return;
    }
    
    switch (expr->kind) {
        case CN_AST_EXPR_INTEGER_LITERAL:
            fprintf(out, "%lld", expr->as.integer_literal.value);
            break;
        case CN_AST_EXPR_FLOAT_LITERAL:
            fprintf(out, "%f", expr->as.float_literal.value);
            break;
        case CN_AST_EXPR_STRING_LITERAL:
            fprintf(out, "\"");
            for (const char *p = expr->as.string_literal.value; p && *p; p++) {
                switch (*p) {
                    case '\n': fprintf(out, "\\n"); break;
                    case '\t': fprintf(out, "\\t"); break;
                    case '\r': fprintf(out, "\\r"); break;
                    case '\\': fprintf(out, "\\\\"); break;
                    case '"': fprintf(out, "\\\""); break;
                    default: fprintf(out, "%c", *p); break;
                }
            }
            fprintf(out, "\"");
            break;
        case CN_AST_EXPR_BOOL_LITERAL:
            fprintf(out, "%s", expr->as.bool_literal.value ? "1" : "0");
            break;
        case CN_AST_EXPR_IDENTIFIER:
            // 标识符引用（参数或变量）
            fprintf(out, "%.*s", (int)expr->as.identifier.name_length, expr->as.identifier.name);
            break;
        case CN_AST_EXPR_BINARY:
            // 检查是否为字符串拼接
            if (is_string_concat(expr)) {
                fprintf(out, "cn_rt_string_concat(");
                cgen_init_expr_simple(out, expr->as.binary.left);
                fprintf(out, ", ");
                cgen_init_expr_simple(out, expr->as.binary.right);
                fprintf(out, ")");
                break;
            }
            // 二元表达式
            fprintf(out, "(");
            cgen_init_expr_simple(out, expr->as.binary.left);
            switch (expr->as.binary.op) {
                case CN_AST_BINARY_OP_ADD: fprintf(out, " + "); break;
                case CN_AST_BINARY_OP_SUB: fprintf(out, " - "); break;
                case CN_AST_BINARY_OP_MUL: fprintf(out, " * "); break;
                case CN_AST_BINARY_OP_DIV: fprintf(out, " / "); break;
                case CN_AST_BINARY_OP_MOD: fprintf(out, " %% "); break;
                case CN_AST_BINARY_OP_EQ: fprintf(out, " == "); break;
                case CN_AST_BINARY_OP_NE: fprintf(out, " != "); break;
                case CN_AST_BINARY_OP_LT: fprintf(out, " < "); break;
                case CN_AST_BINARY_OP_LE: fprintf(out, " <= "); break;
                case CN_AST_BINARY_OP_GT: fprintf(out, " > "); break;
                case CN_AST_BINARY_OP_GE: fprintf(out, " >= "); break;
                case CN_AST_BINARY_OP_BITWISE_AND: fprintf(out, " & "); break;
                case CN_AST_BINARY_OP_BITWISE_OR: fprintf(out, " | "); break;
                case CN_AST_BINARY_OP_BITWISE_XOR: fprintf(out, " ^ "); break;
                case CN_AST_BINARY_OP_LEFT_SHIFT: fprintf(out, " << "); break;
                case CN_AST_BINARY_OP_RIGHT_SHIFT: fprintf(out, " >> "); break;
                default: fprintf(out, " ? "); break;
            }
            cgen_init_expr_simple(out, expr->as.binary.right);
            fprintf(out, ")");
            break;
        default:
            fprintf(out, "0");
            break;
    }
}

/**
 * @brief 生成语句的C代码（用于构造函数/析构函数/方法体）
 *
 * @param ctx 代码生成上下文
 * @param stmt 语句节点
 * @param indent 缩进级别
 */
static void cgen_stmt_in_method(CnCCodeGenContext *ctx, CnAstStmt *stmt, int indent);

/**
 * @brief 生成语句块的C代码
 *
 * @param ctx 代码生成上下文
 * @param block 语句块节点
 * @param indent 缩进级别
 */
static void cgen_block_in_method(CnCCodeGenContext *ctx, CnAstBlockStmt *block, int indent) {
    if (!ctx || !ctx->output_file || !block) return;
    
    FILE *out = ctx->output_file;
    
    for (size_t i = 0; i < block->stmt_count; i++) {
        cgen_stmt_in_method(ctx, block->stmts[i], indent);
    }
}

/**
 * @brief 生成表达式的C代码（用于方法体内）
 *
 * @param ctx 代码生成上下文
 * @param expr 表达式节点
 */
static void cgen_expr_in_method(CnCCodeGenContext *ctx, CnAstExpr *expr) {
    if (!ctx || !ctx->output_file || !expr) {
        fprintf(ctx->output_file, "0");
        return;
    }
    
    FILE *out = ctx->output_file;
    
    switch (expr->kind) {
        case CN_AST_EXPR_INTEGER_LITERAL:
            fprintf(out, "%lld", expr->as.integer_literal.value);
            break;
        case CN_AST_EXPR_FLOAT_LITERAL:
            fprintf(out, "%f", expr->as.float_literal.value);
            break;
        case CN_AST_EXPR_STRING_LITERAL:
            fprintf(out, "\"");
            for (const char *p = expr->as.string_literal.value; p && *p; p++) {
                switch (*p) {
                    case '\n': fprintf(out, "\\n"); break;
                    case '\t': fprintf(out, "\\t"); break;
                    case '\r': fprintf(out, "\\r"); break;
                    case '\\': fprintf(out, "\\\\"); break;
                    case '"': fprintf(out, "\\\""); break;
                    default: fprintf(out, "%c", *p); break;
                }
            }
            fprintf(out, "\"");
            break;
        case CN_AST_EXPR_BOOL_LITERAL:
            fprintf(out, "%s", expr->as.bool_literal.value ? "1" : "0");
            break;
        case CN_AST_EXPR_IDENTIFIER:
            // 检查是否为自身指针
            if (expr->is_this_pointer) {
                fprintf(out, "self");
            } else if (expr->is_base_pointer) {
                // 基类指针：需要生成基类访问代码
                // 简化处理：输出 "base" 标识符，后续在成员访问中处理
                fprintf(out, "base");
            } else {
                // 【修复问题1】直接输出标识符名称（参数名或局部变量名）
                // 参数名会在函数签名中定义，直接使用即可
                fprintf(out, "%.*s", (int)expr->as.identifier.name_length, expr->as.identifier.name);
            }
            break;
        case CN_AST_EXPR_MEMBER_ACCESS:
            // 成员访问：obj.member 或 ptr->member 或 类名.静态成员 或 基类.成员
            if (expr->as.member.is_static_member && expr->as.member.class_name) {
                // 静态成员访问：生成 类名_成员名
                fprintf(out, "%.*s_%.*s",
                        (int)expr->as.member.class_name_length, expr->as.member.class_name,
                        (int)expr->as.member.member_name_length, expr->as.member.member_name);
            } else if (expr->as.member.object && expr->as.member.object->is_this_pointer) {
                // 【修复问题2】self->member 需要检查成员是否来自基类
                // 支持多层继承：递归查找基类链
                
                // 首先检查当前类是否有该成员
                bool member_in_current_class = false;
                if (ctx->current_class) {
                    for (size_t i = 0; i < ctx->current_class->member_count; i++) {
                        CnClassMember *member = &ctx->current_class->members[i];
                        if (member->kind == CN_MEMBER_FIELD && !member->is_static) {
                            if (member->name_length == expr->as.member.member_name_length &&
                                memcmp(member->name, expr->as.member.member_name, member->name_length) == 0) {
                                member_in_current_class = true;
                                break;
                            }
                        }
                    }
                }
                
                if (member_in_current_class) {
                    // 成员在当前类中，直接生成 self->member
                    fprintf(out, "self->%.*s",
                            (int)expr->as.member.member_name_length, expr->as.member.member_name);
                } else if (ctx->current_class && ctx->current_class->base_count > 0) {
                    // 成员不在当前类，递归查找基类链
                    // 构建基类访问路径，如 self->学生_base.人员_base.姓名
                    char base_path[256] = {0};
                    bool found = false;
                    
                    // 递归查找基类链
                    CnAstClassDecl *search_class = ctx->current_class;
                    while (!found && search_class && search_class->base_count > 0) {
                        CnInheritanceInfo *base_info = &search_class->bases[0];
                        
                        // 添加基类路径
                        char segment[64];
                        snprintf(segment, sizeof(segment), "%.*s_base.",
                                 (int)base_info->base_class_name_length, base_info->base_class_name);
                        strncat(base_path, segment, sizeof(base_path) - strlen(base_path) - 1);
                        
                        // 在基类中查找成员
                        if (ctx->program) {
                            for (size_t j = 0; j < ctx->program->class_count; j++) {
                                CnAstStmt *stmt = ctx->program->classes[j];
                                if (stmt->kind == CN_AST_STMT_CLASS_DECL) {
                                    CnAstClassDecl *base_class = stmt->as.class_decl;
                                    if (base_class->name_length == base_info->base_class_name_length &&
                                        memcmp(base_class->name, base_info->base_class_name, base_class->name_length) == 0) {
                                        // 在基类中查找成员
                                        for (size_t k = 0; k < base_class->member_count; k++) {
                                            CnClassMember *m = &base_class->members[k];
                                            if (m->kind == CN_MEMBER_FIELD && !m->is_static) {
                                                if (m->name_length == expr->as.member.member_name_length &&
                                                    memcmp(m->name, expr->as.member.member_name, m->name_length) == 0) {
                                                    found = true;
                                                    break;
                                                }
                                            }
                                        }
                                        if (!found) {
                                            // 继续在更深的基类中查找
                                            search_class = base_class;
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        
                        if (found) break;
                    }
                    
                    // 生成访问代码
                    if (found && strlen(base_path) > 0) {
                        // 移除末尾的点号
                        size_t len = strlen(base_path);
                        if (len > 0 && base_path[len-1] == '.') {
                            base_path[len-1] = '\0';
                        }
                        fprintf(out, "self->%s.%.*s", base_path,
                                (int)expr->as.member.member_name_length, expr->as.member.member_name);
                    } else {
                        // 未找到，使用直接基类
                        CnInheritanceInfo *base_info = &ctx->current_class->bases[0];
                        fprintf(out, "self->%.*s_base.%.*s",
                                (int)base_info->base_class_name_length, base_info->base_class_name,
                                (int)expr->as.member.member_name_length, expr->as.member.member_name);
                    }
                } else {
                    // 没有基类，生成默认代码
                    fprintf(out, "self->%.*s",
                            (int)expr->as.member.member_name_length, expr->as.member.member_name);
                }
            } else if (expr->as.member.object && expr->as.member.object->is_base_pointer) {
                // 基类访问：base->member 或 基类方法调用
                // 生成基类指针转换代码
                if (ctx->current_class && ctx->current_class->base_count > 0) {
                    CnInheritanceInfo *base_info = &ctx->current_class->bases[0];
                    fprintf(out, "((struct %.*s*)((char*)self + %.*s_%.*s_OFFSET))->%.*s",
                            (int)base_info->base_class_name_length, base_info->base_class_name,
                            (int)ctx->current_class->name_length, ctx->current_class->name,
                            (int)base_info->base_class_name_length, base_info->base_class_name,
                            (int)expr->as.member.member_name_length, expr->as.member.member_name);
                } else {
                    // 没有基类，生成错误占位符
                    fprintf(out, "/* 错误: 当前类没有基类 */ %.*s",
                            (int)expr->as.member.member_name_length, expr->as.member.member_name);
                }
            } else {
                cgen_expr_in_method(ctx, expr->as.member.object);
                if (expr->as.member.is_arrow) {
                    fprintf(out, "->");
                } else {
                    fprintf(out, ".");
                }
                fprintf(out, "%.*s",
                        (int)expr->as.member.member_name_length, expr->as.member.member_name);
            }
            break;
        case CN_AST_EXPR_BINARY:
            // 检查是否为字符串拼接
            if (is_string_concat(expr)) {
                // 生成 cn_rt_string_concat 调用
                // 对于嵌套的字符串拼接，需要递归处理
                fprintf(out, "cn_rt_string_concat(");
                // 左操作数：如果是字符串拼接，递归生成；否则直接生成
                if (is_string_concat(expr->as.binary.left)) {
                    cgen_expr_in_method(ctx, expr->as.binary.left);
                } else {
                    cgen_expr_in_method(ctx, expr->as.binary.left);
                }
                fprintf(out, ", ");
                // 右操作数：如果是字符串拼接，递归生成；否则直接生成
                if (is_string_concat(expr->as.binary.right)) {
                    cgen_expr_in_method(ctx, expr->as.binary.right);
                } else {
                    cgen_expr_in_method(ctx, expr->as.binary.right);
                }
                fprintf(out, ")");
                break;
            }
            // 原有的数值运算逻辑
            fprintf(out, "(");
            cgen_expr_in_method(ctx, expr->as.binary.left);
            switch (expr->as.binary.op) {
                case CN_AST_BINARY_OP_ADD: fprintf(out, " + "); break;
                case CN_AST_BINARY_OP_SUB: fprintf(out, " - "); break;
                case CN_AST_BINARY_OP_MUL: fprintf(out, " * "); break;
                case CN_AST_BINARY_OP_DIV: fprintf(out, " / "); break;
                case CN_AST_BINARY_OP_MOD: fprintf(out, " %% "); break;
                case CN_AST_BINARY_OP_EQ: fprintf(out, " == "); break;
                case CN_AST_BINARY_OP_NE: fprintf(out, " != "); break;
                case CN_AST_BINARY_OP_LT: fprintf(out, " < "); break;
                case CN_AST_BINARY_OP_LE: fprintf(out, " <= "); break;
                case CN_AST_BINARY_OP_GT: fprintf(out, " > "); break;
                case CN_AST_BINARY_OP_GE: fprintf(out, " >= "); break;
                case CN_AST_BINARY_OP_BITWISE_AND: fprintf(out, " & "); break;
                case CN_AST_BINARY_OP_BITWISE_OR: fprintf(out, " | "); break;
                case CN_AST_BINARY_OP_BITWISE_XOR: fprintf(out, " ^ "); break;
                case CN_AST_BINARY_OP_LEFT_SHIFT: fprintf(out, " << "); break;
                case CN_AST_BINARY_OP_RIGHT_SHIFT: fprintf(out, " >> "); break;
                default: fprintf(out, " ? "); break;
            }
            cgen_expr_in_method(ctx, expr->as.binary.right);
            fprintf(out, ")");
            break;
        case CN_AST_EXPR_LOGICAL:
            // 逻辑表达式
            fprintf(out, "(");
            cgen_expr_in_method(ctx, expr->as.logical.left);
            switch (expr->as.logical.op) {
                case CN_AST_LOGICAL_OP_AND: fprintf(out, " && "); break;
                case CN_AST_LOGICAL_OP_OR: fprintf(out, " || "); break;
                default: fprintf(out, " ? "); break;
            }
            cgen_expr_in_method(ctx, expr->as.logical.right);
            fprintf(out, ")");
            break;
        case CN_AST_EXPR_UNARY:
            switch (expr->as.unary.op) {
                case CN_AST_UNARY_OP_MINUS: fprintf(out, "-"); break;
                case CN_AST_UNARY_OP_NOT: fprintf(out, "!"); break;
                case CN_AST_UNARY_OP_BITWISE_NOT: fprintf(out, "~"); break;
                case CN_AST_UNARY_OP_PRE_INC: fprintf(out, "++"); break;
                case CN_AST_UNARY_OP_PRE_DEC: fprintf(out, "--"); break;
                case CN_AST_UNARY_OP_POST_INC:
                    cgen_expr_in_method(ctx, expr->as.unary.operand);
                    fprintf(out, "++");
                    return;
                case CN_AST_UNARY_OP_POST_DEC:
                    cgen_expr_in_method(ctx, expr->as.unary.operand);
                    fprintf(out, "--");
                    return;
                default: break;
            }
            cgen_expr_in_method(ctx, expr->as.unary.operand);
            break;
        case CN_AST_EXPR_CALL:
            // 函数调用 - 检查是否为方法调用
            if (expr->as.call.callee->kind == CN_AST_EXPR_MEMBER_ACCESS &&
                expr->as.call.callee->as.member.object) {
                
                const char *method_name = expr->as.call.callee->as.member.member_name;
                size_t method_name_len = expr->as.call.callee->as.member.member_name_length;
                
                // 检查是否为基类方法调用（base.方法名）
                if (expr->as.call.callee->as.member.object->is_base_pointer) {
                    // 基类方法调用：基类.方法名(参数)
                    if (ctx->current_class && ctx->current_class->base_count > 0) {
                        CnInheritanceInfo *base_info = &ctx->current_class->bases[0];
                        
                        // 生成基类方法调用（直接转换this指针）
                        fprintf(out, "%.*s_%.*s((struct %.*s*)((char*)self + %.*s_%.*s_OFFSET)",
                                (int)base_info->base_class_name_length, base_info->base_class_name,
                                (int)method_name_len, method_name,
                                (int)base_info->base_class_name_length, base_info->base_class_name,
                                (int)ctx->current_class->name_length, ctx->current_class->name,
                                (int)base_info->base_class_name_length, base_info->base_class_name);
                        
                        // 输出参数
                        for (size_t i = 0; i < expr->as.call.argument_count; i++) {
                            fprintf(out, ", ");
                            cgen_expr_in_method(ctx, expr->as.call.arguments[i]);
                        }
                        
                        fprintf(out, ")");
                    } else {
                        // 没有基类，生成错误占位符
                        fprintf(out, "/* 错误: 当前类没有基类 */ 0");
                    }
                }
                // 检查是否为自身方法调用（自身.方法名）
                else if (expr->as.call.callee->as.member.object->is_this_pointer) {
                    // 自身方法调用：需要查找方法属于哪个类
                    // 首先在当前类查找，如果没有则在基类查找
                    bool found_in_current = false;
                    bool found_in_base = false;
                    char base_class_name[128] = {0};
                    size_t base_class_name_len = 0;
                    
                    // 在当前类查找方法
                    if (ctx->current_class) {
                        for (size_t i = 0; i < ctx->current_class->member_count; i++) {
                            CnClassMember *member = &ctx->current_class->members[i];
                            if (member->kind == CN_MEMBER_METHOD && !member->is_static) {
                                if (member->name_length == method_name_len &&
                                    memcmp(member->name, method_name, method_name_len) == 0) {
                                    found_in_current = true;
                                    break;
                                }
                            }
                        }
                        
                        // 如果当前类没有，在基类链中查找
                        if (!found_in_current && ctx->current_class->base_count > 0) {
                            CnAstClassDecl *search_class = ctx->current_class;
                            while (!found_in_base && search_class && search_class->base_count > 0) {
                                CnInheritanceInfo *base_info = &search_class->bases[0];
                                
                                // 在程序中查找基类定义
                                if (ctx->program) {
                                    for (size_t j = 0; j < ctx->program->class_count; j++) {
                                        CnAstStmt *stmt = ctx->program->classes[j];
                                        if (stmt->kind == CN_AST_STMT_CLASS_DECL) {
                                            CnAstClassDecl *base_class = stmt->as.class_decl;
                                            if (base_class->name_length == base_info->base_class_name_length &&
                                                memcmp(base_class->name, base_info->base_class_name, base_class->name_length) == 0) {
                                                // 在基类中查找方法
                                                for (size_t k = 0; k < base_class->member_count; k++) {
                                                    CnClassMember *m = &base_class->members[k];
                                                    if (m->kind == CN_MEMBER_METHOD && !m->is_static) {
                                                        if (m->name_length == method_name_len &&
                                                            memcmp(m->name, method_name, method_name_len) == 0) {
                                                            found_in_base = true;
                                                            base_class_name_len = base_info->base_class_name_length;
                                                            memcpy(base_class_name, base_info->base_class_name, base_class_name_len);
                                                            break;
                                                        }
                                                    }
                                                }
                                                if (!found_in_base) {
                                                    // 继续在更深的基类中查找
                                                    search_class = base_class;
                                                }
                                                break;
                                            }
                                        }
                                    }
                                }
                                if (found_in_base) break;
                            }
                        }
                    }
                    
                    if (found_in_current) {
                        // 当前类的方法：类名_方法名(self, 参数...)
                        fprintf(out, "%.*s_%.*s(self",
                                (int)ctx->current_class->name_length, ctx->current_class->name,
                                (int)method_name_len, method_name);
                    } else if (found_in_base) {
                        // 基类的方法：基类名_方法名(&self->基类名_base, 参数...)
                        fprintf(out, "%.*s_%.*s(&self->%.*s_base",
                                (int)base_class_name_len, base_class_name,
                                (int)method_name_len, method_name,
                                (int)base_class_name_len, base_class_name);
                    } else {
                        // 未找到方法，使用当前类名生成（可能后续会报错）
                        fprintf(out, "%.*s_%.*s(self",
                                (int)ctx->current_class->name_length, ctx->current_class->name,
                                (int)method_name_len, method_name);
                    }
                    
                    // 输出参数
                    for (size_t i = 0; i < expr->as.call.argument_count; i++) {
                        fprintf(out, ", ");
                        cgen_expr_in_method(ctx, expr->as.call.arguments[i]);
                    }
                    
                    fprintf(out, ")");
                } else {
                    // 其他成员访问调用（如 obj.method()）
                    cgen_expr_in_method(ctx, expr->as.call.callee);
                    fprintf(out, "(");
                    for (size_t i = 0; i < expr->as.call.argument_count; i++) {
                        if (i > 0) fprintf(out, ", ");
                        cgen_expr_in_method(ctx, expr->as.call.arguments[i]);
                    }
                    fprintf(out, ")");
                }
            } else {
                // 普通函数调用
                cgen_expr_in_method(ctx, expr->as.call.callee);
                fprintf(out, "(");
                for (size_t i = 0; i < expr->as.call.argument_count; i++) {
                    if (i > 0) fprintf(out, ", ");
                    cgen_expr_in_method(ctx, expr->as.call.arguments[i]);
                }
                fprintf(out, ")");
            }
            break;
        case CN_AST_EXPR_ASSIGN:
            // 赋值表达式
            cgen_expr_in_method(ctx, expr->as.assign.target);
            fprintf(out, " = ");
            cgen_expr_in_method(ctx, expr->as.assign.value);
            break;
        case CN_AST_EXPR_TERNARY:
            fprintf(out, "(");
            cgen_expr_in_method(ctx, expr->as.ternary.condition);
            fprintf(out, " ? ");
            cgen_expr_in_method(ctx, expr->as.ternary.true_expr);
            fprintf(out, " : ");
            cgen_expr_in_method(ctx, expr->as.ternary.false_expr);
            fprintf(out, ")");
            break;
        default:
            fprintf(out, "0");
            break;
    }
}

/**
 * @brief 生成语句的C代码（用于构造函数/析构函数/方法体）
 *
 * @param ctx 代码生成上下文
 * @param stmt 语句节点
 * @param indent 缩进级别
 */
static void cgen_stmt_in_method(CnCCodeGenContext *ctx, CnAstStmt *stmt, int indent) {
    if (!ctx || !ctx->output_file || !stmt) return;
    
    FILE *out = ctx->output_file;
    
    // 输出缩进
    print_indent(out, indent);
    
    switch (stmt->kind) {
        case CN_AST_STMT_VAR_DECL: {
            // 变量声明
            CnAstVarDecl *var_decl = &stmt->as.var_decl;
            const char *type_str = "int";
            if (var_decl->declared_type) {
                type_str = get_c_type_string(var_decl->declared_type);
            }
            fprintf(out, "%s %.*s", type_str,
                    (int)var_decl->name_length, var_decl->name);
            if (var_decl->initializer) {
                fprintf(out, " = ");
                cgen_expr_in_method(ctx, var_decl->initializer);
            }
            fprintf(out, ";\n");
            break;
        }
        case CN_AST_STMT_EXPR: {
            // 表达式语句
            cgen_expr_in_method(ctx, stmt->as.expr.expr);
            fprintf(out, ";\n");
            break;
        }
        case CN_AST_STMT_RETURN: {
            // 返回语句
            CnAstReturnStmt *return_stmt = &stmt->as.return_stmt;
            fprintf(out, "return");
            if (return_stmt->expr) {
                fprintf(out, " ");
                cgen_expr_in_method(ctx, return_stmt->expr);
            }
            fprintf(out, ";\n");
            break;
        }
        case CN_AST_STMT_IF: {
            // 条件语句
            CnAstIfStmt *if_stmt = &stmt->as.if_stmt;
            fprintf(out, "if (");
            cgen_expr_in_method(ctx, if_stmt->condition);
            fprintf(out, ") {\n");
            if (if_stmt->then_block) {
                cgen_block_in_method(ctx, if_stmt->then_block, indent + 1);
            }
            print_indent(out, indent);
            fprintf(out, "}");
            if (if_stmt->else_block) {
                fprintf(out, " else {\n");
                cgen_block_in_method(ctx, if_stmt->else_block, indent + 1);
                print_indent(out, indent);
                fprintf(out, "}");
            }
            fprintf(out, "\n");
            break;
        }
        case CN_AST_STMT_WHILE: {
            // while循环
            CnAstWhileStmt *while_stmt = &stmt->as.while_stmt;
            fprintf(out, "while (");
            cgen_expr_in_method(ctx, while_stmt->condition);
            fprintf(out, ") {\n");
            if (while_stmt->body) {
                cgen_block_in_method(ctx, while_stmt->body, indent + 1);
            }
            print_indent(out, indent);
            fprintf(out, "}\n");
            break;
        }
        case CN_AST_STMT_FOR: {
            // for循环
            CnAstForStmt *for_stmt = &stmt->as.for_stmt;
            fprintf(out, "for (");
            if (for_stmt->init) {
                // 简化处理：只支持变量声明初始化
                if (for_stmt->init->kind == CN_AST_STMT_VAR_DECL) {
                    CnAstVarDecl *var_decl = &for_stmt->init->as.var_decl;
                    const char *type_str = "int";
                    if (var_decl->declared_type) {
                        type_str = get_c_type_string(var_decl->declared_type);
                    }
                    fprintf(out, "%s %.*s", type_str,
                            (int)var_decl->name_length, var_decl->name);
                    if (var_decl->initializer) {
                        fprintf(out, " = ");
                        cgen_expr_in_method(ctx, var_decl->initializer);
                    }
                }
            }
            fprintf(out, "; ");
            if (for_stmt->condition) {
                cgen_expr_in_method(ctx, for_stmt->condition);
            }
            fprintf(out, "; ");
            if (for_stmt->update) {
                cgen_expr_in_method(ctx, for_stmt->update);
            }
            fprintf(out, ") {\n");
            if (for_stmt->body) {
                cgen_block_in_method(ctx, for_stmt->body, indent + 1);
            }
            print_indent(out, indent);
            fprintf(out, "}\n");
            break;
        }
        case CN_AST_STMT_BREAK:
            fprintf(out, "break;\n");
            break;
        case CN_AST_STMT_CONTINUE:
            fprintf(out, "continue;\n");
            break;
        case CN_AST_STMT_BLOCK:
            // 嵌套块
            fprintf(out, "{\n");
            cgen_block_in_method(ctx, stmt->as.block, indent + 1);
            print_indent(out, indent);
            fprintf(out, "}\n");
            break;
        default:
            fprintf(out, "/* 未实现的语句类型: %d */\n", stmt->kind);
            break;
    }
}

/**
 * @brief 生成初始化列表代码
 *
 * 将初始化列表转换为成员赋值语句
 * 例如：: x(0), y(0) 生成 self->x = 0; self->y = 0;
 *
 * @param ctx 代码生成上下文
 * @param class_decl 类声明
 * @param constructor 构造函数成员
 */
bool cgen_initializer_list(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl,
                           CnClassMember *constructor) {
    if (!ctx || !ctx->output_file || !constructor) return false;
    if (constructor->kind != CN_MEMBER_CONSTRUCTOR) return false;
    
    FILE *out = ctx->output_file;
    
    /* 如果没有初始化列表，直接返回成功 */
    if (!constructor->initializer_list || constructor->initializer_count == 0) {
        return true;
    }
    
    /* 生成初始化列表注释 */
    fprintf(out, "    // 初始化列表\n");
    
    /* 遍历初始化列表，生成成员赋值语句 */
    for (size_t i = 0; i < constructor->initializer_count; i++) {
        CnAstStructFieldInit *init_item = &constructor->initializer_list[i];
        
        /* 生成 self->成员名 = 初始值; */
        fprintf(out, "    self->%.*s = ",
                (int)init_item->field_name_length, init_item->field_name);
        
        if (init_item->value) {
            cgen_init_expr_simple(out, init_item->value);
        } else {
            fprintf(out, "0");
        }
        
        fprintf(out, ";\n");
    }
    
    return true;
}

/**
 * @brief 检查类是否为抽象类（包含未实现的纯虚函数）
 *
 * 遍历类的所有虚函数，检查是否有纯虚函数未被实现
 *
 * @param class_decl 类声明
 * @return true 是抽象类，false 不是抽象类
 */
static bool class_has_unimplemented_pure_virtual(CnAstClassDecl *class_decl) {
    if (!class_decl) return false;
    
    // 如果类被标记为抽象类，直接返回true
    if (class_decl->is_abstract) return true;
    
    // 检查是否有纯虚函数
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD &&
            member->is_virtual && member->is_pure_virtual) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 生成构造函数的实现
 */
static void cgen_constructor_impl(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl,
                                   CnClassMember *constructor) {
    if (!ctx || !ctx->output_file || !constructor) return;
    if (constructor->kind != CN_MEMBER_CONSTRUCTOR) return;
    
    FILE *out = ctx->output_file;
    
    // 【修复】设置当前类和方法上下文，用于代码生成
    ctx->current_class = class_decl;
    ctx->current_method = constructor;
    
    // 构造函数返回类指针类型
    fprintf(out, "%.*s* %.*s_construct(",
            (int)class_decl->name_length, class_decl->name,
            (int)class_decl->name_length, class_decl->name);
    
    // 第一个参数是 self 指针
    fprintf(out, "%.*s* self",
            (int)class_decl->name_length, class_decl->name);
    
    // 其他参数
    for (size_t i = 0; i < constructor->parameter_count; i++) {
        CnAstParameter *param = &constructor->parameters[i];
        const char *param_type = "int";
        if (param->declared_type) {
            param_type = get_c_type_string(param->declared_type);
        }
        fprintf(out, ", %s %.*s", param_type,
                (int)param->name_length, param->name);
    }
    
    fprintf(out, ") {\n");
    
    // 抽象类实例化检查：如果是抽象类，在运行时报错
    if (class_has_unimplemented_pure_virtual(class_decl)) {
        fprintf(out, "    // 抽象类实例化检查\n");
        fprintf(out, "    fprintf(stderr, \"错误: 不能实例化抽象类 %.*s\\n\");\n",
                (int)class_decl->name_length, class_decl->name);
        fprintf(out, "    fprintf(stderr, \"  抽象类包含未实现的纯虚函数\\n\");\n");
        fprintf(out, "    abort();\n");
        fprintf(out, "}\n\n");
        return;  // 抽象类构造函数直接返回，不生成其他代码
    }
    
    // 初始化虚函数表指针（如果有虚函数）
    if (class_needs_vtable(class_decl)) {
        fprintf(out, "    // 初始化虚函数表指针\n");
        fprintf(out, "    self->vtable = &_%.*s_vtable;\n",
                (int)class_decl->name_length, class_decl->name);
    }
    
    // 初始化类型信息指针（RTTI）
    fprintf(out, "    // 初始化类型信息指针（RTTI）\n");
    fprintf(out, "    self->type_info = &_%.*s_type_info;\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 调用基类构造函数（多继承支持，支持虚继承）
    // 构造顺序：
    // 1. 先构造虚基类（由最派生类直接构造）
    // 2. 再构造非虚基类（按继承顺序）
    
    // 第一步：构造虚基类（只有最派生类才构造虚基类）
    // 虚基类子对象放在结构体末尾，通过vbptr访问
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        // 只处理虚基类
        if (!base_info->is_virtual) continue;
        
        fprintf(out, "    // 构造虚基类 %.*s（由最派生类直接构造）\n",
                (int)base_info->base_class_name_length, base_info->base_class_name);
        // 虚基类通过vbptr访问偏移量
        fprintf(out, "    {\n");
        fprintf(out, "        // 获取虚基类子对象地址（通过vbptr）\n");
        fprintf(out, "        struct %.*s* vbptr_%.*s = (struct %.*s*)((char*)self + self->vbptr->%.*s_offset);\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
        fprintf(out, "        %.*s_construct(vbptr_%.*s);\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
        fprintf(out, "    }\n");
    }
    
    // 第二步：构造非虚基类（按继承顺序）
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        // 跳过虚基类（虚基类已在上面处理）
        if (base_info->is_virtual) continue;
        
        fprintf(out, "    // 初始化非虚基类 %.*s（偏移量: 使用宏 %.*s_%.*s_OFFSET）\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)class_decl->name_length, class_decl->name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
        
        // 【修复问题3】调用基类构造函数并传递参数
        // 查找基类定义以获取构造函数参数信息
        CnAstClassDecl *base_class = NULL;
        if (ctx->program) {
            // 使用 classes 数组查找基类
            for (size_t j = 0; j < ctx->program->class_count; j++) {
                CnAstStmt *stmt = ctx->program->classes[j];
                if (stmt->kind == CN_AST_STMT_CLASS_DECL) {
                    CnAstClassDecl *candidate = stmt->as.class_decl;
                    if (candidate->name_length == base_info->base_class_name_length &&
                        memcmp(candidate->name, base_info->base_class_name, base_info->base_class_name_length) == 0) {
                        base_class = candidate;
                        break;
                    }
                }
            }
        }
        
        // 查找基类构造函数
        CnClassMember *base_constructor = NULL;
        if (base_class) {
            for (size_t j = 0; j < base_class->member_count; j++) {
                if (base_class->members[j].kind == CN_MEMBER_CONSTRUCTOR) {
                    base_constructor = &base_class->members[j];
                    break;
                }
            }
        }
        
        // 只有当基类有显式构造函数时才生成调用
        if (base_constructor) {
            // 生成基类构造函数调用
            fprintf(out, "    %.*s_construct(&self->%.*s_base",
                    (int)base_info->base_class_name_length, base_info->base_class_name,
                    (int)base_info->base_class_name_length, base_info->base_class_name);
            
            // 【修复问题3】传递基类构造函数需要的参数
            // 策略：从当前构造函数参数中按名称匹配传递
            if (constructor) {
                for (size_t pi = 0; pi < base_constructor->parameter_count; pi++) {
                    CnAstParameter *base_param = &base_constructor->parameters[pi];
                    // 在当前构造函数参数中查找同名参数
                    bool found = false;
                    for (size_t ci = 0; ci < constructor->parameter_count; ci++) {
                        CnAstParameter *cur_param = &constructor->parameters[ci];
                        if (cur_param->name_length == base_param->name_length &&
                            memcmp(cur_param->name, cur_param->name, cur_param->name_length) == 0) {
                            fprintf(out, ", %.*s", (int)cur_param->name_length, cur_param->name);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        // 如果找不到同名参数，传递默认值0
                        fprintf(out, ", 0 /* 缺少参数 %.*s */", (int)base_param->name_length, base_param->name);
                    }
                }
            }
            
            fprintf(out, ");  // 调用基类构造函数\n");
        } else {
            // 基类没有显式构造函数，只生成注释
            fprintf(out, "    // 基类 %.*s 没有显式构造函数，跳过调用\n",
                    (int)base_info->base_class_name_length, base_info->base_class_name);
        }
    }
    
    // 生成初始化列表代码
    cgen_initializer_list(ctx, class_decl, constructor);
    
    // 函数体
    if (constructor->body && constructor->body->stmt_count > 0) {
        fprintf(out, "    // 构造函数体\n");
        cgen_block_in_method(ctx, constructor->body, 1);
    }
    
    // 返回 self 指针
    fprintf(out, "    return self;\n");
    
    fprintf(out, "}\n\n");
}

/**
 * @brief 生成成员函数的定义
 *
 * 静态方法不需要 self 指针参数
 */
static void cgen_method_impl(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl,
                              CnClassMember *method) {
    if (!ctx || !ctx->output_file || !method) return;
    if (method->kind != CN_MEMBER_METHOD) return;
    
    FILE *out = ctx->output_file;
    
    // 【修复】设置当前类和方法上下文，用于代码生成
    ctx->current_class = class_decl;
    ctx->current_method = method;
    
    // 返回类型
    const char *return_type = "void";
    if (method->type) {
        return_type = get_c_type_string(method->type);
    }
    
    // 函数名：类名_方法名
    fprintf(out, "%s %.*s_%.*s(",
            return_type,
            (int)class_decl->name_length, class_decl->name,
            (int)method->name_length, method->name);
    
    // 静态方法不需要 self 指针参数
    if (!method->is_static) {
        // 参数列表：第一个参数是 self 指针
        fprintf(out, "struct %.*s* self",
                (int)class_decl->name_length, class_decl->name);
    }
    
    // 其他参数
    for (size_t i = 0; i < method->parameter_count; i++) {
        CnAstParameter *param = &method->parameters[i];
        const char *param_type = "int";
        if (param->declared_type) {
            param_type = get_c_type_string(param->declared_type);
        }
        // 非静态方法需要逗号分隔，静态方法第一个参数不需要逗号
        if (!method->is_static || i > 0) {
            fprintf(out, ", ");
        }
        fprintf(out, "%s %.*s", param_type,
                (int)param->name_length, param->name);
    }
    
    // 静态方法如果没有参数，需要添加 void
    if (method->is_static && method->parameter_count == 0) {
        fprintf(out, "void");
    }
    
    fprintf(out, ") {\n");
    
    // 函数体
    if (method->body && method->body->stmt_count > 0) {
        fprintf(out, "    // 方法体\n");
        cgen_block_in_method(ctx, method->body, 1);
    } else if (method->is_pure_virtual) {
        // 纯虚函数：不生成实现
        fprintf(out, "    // 纯虚函数，无实现\n");
    } else {
        // 空函数体
        fprintf(out, "    // 空方法\n");
    }
    
    fprintf(out, "}\n\n");
}

/**
 * @brief 生成析构函数的实现
 */
static void cgen_destructor_impl(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl,
                                  CnClassMember *destructor) {
    if (!ctx || !ctx->output_file || !destructor) return;
    if (destructor->kind != CN_MEMBER_DESTRUCTOR) return;
    
    FILE *out = ctx->output_file;
    
    // 析构函数返回void，参数只有self指针
    fprintf(out, "void %.*s_destruct(%.*s* self) {\n",
            (int)class_decl->name_length, class_decl->name,
            (int)class_decl->name_length, class_decl->name);
    
    // 函数体
    if (destructor->body && destructor->body->stmt_count > 0) {
        fprintf(out, "    // 析构函数体\n");
        cgen_block_in_method(ctx, destructor->body, 1);
    } else {
        // 空函数体
        fprintf(out, "    // 空析构函数\n");
    }
    
    // 调用基类析构函数（多继承支持，析构顺序与构造相反）
    // 析构顺序：
    // 1. 先析构非虚基类（从最后一个到第一个）
    // 2. 最后析构虚基类（由最派生类负责）
    
    // 第一步：析构非虚基类（从最后一个到第一个）
    for (size_t i = class_decl->base_count; i > 0; i--) {
        CnInheritanceInfo *base_info = &class_decl->bases[i - 1];
        
        // 跳过虚基类（虚基类在后面处理）
        if (base_info->is_virtual) continue;
        
        fprintf(out, "    // 析构非虚基类 %.*s（偏移量: 使用宏 %.*s_%.*s_OFFSET）\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)class_decl->name_length, class_decl->name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
        // 使用基类子对象成员名访问
        fprintf(out, "    %.*s_destruct(&self->%.*s_base);  // 调用基类析构函数\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
    }
    
    // 第二步：析构虚基类（从最后一个到第一个）
    for (size_t i = class_decl->base_count; i > 0; i--) {
        CnInheritanceInfo *base_info = &class_decl->bases[i - 1];
        
        // 只处理虚基类
        if (!base_info->is_virtual) continue;
        
        fprintf(out, "    // 析构虚基类 %.*s（由最派生类负责）\n",
                (int)base_info->base_class_name_length, base_info->base_class_name);
        // 虚基类通过vbptr访问偏移量
        fprintf(out, "    {\n");
        fprintf(out, "        // 获取虚基类子对象地址（通过vbptr）\n");
        fprintf(out, "        struct %.*s* vbptr_%.*s = (struct %.*s*)((char*)self + self->vbptr->%.*s_offset);\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
        fprintf(out, "        %.*s_destruct(vbptr_%.*s);\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
        fprintf(out, "    }\n");
    }
    
    fprintf(out, "}\n\n");
}

/* ============================================================================
 * 公开接口实现
 * ============================================================================ */

bool cn_cgen_class_struct(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl) {
    if (!ctx || !ctx->output_file || !class_decl) return false;
    
    FILE *out = ctx->output_file;
    
    // 计算类布局信息
    CnClassLayout layout;
    cn_class_layout_init(&layout);
    cn_class_layout_calculate(&layout, class_decl, NULL, NULL);
    
    // 结构体前向声明
    fprintf(out, "struct %.*s;\n\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 如果有虚基类，先生成vbtable结构体
    if (layout.has_virtual_base && layout.vbtable_count > 0) {
        fprintf(out, "/* 虚基类表（vbtable）- 记录虚基类偏移量 */\n");
        fprintf(out, "typedef struct %.*s_vbtable {\n",
                (int)class_decl->name_length, class_decl->name);
        for (size_t i = 0; i < layout.vbtable_count; i++) {
            print_indent(out, 1);
            fprintf(out, "size_t %.*s_offset;  // 虚基类 %.*s 的偏移量\n",
                    (int)layout.vbtable_name_lengths[i], layout.vbtable_names[i],
                    (int)layout.vbtable_name_lengths[i], layout.vbtable_names[i]);
        }
        fprintf(out, "} %.*s_vbtable;\n\n",
                (int)class_decl->name_length, class_decl->name);
    }
    
    // 结构体定义开始
    fprintf(out, "typedef struct %.*s {\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 第一步：布局非虚基类子对象
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        // 跳过虚基类，虚基类在后面处理
        if (base_info->is_virtual) continue;
        
        print_indent(out, 1);
        fprintf(out, "struct %.*s %.*s_base;  // 基类子对象（偏移量: %zu字节，%s继承）\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name,
                layout.base_offsets[i],
                base_info->access == CN_ACCESS_PUBLIC ? "公开" :
                base_info->access == CN_ACCESS_PROTECTED ? "保护" : "私有");
    }
    
    // 第二步：如果有虚基类，添加vbptr（虚基类指针）
    if (layout.has_virtual_base) {
        print_indent(out, 1);
        fprintf(out, "struct %.*s_vbtable* vbptr;  // 虚基类表指针（偏移量: %zu字节）\n",
                (int)class_decl->name_length, class_decl->name,
                layout.vbptr_offset != SIZE_MAX ? layout.vbptr_offset : 0);
    }
    
    // 第三步：如果需要虚函数表，添加vtable指针
    if (class_needs_vtable(class_decl)) {
        print_indent(out, 1);
        fprintf(out, "struct %.*s_vtable* vtable;  // 虚函数表指针（偏移量: %zu字节）\n",
                (int)class_decl->name_length, class_decl->name,
                layout.has_vtable ? layout.vtable_offset : 0);
    }
    
    // 第3.5步：添加type_info指针（RTTI支持）
    // 所有类都添加type_info指针，用于运行时类型识别
    {
        print_indent(out, 1);
        fprintf(out, "const CnTypeInfo* type_info;  // 类型信息指针（RTTI）\n");
    }
    
    // 第四步：生成成员变量（字段）
    // 注意：C语言没有访问控制机制，所有成员都必须生成
    // 访问控制在语义分析阶段检查，而不是代码生成阶段
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_FIELD) {
            cgen_member_field(out, member, 1);
        }
    }
    
    // 第五步：虚基类子对象（放在最后，通过vbptr访问）
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        // 只处理虚基类
        if (!base_info->is_virtual) continue;
        
        print_indent(out, 1);
        fprintf(out, "struct %.*s %.*s_vbase;  // 虚基类子对象（偏移量: %zu字节，虚继承）\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name,
                layout.base_offsets[i]);
    }
    
    // 结构体定义结束
    fprintf(out, "} %.*s;\n\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 生成基类偏移量常量（用于this指针调整）
    if (class_decl->base_count > 0) {
        fprintf(out, "/* 类 %.*s 的基类偏移量常量（用于this指针调整） */\n",
                (int)class_decl->name_length, class_decl->name);
        for (size_t i = 0; i < class_decl->base_count; i++) {
            CnInheritanceInfo *base_info = &class_decl->bases[i];
            const char *virtual_mark = base_info->is_virtual ? "（虚基类）" : "";
            fprintf(out, "#define %.*s_%.*s_OFFSET %zu  /* %s */\n",
                    (int)class_decl->name_length, class_decl->name,
                    (int)base_info->base_class_name_length, base_info->base_class_name,
                    layout.base_offsets[i],
                    virtual_mark);
        }
        fprintf(out, "\n");
        
        // 生成this指针调整宏
        fprintf(out, "/* this指针调整宏 */\n");
        for (size_t i = 0; i < class_decl->base_count; i++) {
            CnInheritanceInfo *base_info = &class_decl->bases[i];
            
            if (base_info->is_virtual) {
                // 虚基类通过vbptr访问
                fprintf(out, "/* 虚基类 %.*s 通过vbptr访问 */\n",
                        (int)base_info->base_class_name_length, base_info->base_class_name);
                fprintf(out, "#define %.*s_TO_%.*s(ptr) ((struct %.*s*)((char*)(ptr) + ((struct %.*s*)(ptr))->vbptr->%.*s_offset))\n",
                        (int)class_decl->name_length, class_decl->name,
                        (int)base_info->base_class_name_length, base_info->base_class_name,
                        (int)base_info->base_class_name_length, base_info->base_class_name,
                        (int)class_decl->name_length, class_decl->name,
                        (int)base_info->base_class_name_length, base_info->base_class_name);
            } else {
                // 非虚基类直接计算偏移
                fprintf(out, "#define %.*s_TO_%.*s(ptr) ((struct %.*s*)((char*)(ptr) + %.*s_%.*s_OFFSET))\n",
                        (int)class_decl->name_length, class_decl->name,
                        (int)base_info->base_class_name_length, base_info->base_class_name,
                        (int)base_info->base_class_name_length, base_info->base_class_name,
                        (int)class_decl->name_length, class_decl->name,
                        (int)base_info->base_class_name_length, base_info->base_class_name);
            }
        }
        fprintf(out, "\n");
    }
    
    // 清理布局信息
    cn_class_layout_cleanup(&layout);
    
    return true;
}

/**
 * @brief 生成单个虚函数条目的函数指针声明
 */
static void cgen_vtable_entry_decl(FILE *out, CnVTableEntry *entry,
                                    const char *class_name, size_t class_name_len) {
    if (!entry || !entry->method) return;
    
    CnClassMember *method = entry->method;
    
    // 返回类型
    const char *return_type = "void";
    if (method->type) {
        return_type = get_c_type_string(method->type);
    }
    
    // 函数指针
    fprintf(out, "%s (*%.*s)(struct %.*s* self",
            return_type,
            (int)entry->method_name_length, entry->method_name,
            (int)class_name_len, class_name);
    
    // 其他参数
    for (size_t j = 0; j < method->parameter_count; j++) {
        CnAstParameter *param = &method->parameters[j];
        const char *param_type = "int";
        if (param->declared_type) {
            param_type = get_c_type_string(param->declared_type);
        }
        fprintf(out, ", %s %.*s", param_type,
                (int)param->name_length, param->name);
    }
    
    fprintf(out, ");\n");
}

/**
 * @brief 生成纯虚函数调用错误处理函数
 *
 * 当运行时调用纯虚函数时，输出错误信息并终止程序
 * 支持完整的参数列表，确保函数签名与实际方法一致
 */
static void cgen_pure_virtual_error_func(FILE *out, CnAstClassDecl *class_decl,
                                          CnClassMember *method) {
    if (!out || !method) return;
    
    // 返回类型
    const char *return_type = "void";
    if (method->type) {
        return_type = get_c_type_string(method->type);
    }
    
    // 函数名：类名_方法名_pure_virtual_error
    fprintf(out, "static %s %.*s_%.*s_pure_virtual_error(struct %.*s* self",
            return_type,
            (int)class_decl->name_length, class_decl->name,
            (int)method->name_length, method->name,
            (int)class_decl->name_length, class_decl->name);
    
    // 输出其他参数（保持与实际方法相同的签名）
    for (size_t i = 0; i < method->parameter_count; i++) {
        CnAstParameter *param = &method->parameters[i];
        const char *param_type = "int";
        if (param->declared_type) {
            param_type = get_c_type_string(param->declared_type);
        }
        fprintf(out, ", %s %.*s", param_type,
                (int)param->name_length, param->name);
    }
    
    fprintf(out, ") {\n");
    
    // 输出错误信息
    fprintf(out, "    fprintf(stderr, \"错误: 调用了纯虚函数 %.*s::%.*s\\n\");\n",
            (int)class_decl->name_length, class_decl->name,
            (int)method->name_length, method->name);
    fprintf(out, "    fprintf(stderr, \"  类: %.*s\\n\");\n",
            (int)class_decl->name_length, class_decl->name);
    fprintf(out, "    fprintf(stderr, \"  方法: %.*s\\n\");\n",
            (int)method->name_length, method->name);
    fprintf(out, "    fprintf(stderr, \"  请在派生类中实现此方法\\n\");\n");
    fprintf(out, "    abort();\n");
    fprintf(out, "}\n\n");
}

/**
 * @brief 查找基类的类声明（通过符号表）
 *
 * 注意：这是一个简化实现，完整实现需要通过符号表查找
 *
 * @param class_decl 当前类声明
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @return 找到的基类声明，未找到返回NULL
 */
static CnAstClassDecl *find_base_class_decl(CnAstClassDecl *class_decl,
                                             const char *base_name,
                                             size_t base_name_len) {
    // 简化实现：通过ctx->program查找
    // 完整实现需要通过符号表
    (void)class_decl;  // 暂时未使用
    (void)base_name;
    (void)base_name_len;
    return NULL;
}

/**
 * @brief 递归收集基类虚函数信息
 *
 * 遍历继承链，收集所有基类的虚函数到vtable中
 *
 * @param vtable 目标vtable
 * @param class_decl 当前类声明
 * @param visited 已访问类名集合（防止循环继承）
 * @param visited_count 已访问类名数量
 */
static void collect_base_virtual_methods(CnVTable *vtable,
                                          CnAstClassDecl *class_decl,
                                          const char **visited,
                                          size_t *visited_count) {
    if (!vtable || !class_decl || !visited) return;
    
    // 检查是否已访问（防止循环继承）
    for (size_t i = 0; i < *visited_count; i++) {
        if (visited[i] &&
            memcmp(visited[i], class_decl->name, class_decl->name_length) == 0) {
            return;  // 已访问，跳过
        }
    }
    
    // 标记为已访问
    if (*visited_count < 32) {  // 最多支持32层继承
        visited[*visited_count] = class_decl->name;
        (*visited_count)++;
    }
    
    // 先处理基类（深度优先，确保基类方法在前）
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        // 查找基类声明（简化处理：假设基类在同一编译单元）
        // 完整实现需要通过符号表查找
        // 这里我们只记录基类名称，实际合并由vtable_builder完成
        
        // 递归处理基类的基类
        // 注意：这里需要实际的基类声明，简化实现暂时跳过
        (void)base_info;
    }
    
    // 添加当前类的虚函数
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD && member->is_virtual) {
            cn_vtable_add_entry_ex(vtable, member,
                                    class_decl->name, class_decl->name_length);
        }
    }
}

/**
 * @brief 检查类是否需要生成接口vtable
 *
 * 如果类实现了接口，需要生成接口vtable
 *
 * @param class_decl 类声明
 * @return true 需要生成接口vtable，false 不需要
 */
static bool class_implements_interfaces(CnAstClassDecl *class_decl) {
    return class_decl && class_decl->implemented_interface_count > 0;
}

/* ============================================================================
 * 接口模板参数支持（阶段17）
 * ============================================================================ */

/**
 * @brief 生成接口vtable名称（支持模板参数）
 *
 * 对于模板接口，名称格式为：接口名_类型参数_vtable
 * 例如：可比较<数据项> -> 可比较_数据项_vtable
 *
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param interface_decl 接口声明
 * @param iface_inst 接口实例化（包含类型实参，可为NULL）
 * @return 生成的名称长度
 */
static size_t generate_interface_vtable_name(char *buffer, size_t buffer_size,
                                              CnAstInterfaceDecl *interface_decl,
                                              CnAstInterfaceInstantiation *iface_inst) {
    if (!buffer || buffer_size == 0) return 0;
    
    size_t offset = 0;
    
    // 添加接口名
    if (interface_decl && interface_decl->name) {
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "%.*s", (int)interface_decl->name_length, interface_decl->name);
    }
    
    // 如果有类型实参，添加到名称中
    if (iface_inst && iface_inst->type_arg_count > 0) {
        for (size_t i = 0; i < iface_inst->type_arg_count && offset < buffer_size - 1; i++) {
            CnType *type_arg = iface_inst->type_args[i];
            if (type_arg) {
                // 添加下划线分隔符
                if (offset < buffer_size - 1) {
                    buffer[offset++] = '_';
                }
                // 添加类型名
                const char *type_str = get_c_type_string(type_arg);
                offset += snprintf(buffer + offset, buffer_size - offset, "%s", type_str);
            }
        }
    }
    
    // 添加 _vtable 后缀
    if (offset < buffer_size) {
        offset += snprintf(buffer + offset, buffer_size - offset, "_vtable");
    }
    
    return offset;
}

/**
 * @brief 构建接口模板参数的类型映射表
 *
 * 将接口的模板形参与类实现时提供的类型实参关联起来
 *
 * @param interface_decl 接口声明（包含模板形参）
 * @param iface_inst 接口实例化（包含类型实参）
 * @return CnTypeMap* 类型映射表，调用者负责释放
 */
static CnTypeMap *build_interface_type_map(CnAstInterfaceDecl *interface_decl,
                                            CnAstInterfaceInstantiation *iface_inst) {
    if (!interface_decl || !iface_inst) return NULL;
    
    // 检查接口是否有模板参数
    if (!interface_decl->template_params || interface_decl->template_params->param_count == 0) {
        return NULL;
    }
    
    // 创建类型映射表
    CnTypeMap *type_map = cn_type_map_new();
    if (!type_map) return NULL;
    
    // 遍历模板形参，匹配类型实参
    CnAstTemplateParams *tparams = interface_decl->template_params;
    for (size_t i = 0; i < tparams->param_count && i < iface_inst->type_arg_count; i++) {
        const char *param_name = tparams->params[i].name;
        size_t param_name_len = tparams->params[i].name_length;
        CnType *type_arg = iface_inst->type_args[i];
        
        if (param_name && type_arg) {
            cn_type_map_insert(type_map, param_name, param_name_len, type_arg);
        }
    }
    
    return type_map;
}

/**
 * @brief 获取接口方法的C类型字符串（支持模板参数替换）
 *
 * @param type 原始类型
 * @param type_map 类型映射表（可为NULL）
 * @return C类型字符串
 */
static const char *get_interface_method_c_type(CnType *type, const CnTypeMap *type_map) {
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
 * @brief 生成接口vtable结构体定义
 *
 * @param ctx 代码生成上下文
 * @param interface_decl 接口声明
 * @param iface_inst 接口实例化（可为NULL，表示非模板接口）
 */
static void cgen_interface_vtable_struct(CnCCodeGenContext *ctx,
                                          CnAstInterfaceDecl *interface_decl,
                                          CnAstInterfaceInstantiation *iface_inst) {
    if (!ctx || !ctx->output_file || !interface_decl) return;
    
    FILE *out = ctx->output_file;
    
    // 生成vtable名称
    char vtable_name[256];
    generate_interface_vtable_name(vtable_name, sizeof(vtable_name), interface_decl, iface_inst);
    
    // 构建类型映射表（用于模板参数替换）
    CnTypeMap *type_map = build_interface_type_map(interface_decl, iface_inst);
    
    // 生成vtable结构体定义
    fprintf(out, "typedef struct %s {\n", vtable_name);
    
    // 生成方法指针
    for (size_t i = 0; i < interface_decl->method_count; i++) {
        CnClassMember *method = &interface_decl->methods[i];
        
        // 获取返回类型（可能需要模板参数替换）
        const char *return_type = "void";
        if (method->type) {
            return_type = get_interface_method_c_type(method->type, type_map);
        }
        
        // 生成方法指针
        print_indent(out, 1);
        fprintf(out, "%s (*%.*s)(void* self",
                return_type,
                (int)method->name_length, method->name);
        
        // 生成参数列表
        for (size_t j = 0; j < method->parameter_count; j++) {
            CnAstParameter *param = &method->parameters[j];
            const char *param_type = "int";
            if (param->declared_type) {
                param_type = get_interface_method_c_type(param->declared_type, type_map);
            }
            fprintf(out, ", %s", param_type);
        }
        
        fprintf(out, ");\n");
    }
    
    // 如果没有方法，添加占位符
    if (interface_decl->method_count == 0) {
        print_indent(out, 1);
        fprintf(out, "void* _reserved;  // 占位符（空接口）\n");
    }
    
    fprintf(out, "} %s;\n\n", vtable_name);
    
    // 释放类型映射表
    if (type_map) {
        cn_type_map_free(type_map);
    }
}

/**
 * @brief 使用vtable_builder生成虚函数表
 *
 * 生成策略：
 * 1. 创建vtable结构体定义
 * 2. 合并基类vtable（继承链）
 * 3. 添加当前类的虚函数（包括重写）
 * 4. 添加接口vtable（如果实现了接口）
 * 5. 生成vtable实例
 */
bool cn_cgen_vtable(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl) {
    if (!ctx || !ctx->output_file || !class_decl) return false;
    
    // 如果不需要虚函数表，直接返回成功
    if (!cn_vtable_needs_vtable(class_decl)) {
        return true;
    }
    
    FILE *out = ctx->output_file;
    
    // 创建临时vtable用于代码生成
    CnVTable *vtable = cn_vtable_create(class_decl->name, class_decl->name_length);
    if (!vtable) return false;
    
    // 收集基类虚函数（深度优先，确保基类方法在前）
    if (ctx->program && class_decl->base_count > 0) {
        for (size_t i = 0; i < class_decl->base_count; i++) {
            CnInheritanceInfo *base_info = &class_decl->bases[i];
            
            // 从程序AST中查找基类声明
            CnAstClassDecl *base_class = cn_find_class_in_program(ctx->program,
                                                                   base_info->base_class_name,
                                                                   base_info->base_class_name_length);
            if (base_class) {
                // 递归收集基类的虚函数
                for (size_t j = 0; j < base_class->member_count; j++) {
                    CnClassMember *base_member = &base_class->members[j];
                    if (base_member->kind == CN_MEMBER_METHOD && base_member->is_virtual) {
                        // 添加基类虚函数到vtable，保持基类名称作为定义类
                        cn_vtable_add_entry_ex(vtable, base_member,
                                                base_class->name, base_class->name_length);
                    }
                }
            }
        }
    }
    
    // 添加当前类的虚函数（可能会覆盖基类的虚函数）
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD && member->is_virtual) {
            cn_vtable_add_entry_ex(vtable, member,
                                    class_decl->name, class_decl->name_length);
        }
    }
    
    // 生成纯虚函数的错误处理函数
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD &&
            member->is_virtual && member->is_pure_virtual) {
            cgen_pure_virtual_error_func(out, class_decl, member);
        }
    }
    
    // 虚函数表结构体定义
    fprintf(out, "typedef struct %.*s_vtable {\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 如果实现了接口，先包含接口vtable
    if (class_implements_interfaces(class_decl)) {
        for (size_t i = 0; i < class_decl->implemented_interface_count; i++) {
            CnAstInterfaceInstantiation *iface_inst = class_decl->implemented_interfaces[i];
            
            // 查找接口定义
            CnAstInterfaceDecl *interface_def = NULL;
            if (ctx && ctx->program) {
                for (size_t j = 0; j < ctx->program->interface_count; j++) {
                    CnAstStmt *iface_stmt = ctx->program->interfaces[j];
                    if (iface_stmt && iface_stmt->kind == CN_AST_STMT_INTERFACE_DECL) {
                        CnAstInterfaceDecl *iface = iface_stmt->as.interface_decl;
                        if (iface->name_length == iface_inst->interface_name_length &&
                            strncmp(iface->name, iface_inst->interface_name, iface->name_length) == 0) {
                            interface_def = iface;
                            break;
                        }
                    }
                }
            }
            
            // 生成接口vtable名称（支持模板参数）
            char iface_vtable_name[256];
            if (interface_def) {
                generate_interface_vtable_name(iface_vtable_name, sizeof(iface_vtable_name),
                                               interface_def, iface_inst);
            } else {
                // 未找到接口定义，使用默认名称
                snprintf(iface_vtable_name, sizeof(iface_vtable_name), "%.*s_vtable",
                        (int)iface_inst->interface_name_length, iface_inst->interface_name);
            }
            
            // 生成接口vtable结构体定义（如果接口有模板参数）
            if (interface_def && interface_def->template_params &&
                interface_def->template_params->param_count > 0 && iface_inst->type_arg_count > 0) {
                // 先生成接口vtable结构体定义
                cgen_interface_vtable_struct(ctx, interface_def, iface_inst);
            }
            
            print_indent(out, 1);
            fprintf(out, "struct %s %.*s_iface;  // 实现的接口 %.*s",
                    iface_vtable_name,
                    (int)iface_inst->interface_name_length, iface_inst->interface_name,
                    (int)iface_inst->interface_name_length, iface_inst->interface_name);
            
            // 如果有类型参数，显示它们
            if (iface_inst->type_arg_count > 0) {
                fprintf(out, "<");
                for (size_t t = 0; t < iface_inst->type_arg_count; t++) {
                    if (t > 0) fprintf(out, ", ");
                    fprintf(out, "%s", get_c_type_string(iface_inst->type_args[t]));
                }
                fprintf(out, ">");
            }
            fprintf(out, "\n");
        }
    }
    
    // 遍历vtable条目，生成函数指针
    bool has_entries = false;
    for (size_t i = 0; i < vtable->entry_count; i++) {
        CnVTableEntry *entry = &vtable->entries[i];
        print_indent(out, 1);
        cgen_vtable_entry_decl(out, entry, class_decl->name, class_decl->name_length);
        has_entries = true;
    }
    
    // 如果没有任何条目，添加一个占位符成员（C语言不允许空结构体）
    if (!has_entries && !class_implements_interfaces(class_decl)) {
        print_indent(out, 1);
        fprintf(out, "void* _reserved;  // 占位符（空vtable）\n");
    }
    
    fprintf(out, "} %.*s_vtable;\n\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 生成虚函数表实例（静态变量）
    fprintf(out, "static %.*s_vtable _%.*s_vtable = {\n",
            (int)class_decl->name_length, class_decl->name,
            (int)class_decl->name_length, class_decl->name);
    
    // 初始化接口vtable部分
    bool first = true;
    if (class_implements_interfaces(class_decl)) {
        for (size_t i = 0; i < class_decl->implemented_interface_count; i++) {
            CnAstInterfaceInstantiation *iface_inst = class_decl->implemented_interfaces[i];
            const char *iface_name = iface_inst->interface_name;
            size_t iface_name_len = iface_inst->interface_name_length;
            
            if (!first) {
                fprintf(out, ",\n");
            }
            first = false;
            
            print_indent(out, 1);
            fprintf(out, ".%.*s_iface = {\n", (int)iface_name_len, iface_name);
            
            // 查找接口定义以获取接口方法列表
            CnAstInterfaceDecl *interface_def = NULL;
            if (ctx && ctx->program) {
                for (size_t j = 0; j < ctx->program->interface_count; j++) {
                    CnAstStmt *iface_stmt = ctx->program->interfaces[j];
                    if (iface_stmt && iface_stmt->kind == CN_AST_STMT_INTERFACE_DECL) {
                        CnAstInterfaceDecl *iface = iface_stmt->as.interface_decl;
                        if (iface->name_length == iface_name_len &&
                            strncmp(iface->name, iface_name, iface_name_len) == 0) {
                            interface_def = iface;
                            break;
                        }
                    }
                }
            }
            
            // 构建类型映射表（用于模板参数替换）
            CnTypeMap *type_map = build_interface_type_map(interface_def, iface_inst);
            
            // 初始化接口方法：查找类中实现的接口方法
            bool iface_first = true;
            if (interface_def) {
                for (size_t m = 0; m < interface_def->method_count; m++) {
                    CnClassMember *iface_method = &interface_def->methods[m];
                    
                    // 在类中查找同名方法
                    CnClassMember *impl_method = NULL;
                    for (size_t k = 0; k < class_decl->member_count; k++) {
                        CnClassMember *member = &class_decl->members[k];
                        if (member->kind == CN_MEMBER_METHOD &&
                            member->name_length == iface_method->name_length &&
                            strncmp(member->name, iface_method->name, member->name_length) == 0) {
                            impl_method = member;
                            break;
                        }
                    }
                    
                    if (!iface_first) {
                        fprintf(out, ",\n");
                    }
                    iface_first = false;
                    
                    print_indent(out, 2);
                    if (impl_method) {
                        // 找到实现方法，绑定到类方法
                        fprintf(out, ".%.*s = %.*s_%.*s",
                                (int)iface_method->name_length, iface_method->name,
                                (int)class_decl->name_length, class_decl->name,
                                (int)iface_method->name_length, iface_method->name);
                    } else {
                        // 未找到实现方法，使用占位符（这不应该发生，语义检查应该已经验证）
                        fprintf(out, ".%.*s = %.*s_%.*s_interface_pure_virtual_error  // 警告: 未找到实现",
                                (int)iface_method->name_length, iface_method->name,
                                (int)iface_name_len, iface_name,
                                (int)iface_method->name_length, iface_method->name);
                    }
                }
            }
            
            // 释放类型映射表
            if (type_map) {
                cn_type_map_free(type_map);
            }
            
            if (!iface_first) {
                fprintf(out, "\n");
            }
            print_indent(out, 1);
            fprintf(out, "},  // end interface %.*s", (int)iface_name_len, iface_name);
            
            // 如果有类型参数，显示它们
            if (iface_inst->type_arg_count > 0) {
                fprintf(out, "<");
                for (size_t t = 0; t < iface_inst->type_arg_count; t++) {
                    if (t > 0) fprintf(out, ", ");
                    fprintf(out, "%s", get_c_type_string(iface_inst->type_args[t]));
                }
                fprintf(out, ">");
            }
            fprintf(out, "\n");
        }
    }
    
    // 初始化虚函数表
    bool has_vtable_entries = false;
    for (size_t i = 0; i < vtable->entry_count; i++) {
        CnVTableEntry *entry = &vtable->entries[i];
        if (!first) {
            fprintf(out, ",\n");
        }
        first = false;
        has_vtable_entries = true;
        
        print_indent(out, 1);
        
        // 检查是否为纯虚函数
        if (entry->is_pure_virtual) {
            // 纯虚函数指向错误处理函数
            fprintf(out, ".%.*s = %.*s_%.*s_pure_virtual_error",
                    (int)entry->method_name_length, entry->method_name,
                    (int)class_decl->name_length, class_decl->name,
                    (int)entry->method_name_length, entry->method_name);
        } else {
            // 普通虚函数：使用定义该方法的类名作为函数前缀
            fprintf(out, ".%.*s = %.*s_%.*s",
                    (int)entry->method_name_length, entry->method_name,
                    (int)entry->defined_in_class_len, entry->defined_in_class,
                    (int)entry->method_name_length, entry->method_name);
        }
    }
    
    // 如果没有任何条目且没有接口，初始化占位符成员
    if (!has_vtable_entries && !class_implements_interfaces(class_decl)) {
        if (!first) {
            fprintf(out, ",\n");
        }
        print_indent(out, 1);
        fprintf(out, "._reserved = NULL");
    }
    
    fprintf(out, "\n};\n\n");
    
    // 清理临时vtable
    cn_vtable_destroy(vtable);
    
    return true;
}

bool cn_cgen_class_methods(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl) {
    if (!ctx || !ctx->output_file || !class_decl) return false;
    
    FILE *out = ctx->output_file;
    
    // 首先生成所有构造函数和方法的前向声明
    fprintf(out, "// 类 %.*s 的构造函数和方法声明\n",
            (int)class_decl->name_length, class_decl->name);
    
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_CONSTRUCTOR) {
            cgen_constructor_forward_decl(out, class_decl, member);
        } else if (member->kind == CN_MEMBER_DESTRUCTOR) {
            cgen_destructor_forward_decl(out, class_decl, member);
        } else if (member->kind == CN_MEMBER_METHOD) {
            cgen_method_forward_decl(out, class_decl, member);
        }
    }
    
    fprintf(out, "\n");
    
    // 然后生成所有构造函数和方法的实现
    fprintf(out, "// 类 %.*s 的构造函数和方法实现\n",
            (int)class_decl->name_length, class_decl->name);
    
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_CONSTRUCTOR) {
            cgen_constructor_impl(ctx, class_decl, member);
        } else if (member->kind == CN_MEMBER_DESTRUCTOR) {
            cgen_destructor_impl(ctx, class_decl, member);
        } else if (member->kind == CN_MEMBER_METHOD) {
            cgen_method_impl(ctx, class_decl, member);
        }
    }
    
    return true;
}

/**
 * @brief 计算继承深度
 *
 * 递归计算类的继承深度，根类为0
 *
 * @param class_decl 类声明
 * @return int 继承深度
 */
static int calculate_inheritance_depth(CnAstClassDecl *class_decl) {
    if (!class_decl || class_decl->base_count == 0) {
        return 0;
    }
    
    int max_depth = 0;
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        // 这里简化处理，假设基类深度为继承深度-1
        // 完整实现需要访问基类的类型信息
        (void)base_info;
        max_depth = 1;  // 至少深度为1
    }
    
    return max_depth;
}

/**
 * @brief 生成类型标志字符串
 *
 * 根据类声明生成类型标志的组合字符串
 *
 * @param class_decl 类声明
 * @return const char* 类型标志字符串
 */
static const char* generate_type_flags_string(CnAstClassDecl *class_decl) {
    static char flags_buffer[256];
    flags_buffer[0] = '\0';
    
    bool first = true;
    
    // 所有类都有CN_TYPE_FLAG_CLASS标志
    strcat(flags_buffer, "CN_TYPE_FLAG_CLASS");
    first = false;
    
    if (class_decl->is_abstract) {
        if (!first) strcat(flags_buffer, " | ");
        strcat(flags_buffer, "CN_TYPE_FLAG_ABSTRACT");
        first = false;
    }
    
    // 检查是否有虚函数
    if (class_has_virtual_methods(class_decl)) {
        if (!first) strcat(flags_buffer, " | ");
        strcat(flags_buffer, "CN_TYPE_FLAG_POLYMORPHIC");
        first = false;
    }
    
    // 检查是否有虚基类
    for (size_t i = 0; i < class_decl->base_count; i++) {
        if (class_decl->bases[i].is_virtual) {
            if (!first) strcat(flags_buffer, " | ");
            strcat(flags_buffer, "CN_TYPE_FLAG_HAS_VBASE");
            break;
        }
    }
    
    return flags_buffer;
}

/**
 * @brief 生成类型信息结构（RTTI）
 *
 * 为类生成CnTypeInfo结构，包含类型名称、大小、继承关系等信息
 *
 * @param ctx 代码生成上下文
 * @param class_decl 类声明
 * @return bool 成功返回true
 */
bool cn_cgen_type_info(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl) {
    if (!ctx || !ctx->output_file || !class_decl) return false;
    
    FILE *out = ctx->output_file;
    
    // 生成基类信息数组（如果有基类）
    if (class_decl->base_count > 0) {
        fprintf(out, "/* 类 %.*s 的基类信息 */\n",
                (int)class_decl->name_length, class_decl->name);
        fprintf(out, "static const CnBaseClassInfo _%.*s_bases[] = {\n",
                (int)class_decl->name_length, class_decl->name);
        
        for (size_t i = 0; i < class_decl->base_count; i++) {
            CnInheritanceInfo *base_info = &class_decl->bases[i];
            fprintf(out, "    {\n");
            fprintf(out, "        .type = &_%.*s_type_info,  // 基类 %.*s\n",
                    (int)base_info->base_class_name_length, base_info->base_class_name,
                    (int)base_info->base_class_name_length, base_info->base_class_name);
            fprintf(out, "        .offset = %.*s_%.*s_OFFSET,\n",
                    (int)class_decl->name_length, class_decl->name,
                    (int)base_info->base_class_name_length, base_info->base_class_name);
            fprintf(out, "        .is_virtual = %s,\n",
                    base_info->is_virtual ? "true" : "false");
            fprintf(out, "        .is_public = %s\n",
                    base_info->access == CN_ACCESS_PUBLIC ? "true" : "false");
            fprintf(out, "    },\n");
        }
        
        fprintf(out, "};\n\n");
    }
    
    // 生成类型转换缓存（性能优化：O(n) -> O(1)）
    // 收集所有可达基类（包括间接基类）及其偏移量
    {
        fprintf(out, "/* 类 %.*s 的类型转换缓存 */\n",
                (int)class_decl->name_length, class_decl->name);
        
        // 如果没有基类，生成一个空条目以避免空数组问题
        if (class_decl->base_count == 0) {
            fprintf(out, "static const CnCastInfo _%.*s_cast_cache[1] = {{0}};\n\n",
                    (int)class_decl->name_length, class_decl->name);
        } else {
            fprintf(out, "static const CnCastInfo _%.*s_cast_cache[] = {\n",
                    (int)class_decl->name_length, class_decl->name);
            
            // 遍历所有直接基类
            for (size_t i = 0; i < class_decl->base_count; i++) {
                CnInheritanceInfo *base_info = &class_decl->bases[i];
                
                // 添加直接基类到缓存
                fprintf(out, "    {\n");
                fprintf(out, "        .target_type = &_%.*s_type_info,  // 基类 %.*s\n",
                        (int)base_info->base_class_name_length, base_info->base_class_name,
                        (int)base_info->base_class_name_length, base_info->base_class_name);
                fprintf(out, "        .offset = %.*s_%.*s_OFFSET,\n",
                        (int)class_decl->name_length, class_decl->name,
                        (int)base_info->base_class_name_length, base_info->base_class_name);
                fprintf(out, "        .flags = %s\n",
                        base_info->is_virtual ? "CN_CAST_FLAG_VIRTUAL" : "CN_CAST_FLAG_NONE");
                fprintf(out, "    },\n");
                
                // TODO: 递归添加间接基类（需要编译时计算累积偏移量）
                // 当前版本仅支持直接基类的缓存
            }
            
            fprintf(out, "};\n\n");
        }
        
        // 保存缓存数量供后续使用
        // 注意：这里需要在结构体初始化时使用
    }
    
    // 生成类型信息结构
    fprintf(out, "/* 类 %.*s 的类型信息（RTTI） */\n",
            (int)class_decl->name_length, class_decl->name);
    fprintf(out, "static const CnTypeInfo _%.*s_type_info = {\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 基本信息
    fprintf(out, "    .name = \"%.*s\",\n",
            (int)class_decl->name_length, class_decl->name);
    fprintf(out, "    .name_length = %zu,\n", class_decl->name_length);
    fprintf(out, "    .size = sizeof(%.*s),\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 类型标志
    fprintf(out, "    .flags = %s,\n", generate_type_flags_string(class_decl));
    
    // 继承关系
    if (class_decl->base_count > 0) {
        fprintf(out, "    .bases = _%.*s_bases,\n",
                (int)class_decl->name_length, class_decl->name);
        fprintf(out, "    .base_count = %zu,\n", class_decl->base_count);
    } else {
        fprintf(out, "    .bases = NULL,\n");
        fprintf(out, "    .base_count = 0,\n");
    }
    
    // 虚函数表关联
    if (class_needs_vtable(class_decl)) {
        fprintf(out, "    .vtable = &_%.*s_vtable,\n",
                (int)class_decl->name_length, class_decl->name);
    } else {
        fprintf(out, "    .vtable = NULL,\n");
    }
    
    // 继承深度
    fprintf(out, "    .depth = %d,\n", calculate_inheritance_depth(class_decl));
    
    // 主基类（第一个非虚基类）
    bool has_primary_base = false;
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        if (!base_info->is_virtual) {
            fprintf(out, "    .primary_base = &_%.*s_type_info,  // 主基类 %.*s\n",
                    (int)base_info->base_class_name_length, base_info->base_class_name,
                    (int)base_info->base_class_name_length, base_info->base_class_name);
            has_primary_base = true;
            break;
        }
    }
    
    if (!has_primary_base) {
        fprintf(out, "    .primary_base = NULL,\n");
    }
    
    // 类型转换缓存（性能优化）
    if (class_decl->base_count > 0) {
        fprintf(out, "    .cast_cache = _%.*s_cast_cache,\n",
                (int)class_decl->name_length, class_decl->name);
        fprintf(out, "    .cast_cache_count = %zu\n", class_decl->base_count);
    } else {
        fprintf(out, "    .cast_cache = NULL,\n");
        fprintf(out, "    .cast_cache_count = 0\n");
    }
    
    fprintf(out, "};\n\n");
    
    // 生成类型注册代码（在程序启动时自动注册）
    // 使用条件编译支持MSVC和GCC/Clang
    fprintf(out, "/* 自动注册类型信息 */\n");
    fprintf(out, "#if defined(_MSC_VER)\n");
    fprintf(out, "static void _%.*s_register_type(void);\n",
            (int)class_decl->name_length, class_decl->name);
    fprintf(out, "#pragma section(\".CRT$XCU\", read)\n");
    fprintf(out, "__declspec(allocate(\".CRT$XCU\")) static void (*_%.*s_register_type_ptr)(void) = _%.*s_register_type;\n",
            (int)class_decl->name_length, class_decl->name,
            (int)class_decl->name_length, class_decl->name);
    fprintf(out, "#else\n");
    fprintf(out, "__attribute__((constructor))\n");
    fprintf(out, "#endif\n");
    fprintf(out, "static void _%.*s_register_type(void) {\n",
            (int)class_decl->name_length, class_decl->name);
    fprintf(out, "    cn_register_type_info(&_%.*s_type_info);\n",
            (int)class_decl->name_length, class_decl->name);
    fprintf(out, "}\n\n");
    
    return true;
}

bool cn_cgen_class_decl(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl) {
    if (!ctx || !ctx->output_file || !class_decl) return false;
    
    FILE *out = ctx->output_file;
    
    // 类注释
    fprintf(out, "// ========================================\n");
    fprintf(out, "// 类定义: %.*s\n",
            (int)class_decl->name_length, class_decl->name);
    if (class_decl->is_abstract) {
        fprintf(out, "// (抽象类)\n");
    }
    if (class_decl->is_final) {
        fprintf(out, "// (最终类)\n");
    }
    fprintf(out, "// ========================================\n\n");
    
    // 1. 生成静态成员变量（全局变量）
    cgen_static_fields(out, class_decl);
    
    // 2. 生成结构体定义
    if (!cn_cgen_class_struct(ctx, class_decl)) {
        return false;
    }
    
    // 3. 生成方法前向声明（必须在vtable之前，因为vtable初始化引用这些函数）
    fprintf(out, "// 类 %.*s 的方法前向声明\n",
            (int)class_decl->name_length, class_decl->name);
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD) {
            cgen_method_forward_decl(out, class_decl, member);
        } else if (member->kind == CN_MEMBER_CONSTRUCTOR) {
            cgen_constructor_forward_decl(out, class_decl, member);
        } else if (member->kind == CN_MEMBER_DESTRUCTOR) {
            cgen_destructor_forward_decl(out, class_decl, member);
        }
    }
    fprintf(out, "\n");
    
    // 4. 生成虚函数表（如果需要）
    if (!cn_cgen_vtable(ctx, class_decl)) {
        return false;
    }
    
    // 5. 生成类型信息结构（RTTI）
    if (!cn_cgen_type_info(ctx, class_decl)) {
        return false;
    }
    
    // 6. 生成成员函数实现
    if (!cn_cgen_class_methods(ctx, class_decl)) {
        return false;
    }
    
    return true;
}

/**
 * @brief 生成接口纯虚函数占位符函数
 *
 * 接口方法都是纯虚函数，需要生成占位符函数用于运行时错误报告
 *
 * @param out 输出文件
 * @param interface_decl 接口声明
 * @param method 接口方法
 */
static void cgen_interface_pure_virtual_error_func(FILE *out,
                                                    CnAstInterfaceDecl *interface_decl,
                                                    CnClassMember *method) {
    if (!out || !method) return;
    
    // 返回类型
    const char *return_type = "void";
    if (method->type) {
        return_type = get_c_type_string(method->type);
    }
    
    // 函数名：接口名_方法名_interface_pure_virtual_error
    fprintf(out, "static %s %.*s_%.*s_interface_pure_virtual_error(void* self",
            return_type,
            (int)interface_decl->name_length, interface_decl->name,
            (int)method->name_length, method->name);
    
    // 输出其他参数（保持与实际方法相同的签名）
    for (size_t i = 0; i < method->parameter_count; i++) {
        CnAstParameter *param = &method->parameters[i];
        const char *param_type = "int";
        if (param->declared_type) {
            param_type = get_c_type_string(param->declared_type);
        }
        fprintf(out, ", %s %.*s", param_type,
                (int)param->name_length, param->name);
    }
    
    fprintf(out, ") {\n");
    
    // 输出错误信息
    fprintf(out, "    fprintf(stderr, \"错误: 调用了接口纯虚函数 %.*s::%.*s\\n\");\n",
            (int)interface_decl->name_length, interface_decl->name,
            (int)method->name_length, method->name);
    fprintf(out, "    fprintf(stderr, \"  接口: %.*s\\n\");\n",
            (int)interface_decl->name_length, interface_decl->name);
    fprintf(out, "    fprintf(stderr, \"  方法: %.*s\\n\");\n",
            (int)method->name_length, method->name);
    fprintf(out, "    fprintf(stderr, \"  请在实现类中实现此方法\\n\");\n");
    fprintf(out, "    abort();\n");
    fprintf(out, "}\n\n");
}

bool cn_cgen_interface_decl(CnCCodeGenContext *ctx, CnAstInterfaceDecl *interface_decl) {
    if (!ctx || !ctx->output_file || !interface_decl) return false;
    
    FILE *out = ctx->output_file;
    
    // 接口注释
    fprintf(out, "// ========================================\n");
    fprintf(out, "// 接口定义: %.*s\n",
            (int)interface_decl->name_length, interface_decl->name);
    
    // 如果有基接口，显示继承关系
    if (interface_decl->base_interface_count > 0) {
        fprintf(out, "// 继承接口: ");
        for (size_t i = 0; i < interface_decl->base_interface_count; i++) {
            CnInheritanceInfo *base = &interface_decl->base_interfaces[i];
            if (i > 0) fprintf(out, ", ");
            fprintf(out, "%.*s", (int)base->base_class_name_length, base->base_class_name);
        }
        fprintf(out, "\n");
    }
    fprintf(out, "// ========================================\n\n");
    
    // 第一步：生成纯虚函数占位符函数
    for (size_t i = 0; i < interface_decl->method_count; i++) {
        CnClassMember *method = &interface_decl->methods[i];
        cgen_interface_pure_virtual_error_func(out, interface_decl, method);
    }
    
    // 第二步：接口转换为虚函数表结构体
    fprintf(out, "typedef struct %.*s_vtable {\n",
            (int)interface_decl->name_length, interface_decl->name);
    
    // 如果有基接口，先包含基接口的vtable
    if (interface_decl->base_interface_count > 0) {
        for (size_t i = 0; i < interface_decl->base_interface_count; i++) {
            CnInheritanceInfo *base = &interface_decl->base_interfaces[i];
            print_indent(out, 1);
            fprintf(out, "struct %.*s_vtable %.*s_base;  // 基接口 %.*s\n",
                    (int)base->base_class_name_length, base->base_class_name,
                    (int)base->base_class_name_length, base->base_class_name,
                    (int)base->base_class_name_length, base->base_class_name);
        }
    }
    
    // 生成所有接口方法的函数指针
    for (size_t i = 0; i < interface_decl->method_count; i++) {
        CnClassMember *method = &interface_decl->methods[i];
        
        print_indent(out, 1);
        
        // 返回类型
        const char *return_type = "void";
        if (method->type) {
            return_type = get_c_type_string(method->type);
        }
        
        // 函数指针（接口方法都是纯虚函数）
        fprintf(out, "%s (*%.*s)(void* self",
                return_type,
                (int)method->name_length, method->name);
        
        // 其他参数
        for (size_t j = 0; j < method->parameter_count; j++) {
            CnAstParameter *param = &method->parameters[j];
            const char *param_type = "int";
            if (param->declared_type) {
                param_type = get_c_type_string(param->declared_type);
            }
            fprintf(out, ", %s %.*s", param_type,
                    (int)param->name_length, param->name);
        }
        
        fprintf(out, ");\n");
    }
    
    fprintf(out, "} %.*s_vtable;\n\n",
            (int)interface_decl->name_length, interface_decl->name);
    
    // 第三步：生成接口vtable实例（静态变量，所有方法指向占位符函数）
    fprintf(out, "static %.*s_vtable _%.*s_vtable = {\n",
            (int)interface_decl->name_length, interface_decl->name,
            (int)interface_decl->name_length, interface_decl->name);
    
    // 初始化基接口vtable
    if (interface_decl->base_interface_count > 0) {
        for (size_t i = 0; i < interface_decl->base_interface_count; i++) {
            CnInheritanceInfo *base = &interface_decl->base_interfaces[i];
            print_indent(out, 1);
            fprintf(out, ".%.*s_base = {0},  // 基接口 %.*s 的vtable需要由实现类填充\n",
                    (int)base->base_class_name_length, base->base_class_name,
                    (int)base->base_class_name_length, base->base_class_name);
        }
    }
    
    // 初始化接口方法（指向占位符函数）
    bool first = (interface_decl->base_interface_count == 0);
    for (size_t i = 0; i < interface_decl->method_count; i++) {
        CnClassMember *method = &interface_decl->methods[i];
        if (!first) {
            fprintf(out, ",\n");
        }
        first = false;
        
        print_indent(out, 1);
        fprintf(out, ".%.*s = %.*s_%.*s_interface_pure_virtual_error",
                (int)method->name_length, method->name,
                (int)interface_decl->name_length, interface_decl->name,
                (int)method->name_length, method->name);
    }
    
    fprintf(out, "\n};\n\n");
    
    return true;
}

/* ============================================================================
 * 虚函数调用代码生成实现
 * ============================================================================ */

/**
 * @brief 在类层次结构中查找成员方法
 *
 * @param class_decl 类声明
 * @param method_name 方法名
 * @param method_name_len 方法名长度
 * @return 找到的成员指针，未找到返回NULL
 */
static CnClassMember *find_method_in_class(CnAstClassDecl *class_decl,
                                            const char *method_name,
                                            size_t method_name_len) {
    if (!class_decl || !method_name) return NULL;
    
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD &&
            member->name_length == method_name_len &&
            memcmp(member->name, method_name, method_name_len) == 0) {
            return member;
        }
    }
    
    return NULL;
}

/**
 * @brief 在继承链中递归查找虚函数
 *
 * 深度优先遍历继承链，查找指定方法是否为虚函数
 *
 * @param class_decl 类声明
 * @param method_name 方法名
 * @param method_name_len 方法名长度
 * @param out_method 输出参数，找到的方法成员
 * @param visited 已访问类名数组（防止循环继承）
 * @param visited_count 已访问类名数量
 * @return true 是虚函数，false 不是虚函数或未找到
 */
static bool find_virtual_in_inheritance_chain(CnAstClassDecl *class_decl,
                                               const char *method_name,
                                               size_t method_name_len,
                                               CnClassMember **out_method,
                                               const char **visited,
                                               size_t *visited_count) {
    if (!class_decl || !method_name) return false;
    
    // 检查是否已访问（防止循环继承）
    for (size_t i = 0; i < *visited_count; i++) {
        if (visited[i] &&
            memcmp(visited[i], class_decl->name, class_decl->name_length) == 0) {
            return false;  // 已访问，避免循环
        }
    }
    
    // 标记为已访问
    if (*visited_count < 32) {
        visited[*visited_count] = class_decl->name;
        (*visited_count)++;
    }
    
    // 在当前类中查找方法
    CnClassMember *method = find_method_in_class(class_decl, method_name, method_name_len);
    
    // 如果在当前类找到
    if (method) {
        if (out_method) {
            *out_method = method;
        }
        // 如果当前类重写了该方法，返回其虚函数属性
        return method->is_virtual;
    }
    
    // 当前类未找到，在基类中递归查找
    // 注意：如果基类中该方法是虚函数，则继承后仍然是虚函数
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        // 简化实现：假设基类信息中包含虚函数标记
        // 完整实现需要通过符号表查找基类的声明
        // 这里我们检查基类名称，如果方法名匹配基类中的虚函数，则返回true
        
        // 注意：由于没有实际的基类声明，这里无法直接查找
        // 但我们可以通过基类信息中的标记来判断
        // 如果基类有虚函数，且方法名匹配，则认为是虚函数
        
        (void)base_info;  // 暂时标记为使用
    }
    
    return false;
}

bool cn_cgen_is_virtual_method(CnAstClassDecl *class_decl,
                                const char *method_name, size_t method_name_len,
                                CnClassMember **out_method) {
    if (!class_decl || !method_name) {
        return false;
    }
    
    // 已访问类名数组（防止循环继承）
    const char *visited[32] = {0};
    size_t visited_count = 0;
    
    return find_virtual_in_inheritance_chain(class_decl, method_name, method_name_len,
                                              out_method, visited, &visited_count);
}

/**
 * @brief 生成表达式的C代码（简化版，用于参数）
 */
static void cgen_expr_for_arg(FILE *out, CnAstExpr *expr) {
    if (!out || !expr) {
        fprintf(out, "0");
        return;
    }
    
    switch (expr->kind) {
        case CN_AST_EXPR_INTEGER_LITERAL:
            fprintf(out, "%lld", expr->as.integer_literal.value);
            break;
        case CN_AST_EXPR_FLOAT_LITERAL:
            fprintf(out, "%f", expr->as.float_literal.value);
            break;
        case CN_AST_EXPR_STRING_LITERAL:
            fprintf(out, "\"");
            for (const char *p = expr->as.string_literal.value; p && *p; p++) {
                switch (*p) {
                    case '\n': fprintf(out, "\\n"); break;
                    case '\t': fprintf(out, "\\t"); break;
                    case '\r': fprintf(out, "\\r"); break;
                    case '\\': fprintf(out, "\\\\"); break;
                    case '"': fprintf(out, "\\\""); break;
                    default: fprintf(out, "%c", *p); break;
                }
            }
            fprintf(out, "\"");
            break;
        case CN_AST_EXPR_BOOL_LITERAL:
            fprintf(out, "%s", expr->as.bool_literal.value ? "1" : "0");
            break;
        case CN_AST_EXPR_IDENTIFIER:
            fprintf(out, "%.*s", (int)expr->as.identifier.name_length, expr->as.identifier.name);
            break;
        case CN_AST_EXPR_MEMBER_ACCESS:
            // 成员访问：obj.member 或 ptr->member 或 类名.静态成员
            if (expr->as.member.is_static_member && expr->as.member.class_name) {
                // 静态成员访问：生成 类名_成员名
                fprintf(out, "%.*s_%.*s",
                        (int)expr->as.member.class_name_length, expr->as.member.class_name,
                        (int)expr->as.member.member_name_length, expr->as.member.member_name);
            } else {
                cgen_expr_for_arg(out, expr->as.member.object);
                if (expr->as.member.is_arrow) {
                    fprintf(out, "->");
                } else {
                    fprintf(out, ".");
                }
                fprintf(out, "%.*s", (int)expr->as.member.member_name_length, expr->as.member.member_name);
            }
            break;
        case CN_AST_EXPR_BINARY:
            // 检查是否为字符串拼接
            if (is_string_concat(expr)) {
                fprintf(out, "cn_rt_string_concat(");
                cgen_expr_for_arg(out, expr->as.binary.left);
                fprintf(out, ", ");
                cgen_expr_for_arg(out, expr->as.binary.right);
                fprintf(out, ")");
                break;
            }
            fprintf(out, "(");
            cgen_expr_for_arg(out, expr->as.binary.left);
            switch (expr->as.binary.op) {
                case CN_AST_BINARY_OP_ADD: fprintf(out, " + "); break;
                case CN_AST_BINARY_OP_SUB: fprintf(out, " - "); break;
                case CN_AST_BINARY_OP_MUL: fprintf(out, " * "); break;
                case CN_AST_BINARY_OP_DIV: fprintf(out, " / "); break;
                case CN_AST_BINARY_OP_MOD: fprintf(out, " %% "); break;
                case CN_AST_BINARY_OP_EQ: fprintf(out, " == "); break;
                case CN_AST_BINARY_OP_NE: fprintf(out, " != "); break;
                case CN_AST_BINARY_OP_LT: fprintf(out, " < "); break;
                case CN_AST_BINARY_OP_LE: fprintf(out, " <= "); break;
                case CN_AST_BINARY_OP_GT: fprintf(out, " > "); break;
                case CN_AST_BINARY_OP_GE: fprintf(out, " >= "); break;
                default: fprintf(out, " ? "); break;
            }
            cgen_expr_for_arg(out, expr->as.binary.right);
            fprintf(out, ")");
            break;
        default:
            fprintf(out, "0");
            break;
    }
}

bool cn_cgen_virtual_call(CnCCodeGenContext *ctx,
                          const char *class_name, size_t class_name_len,
                          const char *method_name, size_t method_name_len,
                          const char *object_expr,
                          CnAstExpr **args, size_t arg_count) {
    if (!ctx || !ctx->output_file || !class_name || !method_name || !object_expr) {
        return false;
    }
    
    FILE *out = ctx->output_file;
    
    // 生成虚函数调用：obj->vtable->method(obj, args...)
    // 格式：object_expr->vtable->method_name(object_expr, arg1, arg2, ...)
    fprintf(out, "%s->vtable->%.*s(%s",
            object_expr,
            (int)method_name_len, method_name,
            object_expr);
    
    // 输出参数
    if (args && arg_count > 0) {
        for (size_t i = 0; i < arg_count; i++) {
            fprintf(out, ", ");
            cgen_expr_for_arg(out, args[i]);
        }
    }
    
    fprintf(out, ")");
    
    return true;
}

void cn_cgen_member_access_call(CnCCodeGenContext *ctx,
                                 CnAstExpr *object_expr,
                                 const char *member_name, size_t member_name_len,
                                 bool is_arrow,
                                 CnAstExpr **args, size_t arg_count,
                                 CnAstClassDecl *class_decl) {
    if (!ctx || !ctx->output_file || !object_expr || !member_name) {
        return;
    }
    
    FILE *out = ctx->output_file;
    
    // 检查是否为虚函数调用
    CnClassMember *method = NULL;
    bool is_virtual = false;
    
    if (class_decl) {
        is_virtual = cn_cgen_is_virtual_method(class_decl, member_name, member_name_len, &method);
    }
    
    if (is_virtual && method) {
        // 虚函数调用：通过vtable调用
        // 首先生成对象表达式
        char object_buf[256] = {0};
        
        // 使用临时缓冲区捕获对象表达式
        // 注意：这里简化处理，直接生成到输出文件
        // 完整实现需要使用字符串缓冲区
        
        // 生成对象表达式到临时变量
        fprintf(out, "({ ");
        fprintf(out, "struct %.*s* _obj = ", (int)class_decl->name_length, class_decl->name);
        
        // 生成对象表达式
        if (object_expr->is_this_pointer) {
            fprintf(out, "self");
        } else {
            cgen_expr_for_arg(out, object_expr);
        }
        fprintf(out, "; ");
        
        // 生成虚函数调用
        fprintf(out, "_obj->vtable->%.*s(_obj", (int)member_name_len, member_name);
        
        // 输出参数
        if (args && arg_count > 0) {
            for (size_t i = 0; i < arg_count; i++) {
                fprintf(out, ", ");
                cgen_expr_for_arg(out, args[i]);
            }
        }
        
        fprintf(out, "); })");
    } else {
        // 普通成员访问或非虚函数调用
        if (object_expr->is_this_pointer) {
            // 自身指针访问
            fprintf(out, "self->%.*s", (int)member_name_len, member_name);
        } else {
            // 普通对象访问
            cgen_expr_for_arg(out, object_expr);
            if (is_arrow) {
                fprintf(out, "->");
            } else {
                fprintf(out, ".");
            }
            fprintf(out, "%.*s", (int)member_name_len, member_name);
        }
        
        // 如果有参数，生成函数调用
        if (args && arg_count > 0) {
            fprintf(out, "(");
            
            // 非虚成员函数需要传递self指针作为第一个参数
            if (class_decl && method && !method->is_static) {
                if (object_expr->is_this_pointer) {
                    fprintf(out, "self");
                } else {
                    cgen_expr_for_arg(out, object_expr);
                }
            }
            
            for (size_t i = 0; i < arg_count; i++) {
                if (i > 0 || (class_decl && method && !method->is_static)) {
                    fprintf(out, ", ");
                }
                cgen_expr_for_arg(out, args[i]);
            }
            fprintf(out, ")");
        }
    }
}

/* ============================================================================
 * 多继承this指针调整函数实现
 * ============================================================================ */

/**
 * @brief 生成基类方法调用时的this指针调整代码
 *
 * 当调用基类的方法时，需要将派生类的this指针调整为基类子对象的指针
 * 例如：C类继承A和B，调用B的方法时需要：
 *   B* b_ptr = (B*)((char*)c_ptr + B_OFFSET);
 *   b_ptr->method(b_ptr);
 *
 * @param ctx 代码生成上下文
 * @param class_decl 派生类声明
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @param method_name 方法名称
 * @param method_name_len 方法名称长度
 * @param args 参数表达式数组
 * @param arg_count 参数数量
 */
void cn_cgen_base_method_call(CnCCodeGenContext *ctx,
                               CnAstClassDecl *class_decl,
                               const char *base_name, size_t base_name_len,
                               const char *method_name, size_t method_name_len,
                               CnAstExpr **args, size_t arg_count) {
    if (!ctx || !ctx->output_file || !class_decl || !base_name || !method_name) {
        return;
    }
    
    FILE *out = ctx->output_file;
    
    // 查找基类信息
    CnInheritanceInfo *base_info = NULL;
    size_t base_index = 0;
    for (size_t i = 0; i < class_decl->base_count; i++) {
        if (class_decl->bases[i].base_class_name_length == base_name_len &&
            memcmp(class_decl->bases[i].base_class_name, base_name, base_name_len) == 0) {
            base_info = &class_decl->bases[i];
            base_index = i;
            break;
        }
    }
    
    if (!base_info) {
        // 基类未找到，生成错误占位符
        fprintf(out, "/* 错误: 基类 %.*s 未找到 */", (int)base_name_len, base_name);
        return;
    }
    
    // 生成this指针调整和方法调用
    // 使用语句表达式 ({ ... }) 来实现临时变量
    fprintf(out, "({ ");
    
    // 声明基类指针临时变量
    fprintf(out, "struct %.*s* _base_ptr = (struct %.*s*)((char*)self + %.*s_%.*s_OFFSET); ",
            (int)base_name_len, base_name,
            (int)base_name_len, base_name,
            (int)class_decl->name_length, class_decl->name,
            (int)base_name_len, base_name);
    
    // 调用基类方法
    fprintf(out, "%.*s_%.*s(_base_ptr",
            (int)base_name_len, base_name,
            (int)method_name_len, method_name);
    
    // 输出参数
    if (args && arg_count > 0) {
        for (size_t i = 0; i < arg_count; i++) {
            fprintf(out, ", ");
            cgen_expr_for_arg(out, args[i]);
        }
    }
    
    fprintf(out, "); })");
}

/**
 * @brief 生成向上转型代码（派生类指针转基类指针）
 *
 * @param out 输出文件
 * @param class_decl 派生类声明
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @param expr 派生类对象表达式
 */
void cn_cgen_upcast(FILE *out, CnAstClassDecl *class_decl,
                    const char *base_name, size_t base_name_len,
                    const char *expr) {
    if (!out || !class_decl || !base_name || !expr) {
        return;
    }
    
    // 使用宏进行向上转型
    fprintf(out, "%.*s_TO_%.*s(%s)",
            (int)class_decl->name_length, class_decl->name,
            (int)base_name_len, base_name,
            expr);
}

/**
 * @brief 生成向下转型代码（基类指针转派生类指针）
 *
 * 注意：向下转型需要运行时类型检查（RTTI），这里生成简化版本
 *
 * @param out 输出文件
 * @param class_decl 派生类声明
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @param expr 基类对象表达式
 */
void cn_cgen_downcast(FILE *out, CnAstClassDecl *class_decl,
                      const char *base_name, size_t base_name_len,
                      const char *expr) {
    if (!out || !class_decl || !base_name || !expr) {
        return;
    }
    
    // 向下转型需要减去偏移量
    fprintf(out, "((struct %.*s*)((char*)(%s) - %.*s_%.*s_OFFSET))",
            (int)class_decl->name_length, class_decl->name,
            expr,
            (int)class_decl->name_length, class_decl->name,
            (int)base_name_len, base_name);
}

/**
 * @brief 生成基类成员访问代码
 *
 * 访问基类的成员变量时，需要正确计算偏移量
 *
 * @param out 输出文件
 * @param class_decl 派生类声明
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @param member_name 成员名称
 * @param member_name_len 成员名称长度
 * @param is_arrow 是否使用箭头操作符
 */
void cn_cgen_base_member_access(FILE *out, CnAstClassDecl *class_decl,
                                 const char *base_name, size_t base_name_len,
                                 const char *member_name, size_t member_name_len,
                                 bool is_arrow) {
    if (!out || !class_decl || !base_name || !member_name) {
        return;
    }
    
    // 直接通过基类子对象成员访问
    // self->base_name_base.member 或 self->base_name_base->member
    if (is_arrow) {
        fprintf(out, "self->%.*s_base.%.*s",
                (int)base_name_len, base_name,
                (int)member_name_len, member_name);
    } else {
        fprintf(out, "self->%.*s_base.%.*s",
                (int)base_name_len, base_name,
                (int)member_name_len, member_name);
    }
}
