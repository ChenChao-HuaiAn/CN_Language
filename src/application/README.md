# CN_Language 应用层

## 概述

应用层是CN_Language项目的用户界面层，负责提供用户交互接口。遵循分层架构设计，应用层依赖核心层和基础设施层，实现单向依赖关系。

## 目录结构

```
src/application/
├── cli/                    # 命令行界面模块
│   ├── CN_cli_interface.h  # CLI接口定义
│   └── CN_cli.c           # CLI实现
├── repl/                   # 交互式环境模块
│   └── CN_repl_interface.h # REPL接口定义
├── debugger/               # 调试器模块
│   └── CN_debugger_interface.h # 调试器接口定义
├── ide-plugin/             # IDE插件模块（预留）
├── CN_main.c              # 主程序入口点
└── README.md              # 本文档
```

## 模块说明

### 1. 命令行界面（CLI）

CLI模块采用模块化设计，分为三个子模块：

#### 1.1 命令解析器（Command Parser）
**功能**：
- 解析命令行参数，支持命令、选项、标志和参数值
- 支持短选项（-h）、长选项（--help）、等号分隔选项（--output=file）
- 验证参数语法，提供详细的错误信息

**接口**：`Stru_CommandParserInterface_t`
- `parse()` - 解析命令行参数
- `validate()` - 验证解析结果
- `get_command()` - 获取主命令
- `get_option_value()` - 获取选项值
- `has_flag()` - 检查标志是否存在
- `destroy_result()` - 销毁解析结果
- `destroy()` - 清理资源

#### 1.2 命令执行器（Command Executor）
**功能**：
- 执行具体的命令操作（编译、运行、调试等）
- 提供统一的错误处理机制
- 集成核心层功能

**接口**：`Stru_CommandExecutorInterface_t`
- `execute()` - 执行命令
- `execute_help()` - 执行帮助命令
- `execute_version()` - 执行版本命令
- `execute_compile()` - 执行编译命令
- `execute_run()` - 执行运行命令
- `execute_debug()` - 执行调试命令
- `get_error_message()` - 获取错误信息
- `destroy()` - 清理资源

#### 1.3 命令行界面整合层（CLI Integration）
**功能**：
- 整合命令解析器和执行器
- 提供统一的用户界面
- 处理错误和资源管理

**接口**：`Stru_CliInterface_t`
- `initialize()` - 初始化命令行界面
- `parse_and_execute()` - 解析并执行命令
- `get_parser()` - 获取命令解析器
- `get_executor()` - 获取命令执行器
- `show_help()` - 显示帮助信息
- `show_version()` - 显示版本信息
- `destroy()` - 清理资源

**使用示例**：
```bash
# 显示帮助
./bin/CN_Language help
./bin/CN_Language -h
./bin/CN_Language --help

# 显示版本
./bin/CN_Language version
./bin/CN_Language -v
./bin/CN_Language --version

# 编译CN程序（带选项）
./bin/CN_Language compile hello.cn
./bin/CN_Language compile hello.cn -o hello.exe
./bin/CN_Language compile hello.cn --output=hello.exe --verbose

# 运行CN程序
./bin/CN_Language run hello.cn
./bin/CN_Language run hello.cn --verbose

# 调试CN程序
./bin/CN_Language debug hello.cn
./bin/CN_Language debug hello.cn -V
```

### 2. 交互式环境（REPL）

**功能**：
- 提供读取-求值-打印循环
- 支持逐行执行CN代码
- 历史记录功能
- 自动补全建议

**接口**：`Stru_ReplInterface_t`
- `initialize()` - 初始化交互式环境
- `start()` - 启动REPL循环
- `execute_line()` - 执行单行代码
- `get_history()` - 获取历史记录
- `add_completions()` - 添加自动补全建议
- `show_welcome()` - 显示欢迎信息
- `destroy()` - 清理资源

### 3. 调试器（Debugger）

**功能**：
- 设置和管理断点
- 单步执行（进入/跳过）
- 变量查看和修改
- 调用栈跟踪
- 与运行时系统交互

**接口**：`Stru_DebuggerInterface_t`
- `initialize()` - 初始化调试器
- `start_session()` - 启动调试会话
- `set_breakpoint()` - 设置断点
- `remove_breakpoint()` - 删除断点
- `step_into()` - 单步进入
- `step_over()` - 单步跳过
- `continue_execution()` - 继续执行
- `inspect_variable()` - 查看变量
- `get_call_stack()` - 获取调用栈
- `pause_execution()` - 暂停执行
- `stop_session()` - 停止调试会话
- `destroy()` - 清理资源

## 构建说明

### 依赖关系

应用层依赖以下层：
1. **核心层** (`src/core/`) - 编译器核心功能
2. **基础设施层** (`src/infrastructure/`) - 基础服务和工具

### 构建命令

```bash
# 清理构建文件
make clean

# 构建项目
make

# 运行程序（显示帮助）
make run

# 运行测试
make test-help
make test-version
make test-compile
make test-run
make test-debug

# 显示构建信息
make info

# 显示帮助
make help
```

### 编译选项

- 编译器：gcc
- 标准：C11
- 警告：-Wall -Wextra
- 调试信息：-g
- 特性宏：-D_GNU_SOURCE（启用strdup等函数）

## 架构原则

### 1. 分层架构
- 应用层 → 核心层 → 基础设施层（单向依赖）
- 模块间通过抽象接口通信
- 依赖倒置原则：高层定义接口，低层实现

### 2. SOLID原则
- **单一职责**：每个模块只负责一个功能领域
- **开闭原则**：通过接口支持扩展，对修改封闭
- **里氏替换**：接口实现可无缝替换
- **接口隔离**：为不同客户端提供专用接口
- **依赖倒置**：高层模块不依赖低层模块具体实现

### 3. 模块化设计
- 功能内聚性：每个模块专注于单一功能
- 数据封装：内部数据结构对外不可见
- 接口最小化：只暴露必要的API
- 独立编译：每个模块可独立编译测试

## 开发指南

### 1. 添加新模块
1. 在对应目录创建接口头文件（`*_interface.h`）
2. 定义抽象接口结构体和方法
3. 创建实现文件（`*.c`）
4. 实现所有接口方法
5. 提供工厂函数创建实例
6. 更新主程序集成新模块

### 2. 代码规范
- 函数不超过50行
- 文件不超过500行
- 使用统一的命名约定
- 添加完整的文档注释
- 遵循错误处理规范

### 3. 测试要求
- 为每个公开接口编写单元测试
- 测试所有错误路径
- 保持测试覆盖率≥85%
- 集成测试验证模块协作

## 下一步工作

### 短期目标
1. 实现REPL模块的基本功能
2. 实现调试器模块的框架
3. 集成核心层的编译功能
4. 添加更多命令行选项

### 长期目标
1. 实现完整的IDE插件
2. 添加图形用户界面
3. 支持远程调试
4. 提供Web界面

## 相关文档

- [架构设计文档](../docs/architecture/)
- [技术规范文档](../docs/specifications/)
- [API文档](../docs/api/)
- [测试文档](../tests/)

## 许可证

MIT License
```

<environment_details>
# VSCode Visible Files
src/application/README.md

# VSCode Open Tabs
docs/architecture/001-中文编程语言CN_Language开发规划.md
docs/architecture/002-语编程语言（CN_Language）项目现状的全面评估和后续步骤规划.md
src/application/cli/CN_cli_interface.h
src/application/repl/CN_repl_interface.h
src/application/debugger/CN_debugger_interface.h
src/application/CN_main.c
src/application/cli/CN_cli.c
Makefile
src/application/README.md

# 当前时间
2026/1/9 下午4:20:48 (Asia/Shanghai, UTC+8:00)

# 上下文窗口使用
59,920 / 128K 令牌已使用 (47%)

# 当前模式
行动模式
</environment_details>
