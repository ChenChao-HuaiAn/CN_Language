/**
 * @file CN_runtime_factory.h
 * @brief 运行时系统工厂接口
 * 
 * 本文件定义了CN_Language项目的运行时系统工厂接口，提供统一的
 * 运行时组件创建和管理。遵循SOLID设计原则和分层架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_RUNTIME_FACTORY_H
#define CN_RUNTIME_FACTORY_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_RuntimeInterface_t Stru_RuntimeInterface_t;
typedef struct Stru_RuntimeMemoryInterface_t Stru_RuntimeMemoryInterface_t;
typedef struct Stru_ExecutionEngineInterface_t Stru_ExecutionEngineInterface_t;
typedef struct Stru_DebugSupportInterface_t Stru_DebugSupportInterface_t;
typedef struct Stru_VirtualMachineInterface_t Stru_VirtualMachineInterface_t;

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
     * @brief 重置运行时系统
     * 
     * @param runtime 运行时实例
     */
    void (*reset)(Stru_RuntimeInterface_t* runtime);
    
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
 * @brief 创建主运行时接口实例
 * 
 * @return Stru_RuntimeInterface_t* 运行时接口实例
 */
Stru_RuntimeInterface_t* F_create_runtime_interface(void);

#endif // CN_RUNTIME_FACTORY_H
