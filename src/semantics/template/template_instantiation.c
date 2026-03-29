/**
 * @file template_instantiation.c
 * @brief 模板实例化核心逻辑实现
 *
 * 实现类型推导、名称修饰和模板实例化功能。
 */

#include "cnlang/semantics/template.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * 名称修饰实现
 * ============================================================================ */

/**
 * @brief 计算类型名称的字符串长度
 */
static size_t cn_type_name_length(CnType *type) {
    const char *name = cn_type_get_name(type);
    return strlen(name);
}

/**
 * @brief 将类型名称追加到字符串缓冲区
 */
static void cn_type_name_append(char *buf, size_t *offset, CnType *type) {
    const char *name = cn_type_get_name(type);
    size_t len = strlen(name);
    memcpy(buf + *offset, name, len);
    *offset += len;
}

/**
 * @brief 生成模板实例的唯一名称
 *
 * 格式：__cn_template_原名称_类型1_类型2_...
 */
char *cn_template_mangle_name(const char *template_name,
                              size_t template_name_len,
                              CnType **type_args,
                              size_t type_arg_count) {
    if (!template_name) return NULL;
    
    // 计算总长度
    size_t total_len = strlen("__cn_template_") + template_name_len;
    for (size_t i = 0; i < type_arg_count; i++) {
        total_len += 1;  // 下划线分隔符
        if (type_args[i]) {
            total_len += cn_type_name_length(type_args[i]);
        }
    }
    
    // 分配内存
    char *name = (char *)malloc(total_len + 1);
    if (!name) return NULL;
    
    // 构建名称
    size_t offset = 0;
    
    // 前缀
    const char *prefix = "__cn_template_";
    size_t prefix_len = strlen(prefix);
    memcpy(name + offset, prefix, prefix_len);
    offset += prefix_len;
    
    // 模板名称
    memcpy(name + offset, template_name, template_name_len);
    offset += template_name_len;
    
    // 类型参数
    for (size_t i = 0; i < type_arg_count; i++) {
        name[offset++] = '_';
        if (type_args[i]) {
            cn_type_name_append(name, &offset, type_args[i]);
        }
    }
    
    name[offset] = '\0';
    return name;
}

/* ============================================================================
 * 类型推导实现
 * ============================================================================ */

/**
 * @brief 从表达式获取类型
 *
 * 辅助函数，从表达式节点获取其类型信息
 */
static CnType *cn_expr_get_type(CnAstExpr *expr) {
    if (!expr) return NULL;
    return expr->type;  // 表达式节点中的类型字段
}

/**
 * @brief 从单个参数进行类型推导
 *
 * 用于模式匹配，如 T* 匹配 整数* -> T = 整数
 */
bool cn_template_deduce_from_param(CnType *param_type,
                                   CnType *arg_type,
                                   CnTypeMap *type_map) {
    if (!param_type || !arg_type || !type_map) return false;
    
    // 如果参数类型是模板参数（简单标识符类型）
    // 检查是否是单字母大写名称（约定为模板参数）
    if (param_type->kind == CN_TYPE_STRUCT && 
        param_type->as.struct_type.name &&
        param_type->as.struct_type.name_length == 1) {
        const char *name = param_type->as.struct_type.name;
        if (name[0] >= 'A' && name[0] <= 'Z') {
            // 这是一个模板参数，记录映射
            return cn_type_map_insert(type_map,
                                      param_type->as.struct_type.name,
                                      param_type->as.struct_type.name_length,
                                      arg_type);
        }
    }
    
    // 如果参数类型是指针，递归处理
    if (param_type->kind == CN_TYPE_POINTER && arg_type->kind == CN_TYPE_POINTER) {
        return cn_template_deduce_from_param(
            param_type->as.pointer_to,
            arg_type->as.pointer_to,
            type_map);
    }
    
    // 如果参数类型是数组，递归处理
    if (param_type->kind == CN_TYPE_ARRAY && arg_type->kind == CN_TYPE_ARRAY) {
        // 数组长度必须匹配（或者参数长度为0表示未知）
        if (param_type->as.array.length != 0 &&
            param_type->as.array.length != arg_type->as.array.length) {
            return false;
        }
        return cn_template_deduce_from_param(
            param_type->as.array.element_type,
            arg_type->as.array.element_type,
            type_map);
    }
    
    // 其他情况：类型必须兼容
    return cn_type_compatible(param_type, arg_type);
}

/**
 * @brief 从函数调用实参推导模板参数类型
 */
bool cn_template_deduce_types(CnAstTemplateParams *template_params,
                              CnAstParameter *func_params,
                              size_t func_param_count,
                              CnAstExpr **call_args,
                              size_t call_arg_count,
                              CnType ***out_type_args) {
    if (!template_params || !out_type_args) return false;
    
    // 参数数量必须匹配
    if (call_arg_count != func_param_count) {
        return false;
    }
    
    // 创建类型映射表
    CnTypeMap *type_map = cn_type_map_new();
    if (!type_map) return false;
    
    // 遍历参数进行类型推导
    for (size_t i = 0; i < func_param_count; i++) {
        CnType *param_type = func_params[i].declared_type;
        CnType *arg_type = cn_expr_get_type(call_args[i]);
        
        if (!param_type || !arg_type) {
            cn_type_map_free(type_map);
            return false;
        }
        
        if (!cn_template_deduce_from_param(param_type, arg_type, type_map)) {
            cn_type_map_free(type_map);
            return false;
        }
    }
    
    // 构建类型实参数组
    size_t param_count = template_params->param_count;
    CnType **type_args = (CnType **)calloc(param_count, sizeof(CnType *));
    if (!type_args) {
        cn_type_map_free(type_map);
        return false;
    }
    
    // 从映射表中提取类型
    for (size_t i = 0; i < param_count; i++) {
        CnAstTemplateParam *param = &template_params->params[i];
        CnType *concrete_type = cn_type_map_lookup(type_map,
                                                   param->name,
                                                   param->name_length);
        if (!concrete_type) {
            // 未推导出类型，失败
            free(type_args);
            cn_type_map_free(type_map);
            return false;
        }
        type_args[i] = concrete_type;
    }
    
    *out_type_args = type_args;
    cn_type_map_free(type_map);
    return true;
}

/* ============================================================================
 * 模板函数实例化实现
 * ============================================================================ */

/**
 * @brief 创建函数声明的深拷贝
 *
 * 用于实例化时复制模板函数
 */
static CnAstFunctionDecl *cn_function_decl_clone(CnAstFunctionDecl *orig) {
    if (!orig) return NULL;
    
    CnAstFunctionDecl *clone = (CnAstFunctionDecl *)malloc(sizeof(CnAstFunctionDecl));
    if (!clone) return NULL;
    
    // 复制基本字段
    clone->name = orig->name;
    clone->name_length = orig->name_length;
    clone->return_type = orig->return_type;  // 类型会被后续替换
    clone->body = orig->body;  // 函数体会被共享，类型替换时处理
    clone->visibility = orig->visibility;
    clone->is_interrupt_handler = orig->is_interrupt_handler;
    clone->interrupt_vector = orig->interrupt_vector;
    
    // 复制参数列表
    if (orig->parameter_count > 0 && orig->parameters) {
        clone->parameters = (CnAstParameter *)malloc(
            orig->parameter_count * sizeof(CnAstParameter));
        if (!clone->parameters) {
            free(clone);
            return NULL;
        }
        clone->parameter_count = orig->parameter_count;
        
        for (size_t i = 0; i < orig->parameter_count; i++) {
            clone->parameters[i].name = orig->parameters[i].name;
            clone->parameters[i].name_length = orig->parameters[i].name_length;
            clone->parameters[i].declared_type = orig->parameters[i].declared_type;
            clone->parameters[i].is_const = orig->parameters[i].is_const;
        }
    } else {
        clone->parameters = NULL;
        clone->parameter_count = 0;
    }
    
    return clone;
}

/**
 * @brief 创建结构体声明的深拷贝
 */
static CnAstStructDecl *cn_struct_decl_clone(CnAstStructDecl *orig) {
    if (!orig) return NULL;
    
    CnAstStructDecl *clone = (CnAstStructDecl *)malloc(sizeof(CnAstStructDecl));
    if (!clone) return NULL;
    
    clone->name = orig->name;
    clone->name_length = orig->name_length;
    clone->field_count = orig->field_count;
    
    // 复制字段列表
    if (orig->field_count > 0 && orig->fields) {
        clone->fields = (CnAstStructField *)malloc(
            orig->field_count * sizeof(CnAstStructField));
        if (!clone->fields) {
            free(clone);
            return NULL;
        }
        
        for (size_t i = 0; i < orig->field_count; i++) {
            clone->fields[i].name = orig->fields[i].name;
            clone->fields[i].name_length = orig->fields[i].name_length;
            clone->fields[i].field_type = orig->fields[i].field_type;
            clone->fields[i].is_const = orig->fields[i].is_const;
        }
    } else {
        clone->fields = NULL;
    }
    
    return clone;
}

/**
 * @brief 实例化模板函数
 */
CnAstFunctionDecl *cn_template_instantiate_function(
    CnAstTemplateFunctionDecl *template_func,
    CnType **type_args,
    size_t type_arg_count,
    CnTemplateCache *cache) {
    
    if (!template_func || !template_func->function) return NULL;
    if (!type_args || type_arg_count == 0) return NULL;
    
    CnAstFunctionDecl *orig_func = template_func->function;
    
    // 检查缓存中是否已存在
    if (cache) {
        CnTemplateInstance *existing = cn_template_cache_find(
            cache,
            orig_func->name,
            orig_func->name_length,
            type_args,
            type_arg_count);
        
        if (existing && existing->instantiated_function) {
            return existing->instantiated_function;
        }
    }
    
    // 创建类型映射表
    CnTypeMap *type_map = cn_type_map_new();
    if (!type_map) return NULL;
    
    // 构建模板参数到类型实参的映射
    CnAstTemplateParams *params = template_func->template_params;
    if (!params || params->param_count != type_arg_count) {
        cn_type_map_free(type_map);
        return NULL;
    }
    
    for (size_t i = 0; i < params->param_count; i++) {
        cn_type_map_insert(type_map,
                          params->params[i].name,
                          params->params[i].name_length,
                          type_args[i]);
    }
    
    // 创建函数声明的副本
    CnAstFunctionDecl *instance = cn_function_decl_clone(orig_func);
    if (!instance) {
        cn_type_map_free(type_map);
        return NULL;
    }
    
    // 替换参数类型中的模板参数
    for (size_t i = 0; i < instance->parameter_count; i++) {
        CnType *orig_type = instance->parameters[i].declared_type;
        if (orig_type) {
            instance->parameters[i].declared_type = cn_type_substitute(orig_type, type_map);
        }
    }
    
    // 替换返回类型中的模板参数
    if (instance->return_type) {
        instance->return_type = cn_type_substitute(instance->return_type, type_map);
    }
    
    // 替换函数体中的类型引用
    if (instance->body) {
        cn_type_substitute_block(instance->body, type_map);
    }
    
    // 生成实例化名称
    char *mangled_name = cn_template_mangle_name(
        orig_func->name,
        orig_func->name_length,
        type_args,
        type_arg_count);
    
    if (mangled_name) {
        instance->name = mangled_name;
        instance->name_length = strlen(mangled_name);
    }
    
    // 添加到缓存
    if (cache) {
        CnTemplateInstance *inst_entry = (CnTemplateInstance *)malloc(sizeof(CnTemplateInstance));
        if (inst_entry) {
            inst_entry->mangled_name = mangled_name;
            inst_entry->mangled_name_length = instance->name_length;
            inst_entry->template_name = orig_func->name;
            inst_entry->template_name_length = orig_func->name_length;
            inst_entry->type_args = cn_type_array_copy(type_args, type_arg_count);
            inst_entry->type_arg_count = type_arg_count;
            inst_entry->instantiated_function = instance;
            inst_entry->instantiated_struct = NULL;
            inst_entry->instantiated_type = NULL;
            
            cn_template_cache_add(cache, inst_entry);
        }
    }
    
    cn_type_map_free(type_map);
    return instance;
}

/**
 * @brief 实例化模板结构体
 */
CnAstStructDecl *cn_template_instantiate_struct(
    CnAstTemplateStructDecl *template_struct,
    CnType **type_args,
    size_t type_arg_count,
    CnTemplateCache *cache) {
    
    if (!template_struct || !template_struct->struct_decl) return NULL;
    if (!type_args || type_arg_count == 0) return NULL;
    
    CnAstStructDecl *orig_struct = template_struct->struct_decl;
    
    // 检查缓存中是否已存在
    if (cache) {
        CnTemplateInstance *existing = cn_template_cache_find(
            cache,
            orig_struct->name,
            orig_struct->name_length,
            type_args,
            type_arg_count);
        
        if (existing && existing->instantiated_struct) {
            return existing->instantiated_struct;
        }
    }
    
    // 创建类型映射表
    CnTypeMap *type_map = cn_type_map_new();
    if (!type_map) return NULL;
    
    // 构建模板参数到类型实参的映射
    CnAstTemplateParams *params = template_struct->template_params;
    if (!params || params->param_count != type_arg_count) {
        cn_type_map_free(type_map);
        return NULL;
    }
    
    for (size_t i = 0; i < params->param_count; i++) {
        cn_type_map_insert(type_map,
                          params->params[i].name,
                          params->params[i].name_length,
                          type_args[i]);
    }
    
    // 创建结构体声明的副本
    CnAstStructDecl *instance = cn_struct_decl_clone(orig_struct);
    if (!instance) {
        cn_type_map_free(type_map);
        return NULL;
    }
    
    // 替换字段类型中的模板参数
    for (size_t i = 0; i < instance->field_count; i++) {
        CnType *orig_type = instance->fields[i].field_type;
        if (orig_type) {
            instance->fields[i].field_type = cn_type_substitute(orig_type, type_map);
        }
    }
    
    // 生成实例化名称
    char *mangled_name = cn_template_mangle_name(
        orig_struct->name,
        orig_struct->name_length,
        type_args,
        type_arg_count);
    
    if (mangled_name) {
        instance->name = mangled_name;
        instance->name_length = strlen(mangled_name);
    }
    
    // 添加到缓存
    if (cache) {
        CnTemplateInstance *inst_entry = (CnTemplateInstance *)malloc(sizeof(CnTemplateInstance));
        if (inst_entry) {
            inst_entry->mangled_name = mangled_name;
            inst_entry->mangled_name_length = instance->name_length;
            inst_entry->template_name = orig_struct->name;
            inst_entry->template_name_length = orig_struct->name_length;
            inst_entry->type_args = cn_type_array_copy(type_args, type_arg_count);
            inst_entry->type_arg_count = type_arg_count;
            inst_entry->instantiated_function = NULL;
            inst_entry->instantiated_struct = instance;
            inst_entry->instantiated_type = NULL;
            
            cn_template_cache_add(cache, inst_entry);
        }
    }
    
    cn_type_map_free(type_map);
    return instance;
}
