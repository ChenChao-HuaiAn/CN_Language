/**
 * @file module_semantics.c
 * @brief CN语言 跨文件模块语义分析实现
 * 
 * 实现阶段D的全部功能：
 * - D1: 扩展符号表支持跨文件
 * - D2: 实现模块作用域隔离
 * - D3: 实现跨文件名称解析
 * - D4: 实现导入符号可见性检查
 * - D5: 实现模块初始化顺序分析
 * - D6: 实现导入冲突检测
 * - D7: 实现相对导入路径解析
 * 
 * @version 1.0
 * @date 2026-01-29
 */

#include "cnlang/frontend/semantics.h"
#include "cnlang/frontend/module_loader.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/support/diagnostics.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部数据结构
// ============================================================================

// 符号链表节点（与 symbol_table.c 保持一致）
typedef struct CnSemSymbolNode {
    CnSemSymbol symbol;
    struct CnSemSymbolNode *next;
} CnSemSymbolNode;

// 扩展的作用域结构体（包含文件模块信息）
struct CnSemScope {
    CnSemScopeKind kind;
    CnSemScope *parent;
    CnSemSymbolNode *symbols;
    const char *name;
    size_t name_length;
    
    // 文件模块专用字段
    CnFileModuleSemInfo *file_module_info;  // 仅当 kind == CN_SEM_SCOPE_FILE_MODULE 时有效
};

// ============================================================================
// D1: 扩展符号表支持跨文件 - 文件模块语义信息管理
// ============================================================================

CnFileModuleSemInfo *cn_file_module_sem_info_create(struct CnModuleId *module_id)
{
    CnFileModuleSemInfo *info = (CnFileModuleSemInfo *)malloc(sizeof(CnFileModuleSemInfo));
    if (!info) {
        return NULL;
    }
    
    memset(info, 0, sizeof(CnFileModuleSemInfo));
    info->module_id = module_id;  // 借用引用，不复制
    
    return info;
}

void cn_file_module_sem_info_free(CnFileModuleSemInfo *info)
{
    if (!info) {
        return;
    }
    
    // 释放导入符号列表
    if (info->imported_symbols) {
        free(info->imported_symbols);
    }
    
    // 释放导出符号列表（只释放数组，不释放符号本身）
    if (info->exported_symbols) {
        free(info->exported_symbols);
    }
    
    // 释放依赖列表（只释放数组，不释放模块ID本身）
    if (info->dependencies) {
        free(info->dependencies);
    }
    
    free(info);
}

int cn_file_module_sem_info_add_export(CnFileModuleSemInfo *info, CnSemSymbol *symbol)
{
    if (!info || !symbol) {
        return 0;
    }
    
    // 扩容检查
    if (info->exported_symbol_count >= info->exported_symbol_capacity) {
        size_t new_capacity = info->exported_symbol_capacity == 0 ? 16 : info->exported_symbol_capacity * 2;
        CnSemSymbol **new_exports = (CnSemSymbol **)realloc(
            info->exported_symbols, sizeof(CnSemSymbol *) * new_capacity);
        if (!new_exports) {
            return 0;
        }
        info->exported_symbols = new_exports;
        info->exported_symbol_capacity = new_capacity;
    }
    
    info->exported_symbols[info->exported_symbol_count++] = symbol;
    return 1;
}

int cn_file_module_sem_info_add_dependency(CnFileModuleSemInfo *info, struct CnModuleId *dep_id)
{
    if (!info || !dep_id) {
        return 0;
    }
    
    // 检查是否已存在
    for (size_t i = 0; i < info->dependency_count; i++) {
        if (cn_module_id_equals(info->dependencies[i], dep_id)) {
            return 1;  // 已存在，不重复添加
        }
    }
    
    // 扩容检查
    if (info->dependency_count >= info->dependency_capacity) {
        size_t new_capacity = info->dependency_capacity == 0 ? 8 : info->dependency_capacity * 2;
        struct CnModuleId **new_deps = (struct CnModuleId **)realloc(
            info->dependencies, sizeof(struct CnModuleId *) * new_capacity);
        if (!new_deps) {
            return 0;
        }
        info->dependencies = new_deps;
        info->dependency_capacity = new_capacity;
    }
    
    info->dependencies[info->dependency_count++] = dep_id;
    return 1;
}

// ============================================================================
// D2: 实现模块作用域隔离
// ============================================================================

CnSemScope *cn_sem_file_module_scope_new(struct CnModuleId *module_id, CnSemScope *parent)
{
    CnSemScope *scope = cn_sem_scope_new(CN_SEM_SCOPE_FILE_MODULE, parent);
    if (!scope) {
        return NULL;
    }
    
    // 创建文件模块语义信息
    CnFileModuleSemInfo *file_info = cn_file_module_sem_info_create(module_id);
    if (!file_info) {
        cn_sem_scope_free(scope);
        return NULL;
    }
    
    scope->file_module_info = file_info;
    file_info->file_scope = scope;
    
    // 设置作用域名称为模块名
    if (module_id && module_id->qualified_name) {
        cn_sem_scope_set_name(scope, module_id->qualified_name, module_id->qualified_name_length);
    }
    
    return scope;
}

CnFileModuleSemInfo *cn_sem_scope_get_file_module_info(CnSemScope *scope)
{
    if (!scope || scope->kind != CN_SEM_SCOPE_FILE_MODULE) {
        return NULL;
    }
    
    return scope->file_module_info;
}

// ============================================================================
// D3: 实现跨文件名称解析
// ============================================================================

CnSemSymbol *cn_sem_resolve_module_path(struct CnModuleLoader *loader,
                                         CnSemScope *current_scope,
                                         struct CnAstModulePath *module_path,
                                         struct CnDiagnostics *diagnostics)
{
    if (!loader || !current_scope || !module_path || module_path->segment_count == 0) {
        return NULL;
    }
    
    // 构建模块ID
    const char **segments = (const char **)malloc(sizeof(const char *) * module_path->segment_count);
    if (!segments) {
        return NULL;
    }
    
    for (size_t i = 0; i < module_path->segment_count; i++) {
        segments[i] = module_path->segments[i].name;
    }
    
    CnModuleId *module_id = cn_module_id_create_from_segments(segments, module_path->segment_count);
    free(segments);
    
    if (!module_id) {
        return NULL;
    }
    
    // 加载模块
    CnModuleMetadata *metadata = cn_module_loader_load(loader, module_id);
    if (!metadata) {
        // 模块未找到
        if (diagnostics) {
            cn_support_diag_semantic_error_generic(
                diagnostics,
                CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                NULL, 0, 0,
                "语义错误：模块不存在");
        }
        cn_module_id_free(module_id);
        return NULL;
    }
    
    // 在当前作用域查找模块符号
    CnSemSymbol *module_sym = cn_sem_scope_lookup(current_scope,
                                                   module_id->qualified_name,
                                                   module_id->qualified_name_length);
    
    cn_module_id_free(module_id);
    return module_sym;
}

CnSemSymbol *cn_sem_resolve_module_member(struct CnModuleLoader *loader,
                                           CnSemScope *module_scope,
                                           const char *member_name,
                                           size_t member_name_length,
                                           struct CnDiagnostics *diagnostics)
{
    if (!module_scope || !member_name || member_name_length == 0) {
        return NULL;
    }
    
    // 在模块作用域中查找成员
    CnSemSymbol *member_sym = cn_sem_scope_lookup_shallow(module_scope, member_name, member_name_length);
    
    if (!member_sym) {
        if (diagnostics) {
            cn_support_diag_semantic_error_generic(
                diagnostics,
                CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                NULL, 0, 0,
                "语义错误：模块成员不存在");
        }
        return NULL;
    }
    
    // 检查可见性
    if (!cn_sem_symbol_is_accessible(member_sym)) {
        if (diagnostics) {
            cn_support_diag_semantic_error_generic(
                diagnostics,
                CN_DIAG_CODE_SEM_ACCESS_DENIED,
                NULL, 0, 0,
                "语义错误：无法访问私有成员");
        }
        return NULL;
    }
    
    return member_sym;
}

// ============================================================================
// D4: 实现导入符号可见性检查
// ============================================================================

int cn_sem_symbol_is_accessible(CnSemSymbol *symbol)
{
    if (!symbol) {
        return 0;
    }
    
    // 公开符号可访问
    if (symbol->is_public) {
        return 1;
    }
    
    // 私有符号不可外部访问
    return 0;
}

bool cn_sem_check_import_visibility(CnFileModuleSemInfo *file_info,
                                     struct CnDiagnostics *diagnostics)
{
    if (!file_info) {
        return true;
    }
    
    bool all_accessible = true;
    
    for (size_t i = 0; i < file_info->imported_symbol_count; i++) {
        CnImportedSymbol *imported = &file_info->imported_symbols[i];
        
        if (imported->resolved_symbol && !cn_sem_symbol_is_accessible(imported->resolved_symbol)) {
            all_accessible = false;
            
            if (diagnostics) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg),
                         "语义错误：无法导入私有成员 '%.*s'",
                         (int)imported->original_name_length, imported->original_name);
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_ACCESS_DENIED,
                    NULL, 0, 0,
                    error_msg);
            }
        }
    }
    
    return all_accessible;
}

// ============================================================================
// D5: 实现模块初始化顺序分析
// ============================================================================

bool cn_sem_analyze_init_order(struct CnModuleLoader *loader,
                                struct CnModuleId *entry_module,
                                struct CnModuleId ***out_order,
                                size_t *out_count,
                                struct CnDiagnostics *diagnostics)
{
    if (!loader || !entry_module || !out_order || !out_count) {
        return false;
    }
    
    // 使用模块加载器中的依赖图进行拓扑排序
    int result = cn_module_loader_get_init_order(loader, out_order, out_count);
    
    if (!result) {
        if (diagnostics) {
            cn_support_diag_semantic_error_generic(
                diagnostics,
                CN_DIAG_CODE_UNKNOWN,
                NULL, 0, 0,
                "语义错误：存在循环依赖，无法确定模块初始化顺序");
        }
        return false;
    }
    
    return true;
}

// ============================================================================
// D6: 实现导入冲突检测
// ============================================================================

bool cn_sem_check_import_conflicts(CnFileModuleSemInfo *file_info,
                                    struct CnDiagnostics *diagnostics)
{
    if (!file_info) {
        return true;
    }
    
    bool no_conflicts = true;
    
    // 检查导入符号之间的名称冲突
    for (size_t i = 0; i < file_info->imported_symbol_count; i++) {
        CnImportedSymbol *sym_i = &file_info->imported_symbols[i];
        
        for (size_t j = i + 1; j < file_info->imported_symbol_count; j++) {
            CnImportedSymbol *sym_j = &file_info->imported_symbols[j];
            
            // 比较本地名称
            if (sym_i->local_name_length == sym_j->local_name_length &&
                memcmp(sym_i->local_name, sym_j->local_name, sym_i->local_name_length) == 0) {
                
                no_conflicts = false;
                
                if (diagnostics) {
                    char error_msg[512];
                    snprintf(error_msg, sizeof(error_msg),
                             "语义错误：导入名称冲突 '%.*s'（来自不同模块）",
                             (int)sym_i->local_name_length, sym_i->local_name);
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL,
                        NULL, 0, 0,
                        error_msg);
                }
            }
        }
    }
    
    // 检查导入符号与本地符号的冲突
    if (file_info->file_scope) {
        for (size_t i = 0; i < file_info->imported_symbol_count; i++) {
            CnImportedSymbol *imported = &file_info->imported_symbols[i];
            
            // 在导出符号中查找
            for (size_t j = 0; j < file_info->exported_symbol_count; j++) {
                CnSemSymbol *local = file_info->exported_symbols[j];
                
                if (local->name_length == imported->local_name_length &&
                    memcmp(local->name, imported->local_name, local->name_length) == 0) {
                    
                    no_conflicts = false;
                    
                    if (diagnostics) {
                        char error_msg[512];
                        snprintf(error_msg, sizeof(error_msg),
                                 "语义错误：导入的符号 '%.*s' 与本地定义冲突",
                                 (int)imported->local_name_length, imported->local_name);
                        cn_support_diag_semantic_error_generic(
                            diagnostics,
                            CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL,
                            NULL, 0, 0,
                            error_msg);
                    }
                }
            }
        }
    }
    
    return no_conflicts;
}

int cn_sem_register_imported_symbol(CnFileModuleSemInfo *file_info,
                                     const char *local_name,
                                     size_t local_name_length,
                                     const char *original_name,
                                     size_t original_name_length,
                                     struct CnModuleId *source_module,
                                     CnSemSymbol *resolved_symbol,
                                     int is_public)
{
    if (!file_info || !local_name || local_name_length == 0) {
        return 0;
    }
    
    // 扩容检查
    if (file_info->imported_symbol_count >= file_info->imported_symbol_capacity) {
        size_t new_capacity = file_info->imported_symbol_capacity == 0 ? 16 : file_info->imported_symbol_capacity * 2;
        CnImportedSymbol *new_imports = (CnImportedSymbol *)realloc(
            file_info->imported_symbols, sizeof(CnImportedSymbol) * new_capacity);
        if (!new_imports) {
            return 0;
        }
        file_info->imported_symbols = new_imports;
        file_info->imported_symbol_capacity = new_capacity;
    }
    
    // 添加导入符号
    CnImportedSymbol *imported = &file_info->imported_symbols[file_info->imported_symbol_count];
    imported->local_name = local_name;
    imported->local_name_length = local_name_length;
    imported->original_name = original_name ? original_name : local_name;
    imported->original_name_length = original_name_length > 0 ? original_name_length : local_name_length;
    imported->source_module = source_module;
    imported->resolved_symbol = resolved_symbol;
    imported->is_public = is_public;
    
    file_info->imported_symbol_count++;
    return 1;
}

// ============================================================================
// D7: 实现相对导入路径解析
// ============================================================================

bool cn_sem_resolve_relative_import(struct CnModuleLoader *loader,
                                     struct CnModuleId *current_module,
                                     struct CnAstModulePath *relative_path,
                                     struct CnModuleId **out_resolved_id,
                                     struct CnDiagnostics *diagnostics)
{
    if (!loader || !current_module || !relative_path || !out_resolved_id) {
        return false;
    }
    
    // 检查是否为相对导入
    if (!relative_path->is_relative) {
        // 不是相对导入，直接构建模块ID
        const char **segments = (const char **)malloc(sizeof(const char *) * relative_path->segment_count);
        if (!segments) {
            return false;
        }
        
        for (size_t i = 0; i < relative_path->segment_count; i++) {
            segments[i] = relative_path->segments[i].name;
        }
        
        *out_resolved_id = cn_module_id_create_from_segments(segments, relative_path->segment_count);
        free(segments);
        
        return *out_resolved_id != NULL;
    }
    
    // 相对导入处理
    int relative_level = relative_path->relative_level;
    
    // 验证相对层级不超过当前模块层级
    if ((size_t)relative_level > current_module->segment_count) {
        if (diagnostics) {
            cn_support_diag_semantic_error_generic(
                diagnostics,
                CN_DIAG_CODE_UNKNOWN,
                NULL, 0, 0,
                "语义错误：相对导入层级超出包结构");
        }
        return false;
    }
    
    // 计算基础路径段数量
    size_t base_segment_count = current_module->segment_count - (size_t)relative_level;
    
    // 构建完整路径
    size_t total_segments = base_segment_count + relative_path->segment_count;
    const char **segments = (const char **)malloc(sizeof(const char *) * total_segments);
    if (!segments) {
        return false;
    }
    
    // 添加基础路径段
    for (size_t i = 0; i < base_segment_count; i++) {
        segments[i] = current_module->segments[i];
    }
    
    // 添加相对路径段
    for (size_t i = 0; i < relative_path->segment_count; i++) {
        segments[base_segment_count + i] = relative_path->segments[i].name;
    }
    
    *out_resolved_id = cn_module_id_create_from_segments(segments, total_segments);
    free(segments);
    
    if (!*out_resolved_id) {
        return false;
    }
    
    // 验证模块是否存在
    CnModuleMetadata *metadata = cn_module_loader_load(loader, *out_resolved_id);
    if (!metadata) {
        if (diagnostics) {
            cn_support_diag_semantic_error_generic(
                diagnostics,
                CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                NULL, 0, 0,
                "语义错误：相对导入的目标模块不存在");
        }
        cn_module_id_free(*out_resolved_id);
        *out_resolved_id = NULL;
        return false;
    }
    
    return true;
}
