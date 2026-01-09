/**
 * @file CN_cli.c
 * @brief 命令行界面模块实现
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 * 
 * 本文件实现了命令行界面模块的具体功能，整合命令解析器和执行器，
 * 提供统一的命令行界面。遵循CN_Language项目的分层架构
 * 和SOLID设计原则，每个函数不超过50行，文件不超过500行。
 */

#include "CN_cli_interface.h"
#include "CN_command_parser.h"
#include "CN_command_executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 命令行界面内部数据结构
 * 
 * 封装命令行界面的内部状态，对外部模块不可见。
 * 遵循数据封装原则，隐藏实现细节。
 */
typedef struct {
    int argc;                                   /**< 命令行参数个数 */
    char** argv;                                /**< 命令行参数数组 */
    bool is_initialized;                        /**< 初始化标志 */
    Stru_CommandParserInterface_t* parser;      /**< 命令解析器 */
    Stru_CommandExecutorInterface_t* executor;  /**< 命令执行器 */
    Stru_ParseResult_t* parse_result;           /**< 解析结果 */
} Stru_CliContext_t;

/**
 * @brief 命令行界面实现结构体
 * 
 * 包含接口指针和内部上下文，实现接口隔离原则。
 */
typedef struct {
    Stru_CliInterface_t interface;  /**< 公共接口 */
    Stru_CliContext_t* context;     /**< 内部上下文 */
} Stru_CliImpl_t;

/* 内部函数声明 */
static bool F_cli_initialize(Stru_CliInterface_t* self, int argc, char** argv);
static int F_cli_parse_and_execute(Stru_CliInterface_t* self);
static Stru_CommandParserInterface_t* F_cli_get_parser(Stru_CliInterface_t* self);
static Stru_CommandExecutorInterface_t* F_cli_get_executor(Stru_CliInterface_t* self);
static void F_cli_show_help(Stru_CliInterface_t* self);
static void F_cli_show_version(Stru_CliInterface_t* self);
static void F_cli_destroy(Stru_CliInterface_t* self);
static Stru_CliContext_t* F_create_cli_context(void);
static void F_destroy_cli_context(Stru_CliContext_t* context);

/**
 * @brief 创建命令行界面实例
 * 
 * 工厂函数，创建并初始化命令行界面接口的实例。
 * 遵循单一职责原则，只负责创建实例。
 * 
 * @return Stru_CliInterface_t* 命令行界面接口指针，失败返回NULL
 */
Stru_CliInterface_t* F_create_cli_interface(void)
{
    Stru_CliImpl_t* impl = (Stru_CliImpl_t*)malloc(sizeof(Stru_CliImpl_t));
    if (impl == NULL) {
        return NULL;
    }
    
    /* 初始化接口方法 */
    impl->interface.initialize = F_cli_initialize;
    impl->interface.parse_and_execute = F_cli_parse_and_execute;
    impl->interface.get_parser = F_cli_get_parser;
    impl->interface.get_executor = F_cli_get_executor;
    impl->interface.show_help = F_cli_show_help;
    impl->interface.show_version = F_cli_show_version;
    impl->interface.destroy = F_cli_destroy;
    
    /* 创建内部上下文 */
    impl->context = F_create_cli_context();
    if (impl->context == NULL) {
        free(impl);
        return NULL;
    }
    
    return (Stru_CliInterface_t*)impl;
}

/**
 * @brief 创建命令行上下文
 * 
 * 分配并初始化命令行内部上下文。
 * 
 * @return Stru_CliContext_t* 上下文指针，失败返回NULL
 */
static Stru_CliContext_t* F_create_cli_context(void)
{
    Stru_CliContext_t* context = (Stru_CliContext_t*)malloc(sizeof(Stru_CliContext_t));
    if (context == NULL) {
        return NULL;
    }
    
    /* 初始化上下文 */
    context->argc = 0;
    context->argv = NULL;
    context->is_initialized = false;
    context->parser = NULL;
    context->executor = NULL;
    context->parse_result = NULL;
    
    /* 创建命令解析器 */
    context->parser = F_create_command_parser();
    if (context->parser == NULL) {
        free(context);
        return NULL;
    }
    
    /* 创建命令执行器 */
    context->executor = F_create_command_executor();
    if (context->executor == NULL) {
        context->parser->destroy(context->parser);
        free(context);
        return NULL;
    }
    
    return context;
}

/**
 * @brief 销毁命令行上下文
 * 
 * 清理命令行内部上下文占用的资源。
 * 
 * @param context 要销毁的上下文指针
 */
static void F_destroy_cli_context(Stru_CliContext_t* context)
{
    if (context == NULL) {
        return;
    }
    
    /* 销毁解析结果 */
    if (context->parse_result != NULL && context->parser != NULL) {
        context->parser->destroy_result(context->parser, context->parse_result);
        context->parse_result = NULL;
    }
    
    /* 销毁命令执行器 */
    if (context->executor != NULL) {
        context->executor->destroy(context->executor);
        context->executor = NULL;
    }
    
    /* 销毁命令解析器 */
    if (context->parser != NULL) {
        context->parser->destroy(context->parser);
        context->parser = NULL;
    }
    
    /* 释放上下文结构体 */
    free(context);
}

/**
 * @brief 初始化命令行界面
 * 
 * 设置命令行参数，准备解析和执行。
 * 
 * @param self 接口指针
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return bool 初始化成功返回true，失败返回false
 */
static bool F_cli_initialize(Stru_CliInterface_t* self, int argc, char** argv)
{
    Stru_CliImpl_t* impl = (Stru_CliImpl_t*)self;
    if (impl == NULL || impl->context == NULL) {
        return false;
    }
    
    /* 保存命令行参数 */
    impl->context->argc = argc;
    impl->context->argv = argv;
    impl->context->is_initialized = true;
    
    return true;
}

/**
 * @brief 解析并执行命令行
 * 
 * 解析命令行参数并执行相应的命令。
 * 
 * @param self 接口指针
 * @return int 执行结果，0表示成功，非0表示错误码
 */
static int F_cli_parse_and_execute(Stru_CliInterface_t* self)
{
    Stru_CliImpl_t* impl = (Stru_CliImpl_t*)self;
    if (impl == NULL || impl->context == NULL || !impl->context->is_initialized) {
        return -1;
    }
    
    Stru_CliContext_t* context = impl->context;
    
    /* 解析命令行参数 */
    context->parse_result = context->parser->parse(context->parser, 
                                                  context->argc, 
                                                  context->argv);
    if (context->parse_result == NULL) {
        fprintf(stderr, "错误：命令行参数解析失败\n");
        return 1;
    }
    
    /* 验证解析结果 */
    if (!context->parser->validate(context->parser, context->parse_result)) {
        fprintf(stderr, "错误：命令行参数验证失败\n");
        fprintf(stderr, "使用 'cn help' 查看可用命令\n");
        context->parser->destroy_result(context->parser, context->parse_result);
        context->parse_result = NULL;
        return 1;
    }
    
    /* 执行命令 */
    Eum_ExecutionResult_t exec_result = context->executor->execute(context->executor, 
                                                                  context->parse_result);
    
    /* 检查执行结果 */
    int return_code = 0;
    switch (exec_result) {
        case Eum_EXEC_RESULT_SUCCESS:
            return_code = 0;
            break;
        case Eum_EXEC_RESULT_FAILURE:
            fprintf(stderr, "错误：命令执行失败\n");
            return_code = 1;
            break;
        case Eum_EXEC_RESULT_SYNTAX_ERROR:
            fprintf(stderr, "错误：语法错误\n");
            return_code = 2;
            break;
        case Eum_EXEC_RESULT_FILE_ERROR:
            fprintf(stderr, "错误：文件错误\n");
            return_code = 3;
            break;
        case Eum_EXEC_RESULT_SYSTEM_ERROR:
            fprintf(stderr, "错误：系统错误\n");
            return_code = 4;
            break;
        case Eum_EXEC_RESULT_UNKNOWN_CMD:
            fprintf(stderr, "错误：未知命令\n");
            fprintf(stderr, "使用 'cn help' 查看可用命令\n");
            return_code = 5;
            break;
        default:
            fprintf(stderr, "错误：未知执行结果\n");
            return_code = 6;
            break;
    }
    
    /* 获取并显示错误信息（如果有） */
    const char* error_msg = context->executor->get_error_message(context->executor);
    if (error_msg != NULL && exec_result != Eum_EXEC_RESULT_SUCCESS) {
        fprintf(stderr, "详细信息: %s\n", error_msg);
    }
    
    /* 清理解析结果 */
    if (context->parse_result != NULL) {
        context->parser->destroy_result(context->parser, context->parse_result);
        context->parse_result = NULL;
    }
    
    return return_code;
}

/**
 * @brief 获取命令解析器
 * 
 * 获取内部使用的命令解析器接口。
 * 
 * @param self 接口指针
 * @return Stru_CommandParserInterface_t* 命令解析器接口
 */
static Stru_CommandParserInterface_t* F_cli_get_parser(Stru_CliInterface_t* self)
{
    Stru_CliImpl_t* impl = (Stru_CliImpl_t*)self;
    if (impl == NULL || impl->context == NULL) {
        return NULL;
    }
    
    return impl->context->parser;
}

/**
 * @brief 获取命令执行器
 * 
 * 获取内部使用的命令执行器接口。
 * 
 * @param self 接口指针
 * @return Stru_CommandExecutorInterface_t* 命令执行器接口
 */
static Stru_CommandExecutorInterface_t* F_cli_get_executor(Stru_CliInterface_t* self)
{
    Stru_CliImpl_t* impl = (Stru_CliImpl_t*)self;
    if (impl == NULL || impl->context == NULL) {
        return NULL;
    }
    
    return impl->context->executor;
}

/**
 * @brief 显示帮助信息
 * 
 * 显示命令行工具的使用帮助信息。
 * 
 * @param self 接口指针
 */
static void F_cli_show_help(Stru_CliInterface_t* self)
{
    Stru_CliImpl_t* impl = (Stru_CliImpl_t*)self;
    if (impl == NULL || impl->context == NULL || impl->context->executor == NULL) {
        return;
    }
    
    /* 创建临时的解析结果用于显示帮助 */
    Stru_ParseResult_t temp_result = {0};
    temp_result.command = "help";
    
    impl->context->executor->execute_help(impl->context->executor, &temp_result);
}

/**
 * @brief 显示版本信息
 * 
 * 显示CN_Language的版本信息。
 * 
 * @param self 接口指针
 */
static void F_cli_show_version(Stru_CliInterface_t* self)
{
    Stru_CliImpl_t* impl = (Stru_CliImpl_t*)self;
    if (impl == NULL || impl->context == NULL || impl->context->executor == NULL) {
        return;
    }
    
    /* 创建临时的解析结果用于显示版本 */
    Stru_ParseResult_t temp_result = {0};
    temp_result.command = "version";
    
    impl->context->executor->execute_version(impl->context->executor, &temp_result);
}

/**
 * @brief 销毁命令行界面
 * 
 * 清理命令行界面占用的资源。
 * 
 * @param self 接口指针
 */
static void F_cli_destroy(Stru_CliInterface_t* self)
{
    Stru_CliImpl_t* impl = (Stru_CliImpl_t*)self;
    if (impl == NULL) {
        return;
    }
    
    /* 销毁内部上下文 */
    if (impl->context != NULL) {
        F_destroy_cli_context(impl->context);
        impl->context = NULL;
    }
    
    /* 销毁实现结构体 */
    free(impl);
}

#ifdef __cplusplus
}
#endif
