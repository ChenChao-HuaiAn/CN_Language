### 阶段 5：OS 开发支持与 freestanding 模式 TODO 列表（细分版）

#### 一、目标平台建模（`support/config`）

- **1.1 目标三元组基础设施**
  - [x] 设计并确定目标三元组表示结构（如 `CnTargetTriple`），字段包括架构、厂商、OS、ABI 等  
  - [x] 在 `include/cnlang/support/` 下新增/扩展头文件，声明目标三元组 API  
  - [x] 在 `src/support/` 下实现目标三元组解析与构造函数（从字符串/预设枚举生成）

- **1.2 预设 OS 目标定义**
  - [ ] 定义 `x86_64-elf` 等用于 OS 开发的预设目标三元组  
  - [ ] 为每个预设目标补充：指针大小、对齐约束、整数宽度等基本数据布局信息  
  - [ ] 在 CLI (`src/cli/cnc/main.c`) 中增加命令行选项用于选择目标三元组（如 `--target=x86_64-elf`）

- **1.3 freestanding 编译选项**
  - [ ] 在配置/支持模块中定义“freestanding 模式”标志（如 `CnCompileModeFreestanding`）  
  - [ ] 为 freestanding 目标提供默认编译选项：禁用宿主标准库、禁用异常/某些运行时依赖  
  - [ ] 在 C 代码生成阶段（`src/backend/cgen/cgen.c`）接入目标配置：根据目标三元组和模式生成不同的编译选项（如编译器 flags、入口符号）

---

#### 二、freestanding 运行时

- **2.1 运行时接口抽象**
  - [ ] 盘点现有运行时接口（`src/runtime/*`、`include/cnlang/runtime/*`），标出需在 freestanding 下支持的最小子集  
  - [ ] 为“可选依赖 OS”的功能增加抽象层或条件编译开关（如 I/O、内存分配）

- **2.2 freestanding 版本实现**
  - [ ] 在 `runtime` 中为 freestanding 模式设计独立实现路径（可通过 `#ifdef CN_FREESTANDING` 或单独源文件组织）  
  - [ ] 实现不依赖宿主 OS 的基础功能：  
    - [ ] 最简内存分配策略（可先支持静态/简单堆管理，允许后续替换）  
    - [ ] 最基本的字符串和内存操作（如 `memcpy`、`memcmp` 等）  
  - [ ] 确保 freestanding 运行时不直接依赖系统调用或 C 标准库中非 freestanding 保证的部分

- **2.3 OS 内核环境限制适配**
  - [ ] 设计无堆环境下的运行模式（例如：仅使用静态分配/栈分配的语言特性）  
  - [ ] 明确“无文件系统”环境下的 I/O 策略（如通过提供回调/由内核侧实现输出函数）  
  - [ ] 对运行时中不适用的 API（如文件读写、进程管理）做编译期禁止或 stub 处理

---

#### 三、编译器限制（freestanding 模式语义约束）

- **3.1 语言/库特性白名单与黑名单**
  - [ ] 列出在 freestanding 模式下禁止使用的语言特性和标准库函数（参考 `docs/specifications/CN_Language 语言规范草案（核心子集）.md`）  
  - [ ] 在语义分析阶段（`src/semantics/checker/semantic_passes.c`）增加对 freestanding 模式的额外检查  
  - [ ] 为被禁止特性设计统一的诊断信息（通过 `src/support/diagnostics`）

- **3.2 编译选项与诊断**
  - [ ] 为 CLI 增加显式启用 freestanding 模式的选项（如 `--freestanding`）  
  - [ ] 当 freestanding 模式启用时，确保：  
    - [ ] 禁止链接宿主 OS 相关运行时  
    - [ ] 对不被支持的库调用给出编译错误  
  - [ ] 更新/扩展测试规范文档中关于 freestanding 模式的约束（只在你之后明确要求修改文档时再动 md 文件）

---

#### 四、集成测试（`tests/integration/os/`）

- **4.1 测试基础设施**
  - [ ] 在 `tests/integration/os/` 下规划目录结构（如 `boot/`、`kernels/`、`scripts/`）  
  - [ ] 为 CMake 增加构建 OS 集成测试的目标（在 `tests/integration/CMakeLists.txt` 中添加对应 target）  
  - [ ] 编写脚本/驱动程序，用于调用 `cnc` 生成目标代码并配合启动代码、链接脚本打包为镜像（例如 ELF/binary）

- **4.2 启动代码与链接脚本**
  - [ ] 准备最小化启动代码（如汇编/ C 启动 stub），在构建系统中集成  
  - [ ] 编写简单链接脚本以放置代码和数据段，适配 `x86_64-elf` 目标  
  - [ ] 验证生成的镜像能被 QEMU/Bochs 等模拟器正常加载

- **4.3 自动化运行与结果检查**
  - [ ] 添加基于 QEMU/Bochs 的自动化运行脚本（可在 `tests/integration/os/` 中提供可调用脚本）  
  - [ ] 设计输出检查机制（如通过串口/控制台输出特定字符串，并在脚本中解析）  
  - [ ] 将 OS 集成测试接入 `ctest`，保证运行 `ctest` 时能够自动执行并判断通过/失败

---

#### 五、里程碑验收用例

- **5.1 简单内核示例**
  - [ ] 设计一个最小 CN_Language 内核示例程序（仅打印字符串或做状态切换）  
  - [ ] 为该内核示例编写编译与打包指令（通过 CMake 或脚本集成）  
  - [ ] 在模拟器中确认能成功启动并输出预期内容

- **5.2 验收自动化**
  - [ ] 将上述简单内核示例纳入 `tests/integration/os/` 的自动化测试用例  
  - [ ] 在项目文档/开发计划中标记该用例为阶段 5 的验收标准之一（若你之后要求同步修改文档时一并调整）  