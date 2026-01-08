# CN_Language项目 构建方法和编译流程

## 文档信息

- **文档版本**: 2.0.0
- **创建日期**: 2026-01-08
- **最后更新**: 2026-01-08
- **作者**: CN_Language架构委员会
- **状态**: 正式发布
- **适用版本**: CN_Language 1.0.0+

## 1. 概述

本文档详细描述了CN_Language项目的构建系统、编译流程和测试框架。项目采用分层架构设计，遵循SOLID原则，构建系统支持跨平台开发（Windows/Linux）。

### 1.1 设计目标

1. **模块化构建**: 支持按模块编译和测试
2. **跨平台支持**: 支持Windows和Linux系统
3. **分层架构**: 遵循基础设施层→核心层→应用层的依赖关系
4. **自动化测试**: 集成模块化测试框架
5. **清晰的输出目录**: 构建产物有明确的存放位置

## 2. 构建系统架构

### 2.1 构建工具链

| 工具 | 用途 | 版本要求 |
|------|------|----------|
| GCC/Clang | C编译器 | GCC 9.0+ 或 Clang 10.0+ |
| Make | 构建自动化工具 | GNU Make 4.0+ |
| PowerShell | Windows构建脚本 | PowerShell 5.1+ |
| Bash | Linux构建脚本 | Bash 4.0+ |

### 2.2 构建目录结构

```
CN_Language/
├── src/                    # 源代码目录
│   ├── infrastructure/     # 基础设施层
│   ├── core/              # 核心层
│   └── application/       # 应用层
├── tests/                 # 测试代码目录
│   ├── core/             # 核心层测试
│   ├── infrastructure/   # 基础设施层测试
│   └── unit/             # 单元测试
├── build/                 # 构建输出目录
│   ├── src/              # 源代码编译产物
│   │   ├── infrastructure/
│   │   ├── core/
│   │   └── application/
│   └── tests/            # 测试编译产物
│       ├── core/
│       ├── infrastructure/
│       └── unit/
├── scripts/              # 构建脚本目录
│   ├── tests/           # 测试相关脚本
│   ├── compile_windows.ps1
│   └── compile_linux.sh
├── bin/                  # 最终可执行文件（项目成果）
└── docs/                 # 文档目录
```

## 3. 编译流程

### 3.1 编译阶段

CN_Language项目采用三阶段编译流程：

1. **预处理阶段**: 处理头文件包含、宏展开
2. **编译阶段**: 将C源代码编译为目标文件(.o/.obj)
3. **链接阶段**: 将目标文件链接为可执行文件

### 3.2 编译选项

#### 3.2.1 通用编译选项

```makefile
# 编译器标志
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic
CFLAGS += -I./src -I./src/infrastructure -I./src/core -I./src/application

# 优化选项
OPTIMIZE_DEBUG = -O0 -g3 -DDEBUG
OPTIMIZE_RELEASE = -O2 -DNDEBUG

# 平台特定选项
ifeq ($(OS),Windows_NT)
    CFLAGS += -D_WIN32
else
    CFLAGS += -D_LINUX
endif
```

#### 3.2.2 分层编译选项

```makefile
# 基础设施层编译选项
INFRA_CFLAGS = $(CFLAGS) -I./src/infrastructure

# 核心层编译选项
CORE_CFLAGS = $(CFLAGS) -I./src/core -I./src/infrastructure

# 应用层编译选项
APP_CFLAGS = $(CFLAGS) -I./src/application -I./src/core -I./src/infrastructure
```

### 3.3 依赖管理

#### 3.3.1 头文件包含顺序

遵循以下顺序以防止循环依赖：

1. 对应源文件的头文件（如有）
2. C标准库头文件
3. 第三方库头文件
4. 项目其他模块头文件（按依赖顺序）

#### 3.3.2 依赖方向规则

- 应用层可以依赖核心层和基础设施层
- 核心层可以依赖基础设施层，不能依赖应用层
- 基础设施层只能依赖C标准库和操作系统API

## 4. 测试构建和运行

### 4.1 测试目录结构

测试代码按照与源代码相同的目录结构组织：

```
tests/
├── core/                 # 核心层测试
│   ├── lexer/           # 词法分析器测试
│   ├── parser/          # 语法分析器测试
│   ├── ast/             # 抽象语法树测试
│   ├── semantic/        # 语义分析测试
│   ├── codegen/         # 代码生成测试
│   └── token/           # 令牌模块测试
├── infrastructure/       # 基础设施层测试
│   ├── memory/          # 内存管理测试
│   ├── containers/      # 容器测试
│   ├── utils/           # 工具函数测试
│   └── platform/        # 平台抽象测试
└── unit/                # 单元测试（按功能划分）
```

### 4.2 测试文件命名规范

- 测试源文件: `test_<模块名>_<功能>.c`
- 测试运行器: `test_<模块名>_runner.c`
- 测试头文件: `test_<模块名>.h`

### 4.3 模块化测试框架

#### 4.3.1 总测试运行器

项目使用统一的总测试运行器（`tests/test_runner_main.c`），支持：

1. **模块选择**: 通过命令行参数选择要测试的模块
2. **批量运行**: 运行所有启用的测试模块
3. **结果统计**: 收集和显示测试统计信息
4. **时间测量**: 测量每个模块的运行时间

#### 4.3.2 测试模块注册

每个测试模块需要提供以下接口：

```c
// 测试模块接口
typedef struct {
    const char* module_name;      // 模块名称
    const char* module_path;      // 模块路径
    bool (*run_tests)(void);      // 运行测试的函数指针
    bool enabled;                 // 是否启用
} TestModule;
```

#### 4.3.3 测试构建流程

1. **编译测试源文件**: 将测试代码编译为目标文件
2. **编译被测试代码**: 将被测试的模块代码编译为目标文件
3. **链接测试可执行文件**: 将测试目标文件链接为可执行文件
4. **运行测试**: 执行测试并收集结果

### 4.4 测试构建配置

#### 4.4.1 测试编译选项

```makefile
# 测试专用编译选项
TEST_CFLAGS = $(CFLAGS) -DTEST_BUILD -I./tests

# 测试链接选项
TEST_LDFLAGS = -lm

# 测试构建目录
TEST_BUILD_DIR = ./build/tests
TEST_BIN_DIR = $(TEST_BUILD_DIR)/bin
```

#### 4.4.2 测试目标文件组织

测试期间生成的目标文件按照以下结构组织：

```
build/tests/
├── core/                 # 核心层测试目标文件
│   ├── lexer/
│   ├── parser/
│   └── token/
├── infrastructure/       # 基础设施层测试目标文件
│   ├── memory/
│   ├── containers/
│   └── utils/
└── unit/                # 单元测试目标文件
```

源代码编译的目标文件组织在：

```
build/src/
├── infrastructure/       # 基础设施层目标文件
├── core/                # 核心层目标文件
└── application/         # 应用层目标文件
```

### 4.5 测试运行脚本

生成的测试可执行文件存放在 `scripts/tests/` 目录中：

```
scripts/tests/
├── test_runner.exe      # Windows测试运行器
├── test_runner          # Linux测试运行器
├── run_all_tests.ps1    # Windows测试运行脚本
└── run_all_tests.sh     # Linux测试运行脚本
```

## 5. 平台特定的构建脚本

### 5.1 Windows构建脚本（PowerShell）

#### 5.1.1 脚本位置
- `scripts/compile_windows.ps1` - 主构建脚本
- `scripts/tests/run_all_tests.ps1` - 测试运行脚本

#### 5.1.2 主要功能
```powershell
# 检测编译器
$compiler = Find-Compiler

# 创建构建目录
New-BuildDirectories

# 编译各层模块
Compile-InfrastructureLayer
Compile-CoreLayer
Compile-ApplicationLayer

# 链接可执行文件
Link-Executables

# 运行测试
if ($RunTests) {
    Run-AllTests
}
```

#### 5.1.3 使用示例
```powershell
# 构建所有模块
.\scripts\compile_windows.ps1

# 构建并运行测试
.\scripts\compile_windows.ps1 -RunTests

# 仅构建特定层
.\scripts\compile_windows.ps1 -Layer "infrastructure"
```

### 5.2 Linux构建脚本（Bash）

#### 5.2.1 脚本位置
- `scripts/compile_linux.sh` - 主构建脚本
- `scripts/tests/run_all_tests.sh` - 测试运行脚本

#### 5.2.2 主要功能
```bash
#!/bin/bash

# 检测编译器和环境
check_environment() {
    # 检查GCC/Clang
    # 检查Make版本
    # 检查必要工具
}

# 编译函数
compile_layer() {
    local layer=$1
    # 编译指定层的所有模块
}

# 主构建流程
main() {
    check_environment
    create_build_directories
    compile_all_layers
    link_executables
    
    if [ "$RUN_TESTS" = "true" ]; then
        run_tests
    fi
}
```

#### 5.2.3 使用示例
```bash
# 构建所有模块
./scripts/compile_linux.sh

# 构建并运行测试
./scripts/compile_linux.sh --run-tests

# 调试构建
./scripts/compile_linux.sh --debug
```

## 6. Makefile构建系统

### 6.1 主Makefile结构

项目使用Makefile作为主要的构建系统，支持：

```makefile
# 顶层Makefile结构
include config.mk          # 构建配置
include rules.mk           # 构建规则
include targets.mk         # 构建目标

# 分层构建目标
.PHONY: all infra core app test clean

all: infra core app

infra:
    $(MAKE) -C src/infrastructure

core: infra
    $(MAKE) -C src/core

app: core
    $(MAKE) -C src/application

test: all
    $(MAKE) -C tests
```

### 6.2 测试Makefile

测试目录中的Makefile专门用于测试构建：

```makefile
# tests/Makefile 关键部分

# 检测操作系统
ifeq ($(OS),Windows_NT)
    # Windows特定设置
    EXE_EXT := .exe
    PATH_SEP := \\
else
    # Linux/Unix系统
    EXE_EXT :=
    PATH_SEP := /
endif

# 测试构建目标
test_runner: $(TEST_OBJS) $(SRC_OBJS)
    $(CC) $(LDFLAGS) $^ -o $(TEST_BIN_DIR)/test_runner$(EXE_EXT)
```

### 6.3 模块化构建支持

每个模块可以有自己的Makefile，通过顶层Makefile统一调用：

```makefile
# 模块级Makefile示例（src/infrastructure/memory/Makefile）

MODULE_NAME := memory
MODULE_SRCS := $(wildcard *.c) $(wildcard */*.c)
MODULE_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(MODULE_SRCS))

$(BUILD_DIR)/%.o: %.c
    $(CC) $(CFLAGS) -c $< -o $@

.PHONY: build clean

build: $(MODULE_OBJS)

clean:
    rm -f $(MODULE_OBJS)
```

## 7. 构建配置管理

### 7.1 构建配置选项

#### 7.1.1 构建类型
- **Debug**: 包含调试信息，启用所有检查
- **Release**: 优化性能，禁用调试信息
- **Profile**: 包含性能分析支持

#### 7.1.2 配置示例
```makefile
# config.mk - 构建配置文件

# 构建类型选择
BUILD_TYPE ?= Debug

# 根据构建类型设置选项
ifeq ($(BUILD_TYPE),Debug)
    CFLAGS += -O0 -g3 -DDEBUG -fsanitize=address
    LDFLAGS += -fsanitize=address
else ifeq ($(BUILD_TYPE),Release)
    CFLAGS += -O2 -DNDEBUG
    LDFLAGS += -s
else ifeq ($(BUILD_TYPE),Profile)
    CFLAGS += -O2 -g -pg
    LDFLAGS += -pg
endif
```

### 7.2 环境变量配置

支持通过环境变量自定义构建：

```bash
# 设置构建类型
export BUILD_TYPE=Release

# 设置编译器
export CC=clang

# 设置安装前缀
export PREFIX=/usr/local
```

## 8. 构建工作流程

### 8.1 完整构建流程

1. **环境准备**
   - 检查编译器和工具链
   - 创建构建目录结构
   - 加载构建配置

2. **分层编译**
   - 编译基础设施层（无外部依赖）
   - 编译核心层（依赖基础设施层）
   - 编译应用层（依赖核心层和基础设施层）

3. **测试构建**
   - 编译测试代码
   - 链接测试可执行文件
   - 将测试程序复制到scripts/tests/

4. **最终链接**
   - 链接项目可执行文件（编译器/解释器）
   - 将最终产物复制到bin/目录

5. **验证和测试**
   - 运行单元测试
   - 运行集成测试
   - 运行端到端测试

### 8.2 增量构建

构建系统支持增量构建，只重新编译修改过的文件：

```makefile
# 自动依赖生成
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d

$(BUILD_DIR)/%.o: %.c
    $(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# 包含自动生成的依赖文件
-include $(wildcard $(BUILD_DIR)/*.d)
```

### 8.3 并行构建

支持并行构建以加快编译速度：

```bash
# 使用多核并行构建
make -j$(nproc)

# 或指定核心数
make -j4
```

## 9. 测试执行和报告

### 9.1 测试执行方式

#### 9.1.1 命令行测试
```bash
# 运行所有测试
./scripts/tests/test_runner --all

# 运行特定模块测试
./scripts/tests/test_runner lexer parser

# 列出所有可用测试模块
./scripts/tests/test_runner --list
```

#### 9.1.2 脚本测试
```bash
# 使用测试脚本
./scripts/tests/run_all_tests.sh

# 带参数的测试
./scripts/tests/run_all_tests.sh --verbose --coverage
```

### 9.2 测试报告格式

测试运行器生成标准化的测试报告：

```
========================================
    CN_Language项目 - 模块化测试框架
========================================

开始运行测试...
================

测试模块: lexer
路径: core/lexer
状态: ✅ 通过 (0.125 秒)

测试模块: token
路径: core/token
状态: ✅ 通过 (0.087 秒)

测试完成！
==========

测试统计:
  ├─ 总模块数:      15
  ├─ 测试模块数:    8
  ├─ 模块通过:      8
  ├─ 模块失败:      0
  ├─ 总测试数:      142
  ├─ 测试通过:      142
  ├─ 测试失败:      0
  ├─ 通过率:        100.0%
  └─ 总运行时间:    1.342 秒

✅ 所有测试通过！
```

### 9.3 测试覆盖率

支持测试覆盖率分析：

```bash
# 生成覆盖率报告
make coverage

# 查看HTML格式的覆盖率报告
open build/coverage/index.html
```

## 10. 故障排除

### 10.1 常见构建问题

#### 10.1.1 编译器找不到
```bash
# 解决方案：安装GCC或Clang
# Ubuntu/Debian
sudo apt-get install gcc build-essential

# CentOS/RHEL/Fedora
sudo yum install gcc make

# macOS (使用Homebrew)
brew install gcc
```

#### 10.1.2 Make版本不兼容
```bash
# 检查Make版本
make --version

# 升级Make (Ubuntu/Debian)
sudo apt-get install make

# 从源码编译最新版本
wget https://ftp.gnu.org/gnu/make/make-4.4.tar.gz
tar -xzf make-4.4.tar.gz
cd make-4.4
./configure
make
sudo make install
```

#### 10.1.3 头文件找不到
```bash
# 检查头文件路径
echo $C_INCLUDE_PATH
echo $CPLUS_INCLUDE_PATH

# 添加头文件路径
export C_INCLUDE_PATH=/usr/local/include:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=/usr/local/include:$CPLUS_INCLUDE_PATH

# 或者在Makefile中添加
CFLAGS += -I/path/to/headers
```

#### 10.1.4 库文件找不到
```bash
# 检查库文件路径
echo $LD_LIBRARY_PATH
echo $LIBRARY_PATH

# 添加库文件路径
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
export LIBRARY_PATH=/usr/local/lib:$LIBRARY_PATH

# 或者在Makefile中添加
LDFLAGS += -L/path/to/libs -l库名
```

### 10.2 测试相关问题

#### 10.2.1 测试编译失败
```bash
# 检查测试依赖
make test-deps

# 清理并重新构建
make clean
make test

# 启用详细输出查看错误
make test V=1
```

#### 10.2.2 测试运行失败
```bash
# 运行单个测试模块调试
./scripts/tests/test_runner --模块名

# 启用调试输出
export CN_TEST_DEBUG=1
./scripts/tests/test_runner --all

# 检查测试日志
cat build/tests/test.log
```

#### 10.2.3 内存泄漏检测
```bash
# 使用AddressSanitizer
make clean
make test BUILD_TYPE=Debug

# 运行测试
./scripts/tests/test_runner --all

# 查看泄漏报告
export ASAN_OPTIONS=detect_leaks=1
```

### 10.3 平台特定问题

#### 10.3.1 Windows PowerShell执行策略
```powershell
# 检查执行策略
Get-ExecutionPolicy

# 设置执行策略（需要管理员权限）
Set-ExecutionPolicy RemoteSigned -Scope CurrentUser

# 或者为单个脚本临时允许
powershell -ExecutionPolicy Bypass -File .\scripts\compile_windows.ps1
```

#### 10.3.2 Linux权限问题
```bash
# 确保脚本有执行权限
chmod +x scripts/compile_linux.sh
chmod +x scripts/tests/run_all_tests.sh

# 确保构建目录可写
chmod -R 755 build/
```

#### 10.3.3 跨平台路径问题
```makefile
# 在Makefile中使用正确的路径分隔符
ifeq ($(OS),Windows_NT)
    PATH_SEP := \\
else
    PATH_SEP := /
endif

# 使用变量代替硬编码路径
TARGET := $(BUILD_DIR)$(PATH_SEP)program$(EXE_EXT)
```

## 11. 附录

### 11.1 构建命令参考

#### 11.1.1 常用构建命令
```bash
# 完整构建（包括测试）
make all

# 仅构建源代码
make build

# 仅构建测试
make test-build

# 运行所有测试
make test-run

# 清理构建文件
make clean

# 深度清理（包括下载的依赖）
make distclean
```

#### 11.1.2 开发工作流命令
```bash
# 开发模式构建（快速迭代）
make dev

# 调试构建（包含符号信息）
make debug

# 发布构建（优化）
make release

# 性能分析构建
make profile
```

#### 11.1.3 测试相关命令
```bash
# 运行特定模块测试
make test-module MODULE=lexer

# 运行单元测试
make unit-test

# 运行集成测试
make integration-test

# 生成测试覆盖率报告
make coverage

# 运行性能基准测试
make benchmark
```

### 11.2 环境变量参考

#### 11.2.1 构建环境变量
```bash
# 构建类型
export BUILD_TYPE=Debug    # Debug, Release, Profile

# 编译器选择
export CC=gcc              # gcc, clang
export CXX=g++             # g++, clang++

# 构建目录
export BUILD_DIR=./build
export INSTALL_DIR=/usr/local

# 优化选项
export OPTIMIZE_LEVEL=2    # 0-3
export DEBUG_SYMBOLS=1     # 0或1
```

#### 11.2.2 测试环境变量
```bash
# 测试输出控制
export CN_TEST_VERBOSE=1      # 详细输出
export CN_TEST_QUIET=0        # 安静模式
export CN_TEST_COLOR=1        # 彩色输出

# 测试过滤
export CN_TEST_FILTER="lexer|parser"  # 正则表达式过滤
export CN_TEST_EXCLUDE="integration"  # 排除特定测试

# 测试超时
export CN_TEST_TIMEOUT=30     # 单个测试超时时间（秒）
```

### 11.3 目录结构参考

#### 11.3.1 源代码目录
```
src/
├── infrastructure/          # 基础设施层
│   ├── memory/             # 内存管理
│   ├── containers/         # 容器
│   ├── utils/              # 工具函数
│   └── platform/           # 平台抽象
├── core/                   # 核心层
│   ├── lexer/              # 词法分析
│   ├── parser/             # 语法分析
│   ├── ast/                # 抽象语法树
│   ├── semantic/           # 语义分析
│   ├── codegen/            # 代码生成
│   └── runtime/            # 运行时
└── application/            # 应用层
    ├── cli/                # 命令行界面
    ├── repl/               # 交互式环境
    ├── debugger/           # 调试器
    └── ide-plugin/         # IDE插件
```

#### 11.3.2 测试目录
```
tests/
├── core/                   # 核心层测试
│   ├── lexer/              # 词法分析测试
│   ├── parser/             # 语法分析测试
│   ├── ast/                # AST测试
│   ├── semantic/           # 语义分析测试
│   ├── codegen/            # 代码生成测试
│   └── runtime/            # 运行时测试
├── infrastructure/         # 基础设施层测试
│   ├── memory/             # 内存管理测试
│   ├── containers/         # 容器测试
│   ├── utils/              # 工具函数测试
│   └── platform/           # 平台抽象测试
└── unit/                   # 单元测试
    ├── basic/              # 基础功能测试
    ├── integration/        # 集成测试
    └── performance/        # 性能测试
```

#### 11.3.3 构建输出目录
```
build/
├── src/                    # 源代码编译产物
│   ├── infrastructure/     # 基础设施层目标文件
│   ├── core/              # 核心层目标文件
│   └── application/       # 应用层目标文件
├── tests/                  # 测试编译产物
│   ├── core/              # 核心层测试目标文件
│   ├── infrastructure/    # 基础设施层测试目标文件
│   └── unit/              # 单元测试目标文件
├── bin/                    # 可执行文件（临时）
│   ├── test_runner        # 测试运行器
│   └── cn_language        # 主程序
├── lib/                    # 库文件
│   ├── static/            # 静态库
│   └── shared/            # 共享库
└── coverage/              # 测试覆盖率报告
    ├── html/              # HTML报告
    ├── xml/               # XML报告
    └── lcov/              # LCOV数据
```

### 11.4 相关文档

1. **架构设计文档**: `docs/architecture/001-中文编程语言CN_Language开发规划.md`
2. **技术规范文档**: `docs/specifications/CN_Language项目 技术规范和编码标准.md`
3. **目录结构规范**: `docs/specifications/CN_Language项目 目录结构规范.md`
4. **语法规范文档**: `docs/specifications/CN_Language 语法规范.md`
5. **API文档**: `docs/api/`（使用Doxygen生成）

## 12. 版本历史

### 12.1 版本 2.0.0 (2026-01-08)
- **新增**: 完整的构建方法和编译流程文档
- **新增**: 模块化测试框架详细说明
- **新增**: 跨平台构建脚本规范
- **新增**: 故障排除指南
- **更新**: 基于现有项目结构更新目录规范
- **更新**: 完善构建配置选项

### 12.1 版本 1.0.0 (2025-12-15)
- **初始版本**: 创建基本构建框架
- **基础功能**: 支持基本编译和测试
- **平台支持**: Windows和Linux基础支持

## 13. 维护指南

### 13.1 文档维护
- 每次架构变更时更新本文档
- 保持目录结构与实际项目同步
- 记录所有构建系统的重大变更

### 13.2 构建系统维护
- 定期检查构建脚本的兼容性
- 更新编译器版本要求
- 维护跨平台支持

### 13.3 测试框架维护
- 确保测试框架与源代码同步更新
- 维护测试覆盖率要求
- 定期更新测试依赖

---

**文档结束**

*最后更新: 2026-01-08*
*维护者: CN_Language架构委员会*
*状态: 正式发布*
