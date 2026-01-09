/**
 * @file CN_virtual_machine_interface.h
 * @brief 虚拟机接口定义
 * 
 * 本文件定义了CN_Language项目的虚拟机接口，整合内存管理、
 * 执行引擎和调试支持，提供完整的虚拟机功能。
 * 遵循SOLID设计原则和分层架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_VIRTUAL_MACHINE_INTERFACE_H
#define CN_VIRTUAL_MACHINE_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_VirtualMachineInterface_t Stru_VirtualMachineInterface_t;
typedef struct Stru_RuntimeMemoryInterface_t Stru_RuntimeMemoryInterface_t;
typedef struct Stru_ExecutionEngineInterface_t Stru_ExecutionEngineInterface_t;
typedef struct Stru_DebugSupportInterface_t Stru_DebugSupportInterface_t;

// ============================================================================
// 虚拟机接口
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
     * @brief 重置虚拟机状态
     * 
     * @param vm 虚拟机实例
     */
    void (*reset)(Stru_VirtualMachineInterface_t* vm);
    
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
// 工厂函数
// ============================================================================

/**
 * @brief 创建虚拟机接口实例
 * 
 * @return Stru_VirtualMachineInterface_t* 虚拟机接口实例
 */
Stru_VirtualMachineInterface_t* F_create_virtual_machine_interface(void);

#endif // CN_VIRTUAL_MACHINE_INTERFACE_H
