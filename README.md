# CN_Language - 中文编程语言

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![GitHub last commit](https://img.shields.io/github/last-commit/ChenChao_GitCode/CN_Language)](https://github.com/ChenChao_GitCode/CN_Language)
[![GitHub issues](https://img.shields.io/github/issues/ChenChao_GitCode/CN_Language)](https://github.com/ChenChao_GitCode/CN_Language/issues)

CN_Language 是一个为中文用户设计的系统级编程语言，采用现代软件工程原则和分层架构设计，旨在提供友好、直观的中文编程体验。

## 📋 项目概述

CN_Language 是一个创新的中文编程语言项目，具有以下特点：

- **中文友好**：使用中文关键字和标识符，降低中文用户的学习门槛
- **系统级编程**：支持操作系统开发和底层编程
- **现代架构**：采用三层分层架构和SOLID设计原则
- **高性能**：编译为本地代码，执行效率高
- **跨平台**：支持Windows、Linux、macOS等主流操作系统

## 🏗️ 项目架构

### 三层分层架构

CN_Language 采用清晰的三层架构设计，确保代码的可维护性和可扩展性：

```
┌─────────────────────────────────────────┐
│           应用层 (Application Layer)    │
│  - 命令行界面 (CLI)                     │
│  - REPL交互环境                         │
│  - 调试器界面                           │
│  - 集成开发环境插件                     │
├─────────────────────────────────────────┤
│           核心层 (Core Layer)           │
│  - 编译器前端 (词法分析、语法分析)      │
│  - 编译器后端 (代码生成、优化)          │
│  - 运行时系统 (内存管理、执行引擎)      │
│  - 标准库实现                           │
├─────────────────────────────────────────┤
│       基础设施层 (Infrastructure Layer) │
│  - 基础数据结构 (字符串、数组、哈希表)  │
│  - 内存管理 (分配器、垃圾回收)          │
│  - 错误处理 (异常、日志)                │
│  - 工具库 (UTF-8处理、文件IO、网络)     │
│  - 平台抽象层 (操作系统接口)            │
└─────────────────────────────────────────┘
```

### 架构设计原则

1. **SOLID原则**：
   - 单一职责：每个模块只负责一个功能领域
   - 开闭原则：通过抽象接口支持扩展
   - 里氏替换：接口实现完全遵守契约
   - 接口隔离：为不同客户端提供专用接口
   - 依赖倒置：高层模块定义接口，低层模块实现

2. **模块化设计**：
   - 功能内聚性：模块专注于单一功能领域
   - 数据封装：模块内部数据结构对外部不可见
   - 接口最小化：只暴露必要的API
   - 独立编译：每个模块可独立编译和测试

## 📁 项目结构

```
CN_Language/
├── docs/                    # 项目文档
│   ├── architecture/        # 架构设计文档
│   ├── specifications/      # 技术规范和编码标准
│   ├── api/                # API文档
│   ├── design/             # 设计文档
│   └── tutorials/          # 教程和示例
├── src/                    # 源代码（按三层架构组织）
│   ├── infrastructure/     # 基础设施层
│   │   ├── memory/         # 内存管理
│   │   ├── containers/     # 基础数据结构
│   │   ├── utils/          # 工具函数
│   │   └── platform/       # 平台抽象层
│   ├── core/              # 核心层
│   │   ├── lexer/         # 词法分析器
│   │   ├── parser/        # 语法分析器
│   │   ├── ast/           # 抽象语法树
│   │   ├── semantic/      # 语义分析
│   │   ├── codegen/       # 代码生成器
│   │   └── runtime/       # 运行时系统
│   └── application/       # 应用层
│       ├── cli/           # 命令行界面
│       ├── repl/          # REPL交互环境
│       ├── debugger/      # 调试器
│       └── ide-plugin/    # IDE集成插件
├── tests/                  # 测试代码
│   ├── unit/              # 单元测试
│   ├── integration/       # 集成测试
│   └── e2e/               # 端到端测试
├── examples/               # 示例代码
├── tools/                  # 开发工具
├── scripts/                # 构建脚本
├── bin/                    # 编译生成的可执行文件
└── build/                  # 编译过程中生成的对象文件
```

## 🚀 快速开始

### 系统要求

- **操作系统**：Linux、Windows 或 macOS
- **编译器**：GCC 或 Clang（支持C11标准）
- **构建工具**：Make
- **内存**：至少2GB RAM
- **磁盘空间**：至少500MB可用空间

### 安装步骤

1. **克隆仓库**
   ```bash
   git clone git@gitcode.com:ChenChao_GitCode/CN_Language.git
   cd CN_Language
   ```

2. **构建项目**
   ```bash
   make all
   ```

3. **运行程序**
   ```bash
   ./bin/CN_Language help
   ```

### 验证安装

运行以下命令验证安装是否成功：

```bash
# 显示帮助信息
./bin/CN_Language help

# 显示版本信息
./bin/CN_Language version

# 测试编译功能
./bin/CN_Language compile examples/function_examples.cn
```

## 🔧 构建和编译

### 使用Makefile构建

项目使用Makefile作为构建系统，提供以下常用命令：

```bash
# 构建所有目标（默认）
make

# 清理构建文件
make clean

# 运行程序
make run

# 显示构建信息
make info

# 运行测试
make test-help
make test-version
make test-compile
make test-run
make test-debug
```

### 手动编译

如果需要手动编译，可以使用以下命令：

```bash
# 创建构建目录
mkdir -p build bin

# 编译应用层文件
gcc -Wall -Wextra -std=c11 -g -D_GNU_SOURCE \
    -I./src/application -I./src/core -I./src/infrastructure \
    -c src/application/CN_main.c -o build/application/CN_main.o

# 链接可执行文件
gcc -o bin/CN_Language build/application/*.o -lm
```

## 📝 使用示例

### 基本语法示例

以下是一个简单的CN_Language程序示例：

```cn
// 基本函数示例
函数 整数 加(整数 a, 整数 b) {
    返回 a + b;
}

函数 字符串 生成问候语(字符串 姓名) {
    返回 "你好，" + 姓名 + "！";
}

函数 主程序() {
    打印("=== CN_Language 示例程序 ===");
    
    变量 结果 = 加(5, 3);
    打印("5 + 3 = " + 结果);
    
    变量 问候语 = 生成问候语("张三");
    打印(问候语);
    
    返回 0;
}
```

### 运行示例

1. **创建CN源文件**
   ```bash
   echo '函数 主程序() { 打印("你好，世界！"); 返回 0; }' > hello.cn
   ```

2. **编译并运行**
   ```bash
   ./bin/CN_Language compile hello.cn
   ./bin/CN_Language run hello.cn
   ```

### 更多示例

查看 `examples/` 目录获取更多示例：

- `examples/function_examples.cn` - 函数关键字的各种用法
- `examples/array_examples.cn` - 数组操作示例
- `examples/platform_example.c` - 平台相关示例

## 👨‍💻 开发指南

### 代码规范

CN_Language 项目遵循严格的代码规范：

#### 文件命名规范
- **C源文件**：`CN_模块名_功能.c`（如`CN_lexer_tokenize.c`）
- **头文件**：`CN_模块名_功能.h`（如`CN_lexer_interface.h`）
- **CN语言源文件**：`.cn`扩展名（如`hello_world.cn`）

#### 代码命名规范
- **结构体**：`Stru_`前缀 + 大驼峰命名法 + `_t`后缀（如`Stru_Token_t`）
- **接口**：`Stru_`前缀 + 模块名 + `Interface_t`后缀（如`Stru_LexerInterface_t`）
- **函数**：`F_`前缀 + 描述性名称（如`F_create_token`）
- **枚举**：`Eum_`前缀 + 大驼峰命名法（如`Eum_TokenType`）
- **常量**：全大写（如`MAX_TOKEN_LENGTH`）
- **全局变量**：`g_`前缀 + 小驼峰命名法（如`g_current_line`）

#### 编码标准
- **缩进**：使用4个空格，禁止使用制表符
- **大括号风格**：Allman风格（大括号单独成行）
- **行长度**：每行不超过120个字符
- **函数长度**：每个函数不超过50行
- **文件长度**：每个.c文件不超过500行

### 开发工作流程

1. **代码规范检查**
   ```bash
   # 运行代码格式检查
   make format-check
   
   # 运行静态分析
   make static-analysis
   ```

2. **编写测试**
   ```bash
   # 运行单元测试
   make unit-test
   
   # 运行集成测试
   make integration-test
   
   # 运行端到端测试
   make e2e-test
   ```

3. **提交代码**
   ```bash
   # 提交前检查
   make pre-commit-check
   
   # 提交代码
   git add .
   git commit -m "描述你的更改"
   git push
   ```

### 测试策略

项目采用测试金字塔策略：

- **单元测试**：大量，覆盖所有公开接口
- **集成测试**：中等，测试模块间协作
- **端到端测试**：少量，完整编译流程测试

**测试覆盖率目标**：
- 语句覆盖率：≥85%
- 分支覆盖率：≥70%
- 函数覆盖率：≥90%
- 行覆盖率：≥85%

## 🤝 贡献指南

我们欢迎并感谢所有贡献！以下是参与贡献的步骤：

### 报告问题

如果您发现任何问题或有改进建议，请通过以下方式报告：

1. 在GitHub Issues中搜索是否已有类似问题
2. 如果没有，创建一个新的Issue
3. 提供清晰的问题描述、复现步骤和预期行为

### 提交代码

1. **Fork仓库**
   ```bash
   # Fork项目到您的GitHub账户
   ```

2. **创建分支**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **进行更改**
   ```bash
   # 进行您的更改
   # 确保遵循代码规范
   # 添加适当的测试
   ```

4. **提交更改**
   ```bash
   git add .
   git commit -m "添加功能：描述您的功能"
   git push origin feature/your-feature-name
   ```

5. **创建Pull Request**
   - 在GitHub上创建Pull Request
   - 提供清晰的描述和更改理由
   - 链接相关的Issue（如果有）

### 贡献者公约

- 尊重所有贡献者
- 建设性讨论，避免人身攻击
- 遵循项目代码规范
- 为您的代码添加适当的测试
- 更新相关文档

## 📄 许可证

本项目采用MIT许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

```
MIT License

Copyright (c) 2026 CN_Language项目团队

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
```

## 📞 联系方式

- **项目主页**：[GitHub仓库](https://github.com/ChenChao_GitCode/CN_Language)
- **问题反馈**：[GitHub Issues](https://github.com/ChenChao_GitCode/CN_Language/issues)
- **讨论区**：[GitHub Discussions](https://github.com/ChenChao_GitCode/CN_Language/discussions)

## 🙏 致谢

感谢所有为CN_Language项目做出贡献的开发者！

特别感谢：
- 项目创始人和架构师
- 所有代码贡献者
- 测试和文档贡献者
- 提供反馈和建议的用户

---

**CN_Language - 让中文编程更简单！**

*最后更新：2026年1月9日*
*版本：1.0.0*
