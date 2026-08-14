# HANDOFF - CN语言编译器项目交接文档

> **交接原因**：2026-08-14 会话结束（**阶段C「寄存器分配 + 调试信息 + 优化级别框架」全部完成**——线性扫描寄存器分配器 reg_alloc + debug_info 源码注释 + `--no-regalloc`/`--debug` 开关联动；-O0 与 -O2 E2E 28/28 输出一致、964/964 单测、编译零警告）。本文档写给完全没有上下文的新会话看，帮助快速恢复开发。
>
> **前序里程碑**：阶段3「OOP 与错误处理」（901/901 单测、28/28 E2E）；阶段A「Linux ARM64」（918/918 单测、28/28 E2E）；阶段B 优化补全（949/949 单测、28/28 E2E）；阶段C 寄存器分配/调试信息（本交接，964/964 单测、28/28 E2E）。

## 一、我们在做什么任务

正在开发CN语言编译器——一门全中文语法的系统级编程语言，参考C++编程范式，用C++17从零编写编译器，直接生成汇编代码。当前已完成：阶段0~3（词法→语法→语义→IR→Win x64 后端→OOP/错误处理/模块/泛型）、阶段4 优化（**阶段B 完成 SSA/LICM/强度削减/内联/TCO；阶段C 完成寄存器分配/调试信息**）、阶段5「Linux ARM64」阶段A（ARM64 后端 + 本机全链路）。下一步：**阶段6「标准库」** 或 阶段5 阶段B/C（交叉编译/QEMU 验证）、阶段4 优化级别框架收尾（arm64 寄存器分配按需启用）。

## 二、已经完成了什么

### 2.0 本轮核心任务：阶段C「寄存器分配 + 调试信息 + 优化级别框架」（已完成 ✅，2026-08-14）

**新增模块（codegen 层，规格书 Task 4.3/4.4）**：

| 文件 | 职责 | 挂载 |
|------|------|------|
| [`codegen/reg_alloc.hpp`](src/cn_compiler/codegen/reg_alloc.hpp) / [`reg_alloc.cpp`](src/cn_compiler/codegen/reg_alloc.cpp) | 线性扫描寄存器分配器：活跃区间（def-use + 跨块活跃传播）→ 按 start 扫描 → 空闲寄存器分配/溢出到栈槽 | **-O2**（x64） |
| [`codegen/debug_info.hpp`](src/cn_compiler/codegen/debug_info.hpp) / [`debug_info.cpp`](src/cn_compiler/codegen/debug_info.cpp) | 源码行号映射表 + 汇编注释（`; src:` / `// src:`）+ DWARF 扩展位预留 | `--debug` |
| `x64_codegen.hpp/.cpp` + `x64_instructions.cpp` | operandText/resultText/emitTerminator 三收敛点映射物理寄存器；被调用者保存寄存器 prologue push/epilogue pop | -O2 |
| `backend_factory.cpp` + `driver.hpp/cpp` + `cn_main.cpp` | `--no-regalloc`/`--debug` CLI 开关；createBackend 传参联动 | -O2 起启用 |

**关键设计（保守策略，正确性最高优先）**：
1. **-O0/-O1 全栈帧完全不变**（不引入回归）；-O2 起启用寄存器分配，`--no-regalloc` 可关
2. **仅 i64/u64/ptr 参与分配**（浮点/i128/小位宽保持栈槽——避免与发射器宽度假设冲突）
3. **仅被调用者保存寄存器**（x64 rbx/r12~r15；arm64 x19~x28）——后端发射器内部用调用者保存寄存器（rax/rcx/rdx/r8~r11 / x0~x18）作临时
4. **structReturn/i128/u128 返回函数强制关闭**（epilogue 提前 return 路径，被调用者保存恢复会遗漏）
5. **arm64**：提供 `setRegAllocMap` 注入 API（reg_alloc 模块独立可用），默认关闭（结果写回点分散 35 处，保守全栈帧）
6. 调试信息默认关闭（`--debug` 开启）；x64 MASM `; src:` / arm64 GAS `// src:`（`#` 与立即数冲突）

**测试**：新增 14 个单测（reg_alloc 9 + debug_info 5）→ 总 **964/964**；E2E 28/28（-O0 与 -O2 输出完全一致）；x64 -O0 无物理寄存器、-O2 有（push/pop 配对正确）；编译零警告。

### 2.0b 前序核心任务：阶段B「平台无关优化补全」（已完成 ✅，2026-08-14）

**新增 Pass（操作 IR，与平台无关，服务 x64 与 arm64 双后端）**：

| 文件 | 职责 | 挂载 |
|------|------|------|
| [`opt/cfg.hpp`](src/cn_compiler/opt/cfg.hpp) / [`cfg.cpp`](src/cn_compiler/opt/cfg.cpp) | 支配树（支配者/立即支配者迭代求解）+ 自然循环检测（back edge→header+循环体） | 供 SSA/LICM 共用 |
| [`opt/ssa.hpp`](src/cn_compiler/opt/ssa.hpp) / [`ssa.cpp`](src/cn_compiler/opt/ssa.cpp) | SSA 构造：汇合点变量级 Phi（复用 Load 结果寄存器，语义等价、幂等） | **-O3** |
| [`opt/licm.hpp`](src/cn_compiler/opt/licm.hpp) / [`licm.cpp`](src/cn_compiler/opt/licm.cpp) | 循环不变量外提：迭代不变集合，纯运算外提到 preheader | **-O2** |
| [`opt/strength_reduce.hpp`](src/cn_compiler/opt/strength_reduce.hpp) / [`strength_reduce.cpp`](src/cn_compiler/opt/strength_reduce.cpp) | 强度削减：`Mul x 2^n -> Shl`、无符号 `Div x 2^n -> Shr` | **-O2** |
| [`opt/inline.hpp`](src/cn_compiler/opt/inline.hpp) / [`inline.cpp`](src/cn_compiler/opt/inline.cpp) | 函数内联：阈值 16 指令；排除递归/入口/OOP/lambda/模块函数/参数 Store/AddrOf | **-O3** |
| [`opt/tail_call.hpp`](src/cn_compiler/opt/tail_call.hpp) / [`tail_call.cpp`](src/cn_compiler/opt/tail_call.cpp) | 尾调用优化：尾递归→参数槽 Store+跳回入口 | **-O2** |

**优化级别组合**：-O0 无优化；-O1 折叠+简化+复写+DCE；-O2 +CSE+跨块DCE+LICM+强度削减+TCO；-O3 +全局值传播+SSA+内联。**-O0/-O2/-O3 三级别 E2E 输出完全一致（28/28）**。

**关键安全设计（踩坑修复沉淀）**：
1. LICM：Load/副作用指令结果**不加入不变集合**——避免外提指令在 preheader 引用"循环内 Load 定义"的未定义寄存器（排序/数组场景正确性关键）
2. LICM：循环体必须含 back edge tail 块（`body.insert(tail)`）与自环（tail==header）
3. 内联：排除 `?lambda`（捕获语义）、mangledName 含 `#`（模块/重载函数）、含 `AddrOf 参数`（地址逃逸）、含 `Store 参数`（值传递副本）
4. TCO：保守条件（实参匹配、入口无参数 Store、实参非嵌套调用结果、非结构体返回）

**测试**：新增 31 个单测（SSA 7 + LICM 6 + 强度削减 8 + 内联 6 + TCO 6）→ 总 **949/949**；E2E 三级别全 28/28。

### 2.1 阶段5 阶段A-2「ARM64 后端接入驱动/CLI/E2E + 本机全链路」（已完成 ✅）

**后端工厂与驱动分发**：
- 新建 [`backend_factory.hpp`](src/cn_compiler/codegen/backend_factory.hpp) / [`backend_factory.cpp`](src/cn_compiler/codegen/backend_factory.cpp)：`createBackend(target, diag, sem)`——win-x64 → X64、linux-arm64 → ARM64、未知报错
- [`driver.cpp`](src/cn_compiler/driver/driver.cpp) / [`driver_module.cpp`](src/cn_compiler/driver/driver_module.cpp)：`X64CodeGenerator` 硬编码 → `createBackend` 指针分发

**CLI 工具链平台分支（[`cn_main.cpp`](src/cn_compiler/cn_main.cpp)）**：
- win-x64：ml64 / cl / link（MSVC 流程不变）
- linux-arm64：`as`（GAS）+ `g++ -c -std=c++17 -fno-exceptions -fno-rtti -DCNRT_LINUX_MAIN` + `g++ -no-pie` 链接
- 后缀按平台：.asm/.s、.obj/.o、可执行无 .exe；Linux 工具链探测（CN_AS/CN_CXX → ~/gcc7 → PATH）
- runRun：Linux 直接执行

**E2E 适配（[`run_e2e.py`](tests/e2e/run_e2e.py)）**：编译器候选补无后缀、`--target` 参数（按平台默认）、可执行后缀按平台、透传 --target

**ARM64 后端本机验证修复（15 个汇编/ABI bug）**：见 [`更新日志.md`](更新日志.md) 第三节——含 mul 立即数、`@` 标签、`?` 符号、大栈帧分段、epilogue 恢复顺序、stackMemText 反解、i128 乘除 a/b 同寄存器、f64→i128 out 传 x0、ptr 常量实参、正64 最大值 stoull 等。

### 2.1b 阶段A-1「ARM64 后端核心 + 运行时平台无关改造」（已完成 ✅）

**新增 ARM64 后端（GAS 汇编 + AAPCS64 ABI，7 个文件）**：

| 文件 | 职责 |
|------|------|
| [`arm64_codegen.hpp`](src/cn_compiler/codegen/arm64/arm64_codegen.hpp) | `Arm64CodeGenerator : Backend`，targetPlatform="linux-arm64"，栈槽映射/访存/立即数辅助 |
| [`arm64_codegen.cpp`](src/cn_compiler/codegen/arm64/arm64_codegen.cpp) | 模块装配（.text/.data/.section .rodata）、函数框架（stp x29,x30 / x19 隐藏返回指针 / 16 对齐）、参数装载（x0~x7/v0~v7/栈/i128 双槽/结构体拷贝）、epilogue、符号（_ + UTF-8 hex） |
| [`arm64_instructions.cpp`](src/cn_compiler/codegen/arm64/arm64_instructions.cpp) | 单指令降级：整型/浮点算术、比较 cset、移位、Cast、Load/Store/LoadPtr/StorePtr、FieldAddr、Call、常量（movz/movk、adrp+add、浮点常量池） |
| [`arm64_codegen_dispatch.cpp`](src/cn_compiler/codegen/arm64/arm64_codegen_dispatch.cpp) | 终止指令 + 指令分派（≤1000 行拆分） |
| [`arm64_codegen_i128.cpp`](src/cn_compiler/codegen/arm64/arm64_codegen_i128.cpp) | i128 adds/adc/sbcs + 乘除余/比较调运行时辅助 |
| [`arm64_codegen_oop.cpp`](src/cn_compiler/codegen/arm64/arm64_codegen_oop.cpp) | NewObject/DeleteObject/VirtualCall（blr）/VtableAddr |
| [`arm64_codegen_vtable.cpp`](src/cn_compiler/codegen/arm64/arm64_codegen_vtable.cpp) | 虚表 .section .rodata + .quad、静态字段 .data |

**运行时平台无关改造**：
- [`i128_api.cpp`](src/runtime/i128_api.cpp)：`_umul128`（MSVC intrinsic）→ `umul128` 封装（`#ifdef _MSC_VER` 保留原实现；`#else` 用 `__int128` 内建）
- [`runtime.hpp`](src/runtime/runtime.hpp)：i128 API 签名统一 `std::uint64_t`（修 LP64 `unsigned long` vs `unsigned long long` 重声明冲突）+ 补 `<cstdint>`
- [`runtime.cpp`](src/runtime/runtime.cpp)：Linux `main(argc, argv)` → entry（`CNRT_LINUX_MAIN` 宏控制，避免与 gtest_main 冲突）

**Linux GCC 移植预存缺陷修复**（原 MSVC-only 项目首次 Linux 全量编译暴露）：
- `funcFirstSigKey`/函数名作值遍历 unordered_map 依赖哈希顺序 → 字典序确定性选择（GCC/MSVC 行为不一致）
- ast_printer 的 AddressOf/Deref 未处理、ir.cpp 括号警告、cn_main 未用参数、6 个测试文件 `[[maybe_unused]]`

### 2.2 前序里程碑（已全部完成 ✅）

- 阶段0~3 全部（901/901 单测、28/28 E2E、5 项缺陷全修复）
- 阶段4 优化与 Win x64 完善
- 语言表达力增强规划（P0~P3 写入 plans）

## 三、当前测试基线

| 指标 | 数值 |
|------|------|
| 单元测试 | **949/949**（原 918 + 新增 31 个阶段B Pass 测试：SSA 7/LICM 6/强度削减 8/内联 6/TCO 6） |
| E2E | **28/28**（Linux ARM64 本机真实运行，0 SKIP；**-O0/-O2/-O3 三级别输出一致**；Win x64 亦 28/28） |
| 编译警告 | 0（GCC 7 -Wall -Wextra -Werror） |
| ARM64 汇编验证 | `aarch64-linux-gnu-as` 交叉汇编通过（含 i128/OOP/浮点/字符串全特性 IR） |

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
13. **ARM64 栈帧（本轮新增）**：%vN → [x29,#-8N-8]（与 X64 [rbp-8N-8] 对应）；隐藏返回指针占 x0 用 x19 保存；栈参数偏移 = 16 + 16*needHiddenRet；栈槽 |offset|>255 用 x13 兜底
14. **ARM64 符号（本轮新增）**：nameMangle = `_` 前缀 + UTF-8 hex（去 ?..@@Y）；块标签 L+hex + 函数级前缀 `L<函数符号>_`；常量标签 LstrN/LfpN（GAS 中 @ 是注释符）
15. **ARM64 寄存器策略**：x9/x10/x11 临时、x12 拷贝计数、x13 地址计算、x16 间接调用目标、x19 隐藏返回指针
16. **运行时 main（本轮新增）**：Linux main 用 `CNRT_LINUX_MAIN` 宏控制（cn 可执行开启；单测关闭防 gtest_main 冲突）

## 五、踩过的坑（绝对不要再踩）

1. **DeleteObject 析构符号**：子类无自身析构时符号是 `子类$~父类析构名`，codegen 硬编码 `~类名` 链接失败
2. **MASM .rdata**：ml64 不认 `.rdata`（A2008），用 `.const`；**GAS 相反**：`.section .rodata`
3. **GAS `@` 前缀**：`@str0` 标签非法（@ 是注释符）→ 用 `Lstr0`；块标签需函数级唯一前缀（多函数同名"块0"冲突）
4. **GAS 注释**：`#` 与立即数前缀冲突 → 用 `//`
5. **LP64 类型别名**：`std::uint64_t` = `unsigned long`（Linux）vs `unsigned long long`（runtime.hpp 声明）→ extern "C" 重声明冲突，统一 `std::uint64_t`
6. **unordered_map 遍历顺序**：funcFirstSigKey/函数名作值依赖哈希顺序，GCC/MSVC 不一致 → 字典序确定性选择
7. **gtest 与运行时 main 冲突**：Linux 下 runtime.cpp 的 main 与 gtest_main 冲突 → CNRT_LINUX_MAIN 宏控制
8. **GCC 7 严格警告**：unused-function/unused-parameter/parentheses 在 MSVC 不报 → `[[maybe_unused]]`、括号、`(void)param`
9. **MSBuild 增量构建坑**：apply_diff 修改后须 `--clean-first` 强制重建（CMake 同样）
10. **i128 返回 epilogue**：x19 保存的隐藏返回指针；恢复时仅 needHiddenRet 才 ldp x19（否则读垃圾）
11. **常量返回文本**：termReturnValue="0"/"1"（非 %vN）不能 substr(2) 解析 → 防御分支
12. **movz/movk**：0 块应跳过（movz 已清零），否则输出多余 movk #0

## 六、下一步计划

1. **阶段6「标准库」**（plans/002-阶段6-标准库.md）：容器/数学/文件 IO/格式化/条件编译
2. **阶段5 阶段B/C**（plans/002-阶段5-LinuxARM64.md）：交叉编译/QEMU 验证、ARM64 优化对齐等后续内容（当前阶段A 已全部完成）
3. **遗留风险**：
   - 接口附加 vtable 预留（多接口场景，规格书06-五 预留，未实现）
   - ARM64 E2E 在 x86 主机需交叉工具链/QEMU（本机已为 aarch64，全链路已验证）
   - `~/gcc7` 便携工具链是本机 g++ 唯一来源（PATH 无系统 g++），cn_main 已自动探测

## 七、关键文件索引

| 模块 | 文件 |
|------|------|
| ARM64 后端 | src/cn_compiler/codegen/arm64/（7 文件） |
| X64 后端 | src/cn_compiler/codegen/x64/（4 文件） |
| Backend 接口 | src/cn_compiler/codegen/codegen.hpp |
| 运行时 | src/runtime/（runtime.hpp/cpp、i128_api.cpp、io_api.cpp、string_api.cpp） |
| ARM64 单测 | tests/unit/codegen/test_arm64_codegen.cpp |
| 阶段5 计划 | plans/002-阶段5-LinuxARM64.md（Task 5.1/5.2/5.3 已全部打勾 ✅） |
