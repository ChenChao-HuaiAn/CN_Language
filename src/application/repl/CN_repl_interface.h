/**
 * @file CN_repl_interface.h
 * @brief 交互式环境模块接口定义
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 * 
 * 本文件定义了交互式环境模块的抽象接口，遵循CN_Language项目的
 * 分层架构和SOLID设计原则。REPL模块提供读取-求值-打印循环功能，
 * 支持逐行执行CN代码，并提供历史记录、自动补全等增强功能。
 */

#ifndef CN_REPL_INTERFACE_H
#define CN_REPL_INTERFACE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 交互式环境接口结构体
 * 
 * 定义了交互式环境模块的抽象接口，遵循依赖倒置原则。
 * 高层模块通过此接口与REPL模块交互，实现模块间的解耦。
 */
typedef struct Stru_ReplInterface_t Stru_ReplInterface_t;

/**
 * @brief 交互式环境接口方法定义
 */
struct Stru_ReplInterface_t {
    /**
     * @brief 初始化交互式环境
     * 
     * @param self 接口指针
     * @return bool 初始化成功返回true，失败返回false
     */
    bool (*initialize)(Stru_ReplInterface_t* self);
    
    /**
     * @brief 启动交互式环境
     * 
     * 启动读取-求值-打印循环，等待用户输入并执行。
     * 
     * @param self 接口指针
     * @return int 退出码，0表示正常退出，非0表示错误
     */
    int (*start)(Stru_ReplInterface_t* self);
    
    /**
     * @brief 执行单行代码
     * 
     * 执行用户输入的单行CN代码，并返回结果。
     * 
     * @param self 接口指针
     * @param line 要执行的代码行
     * @return char* 执行结果字符串，需要调用者释放
     */
    char* (*execute_line)(Stru_ReplInterface_t* self, const char* line);
    
    /**
     * @brief 获取历史记录
     * 
     * 获取交互式环境的历史命令记录。
     * 
     * @param self 接口指针
     * @param index 历史记录索引（0表示最近的一条）
     * @return const char* 历史命令，NULL表示索引无效
     */
    const char* (*get_history)(Stru_ReplInterface_t* self, size_t index);
    
    /**
     * @brief 添加自动补全建议
     * 
     * 为当前输入添加自动补全建议。
     * 
     * @param self 接口指针
     * @param input 当前输入
     * @param suggestions 建议数组
     * @param count 建议数量
     */
    void (*add_completions)(Stru_ReplInterface_t* self, const char* input, 
                           const char** suggestions, size_t count);
    
    /**
     * @brief 显示欢迎信息
     * 
     * 显示交互式环境的欢迎信息和帮助提示。
     * 
     * @param self 接口指针
     */
    void (*show_welcome)(Stru_ReplInterface_t* self);
    
    /**
     * @brief 销毁交互式环境
     * 
     * 清理交互式环境占用的资源。
     * 
     * @param self 接口指针
     */
    void (*destroy)(Stru_ReplInterface_t* self);
};

/**
 * @brief 创建交互式环境实例
 * 
 * 工厂函数，创建并返回交互式环境接口的实例。
 * 
 * @return Stru_ReplInterface_t* 交互式环境接口指针，失败返回NULL
 */
Stru_ReplInterface_t* F_create_repl_interface(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_REPL_INTERFACE_H */
