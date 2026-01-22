#include <stdio.h>
#include <assert.h>
#include "cnlang/frontend/semantics.h"

void test_primitive_types() {
    CnType *t_int = cn_type_new_primitive(CN_TYPE_INT);
    CnType *t_float = cn_type_new_primitive(CN_TYPE_FLOAT);
    CnType *t_int2 = cn_type_new_primitive(CN_TYPE_INT);

    assert(cn_type_equals(t_int, t_int2));
    assert(!cn_type_equals(t_int, t_float));

    // 基本兼容性（目前简单等价）
    assert(cn_type_compatible(t_int, t_int2));
    
    printf("test_primitive_types: PASSED\n");
}

void test_pointer_types() {
    CnType *t_int = cn_type_new_primitive(CN_TYPE_INT);
    CnType *p_int = cn_type_new_pointer(t_int);
    CnType *p_int2 = cn_type_new_pointer(t_int);
    CnType *t_float = cn_type_new_primitive(CN_TYPE_FLOAT);
    CnType *p_float = cn_type_new_pointer(t_float);

    assert(cn_type_equals(p_int, p_int2));
    assert(!cn_type_equals(p_int, p_float));
    assert(!cn_type_equals(p_int, t_int));

    printf("test_pointer_types: PASSED\n");
}

void test_function_types() {
    CnType *t_int = cn_type_new_primitive(CN_TYPE_INT);
    CnType *t_void = cn_type_new_primitive(CN_TYPE_VOID);
    
    CnType *params1[] = { t_int };
    CnType *f1 = cn_type_new_function(t_void, params1, 1);

    CnType *params2[] = { t_int };
    CnType *f2 = cn_type_new_function(t_void, params2, 1);

    CnType *params3[] = { t_int, t_int };
    CnType *f3 = cn_type_new_function(t_void, params3, 2);

    assert(cn_type_equals(f1, f2));
    assert(!cn_type_equals(f1, f3));

    printf("test_function_types: PASSED\n");
}

int main() {
    test_primitive_types();
    test_pointer_types();
    test_function_types();
    return 0;
}
