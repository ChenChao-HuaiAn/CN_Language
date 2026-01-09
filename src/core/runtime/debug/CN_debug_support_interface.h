/**
 * @file CN_debug_support_interface.h
 * @brief 调试支持接口定义
 * 
 * 本文件定义了CN_Language项目的调试支持接口，提供运行时调试功能，
 * 包括断点、单步执行、变量检查、调用栈跟踪等。遵循SOLID设计原则。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_DEBUG_SUPPORT_INTERFACE_H
#define CN_DEBUG_SUPPORT_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_DebugSupportInterface_t Stru_DebugSupportInterface_t;
typedef struct Stru_ExecutionEngineInterface_t Stru_ExecutionEngineInterface_t;

// ============================================================================
// 调试支持类型定义
// ============================================================================

/**
 * @brief 断点信息
 */
typedef struct Stru_Breakpoint_t
{
    const char* module_name;      ///< 模块名称
    const char* function_name;    ///< 函数名称
    size_t line_number;           ///< 行号
    size_t instruction_offset;    ///< 指令偏移
    bool enabled;                 ///< 是否启用
    void* user_data;              ///< 用户数据
} Stru_Breakpoint_t;

/**
 * @brief 调试事件类型
 */
typedef enum Eum_DebugEventType
{
    Eum_DEBUG_EVENT_BREAKPOINT_HIT,      ///< 断点命中
    Eum_DEBUG_EVENT_STEP_COMPLETE,       ///< 单步完成
    Eum_DEBUG_EVENT_EXCEPTION,           ///< 异常发生
    Eum_DEBUG_EVENT_PROGRAM_START,       ///< 程序开始
    Eum_DEBUG_EVENT_PROGRAM_END,         ///< 程序结束
    Eum_DEBUG_EVENT_FUNCTION_CALL,       ///< 函数调用
    Eum_DEBUG_EVENT_FUNCTION_RETURN,     ///< 函数返回
    Eum_DEBUG_EVENT_MEMORY_ACCESS        ///< 内存访问
} Eum_DebugEventType;

/**
 * @brief 调试事件
 */
typedef struct Stru_DebugEvent_t
{
    Eum_DebugEventType type;      ///< 事件类型
    void* location;               ///< 事件位置
    const char* message;          ///< 事件消息
    void* context;                ///< 事件上下文
    uint64_t timestamp;           ///< 时间戳
} Stru_DebugEvent_t;

// ============================================================================
// 调试支持接口
// ============================================================================

/**
 * @brief 调试支持接口
 * 
 * 提供运行时调试功能，包括断点、单步执行、变量检查、调用栈跟踪等。
 */
struct Stru_DebugSupportInterface_t
{
    /**
     * @brief 初始化调试器
     * 
     * @param debugger 调试器实例
     * @param engine 执行引擎接口
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_DebugSupportInterface_t* debugger,
                      Stru_ExecutionEngineInterface_t* engine);
    
    /**
     * @brief 设置断点
     * 
     * @param debugger 调试器实例
     * @param breakpoint 断点信息
     * @return bool 设置成功返回true，否则返回false
     */
    bool (*set_breakpoint)(Stru_DebugSupportInterface_t* debugger,
                          const Stru_Breakpoint_t* breakpoint);
    
    /**
     * @brief 清除断点
     * 
     * @param debugger 调试器实例
     * @param breakpoint_id 断点ID
     * @return bool 清除成功返回true，否则返回false
     */
    bool (*clear_breakpoint)(Stru_DebugSupportInterface_t* debugger,
                            int breakpoint_id);
    
    /**
     * @brief 单步执行
     * 
     * @param debugger 调试器实例
     * @param step_type 单步类型（0:步入, 1:步过, 2:步出）
     * @return bool 执行成功返回true，否则返回false
     */
    bool (*step)(Stru_DebugSupportInterface_t* debugger, int step_type);
    
    /**
     * @brief 继续执行
     * 
     * @param debugger 调试器实例
     * @return bool 继续成功返回true，否则返回false
     */
    bool (*continue_execution)(Stru_DebugSupportInterface_t* debugger);
    
    /**
     * @brief 暂停执行
     * 
     * @param debugger 调试器实例
     * @return bool 暂停成功返回true，否则返回false
     */
    bool (*pause_execution)(Stru_DebugSupportInterface_t* debugger);
    
    /**
     * @brief 获取调用栈
     * 
     * @param debugger 调试器实例
     * @param stack_buffer 栈缓冲区
     * @param buffer_size 缓冲区大小
     * @return int 栈帧数量
     */
    int (*get_call_stack)(Stru_DebugSupportInterface_t* debugger,
                         void* stack_buffer, size_t buffer_size);
    
    /**
     * @brief 检查变量值
     * 
     * @param debugger 调试器实例
     * @param variable_name 变量名称
     * @param value_buffer 值缓冲区
     * @param buffer_size 缓冲区大小
     * @return bool 获取成功返回true，否则返回false
     */
    bool (*inspect_variable)(Stru_DebugSupportInterface_t* debugger,
                            const char* variable_name,
                            char* value_buffer, size_t buffer_size);
    
    /**
     * @brief 设置变量值
     * 
     * @param debugger 调试器实例
     * @param variable_name 变量名称
     * @param value 变量值
     * @return bool 设置成功返回true，否则返回false
     */
    bool (*set_variable)(Stru_DebugSupportInterface_t* debugger,
                        const char* variable_name, const char* value);
    
    /**
     * @brief 获取下一个调试事件
     * 
     * @param debugger 调试器实例
     * @param event 输出参数：调试事件
     * @param timeout_ms 超时时间（毫秒）
     * @return bool 获取到事件返回true，否则返回false
     */
    bool (*get_next_event)(Stru_DebugSupportInterface_t* debugger,
                          Stru_DebugEvent_t* event, uint64_t timeout_ms);
    
    /**
     * @brief 销毁调试器
     * 
     * @param debugger 调试器实例
     */
    void (*destroy)(Stru_DebugSupportInterface_t* debugger);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 工厂函数
// ============================================================================

/**
 * @brief 创建调试支持接口实例
 * 
 * @return Stru_DebugSupportInterface_t* 调试支持接口实例
 */
Stru_DebugSupportInterface_t* F_create_debug_support_interface(void);

#endif // CN_DEBUG_SUPPORT_INTERFACE_H
