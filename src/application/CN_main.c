/**
 * @file CN_main.c
 * @brief CN_Language主程序入口点
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 * 
 * 本文件是CN_Language项目的主程序入口点，负责初始化应用层模块，
 * 解析命令行参数，并调用相应的功能模块。遵循分层架构设计，
 * 应用层依赖核心层和基础设施层，实现单向依赖关系。
 */

#include "cli/CN_cli_interface.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief 主函数
 * 
 * 程序入口点，负责初始化命令行界面并执行用户命令。
 * 
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return int 程序退出码，0表示成功，非0表示错误
 */
int main(int argc, char** argv)
{
    /* 创建命令行界面实例 */
    Stru_CliInterface_t* cli = F_create_cli_interface();
    if (cli == NULL) {
        fprintf(stderr, "错误：无法创建命令行界面\n");
        return EXIT_FAILURE;
    }
    
    /* 初始化命令行界面 */
    if (!cli->initialize(cli, argc, argv)) {
        fprintf(stderr, "错误：命令行界面初始化失败\n");
        cli->destroy(cli);
        return EXIT_FAILURE;
    }
    
    /* 解析并执行命令 */
    int result = cli->parse_and_execute(cli);
    
    /* 清理资源 */
    cli->destroy(cli);
    
    return result;
}

/**
 * @brief 简单的命令行测试函数
 * 
 * 用于测试命令行界面的基本功能。
 */
void test_cli_basic(void)
{
    printf("CN_Language 命令行界面测试\n");
    printf("==========================\n");
    
    /* 测试帮助命令 */
    char* test_argv_help[] = {"cn", "help"};
    Stru_CliInterface_t* cli_help = F_create_cli_interface();
    if (cli_help != NULL) {
        cli_help->initialize(cli_help, 2, test_argv_help);
        cli_help->parse_and_execute(cli_help);
        cli_help->destroy(cli_help);
    }
    
    printf("\n");
    
    /* 测试版本命令 */
    char* test_argv_version[] = {"cn", "version"};
    Stru_CliInterface_t* cli_version = F_create_cli_interface();
    if (cli_version != NULL) {
        cli_version->initialize(cli_version, 2, test_argv_version);
        cli_version->parse_and_execute(cli_version);
        cli_version->destroy(cli_version);
    }
}

/**
 * @brief 编译测试函数
 * 
 * 用于测试编译命令的基本功能。
 */
void test_cli_compile(void)
{
    printf("编译命令测试\n");
    printf("============\n");
    
    char* test_argv[] = {"cn", "compile", "test.cn"};
    Stru_CliInterface_t* cli = F_create_cli_interface();
    if (cli != NULL) {
        cli->initialize(cli, 3, test_argv);
        cli->parse_and_execute(cli);
        cli->destroy(cli);
    }
}

/**
 * @brief 带选项的编译测试函数
 * 
 * 用于测试带选项的编译命令。
 */
void test_cli_compile_with_options(void)
{
    printf("带选项的编译命令测试\n");
    printf("====================\n");
    
    char* test_argv[] = {"cn", "compile", "test.cn", "-o", "test.exe", "--verbose"};
    Stru_CliInterface_t* cli = F_create_cli_interface();
    if (cli != NULL) {
        cli->initialize(cli, 6, test_argv);
        cli->parse_and_execute(cli);
        cli->destroy(cli);
    }
}
