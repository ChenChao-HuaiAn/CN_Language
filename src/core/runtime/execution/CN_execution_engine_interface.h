/**
 * @file CN_execution_engine_interface.h
 * @brief 执行引擎接口定义
 * 
 * 本文件定义了CN_Language项目的执行引擎接口，负责执行编译后的字节码
 * 或中间代码，支持解释执行和即时编译。遵循SOLID设计原则和分层架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_EXECUTION_ENGINE_INTERFACE_H
#define CN_EXECUTION_ENGINE_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_ExecutionEngineInterface_t Stru_ExecutionEngineInterface_t;
typedef struct Stru_RuntimeMemoryInterface_t Stru_RuntimeMemoryInterface_t;

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
     * @brief 卸载字节码模块
     * 
     * @param engine 执行引擎实例
     * @param module 模块句柄
     */
    void (*unload_module)(Stru_ExecutionEngineInterface_t* engine,
                         void* module);
    
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
// 工厂函数
// ============================================================================

/**
 * @brief 创建执行引擎接口实例
 * 
 * @return Stru_ExecutionEngineInterface_t* 执行引擎接口实例
 */
Stru_ExecutionEngineInterface_t* F_create_execution_engine_interface(void);

#endif // CN_EXECUTION_ENGINE_INTERFACE_H
