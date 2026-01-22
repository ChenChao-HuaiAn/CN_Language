## CN_Language 编译器/工具链架构设计

### 1. 设计目标与约束

- **总体目标**：
  - 设计并实现一门以中文关键字/语法为主的系统级编程语言 **CN_Language**，编译器和工具链使用 C 语言实现。
  - 语言需具备编写操作系统、驱动、运行时等 **低层系统软件** 的能力，支持裸机环境（freestanding）以及通用操作系统环境。

- **关键约束**：
  - **实现语言**：C（C99/C11）为主，不依赖庞大运行时；可移植到多种平台和编译器。
  - **模块化与低耦合**：组件清晰拆分，模块之间通过明确的接口交互，避免循环依赖。
  - **单一职责原则**：
    - 每个 `.c` 文件不超过 **500 行**。
    - 每个函数不超过 **50 行**。
    - 复杂功能拆分为子模块，每个子模块独立子目录存放。
  - **头文件规范**：
    - `include/` 目录仅暴露公共接口头文件。
    - 模块内部实现头文件放在对应模块目录的 `internal/` 子目录中，不被外部直接包含。
  - **构建与中间文件管理**：
    - 所有 `.o` 等中间文件统一放置在 `build/` 目录下的对应子目录中，按目标和模块分层管理。
  - **测试与质量**：
    - 所有测试代码放在 `tests/` 目录下，按模块映射到对应子目录。
    - 同时覆盖单元测试、集成测试，关键路径支持系统级/回归测试。
  - **规范依赖**：
    - 需 **严格遵循 `docs/specifications` 目录下规范文档** 的要求。
    - 当前仓库尚未创建该目录和具体规范文档，本架构设计基于通用工程实践与上述约束，后续需根据规范文档进行校准与迭代。

---

### 2. 整体架构概览

CN_Language 工具链整体可划分为：

1. **前端（Frontend）**：
   - 词法分析（Lexer）：将源代码字符流拆分为 Token 流。
   - 语法分析（Parser）：将 Token 流构建为抽象语法树（AST）。
   - 源代码诊断与错误恢复。

2. **语义分析与类型系统（Semantics）**：
   - 符号表管理、作用域分析。
   - 类型系统、类型检查。
   - 名称解析、常量折叠、基本语义检查。

3. **中间表示与优化（IR & Middle-End）**：
   - 将 AST 转换为简化的中间表示（IR）。
   - 针对 IR 进行简单的优化（死代码消除、常量折叠、控制流简化等）。

4. **后端与代码生成（Backend）**：
   - 初期：生成 **C 代码**，直接交给现有 C 编译器进行编译和链接。
   - 后期：支持生成汇编/目标文件，甚至直接生成可引导的 OS 镜像（与链接脚本、启动代码配合）。

5. **运行时与标准库（Runtime & Stdlib）**：
   - 最小运行时（启动、内存管理、错误处理）。
   - 基础标准库（字符串、集合、I/O 等）。
   - 针对 OS 开发场景，提供可选的 **不依赖操作系统的运行时**（freestanding runtime）。

6. **工具与辅助组件（Tooling）**：
   - 命令行编译器工具：`cnc`。
   - 交互式 REPL：`cnrepl`。
   - 格式化工具、静态检查工具（未来）。

7. **构建系统与测试（Build & Tests）**：
   - 统一构建入口（如 `cmake`/`make`/自定义脚本）。
   - 所有中间文件进入 `build/` 目录。
   - `tests/` 下单元测试、集成测试、系统级测试结构化组织。

**整体数据流（编译模式）**：

源文件 `.cn` → 字符流 → Token 流 → AST → 带类型信息的 AST → IR → 目标平台相关 IR/中间形式 → C 代码/汇编/目标文件 → 链接 → 可执行文件/内核镜像。

---

### 3. 模块与目录规划

#### 3.1 顶层目录

建议项目整体目录结构如下（部分目录已存在）：

- `src/`
  - 存放编译器、运行时和工具链的 C 源码。
- `include/`
  - 对外暴露的公共头文件。
- `docs/`
  - `design/`：架构设计、开发计划等设计文档（当前文件所在目录）。
  - `implementation-plans/`：实现细节和阶段性方案。
  - `specifications/`：语言规范、编码规范、测试规范等（需补充）。
  - `api/`：API 文档（按模块划分子目录）。
- `examples/`
  - 示例 CN 语言程序（已存在）。
- `tests/`
  - 各模块单元测试、集成测试、系统测试。
- `build/`
  - 编译中间文件和产物，进一步按目标/模块划分子目录。

#### 3.2 `src/` 目录模块拆分

- `src/frontend/`
  - `src/frontend/lexer/`
    - 词法分析器模块，负责 Token 化；输入为字符流，输出为 Token 序列。
  - `src/frontend/parser/`
    - 语法分析器，负责从 Token 构建 AST，并进行基本语法错误恢复。
  - `src/frontend/ast/`
    - 抽象语法树节点定义、AST 遍历工具等。

- `src/semantics/`
  - `src/semantics/symbols/`
    - 符号表结构与操作，作用域管理。
  - `src/semantics/types/`
    - 类型系统相关结构体与操作（基础类型、指针、数组、结构体等）。
  - `src/semantics/resolution/`
    - 名称解析与绑定。
  - `src/semantics/checker/`
    - 语义检查逻辑（类型检查、控制流检查等）。

- `src/ir/`
  - `src/ir/core/`
    - IR 指令和基础结构定义。
  - `src/ir/passes/`
    - 针对 IR 的优化或转换 pass（例如：死代码删除、控制流简化）。

- `src/backend/`
  - `src/backend/cgen/`
    - 将 IR/AST 降低为 C 代码。
  - `src/backend/asmgen/`（后期）
    - 将 IR 直接转换为汇编代码。
  - `src/backend/obj/`（后期）
    - 生成目标文件、对接链接器。

- `src/runtime/`
  - `src/runtime/core/`
    - 与语言语义强关联的基础运行时（启动逻辑、异常/错误处理）。
  - `src/runtime/memory/`
    - 内存管理接口与默认实现（可根据平台替换）。
  - `src/runtime/os_abstraction/`
    - OS 抽象层，封装平台相关调用。
  - `src/runtime/collections/`
    - 常用集合类型实现（动态数组、哈希表等）。

- `src/support/`
  - `src/support/diagnostics/`
    - 日志与诊断系统，统一错误/警告/提示输出。
  - `src/support/utils/`
    - 公共工具函数（字符串处理、小工具函数）等。
  - `src/support/containers/`
    - 内部使用的通用数据结构。
  - `src/support/config/`
    - 配置管理（编译选项、后端目标等）。

- `src/cli/`
  - `src/cli/cnc/`
    - 编译器命令行工具入口（解析命令行参数，调用编译流程）。
  - `src/cli/cnrepl/`
    - REPL 工具入口（脚本模式、交互模式）。

- `src/tools/`（后期）
  - 代码格式化工具、静态检查工具、LSP 服务等。

#### 3.3 `include/` 头文件布局

- `include/cnlang/frontend/*.h`
  - 暴露前端需要的公共接口（如统一的编译入口结构）。
- `include/cnlang/semantics/*.h`
- `include/cnlang/ir/*.h`
- `include/cnlang/backend/*.h`
- `include/cnlang/runtime/*.h`
- `include/cnlang/support/*.h`
- `include/cnlang/cli/*.h`

各模块内部头文件放在：

- `src/<module>/internal/*.h`

仅被对应模块内的 `.c` 文件包含，不在外部被直接使用。

---

### 4. 关键数据结构与数据流

#### 4.1 词法与语法层

- **Token**：
  - 结构体包含：Token 类型、词法文本、位置信息（文件、行号、列号）。
- **AST 节点**：
  - 使用枚举表示节点类型（函数定义、变量声明、表达式等）。
  - 每个节点包含：节点类型、子节点列表、类型信息指针（语义分析阶段填充）、源代码位置。

**数据流**：

1. 源代码通过 `lexer` 转换为 Token 序列。
2. `parser` 读取 Token 序列，根据语法规则构建 AST。
3. 语法错误通过 diagnostics 模块上报，支持错误恢复（如跳过到分号或同步点）。

#### 4.2 语义层

- **符号表（Symbol Table）**：
  - 栈式/树状作用域结构（全局、函数、块级）。
  - 记录标识符名称、声明位置、类型、存储类别等信息。

- **类型系统（Type System）**：
  - 支持基础类型（整型、浮点型、布尔等）。
  - 指针、数组、结构体等复合类型。
  - 后续可扩展泛型、联合类型等。

- **语义检查**：
  - 名称解析：标识符是否在可见作用域内声明。
  - 类型检查：表达式类型一致性、函数调用参数检查等。
  - 控制流检查：返回路径完整性、未初始化变量等（后期）。

**数据流**：

1. AST 作为输入，构建和填充符号表。
2. 对 AST 进行类型推断和检查，将类型信息回写到 AST 节点。
3. 发现语义错误时，通过 diagnostics 模块上报并尽量继续分析。

#### 4.3 IR 与后端

- **IR 设计**：
  - 采用简单的三地址码风格或 SSA-lite 风格，便于 C 代码生成与后续扩展。
  - 包括基本块（basic block）、控制流图（CFG）。

- **IR Pass**：
  - 常量折叠、死代码删除、简单的控制流简化等。

- **C 代码生成**：
  - 将 IR 或经过简化的 AST 缩减到一组接近 C 的结构。
  - 为每个 CN_Language 函数生成对应 C 函数。
  - 映射 CN_Language 的类型到 C 类型，确保 ABI 清晰可控。

- **OS 开发支持**：
  - 支持生成 **freestanding C 代码**，不依赖标准库（`-ffreestanding`）。
  - 提供 hooks 用于自定义入口函数（如 `kmain`）、禁用/替换默认运行时初始化。

---

### 5. 运行模式与命令行接口

#### 5.1 编译模式

命令行工具 `cnc` 的典型子命令：

- `cnc build <源文件...>`：
  - 完整编译流程，生成可执行文件或库文件。
- `cnc compile --emit-c`：
  - 只生成 C 代码，交由外部 C 编译器处理。
- `cnc compile --emit-obj`（后期）：
  - 生成目标文件。
- `cnc compile --emit-ir`：
  - 输出 IR 以便调试与分析。

#### 5.2 检查模式

- `cnc check <源文件...>`：
  - 仅执行解析与语义分析，不生成任何产物。

#### 5.3 REPL 模式

- `cnrepl`：
  - 提供交互式环境，支持输入 CN_Language 语句并立即执行。
  - 内部可复用编译器前端和运行时，通过 JIT 或“片段编译 + 动态加载”实现。

#### 5.4 OS 专用构建模式

- `cnc build --target=<平台> --freestanding`：
  - 为指定目标平台生成 freestanding 代码。
  - 限制使用的语言特性和库，确保在无 OS 环境可运行。

---

### 6. 模块间依赖关系

- **依赖原则**：
  - `support` 模块处于最底层，其他模块可依赖它。
  - `frontend` 可依赖 `support`，但不依赖 `backend`/`runtime`。
  - `semantics` 依赖 `frontend` 的 AST 定义以及 `support`。
  - `ir` 依赖 `semantics` 和 `support`，但不直接依赖 `backend`。
  - `backend` 依赖 `ir` 和 `support`，以及必要时的 `runtime` 接口定义。
  - `runtime` 仅依赖 `support`，不依赖编译器其他模块。
  - `cli`/`tools` 通过 `include/` 中的公共接口调用编译器功能，不直接依赖内部实现。

**约束**：

- 严禁跨层逆向依赖（例如 `frontend` 直接调用 `backend`）。
- 所有跨模块调用必须通过对应的公共头文件接口完成。

---

### 7. 构建系统与目录结构

#### 7.1 构建目录规划

- `build/`
  - `build/host/<配置>/`
    - 编译器自身在当前主机上的构建产物（如 `build/host/debug/`、`build/host/release/`）。
  - `build/target/<三元组>/`
    - 为特定目标平台编译生成的产物（如 `build/target/x86_64-elf/`）。
  - `build/tests/`
    - 测试相关中间文件与二进制。

所有 `.o`、中间 IR 文件、临时生成的 C 代码等均放置于上述子目录中，不污染源码目录。

#### 7.2 构建系统建议

- 初期可采用 **CMake** 或 **自定义 Makefile**：
  - 统一在顶层提供构建入口（如 `cmake` 或 `make`）。
  - 将模块拆分为多个静态库目标，例如：
    - `libcnlang_frontend.a`
    - `libcnlang_semantics.a`
    - `libcnlang_ir.a`
    - `libcnlang_backend.a`
    - `libcnlang_runtime.a`
    - `libcnlang_support.a`
  - 可复用这些库构建 `cnc`、`cnrepl` 等二进制。

- 构建脚本中约束：
  - 源文件列表按模块组织，避免单个目标包含过多 `.c` 文件。
  - 输出目录必须为 `build/` 下子目录，禁止在源码目录内生成 `.o` 文件。

---

### 8. 测试架构设计

#### 8.1 目录结构

- `tests/unit/`
  - `tests/unit/frontend/`
  - `tests/unit/semantics/`
  - `tests/unit/ir/`
  - `tests/unit/backend/`
  - `tests/unit/runtime/`
- `tests/integration/`
  - `tests/integration/compiler/`
    - 典型编译流水线测试（输入 `.cn`，验证输出 C/IR/可执行行为）。
  - `tests/integration/os/`
    - 针对 OS 开发场景的集成测试（后期），如生成可启动镜像并验证启动流程。
- `tests/system/`
  - 端到端系统测试、回归测试。

#### 8.2 测试类型

- **单元测试**：针对模块内部函数、数据结构，使用 C 编写测试驱动；可使用简单测试框架或自研轻量框架。
- **集成测试**：运行完整编译流程，验证生成代码和运行结果；可用脚本驱动，比较标准输出/返回码。
- **系统级测试**（后期）：在模拟器（如 QEMU）或真实硬件上启动由 CN_Language 编写的内核，检查启动输出和关键行为。

#### 8.3 覆盖策略

- 要求：
  - 核心前端、语义分析与代码生成模块具备较高单元测试覆盖度。
  - 对每个语言特性准备至少一个集成测试用例。
  - 每次修复 bug 应添加对应回归测试用例。

---

### 9. 可扩展性与演进

- **语言特性扩展**：
  - 泛型、模块系统改进、协程/并发支持等。

- **后端扩展**：
  - 新增汇编后端和直接目标文件生成后端，逐步摆脱对外部 C 编译器的依赖。

- **工具链扩展**：
  - LSP 支持、调试器集成、性能分析工具等。

- **规范对齐**：
  - 待 `docs/specifications` 目录建立并补充规范后，对本架构进行审查与调整，确保完全符合项目规范。
