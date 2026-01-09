/**
 * @file CN_command_executor.c
 * @brief 命令行命令执行器实现
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 * 
 * 本文件实现了命令行命令执行器的具体功能，包括命令分发和执行。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 */

#include "CN_command_executor.h"
#include "../../core/CN_compiler_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 内部数据结构 */
typedef struct {
    Stru_CommandExecutorInterface_t interface;  /**< 公共接口 */
    char* error_message;                        /**< 错误信息缓冲区 */
} Stru_CommandExecutorImpl_t;

/* 内部函数声明 */
static Eum_ExecutionResult_t F_execute_impl(Stru_CommandExecutorInterface_t* self, 
                                           const Stru_ParseResult_t* parse_result);
static Eum_ExecutionResult_t F_execute_help_impl(Stru_CommandExecutorInterface_t* self, 
                                                const Stru_ParseResult_t* parse_result);
static Eum_ExecutionResult_t F_execute_version_impl(Stru_CommandExecutorInterface_t* self, 
                                                   const Stru_ParseResult_t* parse_result);
static Eum_ExecutionResult_t F_execute_compile_impl(Stru_CommandExecutorInterface_t* self, 
                                                   const Stru_ParseResult_t* parse_result);
static Eum_ExecutionResult_t F_execute_run_impl(Stru_CommandExecutorInterface_t* self, 
                                               const Stru_ParseResult_t* parse_result);
static Eum_ExecutionResult_t F_execute_debug_impl(Stru_CommandExecutorInterface_t* self, 
                                                 const Stru_ParseResult_t* parse_result);
static const char* F_get_error_message_impl(Stru_CommandExecutorInterface_t* self);
static void F_destroy_impl(Stru_CommandExecutorInterface_t* self);

/* 辅助函数声明 */
static void F_set_error_message(Stru_CommandExecutorImpl_t* impl, const char* message);
static const char* F_get_first_value_argument(const Stru_ParseResult_t* parse_result);

/**
 * @brief 创建命令执行器实例
 * 
 * 工厂函数，创建并初始化命令执行器接口的实例。
 * 
 * @return Stru_CommandExecutorInterface_t* 命令执行器接口指针，失败返回NULL
 */
Stru_CommandExecutorInterface_t* F_create_command_executor(void)
{
    Stru_CommandExecutorImpl_t* impl = (Stru_CommandExecutorImpl_t*)malloc(sizeof(Stru_CommandExecutorImpl_t));
    if (impl == NULL) {
        return NULL;
    }
    
    /* 初始化接口方法 */
    impl->interface.execute = F_execute_impl;
    impl->interface.execute_help = F_execute_help_impl;
    impl->interface.execute_version = F_execute_version_impl;
    impl->interface.execute_compile = F_execute_compile_impl;
    impl->interface.execute_run = F_execute_run_impl;
    impl->interface.execute_debug = F_execute_debug_impl;
    impl->interface.get_error_message = F_get_error_message_impl;
    impl->interface.destroy = F_destroy_impl;
    
    /* 初始化内部状态 */
    impl->error_message = NULL;
    
    return (Stru_CommandExecutorInterface_t*)impl;
}

/**
 * @brief 执行命令
 * 
 * 根据解析结果执行相应的命令。
 * 
 * @param self 接口指针
 * @param parse_result 解析结果
 * @return Eum_ExecutionResult_t 执行结果
 */
static Eum_ExecutionResult_t F_execute_impl(Stru_CommandExecutorInterface_t* self, 
                                           const Stru_ParseResult_t* parse_result)
{
    Stru_CommandExecutorImpl_t* impl = (Stru_CommandExecutorImpl_t*)self;
    
    if (parse_result == NULL) {
        F_set_error_message(impl, "解析结果为空");
        return Eum_EXEC_RESULT_SYNTAX_ERROR;
    }
    
    /* 获取命令 */
    const char* command = parse_result->command;
    if (command == NULL) {
        /* 没有命令，显示帮助 */
        return F_execute_help_impl(self, parse_result);
    }
    
    /* 根据命令分发执行 */
    if (strcmp(command, "help") == 0) {
        return F_execute_help_impl(self, parse_result);
    } else if (strcmp(command, "version") == 0) {
        return F_execute_version_impl(self, parse_result);
    } else if (strcmp(command, "compile") == 0) {
        return F_execute_compile_impl(self, parse_result);
    } else if (strcmp(command, "run") == 0) {
        return F_execute_run_impl(self, parse_result);
    } else if (strcmp(command, "debug") == 0) {
        return F_execute_debug_impl(self, parse_result);
    } else {
        F_set_error_message(impl, "未知命令");
        return Eum_EXEC_RESULT_UNKNOWN_CMD;
    }
}

/**
 * @brief 执行帮助命令
 * 
 * 显示帮助信息。
 * 
 * @param self 接口指针
 * @param parse_result 解析结果
 * @return Eum_ExecutionResult_t 执行结果
 */
static Eum_ExecutionResult_t F_execute_help_impl(Stru_CommandExecutorInterface_t* self, 
                                                const Stru_ParseResult_t* parse_result)
{
    (void)self; /* 未使用参数 */
    (void)parse_result; /* 未使用参数 */
    
    printf("CN_Language 命令行工具\n");
    printf("版本: 1.0.0\n");
    printf("\n");
    printf("用法: cn <命令> [选项] [参数]\n");
    printf("\n");
    printf("可用命令:\n");
    printf("  help                   显示此帮助信息\n");
    printf("  version                显示版本信息\n");
    printf("  compile <文件>         编译CN源代码文件\n");
    printf("  run <文件>             运行CN程序\n");
    printf("  debug <文件>           调试CN程序\n");
    printf("\n");
    printf("选项:\n");
    printf("  -h, --help             显示帮助信息\n");
    printf("  -v, --version          显示版本信息\n");
    printf("  -o, --output <文件>    指定输出文件（编译命令）\n");
    printf("  -V, --verbose          显示详细输出\n");
    printf("\n");
    printf("示例:\n");
    printf("  cn help\n");
    printf("  cn version\n");
    printf("  cn compile hello.cn\n");
    printf("  cn compile hello.cn -o hello.exe\n");
    printf("  cn run hello.cn\n");
    printf("  cn debug hello.cn\n");
    
    return Eum_EXEC_RESULT_SUCCESS;
}

/**
 * @brief 执行版本命令
 * 
 * 显示版本信息。
 * 
 * @param self 接口指针
 * @param parse_result 解析结果
 * @return Eum_ExecutionResult_t 执行结果
 */
static Eum_ExecutionResult_t F_execute_version_impl(Stru_CommandExecutorInterface_t* self, 
                                                   const Stru_ParseResult_t* parse_result)
{
    (void)self; /* 未使用参数 */
    (void)parse_result; /* 未使用参数 */
    
    printf("CN_Language 版本 1.0.0\n");
    printf("架构版本: 2.0.0\n");
    printf("构建日期: 2026-01-09\n");
    printf("许可证: MIT\n");
    printf("项目仓库: git@gitcode.com:ChenChao_GitCode/CN_Language.git\n");
    
    return Eum_EXEC_RESULT_SUCCESS;
}

/**
 * @brief 执行编译命令
 * 
 * 编译CN源代码文件。
 * 
 * @param self 接口指针
 * @param parse_result 解析结果
 * @return Eum_ExecutionResult_t 执行结果
 */
static Eum_ExecutionResult_t F_execute_compile_impl(Stru_CommandExecutorInterface_t* self, 
                                                   const Stru_ParseResult_t* parse_result)
{
    Stru_CommandExecutorImpl_t* impl = (Stru_CommandExecutorImpl_t*)self;
    
    /* 获取输入文件 */
    const char* input_file = F_get_first_value_argument(parse_result);
    if (input_file == NULL) {
        F_set_error_message(impl, "编译命令需要指定输入文件");
        return Eum_EXEC_RESULT_SYNTAX_ERROR;
    }
    
    /* 获取输出文件（如果有） */
    const char* output_file = NULL;
    if (parse_result != NULL) {
        /* 检查 -o 或 --output 选项 */
        for (size_t i = 0; i < parse_result->count; i++) {
            const Stru_Argument_t* arg = &parse_result->arguments[i];
            if (arg->type == Eum_ARG_TYPE_OPTION) {
                if (strcmp(arg->name, "-o") == 0 || strcmp(arg->name, "--output") == 0) {
                    output_file = arg->value;
                    break;
                }
            }
        }
    }
    
    /* 检查详细输出标志 */
    bool verbose = false;
    if (parse_result != NULL) {
        for (size_t i = 0; i < parse_result->count; i++) {
            const Stru_Argument_t* arg = &parse_result->arguments[i];
            if (arg->type == Eum_ARG_TYPE_FLAG) {
                if (strcmp(arg->name, "-V") == 0 || strcmp(arg->name, "--verbose") == 0) {
                    verbose = true;
                    break;
                }
            }
        }
    }
    
    /* 执行编译 */
    if (verbose) {
        printf("开始编译: %s\n", input_file);
        if (output_file != NULL) {
            printf("输出文件: %s\n", output_file);
        }
    }
    
    /* 创建编译器实例 */
    Stru_CompilerInterface_t* compiler = F_create_compiler_interface();
    if (compiler == NULL) {
        F_set_error_message(impl, "无法创建编译器实例");
        return Eum_EXEC_RESULT_SYSTEM_ERROR;
    }
    
    /* 创建编译选项 */
    Stru_CompileOptions_t options = F_create_default_compile_options();
    options.input_file = input_file;
    options.output_file = output_file;
    options.verbose = verbose;
    
    /* 执行编译 */
    Stru_CompilationResult_t* result = compiler->compile_file(compiler, &options);
    
    /* 处理编译结果 */
    if (result == NULL) {
        F_set_error_message(impl, "编译过程发生未知错误");
        compiler->destroy(compiler);
        return Eum_EXEC_RESULT_SYSTEM_ERROR;
    }
    
    if (!result->success) {
        if (result->error_message != NULL) {
            F_set_error_message(impl, result->error_message);
        } else {
            F_set_error_message(impl, "编译失败");
        }
        
        F_free_compilation_result(result);
        compiler->destroy(compiler);
        return Eum_EXEC_RESULT_FAILURE;
    }
    
    /* 输出成功信息 */
    if (verbose) {
        printf("编译成功！\n");
        printf("输出文件: %s\n", result->output_path ? result->output_path : "未指定");
        printf("编译耗时: %zu 毫秒\n", result->compile_time_ms);
        printf("错误数量: %d\n", result->error_count);
        printf("警告数量: %d\n", result->warning_count);
    } else {
        printf("编译成功: %s -> %s\n", 
               input_file, 
               result->output_path ? result->output_path : "a.out");
    }
    
    /* 清理资源 */
    F_free_compilation_result(result);
    compiler->destroy(compiler);
    
    return Eum_EXEC_RESULT_SUCCESS;
}

/**
 * @brief 执行运行命令
 * 
 * 运行CN程序。
 * 
 * @param self 接口指针
 * @param parse_result 解析结果
 * @return Eum_ExecutionResult_t 执行结果
 */
static Eum_ExecutionResult_t F_execute_run_impl(Stru_CommandExecutorInterface_t* self, 
                                               const Stru_ParseResult_t* parse_result)
{
    Stru_CommandExecutorImpl_t* impl = (Stru_CommandExecutorImpl_t*)self;
    
    /* 获取输入文件 */
    const char* input_file = F_get_first_value_argument(parse_result);
    if (input_file == NULL) {
        F_set_error_message(impl, "运行命令需要指定输入文件");
        return Eum_EXEC_RESULT_SYNTAX_ERROR;
    }
    
    /* 检查详细输出标志 */
    bool verbose = false;
    if (parse_result != NULL) {
        for (size_t i = 0; i < parse_result->count; i++) {
            const Stru_Argument_t* arg = &parse_result->arguments[i];
            if (arg->type == Eum_ARG_TYPE_FLAG) {
                if (strcmp(arg->name, "-V") == 0 || strcmp(arg->name, "--verbose") == 0) {
                    verbose = true;
                    break;
                }
            }
        }
    }
    
    /* 执行运行 */
    if (verbose) {
        printf("开始运行: %s\n", input_file);
    }
    
    /* TODO: 调用核心层的运行功能 */
    printf("运行功能正在开发中...\n");
    printf("运行文件: %s\n", input_file);
    
    if (verbose) {
        printf("运行完成\n");
    }
    
    return Eum_EXEC_RESULT_SUCCESS;
}

/**
 * @brief 执行调试命令
 * 
 * 调试CN程序。
 * 
 * @param self 接口指针
 * @param parse_result 解析结果
 * @return Eum_ExecutionResult_t 执行结果
 */
static Eum_ExecutionResult_t F_execute_debug_impl(Stru_CommandExecutorInterface_t* self, 
                                                 const Stru_ParseResult_t* parse_result)
{
    Stru_CommandExecutorImpl_t* impl = (Stru_CommandExecutorImpl_t*)self;
    
    /* 获取输入文件 */
    const char* input_file = F_get_first_value_argument(parse_result);
    if (input_file == NULL) {
        F_set_error_message(impl, "调试命令需要指定输入文件");
        return Eum_EXEC_RESULT_SYNTAX_ERROR;
    }
    
    /* 检查详细输出标志 */
    bool verbose = false;
    if (parse_result != NULL) {
        for (size_t i = 0; i < parse_result->count; i++) {
            const Stru_Argument_t* arg = &parse_result->arguments[i];
            if (arg->type == Eum_ARG_TYPE_FLAG) {
                if (strcmp(arg->name, "-V") == 0 || strcmp(arg->name, "--verbose") == 0) {
                    verbose = true;
                    break;
                }
            }
        }
    }
    
    /* 执行调试 */
    if (verbose) {
        printf("开始调试: %s\n", input_file);
    }
    
    /* TODO: 调用调试器功能 */
    printf("调试功能正在开发中...\n");
    printf("调试文件: %s\n", input_file);
    
    if (verbose) {
        printf("调试会话已启动\n");
    }
    
    return Eum_EXEC_RESULT_SUCCESS;
}

/**
 * @brief 获取错误信息
 * 
 * 获取最后一次执行的错误信息。
 * 
 * @param self 接口指针
 * @return const char* 错误信息，无错误返回NULL
 */
static const char* F_get_error_message_impl(Stru_CommandExecutorInterface_t* self)
{
    Stru_CommandExecutorImpl_t* impl = (Stru_CommandExecutorImpl_t*)self;
    return impl->error_message;
}

/**
 * @brief 销毁命令执行器
 * 
 * 清理命令执行器占用的资源。
 * 
 * @param self 接口指针
 */
static void F_destroy_impl(Stru_CommandExecutorInterface_t* self)
{
    Stru_CommandExecutorImpl_t* impl = (Stru_CommandExecutorImpl_t*)self;
    if (impl == NULL) {
        return;
    }
    
    /* 释放错误信息缓冲区 */
    if (impl->error_message != NULL) {
        free(impl->error_message);
        impl->error_message = NULL;
    }
    
    /* 释放实现结构体 */
    free(impl);
}

/* ==================== 辅助函数实现 ==================== */

/**
 * @brief 设置错误信息
 * 
 * 设置命令执行器的错误信息。
 * 
 * @param impl 命令执行器实现
 * @param message 错误信息
 */
static void F_set_error_message(Stru_CommandExecutorImpl_t* impl, const char* message)
{
    if (impl == NULL || message == NULL) {
        return;
    }
    
    /* 释放旧的错误信息 */
    if (impl->error_message != NULL) {
        free(impl->error_message);
        impl->error_message = NULL;
    }
    
    /* 分配新的错误信息 */
    impl->error_message = strdup(message);
}

/**
 * @brief 获取第一个值参数
 * 
 * 从解析结果中获取第一个类型为值的参数。
 * 
 * @param parse_result 解析结果
 * @return const char* 参数值，未找到返回NULL
 */
static const char* F_get_first_value_argument(const Stru_ParseResult_t* parse_result)
{
    if (parse_result == NULL) {
        return NULL;
    }
    
    for (size_t i = 0; i < parse_result->count; i++) {
        const Stru_Argument_t* arg = &parse_result->arguments[i];
        if (arg->type == Eum_ARG_TYPE_VALUE) {
            return arg->name; /* 对于值类型，name字段存储值 */
        }
    }
    
    return NULL;
}

#ifdef __cplusplus
}
#endif
