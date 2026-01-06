/**
 * @file CN_runtime_interface.c
 * @brief 运行时系统接口实现
 * 
 * 实现运行时系统抽象接口的工厂函数和桩实现。
 * 遵循单一职责原则，每个接口独立实现。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_runtime_interface.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 运行时内存管理接口实现
// ============================================================================

/**
 * @brief 运行时内存管理接口私有数据
 */
typedef struct Stru_RuntimeMemoryData_t
{
    size_t heap_size;          ///< 堆大小
    bool use_gc;               ///< 是否启用垃圾回收
    size_t total_allocated;    ///< 总分配字节数
    size_t total_freed;        ///< 总释放字节数
    size_t object_count;       ///< 对象数量
    size_t gc_count;           ///< 垃圾回收次数
} Stru_RuntimeMemoryData_t;

/**
 * @brief 运行时内存管理初始化函数
 */
static bool runtime_memory_initialize(Stru_RuntimeMemoryInterface_t* memory_interface,
                                     size_t heap_size, bool use_gc)
{
    if (memory_interface == NULL || memory_interface->private_data != NULL)
    {
        return false;
    }
    
    Stru_RuntimeMemoryData_t* data = 
        (Stru_RuntimeMemoryData_t*)malloc(sizeof(Stru_RuntimeMemoryData_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->heap_size = heap_size;
    data->use_gc = use_gc;
    data->total_allocated = 0;
    data->total_freed = 0;
    data->object_count = 0;
    data->gc_count = 0;
    
    memory_interface->private_data = data;
    return true;
}

/**
 * @brief 分配运行时对象内存
 */
static void* runtime_memory_allocate_object(Stru_RuntimeMemoryInterface_t* memory_interface,
                                           size_t size, uint32_t type_tag)
{
    (void)type_tag; // 暂时未使用
    
    if (memory_interface == NULL || memory_interface->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_RuntimeMemoryData_t* data = (Stru_RuntimeMemoryData_t*)memory_interface->private_data;
    
    // 简单实现：使用系统malloc
    void* object = malloc(size);
    if (object != NULL)
    {
        data->total_allocated += size;
        data->object_count++;
    }
    
    return object;
}

/**
 * @brief 分配运行时数组内存
 */
static void* runtime_memory_allocate_array(Stru_RuntimeMemoryInterface_t* memory_interface,
                                          size_t element_size, size_t element_count,
                                          uint32_t type_tag)
{
    (void)type_tag; // 暂时未使用
    
    if (memory_interface == NULL || memory_interface->private_data == NULL)
    {
        return NULL;
    }
    
    size_t total_size = element_size * element_count;
    return runtime_memory_allocate_object(memory_interface, total_size, 0);
}

/**
 * @brief 标记对象为活动状态
 */
static void runtime_memory_mark_object(Stru_RuntimeMemoryInterface_t* memory_interface,
                                      void* object)
{
    (void)memory_interface;
    (void)object;
    // 桩实现：垃圾回收未实现
}

/**
 * @brief 执行垃圾回收
 */
static size_t runtime_memory_collect_garbage(Stru_RuntimeMemoryInterface_t* memory_interface)
{
    if (memory_interface == NULL || memory_interface->private_data == NULL)
    {
        return 0;
    }
    
    Stru_RuntimeMemoryData_t* data = (Stru_RuntimeMemoryData_t*)memory_interface->private_data;
    data->gc_count++;
    
    // 桩实现：不实际回收内存
    return 0;
}

/**
 * @brief 获取内存使用统计
 */
static void runtime_memory_get_stats(Stru_RuntimeMemoryInterface_t* memory_interface,
                                    size_t* total_heap, size_t* used_heap,
                                    size_t* object_count, size_t* gc_count)
{
    if (memory_interface == NULL || memory_interface->private_data == NULL)
    {
        return;
    }
    
    Stru_RuntimeMemoryData_t* data = (Stru_RuntimeMemoryData_t*)memory_interface->private_data;
    
    if (total_heap != NULL) *total_heap = data->heap_size;
    if (used_heap != NULL) *used_heap = data->total_allocated - data->total_freed;
    if (object_count != NULL) *object_count = data->object_count;
    if (gc_count != NULL) *gc_count = data->gc_count;
}

/**
 * @brief 销毁运行时内存管理器
 */
static void runtime_memory_destroy(Stru_RuntimeMemoryInterface_t* memory_interface)
{
    if (memory_interface == NULL)
    {
        return;
    }
    
    if (memory_interface->private_data != NULL)
    {
        free(memory_interface->private_data);
        memory_interface->private_data = NULL;
    }
}

/**
 * @brief 创建运行时内存管理接口实例
 */
Stru_RuntimeMemoryInterface_t* F_create_runtime_memory_interface(void)
{
    Stru_RuntimeMemoryInterface_t* interface = 
        (Stru_RuntimeMemoryInterface_t*)malloc(sizeof(Stru_RuntimeMemoryInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = runtime_memory_initialize;
    interface->allocate_object = runtime_memory_allocate_object;
    interface->allocate_array = runtime_memory_allocate_array;
    interface->mark_object = runtime_memory_mark_object;
    interface->collect_garbage = runtime_memory_collect_garbage;
    interface->get_memory_stats = runtime_memory_get_stats;
    interface->destroy = runtime_memory_destroy;
    interface->private_data = NULL;
    
    return interface;
}

// ============================================================================
// 执行引擎接口实现
// ============================================================================

/**
 * @brief 执行引擎接口私有数据
 */
typedef struct Stru_ExecutionEngineData_t
{
    Stru_RuntimeMemoryInterface_t* memory_interface;  ///< 内存管理接口
    bool use_jit;                                     ///< 是否启用JIT编译
    uint64_t instructions_executed;                   ///< 执行的指令数
    uint64_t execution_time_ms;                       ///< 执行时间（毫秒）
    size_t memory_used;                               ///< 使用的内存量
} Stru_ExecutionEngineData_t;

/**
 * @brief 执行引擎初始化函数
 */
static bool execution_engine_initialize(Stru_ExecutionEngineInterface_t* engine,
                                       Stru_RuntimeMemoryInterface_t* memory_interface,
                                       bool use_jit)
{
    if (engine == NULL || engine->private_data != NULL)
    {
        return false;
    }
    
    Stru_ExecutionEngineData_t* data = 
        (Stru_ExecutionEngineData_t*)malloc(sizeof(Stru_ExecutionEngineData_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->memory_interface = memory_interface;
    data->use_jit = use_jit;
    data->instructions_executed = 0;
    data->execution_time_ms = 0;
    data->memory_used = 0;
    
    engine->private_data = data;
    return true;
}

/**
 * @brief 加载字节码模块
 */
static void* execution_engine_load_module(Stru_ExecutionEngineInterface_t* engine,
                                         const uint8_t* bytecode, size_t bytecode_size,
                                         const char* module_name)
{
    (void)engine;
    (void)bytecode;
    (void)bytecode_size;
    (void)module_name;
    
    // 桩实现：返回NULL
    return NULL;
}

/**
 * @brief 执行函数
 */
static void* execution_engine_execute_function(Stru_ExecutionEngineInterface_t* engine,
                                              void* module, const char* function_name,
                                              int argc, void** argv)
{
    (void)engine;
    (void)module;
    (void)function_name;
    (void)argc;
    (void)argv;
    
    // 桩实现：返回NULL
    return NULL;
}

/**
 * @brief 执行字节码片段
 */
static void* execution_engine_execute_bytecode(Stru_ExecutionEngineInterface_t* engine,
                                              const uint8_t* bytecode, size_t bytecode_size,
                                              void* context)
{
    (void)engine;
    (void)bytecode;
    (void)bytecode_size;
    (void)context;
    
    // 桩实现：返回NULL
    return NULL;
}

/**
 * @brief 设置执行超时
 */
static void execution_engine_set_timeout(Stru_ExecutionEngineInterface_t* engine,
                                        uint64_t timeout_ms)
{
    (void)engine;
    (void)timeout_ms;
    // 桩实现：忽略超时设置
}

/**
 * @brief 获取执行统计信息
 */
static void execution_engine_get_stats(Stru_ExecutionEngineInterface_t* engine,
                                      uint64_t* instructions_executed,
                                      uint64_t* execution_time_ms,
                                      size_t* memory_used)
{
    if (engine == NULL || engine->private_data == NULL)
    {
        return;
    }
    
    Stru_ExecutionEngineData_t* data = (Stru_ExecutionEngineData_t*)engine->private_data;
    
    if (instructions_executed != NULL) *instructions_executed = data->instructions_executed;
    if (execution_time_ms != NULL) *execution_time_ms = data->execution_time_ms;
    if (memory_used != NULL) *memory_used = data->memory_used;
}

/**
 * @brief 重置执行引擎状态
 */
static void execution_engine_reset(Stru_ExecutionEngineInterface_t* engine)
{
    if (engine == NULL || engine->private_data == NULL)
    {
        return;
    }
    
    Stru_ExecutionEngineData_t* data = (Stru_ExecutionEngineData_t*)engine->private_data;
    data->instructions_executed = 0;
    data->execution_time_ms = 0;
    data->memory_used = 0;
}

/**
 * @brief 销毁执行引擎
 */
static void execution_engine_destroy(Stru_ExecutionEngineInterface_t* engine)
{
    if (engine == NULL)
    {
        return;
    }
    
    if (engine->private_data != NULL)
    {
        free(engine->private_data);
        engine->private_data = NULL;
    }
}

/**
 * @brief 创建执行引擎接口实例
 */
Stru_ExecutionEngineInterface_t* F_create_execution_engine_interface(void)
{
    Stru_ExecutionEngineInterface_t* interface = 
        (Stru_ExecutionEngineInterface_t*)malloc(sizeof(Stru_ExecutionEngineInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = execution_engine_initialize;
    interface->load_module = execution_engine_load_module;
    interface->execute_function = execution_engine_execute_function;
    interface->execute_bytecode = execution_engine_execute_bytecode;
    interface->set_timeout = execution_engine_set_timeout;
    interface->get_execution_stats = execution_engine_get_stats;
    interface->reset = execution_engine_reset;
    interface->destroy = execution_engine_destroy;
    interface->private_data = NULL;
    
    return interface;
}

// ============================================================================
// 调试支持接口实现
// ============================================================================

/**
 * @brief 调试支持接口私有数据
 */
typedef struct Stru_DebugSupportData_t
{
    Stru_ExecutionEngineInterface_t* engine;  ///< 执行引擎接口
    int breakpoint_count;                     ///< 断点数量
} Stru_DebugSupportData_t;

/**
 * @brief 调试器初始化函数
 */
static bool debug_support_initialize(Stru_DebugSupportInterface_t* debugger,
                                    Stru_ExecutionEngineInterface_t* engine)
{
    if (debugger == NULL || debugger->private_data != NULL)
    {
        return false;
    }
    
    Stru_DebugSupportData_t* data = 
        (Stru_DebugSupportData_t*)malloc(sizeof(Stru_DebugSupportData_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->engine = engine;
    data->breakpoint_count = 0;
    
    debugger->private_data = data;
    return true;
}

/**
 * @brief 设置断点
 */
static bool debug_support_set_breakpoint(Stru_DebugSupportInterface_t* debugger,
                                        const Stru_Breakpoint_t* breakpoint)
{
    (void)debugger;
    (void)breakpoint;
    
    // 桩实现：总是成功
    return true;
}

/**
 * @brief 清除断点
 */
static bool debug_support_clear_breakpoint(Stru_DebugSupportInterface_t* debugger,
                                          int breakpoint_id)
{
    (void)debugger;
    (void)breakpoint_id;
    
    // 桩实现：总是成功
    return true;
}

/**
 * @brief 单步执行
 */
static bool debug_support_step(Stru_DebugSupportInterface_t* debugger, int step_type)
{
    (void)debugger;
    (void)step_type;
    
    // 桩实现：总是成功
    return true;
}

/**
 * @brief 继续执行
 */
static bool debug_support_continue_execution(Stru_DebugSupportInterface_t* debugger)
{
    (void)debugger;
    
    // 桩实现：总是成功
    return true;
}

/**
 * @brief 暂停执行
 */
static bool debug_support_pause_execution(Stru_DebugSupportInterface_t* debugger)
{
    (void)debugger;
    
    // 桩实现：总是成功
    return true;
}

/**
 * @brief 获取调用栈
 */
static int debug_support_get_call_stack(Stru_DebugSupportInterface_t* debugger,
                                       void* stack_buffer, size_t buffer_size)
{
    (void)debugger;
    (void)stack_buffer;
    (void)buffer_size;
    
    // 桩实现：返回0个栈帧
    return 0;
}

/**
 * @brief 检查变量值
 */
static bool debug_support_inspect_variable(Stru_DebugSupportInterface_t* debugger,
                                          const char* variable_name,
                                          char* value_buffer, size_t buffer_size)
{
    (void)debugger;
    (void)variable_name;
    
    // 桩实现：返回空字符串
    if (value_buffer != NULL && buffer_size > 0)
    {
        value_buffer[0] = '\0';
    }
    
    return true;
}

/**
 * @brief 设置变量值
 */
static bool debug_support_set_variable(Stru_DebugSupportInterface_t* debugger,
                                      const char* variable_name, const char* value)
{
    (void)debugger;
    (void)variable_name;
    (void)value;
    
    // 桩实现：总是成功
    return true;
}

/**
 * @brief 获取下一个调试事件
 */
static bool debug_support_get_next_event(Stru_DebugSupportInterface_t* debugger,
                                        Stru_DebugEvent_t* event, uint64_t timeout_ms)
{
    (void)debugger;
    (void)timeout_ms;
    
    // 桩实现：没有事件
    if (event != NULL)
    {
        event->type = Eum_DEBUG_EVENT_PROGRAM_END;
        event->location = NULL;
        event->message = "No debug events available";
        event->context = NULL;
        event->timestamp = 0;
    }
    
    return false;
}

/**
 * @brief 销毁调试器
 */
static void debug_support_destroy(Stru_DebugSupportInterface_t* debugger)
{
    if (debugger == NULL)
    {
        return;
    }
    
    if (debugger->private_data != NULL)
    {
        free(debugger->private_data);
        debugger->private_data = NULL;
    }
}

/**
 * @brief 创建调试支持接口实例
 */
Stru_DebugSupportInterface_t* F_create_debug_support_interface(void)
{
    Stru_DebugSupportInterface_t* interface = 
        (Stru_DebugSupportInterface_t*)malloc(sizeof(Stru_DebugSupportInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = debug_support_initialize;
    interface->set_breakpoint = debug_support_set_breakpoint;
    interface->clear_breakpoint = debug_support_clear_breakpoint;
    interface->step = debug_support_step;
    interface->continue_execution = debug_support_continue_execution;
    interface->pause_execution = debug_support_pause_execution;
    interface->get_call_stack = debug_support_get_call_stack;
    interface->inspect_variable = debug_support_inspect_variable;
    interface->set_variable = debug_support_set_variable;
    interface->get_next_event = debug_support_get_next_event;
    interface->destroy = debug_support_destroy;
    interface->private_data = NULL;
    
    return interface;
}

// ============================================================================
// 虚拟机接口实现
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

/**
 * @brief 虚拟机初始化函数
 */
static bool virtual_machine_initialize(Stru_VirtualMachineInterface_t* vm,
                                      size_t heap_size, bool use_gc, bool use_jit)
{
    if (vm == NULL || vm->private_data != NULL)
    {
        return false;
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
    (void)bytecode;
    (void)bytecode_size;
    (void)module_name;
    (void)argc;
    (void)argv;
    
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
    interface->destroy = virtual_machine_destroy;
    interface->private_data = NULL;
    
    return interface;
}

// ============================================================================
// 主运行时接口实现
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

/**
 * @brief 运行时系统初始化函数
 */
static bool runtime_initialize(Stru_RuntimeInterface_t* runtime, const char* config)
{
    if (runtime == NULL || runtime->private_data != NULL)
    {
        return false;
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
        data->config = strdup(config);
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
    interface->destroy = runtime_destroy;
    interface->private_data = NULL;
    
    return interface;
}
