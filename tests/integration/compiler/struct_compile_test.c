// 结构体编译集成测试
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/ir/irgen.h"
#include "cnlang/backend/cgen.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/support/config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int test_struct_compile() {
    printf("\n=== 测试：结构体编译端到端测试 ===\n");
    
    const char *source = 
        "结构体 点 {\n"
        "    整数 x;\n"
        "    整数 y;\n"
        "}\n"
        "\n"
        "函数 主程序() {\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    // 1. 词法分析和解析
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_struct.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("[失败] 解析阶段有错误\n");
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    
    if (!program || program->struct_count != 1 || program->function_count != 1) {
        printf("[失败] 程序结构不正确\n");
        if (program) cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    printf("[通过] 解析阶段\n");
    
    // 2. 语义分析
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("[失败] 语义分析阶段有错误\n");
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    printf("[通过] 语义分析阶段\n");
    
    // 3. IR生成
    CnTargetTriple target = cn_support_target_triple_make(
        CN_TARGET_ARCH_X86_64,
        CN_TARGET_VENDOR_PC,
        CN_TARGET_OS_NONE,
        CN_TARGET_ABI_ELF);
    CnIrModule *ir_module = cn_ir_gen_program(program, target, CN_COMPILE_MODE_HOSTED);
    
    if (!ir_module) {
        printf("[失败] IR 生成失败\n");
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    printf("[通过] IR生成阶段\n");
    
    // 4. C代码生成（包含结构体定义）
    const char *output_file = "test_struct_compile.c";
    int result = cn_cgen_module_with_structs_to_file(ir_module, program, output_file);
    
    if (result != 0) {
        printf("[失败] C代码生成失败\n");
        cn_ir_module_free(ir_module);
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    printf("[通过] C代码生成阶段\n");
    
    // 5. 验证生成的C代码包含结构体定义
    FILE *f = fopen(output_file, "r");
    if (f) {
        char line[256];
        int found_struct = 0;
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "struct") && strstr(line, "点")) {
                found_struct = 1;
                break;
            }
        }
        fclose(f);
        
        if (!found_struct) {
            printf("[失败] 生成的C代码未包含结构体定义\n");
            cn_ir_module_free(ir_module);
            cn_sem_scope_free(global_scope);
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            cn_support_diagnostics_free(&diagnostics);
            return 1;
        }
        printf("[通过] 生成的C代码包含结构体定义\n");
    } else {
        printf("[失败] 无法读取生成的C文件\n");
        cn_ir_module_free(ir_module);
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    
    // 清理
    remove(output_file);
    cn_ir_module_free(ir_module);
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("[成功] 结构体编译测试通过\n");
    return 0;
}

int main() {
    printf("=== 结构体编译集成测试 ===\n");
    int result = test_struct_compile();
    if (result == 0) {
        printf("\nstruct_compile_test: 测试完成\n");
    }
    return result;
}
