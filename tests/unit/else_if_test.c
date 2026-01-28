// 测试 else if (否则 如果) 语法解析
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/support/diagnostics.h"

// 辅助函数：打印语句类型
static const char* stmt_kind_str(CnAstStmtKind kind) {
    switch (kind) {
        case CN_AST_STMT_VAR_DECL: return "VAR_DECL";
        case CN_AST_STMT_EXPR: return "EXPR";
        case CN_AST_STMT_BLOCK: return "BLOCK";
        case CN_AST_STMT_RETURN: return "RETURN";
        case CN_AST_STMT_IF: return "IF";
        case CN_AST_STMT_WHILE: return "WHILE";
        case CN_AST_STMT_FOR: return "FOR";
        case CN_AST_STMT_BREAK: return "BREAK";
        case CN_AST_STMT_CONTINUE: return "CONTINUE";
        case CN_AST_STMT_SWITCH: return "SWITCH";
        case CN_AST_STMT_STRUCT_DECL: return "STRUCT_DECL";
        case CN_AST_STMT_ENUM_DECL: return "ENUM_DECL";
        case CN_AST_STMT_MODULE_DECL: return "MODULE_DECL";
        case CN_AST_STMT_IMPORT: return "IMPORT";
        default: return "UNKNOWN";
    }
}

// 测试简单的 else if
static void test_simple_else_if(void) {
    const char *source = 
        "函数 主程序() {\n"
        "    整数 x = 5;\n"
        "    如果 (x > 10) {\n"
        "        返回 1;\n"
        "    } 否则 如果 (x > 5) {\n"
        "        返回 2;\n"
        "    } 否则 {\n"
        "        返回 3;\n"
        "    }\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 简单的 else if 语法\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    assert(parser != NULL);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    
    assert(ok && program != NULL);
    assert(program->function_count == 1);
    
    CnAstFunctionDecl *func = program->functions[0];
    assert(func->body != NULL);
    assert(func->body->stmt_count == 2);  // 变量声明 + if语句
    
    // 检查 if 语句
    CnAstStmt *if_stmt = func->body->stmts[1];
    assert(if_stmt->kind == CN_AST_STMT_IF);
    printf("  第一层 if 语句类型: %s\n", stmt_kind_str(if_stmt->kind));
    
    // 检查 else 块
    assert(if_stmt->as.if_stmt.else_block != NULL);
    printf("  else 块存在\n");
    
    // else 块应该只包含一个 if 语句 (else if)
    assert(if_stmt->as.if_stmt.else_block->stmt_count == 1);
    CnAstStmt *nested_if = if_stmt->as.if_stmt.else_block->stmts[0];
    assert(nested_if->kind == CN_AST_STMT_IF);
    printf("  else 块中包含嵌套 if 语句 (else if)\n");
    
    // 检查嵌套 if 的 else 块
    assert(nested_if->as.if_stmt.else_block != NULL);
    printf("  嵌套 if 语句有 else 块\n");
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 简单 else if 测试通过\n\n");
}

// 测试多层 else if 链
static void test_multiple_else_if(void) {
    const char *source = 
        "函数 主程序() {\n"
        "    整数 分数 = 85;\n"
        "    如果 (分数 >= 90) {\n"
        "        返回 4;\n"
        "    } 否则 如果 (分数 >= 80) {\n"
        "        返回 3;\n"
        "    } 否则 如果 (分数 >= 70) {\n"
        "        返回 2;\n"
        "    } 否则 如果 (分数 >= 60) {\n"
        "        返回 1;\n"
        "    } 否则 {\n"
        "        返回 0;\n"
        "    }\n"
        "}\n";
    
    printf("测试: 多层 else if 链\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    assert(parser != NULL);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    
    assert(ok && program != NULL);
    assert(program->function_count == 1);
    
    CnAstFunctionDecl *func = program->functions[0];
    assert(func->body->stmt_count == 2);
    
    // 检查 if 链的深度
    CnAstStmt *current_if = func->body->stmts[1];
    int else_if_count = 0;
    
    while (current_if != NULL && current_if->kind == CN_AST_STMT_IF) {
        else_if_count++;
        printf("  检测到第 %d 层 if/else-if\n", else_if_count);
        
        if (current_if->as.if_stmt.else_block != NULL && 
            current_if->as.if_stmt.else_block->stmt_count == 1) {
            current_if = current_if->as.if_stmt.else_block->stmts[0];
        } else {
            break;
        }
    }
    
    assert(else_if_count == 4);  // 4 个 if/else-if 语句
    printf("  ✓ 检测到 %d 层 if/else-if 链\n", else_if_count);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 多层 else if 测试通过\n\n");
}

// 测试嵌套的 else if
static void test_nested_else_if(void) {
    const char *source = 
        "函数 主程序() {\n"
        "    整数 x = 5;\n"
        "    如果 (x > 10) {\n"
        "        返回 1;\n"
        "    } 否则 如果 (x == 5) {\n"
        "        如果 (x < 10) {\n"
        "            返回 2;\n"
        "        } 否则 {\n"
        "            返回 3;\n"
        "        }\n"
        "    } 否则 {\n"
        "        返回 4;\n"
        "    }\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 嵌套的 else if\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    assert(parser != NULL);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    
    assert(ok && program != NULL);
    assert(program->function_count == 1);
    
    CnAstFunctionDecl *func = program->functions[0];
    CnAstStmt *if_stmt = func->body->stmts[1];
    
    // 第一层 if
    assert(if_stmt->kind == CN_AST_STMT_IF);
    printf("  第一层 if 语句\n");
    
    // 第一层 else if
    assert(if_stmt->as.if_stmt.else_block != NULL);
    CnAstStmt *else_if_stmt = if_stmt->as.if_stmt.else_block->stmts[0];
    assert(else_if_stmt->kind == CN_AST_STMT_IF);
    printf("  第一层 else if 语句\n");
    
    // else if 的 then 块中有嵌套的 if
    assert(else_if_stmt->as.if_stmt.then_block->stmt_count > 0);
    CnAstStmt *nested_if = else_if_stmt->as.if_stmt.then_block->stmts[0];
    assert(nested_if->kind == CN_AST_STMT_IF);
    printf("  else if 块中有嵌套的 if 语句\n");
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 嵌套 else if 测试通过\n\n");
}

int main(void) {
    printf("=== CN Language else if 语法测试 ===\n\n");
    
    test_simple_else_if();
    test_multiple_else_if();
    test_nested_else_if();
    
    printf("=== 所有测试通过! ===\n");
    return 0;
}
