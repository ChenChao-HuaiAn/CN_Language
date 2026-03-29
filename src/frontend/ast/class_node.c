/**
 * @file class_node.c
 * @brief CN语言面向对象编程 - 类AST节点实现
 */

#include "cnlang/frontend/ast/class_node.h"
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
    class_decl->implemented_interface_lengths = NULL;
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
    
    return interface_decl;
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
 * @brief 销毁类成员
 */
void cn_ast_class_member_destroy(CnClassMember *member)
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
            cn_ast_class_member_destroy(&class_decl->members[i]);
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
            cn_ast_class_member_destroy(&interface_decl->methods[i]);
        }
        free(interface_decl->methods);
        interface_decl->methods = NULL;
    }
    
    free(interface_decl);
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
 * @brief 向类声明添加实现的接口
 */
int cn_ast_class_decl_add_interface(CnAstClassDecl *class_decl,
                                    const char *interface_name,
                                    size_t interface_name_length)
{
    if (!class_decl || !interface_name || interface_name_length == 0) {
        return -1;
    }
    
    /* 扩展接口名数组 */
    size_t new_count = class_decl->implemented_interface_count + 1;
    char **new_interfaces = (char **)realloc(
        class_decl->implemented_interfaces,
        new_count * sizeof(char *)
    );
    if (!new_interfaces) {
        return -1;
    }
    class_decl->implemented_interfaces = new_interfaces;
    
    /* 扩展接口名长度数组 */
    size_t *new_lengths = (size_t *)realloc(
        class_decl->implemented_interface_lengths,
        new_count * sizeof(size_t)
    );
    if (!new_lengths) {
        return -1;
    }
    class_decl->implemented_interface_lengths = new_lengths;
    
    /* 复制接口名 */
    class_decl->implemented_interfaces[class_decl->implemented_interface_count] =
        cn_strndup(interface_name, interface_name_length);
    if (!class_decl->implemented_interfaces[class_decl->implemented_interface_count]) {
        return -1;
    }
    class_decl->implemented_interface_lengths[class_decl->implemented_interface_count] =
        interface_name_length;
    
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
