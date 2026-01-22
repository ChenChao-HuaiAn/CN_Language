#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "cnlang/frontend/semantics.h"

void test_scope_creation() {
    CnSemScope *global = cn_sem_scope_new(CN_SEM_SCOPE_GLOBAL, NULL);
    assert(global != NULL);
    assert(cn_sem_scope_parent(global) == NULL);

    CnSemScope *local = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, global);
    assert(local != NULL);
    assert(cn_sem_scope_parent(local) == global);

    cn_sem_scope_free(local);
    cn_sem_scope_free(global);
    printf("test_scope_creation: PASSED\n");
}

void test_symbol_insertion_and_lookup() {
    CnSemScope *global = cn_sem_scope_new(CN_SEM_SCOPE_GLOBAL, NULL);
    
    CnSemSymbol *sym1 = cn_sem_scope_insert_symbol(global, "a", 1, CN_SEM_SYMBOL_VARIABLE);
    assert(sym1 != NULL);
    assert(strcmp(sym1->name, "a") == 0);

    // 重复插入
    CnSemSymbol *sym1_dup = cn_sem_scope_insert_symbol(global, "a", 1, CN_SEM_SYMBOL_VARIABLE);
    assert(sym1_dup == NULL);

    // 查找
    CnSemSymbol *found = cn_sem_scope_lookup(global, "a", 1);
    assert(found == sym1);

    CnSemScope *local = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, global);
    
    // 嵌套查找
    found = cn_sem_scope_lookup(local, "a", 1);
    assert(found == sym1);

    // 浅层查找
    found = cn_sem_scope_lookup_shallow(local, "a", 1);
    assert(found == NULL);

    // 局部遮蔽
    CnSemSymbol *sym2 = cn_sem_scope_insert_symbol(local, "a", 1, CN_SEM_SYMBOL_VARIABLE);
    assert(sym2 != NULL);
    assert(sym2 != sym1);

    found = cn_sem_scope_lookup(local, "a", 1);
    assert(found == sym2);

    cn_sem_scope_free(local);
    cn_sem_scope_free(global);
    printf("test_symbol_insertion_and_lookup: PASSED\n");
}

int main() {
    test_scope_creation();
    test_symbol_insertion_and_lookup();
    return 0;
}
