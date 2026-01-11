# CN_Language 中间代码表示（IR）模块

## 概述

中间代码表示（Intermediate Representation，IR）模块是CN_Language编译器的核心组件之一，负责将抽象语法树（AST）转换为适合优化和代码生成的中间形式。本模块采用分层架构设计，支持多种IR形式。

## 设计原则

1. **模块化设计**：每个IR类型独立实现，通过统一接口访问
2. **可扩展性**：支持添加新的IR类型和优化算法
3. **平台无关性**：IR独立于目标平台
4. **优化友好**：支持各种编译器优化

## 支持的IR类型

### 1. 三地址码（Three-Address Code，TAC）
- **描述**：每条指令最多包含三个操作数
- **适用场景**：中级优化、简单代码生成
- **状态**：已实现基础功能

### 2. 静态单赋值形式（Static Single Assignment，SSA）
- **描述**：每个变量只被赋值一次，便于优化
- **适用场景**：高级优化、数据流分析
- **状态**：设计中

### 3. 控制流图（Control Flow Graph，CFG）
- **描述**：表示程序的控制流结构
- **适用场景**：控制流分析、循环优化
- **状态**：设计中

## 核心接口

### IR接口（`CN_ir_interface.h`）
```c
typedef struct Stru_IrInterface_t {
    // 基本信息
    const char* (*get_name)(void);
    const char* (*get_version)(void);
    Eum_IrType (*get_type)(void);
    
    // IR构建
    void* (*create_from_ast)(Stru_AstNode_t* ast);
    void* (*create_empty)(void);
    
    // IR操作
    bool (*add_instruction)(void* ir, Stru_IrInstruction_t* instr);
    bool (*remove_instruction)(void* ir, size_t index);
    Stru_IrInstruction_t* (*get_instruction)(void* ir, size_t index);
    size_t (*get_instruction_count)(void* ir);
    
    // IR转换
    void* (*convert_to_lower)(void* ir);
    void* (*convert_to_higher)(void* ir);
    void* (*convert_to_type)(void* ir, Eum_IrType target_type);
    
    // IR优化
    bool (*apply_optimization)(void* ir, int optimization_type);
    bool (*apply_all_optimizations)(void* ir, int optimization_level);
    
    // 序列化/反序列化
    const char* (*serialize)(void* ir);
    void* (*deserialize)(const char* data);
    
    // 验证
    bool (*validate)(void* ir);
    
    // 资源管理
    void (*destroy)(void* ir);
} Stru_IrInterface_t;
```

### TAC接口（`CN_tac_interface.h`）
```c
typedef struct Stru_TacInstruction_t {
    Eum_TacOpcode opcode;        ///< TAC操作码
    char* result;                ///< 结果操作数
    char* operand1;              ///< 第一个操作数
    char* operand2;              ///< 第二个操作数
    char* label;                 ///< 标签（用于跳转指令）
    size_t line;                 ///< 源代码行号
    size_t column;               ///< 源代码列号
    void* extra_data;            ///< 额外数据
} Stru_TacInstruction_t;
```

## 目录结构

```
src/core/ir/
├── CN_ir_interface.h          # IR核心接口定义
├── CN_ir_interface.c          # IR接口实现
├── CN_ir_builder.h            # IR构建器接口
├── README.md                  # 本文件
├── implementations/
│   ├── tac/                   # 三地址码实现
│   │   ├── CN_tac_interface.h # TAC接口定义
│   │   ├── CN_tac_impl.c      # TAC实现
│   │   └── README.md          # TAC文档
│   ├── ssa/                   # SSA实现（待实现）
│   └── cfg/                   # 控制流图实现（待实现）
├── optimizations/             # IR优化实现（待实现）
└── analysis/                  # 数据流分析（待实现）
```

## 使用示例

### 创建TAC IR
```c
#include "CN_ir_interface.h"
#include "implementations/tac/CN_tac_interface.h"

// 创建TAC接口
Stru_IrInterface_t* tac_interface = F_create_ir_interface(Eum_IR_TYPE_TAC);
if (!tac_interface) {
    // 错误处理
}

// 创建空的TAC
void* tac_ir = tac_interface->create_empty();
if (!tac_ir) {
    // 错误处理
}

// 添加指令
Stru_IrInstruction_t* instr = F_create_ir_instruction(
    Eum_IR_INSTR_BINARY_OP, "add", 1, 1);
F_ir_instruction_add_operand(instr, "t1");
F_ir_instruction_add_operand(instr, "a");
F_ir_instruction_add_operand(instr, "b");

tac_interface->add_instruction(tac_ir, instr);

// 序列化IR
const char* serialized = tac_interface->serialize(tac_ir);
printf("Serialized IR:\n%s\n", serialized);

// 清理资源
tac_interface->destroy(tac_ir);
F_destroy_ir_interface(tac_interface);
```

## 优化支持

### 已实现的优化
1. **常量折叠**：在编译时计算常量表达式
2. **死代码消除**：移除不会执行的代码

### 计划实现的优化
1. 公共子表达式消除
2. 循环不变代码外提
3. 强度削减
4. 函数内联

## 构建和测试

### 构建
```bash
# 从项目根目录构建
make

# 仅构建IR模块
make ir
```

### 测试
```bash
# 运行IR模块测试
make test-ir

# 运行特定测试
./bin/test_ir_interface
./bin/test_tac_ir
```

## API文档

详细的API文档位于：
- `docs/api/core/ir/README.md` - IR模块API概览
- `docs/api/core/ir/` - 各子模块详细API文档

## 开发指南

### 添加新的IR类型
1. 在`CN_ir_interface.h`中定义新的`Eum_IrType`枚举值
2. 在`CN_ir_interface.c`的`F_create_ir_interface`函数中添加对新类型的支持
3. 在`implementations/`目录下创建新的实现目录
4. 实现新的IR接口
5. 添加测试用例

### 添加新的优化算法
1. 在`optimizations/`目录下创建新的优化实现文件
2. 实现优化算法
3. 在IR接口中添加对优化的支持
4. 添加测试用例

## 性能考虑

1. **内存管理**：使用内存池分配IR节点
2. **构建性能**：增量构建，缓存优化
3. **优化性能**：使用高效的数据结构
4. **序列化性能**：支持二进制序列化

## 错误处理

IR模块使用统一的错误处理机制：
- 错误信息存储在IR实例中
- 可以通过接口函数获取错误信息
- 支持错误清除和重置

## 版本历史

### v1.0.0 (2026-01-10)
- 初始版本发布
- 支持三地址码IR
- 基础IR接口实现
- 常量折叠和死代码消除框架

### v1.1.0 (计划中)
- 完善TAC实现
- 添加SSA支持
- 添加控制流图支持
- 实现更多优化算法

## 维护者

- CN_Language架构委员会
- IR模块工作组

## 许可证

MIT许可证
