/******************************************************************************
 * 文件名: CN_ssa_converter_variable.c
 * 功能: CN_Language SSA转换器变量版本映射表管理
 * 
 * 实现SSA转换器的变量版本映射表管理功能。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_ssa_converter_internal.h"

// ============================================================================
// 静态辅助函数
// ============================================================================

/**
 * @brief 复制字符串（静态辅助函数）
 * 
 * @param str 要复制的字符串
 * @return char* 复制后的字符串，失败返回NULL
 */
static char* cn_strdup(const char* str)
{
    if (str == NULL) {
        return NULL;
    }
    
    size_t len = strlen(str) + 1;
    char* copy = (char*)cn_malloc(len);
    if (copy == NULL) {
        return NULL;
    }
    
    memcpy(copy, str, len);
    return copy;
}

// ============================================================================
// 变量版本映射表函数实现
// ============================================================================

/**
 * @brief 创建变量版本映射表
 */
Stru_VariableVersionMap_t* F_create_variable_version_map(void)
{
    Stru_VariableVersionMap_t* map = (Stru_VariableVersionMap_t*)cn_malloc(
        sizeof(Stru_VariableVersionMap_t));
    if (map == NULL) {
        return NULL;
    }
    
    map->entries = NULL;
    map->entry_count = 0;
    map->capacity = 0;
    
    return map;
}

/**
 * @brief 销毁变量版本映射表
 */
void F_destroy_variable_version_map(Stru_VariableVersionMap_t* map)
{
    if (map == NULL) {
        return;
    }
    
    if (map->entries != NULL) {
        for (size_t i = 0; i < map->entry_count; i++) {
            Stru_VariableVersionEntry_t* entry = map->entries[i];
            if (entry != NULL) {
                if (entry->variable_name != NULL) {
                    cn_free(entry->variable_name);
                }
                // 注意：entry->variable由调用者管理，这里不释放
                cn_free(entry);
            }
        }
        cn_free(map->entries);
    }
    
    cn_free(map);
}

/**
 * @brief 获取或创建变量
 */
Stru_SsaVariable_t* F_get_or_create_variable(Stru_SsaConverterContext_t* context, 
                                           const char* name, bool create_new)
{
    if (context == NULL || name == NULL || context->var_map == NULL) {
        return NULL;
    }
    
    // 查找现有变量
    for (size_t i = 0; i < context->var_map->entry_count; i++) {
        Stru_VariableVersionEntry_t* entry = context->var_map->entries[i];
        if (entry != NULL && strcmp(entry->variable_name, name) == 0) {
            if (create_new) {
                // 创建新版本
                entry->current_version++;
                
                // 创建新变量
                Stru_SsaVariable_t* new_var = F_create_ssa_variable(
                    name, entry->current_version, false);
                if (new_var == NULL) {
                    F_add_error(context, "创建SSA变量失败");
                    return NULL;
                }
                
                entry->variable = new_var;
                return new_var;
            } else {
                // 返回现有变量
                return entry->variable;
            }
        }
    }
    
    // 变量不存在，创建新条目
    if (create_new) {
        // 检查是否需要调整数组大小
        if (context->var_map->entry_count >= context->var_map->capacity) {
            size_t new_capacity = context->var_map->capacity == 0 ? 8 : context->var_map->capacity * 2;
            Stru_VariableVersionEntry_t** new_entries = (Stru_VariableVersionEntry_t**)cn_realloc(
                context->var_map->entries, new_capacity * sizeof(Stru_VariableVersionEntry_t*));
            
            if (new_entries == NULL) {
                F_add_error(context, "调整变量映射表大小失败");
                return NULL;
            }
            
            context->var_map->entries = new_entries;
            context->var_map->capacity = new_capacity;
        }
        
        // 创建新条目
        Stru_VariableVersionEntry_t* entry = (Stru_VariableVersionEntry_t*)cn_malloc(
            sizeof(Stru_VariableVersionEntry_t));
        if (entry == NULL) {
            F_add_error(context, "创建变量映射条目失败");
            return NULL;
        }
        
        entry->variable_name = cn_strdup(name);
        if (entry->variable_name == NULL) {
            cn_free(entry);
            F_add_error(context, "复制变量名失败");
            return NULL;
        }
        
        entry->current_version = 0;
        
        // 创建变量（版本0）
        entry->variable = F_create_ssa_variable(name, 0, false);
        if (entry->variable == NULL) {
            cn_free(entry->variable_name);
            cn_free(entry);
            F_add_error(context, "创建SSA变量失败");
            return NULL;
        }
        
        // 添加到映射表
        context->var_map->entries[context->var_map->entry_count] = entry;
        context->var_map->entry_count++;
        
        return entry->variable;
    }
    
    // 变量不存在且不创建新变量
    return NULL;
}

/**
 * @brief 创建临时变量
 */
Stru_SsaVariable_t* F_create_temporary_variable(Stru_SsaConverterContext_t* context)
{
    if (context == NULL) {
        return NULL;
    }
    
    // 生成临时变量名
    char temp_name[32];
    snprintf(temp_name, sizeof(temp_name), "t%d", context->next_temp_id);
    context->next_temp_id++;
    
    // 创建临时变量（总是创建新版本）
    Stru_SsaVariable_t* temp_var = F_create_ssa_variable(temp_name, 0, true);
    if (temp_var == NULL) {
        F_add_error(context, "创建临时变量失败");
        return NULL;
    }
    
    return temp_var;
}
