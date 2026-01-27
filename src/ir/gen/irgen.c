#include "cnlang/ir/irgen.h"
#include "cnlang/frontend/semantics.h"
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

CnIrGenContext *cn_ir_gen_context_new() {
    CnIrGenContext *ctx = malloc(sizeof(CnIrGenContext));
    ctx->module = cn_ir_module_new();
    ctx->current_func = NULL;
    ctx->current_block = NULL;
    ctx->loop_exit = NULL;
    ctx->loop_continue = NULL;
    ctx->global_scope = NULL;
    ctx->current_scope = NULL;
    return ctx;
}

void cn_ir_gen_context_free(CnIrGenContext *ctx) {
    if (ctx) free(ctx);
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
        case CN_AST_EXPR_BOOL_LITERAL: {
            // 布尔字面量直接返回立即数（0 或 1）
            return cn_ir_op_imm_int(expr->as.bool_literal.value ? 1 : 0,
                                    cn_type_new_primitive(CN_TYPE_BOOL));
        }
        case CN_AST_EXPR_IDENTIFIER: {
            // 标识符：首先检查是否为枚举成员
            if (ctx->current_scope) {
                CnSemSymbol *sym = cn_sem_scope_lookup(ctx->current_scope, 
                                                       expr->as.identifier.name,
                                                       expr->as.identifier.name_length);
                if (sym && sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                    // 枚举成员：直接返回其常量值
                    return cn_ir_op_imm_int(sym->as.enum_value, cn_type_new_primitive(CN_TYPE_INT));
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
            
            // 普通变量：生成 LOAD 指令从变量地址加载值
            int dest_reg = alloc_reg(ctx);
            CnIrOperand dest = cn_ir_op_reg(dest_reg, expr->type);
            char *name = copy_name(expr->as.identifier.name, expr->as.identifier.name_length);
            CnIrOperand src = cn_ir_op_symbol(name, expr->type);
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
                    if (left_type->kind == CN_TYPE_INT) {
                        convert_func = "cn_rt_int_to_string";
                    } else if (left_type->kind == CN_TYPE_BOOL) {
                        convert_func = "cn_rt_bool_to_string";
                    } else if (left_type->kind == CN_TYPE_FLOAT) {
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
                    if (right_type->kind == CN_TYPE_INT) {
                        convert_func = "cn_rt_int_to_string";
                    } else if (right_type->kind == CN_TYPE_BOOL) {
                        convert_func = "cn_rt_bool_to_string";
                    } else if (right_type->kind == CN_TYPE_FLOAT) {
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
                // 普通变量赋值
                char *name = copy_name(target->as.identifier.name, target->as.identifier.name_length);
                CnIrOperand addr = cn_ir_op_symbol(name, target->type);
                free(name);
                emit(ctx, cn_ir_inst_new(CN_IR_INST_STORE, addr, value, cn_ir_op_none()));
            } else if (target->kind == CN_AST_EXPR_INDEX) {
                // 数组索引赋值 arr[index] = value
                CnIrOperand array_op = cn_ir_gen_expr(ctx, target->as.index.array);
                CnIrOperand index_op = cn_ir_gen_expr(ctx, target->as.index.index);
                
                // 调用 cn_rt_array_set_element(数组, 索引, &value, 元素大小)
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

            if (expr->as.logical.op == CN_AST_LOGICAL_OP_AND) {
                // AND: 左为假则短路
                emit(ctx, cn_ir_inst_new(CN_IR_INST_BRANCH, cn_ir_op_label(rhs_block),
                                         left, cn_ir_op_label(merge_block)));
            } else {
                // OR: 左为真则短路
                emit(ctx, cn_ir_inst_new(CN_IR_INST_BRANCH, cn_ir_op_label(merge_block),
                                         left, cn_ir_op_label(rhs_block)));
            }

            switch_to_block(ctx, rhs_block);
            CnIrOperand right = cn_ir_gen_expr(ctx, expr->as.logical.right);
            emit(ctx, cn_ir_inst_new(CN_IR_INST_JUMP, cn_ir_op_label(merge_block),
                                     cn_ir_op_none(), cn_ir_op_none()));

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
                    CnIrOperand addr = cn_ir_op_symbol(name, expr->type);
                    free(name);
                    emit(ctx, cn_ir_inst_new(CN_IR_INST_ADDRESS_OF, dest, addr, cn_ir_op_none()));
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
                // 检查是否是 "打印" 函数
                else if (strcmp(name, "打印") == 0 && expr->as.call.argument_count == 1) {
                    // 打印函数根据参数类型映射到不同的运行时函数
                    CnType *arg_type = expr->as.call.arguments[0]->type;
                    if (arg_type && arg_type->kind == CN_TYPE_INT) {
                        func_name = "cn_rt_print_int";
                    } else if (arg_type && arg_type->kind == CN_TYPE_BOOL) {
                        func_name = "cn_rt_print_bool";
                    } else if (arg_type && arg_type->kind == CN_TYPE_FLOAT) {
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
                } else {
                    // 检查被调用函数是否来自模块
                    bool handled = false;
                    if (ctx->current_scope) {
                        CnSemSymbol *func_sym = cn_sem_scope_lookup(ctx->current_scope,
                                                                    expr->as.call.callee->as.identifier.name,
                                                                    expr->as.call.callee->as.identifier.name_length);
                        
                        if (func_sym && func_sym->decl_scope && func_sym->decl_scope->kind == CN_SEM_SCOPE_MODULE) {
                            // 函数来自模块：生成带模块前缀的名称
                            CnSemScope *global = ctx->current_scope;
                            while (global && global->parent) {
                                global = global->parent;
                            }
                            
                            if (global) {
                                CnSemSymbolNode *node = global->symbols;
                                while (node) {
                                    if (node->symbol.kind == CN_SEM_SYMBOL_MODULE && 
                                        node->symbol.as.module_scope == func_sym->decl_scope) {
                                        // 找到模块，生成带模块前缀的名称
                                        // 格式：cn_module_模块名__函数名（cgen会再加 cn_func_ 前缀）
                                        size_t module_name_len = node->symbol.name_length;
                                        size_t func_name_len = strlen(name);
                                        size_t prefix_len = strlen("cn_module_");
                                        size_t total_len = prefix_len + module_name_len + 2 + func_name_len + 1;
                                        
                                        char *qualified_name = malloc(total_len);
                                        if (qualified_name) {
                                            snprintf(qualified_name, total_len, "cn_module_%.*s__%s",
                                                    (int)module_name_len, node->symbol.name, name);
                                            free(name);
                                            callee = cn_ir_op_symbol(qualified_name, expr->as.call.callee->type);
                                            free(qualified_name);
                                            handled = true;
                                        }
                                        break;
                                    }
                                    node = node->next;
                                }
                            }
                        }
                    }
                    
                    // 如果没有特殊处理，使用普通名称
                    if (!handled) {
                        callee = cn_ir_op_symbol(name, expr->as.call.callee->type);
                        free(name);
                    }
                }
            } 
            // 特殊处理：方法风格调用 arr.长度()
            else if (expr->as.call.callee->kind == CN_AST_EXPR_MEMBER_ACCESS &&
                     expr->as.call.callee->as.member.member_name_length == strlen("长度") &&
                     strncmp(expr->as.call.callee->as.member.member_name, "长度",
                             expr->as.call.callee->as.member.member_name_length) == 0) {
                // 将方法风格调用转换为函数风格：arr.长度() -> 长度(arr)
                CnType *obj_type = expr->as.call.callee->as.member.object->type;
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
            // 如果有返回值，分配结果寄存器
            if (expr->type && expr->type->kind != CN_TYPE_VOID) {
                int dest_reg = alloc_reg(ctx);
                call_inst->dest = cn_ir_op_reg(dest_reg, expr->type);
            }
            emit(ctx, call_inst);
            return call_inst->dest;
        }
        case CN_AST_EXPR_ARRAY_LITERAL: {
            // 数组字面量：调用 cn_rt_array_alloc 分配数组，然后逐个设置元素
            size_t elem_count = expr->as.array_literal.element_count;
            
            // 确定元素类型和大小
            CnType *elem_type = expr->type->as.array.element_type;
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
            
            // 调用 cn_rt_array_get_element(数组, 索引, 元素大小)
            CnIrInst *get_inst = cn_ir_inst_new(CN_IR_INST_CALL, cn_ir_op_none(),
                                                  cn_ir_op_symbol("cn_rt_array_get_element", NULL),
                                                  cn_ir_op_none());
            get_inst->extra_args_count = 3;
            get_inst->extra_args = malloc(3 * sizeof(CnIrOperand));
            get_inst->extra_args[0] = array_op;
            get_inst->extra_args[1] = index_op;
            get_inst->extra_args[2] = cn_ir_op_imm_int(8, cn_type_new_primitive(CN_TYPE_INT));  // 元素大小
            
            int result_reg = alloc_reg(ctx);
            get_inst->dest = cn_ir_op_reg(result_reg, expr->type);
            emit(ctx, get_inst);
            
            return cn_ir_op_reg(result_reg, expr->type);
        }
        case CN_AST_EXPR_MEMBER_ACCESS: {
            // 成员访问：支持结构体 obj.member、模块 module.member 和枚举 enum.member
            
            // 检查是否为枚举成员访问（对象类型为 ENUM 表示枚举）
            if (expr->as.member.object->type && 
                expr->as.member.object->type->kind == CN_TYPE_ENUM &&
                expr->as.member.object->kind == CN_AST_EXPR_IDENTIFIER) {
                // 枚举成员访问：查找枚举成员的值
                CnSemSymbol *member_sym = cn_type_enum_find_member(
                    expr->as.member.object->type,
                    expr->as.member.member_name,
                    expr->as.member.member_name_length);
                
                if (member_sym && member_sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                    // 直接返回枚举成员的常量值
                    return cn_ir_op_imm_int(member_sym->as.enum_value, expr->type);
                }
                // 如果找不到，返回错误操作数
                return cn_ir_op_none();
            }
            
            // 检查是否为模块成员访问（对象类型为 VOID 表示模块）
            if (expr->as.member.object->type && 
                expr->as.member.object->type->kind == CN_TYPE_VOID &&
                expr->as.member.object->kind == CN_AST_EXPR_IDENTIFIER) {
                // 模块成员访问：生成带模块前缀的符号名
                // 格式：cn_module_模块名__成员名
                size_t module_name_len = expr->as.member.object->as.identifier.name_length;
                size_t member_name_len = expr->as.member.member_name_length;
                size_t prefix_len = strlen("cn_module_");
                size_t total_len = prefix_len + module_name_len + 2 + member_name_len + 1; // +2 for "__", +1 for '\0'
                
                char *qualified_name = malloc(total_len);
                if (qualified_name) {
                    snprintf(qualified_name, total_len, "cn_module_%.*s__%.*s",
                            (int)module_name_len, expr->as.member.object->as.identifier.name,
                            (int)member_name_len, expr->as.member.member_name);
                    
                    CnIrOperand result = cn_ir_op_symbol(qualified_name, expr->type);
                    free(qualified_name);
                    return result;
                }
                // 如果分配失败，继续使用默认处理
            }
            
            // 结构体成员访问：obj.member 或 ptr->member
            CnIrOperand object_op = cn_ir_gen_expr(ctx, expr->as.member.object);
            
            // 如果是箭头访问，先解引用指针
            if (expr->as.member.is_arrow) {
                int deref_reg = alloc_reg(ctx);
                CnIrOperand deref_op = cn_ir_op_reg(deref_reg, expr->as.member.object->type->as.pointer_to);
                emit(ctx, cn_ir_inst_new(CN_IR_INST_DEREF, deref_op, object_op, cn_ir_op_none()));
                object_op = deref_op;
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
             */
            CnType *decl_type = decl->declared_type;
            if (!decl_type && decl->initializer && decl->initializer->type) {
                decl_type = decl->initializer->type;
            }
            CnIrOperand addr = cn_ir_op_symbol(name, decl_type);
            free(name);
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
    }

    CnIrFunction *ir_func = cn_ir_function_new(name, return_type);
    free(name);
    ctx->current_func = ir_func;

    // 中断处理函数特殊处理：设置中断属性
    if (func->is_interrupt_handler) {
        ir_func->is_interrupt_handler = 1;
        ir_func->interrupt_vector = func->interrupt_vector;
    }

    // 添加参数
    for (size_t i = 0; i < func->parameter_count; i++) {
        char *param_name = copy_name(func->parameters[i].name, func->parameters[i].name_length);
        CnIrOperand param = cn_ir_op_symbol(param_name,
                                            func->parameters[i].declared_type);
        free(param_name);
        cn_ir_function_add_param(ir_func, param);
    }

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

    // 生成模块内函数的 IR
    for (size_t i = 0; i < program->module_count; i++) {
        CnAstStmt *module_stmt = program->modules[i];
        if (module_stmt && module_stmt->kind == CN_AST_STMT_MODULE_DECL) {
            CnAstModuleDecl *module_decl = &module_stmt->as.module_decl;
            
            // 查找模块作用域
            CnSemSymbol *module_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                                  module_decl->name,
                                                                  module_decl->name_length);
            CnSemScope *module_scope = NULL;
            if (module_sym && module_sym->kind == CN_SEM_SYMBOL_MODULE) {
                module_scope = module_sym->as.module_scope;
            }
            
            // 为模块内的每个函数生成 IR
            for (size_t j = 0; j < module_decl->function_count; j++) {
                CnAstFunctionDecl *func = module_decl->functions[j];
                if (func) {
                    // 生成带模块前缀的函数名：cn_module_模块名__函数名
                    char *prefixed_name = (char *)malloc(14 + module_decl->name_length + 2 + func->name_length + 1);
                    if (prefixed_name) {
                        snprintf(prefixed_name, 14 + module_decl->name_length + 2 + func->name_length + 1,
                                "cn_module_%.*s__%.*s",
                                (int)module_decl->name_length, module_decl->name,
                                (int)func->name_length, func->name);
                        
                        // 保存原函数名，生成IR时使用带前缀的名称
                        const char *original_name = func->name;
                        size_t original_length = func->name_length;
                        
                        // 临时替换函数名
                        func->name = prefixed_name;
                        func->name_length = strlen(prefixed_name);
                        
                        // 生成函数 IR，使用模块作用域作为父作用域
                        cn_ir_gen_function(ctx, func, module_scope);
                        
                        // 恢复原函数名
                        func->name = original_name;
                        func->name_length = original_length;
                        
                        free(prefixed_name);
                    }
                }
            }
        }
    }

    CnIrModule *module = ctx->module;
    ctx->module = NULL;
    cn_ir_gen_context_free(ctx);
    return module;
}
