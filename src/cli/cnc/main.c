#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/preprocessor.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/runtime/runtime.h"
#include "cnlang/support/process/process.h"
#include "cnlang/support/config.h"
#include "cnlang/support/perf.h"
#include "cnlang/support/memory_profiler.h"
#include "cnlang/support/memory_estimator.h"
#include "cnlang/ir/ir.h"
#include "cnlang/ir/irgen.h"
#include "cnlang/ir/pass.h"
#include "cnlang/backend/cgen.h"

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
    
#ifdef _WIN32
    char self_path[MAX_PATH];
    if (GetModuleFileNameA(NULL, self_path, MAX_PATH)) {
        char *last_slash = strrchr(self_path, '\\');
        if (last_slash) {
            *last_slash = '\0';
            snprintf(runtime_path, sizeof(runtime_path), "%s\\lib\\libcn_runtime.a", self_path);
            if (fopen(runtime_path, "r")) return runtime_path;
            
            // 尝试 build 目录下的路径
            snprintf(runtime_path, sizeof(runtime_path), "%s\\runtime\\libcn_runtime.a", self_path);
            if (fopen(runtime_path, "r")) return runtime_path;
        }
    }
#else
    // 原有的探测逻辑...
#endif

    // 如果无法确定可执行文件路径，则尝试几个常用的相对路径
    if (fopen("./lib/libcn_runtime.a", "r")) return "./lib/libcn_runtime.a";
    if (fopen("../lib/libcn_runtime.a", "r")) return "../lib/libcn_runtime.a";
    if (fopen("../../lib/libcn_runtime.a", "r")) return "../../lib/libcn_runtime.a";
    if (fopen("./src/runtime/libcn_runtime.a", "r")) return "./src/runtime/libcn_runtime.a";
    if (fopen("../src/runtime/libcn_runtime.a", "r")) return "../src/runtime/libcn_runtime.a";
    if (fopen("./build/src/runtime/libcn_runtime.a", "r")) return "./build/src/runtime/libcn_runtime.a";
    if (fopen("../build/src/runtime/libcn_runtime.a", "r")) return "../build/src/runtime/libcn_runtime.a";
    if (fopen("./build/src/runtime/libcn_runtime.a", "r")) return "./build/src/runtime/libcn_runtime.a";
    if (fopen("../build/src/runtime/libcn_runtime.a", "r")) return "../build/src/runtime/libcn_runtime.a";

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
    
#ifdef _WIN32
    char self_path[MAX_PATH];
    if (GetModuleFileNameA(NULL, self_path, MAX_PATH)) {
        char *last_slash = strrchr(self_path, '\\');
        if (last_slash) {
            *last_slash = '\0';
            snprintf(header_path, sizeof(header_path), "%s\\include\\cnrt.h", self_path);
            if (fopen(header_path, "r")) return header_path;
            
            // 尝试从 build 目录向上的路径
            snprintf(header_path, sizeof(header_path), "%s\\..\\..\\include\\cnrt.h", self_path);
            if (fopen(header_path, "r")) return header_path;
        }
    }
#else
    // 原有的探测逻辑...
#endif

    // 如果无法确定可执行文件路径，则尝试几个常用的相对路径
    if (fopen("./include/cnrt.h", "r")) return "./include/cnrt.h";
    if (fopen("../include/cnrt.h", "r")) return "../include/cnrt.h";
    if (fopen("../../include/cnrt.h", "r")) return "../../include/cnrt.h";

    return "./include/cnrt.h";
}

// 获取运行时头文件所在目录
const char* get_runtime_include_dir() {
    static char include_dir[1024];
    const char *header_path = get_runtime_header_path();
    strncpy(include_dir, header_path, sizeof(include_dir) - 1);
    include_dir[sizeof(include_dir) - 1] = '\0';
    
    char *last_slash = strrchr(include_dir, '/');
    char *last_backslash = strrchr(include_dir, '\\');
    char *last = (last_slash > last_backslash) ? last_slash : last_backslash;
    
    if (last) {
        *last = '\0';
    } else {
        return ".";
    }
    return include_dir;
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
    CnPreprocessor preprocessor;
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnSemScope *global_scope = NULL;
    CnDiagnostics diagnostics;
    CnPerfStats perf_stats;
    CnMemStats mem_stats;
    bool ok;
    CnTargetTriple target_triple;

    /* 默认目标：后续可根据配置/命令行调整 */
    target_triple = cn_support_target_triple_make(
        CN_TARGET_ARCH_X86_64,
        CN_TARGET_VENDOR_PC,
        CN_TARGET_OS_NONE,
        CN_TARGET_ABI_ELF);

    if (argc < 2) {
        fprintf(stderr, "用法: %s <源文件.cn> [选项]\n", argv[0]);
        fprintf(stderr, "选项:\n");
        fprintf(stderr, "  -o <文件名>    指定输出文件名\n");
        fprintf(stderr, "  -c            仅生成 .c 文件，不编译为可执行文件\n");
        fprintf(stderr, "  -S            仅生成 IR 描述，输出到标准输出或文件 (-o)\n");
        fprintf(stderr, "  --emit-c       生成 C 代码后保留 .c 文件\n");
        fprintf(stderr, "  --cc <编译器>  指定外部 C 编译器路径\n");
        fprintf(stderr, "  -g            生成调试信息\n");
        fprintf(stderr, "  -O<n>         设置优化级别 (0, 1, 2, 3)\n");
        fprintf(stderr, "  --target=<三元组>  指定编译目标 (例如 --target=x86_64-elf)\n");
        fprintf(stderr, "  --freestanding  启用 freestanding 编译模式（最小运行时/OS 开发场景）\n");
        fprintf(stderr, "  --perf         启用编译性能分析\n");
        fprintf(stderr, "  --perf-output=<文件>  指定性能分析输出文件（支持 .json 或 .csv 格式）\n");
        fprintf(stderr, "  --mem-profile  启用内存占用分析\n");
        fprintf(stderr, "  --mem-output=<文件>  指定内存分析输出文件（支持 .json 或 .csv 格式）\n");
        fprintf(stderr, "  --help         显示此帮助信息\n");
        return 1;
    }
    
    // 默认参数
    const char *output_filename = NULL;
    bool compile_only = false;
    bool emit_c = false;
    bool run_pipeline = false;
    bool dump_ir = false;
    const char *cc_override = NULL;
    bool debug_info = false;
    const char *opt_level = NULL;
    bool freestanding_mode = false;
    bool enable_perf = false;
    const char *perf_output = NULL;
    bool enable_mem_profile = false;
    const char *mem_output = NULL;

    // 检查是否是帮助请求
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            fprintf(stderr, "CN Compiler (cnc) - CN Language 编译器\n\n");
            fprintf(stderr, "用法: %s <源文件.cn> [选项]\n\n", argv[0]);
            fprintf(stderr, "选项:\n");
            fprintf(stderr, "  -o <文件名>    指定输出文件名\n");
            fprintf(stderr, "  -c            仅生成 .c 文件，不编译为可执行文件\n");
            fprintf(stderr, "  -S            仅生成 IR 描述，输出到标准输出或文件 (-o)\n");
            fprintf(stderr, "  --emit-c       生成 C 代码后保留 .c 文件\n");
            fprintf(stderr, "  --cc <编译器>  指定外部 C 编译器路径\n");
            fprintf(stderr, "  -g            生成调试信息\n");
            fprintf(stderr, "  -O<n>         设置优化级别 (0, 1, 2, 3)\n");
            fprintf(stderr, "  --target=<三元组>  指定编译目标 (例如 --target=x86_64-elf)\n");
            fprintf(stderr, "  --freestanding  启用 freestanding 编译模式（最小运行时/OS 开发场景）\n");
            fprintf(stderr, "  --perf         启用编译性能分析\n");
            fprintf(stderr, "  --perf-output=<文件>  指定性能分析输出文件（支持 .json 或 .csv 格式）\n");
            fprintf(stderr, "  --mem-profile  启用内存占用分析\n");
            fprintf(stderr, "  --mem-output=<文件>  指定内存分析输出文件（支持 .json 或 .csv 格式）\n");
            fprintf(stderr, "  --help/-h      显示此帮助信息\n\n");
            fprintf(stderr, "环境变量:\n");
            fprintf(stderr, "  CN_RUNTIME_PATH        指定运行时库路径\n");
            fprintf(stderr, "  CN_RUNTIME_HEADER_PATH 指定运行时头文件路径\n\n");
            fprintf(stderr, "示例:\n");
            fprintf(stderr, "  %s hello.cn                    # 仅进行语法和语义检查\n", argv[0]);
            fprintf(stderr, "  %s hello.cn -o hello            # 编译并生成 hello 可执行文件\n", argv[0]);
            fprintf(stderr, "  %s hello.cn -S                  # 打印 IR 描述到 stdout\n", argv[0]);
            fprintf(stderr, "  %s hello.cn -O2 -o hello        # 开启优化并编译\n", argv[0]);
            return 0;
        }
    }

    filename = argv[1];

    // 解析其余参数
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_filename = argv[++i];
            run_pipeline = true;
        } else if (strcmp(argv[i], "-c") == 0) {
            compile_only = true;
            run_pipeline = true;
        } else if (strcmp(argv[i], "-S") == 0) {
            dump_ir = true;
            run_pipeline = true;
        } else if (strcmp(argv[i], "--emit-c") == 0) {
            emit_c = true;
            run_pipeline = true;
        } else if (strcmp(argv[i], "--cc") == 0 && i + 1 < argc) {
            cc_override = argv[++i];
            run_pipeline = true;
        } else if (strcmp(argv[i], "-g") == 0) {
            debug_info = true;
            run_pipeline = true;
        } else if (strlen(argv[i]) == 3 && argv[i][0] == '-' && argv[i][1] == 'O' && isdigit(argv[i][2])) {
            opt_level = argv[i] + 2;
            run_pipeline = true;
        } else if (strncmp(argv[i], "--target=", 9) == 0) {
            const char *triple_str = argv[i] + 9;
            CnTargetTriple parsed_triple;
            bool target_ok = cn_support_target_triple_parse(triple_str, &parsed_triple);
            if (!target_ok) {
                fprintf(stderr, "无效的目标三元组: %s\n", triple_str);
                return 1;
            }
            target_triple = parsed_triple;
            run_pipeline = true;
        } else if (strcmp(argv[i], "--freestanding") == 0) {
            freestanding_mode = true;
            run_pipeline = true;
        } else if (strcmp(argv[i], "--perf") == 0) {
            enable_perf = true;
        } else if (strncmp(argv[i], "--perf-output=", 14) == 0) {
            perf_output = argv[i] + 14;
            enable_perf = true;
        } else if (strcmp(argv[i], "--mem-profile") == 0) {
            enable_mem_profile = true;
        } else if (strncmp(argv[i], "--mem-output=", 13) == 0) {
            mem_output = argv[i] + 13;
            enable_mem_profile = true;
        }
    }

    source = read_file_to_buffer(filename, &source_length);
    if (!source) {
        fprintf(stderr, "无法读取文件: %s\n", filename);
        return 1;
    }

    /* 初始化性能统计 */
    cn_perf_stats_init(&perf_stats, filename, source_length);
    cn_perf_stats_set_enabled(&perf_stats, enable_perf);

    /* 初始化内存统计 */
    cn_mem_stats_init(&mem_stats);
    cn_mem_stats_set_enabled(&mem_stats, enable_mem_profile);

    /* 开始总计时 */
    cn_perf_start(&perf_stats, CN_PERF_PHASE_TOTAL);

    cn_support_diagnostics_init(&diagnostics);

    /* 预处理阶段 */
    cn_perf_start(&perf_stats, CN_PERF_PHASE_LEXER);
    cn_frontend_preprocessor_init(&preprocessor, source, source_length, filename);
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    if (!cn_frontend_preprocessor_process(&preprocessor)) {
        cn_perf_end(&perf_stats, CN_PERF_PHASE_LEXER);
        fprintf(stderr, "预处理失败\n");
        print_diagnostics(&diagnostics);
        cn_frontend_preprocessor_free(&preprocessor);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }
    
    /* 词法分析 - 使用预处理后的输出 */
    cn_frontend_lexer_init(&lexer, preprocessor.output, preprocessor.output_length, filename);
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    cn_perf_end(&perf_stats, CN_PERF_PHASE_LEXER);

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "创建解析器失败\n");
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    /* 语法分析 */
    cn_perf_start(&perf_stats, CN_PERF_PHASE_PARSER);
    ok = cn_frontend_parse_program(parser, &program);
    cn_perf_end(&perf_stats, CN_PERF_PHASE_PARSER);
    if (!ok || !program) {
        fprintf(stderr, "解析失败\n");
        print_diagnostics(&diagnostics);
        cn_support_diagnostics_free(&diagnostics);
        cn_frontend_parser_free(parser);
        cn_frontend_preprocessor_free(&preprocessor);
        free(source);
        return 1;
    }

    if (diagnostics_has_error(&diagnostics)) {
        fprintf(stderr, "解析失败\n");
        print_diagnostics(&diagnostics);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_frontend_preprocessor_free(&preprocessor);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    /* 语义分析 - 作用域构建 */
    cn_perf_start(&perf_stats, CN_PERF_PHASE_SEMANTIC);
    cn_perf_start(&perf_stats, CN_PERF_PHASE_SEMANTIC_SCOPE);
    global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_perf_end(&perf_stats, CN_PERF_PHASE_SEMANTIC_SCOPE);
    if (!global_scope) {
        fprintf(stderr, "构建作用域失败\n");
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_frontend_preprocessor_free(&preprocessor);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    /* 语义分析 - 名称解析 */
    cn_perf_start(&perf_stats, CN_PERF_PHASE_SEMANTIC_RESOLVE);
    if (!cn_sem_resolve_names(global_scope, program, &diagnostics)) {
        cn_perf_end(&perf_stats, CN_PERF_PHASE_SEMANTIC_RESOLVE);
        cn_perf_end(&perf_stats, CN_PERF_PHASE_SEMANTIC);
        fprintf(stderr, "名称解析失败\n");
        cn_perf_end(&perf_stats, CN_PERF_PHASE_SEMANTIC_RESOLVE);
        cn_perf_end(&perf_stats, CN_PERF_PHASE_SEMANTIC);
        print_diagnostics(&diagnostics);
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_frontend_preprocessor_free(&preprocessor);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }
    cn_perf_end(&perf_stats, CN_PERF_PHASE_SEMANTIC_RESOLVE);

    /* 语义分析 - 类型检查 */
    cn_perf_start(&perf_stats, CN_PERF_PHASE_SEMANTIC_TYPECHECK);
    if (!cn_sem_check_types(global_scope, program, &diagnostics)) {
        cn_perf_end(&perf_stats, CN_PERF_PHASE_SEMANTIC_TYPECHECK);
        cn_perf_end(&perf_stats, CN_PERF_PHASE_SEMANTIC);
        fprintf(stderr, "类型检查失败\n");
        cn_perf_end(&perf_stats, CN_PERF_PHASE_SEMANTIC_TYPECHECK);
        cn_perf_end(&perf_stats, CN_PERF_PHASE_SEMANTIC);
        print_diagnostics(&diagnostics);
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_frontend_preprocessor_free(&preprocessor);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }
    cn_perf_end(&perf_stats, CN_PERF_PHASE_SEMANTIC_TYPECHECK);
    cn_perf_end(&perf_stats, CN_PERF_PHASE_SEMANTIC);

    // Freestanding 模式检查
    if (freestanding_mode) {
        if (!cn_sem_check_freestanding(program, &diagnostics, true)) {
            fprintf(stderr, "Freestanding 模式检查失败\n");
            print_diagnostics(&diagnostics);
            cn_sem_scope_free(global_scope);
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            cn_frontend_preprocessor_free(&preprocessor);
            cn_support_diagnostics_free(&diagnostics);
            free(source);
            return 1;
        }
    }

    print_function_summary(program);
    print_diagnostics(&diagnostics);

    // 检查是否需要进行编译和链接
    if (run_pipeline || (argc > 2 && !output_filename && !compile_only && !emit_c && !dump_ir)) {
        if (!output_filename && !compile_only && !dump_ir) {
            output_filename = "a.out";
        }

        /* IR 生成 */
        cn_perf_start(&perf_stats, CN_PERF_PHASE_IR_GEN);
        CnIrModule *ir_module = cn_ir_gen_program(program, global_scope, target_triple, freestanding_mode ? CN_COMPILE_MODE_FREESTANDING : CN_COMPILE_MODE_HOSTED);
        cn_perf_end(&perf_stats, CN_PERF_PHASE_IR_GEN);
        if (!ir_module) {
            fprintf(stderr, "IR 生成失败\n");
            goto cleanup;
        }

        /* IR 优化 */
        cn_perf_start(&perf_stats, CN_PERF_PHASE_IR_OPT);
        cn_ir_run_default_passes(ir_module);
        cn_perf_end(&perf_stats, CN_PERF_PHASE_IR_OPT);

        // 如果只是打印 IR
        if (dump_ir) {
            if (output_filename) {
                FILE *f = fopen(output_filename, "w");
                if (f) {
                    cn_ir_dump_module_to_file(ir_module, f);
                    fclose(f);
                    printf("IR 已导出到: %s\n", output_filename);
                } else {
                    fprintf(stderr, "无法打开文件以导出 IR: %s\n", output_filename);
                }
            } else {
                cn_ir_dump_module(ir_module);
            }
            
            /* 在释放 IR 前统计内存占用 */
            if (enable_mem_profile) {
                size_t ir_size = cn_mem_estimate_ir(ir_module);
                cn_mem_stats_record_alloc(&mem_stats, CN_MEM_CATEGORY_IR, ir_size);
            }
            
            cn_ir_module_free(ir_module);
            goto cleanup;
        }

        // 生成 C 代码文件名
        char c_filename[1024];
        strncpy(c_filename, filename, sizeof(c_filename) - 1);
        c_filename[sizeof(c_filename) - 1] = '\0';
        char *ext = strrchr(c_filename, '.');
        if (ext && strcmp(ext, ".cn") == 0) {
            strcpy(ext, ".c");
        } else {
            strcat(c_filename, ".c");
        }

        /* 代码生成 */
        cn_perf_start(&perf_stats, CN_PERF_PHASE_CODEGEN);
        if (cn_cgen_module_with_structs_to_file(ir_module, program, c_filename) != 0) {
            cn_perf_end(&perf_stats, CN_PERF_PHASE_CODEGEN);
            fprintf(stderr, "C 代码生成失败\n");
            cn_ir_module_free(ir_module);
            goto cleanup;
        }
        cn_perf_end(&perf_stats, CN_PERF_PHASE_CODEGEN);

        if (emit_c || compile_only) {
            printf("已生成 C 代码文件: %s\n", c_filename);
        }

        // 如果不是仅生成 C 代码，则调用外部编译器
        if (!compile_only) {
            char compile_cmd[4096];
            const char *runtime_lib_path = freestanding_mode ? NULL : get_runtime_lib_path();
            const char *runtime_include_dir = get_runtime_include_dir();
            const char *compiler = cc_override ? cc_override : cn_support_detect_c_compiler();

            char extra_flags[256] = "";
            if (debug_info) {
                strcat(extra_flags, (strcmp(compiler, "cl") == 0) ? " /Zi" : " -g");
            }
            if (opt_level) {
                if (strcmp(compiler, "cl") == 0) {
                    char buf[16];
                    snprintf(buf, sizeof(buf), " /O%s", opt_level);
                    strcat(extra_flags, buf);
                } else {
                    char buf[16];
                    snprintf(buf, sizeof(buf), " -O%s", opt_level);
                    strcat(extra_flags, buf);
                }
            }
            if (freestanding_mode) {
                if (strcmp(compiler, "cl") != 0) {
                    strcat(extra_flags, " -ffreestanding -nostdlib");
                }
                /* freestanding 模式下不链接宿主 OS 运行时库 */
            }

            #ifdef _WIN32
            if (strcmp(compiler, "cl") == 0) {
                if (freestanding_mode) {
                    snprintf(compile_cmd, sizeof(compile_cmd), "%s%s /I%s /Fe:%s %s",
                             compiler, extra_flags, runtime_include_dir, output_filename ? output_filename : "a.exe", 
                             c_filename);
                } else {
                    snprintf(compile_cmd, sizeof(compile_cmd), "%s%s /I%s /Fe:%s %s %s",
                             compiler, extra_flags, runtime_include_dir, output_filename ? output_filename : "a.exe", 
                             c_filename, runtime_lib_path);
                }
            } else {
                if (freestanding_mode) {
                    snprintf(compile_cmd, sizeof(compile_cmd), "%s%s -I%s -o %s %s",
                             compiler, extra_flags, runtime_include_dir, output_filename ? output_filename : "a.out", 
                             c_filename);
                } else {
                    snprintf(compile_cmd, sizeof(compile_cmd), "%s%s -I%s -o %s %s %s",
                             compiler, extra_flags, runtime_include_dir, output_filename ? output_filename : "a.out", 
                             c_filename, runtime_lib_path);
                }
            }
            #else
            if (freestanding_mode) {
                snprintf(compile_cmd, sizeof(compile_cmd), "%s%s -I%s -o %s %s",
                         compiler, extra_flags, runtime_include_dir, output_filename ? output_filename : "a.out", 
                         c_filename);
            } else {
                snprintf(compile_cmd, sizeof(compile_cmd), "%s%s -I%s -o %s %s %s",
                         compiler, extra_flags, runtime_include_dir, output_filename ? output_filename : "a.out", 
                         c_filename, runtime_lib_path);
            }
            #endif

            /* 后端编译 */
            cn_perf_start(&perf_stats, CN_PERF_PHASE_BACKEND_COMPILE);
            printf("正在执行编译命令: %s\n", compile_cmd);
            int result;
            bool success = cn_support_run_command(compile_cmd, &result);
            cn_perf_end(&perf_stats, CN_PERF_PHASE_BACKEND_COMPILE);
            if (!success || result != 0) {
                fprintf(stderr, "编译失败，退出码: %d\n", result);
                cn_ir_module_free(ir_module);
                goto cleanup;
            }

            printf("编译成功! 输出文件: %s\n", output_filename ? output_filename : "a.out");

            // 如果没有要求保留 C 文件，则删除它
            if (!emit_c) {
                remove(c_filename);
            }
        }

        /* 在释放 IR 前统计内存占用 */
        if (enable_mem_profile) {
            size_t ir_size = cn_mem_estimate_ir(ir_module);
            cn_mem_stats_record_alloc(&mem_stats, CN_MEM_CATEGORY_IR, ir_size);
        }

        cn_ir_module_free(ir_module);
    }

    /* 结束总计时 */
    cn_perf_end(&perf_stats, CN_PERF_PHASE_TOTAL);

    /* 输出性能统计 */
    if (enable_perf) {
        if (perf_output) {
            /* 根据文件扩展名判断输出格式 */
            size_t len = strlen(perf_output);
            if (len > 5 && strcmp(perf_output + len - 5, ".json") == 0) {
                if (cn_perf_export_json(&perf_stats, perf_output)) {
                    printf("\n性能分析结果已导出到: %s\n", perf_output);
                } else {
                    fprintf(stderr, "\n无法导出性能分析结果到: %s\n", perf_output);
                }
            } else if (len > 4 && strcmp(perf_output + len - 4, ".csv") == 0) {
                if (cn_perf_export_csv(&perf_stats, perf_output)) {
                    printf("\n性能分析结果已导出到: %s\n", perf_output);
                } else {
                    fprintf(stderr, "\n无法导出性能分析结果到: %s\n", perf_output);
                }
            } else {
                fprintf(stderr, "\n不支持的性能分析输出格式: %s（仅支持 .json 或 .csv）\n", perf_output);
            }
        }
        /* 总是打印到控制台 */
        cn_perf_print_stats(&perf_stats, stdout);
    }

    /* 输出内存统计 */
    if (enable_mem_profile) {
        /* 估算各数据结构的内存占用 */
        size_t ast_size = cn_mem_estimate_ast(program);
        size_t symbol_size = cn_mem_estimate_symbol_table(global_scope);
        size_t diag_size = cn_mem_estimate_diagnostics(&diagnostics);
        
        /* 记录到统计中 */
        cn_mem_stats_record_alloc(&mem_stats, CN_MEM_CATEGORY_AST, ast_size);
        cn_mem_stats_record_alloc(&mem_stats, CN_MEM_CATEGORY_SYMBOL, symbol_size);
        cn_mem_stats_record_alloc(&mem_stats, CN_MEM_CATEGORY_DIAGNOSTICS, diag_size);
        
        /* IR 占用需要在 IR 生成后统计，这里可能已经释放了 */
        /* 如果需要统计 IR，应在 IR 生成后、释放前调用 cn_mem_estimate_ir */
        
        if (mem_output) {
            /* 根据文件扩展名判断输出格式 */
            size_t len = strlen(mem_output);
            if (len > 5 && strcmp(mem_output + len - 5, ".json") == 0) {
                if (cn_mem_stats_export_json(&mem_stats, mem_output)) {
                    printf("\n内存分析结果已导出到: %s\n", mem_output);
                } else {
                    fprintf(stderr, "\n无法导出内存分析结果到: %s\n", mem_output);
                }
            } else if (len > 4 && strcmp(mem_output + len - 4, ".csv") == 0) {
                if (cn_mem_stats_export_csv(&mem_stats, mem_output)) {
                    printf("\n内存分析结果已导出到: %s\n", mem_output);
                } else {
                    fprintf(stderr, "\n无法导出内存分析结果到: %s\n", mem_output);
                }
            } else {
                fprintf(stderr, "\n不支持的内存分析输出格式: %s（仅支持 .json 或 .csv）\n", mem_output);
            }
        }
        /* 总是打印到控制台 */
        cn_mem_stats_print(&mem_stats, stdout);
    }

cleanup:
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    free(source);

    return 0;
}
