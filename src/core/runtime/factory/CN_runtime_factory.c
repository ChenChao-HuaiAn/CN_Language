/**
 * @file CN_runtime_factory.c
 * @brief 运行时系统工厂实现
 * 
 * 实现运行时系统工厂接口，整合所有运行时组件。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_runtime_factory.h"
#include "../memory/CN_runtime_memory_interface.h"
#include "../execution/CN_execution_engine_interface.h"
#include "../debug/CN_debug_support_interface.h"
#include "../vm/CN_virtual_machine_interface.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 主运行时接口私有数据结构
// ============================================================================

/**
 * @brief 主运行时接口私有数据
 */
typedef struct Stru_RuntimeData_t
{
    char* config;                               ///< 配置字符串
    Stru_RuntimeMemoryInterface_t* memory;      ///< 内存管理接口
    Stru_ExecutionEngineInterface_t* engine;    ///< 执行引擎接口
    Stru_DebugSupportInterface_t* debugger;     ///< 调试支持接口
    Stru_VirtualMachineInterface_t* vm;         ///< 虚拟机接口
} Stru_RuntimeData_t;

// ============================================================================
// 主运行时接口函数实现
// ============================================================================

/**
 * @brief 运行时系统初始化函数
 */
static bool runtime_initialize(Stru_RuntimeInterface_t* runtime, const char* config)
{
    if (runtime == NULL)
    {
        return false;
    }
    
    // 如果private_data已经存在，检查是否可以重新初始化
    if (runtime->private_data != NULL)
    {
        Stru_RuntimeData_t* data = (Stru_RuntimeData_t*)runtime->private_data;
        // 如果config为NULL，说明已经通过reset重置了，可以重新初始化
        if (data->config == NULL)
        {
            // 保存新的配置字符串
            if (config != NULL)
            {
                size_t len = strlen(config) + 1;
                data->config = (char*)malloc(len);
                if (data->config != NULL)
                {
                    strcpy(data->config, config);
                }
            }
            return true;
        }
        return false; // 已经初始化且未重置
    }
    
    Stru_RuntimeData_t* data = 
        (Stru_RuntimeData_t*)malloc(sizeof(Stru_RuntimeData_t));
    if (data == NULL)
    {
        return false;
    }
    
    // 保存配置字符串
    if (config != NULL)
    {
        size_t len = strlen(config) + 1;
        data->config = (char*)malloc(len);
        if (data->config != NULL)
        {
            strcpy(data->config, config);
        }
    }
    else
    {
        data->config = NULL;
    }
    
    data->memory = NULL;
    data->engine = NULL;
    data->debugger = NULL;
    data->vm = NULL;
    
    runtime->private_data = data;
    return true;
}

/**
 * @brief 获取内存管理接口
 */
static Stru_RuntimeMemoryInterface_t* runtime_get_memory_interface(
    Stru_RuntimeInterface_t* runtime)
{
    if (runtime == NULL || runtime->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_RuntimeData_t* data = (Stru_RuntimeData_t*)runtime->private_data;
    
    if (data->memory == NULL)
    {
        data->memory = F_create_runtime_memory_interface();
        // 使用默认配置初始化
        if (data->memory != NULL)
        {
            data->memory->initialize(data->memory, 1024 * 1024, false); // 1MB堆，无GC
        }
    }
    
    return data->memory;
}

/**
 * @brief 获取执行引擎接口
 */
static Stru_ExecutionEngineInterface_t* runtime_get_execution_engine(
    Stru_RuntimeInterface_t* runtime)
{
    if (runtime == NULL || runtime->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_RuntimeData_t* data = (Stru_RuntimeData_t*)runtime->private_data;
    
    if (data->engine == NULL)
    {
        data->engine = F_create_execution_engine_interface();
        // 使用默认配置初始化
        if (data->engine != NULL)
        {
            Stru_RuntimeMemoryInterface_t* memory = runtime_get_memory_interface(runtime);
            data->engine->initialize(data->engine, memory, false); // 无JIT
        }
    }
    
    return data->engine;
}

/**
 * @brief 获取调试支持接口
 */
static Stru_DebugSupportInterface_t* runtime_get_debug_support(
    Stru_RuntimeInterface_t* runtime)
{
    if (runtime == NULL || runtime->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_RuntimeData_t* data = (Stru_RuntimeData_t*)runtime->private_data;
    
    if (data->debugger == NULL)
    {
        data->debugger = F_create_debug_support_interface();
        // 使用默认配置初始化
        if (data->debugger != NULL)
        {
            Stru_ExecutionEngineInterface_t* engine = runtime_get_execution_engine(runtime);
            data->debugger->initialize(data->debugger, engine);
        }
    }
    
    return data->debugger;
}

/**
 * @brief 获取虚拟机接口
 */
static Stru_VirtualMachineInterface_t* runtime_get_virtual_machine(
    Stru_RuntimeInterface_t* runtime)
{
    if (runtime == NULL || runtime->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_RuntimeData_t* data = (Stru_RuntimeData_t*)runtime->private_data;
    
    if (data->vm == NULL)
    {
        data->vm = F_create_virtual_machine_interface();
        // 使用默认配置初始化
        if (data->vm != NULL)
        {
            data->vm->initialize(data->vm, 1024 * 1024, false, false); // 1MB堆，无GC，无JIT
        }
    }
    
    return data->vm;
}

/**
 * @brief 执行CN语言程序
 */
static int runtime_execute_program(Stru_RuntimeInterface_t* runtime,
                                  const uint8_t* bytecode, size_t bytecode_size,
                                  int argc, char** argv)
{
    if (runtime == NULL || runtime->private_data == NULL)
    {
        return -1;
    }
    
    Stru_RuntimeData_t* data = (Stru_RuntimeData_t*)runtime->private_data;
    
    // 使用虚拟机执行程序
    if (data->vm == NULL)
    {
        data->vm = runtime_get_virtual_machine(runtime);
    }
    
    if (data->vm != NULL)
    {
        return data->vm->load_and_execute(data->vm, bytecode, bytecode_size, 
                                         "main.cn", argc, (void**)argv);
    }
    
    return -1;
}

/**
 * @brief 重置运行时系统
 */
static void runtime_reset(Stru_RuntimeInterface_t* runtime)
{
    if (runtime == NULL || runtime->private_data == NULL)
    {
        return;
    }
    
    Stru_RuntimeData_t* data = (Stru_RuntimeData_t*)runtime->private_data;
    
    if (data->vm != NULL)
    {
        data->vm->reset(data->vm);
    }
    
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
    
    // 清除配置，允许重新初始化
    if (data->config != NULL)
    {
        free(data->config);
        data->config = NULL;
    }
}

/**
 * @brief 销毁运行时系统
 */
static void runtime_destroy(Stru_RuntimeInterface_t* runtime)
{
    if (runtime == NULL)
    {
        return;
    }
    
    if (runtime->private_data != NULL)
    {
        Stru_RuntimeData_t* data = (Stru_RuntimeData_t*)runtime->private_data;
        
        if (data->vm != NULL)
        {
            data->vm->destroy(data->vm);
        }
        
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
        
        if (data->config != NULL)
        {
            free(data->config);
        }
        
        free(data);
        runtime->private_data = NULL;
    }
}

// ============================================================================
// 工厂函数实现
// ============================================================================

/**
 * @brief 创建主运行时接口实例
 */
Stru_RuntimeInterface_t* F_create_runtime_interface(void)
{
    Stru_RuntimeInterface_t* interface = 
        (Stru_RuntimeInterface_t*)malloc(sizeof(Stru_RuntimeInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = runtime_initialize;
    interface->get_memory_interface = runtime_get_memory_interface;
    interface->get_execution_engine = runtime_get_execution_engine;
    interface->get_debug_support = runtime_get_debug_support;
    interface->get_virtual_machine = runtime_get_virtual_machine;
    interface->execute_program = runtime_execute_program;
    interface->reset = runtime_reset;
    interface->destroy = runtime_destroy;
    interface->private_data = NULL;
    
    return interface;
}
