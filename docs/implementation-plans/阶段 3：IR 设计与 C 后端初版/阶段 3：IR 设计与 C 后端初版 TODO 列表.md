
根据 `development_plan.md` 中第 113-137 行关于 **“阶段 3：IR 设计与 C 后端初版”** 的规划，以下是为您整理的详细 TODO 列表：

### 阶段 3：IR 设计与 C 后端初版 TODO 列表

#### 1. IR (中间表示) 设计与实现
- [ ] **定义 IR 指令集**：设计支持三地址码或类似结构的指令集（如 `ADD`, `SUB`, `LOAD`, `STORE`, `CALL` 等）。
- [ ] **定义基本块与控制流图 (CFG)**：设计 IR 的组织结构，支持基本块 (Basic Block) 的划分。
- [ ] **实现 IR 数据结构**：在 `src/ir/` 中实现相关结构体，确保其能良好映射 AST 信息与类型系统。

### 1. 细化后的 TODO 列表 (针对原 5-10 行)

#### 1.1 定义 IR 指令集 (三地址码风格)
- [x] **算术与逻辑指令**：`ADD`, `SUB`, `MUL`, `DIV`, `MOD`, `AND`, `OR`, `XOR`, `SHL`, `SHR`。
- [x] **比较指令**：`EQ` (等于), `NE` (不等), `LT` (小于), `LE` (小等), `GT` (大于), `GE` (大等)。
- [x] **内存操作指令**：
    - [x] `ALLOCA`：在栈上分配局部变量空间。
    - [x] `LOAD`：从内存地址加载值到寄存器（虚拟寄存器）。
    - [x] `STORE`：将值存入内存地址。
- [x] **控制流指令**：
    - [x] `LABEL`：定义跳转目标点。
    - [x] `JUMP`：无条件跳转。
    - [x] `BRANCH`：条件跳转（根据寄存器值跳转到不同 Label）。
    - [x] `CALL`：函数调用。
    - [x] `RET`：函数返回。
- [x] **其他指令**：`NEG` (取负), `NOT` (逻辑非), `PHI` (若后续考虑 SSA 形式)。

#### 1.2 定义基本块与控制流图 (CFG)
- [x] **基本块 (Basic Block)**：
    - [x] 设计 `CnIrBasicBlock` 结构，包含一系列指令。
    - [x] 规定基本块必须以跳转指令或返回指令结尾（Terminator）。
- [x] **控制流图 (CFG)**：
    - [x] 在 `CnIrBasicBlock` 中维护前驱 (Predecessors) 和后继 (Successors) 指针。
    - [x] 实现基本块的拆分与连接算法。
- [x] **函数表示**：
    - [x] 设计 `CnIrFunction` 结构，包含入口基本块、参数列表及局部变量表。

#### 1.3 实现 IR 数据结构 (`src/ir/`)
- [x] **目录初始化**：创建 `src/ir/core/` 目录。
- [x] **指令结构体定义**：在 `include/cnlang/ir/ir.h` 中定义 `CnIrInst`，使用 `union` 存储不同指令的参数。
- [x] **操作数定义**：定义 `CnIrOperand`，支持虚拟寄存器、立即数（整型、字符串字面量）和全局符号。
- [x] **IR 打印工具**：实现将 IR 序列化为文本的函数（如 `cn_ir_dump`），便于调试观察 `AST -> IR` 的结果。

#### 2. AST 到 IR 的转换 (Lowering)
- [x] **表达式转换规则**：实现算术、逻辑、赋值等表达式到 IR 指令的生成逻辑。
- [x] **语句与控制流转换**：实现 `if`、`while`、`for` 等控制流语句到 IR 跳转指令的转换。
- [x] **函数定义转换**：处理函数参数、局部变量作用域及返回值的 IR 生成。

#### 3. IR 优化 (Optional/Initial)
- [x] **常量折叠 (Constant Folding)**：实现基础的编译时常量计算优化。
- [x] **死代码删除 (Dead Code Elimination)**：初步实现移除不可达代码或无用赋值的 Pass。

#### 4. C 代码后端实现
- [ ] **约定 C 接口 ABI**：明确函数命名混淆规则、类型映射表（如 `CN int` -> `int32_t`）。
- [ ] **IR 转换 C 代码逻辑**：编写将 IR 指令序列翻译为合法 C 语法的生成器。
- [ ] **输出模块**：实现将生成的 C 代码写入 `.c` 文件的功能。

### 4. C 代码后端实现 (细分后的 TODO 列表)

#### 4.1 约定 C 接口 ABI
- [x] **确定类型映射**：
    - [x] 在 `docs/specifications/language_spec_draft.md` 中明确 CN_Language 基础类型与 C 语言类型的对应关系（如 `int` -> `long long`, `bool` -> `_Bool`, `string` -> `char*` 或自定义结构体）。
    - [x] 为指针类型设计 C 端表示（如 `int*` -> `long long*`）。
- [x] **函数命名规则**：
    - [x] 设计函数名混淆规则（如 `函数名` -> `cn_func_函数名` 或 UTF-8 编码转义），避免 C 编译器不支持中文标识符。
    - [x] 为函数参数和局部变量的命名也约定规则（如 `变量名` -> `cn_var_变量名`）。
- [x] **运行时约定**：
    - [x] 确定如何处理 CN_Language 的字符串、数组等高级类型在 C 中的表示（是直接用 C 数组还是用结构体封装）。

#### 4.2 IR 转换 C 代码逻辑
- [x] **创建 C 代码生成器模块**：
    - [x] 创建 `src/backend/cgen/` 目录。
    - [x] 创建 `include/cnlang/backend/cgen.h` 和 `src/backend/cgen/cgen.c`。
- [x] **实现基础代码生成函数**：
    - [x] `cn_cgen_module`: 生成整个模块的 C 代码（包含头文件、全局声明等）。
    - [x] `cn_cgen_function`: 生成单个函数的 C 代码（函数签名、参数、函数体）。
    - [x] `cn_cgen_block`: 生成基本块对应的 C 语句块。
    - [x] `cn_cgen_inst`: 核心函数，将每条 IR 指令翻译为对应的 C 语句。
- [x] **处理各类 IR 指令**：
    - [x] `ALLOCA`: 生成 C 的局部变量声明（`long long r0;`）。
    - [x] `LOAD`/`STORE`: 生成 C 的变量赋值（`r0 = @var;` / `@var = r0;`）。
    - [x] `ADD`/`SUB`/`MUL`/`DIV`/...: 生成 C 的二元运算符（`r2 = r0 + r1;`）。
    - [x] `EQ`/`NE`/`LT`/...: 生成 C 的比较运算符（`r2 = r0 == r1;`）。
    - [x] `JUMP`/`BRANCH`: 生成 C 的 `goto` 语句和标签（`goto label_0;` / `label_0:`）。
    - [x] `CALL`: 生成 C 的函数调用（`r0 = cn_func_被调函数名(r1, r2);`）。
    - [x] `RET`: 生成 C 的 `return` 语句。
- [x] **处理控制流结构**：
    - [x] 在 `cn_cgen_block` 中，根据 `JUMP` 和 `BRANCH` 指令构建 C 的 `if`/`else`/`while`/`for` 结构，而不仅仅是 `goto`。已初步实现 `if` 结构化转换，`while` 逻辑已预留。

#### 4.3 输出模块
- [x] **实现文件写入**：
    - [x] 在 `cgen.c` 中添加 `cn_cgen_write_to_file(CnIrModule *module, const char *filename)` 函数。
    - [x] 该函数调用 `cn_cgen_module` 获取完整 C 代码字符串，然后将其写入指定的 `.c` 文件。
- [x] **生成头文件（可选）**：
    - [x] 可以考虑同时生成 `.h` 文件，导出模块内的公共函数声明。


#### 5. 构建与集成
- [ ] **自动化编译流程**：在 `cnc` 中集成调用外部 C 编译器（如 `clang` 或 `gcc`）的功能。
- [ ] **链接运行时库**：确保生成的代码能正确链接基础运行时支持。

### 5. 构建与集成 (细分后的 TODO 列表)

#### 5.1 cnc 命令行参数扩展
- [x] **添加编译模式参数**：
    - [x] 新增 `-o <output>` 参数：指定输出文件名。
    - [x] 新增 `-c` 参数：仅生成 `.c` 文件，不编译为可执行文件。
    - [x] 新增 `-S` 参数：仅生成 IR（用于调试），输出到标准输出或文件。
    - [x] 新增 `--emit-c` 参数：生成 C 代码后保留 `.c` 文件（默认编译完成后删除）。
- [x] **添加编译器选择参数**：
    - [x] 新增 `--cc <compiler>` 参数：指定外部 C 编译器路径（如 `clang`、`gcc`、`cl`）。
    - [x] 实现 Windows 下默认使用 `cl.exe`（MSVC），Linux/macOS 下默认使用 `gcc` 或 `clang`。
- [x] **添加调试/优化参数**：
    - [x] 新增 `-g` 参数：生成调试信息。
    - [x] 新增 `-O0/-O1/-O2/-O3` 参数：传递优化级别给 C 编译器。

#### 5.2 集成 IR 生成与 C 后端
- [x] **在 cnc 中引入 IR 和 cgen 模块**：
    - [x] 在 `main.c` 中添加 `#include "cnlang/ir/ir.h"`、`#include "cnlang/ir/irgen.h"`、`#include "cnlang/backend/cgen.h"`。
- [x] **实现编译流水线扩展**：
    - [x] 语义检查通过后，调用 `cn_ir_gen_module(program)` 生成 IR 模块。（注：实现中为 `cn_ir_gen_program`）
    - [x] 可选：调用优化 Pass（常量折叠、死代码删除）。
    - [x] 调用 `cn_cgen_write_to_file(ir_module, output_c_path)` 生成 C 代码。（注：实现中为 `cn_cgen_module_to_file`）

#### 5.3 外部 C 编译器调用
- [x] **设计编译器调用接口**：
    - [x] 在 `src/support/` 下新建 `process.h` 和 `process.c`，封装跨平台进程执行功能。
    - [x] 实现 `cn_support_run_command(const char *command, int *exit_code)` 函数。
- [x] **实现 C 编译器调用逻辑**：
    - [x] 构建编译命令字符串，如 `gcc -o output output.c -L<runtime_path> -lcnrt`。
    - [x] Windows 平台支持 `cl.exe` 命令格式（`cl /Fe:output.exe output.c cnrt.lib`）。
    - [x] 处理编译器输出，捕获错误/警告信息并转发给用户。
- [x] **编译器自动检测**：
    - [x] 实现 `cn_support_detect_c_compiler()` 函数，自动检测系统中可用的 C 编译器。
    - [x] 检测顺序：`CC` 环境变量 -> `clang` -> `gcc` -> `cl`（Windows）。

#### 5.4 运行时库设计与实现
- [x] **创建运行时库目录结构**：
    - [x] 创建 `src/runtime/` 目录。
    - [x] 创建 `include/cnlang/runtime/runtime.h` 作为运行时库公共头文件。
- [x] **实现基础运行时函数**：
    - [x] `cn_rt_print_int(long long val)`：打印整数。
    - [x] `cn_rt_print_bool(int val)`：打印布尔值。
    - [x] `cn_rt_print_string(const char *str)`：打印字符串。
    - [x] `cn_rt_print_newline()`：打印换行符。
- [x] **实现字符串支持函数**（如需要）：
    - [x] `cn_rt_string_concat(const char *a, const char *b)`：字符串拼接。
    - [x] `cn_rt_string_length(const char *str)`：获取字符串长度。
- [x] **实现数组支持函数**（如需要）：
    - [x] `cn_rt_array_alloc(size_t elem_size, size_t count)`：分配数组内存。
    - [x] `cn_rt_array_length(void *arr)`：获取数组长度。
    - [x] `cn_rt_array_bounds_check(void *arr, size_t index)`：边界检查。

#### 5.5 运行时库构建与链接
- [x] **配置运行时库构建**：
    - [x] 在 `src/runtime/CMakeLists.txt` 中配置静态库构建（`libcnrt.a` 或 `cnrt.lib`）。
    - [x] 可选：配置动态库构建（`libcnrt.so` / `cnrt.dll`）。
- [x] **实现运行时库路径管理**：
    - [x] 在 `cnc` 中实现 `get_runtime_lib_path()` 函数，定位运行时库位置。
    - [x] 支持通过 `CN_RUNTIME_PATH` 环境变量自定义路径。
    - [x] 默认查找：与 `cnc` 同目录的 `lib/` 子目录。
- [x] **生成运行时头文件引用**：
    - [x] 在 `cn_cgen_module` 生成的 C 代码开头添加 `#include "cnrt.h"`。
    - [x] 确保生成的 C 代码能找到运行时头文件。

#### 5.6 端到端编译流程测试
- [x] **编写简单测试用例**：
    - [x] 创建 `examples/hello_compile.cn`：包含 `打印("你好，世界")` 的最简程序。
    - [x] 创建 `examples/arithmetic_compile.cn`：包含简单算术运算和打印。
- [x] **手动验证流程**：
    - [x] 运行 `cnc hello_compile.cn -o hello`，验证生成可执行文件。
    - [x] 运行生成的可执行文件，验证输出正确。
- [x] **集成测试**：
    - [x] 在 `tests/integration/` 下添加编译流程测试。
    - [x] 验证 `.cn` -> `.c` -> 可执行文件 -> 运行输出 的完整链路。

#### 6. 验收与验证
- [x] **端到端测试**：在 `tests/integration/compiler/` 下新增测试用例，验证从 `.cn` 到可执行文件并运行结果正确。
- [x] **文档同步**：根据实现情况微调 `IR` 规范文档。

---
**提示**：您可以根据实际进度，要求我为您拆解其中某个具体子任务的实现细节。