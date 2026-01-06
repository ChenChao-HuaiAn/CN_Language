/**
 * @file CN_runtime_interface.h
 * @brief 运行时系统抽象接口定义
 * 
 * 本文件定义了CN_Language项目的运行时系统接口，包括内存管理、执行引擎、
 * 调试支持等功能。遵循SOLID设计原则和分层架构，提供可扩展、可替换的
 * 运行时实现。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_RUNTIME_INTERFACE_H
#define CN_RUNTIME_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_RuntimeInterface_t Stru_RuntimeInterface_t;
typedef struct Stru_ExecutionEngineInterface_t Stru_ExecutionEngineInterface_t;
typedef struct Stru_DebugSupportInterface_t Stru_DebugSupportInterface_t;
typedef struct Stru_RuntimeMemoryInterface_t Stru_RuntimeMemoryInterface_t;
typedef struct Stru_VirtualMachineInterface_t Stru_VirtualMachineInterface_t;

// ============================================================================
// 运行时内存管理接口
// ============================================================================

/**
 * @brief 运行时内存管理接口
 * 
 * 专门为运行时系统设计的内存管理接口，支持垃圾回收、内存池、
 * 对象生命周期管理等高级功能。
 */
struct Stru_RuntimeMemoryInterface_t
{
    /**
     * @brief 初始化运行时内存管理器
     * 
     * @param memory_interface 内存接口实例
     * @param heap_size 堆大小（字节）
     * @param use_gc 是否启用垃圾回收
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_RuntimeMemoryInterface_t* memory_interface, 
                      size_t heap_size, bool use_gc);
    
    /**
     * @brief 分配运行时对象内存
     * 
     * @param memory_interface 内存接口实例
     * @param size 对象大小（字节）
     * @param type_tag 对象类型标签
     * @return void* 分配的对象指针，失败返回NULL
     */
    void* (*allocate_object)(Stru_RuntimeMemoryInterface_t* memory_interface, 
                            size_t size, uint32_t type_tag);
    
    /**
     * @brief 分配运行时数组内存
     * 
     * @param memory_interface 内存接口实例
     * @param element_size 元素大小（字节）
     * @param element_count 元素数量
     * @param type_tag 元素类型标签
     * @return void* 分配的数组指针，失败返回NULL
     */
    void* (*allocate_array)(Stru_RuntimeMemoryInterface_t* memory_interface,
                           size_t element_size, size_t element_count,
                           uint32_t type_tag);
    
    /**
     * @brief 标记对象为活动状态（用于垃圾回收）
     * 
     * @param memory_interface 内存接口实例
     * @param object 要标记的对象指针
     */
    void (*mark_object)(Stru_RuntimeMemoryInterface_t* memory_interface, 
                       void* object);
    
    /**
     * @brief 执行垃圾回收
     * 
     * @param memory_interface 内存接口实例
     * @return size_t 回收的字节数
     */
    size_t (*collect_garbage)(Stru_RuntimeMemoryInterface_t* memory_interface);
    
    /**
     * @brief 获取内存使用统计
     * 
     * @param memory_interface 内存接口实例
     * @param total_heap 输出参数：堆总大小
     * @param used_heap 输出参数：已使用堆大小
     * @param object_count 输出参数：对象数量
     * @param gc_count 输出参数：垃圾回收次数
     */
    void (*get_memory_stats)(Stru_RuntimeMemoryInterface_t* memory_interface,
                            size_t* total_heap, size_t* used_heap,
                            size_t* object_count, size_t* gc_count);
    
    /**
     * @brief 销毁运行时内存管理器
     * 
     * @param memory_interface 内存接口实例
     */
    void (*destroy)(Stru_RuntimeMemoryInterface_t* memory_interface);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 执行引擎接口
// ============================================================================

/**
 * @brief 执行引擎接口
 * 
 * 负责执行编译后的字节码或中间代码，支持解释执行和即时编译。
 */
struct Stru_ExecutionEngineInterface_t
{
    /**
     * @brief 初始化执行引擎
     * 
     * @param engine 执行引擎实例
     * @param memory_interface 内存管理接口
     * @param use_jit 是否启用JIT编译
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_ExecutionEngineInterface_t* engine,
                      Stru_RuntimeMemoryInterface_t* memory_interface,
                      bool use_jit);
    
    /**
     * @brief 加载字节码模块
     * 
     * @param engine 执行引擎实例
     * @param bytecode 字节码数据
     * @param bytecode_size 字节码大小
     * @param module_name 模块名称
     * @return void* 模块句柄，失败返回NULL
     */
    void* (*load_module)(Stru_ExecutionEngineInterface_t* engine,
                        const uint8_t* bytecode, size_t bytecode_size,
                        const char* module_name);
    
    /**
     * @brief 执行函数
     * 
     * @param engine 执行引擎实例
     * @param module 模块句柄
     * @param function_name 函数名称
     * @param argc 参数数量
     * @param argv 参数数组
     * @return void* 函数返回值
     */
    void* (*execute_function)(Stru_ExecutionEngineInterface_t* engine,
                             void* module, const char* function_name,
                             int argc, void** argv);
    
    /**
     * @brief 执行字节码片段
     * 
     * @param engine 执行引擎实例
     * @param bytecode 字节码数据
     * @param bytecode_size 字节码大小
     * @param context 执行上下文
     * @return void* 执行结果
     */
    void* (*execute_bytecode)(Stru_ExecutionEngineInterface_t* engine,
                             const uint8_t* bytecode, size_t bytecode_size,
                             void* context);
    
    /**
     * @brief 设置执行超时
     * 
     * @param engine 执行引擎实例
     * @param timeout_ms 超时时间（毫秒），0表示无超时
     */
    void (*set_timeout)(Stru_ExecutionEngineInterface_t* engine,
                       uint64_t timeout_ms);
    
    /**
     * @brief 获取执行统计信息
     * 
     * @param engine 执行引擎实例
     * @param instructions_executed 输出参数：执行的指令数
     * @param execution_time_ms 输出参数：执行时间（毫秒）
     * @param memory_used 输出参数：使用的内存量
     */
    void (*get_execution_stats)(Stru_ExecutionEngineInterface_t* engine,
                               uint64_t* instructions_executed,
                               uint64_t* execution_time_ms,
                               size_t* memory_used);
    
    /**
     * @brief 重置执行引擎状态
     * 
     * @param engine 执行引擎实例
     */
    void (*reset)(Stru_ExecutionEngineInterface_t* engine);
    
    /**
     * @brief 销毁执行引擎
     * 
     * @param engine 执行引擎实例
     */
    void (*destroy)(Stru_ExecutionEngineInterface_t* engine);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 调试支持接口
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
// 虚拟机接口（可选，用于更高级的运行时）
// ============================================================================

/**
 * @brief 虚拟机接口
 * 
 * 完整的虚拟机接口，整合内存管理、执行引擎和调试支持。
 */
struct Stru_VirtualMachineInterface_t
{
    /**
     * @brief 初始化虚拟机
     * 
     * @param vm 虚拟机实例
     * @param heap_size 堆大小
     * @param use_gc 是否启用垃圾回收
     * @param use_jit 是否启用JIT编译
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_VirtualMachineInterface_t* vm,
                      size_t heap_size, bool use_gc, bool use_jit);
    
    /**
     * @brief 加载并执行模块
     * 
     * @param vm 虚拟机实例
     * @param bytecode 字节码数据
     * @param bytecode_size 字节码大小
     * @param module_name 模块名称
     * @param argc 参数数量
     * @param argv 参数数组
     * @return int 执行结果（退出码）
     */
    int (*load_and_execute)(Stru_VirtualMachineInterface_t* vm,
                           const uint8_t* bytecode, size_t bytecode_size,
                           const char* module_name, int argc, void** argv);
    
    /**
     * @brief 获取内存管理接口
     * 
     * @param vm 虚拟机实例
     * @return Stru_RuntimeMemoryInterface_t* 内存管理接口
     */
    Stru_RuntimeMemoryInterface_t* (*get_memory_interface)(Stru_VirtualMachineInterface_t* vm);
    
    /**
     * @brief 获取执行引擎接口
     * 
     * @param vm 虚拟机实例
     * @return Stru_ExecutionEngineInterface_t* 执行引擎接口
     */
    Stru_ExecutionEngineInterface_t* (*get_execution_engine)(Stru_VirtualMachineInterface_t* vm);
    
    /**
     * @brief 获取调试支持接口
     * 
     * @param vm 虚拟机实例
     * @return Stru_DebugSupportInterface_t* 调试支持接口
     */
    Stru_DebugSupportInterface_t* (*get_debug_support)(Stru_VirtualMachineInterface_t* vm);
    
    /**
     * @brief 销毁虚拟机
     * 
     * @param vm 虚拟机实例
     */
    void (*destroy)(Stru_VirtualMachineInterface_t* vm);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 主运行时接口
// ============================================================================

/**
 * @brief 运行时系统主接口
 * 
 * 整合所有运行时组件，提供统一的运行时系统访问接口。
 */
struct Stru_RuntimeInterface_t
{
    /**
     * @brief 初始化运行时系统
     * 
     * @param runtime 运行时实例
     * @param config 配置字符串（JSON格式）
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_RuntimeInterface_t* runtime, const char* config);
    
    /**
     * @brief 获取内存管理接口
     * 
     * @param runtime 运行时实例
     * @return Stru_RuntimeMemoryInterface_t* 内存管理接口
     */
    Stru_RuntimeMemoryInterface_t* (*get_memory_interface)(Stru_RuntimeInterface_t* runtime);
    
    /**
     * @brief 获取执行引擎接口
     * 
     * @param runtime 运行时实例
     * @return Stru_ExecutionEngineInterface_t* 执行引擎接口
     */
    Stru_ExecutionEngineInterface_t* (*get_execution_engine)(Stru_RuntimeInterface_t* runtime);
    
    /**
     * @brief 获取调试支持接口
     * 
     * @param runtime 运行时实例
     * @return Stru_DebugSupportInterface_t* 调试支持接口
     */
    Stru_DebugSupportInterface_t* (*get_debug_support)(Stru_RuntimeInterface_t* runtime);
    
    /**
     * @brief 获取虚拟机接口
     * 
     * @param runtime 运行时实例
     * @return Stru_VirtualMachineInterface_t* 虚拟机接口
     */
    Stru_VirtualMachineInterface_t* (*get_virtual_machine)(Stru_RuntimeInterface_t* runtime);
    
    /**
     * @brief 执行CN语言程序
     * 
     * @param runtime 运行时实例
     * @param bytecode 字节码数据
     * @param bytecode_size 字节码大小
     * @param argc 参数数量
     * @param argv 参数数组
     * @return int 程序退出码
     */
    int (*execute_program)(Stru_RuntimeInterface_t* runtime,
                          const uint8_t* bytecode, size_t bytecode_size,
                          int argc, char** argv);
    
    /**
     * @brief 销毁运行时系统
     * 
     * @param runtime 运行时实例
     */
    void (*destroy)(Stru_RuntimeInterface_t* runtime);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 工厂函数
// ============================================================================

/**
 * @brief 创建运行时内存管理接口实例
 * 
 * @return Stru_RuntimeMemoryInterface_t* 内存管理接口实例
 */
Stru_RuntimeMemoryInterface_t* F_create_runtime_memory_interface(void);

/**
 * @brief 创建执行引擎接口实例
 * 
 * @return Stru_ExecutionEngineInterface_t* 执行引擎接口实例
 */
Stru_ExecutionEngineInterface_t* F_create_execution_engine_interface(void);

/**
 * @brief 创建调试支持接口实例
 * 
 * @return Stru_DebugSupportInterface_t* 调试支持接口实例
 */
Stru_DebugSupportInterface_t* F_create_debug_support_interface(void);

/**
 * @brief 创建虚拟机接口实例
 * 
 * @return Stru_VirtualMachineInterface_t* 虚拟机接口实例
 */
Stru_VirtualMachineInterface_t* F_create_virtual_machine_interface(void);

/**
 * @brief 创建主运行时接口实例
 * 
 * @return Stru_RuntimeInterface_t* 运行时接口实例
 */
Stru_RuntimeInterface_t* F_create_runtime_interface(void);

#endif // CN_RUNTIME_INTERFACE_H
