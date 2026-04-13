#include "cnlang/frontend/semantics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 符号链表节点，用于在作用域中维护符号列表
typedef struct CnSemSymbolNode {
    CnSemSymbol symbol;
    struct CnSemSymbolNode *next;
} CnSemSymbolNode;

// 作用域结构体的内部实现
struct CnSemScope {
    CnSemScopeKind kind;
    CnSemScope *parent;
    CnSemSymbolNode *symbols;
    const char *name;          // 作用域名称(对于函数作用域为函数名)
    size_t name_length;        // 作用域名称长度
    CnFileModuleSemInfo *file_module_info;  // 文件模块信息（仅当kind==CN_SEM_SCOPE_FILE_MODULE时有效）
};

static int cn_sem_symbol_name_equals(const CnSemSymbol *symbol,
                                     const char *name,
                                     size_t name_length)
{
    if (!symbol || !name) {
        return 0;
    }

    if (symbol->name_length != name_length) {
        return 0;
    }

    if (symbol->name == name) {
        return 1;
    }

    if (!symbol->name) {
        return 0;
    }

    return memcmp(symbol->name, name, name_length) == 0;
}

CnSemScope *cn_sem_scope_new(CnSemScopeKind kind, CnSemScope *parent)
{
    CnSemScope *scope;

    scope = (CnSemScope *)malloc(sizeof(CnSemScope));
    if (!scope) {
        return NULL;
    }

    scope->kind = kind;
    scope->parent = parent;
    scope->symbols = NULL;
    scope->name = NULL;
    scope->name_length = 0;
    scope->file_module_info = NULL;

    return scope;
}

void cn_sem_scope_free(CnSemScope *scope)
{
    CnSemSymbolNode *node;
    CnSemSymbolNode *next;

    if (!scope) {
        return;
    }

    node = scope->symbols;
    while (node) {
        next = node->next;
        free(node);
        node = next;
    }

    free(scope);
}

CnSemScope *cn_sem_scope_parent(CnSemScope *scope)
{
    if (!scope) {
        return NULL;
    }

    return scope->parent;
}

CnSemScopeKind cn_sem_scope_get_kind(CnSemScope *scope)
{
    if (!scope) {
        return CN_SEM_SCOPE_GLOBAL;  // 默认返回全局作用域类型
    }

    return scope->kind;
}

void cn_sem_scope_set_name(CnSemScope *scope, const char *name, size_t name_length)
{
    if (!scope) {
        return;
    }
    scope->name = name;
    scope->name_length = name_length;
}

const char *cn_sem_scope_get_name(CnSemScope *scope, size_t *out_length)
{
    if (!scope) {
        if (out_length) *out_length = 0;
        return NULL;
    }
    if (out_length) {
        *out_length = scope->name_length;
    }
    return scope->name;
}

// 辅助函数：检查两个符号是否是同一个符号（来自同一模块的同一声明）
// 用于解决跨编译会话的符号唯一性问题
static int cn_sem_is_same_symbol(const CnSemSymbol *sym1, const CnSemSymbol *sym2) {
    if (!sym1 || !sym2) {
        return 0;
    }
    
    // 检查符号种类是否相同
    if (sym1->kind != sym2->kind) {
        return 0;
    }
    
    // 检查名字是否相同
    if (sym1->name_length != sym2->name_length) {
        return 0;
    }
    if (memcmp(sym1->name, sym2->name, sym1->name_length) != 0) {
        return 0;
    }
    
    // 【关键修复】首先检查源模块路径是否相同
    // 这是解决跨编译会话符号唯一性问题的关键
    if (sym1->source_module_path && sym2->source_module_path &&
        sym1->source_module_path_length == sym2->source_module_path_length &&
        memcmp(sym1->source_module_path, sym2->source_module_path, sym1->source_module_path_length) == 0) {
        return 1;  // 来自同一个模块且名称相同，是同一个符号
    }
    
    // 检查类型指针是否相同
    if (sym1->type && sym2->type && sym1->type == sym2->type) {
        return 1;  // 类型指针相同，肯定是同一个符号
    }
    
    // 检查是否来自同一个声明作用域
    if (sym1->decl_scope && sym2->decl_scope && sym1->decl_scope == sym2->decl_scope) {
        return 1;  // 声明作用域相同，是同一个符号
    }
    
    return 0;  // 不是同一个符号
}

CnSemSymbol *cn_sem_scope_insert_symbol(CnSemScope *scope,
                                        const char *name,
                                        size_t name_length,
                                        CnSemSymbolKind kind)
{
    CnSemSymbolNode *node;

    if (!scope || !name || name_length == 0) {
        return NULL;
    }

    // 检查是否已存在同名符号
    CnSemSymbol *existing = cn_sem_scope_lookup_shallow(scope, name, name_length);
    if (existing) {
        // 【关键修复】允许模块符号和类型符号共存
        // 模块符号（CN_SEM_SYMBOL_MODULE）用于点访问语法（如 `词元.词元类型枚举`）
        // 类型符号（CN_SEM_SYMBOL_STRUCT, CN_SEM_SYMBOL_ENUM）用于类型上下文（如 `词元 变量名`）
        // 这两种符号可以共存，因为它们的使用场景不同
        if (existing->kind == CN_SEM_SYMBOL_MODULE &&
            (kind == CN_SEM_SYMBOL_STRUCT || kind == CN_SEM_SYMBOL_ENUM)) {
            // 允许模块符号和类型符号共存，继续插入
            }
        else if ((existing->kind == CN_SEM_SYMBOL_STRUCT || existing->kind == CN_SEM_SYMBOL_ENUM) &&
                 kind == CN_SEM_SYMBOL_MODULE) {
            // 允许类型符号和模块符号共存，继续插入
            }
        else {
            // 其他情况：同名符号插入失败
            // 调用者应该检查是否是同一个符号（使用 cn_sem_is_same_symbol）
            // 如果是同一个符号，应该静默忽略
            return NULL;
        }
    }

    node = (CnSemSymbolNode *)malloc(sizeof(CnSemSymbolNode));
    if (!node) {
        return NULL;
    }

    node->symbol.name = name;
    node->symbol.name_length = name_length;
    node->symbol.kind = kind;
    node->symbol.decl_scope = scope;
    node->symbol.type = NULL;
    node->symbol.is_public = 0; // 默认私有（模块成员需要显式标记为公开）
    node->symbol.is_const = 0;  // 默认非常量
    node->symbol.source_module_path = NULL;  // 初始化源模块路径为 NULL
    node->symbol.source_module_path_length = 0;
    node->symbol.as.module_scope = NULL; // 初始化 module_scope 为 NULL

    node->next = scope->symbols;
    scope->symbols = node;

    return &node->symbol;
}

// 检查两个符号是否是同一个符号（来自同一模块的同一声明）
// 用于解决跨编译会话的符号唯一性问题
int cn_sem_symbol_is_same(const CnSemSymbol *sym1, const CnSemSymbol *sym2) {
    return cn_sem_is_same_symbol(sym1, sym2);
}

CnSemSymbol *cn_sem_scope_lookup_shallow(CnSemScope *scope,
                                         const char *name,
                                         size_t name_length)
{
    CnSemSymbolNode *node;

    if (!scope || !name || name_length == 0) {
        return NULL;
    }

    node = scope->symbols;
    while (node) {
        if (cn_sem_symbol_name_equals(&node->symbol, name, name_length)) {
            return &node->symbol;
        }

        node = node->next;
    }

    return NULL;
}

CnSemSymbol *cn_sem_scope_lookup(CnSemScope *scope,
                                 const char *name,
                                 size_t name_length)
{
    CnSemSymbol *symbol;

    if (!scope || !name || name_length == 0) {
        return NULL;
    }

    while (scope) {
        symbol = cn_sem_scope_lookup_shallow(scope, name, name_length);
        if (symbol) {
            return symbol;
        }

        scope = scope->parent;
    }

    return NULL;
}

// 按符号类型查找：在同名符号共存的情况下，优先返回指定类型的符号
// 用于类型上下文（如变量声明）中查找类型符号，而非模块符号
CnSemSymbol *cn_sem_scope_lookup_by_kind(CnSemScope *scope,
                                         const char *name,
                                         size_t name_length,
                                         CnSemSymbolKind preferred_kind)
{
    CnSemSymbol *fallback = NULL;  // 用于存储非首选类型的符号

    if (!scope || !name || name_length == 0) {
        return NULL;
    }

    while (scope) {
        // 遍历当前作用域的所有符号
        CnSemSymbolNode *node = scope->symbols;
        while (node) {
            if (cn_sem_symbol_name_equals(&node->symbol, name, name_length)) {
                // 找到同名符号
                if (node->symbol.kind == preferred_kind) {
                    // 找到首选类型的符号，直接返回
                    return &node->symbol;
                }
                // 记录第一个找到的非首选类型符号作为备选
                if (!fallback) {
                    fallback = &node->symbol;
                }
            }
            node = node->next;
        }
        scope = scope->parent;
    }

    // 如果没有找到首选类型的符号，返回备选符号
    return fallback;
}

void cn_sem_scope_foreach_symbol(CnSemScope *scope,
                                  CnSemScopeSymbolCallback callback,
                                  void *user_data)
{
    if (!scope || !callback) {
        return;
    }

    CnSemSymbolNode *node = scope->symbols;
    while (node) {
        callback(&node->symbol, user_data);
        node = node->next;
    }
}

/**
 * @brief 深度复制枚举作用域
 *
 * 用于模块导入时复制枚举成员信息，确保导入的枚举成员信息不会因为
 * 原模块被重新编译或跨编译会话而丢失。
 *
 * @param src_scope 源枚举作用域
 * @return 新分配的枚举作用域，失败返回 NULL
 */
CnSemScope *cn_sem_scope_deep_copy_enum(CnSemScope *src_scope) {
    if (!src_scope) {
        return NULL;
    }

    // 只处理枚举作用域
    if (src_scope->kind != CN_SEM_SCOPE_ENUM) {
        return NULL;
    }

    // 创建新的枚举作用域（父作用域设为NULL，因为导入后作用域链不同）
    CnSemScope *dst_scope = cn_sem_scope_new(CN_SEM_SCOPE_ENUM, NULL);
    if (!dst_scope) {
        return NULL;
    }

    // 复制作用域名称
    if (src_scope->name && src_scope->name_length > 0) {
        cn_sem_scope_set_name(dst_scope, src_scope->name, src_scope->name_length);
    }

    // 遍历源作用域中的所有符号，复制枚举成员
    CnSemSymbolNode *node = src_scope->symbols;
    while (node) {
        CnSemSymbol *src_sym = &node->symbol;

        // 在目标作用域中插入新符号
        CnSemSymbol *dst_sym = cn_sem_scope_insert_symbol(
            dst_scope,
            src_sym->name,
            src_sym->name_length,
            src_sym->kind);

        if (dst_sym) {
            // 复制符号属性
            dst_sym->is_public = src_sym->is_public;
            dst_sym->is_const = src_sym->is_const;
            dst_sym->is_static = src_sym->is_static;
            dst_sym->source_module_path = src_sym->source_module_path;
            dst_sym->source_module_path_length = src_sym->source_module_path_length;

            // 复制类型信息（枚举成员的类型是整数，可以直接复制）
            dst_sym->type = cn_type_new_primitive(CN_TYPE_INT);

            // 复制枚举值
            if (src_sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                dst_sym->as.enum_value = src_sym->as.enum_value;
            }
        }

        node = node->next;
    }

    return dst_scope;
}
