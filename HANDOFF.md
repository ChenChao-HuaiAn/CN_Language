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
- **测试结果**：**159/159 单元测试全部通过**（common 12 + lexer 37 + parser 60 + semantic 30 + ir 17），E2E通过，编译零错误零警告

## 三、总结发现的问题

### 3.1 本阶段发现并解决的问题
1. **多字节UTF-8字符与char比较**：`from[0] == '正'` 触发 -Werror=multichar/type-limits → 改用 `from.compare(0, 3, "正") == 0` 字符串前缀比较
2. **GCC 7 不支持中文标识符（测试名）**：`TEST(Suite, 中文名)` 宏展开生成中文类名/函数名编译失败 → 测试名必须用英文，中文仅用于注释/字符串（现有 parser/lexer 测试均如此）
3. **IRGenerator 虚函数缺失**：头文件声明 `visitIfStmt/visitWhileStmt/visitForStmt` 但实现遗漏 → 链接报 undefined reference，补充转发到 genIf/genWhile/genFor
4. **`结果` 是保留关键字**：CN 语言 `结果<T,E>`（Kw_Result）不能作变量名，`变量 结果 = ...` 解析失败（"预期变量名"）→ 测试改用 `答案`

### 3.2 既有经验（HANDOFF继承）
1. **GCC 7 不支持中文标识符**：所有标识符必须英文，中文仅用于注释/字符串
2. **无外网**：googletest 用本地 `~/third-party/unittest/googletest`（LLVM定制版，需 `-DGTEST_NO_LLVM_SUPPORT`）
3. **本机构建命令**：`export PATH=$HOME/gcc7/usr/bin:$PATH && cmake -B target/Debug -S . -DCMAKE_CXX_COMPILER=$HOME/gcc7/usr/bin/g++ ...`
4. **编译产物在 target/**（CMAKE_RUNTIME_OUTPUT_DIRECTORY），测试二进制在 `target/cn_unit_tests`
5. **实施计划示例代码用中文API命名，但用户明确要求英文API命名**（"这些指令优先于任何冲突的通用模式指令"）——新代码一律英文API

## 四、当前卡在哪

无卡点。阶段一 Task 1.1~1.6 已完成并全部测试通过。下一步是 Task 1.7（X64代码生成器）。

## 五、下一步计划是什么

### 5.1 立即需要做的
执行 `plans/002 CN语言编译器实施计划.md` 阶段一：
- Task 1.7：X64代码生成器（`src/cn_compiler/codegen/`，IR→汇编文本）
- Task 1.8：运行时库（打印行等IO API）
- Task 1.9：Hello World全链路（cn build 命令 + NASM/链接器）

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

---

**当前测试状态**：159/159 单元测试通过 + E2E通过，编译零警告。

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
| 阶段一 | Task 1.1~1.6（Token/词法/AST/语法/语义/IR） | ✅ 完成（159测试） |
| 阶段一 | Task 1.7 X64代码生成器 | ⬜ 未开始 |
| 阶段一 | Task 1.8 运行时库 | ⬜ 未开始 |
| 阶段一 | Task 1.9 Hello World E2E | ⬜ 未开始 |
| 阶段二~七 | 控制流完善/函数/类型系统/数组/结构体/OOP/模块/优化 | ⬜ 未开始 |

**编译流水线已就绪**：`Lexer → Parser → SemanticAnalyzer → IRGenerator`，下一步接 `CodeGenerator`（IR→x64汇编→NASM→链接→可执行文件）。

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
