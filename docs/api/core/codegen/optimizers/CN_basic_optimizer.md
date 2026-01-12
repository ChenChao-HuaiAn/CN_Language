# CN_Basic_Optimizer API 文档

## 概述

`CN_basic_optimizer.h` 定义了CN_Language基础优化器的抽象接口。基础优化器提供五种基础优化算法的实现，包括常量折叠、死代码消除、公共子表达式消除、强度削减和窥孔优化。该模块实现了优化器插件接口，可以集成到代码生成器中。

## 文件信息

- **文件名**: `CN_basic_optimizer.h`
- **位置**: `src/core/codegen/optimizers/basic_optimizer/`
- **版本**: 1.0.0
- **作者**: CN_Language架构委员会
- **创建日期**: 2026-01-09

## 包含文件

```c
#include "../../CN_optimizer_interface.h"
```

## 函数声明

### `F_create_basic_optimizer_interface`

创建基础优化器插件接口实例。

```c
Stru_OptimizerPluginInterface_t* F_create_basic_optimizer_interface(void);
```

**返回值**:
- `Stru_OptimizerPluginInterface_t*`: 新创建的基础优化器插件接口实例

**注意事项**:
- 调用者负责在不再使用时调用接口的`destroy`函数
- 返回的接口已分配内存并部分初始化
- 必须调用接口的`initialize`函数进行完全初始化

### `F_get_basic_optimizer_version`

获取基础优化器版本信息。

```c
void F_get_basic_optimizer_version(int* major, int* minor, int* patch);
```

**参数**:
- `major`: 输出参数，主版本号
- `minor`: 输出参数，次版本号
- `patch`: 输出参数，修订号

**版本规范**:
- 主版本号：不兼容的API修改
- 次版本号：向下兼容的功能性新增
- 修订号：向下兼容的问题修正

### `F_get_basic_optimizer_version_string`

获取基础优化器版本字符串。

```c
const char* F_get_basic_optimizer_version_string(void);
```

**返回值**:
- `const char*`: 版本字符串，格式为"主版本号.次版本号.修订号"

**注意事项**:
- 返回的字符串是静态常量，调用者不应修改或释放
- 字符串内容与`F_get_basic_optimizer_version`函数返回的版本号一致

### `F_basic_optimizer_supports_optimization`

检查基础优化器是否支持特定的优化类型。

```c
bool F_basic_optimizer_supports_optimization(Eum_OptimizationType optimization_type);
```

**参数**:
- `optimization_type`: 优化类型枚举值

**返回值**:
- `true`: 支持该优化类型
- `false`: 不支持该优化类型

**支持的优化类型**:
- `Eum_OPT_CONSTANT_FOLDING`: 常量折叠优化
- `Eum_OPT_DEAD_CODE_ELIMINATION`: 死代码消除优化
- `Eum_OPT_COMMON_SUBEXPRESSION_ELIMINATION`: 公共子表达式消除优化
- `Eum_OPT_STRENGTH_REDUCTION`: 强度削减优化
- `Eum_OPT_PEEPHOLE_OPTIMIZATION`: 窥孔优化

### `F_get_basic_optimizer_supported_optimizations`

获取基础优化器支持的所有优化类型。

```c
const Eum_OptimizationType* F_get_basic_optimizer_supported_optimizations(size_t* count);
```

**参数**:
- `count`: 输出参数，优化类型数量

**返回值**:
- `const Eum_OptimizationType*`: 优化类型数组，调用者不应修改

**注意事项**:
- 返回的数组是静态常量，调用者不应修改或释放
- 数组元素数量通过`count`参数返回
- 数组内容与`F_basic_optimizer_supports_optimization`函数的检查一致

## 优化器插件接口实现

基础优化器完整实现了`Stru_OptimizerPluginInterface_t`接口，提供以下功能：

### 1. 插件信息 (`get_info`)

获取优化器插件的基本信息，包括：
- 插件名称: "CN_Basic_Optimizer"
- 插件描述: "CN_Language基础优化器，提供五种基础优化算法"
- 作者: "CN_Language开发团队"
- 版本: "1.0.0"
- 支持的优化类型: 常量折叠、死代码消除、公共子表达式消除、强度削减、窥孔优化

### 2. 初始化 (`initialize`)

初始化优化器插件，准备优化环境。

**返回值**:
- `true`: 初始化成功
- `false`: 初始化失败

### 3. 代码分析 (`analyze`)

分析代码，收集优化信息。

**参数**:
- `context`: 优化上下文，包含要分析的代码

**返回值**:
- `Stru_AnalysisResult_t*`: 分析结果，包含优化建议

### 4. 优化可行性检查 (`can_optimize`)

检查是否可以对特定代码应用优化。

**参数**:
- `context`: 优化上下文
- `optimization_type`: 优化类型

**返回值**:
- `true`: 可以应用优化
- `false`: 不能应用优化

### 5. 应用优化 (`optimize`)

对代码应用特定的优化。

**参数**:
- `context`: 优化上下文
- `optimization_type`: 优化类型
- `options`: 优化选项

**返回值**:
- `Stru_OptimizationResult_t*`: 优化结果，包含优化后的代码和统计信息

### 6. 批量优化 (`optimize_batch`)

对代码应用多个优化。

**参数**:
- `context`: 优化上下文
- `optimization_types`: 优化类型数组
- `type_count`: 优化类型数量
- `options`: 优化选项

**返回值**:
- `Stru_OptimizationResult_t*`: 优化结果

### 7. 级别优化 (`optimize_level`)

根据优化级别应用相应的优化。

**参数**:
- `context`: 优化上下文
- `level`: 优化级别 (0-3)

**返回值**:
- `Stru_OptimizationResult_t*`: 优化结果

**优化级别对应关系**:
- `0`: 无优化
- `1`: 应用常量折叠和窥孔优化
- `2`: 应用所有基础优化（标准优化）
- `3`: 应用所有基础优化，并进行多次迭代（激进优化）

### 8. AST转换 (`transform_ast`)

对抽象语法树应用优化转换。

**参数**:
- `ast`: 抽象语法树根节点
- `optimization_type`: 优化类型

**返回值**:
- `Stru_AstNode_t*`: 转换后的抽象语法树

### 9. 中间表示转换 (`transform_ir`)

对中间代码表示应用优化转换。

**参数**:
- `ir`: 中间代码表示
- `optimization_type`: 优化类型

**返回值**:
- `void*`: 转换后的中间代码表示

### 10. 优化验证 (`validate`)

验证优化的正确性和安全性。

**参数**:
- `original`: 原始代码
- `optimized`: 优化后的代码
- `optimization_type`: 优化类型

**返回值**:
- `true`: 优化验证通过
- `false`: 优化验证失败

### 11. 安全性检查 (`is_safe`)

检查优化是否安全（不会改变程序行为）。

**参数**:
- `context`: 优化上下文
- `optimization_type`: 优化类型

**返回值**:
- `true`: 优化是安全的
- `false`: 优化可能不安全

### 12. 统计信息收集 (`collect_statistics`)

收集优化统计信息。

**参数**:
- `context`: 优化上下文

**返回值**:
- `Stru_OptimizationStatistics_t*`: 统计信息

### 13. 报告生成 (`generate_report`)

生成优化报告。

**参数**:
- `context`: 优化上下文
- `result`: 优化结果

**返回值**:
- `Stru_OptimizationReport_t*`: 优化报告

### 14. 插件配置 (`configure`)

配置优化器插件。

**参数**:
- `option`: 配置选项名称
- `value`: 配置选项值

**返回值**:
- `true`: 配置成功
- `false`: 配置失败

**支持的配置选项**:
- `"max_iterations"`: 最大优化迭代次数
- `"aggressive"`: 是否启用激进优化模式
- `"preserve_debug_info"`: 是否保留调试信息

### 15. 配置选项获取 (`get_config_options`)

获取可用的配置选项。

**返回值**:
- `Stru_ConfigOptionList_t*`: 配置选项列表

### 16. 重置 (`reset`)

重置优化器插件到初始状态。

### 17. 销毁 (`destroy`)

销毁优化器插件，释放所有资源。

## 优化算法详细说明

### 常量折叠 (Constant Folding)

**功能**: 在编译时计算常量表达式的结果，用计算结果替换表达式。

**示例**:
```c
// 优化前
x = 3 + 5 * 2;

// 优化后
x = 13;
```

**适用场景**:
- 算术常量表达式
- 逻辑常量表达式
- 位运算常量表达式

**限制**:
- 只处理编译时可确定的常量
- 避免浮点数精度问题

### 死代码消除 (Dead Code Elimination)

**功能**: 删除永远不会执行的代码。

**示例**:
```c
// 优化前
if (false) {
    x = 10;  // 死代码
}
y = 20;

// 优化后
y = 20;
```

**适用场景**:
- 条件永远为假的if语句
- 无法到达的代码块
- 无用的变量赋值

**限制**:
- 需要准确的流程分析
- 避免删除有副作用的代码

### 公共子表达式消除 (Common Subexpression Elimination)

**功能**: 识别并消除重复计算的相同表达式。

**示例**:
```c
// 优化前
x = a * b + c;
y = a * b + d;

// 优化后
temp = a * b;
x = temp + c;
y = temp + d;
```

**适用场景**:
- 循环内的重复计算
- 复杂表达式的重复部分
- 函数调用的重复参数计算

**限制**:
- 需要表达式等价性判断
- 考虑变量值的变化

### 强度削减 (Strength Reduction)

**功能**: 用更高效的操作替换低效的操作。

**示例**:
```c
// 优化前
x = y * 16;

// 优化后
x = y << 4;  // 用移位代替乘法
```

**适用场景**:
- 乘除2的幂次方用移位代替
- 乘法用加法和移位组合代替
- 复杂运算用查表代替

**限制**:
- 需要确保语义等价
- 考虑目标平台的指令特性

### 窥孔优化 (Peephole Optimization)

**功能**: 在小范围代码窗口内进行局部优化。

**示例**:
```c
// 优化前
x = y + 0;
z = x * 1;

// 优化后
x = y;
z = x;
```

**适用场景**:
- 冗余指令消除
- 常量传播
- 代数简化
- 控制流优化

**限制**:
- 只考虑局部代码模式
- 需要多次迭代达到全局效果

## 使用示例

### 基本使用

```c
#include "src/core/codegen/optimizers/basic_optimizer/CN_basic_optimizer.h"

int main() {
    // 创建基础优化器接口
    Stru_OptimizerPluginInterface_t* optimizer = F_create_basic_optimizer_interface();
    
    if (!optimizer) {
        printf("创建优化器失败\n");
        return 1;
    }
    
    // 初始化优化器
    if (!optimizer->initialize()) {
        printf("初始化优化器失败\n");
        optimizer->destroy();
        return 1;
    }
    
    // 获取优化器信息
    Stru_OptimizerPluginInfo_t info;
    if (optimizer->get_info(&info)) {
        printf("优化器: %s\n", info.name);
        printf("版本: %s\n", info.version);
        printf("描述: %s\n", info.description);
    }
    
    // 检查优化支持
    if (F_basic_optimizer_supports_optimization(Eum_OPT_CONSTANT_FOLDING)) {
        printf("支持常量折叠优化\n");
    }
    
    // 获取支持的优化类型
    size_t count = 0;
    const Eum_OptimizationType* supported = F_get_basic_optimizer_supported_optimizations(&count);
    printf("支持 %zu 种优化类型\n", count);
    
    // 配置优化器
    optimizer->configure("max_iterations", "3");
    optimizer->configure("aggressive", "false");
    
    // 使用优化器进行优化（假设有优化上下文）
    // Stru_OptimizationResult_t* result = optimizer->optimize(context, Eum_OPT_CONSTANT_FOLDING, NULL);
    
    // 清理资源
    optimizer->destroy();
    
    return 0;
}
```

### 集成到代码生成器

```c
#include "src/core/codegen/CN_codegen_interface.h"
#include "src/core/codegen/optimizers/basic_optimizer/CN_basic_optimizer.h"

int main() {
    // 创建代码生成器
    Stru_CodeGeneratorInterface_t* codegen = F_create_codegen_interface();
    
    // 创建基础优化器
    Stru_OptimizerPluginInterface_t* optimizer = F_create_basic_optimizer_interface();
    
    // 注册优化器插件到代码生成器
    if (optimizer && codegen->register_optimizer_plugin) {
        codegen->register_optimizer_plugin(codegen, "basic_optimizer", optimizer);
    }
    
    // 使用代码生成器（会自动使用注册的优化器）
    // ...
    
    // 清理资源
    if (optimizer) {
        optimizer->destroy();
    }
    if (codegen) {
        codegen->destroy(codegen);
    }
    
    return 0;
}
```

## 性能考虑

### 优化开销

基础优化器的优化开销主要包括：
1. **分析阶段**: 收集代码信息，复杂度O(n)
2. **转换阶段**: 应用优化规则，复杂度O(n)
3. **验证阶段**: 检查优化正确性，复杂度O(n)

### 内存使用

优化器内存使用主要包括：
1. **分析数据结构**: 存储代码分析结果
2. **转换缓冲区**: 存储中间转换结果
3. **统计信息**: 收集优化统计信息

### 优化效果

不同优化算法的效果：
1. **常量折叠**: 减少运行时计算，效果显著
2. **死代码消除**: 减少代码大小，提高执行速度
3. **公共子表达式消除**: 减少重复计算，效果取决于代码重复度
4. **强度削减**: 提高指令效率，效果取决于目标平台
5. **窥孔优化**: 局部改进，累积效果显著

## 错误处理

### 常见错误

1. **内存分配失败**: 返回NULL或false
2. **无效参数**: 检查参数有效性，返回false
3. **初始化失败**: 记录错误信息，返回false
4. **优化失败**: 返回包含错误信息的优化结果

### 错误恢复

优化器设计为错误安全的：
1. 内存分配失败时回滚已分配的资源
2. 优化失败时返回原始代码
3. 提供详细的错误信息帮助调试

## 测试建议

### 单元测试

测试每个优化算法的正确性：
1. 常量折叠测试各种常量表达式
2. 死代码消除测试各种控制流
3. 公共子表达式消除测试重复计算
4. 强度削减测试各种强度削减模式
5. 窥孔优化测试各种窥孔模式

### 集成测试

测试优化器与代码生成器的集成：
1. 优化器注册和初始化
2. 优化器配置管理
3. 优化器在代码生成流程中的应用
4. 优化器错误处理

### 性能测试

测试优化器的性能：
1. 优化开销测量
2. 内存使用测量
3. 优化效果测量
4. 大规模代码优化测试

## 扩展指南

### 添加新的优化算法

1. 在`Eum_OptimizationType`枚举中添加新的优化类型
2. 实现优化算法的核心逻辑
3. 在基础优化器中集成新算法
4. 更新`F_basic_optimizer_supports_optimization`函数
5. 编写测试用例验证新算法

### 自定义优化策略

1. 通过配置选项调整优化行为
2. 实现自定义的优化级别映射
3. 提供扩展点支持用户自定义优化规则
4. 支持优化器插件组合使用

## 兼容性说明

### 版本兼容性

- 主版本号变更表示不兼容的API修改
- 次版本号变更表示向下兼容的功能新增
- 修订号变更表示向下兼容的问题修正

### 接口兼容性

基础优化器实现了标准的优化器插件接口，可以与任何符合该接口规范的代码生成器集成。

### 平台兼容性

优化算法设计为平台无关，但某些优化（如强度削减）的效果可能因目标平台而异。
