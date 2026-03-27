# CN_Language

<div align="center">

**中文编程语言 —— 让编程回归中文思维**

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/yourusername/CN_Language/releases)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](#)
[![Tests](https://img.shields.io/badge/tests-106%2F106-success.svg)](#测试覆盖)

[快速开始](#快速开始) • [特性](#核心特性) • [示例](#代码示例) • [文档](#文档) • [贡献](#贡献)

</div>

---

## 📖 简介

CN_Language 是一个**完全使用中文语法**的编程语言，旨在降低中文母语者的编程门槛，让编程更加贴近自然语言思维。本项目提供完整的编译器工具链，包括词法分析、语法分析、语义检查、代码生成和运行时库。

### 🎯 设计目标

- **中文优先**：关键字、函数名、变量名全部使用中文
- **易学易用**：贴近自然语言的语法结构
- **工业级**：完整的编译器和工具链支持
- **系统编程**：支持操作系统内核开发
- **高性能**：通过 C 后端生成高效的机器码

---

## ✨ 核心特性

### 语言特性

- ✅ **基础类型**：整数、小数、布尔、字符串
- ✅ **复合类型**：数组、结构体、枚举
- ✅ **指针支持**：包括函数指针
- ✅ **控制流**：如果/否则、当、循环、选择
- ✅ **函数系统**：参数传递、返回值、递归调用
- ✅ **模块系统**：模块声明、导入、可见性控制
- ✅ **预处理器**：宏定义、条件编译、文件包含

### 编译器工具链

| 工具 | 功能 | 状态 |
|------|------|------|
| **cnc** | CN_Language 编译器 | ✅ 可用 |
| **cnfmt** | 代码格式化工具 | ✅ 可用 |
| **cncheck** | 静态代码检查 | ✅ 可用 |
| **cnlsp** | 语言服务器（IDE支持） | ✅ 可用 |
| **cnrepl** | 交互式解释器 | ✅ 可用 |
| **cnperf** | 性能分析工具 | ✅ 可用 |

### 运行时库

- ✅ **内存管理**：Arena 分配器、安全内存操作
- ✅ **I/O 支持**：控制台输入输出、文件操作
- ✅ **数学库**：基础数学函数
- ✅ **字符串库**：字符串操作函数
- ✅ **并发支持**：锁、条件变量、原子操作

### 操作系统开发

- ✅ **Freestanding 模式**：无标准库依赖
- ✅ **内存映射**：底层内存操作
- ✅ **内联汇编**：直接嵌入汇编代码
- ✅ **中断处理**：通过运行时库 API

---

## 🚀 快速开始

### 安装

#### 从源码构建

```bash
# 克隆仓库
git clone https://github.com/yourusername/CN_Language.git
cd CN_Language

# 配置构建
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build build --config Release

# 运行测试
cd build
ctest -C Release
```

#### 构建产物

构建完成后，可执行文件位于：
- Windows: `build/src/Release/cnc.exe`
- Linux/macOS: `build/src/cnc`

### Hello World

创建 `hello.cn` 文件：

```cn
函数 主程序()
{
    打印("你好，世界！\n");
    返回 0;
}
```

编译并运行：

```bash
cnc hello.cn -o hello
./hello
```

输出：
```
你好，世界！
```

---

## 📝 代码示例

### 基础语法

```cn
// 布尔类型测试示例

函数 主程序() {
    // 布尔变量声明和赋值
    布尔 真值 = 真;
    布尔 假值 = 假;
    
    // 比较运算返回布尔值
    整数 x = 10;
    整数 y = 20;
    布尔 比较结果 = x < y;  // 真
    
    // 逻辑运算
    布尔 逻辑与 = 真值 && 假值;  // 假
    布尔 逻辑或 = 真值 || 假值;  // 真
    
    // 布尔条件判断
    如果 (真值) {
        打印("这是真的");
    }
    
    如果 (x < y) {
        打印("x 小于 y");
    }
    
    // while 循环使用布尔条件
    布尔 应继续 = 真;
    整数 计数 = 0;
    当 (计数 < 5) {
        计数 = 计数 + 1;
    }
    
    // for 循环使用布尔条件
    循环 (整数 i = 0; i < 10; i = i + 1) {
        打印("循环");
    }
    
    // 布尔参数和返回值
    布尔 结果 = 检查值(x);
}

函数 检查值(整数 值) {
    返回 值 > 0;  // 返回布尔值
}

// 结构体中的布尔字段
结构体 配置 {
    布尔 启用;
    整数 值;
}

函数 使用配置() {
    变量 我的配置 = 配置{启用:真, 值:100};
        
    如果 (我的配置.启用) {
        打印("配置已启用");
    }
}

// 布尔指针
函数 布尔指针测试() {
    布尔 标志 = 真;
    布尔* 标志指针 = &标志;
    
    如果 (*标志指针) {
        打印("通过指针访问");
    }
}

```

### 函数定义

```cn
// CN语言函数示例

// 基本函数定义
函数 计算加法(整数 a, 整数 b) {
    返回 a + b;
}

// 带返回值类型的函数
函数 计算加法2(整数 a, 整数 b) -> 整数
{
    返回 a + b;
}

// 带返回值的函数
函数 计算乘法(整数 x, 整数 y) -> 整数
{
    变量 结果 = x * y;
    返回 结果;
}

// 无参数函数
函数 获取常量() -> 整数
{
    返回 42;
}

// 主程序入口
函数 主程序() {
    变量 和 = 计算加法(3, 5);
    变量 积 = 计算乘法(4, 6);
    变量 var_常量 = 获取常量();
    打印(和);
    打印(积);
    打印(var_常量);
    返回 0;
}
```

### 结构体

```cn
结构体 点 {
    整数 x;
    整数 y;
}

函数 主程序() {
    变量 p1 = 点 { x: 10, y: 20 };

    结构体 点 p2 = {10, 20};
    结构体 点 p3 = {x = 30, y = 40};
    结构体 点 p4 = {x: 50, y: 60};

    变量 px = p1.x;
    返回 0;
}

```

### 模块系统

```cn
/**
 * CN语言模块系统完整功能展示
 * 
 * 功能演示:
 * 1. 多层路径导入
 * 2. 选择性导入
 * 3. 通配符导入
 * 4. 包导入(自动查找__包__.cn)
 * 5. 嵌套导入(包重新导出子模块)
 * 6. 命名空间导入
 * 7. 模块缓存(多次导入同一模块只编译一次)
 */

// 功能1+2: 多层路径 + 选择性导入
//从 ./工具/数学/数学 导入 { 加法, 减法, 乘法, 除法 };
从 ./工具/数学/数学 导入 { 加法, 减法, 乘法, 除法 };

// 功能3: 通配符导入
从 ./simple_module 导入 *;

// 功能4+5: 包导入 + 嵌套导入
// __包__.cn会从./数学/数学导入*,然后我们可以直接使用
从 ./工具 导入 { 获取版本 };

// 功能6: 命名空间导入
导入 module_using_simple;

函数 主程序() -> 整数 {
    打印("=== CN语言模块系统功能展示 ===\n\n");
    
    // 使用数学函数
    打印("1. 数学运算:\n");
    整数 和 = 加法(15, 25);
    整数 差 = 减法(50, 20);
    整数 积 = 乘法(6, 7);
    整数 商 = 除法(100, 4);
    
    打印("  15 + 25 = ");
    打印(和);
    打印("\n");
    
    打印("  50 - 20 = ");
    打印(差);
    打印("\n");
    
    打印("  6 × 7 = ");
    打印(积);
    打印("\n");
    
    打印("  100 ÷ 4 = ");
    打印(商);
    打印("\n\n");
    
    // 使用通配符导入的函数
    打印("2. 通配符导入:\n");
    整数 测试值 = 测试函数();
    打印("  测试函数返回: ");
    打印(测试值);
    打印("\n\n");
    
    // 使用包导入的函数
    打印("3. 包导入:\n");
    整数 版本 = 获取版本();
    打印("  工具包版本: ");
    打印(版本);
    打印("\n\n");
    
    // 使用命名空间导入
    打印("4. 命名空间导入:\n");
    整数 使用值 = module_using_simple.使用测试();
    打印("  模块函数返回: ");
    打印(使用值);
    打印("\n\n");
    
    打印("=== 所有功能测试通过! ===\n");
    
    返回 0;
}

```

### 更多示例

查看 [examples/](examples/) 目录获取 140+ 个完整示例，包括：
- 基础语法示例
- 系统编程（内存管理、并发）
- 操作系统内核开发
- 完整的测试用例

---

## 🧪 测试覆盖

CN_Language 1.0.0 版本包含完整的测试套件：

| 测试类型 | 数量 | 通过率 |
|---------|------|--------|
| 单元测试 | 41 | 100% |
| 集成测试 | 17 | 100% |
| 系统测试 | 48 | 100% |
| **总计** | **106** | **100%** |

运行测试：

```bash
cd build
ctest -C Release --output-on-failure
```

---

## 📚 文档

### 规范文档

- [语言规范](docs/specifications/CN_Language%20语言规范草案（核心子集）.md) - 完整的语法和语义规范
- [版本号规范](docs/specifications/CN_Language%20版本号规范.md) - 语义化版本控制
- [测试规范](docs/specifications/CN_Language%20测试规范.md) - 测试编写和组织规范
- [发布流程](docs/specifications/CN_Language%20发布流程与工件管理规范.md) - 发布管理规范
- [C 代码风格](docs/specifications/CN_Language%20C%20代码风格规范.md) - 编译器代码规范
- [CN 代码风格](docs/specifications/CN_Language%20CN代码风格规范.md) - CN 语言代码规范

### 设计文档

- [编译器架构](docs/design/CN_Language%20编译器%20工具链架构设计.md) - 编译器整体架构
- [开发计划](docs/design/CN_Language%20开发计划.md) - 分阶段开发规划
- [语法标准](docs/design/CN_Language%20语法标准.md) - 语法设计标准
- [LSP 架构](docs/design/CN_Language%20LSP%20语言服务架构设计.md) - 语言服务器设计

### 用户指南

- [模块系统使用指南](docs/user-guide/CN语言模块系统使用指南.md) - 模块系统详细说明

### API 文档

- [运行时 API](docs/api/) - 运行时库 API 参考
- [工具链 API](docs/api/tools/) - 工具链接口文档

---

## 🛠️ 编译器使用

### 基本用法

```bash
# 编译并生成可执行文件
cnc 源文件.cn -o 输出文件

# 仅进行语法检查
cnc 源文件.cn

# 生成 C 代码（不编译）
cnc 源文件.cn -c -o 输出.c

# 保留中间 C 代码
cnc 源文件.cn --emit-c -o 输出
```

### 高级选项

```bash
# 指定模块搜索路径
cnc main.cn -I./lib -o app

# 编译整个项目目录
cnc --project ./src -o app

# 启用优化
cnc 源文件.cn -O2 -o 输出

# Freestanding 模式（OS 开发）
cnc kernel.cn --freestanding -o kernel

# 性能分析
cnc 源文件.cn --perf --perf-output=perf.json

# 内存分析
cnc 源文件.cn --mem-profile --mem-output=mem.csv
```

---

## 🏗️ 项目结构

```
CN_Language/
├── src/                    # 编译器源码
│   ├── frontend/          # 前端（词法、语法、AST）
│   ├── semantics/         # 语义分析
│   ├── ir/                # 中间表示
│   ├── backend/           # 代码生成
│   ├── runtime/           # 运行时库
│   └── cli/               # 命令行工具
├── include/               # 头文件
├── tests/                 # 测试套件
│   ├── unit/             # 单元测试
│   ├── integration/      # 集成测试
│   └── system/           # 系统测试
├── examples/              # 示例代码（140+ 个）
├── docs/                  # 文档
│   ├── specifications/   # 规范文档
│   ├── design/           # 设计文档
│   └── api/              # API 文档
└── tools/                 # 辅助工具脚本
```

---

## 🎓 学习路径

### 初学者

1. [Hello World](examples/basic/hello_world.cn) - 第一个程序
2. [函数示例](examples/syntax/functions/function_examples.cn) - 函数定义和调用
3. [控制流](examples/syntax/control-flow/) - 条件和循环
4. [数组](examples/syntax/arrays/array_examples.cn) - 数组操作

### 进阶

5. [指针](examples/syntax/pointers/pointer_example.cn) - 指针和内存
6. [结构体](examples/syntax/structs/struct_complete_example.cn) - 复合类型
7. [模块系统](examples/module-system/完整功能展示.cn) - 代码组织

### 高级

8. [内存管理](examples/system/memory/memory_manager_demo.cn) - 自定义分配器
9. [并发编程](examples/system/concurrency/task_scheduler_demo.cn) - 多任务
10. [OS 开发](examples/os-kernel/os_kernel_demo.cn) - 内核开发

---

## 🤝 贡献

我们欢迎所有形式的贡献！

### 如何贡献

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 提交 Pull Request

### 贡献指南

- 遵循 [C 代码风格规范](docs/specifications/CN_Language%20C%20代码风格规范.md)
- 为新功能添加测试用例
- 更新相关文档
- 确保所有测试通过

---

## 📋 开发状态

### 已完成功能（v1.0.0）

- ✅ 完整的前端（词法、语法、AST）
- ✅ 语义分析（类型检查、作用域）
- ✅ IR 生成与优化
- ✅ C 代码后端
- ✅ 完整的运行时库
- ✅ 模块系统
- ✅ 预处理器
- ✅ Freestanding 模式
- ✅ 完整的工具链

### 开发中

- 🚧 泛型系统
- 🚧 类和接口
- 🚧 异常处理
- 🚧 包管理器
- 🚧 IDE 插件

---

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

---

## 🙏 致谢

感谢所有为 CN_Language 项目做出贡献的开发者！

---

## 📞 联系方式

- 项目主页：[https://github.com/ChenChao-HuaiAn/CN_Language](https://github.com/ChenChao-HuaiAn/CN_Language)
- 问题反馈：[GitHub Issues](https://github.com/ChenChao-HuaiAn/CN_Language/issues)
- 讨论区：[GitHub Discussions](https://github.com/ChenChao-HuaiAn/CN_Language/discussions)

---

<div align="center">

**让中文编程更简单，让编程更贴近思维**

Made with ❤️ by CN_Language Team

[⬆ 回到顶部](#cn_language)

</div>
