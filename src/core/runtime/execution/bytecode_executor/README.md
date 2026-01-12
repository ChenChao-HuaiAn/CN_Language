# 字节码执行器模块 (Bytecode Executor)

## 概述

字节码执行器模块是CN_Language运行时系统的核心执行组件，负责实际执行字节码指令。该模块实现了完整的字节码虚拟机，包括指令解码、执行、栈管理和状态维护等功能。

## 功能特性

- **指令执行**: 支持完整的字节码指令集执行
- **栈管理**: 提供操作数栈和调用栈管理
- **寄存器管理**: 管理虚拟机的寄存器状态
- **内存访问**: 支持字节码内存空间的读写操作
- **异常处理**: 提供执行异常检测和处理机制
- **调试支持**: 集成单步执行、断点、状态查看等调试功能

## 架构设计

### 模块位置
- **层级**: 核心层 (Core Layer)
- **路径**: `src/core/runtime/execution/bytecode_executor/`
- **依赖**: 基础设施层 (内存管理)、核心层 (字节码加载器)

### 虚拟机架构
字节码执行器采用基于栈的虚拟机架构：

```
+-----------------------+
|   字节码执行器        |
+-----------------------+
|  - 指令解码器         |
|  - 执行引擎           |
|  - 栈管理器           |
|  - 寄存器文件         |
|  - 内存控制器         |
+-----------------------+
|  - 操作数栈 (64KB)    |
|  - 调用栈 (128KB)     |
|  - 本地变量区         |
+-----------------------+
```

### 接口设计
执行器实现了`Stru_ExecutionEngineInterface_t`接口：

```c
typedef struct Stru_BytecodeExecutor_t {
    Stru_ExecutionEngineInterface_t interface;  // 公共接口
    
    // 执行状态
    Stru_BytecodeObject_t* bytecode;           // 当前加载的字节码
    Stru_ExecutionState_t state;               // 执行状态
    Stru_ExecutorConfig_t config;              // 配置参数
    
    // 运行时组件
    Stru_Stack_t* operand_stack;               // 操作数栈
    Stru_Stack_t* call_stack;                  // 调用栈
    uint8_t* local_variables;                  // 本地变量区
    uint8_t* memory_space;                     // 内存空间
    
    // 控制标志
    bool is_running;                           // 运行状态
    bool is_paused;                            // 暂停状态
    bool has_error;                            // 错误状态
} Stru_BytecodeExecutor_t;
```

## API文档

### 工厂函数

#### `F_create_bytecode_executor()`
```c
/**
 * @brief 创建字节码执行器实例
 * @return 成功返回执行引擎接口指针，失败返回NULL
 * @note 调用者负责释放返回的接口指针
 */
Stru_ExecutionEngineInterface_t* F_create_bytecode_executor(void);
```

### 核心接口方法

#### `initialize()`
```c
/**
 * @brief 初始化执行引擎
 * @param config 配置参数
 * @return 初始化成功返回true，否则返回false
 * @note 配置包括栈大小、内存大小等参数
 */
bool initialize(const Stru_ExecutionConfig_t* config);
```

#### `load_bytecode()`
```c
/**
 * @brief 加载字节码到执行引擎
 * @param bytecode 字节码对象
 * @return 加载成功返回true，否则返回false
 * @note 字节码对象必须已通过格式验证
 */
bool load_bytecode(Stru_BytecodeObject_t* bytecode);
```

#### `execute()`
```c
/**
 * @brief 执行字节码
 * @param start_address 起始执行地址（字节码偏移）
 * @return 执行成功返回true，执行错误返回false
 * @note 从指定地址开始执行，直到遇到HALT指令或错误
 */
bool execute(uint32_t start_address);
```

#### `step()`
```c
/**
 * @brief 单步执行字节码
 * @return 执行成功返回true，执行结束或错误返回false
 * @note 每次执行一条指令，用于调试
 */
bool step(void);
```

#### `get_state()`
```c
/**
 * @brief 获取执行引擎状态
 * @param state 状态输出参数
 * @return 获取成功返回true，否则返回false
 * @note 状态包括PC、SP、寄存器值等
 */
bool get_state(Stru_ExecutionState_t* state);
```

#### `set_breakpoint()`
```c
/**
 * @brief 设置断点
 * @param address 断点地址（字节码偏移）
 * @return 设置成功返回true，否则返回false
 * @note 断点在单步执行或执行时触发
 */
bool set_breakpoint(uint32_t address);
```

#### `destroy()`
```c
/**
 * @brief 销毁执行引擎实例
 * @note 释放所有分配的资源
 */
void destroy(void);
```

## 使用示例

### 基本执行
```c
#include "CN_bytecode_executor.h"
#include "CN_bytecode_loader.h"

int main() {
    // 创建字节码加载器
    Stru_BytecodeLoaderInterface_t* loader = F_create_bytecode_loader();
    if (!loader) {
        return -1;
    }
    
    // 加载字节码文件
    Stru_BytecodeObject_t* bytecode = loader->load_from_file("program.cnbc");
    if (!bytecode) {
        loader->destroy(loader);
        return -1;
    }
    
    // 创建执行器
    Stru_ExecutionEngineInterface_t* executor = F_create_bytecode_executor();
    if (!executor) {
        loader->destroy_bytecode(bytecode);
        loader->destroy(loader);
        return -1;
    }
    
    // 配置执行器
    Stru_ExecutionConfig_t config = {
        .stack_size = 4096,
        .heap_size = 8192,
        .enable_debug = false,
        .max_instructions = 1000000
    };
    
    // 初始化执行器
    if (!executor->initialize(&config)) {
        printf("初始化失败\n");
        goto cleanup;
    }
    
    // 加载字节码
    if (!executor->load_bytecode(bytecode)) {
        printf("加载字节码失败\n");
        goto cleanup;
    }
    
    // 执行字节码（从入口点0开始）
    if (!executor->execute(0)) {
        printf("执行失败\n");
    } else {
        printf("执行成功\n");
    }
    
cleanup:
    // 清理资源
    executor->destroy();
    loader->destroy_bytecode(bytecode);
    loader->destroy(loader);
    
    return 0;
}
```

### 调试执行
```c
// 启用调试模式
Stru_ExecutionConfig_t debug_config = {
    .stack_size = 4096,
    .heap_size = 8192,
    .enable_debug = true,  // 启用调试
    .max_instructions = 1000
};

executor->initialize(&debug_config);
executor->load_bytecode(bytecode);

// 设置断点
executor->set_breakpoint(0x100);
executor->set_breakpoint(0x200);

// 单步执行并显示状态
int step_count = 0;
while (executor->step()) {
    Stru_ExecutionState_t state;
    if (executor->get_state(&state)) {
        printf("Step %d: PC=0x%04X, SP=0x%04X\n", 
               step_count++, state.pc, state.sp);
    }
    
    // 检查是否到达断点
    if (state.pc == 0x100 || state.pc == 0x200) {
        printf("到达断点 0x%04X\n", state.pc);
        // 可以在这里检查内存、寄存器等
    }
}
```

## 指令集支持

### 算术指令
- `ADD`, `SUB`, `MUL`, `DIV`, `MOD` - 基本算术运算
- `INC`, `DEC` - 自增自减
- `NEG` - 取负

### 逻辑指令
- `AND`, `OR`, `XOR`, `NOT` - 逻辑运算
- `SHL`, `SHR` - 移位运算

### 比较指令
- `CMP` - 比较
- `TEST` - 测试

### 控制流指令
- `JMP` - 无条件跳转
- `JZ`, `JNZ`, `JE`, `JNE`, `JG`, `JGE`, `JL`, `JLE` - 条件跳转
- `CALL`, `RET` - 函数调用和返回
- `HALT` - 停止执行

### 栈操作指令
- `PUSH`, `POP` - 栈操作
- `PUSHI`, `PUSHF` - 立即数入栈
- `DUP`, `SWAP` - 栈操作

### 内存指令
- `LOAD`, `STORE` - 内存读写
- `LOADI`, `STOREF` - 立即数内存操作

### 系统指令
- `NOP` - 空操作
- `SYSCALL` - 系统调用
- `BREAK` - 断点指令

## 执行流程

### 指令执行循环
```c
while (is_running && !has_error) {
    // 1. 获取当前指令
    uint8_t opcode = fetch_instruction();
    
    // 2. 解码指令
    InstructionInfo info = decode_instruction(opcode);
    
    // 3. 执行指令
    execute_instruction(info);
    
    // 4. 更新程序计数器
    update_program_counter(info);
    
    // 5. 检查断点
    check_breakpoints();
    
    // 6. 检查执行限制
    if (instruction_count >= max_instructions) {
        has_error = true;
        error_code = CN_EXECUTION_INSTRUCTION_LIMIT;
        break;
    }
    
    instruction_count++;
}
```

### 异常处理
执行器检测以下异常情况：
1. **栈溢出**: 操作数栈或调用栈超出限制
2. **非法指令**: 未定义的指令操作码
3. **内存访问越界**: 访问超出分配的内存空间
4. **除零错误**: 整数除法除数为零
5. **指令限制**: 超过最大指令执行数

## 性能优化

### 指令缓存
- 使用指令缓存减少内存访问
- 预解码常用指令序列
- 热点代码优化

### 栈优化
- 栈帧预分配
- 栈操作批处理
- 栈内存对齐

### 内存管理
- 内存池分配
- 缓存友好的数据结构
- 减少内存碎片

## 测试策略

### 单元测试
- 测试每条指令的正确执行
- 测试边界条件和异常情况
- 测试栈和内存操作

### 集成测试
- 与字节码加载器集成测试
- 端到端编译执行测试
- 性能基准测试

### 压力测试
- 大程序执行测试
- 长时间运行稳定性测试
- 内存和资源使用测试

## 依赖关系

### 内部依赖
- `src/core/runtime/execution/bytecode_loader/` - 字节码加载器
- `src/infrastructure/memory/` - 内存管理
- `src/infrastructure/containers/` - 栈数据结构

### 外部依赖
- C标准库
- 平台相关的性能计数器

## 配置选项

### 执行配置
```c
typedef struct Stru_ExecutorConfig_t {
    uint32_t stack_size;           // 操作数栈大小（字节）
    uint32_t call_stack_size;      // 调用栈大小（字节）
    uint32_t heap_size;            // 堆大小（字节）
    uint32_t max_instructions;     // 最大指令执行数
    bool enable_debug;             // 启用调试支持
    bool enable_profiling;         // 启用性能分析
    bool strict_mode;              // 严格模式（更多检查）
} Stru_ExecutorConfig_t;
```

### 性能调优
执行器支持多种性能调优选项：
1. **指令预取**: 预取下一条指令
2. **栈缓存**: 缓存栈顶元素
3. **内存池**: 使用内存池减少分配开销
4. **JIT编译**: 支持热点代码的JIT编译（未来扩展）

## 维护说明

### 版本兼容性
- 保持指令集的向后兼容性
- 提供指令集版本管理
- 支持旧版本字节码的执行

### 扩展性
- 通过插件支持新指令
- 可配置的执行策略
- 支持不同的优化级别

## 相关文档

- [字节码指令集规范](../../../docs/specifications/字节码指令集.md)
- [执行引擎API文档](../../../docs/api/core/runtime/execution/bytecode_executor.md)
- [虚拟机设计文档](../../../docs/architecture/004-虚拟机设计.md)

---

**最后更新**: 2026年1月12日  
**版本**: 1.0.0  
**作者**: CN_Language开发团队  
**状态**: 正式发布
