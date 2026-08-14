# HANDOFF - CN语言编译器项目交接文档

> **交接原因**：2026-08-14 会话结束（**打印函数族命名统一"方案C"已完成 ✅**——彻底删除遗留的 `打印行整数`/`打印行浮点`（旧语义"行"=换行），打印函数族仅保留变参 `打印`/`打印行`/`格式化`。全链路修改：语义注册→x64/arm64 codegen→E2E→单元测试→文档同步。审查中发现并修复 3 处遗留缺陷（IR f32→f64 Cast、语义字符参数误拒、常量折叠精度丢失）。构建 0 错误 0 警告、单测 965/965、E2E 28/28 全通过，无回归）。
>
> **前序里程碑**：阶段3「OOP 与错误处理」（901/901 单测、28/28 E2E）；阶段A「Linux ARM64」（918/918 单测、28/28 E2E）；阶段B 优化补全（949/949 单测、28/28 E2E）；阶段C 寄存器分配/调试信息（964/964 单测、28/28 E2E）；reg_alloc -O2 崩溃缺陷修复（965/965 单测、28/28 E2E）；字符串转换函数命名优化（965/965 单测、28/28 E2E）；**打印函数族命名统一方案C（965/965 单测、28/28 E2E，本交接）**。

## 一、我们在做什么任务

正在开发CN语言编译器——一门全中文语法的系统级编程语言，参考C++编程范式，用C++17从零编写编译器，直接生成汇编代码。当前已完成：阶段0~3（词法→语法→语义→IR→Win x64 后端→OOP/错误处理/模块/泛型）、阶段4 优化、阶段5「Linux ARM64」阶段A（ARM64 后端 + 本机全链路）、reg_alloc 崩溃缺陷修复、字符串转换函数命名优化、打印函数族命名统一方案C。下一步：**阶段6「标准库」** 或 阶段5 阶段B/C（交叉编译/QEMU 验证）、阶段4 优化级别框架收尾。

**本次任务**：解决打印函数族命名规律不一致问题——`打印()` 结尾有换行（Task 2.9 新语义，规格书10.6），`打印行()` 不换行（"行"=逐行连续输出语义，不追加换行符），但遗留的 `打印行整数(整64)`/`打印行浮点(浮64)` 仍是旧语义"行"=换行（`printf("%lld\n")`/`printf("%f\n")`）。三个方案对比后用户批准**方案C**：彻底删除这两个遗留函数，统一用 `打印`/`打印行` 变参表达（`打印行整数(42)` → `打印(42)` 行为完全一致）。

## 二、已经完成了什么

### 2.0 本轮核心任务：打印函数族命名统一方案C（已完成 ✅，2026-08-14）

**改动范围（全链路同步）**：

| 层 | 文件 | 修改 |
|----|------|------|
| 语义注册 | [`semantic.cpp`](src/cn_compiler/semantic/semantic.cpp:663) | 删除 `打印行整数`/`打印行浮点` 的内置函数注册；打印函数族仅保留变参 `打印`/`打印行`/`格式化` |
| x64 codegen | src/cn_compiler/codegen/x64/x64_codegen.cpp | 删除 `printLineInt`/`printLineFloat` 符号映射与 EXTERN 声明 |
| arm64 codegen | src/cn_compiler/codegen/arm64/ | 删除 `printLineInt`/`printLineFloat` 符号映射与 EXTERN 声明 |
| 运行时 | src/runtime/（io_api.cpp、runtime.hpp） | `printLineInt`/`printLineFloat`/`printLineI128`/`printLineU128` **保留**（单元测试 test_runtime.cpp 直接调用 + 防御ABI稳定），编译器已不再映射 |
| E2E | 11 个用例 | `打印行整数(X)` → `打印(X)`、`打印行浮点(X)` → `打印(X)`；`.expected` 无需修改（输出一致） |
| 单元测试 | test_string_semantic、test_string_extra、test_ir_string_extra、test_x64_oop、test_arm64_codegen、test_dce | 同步删除 `打印行整数`/`打印行浮点` 调用，改用 `打印`/`打印行` 变参 |
| 文档 | 规格书10.6、阶段1D、阶段2、lessons.md、更新日志.md | 名称同步；lessons.md 3 个缺陷条目均标注 ✅已修复 |

**验证结果**：
- 构建：0 错误 0 警告（MSVC /W4 /WX）
- 单元测试：965/965 通过
- E2E：28/28 通过（全新输出目录复验）

**设计决策**：
1. 运行时符号 `printLineInt`/`printLineFloat`/`printLineI128`/`printLineU128` **保留不变**——单元测试直接调用 + 防御ABI稳定，但编译器已不再映射任何 CN 名到它们
2. `打印`/`打印行` 为变参函数，`打印行整数(42)` 与 `打印(42)` 行为完全一致，属旧语义的等价替换

### 2.1 审查中发现并修复的额外缺陷（均 ✅ 已修复）

| # | 缺陷 | 现象 | 修复位置 |
|---|------|------|----------|
| 1 | **IR 缺陷** | `打印(浮32)` 变参展开缺 f32→f64 Cast（原 `打印行浮点(f32)` 靠语义层隐式宽化），修复前输出 0.000000 | [`ir.cpp`](src/cn_compiler/ir/ir.cpp:2756) 补 Cast |
| 2 | **语义缺陷** | `打印('A')`/`打印(字符变量)` 被变参检查误拒（旧 `打印行整数('A')` 字符→整64隐式转换输出ASCII码） | [`semantic.cpp`](src/cn_compiler/semantic/semantic.cpp:2004) `okNum` 补 `字符` 类型（`格式化` 共用分支同时覆盖） |
| 3 | **常量折叠缺陷** | f32→f64 Cast 常量折叠精度丢失——`格式化("%.10f", 3.14f)` 输出 3.1400000000（正确 3.1400001049） | [`const_fold.cpp`](src/cn_compiler/opt/const_fold.cpp:379) 改为先按 f32 精度截断再提升 |

### 2.2 前序里程碑（已全部完成 ✅）

- 阶段0~3 全部（901/901 单测、28/28 E2E、5 项缺陷全修复）
- 阶段4 优化与 Win x64 完善（949/949 → 964/964 单测）
- 阶段5「Linux ARM64」阶段A（ARM64 后端 + 本机全链路，918/918 单测）
- reg_alloc -O2 崩溃缺陷修复（965/965 单测、28/28 E2E、-O0/-O2/-O3 三级别 84 组合一致）
- 字符串转换函数命名优化（965/965 单测、28/28 E2E）

## 三、当前测试基线

| 指标 | 数值 |
|------|------|
| 单元测试 | **965/965**（打印函数族方案C 回归通过） |
| E2E | **28/28**（含 11 个 `打印`/`打印行` 变参用例；Win x64） |
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
16. **打印函数族语义**：`打印(...)` = 结尾换行；`打印行(...)` = 不换行（"行"=逐行连续输出，不追加换行符）；`格式化(...)` = 格式化字符串。仅支持标量+字符串+字符变参，指针/结构体传参被语义层拒绝。运行时 `printLine*` 符号保留但编译器不再映射
17. **字符串转换函数映射**：CN 公开名（`整数转字符串`/`浮点转字符串`/`字符转字符串`/`布尔转字符串`/`正数转字符串`）→ 运行时符号（`__cn_str_from_int/float/char/bool/uint`），映射集中在 `semantic.cpp` regStrFn 注册 + `ir.cpp` 名称映射 + `ir.cpp` 结果类型映射三处

## 五、踩过的坑（绝对不要再踩）

1. **打印函数族命名"行"字语义已反转**（权重15.0，✅已修复）：`打印行`=不换行（逐行连续输出语义），勿再按直觉理解为换行；遗留 `打印行整数`/`打印行浮点`（旧语义"行"=换行）已彻底删除，统一用 `打印`/`打印行` 变参
2. **DeleteObject 析构符号**：子类无自身析构时符号是 `子类$~父类析构名`，codegen 硬编码 `~类名` 链接失败
3. **MASM .rdata**：ml64 不认 `.rdata`（A2008），用 `.const`；**GAS 相反**：`.section .rodata`
4. **GAS `@` 前缀**：`@str0` 标签非法（@ 是注释符）→ 用 `Lstr0`；块标签需函数级唯一前缀
5. **GAS 注释**：`#` 与立即数前缀冲突 → 用 `//`
6. **LP64 类型别名**：`std::uint64_t` = `unsigned long`（Linux）vs `unsigned long long`（runtime.hpp 声明）→ extern "C" 重声明冲突，统一 `std::uint64_t`
7. **unordered_map 遍历顺序**：funcFirstSigKey/函数名作值依赖哈希顺序 → 字典序确定性选择
8. **gtest 与运行时 main 冲突**：Linux 下 runtime.cpp 的 main 与 gtest_main 冲突 → CNRT_LINUX_MAIN 宏控制
9. **GCC 7 严格警告**：unused-function/unused-parameter/parentheses → `[[maybe_unused]]`、括号、`(void)param`
10. **MSBuild 增量构建坑**：apply_diff 修改后须 `--clean-first` 强制重建（CMake 同样）
11. **i128 返回 epilogue**：x19 保存的隐藏返回指针；恢复时仅 needHiddenRet 才 ldp x19（否则读垃圾）
12. **常量返回文本**：termReturnValue="0"/"1"（非 %vN）不能 substr(2) 解析 → 防御分支
13. **movz/movk**：0 块应跳过（movz 已清零），否则输出多余 movk #0
14. **中文字符串前缀匹配**：`str[i] == '中文字符'` 非法（UTF-8 多字节）→ 用 `s.compare(0, prefix.size(), "前缀") == 0` 字节语义
15. **变参展开参数类型必须与运行时函数签名一致**（权重8.5，✅已修复）：f32 必须 Cast 到 f64 再传参，否则 ABI 不匹配输出 0.000000
16. **f32→f64 常量折叠精度**（权重9.1，✅已修复）：必须先按 f32 精度截断再提升，直接按 double 解析会精度丢失（`格式化("%.10f", 3.14f)` 正确应为 3.1400001049）
17. **删除内置函数要全链路同步检查**：语义注册、x64/arm64 codegen 映射、E2E `.cn` 与 `.expected`、单元测试、规范文档、lessons.md、更新日志——一处遗漏都会造成回归

## 六、下一步计划

1. **阶段6「标准库」**（plans/002-阶段6-标准库.md）：容器/数学/文件 IO/格式化/条件编译（打印函数族相关文档已同步）
2. **阶段5 阶段B/C**（plans/002-阶段5-LinuxARM64.md）：交叉编译/QEMU 验证、ARM64 优化对齐等后续内容
3. **遗留风险**：
   - `打印`/`打印行`/`格式化` 变参仅支持标量+字符串+字符，指针/结构体传参会被语义层拒绝，如需支持需扩展变参检查
   - 接口附加 vtable 预留（多接口场景，规格书06-五 预留，未实现）
   - ARM64 E2E 在 x86 主机需交叉工具链/QEMU（本机已为 aarch64，全链路已验证）
   - `~/gcc7` 便携工具链是本机 g++ 唯一来源（PATH 无系统 g++），cn_main 已自动探测
   - **arm64 寄存器分配保守默认关闭**：结果写回点分散于 35 处 emitStackStore（未收敛于 resultText 单点），已提供 `setRegAllocMap` 注入 API（模块独立可用、单测充分），默认全栈帧——待写回点收敛后再启用（正确性最高优先）
   - **x64_instructions.cpp 存量超行数**：单文件超 1000 行约束，待后续拆分（收尾不改动业务代码）

## 七、关键文件索引

| 模块 | 文件 |
|------|------|
| 语义（打印函数族注册） | [`src/cn_compiler/semantic/semantic.cpp`](src/cn_compiler/semantic/semantic.cpp:663)（`打印`/`打印行`/`格式化` 变参注册）、[`semantic.cpp`](src/cn_compiler/semantic/semantic.cpp:2004)（变参检查 okNum 含 字符 类型）、[`semantic.cpp`](src/cn_compiler/semantic/semantic.cpp:746)（regStrFn 注册） |
| IR（变参展开/结果类型） | [`src/cn_compiler/ir/ir.cpp`](src/cn_compiler/ir/ir.cpp:2756)（f32→f64 Cast）、[`ir.cpp`](src/cn_compiler/ir/ir.cpp:2810)（CN 名 → `__cn_str_from_*`） |
| 常量折叠 | [`src/cn_compiler/opt/const_fold.cpp`](src/cn_compiler/opt/const_fold.cpp:379)（f32 先截断再提升） |
| 运行时 | src/runtime/（runtime.hpp/cpp、io_api.cpp `printLine*`、i128_api.cpp、string_api.cpp） |
| X64 后端 | src/cn_compiler/codegen/x64/（4 文件） |
| ARM64 后端 | src/cn_compiler/codegen/arm64/（7 文件） |
| Backend 接口 | src/cn_compiler/codegen/codegen.hpp |
| E2E 打印用例 | tests/e2e/（11 个用例含 `打印`/`打印行` 变参；`.expected` 未改动） |
| 计划文档 | plans/002-阶段1D-后端与集成.md、plans/002-阶段2-核心语言.md、plans/001 CN语言编译器设计规格书.md（10.6 已同步） |
