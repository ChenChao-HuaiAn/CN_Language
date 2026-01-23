#include "cnlang/analysis/static_check.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 静态检查专用诊断代码（扩展诊断码）
#define CN_DIAG_CODE_CHECK_UNUSED_VAR 2001
#define CN_DIAG_CODE_CHECK_UNUSED_PARAM 2002
#define CN_DIAG_CODE_CHECK_COMPLEXITY 2003

// 初始化静态检查配置为默认值
void cn_check_config_init_default(CnCheckConfig *config)
{
    if (!config) {
        return;
    }

    // 启用所有规则
    for (int i = 0; i < CN_CHECK_RULE_COUNT; i++) {
        config->enabled_rules[i] = true;
    }

    // 设置默认复杂度阈值
    config->max_statements_per_function = 50;
    config->max_nesting_level = 5;
    config->check_only = false;
}

// 辅助函数：计算语句数量
static size_t count_statements_in_block(const CnAstBlockStmt *block)
{
    if (!block) {
        return 0;
    }
    return block->stmt_count;
}

// 辅助函数：计算最大嵌套层级
static int calculate_max_nesting_level(const CnAstStmt *stmt, int current_level)
{
    int max_level = current_level;

    if (!stmt) {
        return max_level;
    }

    switch (stmt->kind) {
    case CN_AST_STMT_BLOCK: {
        const CnAstBlockStmt *block = (const CnAstBlockStmt *)stmt;
        for (size_t i = 0; i < block->stmt_count; i++) {
            int level = calculate_max_nesting_level(block->stmts[i], current_level + 1);
            if (level > max_level) {
                max_level = level;
            }
        }
        break;
    }
    case CN_AST_STMT_IF: {
        const CnAstIfStmt *if_stmt = &stmt->as.if_stmt;
        int then_level = calculate_max_nesting_level((const CnAstStmt *)(if_stmt->then_block), current_level + 1);
        if (then_level > max_level) {
            max_level = then_level;
        }
        if (if_stmt->else_block) {
            int else_level = calculate_max_nesting_level((const CnAstStmt *)(if_stmt->else_block), current_level + 1);
            if (else_level > max_level) {
                max_level = else_level;
            }
        }
        break;
    }
    case CN_AST_STMT_WHILE: {
        const CnAstWhileStmt *while_stmt = &stmt->as.while_stmt;
        int body_level = calculate_max_nesting_level((const CnAstStmt *)(while_stmt->body), current_level + 1);
        if (body_level > max_level) {
            max_level = body_level;
        }
        break;
    }
    case CN_AST_STMT_FOR: {
        const CnAstForStmt *for_stmt = &stmt->as.for_stmt;
        int body_level = calculate_max_nesting_level((const CnAstStmt *)(for_stmt->body), current_level + 1);
        if (body_level > max_level) {
            max_level = body_level;
        }
        break;
    }
    default:
        break;
    }

    return max_level;
}

// 检查函数复杂度
bool cn_check_complexity(
    const CnAstProgram *program,
    const CnCheckConfig *config,
    CnDiagnostics *diagnostics,
    const char *filename)
{
    bool success = true;

    if (!program || !config || !diagnostics) {
        return false;
    }

    if (!config->enabled_rules[CN_CHECK_RULE_COMPLEXITY]) {
        return true;
    }

    // 遍历所有函数
    for (size_t i = 0; i < program->function_count; i++) {
        const CnAstFunctionDecl *func = program->functions[i];
        if (!func || !func->body) {
            continue;
        }

        // 检查语句数量
        size_t stmt_count = count_statements_in_block(func->body);
        if (stmt_count > (size_t)config->max_statements_per_function) {
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "函数复杂度过高：包含 %zu 条语句，超过限制 %d",
                     stmt_count,
                     config->max_statements_per_function);
            cn_support_diagnostics_report_warning(
                diagnostics,
                CN_DIAG_CODE_CHECK_COMPLEXITY,
                filename,
                0,  // CnAstFunctionDecl 没有 line 字段
                0,  // CnAstFunctionDecl 没有 column 字段
                msg);
            success = false;
        }

        // 检查嵌套层级
        int max_nesting = calculate_max_nesting_level((const CnAstStmt *)func->body, 0);
        if (max_nesting > config->max_nesting_level) {
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "函数嵌套层级过深：最大 %d 层，超过限制 %d",
                     max_nesting,
                     config->max_nesting_level);
            cn_support_diagnostics_report_warning(
                diagnostics,
                CN_DIAG_CODE_CHECK_COMPLEXITY,
                filename,
                0,  // CnAstFunctionDecl 没有 line 字段
                0,  // CnAstFunctionDecl 没有 column 字段
                msg);
            success = false;
        }
    }

    return success;
}

// 辅助函数：检查表达式中是否使用了某个变量
static bool expr_uses_var(const CnAstExpr *expr, const char *var_name, size_t var_name_len)
{
    if (!expr || !var_name) {
        return false;
    }

    switch (expr->kind) {
    case CN_AST_EXPR_IDENTIFIER: {
        const CnAstIdentifierExpr *id_expr = (const CnAstIdentifierExpr *)expr;
        if (id_expr->name_length == var_name_len &&
            strncmp(id_expr->name, var_name, var_name_len) == 0) {
            return true;
        }
        break;
    }
    case CN_AST_EXPR_BINARY: {
        const CnAstBinaryExpr *bin_expr = (const CnAstBinaryExpr *)expr;
        return expr_uses_var(bin_expr->left, var_name, var_name_len) ||
               expr_uses_var(bin_expr->right, var_name, var_name_len);
    }
    case CN_AST_EXPR_UNARY: {
        const CnAstUnaryExpr *unary_expr = (const CnAstUnaryExpr *)expr;
        return expr_uses_var(unary_expr->operand, var_name, var_name_len);
    }
    case CN_AST_EXPR_CALL: {
        const CnAstCallExpr *call_expr = &expr->as.call;
        if (expr_uses_var(call_expr->callee, var_name, var_name_len)) {
            return true;
        }
        for (size_t i = 0; i < call_expr->argument_count; i++) {
            if (expr_uses_var(call_expr->arguments[i], var_name, var_name_len)) {
                return true;
            }
        }
        break;
    }
    case CN_AST_EXPR_INDEX: {
        const CnAstIndexExpr *index_expr = &expr->as.index;
        return expr_uses_var(index_expr->array, var_name, var_name_len) ||
               expr_uses_var(index_expr->index, var_name, var_name_len);
    }
    case CN_AST_EXPR_ARRAY_LITERAL: {
        const CnAstArrayLiteralExpr *arr_expr = &expr->as.array_literal;
        for (size_t i = 0; i < arr_expr->element_count; i++) {
            if (expr_uses_var(arr_expr->elements[i], var_name, var_name_len)) {
                return true;
            }
        }
        break;
    }
    default:
        break;
    }

    return false;
}

// 辅助函数：检查语句中是否使用了某个变量
static bool stmt_uses_var(const CnAstStmt *stmt, const char *var_name, size_t var_name_len);

static bool block_uses_var(const CnAstBlockStmt *block, const char *var_name, size_t var_name_len)
{
    if (!block) {
        return false;
    }

    for (size_t i = 0; i < block->stmt_count; i++) {
        if (stmt_uses_var(block->stmts[i], var_name, var_name_len)) {
            return true;
        }
    }

    return false;
}

static bool stmt_uses_var(const CnAstStmt *stmt, const char *var_name, size_t var_name_len)
{
    if (!stmt) {
        return false;
    }

    switch (stmt->kind) {
    case CN_AST_STMT_VAR_DECL: {
        const CnAstVarDecl *var_decl = &stmt->as.var_decl;
        if (var_decl->initializer) {
            return expr_uses_var(var_decl->initializer, var_name, var_name_len);
        }
        break;
    }
    case CN_AST_STMT_EXPR: {
        const CnAstExprStmt *expr_stmt = &stmt->as.expr;
        return expr_uses_var(expr_stmt->expr, var_name, var_name_len);
    }
    case CN_AST_STMT_RETURN: {
        const CnAstReturnStmt *ret_stmt = &stmt->as.return_stmt;
        if (ret_stmt->expr) {
            return expr_uses_var(ret_stmt->expr, var_name, var_name_len);
        }
        break;
    }
    case CN_AST_STMT_IF: {
        const CnAstIfStmt *if_stmt = &stmt->as.if_stmt;
        if (expr_uses_var(if_stmt->condition, var_name, var_name_len)) {
            return true;
        }
        if (block_uses_var(if_stmt->then_block, var_name, var_name_len)) {
            return true;
        }
        if (if_stmt->else_block && block_uses_var(if_stmt->else_block, var_name, var_name_len)) {
            return true;
        }
        break;
    }
    case CN_AST_STMT_WHILE: {
        const CnAstWhileStmt *while_stmt = &stmt->as.while_stmt;
        if (expr_uses_var(while_stmt->condition, var_name, var_name_len)) {
            return true;
        }
        return block_uses_var(while_stmt->body, var_name, var_name_len);
    }
    case CN_AST_STMT_FOR: {
        const CnAstForStmt *for_stmt = &stmt->as.for_stmt;
        if (for_stmt->init && stmt_uses_var(for_stmt->init, var_name, var_name_len)) {
            return true;
        }
        if (for_stmt->condition && expr_uses_var(for_stmt->condition, var_name, var_name_len)) {
            return true;
        }
        if (for_stmt->update && expr_uses_var(for_stmt->update, var_name, var_name_len)) {
            return true;
        }
        return block_uses_var(for_stmt->body, var_name, var_name_len);
    }
    case CN_AST_STMT_BLOCK: {
        const CnAstBlockStmt *block_stmt = stmt->as.block;
        return block_uses_var(block_stmt, var_name, var_name_len);
    }
    default:
        break;
    }

    return false;
}

// 检查未使用的变量和参数
bool cn_check_unused_symbols(
    const CnAstProgram *program,
    const CnCheckConfig *config,
    CnDiagnostics *diagnostics,
    const char *filename)
{
    bool success = true;

    if (!program || !config || !diagnostics) {
        return false;
    }

    bool check_vars = config->enabled_rules[CN_CHECK_RULE_UNUSED_VAR];
    bool check_params = config->enabled_rules[CN_CHECK_RULE_UNUSED_PARAM];

    if (!check_vars && !check_params) {
        return true;
    }

    // 遍历所有函数
    for (size_t i = 0; i < program->function_count; i++) {
        const CnAstFunctionDecl *func = program->functions[i];
        if (!func || !func->body) {
            continue;
        }

        // 检查参数使用情况
        if (check_params) {
            for (size_t j = 0; j < func->parameter_count; j++) {
                const CnAstParameter *param = &func->parameters[j];
                bool used = block_uses_var(func->body, param->name, param->name_length);
                
                if (!used) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "未使用的函数参数: %.*s",
                             (int)param->name_length, param->name);
                    cn_support_diagnostics_report_warning(
                        diagnostics,
                        CN_DIAG_CODE_CHECK_UNUSED_PARAM,
                        filename,
                        0,  // 暂时使用 0，因为 CnAstParameter 没有 line 字段
                        0,  // 暂时使用 0，因为 CnAstParameter 没有 column 字段
                        msg);
                    success = false;
                }
            }
        }

        // 检查局部变量使用情况（仅检查函数体顶层块中的变量）
        if (check_vars && func->body) {
            for (size_t j = 0; j < func->body->stmt_count; j++) {
                const CnAstStmt *stmt = func->body->stmts[j];
                if (stmt->kind == CN_AST_STMT_VAR_DECL) {
                    const CnAstVarDecl *var_decl = &stmt->as.var_decl;
                    
                    // 检查这个变量在后续语句中是否被使用
                    bool used = false;
                    for (size_t k = j + 1; k < func->body->stmt_count; k++) {
                        if (stmt_uses_var(func->body->stmts[k], var_decl->name, var_decl->name_length)) {
                            used = true;
                            break;
                        }
                    }
                    
                    if (!used) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "未使用的局部变量: %.*s",
                                 (int)var_decl->name_length, var_decl->name);
                        cn_support_diagnostics_report_warning(
                            diagnostics,
                            CN_DIAG_CODE_CHECK_UNUSED_VAR,
                            filename,
                            0,  // 暂时使用 0，因为  CnAstVarDecl 可能没有 line 字段
                            0,  // 暂时使用 0
                            msg);
                        success = false;
                    }
                }
            }
        }
    }

    return success;
}

// 对单个 AST 程序执行静态检查
bool cn_check_program(
    const CnAstProgram *program,
    const CnCheckConfig *config,
    CnDiagnostics *diagnostics,
    const char *filename)
{
    bool success = true;

    if (!program || !config || !diagnostics) {
        return false;
    }

    // 执行各项检查规则
    if (!cn_check_unused_symbols(program, config, diagnostics, filename)) {
        success = false;
    }

    if (!cn_check_complexity(program, config, diagnostics, filename)) {
        success = false;
    }

    return success;
}

// 对单个文件执行静态检查
bool cn_check_file(
    const char *filename,
    const CnCheckConfig *config,
    CnDiagnostics *diagnostics)
{
    if (!filename || !config || !diagnostics) {
        return false;
    }

    // 读取文件内容
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *source = (char *)malloc(size + 1);
    if (!source) {
        fclose(fp);
        return false;
    }

    size_t length = fread(source, 1, size, fp);
    fclose(fp);
    source[length] = '\0';

    // 执行前端解析
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, length, filename);
    cn_frontend_lexer_set_diagnostics(&lexer, diagnostics);

    CnParser *parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        free(source);
        return false;
    }
    cn_frontend_parser_set_diagnostics(parser, diagnostics);

    CnAstProgram *program = NULL;
    bool parse_ok = cn_frontend_parse_program(parser, &program);
    
    bool success = false;
    if (parse_ok && program) {
        // 对 AST 执行静态检查
        success = cn_check_program(program, config, diagnostics, filename);
    }

    // 清理资源
    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    free(source);

    return success;
}
