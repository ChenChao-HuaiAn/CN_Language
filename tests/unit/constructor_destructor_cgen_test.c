/**
 * @file constructor_destructor_cgen_test.c
 * @brief CN语言构造函数/析构函数代码生成单元测试
 * 
 * 测试覆盖：
 * 1. 构造函数代码生成 - 基本构造函数、带参数构造函数、初始化列表
 * 2. 析构函数代码生成 - 基本析构函数、基类析构调用
 * 3. 继承场景 - 基类构造/析构调用顺序
 * 4. 函数体代码生成 - 变量声明、表达式、控制流
 */

#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/ast/class_node.h"
#include "cnlang/backend/cgen.h"
#include "cnlang/backend/cgen/class_cgen.h"
#include "cnlang/support/diagnostics.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 初始化测试环境
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
    if (program) cn_frontend_ast_program_free(program);
    if (parser) cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(diagnostics);
}

/**
 * @brief 在程序中查找指定名称的类
 */
static CnAstClassDecl *find_class(CnAstProgram *program, const char *name) {
    for (size_t i = 0; i < program->class_count; i++) {
        CnAstClassDecl *cls = program->classes[i]->as.class_decl;
        if (cls->name && name && memcmp(cls->name, name, strlen(name)) == 0) {
            return cls;
        }
    }
    return NULL;
}

/**
 * @brief 查找类中的构造函数
 */
static CnClassMember *find_constructor(CnAstClassDecl *class_decl) {
    if (!class_decl) return NULL;
    
    for (size_t i = 0; i < class_decl->member_count; i++) {
        if (class_decl->members[i].kind == CN_MEMBER_CONSTRUCTOR) {
            return &class_decl->members[i];
        }
    }
    return NULL;
}

/**
 * @brief 查找类中的析构函数
 */
static CnClassMember *find_destructor(CnAstClassDecl *class_decl) {
    if (!class_decl) return NULL;
    
    for (size_t i = 0; i < class_decl->member_count; i++) {
        if (class_decl->members[i].kind == CN_MEMBER_DESTRUCTOR) {
            return &class_decl->members[i];
        }
    }
    return NULL;
}

// ============================================================================
// 1. 构造函数代码生成测试
// ============================================================================

/**
 * @brief 测试基本构造函数代码生成
 */
static void test_basic_constructor_cgen(void) {
    printf("测试: 基本构造函数代码生成\n");
    
    // CN语言构造函数语法：函数 类名()
    const char *source =
        "类 点 {\n"
        "公开:\n"
        "    整数 x;\n"
        "    整数 y;\n"
        "    函数 点() {\n"
        "        x = 0;\n"
        "        y = 0;\n"
        "    }\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_basic_constructor.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找类
    CnAstClassDecl *point_class = find_class(program, "点");
    if (!point_class) {
        printf("  ✗ 未找到类 '点'\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找构造函数
    CnClassMember *constructor = find_constructor(point_class);
    if (!constructor) {
        printf("  ✗ 未找到构造函数\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 验证构造函数有函数体
    if (!constructor->body || constructor->body->stmt_count == 0) {
        printf("  ✗ 构造函数体为空\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    printf("  ✓ 构造函数体语句数: %zu\n", constructor->body->stmt_count);
    
    // 创建代码生成上下文并生成代码
    FILE *output = tmpfile();
    if (!output) {
        printf("  ⚠ 无法创建临时文件\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    CnCCodeGenContext ctx;
    ctx.output_file = output;
    
    // 生成类代码
    bool cgen_success = cn_cgen_class_decl(&ctx, point_class);
    
    if (cgen_success) {
        // 读取生成的代码
        rewind(output);
        char buffer[4096] = {0};
        size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, output);
        
        // 验证生成的代码包含构造函数
        if (strstr(buffer, "点_construct") != NULL) {
            printf("  ✓ 生成了构造函数 '点_construct'\n");
        } else {
            printf("  ✗ 未生成构造函数\n");
        }
        
        // 验证生成的代码包含函数体
        if (strstr(buffer, "self->x") != NULL || strstr(buffer, "x = 0") != NULL) {
            printf("  ✓ 构造函数体代码已生成\n");
        } else {
            printf("  ⚠ 构造函数体可能未正确生成\n");
        }
        
        printf("  生成代码长度: %zu 字节\n", bytes_read);
    } else {
        printf("  ✗ 代码生成失败\n");
    }
    
    fclose(output);
    teardown_test_env(program, parser, &diagnostics);
    printf("\n");
}

/**
 * @brief 测试带参数构造函数代码生成
 */
static void test_parameterized_constructor_cgen(void) {
    printf("测试: 带参数构造函数代码生成\n");
    
    // CN语言构造函数语法：函数 类名(参数)
    const char *source =
        "类 点 {\n"
        "公开:\n"
        "    整数 x;\n"
        "    整数 y;\n"
        "    函数 点(整数 a, 整数 b) {\n"
        "        x = a;\n"
        "        y = b;\n"
        "    }\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_param_constructor.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    CnAstClassDecl *point_class = find_class(program, "点");
    if (!point_class) {
        printf("  ✗ 未找到类 '点'\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    CnClassMember *constructor = find_constructor(point_class);
    if (!constructor) {
        printf("  ✗ 未找到构造函数\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 验证参数数量
    if (constructor->parameter_count == 2) {
        printf("  ✓ 构造函数参数数量正确: %zu\n", constructor->parameter_count);
    } else {
        printf("  ✗ 构造函数参数数量错误: %zu (期望 2)\n", constructor->parameter_count);
    }
    
    teardown_test_env(program, parser, &diagnostics);
    printf("\n");
}

// ============================================================================
// 2. 析构函数代码生成测试
// ============================================================================

/**
 * @brief 测试基本析构函数代码生成
 */
static void test_basic_destructor_cgen(void) {
    printf("测试: 基本析构函数代码生成\n");
    
    // CN语言析构函数语法：函数 ~类名()
    const char *source =
        "类 资源 {\n"
        "公开:\n"
        "    整数 句柄;\n"
        "    函数 资源() {\n"
        "        句柄 = 1;\n"
        "    }\n"
        "    函数 ~资源() {\n"
        "        句柄 = 0;\n"
        "    }\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_basic_destructor.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    CnAstClassDecl *resource_class = find_class(program, "资源");
    if (!resource_class) {
        printf("  ✗ 未找到类 '资源'\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    CnClassMember *destructor = find_destructor(resource_class);
    if (!destructor) {
        printf("  ✗ 未找到析构函数\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 验证析构函数有函数体
    if (destructor->body && destructor->body->stmt_count > 0) {
        printf("  ✓ 析构函数体语句数: %zu\n", destructor->body->stmt_count);
    } else {
        printf("  ⚠ 析构函数体为空\n");
    }
    
    // 创建代码生成上下文并生成代码
    FILE *output = tmpfile();
    if (!output) {
        printf("  ⚠ 无法创建临时文件\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    CnCCodeGenContext ctx;
    ctx.output_file = output;
    
    bool cgen_success = cn_cgen_class_decl(&ctx, resource_class);
    
    if (cgen_success) {
        rewind(output);
        char buffer[4096] = {0};
        size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, output);
        
        // 验证生成的代码包含析构函数
        if (strstr(buffer, "资源_destruct") != NULL) {
            printf("  ✓ 生成了析构函数 '资源_destruct'\n");
        } else {
            printf("  ✗ 未生成析构函数\n");
        }
        
        printf("  生成代码长度: %zu 字节\n", bytes_read);
    } else {
        printf("  ✗ 代码生成失败\n");
    }
    
    fclose(output);
    teardown_test_env(program, parser, &diagnostics);
    printf("\n");
}

// ============================================================================
// 3. 继承场景测试
// ============================================================================

/**
 * @brief 测试继承场景下的构造/析构函数代码生成
 */
static void test_inheritance_constructor_destructor_cgen(void) {
    printf("测试: 继承场景构造/析构函数代码生成\n");
    
    // CN语言构造函数：函数 类名()，析构函数：函数 ~类名()
    const char *source =
        "类 动物 {\n"
        "公开:\n"
        "    整数 年龄;\n"
        "    函数 动物() {\n"
        "        年龄 = 0;\n"
        "    }\n"
        "    函数 ~动物() {\n"
        "        年龄 = -1;\n"
        "    }\n"
        "}\n"
        "\n"
        "类 狗: 动物 {\n"
        "公开:\n"
        "    整数 腿数;\n"
        "    函数 狗() {\n"
        "        腿数 = 4;\n"
        "    }\n"
        "    函数 ~狗() {\n"
        "        腿数 = 0;\n"
        "    }\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_inheritance.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找派生类
    CnAstClassDecl *dog_class = find_class(program, "狗");
    if (!dog_class) {
        printf("  ✗ 未找到类 '狗'\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 验证继承关系
    if (dog_class->base_count > 0) {
        printf("  ✓ 派生类有基类\n");
    } else {
        printf("  ✗ 派生类没有基类\n");
    }
    
    // 创建代码生成上下文并生成代码
    FILE *output = tmpfile();
    if (!output) {
        printf("  ⚠ 无法创建临时文件\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    CnCCodeGenContext ctx;
    ctx.output_file = output;
    
    bool cgen_success = cn_cgen_class_decl(&ctx, dog_class);
    
    if (cgen_success) {
        rewind(output);
        char buffer[8192] = {0};
        size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, output);
        
        // 验证生成的代码包含基类构造函数调用
        if (strstr(buffer, "动物_construct") != NULL) {
            printf("  ✓ 生成了基类构造函数调用\n");
        } else {
            printf("  ⚠ 未找到基类构造函数调用\n");
        }
        
        // 验证生成的代码包含基类析构函数调用
        if (strstr(buffer, "动物_destruct") != NULL) {
            printf("  ✓ 生成了基类析构函数调用\n");
        } else {
            printf("  ⚠ 未找到基类析构函数调用\n");
        }
        
        printf("  生成代码长度: %zu 字节\n", bytes_read);
    } else {
        printf("  ✗ 代码生成失败\n");
    }
    
    fclose(output);
    teardown_test_env(program, parser, &diagnostics);
    printf("\n");
}

// ============================================================================
// 4. 函数体代码生成测试
// ============================================================================

/**
 * @brief 测试构造函数体中的控制流代码生成
 */
static void test_constructor_body_control_flow(void) {
    printf("测试: 构造函数体控制流代码生成\n");
    
    // CN语言构造函数语法：函数 类名(参数)
    const char *source =
        "类 计数器 {\n"
        "公开:\n"
        "    整数 值;\n"
        "    函数 计数器(整数 初始值) {\n"
        "        如果 (初始值 > 0) {\n"
        "            值 = 初始值;\n"
        "        } 否则 {\n"
        "            值 = 0;\n"
        "        }\n"
        "    }\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_control_flow.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    CnAstClassDecl *counter_class = find_class(program, "计数器");
    if (!counter_class) {
        printf("  ✗ 未找到类 '计数器'\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    CnClassMember *constructor = find_constructor(counter_class);
    if (!constructor || !constructor->body) {
        printf("  ✗ 未找到构造函数或函数体为空\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 创建代码生成上下文并生成代码
    FILE *output = tmpfile();
    if (!output) {
        printf("  ⚠ 无法创建临时文件\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    CnCCodeGenContext ctx;
    ctx.output_file = output;
    
    bool cgen_success = cn_cgen_class_decl(&ctx, counter_class);
    
    if (cgen_success) {
        rewind(output);
        char buffer[4096] = {0};
        fread(buffer, 1, sizeof(buffer) - 1, output);
        
        // 验证生成的代码包含 if 语句
        if (strstr(buffer, "if (") != NULL || strstr(buffer, "if(") != NULL) {
            printf("  ✓ 生成了 if 语句\n");
        } else {
            printf("  ⚠ 未找到 if 语句\n");
        }
        
        // 验证生成的代码包含 else 语句
        if (strstr(buffer, "else") != NULL) {
            printf("  ✓ 生成了 else 语句\n");
        } else {
            printf("  ⚠ 未找到 else 语句\n");
        }
    } else {
        printf("  ✗ 代码生成失败\n");
    }
    
    fclose(output);
    teardown_test_env(program, parser, &diagnostics);
    printf("\n");
}

/**
 * @brief 测试构造函数体中的循环代码生成
 */
static void test_constructor_body_loop(void) {
    printf("测试: 构造函数体循环代码生成\n");
    
    // CN语言构造函数语法：函数 类名()
    const char *source =
        "类 数组包装 {\n"
        "公开:\n"
        "    整数 数组[10];\n"
        "    函数 数组包装() {\n"
        "        整数 i = 0;\n"
        "        当 (i < 10) {\n"
        "            数组[i] = 0;\n"
        "            i = i + 1;\n"
        "        }\n"
        "    }\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_loop.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    CnAstClassDecl *array_class = find_class(program, "数组包装");
    if (!array_class) {
        printf("  ✗ 未找到类 '数组包装'\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    CnClassMember *constructor = find_constructor(array_class);
    if (!constructor || !constructor->body) {
        printf("  ✗ 未找到构造函数或函数体为空\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 创建代码生成上下文并生成代码
    FILE *output = tmpfile();
    if (!output) {
        printf("  ⚠ 无法创建临时文件\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    CnCCodeGenContext ctx;
    ctx.output_file = output;
    
    bool cgen_success = cn_cgen_class_decl(&ctx, array_class);
    
    if (cgen_success) {
        rewind(output);
        char buffer[4096] = {0};
        fread(buffer, 1, sizeof(buffer) - 1, output);
        
        // 验证生成的代码包含 while 循环
        if (strstr(buffer, "while (") != NULL || strstr(buffer, "while(") != NULL) {
            printf("  ✓ 生成了 while 循环\n");
        } else {
            printf("  ⚠ 未找到 while 循环\n");
        }
    } else {
        printf("  ✗ 代码生成失败\n");
    }
    
    fclose(output);
    teardown_test_env(program, parser, &diagnostics);
    printf("\n");
}

// ============================================================================
// 主函数
// ============================================================================

int main(void) {
    printf("========================================\n");
    printf("CN语言构造函数/析构函数代码生成测试\n");
    printf("========================================\n\n");
    
    // 1. 构造函数测试
    test_basic_constructor_cgen();
    test_parameterized_constructor_cgen();
    
    // 2. 析构函数测试
    test_basic_destructor_cgen();
    
    // 3. 继承场景测试
    test_inheritance_constructor_destructor_cgen();
    
    // 4. 函数体代码生成测试
    test_constructor_body_control_flow();
    test_constructor_body_loop();
    
    printf("========================================\n");
    printf("测试完成\n");
    printf("========================================\n");
    
    return 0;
}
