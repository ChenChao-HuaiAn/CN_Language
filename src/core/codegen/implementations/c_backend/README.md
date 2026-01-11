# C语言后端模块

## 概述

C语言后端是CN_Language编译器的一个关键组件，负责将CN_Language的抽象语法树（AST）转换为标准的C语言代码。生成的C代码可以直接用任何C编译器（如GCC、Clang、MSVC）编译执行。

## 功能特性

### 核心功能
- **AST到C代码转换**：将CN_Language的AST节点转换为等效的C语言代码
- **类型映射**：将CN_Language类型映射到C语言类型（如"整数"→"int"）
- **运算符转换**：将CN_Language运算符转换为C语言运算符
- **控制结构生成**：支持if、while、for等控制结构的代码生成
- **函数和变量声明**：生成函数原型和变量声明

### 支持的语言特性
- 基本数据类型：整数、浮点数、布尔值、字符、字符串
- 控制结构：if-else、while循环、for循环
- 函数声明和调用
- 变量声明和赋值
- 表达式：二元运算、一元运算、字面量、标识符
- 代码块和作用域

### 错误处理
- 语法错误检测和报告
- 类型不匹配警告
- 详细的错误信息收集
- 动态错误数组管理

## 架构设计

### 模块结构
```
c_backend/
├── CN_c_backend.h      # 公共接口头文件
├── CN_c_backend.c      # 核心实现文件
├── README.md           # 本文档
└── (未来可能添加测试文件)
```

### 内部状态管理
C后端使用`CBackendState`结构体管理代码生成过程中的状态：
- 输出缓冲区管理
- 缩进级别控制
- 错误和警告信息收集
- 代码生成进度跟踪

### 代码生成流程
1. **初始化**：创建后端状态，分配缓冲区
2. **程序生成**：生成文件头注释和包含指令
3. **声明生成**：遍历AST生成全局声明
4. **主函数生成**：生成main函数框架
5. **语句生成**：根据AST节点类型生成相应的C语句
6. **表达式生成**：生成表达式代码
7. **结果返回**：打包生成结果，包含代码和错误信息

## API接口

### 主要函数

#### `F_generate_c_code`
```c
Stru_CodeGenResult_t* F_generate_c_code(Stru_AstNode_t* ast);
```
将CN_Language的抽象语法树转换为C语言代码。

**参数**：
- `ast`：抽象语法树根节点

**返回值**：
- `Stru_CodeGenResult_t*`：代码生成结果，包含生成的代码、错误信息和状态

**注意**：调用者负责使用`F_destroy_codegen_result()`释放返回的结果。

#### `F_c_backend_supports_feature`
```c
bool F_c_backend_supports_feature(const char* feature);
```
检查C后端是否支持特定的代码生成功能。

**支持的功能**：
- "c99"：支持C99标准
- "c11"：支持C11标准
- "c17"：支持C17标准
- "posix"：支持POSIX扩展
- "threads"：线程安全代码生成（暂不支持）
- "exceptions"：异常处理代码生成（暂不支持）

#### `F_configure_c_backend`
```c
bool F_configure_c_backend(const char* option, const char* value);
```
配置C后端的代码生成选项。

**支持的选项**：
- "standard"：C标准（"c99"、"c11"、"c17"）
- "optimization_level"：优化级别（0-3）
- "debug_info"：是否生成调试信息（true/false）
- "warnings"：是否启用警告（true/false）

#### `F_create_c_backend_interface`
```c
Stru_CodeGeneratorInterface_t* F_create_c_backend_interface(void);
```
创建C后端特定的代码生成器接口实例。

#### 版本信息函数
```c
void F_get_c_backend_version(int* major, int* minor, int* patch);
const char* F_get_c_backend_version_string(void);
```
获取C后端的版本信息。

## 使用示例

### 基本使用
```c
#include "CN_c_backend.h"

// 创建AST（通常由解析器生成）
Stru_AstNode_t* ast = create_sample_ast();

// 生成C代码
Stru_CodeGenResult_t* result = F_generate_c_code(ast);

if (result->success) {
    printf("生成的C代码：\n%s\n", result->code);
    
    // 可以将代码写入文件
    FILE* f = fopen("output.c", "w");
    if (f) {
        fwrite(result->code, 1, result->code_length, f);
        fclose(f);
    }
} else {
    printf("代码生成失败，错误：\n");
    for (size_t i = 0; i < F_dynamic_array_length(result->errors); i++) {
        char** error = (char**)F_dynamic_array_get(result->errors, i);
        if (error && *error) {
            printf("  - %s\n", *error);
        }
    }
}

// 释放资源
F_destroy_codegen_result(result);
F_destroy_ast_node(ast);
```

### 配置后端
```c
// 配置使用C11标准
F_configure_c_backend("standard", "c11");

// 启用优化
F_configure_c_backend("optimization_level", "2");

// 启用调试信息
F_configure_c_backend("debug_info", "true");
```

## 实现细节

### 类型映射
CN_Language类型到C语言类型的映射：
- `整数` / `int` → `int`
- `浮点数` / `float` → `float`
- `双精度` / `double` → `double`
- `布尔` / `bool` → `bool`
- `字符` / `char` → `char`
- `字符串` / `string` → `char*`
- `空` / `void` → `void`

### 运算符映射
CN_Language运算符到C语言运算符的映射：
- 算术运算符：`+`, `-`, `*`, `/`, `%`
- 比较运算符：`==`, `!=`, `<`, `<=`, `>`, `>=`
- 逻辑运算符：`&&`, `||`, `!`
- 赋值运算符：`=`, `+=`, `-=`, `*=`, `/=`

### 代码生成策略
1. **增量生成**：使用动态缓冲区逐步构建代码
2. **错误恢复**：遇到错误时继续生成，收集所有错误信息
3. **格式保持**：生成格式良好的C代码，包含适当的缩进和注释
4. **兼容性**：生成符合ANSI C标准的代码，确保最大兼容性

## 依赖关系

### 内部依赖
- `CN_codegen_interface.h`：代码生成器接口定义
- `CN_ast.h`：抽象语法树定义
- `CN_dynamic_array.h`：动态数组容器

### 外部依赖
- 标准C库：`stdio.h`, `stdlib.h`, `string.h`, `ctype.h`

## 测试

### 单元测试
计划中的测试包括：
1. 基本类型转换测试
2. 表达式生成测试
3. 控制结构生成测试
4. 错误处理测试
5. 边界条件测试

### 集成测试
与解析器和优化器集成，测试完整的编译流程。

## 性能考虑

### 内存使用
- 使用动态缓冲区减少内存碎片
- 错误信息使用动态数组，按需分配
- 及时释放不再使用的资源

### 生成效率
- 缓冲区预分配减少重新分配次数
- 使用内存复制而非字符逐个追加
- 避免不必要的字符串操作

## 扩展性

### 添加新特性
1. 在`get_c_type_from_cn_type()`中添加新的类型映射
2. 在`get_c_operator()`中添加新的运算符映射
3. 添加新的AST节点处理函数
4. 更新相应的生成函数

### 插件支持
未来计划支持插件架构，允许用户自定义：
- 类型映射规则
- 代码生成策略
- 优化规则
- 输出格式

## 限制和未来改进

### 当前限制
1. 不支持完整的CN_Language语法
2. 错误信息不够详细
3. 生成的代码优化有限
4. 缺少高级特性（如异常处理、泛型）

### 计划改进
1. 支持更多CN_Language特性
2. 改进错误信息和定位
3. 添加代码优化选项
4. 支持多平台代码生成
5. 添加调试信息生成

## 版本历史

### v1.0.0 (2026-01-10)
- 初始版本
- 基本AST到C代码转换
- 支持基本数据类型和控制结构
- 错误处理和报告机制

## 贡献指南

### 代码规范
1. 遵循项目编码标准
2. 添加适当的注释和文档
3. 编写单元测试
4. 保持向后兼容性

### 提交流程
1. 创建功能分支
2. 实现功能并添加测试
3. 运行现有测试确保无回归
4. 提交Pull Request
5. 代码审查和合并

## 许可证

本项目采用MIT许可证。详见项目根目录的LICENSE文件。

## 联系方式

如有问题或建议，请联系：
- 项目维护者：CN_Language开发团队
- 问题跟踪：GitHub Issues
- 文档：项目docs目录
