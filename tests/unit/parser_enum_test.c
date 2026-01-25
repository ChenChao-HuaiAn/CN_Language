#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// 测试简单枚举解析
void test_simple_enum_parsing() {
    const char *source = 
        "枚举 颜色 {\n"
        "    红色,\n"
        "    绿色,\n"
        "    蓝色\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    assert(parser != NULL);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success);
    assert(program != NULL);
    assert(program->enum_count == 1);
    
    // 检查枚举声明
    CnAstStmt *enum_stmt = program->enums[0];
    assert(enum_stmt != NULL);
    assert(enum_stmt->kind == CN_AST_STMT_ENUM_DECL);
    
    CnAstEnumDecl *enum_decl = &enum_stmt->as.enum_decl;
    assert(memcmp(enum_decl->name, "颜色", strlen("颜色")) == 0);
    assert(enum_decl->member_count == 3);
    
    // 检查枚举成员
    assert(memcmp(enum_decl->members[0].name, "红色", strlen("红色")) == 0);
    assert(enum_decl->members[0].has_value == 0);
    assert(enum_decl->members[0].value == 0);
    
    assert(memcmp(enum_decl->members[1].name, "绿色", strlen("绿色")) == 0);
    assert(enum_decl->members[1].has_value == 0);
    assert(enum_decl->members[1].value == 1);
    
    assert(memcmp(enum_decl->members[2].name, "蓝色", strlen("蓝色")) == 0);
    assert(enum_decl->members[2].has_value == 0);
    assert(enum_decl->members[2].value == 2);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    
    printf("✓ 测试简单枚举解析通过\n");
}

// 测试带显式值的枚举解析
void test_enum_with_explicit_values() {
    const char *source = 
        "枚举 状态 {\n"
        "    未开始 = 0,\n"
        "    进行中 = 1,\n"
        "    已完成 = 2\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    assert(parser != NULL);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success);
    assert(program != NULL);
    assert(program->enum_count == 1);
    
    CnAstEnumDecl *enum_decl = &program->enums[0]->as.enum_decl;
    assert(enum_decl->member_count == 3);
    
    // 检查显式值
    assert(enum_decl->members[0].has_value == 1);
    assert(enum_decl->members[0].value == 0);
    
    assert(enum_decl->members[1].has_value == 1);
    assert(enum_decl->members[1].value == 1);
    
    assert(enum_decl->members[2].has_value == 1);
    assert(enum_decl->members[2].value == 2);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    
    printf("✓ 测试带显式值的枚举解析通过\n");
}

// 测试非连续值的枚举解析
void test_enum_with_non_sequential_values() {
    const char *source = 
        "枚举 优先级 {\n"
        "    低 = 1,\n"
        "    中 = 5,\n"
        "    高 = 10\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    assert(parser != NULL);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success);
    assert(program != NULL);
    
    CnAstEnumDecl *enum_decl = &program->enums[0]->as.enum_decl;
    
    assert(enum_decl->members[0].value == 1);
    assert(enum_decl->members[1].value == 5);
    assert(enum_decl->members[2].value == 10);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    
    printf("✓ 测试非连续值的枚举解析通过\n");
}

// 测试枚举的语义分析
void test_enum_semantic_analysis() {
    const char *source = 
        "枚举 颜色 {\n"
        "    红色,\n"
        "    绿色\n"
        "}\n"
        "函数 主程序() {\n"
        "    变量 c = 红色;\n"
        "    返回 0;\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success);
    assert(program != NULL);
    
    // 创建诊断系统
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    // 构建作用域
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    assert(global_scope != NULL);
    
    // 检查枚举符号是否在全局作用域中
    CnSemSymbol *color_sym = cn_sem_scope_lookup(global_scope, "颜色", strlen("颜色"));
    assert(color_sym != NULL);
    assert(color_sym->kind == CN_SEM_SYMBOL_ENUM);
    assert(color_sym->type != NULL);
    assert(color_sym->type->kind == CN_TYPE_ENUM);
    
    // 检查枚举成员符号
    CnSemSymbol *red_sym = cn_sem_scope_lookup(global_scope, "红色", strlen("红色"));
    assert(red_sym != NULL);
    assert(red_sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER);
    assert(red_sym->type != NULL);
    assert(red_sym->type->kind == CN_TYPE_INT);
    
    cn_sem_scope_free(global_scope);
    cn_support_diagnostics_free(&diagnostics);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    
    printf("✓ 测试枚举的语义分析通过\n");
}

int main() {
    printf("开始枚举类型测试...\n\n");
    
    test_simple_enum_parsing();
    test_enum_with_explicit_values();
    test_enum_with_non_sequential_values();
    test_enum_semantic_analysis();
    
    printf("\n所有枚举测试通过！✓\n");
    return 0;
}
