# CN语言编译器实施计划（阶段五：Linux ARM64）

> 前序文档：[`plans/002-00-总览与约束.md`](plans/002-00-总览与约束.md)（目标/架构/技术栈/全局约束/执行策略）
> 前序文档：[`plans/002-阶段四-优化与Winx64.md`](plans/002-阶段四-优化与Winx64.md)（阶段四：优化与Win x64完善）
> 后续文档：[`plans/002-阶段六-标准库.md`](plans/002-阶段六-标准库.md)（阶段六：标准库）
> 基于设计规格书：[`plans/001 CN语言编译器设计规格书.md`](plans/001%20CN语言编译器设计规格书.md)

## 阶段五：Linux ARM64

> 目标：ARM64代码生成器、交叉编译支持、ARM64 E2E测试。里程碑：Linux ARM64平台可运行（规格书阶段四末）。

### Task 5.1: ARM64代码生成器
**文件：**
- Create: `src/cn_compiler/codegen/arm64/arm64_codegen.hpp`
- Create: `src/cn_compiler/codegen/arm64/arm64_codegen.cpp`
- Create: `tests/unit/codegen/test_arm64_codegen.cpp`

**验证标准：** AAPCS64调用约定（x0~x7参数、x0返回值、x19~x28被调用者保存，规格书8.3）；GAS汇编语法；NEON浮点指令。

### Task 5.2: 交叉编译支持
**文件：**
- Modify: `src/cn_main.cpp`（`--target linux-arm64` 分发到ARM64后端，调用 `as`/`ld`）
- Modify: `CMakeLists.txt`（交叉编译工具链支持）

**验证标准：** 在Win x64上 `--target linux-arm64` 生成ARM64汇编（`-S`只编译不链接，规格书附录A）。

### Task 5.3: ARM64 E2E测试
**文件：**
- Create: `tests/e2e/run_e2e_arm64.py`（ARM64运行器，可在Linux ARM64或QEMU模拟环境运行）
- Modify: `tests/e2e/01_hello/hello.expected`（复用既有用例）

**验证标准：** ARM64平台全部E2E用例通过（含QEMU环境）。

> ⬜ 未开始

**阶段五完成条件：** ARM64代码生成单元测试通过 + ARM64 E2E通过 + 更新 `plans/002` 阶段五打勾 + Git提交。
