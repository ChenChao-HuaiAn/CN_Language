#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/runtime/runtime.h"
#include "cnlang/support/process/process.h"

/*
 * 运行时库路径管理函数
 */

// 获取运行时库路径
const char* get_runtime_lib_path() {
    // 首先检查环境变量
    const char *env_path = getenv("CN_RUNTIME_PATH");
    if (env_path) {
        return env_path;
    }
    
    // 默认查找与可执行文件同目录下的 lib/ 子目录
    static char runtime_path[1024];
    
    // 获取可执行文件路径
    if (getenv("_")) {
        // 在某些系统上，"_" 环境变量保存了当前执行的程序路径
        const char *self_path = getenv("_");
        size_t len = strlen(self_path);
        
        // 查找最后一个斜杠的位置
        int last_slash = -1;
        for (int i = len - 1; i >= 0; i--) {
            if (self_path[i] == '/' || self_path[i] == '\\') {
                last_slash = i;
                break;
            }
        }
        
        if (last_slash != -1) {
            // 复制可执行文件所在目录路径
            strncpy(runtime_path, self_path, last_slash + 1);
            runtime_path[last_slash + 1] = '\0';
            
            // 添加 lib/cn_runtime.lib 或 lib/libcn_runtime.a 路径
            strcat(runtime_path, "lib/libcn_runtime.a");
            return runtime_path;
        }
    }
    
    // 如果无法确定可执行文件路径，则返回默认路径
    return "./lib/libcn_runtime.a";
}

// 获取运行时头文件路径
const char* get_runtime_header_path() {
    // 首先检查环境变量
    const char *env_path = getenv("CN_RUNTIME_HEADER_PATH");
    if (env_path) {
        return env_path;
    }
    
    // 默认查找与可执行文件同目录下的 include/ 子目录
    static char header_path[1024];
    
    // 获取可执行文件路径
    if (getenv("_")) {
        const char *self_path = getenv("_");
        size_t len = strlen(self_path);
        
        // 查找最后一个斜杠的位置
        int last_slash = -1;
        for (int i = len - 1; i >= 0; i--) {
            if (self_path[i] == '/' || self_path[i] == '\\') {
                last_slash = i;
                break;
            }
        }
        
        if (last_slash != -1) {
            // 复制可执行文件所在目录路径
            strncpy(header_path, self_path, last_slash + 1);
            header_path[last_slash + 1] = '\0';
            
            // 添加 include/cnrt.h 路径
            strcat(header_path, "include/cnrt.h");
            return header_path;
        }
    }
    
    // 如果无法确定可执行文件路径，则返回默认路径
    return "./include/cnrt.h";
}

// 读取整个源文件到内存缓冲区
static char *read_file_to_buffer(const char *filename, size_t *out_length)
{
    FILE *fp;
    char *buffer;
    long size;
    size_t length;

    fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }

    buffer = (char *)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    length = fread(buffer, 1, (size_t)size, fp);
    fclose(fp);

    buffer[length] = '\0';

    if (out_length) {
        *out_length = length;
    }

    return buffer;
}

// 打印函数列表和语句数
static void print_function_summary(const CnAstProgram *program)
{
    size_t i;

    if (!program) {
        return;
    }

    for (i = 0; i < program->function_count; ++i) {
        const CnAstFunctionDecl *fn = program->functions[i];
        const char *name = fn->name;
        size_t name_length = fn->name_length;
        size_t stmt_count = 0;

        if (fn->body) {
            stmt_count = fn->body->stmt_count;
        }

        printf("函数: %.*s，语句数: %zu\n",
               (int)name_length,
               name,
               stmt_count);
    }
}

// 打印诊断信息
static void print_diagnostics(const CnDiagnostics *diagnostics)
{
    size_t i;

    if (!diagnostics || diagnostics->count == 0) {
        return;
    }

    for (i = 0; i < diagnostics->count; ++i) {
        const CnDiagnostic *d = &diagnostics->items[i];
        const char *severity_str = (d->severity == CN_DIAG_SEVERITY_ERROR) ? "错误" : "警告";
        const char *filename = d->filename ? d->filename : "<未知文件>";
        const char *message = d->message ? d->message : "<无消息>";
        int line = d->line;
        int column = d->column;

        fprintf(stderr,
                "%s(%d): %s:%d:%d: %s\n",
                severity_str,
                (int)d->code,
                filename,
                line,
                column,
                message);
    }
}

// 检查诊断中是否存在错误
static bool diagnostics_has_error(const CnDiagnostics *diagnostics)
{
    size_t i;

    if (!diagnostics) {
        return false;
    }

    for (i = 0; i < diagnostics->count; ++i) {
        const CnDiagnostic *d = &diagnostics->items[i];
        if (d->severity == CN_DIAG_SEVERITY_ERROR) {
            return true;
        }
    }

    return false;
}

int main(int argc, char **argv)
{
    const char *filename;
    char *source;
    size_t source_length = 0;
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnSemScope *global_scope = NULL;
    CnDiagnostics diagnostics;
    bool ok;

    if (argc < 2) {
        fprintf(stderr, "用法: %s <源文件.cn> [选项]\n", argv[0]);
        fprintf(stderr, "选项:\n");
        fprintf(stderr, "  -o <文件名>    指定输出文件名 (默认: a.out)\n");
        fprintf(stderr, "  -c            仅生成 .c 文件，不编译为可执行文件\n");
        fprintf(stderr, "  --emit-c       生成 C 代码后保留 .c 文件\n");
        fprintf(stderr, "  --help         显示此帮助信息\n");
        return 1;
    }
    
    // 检查是否是帮助请求
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            fprintf(stderr, "CN Compiler (cnc) - CN Language 编译器\n\n");
            fprintf(stderr, "用法: %s <源文件.cn> [选项]\n\n", argv[0]);
            fprintf(stderr, "选项:\n");
            fprintf(stderr, "  -o <文件名>    指定输出文件名 (默认: a.out)\n");
            fprintf(stderr, "  -c            仅生成 .c 文件，不编译为可执行文件\n");
            fprintf(stderr, "  --emit-c       生成 C 代码后保留 .c 文件\n");
            fprintf(stderr, "  --help/-h      显示此帮助信息\n\n");
            fprintf(stderr, "环境变量:\n");
            fprintf(stderr, "  CN_RUNTIME_PATH        指定运行时库路径\n");
            fprintf(stderr, "  CN_RUNTIME_HEADER_PATH 指定运行时头文件路径\n\n");
            fprintf(stderr, "示例:\n");
            fprintf(stderr, "  %s hello.cn                    # 仅进行语法和语义检查\n", argv[0]);
            fprintf(stderr, "  %s hello.cn -o hello            # 编译并生成 hello 可执行文件\n", argv[0]);
            fprintf(stderr, "  %s hello.cn -c                  # 仅生成 hello.c 文件\n", argv[0]);
            fprintf(stderr, "  %s hello.cn --emit-c            # 生成 C 代码并保留 .c 文件\n", argv[0]);
            return 0;
        }
    }

    filename = argv[1];

    source = read_file_to_buffer(filename, &source_length);
    if (!source) {
        fprintf(stderr, "无法读取文件: %s\n", filename);
        return 1;
    }

    cn_support_diagnostics_init(&diagnostics);

    cn_frontend_lexer_init(&lexer, source, source_length, filename);
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "创建解析器失败\n");
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    ok = cn_frontend_parse_program(parser, &program);
    if (!ok || !program) {
        fprintf(stderr, "解析失败\n");
        print_diagnostics(&diagnostics);
        cn_support_diagnostics_free(&diagnostics);
        cn_frontend_parser_free(parser);
        free(source);
        return 1;
    }

    if (diagnostics_has_error(&diagnostics)) {
        fprintf(stderr, "解析失败\n");
        print_diagnostics(&diagnostics);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    global_scope = cn_sem_build_scopes(program, &diagnostics);
    if (!global_scope) {
        fprintf(stderr, "构建作用域失败\n");
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    if (!cn_sem_resolve_names(global_scope, program, &diagnostics)) {
        fprintf(stderr, "名称解析失败\n");
        print_diagnostics(&diagnostics);
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    if (!cn_sem_check_types(global_scope, program, &diagnostics)) {
        fprintf(stderr, "类型检查失败\n");
        print_diagnostics(&diagnostics);
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    print_function_summary(program);
    print_diagnostics(&diagnostics);

    // 检查是否需要进行编译和链接
    if (argc > 2) {
        // 检查参数以确定输出文件名
        const char *output_filename = "a.out"; // 默认输出文件名
        int compile_only = 0; // -c 参数标志
        int emit_c = 0; // --emit-c 参数标志
        
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                output_filename = argv[i + 1];
                i++; // 跳过下一个参数
            } else if (strcmp(argv[i], "-c") == 0) {
                compile_only = 1;
            } else if (strcmp(argv[i], "--emit-c") == 0) {
                emit_c = 1;
            }
        }
        
        // 检查是否需要生成 C 代码
        if (compile_only || emit_c) {
            // 引入 IR 生成和 C 代码生成功能
            #ifdef CNLANG_IR_ENABLED
            #include "cnlang/ir/ir.h"
            #include "cnlang/ir/irgen.h"
            #include "cnlang/backend/cgen.h"
            
            // 生成 IR
            CnIrModule *ir_module = cn_ir_gen_module(program, &diagnostics);
            if (!ir_module) {
                fprintf(stderr, "IR 生成失败\n");
                print_diagnostics(&diagnostics);
                cn_sem_scope_free(global_scope);
                cn_frontend_ast_program_free(program);
                cn_frontend_parser_free(parser);
                cn_support_diagnostics_free(&diagnostics);
                free(source);
                return 1;
            }
            
            // 生成 C 代码文件名
            char c_filename[1024];
            strcpy(c_filename, filename);
            // 替换 .cn 扩展名为 .c
            char *ext = strrchr(c_filename, '.');
            if (ext && strcmp(ext, ".cn") == 0) {
                strcpy(ext, ".c");
            } else {
                strcat(c_filename, ".c");
            }
            
            // 生成 C 代码
            if (!cn_cgen_write_to_file(ir_module, c_filename)) {
                fprintf(stderr, "C 代码生成失败\n");
                cn_ir_module_free(ir_module);
                cn_sem_scope_free(global_scope);
                cn_frontend_ast_program_free(program);
                cn_frontend_parser_free(parser);
                cn_support_diagnostics_free(&diagnostics);
                free(source);
                return 1;
            }
            
            printf("已生成 C 代码文件: %s\n", c_filename);
            
            // 如果不是仅生成 C 代码（即需要编译），则调用外部编译器
            if (!emit_c) {
                // 构建编译命令
                char compile_cmd[2048];
                const char *runtime_lib_path = get_runtime_lib_path();
                
                // 使用自动检测的编译器
                const char *compiler = cn_support_detect_c_compiler();
                
                #ifdef _WIN32
                // Windows 平台处理
                if (strcmp(compiler, "cl") == 0) {
                    snprintf(compile_cmd, sizeof(compile_cmd), "%s /Fe:%s %s %s", 
                             compiler, output_filename, c_filename, runtime_lib_path);
                } else {
                    snprintf(compile_cmd, sizeof(compile_cmd), "%s -o %s %s %s", 
                             compiler, output_filename, c_filename, runtime_lib_path);
                }
                #else
                snprintf(compile_cmd, sizeof(compile_cmd), "%s -o %s %s %s", 
                         compiler, output_filename, c_filename, runtime_lib_path);
                #endif
                
                printf("正在执行编译命令: %s\n", compile_cmd);
                int result;
                bool success = cn_support_run_command(compile_cmd, &result);
                if (!success || result != 0) {
                    fprintf(stderr, "编译失败，退出码: %d\n", result);
                    cn_ir_module_free(ir_module);
                    cn_sem_scope_free(global_scope);
                    cn_frontend_ast_program_free(program);
                    cn_frontend_parser_free(parser);
                    cn_support_diagnostics_free(&diagnostics);
                    free(source);
                    return 1;
                }
                
                printf("编译成功! 输出文件: %s\n", output_filename);
            }
            
            cn_ir_module_free(ir_module);
            #else
            fprintf(stderr, "错误: IR 和 C 代码生成功能未启用\n");
            #endif
        }
    }
    
    print_function_summary(program);
    print_diagnostics(&diagnostics);
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    free(source);

    return 0;
}
