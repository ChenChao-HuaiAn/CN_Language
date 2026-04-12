#include "cnlang/ir/irgen.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/frontend/ast/class_node.h"  // 类AST节点定义
#include "cnlang/semantics/vtable_builder.h" // 虚函数表支持
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 符号链表节点（与 symbol_table.c 中的定义保持一致）
typedef struct CnSemSymbolNode {
    CnSemSymbol symbol;
    struct CnSemSymbolNode *next;
} CnSemSymbolNode;

// 作用域结构体的内部实现（与 symbol_table.c 中的定义保持一致）
struct CnSemScope {
    CnSemScopeKind kind;
    CnSemScope *parent;
    CnSemSymbolNode *symbols;
};

// 生成新的虚拟寄存器
static int alloc_reg(CnIrGenContext *ctx) {
    return ctx->current_func->next_reg_id++;
}

// 判断类型是否为整数类型
static bool is_integer_type(CnType *type) {
    if (!type) return false;
    return (type->kind == CN_TYPE_INT || 
            type->kind == CN_TYPE_INT32 || 
            type->kind == CN_TYPE_INT64 || 
            type->kind == CN_TYPE_UINT32 || 
            type->kind == CN_TYPE_UINT64 || 
            type->kind == CN_TYPE_UINT64_LL);
}

// 判断类型是否为浮点类型
static bool is_float_type(CnType *type) {
    if (!type) return false;
    return (type->kind == CN_TYPE_FLOAT || 
            type->kind == CN_TYPE_FLOAT32 || 
            type->kind == CN_TYPE_FLOAT64);
}

// 生成唯一的基本块名称
static char *make_block_name(CnIrGenContext *ctx, const char *hint) {
    static int block_counter = 0;
    char buf[64];
    snprintf(buf, sizeof(buf), "%s_%d", hint, block_counter++);
    return strdup(buf);
}

// 向当前基本块添加指令
static void emit(CnIrGenContext *ctx, CnIrInst *inst) {
    cn_ir_basic_block_add_inst(ctx->current_block, inst);
}

// 切换到新的基本块
static void switch_to_block(CnIrGenContext *ctx, CnIrBasicBlock *block) {
    ctx->current_block = block;
}

// 静态变量跟踪结构
typedef struct CnIrGenStaticVar {
    char *name;
    size_t name_length;
    struct CnIrGenStaticVar *next;
} CnIrGenStaticVar;

// 用于查找枚举类型符号的回调上下文
typedef struct EnumLookupContext {
    CnSemScope *target_enum_scope;  // 目标枚举作用域
    const char *enum_name;          // 找到的枚举类型名
    size_t enum_name_len;           // 枚举类型名长度
} EnumLookupContext;

// 查找枚举类型符号的回调函数
static void find_enum_symbol_callback(CnSemSymbol *symbol, void *user_data) {
    EnumLookupContext *ctx = (EnumLookupContext *)user_data;
    // 如果已经找到，直接返回
    if (ctx->enum_name) return;
    
    // 检查是否是枚举类型符号，且其枚举作用域匹配目标
    if (symbol->kind == CN_SEM_SYMBOL_ENUM &&
        symbol->type &&
        symbol->type->kind == CN_TYPE_ENUM &&
        symbol->type->as.enum_type.enum_scope == ctx->target_enum_scope) {
        ctx->enum_name = symbol->name;
        ctx->enum_name_len = symbol->name_length;
    }
}

// 局部变量名映射节点（用于变量名唯一化）
typedef struct CnIrLocalVarEntry {
    char *original_name;      // 原始变量名
    char *unique_name;        // 唯一变量名
    struct CnIrLocalVarEntry *next;
} CnIrLocalVarEntry;

// 局部变量名唯一化计数器（每个函数重置）
static int s_local_var_counter = 0;

// 重置局部变量计数器（在每个函数开始时调用）
static void reset_local_var_counter(void) {
    s_local_var_counter = 0;
}

// 生成唯一的局部变量名
// 格式：原名_序号（如 i_0, i_1, j_0 等）
static char *make_unique_local_name(const char *original_name) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s_%d", original_name, s_local_var_counter++);
    return strdup(buf);
}

// 查找局部变量的唯一名称
// 返回：如果找到返回唯一名称（需要调用者释放），否则返回 NULL
static char *lookup_local_var_unique_name(CnIrGenContext *ctx, const char *original_name) {
    CnIrLocalVarEntry *entry = ctx->local_var_map;
    while (entry) {
        if (strcmp(entry->original_name, original_name) == 0) {
            return strdup(entry->unique_name);
        }
        entry = entry->next;
    }
    return NULL;
}

// 添加局部变量名映射
static void add_local_var_mapping(CnIrGenContext *ctx, const char *original_name, const char *unique_name) {
    CnIrLocalVarEntry *entry = (CnIrLocalVarEntry *)malloc(sizeof(CnIrLocalVarEntry));
    if (entry) {
        entry->original_name = strdup(original_name);
        entry->unique_name = strdup(unique_name);
        entry->next = ctx->local_var_map;
        ctx->local_var_map = entry;
    }
}

// 释放局部变量映射表
static void free_local_var_map(CnIrGenContext *ctx) {
    CnIrLocalVarEntry *entry = ctx->local_var_map;
    while (entry) {
        CnIrLocalVarEntry *next = entry->next;
        free(entry->original_name);
        free(entry->unique_name);
        free(entry);
        entry = next;
    }
    ctx->local_var_map = NULL;
}

CnIrGenContext *cn_ir_gen_context_new() {
    CnIrGenContext *ctx = malloc(sizeof(CnIrGenContext));
    ctx->module = cn_ir_module_new();
    ctx->current_func = NULL;
    ctx->current_block = NULL;
    ctx->loop_exit = NULL;
    ctx->loop_continue = NULL;
    ctx->global_scope = NULL;
    ctx->current_scope = NULL;
    ctx->current_static_vars = NULL;  // 初始化静态变量列表
    ctx->local_var_map = NULL;        // 初始化局部变量映射表
    return ctx;
}

void cn_ir_gen_context_free(CnIrGenContext *ctx) {
    if (ctx) {
        free_local_var_map(ctx);
        free(ctx);
    }
}

// 映射 AST 二元运算符到 IR 指令类型
static CnIrInstKind binary_op_to_ir(CnAstBinaryOp op) {
    switch (op) {
        case CN_AST_BINARY_OP_ADD: return CN_IR_INST_ADD;
        case CN_AST_BINARY_OP_SUB: return CN_IR_INST_SUB;
        case CN_AST_BINARY_OP_MUL: return CN_IR_INST_MUL;
        case CN_AST_BINARY_OP_DIV: return CN_IR_INST_DIV;
        case CN_AST_BINARY_OP_MOD: return CN_IR_INST_MOD;  // 取模运算
        case CN_AST_BINARY_OP_EQ:  return CN_IR_INST_EQ;
        case CN_AST_BINARY_OP_NE:  return CN_IR_INST_NE;
        case CN_AST_BINARY_OP_LT:  return CN_IR_INST_LT;
        case CN_AST_BINARY_OP_GT:  return CN_IR_INST_GT;
        case CN_AST_BINARY_OP_LE:  return CN_IR_INST_LE;
        case CN_AST_BINARY_OP_GE:  return CN_IR_INST_GE;
        // 位运算符
        case CN_AST_BINARY_OP_BITWISE_AND: return CN_IR_INST_AND;
        case CN_AST_BINARY_OP_BITWISE_OR:  return CN_IR_INST_OR;
        case CN_AST_BINARY_OP_BITWISE_XOR: return CN_IR_INST_XOR;
        case CN_AST_BINARY_OP_LEFT_SHIFT:  return CN_IR_INST_SHL;
        case CN_AST_BINARY_OP_RIGHT_SHIFT: return CN_IR_INST_SHR;
        default: return CN_IR_INST_ADD;
    }
}

// 辅助函数：从 AST 标识符生成 null-terminated 字符串
static char *copy_name(const char *name, size_t length) {
    char *buf = malloc(length + 1);
    memcpy(buf, name, length);
    buf[length] = '\0';
    return buf;
}

// 生成表达式的 IR，返回结果操作数
CnIrOperand cn_ir_gen_expr(CnIrGenContext *ctx, CnAstExpr *expr) {
    if (!expr) return cn_ir_op_none();

    switch (expr->kind) {
        case CN_AST_EXPR_INTEGER_LITERAL: {
            // 整数字面量直接返回立即数
            return cn_ir_op_imm_int(expr->as.integer_literal.value,
                                    cn_type_new_primitive(CN_TYPE_INT));
        }
        case CN_AST_EXPR_FLOAT_LITERAL: {
            // 浮点数字面量直接返回立即数
            return cn_ir_op_imm_float(expr->as.float_literal.value,
                                      cn_type_new_primitive(CN_TYPE_FLOAT));
        }
        case CN_AST_EXPR_STRING_LITERAL: {
            // 字符串字面量
            CnIrOperand op;
            op.kind = CN_IR_OP_IMM_STR;
            char *str = copy_name(expr->as.string_literal.value, expr->as.string_literal.length);
            op.as.imm_str = str; // 注意：IR 目前不负责释放这个字符串，可能存在泄露，阶段 3 暂不处理
            op.type = cn_type_new_primitive(CN_TYPE_STRING);
            return op;
        }
        case CN_AST_EXPR_CHAR_LITERAL: {
            // 字符字面量：作为整数值处理
            return cn_ir_op_imm_int(expr->as.char_literal.value,
                                    cn_type_new_primitive(CN_TYPE_INT));
        }
        case CN_AST_EXPR_BOOL_LITERAL: {
            // 布尔字面量直接返回立即数（0 或 1）
            return cn_ir_op_imm_int(expr->as.bool_literal.value ? 1 : 0,
                                    cn_type_new_primitive(CN_TYPE_BOOL));
        }
        case CN_AST_EXPR_IDENTIFIER: {
            // 标识符：首先检查是否为局部变量或参数（优先级最高）
            // 这样可以避免枚举成员覆盖同名的局部变量/参数
            
            // 检查是否为静态局部变量（通过上下文中的静态变量列表）
            CnIrGenStaticVar *static_var = ctx->current_static_vars;
            while (static_var) {
                if (static_var->name_length == expr->as.identifier.name_length &&
                    strncmp(static_var->name, expr->as.identifier.name, expr->as.identifier.name_length) == 0) {
                    // 找到静态变量：生成带函数名前缀的符号名
                    // 格式：cn_static_{函数名}_{变量名}
                    size_t func_name_len = strlen(ctx->current_func->name);
                    size_t var_name_len = expr->as.identifier.name_length;
                    size_t total_len = 10 + func_name_len + 1 + var_name_len + 1; // "cn_static_" + "_" + \0
                    
                    char *static_name = malloc(total_len);
                    if (static_name) {
                        snprintf(static_name, total_len, "cn_static_%s_%.*s",
                                ctx->current_func->name,
                                (int)var_name_len, expr->as.identifier.name);
                        
                        int dest_reg = alloc_reg(ctx);
                        CnIrOperand dest = cn_ir_op_reg(dest_reg, expr->type);
                        CnIrOperand src = cn_ir_op_symbol(static_name, expr->type);
                        free(static_name);
                        emit(ctx, cn_ir_inst_new(CN_IR_INST_LOAD, dest, src, cn_ir_op_none()));
                        return dest;
                    }
                }
                static_var = static_var->next;
            }
            
            // 查找局部变量的唯一名称
            char *name = copy_name(expr->as.identifier.name, expr->as.identifier.name_length);
            char *unique_name = lookup_local_var_unique_name(ctx, name);
            if (unique_name) {
                // 找到局部变量映射，生成LOAD指令
                int dest_reg = alloc_reg(ctx);
                
                // 获取变量类型
                CnType *var_type = expr->type;
                if (!var_type && ctx->current_scope) {
                    CnSemSymbol *sym = cn_sem_scope_lookup(ctx->current_scope,
                                                           expr->as.identifier.name,
                                                           expr->as.identifier.name_length);
                    if (sym && sym->type) {
                        var_type = sym->type;
                    }
                }
                
                CnIrOperand dest = cn_ir_op_reg(dest_reg, var_type);
                CnIrOperand src = cn_ir_op_symbol(unique_name, var_type);
                free(unique_name);
                free(name);
                emit(ctx, cn_ir_inst_new(CN_IR_INST_LOAD, dest, src, cn_ir_op_none()));
                return dest;
            }
            
            // 检查符号表中的符号类型
            if (ctx->current_scope) {
                CnSemSymbol *sym = cn_sem_scope_lookup(ctx->current_scope,
                                                       expr->as.identifier.name,
                                                       expr->as.identifier.name_length);
                
                // 只有当符号是枚举成员且不是变量时才返回枚举值
                // 注意：枚举成员符号的 kind 是 CN_SEM_SYMBOL_ENUM_MEMBER
                // 而变量符号的 kind 是 CN_SEM_SYMBOL_VAR 或 CN_SEM_SYMBOL_PARAM
                if (sym && sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                    // 枚举成员：生成 "枚举类型名_成员名" 格式的符号名
                    // 需要找到枚举类型名称
                    const char *enum_name = NULL;
                    size_t enum_name_len = 0;
                    
                    // 方法1：从符号的类型信息获取枚举类型名称
                    // 枚举成员的 type 应该是枚举类型
                    if (sym->type && sym->type->kind == CN_TYPE_ENUM) {
                        enum_name = sym->type->as.enum_type.name;
                        enum_name_len = sym->type->as.enum_type.name_length;
                    }
                    
                    // 方法2：从表达式的类型信息获取（如果语义分析器设置了正确的类型）
                    if (!enum_name && expr->type && expr->type->kind == CN_TYPE_ENUM) {
                        enum_name = expr->type->as.enum_type.name;
                        enum_name_len = expr->type->as.enum_type.name_length;
                    }
                    
                    // 方法3：从符号的声明作用域获取枚举类型信息
                    if (!enum_name && sym->decl_scope && sym->decl_scope->kind == CN_SEM_SCOPE_ENUM) {
                        // 枚举作用域的父作用域应该包含枚举类型符号
                        CnSemScope *parent_scope = cn_sem_scope_parent(sym->decl_scope);
                        if (parent_scope) {
                            // 使用回调函数遍历父作用域查找枚举类型符号
                            EnumLookupContext lookup_ctx = {
                                .target_enum_scope = sym->decl_scope,
                                .enum_name = NULL,
                                .enum_name_len = 0
                            };
                            cn_sem_scope_foreach_symbol(parent_scope, find_enum_symbol_callback, &lookup_ctx);
                            if (lookup_ctx.enum_name) {
                                enum_name = lookup_ctx.enum_name;
                                enum_name_len = lookup_ctx.enum_name_len;
                            }
                        }
                    }
                    
                    // 方法2：从表达式的类型信息获取（如果语义分析器设置了正确的类型）
                    if (!enum_name && expr->type && expr->type->kind == CN_TYPE_ENUM) {
                        enum_name = expr->type->as.enum_type.name;
                        enum_name_len = expr->type->as.enum_type.name_length;
                    }
                    
                    // 方法3：检查符号名是否已经是 "枚举类型名_成员名" 格式
                    if (!enum_name) {
                        // 检查符号名是否包含下划线
                        const char *underscore = memchr(sym->name, '_', sym->name_length);
                        if (underscore) {
                            // 符号名已经是正确格式，使用 copy_name 正确复制
                            char *sym_name = copy_name(sym->name, sym->name_length);
                            free(name);
                            CnIrOperand result = cn_ir_op_symbol(sym_name, expr->type);
                            free(sym_name);
                            return result;
                        }
                    }
                    
                    // 生成 "枚举类型名_成员名" 格式的符号名
                    if (enum_name && enum_name_len > 0) {
                        size_t member_name_len = sym->name_length;
                        size_t total_len = enum_name_len + 1 + member_name_len + 1; // "_" + "\0"
                        char *qualified_name = malloc(total_len);
                        if (qualified_name) {
                            snprintf(qualified_name, total_len, "%.*s_%.*s",
                                    (int)enum_name_len, enum_name,
                                    (int)member_name_len, sym->name);
                            free(name);
                            CnIrOperand result = cn_ir_op_symbol(qualified_name, expr->type);
                            free(qualified_name);
                            return result;
                        }
                    }
                    
                    // 回退：使用 copy_name 正确复制符号名
                    char *sym_name = copy_name(sym->name, sym->name_length);
                    free(name);
                    CnIrOperand result = cn_ir_op_symbol(sym_name, expr->type);
                    free(sym_name);
                    return result;
                }
                
                // 检查是否来自模块：如果 decl_scope 是模块作用域，生成带模块前缀的名称
                if (sym && sym->decl_scope && sym->decl_scope->kind == CN_SEM_SCOPE_MODULE) {
                    // 需要找到模块名：遍历全局作用域找到拥有该模块作用域的模块符号
                    // 这里使用简化的方法：直接从模块作用域的父作用域查找
                    CnSemScope *global = ctx->current_scope;
                    while (global && global->parent) {
                        global = global->parent;
                    }
                    
                    if (global) {
                        // 在全局作用域中查找拥有该模块作用域的模块
                        CnSemSymbolNode *node = global->symbols;
                        while (node) {
                            if (node->symbol.kind == CN_SEM_SYMBOL_MODULE && 
                                node->symbol.as.module_scope == sym->decl_scope) {
                                // 找到模块，生成带模块前缀的名称
                                size_t module_name_len = node->symbol.name_length;
                                size_t var_name_len = expr->as.identifier.name_length;
                                size_t total_len = module_name_len + 2 + var_name_len + 1; // "__" + \0
                                
                                char *qualified_name = malloc(total_len);
                                if (qualified_name) {
                                    snprintf(qualified_name, total_len, "%.*s__%.*s",
                                            (int)module_name_len, node->symbol.name,
                                            (int)var_name_len, expr->as.identifier.name);
                                    
                                    int dest_reg = alloc_reg(ctx);
                                    CnIrOperand dest = cn_ir_op_reg(dest_reg, expr->type);
                                    CnIrOperand src = cn_ir_op_symbol(qualified_name, expr->type);
                                    free(qualified_name);
                                    emit(ctx, cn_ir_inst_new(CN_IR_INST_LOAD, dest, src, cn_ir_op_none()));
                                    return dest;
                                }
                                break;
                            }
                            node = node->next;
                        }
                    }
                }
            }
            
            // 未找到局部变量映射，使用原始名称（可能是全局变量或参数）
            int dest_reg = alloc_reg(ctx);
            
            // 获取变量类型
            CnType *var_type = expr->type;
            if (!var_type && ctx->current_scope) {
                CnSemSymbol *sym = cn_sem_scope_lookup(ctx->current_scope,
                                                       expr->as.identifier.name,
                                                       expr->as.identifier.name_length);
                if (sym && sym->type) {
                    var_type = sym->type;
                }
            }
            
            CnIrOperand dest = cn_ir_op_reg(dest_reg, var_type);
            CnIrOperand src = cn_ir_op_symbol(name, var_type);
            free(name);
            emit(ctx, cn_ir_inst_new(CN_IR_INST_LOAD, dest, src, cn_ir_op_none()));
            return dest;
        }
        case CN_AST_EXPR_BINARY: {
            // 二元运算：先递归生成左右操作数，再生成运算指令
            
            // 特殊处理：字符串拼接（字符串 + 任何类型）
            if (expr->as.binary.op == CN_AST_BINARY_OP_ADD && 
                expr->type && expr->type->kind == CN_TYPE_STRING) {
                
                CnIrOperand left = cn_ir_gen_expr(ctx, expr->as.binary.left);
                CnIrOperand right = cn_ir_gen_expr(ctx, expr->as.binary.right);
                
                // 将非字符串类型转换为字符串
                CnType *left_type = expr->as.binary.left->type;
                CnType *right_type = expr->as.binary.right->type;
                
                // 转换左操作数
                if (left_type && left_type->kind != CN_TYPE_STRING) {
                    const char *convert_func = NULL;
                    if (is_integer_type(left_type)) {
                        convert_func = "cn_rt_int_to_string";
                    } else if (left_type->kind == CN_TYPE_BOOL) {
                        convert_func = "cn_rt_bool_to_string";
                    } else if (is_float_type(left_type)) {
                        convert_func = "cn_rt_float_to_string";
                    }
                    
                    if (convert_func) {
                        CnIrInst *conv_inst = cn_ir_inst_new(CN_IR_INST_CALL, cn_ir_op_none(),
                                                              cn_ir_op_symbol(convert_func, NULL),
                                                              cn_ir_op_none());
                        conv_inst->extra_args_count = 1;
                        conv_inst->extra_args = malloc(sizeof(CnIrOperand));
                        conv_inst->extra_args[0] = left;
                        
                        int str_reg = alloc_reg(ctx);
                        conv_inst->dest = cn_ir_op_reg(str_reg, cn_type_new_primitive(CN_TYPE_STRING));
                        emit(ctx, conv_inst);
                        left = conv_inst->dest;
                    }
                }
                
                // 转换右操作数
                if (right_type && right_type->kind != CN_TYPE_STRING) {
                    const char *convert_func = NULL;
                    if (is_integer_type(right_type)) {
                        convert_func = "cn_rt_int_to_string";
                    } else if (right_type->kind == CN_TYPE_BOOL) {
                        convert_func = "cn_rt_bool_to_string";
                    } else if (is_float_type(right_type)) {
                        convert_func = "cn_rt_float_to_string";
                    }
                    
                    if (convert_func) {
                        CnIrInst *conv_inst = cn_ir_inst_new(CN_IR_INST_CALL, cn_ir_op_none(),
                                                              cn_ir_op_symbol(convert_func, NULL),
                                                              cn_ir_op_none());
                        conv_inst->extra_args_count = 1;
                        conv_inst->extra_args = malloc(sizeof(CnIrOperand));
                        conv_inst->extra_args[0] = right;
                        
                        int str_reg = alloc_reg(ctx);
                        conv_inst->dest = cn_ir_op_reg(str_reg, cn_type_new_primitive(CN_TYPE_STRING));
                        emit(ctx, conv_inst);
                        right = conv_inst->dest;
                    }
                }
                
                // 调用 cn_rt_string_concat
                CnIrInst *concat_inst = cn_ir_inst_new(CN_IR_INST_CALL, cn_ir_op_none(),
                                                        cn_ir_op_symbol("cn_rt_string_concat", NULL),
                                                        cn_ir_op_none());
                concat_inst->extra_args_count = 2;
                concat_inst->extra_args = malloc(2 * sizeof(CnIrOperand));
                concat_inst->extra_args[0] = left;
                concat_inst->extra_args[1] = right;
                
                int dest_reg = alloc_reg(ctx);
                concat_inst->dest = cn_ir_op_reg(dest_reg, expr->type);
                emit(ctx, concat_inst);
                
                return concat_inst->dest;
            }
            
            // 普通二元运算
            CnIrOperand left = cn_ir_gen_expr(ctx, expr->as.binary.left);
            CnIrOperand right = cn_ir_gen_expr(ctx, expr->as.binary.right);
            int dest_reg = alloc_reg(ctx);
            CnIrOperand dest = cn_ir_op_reg(dest_reg, expr->type);
            CnIrInstKind kind = binary_op_to_ir(expr->as.binary.op);
            emit(ctx, cn_ir_inst_new(kind, dest, left, right));
            return dest;
        }
        case CN_AST_EXPR_ASSIGN: {
            // 赋值：先生成右值，再 STORE 到左值地址
            CnIrOperand value = cn_ir_gen_expr(ctx, expr->as.assign.value);
            CnAstExpr *target = expr->as.assign.target;
            
            if (target->kind == CN_AST_EXPR_IDENTIFIER) {
                // 检查是否为静态局部变量（通过上下文中的静态变量列表）
                int is_static = 0;
                CnIrGenStaticVar *static_var = ctx->current_static_vars;
                while (static_var) {
                    if (static_var->name_length == target->as.identifier.name_length &&
                        strncmp(static_var->name, target->as.identifier.name, target->as.identifier.name_length) == 0) {
                        is_static = 1;
                        break;
                    }
                    static_var = static_var->next;
                }
                
                if (is_static) {
                    // 静态变量赋值：生成带函数名前缀的符号名
                    size_t func_name_len = strlen(ctx->current_func->name);
                    size_t var_name_len = target->as.identifier.name_length;
                    size_t total_len = 10 + func_name_len + 1 + var_name_len + 1;
                    
                    char *static_name = malloc(total_len);
                    if (static_name) {
                        snprintf(static_name, total_len, "cn_static_%s_%.*s",
                                ctx->current_func->name,
                                (int)var_name_len, target->as.identifier.name);
                        CnIrOperand addr = cn_ir_op_symbol(static_name, target->type);
                        free(static_name);
                        emit(ctx, cn_ir_inst_new(CN_IR_INST_STORE, addr, value, cn_ir_op_none()));
                    }
                } else {
                    // 普通变量赋值
                    char *name = copy_name(target->as.identifier.name, target->as.identifier.name_length);
                    // 查找局部变量的唯一名称
                    char *unique_name = lookup_local_var_unique_name(ctx, name);
                    if (unique_name) {
                        // 找到映射，使用唯一名称
                        CnIrOperand addr = cn_ir_op_symbol(unique_name, target->type);
                        free(unique_name);
                        free(name);
                        emit(ctx, cn_ir_inst_new(CN_IR_INST_STORE, addr, value, cn_ir_op_none()));
                    } else {
                        // 未找到映射，使用原始名称（可能是全局变量或参数）
                        CnIrOperand addr = cn_ir_op_symbol(name, target->type);
                        free(name);
                        emit(ctx, cn_ir_inst_new(CN_IR_INST_STORE, addr, value, cn_ir_op_none()));
                    }
                }
            } else if (target->kind == CN_AST_EXPR_INDEX) {
                // 数组索引赋值 arr[index] = value
                CnIrOperand array_op = cn_ir_gen_expr(ctx, target->as.index.array);
                CnIrOperand index_op = cn_ir_gen_expr(ctx, target->as.index.index);
                
                // 【修复】检查是否为静态数组（固定大小数组）
                CnType *array_type = target->as.index.array->type;
                bool is_static_array = false;
                if (array_type && array_type->kind == CN_TYPE_ARRAY && array_type->as.array.length > 0) {
                    is_static_array = true;
                }
                
                if (is_static_array) {
                    // 静态数组：使用 GET_ELEMENT_PTR + STORE 生成 C 风格的数组索引赋值
                    // 生成：array[index] = value
                    CnType *elem_type = expr->type;
                    if (!elem_type) {
                        elem_type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                    }
                    CnType *ptr_type = cn_type_new_pointer(elem_type);
                    
                    int addr_reg = alloc_reg(ctx);
                    CnIrInst *gep_inst = cn_ir_inst_new(CN_IR_INST_GET_ELEMENT_PTR,
                                                         cn_ir_op_reg(addr_reg, ptr_type),
                                                         array_op, index_op);
                    emit(ctx, gep_inst);
                    
                    // 存储：*addr = value
                    emit(ctx, cn_ir_inst_new(CN_IR_INST_STORE,
                                              cn_ir_op_reg(addr_reg, ptr_type),
                                              value, cn_ir_op_none()));
                } else {
                    // 动态数组：调用 cn_rt_array_set_element(数组, 索引, &value, 元素大小)
                    CnIrInst *set_inst = cn_ir_inst_new(CN_IR_INST_CALL, cn_ir_op_none(),
                                                          cn_ir_op_symbol("cn_rt_array_set_element", NULL),
                                                          cn_ir_op_none());
                    set_inst->extra_args_count = 4;
                    set_inst->extra_args = malloc(4 * sizeof(CnIrOperand));
                    set_inst->extra_args[0] = array_op;
                    set_inst->extra_args[1] = index_op;
                    set_inst->extra_args[2] = value;  // 直接传递值
                    set_inst->extra_args[3] = cn_ir_op_imm_int(8, cn_type_new_primitive(CN_TYPE_INT));  // 元素大小
                    emit(ctx, set_inst);
                }
            }
            return value;
        }
        case CN_AST_EXPR_LOGICAL: {
            // 逻辑表达式：需要短路求值
            CnIrBasicBlock *rhs_block = cn_ir_basic_block_new(make_block_name(ctx, "logic_rhs"));
            CnIrBasicBlock *merge_block = cn_ir_basic_block_new(make_block_name(ctx, "logic_merge"));
            cn_ir_function_add_block(ctx->current_func, rhs_block);
            cn_ir_function_add_block(ctx->current_func, merge_block);

            int result_reg = alloc_reg(ctx);
            CnIrOperand result = cn_ir_op_reg(result_reg, cn_type_new_primitive(CN_TYPE_BOOL));
            CnIrOperand left = cn_ir_gen_expr(ctx, expr->as.logical.left);

            // 保存当前块，用于后续设置控制流连接
            CnIrBasicBlock *cond_block = ctx->current_block;

            if (expr->as.logical.op == CN_AST_LOGICAL_OP_AND) {
                // AND: 左为假则短路
                emit(ctx, cn_ir_inst_new(CN_IR_INST_BRANCH, cn_ir_op_label(rhs_block),
                                         left, cn_ir_op_label(merge_block)));
                // 设置控制流连接：条件块 -> rhs_block 和 merge_block
                cn_ir_basic_block_connect(cond_block, rhs_block);
                cn_ir_basic_block_connect(cond_block, merge_block);
            } else {
                // OR: 左为真则短路
                emit(ctx, cn_ir_inst_new(CN_IR_INST_BRANCH, cn_ir_op_label(merge_block),
                                         left, cn_ir_op_label(rhs_block)));
                // 设置控制流连接：条件块 -> merge_block 和 rhs_block
                cn_ir_basic_block_connect(cond_block, merge_block);
                cn_ir_basic_block_connect(cond_block, rhs_block);
            }

            switch_to_block(ctx, rhs_block);
            CnIrOperand right = cn_ir_gen_expr(ctx, expr->as.logical.right);
            emit(ctx, cn_ir_inst_new(CN_IR_INST_JUMP, cn_ir_op_label(merge_block),
                                     cn_ir_op_none(), cn_ir_op_none()));
            // 设置控制流连接：rhs_block -> merge_block
            cn_ir_basic_block_connect(rhs_block, merge_block);

            switch_to_block(ctx, merge_block);
            // 简化处理：PHI 指令预留，此处返回右操作数
            return right;
        }
        case CN_AST_EXPR_UNARY: {
            CnIrOperand operand = cn_ir_gen_expr(ctx, expr->as.unary.operand);
            int dest_reg = alloc_reg(ctx);
            CnIrOperand dest = cn_ir_op_reg(dest_reg, expr->type);

            if (expr->as.unary.op == CN_AST_UNARY_OP_NOT) {
                emit(ctx, cn_ir_inst_new(CN_IR_INST_NOT, dest, operand, cn_ir_op_none()));
            } else if (expr->as.unary.op == CN_AST_UNARY_OP_MINUS) {
                emit(ctx, cn_ir_inst_new(CN_IR_INST_NEG, dest, operand, cn_ir_op_none()));
            } else if (expr->as.unary.op == CN_AST_UNARY_OP_ADDRESS_OF) {
                // 取地址运算符：使用 ADDRESS_OF 指令
                if (expr->as.unary.operand->kind == CN_AST_EXPR_IDENTIFIER) {
                    char *name = copy_name(expr->as.unary.operand->as.identifier.name,
                                           expr->as.unary.operand->as.identifier.name_length);
                    
                    // 查找局部变量的唯一名称（如 "值" -> "值_0"）
                    char *unique_name = lookup_local_var_unique_name(ctx, name);
                    if (unique_name) {
                        free(name);
                        name = unique_name;
                    }
                    
                    CnIrOperand addr = cn_ir_op_symbol(name, expr->type);
                    free(name);
                    emit(ctx, cn_ir_inst_new(CN_IR_INST_ADDRESS_OF, dest, addr, cn_ir_op_none()));
                } else if (expr->as.unary.operand->kind == CN_AST_EXPR_MEMBER_ACCESS) {
                    // 成员访问取地址：&obj.member
                    // 先生成成员访问获取成员值，然后取地址
                    CnIrOperand member = cn_ir_gen_expr(ctx, expr->as.unary.operand);
                    emit(ctx, cn_ir_inst_new(CN_IR_INST_ADDRESS_OF, dest, member, cn_ir_op_none()));
                }
                return dest;
            } else if (expr->as.unary.op == CN_AST_UNARY_OP_DEREFERENCE) {
                // 解引用：使用 DEREF 指令
                emit(ctx, cn_ir_inst_new(CN_IR_INST_DEREF, dest, operand, cn_ir_op_none()));
            } else if (expr->as.unary.op == CN_AST_UNARY_OP_PRE_INC) {
                // 前置自增：++i -> i = i + 1, 返回 i
                CnIrOperand one = cn_ir_op_imm_int(1, expr->type);
                emit(ctx, cn_ir_inst_new(CN_IR_INST_ADD, dest, operand, one));
                // 将结果存回变量
                emit(ctx, cn_ir_inst_new(CN_IR_INST_STORE, operand, dest, cn_ir_op_none()));
                return dest;
            } else if (expr->as.unary.op == CN_AST_UNARY_OP_PRE_DEC) {
                // 前置自减：--i -> i = i - 1, 返回 i
                CnIrOperand one = cn_ir_op_imm_int(1, expr->type);
                emit(ctx, cn_ir_inst_new(CN_IR_INST_SUB, dest, operand, one));
                // 将结果存回变量
                emit(ctx, cn_ir_inst_new(CN_IR_INST_STORE, operand, dest, cn_ir_op_none()));
                return dest;
            } else if (expr->as.unary.op == CN_AST_UNARY_OP_POST_INC) {
                // 后置自增：i++ -> temp = i, i = i + 1, 返回 temp
                int temp_reg = alloc_reg(ctx);
                CnIrOperand temp = cn_ir_op_reg(temp_reg, expr->type);
                emit(ctx, cn_ir_inst_new(CN_IR_INST_MOV, temp, operand, cn_ir_op_none()));
                CnIrOperand one = cn_ir_op_imm_int(1, expr->type);
                emit(ctx, cn_ir_inst_new(CN_IR_INST_ADD, dest, operand, one));
                emit(ctx, cn_ir_inst_new(CN_IR_INST_STORE, operand, dest, cn_ir_op_none()));
                return temp;
            } else if (expr->as.unary.op == CN_AST_UNARY_OP_POST_DEC) {
                // 后置自减：i-- -> temp = i, i = i - 1, 返回 temp
                int temp_reg = alloc_reg(ctx);
                CnIrOperand temp = cn_ir_op_reg(temp_reg, expr->type);
                emit(ctx, cn_ir_inst_new(CN_IR_INST_MOV, temp, operand, cn_ir_op_none()));
                CnIrOperand one = cn_ir_op_imm_int(1, expr->type);
                emit(ctx, cn_ir_inst_new(CN_IR_INST_SUB, dest, operand, one));
                emit(ctx, cn_ir_inst_new(CN_IR_INST_STORE, operand, dest, cn_ir_op_none()));
                return temp;
            }
            return dest;
        }
        case CN_AST_EXPR_TERNARY: {
            // 三元运算符：condition ? true_expr : false_expr
            // 使用 SELECT 指令表示：dest = condition ? true_val : false_val
            // 指令格式：dest, src1=condition, src2=true_val, extra_args[0]=false_val
            
            // 生成三个操作数
            CnIrOperand condition = cn_ir_gen_expr(ctx, expr->as.ternary.condition);
            CnIrOperand true_val = cn_ir_gen_expr(ctx, expr->as.ternary.true_expr);
            CnIrOperand false_val = cn_ir_gen_expr(ctx, expr->as.ternary.false_expr);
            
            // 分配结果寄存器
            int result_reg = alloc_reg(ctx);
            CnIrOperand result = cn_ir_op_reg(result_reg, expr->type);
            
            // 生成 SELECT 指令
            CnIrInst *select_inst = cn_ir_inst_new(CN_IR_INST_SELECT, result, condition, true_val);
            
            // 使用 extra_args 存储 false_val
            select_inst->extra_args_count = 1;
            select_inst->extra_args = malloc(sizeof(CnIrOperand));
            select_inst->extra_args[0] = false_val;
            
            emit(ctx, select_inst);
            
            return result;
        }
        case CN_AST_EXPR_CALL: {
            // 函数调用
            CnIrOperand callee;
            const char *func_name = NULL;
            
            // 特殊处理：内置函数根据参数类型选择运行时函数
            // 支持两种调用风格：长度(arr) 和 arr.长度()
            if (expr->as.call.callee->kind == CN_AST_EXPR_IDENTIFIER) {
                char *name = copy_name(expr->as.call.callee->as.identifier.name,
                                       expr->as.call.callee->as.identifier.name_length);
                
                // 检查是否是构造函数调用（类型名作为函数名）
                // 例如：点(10, 20) 或 学生("张三", 20, 85.5)
                // 判断依据：callee是标识符，且该标识符是结构体类型名（不是函数名）
                // 需要在符号表中查找该名称，确认它是类型而不是函数
                bool is_constructor = false;
                if (expr->type && expr->type->kind == CN_TYPE_STRUCT && ctx->global_scope) {
                    CnSemSymbol *sym = cn_sem_scope_lookup(ctx->global_scope, name, strlen(name));
                    // 只有当符号是结构体类型时才是构造函数调用
                    // 如果符号是函数，则不是构造函数
                    if (sym && (sym->kind == CN_SEM_SYMBOL_STRUCT || sym->kind == CN_SEM_SYMBOL_CLASS)) {
                        is_constructor = true;
                    }
                }
                if (is_constructor) {
                    // 构造函数调用：生成复合字面量
                    // 在C99中：(struct 类型名){.字段1 = 值1, .字段2 = 值2, ...}
                    // 但由于我们不知道字段名，使用位置初始化：
                    // (struct 类型名){值1, 值2, ...}
                    
                    // 分配结果寄存器
                    int dest_reg = alloc_reg(ctx);
                    CnIrOperand dest = cn_ir_op_reg(dest_reg, expr->type);
                    
                    // 生成 STRUCT_INIT 指令
                    // 格式：dest = STRUCT_INIT 类型名, 参数...
                    CnIrInst *init_inst = cn_ir_inst_new(CN_IR_INST_STRUCT_INIT, dest,
                                                          cn_ir_op_symbol(name, expr->type),
                                                          cn_ir_op_none());
                    
                    // 收集参数
                    if (expr->as.call.argument_count > 0) {
                        init_inst->extra_args_count = expr->as.call.argument_count;
                        init_inst->extra_args = malloc(sizeof(CnIrOperand) * expr->as.call.argument_count);
                        for (size_t i = 0; i < expr->as.call.argument_count; i++) {
                            init_inst->extra_args[i] = cn_ir_gen_expr(ctx, expr->as.call.arguments[i]);
                        }
                    }
                    
                    emit(ctx, init_inst);
                    free(name);
                    return dest;
                }
                                        
                // 检查是否是 "长度" 函数（函数风格）
                if (strcmp(name, "长度") == 0 && expr->as.call.argument_count == 1) {
                    // 根据第一个参数的类型决定调用哪个运行时函数
                    CnType *arg_type = expr->as.call.arguments[0]->type;
                    if (arg_type && arg_type->kind == CN_TYPE_ARRAY) {
                        func_name = "cn_rt_array_length";
                    } else {
                        func_name = "cn_rt_string_length";
                    }
                    free(name);
                    callee = cn_ir_op_symbol(func_name, expr->as.call.callee->type);
                } 
                // 检查是否是 “打印” 函数
                else if (strcmp(name, "打印") == 0 && expr->as.call.argument_count == 1) {
                    // 打印函数根据参数类型映射到不同的运行时函数
                    CnType *arg_type = expr->as.call.arguments[0]->type;
                    if (arg_type && is_integer_type(arg_type)) {
                        func_name = "cn_rt_print_int";
                    } else if (arg_type && arg_type->kind == CN_TYPE_BOOL) {
                        func_name = "cn_rt_print_bool";
                    } else if (arg_type && is_float_type(arg_type)) {
                        func_name = "cn_rt_print_float";
                    } else {
                        // 默认为字符串打印（包括 STRING 类型和其他类型）
                        func_name = "cn_rt_print_string";
                    }
                    free(name);
                    callee = cn_ir_op_symbol(func_name, expr->as.call.callee->type);
                }
                // 检查是否是 "打印整数" 函数
                else if (strcmp(name, "打印整数") == 0) {
                    func_name = "cn_rt_print_int";
                    free(name);
                    callee = cn_ir_op_symbol(func_name, expr->as.call.callee->type);
                } 
                // 检查是否是 "打印字符串" 函数
                else if (strcmp(name, "打印字符串") == 0) {
                    func_name = "cn_rt_print_string";
                    free(name);
                    callee = cn_ir_op_symbol(func_name, expr->as.call.callee->type);
                }
                // 检查是否是 "类型大小" 函数（sizeof）
                else if (strcmp(name, "类型大小") == 0 && expr->as.call.argument_count == 1) {
                    // 类型大小 函数需要特殊处理：
                    // 参数可以是类型名或变量名
                    // 生成 CN_IR_INST_SIZEOF 指令
                    
                    CnAstExpr *arg = expr->as.call.arguments[0];
                    
                    // 分配结果寄存器
                    int dest_reg = alloc_reg(ctx);
                    CnIrOperand dest = cn_ir_op_reg(dest_reg, expr->type);
                    
                    // 检查参数是否是类型名（标识符且在符号表中是类型）
                    bool is_type_name = false;
                    if (arg->kind == CN_AST_EXPR_IDENTIFIER && ctx->global_scope) {
                        char *arg_name = copy_name(arg->as.identifier.name, arg->as.identifier.name_length);
                        CnSemSymbol *sym = cn_sem_scope_lookup(ctx->global_scope, arg_name, strlen(arg_name));
                        if (sym && (sym->kind == CN_SEM_SYMBOL_STRUCT ||
                                    sym->kind == CN_SEM_SYMBOL_CLASS ||
                                    sym->kind == CN_SEM_SYMBOL_ENUM)) {
                            is_type_name = true;
                        }
                        free(arg_name);
                    }
                    
                    // 生成 SIZEOF 指令
                    // src1 存储类型信息或变量
                    CnIrOperand sizeof_arg;
                    if (is_type_name) {
                        // 参数是类型名：直接使用类型信息
                        sizeof_arg = cn_ir_op_symbol(
                            copy_name(arg->as.identifier.name, arg->as.identifier.name_length),
                            arg->type);
                    } else {
                        // 参数是变量或表达式：生成表达式代码
                        sizeof_arg = cn_ir_gen_expr(ctx, arg);
                    }
                    
                    CnIrInst *sizeof_inst = cn_ir_inst_new(CN_IR_INST_SIZEOF, dest, sizeof_arg, cn_ir_op_none());
                    sizeof_inst->extra_args_count = is_type_name ? 1 : 0;  // 标记是否是类型名
                    if (is_type_name) {
                        sizeof_inst->extra_args = malloc(sizeof(CnIrOperand));
                        sizeof_inst->extra_args[0] = cn_ir_op_imm_int(1, NULL);  // 1 表示类型名
                    }
                    
                    emit(ctx, sizeof_inst);
                    free(name);
                    return dest;
                } else {
                    // 直接使用原始函数名（不再生成带模块前缀的名称）
                    // 模块系统通过作用域管理避免命名冲突
                    callee = cn_ir_op_symbol(name, expr->as.call.callee->type);
                    free(name);
                }
            } 
            // 特殊处理：方法风格调用 arr.长度()
            else if (expr->as.call.callee->kind == CN_AST_EXPR_MEMBER_ACCESS &&
                     expr->as.call.callee->as.member.member_name_length == strlen("长度") &&
                     strncmp(expr->as.call.callee->as.member.member_name, "长度",
                             expr->as.call.callee->as.member.member_name_length) == 0) {
                // 将方法风格调用转换为函数风格：arr.长度() -> 长度(arr)
                // 【修复】添加空指针检查，防止 object 为 NULL 时崩溃
                CnType *obj_type = NULL;
                if (expr->as.call.callee->as.member.object) {
                    obj_type = expr->as.call.callee->as.member.object->type;
                }
                if (obj_type && obj_type->kind == CN_TYPE_ARRAY) {
                    func_name = "cn_rt_array_length";
                } else {
                    func_name = "cn_rt_string_length";
                }
                callee = cn_ir_op_symbol(func_name, expr->as.call.callee->type);
                
                // 生成调用指令，将 member.object 作为参数
                CnIrInst *call_inst = cn_ir_inst_new(CN_IR_INST_CALL, cn_ir_op_none(),
                                                      callee, cn_ir_op_none());
                call_inst->extra_args_count = 1;
                call_inst->extra_args = malloc(sizeof(CnIrOperand));
                call_inst->extra_args[0] = cn_ir_gen_expr(ctx, expr->as.call.callee->as.member.object);
                
                if (expr->type && expr->type->kind != CN_TYPE_VOID) {
                    int dest_reg = alloc_reg(ctx);
                    call_inst->dest = cn_ir_op_reg(dest_reg, expr->type);
                }
                emit(ctx, call_inst);
                return call_inst->dest;
            }
            // 处理类成员方法调用（包括虚函数调用）
            else if (expr->as.call.callee->kind == CN_AST_EXPR_MEMBER_ACCESS) {
                // 成员访问调用：obj.method(args)
                // 需要判断是否为虚函数调用
                
                // 获取对象表达式和方法名
                CnAstExpr *object_expr = expr->as.call.callee->as.member.object;
                const char *method_name = expr->as.call.callee->as.member.member_name;
                size_t method_name_len = expr->as.call.callee->as.member.member_name_length;
                
                // 检查是否为模块成员调用（模块类型为 VOID）
                // 模块成员调用：直接使用原始函数名（不再生成带模块前缀的名称）
                if (object_expr->type && object_expr->type->kind == CN_TYPE_VOID) {
                    // 模块成员函数调用：直接生成对模块函数的调用
                    // cn_ir_gen_expr 会处理 CN_AST_EXPR_MEMBER_ACCESS 并生成原始函数名
                    callee = cn_ir_gen_expr(ctx, expr->as.call.callee);
                }
                // 检查是否标记为虚函数调用（通过AST节点中的标记）
                // 如果是虚函数，生成虚函数调用指令
                // 【修复】添加空指针检查，防止 object 为 NULL 时崩溃
                else if (expr->as.call.callee->as.member.object &&
                         expr->as.call.callee->as.member.object->type) {
                    CnType *obj_type = expr->as.call.callee->as.member.object->type;
                    // 检查类型是否为类类型（通过结构体类型模拟）
                    // 在类型系统中，类类型使用 CN_TYPE_STRUCT 表示
                    // 完整实现需要扩展类型系统添加 CN_TYPE_CLASS
                    
                    // 生成对象表达式 - 对于方法调用，需要获取变量的地址而不是值
                    CnIrOperand obj_operand;
                    bool need_address = true;  // 方法调用需要传递对象地址
                    
                    // 如果对象是标识符，直接获取变量名用于取地址
                    if (object_expr->kind == CN_AST_EXPR_IDENTIFIER) {
                        char *name = copy_name(object_expr->as.identifier.name,
                                               object_expr->as.identifier.name_length);
                        // 查找局部变量的唯一名称
                        char *unique_name = lookup_local_var_unique_name(ctx, name);
                        if (unique_name) {
                            free(name);
                            name = unique_name;
                        }
                        // 生成取地址指令
                        int addr_reg = alloc_reg(ctx);
                        CnType *ptr_type = cn_type_new_pointer(object_expr->type);
                        obj_operand = cn_ir_op_reg(addr_reg, ptr_type);
                        emit(ctx, cn_ir_inst_new(CN_IR_INST_ADDRESS_OF, obj_operand,
                                                     cn_ir_op_symbol(name, object_expr->type),
                                                     cn_ir_op_none()));
                        free(name);
                    } else {
                        // 其他情况（如成员访问、表达式等），正常生成
                        obj_operand = cn_ir_gen_expr(ctx, object_expr);
                        need_address = false;
                    }
                    
                    // 生成普通成员函数调用
                    // 格式：类名_方法名(self, args...)
                    
                    // 需要查找方法实际定义在哪个类中（可能在基类中）
                    const char *class_name = NULL;
                    size_t class_name_len = 0;
                    
                    // 从对象类型中获取类名
                    if (obj_type->kind == CN_TYPE_STRUCT && obj_type->as.struct_type.name) {
                        const char *obj_class_name = obj_type->as.struct_type.name;
                        size_t obj_class_name_len = obj_type->as.struct_type.name_length;
                        
                        // 首先在当前类中查找方法
                        bool found_in_current = false;
                        if (ctx->program) {
                            for (size_t i = 0; i < ctx->program->class_count; i++) {
                                CnAstStmt *stmt = ctx->program->classes[i];
                                if (stmt->kind == CN_AST_STMT_CLASS_DECL) {
                                    CnAstClassDecl *class_decl = stmt->as.class_decl;
                                    if (class_decl->name_length == obj_class_name_len &&
                                        memcmp(class_decl->name, obj_class_name, obj_class_name_len) == 0) {
                                        // 在当前类中查找方法
                                        for (size_t j = 0; j < class_decl->member_count; j++) {
                                            CnClassMember *member = &class_decl->members[j];
                                            if (member->kind == CN_MEMBER_METHOD && !member->is_static) {
                                                if (member->name_length == method_name_len &&
                                                    memcmp(member->name, method_name, method_name_len) == 0) {
                                                    found_in_current = true;
                                                    class_name = obj_class_name;
                                                    class_name_len = obj_class_name_len;
                                                    break;
                                                }
                                            }
                                        }
                                        
                                        // 如果当前类没有，在基类链中查找
                                        if (!found_in_current && class_decl->base_count > 0) {
                                            CnAstClassDecl *search_class = class_decl;
                                            while (!found_in_current && search_class && search_class->base_count > 0) {
                                                CnInheritanceInfo *base_info = &search_class->bases[0];
                                                
                                                // 在程序中查找基类定义
                                                for (size_t k = 0; k < ctx->program->class_count; k++) {
                                                    CnAstStmt *base_stmt = ctx->program->classes[k];
                                                    if (base_stmt->kind == CN_AST_STMT_CLASS_DECL) {
                                                        CnAstClassDecl *base_class = base_stmt->as.class_decl;
                                                        if (base_class->name_length == base_info->base_class_name_length &&
                                                            memcmp(base_class->name, base_info->base_class_name, base_class->name_length) == 0) {
                                                            // 在基类中查找方法
                                                            for (size_t m = 0; m < base_class->member_count; m++) {
                                                                CnClassMember *base_member = &base_class->members[m];
                                                                if (base_member->kind == CN_MEMBER_METHOD && !base_member->is_static) {
                                                                    if (base_member->name_length == method_name_len &&
                                                                        memcmp(base_member->name, method_name, method_name_len) == 0) {
                                                                        found_in_current = true;
                                                                        class_name = base_class->name;
                                                                        class_name_len = base_class->name_length;
                                                                        
                                                                        // 更新表达式类型为方法的返回类型
                                                                        // base_member->type 就是方法的返回类型
                                                                        // 如果 type 为 NULL，表示方法返回 void
                                                                        if (base_member->type) {
                                                                            expr->type = base_member->type;
                                                                        } else {
                                                                            // void 返回类型
                                                                            expr->type = cn_type_new_primitive(CN_TYPE_VOID);
                                                                        }
                                                                        
                                                                        // 生成基类指针转换
                                                                        // 需要将派生类指针转换为基类指针
                                                                        // 格式：&obj.基类名_base
                                                                        if (object_expr->kind == CN_AST_EXPR_IDENTIFIER) {
                                                                            // 重新生成正确的基类指针
                                                                            char *obj_name = copy_name(object_expr->as.identifier.name,
                                                                                                       object_expr->as.identifier.name_length);
                                                                            char *unique_name = lookup_local_var_unique_name(ctx, obj_name);
                                                                            if (unique_name) {
                                                                                free(obj_name);
                                                                                obj_name = unique_name;
                                                                            }
                                                                            
                                                                            // 创建基类类型（用于类型声明）
                                                                            CnType *base_class_type = cn_type_new_struct(
                                                                                base_class->name,
                                                                                base_class->name_length,
                                                                                NULL, 0,  // fields, field_count
                                                                                NULL,     // decl_scope
                                                                                NULL, 0); // owner_func_name, owner_func_name_length
                                                                            
                                                                            // 生成基类指针：&obj.基类名_base
                                                                            // 步骤1：生成成员访问指令获取基类成员
                                                                            int member_reg = alloc_reg(ctx);
                                                                            CnIrOperand member_operand = cn_ir_op_reg(member_reg, base_class_type);
                                                                            
                                                                            char base_member_name[128];
                                                                            snprintf(base_member_name, sizeof(base_member_name), "%.*s_base",
                                                                                     (int)base_info->base_class_name_length, base_info->base_class_name);
                                                                            
                                                                            CnIrInst *member_inst = cn_ir_inst_new(
                                                                                CN_IR_INST_MEMBER_ACCESS, member_operand,
                                                                                cn_ir_op_symbol(obj_name, object_expr->type),
                                                                                cn_ir_op_symbol(base_member_name, object_expr->type));
                                                                            emit(ctx, member_inst);
                                                                            
                                                                            // 步骤2：生成取地址指令
                                                                            int base_ptr_reg = alloc_reg(ctx);
                                                                            CnType *base_ptr_type = cn_type_new_pointer(base_class_type);
                                                                            obj_operand = cn_ir_op_reg(base_ptr_reg, base_ptr_type);
                                                                            
                                                                            CnIrInst *addr_inst = cn_ir_inst_new(
                                                                                CN_IR_INST_ADDRESS_OF, obj_operand,
                                                                                member_operand, cn_ir_op_none());
                                                                            emit(ctx, addr_inst);
                                                                            free(obj_name);
                                                                        }
                                                                        break;
                                                                    }
                                                                }
                                                            }
                                                            if (!found_in_current) {
                                                                // 继续在更深的基类中查找
                                                                search_class = base_class;
                                                            }
                                                            break;
                                                        }
                                                    }
                                                }
                                                if (found_in_current) break;
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        
                        // 如果没找到，使用对象类型名（可能后续会报错）
                        if (!found_in_current) {
                            class_name = obj_class_name;
                            class_name_len = obj_class_name_len;
                        }
                    }
                    
                    // 生成函数名：类名_方法名
                    char *func_name = NULL;
                    if (class_name && method_name) {
                        // 分配足够的空间：类名 + "_" + 方法名 + "\0"
                        func_name = malloc(class_name_len + 1 + method_name_len + 1);
                        if (func_name) {
                            memcpy(func_name, class_name, class_name_len);
                            func_name[class_name_len] = '_';
                            memcpy(func_name + class_name_len + 1, method_name, method_name_len);
                            func_name[class_name_len + 1 + method_name_len] = '\0';
                        }
                    }
                    
                    // 创建被调用函数的操作数
                    CnIrOperand callee_op = func_name ?
                        cn_ir_op_symbol(func_name, expr->as.call.callee->type) :
                        cn_ir_op_none();
                    
                    // 生成调用指令
                    CnIrInst *call_inst = cn_ir_inst_new(CN_IR_INST_CALL, cn_ir_op_none(),
                                                          callee_op, cn_ir_op_none());
                    
                    // 参数数量 = self + 原始参数
                    size_t total_args = 1 + expr->as.call.argument_count;
                    call_inst->extra_args_count = total_args;
                    call_inst->extra_args = malloc(total_args * sizeof(CnIrOperand));
                    
                    // 第一个参数是 self 指针
                    // obj_operand 已经是地址（对于标识符）或者值（对于其他表达式）
                    call_inst->extra_args[0] = obj_operand;
                    
                    // 其他参数
                    for (size_t i = 0; i < expr->as.call.argument_count; i++) {
                        call_inst->extra_args[i + 1] = cn_ir_gen_expr(ctx, expr->as.call.arguments[i]);
                    }
                    
                    // 如果有返回值，分配结果寄存器
                    // 注意：对于方法调用，expr->type 已经是返回类型（不是函数类型）
                    // 因为在语义分析中，成员访问表达式的类型被设置为方法的返回类型
                    if (expr->type && expr->type->kind != CN_TYPE_VOID) {
                        int dest_reg = alloc_reg(ctx);
                        call_inst->dest = cn_ir_op_reg(dest_reg, expr->type);
                    }
                    
                    emit(ctx, call_inst);
                    return call_inst->dest;
                }
                
                // 默认：生成普通调用
                callee = cn_ir_gen_expr(ctx, expr->as.call.callee);
            }
            else {
                callee = cn_ir_gen_expr(ctx, expr->as.call.callee);
            }
            
            CnIrInst *call_inst = cn_ir_inst_new(CN_IR_INST_CALL, cn_ir_op_none(),
                                                  callee, cn_ir_op_none());
            // 处理参数
            call_inst->extra_args_count = expr->as.call.argument_count;
            if (call_inst->extra_args_count > 0) {
                call_inst->extra_args = malloc(call_inst->extra_args_count * sizeof(CnIrOperand));
                for (size_t i = 0; i < expr->as.call.argument_count; i++) {
                    call_inst->extra_args[i] = cn_ir_gen_expr(ctx, expr->as.call.arguments[i]);
                }
            }
            
            // 确定函数返回类型
            CnType *return_type = expr->type;
            
            // 如果 expr->type 不可靠（NULL 或 VOID），尝试从函数符号获取返回类型
            if ((!return_type || return_type->kind == CN_TYPE_VOID) &&
                expr->as.call.callee->kind == CN_AST_EXPR_IDENTIFIER && ctx->global_scope) {
                char *func_name = copy_name(expr->as.call.callee->as.identifier.name,
                                            expr->as.call.callee->as.identifier.name_length);
                CnSemSymbol *sym = cn_sem_scope_lookup(ctx->global_scope, func_name, strlen(func_name));
                if (sym && sym->kind == CN_SEM_SYMBOL_FUNCTION && sym->type) {
                    // 函数符号的 type 是函数类型，需要从中提取返回类型
                    if (sym->type->kind == CN_TYPE_FUNCTION && sym->type->as.function.return_type) {
                        return_type = sym->type->as.function.return_type;
                    }
                }
                free(func_name);
            }
            
            // 如果有返回值，分配结果寄存器
            if (return_type && return_type->kind != CN_TYPE_VOID) {
                int dest_reg = alloc_reg(ctx);
                call_inst->dest = cn_ir_op_reg(dest_reg, return_type);
            }
            emit(ctx, call_inst);
            return call_inst->dest;
        }
        case CN_AST_EXPR_ARRAY_LITERAL: {
            // 数组字面量：调用 cn_rt_array_alloc 分配数组，然后逐个设置元素
            size_t elem_count = expr->as.array_literal.element_count;
            
            // 【修复】添加空指针检查，防止 expr->type 为 NULL 时崩溃
            CnType *elem_type = NULL;
            if (expr->type && expr->type->kind == CN_TYPE_ARRAY) {
                elem_type = expr->type->as.array.element_type;
            }
            size_t elem_size = 8;  // 默认大小，对于整数和指针
            
            // 生成对 cn_rt_array_alloc 的调用
            CnIrInst *alloc_inst = cn_ir_inst_new(CN_IR_INST_CALL, cn_ir_op_none(),
                                                    cn_ir_op_symbol("cn_rt_array_alloc", NULL),
                                                    cn_ir_op_none());
            alloc_inst->extra_args_count = 2;
            alloc_inst->extra_args = malloc(2 * sizeof(CnIrOperand));
            alloc_inst->extra_args[0] = cn_ir_op_imm_int(elem_size, cn_type_new_primitive(CN_TYPE_INT));
            alloc_inst->extra_args[1] = cn_ir_op_imm_int(elem_count, cn_type_new_primitive(CN_TYPE_INT));
            
            int array_reg = alloc_reg(ctx);
            alloc_inst->dest = cn_ir_op_reg(array_reg, expr->type);
            emit(ctx, alloc_inst);
            
            // 逐个设置数组元素
            for (size_t i = 0; i < elem_count; i++) {
                CnIrOperand elem_val = cn_ir_gen_expr(ctx, expr->as.array_literal.elements[i]);
                
                // 调用 cn_rt_array_set_element(数组, 索引, &元素, 元素大小)
                CnIrInst *set_inst = cn_ir_inst_new(CN_IR_INST_CALL, cn_ir_op_none(),
                                                      cn_ir_op_symbol("cn_rt_array_set_element", NULL),
                                                      cn_ir_op_none());
                set_inst->extra_args_count = 4;
                set_inst->extra_args = malloc(4 * sizeof(CnIrOperand));
                set_inst->extra_args[0] = cn_ir_op_reg(array_reg, expr->type);
                set_inst->extra_args[1] = cn_ir_op_imm_int(i, cn_type_new_primitive(CN_TYPE_INT));
                set_inst->extra_args[2] = elem_val;
                set_inst->extra_args[3] = cn_ir_op_imm_int(elem_size, cn_type_new_primitive(CN_TYPE_INT));
                emit(ctx, set_inst);
            }
            
            return cn_ir_op_reg(array_reg, expr->type);
        }
        case CN_AST_EXPR_INDEX: {
            // 数组索引访问 arr[index]
            CnIrOperand array_op = cn_ir_gen_expr(ctx, expr->as.index.array);
            CnIrOperand index_op = cn_ir_gen_expr(ctx, expr->as.index.index);
            
            // 【修复】检查是否为静态数组（固定大小数组）
            // 静态数组的 length > 0，动态数组的 length == 0
            CnType *array_type = expr->as.index.array->type;
            bool is_static_array = false;
            
            // 【调试】输出数组类型信息
            fprintf(stderr, "[DEBUG] INDEX: array_type=%p, kind=%d, length=%zu\n",
                    (void*)array_type,
                    array_type ? array_type->kind : -1,
                    (array_type && array_type->kind == CN_TYPE_ARRAY) ? array_type->as.array.length : 0);
            
            if (array_type && array_type->kind == CN_TYPE_ARRAY && array_type->as.array.length > 0) {
                is_static_array = true;
                fprintf(stderr, "[DEBUG] INDEX: detected static array, length=%zu\n", array_type->as.array.length);
            }
            
            int result_reg = alloc_reg(ctx);
            CnType *result_type = expr->type;
            if (!result_type) {
                // 如果元素类型未知，默认使用 void* 指针类型
                result_type = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID));
            } else if (result_type->kind != CN_TYPE_POINTER) {
                // 如果元素类型不是指针，则创建指向元素类型的指针
                result_type = cn_type_new_pointer(result_type);
            }
            
            if (is_static_array) {
                // 静态数组：使用 GET_ELEMENT_PTR 指令生成 C 风格的数组索引访问
                // 生成：result = &array[index]
                CnIrInst *gep_inst = cn_ir_inst_new(CN_IR_INST_GET_ELEMENT_PTR,
                                                     cn_ir_op_reg(result_reg, result_type),
                                                     array_op, index_op);
                emit(ctx, gep_inst);
                
                // 然后加载值（如果元素类型不是指针）
                if (expr->type && expr->type->kind != CN_TYPE_POINTER) {
                    int load_reg = alloc_reg(ctx);
                    CnIrInst *load_inst = cn_ir_inst_new(CN_IR_INST_LOAD,
                                                          cn_ir_op_reg(load_reg, expr->type),
                                                          cn_ir_op_reg(result_reg, result_type),
                                                          cn_ir_op_none());
                    emit(ctx, load_inst);
                    return cn_ir_op_reg(load_reg, expr->type);
                }
                return cn_ir_op_reg(result_reg, result_type);
            } else {
                // 动态数组：调用 cn_rt_array_get_element(数组, 索引, 元素大小)
                CnIrInst *get_inst = cn_ir_inst_new(CN_IR_INST_CALL, cn_ir_op_none(),
                                                      cn_ir_op_symbol("cn_rt_array_get_element", NULL),
                                                      cn_ir_op_none());
                get_inst->extra_args_count = 3;
                get_inst->extra_args = malloc(3 * sizeof(CnIrOperand));
                get_inst->extra_args[0] = array_op;
                get_inst->extra_args[1] = index_op;
                get_inst->extra_args[2] = cn_ir_op_imm_int(8, cn_type_new_primitive(CN_TYPE_INT));  // 元素大小
                
                get_inst->dest = cn_ir_op_reg(result_reg, result_type);
                emit(ctx, get_inst);
                
                return cn_ir_op_reg(result_reg, result_type);
            }
        }
        case CN_AST_EXPR_MEMBER_ACCESS: {
            // 成员访问：支持结构体 obj.member、模块 module.member 和枚举 enum.member
            
            // 检查是否为枚举成员访问
            // 方式1：对象类型为 ENUM 表示枚举（语义分析器已设置）
            // 方式2：通过符号表查找对象标识符，确认是否为枚举类型
            bool is_enum_access = false;
            CnSemSymbol *enum_sym = NULL;
            
            // 【修复】添加空指针检查，防止 object 为 NULL 时崩溃
            if (expr->as.member.object &&
                expr->as.member.object->type &&
                expr->as.member.object->type->kind == CN_TYPE_ENUM &&
                expr->as.member.object->kind == CN_AST_EXPR_IDENTIFIER) {
                // 方式1：语义分析器已正确设置类型
                is_enum_access = true;
            } else if (expr->as.member.object &&
                       expr->as.member.object->kind == CN_AST_EXPR_IDENTIFIER && ctx->current_scope) {
                // 方式2：通过符号表查找确认是否为枚举类型
                CnSemSymbol *sym = cn_sem_scope_lookup(ctx->current_scope,
                    expr->as.member.object->as.identifier.name,
                    expr->as.member.object->as.identifier.name_length);
                if (sym && sym->kind == CN_SEM_SYMBOL_ENUM && sym->type) {
                    is_enum_access = true;
                    enum_sym = sym;
                    // 修正对象类型
                    expr->as.member.object->type = sym->type;
                }
            }
            
            if (is_enum_access) {
                // 枚举成员访问：查找枚举成员的值
                CnType *enum_type = enum_sym ? enum_sym->type : expr->as.member.object->type;
                
                // 【调试】输出枚举类型信息
                fprintf(stderr, "[DEBUG] 枚举成员访问: 成员名=%.*s, enum_type=%p, kind=%d\n",
                        (int)expr->as.member.member_name_length, expr->as.member.member_name,
                        (void*)enum_type, enum_type ? enum_type->kind : -1);
                
                CnSemSymbol *member_sym = cn_type_enum_find_member(
                    enum_type,
                    expr->as.member.member_name,
                    expr->as.member.member_name_length);
                
                if (member_sym && member_sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                    // 枚举成员：保留为符号引用，而不是立即数
                    // C代码生成器期望符号名格式为 "枚举类型名_成员名"
                    // 需要生成完整的枚举成员符号名
                    
                    // 获取枚举类型名称
                    const char *enum_name = enum_type->as.enum_type.name;
                    size_t enum_name_len = enum_type->as.enum_type.name_length;
                    
                    // 获取成员名称
                    const char *member_name = member_sym->name;
                    size_t member_name_len = member_sym->name_length;
                    
                    // 生成完整的枚举成员符号名：枚举类型名_成员名
                    size_t full_name_len = enum_name_len + 1 + member_name_len; // "_" + 成员名
                    char *full_name = malloc(full_name_len + 1);
                    if (full_name) {
                        snprintf(full_name, full_name_len + 1, "%.*s_%.*s",
                                (int)enum_name_len, enum_name,
                                (int)member_name_len, member_name);
                        
                        fprintf(stderr, "[DEBUG] 创建枚举成员符号: name=%s, type=%p, kind=%d\n",
                                full_name, (void*)enum_type, enum_type ? enum_type->kind : -1);
                        CnIrOperand result = cn_ir_op_symbol(full_name, enum_type);
                        free(full_name);
                        return result;
                    }
                    
                    // 如果分配失败，回退到使用成员名
                    char *fallback_name = copy_name(member_sym->name, member_sym->name_length);
                    CnIrOperand result = cn_ir_op_symbol(fallback_name, enum_type);
                    free(fallback_name);
                    return result;
                }
                // 如果找不到，返回错误操作数
                return cn_ir_op_none();
            }
            
            // 检查是否为模块成员访问（对象类型为 VOID 表示模块）
            // 【修复】添加空指针检查，防止 object 为 NULL 时崩溃
            if (expr->as.member.object &&
                expr->as.member.object->type &&
                expr->as.member.object->type->kind == CN_TYPE_VOID &&
                expr->as.member.object->kind == CN_AST_EXPR_IDENTIFIER) {
                // 模块成员访问：直接使用成员名称（不再生成带模块前缀的名称）
                // 模块系统通过作用域管理避免命名冲突
                size_t member_name_len = expr->as.member.member_name_length;
                char *member_name = malloc(member_name_len + 1);
                if (member_name) {
                    snprintf(member_name, member_name_len + 1, "%.*s",
                            (int)member_name_len, expr->as.member.member_name);
                    CnIrOperand result = cn_ir_op_symbol(member_name, expr->type);
                    free(member_name);
                    return result;
                }
                // 如果分配失败，继续使用默认处理
            }
            
            // 结构体成员访问：obj.member 或 ptr->member
            // CN语言中，指针成员访问可以使用"."语法，但生成C代码时需要转换为"->"
            // 【修复】添加空指针检查，防止 object 为 NULL 时崩溃
            if (!expr->as.member.object) {
                return cn_ir_op_none();
            }
            CnIrOperand object_op = cn_ir_gen_expr(ctx, expr->as.member.object);
            
            // 判断对象是否为指针类型
            bool object_is_pointer = (expr->as.member.object->type &&
                                      expr->as.member.object->type->kind == CN_TYPE_POINTER);
            
            // 重要：将对象的类型信息设置到object_op中，以便代码生成器判断是否使用"->"
            if (expr->as.member.object->type) {
                object_op.type = expr->as.member.object->type;
            }
            
            // 如果是箭头访问或对象是指针类型，先解引用指针
            if (expr->as.member.is_arrow || object_is_pointer) {
                // 对于指针类型，生成解引用指令
                if (object_is_pointer && !expr->as.member.is_arrow) {
                    // CN语言中指针使用"."语法，但需要生成"->"访问
                    // 这里不解引用，而是在代码生成时使用"->"
                } else if (expr->as.member.is_arrow) {
                    // 显式使用"->"语法，解引用指针
                    // 【修复】添加空指针检查，防止 type 或 pointer_to 为 NULL 时崩溃
                    CnType *pointer_type = expr->as.member.object->type;
                    if (pointer_type && pointer_type->kind == CN_TYPE_POINTER && pointer_type->as.pointer_to) {
                        int deref_reg = alloc_reg(ctx);
                        CnIrOperand deref_op = cn_ir_op_reg(deref_reg, pointer_type->as.pointer_to);
                        emit(ctx, cn_ir_inst_new(CN_IR_INST_DEREF, deref_op, object_op, cn_ir_op_none()));
                        object_op = deref_op;
                    }
                    // 如果类型信息不完整，跳过解引用（后续代码生成会处理）
                }
            }
            
            // 生成成员访问指令，dest操作数记录成员名
            int result_reg = alloc_reg(ctx);
            
            CnIrOperand result = cn_ir_op_reg(result_reg, expr->type);
            
            // 使用 MEMBER_ACCESS 指令，src1为对象，src2为成员名（作为符号）
            char *member_name = copy_name(expr->as.member.member_name, expr->as.member.member_name_length);
            CnIrOperand member_sym = cn_ir_op_symbol(member_name, NULL);
            free(member_name);
            
            emit(ctx, cn_ir_inst_new(CN_IR_INST_MEMBER_ACCESS, result, object_op, member_sym));
            return result;
        }
        case CN_AST_EXPR_STRUCT_LITERAL: {
            // 结构体字面量：使用 AST 表达式操作数，在 cgen 阶段生成 C 复合字面量
            return cn_ir_op_ast_expr(expr, expr->type);
        }
        case CN_AST_EXPR_CAST: {
            // 类型转换表达式：生成操作数的IR，然后标记类型转换
            // 在cgen阶段生成C语言的类型转换语法
            CnIrOperand operand = cn_ir_gen_expr(ctx, expr->as.cast.operand);
            if (operand.kind == CN_IR_OP_NONE) {
                return cn_ir_op_none();
            }
            // 使用AST表达式操作数，在cgen阶段生成类型转换
            return cn_ir_op_ast_expr(expr, expr->as.cast.target_type);
        }
        default:
            return cn_ir_op_none();
    }
}

// 生成语句的 IR
void cn_ir_gen_stmt(CnIrGenContext *ctx, CnAstStmt *stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
        case CN_AST_STMT_VAR_DECL: {
            // 变量声明：ALLOCA + 可选 STORE
            CnAstVarDecl *decl = &stmt->as.var_decl;
            char *name = copy_name(decl->name, decl->name_length);
            /*
             * 对于使用 "变量" 关键字的声明，decl->declared_type 可能为 NULL，
             * 此时我们优先使用已在语义分析阶段推断出的初始值类型。
             * 如果仍然没有类型，尝试从符号表获取。
             */
            CnType *decl_type = decl->declared_type;
            if (!decl_type && decl->initializer && decl->initializer->type) {
                decl_type = decl->initializer->type;
            }
            // 如果仍然没有类型，尝试从符号表获取
            if (!decl_type && ctx->current_scope) {
                CnSemSymbol *sym = cn_sem_scope_lookup(ctx->current_scope,
                                                       decl->name,
                                                       decl->name_length);
                if (sym && sym->type) {
                    decl_type = sym->type;
                }
            }
            // 如果仍然没有类型，使用默认的整数类型
            // 这避免了生成 "void 变量名;" 的错误
            if (!decl_type) {
                decl_type = cn_type_new_primitive(CN_TYPE_INT);
            }
            
            // 检查是否为静态局部变量
            if (decl->is_static) {
                // 静态变量：创建静态变量结构并添加到函数
                CnIrStaticVar *static_var = (CnIrStaticVar *)malloc(sizeof(CnIrStaticVar));
                if (static_var) {
                    static_var->name = strdup(name);
                    static_var->name_length = decl->name_length;
                    static_var->type = decl_type;
                    static_var->next = NULL;
                    
                    // 处理初始化值
                    if (decl->initializer) {
                        // 静态变量的初始化必须是编译时常量
                        // 直接从AST表达式获取常量值
                        if (decl->initializer->kind == CN_AST_EXPR_INTEGER_LITERAL) {
                            static_var->initializer = cn_ir_op_imm_int(
                                decl->initializer->as.integer_literal.value, decl_type);
                        } else if (decl->initializer->kind == CN_AST_EXPR_FLOAT_LITERAL) {
                            static_var->initializer = cn_ir_op_imm_float(
                                decl->initializer->as.float_literal.value, decl_type);
                        } else if (decl->initializer->kind == CN_AST_EXPR_CHAR_LITERAL) {
                            // 字符字面量：作为整数值处理
                            static_var->initializer = cn_ir_op_imm_int(
                                decl->initializer->as.char_literal.value, decl_type);
                        } else if (decl->initializer->kind == CN_AST_EXPR_BOOL_LITERAL) {
                            static_var->initializer = cn_ir_op_imm_int(
                                decl->initializer->as.bool_literal.value ? 1 : 0, decl_type);
                        } else if (decl->initializer->kind == CN_AST_EXPR_STRING_LITERAL) {
                            static_var->initializer = cn_ir_op_imm_str(
                                decl->initializer->as.string_literal.value, decl_type);
                        } else {
                            // 其他情况（应该已经被语义检查拒绝）
                            static_var->initializer = cn_ir_op_none();
                        }
                    } else {
                        // 无初始化，使用零值
                        static_var->initializer = cn_ir_op_none();
                    }
                    
                    cn_ir_function_add_static_var(ctx->current_func, static_var);
                    
                    // 同时添加到上下文的静态变量列表（用于标识符查找）
                    CnIrGenStaticVar *gen_static_var = (CnIrGenStaticVar *)malloc(sizeof(CnIrGenStaticVar));
                    if (gen_static_var) {
                        gen_static_var->name = strdup(name);
                        gen_static_var->name_length = decl->name_length;
                        gen_static_var->next = ctx->current_static_vars;
                        ctx->current_static_vars = gen_static_var;
                    }
                }
                free(name);
                break;
            }
            
            // 检查是否为数组类型
            if (decl_type && decl_type->kind == CN_TYPE_ARRAY) {
                // 数组类型：先声明变量，再调用 cn_rt_array_alloc 分配内存
                CnIrOperand addr = cn_ir_op_symbol(name, decl_type);
                
                // 先发出ALLOCA指令声明变量
                emit(ctx, cn_ir_inst_new(CN_IR_INST_ALLOCA, addr, cn_ir_op_none(), cn_ir_op_none()));
                
                // 获取数组大小和元素大小
                size_t array_size = decl_type->as.array.length;
                size_t elem_size = 8; // 默认8字节
                
                // 根据元素类型确定元素大小
                CnType *elem_type = decl_type->as.array.element_type;
                if (elem_type) {
                    switch (elem_type->kind) {
                        case CN_TYPE_INT:
                        case CN_TYPE_UNKNOWN:
                            elem_size = sizeof(long long);
                            break;
                        case CN_TYPE_FLOAT:
                            elem_size = sizeof(double);
                            break;
                        case CN_TYPE_STRING:
                            elem_size = sizeof(char*);
                            break;
                        case CN_TYPE_BOOL:
                            elem_size = sizeof(int);
                            break;
                        case CN_TYPE_POINTER:
                            elem_size = sizeof(void*);
                            break;
                        default:
                            elem_size = 8;
                            break;
                    }
                }
                
                // 生成: addr = cn_rt_array_alloc(elem_size, array_size)
                CnIrInst *alloc_inst = cn_ir_inst_new(CN_IR_INST_CALL, addr,
                    cn_ir_op_symbol("cn_rt_array_alloc", NULL), cn_ir_op_none());
                alloc_inst->extra_args_count = 2;
                alloc_inst->extra_args = malloc(2 * sizeof(CnIrOperand));
                alloc_inst->extra_args[0] = cn_ir_op_imm_int(elem_size, NULL);
                alloc_inst->extra_args[1] = cn_ir_op_imm_int(array_size, NULL);
                emit(ctx, alloc_inst);
                
                free(name);
                break;
            }
            
            // 普通局部变量：ALLOCA + 可选 STORE
            // 使用唯一变量名避免同一函数内不同作用域的同名变量冲突
            char *unique_name = make_unique_local_name(name);
            // 添加变量名映射（原始名 -> 唯一名）
            add_local_var_mapping(ctx, name, unique_name);
            free(name);
            CnIrOperand addr = cn_ir_op_symbol(unique_name, decl_type);
            free(unique_name);
            emit(ctx, cn_ir_inst_new(CN_IR_INST_ALLOCA, addr, cn_ir_op_none(), cn_ir_op_none()));
            if (decl->initializer) {
                CnIrOperand init_val = cn_ir_gen_expr(ctx, decl->initializer);
                emit(ctx, cn_ir_inst_new(CN_IR_INST_STORE, addr, init_val, cn_ir_op_none()));
            }
            break;
        }
        case CN_AST_STMT_EXPR: {
            cn_ir_gen_expr(ctx, stmt->as.expr.expr);
            break;
        }
        case CN_AST_STMT_RETURN: {
            CnIrOperand ret_val = cn_ir_op_none();
            if (stmt->as.return_stmt.expr) {
                ret_val = cn_ir_gen_expr(ctx, stmt->as.return_stmt.expr);
            }
            emit(ctx, cn_ir_inst_new(CN_IR_INST_RET, cn_ir_op_none(), ret_val, cn_ir_op_none()));
            break;
        }
        case CN_AST_STMT_IF: {
            CnAstIfStmt *if_stmt = &stmt->as.if_stmt;
            CnIrBasicBlock *then_block = cn_ir_basic_block_new(make_block_name(ctx, "if_then"));
            CnIrBasicBlock *else_block = if_stmt->else_block 
                                         ? cn_ir_basic_block_new(make_block_name(ctx, "if_else"))
                                         : NULL;
            CnIrBasicBlock *merge_block = cn_ir_basic_block_new(make_block_name(ctx, "if_merge"));

            cn_ir_function_add_block(ctx->current_func, then_block);
            if (else_block) cn_ir_function_add_block(ctx->current_func, else_block);
            cn_ir_function_add_block(ctx->current_func, merge_block);

            CnIrOperand cond = cn_ir_gen_expr(ctx, if_stmt->condition);
            CnIrBasicBlock *false_target = else_block ? else_block : merge_block;
            emit(ctx, cn_ir_inst_new(CN_IR_INST_BRANCH, cn_ir_op_label(then_block),
                                     cond, cn_ir_op_label(false_target)));
            cn_ir_basic_block_connect(ctx->current_block, then_block);
            cn_ir_basic_block_connect(ctx->current_block, false_target);

            switch_to_block(ctx, then_block);
            cn_ir_gen_block(ctx, if_stmt->then_block);
            emit(ctx, cn_ir_inst_new(CN_IR_INST_JUMP, cn_ir_op_label(merge_block),
                                     cn_ir_op_none(), cn_ir_op_none()));
            cn_ir_basic_block_connect(ctx->current_block, merge_block);

            if (else_block) {
                switch_to_block(ctx, else_block);
                cn_ir_gen_block(ctx, if_stmt->else_block);
                emit(ctx, cn_ir_inst_new(CN_IR_INST_JUMP, cn_ir_op_label(merge_block),
                                         cn_ir_op_none(), cn_ir_op_none()));
                cn_ir_basic_block_connect(ctx->current_block, merge_block);
            }

            switch_to_block(ctx, merge_block);
            break;
        }
        case CN_AST_STMT_WHILE: {
            CnAstWhileStmt *while_stmt = &stmt->as.while_stmt;
            CnIrBasicBlock *cond_block = cn_ir_basic_block_new(make_block_name(ctx, "while_cond"));
            CnIrBasicBlock *body_block = cn_ir_basic_block_new(make_block_name(ctx, "while_body"));
            CnIrBasicBlock *exit_block = cn_ir_basic_block_new(make_block_name(ctx, "while_exit"));

            cn_ir_function_add_block(ctx->current_func, cond_block);
            cn_ir_function_add_block(ctx->current_func, body_block);
            cn_ir_function_add_block(ctx->current_func, exit_block);

            emit(ctx, cn_ir_inst_new(CN_IR_INST_JUMP, cn_ir_op_label(cond_block),
                                     cn_ir_op_none(), cn_ir_op_none()));
            cn_ir_basic_block_connect(ctx->current_block, cond_block);

            switch_to_block(ctx, cond_block);
            CnIrOperand cond = cn_ir_gen_expr(ctx, while_stmt->condition);
            emit(ctx, cn_ir_inst_new(CN_IR_INST_BRANCH, cn_ir_op_label(body_block),
                                     cond, cn_ir_op_label(exit_block)));
            cn_ir_basic_block_connect(ctx->current_block, body_block);
            cn_ir_basic_block_connect(ctx->current_block, exit_block);

            CnIrBasicBlock *saved_exit = ctx->loop_exit;
            CnIrBasicBlock *saved_continue = ctx->loop_continue;
            ctx->loop_exit = exit_block;
            ctx->loop_continue = cond_block;

            switch_to_block(ctx, body_block);
            cn_ir_gen_block(ctx, while_stmt->body);
            emit(ctx, cn_ir_inst_new(CN_IR_INST_JUMP, cn_ir_op_label(cond_block),
                                     cn_ir_op_none(), cn_ir_op_none()));
            cn_ir_basic_block_connect(ctx->current_block, cond_block);

            ctx->loop_exit = saved_exit;
            ctx->loop_continue = saved_continue;

            switch_to_block(ctx, exit_block);
            break;
        }
        case CN_AST_STMT_FOR: {
            CnAstForStmt *for_stmt = &stmt->as.for_stmt;
            if (for_stmt->init) cn_ir_gen_stmt(ctx, for_stmt->init);

            CnIrBasicBlock *cond_block = cn_ir_basic_block_new(make_block_name(ctx, "for_cond"));
            CnIrBasicBlock *body_block = cn_ir_basic_block_new(make_block_name(ctx, "for_body"));
            CnIrBasicBlock *update_block = cn_ir_basic_block_new(make_block_name(ctx, "for_update"));
            CnIrBasicBlock *exit_block = cn_ir_basic_block_new(make_block_name(ctx, "for_exit"));

            cn_ir_function_add_block(ctx->current_func, cond_block);
            cn_ir_function_add_block(ctx->current_func, body_block);
            cn_ir_function_add_block(ctx->current_func, update_block);
            cn_ir_function_add_block(ctx->current_func, exit_block);

            emit(ctx, cn_ir_inst_new(CN_IR_INST_JUMP, cn_ir_op_label(cond_block),
                                     cn_ir_op_none(), cn_ir_op_none()));
            cn_ir_basic_block_connect(ctx->current_block, cond_block);

            switch_to_block(ctx, cond_block);
            if (for_stmt->condition) {
                CnIrOperand cond = cn_ir_gen_expr(ctx, for_stmt->condition);
                emit(ctx, cn_ir_inst_new(CN_IR_INST_BRANCH, cn_ir_op_label(body_block),
                                         cond, cn_ir_op_label(exit_block)));
                cn_ir_basic_block_connect(ctx->current_block, body_block);
                cn_ir_basic_block_connect(ctx->current_block, exit_block);
            } else {
                emit(ctx, cn_ir_inst_new(CN_IR_INST_JUMP, cn_ir_op_label(body_block),
                                         cn_ir_op_none(), cn_ir_op_none()));
                cn_ir_basic_block_connect(ctx->current_block, body_block);
            }

            CnIrBasicBlock *saved_exit = ctx->loop_exit;
            CnIrBasicBlock *saved_continue = ctx->loop_continue;
            ctx->loop_exit = exit_block;
            ctx->loop_continue = update_block;

            switch_to_block(ctx, body_block);
            cn_ir_gen_block(ctx, for_stmt->body);
            emit(ctx, cn_ir_inst_new(CN_IR_INST_JUMP, cn_ir_op_label(update_block),
                                     cn_ir_op_none(), cn_ir_op_none()));
            cn_ir_basic_block_connect(ctx->current_block, update_block);

            switch_to_block(ctx, update_block);
            if (for_stmt->update) cn_ir_gen_expr(ctx, for_stmt->update);
            emit(ctx, cn_ir_inst_new(CN_IR_INST_JUMP, cn_ir_op_label(cond_block),
                                     cn_ir_op_none(), cn_ir_op_none()));
            cn_ir_basic_block_connect(ctx->current_block, cond_block);

            ctx->loop_exit = saved_exit;
            ctx->loop_continue = saved_continue;

            switch_to_block(ctx, exit_block);
            break;
        }
        case CN_AST_STMT_BREAK: {
            if (ctx->loop_exit) {
                emit(ctx, cn_ir_inst_new(CN_IR_INST_JUMP, cn_ir_op_label(ctx->loop_exit),
                                         cn_ir_op_none(), cn_ir_op_none()));
            }
            break;
        }
        case CN_AST_STMT_CONTINUE: {
            if (ctx->loop_continue) {
                emit(ctx, cn_ir_inst_new(CN_IR_INST_JUMP, cn_ir_op_label(ctx->loop_continue),
                                         cn_ir_op_none(), cn_ir_op_none()));
            }
            break;
        }
        case CN_AST_STMT_SWITCH: {
            // switch 语句的 IR 生成：转换为一系列 if-else 语句
            CnAstSwitchStmt *switch_stmt = &stmt->as.switch_stmt;
            CnIrBasicBlock *merge_block = cn_ir_basic_block_new(make_block_name(ctx, "switch_merge"));
            CnIrBasicBlock *old_break_target = ctx->loop_exit;
            
            // 生成 switch 表达式
            CnIrOperand switch_val = cn_ir_gen_expr(ctx, switch_stmt->expr);
            
            // 设置 break 跳转目标
            ctx->loop_exit = merge_block;
            
            // 为每个 case 创建基本块
            CnIrBasicBlock **case_blocks = (CnIrBasicBlock **)malloc(sizeof(CnIrBasicBlock *) * switch_stmt->case_count);
            CnIrBasicBlock *default_block = NULL;
            
            // 首先创建所有 case 块
            for (size_t i = 0; i < switch_stmt->case_count; i++) {
                if (switch_stmt->cases[i].value == NULL) {
                    // default 分支
                    default_block = cn_ir_basic_block_new(make_block_name(ctx, "case_default"));
                    case_blocks[i] = default_block;
                } else {
                    case_blocks[i] = cn_ir_basic_block_new(make_block_name(ctx, "case_body"));
                }
            }
            
            // 生成比较和跳转指令（在当前块中）
            CnIrBasicBlock *next_check_block = NULL;
            size_t non_default_case_index = 0;
            for (size_t i = 0; i < switch_stmt->case_count; i++) {
                if (switch_stmt->cases[i].value == NULL) {
                    // default 分支：跳过比较
                    continue;
                }
                
                // 生成 case 值
                CnIrOperand case_val = cn_ir_gen_expr(ctx, switch_stmt->cases[i].value);
                
                // 比较 switch_val == case_val
                int cmp_reg = alloc_reg(ctx);
                CnIrOperand cmp_result = cn_ir_op_reg(cmp_reg, cn_type_new_primitive(CN_TYPE_BOOL));
                emit(ctx, cn_ir_inst_new(CN_IR_INST_EQ, cmp_result, switch_val, case_val));
                
                non_default_case_index++;
                // 创建下一个检查块
                size_t non_default_count = 0;
                for (size_t j = 0; j < switch_stmt->case_count; j++) {
                    if (switch_stmt->cases[j].value != NULL) non_default_count++;
                }
                
                if (non_default_case_index < non_default_count) {
                    next_check_block = cn_ir_basic_block_new(make_block_name(ctx, "switch_check"));
                    cn_ir_function_add_block(ctx->current_func, next_check_block);
                } else {
                    // 最后一个 case：如果没有 default，跳到 merge；否则跳到 default
                    next_check_block = default_block ? default_block : merge_block;
                    // 注意：如果是 default_block，不要在这里 add，后面会统一添加
                    // 如果是 merge_block，也不要在这里 add，后面会添加
                }
                
                // 条件跳转：如果匹配则跳到 case 体，否则跳到下一个检查
                emit(ctx, cn_ir_inst_new(CN_IR_INST_BRANCH, cn_ir_op_label(case_blocks[i]),
                                         cmp_result, cn_ir_op_label(next_check_block)));
                cn_ir_basic_block_connect(ctx->current_block, case_blocks[i]);
                cn_ir_basic_block_connect(ctx->current_block, next_check_block);
                
                // 切换到下一个检查块
                switch_to_block(ctx, next_check_block);
            }
            
            // 如果当前在 next_check_block 且没有 default，跳到 merge
            if (!default_block && next_check_block != merge_block) {
                emit(ctx, cn_ir_inst_new(CN_IR_INST_JUMP, cn_ir_op_label(merge_block),
                                         cn_ir_op_none(), cn_ir_op_none()));
                cn_ir_basic_block_connect(ctx->current_block, merge_block);  // 设置控制流连接
            }
            
            // 生成每个 case 的代码体
            for (size_t i = 0; i < switch_stmt->case_count; i++) {
                // 先添加块到函数，然后切换并生成代码
                // 注意：需要检查块是否已经在链表中，避免重复添加
                bool already_added = false;
                CnIrBasicBlock *check_block = ctx->current_func->first_block;
                while (check_block) {
                    if (check_block == case_blocks[i]) {
                        already_added = true;
                        break;
                    }
                    check_block = check_block->next;
                }
                
                if (!already_added) {
                    cn_ir_function_add_block(ctx->current_func, case_blocks[i]);
                }
                
                switch_to_block(ctx, case_blocks[i]);
                cn_ir_gen_block(ctx, switch_stmt->cases[i].body);
                
                // 如果 case 体没有显式 break，自动添加跳转到 merge
                // （C 语言 switch 有 fall-through，但这里我们默认每个 case 自动 break）
                emit(ctx, cn_ir_inst_new(CN_IR_INST_JUMP, cn_ir_op_label(merge_block),
                                         cn_ir_op_none(), cn_ir_op_none()));
                cn_ir_basic_block_connect(ctx->current_block, merge_block);  // 设置控制流连接
            }
            
            // 添加 merge_block 到函数，确保它在最后
            cn_ir_function_add_block(ctx->current_func, merge_block);
            
            free(case_blocks);
            
            // 切换到 merge 块，确保后续代码在 merge 块中生成
            switch_to_block(ctx, merge_block);
            
            // 恢复 break 目标
            ctx->loop_exit = old_break_target;
            break;
        }
        case CN_AST_STMT_BLOCK: {
            cn_ir_gen_block(ctx, stmt->as.block);
            break;
        }
        case CN_AST_STMT_STRUCT_DECL: {
            // 结构体声明：在IR层面不需要特殊处理，类型信息已经在语义分析阶段处理
            // 结构体定义不生成运行时代码，只是类型信息
            break;
        }
        case CN_AST_STMT_ENUM_DECL: {
            // 枚举声明：在IR层面不需要特殊处理，类型信息已经在语义分析阶段处理
            // 枚举成员作为常量值已经注册到符号表
            break;
        }
        default:
            break;
    }
}

void cn_ir_gen_block(CnIrGenContext *ctx, CnAstBlockStmt *block) {
    if (!block) return;
    for (size_t i = 0; i < block->stmt_count; i++) {
        cn_ir_gen_stmt(ctx, block->stmts[i]);
    }
}

void cn_ir_gen_function(CnIrGenContext *ctx, CnAstFunctionDecl *func, CnSemScope *parent_scope) {
    if (!func) return;
    
    // 使用 parent_scope 参数，如果没有提供则使用全局作用域
    if (!parent_scope) {
        parent_scope = ctx->global_scope;
    }

    // 确保函数名是 null-terminated
    char *name = malloc(func->name_length + 1);
    memcpy(name, func->name, func->name_length);
    name[func->name_length] = '\0';

    // 使用显式声明的返回类型，如果没有则默认为整数类型
    CnType *return_type = func->return_type;
    if (!return_type) {
        // 如果没有显式声明返回类型，默认为整数类型（后续可通过返回语句推断）
        return_type = cn_type_new_primitive(CN_TYPE_INT);
    } else if (return_type->kind == CN_TYPE_STRUCT && ctx->global_scope) {
        // 特殊处理：如果返回类型是结构体类型，可能是枚举类型或类类型
        // 需要从符号表查找真实类型
        CnSemSymbol *type_sym = cn_sem_scope_lookup(ctx->global_scope,
                                return_type->as.struct_type.name,
                                return_type->as.struct_type.name_length);
        if (type_sym && type_sym->type) {
            if (type_sym->kind == CN_SEM_SYMBOL_ENUM) {
                // 替换为枚举类型
                return_type = type_sym->type;
            } else if (type_sym->kind == CN_SEM_SYMBOL_STRUCT) {
                // 替换为完整的结构体/类类型
                return_type = type_sym->type;
            }
        }
    }

    CnIrFunction *ir_func = cn_ir_function_new(name, return_type);
    free(name);
    ctx->current_func = ir_func;
    
    // 重置局部变量计数器和清空映射表（每个函数独立）
    reset_local_var_counter();
    free_local_var_map(ctx);

    // 中断处理函数特殊处理：设置中断属性
    if (func->is_interrupt_handler) {
        ir_func->is_interrupt_handler = 1;
        ir_func->interrupt_vector = func->interrupt_vector;
    }

    // 添加参数
    for (size_t i = 0; i < func->parameter_count; i++) {
        char *param_name = copy_name(func->parameters[i].name, func->parameters[i].name_length);
        CnType *param_type = func->parameters[i].declared_type;
        
        // 特殊处理：如果参数类型是结构体类型，可能是枚举类型或类类型
        // 需要从符号表查找真实类型
        if (param_type && param_type->kind == CN_TYPE_STRUCT && ctx->global_scope) {
            CnSemSymbol *type_sym = cn_sem_scope_lookup(ctx->global_scope,
                                    param_type->as.struct_type.name,
                                    param_type->as.struct_type.name_length);
            if (type_sym && type_sym->type) {
                if (type_sym->kind == CN_SEM_SYMBOL_ENUM) {
                    param_type = type_sym->type;
                } else if (type_sym->kind == CN_SEM_SYMBOL_STRUCT) {
                    param_type = type_sym->type;
                }
            }
        }
        
        // 为参数生成唯一名称（使用 cn_var_ 前缀）
        size_t param_name_len = strlen(param_name);
        char *unique_param_name = malloc(7 + param_name_len + 1); // "cn_var_" + name + "\0"
        if (unique_param_name) {
            snprintf(unique_param_name, 7 + param_name_len + 1, "cn_var_%s", param_name);
            
            // 添加参数到局部变量映射表，这样在标识符查找时能优先找到参数
            add_local_var_mapping(ctx, param_name, unique_param_name);
        }
        
        CnIrOperand param = cn_ir_op_symbol(unique_param_name ? unique_param_name : param_name, param_type);
        if (unique_param_name) free(unique_param_name);
        free(param_name);
        cn_ir_function_add_param(ir_func, param);
    }

    // 检查是否为函数原型声明（无函数体）
    if (func->is_prototype) {
        // 函数原型声明：只声明不定义，不生成函数体
        ir_func->is_prototype = 1;
    } else if (func->body) {
        // 函数定义：生成函数体
        // 创建入口基本块
        CnIrBasicBlock *entry = cn_ir_basic_block_new("entry");
        cn_ir_function_add_block(ir_func, entry);
        ctx->current_block = entry;
        
        // 为函数创建作用域（父级为传入的 parent_scope）
        CnSemScope *func_scope = cn_sem_scope_new(CN_SEM_SCOPE_FUNCTION, parent_scope);
        ctx->current_scope = func_scope;

        // 生成函数体
        cn_ir_gen_block(ctx, func->body);
        
        // 恢复作用域
        cn_sem_scope_free(func_scope);
        ctx->current_scope = ctx->global_scope;
    }
    
    // 清理静态变量列表
    CnIrGenStaticVar *sv = ctx->current_static_vars;
    while (sv) {
        CnIrGenStaticVar *next = sv->next;
        if (sv->name) free(sv->name);
        free(sv);
        sv = next;
    }
    ctx->current_static_vars = NULL;

    // 添加到模块
    if (!ctx->module->first_func) {
        ctx->module->first_func = ir_func;
        ctx->module->last_func = ir_func;
    } else {
        ctx->module->last_func->next = ir_func;
        ctx->module->last_func = ir_func;
    }
}

CnIrModule *cn_ir_gen_program(CnAstProgram *program, CnSemScope *global_scope, CnTargetTriple target, CnCompileMode mode) {
    if (!program) return NULL;

    CnIrGenContext *ctx = cn_ir_gen_context_new();
    ctx->global_scope = global_scope;
    ctx->current_scope = global_scope;
    ctx->program = program;  // 保存程序指针，用于查找类定义
    
    if (ctx->module) {
        ctx->module->target = target;
        ctx->module->compile_mode = mode;
    }

    // 生成全局变量的 IR
    for (size_t i = 0; i < program->global_var_count; i++) {
        CnAstStmt *var_stmt = program->global_vars[i];
        if (!var_stmt || var_stmt->kind != CN_AST_STMT_VAR_DECL) {
            continue;
        }
        
        CnAstVarDecl *var_decl = &var_stmt->as.var_decl;
        CnIrGlobalVar *global = (CnIrGlobalVar *)malloc(sizeof(CnIrGlobalVar));
        if (global) {
            // 为变量名分配并复制字符串
            char *name_copy = (char *)malloc(var_decl->name_length + 1);
            if (name_copy) {
                memcpy(name_copy, var_decl->name, var_decl->name_length);
                name_copy[var_decl->name_length] = '\0';
                global->name = name_copy;
            } else {
                global->name = NULL;
            }
            
            // 获取类型：优先使用显式声明的类型，否则使用初始化表达式的类型
            CnType *var_type = var_decl->declared_type;
            if (!var_type && var_decl->initializer) {
                var_type = var_decl->initializer->type;
            }
            global->type = var_type;
            global->is_const = var_decl->is_const;
            
            // 处理初始化表达式
            if (var_decl->initializer) {
                // 对于全局变量，只支持常量表达式初始化
                if (var_decl->initializer->kind == CN_AST_EXPR_INTEGER_LITERAL) {
                    global->initializer = cn_ir_op_imm_int(
                        var_decl->initializer->as.integer_literal.value,
                        var_type);
                } else if (var_decl->initializer->kind == CN_AST_EXPR_FLOAT_LITERAL) {
                    global->initializer = cn_ir_op_imm_float(
                        var_decl->initializer->as.float_literal.value,
                        var_type);
                } else if (var_decl->initializer->kind == CN_AST_EXPR_CHAR_LITERAL) {
                    // 字符字面量：作为整数值处理
                    global->initializer = cn_ir_op_imm_int(
                        var_decl->initializer->as.char_literal.value,
                        var_type);
                } else if (var_decl->initializer->kind == CN_AST_EXPR_BOOL_LITERAL) {
                    // 布尔字面量：作为整数值处理
                    global->initializer = cn_ir_op_imm_int(
                        var_decl->initializer->as.bool_literal.value ? 1 : 0,
                        var_type);
                } else if (var_decl->initializer->kind == CN_AST_EXPR_STRING_LITERAL) {
                    // 字符串字面量
                    global->initializer = cn_ir_op_imm_str(
                        var_decl->initializer->as.string_literal.value,
                        var_type);
                } else {
                    // 其他类型的初始化表达式暂不支持，使用0初始化
                    global->initializer = cn_ir_op_imm_int(0, var_type);
                }
            } else {
                // 没有初始化表达式，默认为0
                global->initializer = cn_ir_op_imm_int(0, var_type);
            }
            
            global->next = NULL;
            
            // 添加到模块
            if (!ctx->module->first_global) {
                ctx->module->first_global = global;
                ctx->module->last_global = global;
            } else {
                ctx->module->last_global->next = global;
                ctx->module->last_global = global;
            }
        }
    }

    // 生成全局函数的 IR
    for (size_t i = 0; i < program->function_count; i++) {
        cn_ir_gen_function(ctx, program->functions[i], NULL);
    }

    CnIrModule *module = ctx->module;
    ctx->module = NULL;
    cn_ir_gen_context_free(ctx);
    return module;
}
