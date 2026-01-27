// 测试：函数参数与结构体字段的类型覆盖补全
// 验证 parse_type 统一类型解析函数支持所有类型组合

#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

// 测试：函数参数支持基础类型和指针类型
// 注意：当前解析器尚未支持 '整数 arr[]' 数组参数语法
static void test_function_param_type_coverage(void) {
    const char *source = 
        "函数 测试函数(\n"
        "    整数 参数1,\n"
        "    小数 参数2,\n"
        "    布尔 参数3,\n"
        "    字符串 参数4,\n"
        "    整数* 参数5,\n"
        "    小数* 参数6\n"
        ") {\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 函数参数类型覆盖\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && "解析应该成功");
    assert(program != NULL);
    assert(program->function_count == 1);
    
    // 检查函数声明
    CnAstFunctionDecl *func = program->functions[0];
    assert(func->parameter_count == 6);
    
    // 验证参数类型
    assert(func->parameters[0].declared_type->kind == CN_TYPE_INT);
    assert(func->parameters[1].declared_type->kind == CN_TYPE_FLOAT);
    assert(func->parameters[2].declared_type->kind == CN_TYPE_BOOL);
    assert(func->parameters[3].declared_type->kind == CN_TYPE_STRING);
    assert(func->parameters[4].declared_type->kind == CN_TYPE_POINTER);
    assert(func->parameters[4].declared_type->as.pointer_to->kind == CN_TYPE_INT);
    assert(func->parameters[5].declared_type->kind == CN_TYPE_POINTER);
    assert(func->parameters[5].declared_type->as.pointer_to->kind == CN_TYPE_FLOAT);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 函数参数类型覆盖测试通过\n");
}

// 测试：结构体字段支持基础类型和指针类型
// 注意：当前解析器尚未支持 '整数 字段[10]' 数组字段语法
static void test_struct_field_type_coverage(void) {
    const char *source = 
        "结构体 复杂结构 {\n"
        "    整数 字段1;\n"
        "    小数 字段2;\n"
        "    布尔 字段3;\n"
        "    字符串 字段4;\n"
        "    整数* 字段5;\n"
        "    小数* 字段6;\n"
        "}\n";
    
    printf("测试: 结构体字段类型覆盖\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && "解析应该成功");
    assert(program != NULL);
    assert(program->struct_count == 1);
    
    // 检查结构体声明
    CnAstStmt *struct_stmt = program->structs[0];
    assert(struct_stmt->kind == CN_AST_STMT_STRUCT_DECL);
    
    CnAstStructDecl *struct_decl = &struct_stmt->as.struct_decl;
    assert(struct_decl->field_count == 6);
    
    // 验证字段类型
    assert(struct_decl->fields[0].field_type->kind == CN_TYPE_INT);
    assert(struct_decl->fields[1].field_type->kind == CN_TYPE_FLOAT);
    assert(struct_decl->fields[2].field_type->kind == CN_TYPE_BOOL);
    assert(struct_decl->fields[3].field_type->kind == CN_TYPE_STRING);
    assert(struct_decl->fields[4].field_type->kind == CN_TYPE_POINTER);
    assert(struct_decl->fields[4].field_type->as.pointer_to->kind == CN_TYPE_INT);
    assert(struct_decl->fields[5].field_type->kind == CN_TYPE_POINTER);
    assert(struct_decl->fields[5].field_type->as.pointer_to->kind == CN_TYPE_FLOAT);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 结构体字段类型覆盖测试通过\n");
}

// 测试：多级指针支持
static void test_multi_level_pointer(void) {
    const char *source = 
        "函数 测试多级指针(\n"
        "    整数** 二级指针,\n"
        "    小数*** 三级指针\n"
        ") {\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 多级指针支持\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && "解析应该成功");
    assert(program != NULL);
    assert(program->function_count == 1);
    
    CnAstFunctionDecl *func = program->functions[0];
    assert(func->parameter_count == 2);
    
    // 验证二级指针: 整数**
    CnType *param1_type = func->parameters[0].declared_type;
    assert(param1_type->kind == CN_TYPE_POINTER);
    assert(param1_type->as.pointer_to->kind == CN_TYPE_POINTER);
    assert(param1_type->as.pointer_to->as.pointer_to->kind == CN_TYPE_INT);
    
    // 验证三级指针: 小数***
    CnType *param2_type = func->parameters[1].declared_type;
    assert(param2_type->kind == CN_TYPE_POINTER);
    assert(param2_type->as.pointer_to->kind == CN_TYPE_POINTER);
    assert(param2_type->as.pointer_to->as.pointer_to->kind == CN_TYPE_POINTER);
    assert(param2_type->as.pointer_to->as.pointer_to->as.pointer_to->kind == CN_TYPE_FLOAT);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 多级指针支持测试通过\n");
}

// 测试：变量声明也使用统一类型解析
// 注意：当前解析器尚未支持 '整数 变量[]' 语法
static void test_variable_decl_type_coverage(void) {
    const char *source = 
        "函数 主程序() {\n"
        "    整数 变量1 = 10;\n"
        "    小数 变量2 = 3.14;\n"
        "    布尔 变量3 = 真;\n"
        "    字符串 变量4 = \"测试\";\n"
        "    整数* 变量5;\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 变量声明类型覆盖\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && "解析应该成功");
    assert(program != NULL);
    assert(program->function_count == 1);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 变量声明类型覆盖测试通过\n");
}

int main(void) {
    printf("=== 类型覆盖补全测试 ===\n");
    
    test_function_param_type_coverage();
    test_struct_field_type_coverage();
    test_multi_level_pointer();
    test_variable_decl_type_coverage();
    
    printf("parser_type_coverage_test: 所有测试通过\n");
    return 0;
}
