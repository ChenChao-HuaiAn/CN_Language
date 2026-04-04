/**
 * @file template_cache.c
 * @brief 模板实例化缓存实现
 *
 * 实现类型映射表、模板实例化缓存和模板注册表的核心功能。
 */

#include "cnlang/semantics/template.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * 类型映射表实现
 * ============================================================================ */

/**
 * @brief 创建类型映射表
 */
CnTypeMap *cn_type_map_new(void) {
    CnTypeMap *map = (CnTypeMap *)malloc(sizeof(CnTypeMap));
    if (!map) return NULL;
    
    map->entries = NULL;
    map->entry_count = 0;
    map->capacity = 0;
    
    return map;
}

/**
 * @brief 释放类型映射表
 */
void cn_type_map_free(CnTypeMap *map) {
    if (!map) return;
    
    // 释放所有映射项（但不释放类型本身，类型由其他系统管理）
    if (map->entries) {
        free(map->entries);
    }
    free(map);
}

/**
 * @brief 向映射表中添加类型映射
 */
bool cn_type_map_insert(CnTypeMap *map, 
                        const char *param_name, 
                        size_t param_name_len,
                        CnType *concrete_type) {
    if (!map || !param_name || param_name_len == 0) return false;
    
    // 检查是否已存在
    for (size_t i = 0; i < map->entry_count; i++) {
        CnTypeMapEntry *entry = &map->entries[i];
        if (entry->param_name_length == param_name_len &&
            memcmp(entry->param_name, param_name, param_name_len) == 0) {
            // 已存在，更新类型
            entry->concrete_type = concrete_type;
            return true;
        }
    }
    
    // 需要扩容
    if (map->entry_count >= map->capacity) {
        size_t new_capacity = map->capacity == 0 ? 8 : map->capacity * 2;
        CnTypeMapEntry *new_entries = (CnTypeMapEntry *)realloc(
            map->entries, new_capacity * sizeof(CnTypeMapEntry));
        if (!new_entries) return false;
        
        map->entries = new_entries;
        map->capacity = new_capacity;
    }
    
    // 添加新映射项
    CnTypeMapEntry *entry = &map->entries[map->entry_count++];
    entry->param_name = param_name;
    entry->param_name_length = param_name_len;
    entry->concrete_type = concrete_type;
    
    return true;
}

/**
 * @brief 在映射表中查找类型
 */
CnType *cn_type_map_lookup(const CnTypeMap *map,
                           const char *param_name,
                           size_t param_name_len) {
    if (!map || !param_name) return NULL;
    
    for (size_t i = 0; i < map->entry_count; i++) {
        const CnTypeMapEntry *entry = &map->entries[i];
        if (entry->param_name_length == param_name_len &&
            memcmp(entry->param_name, param_name, param_name_len) == 0) {
            return entry->concrete_type;
        }
    }
    
    return NULL;
}

/* ============================================================================
 * 模板实例化缓存实现
 * ============================================================================ */

/**
 * @brief 创建模板实例化缓存
 */
CnTemplateCache *cn_template_cache_new(void) {
    CnTemplateCache *cache = (CnTemplateCache *)malloc(sizeof(CnTemplateCache));
    if (!cache) return NULL;
    
    cache->instances = NULL;
    cache->instance_count = 0;
    cache->capacity = 0;
    
    return cache;
}

/**
 * @brief 释放单个实例化项
 */
static void cn_template_instance_free(CnTemplateInstance *instance) {
    if (!instance) return;
    
    // 释放修饰名称
    if (instance->mangled_name) {
        free(instance->mangled_name);
    }
    
    // 释放类型实参数组（不释放类型本身）
    if (instance->type_args) {
        free(instance->type_args);
    }
    
    // 注意：不释放 instantiated_function 和 instantiated_struct
    // 因为它们可能被 AST 引用
    
    free(instance);
}

/**
 * @brief 释放模板实例化缓存
 */
void cn_template_cache_free(CnTemplateCache *cache) {
    if (!cache) return;
    
    // 释放所有实例化项
    if (cache->instances) {
        for (size_t i = 0; i < cache->instance_count; i++) {
            cn_template_instance_free(cache->instances[i]);
        }
        free(cache->instances);
    }
    
    free(cache);
}

/**
 * @brief 比较两个类型数组是否相等
 */
bool cn_type_array_equals(CnType **types1, size_t count1,
                          CnType **types2, size_t count2) {
    if (count1 != count2) return false;
    if (count1 == 0) return true;  // 两个空数组相等
    if (!types1 || !types2) return false;
    
    for (size_t i = 0; i < count1; i++) {
        if (!cn_type_equals(types1[i], types2[i])) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 在缓存中查找已实例化的模板
 */
CnTemplateInstance *cn_template_cache_find(const CnTemplateCache *cache,
                                           const char *template_name,
                                           size_t template_name_len,
                                           CnType **type_args,
                                           size_t type_arg_count) {
    if (!cache || !template_name) return NULL;
    
    for (size_t i = 0; i < cache->instance_count; i++) {
        CnTemplateInstance *instance = cache->instances[i];
        
        // 比较模板名称
        if (instance->template_name_length != template_name_len) continue;
        if (memcmp(instance->template_name, template_name, template_name_len) != 0) {
            continue;
        }
        
        // 比较类型实参
        if (cn_type_array_equals(instance->type_args, instance->type_arg_count,
                                  type_args, type_arg_count)) {
            return instance;
        }
    }
    
    return NULL;
}

/**
 * @brief 向缓存中添加新实例化
 */
bool cn_template_cache_add(CnTemplateCache *cache, CnTemplateInstance *instance) {
    if (!cache || !instance) return false;
    
    // 检查是否已存在（避免重复添加）
    CnTemplateInstance *existing = cn_template_cache_find(
        cache,
        instance->template_name,
        instance->template_name_length,
        instance->type_args,
        instance->type_arg_count);
    
    if (existing) {
        // 已存在，不重复添加
        return true;
    }
    
    // 需要扩容
    if (cache->instance_count >= cache->capacity) {
        size_t new_capacity = cache->capacity == 0 ? 16 : cache->capacity * 2;
        CnTemplateInstance **new_instances = (CnTemplateInstance **)realloc(
            cache->instances, new_capacity * sizeof(CnTemplateInstance *));
        if (!new_instances) return false;
        
        cache->instances = new_instances;
        cache->capacity = new_capacity;
    }
    
    // 添加实例
    cache->instances[cache->instance_count++] = instance;
    
    return true;
}

/* ============================================================================
 * 模板注册表实现
 * ============================================================================ */

/**
 * @brief 创建模板注册表
 */
CnTemplateRegistry *cn_template_registry_new(void) {
    CnTemplateRegistry *registry = (CnTemplateRegistry *)malloc(sizeof(CnTemplateRegistry));
    if (!registry) return NULL;
    
    registry->entries = NULL;
    registry->entry_count = 0;
    registry->capacity = 0;
    
    return registry;
}

/**
 * @brief 释放模板注册表
 */
void cn_template_registry_free(CnTemplateRegistry *registry) {
    if (!registry) return;
    
    // 注意：不释放 entries 中的模板定义，因为它们属于 AST
    if (registry->entries) {
        free(registry->entries);
    }
    
    free(registry);
}

/**
 * @brief 内部函数：向注册表添加条目
 */
static bool cn_template_registry_add_entry(CnTemplateRegistry *registry,
                                           CnTemplateRegistryEntry *entry) {
    // 检查是否已存在同名模板
    for (size_t i = 0; i < registry->entry_count; i++) {
        CnTemplateRegistryEntry *existing = &registry->entries[i];
        if (existing->name_length == entry->name_length &&
            memcmp(existing->name, entry->name, entry->name_length) == 0) {
            // 已存在同名模板
            return false;
        }
    }
    
    // 需要扩容
    if (registry->entry_count >= registry->capacity) {
        size_t new_capacity = registry->capacity == 0 ? 16 : registry->capacity * 2;
        CnTemplateRegistryEntry *new_entries = (CnTemplateRegistryEntry *)realloc(
            registry->entries, new_capacity * sizeof(CnTemplateRegistryEntry));
        if (!new_entries) return false;
        
        registry->entries = new_entries;
        registry->capacity = new_capacity;
    }
    
    // 添加条目
    registry->entries[registry->entry_count++] = *entry;
    
    return true;
}

/**
 * @brief 注册模板函数
 */
bool cn_template_registry_register_function(CnTemplateRegistry *registry,
                                            CnAstTemplateFunctionDecl *template_func) {
    if (!registry || !template_func || !template_func->function) return false;
    
    CnTemplateRegistryEntry entry = {
        .name = template_func->function->name,
        .name_length = template_func->function->name_length,
        .params = template_func->template_params,
        .template_func = template_func,
        .template_struct = NULL,
        .is_function = true
    };
    
    return cn_template_registry_add_entry(registry, &entry);
}

/**
 * @brief 注册模板结构体
 */
bool cn_template_registry_register_struct(CnTemplateRegistry *registry,
                                          CnAstTemplateStructDecl *template_struct) {
    if (!registry || !template_struct || !template_struct->struct_decl) return false;
    
    CnTemplateRegistryEntry entry = {
        .name = template_struct->struct_decl->name,
        .name_length = template_struct->struct_decl->name_length,
        .params = template_struct->template_params,
        .template_func = NULL,
        .template_struct = template_struct,
        .is_function = false
    };
    
    return cn_template_registry_add_entry(registry, &entry);
}

/**
 * @brief 在注册表中查找模板
 */
CnTemplateRegistryEntry *cn_template_registry_lookup(const CnTemplateRegistry *registry,
                                                     const char *name,
                                                     size_t name_len) {
    if (!registry || !name) return NULL;
    
    for (size_t i = 0; i < registry->entry_count; i++) {
        CnTemplateRegistryEntry *entry = &registry->entries[i];
        if (entry->name_length == name_len &&
            memcmp(entry->name, name, name_len) == 0) {
            return entry;
        }
    }
    
    return NULL;
}

/* ============================================================================
 * 辅助函数实现
 * ============================================================================ */

/**
 * @brief 复制类型数组
 */
CnType **cn_type_array_copy(CnType **types, size_t count) {
    if (count == 0) return NULL;
    if (!types) return NULL;
    
    CnType **copy = (CnType **)malloc(count * sizeof(CnType *));
    if (!copy) return NULL;
    
    memcpy(copy, types, count * sizeof(CnType *));
    
    return copy;
}

/**
 * @brief 获取类型的名称字符串
 */
const char *cn_type_get_name(CnType *type) {
    if (!type) return "未知";
    
    switch (type->kind) {
        case CN_TYPE_VOID:       return "空类型";
        case CN_TYPE_INT:        return "整数";
        case CN_TYPE_FLOAT:      return "小数";
        case CN_TYPE_CHAR:       return "字符";
        case CN_TYPE_BOOL:       return "布尔";
        case CN_TYPE_STRING:     return "字符串";
        case CN_TYPE_INT32:      return "整数32";
        case CN_TYPE_INT64:      return "整数64";
        case CN_TYPE_UINT32:     return "无符号整数32";
        case CN_TYPE_UINT64:     return "无符号整数64";
        case CN_TYPE_UINT64_LL:  return "无符号整数64";
        case CN_TYPE_FLOAT32:    return "小数32";
        case CN_TYPE_FLOAT64:    return "小数64";
        case CN_TYPE_POINTER:    return "指针";
        case CN_TYPE_ARRAY:      return "数组";
        case CN_TYPE_STRUCT:     return type->as.struct_type.name;
        case CN_TYPE_ENUM:       return type->as.enum_type.name;
        case CN_TYPE_FUNCTION:   return "函数";
        case CN_TYPE_MEMORY_ADDRESS: return "内存地址";
        case CN_TYPE_UNKNOWN:    return "未知";
        default:                 return "未知";
    }
}

/**
 * @brief 检查类型是否为模板参数类型
 *
 * 模板参数类型在类型系统中用特殊的 CN_TYPE_UNKNOWN 表示，
 * 并且其名称与模板参数名匹配。
 */
bool cn_type_is_template_param(CnType *type) {
    // 目前使用 CN_TYPE_UNKNOWN 作为模板参数类型的标记
    // 后续可以添加专门的 CN_TYPE_TEMPLATE_PARAM 类型
    if (!type) return false;
    
    // 如果是结构体类型且名称为单字母大写（如 T, K, V），可能是模板参数
    if (type->kind == CN_TYPE_STRUCT && type->as.struct_type.name) {
        const char *name = type->as.struct_type.name;
        size_t len = type->as.struct_type.name_length;
        // 简单判断：单字母或双字母大写名称可能是模板参数
        if (len == 1 && name[0] >= 'A' && name[0] <= 'Z') {
            return true;
        }
        if (len == 2 && name[0] >= 'A' && name[0] <= 'Z' && 
            (name[1] >= 'A' && name[1] <= 'Z')) {
            return true;
        }
    }
    
    return false;
}
