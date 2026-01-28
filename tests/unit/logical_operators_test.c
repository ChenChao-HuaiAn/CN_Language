/**
 * @file logical_operators_test.c
 * @brief 测试逻辑操作符（&& 和 ||）的 IR 生成和 C 代码生成
 * 
 * 验证问题：CN 语言编译器在处理复杂条件语句中逻辑与/逻辑或操作符时
 * 生成的 C 代码中包含未定义的标签，导致编译失败。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/ir/irgen.h"
#include "cnlang/backend/cgen.h"
#include "cnlang/support/config.h"

static int test_count = 0;
static int pass_count = 0;

/**
 * 检查生成的 C 代码中的标签是否都有定义
 * 扫描所有 goto 语句引用的标签，确认它们在代码中都有定义
 */
static int check_labels_defined(const char *c_code) {
    // 收集所有 goto 引用的标签
    char labels_used[100][64];
    int label_count = 0;
    
    const char *p = c_code;
    while ((p = strstr(p, "goto ")) != NULL) {
        p += 5;  // 跳过 "goto "
        char label[64];
        int i = 0;
        while (*p && *p != ';' && *p != ' ' && i < 63) {
            label[i++] = *p++;
        }
        label[i] = '\0';
        
        // 检查是否已经记录
        int found = 0;
        for (int j = 0; j < label_count; j++) {
            if (strcmp(labels_used[j], label) == 0) {
                found = 1;
                break;
            }
        }
        if (!found && label_count < 100) {
            strcpy(labels_used[label_count++], label);
        }
    }
    
    // 检查每个标签是否有定义（格式：标签名:）
    for (int i = 0; i < label_count; i++) {
        char label_def[68];
        snprintf(label_def, sizeof(label_def), "%s:", labels_used[i]);
        if (strstr(c_code, label_def) == NULL) {
            printf("标签 '%s' 被引用但未定义\n", labels_used[i]);
            return 0;
        }
    }
    
    return 1;
}

/**
 * 编译 CN 代码到 C 代码并返回生成的 C 代码
 */
static char *compile_to_c(const char *cn_code) {
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, cn_code, strlen(cn_code), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0 || !program) {
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return NULL;
    }
    
    // 语义分析
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return NULL;
    }
    
    // IR 生成
    CnTargetTriple target = cn_support_target_triple_make(
        CN_TARGET_ARCH_X86_64,
        CN_TARGET_VENDOR_PC,
        CN_TARGET_OS_NONE,
        CN_TARGET_ABI_ELF);
    CnIrModule *ir_module = cn_ir_gen_program(program, global_scope, target, CN_COMPILE_MODE_HOSTED);
    
    if (!ir_module) {
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return NULL;
    }
    
    // 运行 IR 优化 passes（包括死代码消除）
    cn_ir_run_default_passes(ir_module);
    
    // C 代码生成 - 输出到字符串
    char *c_code = cn_cgen_module_to_string(ir_module);
    
    cn_ir_module_free(ir_module);
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    return c_code;
}

// 测试1：简单的逻辑与表达式
void test_simple_logical_and(void) {
    test_count++;
    printf("测试 %d: 简单逻辑与表达式... ", test_count);
    
    const char *cn_code = 
        "函数 主程序() -> 整数 {\n"
        "    整数 x = 2;\n"
        "    整数 y = 3;\n"
        "    布尔 result = x == 2 && y == 3;\n"
        "    返回 0;\n"
        "}\n";
    
    char *c_code = compile_to_c(cn_code);
    if (!c_code) {
        printf("失败: 编译失败\n");
        return;
    }
    
    // 检查所有引用的标签是否都有定义（核心验证）
    if (!check_labels_defined(c_code)) {
        free(c_code);
        printf("失败: 存在未定义的标签\n");
        return;
    }
    
    free(c_code);
    pass_count++;
    printf("通过\n");
}

// 测试2：if 语句中的逻辑与
void test_logical_and_in_if(void) {
    test_count++;
    printf("测试 %d: if 语句中的逻辑与... ", test_count);
    
    const char *cn_code = 
        "函数 主程序() -> 整数 {\n"
        "    整数 x = 2;\n"
        "    整数 y = 3;\n"
        "    如果 (x == 2 && y == 3) {\n"
        "        打印(1);\n"
        "    }\n"
        "    返回 0;\n"
        "}\n";
    
    char *c_code = compile_to_c(cn_code);
    if (!c_code) {
        printf("失败: 编译失败\n");
        return;
    }
    
    // 检查所有引用的标签是否都有定义
    if (!check_labels_defined(c_code)) {
        free(c_code);
        printf("失败: 存在未定义的标签\n");
        return;
    }
    
    free(c_code);
    pass_count++;
    printf("通过\n");
}

// 测试3：逻辑或表达式
void test_logical_or(void) {
    test_count++;
    printf("测试 %d: 逻辑或表达式... ", test_count);
    
    const char *cn_code = 
        "函数 主程序() -> 整数 {\n"
        "    整数 x = 2;\n"
        "    整数 y = 3;\n"
        "    布尔 result = x == 1 || y == 3;\n"
        "    返回 0;\n"
        "}\n";
    
    char *c_code = compile_to_c(cn_code);
    if (!c_code) {
        printf("失败: 编译失败\n");
        return;
    }
    
    if (!check_labels_defined(c_code)) {
        free(c_code);
        printf("失败: 存在未定义的标签\n");
        return;
    }
    
    free(c_code);
    pass_count++;
    printf("通过\n");
}

// 测试4：嵌套的逻辑表达式
void test_nested_logical(void) {
    test_count++;
    printf("测试 %d: 嵌套逻辑表达式... ", test_count);
    
    const char *cn_code = 
        "函数 主程序() -> 整数 {\n"
        "    整数 a = 1;\n"
        "    整数 b = 2;\n"
        "    整数 c = 3;\n"
        "    布尔 result = (a == 1 && b == 2) || c == 3;\n"
        "    返回 0;\n"
        "}\n";
    
    char *c_code = compile_to_c(cn_code);
    if (!c_code) {
        printf("失败: 编译失败\n");
        return;
    }
    
    if (!check_labels_defined(c_code)) {
        free(c_code);
        printf("失败: 存在未定义的标签\n");
        return;
    }
    
    free(c_code);
    pass_count++;
    printf("通过\n");
}

// 测试5：while 循环中的逻辑表达式
void test_logical_in_while(void) {
    test_count++;
    printf("测试 %d: while 循环中的逻辑表达式... ", test_count);
    
    const char *cn_code = 
        "函数 主程序() -> 整数 {\n"
        "    整数 x = 0;\n"
        "    整数 y = 0;\n"
        "    当 (x < 10 && y < 5) {\n"
        "        x = x + 1;\n"
        "        y = y + 1;\n"
        "    }\n"
        "    返回 0;\n"
        "}\n";
    
    char *c_code = compile_to_c(cn_code);
    if (!c_code) {
        printf("失败: 编译失败\n");
        return;
    }
    
    if (!check_labels_defined(c_code)) {
        free(c_code);
        printf("失败: 存在未定义的标签\n");
        return;
    }
    
    free(c_code);
    pass_count++;
    printf("通过\n");
}

int main(void) {
    printf("===== CN 语言逻辑操作符测试 =====\n\n");
    
    test_simple_logical_and();
    test_logical_and_in_if();
    test_logical_or();
    test_nested_logical();
    test_logical_in_while();
    
    printf("\n===== 测试结果 =====\n");
    printf("总测试数: %d\n", test_count);
    printf("通过: %d\n", pass_count);
    printf("失败: %d\n", test_count - pass_count);
    
    return (pass_count == test_count) ? 0 : 1;
}
