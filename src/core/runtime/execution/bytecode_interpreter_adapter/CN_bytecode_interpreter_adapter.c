/******************************************************************************
 * 文件名: CN_bytecode_interpreter_adapter.c
 * 功能: CN_Language 字节码解释器适配器模块
 * 
 * 将现有的字节码解释器适配到执行引擎接口，实现字节码加载和执行功能。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * 作者: CN_Language架构委员会
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_bytecode_interpreter_adapter.h"
#include "../bytecode_loader/CN_bytecode_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 前向声明，避免复杂的依赖
typedef struct Stru_BytecodeInterpreter_t Stru_BytecodeInterpreter_t;
typedef struct Stru_BytecodeProgram_t Stru_BytecodeProgram_t;

// 简化的配置结构体
typedef struct {
    bool use_jit;
    size_t stack_size;
    size_t heap_size;
    bool enable_profiling;
} Stru_BytecodeInterpreterConfig_t;

// 简化版本的外部函数声明
Stru_BytecodeInterpreter_t* F_create_bytecode_interpreter(const Stru_BytecodeInterpreterConfig_t* config);
void F_destroy_bytecode_interpreter(Stru_BytecodeInterpreter_t* interpreter);
bool F_load_bytecode_program(Stru_BytecodeInterpreter_t* interpreter, Stru_BytecodeProgram_t* program);
bool F_execute_bytecode_program(Stru_BytecodeInterpreter_t* interpreter);
void F_reset_bytecode_interpreter(Stru_BytecodeInterpreter_t* interpreter);
Stru_BytecodeInterpreterConfig_t F_create_default_bytecode_interpreter_config(void);
void F_get_profiling_data(const Stru_BytecodeInterpreter_t* interpreter,
                         uint64_t* instruction_count,
                         uint64_t* execution_time,
                         size_t* memory_usage);
Stru_BytecodeProgram_t* F_create_bytecode_program(void);
void F_destroy_bytecode_program(Stru_BytecodeProgram_t* program);
void F_unload_bytecode_module(Stru_BytecodeModule_t* module);

// ============================================================================
// 适配器私有数据结构
// ============================================================================

/**
 * @brief 适配器私有数据
 */
typedef struct Stru_BytecodeAdapterData_t {
    Stru_RuntimeMemoryInterface_t* memory_interface;  ///< 内存管理接口
    Stru_BytecodeInterpreter_t* interpreter;          ///< 字节码解释器实例
    Stru_BytecodeModule_t* current_module;            ///< 当前加载的模块
    bool use_jit;                                     ///< 是否启用JIT编译
    uint64_t instructions_executed;                   ///< 执行的指令数
    uint64_t execution_time_ms;                       ///< 执行时间（毫秒）
    size_t memory_used;                               ///< 使用的内存量
} Stru_BytecodeAdapterData_t;

// ============================================================================
// 内部辅助函数声明
// ============================================================================

static Stru_BytecodeProgram_t* F_convert_module_to_program(Stru_BytecodeModule_t* module);
static void F_update_execution_stats(Stru_BytecodeAdapterData_t* data, 
                                    Stru_BytecodeInterpreter_t* interpreter);

// ============================================================================
// 适配器函数实现
// ============================================================================

/**
 * @brief 初始化字节码执行引擎
 */
static bool bytecode_adapter_initialize(Stru_ExecutionEngineInterface_t* engine,
                                       Stru_RuntimeMemoryInterface_t* memory_interface,
                                       bool use_jit)
{
    if (engine == NULL || engine->private_data != NULL) {
        return false;
    }
    
    // 分配适配器数据
    Stru_BytecodeAdapterData_t* data = 
        (Stru_BytecodeAdapterData_t*)malloc(sizeof(Stru_BytecodeAdapterData_t));
    if (data == NULL) {
        return false;
    }
    
    memset(data, 0, sizeof(Stru_BytecodeAdapterData_t));
    data->memory_interface = memory_interface;
    data->use_jit = use_jit;
    
    // 创建字节码解释器
    Stru_BytecodeInterpreterConfig_t config = F_create_default_bytecode_interpreter_config();
    data->interpreter = F_create_bytecode_interpreter(&config);
    if (data->interpreter == NULL) {
        free(data);
        return false;
    }
    
    engine->private_data = data;
    return true;
}

/**
 * @brief 加载字节码模块
 */
static void* bytecode_adapter_load_module(Stru_ExecutionEngineInterface_t* engine,
                                         const uint8_t* bytecode, size_t bytecode_size,
                                         const char* module_name)
{
    if (engine == NULL || engine->private_data == NULL || bytecode == NULL) {
        return NULL;
    }
    
    Stru_BytecodeAdapterData_t* data = (Stru_BytecodeAdapterData_t*)engine->private_data;
    
    // 如果已有模块，先卸载
    if (data->current_module != NULL) {
        F_unload_bytecode_module(data->current_module);
        data->current_module = NULL;
    }
    
    // 加载字节码模块
    data->current_module = F_load_bytecode_from_memory(bytecode, bytecode_size, module_name);
    if (data->current_module == NULL) {
        return NULL;
    }
    
    // 将模块转换为解释器可用的程序格式
    Stru_BytecodeProgram_t* program = F_convert_module_to_program(data->current_module);
    if (program == NULL) {
        F_unload_bytecode_module(data->current_module);
        data->current_module = NULL;
        return NULL;
    }
    
    // 加载程序到解释器
    if (!F_load_bytecode_program(data->interpreter, program)) {
        // 注意：这里不释放program，因为解释器可能已经接管了它
        F_unload_bytecode_module(data->current_module);
        data->current_module = NULL;
        return NULL;
    }
    
    return data->current_module;
}

/**
 * @brief 卸载字节码模块
 */
static void bytecode_adapter_unload_module(Stru_ExecutionEngineInterface_t* engine,
                                          void* module)
{
    if (engine == NULL || engine->private_data == NULL) {
        return;
    }
    
    Stru_BytecodeAdapterData_t* data = (Stru_BytecodeAdapterData_t*)engine->private_data;
    
    // 验证模块指针
    if (data->current_module != module) {
        return;
    }
    
    // 重置解释器
    F_reset_bytecode_interpreter(data->interpreter);
    
    // 卸载模块
    F_unload_bytecode_module(data->current_module);
    data->current_module = NULL;
}

/**
 * @brief 执行函数
 */
static void* bytecode_adapter_execute_function(Stru_ExecutionEngineInterface_t* engine,
                                              void* module, const char* function_name,
                                              int argc, void** argv)
{
    if (engine == NULL || engine->private_data == NULL || module == NULL) {
        return NULL;
    }
    
    Stru_BytecodeAdapterData_t* data = (Stru_BytecodeAdapterData_t*)engine->private_data;
    
    // 验证模块
    if (data->current_module != module) {
        return NULL;
    }
    
    // 记录开始时间
    clock_t start_time = clock();
    
    // 执行字节码程序
    bool success = F_execute_bytecode_program(data->interpreter);
    
    // 记录结束时间
    clock_t end_time = clock();
    
    // 更新执行统计
    data->execution_time_ms += (uint64_t)((end_time - start_time) * 1000 / CLOCKS_PER_SEC);
    F_update_execution_stats(data, data->interpreter);
    
    // 获取执行结果
    // 简化实现：返回NULL，实际实现应从解释器获取返回值
    (void)function_name;
    (void)argc;
    (void)argv;
    
    return success ? (void*)1 : NULL;
}

/**
 * @brief 执行字节码片段
 */
static void* bytecode_adapter_execute_bytecode(Stru_ExecutionEngineInterface_t* engine,
                                              const uint8_t* bytecode, size_t bytecode_size,
                                              void* context)
{
    if (engine == NULL || engine->private_data == NULL || bytecode == NULL) {
        return NULL;
    }
    
    Stru_BytecodeAdapterData_t* data = (Stru_BytecodeAdapterData_t*)engine->private_data;
    
    // 记录开始时间
    clock_t start_time = clock();
    
    // 临时加载并执行字节码片段
    Stru_BytecodeModule_t* temp_module = F_load_bytecode_from_memory(bytecode, bytecode_size, "temp");
    if (temp_module == NULL) {
        return NULL;
    }
    
    Stru_BytecodeProgram_t* program = F_convert_module_to_program(temp_module);
    if (program == NULL) {
        F_unload_bytecode_module(temp_module);
        return NULL;
    }
    
    // 创建临时解释器
    Stru_BytecodeInterpreterConfig_t config = F_create_default_bytecode_interpreter_config();
    Stru_BytecodeInterpreter_t* temp_interpreter = F_create_bytecode_interpreter(&config);
    if (temp_interpreter == NULL) {
        F_destroy_bytecode_program(program);
        F_unload_bytecode_module(temp_module);
        return NULL;
    }
    
    // 加载并执行
    bool success = false;
    if (F_load_bytecode_program(temp_interpreter, program)) {
        success = F_execute_bytecode_program(temp_interpreter);
    }
    
    // 记录结束时间
    clock_t end_time = clock();
    
    // 更新执行统计
    data->execution_time_ms += (uint64_t)((end_time - start_time) * 1000 / CLOCKS_PER_SEC);
    F_update_execution_stats(data, temp_interpreter);
    
    // 清理资源
    F_destroy_bytecode_interpreter(temp_interpreter);
    // 注意：不释放program，因为解释器可能已经接管了它
    F_unload_bytecode_module(temp_module);
    
    (void)context;
    return success ? (void*)1 : NULL;
}

/**
 * @brief 设置执行超时
 */
static void bytecode_adapter_set_timeout(Stru_ExecutionEngineInterface_t* engine,
                                        uint64_t timeout_ms)
{
    if (engine == NULL || engine->private_data == NULL) {
        return;
    }
    
    Stru_BytecodeAdapterData_t* data = (Stru_BytecodeAdapterData_t*)engine->private_data;
    
    // 简化实现：记录超时设置
    // 实际实现应在解释器中设置超时机制
    (void)data;
    (void)timeout_ms;
}

/**
 * @brief 获取执行统计信息
 */
static void bytecode_adapter_get_stats(Stru_ExecutionEngineInterface_t* engine,
                                      uint64_t* instructions_executed,
                                      uint64_t* execution_time_ms,
                                      size_t* memory_used)
{
    if (engine == NULL || engine->private_data == NULL) {
        return;
    }
    
    Stru_BytecodeAdapterData_t* data = (Stru_BytecodeAdapterData_t*)engine->private_data;
    
    if (instructions_executed != NULL) {
        *instructions_executed = data->instructions_executed;
    }
    
    if (execution_time_ms != NULL) {
        *execution_time_ms = data->execution_time_ms;
    }
    
    if (memory_used != NULL) {
        *memory_used = data->memory_used;
    }
}

/**
 * @brief 重置执行引擎状态
 */
static void bytecode_adapter_reset(Stru_ExecutionEngineInterface_t* engine)
{
    if (engine == NULL || engine->private_data == NULL) {
        return;
    }
    
    Stru_BytecodeAdapterData_t* data = (Stru_BytecodeAdapterData_t*)engine->private_data;
    
    // 重置解释器
    if (data->interpreter != NULL) {
        F_reset_bytecode_interpreter(data->interpreter);
    }
    
    // 重置统计信息
    data->instructions_executed = 0;
    data->execution_time_ms = 0;
    data->memory_used = 0;
    
    // 卸载当前模块
    if (data->current_module != NULL) {
        F_unload_bytecode_module(data->current_module);
        data->current_module = NULL;
    }
}

/**
 * @brief 销毁执行引擎
 */
static void bytecode_adapter_destroy(Stru_ExecutionEngineInterface_t* engine)
{
    if (engine == NULL) {
        return;
    }
    
    if (engine->private_data != NULL) {
        Stru_BytecodeAdapterData_t* data = (Stru_BytecodeAdapterData_t*)engine->private_data;
        
        // 销毁解释器
        if (data->interpreter != NULL) {
            F_destroy_bytecode_interpreter(data->interpreter);
        }
        
        // 卸载模块
        if (data->current_module != NULL) {
            F_unload_bytecode_module(data->current_module);
        }
        
        free(data);
        engine->private_data = NULL;
    }
}

// ============================================================================
// 工厂函数实现
// ============================================================================

/**
 * @brief 创建字节码执行引擎接口实例
 */
Stru_ExecutionEngineInterface_t* F_create_bytecode_execution_engine(void)
{
    Stru_ExecutionEngineInterface_t* interface = 
        (Stru_ExecutionEngineInterface_t*)malloc(sizeof(Stru_ExecutionEngineInterface_t));
    
    if (interface == NULL) {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = bytecode_adapter_initialize;
    interface->load_module = bytecode_adapter_load_module;
    interface->unload_module = bytecode_adapter_unload_module;
    interface->execute_function = bytecode_adapter_execute_function;
    interface->execute_bytecode = bytecode_adapter_execute_bytecode;
    interface->set_timeout = bytecode_adapter_set_timeout;
    interface->get_execution_stats = bytecode_adapter_get_stats;
    interface->reset = bytecode_adapter_reset;
    interface->destroy = bytecode_adapter_destroy;
    interface->private_data = NULL;
    
    return interface;
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 将模块转换为解释器程序
 */
static Stru_BytecodeProgram_t* F_convert_module_to_program(Stru_BytecodeModule_t* module)
{
    if (module == NULL) {
        return NULL;
    }
    
    // 简化实现：创建一个基本的字节码程序
    // 实际实现应根据字节码格式进行解析和转换
    
    Stru_BytecodeProgram_t* program = F_create_bytecode_program();
    if (program == NULL) {
        return NULL;
    }
    
    // 注意：这里需要实际解析字节码指令，但为了简化，我们创建一个空程序
    // 实际实现应解析module->code_segment中的指令
    
    return program;
}

/**
 * @brief 更新执行统计
 */
static void F_update_execution_stats(Stru_BytecodeAdapterData_t* data, 
                                    Stru_BytecodeInterpreter_t* interpreter)
{
    if (data == NULL || interpreter == NULL) {
        return;
    }
    
    // 获取解释器的性能数据
    uint64_t instruction_count = 0;
    uint64_t execution_time = 0;
    size_t memory_usage = 0;
    
    F_get_profiling_data(interpreter, &instruction_count, &execution_time, &memory_usage);
    
    // 更新适配器统计
    data->instructions_executed += instruction_count;
    data->memory_used = memory_usage > data->memory_used ? memory_usage : data->memory_used;
}
