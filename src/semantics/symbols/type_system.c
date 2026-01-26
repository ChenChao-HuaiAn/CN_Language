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

CnType *cn_type_new_function(CnType *return_type, CnType **param_types, size_t param_count) {
    CnType *type = (CnType *)malloc(sizeof(CnType));
    if (!type) return NULL;
    type->kind = CN_TYPE_FUNCTION;
    type->as.function.return_type = return_type;
    type->as.function.param_types = param_types;
    type->as.function.param_count = param_count;
    return type;
}

// 创建结构体类型
CnType *cn_type_new_struct(const char *name, size_t name_length, CnStructField *fields, size_t field_count) {
    CnType *type = (CnType *)malloc(sizeof(CnType));
    if (!type) return NULL;
    type->kind = CN_TYPE_STRUCT;
    type->as.struct_type.name = name;
    type->as.struct_type.name_length = name_length;
    type->as.struct_type.fields = fields;
    type->as.struct_type.field_count = field_count;
    return type;
}

// 创建枚举类型
CnType *cn_type_new_enum(const char *name, size_t name_length) {
    CnType *type = (CnType *)malloc(sizeof(CnType));
    if (!type) return NULL;
    type->kind = CN_TYPE_ENUM;
    type->as.enum_type.name = name;
    type->as.enum_type.name_length = name_length;
    type->as.enum_type.enum_scope = NULL; // 作用域将在scope_builder中创建
    return type;
}

CnType *cn_type_new_memory_address(void) {
    CnType *type = (CnType *)malloc(sizeof(CnType));
    if (!type) return NULL;
    type->kind = CN_TYPE_MEMORY_ADDRESS;
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
            if (!a->as.struct_type.name || !b->as.struct_type.name) return false;
            if (a->as.struct_type.name_length != b->as.struct_type.name_length) return false;
            return memcmp(a->as.struct_type.name, b->as.struct_type.name, a->as.struct_type.name_length) == 0;
        case CN_TYPE_ENUM:
            if (!a->as.enum_type.name || !b->as.enum_type.name) return false;
            if (a->as.enum_type.name_length != b->as.enum_type.name_length) return false;
            return memcmp(a->as.enum_type.name, b->as.enum_type.name, a->as.enum_type.name_length) == 0;
        case CN_TYPE_FUNCTION:
            if (a->as.function.param_count != b->as.function.param_count) return false;
            if (!cn_type_equals(a->as.function.return_type, b->as.function.return_type)) return false;
            for (size_t i = 0; i < a->as.function.param_count; i++) {
                if (!cn_type_equals(a->as.function.param_types[i], b->as.function.param_types[i])) return false;
            }
            return true;
        default:
            return true;
    }
}

bool cn_type_compatible(CnType *a, CnType *b) {
    // 基础实现：等价即兼容
    // 后续可以增加隐式转换逻辑，例如 int -> float
    return cn_type_equals(a, b);
}

// 在结构体类型中查找成员字段
CnStructField *cn_type_struct_find_field(CnType *struct_type,
                                         const char *field_name,
                                         size_t field_name_length) {
    if (!struct_type || struct_type->kind != CN_TYPE_STRUCT) {
        return NULL;
    }
    if (!field_name) {
        return NULL;
    }

    // 遍历结构体的所有字段
    for (size_t i = 0; i < struct_type->as.struct_type.field_count; i++) {
        CnStructField *field = &struct_type->as.struct_type.fields[i];
        if (field->name_length == field_name_length &&
            memcmp(field->name, field_name, field_name_length) == 0) {
            return field;
        }
    }

    return NULL; // 未找到匹配的字段
}

// 在枚举类型中查找成员
CnSemSymbol *cn_type_enum_find_member(CnType *enum_type,
                                      const char *member_name,
                                      size_t member_name_length) {
    if (!enum_type || enum_type->kind != CN_TYPE_ENUM) {
        return NULL;
    }
    if (!member_name || !enum_type->as.enum_type.enum_scope) {
        return NULL;
    }

    // 在枚举作用域中查找成员
    return cn_sem_scope_lookup_shallow(enum_type->as.enum_type.enum_scope,
                                       member_name,
                                       member_name_length);
}
