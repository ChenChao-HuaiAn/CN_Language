/**
 * @file class_node.c
 * @brief CN语言面向对象编程 - 类AST节点实现
 */

#include "cnlang/frontend/ast/class_node.h"
#include "cnlang/frontend/semantics.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

/**
 * @brief 跨平台字符串复制函数（替代strndup）
 *
 * @param src 源字符串
 * @param length 要复制的长度
 * @return char* 新分配的字符串，需要调用者释放
 */
static char *cn_strndup(const char *src, size_t length)
{
    if (!src || length == 0) {
        return NULL;
    }
    
    /* 分配内存：长度 + 空终止符 */
    char *dest = (char *)malloc(length + 1);
    if (!dest) {
        return NULL;
    }
    
    /* 复制字符串内容 */
    memcpy(dest, src, length);
    dest[length] = '\0';
    
    return dest;
}

/* ============================================================================
 * 创建函数实现
 * ============================================================================ */

/**
 * @brief 创建类声明节点
 */
CnAstClassDecl *cn_ast_class_decl_create(const char *name, size_t name_length)
{
    if (!name || name_length == 0) {
        return NULL;
    }
    
    CnAstClassDecl *class_decl = (CnAstClassDecl *)calloc(1, sizeof(CnAstClassDecl));
    if (!class_decl) {
        return NULL;
    }
    
    /* 设置基础节点信息 */
    class_decl->base.kind = CN_AST_STMT_STRUCT_DECL; /* 暂时使用结构体声明类型，后续会添加专门的类型 */
    
    /* 复制类名 */
    class_decl->name = cn_strndup(name, name_length);
    if (!class_decl->name) {
        free(class_decl);
        return NULL;
    }
    class_decl->name_length = name_length;
    
    /* 初始化数组指针为NULL */
    class_decl->bases = NULL;
    class_decl->base_count = 0;
    class_decl->members = NULL;
    class_decl->member_count = 0;
    
    /* 初始化实现的接口列表 */
    class_decl->implemented_interfaces = NULL;
    class_decl->implemented_interface_count = 0;
    
    /* 初始化属性标志 */
    class_decl->is_abstract = false;
    class_decl->is_interface = false;
    class_decl->is_final = false;
    
    return class_decl;
}

/**
 * @brief 创建接口声明节点
 */
CnAstInterfaceDecl *cn_ast_interface_decl_create(const char *name, size_t name_length)
{
    if (!name || name_length == 0) {
        return NULL;
    }
    
    CnAstInterfaceDecl *interface_decl = (CnAstInterfaceDecl *)calloc(1, sizeof(CnAstInterfaceDecl));
    if (!interface_decl) {
        return NULL;
    }
    
    /* 设置基础节点信息 */
    interface_decl->base.kind = CN_AST_STMT_STRUCT_DECL; /* 暂时使用结构体声明类型 */
    
    /* 复制接口名 */
    interface_decl->name = cn_strndup(name, name_length);
    if (!interface_decl->name) {
        free(interface_decl);
        return NULL;
    }
    interface_decl->name_length = name_length;
    
    /* 初始化数组指针为NULL */
    interface_decl->methods = NULL;
    interface_decl->method_count = 0;
    interface_decl->base_interfaces = NULL;
    interface_decl->base_interface_count = 0;
    
    /* 初始化模板参数 */
    interface_decl->template_params = NULL;
    
    return interface_decl;
}

/**
 * @brief 创建接口实例化节点
 */
CnAstInterfaceInstantiation *cn_ast_interface_instantiation_create(
    const char *interface_name, size_t interface_name_length,
    int line, int column)
{
    if (!interface_name || interface_name_length == 0) {
        return NULL;
    }
    
    CnAstInterfaceInstantiation *inst = (CnAstInterfaceInstantiation *)calloc(1, sizeof(CnAstInterfaceInstantiation));
    if (!inst) {
        return NULL;
    }
    
    /* 复制接口名 */
    inst->interface_name = cn_strndup(interface_name, interface_name_length);
    if (!inst->interface_name) {
        free(inst);
        return NULL;
    }
    inst->interface_name_length = interface_name_length;
    
    /* 初始化类型实参 */
    inst->type_args = NULL;
    inst->type_arg_count = 0;
    
    /* 设置源码位置 */
    inst->line = line;
    inst->column = column;
    
    return inst;
}

/**
 * @brief 向接口实例化添加类型实参
 */
int cn_ast_interface_instantiation_add_type_arg(
    CnAstInterfaceInstantiation *instantiation, CnType *type_arg)
{
    if (!instantiation || !type_arg) {
        return -1;
    }
    
    /* 扩展类型实参数组 */
    size_t new_count = instantiation->type_arg_count + 1;
    CnType **new_args = (CnType **)realloc(
        instantiation->type_args,
        new_count * sizeof(CnType *)
    );
    if (!new_args) {
        return -1;
    }
    instantiation->type_args = new_args;
    
    /* 添加类型实参 */
    instantiation->type_args[instantiation->type_arg_count] = type_arg;
    instantiation->type_arg_count = new_count;
    
    return 0;
}

/**
 * @brief 创建类成员
 */
CnClassMember *cn_ast_class_member_create(const char *name, size_t name_length,
                                          CnMemberKind kind, CnAccessLevel access)
{
    if (!name || name_length == 0) {
        return NULL;
    }
    
    CnClassMember *member = (CnClassMember *)calloc(1, sizeof(CnClassMember));
    if (!member) {
        return NULL;
    }
    
    /* 复制成员名称 */
    member->name = cn_strndup(name, name_length);
    if (!member->name) {
        free(member);
        return NULL;
    }
    member->name_length = name_length;
    
    /* 设置成员属性 */
    member->kind = kind;
    member->access = access;
    member->type = NULL;
    
    /* 初始化所有标志为false */
    member->is_static = false;
    member->is_virtual = false;
    member->is_override = false;
    member->is_pure_virtual = false;
    member->is_const = false;
    
    /* 初始化关联的AST节点 */
    member->init_expr = NULL;
    member->body = NULL;
    member->parameters = NULL;
    member->parameter_count = 0;
    
    return member;
}

/* ============================================================================
 * 销毁函数实现
 * ============================================================================ */

/**
 * @brief 清理类成员内容（不释放结构体本身）
 *
 * 用于释放类成员内部的动态分配资源，但不释放成员结构体本身。
 * 适用于数组元素的清理，因为数组元素是连续分配的，不能单独释放。
 *
 * @param member 要清理的类成员
 */
static void cn_ast_class_member_cleanup(CnClassMember *member)
{
    if (!member) {
        return;
    }
    
    /* 释放成员名称 */
    if (member->name) {
        free((void *)member->name);
        member->name = NULL;
    }
    
    /* 注意：type、init_expr、body、parameters 由其他模块管理，这里不释放 */
    
    /* 释放参数数组 */
    if (member->parameters) {
        for (size_t i = 0; i < member->parameter_count; i++) {
            if (member->parameters[i].name) {
                free((void *)member->parameters[i].name);
            }
        }
        free(member->parameters);
        member->parameters = NULL;
    }
    
    /* 注意：不释放 member 本身，因为可能是数组元素 */
}

/**
 * @brief 销毁类成员
 *
 * 释放类成员的所有资源，包括结构体本身。
 * 仅用于单独分配的类成员，不适用于数组元素。
 */
void cn_ast_class_member_destroy(CnClassMember *member)
{
    if (!member) {
        return;
    }
    
    /* 清理成员内容 */
    cn_ast_class_member_cleanup(member);
    
    /* 释放结构体本身 */
    free(member);
}

/**
 * @brief 销毁类声明节点
 */
void cn_ast_class_decl_destroy(CnAstClassDecl *class_decl)
{
    if (!class_decl) {
        return;
    }
    
    /* 释放类名 */
    if (class_decl->name) {
        free((void *)class_decl->name);
        class_decl->name = NULL;
    }
    
    /* 释放基类数组 */
    if (class_decl->bases) {
        for (size_t i = 0; i < class_decl->base_count; i++) {
            if (class_decl->bases[i].base_class_name) {
                free((void *)class_decl->bases[i].base_class_name);
            }
        }
        free(class_decl->bases);
        class_decl->bases = NULL;
    }
    
    /* 释放成员数组 */
    if (class_decl->members) {
        for (size_t i = 0; i < class_decl->member_count; i++) {
            /* 使用 cleanup 函数，因为成员是数组元素，不能单独释放 */
            cn_ast_class_member_cleanup(&class_decl->members[i]);
        }
        free(class_decl->members);
        class_decl->members = NULL;
    }
    
    free(class_decl);
}

/**
 * @brief 销毁接口声明节点
 */
void cn_ast_interface_decl_destroy(CnAstInterfaceDecl *interface_decl)
{
    if (!interface_decl) {
        return;
    }
    
    /* 释放接口名 */
    if (interface_decl->name) {
        free((void *)interface_decl->name);
        interface_decl->name = NULL;
    }
    
    /* 释放基接口数组 */
    if (interface_decl->base_interfaces) {
        for (size_t i = 0; i < interface_decl->base_interface_count; i++) {
            if (interface_decl->base_interfaces[i].base_class_name) {
                free((void *)interface_decl->base_interfaces[i].base_class_name);
            }
        }
        free(interface_decl->base_interfaces);
        interface_decl->base_interfaces = NULL;
    }
    
    /* 释放方法数组 */
    if (interface_decl->methods) {
        for (size_t i = 0; i < interface_decl->method_count; i++) {
            /* 使用 cleanup 函数，因为方法是数组元素，不能单独释放 */
            cn_ast_class_member_cleanup(&interface_decl->methods[i]);
        }
        free(interface_decl->methods);
        interface_decl->methods = NULL;
    }
    
    free(interface_decl);
}

/**
 * @brief 销毁接口实例化节点
 */
void cn_ast_interface_instantiation_destroy(CnAstInterfaceInstantiation *instantiation)
{
    if (!instantiation) {
        return;
    }
    
    /* 释放接口名 */
    if (instantiation->interface_name) {
        free((void *)instantiation->interface_name);
        instantiation->interface_name = NULL;
    }
    
    /* 释放类型实参数组（不释放类型本身，由类型系统管理） */
    if (instantiation->type_args) {
        free(instantiation->type_args);
        instantiation->type_args = NULL;
    }
    
    free(instantiation);
}

/* ============================================================================
 * 操作函数实现
 * ============================================================================ */

/**
 * @brief 向类声明添加成员
 */
int cn_ast_class_decl_add_member(CnAstClassDecl *class_decl, CnClassMember *member)
{
    if (!class_decl || !member) {
        return -1;
    }
    
    /* 扩展成员数组 */
    size_t new_count = class_decl->member_count + 1;
    CnClassMember *new_members = (CnClassMember *)realloc(
        class_decl->members,
        new_count * sizeof(CnClassMember)
    );
    
    if (!new_members) {
        return -1;
    }
    
    class_decl->members = new_members;
    
    /* 复制成员到数组末尾 */
    class_decl->members[class_decl->member_count] = *member;
    class_decl->member_count = new_count;
    
    /* 释放原始member结构体（内容已复制） */
    free(member);
    
    return 0;
}

/**
 * @brief 向类声明添加基类
 */
int cn_ast_class_decl_add_base(CnAstClassDecl *class_decl,
                               const char *base_class_name, size_t base_class_name_length,
                               CnAccessLevel access, bool is_virtual)
{
    if (!class_decl || !base_class_name || base_class_name_length == 0) {
        return -1;
    }
    
    /* 扩展基类数组 */
    size_t new_count = class_decl->base_count + 1;
    CnInheritanceInfo *new_bases = (CnInheritanceInfo *)realloc(
        class_decl->bases,
        new_count * sizeof(CnInheritanceInfo)
    );
    
    if (!new_bases) {
        return -1;
    }
    
    class_decl->bases = new_bases;
    
    /* 设置新的基类信息 */
    CnInheritanceInfo *info = &class_decl->bases[class_decl->base_count];
    info->base_class_name = cn_strndup(base_class_name, base_class_name_length);
    if (!info->base_class_name) {
        return -1;
    }
    info->base_class_name_length = base_class_name_length;
    info->access = access;
    info->is_virtual = is_virtual;
    
    class_decl->base_count = new_count;
    
    return 0;
}

/**
 * @brief 向类声明添加实现的接口（支持模板参数）
 */
int cn_ast_class_decl_add_interface(CnAstClassDecl *class_decl,
                                    CnAstInterfaceInstantiation *interface_inst)
{
    if (!class_decl || !interface_inst) {
        return -1;
    }
    
    /* 扩展接口实例化数组 */
    size_t new_count = class_decl->implemented_interface_count + 1;
    CnAstInterfaceInstantiation **new_interfaces = (CnAstInterfaceInstantiation **)realloc(
        class_decl->implemented_interfaces,
        new_count * sizeof(CnAstInterfaceInstantiation *)
    );
    if (!new_interfaces) {
        return -1;
    }
    class_decl->implemented_interfaces = new_interfaces;
    
    /* 添加接口实例化节点 */
    class_decl->implemented_interfaces[class_decl->implemented_interface_count] = interface_inst;
    class_decl->implemented_interface_count = new_count;
    
    return 0;
}

/**
 * @brief 向接口声明添加方法
 */
int cn_ast_interface_decl_add_method(CnAstInterfaceDecl *interface_decl, CnClassMember *method)
{
    if (!interface_decl || !method) {
        return -1;
    }
    
    /* 扩展方法数组 */
    size_t new_count = interface_decl->method_count + 1;
    CnClassMember *new_methods = (CnClassMember *)realloc(
        interface_decl->methods,
        new_count * sizeof(CnClassMember)
    );
    
    if (!new_methods) {
        return -1;
    }
    
    interface_decl->methods = new_methods;
    
    /* 复制方法到数组末尾 */
    interface_decl->methods[interface_decl->method_count] = *method;
    interface_decl->method_count = new_count;
    
    /* 释放原始method结构体（内容已复制） */
    free(method);
    
    return 0;
}

/**
 * @brief 向接口声明添加基接口
 */
int cn_ast_interface_decl_add_base_interface(CnAstInterfaceDecl *interface_decl,
                                             const char *base_interface_name,
                                             size_t base_interface_name_length)
{
    if (!interface_decl || !base_interface_name || base_interface_name_length == 0) {
        return -1;
    }
    
    /* 扩展基接口数组 */
    size_t new_count = interface_decl->base_interface_count + 1;
    CnInheritanceInfo *new_bases = (CnInheritanceInfo *)realloc(
        interface_decl->base_interfaces,
        new_count * sizeof(CnInheritanceInfo)
    );
    
    if (!new_bases) {
        return -1;
    }
    
    interface_decl->base_interfaces = new_bases;
    
    /* 设置新的基接口信息 */
    CnInheritanceInfo *info = &interface_decl->base_interfaces[interface_decl->base_interface_count];
    info->base_class_name = cn_strndup(base_interface_name, base_interface_name_length);
    if (!info->base_class_name) {
        return -1;
    }
    info->base_class_name_length = base_interface_name_length;
    info->access = CN_ACCESS_PUBLIC; /* 接口继承默认为公开 */
    info->is_virtual = false;        /* 接口不支持虚继承 */
    
    interface_decl->base_interface_count = new_count;
    
    return 0;
}
