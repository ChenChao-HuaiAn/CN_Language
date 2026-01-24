# CN_Language 变更日志 (Changelog)

本文档记录 CN_Language 编译器及工具链的所有重要变更。

## 变更日志书写规范

### 格式约定

每个版本的变更记录遵循以下格式：

```markdown
## [版本号] - YYYY-MM-DD

### Added（新增）
- 新增功能描述

### Changed（变更）
- 功能变更或改进描述

### Deprecated（废弃）
- 即将移除的功能警告

### Removed（移除）
- 已移除的功能

### Fixed（修复）
- Bug 修复描述

### Security（安全）
- 安全问题修复

### Performance（性能）
- 性能优化描述
```

### 分类说明

| 分类 | 用途 | 示例 |
|------|------|------|
| **Added** | 新增功能、新增 API、新增工具 | 新增 `cnfmt` 代码格式化工具 |
| **Changed** | 功能改进、行为变更（向后兼容） | 优化词法分析器性能，提升 20% |
| **Deprecated** | 标记为废弃，但尚未移除 | 废弃 `--legacy-mode` 选项，将在 2.0 移除 |
| **Removed** | 完全移除的功能（不兼容变更） | 移除已废弃的 `--old-syntax` 选项 |
| **Fixed** | Bug 修复 | 修复解析器在处理嵌套数组时的崩溃 |
| **Security** | 安全漏洞修复 | 修复缓冲区溢出漏洞 CVE-2026-XXXX |
| **Performance** | 性能优化（不改变功能） | 优化 IR 生成，减少内存分配 30% |

### 书写原则

1. **清晰简洁**：每条变更使用一句话描述，突出关键信息
2. **面向用户**：从用户角度描述影响，而非实现细节
3. **标注影响**：对于不兼容变更，明确标注 `[Breaking]` 或 `[兼容性变更]`
4. **引用问题**：关联 Issue 或 PR 编号，如 `#123`、`PR #456`
5. **分类准确**：每条变更归入正确的分类
6. **及时更新**：每次发布前必须更新 CHANGELOG

### 版本链接

每个版本号应链接到对应的 Git 标签或 GitHub Release 页面。

---

## [Unreleased]

### Added
- 变更日志规范与书写指南
- 版本号规范文档（SemVer 2.0.0）
- 发布流程与工件管理规范文档

---

## [0.6.4] - 2026-01-24

### Added
- **6.5 REPL 测试与验收**：完整的 REPL 集成测试套件
  - 表达式测试：`integration_repl_expr_test.c`（9 个测试用例）
  - 语句测试：`integration_repl_statement_test.c`（11 个测试用例）
  - 会话测试：`integration_repl_session_test.c`（10 个测试用例）
  - 命令测试：`repl_command_test.c`（8 个测试用例）

### Changed
- 统一 REPL 错误处理机制，所有执行错误通过 `result.success` 标识
- 优化 REPL 会话管理，修复变量上下文传递问题

### Fixed
- 修复 REPL 重复声明变量的内存管理问题
- 修复 REPL 嵌套作用域的变量解析错误
- 修复 REPL 多条语句执行时的状态不一致问题

---

## [0.6.3] - 2026-01-23

### Added
- **6.4 REPL 诊断与用户体验**：完整的错误诊断与用户友好提示
  - 语法错误诊断测试脚本：`test_repl_diagnostics.ps1`
  - 多行输入自动检测与续行提示
  - 命令帮助系统（`:help` 命令）
  - 上下文感知的错误提示

### Changed
- 改进 REPL 提示符，区分正常模式（`cn> `）和续行模式（`... `）
- 优化错误消息格式，突出显示错误位置

---

## [0.6.2] - 2026-01-22

### Added
- **6.3 REPL 执行模型与状态管理**：完整的执行引擎与会话管理
  - 渐进式执行模型：支持表达式即时求值和语句累积执行
  - 会话状态管理：变量持久化、作用域管理、状态重置
  - 变量查询与状态检查功能
  - 会话单元测试：`repl_session_test.c`（8 个测试用例）

### Changed
- 统一 REPL 执行接口，简化调用流程
- 优化内存管理，修复会话重置时的内存泄漏

---

## [0.6.1] - 2026-01-21

### Added
- **6.2 REPL 基础能力**：完整的 REPL 核心功能
  - 实现 `cnrepl` 可执行文件：`src/cli/cnrepl/main.c`
  - REPL 核心引擎：`src/cli/cnrepl/repl.c`
  - 命令系统：支持 `:quit`、`:clear`、`:vars` 等内置命令
  - 集成测试框架：`tests/integration/integration_repl_test.c`

### Performance
- REPL 启动时间优化至 < 100ms
- 表达式求值响应时间 < 50ms

---

## [0.6.0] - 2026-01-20

### Added
- **阶段 6 工具链与生态**启动
- **6.1 REPL 设计与规划**：完整的 REPL 架构设计文档
- 代码格式化工具 `cnfmt` 基础实现
- 静态检查工具 `cncheck` 基础实现

---

## [0.5.5] - 2026-01-19

### Added
- **5.2 验收自动化**：完整的 OS 集成测试自动化
  - `test_hello_kernel_example()` 集成测试用例
  - 支持 QEMU 自动化验证（Windows/Linux）
  - 添加 Hello Kernel 示例到项目文档

### Fixed
- 修复 freestanding 模式下缺少运行时函数声明的问题
- 修复 C 后端在 freestanding 模式下的代码生成错误

---

## [0.5.4] - 2026-01-18

### Added
- **5.1 简单内核示例**：可在 QEMU 中运行的 Hello Kernel
  - `examples/hello_kernel.cn`：带打印功能的简单内核
  - `examples/boot_hello.c`：串口 I/O 回调实现
  - `build_hello_kernel.ps1`：快速构建脚本

### Changed
- 更新项目 README，添加 Hello Kernel 示例说明

---

## [0.5.3] - 2026-01-17

### Added
- **5.5 集成测试与验证**：freestanding 模式集成测试
  - `tests/integration/os/os_integration_test.c`
  - `tests/integration/os/freestanding/test_freestanding_allowed.cn`
  - `tests/integration/os/freestanding/test_freestanding_violation.cn`

### Fixed
- 修复 freestanding 检查器在处理嵌套调用时的误报

---

## [0.5.2] - 2026-01-16

### Added
- **5.4 freestanding 静态检查**：编译时禁用不允许的函数
  - `src/analysis/checker/static_check.c`：静态检查器实现
  - `tests/unit/freestanding_check_test.c`：单元测试（8 个测试用例）
  - 支持白名单机制，允许明确允许的运行时函数

### Changed
- 扩展语义检查器，集成 freestanding 静态检查

---

## [0.5.1] - 2026-01-15

### Added
- **5.3 编译器选项与模式切换**：支持 freestanding 模式编译
  - 新增 `--target` 编译选项，支持目标平台指定
  - 新增 `--freestanding` 编译选项
  - 目标三元组解析：`support/config/target_triple.c`
  - 单元测试：`support_target_triple_test.c`（7 个测试用例）

### Changed
- 重构 `cnc` 命令行参数解析，支持更多编译选项
- 更新编译流程，根据目标平台选择不同的运行时库

---

## [0.5.0] - 2026-01-14

### Added
- **阶段 5 OS 开发支持与 freestanding 模式**启动
- **5.6 目标平台建模**：支持多目标平台编译
  - 目标三元组定义（`x86_64-elf`、`aarch64-linux-gnu` 等）
  - 平台特定配置管理
- freestanding 运行时初始实现
  - `runtime/core/cnrt_freestanding.c`
  - 不依赖标准库的基础内存操作

### Changed
- 运行时库拆分为 hosted 和 freestanding 两个版本

---

## [0.4.3] - 2026-01-13

### Added
- **4.5 运行时测试与集成**：完整的运行时库测试套件
  - 字符串运行时测试：`runtime_string_test.c`（6 个测试用例）
  - 数组运行时测试：`runtime_array_test.c`（7 个测试用例）
  - I/O 运行时测试：`runtime_io_test.c`（5 个测试用例）
  - 数学运行时测试：`runtime_math_test.c`（4 个测试用例）

### Performance
- 优化数组长度查询，减少函数调用开销
- 优化字符串拼接，使用预分配策略

---

## [0.4.2] - 2026-01-12

### Added
- **4.4 基础标准库**：完整的标准库实现
  - I/O 模块：`runtime/io/cnrt_io.c`、`runtime/io/cnrt_print.c`
  - 数学模块：`runtime/math/cnrt_math.c`
  - 集合模块：`runtime/collections/cnrt_array.c`
- 标准库函数头文件：`include/cnrt.h`

### Changed
- 统一标准库命名规范，所有函数以 `cnrt_` 为前缀

---

## [0.4.1] - 2026-01-11

### Added
- **4.3 内存管理与 Arena 分配器**：高性能内存分配器
  - Arena 分配器实现：`support/memory/arena_allocator.c`
  - 内存池实现：`support/memory/memory_pool.c`
  - Arena 单元测试：`arena_allocator_test.c`（6 个测试用例）
  - Arena 压力测试：`arena_stress_test.c`（大规模分配测试）

### Performance
- Arena 分配器性能：比 malloc 快 3-5 倍（连续小对象分配）
- 内存碎片减少 80%+

### Fixed
- 修复内存池在大块分配时的对齐问题

---

## [0.4.0] - 2026-01-10

### Added
- **阶段 4 最小运行时与基础标准库**启动
- **4.1 运行时架构设计**：完整的运行时架构文档
- **4.2 运行时初始化与生命周期**：程序启动与退出流程
  - 运行时初始化：`runtime/core/cnrt_init.c`
  - 内存管理接口：`runtime/memory/cnrt_memory.c`

---

## [0.3.5] - 2026-01-09

### Added
- **3.8 端到端编译测试**：完整的编译流程验证
  - 集成测试：编译 `hello_world.cn` 并执行验证
  - 集成测试：编译 `array_examples.cn` 并执行验证
  - 集成测试：编译 `function_examples.cn` 并执行验证

### Fixed
- 修复 C 后端在生成数组字面量时的内存泄漏
- 修复 IR 优化在处理控制流时的错误

---

## [0.3.4] - 2026-01-08

### Added
- **3.7 IR 优化 Pass**：基础优化 Pass 实现
  - 常量折叠优化：`ir/passes/constant_folding.c`
  - 死代码消除优化：`ir/passes/dead_code_elimination.c`

### Performance
- 编译后代码体积减少 15-20%
- 运行时性能提升 5-10%（简单算术表达式）

---

## [0.3.3] - 2026-01-07

### Added
- **3.6 构建集成与 C 编译调用**：自动化编译流程
  - `cnc` 编译器支持 `-S` 选项（仅生成 C 代码）
  - `cnc` 编译器支持 `-o` 选项（指定输出文件名）
  - `cnc` 编译器支持 `--cc` 选项（指定 C 编译器）
  - 自动调用 GCC/Clang/MSVC 编译生成的 C 代码

### Changed
- 优化 `cnc` 命令行参数解析，支持更多编译选项

---

## [0.3.2] - 2026-01-06

### Added
- **3.5 C 后端代码生成**：完整的 C 代码生成器
  - C 代码生成器：`backend/cgen/cgen.c`
  - 支持生成标准 C11 代码
  - 支持生成注释和调试信息

### Changed
- C 后端 ABI 规范确定：
  - 函数命名：保持原名或添加 `cn_` 前缀
  - 类型映射：CN_Language 类型到 C 类型的明确映射
  - 数组表示：使用结构体封装（包含长度和数据指针）

---

## [0.3.1] - 2026-01-05

### Added
- **3.3 AST 到 IR 转换**：完整的 IR 生成器
  - IR 生成器：`ir/gen/ir_gen.c`
  - 支持表达式、语句、函数定义的 IR 生成
  - 支持控制流语句的 IR 生成（if、while、for）

### Fixed
- 修复 IR 生成器在处理嵌套表达式时的类型错误

---

## [0.3.0] - 2026-01-04

### Added
- **阶段 3 IR 设计与 C 后端初版**启动
- **3.1 IR 架构设计**：完整的 IR 架构文档
- **3.2 IR 核心数据结构**：IR 指令集与基本块
  - IR 核心实现：`ir/core/ir.c`
  - IR 指令类型：算术、逻辑、控制流、函数调用等
  - IR 基本块与控制流图

---

## [0.2.5] - 2026-01-03

### Added
- **2.8 语义分析集成测试**：完整的语义分析测试套件
  - 语义错误测试：`integration_semantic_error_test.c`（10 个测试用例）
  - 测试示例文件：`test_semantic_errors.cn`

### Fixed
- 修复语义检查器在处理函数重载时的误报
- 修复类型检查器在处理隐式类型转换时的错误

---

## [0.2.4] - 2026-01-02

### Added
- **2.7 语义错误恢复**：错误恢复机制，提高编译器鲁棒性
  - 支持多个语义错误的收集与报告
  - 错误后继续分析，提供更完整的错误信息

---

## [0.2.3] - 2026-01-01

### Added
- **2.6 函数语义分析**：函数定义与调用的完整语义检查
  - 函数签名检查：参数类型匹配、返回类型检查
  - 函数调用验证：参数数量与类型匹配
  - 返回语句检查：确保所有路径都有返回值
- 单元测试：`function_semantics_test.c`（9 个测试用例）

---

## [0.2.2] - 2025-12-31

### Added
- **2.5 表达式类型检查**：完整的表达式类型推导与检查
  - 二元运算符类型检查（算术、比较、逻辑）
  - 一元运算符类型检查
  - 类型兼容性检查与隐式转换规则
- 单元测试：`expression_type_test.c`（12 个测试用例）

---

## [0.2.1] - 2025-12-30

### Added
- **2.4 作用域与名称解析**：完整的作用域管理与符号解析
  - 嵌套作用域支持（全局、函数、块）
  - 名称解析：查找变量、函数定义
  - 重复定义检测与未定义标识符检测
- 单元测试：`scope_test.c`（8 个测试用例）

---

## [0.2.0] - 2025-12-29

### Added
- **阶段 2 语义分析与基础类型系统**启动
- **2.1 符号表设计**：完整的符号表数据结构
- **2.2 符号表实现**：符号表核心功能
  - 符号表实现：`semantics/symbols/symbol_table.c`
  - 支持变量、函数、类型符号的存储与查询
- **2.3 类型系统设计与实现**：基础类型系统
  - 类型定义：`semantics/symbols/type_system.c`
  - 基础类型：整型（int8/int16/int32/int64）、浮点型、布尔型、指针
  - 复合类型：数组、结构体
- 单元测试：`symbol_table_test.c`（7 个测试用例）
- 单元测试：`type_system_test.c`（10 个测试用例）

---

## [0.1.8] - 2025-12-28

### Added
- **1.8 前端集成测试**：完整的前端测试套件
  - 解析成功测试：`integration_parse_success_test.c`（5 个测试用例）
  - 解析失败测试：`integration_parse_failure_test.c`（4 个测试用例）
  - 完整前端测试：`integration_full_frontend_test.c`（结合词法、语法、语义）

---

## [0.1.7] - 2025-12-27

### Added
- **1.7 错误恢复与诊断**：友好的错误提示与恢复机制
  - 语法错误恢复策略（同步点恢复）
  - 丰富的错误诊断信息（行号、列号、上下文）
  - 诊断系统：`support/diagnostics/diagnostics.c`

### Changed
- 优化错误消息格式，提供更清晰的错误位置标识

---

## [0.1.6] - 2025-12-26

### Added
- **1.6 Parser 主要语句解析**：完整的语句解析支持
  - 控制流语句：if/else、while、for、break、continue
  - 返回语句：return
  - 复合语句：代码块
- 单元测试：
  - `parser_loop_test.c`（8 个测试用例）
  - `parser_break_test.c`（6 个测试用例）

---

## [0.1.5] - 2025-12-25

### Added
- **1.5 Parser 表达式与变量声明**：表达式与声明的解析
  - 表达式解析：算术、比较、逻辑运算
  - 变量声明解析：类型标注、初始化
  - 赋值语句解析
  - 数组字面量解析
- 单元测试：
  - `parser_var_test.c`（7 个测试用例）
  - `parser_assign_test.c`（5 个测试用例）
  - `parser_comparison_test.c`（6 个测试用例）
  - `parser_logical_test.c`（8 个测试用例）
  - `parser_array_literal_test.c`（6 个测试用例）
  - `parser_array_assign_test.c`（5 个测试用例）

---

## [0.1.4] - 2025-12-24

### Added
- **1.4 Parser 函数定义解析**：函数声明与定义的解析
  - 函数签名解析（参数列表、返回类型）
  - 函数体解析
- 单元测试：`parser_func_test.c`（8 个测试用例）

---

## [0.1.3] - 2025-12-23

### Added
- **1.3 Parser 基础实现**：递归下降解析器核心
  - Parser 实现：`frontend/parser/parser.c`
  - AST 节点定义：`frontend/ast/ast.c`
  - 支持基础语法结构解析（变量声明、表达式、语句）
- 单元测试：
  - `parser_minimal_test.c`（4 个测试用例）
  - `parser_hello_world_test.c`（3 个测试用例）

---

## [0.1.2] - 2025-12-22

### Added
- **1.2 Lexer 错误处理与诊断**：词法错误处理
  - 错误恢复策略（跳过非法字符）
  - 错误位置标记（行号、列号）
- 单元测试：`lexer_error_test.c`（7 个测试用例）

### Performance
- 词法分析性能测试：`lexer_performance_test.c`
  - 处理 10,000 行代码 < 100ms

---

## [0.1.1] - 2025-12-21

### Added
- **1.1 Lexer 核心实现**：完整的词法分析器
  - Lexer 实现：`frontend/lexer/lexer.c`
  - Token 定义：`frontend/lexer/token.c`
  - 支持中文关键字：`函数`、`变量`、`如果`、`否则`、`循环`、`返回` 等
  - 支持中文标识符（UTF-8 编码）
- 单元测试：
  - `lexer_token_test.c`（15 个测试用例）
  - `lexer_function_examples_test.c`（测试完整程序词法分析）

---

## [0.1.0] - 2025-12-20

### Added
- **阶段 1 前端基础**启动
- 项目初始化与基础架构
  - CMake 构建系统
  - 目录结构：`src/`、`include/`、`tests/`、`docs/`
  - 测试框架搭建（单元测试、集成测试、系统测试）
- 规范文档：
  - `docs/specifications/CN_Language C 代码风格规范.md`
  - `docs/specifications/CN_Language 语言规范草案（核心子集）.md`
  - `docs/specifications/CN_Language 测试规范.md`
- 设计文档：
  - `docs/design/CN_Language 开发计划.md`
  - `docs/design/CN_Language 编译器 工具链架构设计.md`

---

## 版本链接

- [Unreleased]: https://github.com/{org}/CN_Language/compare/v0.6.4...HEAD
- [0.6.4]: https://github.com/{org}/CN_Language/releases/tag/v0.6.4
- [0.6.3]: https://github.com/{org}/CN_Language/releases/tag/v0.6.3
- [0.6.2]: https://github.com/{org}/CN_Language/releases/tag/v0.6.2
- [0.6.1]: https://github.com/{org}/CN_Language/releases/tag/v0.6.1
- [0.6.0]: https://github.com/{org}/CN_Language/releases/tag/v0.6.0
- [0.5.5]: https://github.com/{org}/CN_Language/releases/tag/v0.5.5
- [0.5.4]: https://github.com/{org}/CN_Language/releases/tag/v0.5.4
- [0.5.3]: https://github.com/{org}/CN_Language/releases/tag/v0.5.3
- [0.5.2]: https://github.com/{org}/CN_Language/releases/tag/v0.5.2
- [0.5.1]: https://github.com/{org}/CN_Language/releases/tag/v0.5.1
- [0.5.0]: https://github.com/{org}/CN_Language/releases/tag/v0.5.0
- [0.4.3]: https://github.com/{org}/CN_Language/releases/tag/v0.4.3
- [0.4.2]: https://github.com/{org}/CN_Language/releases/tag/v0.4.2
- [0.4.1]: https://github.com/{org}/CN_Language/releases/tag/v0.4.1
- [0.4.0]: https://github.com/{org}/CN_Language/releases/tag/v0.4.0
- [0.3.5]: https://github.com/{org}/CN_Language/releases/tag/v0.3.5
- [0.3.4]: https://github.com/{org}/CN_Language/releases/tag/v0.3.4
- [0.3.3]: https://github.com/{org}/CN_Language/releases/tag/v0.3.3
- [0.3.2]: https://github.com/{org}/CN_Language/releases/tag/v0.3.2
- [0.3.1]: https://github.com/{org}/CN_Language/releases/tag/v0.3.1
- [0.3.0]: https://github.com/{org}/CN_Language/releases/tag/v0.3.0
- [0.2.5]: https://github.com/{org}/CN_Language/releases/tag/v0.2.5
- [0.2.4]: https://github.com/{org}/CN_Language/releases/tag/v0.2.4
- [0.2.3]: https://github.com/{org}/CN_Language/releases/tag/v0.2.3
- [0.2.2]: https://github.com/{org}/CN_Language/releases/tag/v0.2.2
- [0.2.1]: https://github.com/{org}/CN_Language/releases/tag/v0.2.1
- [0.2.0]: https://github.com/{org}/CN_Language/releases/tag/v0.2.0
- [0.1.8]: https://github.com/{org}/CN_Language/releases/tag/v0.1.8
- [0.1.7]: https://github.com/{org}/CN_Language/releases/tag/v0.1.7
- [0.1.6]: https://github.com/{org}/CN_Language/releases/tag/v0.1.6
- [0.1.5]: https://github.com/{org}/CN_Language/releases/tag/v0.1.5
- [0.1.4]: https://github.com/{org}/CN_Language/releases/tag/v0.1.4
- [0.1.3]: https://github.com/{org}/CN_Language/releases/tag/v0.1.3
- [0.1.2]: https://github.com/{org}/CN_Language/releases/tag/v0.1.2
- [0.1.1]: https://github.com/{org}/CN_Language/releases/tag/v0.1.1
- [0.1.0]: https://github.com/{org}/CN_Language/releases/tag/v0.1.0

---

**注**：本变更日志遵循 [Keep a Changelog](https://keepachangelog.com/) 格式规范。
