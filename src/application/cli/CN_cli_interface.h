/**
 * @file CN_cli_interface.h
 * @brief 命令行界面模块接口定义
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 * 
 * 本文件定义了命令行界面模块的抽象接口，遵循CN_Language项目的
 * 分层架构和SOLID设计原则。CLI模块负责解析用户输入的命令行参数，
 * 调用相应的核心层功能，并提供统一的用户交互体验。
 */

#ifndef CN_CLI_INTERFACE_H
#define CN_CLI_INTERFACE_H

#include "CN_command_parser.h"
#include "CN_command_executor.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 命令行界面接口结构体
 * 
 * 定义了命令行界面模块的抽象接口，遵循依赖倒置原则。
 * 高层模块通过此接口与CLI模块交互，实现模块间的解耦。
 */
typedef struct Stru_CliInterface_t Stru_CliInterface_t;

/**
 * @brief 命令行界面接口方法定义
 */
struct Stru_CliInterface_t {
    /**
     * @brief 初始化命令行界面
     * 
     * @param self 接口指针
     * @param argc 命令行参数个数
     * @param argv 命令行参数数组
     * @return bool 初始化成功返回true，失败返回false
     */
    bool (*initialize)(Stru_CliInterface_t* self, int argc, char** argv);
    
    /**
     * @brief 解析并执行命令行
     * 
     * 解析命令行参数并执行相应的命令。
     * 
     * @param self 接口指针
     * @return int 执行结果，0表示成功，非0表示错误码
     */
    int (*parse_and_execute)(Stru_CliInterface_t* self);
    
    /**
     * @brief 获取命令解析器
     * 
     * 获取内部使用的命令解析器接口。
     * 
     * @param self 接口指针
     * @return Stru_CommandParserInterface_t* 命令解析器接口
     */
    Stru_CommandParserInterface_t* (*get_parser)(Stru_CliInterface_t* self);
    
    /**
     * @brief 获取命令执行器
     * 
     * 获取内部使用的命令执行器接口。
     * 
     * @param self 接口指针
     * @return Stru_CommandExecutorInterface_t* 命令执行器接口
     */
    Stru_CommandExecutorInterface_t* (*get_executor)(Stru_CliInterface_t* self);
    
    /**
     * @brief 显示帮助信息
     * 
     * 显示命令行工具的使用帮助信息。
     * 
     * @param self 接口指针
     */
    void (*show_help)(Stru_CliInterface_t* self);
    
    /**
     * @brief 显示版本信息
     * 
     * 显示CN_Language的版本信息。
     * 
     * @param self 接口指针
     */
    void (*show_version)(Stru_CliInterface_t* self);
    
    /**
     * @brief 销毁命令行界面
     * 
     * 清理命令行界面占用的资源。
     * 
     * @param self 接口指针
     */
    void (*destroy)(Stru_CliInterface_t* self);
};

/**
 * @brief 创建命令行界面实例
 * 
 * 工厂函数，创建并返回命令行界面接口的实例。
 * 
 * @return Stru_CliInterface_t* 命令行界面接口指针，失败返回NULL
 */
Stru_CliInterface_t* F_create_cli_interface(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_CLI_INTERFACE_H */
