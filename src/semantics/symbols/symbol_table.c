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
        // 如果已存在同名符号，返回 NULL 表示插入失败
        // 调用者应该检查是否是同一个符号（使用 cn_sem_is_same_symbol）
        // 如果是同一个符号，应该静默忽略
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
