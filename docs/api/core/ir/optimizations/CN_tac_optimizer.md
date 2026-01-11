# CN_tac_optimizer - 三地址码优化器API文档

## 概述

`CN_tac_optimizer` 模块提供了三地址码（TAC）的优化功能，支持多种TAC级别的优化算法。该模块是CN_Language编译器优化系统的重要组成部分，专门针对三地址码中间表示进行优化。

## 头文件

```c
#include "CN_tac_optimizer.h"
```

## 数据结构

### TacConstantFoldingPrivateData - TAC常量折叠器私有数据

```c
typedef struct {
    Stru_HashTable_t* constant_values;   ///< 常量值映射表
    Stru_HashTable_t* expression_cache;  ///< 表达式缓存
    bool aggressive_folding;             ///< 是否启用激进折叠
    size_t max_cache_size;               ///< 最大缓存大小
    size_t cache_hits;                   ///< 缓存命中次数
    size_t cache_misses;                 ///< 缓存未命中次数
} TacConstantFoldingPrivateData;
```

**字段说明：**
- `constant_values`: 常量值映射表，存储变量名到常量值的映射
- `expression_cache`: 表达式缓存，存储已计算的表达式结果
- `aggressive_folding`: 是否启用激进折叠模式
- `max_cache_size`: 最大缓存大小限制
- `cache_hits`: 缓存命中次数统计
- `cache_misses`: 缓存未命中次数统计

### TacDeadCodeEliminationPrivateData - TAC死代码消除器私有数据

```c
typedef struct {
    Stru_HashTable_t* used_variables;    ///< 使用的变量集合
    Stru_HashTable_t* defined_variables; ///< 定义的变量集合
    Stru_HashTable_t* reachable_code;    ///< 可达代码集合
    bool analyze_control_flow;           ///< 是否分析控制流
    bool remove_unused_vars;             ///< 是否移除未使用变量
    size_t removed_instructions;         ///< 移除的指令数量
} TacDeadCodeEliminationPrivateData;
```

**字段说明：**
- `used_variables`: 使用的变量集合，记录哪些变量被使用
- `defined_variables`: 定义的变量集合，记录哪些变量被定义
- `reachable_code`: 可达代码集合，记录哪些代码是可达的
- `analyze_control_flow`: 是否分析控制流关系
- `remove_unused_vars`: 是否移除未使用的变量
- `removed_instructions`: 移除的指令数量统计

### TacCommonSubexpressionPrivateData - TAC公共子表达式消除器私有数据

```c
typedef struct {
    Stru_HashTable_t* expression_table;  ///< 表达式哈希表
    Stru_HashTable_t* value_numbering;   ///< 值编号表
    size_t next_value_number;            ///< 下一个值编号
    bool enable_global_cse;              ///< 是否启用全局CSE
    size_t eliminated_expressions;       ///< 消除的表达式数量
} TacCommonSubexpressionPrivateData;
```

**字段说明：**
- `expression_table`: 表达式哈希表，存储已计算的表达式
- `value_numbering`: 值编号表，为表达式分配唯一编号
- `next_value_number`: 下一个可用的值编号
- `enable_global_cse`: 是否启用全局公共子表达式消除
- `eliminated_expressions`: 消除的表达式数量统计

### TacStrengthReductionPrivateData - TAC强度削减器私有数据

```c
typedef struct {
    Stru_HashTable_t* strength_patterns; ///< 强度削减模式表
    Stru_HashTable_t* induction_vars;    ///< 归纳变量表
    bool enable_loop_strength_reduction; ///< 是否启用循环强度削减
    size_t reduced_operations;           ///< 减少的操作数量
} TacStrengthReductionPrivateData;
```

**字段说明：**
- `strength_patterns`: 强度削减模式表，存储强度削减规则
- `induction_vars`: 归纳变量表，记录循环中的归纳变量
- `enable_loop_strength_reduction`: 是否启用循环强度削减
- `reduced_operations`: 减少的操作数量统计

### TacOptimizerPrivateData - TAC优化器私有数据

```c
typedef struct {
    // 优化器组件
    TacConstantFoldingPrivateData* constant_folder;      ///< 常量折叠器
    TacDeadCodeEliminationPrivateData* dead_code_eliminator; ///< 死代码消除器
    TacCommonSubexpressionPrivateData* cse_eliminator;   ///< 公共子表达式消除器
    TacStrengthReductionPrivateData* strength_reducer;   ///< 强度削减器
    
    // 统计信息
    Stru_IrOptimizationStats_t* stats;                   ///< 优化统计信息
    Stru_IrOptimizationConfig_t* config;                 ///< 优化配置
    
    // 状态标志
    bool initialized;                                    ///< 是否已初始化
    bool optimization_in_progress;                       ///< 优化是否进行中
    
    // 性能监控
    double total_optimization_time;                      ///< 总优化时间
    size_t total_optimizations_applied;                  ///< 应用的总优化次数
} TacOptimizerPrivateData;
```

**字段说明：**
- `constant_folder`: 常量折叠器实例
- `dead_code_eliminator`: 死代码消除器实例
- `cse_eliminator`: 公共子表达式消除器实例
- `strength_reducer`: 强度削减器实例
- `stats`: 优化统计信息
- `config`: 优化配置
- `initialized`: 优化器是否已初始化
- `optimization_in_progress`: 优化是否正在进行中
- `total_optimization_time`: 总优化时间统计
- `total_optimizations_applied`: 应用的总优化次数统计

## 工厂函数

### F_create_tac_optimizer_interface

```c
Stru_IrOptimizerInterface_t* F_create_tac_optimizer_interface(void);
```

**功能：** 创建TAC优化器接口实例

**参数：** 无

**返回值：**
- 成功：返回TAC优化器接口实例指针
- 失败：返回NULL

**示例：**
```c
Stru_IrOptimizerInterface_t* optimizer = F_create_tac_optimizer_interface();
if (!optimizer) {
    // 错误处理
}
```

### F_destroy_tac_optimizer_interface

```c
void F_destroy_tac_optimizer_interface(Stru_IrOptimizerInterface_t* optimizer);
```

**功能：** 销毁TAC优化器接口实例及其所有资源

**参数：**
- `optimizer`: TAC优化器接口实例

**返回值：** 无

**示例：**
```c
F_destroy_tac_optimizer_interface(optimizer);
```

## TAC特定优化函数

### F_apply_tac_constant_folding

```c
Stru_IrOptimizationResult_t* F_apply_tac_constant_folding(Stru_TacData_t* tac_data,
                                                         const Stru_IrOptimizationConfig_t* config);
```

**功能：** 对TAC IR应用常量折叠优化

**参数：**
- `tac_data`: TAC数据
- `config`: 优化配置

**返回值：**
- 成功：返回优化结果指针
- 失败：返回NULL

**示例：**
```c
Stru_IrOptimizationResult_t* result = 
    F_apply_tac_constant_folding(tac_data, config);
if (result && result->success) {
    // 优化成功
}
```

### F_apply_tac_dead_code_elimination

```c
Stru_IrOptimizationResult_t* F_apply_tac_dead_code_elimination(Stru_TacData_t* tac_data,
                                                              const Stru_IrOptimizationConfig_t* config);
```

**功能：** 对TAC IR应用死代码消除优化

**参数：**
- `tac_data`: TAC数据
- `config`: 优化配置

**返回值：**
- 成功：返回优化结果指针
- 失败：返回NULL

**示例：**
```c
Stru_IrOptimizationResult_t* result = 
    F_apply_tac_dead_code_elimination(tac_data, config);
if (result && result->success) {
    // 优化成功
}
```

### F_apply_tac_common_subexpression_elimination

```c
Stru_IrOptimizationResult_t* F_apply_tac_common_subexpression_elimination(Stru_TacData_t* tac_data,
                                                                         const Stru_IrOptimizationConfig_t* config);
```

**功能：** 对TAC IR应用公共子表达式消除优化

**参数：**
- `tac_data`: TAC数据
- `config`: 优化配置

**返回值：**
- 成功：返回优化结果指针
- 失败：返回NULL

**示例：**
```c
Stru_IrOptimizationResult_t* result = 
    F_apply_tac_common_subexpression_elimination(tac_data, config);
if (result && result->success) {
    // 优化成功
}
```

### F_apply_tac_strength_reduction

```c
Stru_IrOptimizationResult_t* F_apply_tac_strength_reduction(Stru_TacData_t* tac_data,
                                                           const Stru_IrOptimizationConfig_t* config);
```

**功能：** 对TAC IR应用强度削减优化

**参数：**
- `tac_data`: TAC数据
- `config`: 优化配置

**返回值：**
- 成功：返回优化结果指针
- 失败：返回NULL

**示例：**
```c
Stru_IrOptimizationResult_t* result = 
    F_apply_tac_strength_reduction(tac_data, config);
if (result && result->success) {
    // 优化成功
}
```

### F_apply_tac_peephole_optimization

```c
Stru_IrOptimizationResult_t* F_apply_tac_peephole_optimization(Stru_TacData_t* tac_data,
                                                              const Stru_IrOptimizationConfig_t* config);
```

**功能：** 对TAC IR应用窥孔优化

**参数：**
- `tac_data`: TAC数据
- `config`: 优化配置

**返回值：**
- 成功：返回优化结果指针
- 失败：返回NULL

**示例：**
```c
Stru_IrOptimizationResult_t* result = 
    F_apply_tac_peephole_optimization(tac_data, config);
if (result && result->success) {
    // 优化成功
}
```

### F_analyze_tac_optimization_potential

```c
bool F_analyze_tac_optimization_potential(Stru_TacData_t* tac_data,
                                         void** potential_areas,
                                         size_t* area_count);
```

**功能：** 分析TAC IR的优化潜力，识别可优化的区域

**参数：**
- `tac_data`: TAC数据
- `potential_areas`: 输出参数，潜在优化区域数组
- `area_count`: 输出参数，区域数量

**返回值：**
- 成功：返回true
- 失败：返回false

**示例：**
```c
void* potential_areas[10];
size_t area_count = 0;
bool success = F_analyze_tac_optimization_potential(tac_data, potential_areas, &area_count);
if (success) {
    // 分析成功，potential_areas包含潜在优化区域
}
```

### F_validate_tac_optimization

```c
bool F_validate_tac_optimization(const Stru_TacData_t* original_tac,
                                const Stru_TacData_t* optimized_tac);
```

**功能：** 验证TAC优化前后的正确性

**参数：**
- `original_tac`: 原始TAC数据
- `optimized_tac`: 优化后TAC数据

**返回值：**
- 优化正确：返回true
- 优化错误：返回false

**示例：**
```c
bool valid = F_validate_tac_optimization(original_tac, optimized_tac);
if (valid) {
    // 优化正确
} else {
    // 优化错误
}
```

### F_calculate_tac_instruction_stats

```c
bool F_calculate_tac_instruction_stats(const Stru_TacData_t* tac_data,
                                      Stru_IrOptimizationStats_t* stats);
```

**功能：** 计算TAC指令的统计信息

**参数：**
- `tac_data`: TAC数据
- `stats`: 输出参数，统计信息

**返回值：**
- 成功：返回true
- 失败：返回false

**示例：**
```c
Stru_IrOptimizationStats_t stats;
bool success = F_calculate_tac_instruction_stats(tac_data, &stats);
if (success) {
    // 统计信息计算成功
}
```

### F_optimize_tac_instruction_sequence

```c
Stru_TacInstruction_t** F_optimize_tac_instruction_sequence(Stru_TacInstruction_t** instructions,
                                                           size_t count,
                                                           const Stru_IrOptimizationConfig_t* config,
                                                           size_t* optimized_count);
```

**功能：** 优化TAC指令序列，应用多种优化

**参数：**
- `instructions`: TAC指令数组
- `count`: 指令数量
- `config`: 优化配置
- `optimized_count`: 输出参数，优化后指令数量

**返回值：**
- 成功：返回优化后的指令数组
- 失败：返回NULL

**示例：**
```c
size_t optimized_count = 0;
Stru_TacInstruction_t** optimized_instructions = 
    F_optimize_tac_instruction_sequence(instructions, count, config, &optimized_count);
if (optimized_instructions) {
    // 优化成功
}
```

## TAC优化工具函数

### F_is_tac_instruction_constant

```c
bool F_is_tac_instruction_constant(const Stru_TacInstruction_t* instruction);
```

**功能：** 检查TAC指令是否为常量表达式

**参数：**
- `instruction`: TAC指令

**返回值：**
- 是常量表达式：返回true
- 不是常量表达式：返回false

**示例：**
```c
bool is_constant = F_is_tac_instruction_constant(instruction);
if (is_constant) {
    // 指令是常量表达式
}
```

### F_fold_constant_tac_instruction

```c
bool F_fold_constant_tac_instruction(const Stru_TacInstruction_t* instruction,
                                    char** result);
```

**功能：** 折叠常量TAC指令，计算其结果

**参数：**
- `instruction`: TAC指令
- `result`: 输出参数，结果字符串

**返回值：**
- 成功折叠：返回true
- 无法折叠：返回false

**示例：**
```c
char* folded_result = NULL;
bool folded = F_fold_constant_tac_instruction(instruction, &folded_result);
if (folded && folded_result) {
    // 折叠成功，folded_result包含结果
    cn_free(folded_result);
}
```

### F_is_tac_instruction_dead_code

```c
bool F_is_tac_instruction_dead_code(const Stru_TacInstruction_t* instruction,
                                   const Stru_HashTable_t* used_vars,
                                   const Stru_HashTable_t* defined_vars);
```

**功能：** 检查TAC指令是否死代码

**参数：**
- `instruction`: TAC指令
- `used_vars`: 使用的变量集合
- `defined_vars`: 定义的变量集合

**返回值：**
- 是死代码：返回true
- 不是死代码：返回false

**示例：**
```c
bool is_dead = F_is_tac_instruction_dead_code(instruction, used_vars, defined_vars);
if (is_dead) {
    // 指令是死代码，可以移除
}
```

### F_hash_tac_expression

```c
uint64_t F_hash_tac_expression(const Stru_TacInstruction_t* instruction);
```

**功能：** 计算TAC表达式的哈希值，用于公共子表达式消除

**参数：**
- `instruction`: TAC指令

**返回值：** 表达式哈希值

**示例：**
```c
uint64_t hash = F_hash_tac_expression(instruction);
// 使用哈希值进行表达式比较
```

### F_are_tac_expressions_equivalent

```c
bool F_are_tac_expressions_equivalent(const Stru_TacInstruction_t* instr1,
                                     const Stru_TacInstruction_t* instr2);
```

**功能：** 检查两个TAC表达式是否等价

**参数：**
- `instr1`: 第一个TAC指令
- `instr2`: 第二个TAC指令

**返回值：**
- 表达式等价：返回true
- 表达式不等价：返回false

**示例：**
```c
bool equivalent = F_are_tac_expressions_equivalent(instr1, instr2);
if (equivalent) {
    // 表达式等价，可以进行公共子表达式消除
}
```

### F_apply_tac_strength_reduction_pattern

```c
bool F_apply_tac_strength_reduction_pattern(const Stru_TacInstruction_t* instruction,
                                           Stru_TacInstruction_t** reduced_instr);
```

**功能：** 对TAC指令应用强度削减模式

**参数：**
- `instruction`: TAC指令
- `reduced_instr`: 输出参数，削减后的指令

**返回值：**
- 成功应用：返回true
- 无法应用：返回false

**示例：**
```c
Stru_TacInstruction_t* reduced_instruction = NULL;
bool reduced = F_apply_tac_strength_reduction_pattern(instruction, &reduced_instruction);
if (reduced && reduced_instruction) {
    // 强度削减成功
}
```

### F_apply_tac_peephole_pattern

```c
size_t F_apply_tac_peephole_pattern(Stru_TacInstruction_t** instructions,
                                   size_t count,
                                   size_t start_index,
                                   bool* optimized);
```

**功能：** 对TAC指令序列应用窥孔优化模式

**参数：**
- `instructions`: TAC指令数组
- `count`: 指令数量
- `start_index`: 起始索引
- `optimized`: 输出参数，是否优化

**返回值：** 优化的指令数量

**示例：**
```c
bool optimized = false;
size_t optimized_count = F_apply_tac_peephole_pattern(instructions, count, 0, &optimized);
if (optimized) {
    // 窥孔优化成功应用
}
```

## TAC优化器版本信息函数

### F_get_tac_optimizer_version

```c
void F_get_tac_optimizer_version(int* major, int* minor, int* patch);
```

**功能：** 获取TAC优化器的版本信息

**参数：**
- `major`: 输出参数，主版本号
- `minor`: 输出参数，次版本号
- `patch`: 输出参数，修订号

**返回值：** 无

**示例：**
```c
int major, minor, patch;
F_get_tac_optimizer_version(&major, &minor, &patch);
printf("TAC优化器版本: %d.%d.%d\n", major, minor, patch);
```

### F_get_tac_optimizer_version_string

```c
const char* F_get_tac_optimizer_version_string(void);
```

**功能：** 获取TAC优化器的版本字符串

**参数：** 无

**返回值：** 版本字符串

**示例：**
```c
const char* version = F_get_tac_optimizer_version_string();
printf("TAC优化器版本: %s\n", version);
```

## 使用示例

### 完整示例：应用TAC优化

```c
#include "CN_tac_optimizer.h"
#include <stdio.h>

void apply_tac_optimizations_example(Stru_TacData_t* tac_data) {
    // 创建默认优化配置
    Stru_IrOptimizationConfig_t* config = F_create_default_ir_optimization_config();
    if (!config) {
        printf("无法创建优化配置\n");
        return;
    }
    
    // 启用所有优化
    config->enable_constant_folding = true;
    config->enable_dead_code_elimination = true;
    config->enable_common_subexpression = true;
    config->enable_strength_reduction = true;
    config->enable_peephole_optimization = true;
    
    // 应用常量折叠优化
    Stru_IrOptimizationResult_t* constant_folding_result = 
        F_apply_tac_constant_folding(tac_data, config);
    if (constant_folding_result && constant_folding_result->success) {
        printf("常量折叠优化成功\n");
        printf("移除指令: %zu\n", constant_folding_result->removed_instructions);
        printf("改进比例: %.1f%%\n", constant_folding_result->improvement_ratio * 100);
    }
    
    // 应用死代码消除优化
    Stru_IrOptimizationResult_t* dead_code_result = 
        F_apply_tac_dead_code_elimination(tac_data, config);
    if (dead_code_result && dead_code_result->success) {
        printf("死代码消除优化成功\n");
        printf("移除指令: %zu\n", dead_code_result->removed_instructions);
        printf("改进比例: %.1f%%\n", dead_code_result->improvement_ratio * 100);
    }
    
    // 应用公共子表达式消除优化
    Stru_IrOptimizationResult_t* cse_result = 
        F_apply_tac_common_subexpression_elimination(tac_data, config);
    if (cse_result && cse_result->success) {
        printf("公共子表达式消除优化成功\n");
        printf("移除指令: %zu\n", cse_result->removed_instructions);
        printf("改进比例: %.1f%%\n", cse_result->improvement_ratio * 100);
    }
    
    // 清理资源
    if (constant_folding_result) {
        // 注意：实际使用中应该使用优化器的destroy_optimization_result函数
        cn_free(constant_folding_result->error_message);
        cn_free(constant_folding_result->warning_message);
        cn_free(constant_folding_result);
    }
    
    if (dead_code_result) {
        cn_free(dead_code_result->error_message);
        cn_free(dead_code_result->warning_message);
        cn_free(dead_code_result);
    }
    
    if (cse_result) {
        cn_free(cse_result->error_message);
        cn_free(cse_result->warning_message);
        cn_free(cse_result);
    }
    
    F_destroy_ir_optimization_config(config);
}
```

### 示例：优化TAC指令序列

```c
#include "CN_tac_optimizer.h"
#include <stdio.h>

void optimize_instruction_sequence_example(Stru_TacInstruction_t** instructions, size_t count) {
    // 创建优化配置
    Stru_IrOptimizationConfig_t* config = F_create_default_ir_optimization_config();
    if (!config) {
        printf("无法创建优化配置\n");
        return;
    }
    
    // 设置优化选项
    config->enable_constant_folding = true;
    config->enable_dead_code_elimination = true;
    config->enable_peephole_optimization = true;
    
    // 优化指令序列
    size_t optimized_count = 0;
    Stru_TacInstruction_t** optimized_instructions = 
        F_optimize_tac_instruction_sequence(instructions, count, config, &optimized_count);
    
    if (optimized_instructions) {
        printf("指令序列优化成功\n");
        printf("原始指令数量: %zu\n", count);
        printf("优化后指令数量: %zu\n", optimized_count);
        printf("减少比例: %.1f%%\n", (count - optimized_count) * 100.0 / count);
        
        // 使用优化后的指令...
        
        // 释放优化后的指令
        for (size_t i = 0; i < optimized_count; i++) {
            // 释放每个指令的内存
            // 注意：实际释放方式取决于指令结构
        }
        cn_free(optimized_instructions);
    } else {
        printf("指令序列优化失败\n");
    }
    
    F_destroy_ir_optimization_config(config);
}
```

### 示例：分析优化潜力

```c
#include "CN_tac_optimizer.h"
#include <stdio.h>

void analyze_optimization_potential_example(Stru_TacData_t* tac_data) {
    void* potential_areas[100];
    size_t area_count = 0;
    
    bool success = F_analyze_tac_optimization_potential(tac_data, potential_areas, &area_count);
    
    if (success) {
        printf("优化潜力分析成功\n");
        printf("发现 %zu 个潜在优化区域\n", area_count);
        
        for (size_t i = 0; i < area_count; i++) {
            // 处理每个潜在优化区域
            // 注意：实际处理方式取决于区域数据结构
            printf("区域 %zu: 可优化\n", i + 1);
        }
        
        // 释放潜在优化区域内存
        for (size_t i = 0; i < area_count; i++) {
            cn_free(potential_areas[i]);
        }
    } else {
        printf("优化潜力分析失败\n");
    }
}
```

## 错误处理

### 常见错误

1. **内存分配失败**: 当系统内存不足时，内存分配函数可能返回NULL
2. **无效参数**: 传入NULL指针或无效的TAC数据指针
3. **优化配置错误**: 优化配置参数无效或不一致
4. **优化失败**: 优化算法无法应用于给定的TAC数据

### 错误处理示例

```c
#include "CN_tac_optimizer.h"
#include <stdio.h>

void error_handling_example(Stru_TacData_t* tac_data) {
    Stru_IrOptimizationConfig_t* config = F_create_default_ir_optimization_config();
    if (!config) {
        printf("错误: 无法创建优化配置（内存不足）\n");
        return;
    }
    
    Stru_IrOptimizationResult_t* result = F_apply_tac_constant_folding(tac_data, config);
    
    if (!result) {
        printf("错误: 无法创建优化结果\n");
        F_destroy_ir_optimization_config(config);
        return;
    }
    
    if (!result->success) {
        printf("常量折叠优化失败\n");
        if (result->error_message) {
            printf("错误信息: %s\n", result->error_message);
        }
        if (result->warning_message) {
            printf("警告信息: %s\n", result->warning_message);
        }
    } else {
        printf("常量折叠优化成功\n");
        printf("改进比例: %.1f%%\n", result->improvement_ratio * 100);
    }
    
    // 清理资源
    if (result->error_message) cn_free(result->error_message);
    if (result->warning_message) cn_free(result->warning_message);
    cn_free(result);
    F_destroy_ir_optimization_config(config);
}
```

## 性能注意事项

1. **内存使用**: 优化器会分配内存存储中间结果和优化后的指令，使用后应及时释放
2. **执行时间**: 复杂的优化算法可能需要较长时间，建议在后台线程执行
3. **缓存效果**: 常量折叠和公共子表达式消除使用缓存提高性能，但会增加内存使用
4. **优化顺序**: 优化算法的应用顺序会影响最终效果，建议按照常量折叠→死代码消除→公共子表达式消除→强度削减的顺序应用

## 兼容性

### 支持的TAC指令类型

- 算术运算指令（加、减、乘、除、取模）
- 逻辑运算指令（与、或、非、异或）
- 比较指令（等于、不等于、大于、小于等）
- 赋值指令
- 跳转指令
- 函数调用指令

### 支持的优化类型

- `Eum_IR_OPT_CONSTANT_FOLDING`: 常量折叠
- `Eum_IR_OPT_DEAD_CODE_ELIMINATION`: 死代码消除
- `Eum_IR_OPT_COMMON_SUBEXPRESSION`: 公共子表达式消除
- `Eum_IR_OPT_STRENGTH_REDUCTION`: 强度削减
- `Eum_IR_OPT_PEEPHOLE`: 窥孔优化

## 版本历史

### v1.0.0 (2026-01-11)
- 初始版本
- 实现TAC优化器核心功能
- 支持常量折叠、死代码消除、公共子表达式消除、强度削减、窥孔优化
- 提供完整的API接口和工具函数
- 支持优化效果评估和验证

## 相关文档

- [优化模块README](../src/core/ir/optimizations/README.md)
- [IR优化器接口](../src/core/ir/CN_ir_optimizer.h)
- [优化效果评估器](./CN_optimization_evaluator.md)
- [CN_Language架构文档](../../../architecture/中间代码表示设计文档.md)

## 许可证

MIT License

Copyright (c) 2026 CN_Language项目组

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
