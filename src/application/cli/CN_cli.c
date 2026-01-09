/**
 * @file CN_cli.c
 * @brief 命令行界面模块实现
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 * 
 * 本文件实现了命令行界面模块的具体功能，包括命令行参数解析、
 * 命令执行和用户交互等功能。遵循CN_Language项目的分层架构
 * 和SOLID设计原则，每个函数不超过50行，文件不超过500行。
 */

#include "CN_cli_interface.h"
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
    int argc;               /**< 命令行参数个数 */
    char** argv;            /**< 命令行参数数组 */
    char* command;          /**< 解析出的命令 */
    char** arguments;       /**< 命令参数列表 */
    int argument_count;     /**< 参数个数 */
    bool is_initialized;    /**< 初始化标志 */
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
static bool F_cli_parse_arguments(Stru_CliInterface_t* self);
static int F_cli_execute_command(Stru_CliInterface_t* self);
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
    impl->interface.parse_arguments = F_cli_parse_arguments;
    impl->interface.execute_command = F_cli_execute_command;
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
    context->command = NULL;
    context->arguments = NULL;
    context->argument_count = 0;
    context->is_initialized = false;
    
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
    
    /* 释放动态分配的内存 */
    if (context->command != NULL) {
        free(context->command);
    }
    
    if (context->arguments != NULL) {
        for (int i = 0; i < context->argument_count; i++) {
            if (context->arguments[i] != NULL) {
                free(context->arguments[i]);
            }
        }
        free(context->arguments);
    }
    
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
 * @brief 解析命令行参数
 * 
 * 解析用户输入的命令行参数，提取命令、选项和参数。
 * 支持常见命令：compile（编译）、run（运行）、debug（调试）。
 * 
 * @param self 接口指针
 * @return bool 解析成功返回true，失败返回false
 */
static bool F_cli_parse_arguments(Stru_CliInterface_t* self)
{
    Stru_CliImpl_t* impl = (Stru_CliImpl_t*)self;
    if (impl == NULL || impl->context == NULL || !impl->context->is_initialized) {
        return false;
    }
    
    Stru_CliContext_t* context = impl->context;
    
    /* 至少需要一个参数（命令本身） */
    if (context->argc < 2) {
        return false;
    }
    
    /* 提取命令 */
    context->command = strdup(context->argv[1]);
    if (context->command == NULL) {
        return false;
    }
    
    /* 提取参数（如果有） */
    context->argument_count = context->argc - 2;
    if (context->argument_count > 0) {
        context->arguments = (char**)malloc(context->argument_count * sizeof(char*));
        if (context->arguments == NULL) {
            free(context->command);
            context->command = NULL;
            return false;
        }
        
        for (int i = 0; i < context->argument_count; i++) {
            context->arguments[i] = strdup(context->argv[i + 2]);
            if (context->arguments[i] == NULL) {
                /* 清理已分配的内存 */
                for (int j = 0; j < i; j++) {
                    free(context->arguments[j]);
                }
                free(context->arguments);
                free(context->command);
                context->arguments = NULL;
                context->command = NULL;
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief 执行命令行命令
 * 
 * 根据解析的命令执行相应的操作。
 * 目前支持的命令：help（帮助）、version（版本）、compile（编译）。
 * 
 * @param self 接口指针
 * @return int 执行结果，0表示成功，非0表示错误码
 */
static int F_cli_execute_command(Stru_CliInterface_t* self)
{
    Stru_CliImpl_t* impl = (Stru_CliImpl_t*)self;
    if (impl == NULL || impl->context == NULL || !impl->context->is_initialized) {
        return -1;
    }
    
    Stru_CliContext_t* context = impl->context;
    
    /* 如果没有命令，显示帮助 */
    if (context->command == NULL) {
        F_cli_show_help(self);
        return 0;
    }
    
    /* 根据命令执行相应操作 */
    if (strcmp(context->command, "help") == 0) {
        F_cli_show_help(self);
        return 0;
    } else if (strcmp(context->command, "version") == 0) {
        F_cli_show_version(self);
        return 0;
    } else if (strcmp(context->command, "compile") == 0) {
        /* TODO: 调用核心层的编译功能 */
        printf("编译功能暂未实现\n");
        return 0;
    } else if (strcmp(context->command, "run") == 0) {
        /* TODO: 调用核心层的运行功能 */
        printf("运行功能暂未实现\n");
        return 0;
    } else if (strcmp(context->command, "debug") == 0) {
        /* TODO: 调用调试器功能 */
        printf("调试功能暂未实现\n");
        return 0;
    } else {
        printf("未知命令: %s\n", context->command);
        printf("使用 'cn help' 查看可用命令\n");
        return 1;
    }
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
    (void)self; /* 未使用参数 */
    printf("CN_Language 命令行工具\n");
    printf("版本: 1.0.0\n");
    printf("\n");
    printf("用法: cn <命令> [参数]\n");
    printf("\n");
    printf("可用命令:\n");
    printf("  help        显示此帮助信息\n");
    printf("  version     显示版本信息\n");
    printf("  compile     编译CN源代码文件\n");
    printf("  run         运行CN程序\n");
    printf("  debug       调试CN程序\n");
    printf("\n");
    printf("示例:\n");
    printf("  cn help\n");
    printf("  cn version\n");
    printf("  cn compile hello.cn\n");
    printf("  cn run hello.cn\n");
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
    (void)self; /* 未使用参数 */
    printf("CN_Language 版本 1.0.0\n");
    printf("架构版本: 2.0.0\n");
    printf("构建日期: 2026-01-09\n");
    printf("许可证: MIT\n");
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
