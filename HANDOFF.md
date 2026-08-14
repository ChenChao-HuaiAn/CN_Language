# HANDOFF - CN语言编译器项目交接文档

> **交接原因**：2026-08-14 会话结束（**字符串转换函数命名优化已完成 ✅**——命名从直译"字符串从X"改为符合中文习惯的"X转字符串"：整数转字符串/浮点转字符串/字符转字符串/布尔转字符串/正数转字符串。全链路修改完成：语义注册→IR映射→单元测试→E2E→文档同步。构建 0 错误 0 警告、单测 965/965、E2E 28/28 全通过，无回归）。
>
> **前序里程碑**：阶段3「OOP 与错误处理」（901/901 单测、28/28 E2E）；阶段A「Linux ARM64」（918/918 单测、28/28 E2E）；阶段B 优化补全（949/949 单测、28/28 E2E）；阶段C 寄存器分配/调试信息（964/964 单测、28/28 E2E）；reg_alloc -O2 崩溃缺陷修复（965/965 单测、28/28 E2E）；**字符串转换函数命名优化（965/965 单测、28/28 E2E，本交接）**。

## 一、我们在做什么任务

正在开发CN语言编译器——一门全中文语法的系统级编程语言，参考C++编程范式，用C++17从零编写编译器，直接生成汇编代码。当前已完成：阶段0~3（词法→语法→语义→IR→Win x64 后端→OOP/错误处理/模块/泛型）、阶段4 优化、阶段5「Linux ARM64」阶段A（ARM64 后端 + 本机全链路）、reg_alloc 崩溃缺陷修复、字符串转换函数命名优化。下一步：**阶段6「标准库」** 或 阶段5 阶段B/C（交叉编译/QEMU 验证）、阶段4 优化级别框架收尾。

**本次任务**：字符串转换函数命名从英文直译"字符串从X"（字符串从整数/字符串从浮点/字符串从字符/字符串从布尔）改为中文动宾结构"X转字符串"（整数转字符串/浮点转字符串/字符转字符串/布尔转字符串），并新增公开函数 `正数转字符串`（正64 → `__cn_str_from_uint`，此前无符号的 `__cn_str_from_uint` 无公开入口）。

## 二、已经完成了什么

### 2.0 本轮核心任务：字符串转换函数命名优化（已完成 ✅，2026-08-14）

**改动范围（全链路同步）**：

| 层 | 文件 | 修改 |
|----|------|------|
| 语义注册 | [`semantic.cpp`](src/cn_compiler/semantic/semantic.cpp:746) | `regStrFn` 注册改名：`字符串从整数/浮点/字符/布尔` → `整数转字符串/浮点转字符串/字符转字符串/布尔转字符串`；新增公开函数 `正数转字符串`（正64 → `__cn_str_from_uint`） |
| IR 名称映射 | [`ir.cpp`](src/cn_compiler/ir/ir.cpp:2810) | CN 名 → 运行时符号映射同步：`整数转字符串` → `__cn_str_from_int`、`浮点转字符串` → `__cn_str_from_float`、`字符转字符串` → `__cn_str_from_char`、`布尔转字符串` → `__cn_str_from_bool`、`正数转字符串` → `__cn_str_from_uint` |
| IR 结果类型映射 | [`ir.cpp`](src/cn_compiler/ir/ir.cpp:2868) | `__cn_str_from_uint` 加入 ptr 结果类型（返回字符串指针） |
| 单元测试 | test_string_extra、test_ir_string_extra | CN 名调用更新为"X转字符串" |
| E2E | 6 个 E2E 文件改名 + 新增 `正数转字符串` 用例 | `字符串完善.cn` + `.expected` 同步 |
| 文档 | specs/02-类型系统.md、specs/01b、plans/001 设计规格书、阶段2-核心语言、阶段6-标准库 | 名称同步 + 补充布尔转字符串/正数转字符串 |

**验证结果**：
- 构建：0 错误 0 警告（MSVC /W4 /WX）
- 单元测试：965/965 通过
- E2E：28/28 通过（含新增 `正数转字符串` 用例）

**设计决策**：
1. 运行时符号 `__cn_str_from_int/uint/float/char/bool` **保留不变**——属内部实现细节，改名不波及代码生成与 target/*.asm，避免无谓回归
2. `正数转字符串` 为新增公开名——此前无符号的 `__cn_str_from_uint` 只有隐式拼接路径使用，无公开函数入口

### 2.1 前序里程碑（已全部完成 ✅）

- 阶段0~3 全部（901/901 单测、28/28 E2E、5 项缺陷全修复）
- 阶段4 优化与 Win x64 完善（949/949 → 964/964 单测）
- 阶段5「Linux ARM64」阶段A（ARM64 后端 + 本机全链路，918/918 单测）
- reg_alloc -O2 崩溃缺陷修复（965/965 单测、28/28 E2E、-O0/-O2/-O3 三级别 84 组合一致）

## 三、当前测试基线

| 指标 | 数值 |
|------|------|
| 单元测试 | **965/965**（字符串转换函数改名后回归通过） |
| E2E | **28/28**（含新增 `正数转字符串` 用例；Win x64） |
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
16. **字符串转换函数映射**：CN 公开名（`整数转字符串`/`浮点转字符串`/`字符转字符串`/`布尔转字符串`/`正数转字符串`）→ 运行时符号（`__cn_str_from_int/float/char/bool/uint`），映射集中在 `semantic.cpp` regStrFn 注册 + `ir.cpp` 名称映射 + `ir.cpp` 结果类型映射三处

## 五、踩过的坑（绝对不要再踩）

1. **DeleteObject 析构符号**：子类无自身析构时符号是 `子类$~父类析构名`，codegen 硬编码 `~类名` 链接失败
2. **MASM .rdata**：ml64 不认 `.rdata`（A2008），用 `.const`；**GAS 相反**：`.section .rodata`
3. **GAS `@` 前缀**：`@str0` 标签非法（@ 是注释符）→ 用 `Lstr0`；块标签需函数级唯一前缀
4. **GAS 注释**：`#` 与立即数前缀冲突 → 用 `//`
5. **LP64 类型别名**：`std::uint64_t` = `unsigned long`（Linux）vs `unsigned long long`（runtime.hpp 声明）→ extern "C" 重声明冲突，统一 `std::uint64_t`
6. **unordered_map 遍历顺序**：funcFirstSigKey/函数名作值依赖哈希顺序 → 字典序确定性选择
7. **gtest 与运行时 main 冲突**：Linux 下 runtime.cpp 的 main 与 gtest_main 冲突 → CNRT_LINUX_MAIN 宏控制
8. **GCC 7 严格警告**：unused-function/unused-parameter/parentheses → `[[maybe_unused]]`、括号、`(void)param`
9. **MSBuild 增量构建坑**：apply_diff 修改后须 `--clean-first` 强制重建（CMake 同样）
10. **i128 返回 epilogue**：x19 保存的隐藏返回指针；恢复时仅 needHiddenRet 才 ldp x19（否则读垃圾）
11. **常量返回文本**：termReturnValue="0"/"1"（非 %vN）不能 substr(2) 解析 → 防御分支
12. **movz/movk**：0 块应跳过（movz 已清零），否则输出多余 movk #0
13. **中文字符串前缀匹配**：`str[i] == '中文字符'` 非法（UTF-8 多字节）→ 用 `s.compare(0, prefix.size(), "前缀") == 0` 字节语义

## 六、下一步计划

1. **阶段6「标准库」**（plans/002-阶段6-标准库.md）：容器/数学/文件 IO/格式化/条件编译（本次已同步布尔转字符串/正数转字符串文档）
2. **阶段5 阶段B/C**（plans/002-阶段5-LinuxARM64.md）：交叉编译/QEMU 验证、ARM64 优化对齐等后续内容
3. **遗留风险**：
   - 接口附加 vtable 预留（多接口场景，规格书06-五 预留，未实现）
   - ARM64 E2E 在 x86 主机需交叉工具链/QEMU（本机已为 aarch64，全链路已验证）
   - `~/gcc7` 便携工具链是本机 g++ 唯一来源（PATH 无系统 g++），cn_main 已自动探测
   - **arm64 寄存器分配保守默认关闭**：结果写回点分散于 35 处 emitStackStore（未收敛于 resultText 单点），已提供 `setRegAllocMap` 注入 API（模块独立可用、单测充分），默认全栈帧——待写回点收敛后再启用（正确性最高优先）
   - **x64_instructions.cpp 存量超行数**：单文件超 1000 行约束，待后续拆分（收尾不改动业务代码）

## 七、关键文件索引

| 模块 | 文件 |
|------|------|
| 语义（字符串函数注册） | [`src/cn_compiler/semantic/semantic.cpp`](src/cn_compiler/semantic/semantic.cpp:746)（regStrFn 注册，含 整数转字符串 等） |
| IR（名称/结果类型映射） | [`src/cn_compiler/ir/ir.cpp`](src/cn_compiler/ir/ir.cpp:2810)（CN 名 → `__cn_str_from_*`）、[`ir.cpp`](src/cn_compiler/ir/ir.cpp:2868)（ptr 结果类型） |
| 运行时字符串 | src/runtime/string_api.cpp（`__cn_str_from_*` 符号，本次未改） |
| ARM64 后端 | src/cn_compiler/codegen/arm64/（7 文件） |
| X64 后端 | src/cn_compiler/codegen/x64/（4 文件） |
| Backend 接口 | src/cn_compiler/codegen/codegen.hpp |
| 运行时 | src/runtime/（runtime.hpp/cpp、i128_api.cpp、io_api.cpp、string_api.cpp） |
| E2E 字符串用例 | tests/e2e/12_string_extra/字符串完善.cn（含 `正数转字符串` 用例） |
| 计划文档 | plans/002-阶段2-核心语言.md、plans/002-阶段6-标准库.md（本次已同步） |
