/******************************************************************************
 * 文件名: CN_bytecode_interpreter.h
 * 功能: CN_Language 字节码解释器接口
 * 
 * 定义字节码解释器的公共接口，提供字节码程序的加载、解释执行和调试功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_BYTECODE_INTERPRETER_H
#define CN_BYTECODE_INTERPRETER_H

#include "CN_bytecode_backend.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 字节码解释器配置结构体
// ============================================================================

/**
 * @brief 字节码解释器配置结构体
 * 
 * 配置字节码解释器的运行时参数。
 */
typedef struct Stru_BytecodeInterpreterConfig_t {
    size_t stack_size;               ///< 栈大小（字节）
    size_t heap_size;                ///< 堆大小（字节）
    size_t max_call_depth;           ///< 最大调用深度
    size_t max_instruction_count;    ///< 最大指令执行数（防止无限循环）
    bool enable_gc;                  ///< 是否启用垃圾回收
    bool enable_profiling;           ///< 是否启用性能分析
    bool enable_debugging;           ///< 是否启用调试支持
    bool enable_tracing;             ///< 是否启用执行跟踪
    const char* log_level;           ///< 日志级别 ("none", "error", "warning", "info", "debug")
} Stru_BytecodeInterpreterConfig_t;

// ============================================================================
// 字节码值类型枚举
// ============================================================================

/**
 * @brief 字节码值类型枚举
 * 
 * 定义字节码解释器中支持的值类型。
 */
typedef enum Eum_BytecodeValueType {
    Eum_BCV_NULL = 0,        ///< 空值
    Eum_BCV_BOOL,            ///< 布尔值
    Eum_BCV_INT8,            ///< 8位整数
    Eum_BCV_INT16,           ///< 16位整数
    Eum_BCV_INT32,           ///< 32位整数
    Eum_BCV_INT64,           ///< 64位整数
    Eum_BCV_UINT8,           ///< 8位无符号整数
    Eum_BCV_UINT16,          ///< 16位无符号整数
    Eum_BCV_UINT32,          ///< 32位无符号整数
    Eum_BCV_UINT64,          ///< 64位无符号整数
    Eum_BCV_FLOAT32,         ///< 32位浮点数
    Eum_BCV_FLOAT64,         ///< 64位浮点数
    Eum_BCV_STRING,          ///< 字符串
    Eum_BCV_ARRAY,           ///< 数组
    Eum_BCV_OBJECT,          ///< 对象
    Eum_BCV_FUNCTION,        ///< 函数
    Eum_BCV_NATIVE_FUNCTION, ///< 原生函数
    Eum_BCV_CLOSURE,         ///< 闭包
    Eum_BCV_POINTER          ///< 指针
} Eum_BytecodeValueType;

// ============================================================================
// 字节码值结构体
// ============================================================================

/**
 * @brief 字节码值结构体
 * 
 * 表示字节码解释器中的值，使用联合体存储不同类型的值。
 */
typedef struct Stru_BytecodeValue_t {
    Eum_BytecodeValueType type;      ///< 值类型
    union {
        bool bool_value;             ///< 布尔值
        int8_t int8_value;           ///< 8位整数
        int16_t int16_value;         ///< 16位整数
        int32_t int32_value;         ///< 32位整数
        int64_t int64_value;         ///< 64位整数
        uint8_t uint8_value;         ///< 8位无符号整数
        uint16_t uint16_value;       ///< 16位无符号整数
        uint32_t uint32_value;       ///< 32位无符号整数
        uint64_t uint64_value;       ///< 64位无符号整数
        float float32_value;         ///< 32位浮点数
        double float64_value;        ///< 64位浮点数
        char* string_value;          ///< 字符串值
        void* pointer_value;         ///< 指针值
        void* object_value;          ///< 对象值
        void* array_value;           ///< 数组值
        void* function_value;        ///< 函数值
    } data;                          ///< 值数据
    size_t ref_count;                ///< 引用计数（用于垃圾回收）
    void* extra_data;                ///< 额外数据
} Stru_BytecodeValue_t;

// ============================================================================
// 字节码解释器状态枚举
// ============================================================================

/**
 * @brief 字节码解释器状态枚举
 * 
 * 定义字节码解释器的执行状态。
 */
typedef enum Eum_BytecodeInterpreterState {
    Eum_BCI_STATE_CREATED = 0,       ///< 已创建
    Eum_BCI_STATE_INITIALIZED,       ///< 已初始化
    Eum_BCI_STATE_RUNNING,           ///< 运行中
    Eum_BCI_STATE_PAUSED,            ///< 已暂停
    Eum_BCI_STATE_STEPPING,          ///< 单步执行中
    Eum_BCI_STATE_BREAKPOINT,        ///< 断点命中
    Eum_BCI_STATE_ERROR,             ///< 错误状态
    Eum_BCI_STATE_FINISHED,          ///< 执行完成
    Eum_BCI_STATE_TERMINATED         ///< 已终止
} Eum_BytecodeInterpreterState;

// ============================================================================
// 字节码解释器结构体
// ============================================================================

/**
 * @brief 字节码解释器结构体
 * 
 * 表示字节码解释器的实例，包含执行状态和运行时数据。
 */
typedef struct Stru_BytecodeInterpreter_t {
    // 状态信息
    Eum_BytecodeInterpreterState state;          ///< 当前状态
    bool has_errors;                             ///< 是否有错误
    bool has_warnings;                           ///< 是否有警告
    
    // 程序信息
    Stru_BytecodeProgram_t* program;             ///< 当前执行的字节码程序
    uint32_t program_counter;                    ///< 程序计数器（指令索引）
    uint32_t call_stack_depth;                   ///< 调用栈深度
    
    // 运行时数据
    Stru_BytecodeValue_t* stack;                 ///< 值栈
    size_t stack_top;                            ///< 栈顶指针
    size_t stack_capacity;                       ///< 栈容量
    
    Stru_BytecodeValue_t* globals;               ///< 全局变量数组
    size_t global_count;                         ///< 全局变量数量
    
    void** call_stack;                           ///< 调用栈（帧指针数组）
    size_t call_stack_capacity;                  ///< 调用栈容量
    
    // 调试信息
    bool breakpoints_enabled;                    ///< 断点是否启用
    uint32_t* breakpoints;                       ///< 断点数组（指令偏移）
    size_t breakpoint_count;                     ///< 断点数量
    
    // 性能分析
    uint64_t instruction_count;                  ///< 已执行指令计数
    uint64_t start_time;                         ///< 开始时间（毫秒）
    uint64_t total_time;                         ///< 总执行时间（毫秒）
    
    // 错误处理
    char** error_messages;                       ///< 错误消息数组
    size_t error_count;                          ///< 错误数量
    size_t error_capacity;                       ///< 错误数组容量
    
    // 内部状态
    void* internal_state;                        ///< 内部状态
} Stru_BytecodeInterpreter_t;

// ============================================================================
// 字节码解释器接口函数
// ============================================================================

/**
 * @brief 创建字节码解释器
 * 
 * 创建并返回一个新的字节码解释器实例。
 * 
 * @param config 解释器配置
 * @return Stru_BytecodeInterpreter_t* 新创建的字节码解释器实例
 * 
 * @note 调用者负责在不再使用时调用F_destroy_bytecode_interpreter()销毁实例。
 */
Stru_BytecodeInterpreter_t* F_create_bytecode_interpreter(
    const Stru_BytecodeInterpreterConfig_t* config);

/**
 * @brief 销毁字节码解释器
 * 
 * 释放字节码解释器占用的所有资源。
 * 
 * @param interpreter 要销毁的字节码解释器
 */
void F_destroy_bytecode_interpreter(Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 加载字节码程序
 * 
 * 将字节码程序加载到解释器中，准备执行。
 * 
 * @param interpreter 字节码解释器
 * @param program 要加载的字节码程序
 * @return 加载成功返回true，失败返回false
 */
bool F_load_bytecode_program(Stru_BytecodeInterpreter_t* interpreter,
                             Stru_BytecodeProgram_t* program);

/**
 * @brief 执行字节码程序
 * 
 * 执行已加载的字节码程序。
 * 
 * @param interpreter 字节码解释器
 * @return 执行成功返回true，失败返回false
 * 
 * @note 此函数会阻塞直到程序执行完成或发生错误。
 */
bool F_execute_bytecode_program(Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 单步执行字节码程序
 * 
 * 执行下一条指令并暂停。
 * 
 * @param interpreter 字节码解释器
 * @return 执行成功返回true，失败返回false
 */
bool F_step_bytecode_program(Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 继续执行字节码程序
 * 
 * 从当前暂停状态继续执行。
 * 
 * @param interpreter 字节码解释器
 * @return 继续执行成功返回true，失败返回false
 */
bool F_continue_bytecode_program(Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 暂停字节码程序执行
 * 
 * 暂停当前正在执行的字节码程序。
 * 
 * @param interpreter 字节码解释器
 * @return 暂停成功返回true，失败返回false
 */
bool F_pause_bytecode_program(Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 停止字节码程序执行
 * 
 * 停止当前正在执行的字节码程序。
 * 
 * @param interpreter 字节码解释器
 * @return 停止成功返回true，失败返回false
 */
bool F_stop_bytecode_program(Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 重置字节码解释器
 * 
 * 重置解释器到初始状态，清除所有运行时数据。
 * 
 * @param interpreter 字节码解释器
 */
void F_reset_bytecode_interpreter(Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 获取解释器状态
 * 
 * 获取字节码解释器的当前状态。
 * 
 * @param interpreter 字节码解释器
 * @return Eum_BytecodeInterpreterState 解释器状态
 */
Eum_BytecodeInterpreterState F_get_interpreter_state(
    const Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 检查解释器是否有错误
 * 
 * 检查字节码解释器是否有错误。
 * 
 * @param interpreter 字节码解释器
 * @return 有错误返回true，否则返回false
 */
bool F_interpreter_has_errors(const Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 获取解释器错误信息
 * 
 * 获取字节码解释器的所有错误信息。
 * 
 * @param interpreter 字节码解释器
 * @param errors 输出参数，错误信息数组
 * @param error_count 输出参数，错误数量
 */
void F_get_interpreter_errors(const Stru_BytecodeInterpreter_t* interpreter,
                              char*** errors, size_t* error_count);

/**
 * @brief 清除解释器错误
 * 
 * 清除字节码解释器的所有错误信息。
 * 
 * @param interpreter 字节码解释器
 */
void F_clear_interpreter_errors(Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 设置断点
 * 
 * 在指定指令偏移处设置断点。
 * 
 * @param interpreter 字节码解释器
 * @param instruction_offset 指令偏移
 * @return 设置成功返回true，失败返回false
 */
bool F_set_breakpoint(Stru_BytecodeInterpreter_t* interpreter,
                      uint32_t instruction_offset);

/**
 * @brief 清除断点
 * 
 * 清除指定指令偏移处的断点。
 * 
 * @param interpreter 字节码解释器
 * @param instruction_offset 指令偏移
 * @return 清除成功返回true，失败返回false
 */
bool F_clear_breakpoint(Stru_BytecodeInterpreter_t* interpreter,
                        uint32_t instruction_offset);

/**
 * @brief 清除所有断点
 * 
 * 清除所有断点。
 * 
 * @param interpreter 字节码解释器
 */
void F_clear_all_breakpoints(Stru_BytecodeInterpreter_t* interpreter);

/**
 * @brief 获取当前栈帧信息
 * 
 * 获取当前调用栈帧的信息。
 * 
 * @param interpreter 字节码解释器
 * @param frame_index 栈帧索引（0表示当前帧）
 * @param function_name 输出参数，函数名
 * @param line_number 输出参数，行号
 * @param instruction_offset 输出参数，指令偏移
 * @return 获取成功返回true，失败返回false
 */
bool F_get_current_stack_frame(const Stru_BytecodeInterpreter_t* interpreter,
                               size_t frame_index,
                               const char** function_name,
                               uint32_t* line_number,
                               uint32_t* instruction_offset);

/**
 * @brief 获取栈跟踪
 * 
 * 获取当前的调用栈跟踪。
 * 
 * @param interpreter 字节码解释器
 * @param frames 输出参数，栈帧数组
 * @param frame_count 输出参数，栈帧数量
 * @return 获取成功返回true，失败返回false
 */
bool F_get_interpreter_stack_trace(const Stru_BytecodeInterpreter_t* interpreter,
                                   void*** frames, size_t* frame_count);

/**
 * @brief 获取性能分析数据
 * 
 * 获取字节码解释器的性能分析数据。
 * 
 * @param interpreter 字节码解释器
 * @param instruction_count 输出参数，已执行指令计数
 * @param execution_time 输出参数，执行时间（毫秒）
 * @param memory_usage 输出参数，内存使用量（字节）
 */
void F_get_profiling_data(const Stru_BytecodeInterpreter_t* interpreter,
                          uint64_t* instruction_count,
                          uint64_t* execution_time,
                          size_t* memory_usage);

/**
 * @brief 创建默认字节码解释器配置
 * 
 * 创建并返回默认的字节码解释器配置。
 * 
 * @return Stru_BytecodeInterpreterConfig_t 默认字节码解释器配置
 */
Stru_BytecodeInterpreterConfig_t F_create_default_bytecode_interpreter_config(void);

/**
 * @brief 获取字节码解释器版本信息
 * 
 * 返回字节码解释器的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 */
void F_get_bytecode_interpreter_version(int* major, int* minor, int* patch);

/**
 * @brief 获取字节码解释器版本字符串
 * 
 * 返回字节码解释器的版本字符串。
 * 
 * @return 版本字符串
 */
const char* F_get_bytecode_interpreter_version_string(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_BYTECODE_INTERPRETER_H */
