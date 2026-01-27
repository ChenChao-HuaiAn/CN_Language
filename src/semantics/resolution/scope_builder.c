#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

#include <stdlib.h>
#include <string.h>

// 符号链表节点，用于在作用域中维护符号列表
// （与 symbol_table.c 中的定义保持一致）
typedef struct CnSemSymbolNode {
    CnSemSymbol symbol;
    struct CnSemSymbolNode *next;
} CnSemSymbolNode;

// 作用域结构体的内部实现
// （与 symbol_table.c 中的定义保持一致）
struct CnSemScope {
    CnSemScopeKind kind;
    CnSemScope *parent;
    CnSemSymbolNode *symbols;
};

static void cn_sem_build_function_scope(CnSemScope *parent_scope,
                                         CnAstFunctionDecl *function_decl,
                                         CnDiagnostics *diagnostics);
static void cn_sem_build_block_scope(CnSemScope *parent_scope,
                                     CnAstBlockStmt *block,
                                     CnDiagnostics *diagnostics);
static void cn_sem_build_module_scope(CnSemScope *parent_scope,
                                      CnAstStmt *module_stmt,
                                      CnDiagnostics *diagnostics);
static void cn_sem_build_stmt(CnSemScope *scope, CnAstStmt *stmt, CnDiagnostics *diagnostics);
static void cn_sem_build_expr(CnSemScope *scope, CnAstExpr *expr, CnDiagnostics *diagnostics);
static void cn_sem_build_if_stmt(CnSemScope *scope, CnAstIfStmt *if_stmt, CnDiagnostics *diagnostics);
static void cn_sem_build_while_stmt(CnSemScope *scope, CnAstWhileStmt *while_stmt, CnDiagnostics *diagnostics);
static void cn_sem_build_for_stmt(CnSemScope *scope, CnAstForStmt *for_stmt, CnDiagnostics *diagnostics);

CnSemScope *cn_sem_build_scopes(CnAstProgram *program, CnDiagnostics *diagnostics)
{
    CnSemScope *global_scope;
    size_t i;

    if (!program) {
        return NULL;
    }

    global_scope = cn_sem_scope_new(CN_SEM_SCOPE_GLOBAL, NULL);
    if (!global_scope) {
        return NULL;
    }

    // 注册内置函数：打印 (print)
    CnSemSymbol *print_sym = cn_sem_scope_insert_symbol(global_scope, "打印", strlen("打印"), CN_SEM_SYMBOL_FUNCTION);
    if (print_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);
        print_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_VOID), param_types, 1);
    }

    // 注册内置函数：打印整数 (print_int)
    CnSemSymbol *print_int_sym = cn_sem_scope_insert_symbol(global_scope, "打印整数", strlen("打印整数"), CN_SEM_SYMBOL_FUNCTION);
    if (print_int_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_INT);
        print_int_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_VOID), param_types, 1);
    }

    // 注册内置函数：打印字符串 (print_string)
    CnSemSymbol *print_str_sym = cn_sem_scope_insert_symbol(global_scope, "打印字符串", strlen("打印字符串"), CN_SEM_SYMBOL_FUNCTION);
    if (print_str_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);
        print_str_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_VOID), param_types, 1);
    }

    // 注册内置函数：长度 (length)
    // 注意：长度函数是特殊的，它可以接受字符串或数组参数
    // 我们在符号表中标记它，但在 semantic_passes.c 中特殊处理其类型检查
    // 为了避免类型冲突，我们将其标记为 UNKNOWN 类型，让特殊处理逻辑来判断
    CnSemSymbol *length_sym = cn_sem_scope_insert_symbol(global_scope, "长度", strlen("长度"), CN_SEM_SYMBOL_FUNCTION);
    if (length_sym) {
        // 使用 UNKNOWN 类型标记，让类型检查器特殊处理
        length_sym->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
    }

    // 注册结构体声明到全局作用域
    for (i = 0; i < program->struct_count; ++i) {
        CnAstStmt *struct_stmt = program->structs[i];
        if (!struct_stmt || struct_stmt->kind != CN_AST_STMT_STRUCT_DECL) {
            continue;
        }

        CnAstStructDecl *struct_decl = &struct_stmt->as.struct_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(global_scope,
                                   struct_decl->name,
                                   struct_decl->name_length,
                                   CN_SEM_SYMBOL_STRUCT);
        if (sym) {
            // 创建结构体类型，包含字段信息
            CnStructField *fields = NULL;
            if (struct_decl->field_count > 0) {
                fields = (CnStructField *)malloc(sizeof(CnStructField) * struct_decl->field_count);
                for (size_t j = 0; j < struct_decl->field_count; j++) {
                    fields[j].name = struct_decl->fields[j].name;
                    fields[j].name_length = struct_decl->fields[j].name_length;
                    fields[j].field_type = struct_decl->fields[j].field_type;
                    fields[j].is_const = struct_decl->fields[j].is_const;  // 传递常量字段标记
                }
            }
            sym->type = cn_type_new_struct(struct_decl->name,
                                          struct_decl->name_length,
                                          fields,
                                          struct_decl->field_count,
                                          global_scope,
                                          NULL, 0);  // 全局结构体，无所属函数
        } else {
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, struct_decl->name);
        }
    }

    // 注册枚举声明到全局作用域
    for (i = 0; i < program->enum_count; ++i) {
        CnAstStmt *enum_stmt = program->enums[i];
        if (!enum_stmt || enum_stmt->kind != CN_AST_STMT_ENUM_DECL) {
            continue;
        }

        CnAstEnumDecl *enum_decl = &enum_stmt->as.enum_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(global_scope,
                                   enum_decl->name,
                                   enum_decl->name_length,
                                   CN_SEM_SYMBOL_ENUM);
        if (sym) {
            // 创建枚举类型
            sym->type = cn_type_new_enum(enum_decl->name, enum_decl->name_length);
            
            // 为枚举创建一个作用域来存储其成员
            CnSemScope *enum_scope = cn_sem_scope_new(CN_SEM_SCOPE_ENUM, global_scope);
            if (enum_scope && sym->type) {
                sym->type->as.enum_type.enum_scope = enum_scope;
                
                // 注册枚举成员到枚举作用域（而非全局作用域）
                for (size_t j = 0; j < enum_decl->member_count; j++) {
                    CnAstEnumMember *member = &enum_decl->members[j];
                    CnSemSymbol *member_sym = cn_sem_scope_insert_symbol(enum_scope,
                                                       member->name,
                                                       member->name_length,
                                                       CN_SEM_SYMBOL_ENUM_MEMBER);
                    if (member_sym) {
                        // 枚举成员的类型是整数
                        member_sym->type = cn_type_new_primitive(CN_TYPE_INT);
                        // 保存枚举成员的值
                        member_sym->as.enum_value = member->value;
                    } else {
                        // 报告重复定义
                        cn_support_diag_semantic_error_duplicate_symbol(
                            diagnostics, NULL, 0, 0, member->name);
                    }
                }
            }
        } else {
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, enum_decl->name);
        }
    }

    // 注册模块声明到全局作用域
    for (i = 0; i < program->module_count; ++i) {
        CnAstStmt *module_stmt = program->modules[i];
        if (!module_stmt || module_stmt->kind != CN_AST_STMT_MODULE_DECL) {
            continue;
        }

        CnAstModuleDecl *module_decl = &module_stmt->as.module_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(global_scope,
                                   module_decl->name,
                                   module_decl->name_length,
                                   CN_SEM_SYMBOL_MODULE);
        if (sym) {
            // 为模块创建一个新的作用域
            CnSemScope *module_scope = cn_sem_scope_new(CN_SEM_SCOPE_MODULE, global_scope);
            if (module_scope) {
                sym->as.module_scope = module_scope;
                // 构建模块内的符号
                cn_sem_build_module_scope(global_scope, module_stmt, diagnostics);
            }
            // 模块符号本身不需要类型
            sym->type = cn_type_new_primitive(CN_TYPE_VOID);
        } else {
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, module_decl->name);
        }
    }
    
    // 处理导入语句：将被导入模块的符号添加到全局作用域
    for (i = 0; i < program->import_count; ++i) {
        CnAstStmt *import_stmt = program->imports[i];
        if (!import_stmt || import_stmt->kind != CN_AST_STMT_IMPORT) {
            continue;
        }

        CnAstImportStmt *import = &import_stmt->as.import_stmt;
        
        // 查找被导入的模块
        CnSemSymbol *module_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                              import->module_name,
                                                              import->module_name_length);
        
        if (!module_sym) {
            // 模块不存在，报错
            cn_support_diag_semantic_error_generic(
                diagnostics,
                CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                NULL, 0, 0,
                "语义错误：导入的模块不存在");
            continue;
        }
        
        if (module_sym->kind != CN_SEM_SYMBOL_MODULE || !module_sym->as.module_scope) {
            // 符号不是模块，报错
            cn_support_diag_semantic_error_generic(
                diagnostics,
                CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                NULL, 0, 0,
                "语义错误：导入的符号不是模块");
            continue;
        }
        
        CnSemScope *module_scope = module_sym->as.module_scope;
        
        // 如果指定了别名，则以别名注册模块符号
        if (import->alias) {
            // 使用别名注册模块
            CnSemSymbol *existing_alias = cn_sem_scope_lookup_shallow(global_scope,
                                                                      import->alias,
                                                                      import->alias_length);
            if (existing_alias) {
                // 别名冲突，报错
                cn_support_diag_semantic_error_duplicate_symbol(
                    diagnostics, NULL, 0, 0, import->alias);
                continue;
            }
            
            // 以别名注册模块符号
            CnSemSymbol *alias_sym = cn_sem_scope_insert_symbol(global_scope,
                                                                import->alias,
                                                                import->alias_length,
                                                                CN_SEM_SYMBOL_MODULE);
            if (alias_sym) {
                alias_sym->type = module_sym->type;
                alias_sym->is_public = module_sym->is_public;
                alias_sym->is_const = module_sym->is_const;
                alias_sym->as.module_scope = module_sym->as.module_scope;
            }
            continue;  // 使用别名时不进行成员导入
        }
        
        // 判断是全量导入还是选择性导入
        if (import->member_count == 0) {
            // 全量导入：遍历模块作用域中的所有符号，添加到全局作用域
            CnSemSymbolNode *node = module_scope->symbols;
            while (node) {
                CnSemSymbol *sym = &node->symbol;
                
                // 检查名称冲突
                CnSemSymbol *existing_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                                        sym->name,
                                                                        sym->name_length);
                if (existing_sym && existing_sym->kind != CN_SEM_SYMBOL_MODULE) {
                    // 名称冲突，报错
                    cn_support_diag_semantic_error_duplicate_symbol(
                        diagnostics, NULL, 0, 0, sym->name);
                } else if (!existing_sym) {
                    // 没有冲突，添加符号
                    CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(global_scope,
                                                                      sym->name,
                                                                      sym->name_length,
                                                                      sym->kind);
                    if (new_sym) {
                        // 复制符号信息
                        new_sym->type = sym->type;
                        new_sym->is_public = sym->is_public;
                        new_sym->is_const = sym->is_const;
                        new_sym->decl_scope = sym->decl_scope;
                        if (sym->kind == CN_SEM_SYMBOL_MODULE) {
                            new_sym->as.module_scope = sym->as.module_scope;
                        } else if (sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                            new_sym->as.enum_value = sym->as.enum_value;
                        }
                    }
                }
                
                node = node->next;
            }
        } else {
            // 选择性导入：只导入指定的成员
            for (size_t j = 0; j < import->member_count; ++j) {
                const char *member_name = import->members[j].name;
                size_t member_name_length = import->members[j].name_length;
                
                // 在模块作用域中查找指定成员
                CnSemSymbol *member_sym = cn_sem_scope_lookup_shallow(module_scope,
                                                                      member_name,
                                                                      member_name_length);
                
                if (!member_sym) {
                    // 成员不存在，报错
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                        NULL, 0, 0,
                        "语义错误：导入的成员不存在");
                    continue;
                }
                
                // 检查可见性：只能导入公开成员
                if (!member_sym->is_public) {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_ACCESS_DENIED,
                        NULL, 0, 0,
                        "语义错误：无法导入私有成员");
                    continue;
                }
                
                // 检查名称冲突
                CnSemSymbol *existing_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                                        member_name,
                                                                        member_name_length);
                if (existing_sym && existing_sym->kind != CN_SEM_SYMBOL_MODULE) {
                    // 名称冲突，报错
                    cn_support_diag_semantic_error_duplicate_symbol(
                        diagnostics, NULL, 0, 0, member_name);
                } else if (!existing_sym) {
                    // 没有冲突，添加符号
                    CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(global_scope,
                                                                      member_name,
                                                                      member_name_length,
                                                                      member_sym->kind);
                    if (new_sym) {
                        // 复制符号信息
                        new_sym->type = member_sym->type;
                        new_sym->is_public = member_sym->is_public;
                        new_sym->decl_scope = member_sym->decl_scope;
                        if (member_sym->kind == CN_SEM_SYMBOL_MODULE) {
                            new_sym->as.module_scope = member_sym->as.module_scope;
                        } else if (member_sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                            new_sym->as.enum_value = member_sym->as.enum_value;
                        }
                    }
                }
            }
        }
    }

    // 注册全局变量声明到全局作用域
    for (i = 0; i < program->global_var_count; ++i) {
        CnAstStmt *var_stmt = program->global_vars[i];
        if (!var_stmt || var_stmt->kind != CN_AST_STMT_VAR_DECL) {
            continue;
        }

        CnAstVarDecl *var_decl = &var_stmt->as.var_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(global_scope,
                                   var_decl->name,
                                   var_decl->name_length,
                                   CN_SEM_SYMBOL_VARIABLE);
        if (sym) {
            sym->type = var_decl->declared_type;
            sym->is_const = var_decl->is_const;
        } else {
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, var_decl->name);
        }
    }

    for (i = 0; i < program->function_count; ++i) {
        CnAstFunctionDecl *function_decl = program->functions[i];

        if (!function_decl) {
            continue;
        }

        CnSemSymbol *sym = cn_sem_scope_insert_symbol(global_scope,
                                   function_decl->name,
                                   function_decl->name_length,
                                   CN_SEM_SYMBOL_FUNCTION);
        if (sym) {
            // 构建完整的函数类型
            CnType **param_types = NULL;
            if (function_decl->parameter_count > 0) {
                param_types = (CnType **)malloc(sizeof(CnType *) * function_decl->parameter_count);
                for (size_t j = 0; j < function_decl->parameter_count; j++) {
                    param_types[j] = function_decl->parameters[j].declared_type;
                }
            }
            // TODO: 当前返回类型需要通过return语句推断，暂时使用UNKNOWN
            // 后续在类型检查阶段会通过分析return语句来补充返回类型
            sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_UNKNOWN),
                                            param_types,
                                            function_decl->parameter_count);
        } else {
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, function_decl->name);
        }

        cn_sem_build_function_scope(global_scope, function_decl, diagnostics);
    }

    return global_scope;
}

static void cn_sem_build_function_scope(CnSemScope *parent_scope,
                                        CnAstFunctionDecl *function_decl,
                                        CnDiagnostics *diagnostics)
{
    CnSemScope *function_scope;
    size_t i;

    if (!parent_scope || !function_decl) {
        return;
    }

    function_scope = cn_sem_scope_new(CN_SEM_SCOPE_FUNCTION, parent_scope);
    if (!function_scope) {
        return;
    }
    
    // 设置函数作用域的名称为函数名
    cn_sem_scope_set_name(function_scope, function_decl->name, function_decl->name_length);

    for (i = 0; i < function_decl->parameter_count; ++i) {
        CnAstParameter *param = &function_decl->parameters[i];

        CnSemSymbol *sym = cn_sem_scope_insert_symbol(function_scope,
                                   param->name,
                                   param->name_length,
                                   CN_SEM_SYMBOL_VARIABLE);
        if (sym) {
            sym->type = param->declared_type;
            sym->is_const = param->is_const;  // 传递常量参数标记
        } else {
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, param->name);
        }
    }

    cn_sem_build_block_scope(function_scope, function_decl->body, diagnostics);
}

static void cn_sem_build_block_scope(CnSemScope *parent_scope,
                                     CnAstBlockStmt *block,
                                     CnDiagnostics *diagnostics)
{
    CnSemScope *block_scope;
    size_t i;

    if (!parent_scope || !block) {
        return;
    }

    block_scope = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, parent_scope);
    if (!block_scope) {
        return;
    }

    for (i = 0; i < block->stmt_count; ++i) {
        cn_sem_build_stmt(block_scope, block->stmts[i], diagnostics);
    }
}

static void cn_sem_build_stmt(CnSemScope *scope, CnAstStmt *stmt, CnDiagnostics *diagnostics)
{
    if (!scope || !stmt) {
        return;
    }

    switch (stmt->kind) {
    case CN_AST_STMT_BLOCK:
        cn_sem_build_block_scope(scope, stmt->as.block, diagnostics);
        break;
    case CN_AST_STMT_VAR_DECL: {
        CnAstVarDecl *var_decl = &stmt->as.var_decl;

        CnSemSymbol *sym = cn_sem_scope_insert_symbol(scope,
                                   var_decl->name,
                                   var_decl->name_length,
                                   CN_SEM_SYMBOL_VARIABLE);
        if (sym) {
            // 如果声明类型是结构体,需要从符号表查找真实的结构体定义(含有正确的decl_scope)
            if (var_decl->declared_type && var_decl->declared_type->kind == CN_TYPE_STRUCT) {
                // 从符号表中查找结构体类型定义
                CnSemSymbol *struct_sym = cn_sem_scope_lookup(scope,
                                        var_decl->declared_type->as.struct_type.name,
                                        var_decl->declared_type->as.struct_type.name_length);
                if (struct_sym && struct_sym->kind == CN_SEM_SYMBOL_STRUCT && struct_sym->type) {
                    // 使用从符号表查找到的结构体类型(含有正确的decl_scope)
                    sym->type = struct_sym->type;
                } else {
                    // 找不到结构体定义,使用原始类型
                    sym->type = var_decl->declared_type;
                }
            } else {
                // 非结构体类型,直接使用
                sym->type = var_decl->declared_type;
            }
            sym->is_const = var_decl->is_const;
            // 设置可见性（根据 AST 中的可见性标志）
            if (var_decl->visibility == CN_VISIBILITY_PUBLIC) {
                sym->is_public = 1;  // 显式标记为公开
            } else if (var_decl->visibility == CN_VISIBILITY_PRIVATE) {
                sym->is_public = 0;  // 显式标记为私有
            }
            // CN_VISIBILITY_DEFAULT 保持默认值（已在 symbol_table.c 中设置为 0，即私有）
        } else {
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, var_decl->name);
        }

        cn_sem_build_expr(scope, var_decl->initializer, diagnostics);
        break;
    }
    case CN_AST_STMT_EXPR:
        cn_sem_build_expr(scope, stmt->as.expr.expr, diagnostics);
        break;
    case CN_AST_STMT_RETURN:
        cn_sem_build_expr(scope, stmt->as.return_stmt.expr, diagnostics);
        break;
    case CN_AST_STMT_IF:
        cn_sem_build_if_stmt(scope, &stmt->as.if_stmt, diagnostics);
        break;
    case CN_AST_STMT_WHILE:
        cn_sem_build_while_stmt(scope, &stmt->as.while_stmt, diagnostics);
        break;
    case CN_AST_STMT_FOR:
        cn_sem_build_for_stmt(scope, &stmt->as.for_stmt, diagnostics);
        break;
    case CN_AST_STMT_SWITCH: {
        // 解析 switch 表达式
        cn_sem_build_expr(scope, stmt->as.switch_stmt.expr, diagnostics);
        // 解析每个 case/default 分支
        for (size_t i = 0; i < stmt->as.switch_stmt.case_count; i++) {
            CnAstSwitchCase *case_stmt = &stmt->as.switch_stmt.cases[i];
            // 解析 case 值表达式（如果有）
            cn_sem_build_expr(scope, case_stmt->value, diagnostics);
            // 解析 case 体
            cn_sem_build_block_scope(scope, case_stmt->body, diagnostics);
        }
        break;
    }
    case CN_AST_STMT_BREAK:
    case CN_AST_STMT_CONTINUE:
        break;
    case CN_AST_STMT_STRUCT_DECL: {
        // 局部结构体定义：注册到当前作用域
        CnAstStructDecl *struct_decl = &stmt->as.struct_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(scope,
                                   struct_decl->name,
                                   struct_decl->name_length,
                                   CN_SEM_SYMBOL_STRUCT);
        if (sym) {
            // 创建结构体类型，包含字段信息
            CnStructField *fields = NULL;
            if (struct_decl->field_count > 0) {
                fields = (CnStructField *)malloc(sizeof(CnStructField) * struct_decl->field_count);
                for (size_t j = 0; j < struct_decl->field_count; j++) {
                    fields[j].name = struct_decl->fields[j].name;
                    fields[j].name_length = struct_decl->fields[j].name_length;
                    fields[j].field_type = struct_decl->fields[j].field_type;
                    fields[j].is_const = struct_decl->fields[j].is_const;
                }
            }
            
            // 局部结构体应该绑定到函数作用域,向上查找直到函数作用域
            CnSemScope *decl_scope = scope;
            while (decl_scope && cn_sem_scope_get_kind(decl_scope) != CN_SEM_SCOPE_FUNCTION 
                   && cn_sem_scope_get_kind(decl_scope) != CN_SEM_SCOPE_GLOBAL
                   && cn_sem_scope_get_kind(decl_scope) != CN_SEM_SCOPE_MODULE) {
                decl_scope = cn_sem_scope_parent(decl_scope);
            }
            
            // 获取函数名(如果是局部结构体)
            const char *owner_func_name = NULL;
            size_t owner_func_name_length = 0;
            if (decl_scope && cn_sem_scope_get_kind(decl_scope) == CN_SEM_SCOPE_FUNCTION) {
                owner_func_name = cn_sem_scope_get_name(decl_scope, &owner_func_name_length);
            }
            
            sym->type = cn_type_new_struct(struct_decl->name,
                                          struct_decl->name_length,
                                          fields,
                                          struct_decl->field_count,
                                          decl_scope,
                                          owner_func_name,
                                          owner_func_name_length);
        } else {
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, struct_decl->name);
        }
        break;
    }
    case CN_AST_STMT_ENUM_DECL: {
        // 局部枚举定义：注册到当前作用域
        CnAstEnumDecl *enum_decl = &stmt->as.enum_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(scope,
                                   enum_decl->name,
                                   enum_decl->name_length,
                                   CN_SEM_SYMBOL_ENUM);
        if (sym) {
            // 创建枚举类型
            sym->type = cn_type_new_enum(enum_decl->name, enum_decl->name_length);
            
            // 为枚举创建一个作用域来存储其成员
            CnSemScope *enum_scope = cn_sem_scope_new(CN_SEM_SCOPE_ENUM, scope);
            if (enum_scope && sym->type) {
                sym->type->as.enum_type.enum_scope = enum_scope;
                
                // 注册枚举成员到枚举作用域
                for (size_t j = 0; j < enum_decl->member_count; j++) {
                    CnAstEnumMember *member = &enum_decl->members[j];
                    CnSemSymbol *member_sym = cn_sem_scope_insert_symbol(enum_scope,
                                                       member->name,
                                                       member->name_length,
                                                       CN_SEM_SYMBOL_ENUM_MEMBER);
                    if (member_sym) {
                        // 枚举成员的类型是整数
                        member_sym->type = cn_type_new_primitive(CN_TYPE_INT);
                        // 保存枚举成员的值
                        member_sym->as.enum_value = member->value;
                    } else {
                        // 报告重复定义
                        cn_support_diag_semantic_error_duplicate_symbol(
                            diagnostics, NULL, 0, 0, member->name);
                    }
                }
            }
        } else {
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, enum_decl->name);
        }
        break;
    }
    case CN_AST_STMT_MODULE_DECL:
        // 模块声明已在全局作用域构建时处理，这里不需要额外操作
        break;
    case CN_AST_STMT_IMPORT:
        // 导入语句在后续的名称解析阶段处理
        break;
    }
}

static void cn_sem_build_if_stmt(CnSemScope *scope, CnAstIfStmt *if_stmt, CnDiagnostics *diagnostics)
{
    if (!scope || !if_stmt) {
        return;
    }

    cn_sem_build_expr(scope, if_stmt->condition, diagnostics);
    cn_sem_build_block_scope(scope, if_stmt->then_block, diagnostics);
    cn_sem_build_block_scope(scope, if_stmt->else_block, diagnostics);
}

static void cn_sem_build_while_stmt(CnSemScope *scope, CnAstWhileStmt *while_stmt, CnDiagnostics *diagnostics)
{
    if (!scope || !while_stmt) {
        return;
    }

    cn_sem_build_expr(scope, while_stmt->condition, diagnostics);
    cn_sem_build_block_scope(scope, while_stmt->body, diagnostics);
}

static void cn_sem_build_for_stmt(CnSemScope *scope, CnAstForStmt *for_stmt, CnDiagnostics *diagnostics)
{
    CnSemScope *for_scope;

    if (!scope || !for_stmt) {
        return;
    }

    for_scope = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, scope);
    if (!for_scope) {
        return;
    }

    cn_sem_build_stmt(for_scope, for_stmt->init, diagnostics);
    cn_sem_build_expr(for_scope, for_stmt->condition, diagnostics);
    cn_sem_build_expr(for_scope, for_stmt->update, diagnostics);
    cn_sem_build_block_scope(for_scope, for_stmt->body, diagnostics);
}

// 构建模块作用域
static void cn_sem_build_module_scope(CnSemScope *parent_scope,
                                      CnAstStmt *module_stmt,
                                      CnDiagnostics *diagnostics)
{
    if (!parent_scope || !module_stmt || module_stmt->kind != CN_AST_STMT_MODULE_DECL) {
        return;
    }

    CnAstModuleDecl *module_decl = &module_stmt->as.module_decl;
    
    // 获取模块符号
    CnSemSymbol *module_sym = cn_sem_scope_lookup_shallow(parent_scope,
                                                          module_decl->name,
                                                          module_decl->name_length);
    if (!module_sym || module_sym->kind != CN_SEM_SYMBOL_MODULE) {
        return;
    }

    CnSemScope *module_scope = module_sym->as.module_scope;
    if (!module_scope) {
        return;
    }

    // 首先注册模块变量，然后注册模块函数，最后构建函数作用域
    // 这确保在构建函数作用域时，模块内的所有成员已经可见

    // 步骤1：注册模块变量
    for (size_t i = 0; i < module_decl->stmt_count; ++i) {
        CnAstStmt *stmt = module_decl->stmts[i];
        if (!stmt || stmt->kind != CN_AST_STMT_VAR_DECL) {
            continue;
        }

        CnAstVarDecl *var_decl = &stmt->as.var_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(module_scope,
                                   var_decl->name,
                                   var_decl->name_length,
                                   CN_SEM_SYMBOL_VARIABLE);
        if (sym) {
            sym->type = var_decl->declared_type;
            sym->is_const = var_decl->is_const;
            // 设置可见性（模块成员）
            if (var_decl->visibility == CN_VISIBILITY_PUBLIC) {
                sym->is_public = 1;  // 显式标记为公开
            } else {
                // CN_VISIBILITY_PRIVATE 和 CN_VISIBILITY_DEFAULT: 默认为公开（临时策略，方便测试）
                // TODO: 在实现“公开”/“私有”关键字后，需要改为正确的可见性处理
                sym->is_public = 1;
            }
        } else {
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, var_decl->name);
        }
    }

    // 步骤2：注册模块函数
    for (size_t i = 0; i < module_decl->function_count; ++i) {
        CnAstFunctionDecl *function_decl = module_decl->functions[i];
        if (!function_decl) {
            continue;
        }

        CnSemSymbol *sym = cn_sem_scope_insert_symbol(module_scope,
                                   function_decl->name,
                                   function_decl->name_length,
                                   CN_SEM_SYMBOL_FUNCTION);
        if (sym) {
            CnType **param_types = NULL;
            if (function_decl->parameter_count > 0) {
                param_types = (CnType **)malloc(sizeof(CnType *) * function_decl->parameter_count);
                for (size_t j = 0; j < function_decl->parameter_count; j++) {
                    param_types[j] = function_decl->parameters[j].declared_type;
                }
            }
            sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_UNKNOWN),
                                            param_types,
                                            function_decl->parameter_count);
            // 注意：由于 CnAstFunctionDecl 没有 visibility 字段，我们默认模块函数为公开
            // 这是因为 parser 在解析时使用了 current_visibility，但没有将其保存到 FunctionDecl 中
            // 在实际使用中，如果需要私有函数，应该在 parser 中添加 visibility 字段
            sym->is_public = 1;
        } else {
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, function_decl->name);
        }
    }

    // 步骤3：构建函数作用域
    for (size_t i = 0; i < module_decl->function_count; ++i) {
        CnAstFunctionDecl *function_decl = module_decl->functions[i];
        if (!function_decl) {
            continue;
        }
        cn_sem_build_function_scope(module_scope, function_decl, diagnostics);
    }

    // 步骤4：处理模块变量的初始化表达式
    for (size_t i = 0; i < module_decl->stmt_count; ++i) {
        CnAstStmt *stmt = module_decl->stmts[i];
        if (!stmt || stmt->kind != CN_AST_STMT_VAR_DECL) {
            continue;
        }

        CnAstVarDecl *var_decl = &stmt->as.var_decl;
        cn_sem_build_expr(module_scope, var_decl->initializer, diagnostics);
    }
}

static void cn_sem_build_expr(CnSemScope *scope, CnAstExpr *expr, CnDiagnostics *diagnostics)
{
    size_t i;

    if (!scope || !expr) {
        return;
    }

    switch (expr->kind) {
    case CN_AST_EXPR_BINARY:
        cn_sem_build_expr(scope, expr->as.binary.left, diagnostics);
        cn_sem_build_expr(scope, expr->as.binary.right, diagnostics);
        break;
    case CN_AST_EXPR_CALL:
        cn_sem_build_expr(scope, expr->as.call.callee, diagnostics);
        for (i = 0; i < expr->as.call.argument_count; ++i) {
            cn_sem_build_expr(scope, expr->as.call.arguments[i], diagnostics);
        }
        break;
    case CN_AST_EXPR_IDENTIFIER:
    case CN_AST_EXPR_INTEGER_LITERAL:
    case CN_AST_EXPR_STRING_LITERAL:
        break;
    case CN_AST_EXPR_ASSIGN:
        cn_sem_build_expr(scope, expr->as.assign.target, diagnostics);
        cn_sem_build_expr(scope, expr->as.assign.value, diagnostics);
        break;
    case CN_AST_EXPR_LOGICAL:
        cn_sem_build_expr(scope, expr->as.logical.left, diagnostics);
        cn_sem_build_expr(scope, expr->as.logical.right, diagnostics);
        break;
    case CN_AST_EXPR_UNARY:
        cn_sem_build_expr(scope, expr->as.unary.operand, diagnostics);
        break;
    case CN_AST_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expr->as.array_literal.element_count; ++i) {
            cn_sem_build_expr(scope, expr->as.array_literal.elements[i], diagnostics);
        }
        break;
    case CN_AST_EXPR_INDEX:
        cn_sem_build_expr(scope, expr->as.index.array, diagnostics);
        cn_sem_build_expr(scope, expr->as.index.index, diagnostics);
        break;
    case CN_AST_EXPR_MEMBER_ACCESS:
        cn_sem_build_expr(scope, expr->as.member.object, diagnostics);
        break;
    case CN_AST_EXPR_STRUCT_LITERAL:
        for (i = 0; i < expr->as.struct_lit.field_count; ++i) {
            cn_sem_build_expr(scope, expr->as.struct_lit.fields[i].value, diagnostics);
        }
        break;
    }
}
