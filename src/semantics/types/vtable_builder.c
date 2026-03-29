/**
 * @file vtable_builder.c
 * @brief CN语言面向对象编程 - 虚函数表构建器实现
 *
 * 本文件实现了CN语言虚函数表构建器的功能，包括：
 * - vtable创建和销毁
 * - 条目添加和查找
 * - 继承合并
 * - 抽象类检测
 */

#include "cnlang/semantics/vtable_builder.h"
#include "cnlang/semantics/inheritance_resolver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * 内部常量定义
 * ============================================================================ */

/** 初始vtable容量 */
#define VTABLE_INITIAL_CAPACITY 8

/** 初始条目容量 */
#define ENTRY_INITIAL_CAPACITY 4

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 安全的字符串比较（带长度）
 */
static bool str_equals_len(const char *s1, size_t len1, const char *s2, size_t len2) {
    if (len1 != len2) return false;
    if (s1 == NULL || s2 == NULL) return s1 == s2;
    return memcmp(s1, s2, len1) == 0;
}

/**
 * @brief 复制字符串（带长度）
 */
static char *str_dup_len(const char *str, size_t len) {
    if (!str || len == 0) return NULL;
    char *result = (char *)malloc(len + 1);
    if (result) {
        memcpy(result, str, len);
        result[len] = '\0';
    }
    return result;
}

/* ============================================================================
 * 虚函数表创建和销毁
 * ============================================================================ */

CnVTable *cn_vtable_create(const char *class_name, size_t class_name_length) {
    if (!class_name || class_name_length == 0) return NULL;

    CnVTable *vtable = (CnVTable *)calloc(1, sizeof(CnVTable));
    if (!vtable) return NULL;

    // 设置类名
    vtable->class_name = str_dup_len(class_name, class_name_length);
    vtable->class_name_length = class_name_length;
    if (!vtable->class_name) {
        free(vtable);
        return NULL;
    }

    // 分配条目数组
    vtable->entries = (CnVTableEntry *)calloc(ENTRY_INITIAL_CAPACITY, sizeof(CnVTableEntry));
    if (!vtable->entries) {
        free((void *)vtable->class_name);
        free(vtable);
        return NULL;
    }

    vtable->entry_count = 0;
    vtable->capacity = ENTRY_INITIAL_CAPACITY;
    vtable->base_vtable = NULL;
    vtable->is_complete = true;  // 初始为完整，添加纯虚函数后变为不完整
    
    /* 多继承支持初始化 */
    vtable->secondary_vtables = NULL;
    vtable->secondary_vtable_count = 0;

    return vtable;
}

void cn_vtable_destroy(CnVTable *vtable) {
    if (!vtable) return;

    // 释放所有条目中的字符串
    for (size_t i = 0; i < vtable->entry_count; i++) {
        CnVTableEntry *entry = &vtable->entries[i];
        if (entry->method_name) {
            free((void *)entry->method_name);
        }
        if (entry->defined_in_class) {
            free((void *)entry->defined_in_class);
        }
    }

    // 释放条目数组
    free(vtable->entries);
    
    // 释放次要基类vtable数组（不销毁vtable本身，它们由builder管理）
    if (vtable->secondary_vtables) {
        free(vtable->secondary_vtables);
    }

    // 释放类名
    free((void *)vtable->class_name);

    // 释放vtable本身
    free(vtable);
}

/* ============================================================================
 * 虚函数表条目操作
 * ============================================================================ */

bool cn_vtable_add_entry(CnVTable *vtable, CnClassMember *method) {
    if (!vtable || !method || !method->name) return false;

    // 只有虚函数才能添加到vtable
    if (!method->is_virtual) return false;

    return cn_vtable_add_entry_ex(vtable, method, 
                                   vtable->class_name, 
                                   vtable->class_name_length);
}

bool cn_vtable_add_entry_ex(CnVTable *vtable, CnClassMember *method,
                            const char *defined_in_class, size_t defined_in_class_len) {
    if (!vtable || !method || !method->name) return false;

    // 只有虚函数才能添加到vtable
    if (!method->is_virtual) return false;

    // 检查是否已存在同名方法
    int existing_idx = cn_vtable_find_entry(vtable, method->name, method->name_length);
    if (existing_idx >= 0) {
        // 已存在，更新条目（重写情况）
        CnVTableEntry *entry = &vtable->entries[existing_idx];
        entry->method = method;
        entry->is_pure_virtual = method->is_pure_virtual;
        entry->is_override = true;
        if (entry->method_name) {
            free((void *)entry->method_name);
        }
        entry->method_name = str_dup_len(method->name, method->name_length);
        if (entry->defined_in_class) {
            free((void *)entry->defined_in_class);
        }
        entry->defined_in_class = str_dup_len(defined_in_class, defined_in_class_len);
        entry->defined_in_class_len = defined_in_class_len;

        // 更新完整性
        if (method->is_pure_virtual) {
            vtable->is_complete = false;
        }
        return true;
    }

    // 检查是否需要扩容
    if (vtable->entry_count >= vtable->capacity) {
        size_t new_capacity = vtable->capacity * 2;
        CnVTableEntry *new_entries = (CnVTableEntry *)realloc(vtable->entries,
                                                               new_capacity * sizeof(CnVTableEntry));
        if (!new_entries) return false;

        // 初始化新空间
        memset(new_entries + vtable->capacity, 0,
               (new_capacity - vtable->capacity) * sizeof(CnVTableEntry));

        vtable->entries = new_entries;
        vtable->capacity = new_capacity;
    }

    // 添加新条目
    CnVTableEntry *entry = &vtable->entries[vtable->entry_count];
    entry->method_name = str_dup_len(method->name, method->name_length);
    entry->method_name_length = method->name_length;
    entry->method = method;
    entry->offset = vtable->entry_count;
    entry->is_pure_virtual = method->is_pure_virtual;
    entry->is_override = false;
    entry->defined_in_class = str_dup_len(defined_in_class, defined_in_class_len);
    entry->defined_in_class_len = defined_in_class_len;

    // 更新完整性
    if (method->is_pure_virtual) {
        vtable->is_complete = false;
    }

    vtable->entry_count++;
    return true;
}

int cn_vtable_find_entry(CnVTable *vtable, const char *method_name, 
                         size_t method_name_length) {
    if (!vtable || !method_name || method_name_length == 0) return -1;

    for (size_t i = 0; i < vtable->entry_count; i++) {
        if (str_equals_len(vtable->entries[i].method_name, 
                           vtable->entries[i].method_name_length,
                           method_name, method_name_length)) {
            return (int)i;
        }
    }

    return -1;
}

/* ============================================================================
 * 虚函数表合并
 * ============================================================================ */

bool cn_vtable_merge_base(CnVTable *derived, CnVTable *base) {
    if (!derived || !base) return false;

    // 设置基类vtable引用
    derived->base_vtable = base;

    // 遍历基类vtable的所有条目
    for (size_t i = 0; i < base->entry_count; i++) {
        CnVTableEntry *base_entry = &base->entries[i];

        // 检查派生类是否已重写该方法
        int derived_idx = cn_vtable_find_entry(derived, 
                                                base_entry->method_name,
                                                base_entry->method_name_length);

        if (derived_idx < 0) {
            // 派生类未重写，添加基类的条目
            // 需要保持相同的偏移
            if (derived->entry_count != base_entry->offset) {
                // 偏移不匹配，需要插入空位
                // 这里简化处理，假设顺序一致
            }

            // 添加基类方法条目
            CnVTableEntry *new_entry = &derived->entries[derived->entry_count];
            new_entry->method_name = str_dup_len(base_entry->method_name, 
                                                  base_entry->method_name_length);
            new_entry->method_name_length = base_entry->method_name_length;
            new_entry->method = base_entry->method;
            new_entry->offset = derived->entry_count;
            new_entry->is_pure_virtual = base_entry->is_pure_virtual;
            new_entry->is_override = false;
            new_entry->defined_in_class = str_dup_len(base_entry->defined_in_class,
                                                       base_entry->defined_in_class_len);
            new_entry->defined_in_class_len = base_entry->defined_in_class_len;

            derived->entry_count++;

            // 更新完整性
            if (base_entry->is_pure_virtual) {
                derived->is_complete = false;
            }
        }
        // 如果派生类已重写，则保留派生类的版本（已在vtable中）
    }

    return true;
}

/* ============================================================================
 * 虚函数表构建器
 * ============================================================================ */

CnVTableBuilder *cn_vtable_builder_create(CnDiagnostics *diag) {
    CnVTableBuilder *builder = (CnVTableBuilder *)calloc(1, sizeof(CnVTableBuilder));
    if (!builder) return NULL;

    builder->vtables = (CnVTable *)calloc(VTABLE_INITIAL_CAPACITY, sizeof(CnVTable));
    if (!builder->vtables) {
        free(builder);
        return NULL;
    }

    builder->vtable_count = 0;
    builder->vtable_capacity = VTABLE_INITIAL_CAPACITY;
    builder->diagnostics = diag;

    return builder;
}

void cn_vtable_builder_destroy(CnVTableBuilder *builder) {
    if (!builder) return;

    // 销毁所有vtable
    for (size_t i = 0; i < builder->vtable_count; i++) {
        cn_vtable_destroy(&builder->vtables[i]);
    }

    // 释放vtable数组
    free(builder->vtables);

    // 释放构建器
    free(builder);
}

CnVTable *cn_vtable_build_for_class(CnVTableBuilder *builder,
                                     CnAstClassDecl *class_decl) {
    return cn_vtable_build_for_class_ex(builder, class_decl, NULL);
}

CnVTable *cn_vtable_build_for_class_ex(CnVTableBuilder *builder,
                                        CnAstClassDecl *class_decl,
                                        CnInheritanceResolver *inheritance_resolver) {
    if (!builder || !class_decl) return NULL;

    // 检查是否已存在
    CnVTable *existing = cn_vtable_builder_get_vtable(builder,
                                                       class_decl->name,
                                                       class_decl->name_length);
    if (existing) return existing;

    // 检查是否需要vtable
    if (!cn_vtable_needs_vtable(class_decl)) {
        return NULL;
    }

    // 创建新的vtable
    CnVTable *vtable = cn_vtable_create(class_decl->name, class_decl->name_length);
    if (!vtable) return NULL;

    // 如果有基类，先合并基类的vtable
    if (class_decl->base_count > 0 && inheritance_resolver) {
        for (size_t i = 0; i < class_decl->base_count; i++) {
            CnInheritanceInfo *base_info = &class_decl->bases[i];

            // 查找基类的vtable
            CnVTable *base_vtable = cn_vtable_builder_get_vtable(builder,
                                                                  base_info->base_class_name,
                                                                  base_info->base_class_name_length);
            if (base_vtable) {
                // 合并基类vtable
                cn_vtable_merge_base(vtable, base_vtable);
            }
        }
    }

    // 添加当前类的虚函数
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD && member->is_virtual) {
            cn_vtable_add_entry_ex(vtable, member,
                                    class_decl->name, class_decl->name_length);
        }
    }

    // 检查是否需要扩容
    if (builder->vtable_count >= builder->vtable_capacity) {
        size_t new_capacity = builder->vtable_capacity * 2;
        CnVTable *new_vtables = (CnVTable *)realloc(builder->vtables,
                                                     new_capacity * sizeof(CnVTable));
        if (!new_vtables) {
            cn_vtable_destroy(vtable);
            return NULL;
        }
        builder->vtables = new_vtables;
        builder->vtable_capacity = new_capacity;
    }

    // 添加到构建器
    builder->vtables[builder->vtable_count] = *vtable;
    builder->vtable_count++;

    // 释放vtable结构体（内容已复制到数组中）
    free(vtable);

    return &builder->vtables[builder->vtable_count - 1];
}

/* ============================================================================
 * 查询函数
 * ============================================================================ */

CnVTable *cn_vtable_builder_get_vtable(CnVTableBuilder *builder,
                                        const char *class_name,
                                        size_t class_name_length) {
    if (!builder || !class_name || class_name_length == 0) return NULL;

    for (size_t i = 0; i < builder->vtable_count; i++) {
        if (str_equals_len(builder->vtables[i].class_name,
                           builder->vtables[i].class_name_length,
                           class_name, class_name_length)) {
            return &builder->vtables[i];
        }
    }

    return NULL;
}

bool cn_vtable_needs_vtable(CnAstClassDecl *class_decl) {
    if (!class_decl) return false;

    // 检查是否有虚函数
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD && member->is_virtual) {
            return true;
        }
    }

    // 如果有基类，可能需要vtable（简化处理，假设需要）
    // 完整实现需要检查基类链
    return class_decl->base_count > 0;
}

bool cn_vtable_is_abstract(CnVTable *vtable) {
    if (!vtable) return false;
    return !vtable->is_complete;
}

size_t cn_vtable_pure_virtual_count(CnVTable *vtable) {
    if (!vtable) return 0;

    size_t count = 0;
    for (size_t i = 0; i < vtable->entry_count; i++) {
        if (vtable->entries[i].is_pure_virtual) {
            count++;
        }
    }
    return count;
}

void cn_vtable_print(CnVTable *vtable, FILE *out) {
    if (!vtable || !out) return;

    fprintf(out, "虚函数表 [%.*s]:\n",
            (int)vtable->class_name_length, vtable->class_name);
    fprintf(out, "  条目数量: %zu\n", vtable->entry_count);
    fprintf(out, "  完整性: %s\n", vtable->is_complete ? "完整" : "不完整（抽象类）");

    if (vtable->base_vtable) {
        fprintf(out, "  基类vtable: [%.*s]\n",
                (int)vtable->base_vtable->class_name_length,
                vtable->base_vtable->class_name);
    }

    fprintf(out, "  条目列表:\n");
    for (size_t i = 0; i < vtable->entry_count; i++) {
        CnVTableEntry *entry = &vtable->entries[i];
        fprintf(out, "    [%zu] %.*s (定义于 %.*s)%s%s\n",
                entry->offset,
                (int)entry->method_name_length, entry->method_name,
                (int)entry->defined_in_class_len, entry->defined_in_class,
                entry->is_pure_virtual ? " [纯虚]" : "",
                entry->is_override ? " [重写]" : "");
    }
    
    /* 打印次要基类vtable信息 */
    if (vtable->secondary_vtable_count > 0) {
        fprintf(out, "  次要基类vtable数量: %zu\n", vtable->secondary_vtable_count);
        for (size_t i = 0; i < vtable->secondary_vtable_count; i++) {
            CnVTable *secondary = vtable->secondary_vtables[i];
            if (secondary) {
                fprintf(out, "    [%zu] %.*s\n", i,
                        (int)secondary->class_name_length, secondary->class_name);
            }
        }
    }
}

/* ============================================================================
 * 多基类vtable合并函数
 * ============================================================================ */

bool cn_vtable_detect_method_conflict(CnVTable **base_vtables,
                                       size_t base_count,
                                       const char **conflict_name,
                                       size_t *conflict_len) {
    if (!base_vtables || base_count <= 1) return false;
    
    /* 遍历所有基类对，检查方法名冲突 */
    for (size_t i = 0; i < base_count; i++) {
        CnVTable *vtable_i = base_vtables[i];
        if (!vtable_i) continue;
        
        for (size_t j = i + 1; j < base_count; j++) {
            CnVTable *vtable_j = base_vtables[j];
            if (!vtable_j) continue;
            
            /* 检查两个vtable中是否有同名方法 */
            for (size_t ki = 0; ki < vtable_i->entry_count; ki++) {
                CnVTableEntry *entry_i = &vtable_i->entries[ki];
                
                for (size_t kj = 0; kj < vtable_j->entry_count; kj++) {
                    CnVTableEntry *entry_j = &vtable_j->entries[kj];
                    
                    /* 比较方法名 */
                    if (str_equals_len(entry_i->method_name, entry_i->method_name_length,
                                        entry_j->method_name, entry_j->method_name_length)) {
                        /* 发现冲突 */
                        if (conflict_name) {
                            *conflict_name = entry_i->method_name;
                        }
                        if (conflict_len) {
                            *conflict_len = entry_i->method_name_length;
                        }
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

bool cn_vtable_merge_multiple_bases(CnVTable *derived,
                                     CnVTable **base_vtables,
                                     size_t base_count) {
    if (!derived || !base_vtables || base_count == 0) return false;
    
    /* 第一个基类作为主基类 */
    if (base_vtables[0]) {
        derived->base_vtable = base_vtables[0];
        
        /* 合并第一个基类的所有条目 */
        for (size_t i = 0; i < base_vtables[0]->entry_count; i++) {
            CnVTableEntry *base_entry = &base_vtables[0]->entries[i];
            
            /* 检查派生类是否已重写 */
            int derived_idx = cn_vtable_find_entry(derived,
                                                    base_entry->method_name,
                                                    base_entry->method_name_length);
            
            if (derived_idx < 0) {
                /* 未重写，添加基类条目 */
                CnVTableEntry new_entry;
                new_entry.method_name = str_dup_len(base_entry->method_name,
                                                     base_entry->method_name_length);
                new_entry.method_name_length = base_entry->method_name_length;
                new_entry.method = base_entry->method;
                new_entry.offset = derived->entry_count;
                new_entry.is_pure_virtual = base_entry->is_pure_virtual;
                new_entry.is_override = false;
                new_entry.defined_in_class = str_dup_len(base_entry->defined_in_class,
                                                          base_entry->defined_in_class_len);
                new_entry.defined_in_class_len = base_entry->defined_in_class_len;
                
                /* 添加到派生类vtable */
                if (derived->entry_count >= derived->capacity) {
                    size_t new_capacity = derived->capacity * 2;
                    CnVTableEntry *new_entries = realloc(derived->entries,
                                                          new_capacity * sizeof(CnVTableEntry));
                    if (!new_entries) {
                        if (new_entry.method_name) free((void *)new_entry.method_name);
                        if (new_entry.defined_in_class) free((void *)new_entry.defined_in_class);
                        return false;
                    }
                    derived->entries = new_entries;
                    derived->capacity = new_capacity;
                }
                
                derived->entries[derived->entry_count++] = new_entry;
                
                if (base_entry->is_pure_virtual) {
                    derived->is_complete = false;
                }
            }
        }
    }
    
    /* 处理次要基类（多继承） */
    if (base_count > 1) {
        /* 分配次要基类vtable数组 */
        derived->secondary_vtables = calloc(base_count - 1, sizeof(CnVTable*));
        if (!derived->secondary_vtables) return false;
        
        derived->secondary_vtable_count = base_count - 1;
        
        for (size_t i = 1; i < base_count; i++) {
            CnVTable *secondary = base_vtables[i];
            if (secondary) {
                derived->secondary_vtables[i - 1] = secondary;
                
                /* 检查次要基类的纯虚函数是否被派生类实现 */
                for (size_t j = 0; j < secondary->entry_count; j++) {
                    CnVTableEntry *entry = &secondary->entries[j];
                    if (entry->is_pure_virtual) {
                        /* 检查派生类是否实现了该方法 */
                        int impl_idx = cn_vtable_find_entry(derived,
                                                             entry->method_name,
                                                             entry->method_name_length);
                        if (impl_idx < 0 || derived->entries[impl_idx].is_pure_virtual) {
                            derived->is_complete = false;
                        }
                    }
                }
            }
        }
    }
    
    return true;
}

int cn_vtable_find_entry_multi(CnVTable *vtable,
                                const char *method_name,
                                size_t method_name_length,
                                size_t *base_index) {
    if (!vtable || !method_name || method_name_length == 0) return -1;
    
    /* 首先在主vtable中查找 */
    int idx = cn_vtable_find_entry(vtable, method_name, method_name_length);
    if (idx >= 0) {
        if (base_index) *base_index = 0;
        return idx;
    }
    
    /* 在次要基类vtable中查找 */
    for (size_t i = 0; i < vtable->secondary_vtable_count; i++) {
        CnVTable *secondary = vtable->secondary_vtables[i];
        if (secondary) {
            idx = cn_vtable_find_entry(secondary, method_name, method_name_length);
            if (idx >= 0) {
                if (base_index) *base_index = i + 1;
                return idx;
            }
        }
    }
    
    return -1;
}
