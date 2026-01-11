# CN_Peephole_Optimization API 文档

## 概述

`CN_peephole_optimization.h` 定义了CN_Language窥孔优化算法的接口。该算法对生成的代码进行局部优化，通过分析小段代码（"窥孔"）来识别和替换低效的代码模式。

## 文件信息

- **文件名**: `CN_peephole_optimization.h`
- **位置**: `src/core/codegen/optimizers/basic_optimizer/`
- **版本**: 1.0.0
- **作者**: CN_Language架构委员会
- **创建日期**: 2026-01-11

## 包含文件

```c
#include "../../../ast/CN_ast.h"
#include "../CN_basic_optimizer.h"
```

## 枚举类型

### Eum_PeepholePatternType

窥孔模式类型枚举，定义支持的优化模式类别。

```c
typedef enum Eum_PeepholePatternType {
    Eum_PP_REDUNDANT_LOAD_STORE = 0,   ///< 冗余加载/存储
    Eum_PP_UNNECESSARY_JUMPS,          ///< 不必要的跳转
    Eum_PP_DEAD_CODE,                  ///< 死代码
    Eum_PP_STRENGTH_REDUCTION,         ///< 强度削减
    Eum_PP_ALGEBRAIC_IDENTITIES,       ///< 代数恒等式
    Eum_PP_CONSTANT_PROPAGATION,       ///< 常量传播
    Eum_PP_COMMON_SUBEXPRESSION,       ///< 公共子表达式
    Eum_PP_LOOP_OPTIMIZATIONS,         ///< 循环优化
    Eum_PP_BRANCH_OPTIMIZATIONS,       ///< 分支优化
    Eum_PP_MEMORY_ACCESS,              ///< 内存访问优化
    Eum_PP_REGISTER_USAGE,             ///< 寄存器使用优化
    Eum_PP_INSTRUCTION_SELECTION,      ///< 指令选择优化
    Eum_PP_CUSTOM_PATTERN              ///< 自定义模式
} Eum_PeepholePatternType;
```

### Eum_PeepholeWindowSize

窥孔窗口大小枚举，定义分析窗口的大小。

```c
typedef enum Eum_PeepholeWindowSize {
    Eum_PW_SIZE_1 = 1,     ///< 单指令窗口
    Eum_PW_SIZE_2 = 2,     ///< 双指令窗口
    Eum_PW_SIZE_3 = 3,     ///< 三指令窗口
    Eum_PW_SIZE_4 = 4,     ///< 四指令窗口
    Eum_PW_SIZE_5 = 5,     ///< 五指令窗口
    Eum_PW_SIZE_CUSTOM     ///< 自定义窗口大小
} Eum_PeepholeWindowSize;
```

## 结构体类型

### Stru_PeepholePattern_t

窥孔模式结构体，定义具体的优化模式。

```c
typedef struct Stru_PeepholePattern_t {
    Eum_PeepholePatternType type;      ///< 模式类型
    const char* name;                  ///< 模式名称
    const char* description;           ///< 模式描述
    Eum_PeepholeWindowSize window_size; ///< 所需窗口大小
    
    // 模式匹配
    const char* pattern_code;          ///< 模式代码（字符串表示）
    bool (*match_function)(Stru_AstNode_t** window, size_t window_size); ///< 匹配函数
    
    // 模式应用
    const char* replacement_code;      ///< 替换代码（字符串表示）
    bool (*apply_function)(Stru_AstNode_t** window, size_t window_size); ///< 应用函数
    
    // 优化信息
    bool is_safe;                      ///< 是否安全（不改变语义）
    int priority;                      ///< 优先级（高优先级先应用）
    size_t min_occurrences;            ///< 最小出现次数阈值
    double improvement_estimate;       ///< 改进估计（0.0-1.0）
} Stru_PeepholePattern_t;
```

### Stru_PeepholeWindow_t

窥孔窗口结构体，表示正在分析的代码窗口。

```c
typedef struct Stru_PeepholeWindow_t {
    Stru_AstNode_t** instructions;     ///< 指令数组
    size_t window_size;                ///< 窗口大小
    size_t current_position;           ///< 当前位置
    size_t total_instructions;         ///< 总指令数
    
    // 分析上下文
    Stru_AstNode_t* parent_block;      ///< 父代码块
    size_t start_index;                ///< 起始索引
    size_t end_index;                  ///< 结束索引
    
    // 匹配信息
    Stru_PeepholePattern_t* matched_pattern; ///< 匹配的模式
    size_t match_start;                ///< 匹配起始位置
    size_t match_end;                  ///< 匹配结束位置
} Stru_PeepholeWindow_t;
```

### Stru_PeepholeOptimizationContext_t

窥孔优化上下文结构体，包含PO优化的状态和配置。

```c
typedef struct Stru_PeepholeOptimizationContext_t {
    Stru_AstNode_t* ast;               ///< AST根节点
    Stru_OptimizationContext_t* opt_context; ///< 优化上下文
    
    // 模式管理
    Stru_PeepholePattern_t** patterns; ///< 可用模式数组
    size_t pattern_count;              ///< 模式数量
    size_t pattern_capacity;           ///< 模式数组容量
    
    // 窗口配置
    Eum_PeepholeWindowSize window_size; ///< 窗口大小
    size_t max_iterations;             ///< 最大迭代次数
    bool sliding_window;               ///< 是否使用滑动窗口
    bool overlapping_windows;          ///< 是否允许重叠窗口
    
    // 统计信息
    size_t windows_analyzed;           ///< 分析的窗口数量
    size_t patterns_matched;           ///< 匹配的模式数量
    size_t optimizations_applied;      ///< 应用的优化数量
    size_t instructions_removed;       ///< 移除的指令数量
    size_t instructions_added;         ///< 添加的指令数量
    size_t cycles_saved;               ///< 节省的时钟周期（估计）
    
    // 配置选项
    bool enable_aggressive_optimizations; ///< 启用激进优化
    bool preserve_semantics;           ///< 保留语义
    bool consider_data_dependencies;   ///< 考虑数据依赖
    bool track_side_effects;           ///< 跟踪副作用
    
    // 错误处理
    size_t error_count;                ///< 错误数量
    char** errors;                     ///< 错误信息数组
    size_t error_capacity;             ///< 错误数组容量
} Stru_PeepholeOptimizationContext_t;
```

## 函数接口

### 1. `apply_peephole_optimization`

**功能：** 应用窥孔优化

**原型：**
```c
bool apply_peephole_optimization(Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);
```

**参数：**
- `ast`: 抽象语法树根节点
- `context`: 优化上下文

**返回值：**
- 优化成功应用返回`true`，失败返回`false`

**算法描述：**
窥孔优化是一种局部优化技术，通过分析小段连续的代码（"窥孔"）来识别和替换低效的代码模式。算法步骤：

1. **代码遍历**：遍历代码，创建滑动窗口
2. **模式匹配**：对每个窗口尝试所有注册的模式
3. **优化应用**：应用匹配的模式进行优化
4. **迭代优化**：重复直到没有更多优化或达到最大迭代次数

**支持的优化模式：**

#### 冗余加载/存储消除
```c
// 模式：加载后立即存储到同一位置
load r1, [addr]
store [addr], r1
// 优化为：无（可以移除这两条指令）
```

#### 不必要的跳转消除
```c
// 模式：跳转到下一条指令
jmp label
label:  // 下一条指令
// 优化为：移除跳转指令
```

#### 死代码消除
```c
// 模式：计算结果从未使用
计算表达式
// 优化为：移除计算（如果没有副作用）
```

#### 强度削减
```c
// 模式：乘以2的幂次方
mul r1, r2, 8
// 优化为：左移3位
shl r1, r2, 3
```

#### 代数恒等式
```c
// 模式：x + 0 或 x - 0
add r1, r2, 0
// 优化为：mov r1, r2
```

#### 常量传播
```c
// 模式：使用已知常量值
mov r1, 5
add r2, r1, 3  // r1已知为5
// 优化为：add r2, 5, 3 → mov r2, 8
```

### 2. `create_peephole_context`

**功能：** 创建窥孔优化上下文

**原型：**
```c
Stru_PeepholeOptimizationContext_t* create_peephole_context(
    Stru_AstNode_t* ast, 
    Stru_OptimizationContext_t* opt_context);
```

**参数：**
- `ast`: AST根节点
- `opt_context`: 优化上下文

**返回值：**
- 成功：返回新创建的PO上下文
- 失败：返回NULL

**说明：**
- 调用者负责在不再使用时调用`destroy_peephole_context()`释放资源
- 上下文预加载标准优化模式

### 3. `destroy_peephole_context`

**功能：** 销毁窥孔优化上下文

**原型：**
```c
void destroy_peephole_context(Stru_PeepholeOptimizationContext_t* context);
```

**参数：**
- `context`: PO上下文

**说明：**
- 释放上下文及其所有相关资源
- 如果context为NULL，函数不执行任何操作

### 4. `register_peephole_pattern`

**功能：** 注册窥孔优化模式

**原型：**
```c
bool register_peephole_pattern(
    Stru_PeepholeOptimizationContext_t* context,
    const Stru_PeepholePattern_t* pattern);
```

**参数：**
- `context`: PO上下文
- `pattern`: 要注册的模式

**返回值：**
- 注册成功返回`true`，失败返回`false`

**说明：**
- 允许动态添加自定义优化模式
- 模式按优先级排序，高优先级模式先尝试

### 5. `create_peephole_window`

**功能：** 创建窥孔窗口

**原型：**
```c
Stru_PeepholeWindow_t* create_peephole_window(
    Stru_AstNode_t* code_block,
    size_t start_index,
    Eum_PeepholeWindowSize window_size);
```

**参数：**
- `code_block`: 代码块AST节点
- `start_index`: 起始索引
- `window_size`: 窗口大小

**返回值：**
- 成功：返回新创建的窥孔窗口
- 失败：返回NULL

### 6. `destroy_peephole_window`

**功能：** 销毁窥孔窗口

**原型：**
```c
void destroy_peephole_window(Stru_PeepholeWindow_t* window);
```

**参数：**
- `window`: 窥孔窗口

**说明：**
- 释放窗口及其相关资源

### 7. `slide_window`

**功能：** 滑动窗口到下一个位置

**原型：**
```c
bool slide_window(Stru_PeepholeWindow_t* window);
```

**参数：**
- `window`: 窥孔窗口

**返回值：**
- 如果窗口可以继续滑动返回`true`，否则返回`false`

### 8. `match_patterns_in_window`

**功能：** 在窗口中匹配模式

**原型：**
```c
Stru_PeepholePattern_t* match_patterns_in_window(
    Stru_PeepholeOptimizationContext_t* context,
    Stru_PeepholeWindow_t* window);
```

**参数：**
- `context`: PO上下文
- `window`: 窥孔窗口

**返回值：**
- 匹配的模式，如果没有匹配返回NULL

### 9. `apply_pattern_to_window`

**功能：** 对窗口应用优化模式

**原型：**
```c
bool apply_pattern_to_window(
    Stru_PeepholeOptimizationContext_t* context,
    Stru_PeepholeWindow_t* window,
    Stru_PeepholePattern_t* pattern);
```

**参数：**
- `context`: PO上下文
- `window`: 窥孔窗口
- `pattern`: 要应用的优化模式

**返回值：**
- 优化成功应用返回`true`，失败返回`false`

### 10. `get_peephole_statistics`

**功能：** 获取窥孔优化统计信息

**原型：**
```c
void get_peephole_statistics(
    const Stru_PeepholeOptimizationContext_t* context,
    size_t* windows_analyzed,
    size_t* patterns_matched,
    size_t* optimizations_applied,
    size_t* instructions_removed,
    size_t* instructions_added,
    size_t* cycles_saved);
```

**参数：**
- `context`: PO上下文
- `windows_analyzed`: 输出参数，分析的窗口数量
- `patterns_matched`: 输出参数，匹配的模式数量
- `optimizations_applied`: 输出参数，应用的优化数量
- `instructions_removed`: 输出参数，移除的指令数量
- `instructions_added`: 输出参数，添加的指令数量
- `cycles_saved`: 输出参数，节省的时钟周期

### 11. `configure_peephole_optimization`

**功能：** 配置窥孔优化参数

**原型：**
```c
bool configure_peephole_optimization(
    Stru_PeepholeOptimizationContext_t* context,
    Eum_PeepholeWindowSize window_size,
    size_t max_iterations,
    bool enable_aggressive_optimizations);
```

**参数：**
- `context`: PO上下文
- `window_size`: 窗口大小
- `max_iterations`: 最大迭代次数
- `enable_aggressive_optimizations`: 是否启用激进优化

**返回值：**
- 配置成功返回`true`，失败返回`false`

### 12. `test_peephole_optimization`

**功能：** 测试窥孔优化算法

**原型：**
```c
void test_peephole_optimization(void);
```

**说明：**
- 运行PO算法的测试用例
- 输出测试结果和性能统计

## 使用示例

### 基本使用

```c
#include "src/core/codegen/optimizers/basic_optimizer/CN_peephole_optimization.h"

int main(void)
{
    // 创建AST和优化上下文
    Stru_AstNode_t* ast = ...;
    Stru_OptimizationContext_t* opt_context = ...;
    
    // 应用窥孔优化
    if (apply_peephole_optimization(ast, opt_context)) {
        printf("窥孔优化成功应用\n");
        
        // 获取统计信息
        Stru_PeepholeOptimizationContext_t* po_context = 
            create_peephole_context(ast, opt_context);
        if (po_context) {
            size_t windows, matched, applied, removed, added, cycles;
            get_peephole_statistics(po_context, 
                &windows, &matched, &applied, &removed, &added, &cycles);
            
            printf("分析了 %zu 个窗口，匹配了 %zu 个模式\n", windows, matched);
            printf("应用了 %zu 个优化，移除了 %zu 条指令\n", applied, removed);
            printf("添加了 %zu 条指令，节省了约 %zu 个时钟周期\n", added, cycles);
            
            destroy_peephole_context(po_context);
        }
    }
    
    return EXIT_SUCCESS;
}
```

### 高级配置

```c
#include "src/core/codegen/optimizers/basic_optimizer/CN_peephole_optimization.h"

int main(void)
{
    // 创建AST和优化上下文
    Stru_AstNode_t* ast = ...;
    Stru_OptimizationContext_t* opt_context = ...;
    
    // 创建PO上下文
    Stru_PeepholeOptimizationContext_t* po_context = 
        create_peephole_context(ast, opt_context);
    if (!po_context) {
        fprintf(stderr, "创建PO上下文失败\n");
        return EXIT_FAILURE;
    }
    
    // 配置PO优化
    if (!configure_peephole_optimization(po_context,
                                         Eum_PW_SIZE_3,  // 3指令窗口
                                         10,             // 最大10次迭代
                                         true)) {        // 启用激进优化
        fprintf(stderr, "配置PO优化失败\n");
        destroy_peephole_context(po_context);
        return EXIT_FAILURE;
    }
    
    // 遍历代码块进行优化
    size_t iteration = 0;
    bool changed;
    
    do {
        changed = false;
        iteration++;
        
        // 获取代码块
        Stru_AstNode_t* code_block = get_code_block(ast);
        if (!code_block) {
            break;
        }
        
        // 创建滑动窗口
        size_t instruction_count = get_instruction_count(code_block);
        for (size_t i = 0; i <= instruction_count - po_context->window_size; i++) {
            // 创建窗口
            Stru_PeepholeWindow_t* window = create_peephole_window(
                code_block, i, po_context->window_size);
            if (!window) {
                continue;
            }
            
            // 匹配模式
            Stru_P
