# HANDOFF - CN语言编译器项目交接文档

> **交接原因**：2026-08-12 开发者更换电脑。本文档写给完全没有上下文的新会话看，帮助新环境快速恢复开发。

## 一、我们在做什么任务

正在开发CN语言编译器——一门全中文语法的系统级编程语言，参考C++编程范式，用C++从零编写编译器，直接生成汇编代码。

## 二、已经完成了什么

### 2.1 设计规格书（已批准）
- 文件：`plans/001 CN语言编译器设计规格书.md`（984行，14章）
- 状态：用户已批准，已通过规格自审（13个问题全部修复）

### 2.2 cn-language-spec 技能（已创建）
- 目录：`.ai-coder/skills/cn-language-spec/`（16个文件）
- 内容：覆盖词法/类型/控制流/函数/结构体枚举/OOP/错误处理/模块系统全部语法规范

### 2.3 阶段零：工程基础设施（已完成）
- **Task 0.1**：CMake项目骨架、CLI框架（build/compile/run/check/ir/ast/token命令）、诊断系统（SourceLocation + 诊断引擎）、Google Test集成
- **Task 0.2**：Google Test集成（实际已在Task 0.1中一并完成）
- **Task 0.3**：E2E测试框架（`tests/e2e/run_e2e.py`，CMake集成 + CTest注册）

### 2.4 阶段一：Task 1.1 + Task 1.1a + Task 1.2（已完成）
- **Task 1.1**：Token定义（`token.hpp/.cpp`、`test_token.cpp`）——53个关键字/字面量/运算符/分隔符
- **Task 1.1a**：补充 `++`/`--`（PlusPlus/MinusMinus）和 `?`（Question）Token
- **Task 1.2**：词法分析器完整实现（`lexer.hpp/.cpp`、`test_lexer.cpp`）

### 2.5 阶段一：Task 1.3 + Task 1.4（已完成）
- **Task 1.3**：AST节点定义（`src/cn_compiler/parser/ast.hpp/.cpp`、`test_ast.cpp`）
  - 22个节点类 + AstVisitor访问者基类 + NodeType/Operator枚举
- **Task 1.4**：语法分析器（`src/cn_compiler/parser/parser.hpp/.cpp`、`test_parser.cpp`）
  - 递归下降 + Pratt表达式解析（10级优先级链）、错误恢复、可选分号策略

### 2.6 阶段一：Task 1.5 + Task 1.6（本次完成）
- **Task 1.5**：语义分析器（`src/cn_compiler/semantic/semantic.hpp/.cpp`、`test_semantic.cpp` 30个测试）
  - `SemanticAnalyzer` 类：继承 AstVisitor，两趟处理（先注册全部函数符号支持前向调用，再检查函数体）
  - 符号表：函数符号表（返回类型+参数类型）、变量作用域栈（嵌套作用域、遮蔽、重复声明检测）
  - 类型检查：变量声明/赋值/二元/一元/函数调用/返回/条件；类型系统覆盖整8~128/正8~128/浮32/64/字符/布尔/字符串/空类型
  - 语义错误：未声明、重复声明/定义、类型不匹配、非循环中中断/继续、缺少返回语句
- **Task 1.6**：CN-IR生成（`src/cn_compiler/ir/ir.hpp/.cpp`、`test_ir.cpp` 17个测试）
  - IR结构：IRValue（%vN寄存器/常量/变量）、IRInstruction（三地址码）、IRBlock（CFG+终止）、IRFunction、IRModule（函数+字符串常量池）
  - Opcode 25种：ConstInt/Float/String/Bool、Add/Sub/Mul/Div/Mod、Eq/Ne/Lt/Le/Gt/Ge、And/Or/Not、Load/Store/Alloca、Jump/Branch/Call/Return、Phi预留
  - `IRGenerator` 类：AST→IR 全链路（函数/变量Alloca+Store/控制流基本块/调用/字符串常量池去重）
- **CMake集成**：新增 `cn_compiler_semantic`、`cn_compiler_ir` 静态库；测试链接入 `cn_unit_tests`
- **测试结果**：**181/181 单元测试全部通过**（common 12 + lexer 37 + parser 60 + semantic 33 + ir 17 + codegen 10 + runtime 12），编译零错误零警告

### 2.7 阶段一：Task 1.7 + Task 1.8（已完成，前一会话）
- **Task 1.7**：X64代码生成器（`src/cn_compiler/codegen/x64/x64_codegen.cpp/.hpp`、`x64_instructions.cpp`、`test_x64_codegen.cpp` 10个测试）
  - `X64CodeGenerator` 类：IR模块→Win x64 MASM汇编（.data段+ .code段 + END）
  - 栈槽分配（虚拟寄存器→[rbp-8N-8]）、Win x64调用约定（rcx/rdx/r8/r9 + 影子空间）、符号修饰（中文名UTF-8十六进制 ?XX..@@Y）
- **Task 1.8**：运行时库（`src/runtime/runtime.hpp`、`io_api.cpp`、`runtime.cpp`、`test_runtime.cpp` 12个测试）
  - `extern "C"` 导出：`printLine/printLineInt/printLineFloat`（打印行系列）、`cn_alloc/cn_free/...`（内存管理）
  - 程序入口 `entry()`（crt0风格调用 `cn_main`）+ Windows `WinMain` 转发
  - 符号映射：CN `主`→`cn_main`、`打印行`→`printLine`

### 2.8 阶段一：Task 1.9 Hello World 全链路（本次完成 ✅ 阶段一里程碑）
- **`cn build` 命令**（`src/cn_main.cpp`）：词法→语法→语义→IR→X64汇编文本→写 .asm→ml64 汇编→cl 编译运行时→link 链接→.exe
- **符号映射（方案A：代码生成器内处理）**：`x64_codegen.cpp` 新增 `symbolName()`——`主`→`cn_main`、`打印行`→`printLine` 等9个运行时符号；`x64_instructions.cpp` Call 指令改用 `symbolName()`
- **语义内置函数**：`semantic.cpp` 新增 `registerBuiltins()` 注册 `打印行/打印行整数/打印行浮点`
- **ml64 MASM 兼容性**：asm 写 UTF-8 无 BOM；字符串常量改十六进制字节（`hexBytesString()`）；IR 块标签 `块N`→`bbN`；`.code` 段加 `EXTERN` 声明
- **链接配置**：`/ENTRY:WinMainCRTStartup` + `/DEFAULTLIB:libcmt.lib /DEFAULTLIB:libucrt.lib /DEFAULTLIB:kernel32.lib`
- **测试同步**：`test_x64_codegen.cpp` 字符串断言改为十六进制字节
- **验证**：`cn build tests/e2e/01_hello/hello.cn` → `target/hello.exe` 输出"你好，世界"；E2E 1/1 通过；单元测试 181/181 通过

### 2.9 阶段一：Task 1.10 全命令集成（已完成 ✅）
- **公共流水线抽取**：新建 `src/cn_compiler/driver/driver.hpp/.cpp`，`runPipeline()` 抽取 词法→语法→语义→IR→汇编文本 公共流水线，供 build/compile/run/ir 复用；`runCheck()`/`printTokens()`/`printIr()` 调试输出
- **AST打印器**：新建 `src/cn_compiler/parser/ast_printer.hpp/.cpp`，继承 AstVisitor 递归打印 AST 树
- **7 个命令全部可用**：`build`（复用 runPipeline，行为与 Task 1.9 一致）/`compile`（输出 .asm）/`run`（编译+执行透传退出码，修复 cmd 正斜杠路径 bug）/`check`（语义检查）/`ir`（字符串池+函数+基本块+指令）/`ast`（AST 树）/`token`（Token 流）
- **CMake**：注册 `cn_compiler_ast_printer`、`cn_compiler_driver` 静态库

### 2.10 阶段一：生产环境审查修复（已完成 ✅，2026-08-12 19:04）
Debug 子任务模拟生产环境发现并修复 8 个 BUG（详见 lessons.md 权重31.9 记录）：
1. **类型别名规范化**：`canonicalType()` 统一 `整数`→`整32`/`小数`→`浮64`，declareVar/registerFunction/canConvert/commonNumericType 全链路使用
2. **变量槽登记**：`generateFunctionAssembly` 扫描全部 Alloca 登记局部变量槽（此前全部生成 `[rbp0]` 报 A2006）
3. **idiv size 前缀**：`idiv dword ptr/qword ptr [rbp-X]`（此前 A2023）
4. **Win x64 影子空间**：`emitCall` 一次性 `sub rsp,(32+栈参+对齐)` 预留；接收侧栈参偏移 `[rbp+48+(i-4)*8]`
5. **IR 变量遮蔽**：作用域栈 + 唯一内部名 `name$N`，func.params 保留源码名 + paramUniques
6. **i32 参数符号扩展**：传参前 `movsxd` 符号扩展 i32→i64（负数不再变巨大正数）
7. **--output 目录创建**：`ensureDirExists()` 逐级创建目录（此前 LNK1104）
8. **流水线错误检查**：driver 流水线 IR/codegen 阶段补 `diagnostics.hasErrors()` 检查

### 2.11 阶段一收尾（已完成 ✅，2026-08-12 19:05）
- **清理根目录 10 个杂散 .obj**（asm_demo/ast/diagnostics/ir/lexer/parser/semantic/token/x64_codegen/x64_instructions），确认无 .exe/.asm/.lib/.pdb 杂散
- **.gitignore 新增**：`*.obj/*.exe/*.asm/*.lib/*.pdb/*.ilk/*.exp/*.res`
- **最终回归**：构建 14 工程零警告；单元测试 181/181；E2E 1/1（--strict）；手动 `cn build hello.cn` → `hello.exe` 输出"你好，世界"
- **plans 打勾**：Task 1.8 的 5 个 Step 补勾；Task 1.10 后标注 8 个 BUG 已修复
- **已推送** gitcode develop 分支

### 2.12 实施计划文档拆分（已完成 ✅，2026-08-12 20:36）
- **文档结构重构（非代码变更）**：原 [`plans/002 CN语言编译器实施计划.md`](plans/002%20CN语言编译器实施计划.md)（3968 行）按阶段边界拆分为 **12 个子文件 + 1 个索引文件**
- 原 002 文件已改写为 **导航索引**（70 行）：[`plans/002 CN语言编译器实施计划.md`](plans/002%20CN语言编译器实施计划.md)
- 子文件（已完成阶段）：[`plans/002-00-总览与约束.md`](plans/002-00-总览与约束.md)（110行）、[`plans/002-阶段零-工程搭建.md`](plans/002-阶段零-工程搭建.md)（565行）、[`plans/002-阶段一A-词法分析.md`](plans/002-阶段一A-词法分析.md)（808行）、[`plans/002-阶段一B-语法分析.md`](plans/002-阶段一B-语法分析.md)（843行）、[`plans/002-阶段一C-语义与IR.md`](plans/002-阶段一C-语义与IR.md)（957行）、[`plans/002-阶段一D-后端与集成.md`](plans/002-阶段一D-后端与集成.md)（482行）
- 子文件（未开始阶段）：[`plans/002-阶段二-核心语言.md`](plans/002-阶段二-核心语言.md)（91行）、[`plans/002-阶段三-OOP与错误处理.md`](plans/002-阶段三-OOP与错误处理.md)（54行）、[`plans/002-阶段四-优化与Winx64.md`](plans/002-阶段四-优化与Winx64.md)（35行）、[`plans/002-阶段五-LinuxARM64.md`](plans/002-阶段五-LinuxARM64.md)（25行）、[`plans/002-阶段六-标准库.md`](plans/002-阶段六-标准库.md)（30行）、[`plans/002-阶段七-自举.md`](plans/002-阶段七-自举.md)（28行）
- **质量保障**：内容零丢失（88 个 `[x]` 勾选框、全部代码块、"已实现/已修复"历史备注、Task 1.1a 特殊任务均保留）；每个子文件头部带"前序文档/后续文档"双向链接；索引文件 ≤200 行
- 后续开发任务按索引导航进入对应阶段子文件执行

## 三、总结发现的问题

### 3.1 本阶段发现并解决的问题
1. **多字节UTF-8字符与char比较**：`from[0] == '正'` 触发 -Werror=multichar/type-limits → 改用 `from.compare(0, 3, "正") == 0` 字符串前缀比较
2. **GCC 7 不支持中文标识符（测试名）**：`TEST(Suite, 中文名)` 宏展开生成中文类名/函数名编译失败 → 测试名必须用英文，中文仅用于注释/字符串（现有 parser/lexer 测试均如此）
3. **IRGenerator 虚函数缺失**：头文件声明 `visitIfStmt/visitWhileStmt/visitForStmt` 但实现遗漏 → 链接报 undefined reference，补充转发到 genIf/genWhile/genFor
4. **`结果` 是保留关键字**：CN 语言 `结果<T,E>`（Kw_Result）不能作变量名，`变量 结果 = ...` 解析失败（"预期变量名"）→ 测试改用 `答案`
5. **ml64 UTF-8 兼容性（Task 1.9）**：带 BOM 的 asm 全部 A2044；`db "中文"` 原始 UTF-8 字节 A2044；中文块标签 A2044 → 解决方案：asm 无 BOM + 字符串十六进制字节 + ASCII 标签
6. **链接 CRT 注入（Task 1.9）**：`/ENTRY:WinMain` 时 link 不自动注入 CRT 库（LNK2019 大量未解析）；`/ENTRY:WinMain` 跳过 CRT 启动导致 stdout 未初始化输出为空 → 用 `WinMainCRTStartup` + 显式 `/DEFAULTLIB`
7. **内置函数符号缺失（Task 1.9）**：语义分析器报"未声明的函数 '打印行'" → 新增 `registerBuiltins()`
8. **MSVC getenv C4996**：/W4 /WX 下 `getenv` 报 C2220 错误 → 用 `_dupenv_s` 封装

### 3.2 既有经验（HANDOFF继承 + Task 1.9 补充）
1. **GCC 7 不支持中文标识符**：所有标识符必须英文，中文仅用于注释/字符串
2. **无外网**：googletest 用本地 `~/third-party/unittest/googletest`（LLVM定制版，需 `-DGTEST_NO_LLVM_SUPPORT`）
3. **编译产物在 target/**（CMAKE_RUNTIME_OUTPUT_DIRECTORY），测试二进制在 `target/Debug/cn_unit_tests.exe`
4. **实施计划示例代码用中文API命名，但用户明确要求英文API命名**（"这些指令优先于任何冲突的通用模式指令"）——新代码一律英文API
5. **本机构建命令（Windows MSVC）**：`"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build target/build --config Debug --target cn`（cmake 不在 PATH，用 VS 自带完整路径）
6. **本机工具链**：VS2022 Community + MSVC 14.38.33130（vcvars64.bat 用此版本）+ Windows SDK 10.0.26100.0；vswhere 定位安装路径
7. **ml64 汇编限制**：asm 必须 UTF-8 无 BOM；中文只在注释和 `db` 十六进制；标签/符号 ASCII
8. **cn build 链接命令**：`link /ENTRY:WinMainCRTStartup /SUBSYSTEM:CONSOLE /DEFAULTLIB:libcmt.lib /DEFAULTLIB:libucrt.lib /DEFAULTLIB:kernel32.lib`

## 四、当前卡在哪

无卡点。**阶段一（Task 1.1~1.10）已全部完成**：7 个 CLI 命令全可用，生产环境审查 8 个 BUG 已修复，最终回归验证通过（181/181 单元测试 + E2E 1/1 + 手动 build 输出"你好，世界"），已提交推送 gitcode develop。

## 五、下一步计划是什么

### 5.1 立即需要做的
实施计划已按阶段拆分，**从导航索引进入对应阶段文档**：[`plans/002 CN语言编译器实施计划.md`](plans/002%20CN语言编译器实施计划.md)（索引）→ 阶段二：

执行 [`plans/002-阶段二-核心语言.md`](plans/002-阶段二-核心语言.md)（核心语言）：
- **Task 2.1**：控制流（如果/否则/当/循环/选择/中断/继续）——parser 解析、semantic 布尔检查+循环嵌套、IR 条件跳转/基本块拆分、x64 cmp/jcc
- **Task 2.2**：函数完整支持（原型声明/递归/函数指针）
- 后续：全部位宽类型/数组指针/字符串系统/结构体枚举/基础优化
- 每阶段继续遵循 E2E 先行 + 打勾 plans + 更新 更新日志/HANDOFF/lessons
- 阶段三~七待办文件：[`plans/002-阶段三-OOP与错误处理.md`](plans/002-阶段三-OOP与错误处理.md) / [`plans/002-阶段四-优化与Winx64.md`](plans/002-阶段四-优化与Winx64.md) / [`plans/002-阶段五-LinuxARM64.md`](plans/002-阶段五-LinuxARM64.md) / [`plans/002-阶段六-标准库.md`](plans/002-阶段六-标准库.md) / [`plans/002-阶段七-自举.md`](plans/002-阶段七-自举.md)

### 5.2 实施时的注意事项
- E2E先行：每个功能必须先写E2E测试
- 禁止skip()：所有测试必须真正运行
- 编译产物放在target/目录
- 每个源文件不超过1000行，每个函数不超过100行
- **代码添加中文注释，但标识符用英文**（GCC 7限制 + 用户英文API要求）
- 完成后更新plans目录中的计划文档（打勾标记已完成）
- **本机构建命令**：见上文3.2
- ctest 的 gtest_discover_tests 在此LLVM定制版gtest下只显示e2e，单元测试直接运行 `target/cn_unit_tests` 验证
- 测试名必须英文（`TEST(SuiteName, TestName)`），中文测试名会编译失败

## 六、踩过的坑绝对不要再踩

### 6.1 环境与工具链（本机arm64麒麟，重要！）
1. **系统无 g++**：必须用 `~/gcc7/usr/bin/g++`（便携工具链）
2. **无外网**：不要用 FetchContent 拉取依赖
3. **GCC 7 不支持中文标识符**：标识符（含gtest测试名）必须英文
4. **pkill 自匹配**：`pkill -f` 会匹配到自身命令，用 `pgrep -af` 先列PID再精确kill
5. **测试二进制位置**：在 `target/cn_unit_tests`（不是 target/Debug/）
6. **多字节UTF-8字符不能与 char 比较**：'正' 等中文是多字节字符，`str[i] == '正'` 触发 -Werror=multichar，必须用 `str.compare(0, N, "正") == 0`

### 6.2 语法分析器实现经验（继承）
1. **synchronize() 必须保证前进**：错误恢复时若停在边界字符（} ; EOF）会死循环
2. **parseStmt() 必须分发 LeftBrace**：嵌套代码块是合法语句
3. **头文件声明与实现必须严格一致**
4. **`0 ++i` 词法歧义**：字面量后紧跟 `++` 会被解析为后缀自增，测试需用分号分隔
5. **可选分号**：consumeSemicolon() 只消费不报错，兼容两种风格

### 6.3 词法分析器经验（继承）
1. `peekNext(n)` 必须先跳过当前位置（`i <= n`）
2. 运算符贪婪匹配顺序：三字符 > 双字符 > 单字符
3. 字符串Token的value保留完整原始文本

### 6.4 语义/IR实现经验（新增）
1. **两趟处理**：语义分析必须先注册全部函数符号再检查函数体，否则前向调用（先调用后定义）报错
2. **CN关键字不能作标识符**：`结果`/`变量`/`选择` 等53个关键字作变量名会解析失败，测试源码避免使用
3. **IR基本块必须终止**：函数末尾无终止指令时补默认返回，否则后续代码生成器无法处理
4. **生成器类实现全部虚函数**：继承 AstVisitor 后必须实现所有纯虚函数，遗漏导致链接错误（undefined reference 到 vtable）

### 6.5 文档维护经验（新增）
1. **大型计划文档拆分要点**：按阶段边界切割（`### Task` 标题行处），切分后每子文件头部带"前序文档/后续文档"双向链接
2. **勾选框/历史备注易误删**：拆分文档时 `[x]` 勾选框与"已实现/已修复"备注块极易在复制粘贴中丢失（本次 88 个勾选框全部保留）；后续拆分文档必须逐段核对
3. **原文件改写为索引而非删除**：保留原文件名作导航索引（≤200行），避免破坏既有链接/引用

---

**当前测试状态**：181/181 单元测试通过（common 12 + lexer 37 + parser 60 + semantic 33 + ir 17 + codegen 10 + runtime 12）+ E2E 1/1 通过，编译零警告。

---

## 七、新环境恢复指南（换电脑必读）

> 2026-08-12 因更换电脑执行此交接。新机器上按以下步骤恢复开发。

### 7.1 克隆与远程配置

```bash
# 克隆仓库（默认 develop 分支）
git clone -b develop https://gitcode.com/ChenChao_GitCode/CN_Language_C.git
cd CN_Language_C

# 远程名称为 origin（指向 gitcode），如习惯用 gitcode 名称可添加：
git remote add gitcode https://gitcode.com/ChenChao_GitCode/CN_Language_C.git
# 之后推送：git push gitcode develop
```

### 7.2 新机器环境准备（重要！原机是 arm64 麒麟）

1. **GCC 7 便携工具链**：原机在 `~/gcc7/usr/bin/g++`，新机器需自行准备 GCC（≥7，支持 C++17）。若新机是 x86_64，可直接用系统 g++（注意：本项目目标是生成 x64 汇编，x86_64 开发机更合适）
2. **googletest 本地源码**：原机在 `~/third-party/unittest/googletest`（LLVM定制版，需 `-DGTEST_NO_LLVM_SUPPORT`）。新机器克隆后放在相同路径，或改 `CMakeLists.txt` 中的路径
3. **NASM（Task 1.7+ 需要）**：`sudo apt install nasm`（生成 x64 汇编后汇编用）
4. **无外网环境**：不要用 FetchContent；所有依赖本地化

### 7.3 构建与测试（新机器验证）

```bash
# 方式A：系统自带 g++（推荐，若满足 C++17）
cmake -B target/Debug -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build target/Debug --config Debug -j$(nproc)

# 方式B：便携工具链（与原机一致）
export PATH=$HOME/gcc7/usr/bin:$PATH
cmake -B target/Debug -S . -DCMAKE_CXX_COMPILER=$HOME/gcc7/usr/bin/g++
cmake --build target/Debug --config Debug

# 运行单元测试（直接运行二进制；ctest 的 gtest_discover_tests 在此版 gtest 下只显示 e2e）
./target/cn_unit_tests

# 运行 E2E
cd target/Debug && ctest -C Debug --output-on-failure
```

### 7.4 项目当前进度速览（给新会话）

| 阶段 | 任务 | 状态 |
|------|------|------|
| 阶段零 | Task 0.1~0.3（工程骨架/CLI/诊断/E2E框架） | ✅ 完成 |
| 阶段一 | Task 1.1~1.6（Token/词法/AST/语法/语义/IR） | ✅ 完成（181测试） |
| 阶段一 | Task 1.7 X64代码生成器 | ✅ 完成 |
| 阶段一 | Task 1.8 运行时库 | ✅ 完成 |
| 阶段一 | Task 1.9 Hello World E2E | ✅ 完成（里程碑） |
| 阶段一 | Task 1.10 全命令集成 + 生产审查8BUG修复 | ✅ 完成 |
| 阶段二~七 | 控制流完善/函数/类型系统/数组/结构体/OOP/模块/优化 | ⬜ 未开始 |

**编译流水线已全链路打通**：`Lexer → Parser → SemanticAnalyzer → IRGenerator → X64CodeGenerator → ml64 → link → .exe`，7 个 CLI 命令（build/compile/run/check/ir/ast/token）全部可用。

### 7.5 关键文件索引（新会话快速定位）

| 模块 | 头文件 | 实现 | 测试 |
|------|--------|------|------|
| Token | [`token.hpp`](src/cn_compiler/lexer/token.hpp) | [`token.cpp`](src/cn_compiler/lexer/token.cpp) | [`test_token.cpp`](tests/unit/lexer/test_token.cpp) |
| 词法 | [`lexer.hpp`](src/cn_compiler/lexer/lexer.hpp) | [`lexer.cpp`](src/cn_compiler/lexer/lexer.cpp) | [`test_lexer.cpp`](tests/unit/lexer/test_lexer.cpp) |
| AST | [`ast.hpp`](src/cn_compiler/parser/ast.hpp) | [`ast.cpp`](src/cn_compiler/parser/ast.cpp) | [`test_ast.cpp`](tests/unit/parser/test_ast.cpp) |
| 语法 | [`parser.hpp`](src/cn_compiler/parser/parser.hpp) | [`parser.cpp`](src/cn_compiler/parser/parser.cpp) | [`test_parser.cpp`](tests/unit/parser/test_parser.cpp) |
| 语义 | [`semantic.hpp`](src/cn_compiler/semantic/semantic.hpp) | [`semantic.cpp`](src/cn_compiler/semantic/semantic.cpp) | [`test_semantic.cpp`](tests/unit/semantic/test_semantic.cpp) |
| IR | [`ir.hpp`](src/cn_compiler/ir/ir.hpp) | [`ir.cpp`](src/cn_compiler/ir/ir.cpp) | [`test_ir.cpp`](tests/unit/ir/test_ir.cpp) |
| CLI | [`cn_main.cpp`](src/cn_main.cpp) | - | - |
| 计划 | [`plans/002`](plans/002%20CN语言编译器实施计划.md) | - | - |

### 7.6 交接注意事项

1. **Rust参考/ 目录已从工作区删除**（该目录内容已在 `.ai-coder/skills/cn-language-spec/` 规范化），git 已记录删除，新克隆不会包含
2. **`.ai-coder/memory/memory.db` 是 AI 记忆库**，已加入 .gitignore，不随 git 传递；新机器从零开始积累记忆
3. **实施计划示例用中文API，实际代码用英文API**（用户明确要求），新代码一律英文API + 中文注释
4. **测试名必须英文**：`TEST(Suite, 中文名)` 在 GCC 7 下编译失败
5. **所有标识符英文，中文仅用于注释/字符串/输出**
