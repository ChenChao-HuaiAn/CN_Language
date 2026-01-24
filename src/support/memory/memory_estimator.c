#include "cnlang/support/memory_estimator.h"
#include <string.h>

/* 前向声明内部函数 */
static size_t estimate_expr(const CnAstExpr *expr);
static size_t estimate_stmt(const CnAstStmt *stmt);
static size_t estimate_block(const CnAstBlockStmt *block);
static size_t estimate_type(const CnType *type);
static size_t estimate_scope_recursive(const CnSemScope *scope);

/* 估算类型结构的内存占用 */
static size_t estimate_type(const CnType *type)
{
    size_t size = 0;
    
    if (!type) {
        return 0;
    }
    
    size += sizeof(CnType);
    
    switch (type->kind) {
        case CN_TYPE_POINTER:
            size += estimate_type(type->as.pointer_to);
            break;
        case CN_TYPE_ARRAY:
            size += estimate_type(type->as.array.element_type);
            break;
        case CN_TYPE_FUNCTION:
            size += estimate_type(type->as.function.return_type);
            size += sizeof(CnType*) * type->as.function.param_count;
            for (size_t i = 0; i < type->as.function.param_count; i++) {
                size += estimate_type(type->as.function.param_types[i]);
            }
            break;
        case CN_TYPE_STRUCT:
            if (type->as.struct_name) {
                size += strlen(type->as.struct_name) + 1;
            }
            break;
        default:
            /* 基础类型不额外占用内存 */
            break;
    }
    
    return size;
}

/* 估算表达式的内存占用 */
static size_t estimate_expr(const CnAstExpr *expr)
{
    size_t size = 0;
    
    if (!expr) {
        return 0;
    }
    
    size += sizeof(CnAstExpr);
    size += estimate_type(expr->type);
    
    switch (expr->kind) {
        case CN_AST_EXPR_BINARY:
            size += estimate_expr(expr->as.binary.left);
            size += estimate_expr(expr->as.binary.right);
            break;
            
        case CN_AST_EXPR_CALL:
            size += estimate_expr(expr->as.call.callee);
            size += sizeof(CnAstExpr*) * expr->as.call.argument_count;
            for (size_t i = 0; i < expr->as.call.argument_count; i++) {
                size += estimate_expr(expr->as.call.arguments[i]);
            }
            break;
            
        case CN_AST_EXPR_IDENTIFIER:
            /* 名称字符串通常指向 token,不重复计数 */
            break;
            
        case CN_AST_EXPR_INTEGER_LITERAL:
            /* 无额外分配 */
            break;
            
        case CN_AST_EXPR_STRING_LITERAL:
            /* 字符串字面量通常指向 token,不重复计数 */
            break;
            
        case CN_AST_EXPR_ASSIGN:
            size += estimate_expr(expr->as.assign.target);
            size += estimate_expr(expr->as.assign.value);
            break;
            
        case CN_AST_EXPR_LOGICAL:
            size += estimate_expr(expr->as.logical.left);
            size += estimate_expr(expr->as.logical.right);
            break;
            
        case CN_AST_EXPR_UNARY:
            size += estimate_expr(expr->as.unary.operand);
            break;
            
        case CN_AST_EXPR_ARRAY_LITERAL:
            size += sizeof(CnAstExpr*) * expr->as.array_literal.element_count;
            for (size_t i = 0; i < expr->as.array_literal.element_count; i++) {
                size += estimate_expr(expr->as.array_literal.elements[i]);
            }
            break;
            
        case CN_AST_EXPR_INDEX:
            size += estimate_expr(expr->as.index.array);
            size += estimate_expr(expr->as.index.index);
            break;
    }
    
    return size;
}

/* 估算语句块的内存占用 */
static size_t estimate_block(const CnAstBlockStmt *block)
{
    size_t size = 0;
    
    if (!block) {
        return 0;
    }
    
    size += sizeof(CnAstBlockStmt);
    size += sizeof(CnAstStmt*) * block->stmt_count;
    
    for (size_t i = 0; i < block->stmt_count; i++) {
        size += estimate_stmt(block->stmts[i]);
    }
    
    return size;
}

/* 估算语句的内存占用 */
static size_t estimate_stmt(const CnAstStmt *stmt)
{
    size_t size = 0;
    
    if (!stmt) {
        return 0;
    }
    
    size += sizeof(CnAstStmt);
    
    switch (stmt->kind) {
        case CN_AST_STMT_BLOCK:
            size += estimate_block(stmt->as.block);
            break;
            
        case CN_AST_STMT_VAR_DECL:
            size += estimate_type(stmt->as.var_decl.declared_type);
            size += estimate_expr(stmt->as.var_decl.initializer);
            break;
            
        case CN_AST_STMT_EXPR:
            size += estimate_expr(stmt->as.expr.expr);
            break;
            
        case CN_AST_STMT_RETURN:
            size += estimate_expr(stmt->as.return_stmt.expr);
            break;
            
        case CN_AST_STMT_IF:
            size += estimate_expr(stmt->as.if_stmt.condition);
            size += estimate_block(stmt->as.if_stmt.then_block);
            size += estimate_block(stmt->as.if_stmt.else_block);
            break;
            
        case CN_AST_STMT_WHILE:
            size += estimate_expr(stmt->as.while_stmt.condition);
            size += estimate_block(stmt->as.while_stmt.body);
            break;
            
        case CN_AST_STMT_FOR:
            size += estimate_stmt(stmt->as.for_stmt.init);
            size += estimate_expr(stmt->as.for_stmt.condition);
            size += estimate_expr(stmt->as.for_stmt.update);
            size += estimate_block(stmt->as.for_stmt.body);
            break;
            
        case CN_AST_STMT_BREAK:
        case CN_AST_STMT_CONTINUE:
            /* 无额外分配 */
            break;
    }
    
    return size;
}

/* 估算 AST 的内存占用 */
size_t cn_mem_estimate_ast(const CnAstProgram *program)
{
    size_t size = 0;
    
    if (!program) {
        return 0;
    }
    
    size += sizeof(CnAstProgram);
    size += sizeof(CnAstFunctionDecl*) * program->function_count;
    
    for (size_t i = 0; i < program->function_count; i++) {
        const CnAstFunctionDecl *func = program->functions[i];
        if (!func) {
            continue;
        }
        
        size += sizeof(CnAstFunctionDecl);
        size += sizeof(CnAstParameter) * func->parameter_count;
        
        /* 估算参数类型 */
        for (size_t j = 0; j < func->parameter_count; j++) {
            size += estimate_type(func->parameters[j].declared_type);
        }
        
        /* 估算函数体 */
        size += estimate_block(func->body);
    }
    
    return size;
}

/* 估算符号表的内存占用 */
size_t cn_mem_estimate_symbol_table(const CnSemScope *global_scope)
{
    if (!global_scope) {
        return 0;
    }
    
    /* 注意：CnSemScope 是不透明类型，我们无法直接访问其内部结构。
     * 这里返回一个保守的估算值：假设平均每个作用域约 1KB
     * （包括作用域结构体本身、符号节点等）*/
    
    /* 粗略估算：假设有全局作用域 + 函数作用域 + 块作用域
     * 每个作用域平均 1KB */
    return 1024 * 10;  /* 假设总共约 10 个作用域 */
}

/* 递归估算作用域及其符号的内存占用 */
static size_t estimate_scope_recursive(const CnSemScope *scope)
{
    /* 这个函数现在不再使用，但保留以备将来需要 */
    (void)scope;
    return 0;
}

/* 估算 IR 指令的内存占用 */
static size_t estimate_ir_inst(const CnIrInst *inst)
{
    size_t size = 0;
    
    if (!inst) {
        return 0;
    }
    
    size += sizeof(CnIrInst);
    
    /* 估算额外参数数组 */
    if (inst->extra_args_count > 0) {
        size += sizeof(CnIrOperand) * inst->extra_args_count;
    }
    
    /* 估算操作数关联的类型 */
    size += estimate_type(inst->dest.type);
    size += estimate_type(inst->src1.type);
    size += estimate_type(inst->src2.type);
    
    for (size_t i = 0; i < inst->extra_args_count; i++) {
        size += estimate_type(inst->extra_args[i].type);
    }
    
    return size;
}

/* 估算基本块的内存占用 */
static size_t estimate_ir_block(const CnIrBasicBlock *block)
{
    size_t size = 0;
    const CnIrInst *inst;
    const CnIrBasicBlockList *list_node;
    
    if (!block) {
        return 0;
    }
    
    size += sizeof(CnIrBasicBlock);
    
    /* 估算块名称 */
    if (block->name) {
        size += strlen(block->name) + 1;
    }
    
    /* 估算指令链表 */
    for (inst = block->first_inst; inst != NULL; inst = inst->next) {
        size += estimate_ir_inst(inst);
    }
    
    /* 估算前驱/后继链表节点 */
    for (list_node = block->preds; list_node != NULL; list_node = list_node->next) {
        size += sizeof(CnIrBasicBlockList);
    }
    for (list_node = block->succs; list_node != NULL; list_node = list_node->next) {
        size += sizeof(CnIrBasicBlockList);
    }
    
    return size;
}

/* 估算 IR 函数的内存占用 */
static size_t estimate_ir_function(const CnIrFunction *func)
{
    size_t size = 0;
    const CnIrBasicBlock *block;
    
    if (!func) {
        return 0;
    }
    
    size += sizeof(CnIrFunction);
    
    /* 估算函数名 */
    if (func->name) {
        size += strlen(func->name) + 1;
    }
    
    /* 估算返回类型 */
    size += estimate_type(func->return_type);
    
    /* 估算参数和局部变量数组 */
    size += sizeof(CnIrOperand) * func->param_count;
    size += sizeof(CnIrOperand) * func->local_count;
    
    for (size_t i = 0; i < func->param_count; i++) {
        size += estimate_type(func->params[i].type);
    }
    for (size_t i = 0; i < func->local_count; i++) {
        size += estimate_type(func->locals[i].type);
    }
    
    /* 估算基本块链表 */
    for (block = func->first_block; block != NULL; block = block->next) {
        size += estimate_ir_block(block);
    }
    
    return size;
}

/* 估算 IR 模块的内存占用 */
size_t cn_mem_estimate_ir(const CnIrModule *module)
{
    size_t size = 0;
    const CnIrFunction *func;
    
    if (!module) {
        return 0;
    }
    
    size += sizeof(CnIrModule);
    
    /* 遍历函数链表 */
    for (func = module->first_func; func != NULL; func = func->next) {
        size += estimate_ir_function(func);
    }
    
    return size;
}

/* 估算诊断信息的内存占用 */
size_t cn_mem_estimate_diagnostics(const CnDiagnostics *diagnostics)
{
    size_t size = 0;
    
    if (!diagnostics) {
        return 0;
    }
    
    size += sizeof(CnDiagnostics);
    
    /* 估算诊断条目数组 */
    size += sizeof(CnDiagnostic) * diagnostics->capacity;
    
    /* 注意：诊断消息通常是静态字符串，不计入动态分配 */
    
    return size;
}
