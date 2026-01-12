# 字节码执行器 API 文档

## 概述

字节码执行器模块是CN_Language运行时系统的核心执行组件，实现了完整的基于栈的虚拟机。该模块直接执行字节码指令，提供高性能的字节码执行环境。

## 头文件

```c
#include "src/core/runtime/execution/bytecode_executor/CN_bytecode_executor.h"
```

## 数据结构

### Stru_BytecodeExecutor_t
执行器内部结构体，包含完整的虚拟机状态。

```c
typedef struct Stru_BytecodeExecutor_t {
    Stru_ExecutionEngineInterface_t interface;  // 公共接口
    
    // 执行状态
    Stru_BytecodeObject_t* bytecode;           // 当前字节码
    Stru_ExecutionState_t state;               // 执行状态
    Stru_ExecutorConfig_t config;              // 执行器配置
    
    // 运行时内存
    Stru_Stack_t* operand_stack;               // 操作数栈
    Stru_Stack_t* call_stack;                  // 调用栈
    uint8_t* local_variables;                  // 本地变量区
    uint8_t* heap_memory;                      // 堆内存
    
    // 控制状态
    bool is_initialized;                       // 初始化标志
    bool is_running;                           // 运行标志
    bool is_paused;                            // 暂停标志
    bool has_error;                            // 错误标志
    
    // 调试支持
    uint32_t breakpoints[CN_MAX_BREAKPOINTS];  // 断点列表
    uint32_t breakpoint_count;                 // 断点数量
    uint64_t instruction_counter;              // 指令计数器
    ProfilingData profiling;                   // 性能分析数据
} Stru_BytecodeExecutor_t;
```

### Stru_ExecutorConfig_t
执行器配置结构体，控制虚拟机的行为。

```c
typedef struct Stru_ExecutorConfig_t {
    uint32_t operand_stack_size;      // 操作数栈大小（字节）
    uint32_t call_stack_size;         // 调用栈大小（字节）
    uint32_t heap_size;               // 堆大小（字节）
    uint32_t max_local_variables;     // 最大本地变量数
    uint32_t max_instructions;        // 最大指令执行数
    bool enable_debug;                // 启用调试支持
    bool enable_profiling;            // 启用性能分析
    bool strict_mode;                 // 严格模式（更多检查）
    bool enable_optimizations;        // 启用优化
} Stru_ExecutorConfig_t;
```

### Stru_ExecutionState_t
执行状态结构体，保存虚拟机的当前状态。

```c
typedef struct Stru_ExecutionState_t {
    uint32_t pc;                      // 程序计数器
    uint32_t sp;                      // 栈指针
    uint32_t fp;                      // 帧指针
    uint32_t ip;                      // 指令指针
    uint32_t registers[CN_NUM_REGISTERS];  // 通用寄存器
    uint32_t status_flags;            // 状态标志
    ExecutionStatus status;           // 执行状态
    uint32_t error_code;              // 错误码
    const char* error_message;        // 错误消息
} Stru_ExecutionState_t;
```

### Stru_ProfilingData_t
性能分析数据结构体，收集执行统计信息。

```c
typedef struct Stru_ProfilingData_t {
    uint64_t total_instructions;      // 总指令数
    uint64_t instruction_counts[256]; // 各指令执行次数
    uint64_t stack_operations;        // 栈操作次数
    uint64_t memory_accesses;         // 内存访问次数
    uint64_t function_calls;          // 函数调用次数
    uint64_t start_time;              // 开始时间（微秒）
    uint64_t end_time;                // 结束时间（微秒）
} Stru_ProfilingData_t;
```

## 常量定义

### 寄存器常量
```c
#define CN_NUM_REGISTERS 16           // 寄存器数量
#define CN_REGISTER_PC 15             // 程序计数器寄存器索引
#define CN_REGISTER_SP 14             // 栈指针寄存器索引
#define CN_REGISTER_FP 13             // 帧指针寄存器索引
```

### 状态标志
```c
#define STATUS_ZERO 0x0001            // 零标志
#define STATUS_CARRY 0x0002           // 进位标志
#define STATUS_OVERFLOW 0x0004        // 溢出标志
#define STATUS_NEGATIVE 0x0008        // 负标志
#define STATUS_INTERRUPT 0x0010       // 中断标志
#define STATUS_DEBUG 0x0020           // 调试标志
```

### 错误码
```c
typedef enum Eum_ExecutionError_t {
    EXECUTION_OK = 0,                 // 执行成功
    EXECUTION_STACK_OVERFLOW = 1,     // 栈溢出
    EXECUTION_STACK_UNDERFLOW = 2,    // 栈下溢
    EXECUTION_INVALID_OPCODE = 3,     // 无效操作码
    EXECUTION_DIVISION_BY_ZERO = 4,   // 除零错误
    EXECUTION_MEMORY_ACCESS = 5,      // 内存访问错误
    EXECUTION_INSTRUCTION_LIMIT = 6,  // 指令限制
    EXECUTION_INTERNAL_ERROR = 7,     // 内部错误
    EXECUTION_BREAKPOINT = 8,         // 断点触发
    EXECUTION_HALT = 9                // 正常停止
} Eum_ExecutionError_t;
```

### 限制常量
```c
#define CN_MAX_BREAKPOINTS 128        // 最大断点数
#define CN_MAX_STACK_DEPTH 65536      // 最大栈深度
#define CN_MAX_LOCAL_VARIABLES 256    // 最大本地变量数
#define CN_DEFAULT_HEAP_SIZE 1048576  // 默认堆大小（1MB）
```

## 函数接口

### 工厂函数

#### F_create_bytecode_executor
```c
/**
 * @brief 创建字节码执行器实例
 * @return 成功返回执行引擎接口指针，失败返回NULL
 * 
 * @details
 * 创建新的字节码执行器实例。执行器使用默认配置，需要调用
 * initialize方法进行初始化后才能使用。
 * 
 * @note
 * - 返回的接口指针必须通过destroy方法释放
 * - 执行器实例是独立的，可以创建多个
 * - 创建失败通常是由于内存不足
 * 
 * @example
 * ```c
 * Stru_ExecutionEngineInterface_t* executor = F_create_bytecode_executor();
 * if (!executor) {
 *     fprintf(stderr, "创建执行器失败\n");
 *     return -1;
 * }
 * ```
 */
Stru_ExecutionEngineInterface_t* F_create_bytecode_executor(void);
```

#### F_create_bytecode_executor_with_config
```c
/**
 * @brief 使用配置创建字节码执行器实例
 * @param config 执行器配置
 * @return 成功返回执行引擎接口指针，失败返回NULL
 * 
 * @details
 * 使用指定的配置创建执行器实例。配置控制虚拟机的资源分配
 * 和行为特性。配置在创建时应用，运行时不能修改。
 * 
 * @note
 * - 如果config为NULL，使用默认配置
 * - 配置中的资源限制在创建时分配
 * - 复杂的配置可能增加创建时间
 * 
 * @example
 * ```c
 * Stru_ExecutorConfig_t config = {
 *     .operand_stack_size = 8192,
 *     .call_stack_size = 16384,
 *     .heap_size = 1048576,
 *     .enable_debug = true,
 *     .enable_profiling = true
 * };
 * Stru_ExecutionEngineInterface_t* executor = 
 *     F_create_bytecode_executor_with_config(&config);
 * ```
 */
Stru_ExecutionEngineInterface_t* F_create_bytecode_executor_with_config(
    const Stru_ExecutorConfig_t* config);
```

### 核心接口方法

#### initialize
```c
/**
 * @brief 初始化执行引擎
 * @param config 执行配置参数
 * @return 初始化成功返回true，否则返回false
 * 
 * @details
 * 初始化执行引擎，分配栈和内存资源。如果执行器是通过
 * F_create_bytecode_executor_with_config创建的，config参数
 * 可以覆盖部分配置（如调试标志）。
 * 
 * @note
 * - 必须先初始化才能执行字节码
 * - 可以多次调用重新初始化（会释放原有资源）
 * - 初始化失败会设置错误状态
 * 
 * @errors
 * - 内存分配失败返回false
 * - 配置无效返回false
 * - 已经运行中返回false
 * 
 * @example
 * ```c
 * Stru_ExecutionConfig_t exec_config = {
 *     .stack_size = 4096,
 *     .heap_size = 8192,
 *     .enable_debug = false
 * };
 * if (!executor->initialize(&exec_config)) {
 *     fprintf(stderr, "初始化失败\n");
 *     executor->destroy();
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
 * 加载字节码对象到执行引擎，准备执行。执行器会验证字节码
 * 的兼容性，并初始化执行环境（重置PC、栈等）。
 * 
 * @note
 * - bytecode必须是通过字节码加载器加载的有效对象
 * - 加载会重置所有执行状态
 * - 可以多次调用加载不同的字节码
 * 
 * @errors
 * - bytecode为NULL返回false
 * - 字节码版本不兼容返回false
 * - 内存不足返回false
 * - 已经运行中返回false
 * 
 * @example
 * ```c
 * Stru_BytecodeObject_t* bytecode = loader->load_from_file("program.cnbc");
 * if (!executor->load_bytecode(bytecode)) {
 *     fprintf(stderr, "加载字节码失败\n");
 *     loader->destroy_bytecode(bytecode);
 *     executor->destroy();
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
 * 从指定地址开始执行字节码。执行是同步的，函数会运行直到：
 * 1. 遇到HALT指令
 * 2. 发生错误
 * 3. 达到指令限制
 * 4. 触发断点（如果启用调试）
 * 
 * @note
 * - start_address为0表示从入口点开始
 * - 执行期间可以响应断点
 * - 执行错误会设置详细的错误信息
 * 
 * @errors
 * - 未初始化返回false
 * - 未加载字节码返回false
 * - 地址越界返回false
 * - 执行错误返回false
 * 
 * @example
 * ```c
 * // 从入口点执行完整程序
 * if (!executor->execute(0)) {
 *     Stru_ExecutionState_t state;
 *     executor->get_state(&state);
 *     fprintf(stderr, "执行失败: %s (错误码: %d)\n",
 *             state.error_message, state.error_code);
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
 * 函数会更新所有状态信息，并检查断点。
 * 
 * @note
 * - 必须在加载字节码后调用
 * - 单步执行会计入指令计数器
 * - 返回false表示执行结束或错误
 * 
 * @errors
 * - 未初始化返回false
 * - 未加载字节码返回false
 * - 指令执行错误返回false
 * 
 * @example
 * ```c
 * // 单步执行并显示状态
 * int step_limit = 100;
 * for (int i = 0; i < step_limit; i++) {
 *     if (!executor->step()) {
 *         printf("执行结束或错误\n");
 *         break;
 *     }
 *     
 *     Stru_ExecutionState_t state;
 *     executor->get_state(&state);
 *     printf("Step %d: PC=0x%04X, SP=0x%04X\n", i, state.pc, state.sp);
 *     
 *     // 检查是否到达断点
 *     if (state.status == EXECUTION_PAUSED) {
 *         printf("在断点暂停\n");
 *         break;
 *     }
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
 * 获取当前的完整执行状态，包括寄存器、栈指针、程序计数器、
 * 状态标志和错误信息。状态信息是执行时的快照。
 * 
 * @note
 * - state必须是非NULL的有效指针
 * - 状态信息可能很大，获取需要时间
 * - 获取状态不会影响执行
 * 
 * @errors
 * - state为NULL返回false
 * - 未初始化返回false
 * 
 * @example
 * ```c
 * Stru_ExecutionState_t state;
 * if (executor->get_state(&state)) {
 *     printf("执行状态:\n");
 *     printf("  PC: 0x%08X\n", state.pc);
 *     printf("  SP: 0x%08X\n", state.sp);
 *     printf("  FP: 0x%08X\n", state.fp);
 *     printf("  状态: %s\n", execution_status_to_string(state.status));
 *     
 *     if (state.error_code != EXECUTION_OK) {
 *         printf("  错误: %s (代码: %d)\n",
 *                state.error_message, state.error_code);
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
 * 在指定地址设置断点。当执行到断点地址时，执行会暂停，
 * 状态变为EXECUTION_PAUSED。断点用于调试和控制执行流程。
 * 
 * @note
 * - 地址必须在字节码范围内
 * - 断点数量有限制（CN_MAX_BREAKPOINTS）
 * - 相同的地址只能设置一个断点
 * - 断点在单步和执行时都有效
 * 
 * @errors
 * - 地址越界返回false
 * - 断点数量超限返回false
 * - 重复地址返回false
 * - 未加载字节码返回false
 * 
 * @example
 * ```c
 * // 在关键位置设置断点
 * uint32_t breakpoints[] = {0x0100, 0x0200, 0x0300};
 * for (int i = 0; i < 3; i++) {
 *     if (!executor->set_breakpoint(breakpoints[i])) {
 *         fprintf(stderr, "设置断点 0x%04X 失败\n", breakpoints[i]);
 *     }
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
 * 清除后执行不再在该地址暂停。
 * 
 * @note
 * - 地址必须已设置断点
 * - 清除不存在的断点返回false
 * - 断点清除后可以重新设置
 * 
 * @errors
 * - 地址未设置断点返回false
 * 
 * @example
 * ```c
 * // 设置断点
 * executor->set_breakpoint(0x0100);
 * 
 * // 执行到断点
 * executor->execute(0);
 * 
 * // 清除断点继续执行
 * executor->clear_breakpoint(0x0100);
 * executor->execute(executor->get_state()->pc);
 * ```
 */
bool clear_breakpoint(uint32_t address);
```

#### clear_all_breakpoints
```c
/**
 * @brief 清除所有断点
 * 
 * @details
 * 清除所有已设置的断点。用于重置调试状态或结束调试会话。
 * 
 * @note
 * - 操作立即生效
 * - 不会影响执行状态
 * - 可以安全地多次调用
 * 
 * @example
 * ```c
 * // 设置多个断点
 * executor->set_breakpoint(0x0100);
 * executor->set_breakpoint(0x0200);
 * executor->set_breakpoint(0x0300);
 * 
 * // 调试完成后清除所有断点
 * executor->clear_all_breakpoints();
 * ```
 */
void clear_all_breakpoints(void);
```

#### destroy
```c
/**
 * @brief 销毁执行引擎实例
 * 
 * @details
 * 释放执行引擎占用的所有资源，包括：
 * 1. 栈和堆内存
 * 2. 执行状态和上下文
 * 3. 断点列表
 * 4
