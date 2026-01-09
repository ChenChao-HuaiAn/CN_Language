# CN_Language 命令行界面（CLI）模块 API 文档

## 概述

命令行界面（CLI）模块是CN_Language项目的用户交互入口点，负责解析用户输入的命令行参数，执行相应的命令，并提供统一的用户交互体验。本模块采用模块化设计，遵循SOLID原则和分层架构。

## 模块架构

CLI模块采用三层设计，实现关注点分离：

```
┌─────────────────────────────────────────┐
│         命令行界面（CN_cli）            │ ← 整合层
├─────────────────────────────────────────┤
│     命令执行器（CN_command_executor）   │ ← 业务逻辑层
├─────────────────────────────────────────┤
│     命令解析器（CN_command_parser）     │ ← 数据解析层
└─────────────────────────────────────────┘
```

### 1. 命令解析器（Command Parser）

**职责**：解析命令行参数，提取命令、选项、标志和参数值。

#### 接口定义

```c
// 文件：CN_command_parser.h

// 参数类型枚举
typedef enum {
    Eum_ARG_TYPE_COMMAND,      // 命令（如compile、run、debug）
    Eum_ARG_TYPE_OPTION,       // 选项（如-o、--output）
    Eum_ARG_TYPE_FLAG,         // 标志（如-v、--verbose）
    Eum_ARG_TYPE_VALUE,        // 参数值（如文件名）
    Eum_ARG_TYPE_INVALID       // 无效参数
} Eum_ArgumentType_t;

// 命令行参数结构体
typedef struct {
    Eum_ArgumentType_t type;   // 参数类型
    const char* name;          // 参数名称
    const char* value;         // 参数值（如有）
    size_t position;           // 参数在命令行中的位置
} Stru_Argument_t;

// 解析结果结构体
typedef struct {
    Stru_Argument_t* arguments;    // 参数数组
    size_t count;                  // 参数个数
    size_t capacity;               // 数组容量
    const char* program_name;      // 程序名称
    const char* command;           // 主命令
} Stru_ParseResult_t;

// 命令解析器接口
typedef struct Stru_CommandParserInterface_t {
    Stru_ParseResult_t* (*parse)(Stru_CommandParserInterface_t* self, int argc, char** argv);
    bool (*validate)(Stru_CommandParserInterface_t* self, const Stru_ParseResult_t* result);
    const Stru_Argument_t* (*find_argument)(Stru_CommandParserInterface_t* self, 
                                           const Stru_ParseResult_t* result, 
                                           const char* name);
    const char* (*get_command)(Stru_CommandParserInterface_t* self, 
                              const Stru_ParseResult_t* result);
    const char* (*get_option_value)(Stru_CommandParserInterface_t* self, 
                                   const Stru_ParseResult_t* result, 
                                   const char* option_name);
    bool (*has_flag)(Stru_CommandParserInterface_t* self, 
                    const Stru_ParseResult_t* result, 
                    const char* flag_name);
    void (*destroy_result)(Stru_CommandParserInterface_t* self, Stru_ParseResult_t* result);
    void (*destroy)(Stru_CommandParserInterface_t* self);
} Stru_CommandParserInterface_t;

// 工厂函数
Stru_CommandParserInterface_t* F_create_command_parser(void);
```

#### 支持的参数格式

1. **命令**：`compile`, `run`, `debug`, `help`, `version`
2. **短选项**：`-o`, `-v`, `-h`
3. **长选项**：`--output`, `--verbose`, `--help`
4. **带值选项**：
   - 空格分隔：`-o output.exe`
   - 等号分隔：`--output=output.exe`
5. **标志**：`-V`, `--verbose`

#### 使用示例

```c
// 创建解析器
Stru_CommandParserInterface_t* parser = F_create_command_parser();

// 解析命令行参数
Stru_ParseResult_t* result = parser->parse(parser, argc, argv);

// 验证解析结果
if (parser->validate(parser, result)) {
    // 获取命令
    const char* command = parser->get_command(parser, result);
    
    // 获取选项值
    const char* output_file = parser->get_option_value(parser, result, "-o");
    
    // 检查标志
    bool verbose = parser->has_flag(parser, result, "--verbose");
}

// 清理资源
parser->destroy_result(parser, result);
parser->destroy(parser);
```

### 2. 命令执行器（Command Executor）

**职责**：执行具体的命令操作，如编译、运行、调试等。

#### 接口定义

```c
// 文件：CN_command_executor.h

// 执行结果枚举
typedef enum {
    Eum_EXEC_RESULT_SUCCESS,      // 执行成功
    Eum_EXEC_RESULT_FAILURE,      // 执行失败
    Eum_EXEC_RESULT_SYNTAX_ERROR, // 语法错误
    Eum_EXEC_RESULT_FILE_ERROR,   // 文件错误
    Eum_EXEC_RESULT_SYSTEM_ERROR, // 系统错误
    Eum_EXEC_RESULT_UNKNOWN_CMD   // 未知命令
} Eum_ExecutionResult_t;

// 命令执行器接口
typedef struct Stru_CommandExecutorInterface_t {
    Eum_ExecutionResult_t (*execute)(Stru_CommandExecutorInterface_t* self, 
                                    const Stru_ParseResult_t* parse_result);
    Eum_ExecutionResult_t (*execute_help)(Stru_CommandExecutorInterface_t* self, 
                                         const Stru_ParseResult_t* parse_result);
    Eum_ExecutionResult_t (*execute_version)(Stru_CommandExecutorInterface_t* self, 
                                            const Stru_ParseResult_t* parse_result);
    Eum_ExecutionResult_t (*execute_compile)(Stru_CommandExecutorInterface_t* self, 
                                            const Stru_ParseResult_t* parse_result);
    Eum_ExecutionResult_t (*execute_run)(Stru_CommandExecutorInterface_t* self, 
                                        const Stru_ParseResult_t* parse_result);
    Eum_ExecutionResult_t (*execute_debug)(Stru_CommandExecutorInterface_t* self, 
                                          const Stru_ParseResult_t* parse_result);
    const char* (*get_error_message)(Stru_CommandExecutorInterface_t* self);
    void (*destroy)(Stru_CommandExecutorInterface_t* self);
} Stru_CommandExecutorInterface_t;

// 工厂函数
Stru_CommandExecutorInterface_t* F_create_command_executor(void);
```

#### 支持的命令

1. **help**：显示帮助信息
2. **version**：显示版本信息
3. **compile**：编译CN源代码文件
4. **run**：运行CN程序
5. **debug**：调试CN程序

#### 使用示例

```c
// 创建执行器
Stru_CommandExecutorInterface_t* executor = F_create_command_executor();

// 执行命令
Eum_ExecutionResult_t result = executor->execute(executor, parse_result);

// 检查执行结果
if (result == Eum_EXEC_RESULT_SUCCESS) {
    printf("命令执行成功\n");
} else {
    // 获取错误信息
    const char* error_msg = executor->get_error_message(executor);
    fprintf(stderr, "错误: %s\n", error_msg);
}

// 清理资源
executor->destroy(executor);
```

### 3. 命令行界面（CLI）

**职责**：整合命令解析器和执行器，提供统一的命令行界面。

#### 接口定义

```c
// 文件：CN_cli_interface.h

// 命令行界面接口
typedef struct Stru_CliInterface_t {
    bool (*initialize)(Stru_CliInterface_t* self, int argc, char** argv);
    int (*parse_and_execute)(Stru_CliInterface_t* self);
    Stru_CommandParserInterface_t* (*get_parser)(Stru_CliInterface_t* self);
    Stru_CommandExecutorInterface_t* (*get_executor)(Stru_CliInterface_t* self);
    void (*show_help)(Stru_CliInterface_t* self);
    void (*show_version)(Stru_CliInterface_t* self);
    void (*destroy)(Stru_CliInterface_t* self);
} Stru_CliInterface_t;

// 工厂函数
Stru_CliInterface_t* F_create_cli_interface(void);
```

#### 使用示例

```c
// 创建CLI实例
Stru_CliInterface_t* cli = F_create_cli_interface();

// 初始化
cli->initialize(cli, argc, argv);

// 解析并执行命令
int exit_code = cli->parse_and_execute(cli);

// 清理资源
cli->destroy(cli);

return exit_code;
```

## 命令行用法

### 基本语法

```
cn <命令> [选项] [参数]
```

### 可用命令

| 命令 | 描述 | 示例 |
|------|------|------|
| `help` | 显示帮助信息 | `cn help` |
| `version` | 显示版本信息 | `cn version` |
| `compile` | 编译CN源代码文件 | `cn compile hello.cn` |
| `run` | 运行CN程序 | `cn run hello.cn` |
| `debug` | 调试CN程序 | `cn debug hello.cn` |

### 可用选项

| 选项 | 长选项 | 描述 | 适用命令 |
|------|--------|------|----------|
| `-h` | `--help` | 显示帮助信息 | 所有命令 |
| `-v` | `--version` | 显示版本信息 | 所有命令 |
| `-o` | `--output` | 指定输出文件 | `compile` |
| `-V` | `--verbose` | 显示详细输出 | 所有命令 |

### 使用示例

1. **显示帮助**：
   ```bash
   cn help
   cn -h
   cn --help
   ```

2. **显示版本**：
   ```bash
   cn version
   cn -v
   cn --version
   ```

3. **编译程序**：
   ```bash
   cn compile hello.cn
   cn compile hello.cn -o hello.exe
   cn compile hello.cn --output=hello.exe --verbose
   ```

4. **运行程序**：
   ```bash
   cn run hello.cn
   cn run hello.cn --verbose
   ```

5. **调试程序**：
   ```bash
   cn debug hello.cn
   cn debug hello.cn -V
   ```

## 错误处理

### 错误码

| 错误码 | 描述 | 可能原因 |
|--------|------|----------|
| 0 | 成功 | 命令执行成功 |
| 1 | 一般错误 | 命令执行失败 |
| 2 | 语法错误 | 命令行参数格式错误 |
| 3 | 文件错误 | 文件不存在或无法访问 |
| 4 | 系统错误 | 系统资源不足 |
| 5 | 未知命令 | 输入了不支持的命令 |

### 错误信息

所有错误信息通过标准错误输出（stderr）显示，同时可以通过`get_error_message()`方法获取详细的错误描述。

## 集成核心层

### 编译管道集成

CLI模块通过命令执行器与核心层的编译管道集成：

```c
// 在execute_compile方法中集成核心层
static Eum_ExecutionResult_t F_execute_compile_impl(Stru_CommandExecutorInterface_t* self, 
                                                   const Stru_ParseResult_t* parse_result)
{
    // 1. 获取输入文件
    const char* input_file = F_get_first_value_argument(parse_result);
    
    // 2. 获取输出文件（如果有）
    const char* output_file = F_get_option_value(parse_result, "-o");
    
    // 3. 调用核心层编译功能
    // TODO: 集成核心层编译管道
    // Stru_CompilerInterface_t* compiler = F_create_compiler();
    // compiler->compile(compiler, input_file, output_file);
    
    return Eum_EXEC_RESULT_SUCCESS;
}
```

### 运行时集成

类似地，运行和调试命令也将集成核心层的运行时系统。

## 扩展性

### 添加新命令

要添加新命令，需要：

1. 在命令解析器中添加命令识别
2. 在命令执行器中实现命令逻辑
3. 更新帮助信息

### 添加新选项

要添加新选项，需要：

1. 在命令解析器中添加选项解析逻辑
2. 在命令执行器中处理选项值
3. 更新帮助信息

## 测试

### 单元测试

每个模块都有对应的单元测试，确保功能正确性：

1. **命令解析器测试**：测试各种参数格式的解析
2. **命令执行器测试**：测试各命令的执行逻辑
3. **CLI集成测试**：测试完整的命令行流程

### 集成测试

测试CLI与核心层的集成：

```bash
# 编译测试
make test-compile

# 运行测试
make test-run

# 调试测试
make test-debug
```

## 性能考虑

1. **内存效率**：使用动态数组存储解析结果，避免固定大小限制
2. **解析速度**：使用高效的字符串处理算法
3. **错误恢复**：提供清晰的错误信息，便于用户调试

## 兼容性

### 支持的平台

- Linux
- Windows（通过MinGW或Cygwin）
- macOS

### 编译器要求

- GCC 4.8+ 或 Clang 3.5+
- C11标准兼容

## 参考

- [CN_Language架构设计文档](../architecture/001-中文编程语言CN_Language开发规划.md)
- [CN_Language技术规范文档](../specifications/CN_Language项目%20技术规范和编码标准.md)
- [CN_Language构建方法文档](../specifications/CN_Language项目%20构建方法和编译流程.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-09 | 初始版本，基本命令解析和执行 |
| 1.1.0 | 2026-01-09 | 添加选项支持，改进错误处理 |

## 维护者

- CN_Language架构委员会
- 问题反馈：项目仓库 Issues 页面

## 许可证

MIT License
