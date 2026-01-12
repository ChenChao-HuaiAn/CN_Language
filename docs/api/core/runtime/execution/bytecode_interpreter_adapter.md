# 字节码解释器适配器 API 文档

## 概述

字节码解释器适配器模块是执行引擎系统的桥梁组件，负责将现有的字节码解释器适配到统一的执行引擎接口。该模块实现了适配器设计模式，提供透明的接口转换和错误处理。

## 头文件

```c
#include "src/core/runtime/execution/bytecode_interpreter_adapter/CN_bytecode_interpreter_adapter.h"
```

## 数据结构

### Stru_BytecodeInterpreterAdapter_t
适配器内部结构体，包装底层解释器和执行上下文。

```c
typedef struct Stru_BytecodeInterpreterAdapter_t {
    Stru_ExecutionEngineInterface_t engine_interface;  // 执行引擎接口
    Stru_BytecodeInterpreterInterface_t* interpreter;  // 底层解释器
    Stru_ExecutionContext_t* context;                  // 执行上下文
    Stru_BytecodeObject_t* current_bytecode;          // 当前加载的字节码
    Stru_AdapterConfig_t config;                      // 适配器配置
    bool is_initialized;                              // 初始化状态
    bool is_running;                                  // 运行状态
    uint32_t breakpoints[CN_MAX_BREAKPOINTS];         // 断点数组
    uint32_t breakpoint_count;                        // 断点数量
} Stru_BytecodeInterpreterAdapter_t;
```

### Stru_AdapterConfig_t
适配器配置结构体，控制适配器的行为模式。

```c
typedef struct Stru_AdapterConfig_t {
    AdapterStrategy strategy;          // 适配策略
    ErrorHandlingMode error_handling;  // 错误处理模式
    PerformanceMode performance_mode;  // 性能模式
    uint32_t max_stack_depth;          // 最大栈深度
    bool enable_profiling;             // 启用性能分析
    bool strict_mode;                  // 严格模式
} Stru_AdapterConfig_t;
```

### Stru_ExecutionContext_t
执行上下文结构体，保存虚拟机状态信息。

```c
typedef struct Stru_ExecutionContext_t {
    uint32_t pc;                      // 程序计数器
    uint32_t sp;                      // 栈指针
    uint32_t fp;                      // 帧指针
    uint32_t registers[CN_NUM_REGISTERS];  // 寄存器文件
    uint64_t instruction_count;       // 已执行指令数
    ExecutionStatus status;           // 执行状态
    ErrorInfo last_error;             // 最后错误信息
} Stru_ExecutionContext_t;
```

## 常量定义

### 适配策略
```c
typedef enum Eum_AdapterStrategy_t {
    ADAPTER_STRATEGY_DIRECT = 0,      // 直接适配（最小开销）
    ADAPTER_STRATEGY_BUFFERED = 1,    // 缓冲适配（平衡性能）
    ADAPTER_STRATEGY_OPTIMIZED = 2    // 优化适配（最大性能）
} Eum_AdapterStrategy_t;
```

### 错误处理模式
```c
typedef enum Eum_ErrorHandlingMode_t {
    ERROR_HANDLING_LAX = 0,           // 宽松模式（忽略非致命错误）
    ERROR_HANDLING_STRICT = 1,        // 严格模式（所有错误都失败）
    ERROR_HANDLING_RECOVERABLE = 2    // 可恢复模式（尝试恢复）
} Eum_ErrorHandlingMode_t;
```

### 性能模式
```c
typedef enum Eum_PerformanceMode_t {
    PERFORMANCE_MODE_MINIMAL = 0,     // 最小性能（调试用）
    PERFORMANCE_MODE_BALANCED = 1,    // 平衡性能（默认）
    PERFORMANCE_MODE_MAXIMUM = 2      // 最大性能（生产环境）
} Eum_PerformanceMode_t;
```

### 执行状态
```c
typedef enum Eum_ExecutionStatus_t {
    EXECUTION_STOPPED = 0,            // 已停止
    EXECUTION_RUNNING = 1,            // 运行中
    EXECUTION_PAUSED = 2,             // 已暂停
    EXECUTION_ERROR = 3,              // 错误状态
    EXECUTION_FINISHED = 4            // 执行完成
} Eum_ExecutionStatus_t;
```

### 限制常量
```c
#define CN_MAX_BREAKPOINTS 64         // 最大断点数
#define CN_NUM_REGISTERS 16           // 寄存器数量
#define CN_MAX_STACK_DEPTH 65536      // 最大栈深度
```

## 函数接口

### 工厂函数

#### F_create_bytecode_interpreter_adapter
```c
/**
 * @brief 创建字节码解释器适配器实例
 * @param interpreter 底层字节码解释器接口
 * @return 成功返回执行引擎接口指针，失败返回NULL
 * 
 * @details
 * 创建适配器实例，包装提供的字节码解释器。适配器会接管解释器的
 * 生命周期管理，调用者不应再直接使用或销毁解释器。
 * 
 * @note
 * - interpreter必须是非NULL的有效解释器接口
 * - 适配器创建失败会返回NULL，但不会销毁解释器
 * - 返回的接口指针必须通过destroy方法释放
 * 
 * @errors
 * - 内存分配失败返回NULL
 * - 解释器接口无效返回NULL
 * 
 * @example
 * ```c
 * Stru_BytecodeInterpreterInterface_t* interpreter = F_create_bytecode_interpreter();
 * Stru_ExecutionEngineInterface_t* engine = 
 *     F_create_bytecode_interpreter_adapter(interpreter);
 * if (!engine) {
 *     interpreter->destroy(interpreter);
 *     return -1;
 * }
 * ```
 */
Stru_ExecutionEngineInterface_t* F_create_bytecode_interpreter_adapter(
    Stru_BytecodeInterpreterInterface_t* interpreter);
```

#### F_create_bytecode_interpreter_adapter_with_config
```c
/**
 * @brief 使用配置创建字节码解释器适配器实例
 * @param interpreter 底层字节码解释器接口
 * @param config 适配器配置
 * @return 成功返回执行引擎接口指针，失败返回NULL
 * 
 * @details
 * 使用指定的配置创建适配器实例。配置控制适配器的行为模式、
 * 错误处理和性能特性。
 * 
 * @note
 * - 如果config为NULL，使用默认配置
 * - 配置在创建时应用，运行时不能修改
 * - 不同的配置可能影响性能和兼容性
 * 
 * @example
 * ```c
 * Stru_AdapterConfig_t config = {
 *     .strategy = ADAPTER_STRATEGY_OPTIMIZED,
 *     .error_handling = ERROR_HANDLING_STRICT,
 *     .performance_mode = PERFORMANCE_MODE_MAXIMUM
 * };
 * Stru_ExecutionEngineInterface_t* engine = 
 *     F_create_bytecode_interpreter_adapter_with_config(interpreter, &config);
 * ```
 */
Stru_ExecutionEngineInterface_t* F_create_bytecode_interpreter_adapter_with_config(
    Stru_BytecodeInterpreterInterface_t* interpreter,
    const Stru_AdapterConfig_t* config);
```

### 适配的接口方法

#### initialize
```c
/**
 * @brief 初始化执行引擎
 * @param config 执行配置参数
 * @return 初始化成功返回true，否则返回false
 * 
 * @details
 * 初始化执行引擎，分配必要的资源（栈、内存等）。配置参数
 * 控制执行环境的大小和特性。
 * 
 * @note
 * - 必须先初始化才能执行字节码
 * - 可以多次调用，但只有第一次调用有效
 * - 初始化失败会设置错误状态
 * 
 * @errors
 * - 内存分配失败返回false
 * - 配置无效返回false
 * - 底层解释器初始化失败返回false
 * 
 * @example
 * ```c
 * Stru_ExecutionConfig_t exec_config = {
 *     .stack_size = 4096,
 *     .heap_size = 8192,
 *     .enable_debug = true
 * };
 * if (!engine->initialize(&exec_config)) {
 *     printf("初始化失败\n");
 *     engine->destroy();
 *     return -1;
 * }
 * ```
 */
bool initialize(const Stru_ExecutionConfig_t* config);
```

#### load_bytecode
```c
/**
 * @brief 加载字节码到执行引擎
 * @param bytecode 字节码对象
 * @return 加载成功返回true，否则返回false
 * 
 * @details
 * 加载字节码对象到执行引擎，准备执行。适配器会验证字节码
 * 格式，并转换为底层解释器需要的格式。
 * 
 * @note
 * - bytecode必须是通过字节码加载器加载的有效对象
 * - 加载会重置执行状态（PC、栈等）
 * - 可以多次调用加载不同的字节码
 * 
 * @errors
 * - bytecode为NULL返回false
 * - 字节码格式无效返回false
 * - 内存不足返回false
 * - 底层解释器加载失败返回false
 * 
 * @example
 * ```c
 * Stru_BytecodeObject_t* bytecode = loader->load_from_file("program.cnbc");
 * if (!engine->load_bytecode(bytecode)) {
 *     printf("加载字节码失败\n");
 *     loader->destroy_bytecode(bytecode);
 *     engine->destroy();
 *     return -1;
 * }
 * ```
 */
bool load_bytecode(Stru_BytecodeObject_t* bytecode);
```

#### execute
```c
/**
 * @brief 执行字节码
 * @param start_address 起始执行地址（字节码偏移）
 * @return 执行成功返回true，执行错误返回false
 * 
 * @details
 * 从指定地址开始执行字节码，直到遇到HALT指令、错误或
 * 达到执行限制。执行是同步的，函数在执行完成后返回。
 * 
 * @note
 * - start_address为0表示从入口点开始执行
 * - 执行期间可以响应断点
 * - 执行错误会设置错误状态
 * 
 * @errors
 * - 未初始化返回false
 * - 未加载字节码返回false
 * - 地址越界返回false
 * - 执行错误返回false
 * 
 * @example
 * ```c
 * // 从入口点执行
 * if (!engine->execute(0)) {
 *     Stru_ExecutionState_t state;
 *     engine->get_state(&state);
 *     printf("执行失败在地址 0x%08X\n", state.pc);
 * }
 * ```
 */
bool execute(uint32_t start_address);
```

#### step
```c
/**
 * @brief 单步执行字节码
 * @return 执行成功返回true，执行结束或错误返回false
 * 
 * @details
 * 执行一条指令，然后暂停。用于调试和单步执行。
 * 函数会更新执行状态，包括PC、寄存器和栈。
 * 
 * @note
 * - 必须在加载字节码后调用
 * - 单步执行会触发断点检查
 * - 返回false表示执行结束或错误
 * 
 * @errors
 * - 未初始化返回false
 * - 未加载字节码返回false
 * - 指令执行错误返回false
 * 
 * @example
 * ```c
 * // 单步执行10条指令
 * for (int i = 0; i < 10; i++) {
 *     if (!engine->step()) {
 *         printf("执行结束或错误\n");
 *         break;
 *     }
 *     Stru_ExecutionState_t state;
 *     engine->get_state(&state);
 *     printf("Step %d: PC=0x%04X\n", i, state.pc);
 * }
 * ```
 */
bool step(void);
```

#### get_state
```c
/**
 * @brief 获取执行引擎状态
 * @param state 状态输出参数
 * @return 获取成功返回true，否则返回false
 * 
 * @details
 * 获取当前的执行状态，包括程序计数器、栈指针、寄存器值
 * 和执行状态。状态信息用于调试和监控。
 * 
 * @note
 * - state必须是非NULL的有效指针
 * - 状态信息是执行时的快照
 * - 获取状态不会影响执行
 * 
 * @errors
 * - state为NULL返回false
 * - 未初始化返回false
 * 
 * @example
 * ```c
 * Stru_ExecutionState_t state;
 * if (engine->get_state(&state)) {
 *     printf("PC: 0x%08X, SP: 0x%08X, Status: %d\n",
 *            state.pc, state.sp, state.status);
 *     for (int i = 0; i < CN_NUM_REGISTERS; i++) {
 *         printf("  R%d: 0x%08X\n", i, state.registers[i]);
 *     }
 * }
 * ```
 */
bool get_state(Stru_ExecutionState_t* state);
```

#### set_breakpoint
```c
/**
 * @brief 设置断点
 * @param address 断点地址（字节码偏移）
 * @return 设置成功返回true，否则返回false
 * 
 * @details
 * 在指定地址设置断点。当执行到断点地址时，执行会暂停。
 * 断点用于调试和控制执行流程。
 * 
 * @note
 * - 地址必须在字节码范围内
 * - 断点数量有限制（CN_MAX_BREAKPOINTS）
 * - 相同的地址只能设置一个断点
 * 
 * @errors
 * - 地址越界返回false
 * - 断点数量超限返回false
 * - 重复地址返回false
 * 
 * @example
 * ```c
 * // 在函数入口设置断点
 * if (!engine->set_breakpoint(0x0100)) {
 *     printf("设置断点失败\n");
 * }
 * 
 * // 执行直到断点
 * engine->execute(0);
 * Stru_ExecutionState_t state;
 * engine->get_state(&state);
 * if (state.status == EXECUTION_PAUSED) {
 *     printf("在断点 0x%08X 暂停\n", state.pc);
 * }
 * ```
 */
bool set_breakpoint(uint32_t address);
```

#### clear_breakpoint
```c
/**
 * @brief 清除断点
 * @param address 断点地址
 * @return 清除成功返回true，否则返回false
 * 
 * @details
 * 清除指定地址的断点。如果地址没有设置断点，操作失败。
 * 
 * @note
 * - 地址必须已设置断点
 * - 清除不存在的断点返回false
 * - 断点清除后执行不再在该地址暂停
 * 
 * @errors
 * - 地址未设置断点返回false
 * 
 * @example
 * ```c
 * // 设置断点
 * engine->set_breakpoint(0x0100);
 * 
 * // 执行一些操作...
 * 
 * // 清除断点
 * engine->clear_breakpoint(0x0100);
 * ```
 */
bool clear_breakpoint(uint32_t address);
```

#### destroy
```c
/**
 * @brief 销毁执行引擎实例
 * 
 * @details
 * 释放执行引擎占用的所有资源，包括：
 * 1. 适配器内部结构
 * 2. 底层解释器实例
 * 3. 执行上下文和状态
 * 4. 分配的栈和内存
 * 
 * @note
 * - 调用后引擎指针变为无效，不应再使用
 * - 该函数是幂等的，多次调用是安全的
 * - 会自动清理所有断点
 * 
 * @example
 * ```c
 * engine->destroy();
 * engine = NULL;  // 避免悬空指针
 * ```
 */
void destroy(void);
```

## 工具函数

### get_adapter_config
```c
/**
 * @brief 获取适配器配置
 * @param engine 执行引擎接口
 * @param config 配置输出参数
 * @return 获取成功返回true，否则返回false
 * 
 * @details
 * 获取适配器的当前配置。配置在创建时设置，运行时只读。
 * 
 * @note
 * - engine必须是通过适配器工厂函数创建的
 * - config必须是非NULL的有效指针
 * - 配置信息是创建时的副本
 */
bool get_adapter_config(Stru_ExecutionEngineInterface_t* engine,
                       Stru_AdapterConfig_t* config);
```

### get_last_error_info
```c
/**
 * @brief 获取详细的错误信息
 * @param engine 执行引擎接口
 * @param error_info 错误信息输出参数
 * @return 获取成功返回true，否则返回false
 * 
 * @details
 * 获取最近一次错误的详细信息，包括错误码、错误消息
 * 和错误上下文（如地址、指令等）。
 * 
 * @note
 * - 错误信息在每次操作后更新
 * - 成功操作会清除错误信息
 * - 错误信息是线程本地的
 */
bool get_last_error_info(Stru_ExecutionEngineInterface_t* engine,
                        ErrorInfo* error_info);
```

### get_breakpoint_list
```c
/**
 * @brief 获取断点列表
 * @param engine 执行引擎接口
 * @param addresses 地址数组输出参数
 * @param max_count 数组最大容量
 * @return 实际断点数量
 * 
 * @details
 * 获取当前设置的所有断点地址。地址按设置顺序返回。
 * 
 * @note
 * - addresses必须有足够的空间（至少CN_MAX_BREAKPOINTS）
 * - 返回0表示没有设置断点
 * - 实际数量可能小于max_count
 */
uint32_t get_breakpoint_list(Stru_ExecutionEngineInterface_t* engine,
                            uint32_t* addresses,
                            uint32_t max_count);
```

## 使用示例

### 完整调试示例
```c
#include "CN_bytecode_interpreter_adapter.h"
#include "CN_bytecode_loader.h"
#include <stdio.h>

void debug_execution(Stru_ExecutionEngineInterface_t* engine) {
    Stru_ExecutionState_t state;
    
    // 设置断点
    engine->set_breakpoint(0x0100);  // 函数入口
