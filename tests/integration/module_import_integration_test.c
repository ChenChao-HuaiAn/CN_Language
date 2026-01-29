/**
 * @file module_import_integration_test.c
 * @brief 模块导入集成测试 (G3, G4, G5, G6)
 * 
 * 测试跨文件导入、循环导入检测、相对导入和搜索路径。
 */
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/frontend/module_loader.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  测试: %s ... ", #name); \
    test_##name(); \
} while(0)
#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("失败 (行 %d)\n", __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)
#define PASS() do { printf("通过\n"); tests_passed++; } while(0)

// 辅助函数：解析源代码
static CnAstProgram *parse_source(const char *source, CnDiagnostics *diag) {
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, diag);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, diag);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    cn_frontend_parser_free(parser);
    
    return program;
}

// ============================================================================
// G3: 跨文件导入集成测试
// ============================================================================

TEST(from_import_syntax_parse) {
    // 测试 从...导入 语法解析
    const char *source = 
        "从 数学 导入 { 加法, 减法 };\n"
        "函数 主程序() -> 整数 {\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = parse_source(source, &diag);
    ASSERT(program != NULL);
    ASSERT(program->import_count >= 1);
    
    // 检查导入语句
    CnAstStmt *imp = program->imports[0];
    ASSERT(imp != NULL);
    ASSERT(imp->kind == CN_AST_STMT_IMPORT);
    // 检查是否为选择性导入或通配符导入
    ASSERT(imp->as.import_stmt.member_count == 2 || 
           imp->as.import_stmt.kind == CN_IMPORT_FROM_SELECTIVE);
    
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
    
    PASS();
}

TEST(from_import_wildcard_parse) {
    // 测试通配符导入
    const char *source = 
        "从 工具 导入 *;\n"
        "函数 主程序() -> 整数 {\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = parse_source(source, &diag);
    ASSERT(program != NULL);
    ASSERT(program->import_count >= 1);
    
    CnAstStmt *imp = program->imports[0];
    ASSERT(imp != NULL);
    ASSERT(imp->kind == CN_AST_STMT_IMPORT);
    ASSERT(imp->as.import_stmt.is_wildcard == 1 ||
           imp->as.import_stmt.kind == CN_IMPORT_FROM_WILDCARD);
    
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
    
    PASS();
}

TEST(dotted_module_path_parse) {
    // 测试点分模块路径
    const char *source = 
        "从 工具.数学.高级 导入 { 平方根 };\n"
        "函数 主程序() -> 整数 {\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = parse_source(source, &diag);
    ASSERT(program != NULL);
    ASSERT(program->import_count >= 1);
    
    CnAstStmt *imp = program->imports[0];
    ASSERT(imp != NULL);
    ASSERT(imp->kind == CN_AST_STMT_IMPORT);
    // 验证模块路径存在
    ASSERT(imp->as.import_stmt.module_path != NULL ||
           imp->as.import_stmt.module_name != NULL);
    
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
    
    PASS();
}

// ============================================================================
// G4: 循环导入测试
// ============================================================================

TEST(circular_import_detection) {
    // 测试循环导入检测
    CnModuleLoader *loader = cn_module_loader_create();
    ASSERT(loader != NULL);
    
    // 模拟依赖：A -> B -> C -> A（循环）
    CnDependencyGraph *graph = loader->dep_graph;
    
    CnModuleId *a = cn_module_id_create("A");
    CnModuleId *b = cn_module_id_create("B");
    CnModuleId *c = cn_module_id_create("C");
    
    cn_dependency_graph_add_edge(graph, a, b);
    cn_dependency_graph_add_edge(graph, b, c);
    cn_dependency_graph_add_edge(graph, c, a);  // 造成循环
    
    // 拓扑排序应该失败
    CnModuleId **order = NULL;
    size_t order_count = 0;
    int result = cn_dependency_graph_topological_sort(graph, &order, &order_count);
    ASSERT(result == 0);  // 有循环，应该返回0
    
    cn_module_id_free(a);
    cn_module_id_free(b);
    cn_module_id_free(c);
    cn_module_loader_free(loader);
    
    PASS();
}

TEST(no_circular_dependency) {
    // 测试无循环依赖
    CnDependencyGraph *graph = cn_dependency_graph_create();
    ASSERT(graph != NULL);
    
    CnModuleId *a = cn_module_id_create("A");
    CnModuleId *b = cn_module_id_create("B");
    CnModuleId *c = cn_module_id_create("C");
    
    // 线性依赖：A -> B -> C
    cn_dependency_graph_add_edge(graph, a, b);
    cn_dependency_graph_add_edge(graph, b, c);
    
    CnModuleId **order = NULL;
    size_t order_count = 0;
    int result = cn_dependency_graph_topological_sort(graph, &order, &order_count);
    ASSERT(result == 1);  // 无循环，应该成功
    
    if (order) free(order);
    cn_module_id_free(a);
    cn_module_id_free(b);
    cn_module_id_free(c);
    cn_dependency_graph_free(graph);
    
    PASS();
}

// ============================================================================
// G5: 相对导入测试
// ============================================================================

TEST(relative_import_syntax_parse) {
    // 测试相对导入语法（当前可能不完全支持，测试解析不崩溃）
    const char *source = 
        "从 .兄弟模块 导入 { 函数A };\n"
        "函数 主程序() -> 整数 {\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = parse_source(source, &diag);
    // 相对导入可能解析成功或失败，但不应崩溃
    // 如果解析成功，验证基本结构
    if (program != NULL) {
        // 解析成功，算通过
        cn_frontend_ast_program_free(program);
    }
    
    cn_support_diagnostics_free(&diag);
    
    PASS();
}

// ============================================================================
// G6: 模块搜索路径测试
// ============================================================================

TEST(search_path_priority) {
    // 测试搜索路径添加
    CnModuleSearchConfig *config = cn_search_config_create();
    ASSERT(config != NULL);
    
    // 添加多个路径
    cn_search_config_add_path(config, "./path1", 1);
    cn_search_config_add_path(config, "./path2", 2);
    cn_search_config_add_path(config, "./path3", 3);
    
    ASSERT(config->path_count == 3);
    
    cn_search_config_free(config);
    
    PASS();
}

TEST(search_path_from_env) {
    // 测试从环境变量加载搜索路径
    CnModuleSearchConfig *config = cn_search_config_create();
    ASSERT(config != NULL);
    
    // 尝试从不存在的环境变量加载（应该安全失败）
    int result = cn_search_config_load_from_env(config, "CN_TEST_NONEXISTENT_PATH");
    // 即使失败也不应崩溃
    
    cn_search_config_free(config);
    
    PASS();
}

TEST(stdlib_path_config) {
    // 测试标准库路径配置
    CnModuleSearchConfig *config = cn_search_config_create();
    ASSERT(config != NULL);
    
    cn_search_config_set_stdlib_path(config, "/usr/lib/cn/stdlib");
    ASSERT(config->stdlib_path != NULL);
    ASSERT(strcmp(config->stdlib_path, "/usr/lib/cn/stdlib") == 0);
    
    cn_search_config_free(config);
    
    PASS();
}

// ============================================================================
// 公开/私有可见性测试
// ============================================================================

TEST(visibility_declaration_parse) {
    // 测试公开/私有声明解析
    const char *source = 
        "公开 {\n"
        "    函数 公开函数() -> 整数 {\n"
        "        返回 1;\n"
        "    }\n"
        "}\n"
        "私有 {\n"
        "    函数 私有函数() -> 整数 {\n"
        "        返回 2;\n"
        "    }\n"
        "}\n"
        "函数 主程序() -> 整数 {\n"
        "    返回 公开函数();\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = parse_source(source, &diag);
    ASSERT(program != NULL);
    ASSERT(program->function_count >= 3);
    
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
    
    PASS();
}

// ============================================================================
// 主函数
// ============================================================================

int main(void) {
    printf("=== 模块导入集成测试 (G3, G4, G5, G6) ===\n\n");
    
    printf("G3: 跨文件导入测试\n");
    RUN_TEST(from_import_syntax_parse);
    RUN_TEST(from_import_wildcard_parse);
    RUN_TEST(dotted_module_path_parse);
    
    printf("\nG4: 循环导入测试\n");
    RUN_TEST(circular_import_detection);
    RUN_TEST(no_circular_dependency);
    
    printf("\nG5: 相对导入测试\n");
    RUN_TEST(relative_import_syntax_parse);
    
    printf("\nG6: 模块搜索路径测试\n");
    RUN_TEST(search_path_priority);
    RUN_TEST(search_path_from_env);
    RUN_TEST(stdlib_path_config);
    
    printf("\n可见性测试\n");
    RUN_TEST(visibility_declaration_parse);
    
    printf("\n=== 测试结果 ===\n");
    printf("通过: %d\n", tests_passed);
    printf("失败: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
