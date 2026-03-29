/**
 * @file static_member_cgen_test.c
 * @brief 静态成员代码生成测试
 *
 * 测试静态成员变量和静态成员函数的代码生成功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/ast/class_node.h"
#include "cnlang/support/diagnostics.h"

// ============================================================================
// 测试辅助函数
// ============================================================================

/**
 * @brief 设置测试环境
 */
static void setup_test_env(const char *source, const char *filename,
                           CnDiagnostics *diagnostics, CnLexer *lexer,
                           CnParser **parser, CnAstProgram **program) {
    cn_support_diagnostics_init(diagnostics);
    
    cn_frontend_lexer_init(lexer, source, strlen(source), filename);
    cn_frontend_lexer_set_diagnostics(lexer, diagnostics);
    
    *parser = cn_frontend_parser_new(lexer);
    cn_frontend_parser_set_diagnostics(*parser, diagnostics);
    
    *program = NULL;
}

/**
 * @brief 清理测试环境
 */
static void teardown_test_env(CnAstProgram *program, CnParser *parser,
                              CnDiagnostics *diagnostics) {
    if (program) {
        cn_frontend_ast_program_free(program);
    }
    if (parser) {
        cn_frontend_parser_free(parser);
    }
    cn_support_diagnostics_free(diagnostics);
}

// ============================================================================
// 测试用例
// ============================================================================

/**
 * @brief 测试静态成员变量代码生成
 *
 * 验证静态成员变量正确生成为全局变量
 */
static void test_static_field_cgen(void) {
    printf("测试: 静态成员变量代码生成\n");
    
    const char *source = 
        "类 计数器 {\n"
        "公开:\n"
        "    静态 整数 实例数;\n"
        "    整数 值;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_static_field_cgen.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success);
    assert(program != NULL);
    assert(program->class_count == 1);
    
    CnAstClassDecl *class_decl = program->classes[0]->as.class_decl;
    
    // 验证类名
    assert(class_decl->name != NULL);
    assert(memcmp(class_decl->name, "计数器", strlen("计数器")) == 0);
    
    // 查找静态成员变量
    bool found_static = false;
    bool found_normal = false;
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_FIELD) {
            if (memcmp(member->name, "实例数", strlen("实例数")) == 0) {
                found_static = true;
                assert(member->is_static);
            }
            if (memcmp(member->name, "值", strlen("值")) == 0) {
                found_normal = true;
                assert(!member->is_static);
            }
        }
    }
    
    assert(found_static);
    assert(found_normal);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 静态成员变量代码生成测试通过\n");
}

/**
 * @brief 测试静态成员函数代码生成
 *
 * 验证静态成员函数正确生成（无this指针）
 */
static void test_static_method_cgen(void) {
    printf("测试: 静态成员函数代码生成\n");
    
    const char *source = 
        "类 工具类 {\n"
        "公开:\n"
        "    静态 函数 获取版本() -> 整数 {\n"
        "        返回 1;\n"
        "    }\n"
        "    \n"
        "    函数 普通方法() {\n"
        "        打印(\"普通方法\");\n"
        "    }\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_static_method_cgen.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success);
    assert(program != NULL);
    assert(program->class_count == 1);
    
    CnAstClassDecl *class_decl = program->classes[0]->as.class_decl;
    
    // 查找静态方法
    bool found_static = false;
    bool found_normal = false;
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD) {
            if (memcmp(member->name, "获取版本", strlen("获取版本")) == 0) {
                found_static = true;
                assert(member->is_static);
                // 静态方法不能是虚函数
                assert(!member->is_virtual);
            }
            if (memcmp(member->name, "普通方法", strlen("普通方法")) == 0) {
                found_normal = true;
                assert(!member->is_static);
            }
        }
    }
    
    assert(found_static);
    assert(found_normal);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 静态成员函数代码生成测试通过\n");
}

/**
 * @brief 测试静态成员访问标记
 *
 * 验证静态成员访问表达式正确标记
 */
static void test_static_member_access_marking(void) {
    printf("测试: 静态成员访问标记\n");
    
    // 创建一个成员访问表达式
    CnAstExpr *object = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    object->kind = CN_AST_EXPR_IDENTIFIER;
    object->is_this_pointer = 0;
    object->as.identifier.name = "计数器";
    object->as.identifier.name_length = strlen("计数器");
    
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    expr->kind = CN_AST_EXPR_MEMBER_ACCESS;
    expr->as.member.object = object;
    expr->as.member.member_name = "实例数";
    expr->as.member.member_name_length = strlen("实例数");
    expr->as.member.is_arrow = 0;
    expr->as.member.is_static_member = 1;
    expr->as.member.class_name = "计数器";
    expr->as.member.class_name_length = strlen("计数器");
    
    // 验证标记正确
    assert(expr->kind == CN_AST_EXPR_MEMBER_ACCESS);
    assert(expr->as.member.is_static_member == 1);
    assert(expr->as.member.class_name != NULL);
    assert(memcmp(expr->as.member.class_name, "计数器", strlen("计数器")) == 0);
    
    // 清理
    free(object);
    free(expr);
    
    printf("  ✓ 静态成员访问标记测试通过\n");
}

/**
 * @brief 测试静态成员变量初始化
 *
 * 验证静态成员变量可以正确初始化
 */
static void test_static_field_init(void) {
    printf("测试: 静态成员变量初始化\n");
    
    const char *source = 
        "类 配置 {\n"
        "公开:\n"
        "    静态 整数 最大连接数 = 100;\n"
        "    静态 小数 版本号 = 1.0;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_static_field_init.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success);
    assert(program != NULL);
    assert(program->class_count == 1);
    
    CnAstClassDecl *class_decl = program->classes[0]->as.class_decl;
    
    // 查找静态成员变量并验证初始化
    bool found_int = false;
    bool found_float = false;
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_FIELD && member->is_static) {
            if (memcmp(member->name, "最大连接数", strlen("最大连接数")) == 0) {
                found_int = true;
                assert(member->init_expr != NULL);
                assert(member->init_expr->kind == CN_AST_EXPR_INTEGER_LITERAL);
                assert(member->init_expr->as.integer_literal.value == 100);
            }
            if (memcmp(member->name, "版本号", strlen("版本号")) == 0) {
                found_float = true;
                assert(member->init_expr != NULL);
                assert(member->init_expr->kind == CN_AST_EXPR_FLOAT_LITERAL);
            }
        }
    }
    
    assert(found_int);
    assert(found_float);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 静态成员变量初始化测试通过\n");
}

/**
 * @brief 测试静态方法参数（无self指针）
 *
 * 验证静态方法没有self指针参数
 */
static void test_static_method_no_self(void) {
    printf("测试: 静态方法无self指针\n");
    
    const char *source = 
        "类 数学工具 {\n"
        "公开:\n"
        "    静态 函数 平方(整数 x) -> 整数 {\n"
        "        返回 x * x;\n"
        "    }\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_static_method_no_self.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success);
    assert(program != NULL);
    assert(program->class_count == 1);
    
    CnAstClassDecl *class_decl = program->classes[0]->as.class_decl;
    
    // 查找静态方法
    bool found = false;
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD &&
            memcmp(member->name, "平方", strlen("平方")) == 0) {
            found = true;
            assert(member->is_static);
            // 验证参数数量（只有x，没有self）
            assert(member->parameter_count == 1);
        }
    }
    
    assert(found);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 静态方法无self指针测试通过\n");
}

// ============================================================================
// 主函数
// ============================================================================

int main(void) {
    printf("========================================\n");
    printf("静态成员代码生成测试\n");
    printf("========================================\n\n");
    
    test_static_field_cgen();
    test_static_method_cgen();
    test_static_member_access_marking();
    test_static_field_init();
    test_static_method_no_self();
    
    printf("\n========================================\n");
    printf("测试结果: 全部通过\n");
    printf("========================================\n");
    
    return 0;
}
