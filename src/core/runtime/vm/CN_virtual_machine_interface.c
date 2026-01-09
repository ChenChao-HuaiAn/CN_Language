/**
 * @file CN_virtual_machine_interface.c
 * @brief 虚拟机接口实现
 * 
 * 实现虚拟机抽象接口的工厂函数和桩实现。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_virtual_machine_interface.h"
#include "../memory/CN_runtime_memory_interface.h"
#include "../execution/CN_execution_engine_interface.h"
#include "../debug/CN_debug_support_interface.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 虚拟机接口私有数据结构
// ============================================================================

/**
 * @brief 虚拟机接口私有数据
 */
typedef struct Stru_VirtualMachineData_t
{
    size_t heap_size;                          ///< 堆大小
    bool use_gc;                               ///< 是否启用垃圾回收
    bool use_jit;                              ///< 是否启用JIT编译
    Stru_RuntimeMemoryInterface_t* memory;     ///< 内存管理接口
    Stru_ExecutionEngineInterface_t* engine;   ///< 执行引擎接口
    Stru_DebugSupportInterface_t* debugger;    ///< 调试支持接口
} Stru_VirtualMachineData_t;

// ============================================================================
// 虚拟机接口函数实现
// ============================================================================

/**
 * @brief 虚拟机初始化函数
 */
static bool virtual_machine_initialize(Stru_VirtualMachineInterface_t* vm,
                                      size_t heap_size, bool use_gc, bool use_jit)
{
    if (vm == NULL)
    {
        return false;
    }
    
    // 如果private_data已经存在，检查是否可以重新初始化
    if (vm->private_data != NULL)
    {
        Stru_VirtualMachineData_t* data = (Stru_VirtualMachineData_t*)vm->private_data;
        // 如果堆大小为0，说明已经通过reset重置了配置，可以重新初始化
        if (data->heap_size == 0)
        {
            data->heap_size = heap_size;
            data->use_gc = use_gc;
            data->use_jit = use_jit;
            return true;
        }
        return false; // 已经初始化且未重置
    }
    
    Stru_VirtualMachineData_t* data = 
        (Stru_VirtualMachineData_t*)malloc(sizeof(Stru_VirtualMachineData_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->heap_size = heap_size;
    data->use_gc = use_gc;
    data->use_jit = use_jit;
    data->memory = NULL;
    data->engine = NULL;
    data->debugger = NULL;
    
    vm->private_data = data;
    return true;
}

/**
 * @brief 加载并执行模块
 */
static int virtual_machine_load_and_execute(Stru_VirtualMachineInterface_t* vm,
                                           const uint8_t* bytecode, size_t bytecode_size,
                                           const char* module_name, int argc, void** argv)
{
    (void)vm;
    (void)module_name;
    (void)argc;
    (void)argv;
    
    // 桩实现：如果字节码为空或大小为0，返回错误
    if (bytecode == NULL || bytecode_size == 0)
    {
        return -1; // 错误退出码
    }
    
    // 桩实现：返回成功退出码
    return 0;
}

/**
 * @brief 获取内存管理接口
 */
static Stru_RuntimeMemoryInterface_t* virtual_machine_get_memory_interface(
    Stru_VirtualMachineInterface_t* vm)
{
    if (vm == NULL || vm->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_VirtualMachineData_t* data = (Stru_VirtualMachineData_t*)vm->private_data;
    
    if (data->memory == NULL)
    {
        data->memory = F_create_runtime_memory_interface();
        if (data->memory != NULL)
        {
            data->memory->initialize(data->memory, data->heap_size, data->use_gc);
        }
    }
    
    return data->memory;
}

/**
 * @brief 获取执行引擎接口
 */
static Stru_ExecutionEngineInterface_t* virtual_machine_get_execution_engine(
    Stru_VirtualMachineInterface_t* vm)
{
    if (vm == NULL || vm->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_VirtualMachineData_t* data = (Stru_VirtualMachineData_t*)vm->private_data;
    
    if (data->engine == NULL)
    {
        data->engine = F_create_execution_engine_interface();
        if (data->engine != NULL)
        {
            Stru_RuntimeMemoryInterface_t* memory = virtual_machine_get_memory_interface(vm);
            data->engine->initialize(data->engine, memory, data->use_jit);
        }
    }
    
    return data->engine;
}

/**
 * @brief 获取调试支持接口
 */
static Stru_DebugSupportInterface_t* virtual_machine_get_debug_support(
    Stru_VirtualMachineInterface_t* vm)
{
    if (vm == NULL || vm->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_VirtualMachineData_t* data = (Stru_VirtualMachineData_t*)vm->private_data;
    
    if (data->debugger == NULL)
    {
        data->debugger = F_create_debug_support_interface();
        if (data->debugger != NULL)
        {
            Stru_ExecutionEngineInterface_t* engine = virtual_machine_get_execution_engine(vm);
            data->debugger->initialize(data->debugger, engine);
        }
    }
    
    return data->debugger;
}

/**
 * @brief 重置虚拟机状态
 */
static void virtual_machine_reset(Stru_VirtualMachineInterface_t* vm)
{
    if (vm == NULL || vm->private_data == NULL)
    {
        return;
    }
    
    Stru_VirtualMachineData_t* data = (Stru_VirtualMachineData_t*)vm->private_data;
    
    if (data->debugger != NULL)
    {
        // 调试器没有重置函数，直接销毁并重新创建
        data->debugger->destroy(data->debugger);
        data->debugger = NULL;
    }
    
    if (data->engine != NULL)
    {
        data->engine->reset(data->engine);
    }
    
    if (data->memory != NULL)
    {
        data->memory->reset(data->memory);
    }
    
    // 重置配置但不释放private_data，允许重新初始化
    data->heap_size = 0;
    data->use_gc = false;
    data->use_jit = false;
}

/**
 * @brief 销毁虚拟机
 */
static void virtual_machine_destroy(Stru_VirtualMachineInterface_t* vm)
{
    if (vm == NULL)
    {
        return;
    }
    
    if (vm->private_data != NULL)
    {
        Stru_VirtualMachineData_t* data = (Stru_VirtualMachineData_t*)vm->private_data;
        
        if (data->debugger != NULL)
        {
            data->debugger->destroy(data->debugger);
        }
        
        if (data->engine != NULL)
        {
            data->engine->destroy(data->engine);
        }
        
        if (data->memory != NULL)
        {
            data->memory->destroy(data->memory);
        }
        
        free(data);
        vm->private_data = NULL;
    }
}

// ============================================================================
// 工厂函数实现
// ============================================================================

/**
 * @brief 创建虚拟机接口实例
 */
Stru_VirtualMachineInterface_t* F_create_virtual_machine_interface(void)
{
    Stru_VirtualMachineInterface_t* interface = 
        (Stru_VirtualMachineInterface_t*)malloc(sizeof(Stru_VirtualMachineInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = virtual_machine_initialize;
    interface->load_and_execute = virtual_machine_load_and_execute;
    interface->get_memory_interface = virtual_machine_get_memory_interface;
    interface->get_execution_engine = virtual_machine_get_execution_engine;
    interface->get_debug_support = virtual_machine_get_debug_support;
    interface->reset = virtual_machine_reset;
    interface->destroy = virtual_machine_destroy;
    interface->private_data = NULL;
    
    return interface;
}
