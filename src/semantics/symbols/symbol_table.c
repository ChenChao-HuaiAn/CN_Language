#include "cnlang/frontend/semantics.h"

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

CnSemSymbol *cn_sem_scope_insert_symbol(CnSemScope *scope,
                                        const char *name,
                                        size_t name_length,
                                        CnSemSymbolKind kind)
{
    CnSemSymbolNode *node;

    if (!scope || !name || name_length == 0) {
        return NULL;
    }

    if (cn_sem_scope_lookup_shallow(scope, name, name_length) != NULL) {
        return NULL;
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

    node->next = scope->symbols;
    scope->symbols = node;

    return &node->symbol;
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

    if (!name || name_length == 0) {
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
