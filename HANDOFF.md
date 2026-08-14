# HANDOFF - CN语言编译器项目交接文档

> **交接原因**：2026-08-14 会话结束（**阶段6「标准库」第1步已完成 ✅**——条件编译（Task 6.6）+ 数学库（Task 6.3）全链路实现：Preprocessor 条件编译预处理器（#定义/#如果定义/#否则/#结束如果 + 命令行 -D 宏注入）、math_api.cpp 8 个数学运行时函数（__cn_sqrt/pow/sin/cos/tan/fabs/ceil/floor）、semantic 注册数学.xxx 内置、ir 名称映射、stdlib/数学.cn 模块。单测 987/987、E2E 30/30 全通过，构建 0 错误 0 警告，已推送 gitcode develop（d61badb））。
>
> **前序里程碑**：阶段3「OOP 与错误处理」（901/901 单测、28/28 E2E）；阶段A「Linux ARM64」（918/918 单测、28/28 E2E）；阶段B 优化补全（949/949 单测、28/28 E2E）；阶段C 寄存器分配/调试信息（964/964 单测、28/28 E2E）；reg_alloc -O2 崩溃缺陷修复（965/965 单测、28/28 E2E）；字符串转换函数命名优化（965/965 单测、28/28 E2E）；打印函数族命名统一方案C（965/965 单测、28/28 E2E）；**阶段6第1步：条件编译+数学库（987/987 单测、30/30 E2E，本交接）**。

## 一、我们在做什么任务

正在开发CN语言编译器——一门全中文语法的系统级编程语言，参考C++编程范式，用C++17从零编写编译器，直接生成汇编代码。当前处于**阶段6「标准库」**（plans/002-阶段6-标准库.md）：第1步（条件编译+数学库）已完成，剩余 Task 6.1 核心库、Task 6.2 IO库、Task 6.4 集合库、Task 6.5 字符串处理库（格式化）。

**本次任务**：阶段6第1步子任务——① 条件编译（Task 6.6）：`#定义`/`#如果定义`/`#否则`/`#结束如果` 指令 + 命令行 `-D 宏名` 注入；② 数学库（Task 6.3）：8 个数学函数（平方根/幂/正弦/余弦/正切/绝对值/向上取整/向下取整）全链路（运行时/语义/IR/codegen/标准库模块/E2E）。

## 二、已经完成了什么

### 2.0 本轮核心任务：条件编译（Task 6.6）+ 数学库（Task 6.3）（已完成 ✅，2026-08-14）

**条件编译（Task 6.6）**：

| 层 | 文件 | 修改 |
|----|------|------|
| 新增 | [`preprocessor.hpp/cpp`](src/cn_compiler/lexer/preprocessor.cpp) | 条件编译预处理器：逐行裁剪（不活跃行替换为空白保留换行→行号一致）、条件栈（parentActive/branchTaken/active）支持任意嵌套、父层不活跃时内层 #定义 不生效（C 语义） |
| 词法 | lexer.hpp/cpp | tokenize 先经 Preprocessor 裁剪再切分；构造支持命令行宏集合 |
| CLI | cn_main.cpp | 新增 `-D 宏名` 选项（可多次）；DriverOptions.macros 透传 |
| 链路 | driver.cpp/driver_module.cpp/module.cpp | 宏集合传参（runPipeline/runModulePipeline/parseSourceText） |
| 单测 | tests/unit/lexer/test_preprocessor.cpp | 13 用例：分支选择/嵌套/行号保留/错误诊断 |
| E2E | tests/e2e/24_preprocessor/ | 条件编译.cn + .expected（源码内 #定义 + 嵌套 + 裁剪非法语法不报错） |

**数学库（Task 6.3）**：

| 层 | 文件 | 修改 |
|----|------|------|
| 新增 | src/runtime/math_api.cpp | 8 个数学运行时函数（英文 API）`__cn_sqrt/pow/sin/cos/tan/fabs/ceil/floor`，封装 `<cmath>` |
| 运行时 | runtime.hpp | 8 个数学 API 声明 |
| 语义 | semantic.cpp | registerBuiltins 注册 8 个中文限定名内置函数（`数学.平方根` 等）；visitCallExpr 模块限定重写特判 |
| IR | ir.cpp | 名称映射（8 处 `数学.xxx`→`__cn_xxx`）+ 结果类型映射（f64） |
| 模块 | stdlib/数学.cn | 新建：公开 8 个中文函数（平方根/幂/正弦/余弦/正切/绝对值/向上取整/向下取整） |
| 构建 | cn_main.cpp | compileRuntime 源文件数组 + linkExe .obj 列表加入 math_api（x64/arm64 同步） |
| 单测 | tests/unit/runtime/test_math_api.cpp | 9 用例：8 函数数值正确性 + 组合一致性 |
| E2E | tests/e2e/25_math/ | 主.cn + .expected（11 行输出，直接内置直调） |

**验证结果**：构建 0 错误 0 警告（MSVC /W4 /WX）；单元测试 987/987（965+22 新增）；E2E 30/30（28+24_preprocessor+25_math）。

### 2.1 本轮修复的关键缺陷（✅ 已修复）

| # | 缺陷 | 现象 | 修复 |
|---|------|------|------|
| 1 | **CRLF 宏名带 `\r`** | `-D DEBUG` 注入后 `#如果定义 DEBUG` 判定未命中（宏名提取为 `DEBUG\r` len=6） | extractMacroName 终止字符集合补 `\r`/`\n` |
| 2 | **指令行前导空白** | 缩进的 `#如果定义` 被当未知指令 | parseDirective 先跳过前导空白再找 `#` |
| 3 | **中文字节长度** | `compare(0,5,"定义")` 只比 5 字节恒失败 | 用 `kDefine.size()`（6 字节）等完整字节长度，先比较长关键字 |
| 4 | **数学.xxx 与用户模块冲突** | 27_module `数学.平方根` 被误映射为内置 `__cn_sqrt` 链接失败 | 用户模块公开函数优先；无同名用户函数才走内置限定名 |
| 5 | **25_math 死循环** | 导入 数学.cn 后模块函数体内 `数学.平方根` 重写为纯名自递归 | E2E 改内置直调（stdlib/数学.cn 仅供导入使用） |
| 6 | **math_api.obj 缺失** | 链接 LNK2019/LNK1181（linkExe 硬编码 4 个 .obj） | compileRuntime 数组 + linkExe .obj 列表两处加 math_api |

### 2.2 前序里程碑（已全部完成 ✅）

- 阶段0~3 全部（901/901 单测、28/28 E2E、5 项缺陷全修复）
- 阶段4 优化与 Win x64 完善（949/949 → 964/964 单测）
- 阶段5「Linux ARM64」阶段A（ARM64 后端 + 本机全链路，918/918 单测）
- reg_alloc -O2 崩溃缺陷修复（965/965 单测、28/28 E2E）
- 字符串转换函数命名优化（965/965 单测、28/28 E2E）
- 打印函数族命名统一方案C（965/965 单测、28/28 E2E）

## 三、当前测试基线

| 指标 | 数值 |
|------|------|
| 单元测试 | **987/987**（条件编译 13 + 数学库 9 新增；Win x64） |
| E2E | **30/30**（含 24_preprocessor、25_math；Win x64） |
| 编译警告 | 0（MSVC /W4 /WX） |

## 四、关键架构约定（必须遵守）

1. **IR OOP 指令契约**：NewObject.extra=`类名|大小字节`；VirtualCall.extra=`类名.虚方法名`（operand[0]=this）；VtableAddr.extra=类名；DeleteObject.extra=类名
2. **方法链接符号 = 类名$sigKey**（sigKey=名#参数串）；虚表 dq/.quad 项与 classMethodSymbol 一致
3. **重写方法虚分派判据 = classVtableIndex >= 0**（不是 isVirtual）
4. **自身/父类/-> 一律剥指针取类类型**
5. **析构 name 规范为 ~类名**；DeleteObject 沿继承链解析实际析构名
6. **this 是第一参数**（静态无 this）；VirtualCall this 占 operand[0]
7. **i128 双槽约定**：字段偏移/stride/LoadPtr/StorePtr 按 16 字节；%vN=高64、%vN+1=低64（低地址槽）
8. **Win x64 ABI**：隐藏返回指针占 rcx；this 参与 paramOffset
9. **类实例布局**：`[vtable指针(8B) | 父类成员区 | 自身成员区]`；无虚函数类 hasVtable=false
10. **结果/可选降级**：`结果<T,E>` → 合成结构体；`可选<T>` → 合成结构体
11. **泛型实例化符号**：类名$实参；单模块文件（非 主）视为入口
12. **类对象赋值深拷贝**：禁止浅拷贝 Store 源指针（RAII double free）
13. **ARM64 栈帧**：%vN → [x29,#-8N-8]；隐藏返回指针占 x0 用 x19 保存；栈参数偏移 = 16 + 16*needHiddenRet；栈槽 |offset|>255 用 x13 兜底
14. **ARM64 符号**：nameMangle = `_` 前缀 + UTF-8 hex（去 ?..@@Y）；块标签 L+hex + 函数级前缀 `L<函数符号>_`；常量标签 LstrN/LfpN
15. **ARM64 寄存器策略**：x9/x10/x11 临时、x12 拷贝计数、x13 地址计算、x16 间接调用目标、x19 隐藏返回指针
16. **打印函数族语义**：`打印(...)` = 结尾换行；`打印行(...)` = 不换行（"行"=逐行连续输出，不追加换行符）；`格式化(...)` = 格式化字符串。仅支持标量+字符串+字符变参。运行时 `printLine*` 符号保留但编译器不再映射
17. **字符串转换函数映射**：CN 公开名 → 运行时符号（`__cn_str_from_*`），映射集中在 semantic.cpp regStrFn + ir.cpp 名称映射 + ir.cpp 结果类型映射三处
18. **条件编译指令**：`#定义`/`#如果定义(宏)`（或空格形式）`/#否则`/`#结束如果`，行级裁剪保留行号，命令行 `-D 宏名` 注入；宏名收集终止字符集合必须含 `\r`（CRLF）
19. **数学库内置函数**：注册为带 `数学.` 前缀限定名（`数学.平方根`），用户模块公开函数优先；`stdlib/数学.cn` 仅供导入使用（体内 `数学.xxx` 直调会触发用户函数自递归——E2E 用内置直调）
20. **运行时 .obj 清单**：cn_main.cpp compileRuntime 源文件数组 + linkExe .obj 列表必须同步（新增运行时源文件两处都要加）

## 五、踩过的坑（绝对不要再踩）

1. **打印函数族命名"行"字语义已反转**（权重15.0，✅已修复）：`打印行`=不换行（逐行连续输出语义），勿再按直觉理解为换行
2. **CRLF 宏名带 `\r`**（新增，✅已修复）：逐行按 `\n` 分割时行内容含 `\r`（Windows CRLF），凡"终止字符集合"收集标识符必须含 `\r`/`\n`
3. **中文字节长度比较**（新增，✅已修复）：`compare(0,N,"中文")` 的 N 必须是 `"中文".size()`（字节数），先比较较长关键字避免前缀截断
4. **内置函数与模块函数冲突**（新增，✅已修复）：带 `模块.函数` 限定名的内置函数，用户模块公开函数优先；`stdlib/数学.cn` 模块体内不能直调同名内置（自递归）
5. **运行时新增源文件两处同步**（新增，✅已修复）：compileRuntime 源文件数组 + linkExe .obj 列表（x64/arm64 两平台各一处）
6. **DeleteObject 析构符号**：子类无自身析构时符号是 `子类$~父类析构名`，codegen 硬编码 `~类名` 链接失败
7. **MASM .rdata**：ml64 不认 `.rdata`（A2008），用 `.const`；**GAS 相反**：`.section .rodata`
8. **GAS `@` 前缀**：`@str0` 标签非法（@ 是注释符）→ 用 `Lstr0`；块标签需函数级唯一前缀
9. **GAS 注释**：`#` 与立即数前缀冲突 → 用 `//`
10. **LP64 类型别名**：`std::uint64_t` = `unsigned long`（Linux）vs `unsigned long long`（runtime.hpp 声明）→ extern "C" 重声明冲突，统一 `std::uint64_t`
11. **unordered_map 遍历顺序**：funcFirstSigKey/函数名作值依赖哈希顺序 → 字典序确定性选择
12. **gtest 与运行时 main 冲突**：Linux 下 runtime.cpp 的 main 与 gtest_main 冲突 → CNRT_LINUX_MAIN 宏控制
13. **GCC 7 严格警告**：unused-function/unused-parameter/parentheses → `[[maybe_unused]]`、括号、`(void)param`
14. **MSBuild 增量构建坑**：apply_diff 修改后须 `--clean-first` 强制重建（CMake 同样）
15. **i128 返回 epilogue**：x19 保存的隐藏返回指针；恢复时仅 needHiddenRet 才 ldp x19（否则读垃圾）
16. **常量返回文本**：termReturnValue="0"/"1"（非 %vN）不能 substr(2) 解析 → 防御分支
17. **movz/movk**：0 块应跳过（movz 已清零），否则输出多余 movk #0
18. **中文字符串前缀匹配**：`str[i] == '中文字符'` 非法（UTF-8 多字节）→ 用 `s.compare(0, prefix.size(), "前缀") == 0` 字节语义
19. **变参展开参数类型必须与运行时函数签名一致**（权重8.5，✅已修复）：f32 必须 Cast 到 f64 再传参
20. **f32→f64 常量折叠精度**（权重9.1，✅已修复）：先按 f32 精度截断再提升
21. **删除内置函数要全链路同步检查**：语义注册、x64/arm64 codegen 映射、E2E `.cn` 与 `.expected`、单元测试、规范文档、lessons.md、更新日志

## 六、下一步计划

1. **阶段6「标准库」后续任务**（plans/002-阶段6-标准库.md）：
   - Task 6.1 核心库（stdlib/核心.cn：列表/字典/集合）
   - Task 6.2 IO库（stdlib/IO.cn + io_api.cpp 读取行/读取整数/读取浮点）
   - Task 6.4 集合库（stdlib/集合.cn：表.包含等）
   - Task 6.5 字符串处理库（stdlib/字符串.cn：长度/比较/连接/复制/查找/格式化）
   - 数学库 P1：对数/反三角/随机数（本次未做）
2. **阶段5 阶段B/C**（plans/002-阶段5-LinuxARM64.md）：交叉编译/QEMU 验证、ARM64 优化对齐
3. **遗留风险**：
   - `stdlib/数学.cn` 模块函数体内 `数学.xxx` 直调会与同名内置冲突（用户模块函数优先导致自递归）——设计上仅供外部导入使用；如需模块内自调用应改用 `从 数学 导入` 或内置直调
   - `打印`/`打印行`/`格式化` 变参仅支持标量+字符串+字符，指针/结构体传参会被语义层拒绝
   - 接口附加 vtable 预留（多接口场景，规格书06-五 预留，未实现）
   - ARM64 E2E 在 x86 主机需交叉工具链/QEMU
   - `~/gcc7` 便携工具链是本机 g++ 唯一来源（PATH 无系统 g++），cn_main 已自动探测
   - **arm64 寄存器分配保守默认关闭**：结果写回点分散于 35 处 emitStackStore，默认全栈帧，待写回点收敛后再启用
   - **x64_instructions.cpp 存量超行数**：单文件超 1000 行约束，待后续拆分

## 七、关键文件索引

| 模块 | 文件 |
|------|------|
| 条件编译（新增） | [`src/cn_compiler/lexer/preprocessor.hpp/cpp`](src/cn_compiler/lexer/preprocessor.cpp)（指令解析/条件栈/CRLF 处理）、[`lexer.cpp`](src/cn_compiler/lexer/lexer.cpp:603)（tokenize 预处理接入）、[`cn_main.cpp`](src/cn_main.cpp:107)（-D 选项）、driver/module 宏传参链路 |
| 数学库（新增） | [`src/runtime/math_api.cpp`](src/runtime/math_api.cpp)（8 函数）、[`semantic.cpp`](src/cn_compiler/semantic/semantic.cpp:763)（registerBuiltins 数学注册）、[`semantic.cpp`](src/cn_compiler/semantic/semantic.cpp:1788)（模块限定重写特判）、[`ir.cpp`](src/cn_compiler/ir/ir.cpp:2825)（数学名称映射）、[`stdlib/数学.cn`](stdlib/数学.cn) |
| 运行时 .obj 清单 | [`cn_main.cpp`](src/cn_main.cpp:354)（compileRuntime 数组）、[`cn_main.cpp`](src/cn_main.cpp:399)（linkExe .obj 列表） |
| 单测 | tests/unit/lexer/test_preprocessor.cpp、tests/unit/runtime/test_math_api.cpp |
| E2E | tests/e2e/24_preprocessor/、tests/e2e/25_math/ |
| 计划文档 | plans/002-阶段6-标准库.md（Task 6.3/6.6 已打勾 ✅） |
