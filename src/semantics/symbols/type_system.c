#include "cnlang/frontend/semantics.h"
#include <stdlib.h>
#include <string.h>

CnType *cn_type_new_primitive(CnTypeKind kind) {
    CnType *type = (CnType *)malloc(sizeof(CnType));
    if (!type) return NULL;
    type->kind = kind;
    return type;
}

CnType *cn_type_new_pointer(CnType *base) {
    CnType *type = (CnType *)malloc(sizeof(CnType));
    if (!type) return NULL;
    type->kind = CN_TYPE_POINTER;
    type->as.pointer_to = base;
    return type;
}

CnType *cn_type_new_array(CnType *element, size_t length) {
    CnType *type = (CnType *)malloc(sizeof(CnType));
    if (!type) return NULL;
    type->kind = CN_TYPE_ARRAY;
    type->as.array.element_type = element;
    type->as.array.length = length;
    return type;
}

bool cn_type_equals(CnType *a, CnType *b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;

    switch (a->kind) {
        case CN_TYPE_POINTER:
            return cn_type_equals(a->as.pointer_to, b->as.pointer_to);
        case CN_TYPE_ARRAY:
            return (a->as.array.length == b->as.array.length) &&
                   cn_type_equals(a->as.array.element_type, b->as.array.element_type);
        case CN_TYPE_STRUCT:
            if (!a->as.struct_name || !b->as.struct_name) return false;
            return strcmp(a->as.struct_name, b->as.struct_name) == 0;
        default:
            return true;
    }
}

bool cn_type_compatible(CnType *a, CnType *b) {
    // 基础实现：等价即兼容
    // 后续可以增加隐式转换逻辑，例如 int -> float
    return cn_type_equals(a, b);
}
