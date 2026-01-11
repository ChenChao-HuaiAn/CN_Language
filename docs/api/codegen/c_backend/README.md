# CN_Language C语言后端 API 文档

## 概述

C语言后端是CN_Language编译器的一个目标代码生成器实现，负责将CN_Language的抽象语法树（AST）转换为标准的C语言代码。生成的C代码可以直接使用任何符合C99或更高标准的C编译器进行编译和执行。

## 模块位置

```
src/core/codegen/implementations/c_backend/
├── CN_c_backend.h      # C后端公共接口头文件
├── CN_c_backend.c      # C后端实现文件
└── README.md           # C后端模块文档
```

## 核心API

### 1. `F_generate_c_code`

**功能：** 从AST生成C代码

**原型：**
```c
Stru_CodeGenResult_t* F_generate_c_code(Stru_AstNode_t* ast);
```

**参数：**
- `ast`: 抽象语法树根节点

**返回值：**
- 成功：返回包含生成代码的`Stru_CodeGenResult_t`结构体指针
- 失败：返回NULL

**说明：**
- 这是C后端的主要入口函数
- 生成的C代码可以直接用C编译器编译执行
- 调用者负责使用`F_destroy_codegen_result()`函数销毁返回的结果

**示例：**
```c
#include "src/core/codegen/implementations/c_backend/CN_c_backend.h"

int main(void)
{
    // 假设ast_root是有效的AST根节点
    Stru_AstNode_t* ast_root = ...;
    
    // 生成C代码
    Stru_CodeGenResult_t* result = F_generate_c_code(ast_root);
    
    if (result && result->success) {
        printf("生成的C代码:\n%s\n", result->code);
    } else {
        fprintf(stderr, "代码生成失败\n");
    }
    
    // 清理资源
    if (result) {
        F_destroy_codegen_result(result);
    }
    
    return EXIT_SUCCESS;
}
```

### 2. `F_get_c_backend_version`

**功能：** 获取C后端版本信息

**原型：**
```c
void F_get_c_backend_version(int* major, int* minor, int* patch);
```

**参数：**
- `major`: 输出参数，主版本号
- `minor`: 输出参数，次版本号
- `patch`: 输出参数，修订号

**说明：**
- 版本号遵循语义化版本规范：
  - 主版本号：不兼容的API修改
  - 次版本号：向下兼容的功能性新增
  - 修订号：向下兼容的问题修正

**示例：**
```c
int major, minor, patch;
F_get_c_backend_version(&major, &minor, &patch);
printf("C后端版本: %d.%d.%d\n", major, minor, patch);
```

### 3. `F_get_c_backend_version_string`

**功能：** 获取C后端版本字符串

**原型：**
```c
const char* F_get_c_backend_version_string(void);
```

**返回值：**
- 版本字符串，格式为"主版本号.次版本号.修订号"

**说明：**
- 返回的字符串是静态常量，调用者不应修改或释放

**示例：**
```c
const char* version = F_get_c_backend_version_string();
printf("C后端版本: %s\n", version);
```

### 4. `F_c_backend_supports_feature`

**功能：** 检查C后端是否支持特定功能

**原型：**
```c
bool F_c_backend_supports_feature(const char* feature);
```

**参数：**
- `feature`: 功能标识符

**返回值：**
- 支持返回`true`，不支持返回`false`

**支持的功能标识符：**
- `"c99"`: 支持C99标准
- `"c11"`: 支持C11标准
- `"c17"`: 支持C17标准
- `"posix"`: 支持POSIX扩展
- `"threads"`: 支持线程安全代码生成（暂不支持）
- `"exceptions"`: 支持异常处理代码生成（暂不支持）

**示例：**
```c
if (F_c_backend_supports_feature("c11")) {
    printf("支持C11标准\n");
}
```

### 5. `F_configure_c_backend`

**功能：** 配置C后端选项

**原型：**
```c
bool F_configure_c_backend(const char* option, const char* value);
```

**参数：**
- `option`: 选项名称
- `value`: 选项值

**返回值：**
- 配置成功返回`true`，失败返回`false`

**支持的选项：**
- `"standard"`: C标准 (`"c99"`, `"c11"`, `"c17"`)
- `"optimization_level"`: 优化级别 (0-3)
- `"debug_info"`: 是否生成调试信息 (`true`/`false`)
- `"warnings"`: 是否启用警告 (`true`/`false`)

**示例：**
```c
// 配置使用C11标准
F_configure_c_backend("standard", "c11");

// 启用优化级别2
F_configure_c_backend("optimization_level", "2");

// 启用调试信息
F_configure_c_backend("debug_info", "true");
```

### 6. `F_create_c_backend_interface`

**功能：** 创建C后端代码生成器接口

**原型：**
```c
Stru_CodeGeneratorInterface_t* F_create_c_backend_interface(void);
```

**返回值：**
- 成功：返回C后端代码生成器接口实例
- 失败：返回NULL

**说明：**
- 此函数创建专门用于C代码生成的接口实例
- 与通用的`F_create_codegen_interface()`函数不同，此函数创建的接口已经预配置为C后端
- 调用者负责在不再使用时调用接口的`destroy()`函数

**示例：**
```c
Stru_CodeGeneratorInterface_t* c_backend = F_create_c_backend_interface();
if (c_backend) {
    // 使用C后端接口
    // ...
    c_backend->destroy(c_backend);
}
```

## 数据类型

### 1. `Stru_CodeGenResult_t`

代码生成结果结构体，定义在`CN_codegen_interface.h`中：

```c
typedef struct {
    bool success;                    ///< 是否成功
    char* code;                      ///< 生成的代码字符串
    size_t code_length;              ///< 代码长度
    Stru_DynamicArray_t* errors;     ///< 错误信息数组
    Stru_DynamicArray_t* warnings;   ///< 警告信息数组
    size_t instruction_count;        ///< 生成的指令数量
    size_t memory_usage;             ///< 内存使用量（字节）
} Stru_CodeGenResult_t;
```

### 2. 内部状态结构体

C后端内部使用的状态结构体：

```c
typedef struct {
    int indent_level;                ///< 当前缩进级别
    char* output_buffer;             ///< 输出缓冲区
    size_t buffer_size;              ///< 缓冲区大小
    size_t buffer_pos;               ///< 缓冲区当前位置
    Stru_DynamicArray_t* errors;     ///< 错误信息
    Stru_DynamicArray_t* warnings;   ///< 警告信息
    bool has_errors;                 ///< 是否有错误
    bool has_warnings;               ///< 是否有警告
} CBackendState;
```

## 代码生成过程

### 1. 初始化阶段
- 创建内部状态结构体
- 初始化输出缓冲区
- 创建错误和警告收集器

### 2. 代码生成阶段
- 生成文件头注释
- 添加必要的C头文件包含
- 遍历AST生成全局声明
- 生成`main`函数框架
- 递归处理AST节点生成具体代码

### 3. 清理阶段
- 将生成的代码和错误信息转移到结果结构体
- 清理内部状态
- 返回结果

## 支持的AST节点类型

C后端支持以下AST节点类型的代码生成：

### 声明节点
- `Eum_AST_VARIABLE_DECL`: 变量声明
- `Eum_AST_FUNCTION_DECL`: 函数声明

### 语句节点
- `Eum_AST_EXPRESSION_STMT`: 表达式语句
- `Eum_AST_IF_STMT`: if语句
- `Eum_AST_WHILE_STMT`: while语句
- `Eum_AST_RETURN_STMT`: return语句
- `Eum_AST_BLOCK_STMT`: 代码块语句

### 表达式节点
- `Eum_AST_BINARY_EXPR`: 二元表达式
- `Eum_AST_UNARY_EXPR`: 一元表达式
- `Eum_AST_LITERAL_EXPR`: 字面量表达式
- `Eum_AST_INT_LITERAL`: 整数字面量
- `Eum_AST_FLOAT_LITERAL`: 浮点数字面量
- `Eum_AST_STRING_LITERAL`: 字符串字面量
- `Eum_AST_BOOL_LITERAL`: 布尔字面量
- `Eum_AST_IDENTIFIER_EXPR`: 标识符表达式
- `Eum_AST_CALL_EXPR`: 函数调用表达式

## 类型映射

C后端将CN_Language类型映射到C类型：

| CN_Language类型 | C类型 |
|----------------|-------|
| `"整数"` 或 `"int"` | `int` |
| `"浮点数"` 或 `"float"` | `float` |
| `"双精度"` 或 `"double"` | `double` |
| `"布尔"` 或 `"bool"` | `bool` |
| `"字符"` 或 `"char"` | `char` |
| `"字符串"` 或 `"string"` | `char*` |
| `"空"` 或 `"void"` | `void` |

## 运算符映射

C后端将CN_Language运算符映射到C运算符：

| CN_Language运算符 | C运算符 | 说明 |
|------------------|---------|------|
| 1 | `+` | 加法 |
| 2 | `-` | 减法 |
| 3 | `*` | 乘法 |
| 4 | `/` | 除法 |
| 5 | `%` | 取模 |
| 6 | `==` | 等于 |
| 7 | `!=` | 不等于 |
| 8 | `<` | 小于 |
| 9 | `<=` | 小于等于 |
| 10 | `>` | 大于 |
| 11 | `>=` | 大于等于 |
| 12 | `&&` | 逻辑与 |
| 13 | `||` | 逻辑或 |
| 14 | `!` | 逻辑非 |
| 15 | `=` | 赋值 |
| 16 | `+=` | 加法赋值 |
| 17 | `-=` | 减法赋值 |
| 18 | `*=` | 乘法赋值 |
| 19 | `/=` | 除法赋值 |

## 错误处理

C后端使用统一的错误处理机制：

### 1. 错误收集
- 所有错误被收集到错误数组中
- 错误按严重程度分类（错误、警告、信息）
- 尽可能从错误中恢复并继续

### 2. 错误类型
- **语法错误**: AST结构不符合预期
- **类型错误**: 类型不匹配或未定义
- **语义错误**: 语义规则违反
- **资源错误**: 内存分配失败等

### 3. 错误报告
- 提供详细的错误信息和位置
- 错误信息包含上下文信息
- 支持多语言错误消息

## 使用示例

### 完整示例

```c
#include <stdio.h>
#include <stdlib.h>
#include "src/core/codegen/implementations/c_backend/CN_c_backend.h"

int main(void)
{
    // 创建AST（简化示例）
    // 实际应用中，AST应该由解析器生成
    Stru_AstNode_t* ast = create_sample_ast();
    
    if (!ast) {
        fprintf(stderr, "创建AST失败\n");
        return EXIT_FAILURE;
    }
    
    // 配置C后端
    F_configure_c_backend("standard", "c11");
    F_configure_c_backend("optimization_level", "2");
    F_configure_c_backend("debug_info", "true");
    
    // 生成C代码
    Stru_CodeGenResult_t* result = F_generate_c_code(ast);
    
    // 检查结果
    if (!result) {
        fprintf(stderr, "代码生成失败：无法创建结果\n");
        destroy_ast(ast);
        return EXIT_FAILURE;
    }
    
    if (result->success) {
        printf("代码生成成功！\n");
        printf("生成的代码长度: %zu 字节\n", result->code_length);
        printf("生成的代码:\n%s\n", result->code);
        
        // 将代码写入文件
        FILE* output = fopen("generated.c", "w");
        if (output) {
            fwrite(result->code, 1, result->code_length, output);
            fclose(output);
            printf("代码已写入 generated.c\n");
        }
    } else {
        fprintf(stderr, "代码生成失败\n");
        
        // 输出错误信息
        if (result->errors) {
            size_t error_count = F_dynamic_array_length(result->errors);
            for (size_t i = 0; i < error_count; i++) {
                char** error_ptr = (char**)F_dynamic_array_get(result->errors, i);
                if (error_ptr && *error_ptr) {
                    fprintf(stderr, "错误 %zu: %s\n", i + 1, *error_ptr);
                }
            }
        }
    }
    
    // 输出警告信息
    if (result->warnings) {
        size_t warning_count = F_dynamic_array_length(result->warnings);
        if (warning_count > 0) {
            printf("警告信息 (%zu 个):\n", warning_count);
            for (size_t i = 0; i < warning_count; i++) {
                char** warning_ptr = (char**)F_dynamic_array_get(result->warnings, i);
                if (warning_ptr && *warning_ptr) {
                    printf("警告 %zu: %s\n", i + 1, *warning_ptr);
                }
            }
        }
    }
    
    // 清理资源
    F_destroy_codegen_result(result);
    destroy_ast(ast);
    
    return EXIT_SUCCESS;
}
```

### 生成的C代码示例

输入CN_Language代码：
```
函数 主() {
    变量 数字 = 42;
    打印("数字是: " + 数字);
    返回 0;
}
```

生成的C代码：
```c
/******************************************************************************
 * 文件名: generated.c
 * 功能: 由CN_Language编译器生成的C代码
 * 作者: CN_Language编译器
 * 生成日期: 2026-01-10
 * 警告: 这是自动生成的代码，请勿手动修改
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* 全局声明 */
int 主(void);

int main(void)
{
    printf("CN_Language程序开始执行...\n");
    int 数字 = 42;
    printf("数字是: %d\n", 数字);
    printf("CN_Language程序执行完成。\n");
    return 0;
}
```

## 性能考虑

### 1. 内存使用
- 使用动态缓冲区管理输出代码
- 缓冲区按需增长，避免频繁重新分配
- 错误和警告信息使用动态数组存储

### 2. 代码生成效率
- 递归遍历AST，时间复杂度O(n)
- 使用缓冲区追加操作，避免字符串拼接开销
- 支持增量代码生成

### 3. 可扩展性
- 模块化设计，易于添加新的AST节点支持
- 支持插件式优化器
- 可配置的代码生成选项

## 限制和已知问题

### 1. 当前限制
- 不支持完整的C++特性
- 线程安全代码生成暂未实现
- 异常处理支持有限
- 某些高级优化尚未实现

### 2. 已知问题
- 复杂嵌套表达式的代码生成可能不够优化
- 某些边界情况下的错误处理可能不够完善
- 类型系统映射可能不完全准确

## 版本历史

### 版本 1.0.0 (2026-01-10)
- 初始版本发布
- 支持基本AST节点代码生成
- 实现类型和运算符映射
- 添加错误处理和警告机制
- 提供配置接口

## 未来计划

### 短期计划
- 添加更多C标准支持（C23）
- 实现线程安全代码生成
- 改进错误消息质量

### 中期计划
- 添加代码优化功能
- 支持生成调试信息
- 实现增量代码生成

### 长期计划
- 支持生成SIMD指令
- 添加自动向量化
- 支持生成GPU代码

## 许可证

MIT许可证

## 维护者

CN_Language架构委员会
C后端开发工作组

## 联系方式

- 项目主页: https://github.com/ChenChao-HuaiAn/CN_Language
- 问题跟踪: https://github.com/ChenChao-HuaiAn/CN_Language/issues
- 文档: https://github.com/ChenChao-HuaiAn/CN_Language/docs
