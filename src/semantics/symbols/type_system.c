#include "cnlang/frontend/semantics.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

CnType *cn_type_new_primitive(CnTypeKind kind) {
    CnType *type = (CnType *)malloc(sizeof(CnType));
    if (!type) {
        // 内存分配失败时返回一个静态的未知类型，避免 NULL 指针
        static CnType unknown_type = { .kind = CN_TYPE_UNKNOWN };
        return &unknown_type;
    }
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
CnType *cn_type_new_struct(const char *name, size_t name_length, CnStructField *fields, size_t field_count, CnSemScope *decl_scope, const char *owner_func_name, size_t owner_func_name_length) {
    CnType *type = (CnType *)malloc(sizeof(CnType));
    if (!type) return NULL;
    type->kind = CN_TYPE_STRUCT;
    type->as.struct_type.name = name;
    type->as.struct_type.name_length = name_length;
    type->as.struct_type.fields = fields;
    type->as.struct_type.field_count = field_count;
    type->as.struct_type.decl_scope = decl_scope;
    type->as.struct_type.owner_func_name = owner_func_name;
    type->as.struct_type.owner_func_name_length = owner_func_name_length;
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
    // 空指针检查
    if (!a || !b) return false;
    
    // 基础实现：等价即兼容
    if (cn_type_equals(a, b)) {
        return true;
    }
    
    // 整数类型之间的兼容性
    bool a_is_int = (a->kind == CN_TYPE_INT || a->kind == CN_TYPE_INT32 ||
                     a->kind == CN_TYPE_INT64 || a->kind == CN_TYPE_UINT32 ||
                     a->kind == CN_TYPE_UINT64 || a->kind == CN_TYPE_UINT64_LL);
    bool b_is_int = (b->kind == CN_TYPE_INT || b->kind == CN_TYPE_INT32 ||
                     b->kind == CN_TYPE_INT64 || b->kind == CN_TYPE_UINT32 ||
                     b->kind == CN_TYPE_UINT64 || b->kind == CN_TYPE_UINT64_LL);
    
    if (a_is_int && b_is_int) {
        return true;  // 所有整数类型之间可以隐式转换
    }
    
    // 字符类型与整数类型的兼容性
    // 字符类型可以与整数类型互相赋值
    bool a_is_char = (a->kind == CN_TYPE_CHAR);
    bool b_is_char = (b->kind == CN_TYPE_CHAR);
    
    if ((a_is_char && b_is_int) || (a_is_int && b_is_char) || (a_is_char && b_is_char)) {
        return true;  // 字符类型与整数类型之间可以隐式转换
    }
    
    // 枚举类型与整数类型的兼容性
    // 枚举类型可以与整数类型互相赋值
    // 枚举类型也可以与其他枚举类型赋值（都视为整数）
    bool a_is_enum_or_int = a_is_int || (a->kind == CN_TYPE_ENUM);
    bool b_is_enum_or_int = b_is_int || (b->kind == CN_TYPE_ENUM);
    
    if (a_is_enum_or_int && b_is_enum_or_int) {
        return true;  // 枚举类型与整数类型之间可以隐式转换
    }
    
    // 浮点类型之间的兼容性
    bool a_is_float = (a->kind == CN_TYPE_FLOAT || a->kind == CN_TYPE_FLOAT32 || 
                       a->kind == CN_TYPE_FLOAT64);
    bool b_is_float = (b->kind == CN_TYPE_FLOAT || b->kind == CN_TYPE_FLOAT32 || 
                       b->kind == CN_TYPE_FLOAT64);
    
    if (a_is_float && b_is_float) {
        return true;  // 所有浮点类型之间可以隐式转换
    }
    
    // 整数到浮点的隐式转换
    if (a_is_int && b_is_float) {
        return true;
    }
    
    // 整数到指针的隐式转换（用于空指针常量：整数0可以赋值给任何指针类型）
    // 在CN语言中，"无"关键字被解析为整数0，需要能够赋值给任何指针类型
    if (a_is_int && b->kind == CN_TYPE_POINTER) {
        return true;
    }
    
    // 整数到字符串的隐式转换（用于空指针常量：整数0可以赋值给字符串类型）
    // 在CN语言中，"无"关键字被解析为整数0，需要能够赋值给字符串类型
    if (a_is_int && b->kind == CN_TYPE_STRING) {
        return true;
    }
    
    // 整数到结构体类型的隐式转换（用于枚举类型参数）
    // 在CN语言中，枚举类型在解析阶段被识别为结构体类型，
    // 而枚举成员的类型是整数类型，因此需要允许整数传递给结构体类型参数
    // 这样可以支持函数调用时传入枚举成员值
    if (a_is_int && b->kind == CN_TYPE_STRUCT) {
        return true;
    }
    
    // 枚举类型到结构体类型的隐式转换
    // 在CN语言中，枚举类型在解析阶段被识别为结构体类型，
    // 当函数参数类型是枚举类型时，解析器会将其识别为结构体类型，
    // 而传入的枚举成员的类型是枚举类型(CN_TYPE_ENUM)，
    // 因此需要允许枚举类型传递给结构体类型参数
    if (a->kind == CN_TYPE_ENUM && b->kind == CN_TYPE_STRUCT) {
        return true;
    }
    
    // 指针类型之间的兼容性
    if (a->kind == CN_TYPE_POINTER && b->kind == CN_TYPE_POINTER) {
        // 特殊处理：空类型指针（void*）可以接受任何类型的指针
        if (b->as.pointer_to->kind == CN_TYPE_VOID) {
            return true;  // 任何指针都可以赋值给 void*
        }
        // 特殊处理：任何类型的指针可以赋值给空类型指针（void*）
        if (a->as.pointer_to->kind == CN_TYPE_VOID) {
            return true;  // void* 可以赋值给任何指针类型（C语言中需要显式转换，但CN语言允许）
        }
        // 普通情况：检查指向的类型是否兼容
        return cn_type_compatible(a->as.pointer_to, b->as.pointer_to);
    }
    
    // 数组到指针的隐式转换（C语言中数组参数退化为指针）
    // 数组类型传递给指针参数
    if (a->kind == CN_TYPE_ARRAY && b->kind == CN_TYPE_POINTER) {
        // 特殊处理：任何数组都可以隐式转换为 void*
        if (b->as.pointer_to && b->as.pointer_to->kind == CN_TYPE_VOID) {
            return true;  // 数组可以退化为 void*
        }
        // 检查数组元素类型与指针指向类型是否兼容
        return cn_type_compatible(a->as.array.element_type, b->as.pointer_to);
    }
    
    // 数组类型传递给数组参数（不同大小的数组之间的兼容性）
    if (a->kind == CN_TYPE_ARRAY && b->kind == CN_TYPE_ARRAY) {
        // 元素类型必须兼容
        return cn_type_compatible(a->as.array.element_type, b->as.array.element_type);
    }
    
    // 指针类型传递给数组参数（反向兼容）
    if (a->kind == CN_TYPE_POINTER && b->kind == CN_TYPE_ARRAY) {
        return cn_type_compatible(a->as.pointer_to, b->as.array.element_type);
    }
    
    // 字符* 到 字符串 的隐式转换
    // 根据设计文档：字符* 可以隐式转换为字符串（安全转换）
    if (a->kind == CN_TYPE_POINTER && b->kind == CN_TYPE_STRING) {
        // 检查是否是 字符* (char*)
        if (a->as.pointer_to && a->as.pointer_to->kind == CN_TYPE_CHAR) {
            return true;  // 字符* 可以隐式转换为字符串
        }
    }
    
    // 字符[] 到 字符串 的隐式转换
    // 根据设计文档：字符[] 可以隐式转换为字符串（安全转换）
    if (a->kind == CN_TYPE_ARRAY && b->kind == CN_TYPE_STRING) {
        // 检查是否是 字符[] (char[])
        if (a->as.array.element_type && a->as.array.element_type->kind == CN_TYPE_CHAR) {
            return true;  // 字符[] 可以隐式转换为字符串
        }
    }
    
    // 字符串 到 字符* 需要显式转换（根据设计文档标记为 ⚠️）
    // 但为了便利性，我们允许隐式转换（编译器可以发出警告）
    if (a->kind == CN_TYPE_STRING && b->kind == CN_TYPE_POINTER) {
        if (b->as.pointer_to && b->as.pointer_to->kind == CN_TYPE_CHAR) {
            return true;  // 字符串 可以隐式转换为字符*（但可能发出警告）
        }
    }
    
    // 字符串 到 空类型* 的隐式转换
    // 字符串类型可以隐式转换为 void* 指针
    if (a->kind == CN_TYPE_STRING && b->kind == CN_TYPE_POINTER) {
        if (b->as.pointer_to && b->as.pointer_to->kind == CN_TYPE_VOID) {
            return true;  // 字符串 可以隐式转换为 void*
        }
    }
    
    // 任何指针 到 空类型* 的隐式转换
    // 这是C语言的标准行为
    if (a->kind == CN_TYPE_POINTER && b->kind == CN_TYPE_POINTER) {
        if (b->as.pointer_to && b->as.pointer_to->kind == CN_TYPE_VOID) {
            return true;  // 任何指针都可以隐式转换为 void*
        }
    }
    
    // 任何数组 到 空类型* 的隐式转换
    // 数组可以退化为指针，然后转换为 void*
    if (a->kind == CN_TYPE_ARRAY && b->kind == CN_TYPE_POINTER) {
        if (b->as.pointer_to && b->as.pointer_to->kind == CN_TYPE_VOID) {
            return true;  // 任何数组都可以隐式转换为 void*
        }
    }
    
    // 任何指针 到 字符串 的隐式转换
    // 如果指针指向的是整数类型（可能是字符），允许转换为字符串
    if (a->kind == CN_TYPE_POINTER && b->kind == CN_TYPE_STRING) {
        // 允许任何指针类型转换为字符串（C语言中指针可以赋值给字符串）
        return true;
    }
    
    // 任何数组 到 字符串 的隐式转换
    // 如果数组元素是整数类型（可能是字符），允许转换为字符串
    if (a->kind == CN_TYPE_ARRAY && b->kind == CN_TYPE_STRING) {
        // 允许任何数组类型转换为字符串（C语言中数组可以退化为字符串）
        return true;
    }
    
    // UNKNOWN 类型可以接受任何类型（用于内置函数如 类型大小）
    // 同时允许 UNKNOWN 类型作为实际参数类型（临时方案，解决模块导入时类型信息丢失问题）
    if (a->kind == CN_TYPE_UNKNOWN || b->kind == CN_TYPE_UNKNOWN) {
        return true;
    }
    
    // 整数类型之间的隐式转换
    // 所有整数类型都可以隐式转换为整数
    if (b->kind == CN_TYPE_INT) {
        if (a->kind == CN_TYPE_INT32 || a->kind == CN_TYPE_INT64 ||
            a->kind == CN_TYPE_UINT32 || a->kind == CN_TYPE_UINT64 ||
            a->kind == CN_TYPE_UINT64_LL) {
            return true;
        }
    }
    
    // 整数类型之间可以相互转换
    if ((a->kind == CN_TYPE_INT || a->kind == CN_TYPE_INT32 || a->kind == CN_TYPE_INT64 ||
         a->kind == CN_TYPE_UINT32 || a->kind == CN_TYPE_UINT64 || a->kind == CN_TYPE_UINT64_LL) &&
        (b->kind == CN_TYPE_INT || b->kind == CN_TYPE_INT32 || b->kind == CN_TYPE_INT64 ||
         b->kind == CN_TYPE_UINT32 || b->kind == CN_TYPE_UINT64 || b->kind == CN_TYPE_UINT64_LL)) {
        return true;
    }
    
    return false;
}

// 前向声明：符号查找函数（来自 symbol_table.c）
extern CnSemSymbol *cn_sem_scope_lookup(CnSemScope *scope, const char *name, size_t name_length);

// 全局变量：用于动态解析的当前作用域
// 这是在 semantic_passes.c 中设置的
static CnSemScope *g_current_scope_for_resolution = NULL;

// 设置当前作用域（供 semantic_passes.c 调用）
void cn_type_set_resolution_scope(CnSemScope *scope) {
    g_current_scope_for_resolution = scope;
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

    // 动态解析：如果结构体没有字段信息，尝试从声明作用域或全局作用域查找真实类型
    // 这解决了模块导入时结构体字段信息丢失的问题
    if (!struct_type->as.struct_type.fields &&
        struct_type->as.struct_type.name) {
        CnSemSymbol *type_sym = NULL;
        
        // 首先尝试从声明作用域查找
        if (struct_type->as.struct_type.decl_scope) {
            type_sym = cn_sem_scope_lookup(
                struct_type->as.struct_type.decl_scope,
                struct_type->as.struct_type.name,
                struct_type->as.struct_type.name_length);
        }
        
        // 如果没找到，尝试从全局作用域查找
        if (!type_sym && g_current_scope_for_resolution) {
            type_sym = cn_sem_scope_lookup(
                g_current_scope_for_resolution,
                struct_type->as.struct_type.name,
                struct_type->as.struct_type.name_length);
        }
        
        if (type_sym && type_sym->type &&
            type_sym->kind == CN_SEM_SYMBOL_STRUCT &&
            type_sym->type->as.struct_type.fields) {
            // 更新结构体类型的字段信息
            struct_type->as.struct_type.fields = type_sym->type->as.struct_type.fields;
            struct_type->as.struct_type.field_count = type_sym->type->as.struct_type.field_count;
            // 同时更新声明作用域
            struct_type->as.struct_type.decl_scope = type_sym->type->as.struct_type.decl_scope;
        } else {
            return NULL;
        }
    }

    // 遍历结构体的所有字段
    if (!struct_type->as.struct_type.fields) {
        return NULL;
    }
    for (size_t i = 0; i < struct_type->as.struct_type.field_count; i++) {
        CnStructField *field = &struct_type->as.struct_type.fields[i];
        if (field && field->name && field->name_length == field_name_length &&
            memcmp(field->name, field_name, field_name_length) == 0) {
            if (field->field_type && field->field_type->kind == CN_TYPE_POINTER &&
                field->field_type->as.pointer_to) {
                if (field->field_type->as.pointer_to->kind == CN_TYPE_STRUCT &&
                    field->field_type->as.pointer_to->as.struct_type.name) {
                    
                    // 动态解析：如果指针指向的结构体没有字段信息，尝试从声明作用域或全局作用域查找真实类型
                    CnType *pointee_type = field->field_type->as.pointer_to;
                    if (!pointee_type->as.struct_type.fields) {
                        CnSemSymbol *pointee_sym = NULL;
                        
                        // 首先尝试从声明作用域查找
                        if (pointee_type->as.struct_type.decl_scope) {
                            pointee_sym = cn_sem_scope_lookup(
                                pointee_type->as.struct_type.decl_scope,
                                pointee_type->as.struct_type.name,
                                pointee_type->as.struct_type.name_length);
                        }
                        
                        // 如果没找到，尝试从全局作用域查找
                        if (!pointee_sym && g_current_scope_for_resolution) {
                            pointee_sym = cn_sem_scope_lookup(
                                g_current_scope_for_resolution,
                                pointee_type->as.struct_type.name,
                                pointee_type->as.struct_type.name_length);
                        }
                        
                        if (pointee_sym && pointee_sym->type &&
                            pointee_sym->kind == CN_SEM_SYMBOL_STRUCT &&
                            pointee_sym->type->as.struct_type.fields) {
                            // 更新指针指向的类型
                            field->field_type->as.pointer_to = pointee_sym->type;
                        }
                    }
                }
            }
            return field;
        }
    }

    return NULL; // 未找到匹配的字段
}

// 枚举成员查找回调上下文
typedef struct {
    const char *member_name;
    size_t member_name_length;
    CnSemSymbol *found_sym;
} EnumMemberFindContext;

// 枚举成员查找回调函数
static void enum_member_find_callback(CnSemSymbol *symbol, void *user_data) {
    EnumMemberFindContext *ctx = (EnumMemberFindContext *)user_data;
    
    // 如果已经找到，直接返回
    if (ctx->found_sym) return;
    
    if (symbol && symbol->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
        // 检查成员名是否以 "_" + member_name 结尾
        size_t s_name_len = symbol->name_length;
        if (s_name_len > ctx->member_name_length + 1) {
            // 查找最后一个下划线的位置
            const char *last_underscore = NULL;
            for (size_t j = 0; j < s_name_len; j++) {
                if (symbol->name[j] == '_') {
                    last_underscore = symbol->name + j;
                }
            }
            if (last_underscore) {
                // 比较下划线后的部分
                size_t suffix_len = s_name_len - (last_underscore - symbol->name) - 1;
                if (suffix_len == ctx->member_name_length &&
                    memcmp(last_underscore + 1, ctx->member_name, ctx->member_name_length) == 0) {
                    ctx->found_sym = symbol;
                }
            }
        }
    }
}

// 在枚举类型中查找成员
// 支持多种查找方式：
// 1. 直接使用成员名查找（如 "错误"）
// 2. 使用带枚举类型名前缀的成员名查找（如 "诊断严重级别_错误"）
// 3. 遍历所有成员，查找以 "_" + 成员名 结尾的成员（如 "诊断_错误" 匹配 "错误"）
CnSemSymbol *cn_type_enum_find_member(CnType *enum_type,
                                      const char *member_name,
                                      size_t member_name_length) {
    if (!enum_type || enum_type->kind != CN_TYPE_ENUM || !member_name) {
        return NULL;
    }
    if (!enum_type->as.enum_type.enum_scope) {
        return NULL;
    }

    // 首先尝试直接查找成员名
    CnSemSymbol *sym = cn_sem_scope_lookup_shallow(enum_type->as.enum_type.enum_scope,
                                                   member_name,
                                                   member_name_length);
    if (sym) {
        return sym;
    }

    // 如果直接查找失败，尝试使用枚举类型名作为前缀查找
    // 例如：成员名 "错误" 尝试查找 "诊断严重级别_错误"
    const char *enum_name = enum_type->as.enum_type.name;
    size_t enum_name_len = enum_type->as.enum_type.name_length;
    
    if (enum_name && enum_name_len > 0) {
        // 构建带前缀的成员名：枚举类型名_成员名
        size_t prefixed_name_len = enum_name_len + 1 + member_name_length; // 枚举名 + "_" + 成员名
        char *prefixed_name = malloc(prefixed_name_len + 1);
        if (prefixed_name) {
            memcpy(prefixed_name, enum_name, enum_name_len);
            prefixed_name[enum_name_len] = '_';
            memcpy(prefixed_name + enum_name_len + 1, member_name, member_name_length);
            prefixed_name[prefixed_name_len] = '\0';
            
            sym = cn_sem_scope_lookup_shallow(enum_type->as.enum_type.enum_scope,
                                              prefixed_name,
                                              prefixed_name_len);
            free(prefixed_name);
            if (sym) {
                return sym;
            }
        }
    }

    // 如果仍然找不到，遍历枚举作用域中所有符号，查找以 "_" + 成员名 结尾的成员
    // 例如：成员名 "错误" 匹配 "诊断_错误" 或 "错误类型_错误"
    EnumMemberFindContext ctx = { member_name, member_name_length, NULL };
    cn_sem_scope_foreach_symbol(enum_type->as.enum_type.enum_scope,
                                enum_member_find_callback, &ctx);

    return ctx.found_sym;
}

// =============================================================================
// 类型推断辅助函数
// =============================================================================

/**
 * @brief 从表达式推断类型（简化版本，用于变量声明初始化）
 *
 * 支持的表达式类型：
 * - 字面量表达式（整数、浮点、字符串、布尔、字符）
 * - 强制类型转换表达式
 * - 函数调用表达式
 * - 成员访问表达式
 * - 标识符引用
 *
 * @param scope 当前作用域
 * @param expr 表达式节点
 * @return 推断出的类型，如果无法推断则返回 NULL
 */
CnType *cn_type_infer_from_expr(CnSemScope *scope, CnAstExpr *expr) {
    if (!expr || !scope) return NULL;
    
    // 如果表达式已经有类型信息，直接返回
    if (expr->type && expr->type->kind != CN_TYPE_UNKNOWN) {
        return expr->type;
    }
    
    switch (expr->kind) {
        case CN_AST_EXPR_INTEGER_LITERAL:
            return cn_type_new_primitive(CN_TYPE_INT);
        case CN_AST_EXPR_FLOAT_LITERAL:
            return cn_type_new_primitive(CN_TYPE_FLOAT);
        case CN_AST_EXPR_STRING_LITERAL:
            return cn_type_new_primitive(CN_TYPE_STRING);
        case CN_AST_EXPR_BOOL_LITERAL:
            return cn_type_new_primitive(CN_TYPE_BOOL);
        case CN_AST_EXPR_CHAR_LITERAL:
            return cn_type_new_primitive(CN_TYPE_CHAR);
        case CN_AST_EXPR_CAST:
            // 强制类型转换：直接使用目标类型
            return expr->as.cast.target_type;
        case CN_AST_EXPR_CALL:
            // 函数调用：返回函数返回类型
            return cn_type_infer_call_expr(scope, expr);
        case CN_AST_EXPR_MEMBER_ACCESS:
            // 成员访问：返回成员类型
            return cn_type_infer_member_access(scope, expr);
        case CN_AST_EXPR_IDENTIFIER:
            // 标识符引用：返回变量类型
            return cn_type_infer_identifier(scope, expr);
        default:
            return NULL;
    }
}

/**
 * @brief 推断函数调用表达式的返回类型
 *
 * @param scope 当前作用域
 * @param call_expr 函数调用表达式
 * @return 函数返回类型，如果无法推断则返回 NULL
 */
CnType *cn_type_infer_call_expr(CnSemScope *scope, CnAstExpr *call_expr) {
    if (!call_expr || call_expr->kind != CN_AST_EXPR_CALL) return NULL;
    if (!call_expr->as.call.callee) return NULL;
    
    // 只处理标识符形式的函数调用
    if (call_expr->as.call.callee->kind != CN_AST_EXPR_IDENTIFIER) return NULL;
    
    const char *func_name = call_expr->as.call.callee->as.identifier.name;
    size_t func_name_len = call_expr->as.call.callee->as.identifier.name_length;
    
    if (!func_name || func_name_len == 0) return NULL;
    
    // 在作用域中查找函数符号
    CnSemSymbol *func_sym = cn_sem_scope_lookup(scope, func_name, func_name_len);
    if (!func_sym || !func_sym->type) return NULL;
    
    // 检查是否为函数类型
    if (func_sym->type->kind == CN_TYPE_FUNCTION) {
        return func_sym->type->as.function.return_type;
    }
    
    // 检查是否为函数指针类型
    if (func_sym->type->kind == CN_TYPE_POINTER &&
        func_sym->type->as.pointer_to &&
        func_sym->type->as.pointer_to->kind == CN_TYPE_FUNCTION) {
        return func_sym->type->as.pointer_to->as.function.return_type;
    }
    
    return NULL;
}

/**
 * @brief 推断成员访问表达式的类型
 *
 * @param scope 当前作用域
 * @param member_expr 成员访问表达式
 * @return 成员类型，如果无法推断则返回 NULL
 */
CnType *cn_type_infer_member_access(CnSemScope *scope, CnAstExpr *member_expr) {
    if (!member_expr || member_expr->kind != CN_AST_EXPR_MEMBER_ACCESS) return NULL;
    if (!member_expr->as.member.object) return NULL;
    
    // 获取对象类型
    CnType *object_type = cn_type_infer_from_expr(scope, member_expr->as.member.object);
    if (!object_type) return NULL;
    
    // 处理指针类型（自动解引用）
    if (object_type->kind == CN_TYPE_POINTER) {
        object_type = object_type->as.pointer_to;
        if (!object_type) return NULL;
    }
    
    // 只处理结构体类型
    if (object_type->kind != CN_TYPE_STRUCT) return NULL;
    
    // 在结构体中查找成员
    CnStructField *field = cn_type_struct_find_field(
        object_type,
        member_expr->as.member.member_name,
        member_expr->as.member.member_name_length);
    
    return field ? field->field_type : NULL;
}

/**
 * @brief 推断标识符表达式的类型
 *
 * @param scope 当前作用域
 * @param ident_expr 标识符表达式
 * @return 变量类型，如果无法推断则返回 NULL
 */
CnType *cn_type_infer_identifier(CnSemScope *scope, CnAstExpr *ident_expr) {
    if (!ident_expr || ident_expr->kind != CN_AST_EXPR_IDENTIFIER) return NULL;
    
    const char *name = ident_expr->as.identifier.name;
    size_t name_len = ident_expr->as.identifier.name_length;
    
    if (!name || name_len == 0) return NULL;
    
    // 在作用域中查找符号
    CnSemSymbol *sym = cn_sem_scope_lookup(scope, name, name_len);
    
    return sym ? sym->type : NULL;
}

// ============================================================================
// 类型深度复制：用于模块导入时复制完整的类型信息
// ============================================================================

// 递归类型检测：使用线程局部存储记录正在复制的类型
// 避免递归类型定义（如结构体包含指向自身的指针）导致无限递归

#define MAX_RECURSION_DEPTH 64

// 线程局部存储：记录正在复制的结构体类型
static _Thread_local const CnType *g_copying_types[MAX_RECURSION_DEPTH];
static _Thread_local size_t g_copying_depth = 0;

/**
 * @brief 检查类型是否正在被复制（递归检测）
 *
 * @param type 要检查的类型
 * @return true 如果类型正在被复制，false 否则
 */
static bool is_type_being_copied(const CnType *type) {
    for (size_t i = 0; i < g_copying_depth; i++) {
        if (g_copying_types[i] == type) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 深度复制结构体字段数组
 *
 * @param src_fields 源字段数组
 * @param field_count 字段数量
 * @param current_struct 当前正在复制的结构体类型（用于递归检测）
 * @return 新分配的字段数组，失败返回 NULL
 */
static CnStructField *cn_type_deep_copy_fields_ex(CnStructField *src_fields, size_t field_count, const CnType *current_struct) {
    if (!src_fields || field_count == 0) {
        return NULL;
    }
    
    CnStructField *dst_fields = (CnStructField *)malloc(sizeof(CnStructField) * field_count);
    if (!dst_fields) {
        return NULL;
    }
    
    for (size_t i = 0; i < field_count; i++) {
        // 复制字段基本信息
        dst_fields[i].name = src_fields[i].name;
        dst_fields[i].name_length = src_fields[i].name_length;
        dst_fields[i].is_const = src_fields[i].is_const;
        
        // 检查字段类型是否是递归引用（指向当前结构体的指针）
        CnType *field_type = src_fields[i].field_type;
        if (field_type && field_type->kind == CN_TYPE_POINTER &&
            field_type->as.pointer_to &&
            field_type->as.pointer_to->kind == CN_TYPE_STRUCT) {
            
            // 检查是否指向当前结构体（递归引用）
            CnType *pointee = field_type->as.pointer_to;
            if (current_struct &&
                pointee->as.struct_type.name && current_struct->as.struct_type.name &&
                pointee->as.struct_type.name_length == current_struct->as.struct_type.name_length &&
                memcmp(pointee->as.struct_type.name, current_struct->as.struct_type.name,
                       pointee->as.struct_type.name_length) == 0) {
                // 递归引用：创建指向新结构体的指针
                // 注意：这里需要创建一个指向新结构体的指针，但新结构体还未完成
                // 我们先创建一个浅拷贝的指针，后续会更新
                dst_fields[i].field_type = cn_type_new_pointer(NULL);  // 暂时设为NULL
                continue;
            }
        }
        
        // 深度复制字段类型
        dst_fields[i].field_type = cn_type_deep_copy(field_type);
    }
    
    return dst_fields;
}

// 保留旧函数签名以兼容
static CnStructField *cn_type_deep_copy_fields(CnStructField *src_fields, size_t field_count) {
    return cn_type_deep_copy_fields_ex(src_fields, field_count, NULL);
}

/**
 * @brief 深度复制函数参数类型数组
 *
 * @param src_params 源参数类型数组
 * @param param_count 参数数量
 * @return 新分配的参数类型数组，失败返回 NULL
 */
static CnType **cn_type_deep_copy_param_types(CnType **src_params, size_t param_count) {
    if (!src_params || param_count == 0) {
        return NULL;
    }
    
    CnType **dst_params = (CnType **)malloc(sizeof(CnType *) * param_count);
    if (!dst_params) {
        return NULL;
    }
    
    for (size_t i = 0; i < param_count; i++) {
        dst_params[i] = cn_type_deep_copy(src_params[i]);
    }
    
    return dst_params;
}

/**
 * @brief 深度复制类型信息
 *
 * 用于模块导入时复制完整的类型信息，确保导入的类型信息不会因为
 * 原模块被重新编译或跨编译会话而丢失。
 *
 * @param src 源类型
 * @return 新分配的类型对象，失败返回 NULL
 */
CnType *cn_type_deep_copy(CnType *src) {
    if (!src) {
        return NULL;
    }
    
    switch (src->kind) {
        case CN_TYPE_UNKNOWN:
        case CN_TYPE_VOID:
        case CN_TYPE_INT:
        case CN_TYPE_INT32:
        case CN_TYPE_INT64:
        case CN_TYPE_UINT32:
        case CN_TYPE_UINT64:
        case CN_TYPE_UINT64_LL:
        case CN_TYPE_FLOAT:
        case CN_TYPE_FLOAT32:
        case CN_TYPE_FLOAT64:
        case CN_TYPE_CHAR:
        case CN_TYPE_STRING:
        case CN_TYPE_BOOL:
        case CN_TYPE_MEMORY_ADDRESS:
            // 基本类型：直接创建新对象
            return cn_type_new_primitive(src->kind);
            
        case CN_TYPE_POINTER: {
            // 指针类型：递归复制指向的类型
            CnType *base_copy = cn_type_deep_copy(src->as.pointer_to);
            return cn_type_new_pointer(base_copy);
        }
        
        case CN_TYPE_ARRAY: {
            // 数组类型：递归复制元素类型
            CnType *element_copy = cn_type_deep_copy(src->as.array.element_type);
            return cn_type_new_array(element_copy, src->as.array.length);
        }
        
        case CN_TYPE_STRUCT: {
            // 结构体类型：深度复制字段信息
            // 【关键修复】检测递归类型定义，避免无限递归
            
            // 检查是否正在复制此类型（递归引用）
            if (is_type_being_copied(src)) {
                // 递归引用：返回一个浅拷贝的结构体类型
                // 这避免了无限递归，同时保留了类型名称信息
                return cn_type_new_struct(
                    src->as.struct_type.name,
                    src->as.struct_type.name_length,
                    NULL,  // 字段暂时为空，后续可以动态解析
                    0,
                    src->as.struct_type.decl_scope,
                    src->as.struct_type.owner_func_name,
                    src->as.struct_type.owner_func_name_length);
            }
            
            // 检查递归深度
            if (g_copying_depth >= MAX_RECURSION_DEPTH) {
                fprintf(stderr, "[WARNING] cn_type_deep_copy: 递归深度超过限制\n");
                return NULL;
            }
            
            // 记录正在复制的类型
            g_copying_types[g_copying_depth++] = src;
            
            CnStructField *fields_copy = cn_type_deep_copy_fields_ex(
                src->as.struct_type.fields,
                src->as.struct_type.field_count,
                src);  // 传入当前结构体类型用于递归检测
            
            CnType *dst = cn_type_new_struct(
                src->as.struct_type.name,
                src->as.struct_type.name_length,
                fields_copy,
                src->as.struct_type.field_count,
                src->as.struct_type.decl_scope,
                src->as.struct_type.owner_func_name,
                src->as.struct_type.owner_func_name_length);
            
            // 更新递归指针字段：将NULL指针更新为指向新结构体
            if (dst && fields_copy) {
                for (size_t i = 0; i < src->as.struct_type.field_count; i++) {
                    CnType *field_type = src->as.struct_type.fields[i].field_type;
                    if (field_type && field_type->kind == CN_TYPE_POINTER &&
                        field_type->as.pointer_to &&
                        field_type->as.pointer_to->kind == CN_TYPE_STRUCT) {
                        
                        CnType *pointee = field_type->as.pointer_to;
                        if (pointee->as.struct_type.name && src->as.struct_type.name &&
                            pointee->as.struct_type.name_length == src->as.struct_type.name_length &&
                            memcmp(pointee->as.struct_type.name, src->as.struct_type.name,
                                   pointee->as.struct_type.name_length) == 0) {
                            // 这是递归指针字段，更新为指向新结构体
                            if (dst->as.struct_type.fields[i].field_type &&
                                dst->as.struct_type.fields[i].field_type->kind == CN_TYPE_POINTER) {
                                dst->as.struct_type.fields[i].field_type->as.pointer_to = dst;
                            }
                        }
                    }
                }
            }
            
            // 弹出递归栈
            g_copying_depth--;
            
            return dst;
        }
        
        case CN_TYPE_ENUM: {
            // 枚举类型：深度复制名称和枚举作用域
            CnType *dst = cn_type_new_enum(
                src->as.enum_type.name,
                src->as.enum_type.name_length);
            
            if (dst) {
                // 【关键修复】深度复制枚举作用域，避免跨模块访问时野指针崩溃
                // 原来只是复制指针，当原模块被重新编译时，枚举作用域可能被释放
                dst->as.enum_type.enum_scope = cn_sem_scope_deep_copy_enum(src->as.enum_type.enum_scope);
            }
            
            return dst;
        }
        
        case CN_TYPE_FUNCTION: {
            // 函数类型：深度复制返回类型和参数类型
            CnType *return_copy = cn_type_deep_copy(src->as.function.return_type);
            CnType **params_copy = cn_type_deep_copy_param_types(
                src->as.function.param_types,
                src->as.function.param_count);
            
            return cn_type_new_function(return_copy, params_copy, src->as.function.param_count);
        }
        
        default:
            // 未知类型：返回 NULL
            fprintf(stderr, "[WARNING] cn_type_deep_copy: unknown type kind %d\n", src->kind);
            return NULL;
    }
}
