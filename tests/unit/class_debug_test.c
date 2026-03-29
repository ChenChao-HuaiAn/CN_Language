/**
 * @file class_debug_test.c
 * @brief 类解析调试测试
 */

#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/ast/class_node.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    printf("=== 类解析调试测试 ===\n\n");
    
    // 测试1：简单类定义
    printf("测试1：简单类定义\n");
    const char *source1 = "类 空类 { } 函数 主程序() { 返回 0; }";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source1, strlen(source1), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    printf("解析结果: %s\n", success ? "成功" : "失败");
    printf("错误数量: %zu\n", cn_support_diagnostics_error_count(&diagnostics));
    printf("类数量: %zu\n", program ? program->class_count : 0);
    
    if (program) cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    printf("\n");
    
    // 测试2：带成员变量的类（使用公开: 标签）
    printf("测试2：带成员变量的类（使用公开: 标签）\n");
    const char *source2 = 
        "类 点 {\n"
        "公开:\n"
        "    整数 x;\n"
        "    整数 y;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source2, strlen(source2), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    program = NULL;
    success = cn_frontend_parse_program(parser, &program);
    
    printf("解析结果: %s\n", success ? "成功" : "失败");
    printf("错误数量: %zu\n", cn_support_diagnostics_error_count(&diagnostics));
    
    if (program && program->class_count > 0) {
        CnAstClassDecl *class_decl = program->classes[0]->as.class_decl;
        printf("类名: %.*s\n", (int)class_decl->name_length, class_decl->name);
        printf("成员数量: %zu\n", class_decl->member_count);
        
        for (size_t i = 0; i < class_decl->member_count; i++) {
            CnClassMember *member = &class_decl->members[i];
            printf("  成员 %zu: name='%.*s', kind=%d, access=%d\n", 
                   i, (int)member->name_length, member->name, 
                   member->kind, member->access);
        }
    }
    
    if (program) cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    printf("\n");
    
    // 测试3：单继承
    printf("测试3：单继承\n");
    const char *source3 = 
        "类 动物 {\n"
        "公开:\n"
        "    整数 年龄;\n"
        "}\n"
        "\n"
        "类 狗: 动物 {\n"
        "公开:\n"
        "    整数 腿数;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source3, strlen(source3), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    program = NULL;
    success = cn_frontend_parse_program(parser, &program);
    
    printf("解析结果: %s\n", success ? "成功" : "失败");
    printf("错误数量: %zu\n", cn_support_diagnostics_error_count(&diagnostics));
    printf("类数量: %zu\n", program ? program->class_count : 0);
    
    if (program) {
        for (size_t i = 0; i < program->class_count; i++) {
            CnAstClassDecl *class_decl = program->classes[i]->as.class_decl;
            printf("  类 %zu: name='%.*s', base_count=%zu\n", 
                   i, (int)class_decl->name_length, class_decl->name, 
                   class_decl->base_count);
            if (class_decl->base_count > 0) {
                printf("    基类: %.*s\n", 
                       (int)class_decl->bases[0].base_class_name_length,
                       class_decl->bases[0].base_class_name);
            }
        }
    }
    
    if (program) cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    printf("\n");
    
    // 测试4：虚函数
    printf("测试4：虚函数\n");
    const char *source4 = 
        "类 动物 {\n"
        "公开:\n"
        "    虚拟 函数 说话() {\n"
        "        打印(\"动物发出声音\");\n"
        "    }\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source4, strlen(source4), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    program = NULL;
    success = cn_frontend_parse_program(parser, &program);
    
    printf("解析结果: %s\n", success ? "成功" : "失败");
    printf("错误数量: %zu\n", cn_support_diagnostics_error_count(&diagnostics));
    
    if (program && program->class_count > 0) {
        CnAstClassDecl *class_decl = program->classes[0]->as.class_decl;
        printf("成员数量: %zu\n", class_decl->member_count);
        
        for (size_t i = 0; i < class_decl->member_count; i++) {
            CnClassMember *member = &class_decl->members[i];
            printf("  成员 %zu: name='%.*s', kind=%d, is_virtual=%d\n", 
                   i, (int)member->name_length, member->name, 
                   member->kind, member->is_virtual);
        }
    }
    
    if (program) cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    printf("\n");
    
    // 测试5：接口
    printf("测试5：接口\n");
    const char *source5 = 
        "接口 可序列化 {\n"
        "    虚拟 函数 序列化() -> 字符串 抽象;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source5, strlen(source5), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    program = NULL;
    success = cn_frontend_parse_program(parser, &program);
    
    printf("解析结果: %s\n", success ? "成功" : "失败");
    printf("错误数量: %zu\n", cn_support_diagnostics_error_count(&diagnostics));
    printf("接口数量: %zu\n", program ? program->interface_count : 0);
    
    if (program && program->interface_count > 0) {
        CnAstInterfaceDecl *interface_decl = program->interfaces[0]->as.interface_decl;
        printf("接口名: %.*s\n", (int)interface_decl->name_length, interface_decl->name);
        printf("方法数量: %zu\n", interface_decl->method_count);
    }
    
    if (program) cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("\n=== 测试完成 ===\n");
    return 0;
}
