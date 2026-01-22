#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    /* 测试函数参数和函数调用 */
    const char *source = "函数 add(a, b) { 返回 a + b; } 函数 main() { 变量 result = add(1, 2); 返回 result; }";
    size_t length = strlen(source);
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    bool ok;

    cn_frontend_lexer_init(&lexer, source, length, "<memory>");

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_func_test: 创建解析器失败\n");
        return 1;
    }

    ok = cn_frontend_parse_program(parser, &program);
    if (!ok) {
        fprintf(stderr, "parser_func_test: 解析失败 (ok=false)\n");
        cn_frontend_parser_free(parser);
        return 1;
    }
    
    if (!program) {
        fprintf(stderr, "parser_func_test: 解析失败 (program=NULL)\n");
        cn_frontend_parser_free(parser);
        return 1;
    }

    if (program->function_count != 2) {
        fprintf(stderr, "parser_func_test: 期望 2 个函数，实际为 %zu\n",
                program->function_count);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        return 1;
    }

    // 验证第一个函数：add(a, b)
    {
        CnAstFunctionDecl *fn = program->functions[0];
        
        if (fn->parameter_count != 2) {
            fprintf(stderr, "parser_func_test: add 函数应有 2 个参数，实际为 %zu\n",
                    fn->parameter_count);
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证参数名
        if (fn->parameters[0].name_length != 1 ||
            strncmp(fn->parameters[0].name, "a", 1) != 0) {
            fprintf(stderr, "parser_func_test: 第一个参数应为 'a'\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (fn->parameters[1].name_length != 1 ||
            strncmp(fn->parameters[1].name, "b", 1) != 0) {
            fprintf(stderr, "parser_func_test: 第二个参数应为 'b'\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证函数体：返回 a + b
        if (!fn->body || fn->body->stmt_count != 1) {
            fprintf(stderr, "parser_func_test: add 函数体应有 1 条语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        CnAstStmt *return_stmt = fn->body->stmts[0];
        if (return_stmt->kind != CN_AST_STMT_RETURN) {
            fprintf(stderr, "parser_func_test: add 函数体第一条应为 return 语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        CnAstExpr *return_value = return_stmt->as.return_stmt.expr;
        if (!return_value || return_value->kind != CN_AST_EXPR_BINARY) {
            fprintf(stderr, "parser_func_test: add 返回值应为二元表达式\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }
    }

    // 验证第二个函数：main()
    {
        CnAstFunctionDecl *fn = program->functions[1];
        
        if (fn->parameter_count != 0) {
            fprintf(stderr, "parser_func_test: main 函数应无参数，实际为 %zu\n",
                    fn->parameter_count);
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证函数体：变量 result = add(1, 2); 返回 result;
        if (!fn->body || fn->body->stmt_count != 2) {
            fprintf(stderr, "parser_func_test: main 函数体应有 2 条语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 第一条语句：变量 result = add(1, 2);
        CnAstStmt *var_decl = fn->body->stmts[0];
        if (var_decl->kind != CN_AST_STMT_VAR_DECL) {
            fprintf(stderr, "parser_func_test: main 第一条应为变量声明\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        CnAstExpr *initializer = var_decl->as.var_decl.initializer;
        if (!initializer || initializer->kind != CN_AST_EXPR_CALL) {
            fprintf(stderr, "parser_func_test: result 的初始化值应为函数调用\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证函数调用：add(1, 2)
        CnAstCallExpr *call = &initializer->as.call;
        
        if (!call->callee || call->callee->kind != CN_AST_EXPR_IDENTIFIER) {
            fprintf(stderr, "parser_func_test: 被调用函数应为标识符\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (call->argument_count != 2) {
            fprintf(stderr, "parser_func_test: 函数调用应有 2 个参数，实际为 %zu\n",
                    call->argument_count);
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证第一个参数：1
        if (!call->arguments[0] || 
            call->arguments[0]->kind != CN_AST_EXPR_INTEGER_LITERAL ||
            call->arguments[0]->as.integer_literal.value != 1) {
            fprintf(stderr, "parser_func_test: 第一个参数应为整数 1\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证第二个参数：2
        if (!call->arguments[1] || 
            call->arguments[1]->kind != CN_AST_EXPR_INTEGER_LITERAL ||
            call->arguments[1]->as.integer_literal.value != 2) {
            fprintf(stderr, "parser_func_test: 第二个参数应为整数 2\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 第二条语句：返回 result;
        CnAstStmt *return_stmt = fn->body->stmts[1];
        if (return_stmt->kind != CN_AST_STMT_RETURN) {
            fprintf(stderr, "parser_func_test: main 第二条应为 return 语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }
    }

    printf("parser_func_test: OK\n");

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);

    return 0;
}
