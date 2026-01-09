/**
 * @file CN_command_executor.h
 * @brief 命令行命令执行器接口定义
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 * 
 * 本文件定义了命令行命令执行器的抽象接口，负责执行具体的命令操作，
 * 如编译、运行、调试等。遵循单一职责原则和依赖倒置原则。
 */

#ifndef CN_COMMAND_EXECUTOR_H
#define CN_COMMAND_EXECUTOR_H

#include "CN_command_parser.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 命令执行结果枚举
 * 
 * 定义命令执行的不同结果状态。
 */
typedef enum {
    Eum_EXEC_RESULT_SUCCESS,      /**< 执行成功 */
    Eum_EXEC_RESULT_FAILURE,      /**< 执行失败 */
    Eum_EXEC_RESULT_SYNTAX_ERROR, /**< 语法错误 */
    Eum_EXEC_RESULT_FILE_ERROR,   /**< 文件错误 */
    Eum_EXEC_RESULT_SYSTEM_ERROR, /**< 系统错误 */
    Eum_EXEC_RESULT_UNKNOWN_CMD   /**< 未知命令 */
} Eum_ExecutionResult_t;

/**
 * @brief 命令执行器接口结构体
 * 
 * 定义了命令执行器模块的抽象接口，遵循依赖倒置原则。
 */
typedef struct Stru_CommandExecutorInterface_t Stru_CommandExecutorInterface_t;

/**
 * @brief 命令执行器接口方法定义
 */
struct Stru_CommandExecutorInterface_t {
    /**
     * @brief 执行命令
     * 
     * 根据解析结果执行相应的命令。
     * 
     * @param self 接口指针
     * @param parse_result 解析结果
     * @return Eum_ExecutionResult_t 执行结果
     */
    Eum_ExecutionResult_t (*execute)(Stru_CommandExecutorInterface_t* self, 
                                    const Stru_ParseResult_t* parse_result);
    
    /**
     * @brief 执行帮助命令
     * 
     * 显示帮助信息。
     * 
     * @param self 接口指针
     * @param parse_result 解析结果
     * @return Eum_ExecutionResult_t 执行结果
     */
    Eum_ExecutionResult_t (*execute_help)(Stru_CommandExecutorInterface_t* self, 
                                         const Stru_ParseResult_t* parse_result);
    
    /**
     * @brief 执行版本命令
     * 
     * 显示版本信息。
     * 
     * @param self 接口指针
     * @param parse_result 解析结果
     * @return Eum_ExecutionResult_t 执行结果
     */
    Eum_ExecutionResult_t (*execute_version)(Stru_CommandExecutorInterface_t* self, 
                                            const Stru_ParseResult_t* parse_result);
    
    /**
     * @brief 执行编译命令
     * 
     * 编译CN源代码文件。
     * 
     * @param self 接口指针
     * @param parse_result 解析结果
     * @return Eum_ExecutionResult_t 执行结果
     */
    Eum_ExecutionResult_t (*execute_compile)(Stru_CommandExecutorInterface_t* self, 
                                            const Stru_ParseResult_t* parse_result);
    
    /**
     * @brief 执行运行命令
     * 
     * 运行CN程序。
     * 
     * @param self 接口指针
     * @param parse_result 解析结果
     * @return Eum_ExecutionResult_t 执行结果
     */
    Eum_ExecutionResult_t (*execute_run)(Stru_CommandExecutorInterface_t* self, 
                                        const Stru_ParseResult_t* parse_result);
    
    /**
     * @brief 执行调试命令
     * 
     * 调试CN程序。
     * 
     * @param self 接口指针
     * @param parse_result 解析结果
     * @return Eum_ExecutionResult_t 执行结果
     */
    Eum_ExecutionResult_t (*execute_debug)(Stru_CommandExecutorInterface_t* self, 
                                          const Stru_ParseResult_t* parse_result);
    
    /**
     * @brief 获取错误信息
     * 
     * 获取最后一次执行的错误信息。
     * 
     * @param self 接口指针
     * @return const char* 错误信息，无错误返回NULL
     */
    const char* (*get_error_message)(Stru_CommandExecutorInterface_t* self);
    
    /**
     * @brief 销毁命令执行器
     * 
     * 清理命令执行器占用的资源。
     * 
     * @param self 接口指针
     */
    void (*destroy)(Stru_CommandExecutorInterface_t* self);
};

/**
 * @brief 创建命令执行器实例
 * 
 * 工厂函数，创建并返回命令执行器接口的实例。
 * 
 * @return Stru_CommandExecutorInterface_t* 命令执行器接口指针，失败返回NULL
 */
Stru_CommandExecutorInterface_t* F_create_command_executor(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_COMMAND_EXECUTOR_H */
