#include "cnlang/ir/irgen.h"
#include "cnlang/frontend/semantics.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
        case CN_AST_EXPR_STRING_LITERAL: {
            // 字符串字面量
            CnIrOperand op;
            op.kind = CN_IR_OP_IMM_STR;
            char *str = copy_name(expr->as.string_literal.value, expr->as.string_literal.length);
            op.as.imm_str = str; // 注意：IR 目前不负责释放这个字符串，可能存在泄露，阶段 3 暂不处理
            op.type = cn_type_new_primitive(CN_TYPE_STRING);
            return op;
        }
        case CN_AST_EXPR_IDENTIFIER: {
            // 标识符：生成 LOAD 指令从变量地址加载值
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
            }
            return dest;
        }
        case CN_AST_EXPR_CALL: {
            // 函数调用
            CnIrOperand callee;
            const char *func_name = NULL;
            
            // 特殊处理：内置函数根据参数类型选择运行时函数
            if (expr->as.call.callee->kind == CN_AST_EXPR_IDENTIFIER) {
                char *name = copy_name(expr->as.call.callee->as.identifier.name,
                                       expr->as.call.callee->as.identifier.name_length);
                                       
                // 检查是否是 "长度" 函数
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
                    // 打印函数映射到 cn_rt_print_string 或 cn_rt_print_int
                    CnType *arg_type = expr->as.call.arguments[0]->type;
                    if (arg_type && arg_type->kind == CN_TYPE_INT) {
                        func_name = "cn_rt_print_int";
                    } else {
                        func_name = "cn_rt_print_string";
                    }
                    free(name);
                    callee = cn_ir_op_symbol(func_name, expr->as.call.callee->type);
                } else {
                    callee = cn_ir_op_symbol(name, expr->as.call.callee->type);
                    free(name);
                }
            } else {
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
        case CN_AST_STMT_BLOCK: {
            cn_ir_gen_block(ctx, stmt->as.block);
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

void cn_ir_gen_function(CnIrGenContext *ctx, CnAstFunctionDecl *func) {
    if (!func) return;

    // 确保函数名是 null-terminated
    char *name = malloc(func->name_length + 1);
    memcpy(name, func->name, func->name_length);
    name[func->name_length] = '\0';

    CnIrFunction *ir_func = cn_ir_function_new(name, cn_type_new_primitive(CN_TYPE_INT));
    free(name);
    ctx->current_func = ir_func;

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

    // 生成函数体
    cn_ir_gen_block(ctx, func->body);

    // 添加到模块
    if (!ctx->module->first_func) {
        ctx->module->first_func = ir_func;
        ctx->module->last_func = ir_func;
    } else {
        ctx->module->last_func->next = ir_func;
        ctx->module->last_func = ir_func;
    }
}

CnIrModule *cn_ir_gen_program(CnAstProgram *program, CnTargetTriple target, CnCompileMode mode) {
    if (!program) return NULL;

    CnIrGenContext *ctx = cn_ir_gen_context_new();
    if (ctx->module) {
        ctx->module->target = target;
        ctx->module->compile_mode = mode;
    }

    for (size_t i = 0; i < program->function_count; i++) {
        cn_ir_gen_function(ctx, program->functions[i]);
    }

    CnIrModule *module = ctx->module;
    ctx->module = NULL;
    cn_ir_gen_context_free(ctx);
    return module;
}
