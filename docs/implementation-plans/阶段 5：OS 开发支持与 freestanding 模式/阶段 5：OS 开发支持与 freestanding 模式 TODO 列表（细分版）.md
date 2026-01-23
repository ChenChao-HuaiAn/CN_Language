### 阶段 5：OS 开发支持与 freestanding 模式 TODO 列表（细分版）

#### 一、目标平台建模（`support/config`）

- **1.1 目标三元组基础设施**
  - [x] 设计并确定目标三元组表示结构（如 `CnTargetTriple`），字段包括架构、厂商、OS、ABI 等  
  - [x] 在 `include/cnlang/support/` 下新增/扩展头文件，声明目标三元组 API  
  - [x] 在 `src/support/` 下实现目标三元组解析与构造函数（从字符串/预设枚举生成）

- **1.2 预设 OS 目标定义**
  - [x] 定义 `x86_64-elf` 等用于 OS 开发的预设目标三元组  
  - [x] 为每个预设目标补充：指针大小、对齐约束、整数宽度等基本数据布局信息  
  - [x] 在 CLI (`src/cli/cnc/main.c`) 中增加命令行选项用于选择目标三元组（如 `--target=x86_64-elf`）

- **1.3 freestanding 编译选项**
  - [x] 在配置/支持模块中定义"freestanding 模式"标志（如 `CnCompileModeFreestanding`）  
  - [x] 为 freestanding 目标提供默认编译选项：禁用宿主标准库、禁用异常/某些运行时依赖  
  - [x] 在 C 代码生成阶段（`src/backend/cgen/cgen.c`）接入目标配置：根据目标三元组和模式生成不同的编译选项（如编译器 flags、入口符号）

- **1.4 编译管线目标传递与后端集成**（建议新增小节）

 **[x] 在前端 / IR 生成阶段接入 `CnTargetTriple`**
  - **[x]** 在 `cnc` 的编译主流程中，将 `target_triple` 作为参数传递给 IR 生成入口（如 `cn_ir_gen_program` 或后续扩展接口）。
  - **[x]** 在 IR 模块结构中预留“目标信息”字段（如 `CnIrModule` 内添加 `CnTargetTriple target` 或等效抽象）。
  - **[x]** 在 IR 生成实现中（`src/ir/gen/irgen.c`），确保创建 IR 模块时初始化目标信息。

 **[x] 在 C 后端接入目标与数据布局**
  - **[x]** 为 C 后端入口（`cn_cgen_module_to_file` 等）增加参数或配置结构，用于接收 `CnTargetTriple` / `CnTargetDataLayout`。
  - **[x]** 在调用 C 后端前，通过 `cn_support_target_get_data_layout` 获取目标的数据布局，并在获取失败时给出明确错误（后续可通过 Diagnostics 统一）。
  - **[x]** 在 C 代码生成实现中，根据数据布局信息调整类型映射（指针大小、整数类型映射等），为后续 ABI 约定打基础。

 **[x] 将目标信息用于外部 C 编译器调用**
  - **[x]** 在 `cnc` 构造外部编译命令时，预留基于目标的额外参数扩展点（如针对 `x86_64-elf` 的特定 `-target` 或架构选项，先以 TODO 注释形式标记）。
  - **[x]** 为 freestanding 场景预留“禁用宿主标准库”的选项挂接点（后续与运行时/链接脚本集成时填充具体 flag）。
  - **[x]** 在命令行帮助中标注“当前目标仅部分影响后端行为，完整 freestanding 支持将在后续小节实现”，避免语义不清。

 **[x] 测试与验证 TODO**
  - **[x]** 新增/扩展一个集成测试（可放在 `tests/integration/compiler/`），编译同一个 `.cn` 程序，分别使用默认目标与显式 `--target=x86_64-elf`，验证编译流程正常。
  - **[x]** 为 C 后端目标数据布局接入增加至少一个单元测试（例如检查 IR → C 的类型映射是否符合 `x86_64-elf` 的宽度预期）。
  - **[x]** 在 `ctest` 中通过相关测试，作为“目标信息贯通编译管线”的初步验收标准。
---

#### 二、freestanding 运行时

- **2.1 运行时接口抽象**
  - [x] 盘点现有运行时接口（`src/runtime/*`、`include/cnlang/runtime/*`），标出需在 freestanding 下支持的最小子集  
  - [x] 为“可选依赖 OS”的功能增加抽象层或条件编译开关（如 I/O、内存分配）

- **2.2 freestanding 版本实现**
  - [x] 在 `runtime` 中为 freestanding 模式设计独立实现路径（可通过 `#ifdef CN_FREESTANDING` 或单独源文件组织）  
  - [x] 实现不依赖宿主 OS 的基础功能：  
    - [x] 最简内存分配策略（可先支持静态/简单堆管理，允许后续替换）  
    - [x] 最基本的字符串和内存操作（如 `memcpy`、`memcmp` 等）  
  - [x] 确保 freestanding 运行时不直接依赖系统调用或 C 标准库中非 freestanding 保证的部分

- **2.3 OS 内核环境限制适配**
  - [x] 设计无堆环境下的运行模式（例如：仅使用静态分配/栈分配的语言特性）  
  - [x] 明确“无文件系统”环境下的 I/O 策略（如通过提供回调/由内核侧实现输出函数）  
  - [x] 对运行时中不适用的 API（如文件读写、进程管理）做编译期禁止或 stub 处理

---

#### 三、编译器限制（freestanding 模式语义约束）

- **3.1 语言/库特性白名单与黑名单**
  - [x] 列出在 freestanding 模式下禁止使用的语言特性和标准库函数（参考 `docs/specifications/CN_Language 语言规范草案（核心子集）.md`）  
  - [x] 在语义分析阶段（`src/semantics/checker/semantic_passes.c`）增加对 freestanding 模式的额外检查  
  - [x] 为被禁止特性设计统一的诊断信息（通过 `src/support/diagnostics`）

- **3.2 编译选项与诊断**
  - [x] 为 CLI 增加显式启用 freestanding 模式的选项（如 `--freestanding`）  
  - [x] 当 freestanding 模式启用时，确保：  
    - [x] 禁止链接宿主 OS 相关运行时  
    - [x] 对不被支持的库调用给出编译错误  
  - [x] 更新/扩展测试规范文档中关于 freestanding 模式的约束

---

#### 四、集成测试（`tests/integration/os/`）

- **4.1 测试基础设施**
  - [x] 在 `tests/integration/os/` 下规划目录结构（如 `boot/`、`kernels/`、`scripts/`）  
  - [x] 为 CMake 增加构建 OS 集成测试的目标（在 `tests/integration/CMakeLists.txt` 中添加对应 target）  
  - [x] 编写脚本/驱动程序，用于调用 `cnc` 生成目标代码并配合启动代码、链接脚本打包为镜像（例如 ELF/binary）
  - [ ] **注意**：当前测试需要 C 后端支持 freestanding 模式（不生成 `#include "cnrt.h"`），待后续实现

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

---

#### 六、字符串与数组语义验证（`runtime_test_full` 驱动）

- **6.1 字符串变量语义补全**
  - [ ] 明确当前阶段支持的字符串变量语义范围（赋值、拷贝、作为参数等），并在语言规范中形成约束（仅在你之后要求时再修改规范文档）。  
  - [ ] 在语义分析（`src/semantics/checker/semantic_passes.c`）中，确保使用 `变量` 声明且初始化为字符串字面量时，符号表中的类型为 `字符串`，并在后续表达式中保持一致。  
  - [ ] 在 IR 生成与 C 后端中，保证字符串变量不会被误当作整数寄存器使用（地址/值在类型上传递正确）。  
  - [ ] 逐步恢复 `examples/runtime_test_full.cn` 中关于 `文本`、`修整后的文本` 的注释代码，使其在当前阶段能够成功编译并运行，输出与 C 版本 `runtime_test_full.c` 一致。  

- **6.2 数组字面量与长度运算（最小子集）**
  - [ ] 为数组字面量（如 `[1, 2, 3, 4, 5]`）设计并实现最小语法规则，在 Parser 中接入并保证与现有优先级体系兼容。  
  - [ ] 在类型系统中为数组引入（或补全）最小表示，至少支持整型数组和字符串数组，能够参与 `长度(数组)` 等基础操作。  
  - [ ] 在运行时与标准库中，为数组提供 `长度(数组)` 的最小支持路径（可以复用/封装现有 `cn_rt_array_*` API），并对越界等行为留出后续扩展点。  
  - [ ] 在 `examples/runtime_test_full.cn` 中逐步恢复数组相关测试（先恢复数组长度，再按阶段扩展索引访问等），每次恢复都对应至少一个单元/集成测试用例。  

- **6.3 测试驱动与回归保障**
  - [ ] 在 `tests/unit/` 下为字符串变量和数组字面量增加或扩展单元测试（例如 Parser 层的最小语法用例、运行时长度计算的边界用例等）。  
  - [ ] 在 `tests/integration/compiler/` 中增加专门覆盖字符串/数组组合场景的集成测试，或在现有 `integration_compile_full_test` 中增加带断言的新子场景。  
  - [ ] 将 `runtime_test_full` 的关键行为（打印字符串内容与长度）纳入自动输出检查，确保后续修改不会破坏已验证的字符串/数组语义。  