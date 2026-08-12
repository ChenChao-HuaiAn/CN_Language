# lessons.md - AI错误记录与经验教训

> 本文件记录项目开发中遇到的问题、原因与解决方案，防止重复犯错。
> 权重规则：≥15 必须提及；10-14 应该阅读；5-9 可选；<5 可跳过。

## 高权重问题（必须避免）

- [2026-08-12 17:59] **问题类型**: 集成问题（权重 27.3）
  - **描述**: 换电脑（Linux GCC7 → Windows MSVC）后 Task 1.7 构建失败：1) `ir.hpp` 的 `IRFunction`/`IRModule` 含 `vector<unique_ptr>`，MSVC traits 探测实例化隐式拷贝构造触发 C2280；2) `test_lexer.cpp` raw string 内 `\"` 序列 MSVC 报 C2017 非法转义；3) 测试变量名 `asm` 是 MSVC 关键字报 C2059；4) cmake 不在 PATH，需用 VS 自带完整路径；5) CMakeLists gtest 本地源码回退路径指向不存在的 `~/third-party`，实际缓存在 `target/build/_deps`
  - **原因**: 项目原在 Linux GCC7 开发，未覆盖 MSVC 编译差异（unique_ptr容器拷贝/raw string转义/MSVC关键字/工具链路径）
  - **解决**: 1) `ir.hpp` 显式移动语义（拷贝删除）；2) raw string 含 `\"` 改普通字符串；3) 变量避免用 `asm` 等 MSVC 关键字；4) 用 VS 自带 cmake 完整路径；5) gtest 回退路径增加 `target/build/_deps/googletest-src/googletest` 检查
  - **预防**: 新代码避免 `asm`/`cl` 作标识符；raw string 含引号转义用普通字符串；含 unique_ptr 的容器成员类显式移动语义；构建前先确认工具链与依赖缓存路径
  - **权重**: 27.3（集成问题7 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-12 16:44] **问题类型**: 工具执行错误（权重 15）
  - **描述**: `TEST(SemanticTest, 递归函数)` 中文测试名导致编译失败：`expected initializer before ':' token`（GCC 7 不支持中文标识符）
  - **原因**: GCC 7 的 gtest 宏 `GTEST_TEST_` 用测试名生成 C++ 类名/函数名，中文标识符不合法；注释中的中文虽可编译，但宏展开的标识符不行
  - **解决**: 全部测试名改为英文（如 `TEST(SemanticTest, RecursiveFunction)`），中文仅用于注释/字符串
  - **预防**: 新建测试文件时测试名一律英文；标识符（变量/函数/类/枚举）一律英文

- [2026-08-12 16:44] **问题类型**: 工具执行错误（权重 12）
  - **描述**: `from[0] == '正'` 触发 `-Werror=multichar` 和 `-Werror=type-limits` 编译失败
  - **原因**: '正' 是 UTF-8 多字节字符（3字节），不能与单字节 char 比较；GCC 将多字符常量视为警告并在 -Werror 下报错
  - **解决**: 改用 `from.compare(0, 3, "正") == 0` 字符串前缀比较
  - **预防**: 任何中文字符比较必须用 std::string 的 compare/substr，禁止 `str[i] == '中文字符'`

- [2026-08-12 16:44] **问题类型**: 集成问题（权重 10）
  - **描述**: IRGenerator 继承 AstVisitor 后未实现 `visitIfStmt/visitWhileStmt/visitForStmt`，链接报 `undefined reference to ... vtable`
  - **原因**: 三个虚函数在头文件声明但实现遗漏，vtable 引用缺失
  - **解决**: 补充实现，转发到 genIf/genWhile/genFor
  - **预防**: 继承 AstVisitor 后逐一核对所有纯虚函数是否实现；链接错误先看 undefined reference 到哪个类

- [2026-08-12 16:44] **问题类型**: 逻辑错误（权重 8）
  - **描述**: 测试中 `变量 结果 = 阶乘(5)` 解析失败："预期变量名，实际为 '结果'"
  - **原因**: `结果`（Kw_Result）是 CN 语言53个保留关键字之一（结果<T,E>类型），不能作标识符
  - **解决**: 测试源码改用 `答案` 等非关键字变量名
  - **预防**: 编写 CN 测试代码时避免使用53个关键字作变量/函数名（结果/变量/选择/情况/默认等）

## 高权重问题（Task 1.9 新增）

- [2026-08-12 18:22] **问题类型**: 集成问题（权重 24.6）
  - **描述**: Task 1.9 `cn build` 链接阶段 LNK2019 无法解析 CRT 符号：1) `/ENTRY:WinMain` 时 link 不自动注入 CRT 默认库，puts/printf/malloc 等全部未解析；2) 改显式 `/DEFAULTLIB:ucrt.lib` 后仍缺 `memcpy/memset`（它们在静态 `libucrt.lib`，导入库 `ucrt.lib` 无定义）；3) 即使链接成功，`/ENTRY:WinMain` 跳过 CRT 启动代码，stdout 未初始化导致 `printLine`（puts）输出为空
  - **原因**: 自定义 `/ENTRY` 绕过了 CRT 启动（mainCRTStartup/WinMainCRTStartup），需自行注入默认库；puts 依赖 CRT 初始化 stdout
  - **解决**: 链接命令改为 `link /ENTRY:WinMainCRTStartup /SUBSYSTEM:CONSOLE /DEFAULTLIB:libcmt.lib /DEFAULTLIB:libucrt.lib /DEFAULTLIB:kernel32.lib`（CRT 初始化 stdout 后调用用户 WinMain）
  - **预防**: 自定义入口时必须用 `WinMainCRTStartup`（而非 WinMain）；静态 CRT 配套 `libucrt.lib` 而非 `ucrt.lib`；显式 `/DEFAULTLIB` 注入 CRT/内核库
  - **权重**: 24.6（集成问题7 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-12 18:22] **问题类型**: 配置错误（权重 15.6）
  - **描述**: ml64 汇编 UTF-8 文件失败 A2044（invalid character in file）：1) 带 UTF-8 BOM 的 asm 文件从第1行起全部 A2044；2) `db "你好，世界"` 中文原始 UTF-8 字节触发 A2044；3) 中文块标签 `块0:` 也是 A2044
  - **原因**: 此 ml64 版本不识别 UTF-8 BOM，且把原始 UTF-8 中文字节当非法字符；仅 UTF-8 无 BOM 的纯 ASCII 代码 + 中文注释可接受
  - **解决**: 1) asm 写 UTF-8 无 BOM；2) 字符串常量改用十六进制字节 `db 0E4h,0BDh,0A0h,0`（代码生成器 `hexBytesString()`）；3) IR 块标签 块N→bbN（ml64 不识别中文标识符）
  - **预防**: 生成汇编一律 UTF-8 无 BOM；中文内容只出现在注释和 `db` 十六进制中；符号/标签必须 ASCII
  - **权重**: 15.6（配置错误6 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

- [2026-08-12 18:22] **问题类型**: 集成问题（权重 13.7）
  - **描述**: 语义分析器报"未声明的函数 '打印行'"——`打印行` 是 CN 内置函数，但 SemanticAnalyzer 未注册其符号，visitCallExpr 找不到
  - **原因**: 阶段一语义分析器只注册源码中的函数声明，未内置 IO 函数符号表
  - **解决**: semantic.cpp 新增 `registerBuiltins()`，在 visitProgram 第零趟注册 `打印行(字符串)->空类型`/`打印行整数(整64)`/`打印行浮点(浮64)`；代码生成器 `symbolName()` 映射到运行时 `printLine` 系列
  - **预防**: 新增内置函数必须同时注册语义符号表 + 代码生成器符号映射 + 运行时 extern "C" 导出三处
  - **权重**: 13.7（集成问题7 × 详细分析2.0 × 解决方案1.5 × 已解决1.0）

- [2026-08-12 18:22] **问题类型**: 工具执行错误（权重 3.9）
  - **描述**: MSVC 将 `getenv` 视为不安全函数，C4996 警告在 /WX 下报 C2220 错误
  - **原因**: MSVC 建议用 `_dupenv_s`，/W4 /WX 把 C4996 提升为错误
  - **解决**: 封装 `getEnvVar()` 用 `_dupenv_s` 实现
  - **预防**: Windows 代码避免直接 getenv/strcpy 等弃用函数，用 _s 安全版
  - **权重**: 3.9（工具执行错误2 × 详细分析2.0 × 解决方案1.5 × 已解决1.0）

## 高权重问题（生产环境审查 Task 1.10 新增，2026-08-12 19:04）

- [2026-08-12 19:04] **问题类型**: 设计缺陷（权重 31.9）
  - **描述**: 生产环境模拟发现编译器8个BUG：1)语义层 `整数`/`小数` 类型别名未规范化，`整数 x = 10` 报"无法将 '整32' 隐式转换为 '整数'"；2) x64_codegen 未登记局部变量槽，全部变量访问生成 `[rbp0]`（A2006 undefined symbol: rbp0）；3) `idiv [rbp-X]` 内存操作数缺 size 前缀（A2023）；4) Win x64 调用约定违反：调用侧未预留32字节影子空间、接收侧栈参偏移 `[rbp+40]` 错位（应 `[rbp+48]`）；5) IR 层变量遮蔽失效：`varRegs_` 平铺映射被内层同名覆盖，块外读错值；6) i32 参数传 `printLineInt(long long)` 未符号扩展，负数变巨大正数（4294967291）；7) `--output` 深层目录不自动创建（LNK1104）；8) driver 流水线 IR/codegen 阶段未检查 `diagnostics.hasErrors()` 仍返回0
  - **原因**: 类型别名用字符串相等比较、槽登记遗漏 Alloca、MASM 内存操作数需显式 size、调用约定实现未按 MS ABI（影子空间+栈参布局）、作用域用平铺 map、C ABI 整型参数需64位+符号扩展、路径处理未建目录、错误检查遗漏
  - **解决**: 1) `canonicalType()` 规范化（整数→整32/小数→浮64），declareVar/registerFunction/canConvert/commonNumericType 统一使用；2) `generateFunctionAssembly` 扫描全部 Alloca 登记变量槽；3) `idiv dword ptr/qword ptr [rbp-X]`；4) `emitCall` 一次性 `sub rsp,(32+栈参+对齐)` 预留影子空间，接收侧 `[rbp+48+(i-4)*8]`；5) IR 作用域栈 + 唯一内部名 `name$N`，func.params 保留源码名 + paramUniques 存唯一名；6) 传参前 `movsxd` 符号扩展 i32→i64；7) `ensureDirExists()` 逐级创建目录，中间文件与 exe 同目录；8) runPipeline 补 IR/codegen 后 hasErrors 检查
  - **预防**: 类型别名必须统一规范化后再比较；Alloca 必须登记槽；MASM 内存操作数带 size 前缀；Win x64 调用必须预留影子空间+16对齐+符号扩展；作用域必须用栈结构；`--output` 路径必须自动建目录；流水线每个阶段后检查错误
  - **权重**: 31.9（设计缺陷10 × 详细分析2.0 × 解决方案1.5 × 预防措施1.3 × 已解决1.0）

## 既有经验（继承自 HANDOFF，权重 10）

- **GCC 7 不支持中文标识符**：所有标识符（含 gtest 测试名）必须英文，中文仅用于注释/字符串
- **无外网**：googletest 用本地 `~/third-party/unittest/googletest`（LLVM定制版，需 `-DGTEST_NO_LLVM_SUPPORT`）
- **本机构建命令**：`export PATH=$HOME/gcc7/usr/bin:$PATH && cmake -B target/Debug -S . -DCMAKE_CXX_COMPILER=$HOME/gcc7/usr/bin/g++`
- **编译产物在 target/**：测试二进制在 `target/cn_unit_tests`
- **pkill 自匹配**：`pkill -f` 会匹配自身 shell，先 `pgrep -af` 再精确 kill
- **synchronize() 必须前进**：错误恢复停在边界字符会死循环
- **parseStmt() 必须分发 LeftBrace**：嵌套代码块是合法语句
- **`0 ++i` 词法歧义**：字面量后紧跟 ++ 解析为后缀自增，测试需分号分隔
- **peekNext(n) 先跳过当前位置**（`i <= n`）
- **运算符贪婪匹配**：三字符 > 双字符 > 单字符
- **字符串Token保留完整原始文本**

## 实施规范（用户规则）

- 中文回复用户；代码添加中文注释但标识符英文
- 文件 ≤1000行、函数 ≤100行
- E2E先行；禁止 skip()；产物在 target/
- 编译零警告（-Wall -Wextra -Werror）
- plans 文档完成后打勾
- 提交前更新"更新日志.md"（覆盖写入本次内容）
- 会话结束前写 HANDOFF.md 交接
- 实施计划示例代码用中文API命名，但**用户明确要求英文API命名**（指令优先）
