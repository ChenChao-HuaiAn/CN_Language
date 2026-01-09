/**
 * @file CN_debugger_interface.h
 * @brief 调试器模块接口定义
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 * 
 * 本文件定义了调试器模块的抽象接口，遵循CN_Language项目的
 * 分层架构和SOLID设计原则。调试器模块提供断点设置、单步执行、
 * 变量查看等功能，与运行时系统交互实现程序调试。
 */

#ifndef CN_DEBUGGER_INTERFACE_H
#define CN_DEBUGGER_INTERFACE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 断点信息结构体
 * 
 * 描述断点的位置和状态信息。
 */
typedef struct {
    const char* file_name;      /**< 文件名 */
    size_t line_number;         /**< 行号 */
    bool enabled;               /**< 是否启用 */
    size_t hit_count;           /**< 命中次数 */
} Stru_Breakpoint_t;

/**
 * @brief 调试器接口结构体
 * 
 * 定义了调试器模块的抽象接口，遵循依赖倒置原则。
 * 高层模块通过此接口与调试器模块交互，实现模块间的解耦。
 */
typedef struct Stru_DebuggerInterface_t Stru_DebuggerInterface_t;

/**
 * @brief 调试器接口方法定义
 */
struct Stru_DebuggerInterface_t {
    /**
     * @brief 初始化调试器
     * 
     * @param self 接口指针
     * @param program_path 要调试的程序路径
     * @return bool 初始化成功返回true，失败返回false
     */
    bool (*initialize)(Stru_DebuggerInterface_t* self, const char* program_path);
    
    /**
     * @brief 启动调试会话
     * 
     * 启动调试会话，准备调试目标程序。
     * 
     * @param self 接口指针
     * @return bool 启动成功返回true，失败返回false
     */
    bool (*start_session)(Stru_DebuggerInterface_t* self);
    
    /**
     * @brief 设置断点
     * 
     * 在指定位置设置断点。
     * 
     * @param self 接口指针
     * @param file_name 文件名
     * @param line_number 行号
     * @return int 断点ID，失败返回-1
     */
    int (*set_breakpoint)(Stru_DebuggerInterface_t* self, 
                         const char* file_name, size_t line_number);
    
    /**
     * @brief 删除断点
     * 
     * 删除指定ID的断点。
     * 
     * @param self 接口指针
     * @param breakpoint_id 断点ID
     * @return bool 删除成功返回true，失败返回false
     */
    bool (*remove_breakpoint)(Stru_DebuggerInterface_t* self, int breakpoint_id);
    
    /**
     * @brief 单步执行
     * 
     * 执行单步调试，进入函数调用。
     * 
     * @param self 接口指针
     * @return bool 执行成功返回true，失败返回false
     */
    bool (*step_into)(Stru_DebuggerInterface_t* self);
    
    /**
     * @brief 单步跳过
     * 
     * 执行单步调试，跳过函数调用。
     * 
     * @param self 接口指针
     * @return bool 执行成功返回true，失败返回false
     */
    bool (*step_over)(Stru_DebuggerInterface_t* self);
    
    /**
     * @brief 继续执行
     * 
     * 继续执行程序，直到遇到下一个断点或程序结束。
     * 
     * @param self 接口指针
     * @return bool 执行成功返回true，失败返回false
     */
    bool (*continue_execution)(Stru_DebuggerInterface_t* self);
    
    /**
     * @brief 查看变量值
     * 
     * 查看指定变量的当前值。
     * 
     * @param self 接口指针
     * @param variable_name 变量名
     * @return char* 变量值字符串，需要调用者释放
     */
    char* (*inspect_variable)(Stru_DebuggerInterface_t* self, 
                             const char* variable_name);
    
    /**
     * @brief 获取调用栈
     * 
     * 获取当前的函数调用栈信息。
     * 
     * @param self 接口指针
     * @param stack_depth 栈深度（输出参数）
     * @return char** 调用栈字符串数组，需要调用者释放
     */
    char** (*get_call_stack)(Stru_DebuggerInterface_t* self, size_t* stack_depth);
    
    /**
     * @brief 暂停执行
     * 
     * 暂停正在执行的程序。
     * 
     * @param self 接口指针
     * @return bool 暂停成功返回true，失败返回false
     */
    bool (*pause_execution)(Stru_DebuggerInterface_t* self);
    
    /**
     * @brief 停止调试
     * 
     * 停止调试会话，结束调试。
     * 
     * @param self 接口指针
     * @return bool 停止成功返回true，失败返回false
     */
    bool (*stop_session)(Stru_DebuggerInterface_t* self);
    
    /**
     * @brief 销毁调试器
     * 
     * 清理调试器占用的资源。
     * 
     * @param self 接口指针
     */
    void (*destroy)(Stru_DebuggerInterface_t* self);
};

/**
 * @brief 创建调试器实例
 * 
 * 工厂函数，创建并返回调试器接口的实例。
 * 
 * @return Stru_DebuggerInterface_t* 调试器接口指针，失败返回NULL
 */
Stru_DebuggerInterface_t* F_create_debugger_interface(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_DEBUGGER_INTERFACE_H */
