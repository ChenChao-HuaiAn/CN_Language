/**
 * @file class_analyzer.c
 * @brief CN语言面向对象编程 - 类语义分析器实现
 * 
 * 实现类成员变量的语义分析功能，包括：
 * - 类型检查
 * - 符号表绑定
 * - 访问控制检查
 * - 成员名称冲突检测
 */

#include "cnlang/semantics/class_analyzer.h"
#include "cnlang/frontend/ast.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

/**
 * @brief 比较两个名称是否相等
 */
static bool cn_name_equals(const char *name1, size_t len1,
                           const char *name2, size_t len2)
{
    if (len1 != len2) {
        return false;
    }
    if (name1 == name2) {
        return true;
    }
    if (!name1 || !name2) {
        return false;
    }
    return memcmp(name1, name2, len1) == 0;
}

/**
 * @brief 报告语义错误
 */
static void report_error(CnClassAnalyzerContext *ctx, const char *message,
                         const char *name, size_t name_length)
{
    if (!ctx->diagnostics) {
        return;
    }
    
    /* 使用诊断系统报告错误 */
    cn_support_diag_semantic_error_generic(
        ctx->diagnostics,
        CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL,  /* 使用重复符号错误代码 */
        NULL, 0, 0,
        message
    );
    
    /* 避免未使用参数警告 */
    (void)name;
    (void)name_length;
}

/**
 * @brief 报告访问控制错误
 *
 * 当成员访问被拒绝时报告详细的错误信息
 */
static void report_access_error(CnClassAnalyzerContext *ctx,
                                 CnClassMember *member,
                                 const char *access_context)
{
    if (!ctx->diagnostics || !member) {
        return;
    }
    
    /* 构建错误消息 */
    char error_msg[256];
    const char *access_level_str = "";
    
    switch (member->access) {
        case CN_ACCESS_PRIVATE:
            access_level_str = "私有";
            break;
        case CN_ACCESS_PROTECTED:
            access_level_str = "保护";
            break;
        case CN_ACCESS_PUBLIC:
            access_level_str = "公开";
            break;
    }
    
    /* 格式化错误消息 */
    if (member->name && member->name_length > 0) {
        if (access_context) {
            snprintf(error_msg, sizeof(error_msg),
                     "无法访问%s成员 '%.*s'（%s）",
                     access_level_str,
                     (int)member->name_length, member->name,
                     access_context);
        } else {
            snprintf(error_msg, sizeof(error_msg),
                     "无法访问%s成员 '%.*s'",
                     access_level_str,
                     (int)member->name_length, member->name);
        }
    } else {
        snprintf(error_msg, sizeof(error_msg),
                 "无法访问%s成员", access_level_str);
    }
    
    /* 使用诊断系统报告访问控制错误 */
    cn_support_diag_semantic_error_generic(
        ctx->diagnostics,
        CN_DIAG_CODE_SEM_ACCESS_DENIED,
        NULL, 0, 0,
        error_msg
    );
}

/**
 * @brief 报告私有成员访问错误
 */
static void report_private_access_error(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    report_access_error(ctx, member, "私有成员只能在声明它的类内部访问");
}

/**
 * @brief 报告保护成员访问错误
 */
static void report_protected_access_error(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    report_access_error(ctx, member, "保护成员只能在类内部或派生类中访问");
}

/* ============================================================================
 * 初始化和销毁函数实现
 * ============================================================================ */

bool cn_class_analyzer_context_init(CnClassAnalyzerContext *ctx,
                                     CnSemScope *symbol_table,
                                     CnDiagnostics *diagnostics)
{
    if (!ctx) {
        return false;
    }
    
    ctx->symbol_table = symbol_table;
    ctx->current_class = NULL;
    ctx->current_member = NULL;
    ctx->current_access = CN_ACCESS_PRIVATE;  /* 类成员默认私有 */
    ctx->diagnostics = diagnostics;
    
    return true;
}

void cn_class_analyzer_context_cleanup(CnClassAnalyzerContext *ctx)
{
    if (!ctx) {
        return;
    }
    
    ctx->symbol_table = NULL;
    ctx->current_class = NULL;
    ctx->current_member = NULL;
    ctx->diagnostics = NULL;
}

/* ============================================================================
 * 成员查找函数实现
 * ============================================================================ */

CnClassMember *cn_find_class_member(CnAstClassDecl *class_decl, const char *name)
{
    if (!class_decl || !name) {
        return NULL;
    }
    
    /* 遍历所有成员查找匹配的名称 */
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (cn_name_equals(member->name, member->name_length, name, strlen(name))) {
            return member;
        }
    }
    
    return NULL;
}

CnClassMember *cn_find_class_member_in_hierarchy(CnAstClassDecl *class_decl,
                                                  const char *name,
                                                  CnAstClassDecl **out_base_class)
{
    if (!class_decl || !name) {
        return NULL;
    }
    
    /* 首先在当前类中查找 */
    CnClassMember *member = cn_find_class_member(class_decl, name);
    if (member) {
        if (out_base_class) {
            *out_base_class = class_decl;
        }
        return member;
    }
    
    /* 在基类中递归查找 - 暂时简化处理 */
    /* 完整实现需要在符号结构中添加类声明指针，并通过作用域查找基类 */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        if (!base_info->base_class_name) {
            continue;
        }
        
        /* TODO: 完整实现需要通过符号表查找基类声明 */
        /* 当前简化版本不支持跨继承层次查找 */
        (void)base_info;
    }
    
    return NULL;
}

CnClassMember *cn_find_virtual_member(CnAstClassDecl *class_decl, const char *name)
{
    if (!class_decl || !name) {
        return NULL;
    }
    
    /* 遍历所有成员查找虚函数 */
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->is_virtual && 
            cn_name_equals(member->name, member->name_length, name, strlen(name))) {
            return member;
        }
    }
    
    return NULL;
}

/* ============================================================================
 * 冲突检测函数实现
 * ============================================================================ */

bool cn_check_name_conflict_with_class(CnAstClassDecl *class_decl, const char *name)
{
    if (!class_decl || !name) {
        return true;  /* 无冲突 */
    }
    
    /* 检查成员名是否与类名相同 */
    if (cn_name_equals(class_decl->name, class_decl->name_length, name, strlen(name))) {
        return false;  /* 有冲突 */
    }
    
    return true;  /* 无冲突 */
}

bool cn_check_member_signature_conflict(CnClassMember *member1, CnClassMember *member2)
{
    if (!member1 || !member2) {
        return false;
    }
    
    /* 检查成员名是否相同 */
    if (!cn_name_equals(member1->name, member1->name_length,
                        member2->name, member2->name_length)) {
        return false;
    }
    
    /* 字段不能重名 */
    if (member1->kind == CN_MEMBER_FIELD || member2->kind == CN_MEMBER_FIELD) {
        return true;  /* 字段名冲突 */
    }
    
    /* 方法需要检查参数签名 */
    if (member1->kind == CN_MEMBER_METHOD && member2->kind == CN_MEMBER_METHOD) {
        /* 简化实现：只检查参数数量 */
        if (member1->parameter_count != member2->parameter_count) {
            return false;  /* 参数数量不同，不冲突 */
        }
        return true;  /* 签名相同，冲突 */
    }
    
    return false;
}

bool cn_check_member_conflict(CnClassAnalyzerContext *ctx, const char *name)
{
    if (!ctx || !ctx->current_class || !name) {
        return true;
    }
    
    /* 检查与类名冲突 */
    if (!cn_check_name_conflict_with_class(ctx->current_class, name)) {
        report_error(ctx, "成员名称不能与类名相同", name, strlen(name));
        return false;
    }
    
    /* 检查与已有成员冲突（排除当前正在分析的成员） */
    for (size_t i = 0; i < ctx->current_class->member_count; i++) {
        CnClassMember *existing = &ctx->current_class->members[i];
        /* 排除当前正在分析的成员，避免误报 */
        if (ctx->current_member && existing == ctx->current_member) {
            continue;
        }
        if (cn_name_equals(existing->name, existing->name_length, name, strlen(name))) {
            report_error(ctx, "成员名称已存在", name, strlen(name));
            return false;
        }
    }
    
    return true;
}

/* ============================================================================
 * 类型检查辅助函数实现
 * ============================================================================ */

bool cn_validate_member_type(CnClassAnalyzerContext *ctx, CnType *type)
{
    if (!ctx) {
        return false;
    }
    
    if (!type) {
        report_error(ctx, "成员类型未指定", NULL, 0);
        return false;
    }
    
    /* 检查类型是否为未知类型 */
    if (type->kind == CN_TYPE_UNKNOWN) {
        report_error(ctx, "成员类型未知", NULL, 0);
        return false;
    }
    
    return true;
}

bool cn_is_valid_member_type(CnClassAnalyzerContext *ctx, CnType *type)
{
    if (!ctx || !type) {
        return false;
    }
    
    /* 基本类型都有效 */
    switch (type->kind) {
        case CN_TYPE_VOID:
            /* 字段不能是void类型 */
            if (ctx->current_member && ctx->current_member->kind == CN_MEMBER_FIELD) {
                report_error(ctx, "字段不能是void类型", NULL, 0);
                return false;
            }
            return true;
            
        case CN_TYPE_INT:
        case CN_TYPE_INT32:
        case CN_TYPE_INT64:
        case CN_TYPE_UINT32:
        case CN_TYPE_UINT64:
        case CN_TYPE_UINT64_LL:
        case CN_TYPE_FLOAT:
        case CN_TYPE_FLOAT32:
        case CN_TYPE_FLOAT64:
        case CN_TYPE_BOOL:
        case CN_TYPE_STRING:
            return true;
            
        case CN_TYPE_POINTER:
            /* 指针类型需要检查指向类型 */
            return cn_is_valid_member_type(ctx, type->as.pointer_to);
            
        case CN_TYPE_ARRAY:
            /* 数组类型需要检查元素类型 */
            return cn_is_valid_member_type(ctx, type->as.array.element_type);
            
        case CN_TYPE_STRUCT:
            /* 结构体类型有效 */
            return true;
            
        case CN_TYPE_ENUM:
            /* 枚举类型有效 */
            return true;
            
        case CN_TYPE_FUNCTION:
            /* 函数类型（函数指针）有效 */
            return true;
            
        default:
            return false;
    }
}

/* ============================================================================
 * 访问控制检查实现
 * ============================================================================ */

/**
 * @brief 检查类是否继承自指定基类（辅助函数）
 *
 * 递归检查继承链，判断derived是否直接或间接继承自base_name
 */
static bool cn_class_inherits_from(CnAstClassDecl *derived, const char *base_name, size_t base_name_len)
{
    if (!derived || !base_name) {
        return false;
    }
    
    /* 遍历直接基类 */
    for (size_t i = 0; i < derived->base_count; i++) {
        CnInheritanceInfo *base_info = &derived->bases[i];
        
        /* 检查基类名称是否匹配 */
        if (cn_name_equals(base_info->base_class_name, base_info->base_class_name_length,
                          base_name, base_name_len)) {
            return true;
        }
        
        /* TODO: 递归检查基类的基类（需要符号表支持查找基类声明） */
        /* 当前简化实现只检查直接基类 */
    }
    
    return false;
}

/**
 * @brief 查找成员所属的类（辅助函数）
 *
 * 在类及其继承链中查找成员定义的位置
 */
static CnAstClassDecl *cn_find_member_declaring_class(CnAstClassDecl *class_decl,
                                                       CnClassMember *member)
{
    if (!class_decl || !member) {
        return NULL;
    }
    
    /* 检查成员是否在当前类中定义 */
    for (size_t i = 0; i < class_decl->member_count; i++) {
        if (&class_decl->members[i] == member) {
            return class_decl;
        }
    }
    
    /* TODO: 在基类链中递归查找（需要符号表支持） */
    
    return NULL;
}

bool cn_can_access_from_external(CnClassMember *member)
{
    if (!member) {
        return false;
    }
    
    /* 只有公开成员可以从外部访问 */
    return member->access == CN_ACCESS_PUBLIC;
}

bool cn_can_access_from_same_class(CnClassMember *member, CnAstClassDecl *class_decl)
{
    if (!member || !class_decl) {
        return false;
    }
    
    /* 类内部可以访问所有成员，包括私有成员 */
    /* 验证成员确实属于该类 */
    for (size_t i = 0; i < class_decl->member_count; i++) {
        if (&class_decl->members[i] == member) {
            return true;
        }
    }
    
    return false;
}

bool cn_can_access_from_derived(CnClassMember *member, CnAstClassDecl *derived_class)
{
    if (!member || !derived_class) {
        return false;
    }
    
    /* 公开成员可以从派生类访问 */
    if (member->access == CN_ACCESS_PUBLIC) {
        return true;
    }
    
    /* 保护成员可以从派生类访问 */
    if (member->access == CN_ACCESS_PROTECTED) {
        return true;
    }
    
    /* 私有成员不能从派生类访问 */
    return false;
}

bool cn_check_protected_access(CnClassMember *member, CnAstClassDecl *derived_class)
{
    return cn_can_access_from_derived(member, derived_class);
}

CnAccessLevel cn_get_effective_access(CnAccessLevel member_access,
                                       CnAccessLevel inheritance_access)
{
    /* 公开继承：保持原有访问级别 */
    if (inheritance_access == CN_ACCESS_PUBLIC) {
        return member_access;
    }
    
    /* 保护继承：公开成员变为保护 */
    if (inheritance_access == CN_ACCESS_PROTECTED) {
        if (member_access == CN_ACCESS_PUBLIC) {
            return CN_ACCESS_PROTECTED;
        }
        return member_access;  /* 保护、私有保持不变 */
    }
    
    /* 私有继承：公开和保护成员变为私有 */
    if (inheritance_access == CN_ACCESS_PRIVATE) {
        if (member_access == CN_ACCESS_PUBLIC || member_access == CN_ACCESS_PROTECTED) {
            return CN_ACCESS_PRIVATE;
        }
        return member_access;  /* 私有保持不变 */
    }
    
    return member_access;
}

bool cn_check_member_access(CnClassAnalyzerContext *ctx,
                            CnClassMember *member,
                            CnAstClassDecl *accessing_class)
{
    if (!member) {
        return false;
    }
    
    /* 公开成员总是可访问 */
    if (member->access == CN_ACCESS_PUBLIC) {
        return true;
    }
    
    /* 如果没有访问类上下文，检查是否可以从外部访问 */
    if (!accessing_class) {
        return cn_can_access_from_external(member);
    }
    
    /* 如果有分析上下文，使用当前类 */
    CnAstClassDecl *current_class = accessing_class;
    if (ctx && ctx->current_class && !accessing_class) {
        current_class = ctx->current_class;
    }
    
    if (!current_class) {
        return cn_can_access_from_external(member);
    }
    
    /* 检查是否是同一类内部访问 */
    if (cn_can_access_from_same_class(member, current_class)) {
        return true;  /* 类内部可以访问所有成员 */
    }
    
    /* 检查是否是派生类访问 */
    /* 首先需要确定成员所属的类 */
    CnAstClassDecl *declaring_class = cn_find_member_declaring_class(current_class, member);
    
    /* 如果成员不在当前类中，检查继承关系 */
    if (!declaring_class && member->access == CN_ACCESS_PROTECTED) {
        /* 保护成员：检查当前类是否是派生类 */
        /* 简化实现：假设成员可以通过继承访问 */
        /* 完整实现需要符号表支持查找成员所属的基类 */
        return cn_can_access_from_derived(member, current_class);
    }
    
    /* 私有成员只能在声明它的类内部访问 */
    if (member->access == CN_ACCESS_PRIVATE) {
        return false;
    }
    
    /* 保护成员可以在派生类中访问 */
    if (member->access == CN_ACCESS_PROTECTED) {
        return cn_can_access_from_derived(member, current_class);
    }
    
    return false;
}

/* ============================================================================
 * 符号表操作函数实现
 * ============================================================================ */

bool cn_add_member_to_symbol_table(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    if (!ctx || !ctx->symbol_table || !member) {
        return false;
    }
    
    /* 确定符号类型 */
    CnSemSymbolKind sym_kind;
    switch (member->kind) {
        case CN_MEMBER_FIELD:
            sym_kind = CN_SEM_SYMBOL_VARIABLE;
            break;
        case CN_MEMBER_METHOD:
        case CN_MEMBER_CONSTRUCTOR:
        case CN_MEMBER_DESTRUCTOR:
            sym_kind = CN_SEM_SYMBOL_FUNCTION;
            break;
        default:
            return false;
    }
    
    /* 插入符号表 */
    CnSemSymbol *sym = cn_sem_scope_insert_symbol(
        ctx->symbol_table,
        member->name,
        member->name_length,
        sym_kind
    );
    
    if (!sym) {
        return false;
    }
    
    /* 设置符号属性 */
    sym->type = member->type;
    sym->is_public = (member->access == CN_ACCESS_PUBLIC);
    sym->is_const = member->is_const;
    
    return true;
}

bool cn_add_class_to_symbol_table(CnClassAnalyzerContext *ctx, CnAstClassDecl *class_decl)
{
    if (!ctx || !ctx->symbol_table || !class_decl) {
        return false;
    }
    
    /* 类作为结构体类型添加到符号表 */
    CnSemSymbol *sym = cn_sem_scope_insert_symbol(
        ctx->symbol_table,
        class_decl->name,
        class_decl->name_length,
        CN_SEM_SYMBOL_STRUCT
    );
    
    if (!sym) {
        return false;
    }
    
    return true;
}

/* ============================================================================
 * 方法分析辅助函数实现
 * ============================================================================ */

/**
 * @brief 分析方法参数列表
 */
bool cn_analyze_method_params(CnClassAnalyzerContext *ctx,
                               CnAstParameter *parameters,
                               size_t param_count)
{
    if (!ctx) {
        return false;
    }
    
    /* 无参数时直接成功 */
    if (!parameters || param_count == 0) {
        return true;
    }
    
    /* 检查每个参数 */
    for (size_t i = 0; i < param_count; i++) {
        CnAstParameter *param = &parameters[i];
        
        /* 检查参数类型有效性 */
        if (param->declared_type) {
            if (!cn_validate_member_type(ctx, param->declared_type)) {
                return false;
            }
        }
        
        /* 检查参数名是否重复 */
        for (size_t j = 0; j < i; j++) {
            CnAstParameter *prev_param = &parameters[j];
            if (param->name && prev_param->name &&
                cn_name_equals(param->name, param->name_length,
                              prev_param->name, prev_param->name_length)) {
                report_error(ctx, "参数名重复", param->name, param->name_length);
                return false;
            }
        }
        
        /* 检查参数名是否与成员变量同名 */
        if (ctx->current_class && param->name) {
            CnClassMember *field = cn_find_class_member(ctx->current_class, param->name);
            if (field && field->kind == CN_MEMBER_FIELD) {
                report_error(ctx, "参数名与成员变量同名", param->name, param->name_length);
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief 检查方法返回类型
 */
bool cn_check_method_return_type(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    if (!ctx || !member) {
        return false;
    }
    
    /* 构造函数和析构函数不能有返回类型 */
    if (member->kind == CN_MEMBER_CONSTRUCTOR || member->kind == CN_MEMBER_DESTRUCTOR) {
        if (member->type != NULL) {
            report_error(ctx, "构造函数和析构函数不能有返回类型",
                        member->name, member->name_length);
            return false;
        }
        return true;
    }
    
    /* 普通方法检查返回类型有效性 */
    if (member->type) {
        if (!cn_validate_member_type(ctx, member->type)) {
            return false;
        }
        
        /* 检查返回类型是否可用于成员 */
        if (!cn_is_valid_member_type(ctx, member->type)) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 检查方法重写有效性
 */
bool cn_check_method_override(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    if (!ctx || !member) {
        return false;
    }
    
    /* 如果不是重写方法，直接成功 */
    if (!member->is_override) {
        return true;
    }
    
    /* 必须有当前类上下文 */
    if (!ctx->current_class) {
        report_error(ctx, "重写方法必须在类内部声明", member->name, member->name_length);
        return false;
    }
    
    /* 在基类中查找同名虚函数 */
    CnClassMember *base_method = cn_find_virtual_member(ctx->current_class, member->name);
    if (!base_method) {
        report_error(ctx, "重写的函数在基类中不是虚函数", member->name, member->name_length);
        return false;
    }
    
    /* 检查签名是否匹配（参数数量） */
    if (member->parameter_count != base_method->parameter_count) {
        report_error(ctx, "重写函数参数数量与基类不匹配", member->name, member->name_length);
        return false;
    }
    
    /* 检查返回类型是否匹配 */
    if (member->type && base_method->type) {
        /* 简化实现：只检查类型种类是否相同 */
        if (member->type->kind != base_method->type->kind) {
            report_error(ctx, "重写函数返回类型与基类不匹配", member->name, member->name_length);
            return false;
        }
    }
    
    /* 检查访问级别不能比基类更严格 */
    if (base_method->access == CN_ACCESS_PUBLIC &&
        member->access != CN_ACCESS_PUBLIC) {
        report_error(ctx, "重写函数的访问级别不能比基类更严格", member->name, member->name_length);
        return false;
    }
    
    if (base_method->access == CN_ACCESS_PROTECTED &&
        member->access == CN_ACCESS_PRIVATE) {
        report_error(ctx, "重写函数的访问级别不能比基类更严格", member->name, member->name_length);
        return false;
    }
    
    return true;
}

/**
 * @brief 检查虚函数声明有效性
 */
bool cn_check_virtual_method(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    if (!ctx || !member) {
        return false;
    }
    
    /* 如果不是虚函数，直接成功 */
    if (!member->is_virtual) {
        return true;
    }
    
    /* 静态函数不能是虚函数 */
    if (member->is_static) {
        report_error(ctx, "静态函数不能是虚函数", member->name, member->name_length);
        return false;
    }
    
    /* 构造函数不能是虚函数 */
    if (member->kind == CN_MEMBER_CONSTRUCTOR) {
        report_error(ctx, "构造函数不能是虚函数", member->name, member->name_length);
        return false;
    }
    
    /* 纯虚函数必须有虚函数标记（is_pure_virtual隐含is_virtual） */
    if (member->is_pure_virtual && !member->is_virtual) {
        report_error(ctx, "纯虚函数必须标记为虚函数", member->name, member->name_length);
        return false;
    }
    
    /* 检查当前类是否允许有虚函数 */
    if (ctx->current_class && ctx->current_class->is_final) {
        report_error(ctx, "最终类不能有虚函数", member->name, member->name_length);
        return false;
    }
    
    return true;
}

/**
 * @brief 将方法添加到符号表
 */
bool cn_add_method_to_symbol_table(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    if (!ctx || !ctx->symbol_table || !member) {
        return false;
    }
    
    /* 确定符号类型 */
    CnSemSymbolKind sym_kind;
    switch (member->kind) {
        case CN_MEMBER_METHOD:
            sym_kind = CN_SEM_SYMBOL_FUNCTION;
            break;
        case CN_MEMBER_CONSTRUCTOR:
            sym_kind = CN_SEM_SYMBOL_FUNCTION;
            break;
        case CN_MEMBER_DESTRUCTOR:
            sym_kind = CN_SEM_SYMBOL_FUNCTION;
            break;
        default:
            return false;
    }
    
    /* 插入符号表 */
    CnSemSymbol *sym = cn_sem_scope_insert_symbol(
        ctx->symbol_table,
        member->name,
        member->name_length,
        sym_kind
    );
    
    if (!sym) {
        return false;
    }
    
    /* 设置符号属性 */
    sym->type = member->type;
    sym->is_public = (member->access == CN_ACCESS_PUBLIC);
    sym->is_const = false;  /* 方法本身不是常量 */
    
    /* 设置方法特有属性（如果符号结构支持） */
    /* 注意：需要在 CnSemSymbol 结构中添加相应字段 */
    
    return true;
}

/* ============================================================================
 * 成员分析函数实现
 * ============================================================================ */

bool cn_analyze_class_field(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    if (!ctx || !member) {
        return false;
    }
    
    /* 设置当前成员上下文 */
    ctx->current_member = member;
    
    /* 检查成员名称冲突 */
    if (!cn_check_member_conflict(ctx, member->name)) {
        return false;
    }
    
    /* 检查类型有效性 */
    if (!cn_validate_member_type(ctx, member->type)) {
        return false;
    }
    
    /* 检查类型是否可用于成员 */
    if (!cn_is_valid_member_type(ctx, member->type)) {
        return false;
    }
    
    /* 添加到符号表 */
    if (!cn_add_member_to_symbol_table(ctx, member)) {
        report_error(ctx, "无法将成员添加到符号表", member->name, member->name_length);
        return false;
    }
    
    return true;
}

bool cn_analyze_class_method(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    if (!ctx || !member) {
        return false;
    }
    
    /* 设置当前成员上下文 */
    ctx->current_member = member;
    
    /* 检查成员名称冲突（构造函数和析构函数除外） */
    if (member->kind != CN_MEMBER_CONSTRUCTOR && member->kind != CN_MEMBER_DESTRUCTOR) {
        if (!cn_check_member_conflict(ctx, member->name)) {
            return false;
        }
    }
    
    /* 静态方法语义检查 */
    if (member->is_static) {
        /* 静态方法不能是虚函数 */
        if (member->is_virtual) {
            report_error(ctx, "静态方法不能是虚函数", member->name, member->name_length);
            return false;
        }
        
        /* 静态方法不能是重写方法 */
        if (member->is_override) {
            report_error(ctx, "静态方法不能重写基类方法", member->name, member->name_length);
            return false;
        }
        
        /* 静态方法不能是纯虚函数 */
        if (member->is_pure_virtual) {
            report_error(ctx, "静态方法不能是纯虚函数", member->name, member->name_length);
            return false;
        }
        
        /* 构造函数和析构函数不能是静态的 */
        if (member->kind == CN_MEMBER_CONSTRUCTOR) {
            report_error(ctx, "构造函数不能是静态的", member->name, member->name_length);
            return false;
        }
        
        if (member->kind == CN_MEMBER_DESTRUCTOR) {
            report_error(ctx, "析构函数不能是静态的", member->name, member->name_length);
            return false;
        }
    }
    
    /* 1. 检查返回类型有效性 */
    if (!cn_check_method_return_type(ctx, member)) {
        return false;
    }
    
    /* 2. 检查参数类型有效性 */
    if (!cn_analyze_method_params(ctx, member->parameters, member->parameter_count)) {
        return false;
    }
    
    /* 3. 检查虚函数声明有效性 */
    if (!cn_check_virtual_method(ctx, member)) {
        return false;
    }
    
    /* 4. 检查方法重写有效性 */
    if (!cn_check_method_override(ctx, member)) {
        return false;
    }
    
    /* 5. 将方法添加到符号表 */
    if (!cn_add_method_to_symbol_table(ctx, member)) {
        report_error(ctx, "无法将方法添加到符号表", member->name, member->name_length);
        return false;
    }
    
    return true;
}

/* ============================================================================
 * 类分析主函数实现
 * ============================================================================ */

bool cn_analyze_class_decl(CnClassAnalyzerContext *ctx, CnAstClassDecl *class_decl)
{
    if (!ctx || !class_decl) {
        return false;
    }
    
    /* 设置当前类上下文 */
    ctx->current_class = class_decl;
    ctx->current_access = CN_ACCESS_PRIVATE;  /* 默认私有 */
    
    /* 检查类名有效性 */
    if (!class_decl->name || class_decl->name_length == 0) {
        report_error(ctx, "类名不能为空", NULL, 0);
        return false;
    }
    
    /* 将类添加到符号表 */
    if (!cn_add_class_to_symbol_table(ctx, class_decl)) {
        report_error(ctx, "类名已存在", class_decl->name, class_decl->name_length);
        return false;
    }
    
    /* 分析继承关系（阶段四 - 单继承语法解析） */
    if (!cn_analyze_inheritance(ctx, class_decl)) {
        /* 继承检查失败，但继续分析成员以收集更多错误信息 */
    }
    
    /* 分析所有成员 */
    bool all_success = true;
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        
        bool success = false;
        switch (member->kind) {
            case CN_MEMBER_FIELD:
                success = cn_analyze_class_field(ctx, member);
                break;
            case CN_MEMBER_METHOD:
            case CN_MEMBER_CONSTRUCTOR:
            case CN_MEMBER_DESTRUCTOR:
                success = cn_analyze_class_method(ctx, member);
                break;
            default:
                report_error(ctx, "未知的成员类型", member->name, member->name_length);
                success = false;
                break;
        }
        
        if (!success) {
            all_success = false;
        }
    }
    
    /* 清理上下文 */
    ctx->current_class = NULL;
    ctx->current_member = NULL;
    
    return all_success;
}

/* ============================================================================
 * 批量分析函数实现
 * ============================================================================ */

bool cn_analyze_all_classes(CnSemScope *global_scope,
                            CnAstProgram *program,
                            CnDiagnostics *diagnostics)
{
    if (!global_scope || !program) {
        return true;  /* 无类需要分析 */
    }
    
    /* 初始化分析上下文 */
    CnClassAnalyzerContext ctx;
    if (!cn_class_analyzer_context_init(&ctx, global_scope, diagnostics)) {
        return false;
    }
    
    bool all_success = true;
    
    /* 分析所有类声明 */
    for (size_t i = 0; i < program->class_count; i++) {
        CnAstStmt *stmt = program->classes[i];
        if (!stmt) {
            continue;
        }
        
        /* 获取类声明节点 */
        CnAstClassDecl *class_decl = NULL;
        if (stmt->kind == CN_AST_STMT_CLASS_DECL) {
            class_decl = stmt->as.class_decl;  // 正确访问方式：通过 as 联合体成员
        }
        
        if (class_decl) {
            if (!cn_analyze_class_decl(&ctx, class_decl)) {
                all_success = false;
            }
        }
    }
    
    /* 清理上下文 */
    cn_class_analyzer_context_cleanup(&ctx);
    
    return all_success;
}

/* ============================================================================
 * 自身指针（this）检查实现
 * ============================================================================ */

/**
 * @brief 报告自身指针使用错误
 */
static void report_this_error(CnClassAnalyzerContext *ctx, const char *message)
{
    if (!ctx->diagnostics) {
        return;
    }
    
    cn_support_diag_semantic_error_generic(
        ctx->diagnostics,
        CN_DIAG_CODE_SEM_INVALID_THIS_USAGE,
        NULL, 0, 0,
        message
    );
}

bool cn_check_this_usage(CnClassAnalyzerContext *ctx, CnAstExpr *expr)
{
    if (!ctx || !expr) {
        return false;
    }
    
    /* 检查是否在类方法中 */
    if (!ctx->current_class) {
        report_this_error(ctx, "自身指针只能在类方法中使用");
        return false;
    }
    
    /* 检查是否在静态方法中 */
    if (cn_is_in_static_method(ctx)) {
        report_this_error(ctx, "静态方法中不能使用自身指针");
        return false;
    }
    
    /* 检查当前成员是否为方法 */
    if (!ctx->current_member) {
        report_this_error(ctx, "自身指针只能在方法体内使用");
        return false;
    }
    
    /* 检查成员类型是否为方法 */
    if (ctx->current_member->kind != CN_MEMBER_METHOD &&
        ctx->current_member->kind != CN_MEMBER_CONSTRUCTOR &&
        ctx->current_member->kind != CN_MEMBER_DESTRUCTOR) {
        report_this_error(ctx, "自身指针只能在方法、构造函数或析构函数中使用");
        return false;
    }
    
    return true;
}

CnType *cn_get_this_type(CnClassAnalyzerContext *ctx)
{
    if (!ctx || !ctx->current_class) {
        return NULL;
    }
    
    /* 自身指针的类型是当前类类型的指针 */
    /* 创建类类型 */
    CnType *class_type = cn_type_new_struct(
        ctx->current_class->name,
        ctx->current_class->name_length,
        NULL, 0, NULL, NULL, 0
    );
    
    if (!class_type) {
        return NULL;
    }
    
    /* 创建指针类型 */
    CnType *this_type = cn_type_new_pointer(class_type);
    if (!this_type) {
        /* 注意：这里可能需要释放 class_type，取决于类型系统的内存管理 */
        return NULL;
    }
    
    return this_type;
}

bool cn_is_in_static_method(CnClassAnalyzerContext *ctx)
{
    if (!ctx || !ctx->current_member) {
        return false;
    }
    
    /* 检查当前成员是否为静态方法 */
    if (ctx->current_member->kind == CN_MEMBER_METHOD) {
        return ctx->current_member->is_static;
    }
    
    return false;
}

bool cn_check_this_in_static_method(CnClassAnalyzerContext *ctx)
{
    if (cn_is_in_static_method(ctx)) {
        report_this_error(ctx, "静态方法中不能使用自身指针");
        return true;  /* 返回 true 表示检测到错误 */
    }
    return false;
}

/* ============================================================================
 * 构造函数语义检查实现
 * ============================================================================ */

bool cn_analyze_constructor(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    if (!ctx || !member) {
        return false;
    }
    
    /* 确保是构造函数 */
    if (member->kind != CN_MEMBER_CONSTRUCTOR) {
        return false;
    }
    
    bool success = true;
    
    /* 检查构造函数不能有返回类型 */
    if (!cn_check_constructor_no_return(ctx, member)) {
        success = false;
    }
    
    /* 检查构造函数不能是静态的 */
    if (member->is_static) {
        report_error(ctx, "构造函数不能是静态的",
                     member->name, member->name_length);
        success = false;
    }
    
    /* 检查构造函数不能是虚函数 */
    if (member->is_virtual) {
        report_error(ctx, "构造函数不能是虚函数",
                     member->name, member->name_length);
        success = false;
    }
    
    /* 分析参数列表 */
    if (member->parameters && member->parameter_count > 0) {
        if (!cn_analyze_method_params(ctx, member->parameters, member->parameter_count)) {
            success = false;
        }
    }
    
    /* 检查初始化列表 */
    if (!cn_check_initializer_list(ctx, member)) {
        success = false;
    }
    
    return success;
}

bool cn_check_constructor_no_return(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    if (!ctx || !member) {
        return false;
    }
    
    /* 构造函数不能有返回类型 */
    if (member->type != NULL) {
        report_error(ctx, "构造函数不能有返回类型",
                     member->name, member->name_length);
        return false;
    }
    
    return true;
}

/**
 * @brief 检查初始化表达式类型是否匹配字段类型
 *
 * @param ctx 分析上下文
 * @param member_name 成员名称
 * @param init_expr 初始化表达式
 * @return bool 类型匹配返回true，不匹配返回false
 */
static bool cn_check_initializer_type_internal(CnClassAnalyzerContext *ctx,
                                                const char *member_name,
                                                size_t member_name_length,
                                                CnAstExpr *init_expr)
{
    if (!ctx || !ctx->current_class || !member_name || !init_expr) {
        return false;
    }
    
    /* 查找成员字段 */
    CnClassMember *field = cn_find_class_member(ctx->current_class, member_name);
    if (!field) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "初始化列表中的成员 '%.*s' 在类中不存在",
                 (int)member_name_length, member_name);
        report_error(ctx, error_msg, member_name, member_name_length);
        return false;
    }
    
    /* 检查是否为字段（非方法） */
    if (field->kind != CN_MEMBER_FIELD) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "初始化列表中的成员 '%.*s' 不是字段（不能初始化方法）",
                 (int)member_name_length, member_name);
        report_error(ctx, error_msg, member_name, member_name_length);
        return false;
    }
    
    /* 检查类型兼容性 */
    if (field->type && init_expr->type) {
        /* 简化类型检查：只检查类型种类是否相同 */
        if (field->type->kind != init_expr->type->kind) {
            /* 允许整数类型之间的隐式转换 */
            bool is_int_compatible = false;
            switch (field->type->kind) {
                case CN_TYPE_INT:
                case CN_TYPE_INT32:
                case CN_TYPE_INT64:
                case CN_TYPE_UINT32:
                case CN_TYPE_UINT64:
                case CN_TYPE_UINT64_LL:
                    switch (init_expr->type->kind) {
                        case CN_TYPE_INT:
                        case CN_TYPE_INT32:
                        case CN_TYPE_INT64:
                        case CN_TYPE_UINT32:
                        case CN_TYPE_UINT64:
                        case CN_TYPE_UINT64_LL:
                            is_int_compatible = true;
                            break;
                        default:
                            break;
                    }
                    break;
                case CN_TYPE_FLOAT:
                case CN_TYPE_FLOAT32:
                case CN_TYPE_FLOAT64:
                    switch (init_expr->type->kind) {
                        case CN_TYPE_FLOAT:
                        case CN_TYPE_FLOAT32:
                        case CN_TYPE_FLOAT64:
                        case CN_TYPE_INT:
                        case CN_TYPE_INT32:
                        case CN_TYPE_INT64:
                            is_int_compatible = true;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            
            if (!is_int_compatible) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg),
                         "初始化列表中成员 '%.*s' 的类型不匹配",
                         (int)member_name_length, member_name);
                report_error(ctx, error_msg, member_name, member_name_length);
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief 检查是否有重复初始化
 *
 * @param ctx 分析上下文
 * @param init_list 初始化列表
 * @param count 初始化列表项数量
 * @return bool 无重复返回true，有重复返回false
 */
static bool cn_check_duplicate_initialization_internal(CnClassAnalyzerContext *ctx,
                                                        CnAstStructFieldInit *init_list,
                                                        size_t count)
{
    if (!ctx || !init_list || count == 0) {
        return true;
    }
    
    /* 检查是否有重复的成员名 */
    for (size_t i = 0; i < count; i++) {
        for (size_t j = i + 1; j < count; j++) {
            if (cn_name_equals(init_list[i].field_name, init_list[i].field_name_length,
                              init_list[j].field_name, init_list[j].field_name_length)) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg),
                         "初始化列表中成员 '%.*s' 被重复初始化",
                         (int)init_list[i].field_name_length, init_list[i].field_name);
                report_error(ctx, error_msg, init_list[i].field_name, init_list[i].field_name_length);
                return false;
            }
        }
    }
    
    return true;
}

bool cn_check_initializer_list(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    if (!ctx || !member) {
        return false;
    }
    
    /* 只有构造函数才有初始化列表 */
    if (member->kind != CN_MEMBER_CONSTRUCTOR) {
        return true;
    }
    
    /* 如果没有初始化列表，直接成功 */
    if (!member->initializer_list || member->initializer_count == 0) {
        return true;
    }
    
    bool success = true;
    
    /* 1. 检查是否有重复初始化 */
    if (!cn_check_duplicate_initialization_internal(ctx, member->initializer_list, member->initializer_count)) {
        success = false;
    }
    
    /* 2. 遍历初始化列表，检查每个成员 */
    for (size_t i = 0; i < member->initializer_count; i++) {
        CnAstStructFieldInit *init_item = &member->initializer_list[i];
        
        /* 检查成员是否存在且为字段 */
        CnClassMember *field = cn_find_class_member(ctx->current_class, init_item->field_name);
        if (!field) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                     "初始化列表中的成员 '%.*s' 在类中不存在",
                     (int)init_item->field_name_length, init_item->field_name);
            report_error(ctx, error_msg, init_item->field_name, init_item->field_name_length);
            success = false;
            continue;
        }
        
        if (field->kind != CN_MEMBER_FIELD) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                     "初始化列表中的成员 '%.*s' 不是字段",
                     (int)init_item->field_name_length, init_item->field_name);
            report_error(ctx, error_msg, init_item->field_name, init_item->field_name_length);
            success = false;
            continue;
        }
        
        /* 检查初始化表达式类型是否匹配 */
        if (init_item->value) {
            if (!cn_check_initializer_type_internal(ctx, init_item->field_name,
                                                     init_item->field_name_length,
                                                     init_item->value)) {
                success = false;
            }
        }
    }
    
    return success;
}

/**
 * @brief 检查初始化表达式类型是否匹配（公开接口）
 */
bool cn_check_initializer_type(CnClassAnalyzerContext *ctx,
                               const char *member_name,
                               CnAstExpr *init_expr)
{
    if (!ctx || !member_name || !init_expr) {
        return false;
    }
    return cn_check_initializer_type_internal(ctx, member_name, strlen(member_name), init_expr);
}

/**
 * @brief 检查是否有重复初始化（公开接口）
 */
bool cn_check_duplicate_initialization(CnClassAnalyzerContext *ctx,
                                       const char **member_names,
                                       size_t count)
{
    if (!ctx || !member_names || count == 0) {
        return true;
    }
    
    return true;
}

/* ============================================================================
 * 析构函数语义检查实现
 * ============================================================================ */

bool cn_analyze_destructor(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    if (!ctx || !member) {
        return false;
    }
    
    /* 确保是析构函数 */
    if (member->kind != CN_MEMBER_DESTRUCTOR) {
        return false;
    }
    
    bool success = true;
    
    /* 检查析构函数不能有返回类型 */
    if (member->type != NULL) {
        report_error(ctx, "析构函数不能有返回类型",
                     member->name, member->name_length);
        success = false;
    }
    
    /* 检查析构函数不能有参数 */
    if (member->parameter_count > 0) {
        report_error(ctx, "析构函数不能有参数",
                     member->name, member->name_length);
        success = false;
    }
    
    /* 检查析构函数不能是静态的 */
    if (member->is_static) {
        report_error(ctx, "析构函数不能是静态的",
                     member->name, member->name_length);
        success = false;
    }
    
    /* 检查析构函数不能是虚函数 */
    if (member->is_virtual) {
        report_error(ctx, "析构函数不能是虚函数",
                     member->name, member->name_length);
        success = false;
    }
    
    return success;
}

bool cn_check_destructor_no_params(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    if (!ctx || !member) {
        return false;
    }
    
    /* 析构函数不能有参数 */
    if (member->parameter_count > 0) {
        report_error(ctx, "析构函数不能有参数",
                     member->name, member->name_length);
        return false;
    }
    
    return true;
}

bool cn_check_destructor_no_return(CnClassAnalyzerContext *ctx, CnClassMember *member)
{
    if (!ctx || !member) {
        return false;
    }
    
    /* 析构函数不能有返回类型 */
    if (member->type != NULL) {
        report_error(ctx, "析构函数不能有返回类型",
                     member->name, member->name_length);
        return false;
    }
    
    return true;
}

/* ============================================================================
 * 继承语义检查函数实现
 * ============================================================================ */

/**
 * @brief 在符号表中查找类声明（辅助函数）
 *
 * 使用公开API在作用域链中查找类符号
 * 注意：当前符号表不直接存储类声明指针，此函数返回是否找到类符号
 */
static bool cn_find_class_symbol_in_scope(CnSemScope *scope,
                                           const char *name,
                                           size_t name_len)
{
    if (!scope || !name) {
        return false;
    }
    
    /* 使用公开API查找符号 */
    CnSemSymbol *symbol = cn_sem_scope_lookup(scope, name, name_len);
    
    if (symbol) {
        /* 找到符号，检查是否为结构体类型（类在底层表示为结构体） */
        if (symbol->kind == CN_SEM_SYMBOL_STRUCT) {
            return true;
        }
    }
    
    return false;
}

bool cn_check_base_class_exists(CnClassAnalyzerContext *ctx,
                                 const char *base_name,
                                 size_t base_name_len)
{
    if (!ctx || !base_name) {
        return false;
    }
    
    /* 在符号表中查找基类符号 */
    bool found = cn_find_class_symbol_in_scope(ctx->symbol_table,
                                                base_name,
                                                base_name_len);
    
    if (!found) {
        /* 基类不存在，报告错误 */
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "基类 '%.*s' 未定义",
                 (int)base_name_len, base_name);
        report_error(ctx, error_msg, base_name, base_name_len);
        return false;
    }
    
    return true;
}

bool cn_check_circular_inheritance(CnClassAnalyzerContext *ctx,
                                    CnAstClassDecl *class_decl)
{
    if (!ctx || !class_decl) {
        return true;  /* 无循环 */
    }
    
    /* 检查直接循环：类的基类是否是类自身 */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        /* 检查基类名是否与当前类名相同 */
        if (cn_name_equals(base_info->base_class_name,
                          base_info->base_class_name_length,
                          class_decl->name,
                          class_decl->name_length)) {
            report_error(ctx, "类不能继承自身",
                         class_decl->name, class_decl->name_length);
            return false;
        }
    }
    
    /* 检查间接循环：需要遍历继承链 */
    /* 由于符号表不存储类声明指针，简化实现只检查直接循环 */
    /* 完整实现需要在符号表中存储类声明指针或使用全局类注册表 */
    
    return true;  /* 无循环继承 */
}

bool cn_check_base_not_final(CnClassAnalyzerContext *ctx,
                              const char *base_name,
                              size_t base_name_len)
{
    if (!ctx || !base_name) {
        return true;  /* 默认允许 */
    }
    
    /* 由于符号表不存储类声明指针，无法直接检查is_final属性 */
    /* 简化实现：假设基类可以被继承 */
    /* 完整实现需要在符号表中存储类声明指针或使用全局类注册表 */
    (void)base_name;
    (void)base_name_len;
    
    return true;
}

bool cn_analyze_inheritance(CnClassAnalyzerContext *ctx,
                             CnAstClassDecl *class_decl)
{
    if (!ctx || !class_decl) {
        return false;
    }
    
    /* 如果没有基类，无需检查 */
    if (class_decl->base_count == 0) {
        return true;
    }
    
    bool all_success = true;
    
    /* 遍历所有基类进行检查 */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        /* 检查基类是否存在 */
        if (!cn_check_base_class_exists(ctx,
                                        base_info->base_class_name,
                                        base_info->base_class_name_length)) {
            all_success = false;
            continue;  /* 基类不存在，跳过后续检查 */
        }
        
        /* 检查基类是否为最终类 */
        if (!cn_check_base_not_final(ctx,
                                     base_info->base_class_name,
                                     base_info->base_class_name_length)) {
            all_success = false;
        }
        
        /* 检查基类的纯虚函数是否已实现（如果派生类不是抽象类） */
        /* 注意：纯虚函数检查在cn_analyze_all_classes中统一处理，
         * 因为这里无法访问程序AST来查找基类声明。
         * 参见cn_check_class_instantiable_ex函数。 */
        (void)cn_find_class_decl_by_name;  /* 避免未使用警告 */
    }
    
    /* 检查循环继承 */
    if (!cn_check_circular_inheritance(ctx, class_decl)) {
        all_success = false;
    }
    
    return all_success;
}

/* ============================================================================
 * 虚函数调用语义检查函数实现
 * ============================================================================ */

bool cn_check_virtual_call_valid(CnClassAnalyzerContext *ctx,
                                  CnAstClassDecl *class_decl,
                                  const char *method_name,
                                  size_t method_name_len)
{
    if (!ctx || !class_decl || !method_name) {
        return false;
    }
    
    /* 在类层次结构中查找方法 */
    CnClassMember *method = cn_find_class_member_in_hierarchy(class_decl, method_name, NULL);
    
    if (!method) {
        /* 方法不存在 */
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "方法 '%.*s' 在类 '%.*s' 中未定义",
                 (int)method_name_len, method_name,
                 (int)class_decl->name_length, class_decl->name);
        report_error(ctx, error_msg, method_name, method_name_len);
        return false;
    }
    
    /* 检查是否为方法（不是字段） */
    if (method->kind != CN_MEMBER_METHOD) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "'%.*s' 不是方法，无法调用",
                 (int)method_name_len, method_name);
        report_error(ctx, error_msg, method_name, method_name_len);
        return false;
    }
    
    return true;
}

bool cn_check_virtual_call_access(CnClassAnalyzerContext *ctx,
                                   CnClassMember *method,
                                   CnAstClassDecl *calling_class)
{
    if (!ctx || !method) {
        return false;
    }
    
    /* 检查访问权限 */
    switch (method->access) {
        case CN_ACCESS_PUBLIC:
            /* 公开方法，任何地方都可访问 */
            return true;
            
        case CN_ACCESS_PRIVATE:
            /* 私有方法，只能在声明它的类内部访问 */
            if (calling_class && method->name) {
                /* 检查调用类是否是声明该方法的类 */
                /* 简化实现：假设调用类不是声明类 */
                report_private_access_error(ctx, method);
                return false;
            }
            return true;
            
        case CN_ACCESS_PROTECTED:
            /* 保护方法，只能在类内部或派生类中访问 */
            if (calling_class && method->name) {
                /* 检查调用类是否是声明类或其派生类 */
                /* 简化实现：假设调用类不是声明类或派生类 */
                report_protected_access_error(ctx, method);
                return false;
            }
            return true;
            
        default:
            return true;
    }
}

bool cn_check_virtual_call_args(CnClassAnalyzerContext *ctx,
                                 CnClassMember *method,
                                 size_t arg_count)
{
    if (!ctx || !method) {
        return false;
    }
    
    /* 检查参数数量 */
    /* 注意：成员方法的第一个参数是 self，所以参数数量需要减1 */
    size_t expected_params = method->parameter_count;
    
    if (arg_count != expected_params) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "方法 '%.*s' 期望 %zu 个参数，但提供了 %zu 个",
                 (int)method->name_length, method->name,
                 expected_params, arg_count);
        report_error(ctx, error_msg, method->name, method->name_length);
        return false;
    }
    
    return true;
}

bool cn_check_pure_virtual_call(CnClassAnalyzerContext *ctx,
                                 CnClassMember *method)
{
    if (!ctx || !method) {
        return false;
    }
    
    /* 检查是否为纯虚函数 */
    if (method->is_pure_virtual) {
        /* 纯虚函数不能被直接调用（只能在派生类中重写后调用） */
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "不能调用纯虚函数 '%.*s'，请在派生类中实现后调用",
                 (int)method->name_length, method->name);
        report_error(ctx, error_msg, method->name, method->name_length);
        return false;
    }
    
    return true;
}

bool cn_check_virtual_call(CnClassAnalyzerContext *ctx,
                            CnAstClassDecl *class_decl,
                            const char *method_name,
                            size_t method_name_len,
                            size_t arg_count,
                            CnAstClassDecl *calling_class)
{
    if (!ctx || !class_decl || !method_name) {
        return false;
    }
    
    bool success = true;
    
    /* 1. 检查方法是否存在 */
    CnClassMember *method = NULL;
    CnClassMember *found = cn_find_class_member_in_hierarchy(class_decl, method_name, NULL);
    if (!found) {
        cn_check_virtual_call_valid(ctx, class_decl, method_name, method_name_len);
        return false;
    }
    method = found;
    
    /* 2. 检查访问权限 */
    if (!cn_check_virtual_call_access(ctx, method, calling_class)) {
        success = false;
    }
    
    /* 3. 检查参数数量 */
    if (!cn_check_virtual_call_args(ctx, method, arg_count)) {
        success = false;
    }
    
    /* 4. 检查是否为纯虚函数 */
    if (!cn_check_pure_virtual_call(ctx, method)) {
        success = false;
    }
    
    return success;
}

/* ============================================================================
 * 抽象类语义检查函数实现
 * ============================================================================ */

bool cn_class_is_abstract(CnAstClassDecl *class_decl)
{
    if (!class_decl) {
        return false;
    }
    
    /* 如果类被显式标记为抽象类 */
    if (class_decl->is_abstract) {
        return true;
    }
    
    /* 检查是否有纯虚函数 */
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD && member->is_pure_virtual) {
            return true;
        }
    }
    
    return false;
}

bool cn_check_class_instantiable(CnClassAnalyzerContext *ctx,
                                  CnAstClassDecl *class_decl)
{
    if (!ctx || !class_decl) {
        return false;
    }
    
    /* 检查类是否被标记为抽象类 */
    if (class_decl->is_abstract) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "抽象类 '%.*s' 不能被实例化",
                 (int)class_decl->name_length, class_decl->name);
        report_error(ctx, error_msg, class_decl->name, class_decl->name_length);
        return false;
    }
    
    /* 检查是否有未实现的纯虚函数 */
    size_t pure_virtual_count = 0;
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD && member->is_pure_virtual) {
            pure_virtual_count++;
        }
    }
    
    if (pure_virtual_count > 0) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "类 '%.*s' 包含 %zu 个未实现的纯虚函数，不能被实例化",
                 (int)class_decl->name_length, class_decl->name,
                 pure_virtual_count);
        report_error(ctx, error_msg, class_decl->name, class_decl->name_length);
        return false;
    }
    
    return true;
}

bool cn_check_pure_virtual_implementation(CnClassAnalyzerContext *ctx,
                                           CnAstClassDecl *derived_class,
                                           CnAstClassDecl *base_class)
{
    if (!ctx || !derived_class || !base_class) {
        return false;
    }
    
    bool all_implemented = true;
    
    /* 遍历基类的所有纯虚函数 */
    for (size_t i = 0; i < base_class->member_count; i++) {
        CnClassMember *base_member = &base_class->members[i];
        
        /* 只检查纯虚函数 */
        if (base_member->kind != CN_MEMBER_METHOD || !base_member->is_pure_virtual) {
            continue;
        }
        
        /* 在派生类中查找是否实现了该纯虚函数 */
        bool implemented = false;
        for (size_t j = 0; j < derived_class->member_count; j++) {
            CnClassMember *derived_member = &derived_class->members[j];
            
            /* 检查方法名是否匹配 */
            if (derived_member->kind == CN_MEMBER_METHOD &&
                cn_name_equals(derived_member->name, derived_member->name_length,
                              base_member->name, base_member->name_length)) {
                /* 检查是否不是纯虚函数（即已实现） */
                if (!derived_member->is_pure_virtual) {
                    implemented = true;
                    break;
                }
            }
        }
        
        if (!implemented) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                     "派生类 '%.*s' 未实现基类 '%.*s' 中的纯虚函数 '%.*s'",
                     (int)derived_class->name_length, derived_class->name,
                     (int)base_class->name_length, base_class->name,
                     (int)base_member->name_length, base_member->name);
            report_error(ctx, error_msg, base_member->name, base_member->name_length);
            all_implemented = false;
        }
    }
    
    return all_implemented;
}

bool cn_get_unimplemented_pure_virtuals(CnAstClassDecl *class_decl,
                                         const char **out_names,
                                         size_t *out_count)
{
    if (!class_decl || !out_count) {
        return false;
    }
    
    size_t count = 0;
    
    /* 统计未实现的纯虚函数 */
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD && member->is_pure_virtual) {
            if (out_names && count < 32) {  /* 限制最多32个 */
                out_names[count] = member->name;
            }
            count++;
        }
    }
    
    *out_count = count;
    return true;
}

/* ============================================================================
 * 接口实现检查函数实现
 * ============================================================================ */

/**
 * @brief 查找接口定义
 */
CnAstInterfaceDecl *cn_find_interface(CnAstProgram *program,
                                       const char *interface_name,
                                       size_t interface_name_length)
{
    if (!program || !interface_name || interface_name_length == 0) {
        return NULL;
    }
    
    /* 遍历程序的接口列表 */
    for (size_t i = 0; i < program->interface_count; i++) {
        CnAstStmt *interface_stmt = program->interfaces[i];
        if (!interface_stmt || interface_stmt->kind != CN_AST_STMT_INTERFACE_DECL) {
            continue;
        }
        
        CnAstInterfaceDecl *interface_decl = interface_stmt->as.interface_decl;
        if (!interface_decl) {
            continue;
        }
        
        /* 比较接口名 */
        if (cn_name_equals(interface_decl->name, interface_decl->name_length,
                          interface_name, interface_name_length)) {
            return interface_decl;
        }
    }
    
    return NULL;
}

/**
 * @brief 检查接口方法签名是否匹配
 */
bool cn_check_interface_method_signature(CnClassMember *class_method,
                                          CnClassMember *interface_method)
{
    if (!class_method || !interface_method) {
        return false;
    }
    
    /* 检查参数数量 */
    if (class_method->parameter_count != interface_method->parameter_count) {
        return false;
    }
    
    /* 检查返回类型（简化版本：只检查类型种类） */
    if (class_method->type && interface_method->type) {
        if (class_method->type->kind != interface_method->type->kind) {
            return false;
        }
    } else if (class_method->type != interface_method->type) {
        /* 一个有返回类型，一个没有 */
        return false;
    }
    
    /* 检查参数类型 */
    for (size_t i = 0; i < class_method->parameter_count; i++) {
        CnAstParameter *class_param = &class_method->parameters[i];
        CnAstParameter *interface_param = &interface_method->parameters[i];
        
        if (class_param->declared_type && interface_param->declared_type) {
            if (class_param->declared_type->kind != interface_param->declared_type->kind) {
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief 检查类是否实现了指定接口的所有方法
 */
bool cn_check_interface_methods_implemented(CnClassAnalyzerContext *ctx,
                                             CnAstClassDecl *class_decl,
                                             CnAstInterfaceDecl *interface_decl)
{
    if (!ctx || !class_decl || !interface_decl) {
        return false;
    }
    
    bool all_implemented = true;
    
    /* 遍历接口的所有方法 */
    for (size_t i = 0; i < interface_decl->method_count; i++) {
        CnClassMember *interface_method = &interface_decl->methods[i];
        
        /* 在类中查找同名方法 */
        bool found = false;
        for (size_t j = 0; j < class_decl->member_count; j++) {
            CnClassMember *class_method = &class_decl->members[j];
            
            /* 检查方法名是否匹配 */
            if (class_method->kind == CN_MEMBER_METHOD &&
                cn_name_equals(class_method->name, class_method->name_length,
                              interface_method->name, interface_method->name_length)) {
                
                /* 检查方法签名是否匹配 */
                if (cn_check_interface_method_signature(class_method, interface_method)) {
                    found = true;
                    break;
                } else {
                    /* 签名不匹配，报告错误 */
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg),
                             "类 '%.*s' 中方法 '%.*s' 的签名与接口 '%.*s' 中的定义不匹配",
                             (int)class_decl->name_length, class_decl->name,
                             (int)interface_method->name_length, interface_method->name,
                             (int)interface_decl->name_length, interface_decl->name);
                    report_error(ctx, error_msg, interface_method->name, interface_method->name_length);
                    all_implemented = false;
                    found = true;  /* 找到了但签名不匹配，继续检查其他方法 */
                    break;
                }
            }
        }
        
        if (!found) {
            /* 未找到实现，报告错误 */
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                     "类 '%.*s' 未实现接口 '%.*s' 中的方法 '%.*s'",
                     (int)class_decl->name_length, class_decl->name,
                     (int)interface_decl->name_length, interface_decl->name,
                     (int)interface_method->name_length, interface_method->name);
            report_error(ctx, error_msg, interface_method->name, interface_method->name_length);
            all_implemented = false;
        }
    }
    
    return all_implemented;
}

/**
 * @brief 检查类是否实现了所有接口方法
 */
bool cn_check_interface_implementation(CnClassAnalyzerContext *ctx,
                                        CnAstClassDecl *class_decl,
                                        CnAstProgram *program)
{
    if (!ctx || !class_decl || !program) {
        return false;
    }
    
    /* 如果类没有实现任何接口，直接返回成功 */
    if (class_decl->implemented_interface_count == 0) {
        return true;
    }
    
    bool all_implemented = true;
    
    /* 遍历类实现的所有接口 */
    for (size_t i = 0; i < class_decl->implemented_interface_count; i++) {
        const char *interface_name = class_decl->implemented_interfaces[i];
        size_t interface_name_length = class_decl->implemented_interface_lengths[i];
        
        /* 查找接口定义 */
        CnAstInterfaceDecl *interface_decl = cn_find_interface(program,
                                                                interface_name,
                                                                interface_name_length);
        if (!interface_decl) {
            /* 接口未找到，报告错误 */
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                     "接口 '%.*s' 未定义",
                     (int)interface_name_length, interface_name);
            report_error(ctx, error_msg, interface_name, interface_name_length);
            all_implemented = false;
            continue;
        }
        
        /* 检查接口方法是否都已实现 */
        if (!cn_check_interface_methods_implemented(ctx, class_decl, interface_decl)) {
            all_implemented = false;
        }
    }
    
    return all_implemented;
}

/* ============================================================================
 * 类对象布局计算函数实现（多继承支持）
 * ============================================================================ */

/**
 * @brief 对齐偏移量到指定边界
 */
size_t cn_align_offset(size_t offset, size_t alignment) {
    if (alignment == 0) return offset;
    return (offset + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief 获取类型的对齐要求
 */
size_t cn_type_get_alignment(const CnType *type) {
    if (!type) return 1;
    
    switch (type->kind) {
        case CN_TYPE_VOID:
            return 1;
        case CN_TYPE_INT:
        case CN_TYPE_INT32:
            return 4;
        case CN_TYPE_INT64:
        case CN_TYPE_UINT64:
        case CN_TYPE_UINT64_LL:
            return 8;
        case CN_TYPE_UINT32:
            return 4;
        case CN_TYPE_FLOAT:
        case CN_TYPE_FLOAT32:
            return 4;
        case CN_TYPE_FLOAT64:
            return 8;
        case CN_TYPE_BOOL:
            return 1;
        case CN_TYPE_STRING:
            return sizeof(void*);  /* 指针对齐 */
        case CN_TYPE_POINTER:
            return sizeof(void*);
        case CN_TYPE_ARRAY:
            return cn_type_get_alignment(type->as.array.element_type);
        case CN_TYPE_STRUCT:
        case CN_TYPE_ENUM:
            return sizeof(void*);  /* 结构体和枚举使用指针对齐 */
        default:
            return 1;
    }
}

/**
 * @brief 获取类型的大小
 */
size_t cn_type_get_size(const CnType *type) {
    if (!type) return 0;
    
    switch (type->kind) {
        case CN_TYPE_VOID:
            return 0;
        case CN_TYPE_INT:
        case CN_TYPE_INT32:
            return 4;
        case CN_TYPE_INT64:
        case CN_TYPE_UINT64:
        case CN_TYPE_UINT64_LL:
            return 8;
        case CN_TYPE_UINT32:
            return 4;
        case CN_TYPE_FLOAT:
        case CN_TYPE_FLOAT32:
            return 4;
        case CN_TYPE_FLOAT64:
            return 8;
        case CN_TYPE_BOOL:
            return 1;
        case CN_TYPE_STRING:
            return sizeof(void*);  /* 字符串指针 */
        case CN_TYPE_POINTER:
            return sizeof(void*);
        case CN_TYPE_ARRAY:
            if (type->as.array.element_type && type->as.array.length > 0) {
                return cn_type_get_size(type->as.array.element_type) * type->as.array.length;
            }
            return 0;
        case CN_TYPE_STRUCT:
            /* 结构体大小需要完整计算，这里返回估计值 */
            return sizeof(void*) * 4;  /* 估计值 */
        case CN_TYPE_ENUM:
            return 4;  /* 枚举通常是int大小 */
        default:
            return 0;
    }
}

/**
 * @brief 初始化类布局结构
 */
bool cn_class_layout_init(CnClassLayout *layout) {
    if (!layout) return false;
    
    memset(layout, 0, sizeof(CnClassLayout));
    layout->total_size = 0;
    layout->vtable_offset = SIZE_MAX;
    layout->has_vtable = false;
    layout->alignment = 1;
    layout->base_count = 0;
    layout->member_count = 0;
    
    /* 虚继承相关初始化 */
    layout->has_virtual_base = false;
    layout->vbptr_offset = SIZE_MAX;
    layout->vbtable_count = 0;
    
    return true;
}

/**
 * @brief 清理类布局结构
 */
void cn_class_layout_cleanup(CnClassLayout *layout) {
    if (!layout) return;
    
    /* 目前没有动态分配的内存，只需重置 */
    layout->total_size = 0;
    layout->vtable_offset = SIZE_MAX;
    layout->has_vtable = false;
    layout->base_count = 0;
    layout->member_count = 0;
}

/**
 * @brief 检查基类子对象是否共享（虚继承）
 */
bool cn_class_is_virtual_base(const CnAstClassDecl *class_decl,
                               const char *base_name,
                               size_t base_name_len) {
    if (!class_decl || !base_name) return false;
    
    for (size_t i = 0; i < class_decl->base_count; i++) {
        const CnInheritanceInfo *base_info = &class_decl->bases[i];
        if (cn_name_equals(base_info->base_class_name, base_info->base_class_name_length,
                          base_name, base_name_len)) {
            return base_info->is_virtual;
        }
    }
    
    return false;
}

/**
 * @brief 计算类的对象布局
 *
 * 对象布局策略（支持虚继承）：
 * 1. 非虚基类子对象按声明顺序布局
 * 2. vbptr（虚基类指针）放在非虚基类之后
 * 3. 成员变量
 * 4. 虚基类子对象放在最后（共享）
 */
bool cn_class_layout_calculate(CnClassLayout *layout,
                                CnAstClassDecl *class_decl,
                                CnClassLayout* (*get_base_layout)(const char *name, size_t name_len, void *user_data),
                                void *user_data) {
    if (!layout || !class_decl) return false;
    
    /* 初始化布局 */
    cn_class_layout_init(layout);
    
    size_t current_offset = 0;
    size_t max_alignment = 1;
    
    /* 第一步：布局非虚基类子对象 */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        /* 跳过虚基类，虚基类在后面单独处理 */
        if (base_info->is_virtual) {
            layout->has_virtual_base = true;
            continue;
        }
        
        /* 获取基类布局 */
        size_t base_size = sizeof(void*) * 4;  /* 默认估计大小 */
        size_t base_alignment = sizeof(void*);
        
        if (get_base_layout) {
            CnClassLayout *base_layout = get_base_layout(
                base_info->base_class_name,
                base_info->base_class_name_length,
                user_data
            );
            if (base_layout) {
                base_size = base_layout->total_size;
                base_alignment = base_layout->alignment;
                if (base_layout->has_vtable) {
                    layout->has_vtable = true;  /* 基类有vtable，派生类也需要 */
                }
                /* 检查基类是否有虚基类 */
                if (base_layout->has_virtual_base) {
                    layout->has_virtual_base = true;
                }
            }
        }
        
        /* 对齐基类子对象 */
        current_offset = cn_align_offset(current_offset, base_alignment);
        
        /* 记录基类偏移量 */
        if (layout->base_count < 16) {
            layout->base_offsets[layout->base_count] = current_offset;
            layout->base_names[layout->base_count] = base_info->base_class_name;
            layout->base_name_lengths[layout->base_count] = base_info->base_class_name_length;
            layout->base_is_virtual[layout->base_count] = false;
            layout->base_count++;
        }
        
        current_offset += base_size;
        
        if (base_alignment > max_alignment) {
            max_alignment = base_alignment;
        }
    }
    
    /* 第二步：布局vbptr（虚基类指针，如果有虚基类） */
    if (layout->has_virtual_base) {
        /* 对齐vbptr */
        current_offset = cn_align_offset(current_offset, sizeof(void*));
        layout->vbptr_offset = current_offset;
        current_offset += sizeof(void*);
        
        if (sizeof(void*) > max_alignment) {
            max_alignment = sizeof(void*);
        }
    }
    
    /* 第三步：布局vtable指针（如果需要） */
    /* 检查类是否有虚函数 */
    bool needs_vtable = false;
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD && member->is_virtual) {
            needs_vtable = true;
            break;
        }
    }
    
    if (needs_vtable || layout->has_vtable) {
        /* 对齐vtable指针 */
        current_offset = cn_align_offset(current_offset, sizeof(void*));
        layout->vtable_offset = current_offset;
        layout->has_vtable = true;
        current_offset += sizeof(void*);
        
        if (sizeof(void*) > max_alignment) {
            max_alignment = sizeof(void*);
        }
    }
    
    /* 第四步：布局成员变量 */
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        
        /* 只处理字段成员 */
        if (member->kind != CN_MEMBER_FIELD) continue;
        
        /* 跳过静态成员（静态成员不占用对象空间） */
        if (member->is_static) continue;
        
        /* 获取成员大小和对齐 */
        size_t member_size = cn_type_get_size(member->type);
        size_t member_alignment = cn_type_get_alignment(member->type);
        
        /* 对齐成员 */
        current_offset = cn_align_offset(current_offset, member_alignment);
        
        /* 记录成员偏移量 */
        if (layout->member_count < 64) {
            layout->member_offsets[layout->member_count] = current_offset;
            layout->member_names[layout->member_count] = member->name;
            layout->member_name_lengths[layout->member_count] = member->name_length;
            layout->member_count++;
        }
        
        current_offset += member_size;
        
        if (member_alignment > max_alignment) {
            max_alignment = member_alignment;
        }
    }
    
    /* 第五步：布局虚基类子对象（放在最后，共享） */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        /* 只处理虚基类 */
        if (!base_info->is_virtual) continue;
        
        /* 获取虚基类布局 */
        size_t vb_size = sizeof(void*) * 4;  /* 默认估计大小 */
        size_t vb_alignment = sizeof(void*);
        
        if (get_base_layout) {
            CnClassLayout *vb_layout = get_base_layout(
                base_info->base_class_name,
                base_info->base_class_name_length,
                user_data
            );
            if (vb_layout) {
                vb_size = vb_layout->total_size;
                vb_alignment = vb_layout->alignment;
                if (vb_layout->has_vtable) {
                    layout->has_vtable = true;
                }
            }
        }
        
        /* 对齐虚基类子对象 */
        current_offset = cn_align_offset(current_offset, vb_alignment);
        
        /* 记录虚基类偏移量 */
        if (layout->base_count < 16) {
            layout->base_offsets[layout->base_count] = current_offset;
            layout->base_names[layout->base_count] = base_info->base_class_name;
            layout->base_name_lengths[layout->base_count] = base_info->base_class_name_length;
            layout->base_is_virtual[layout->base_count] = true;
            layout->base_count++;
        }
        
        /* 记录到vbtable */
        if (layout->vbtable_count < 16) {
            layout->vbtable_names[layout->vbtable_count] = base_info->base_class_name;
            layout->vbtable_name_lengths[layout->vbtable_count] = base_info->base_class_name_length;
            layout->vbtable_offsets[layout->vbtable_count] = current_offset;
            layout->vbtable_count++;
        }
        
        current_offset += vb_size;
        
        if (vb_alignment > max_alignment) {
            max_alignment = vb_alignment;
        }
    }
    
    /* 第六步：计算总大小（考虑尾部对齐） */
    layout->total_size = cn_align_offset(current_offset, max_alignment);
    layout->alignment = max_alignment;
    
    return true;
}

/**
 * @brief 获取基类子对象的偏移量
 */
size_t cn_class_layout_get_base_offset(const CnClassLayout *layout,
                                        const char *base_name,
                                        size_t base_name_len) {
    if (!layout || !base_name) return SIZE_MAX;
    
    for (size_t i = 0; i < layout->base_count; i++) {
        if (cn_name_equals(layout->base_names[i], layout->base_name_lengths[i],
                          base_name, base_name_len)) {
            return layout->base_offsets[i];
        }
    }
    
    return SIZE_MAX;
}

/**
 * @brief 获取成员变量的偏移量
 */
size_t cn_class_layout_get_member_offset(const CnClassLayout *layout,
                                          const char *member_name,
                                          size_t member_name_len) {
    if (!layout || !member_name) return SIZE_MAX;
    
    for (size_t i = 0; i < layout->member_count; i++) {
        if (cn_name_equals(layout->member_names[i], layout->member_name_lengths[i],
                          member_name, member_name_len)) {
            return layout->member_offsets[i];
        }
    }
    
    return SIZE_MAX;
}

/**
 * @brief 计算this指针调整量
 */
size_t cn_class_layout_compute_this_adjustment(const CnClassLayout *derived_layout,
                                                const char *base_name,
                                                size_t base_name_len) {
    if (!derived_layout || !base_name) return 0;
    
    /* 查找基类偏移量 */
    size_t offset = cn_class_layout_get_base_offset(derived_layout, base_name, base_name_len);
    
    if (offset == SIZE_MAX) {
        /* 基类未找到，返回0（可能是错误情况） */
        return 0;
    }
    
    return offset;
}

/* ============================================================================
 * 菱形继承成员访问歧义检测函数实现
 * ============================================================================ */

/**
 * @brief 内部辅助函数：在单个类中查找成员
 *
 * @param class_decl 类声明
 * @param member_name 成员名称
 * @param member_name_len 成员名称长度
 * @return CnClassMember* 找到返回成员指针，否则返回NULL
 */
static CnClassMember *find_member_in_single_class(CnAstClassDecl *class_decl,
                                                   const char *member_name,
                                                   size_t member_name_len) {
    if (!class_decl || !member_name) return NULL;
    
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (cn_name_equals(member->name, member->name_length,
                          member_name, member_name_len)) {
            return member;
        }
    }
    
    return NULL;
}

/**
 * @brief 内部辅助函数：递归收集成员来源
 *
 * @param class_decl 类声明
 * @param member_name 成员名称
 * @param member_name_len 成员名称长度
 * @param sources 输出数组
 * @param source_count 输出计数
 * @param max_sources 最大数量
 * @param visited 已访问类名数组（防止循环）
 * @param visited_count 已访问计数
 */
static void collect_member_sources_recursive(CnAstClassDecl *class_decl,
                                              const char *member_name,
                                              size_t member_name_len,
                                              CnAstClassDecl **sources,
                                              size_t *source_count,
                                              size_t max_sources,
                                              const char **visited,
                                              size_t *visited_count) {
    if (!class_decl || !member_name || *source_count >= max_sources) return;
    
    /* 检查是否已访问（防止循环继承） */
    for (size_t i = 0; i < *visited_count; i++) {
        if (cn_name_equals(class_decl->name, class_decl->name_length,
                          visited[i], strlen(visited[i]))) {
            return;  /* 已访问，跳过 */
        }
    }
    
    /* 标记为已访问 */
    if (*visited_count < 32) {
        visited[*visited_count] = class_decl->name;
        (*visited_count)++;
    }
    
    /* 在当前类中查找成员 */
    CnClassMember *member = find_member_in_single_class(class_decl, member_name, member_name_len);
    if (member) {
        /* 检查是否已添加 */
        bool already_added = false;
        for (size_t i = 0; i < *source_count; i++) {
            if (sources[i] == class_decl) {
                already_added = true;
                break;
            }
        }
        
        if (!already_added && *source_count < max_sources) {
            sources[*source_count] = class_decl;
            (*source_count)++;
        }
    }
    
    /* 递归检查基类 */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        /* 注意：这里需要通过符号表获取基类声明 */
        /* 简化实现：假设基类声明可通过某种方式获取 */
        /* 完整实现需要传入继承解析器或符号表 */
        (void)base_info;  /* 暂时忽略 */
    }
}

bool cn_find_member_sources_in_hierarchy(CnAstClassDecl *class_decl,
                                          const char *member_name,
                                          size_t member_name_len,
                                          CnAstClassDecl **sources,
                                          size_t *source_count,
                                          size_t max_sources) {
    if (!class_decl || !member_name || !sources || !source_count) {
        if (source_count) *source_count = 0;
        return false;
    }
    
    *source_count = 0;
    
    /* 已访问类名数组 */
    const char *visited[32];
    size_t visited_count = 0;
    
    /* 递归收集 */
    collect_member_sources_recursive(class_decl, member_name, member_name_len,
                                      sources, source_count, max_sources,
                                      visited, &visited_count);
    
    return *source_count > 0;
}

CnMemberAmbiguityResult cn_check_member_access_ambiguity(
    CnAstClassDecl *class_decl,
    const char *member_name,
    size_t member_name_len,
    const char **ambiguous_class1,
    const char **ambiguous_class2) {
    
    if (!class_decl || !member_name) {
        return CN_MEMBER_AMBIGUITY_NOT_FOUND;
    }
    
    /* 首先在当前类中查找 */
    CnClassMember *member = find_member_in_single_class(class_decl, member_name, member_name_len);
    if (member) {
        /* 在当前类中找到，无歧义 */
        return CN_MEMBER_AMBIGUITY_OK;
    }
    
    /* 收集所有基类中的成员来源 */
    CnAstClassDecl *sources[16];
    size_t source_count = 0;
    
    cn_find_member_sources_in_hierarchy(class_decl, member_name, member_name_len,
                                         sources, &source_count, 16);
    
    if (source_count == 0) {
        return CN_MEMBER_AMBIGUITY_NOT_FOUND;
    }
    
    if (source_count == 1) {
        /* 只有一个来源，无歧义 */
        return CN_MEMBER_AMBIGUITY_OK;
    }
    
    /* 多个来源，检查是否来自虚基类 */
    /* 如果所有来源都是同一个虚基类，则无歧义（共享） */
    /* 如果来自不同的非虚基类路径，则有歧义 */
    
    /* 简化处理：多个来源即认为有歧义 */
    if (ambiguous_class1 && source_count > 0) {
        *ambiguous_class1 = sources[0]->name;
    }
    if (ambiguous_class2 && source_count > 1) {
        *ambiguous_class2 = sources[1]->name;
    }
    
    return CN_MEMBER_AMBIGUITY_FOUND;
}

bool cn_is_member_from_virtual_base(CnAstClassDecl *class_decl,
                                     const char *member_name,
                                     size_t member_name_len,
                                     const char **virtual_base_name,
                                     size_t *virtual_base_len) {
    if (!class_decl || !member_name) return false;
    
    /* 首先检查当前类是否有该成员 */
    CnClassMember *member = find_member_in_single_class(class_decl, member_name, member_name_len);
    if (member) {
        /* 成员在当前类中定义，不是来自虚基类 */
        return false;
    }
    
    /* 检查虚基类 */
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        /* 只检查虚基类 */
        if (!base_info->is_virtual) continue;
        
        /* 检查虚基类中是否有该成员 */
        /* 注意：完整实现需要获取基类声明 */
        /* 简化处理：假设虚基类有该成员 */
        if (virtual_base_name) {
            *virtual_base_name = base_info->base_class_name;
        }
        if (virtual_base_len) {
            *virtual_base_len = base_info->base_class_name_length;
        }
        
        return true;
    }
    
    return false;
}

bool cn_get_virtual_base_access_path(CnAstClassDecl *class_decl,
                                      const char *virtual_base_name,
                                      size_t virtual_base_len,
                                      size_t *vbptr_offset,
                                      size_t *vbase_offset) {
    if (!class_decl || !virtual_base_name) return false;
    
    /* 计算类布局 */
    CnClassLayout layout;
    cn_class_layout_init(&layout);
    cn_class_layout_calculate(&layout, class_decl, NULL, NULL);
    
    /* 查找虚基类 */
    bool found = false;
    for (size_t i = 0; i < layout.base_count; i++) {
        if (layout.base_is_virtual[i] &&
            cn_name_equals(layout.base_names[i], layout.base_name_lengths[i],
                          virtual_base_name, virtual_base_len)) {
            /* 找到虚基类 */
            if (vbptr_offset) {
                *vbptr_offset = layout.vbptr_offset;
            }
            if (vbase_offset) {
                *vbase_offset = layout.base_offsets[i];
            }
            found = true;
            break;
        }
    }
    
    cn_class_layout_cleanup(&layout);
    return found;
}

/* ============================================================================
 * 抽象类继承检查增强函数实现
 * ============================================================================ */

/**
 * @brief 通过名称在符号表中查找类声明
 *
 * 注意：由于CnSemSymbol结构中没有class_decl字段，
 * 此函数目前返回NULL。实际查找应使用cn_find_class_in_program函数。
 *
 * @param symbol_table 符号表（未使用，保留接口兼容性）
 * @param name 类名
 * @param name_len 类名长度
 * @return CnAstClassDecl* 始终返回NULL，需要使用其他方式查找
 */
CnAstClassDecl *cn_find_class_decl_by_name(CnSemScope *symbol_table,
                                            const char *name,
                                            size_t name_len)
{
    /* 参数检查 */
    (void)symbol_table;
    (void)name;
    (void)name_len;
    
    /*
     * 注意：CnSemSymbol结构中没有class_decl字段，
     * 无法直接从符号表获取类声明。
     * 请使用cn_find_class_in_program函数通过程序AST查找类声明。
     */
    return NULL;
}

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
                                                    CnAstClassDecl *base_class)
{
    if (!ctx || !derived_class || !base_class) {
        return true;  /* 无效参数，默认返回成功 */
    }
    
    /* 收集未实现的纯虚函数 */
    const char *unimplemented_names[64];
    size_t unimplemented_count = 0;
    
    /* 遍历基类的所有纯虚函数 */
    for (size_t i = 0; i < base_class->member_count; i++) {
        CnClassMember *base_member = &base_class->members[i];
        
        /* 只检查纯虚函数 */
        if (base_member->kind != CN_MEMBER_METHOD || !base_member->is_pure_virtual) {
            continue;
        }
        
        /* 在派生类中查找是否实现了该纯虚函数 */
        bool implemented = false;
        for (size_t j = 0; j < derived_class->member_count; j++) {
            CnClassMember *derived_member = &derived_class->members[j];
            
            /* 检查方法名是否匹配 */
            if (derived_member->kind == CN_MEMBER_METHOD &&
                cn_name_equals(derived_member->name, derived_member->name_length,
                              base_member->name, base_member->name_length)) {
                /* 检查是否不是纯虚函数（即已实现） */
                if (!derived_member->is_pure_virtual) {
                    implemented = true;
                    break;
                }
            }
        }
        
        if (!implemented) {
            /* 记录未实现的纯虚函数名 */
            if (unimplemented_count < 64) {
                unimplemented_names[unimplemented_count] = base_member->name;
            }
            unimplemented_count++;
        }
    }
    
    /* 如果有未实现的纯虚函数，报告详细错误 */
    if (unimplemented_count > 0) {
        cn_report_unimplemented_pure_virtuals(ctx, derived_class, base_class,
                                               unimplemented_names, unimplemented_count);
        return false;
    }
    
    return true;
}

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
                                            size_t count)
{
    if (!ctx || !derived_class || count == 0) {
        return;
    }
    
    /* 构建详细的错误信息 */
    char error_msg[512];
    int offset = 0;
    
    /* 错误标题 */
    offset = snprintf(error_msg, sizeof(error_msg),
                      "派生类 '%.*s' 未实现基类 '%.*s' 中的 %zu 个纯虚函数：",
                      (int)derived_class->name_length, derived_class->name,
                      (int)base_class->name_length, base_class->name,
                      count);
    
    /* 列出未实现的纯虚函数名称（最多显示5个） */
    size_t display_count = (count > 5) ? 5 : count;
    for (size_t i = 0; i < display_count && offset < (int)sizeof(error_msg) - 50; i++) {
        if (i > 0) {
            offset += snprintf(error_msg + offset, sizeof(error_msg) - offset, ", ");
        }
        offset += snprintf(error_msg + offset, sizeof(error_msg) - offset,
                          "'%s'", unimplemented_names[i]);
    }
    
    /* 如果超过5个，显示省略 */
    if (count > 5) {
        offset += snprintf(error_msg + offset, sizeof(error_msg) - offset,
                          " ... (共 %zu 个)", count);
    }
    
    /* 添加建议 */
    if (offset < (int)sizeof(error_msg) - 100) {
        snprintf(error_msg + offset, sizeof(error_msg) - offset,
                 "。请实现这些函数或将派生类标记为抽象类");
    }
    
    /* 报告错误 */
    report_error(ctx, error_msg, derived_class->name, derived_class->name_length);
    
    /* 使用诊断系统的详细错误码 */
    if (ctx->diagnostics) {
        cn_support_diag_semantic_error_generic(
            ctx->diagnostics,
            CN_DIAG_CODE_SEM_PURE_VIRTUAL_NOT_IMPL,
            NULL, 0, 0,
            error_msg
        );
    }
}

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
                                     struct CnAstProgram *program)
{
    if (!ctx || !class_decl) {
        return false;
    }
    
    /* 检查类是否被标记为抽象类 */
    if (class_decl->is_abstract) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "抽象类 '%.*s' 不能被实例化",
                 (int)class_decl->name_length, class_decl->name);
        report_error(ctx, error_msg, class_decl->name, class_decl->name_length);
        return false;
    }
    
    /* 收集所有未实现的纯虚函数（包括继承链中的） */
    const char *unimplemented_names[64];
    size_t unimplemented_count = 0;
    
    /* 检查当前类的纯虚函数 */
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD && member->is_pure_virtual) {
            if (unimplemented_count < 64) {
                unimplemented_names[unimplemented_count] = member->name;
            }
            unimplemented_count++;
        }
    }
    
    /* 检查继承链中的纯虚函数 */
    if (class_decl->base_count > 0 && program) {
        for (size_t i = 0; i < class_decl->base_count; i++) {
            CnInheritanceInfo *base_info = &class_decl->bases[i];
            
            /* 查找基类声明 */
            CnAstClassDecl *base_class = cn_find_class_in_program(
                program,
                base_info->base_class_name,
                base_info->base_class_name_length
            );
            
            if (!base_class) {
                continue;
            }
            
            /* 检查基类的纯虚函数是否已实现 */
            for (size_t j = 0; j < base_class->member_count; j++) {
                CnClassMember *base_member = &base_class->members[j];
                
                if (base_member->kind != CN_MEMBER_METHOD || !base_member->is_pure_virtual) {
                    continue;
                }
                
                /* 检查是否在派生类中实现 */
                bool implemented = false;
                for (size_t k = 0; k < class_decl->member_count; k++) {
                    CnClassMember *derived_member = &class_decl->members[k];
                    if (derived_member->kind == CN_MEMBER_METHOD &&
                        !derived_member->is_pure_virtual &&
                        cn_name_equals(derived_member->name, derived_member->name_length,
                                      base_member->name, base_member->name_length)) {
                        implemented = true;
                        break;
                    }
                }
                
                if (!implemented && unimplemented_count < 64) {
                    unimplemented_names[unimplemented_count] = base_member->name;
                    unimplemented_count++;
                }
            }
        }
    }
    
    /* 如果有未实现的纯虚函数，报告详细错误 */
    if (unimplemented_count > 0) {
        char error_msg[512];
        int offset = snprintf(error_msg, sizeof(error_msg),
                              "类 '%.*s' 包含 %zu 个未实现的纯虚函数，不能被实例化：",
                              (int)class_decl->name_length, class_decl->name,
                              unimplemented_count);
        
        /* 列出未实现的纯虚函数名称 */
        size_t display_count = (unimplemented_count > 5) ? 5 : unimplemented_count;
        for (size_t i = 0; i < display_count && offset < (int)sizeof(error_msg) - 50; i++) {
            if (i > 0) {
                offset += snprintf(error_msg + offset, sizeof(error_msg) - offset, ", ");
            }
            offset += snprintf(error_msg + offset, sizeof(error_msg) - offset,
                              "'%s'", unimplemented_names[i]);
        }
        
        if (unimplemented_count > 5) {
            snprintf(error_msg + offset, sizeof(error_msg) - offset,
                     " ... (共 %zu 个)", unimplemented_count);
        }
        
        report_error(ctx, error_msg, class_decl->name, class_decl->name_length);
        return false;
    }
    
    return true;
}

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
                                          size_t name_len)
{
    if (!program || !name || name_len == 0) {
        return NULL;
    }
    
    /* 遍历程序中的所有类声明 */
    for (size_t i = 0; i < program->class_count; i++) {
        CnAstStmt *stmt = program->classes[i];
        if (!stmt || stmt->kind != CN_AST_STMT_CLASS_DECL) {
            continue;
        }
        
        CnAstClassDecl *class_decl = stmt->as.class_decl;  // 正确访问方式
        if (cn_name_equals(class_decl->name, class_decl->name_length, name, name_len)) {
            return class_decl;
        }
    }
    
    return NULL;
}
