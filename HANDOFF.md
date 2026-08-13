# HANDOFF - CN语言编译器项目交接文档

> **交接原因**：2026-08-13 会话结束（**Task 2.9/2.10 最终收尾**——plans 打勾、三文档同步、全量验证 738/738 + E2E 18/18 + 72 组合一致、git 提交推送）。本文档写给完全没有上下文的新会话看，帮助快速恢复开发。
>
> **前序里程碑**：基础语言缺陷完善A（i128 + 结构体按值 + CopyStruct）、Task 2.8 字符串系统完善B（13 API）、优化器增强完善C（5 Pass + -O2/-O3）、集成验证（14_integration2 + 6 BUG）、Debug 全面审查（9 用例 + 5 BUG）、Task 2.9 语言表达力增强、Task 2.10 函数特性增强、Task 2.9/2.10 Debug 审查（8 BUG）、缺陷修复（lambda 捕获语义 + 无符号打印，2 缺陷 + 1 顺带 BUG）、缺陷修复针对性复核（lambda 结构体值捕获 + 嵌套 lambda，2 缺陷）、**本次最终收尾（plans 打勾 + 文档同步 + 全量验证 + git 提交）**，单测 **738/738**、E2E **18/18**、18 用例 × 4 级别 = **72 组合输出一致**、编译零警告（/W4 /WX）。

## 一、我们在做什么任务

正在开发CN语言编译器——一门全中文语法的系统级编程语言，参考C++编程范式，用C++17从零编写编译器，直接生成汇编代码（Win x64 MASM，ml64 汇编）。当前完成**阶段2全部（Task 0.1~2.10）**+ 缺陷完善 A/B/C + Debug 审查 + Task 2.9 表达力 + Task 2.10 函数特性增强 + Debug 审查 + 缺陷修复 + 复核 + **最终收尾**。下一步：Task 2.11（或阶段3 OOP 准备，见 plans 阶段2 文档与阶段3 文档）。

## 二、已经完成了什么

### 2.1 设计规格书（已批准）
- 文件：`plans/001 CN语言编译器设计规格书.md`（984行，14章）
- 关键字计数 55（新增"自动"）

### 2.2 cn-language-spec 技能（已创建）
- 目录：`.ai-coder/skills/cn-language-spec/`（16个文件）
- **编写任何 CN 语言相关代码前必须调用此技能查规范**

### 2.3 阶段零/一/二（全部完成 ✅）
- Task 0.1~2.10 全部完成 + Task 2.9/2.10 Debug 审查完成 + 缺陷修复 + 复核 + 最终收尾
- **plans/002-阶段2-核心语言.md**：Task 2.9 ✅ / Task 2.10 ✅ 已打勾，验证数字更新为 738/738 + E2E 18/18 + 72 组合一致，补"复核缺陷已全部修复"标注

### 2.4 Task 2.9 语言表达力增强（已完成）
1. **三元表达式 `? :`**：Pratt 1.5 级右结合；IR 层条件跳转 CFG **惰性求值**；语义层分支类型统一
2. **字符串 + 数值隐式拼接**：右操作数整数/浮点/布尔/字符/枚举 → 转换 + `字符串连接`；布尔 `"真"/"假"`；内存调用方释放
3. **`格式化()`**：sprintf 风格变参；Win x64 浮点参数按参数位 xmmN + `movq` 位模式到 GPR（va_arg 可读）
4. **`打印`=println（换行）/`打印行`=print（不换行）**，旧 E2E 01~14 同步改 `打印`
5. E2E `15_expressiveness` + 4 套件单测

### 2.5 Task 2.10 函数特性增强（已完成）
1. **函数重载**：conversionLevel 决议 + sigKey → mangledName（附录C）
2. **默认参数**：从右向左连续 + 按位置补缺省
3. **强制类型转换 `类型名(表达式)`**：CastExpr + emitCast 全矩阵
4. **lambda**：`自动` 关键字 + 匿名函数降级 + 闭包捕获（`[=]`/`[&]`/`[]`/`[变量]`）
5. E2E `16_function_extra` + 4 套件单测（overload/default_param/cast_expr/lambda）

### 2.6 Debug 审查（8 BUG，权重 27.3）
i128 拼接 0xC0000005、布尔→整数转换被拒、显式捕获无参 lambda 解析失败、负浮点常量 A2006、无符号字面量不提升、浮→i128 拦截、i128→i128 Cast 32位、u64→i64 截断。**728/728 + E2E 17/17 + 68 组合一致**。

### 2.7 缺陷修复（2 设计缺陷 + 1 顺带，权重 22.5）
1. **lambda `[=]` 值捕获 vs `[&]` 引用捕获**——捕获实参求值提前到定义处（genVarDecl 固化 captureArgs）：`[=]` 存值快照、`[&]` 存地址指针；`[&]` 参数 ptr，体内 LoadPtr 读/StorePtr 写回
2. **无符号大值打印负数**——新增 `__cn_print_uint`/`__cn_str_from_uint`（%llu），按无符号类型分派
3. **i128 捕获垃圾值**（顺带）——`maxRegIdIn` 按 `id+1` 计双槽
4. **736/736 + E2E 18/18（新增 18_缺陷修复）**

### 2.8 缺陷修复针对性复核（2 个新缺陷，权重 20.8）
1. **lambda `[=]` 值捕获结构体字段读垃圾值**（[`ir.cpp`](src/cn_compiler/ir/ir.cpp:1386) + [`ir.cpp`](src/cn_compiler/ir/ir.cpp:3134)）——captureArgs 只存 `AddrOf(结构体)` 指针 + 闭包函数未标记 `structParamIndexes`。**修复**：结构体值捕获走"定义处深拷贝快照"（Alloca + CopyStruct），闭包参数循环对非 byRef 结构体捕获插入 `func.structParamIndexes`
2. **嵌套 lambda 汇编失败**（[`semantic.cpp`](src/cn_compiler/semantic/semantic.cpp:1869)）——`collectLambdaCaptures` 漏 `LambdaExpr` case。**修复**：补 LambdaExpr case——递归收集内层捕获集（内层参数并入 paramNames 去重）再合并到本层 `explicitCaptures`
3. 回归：E2E `18_缺陷修复` +2 场景 + 单测 `test_lambda.cpp` +2（NestedLambdaCaptureOk、StructByValueCaptureSnapshot）

### 2.9 最终收尾（本次完成 ✅ 2026-08-13 22:05）
- **plans**：Task 2.9/2.10 打勾确认 + 验证数字更新（738/738、E2E 18/18、72 组合）+ 复核缺陷已修复标注
- **更新日志.md**：覆盖写入全部内容（Task 2.9 + 2.10 + Debug 审查 + 缺陷修复 + 复核 + 收尾）
- **HANDOFF.md**：本交接文档
- **lessons.md**：核对完整性（27.3/22.5/20.8 等全部记录，缺陷标注已解决）
- **全量验证**：编译零警告 + 单测 **738/738** + E2E **18/18** + **72 组合**（18 用例 × -O0/-O1/-O2/-O3）输出一致
- **git 提交推送**：develop 分支（中文提交）

## 三、当前测试基线

| 指标 | 数值 |
|------|------|
| 单元测试 | **738/738**（62 套件） |
| E2E 用例 | **18/18** |
| 优化一致 | 18 用例 × -O0/-O1/-O2/-O3 = **72 组合输出一致** |
| 编译警告 | 0（/W4 /WX） |

## 四、关键架构约定（必须遵守）

1. **i128/正128 双寄存器约定**：`%vN`=高64位槽、`%vN+1`=低64位槽；凡 `result.type` 为 i128/u128 的指令结果占 2 槽，`maxRegIdIn`/`registerVarSlots`/数组 stride/指针步进均按 16 字节处理
2. **结构体按值**：≤8 字节寄存器传递，>8 字节调用方栈分配 + 传指针；被调方 `emitParamSetup` 按 `structParamIndexes` 从指针 rep movsb 拷贝到参数槽；返回走隐藏返回指针（rcx→r12）
3. **lambda 捕获语义**（规格书04-一D）：`[=]` 值捕获=定义处快照（结构体须深拷贝）、`[&]` 引用捕获=存地址指针（体内 LoadPtr 读/StorePtr 写回）；捕获实参在 `genVarDecl` 定义处固化，调用点只展开
4. **无符号整数打印/拼接**：正8~正64 走 `__cn_print_uint`/`__cn_str_from_uint`（%llu），禁止统一 Cast i64 走 %lld
5. **Win x64 ABI**：隐藏返回指针占 rcx（参数位后移 paramOffset）；浮点参数独立 xmmN；被调方栈参数 rbp 锚定（[rbp+48] 为第5参数位）；调用前预留 32 字节影子空间
6. **变参函数（printf 家族）**：浮点参数按参数位 xmmN + `movq` 位模式到同参数位 GPR，否则 va_arg 读垃圾

## 五、踩过的坑（绝对不要再踩）

1. **lambda 值捕获结构体/数组必须"定义处深拷贝"**，不能只存地址指针（破坏快照语义 + 闭包参数槽类型错配读垃圾）
2. **语义层表达式展开 switch 必须覆盖所有节点类型**（尤其 LambdaExpr 嵌套闭包），漏 case 导致捕获集缺失、跨函数槽引用（`[rbp0]` 非法汇编）
3. **codegen 结构体参数拷贝依赖 `structParamIndexes` 标记**，凡"以指针传入需拷贝到参数槽"的参数（含 lambda 结构体值捕获参数）都要登记
4. **捕获实参求值时机 = 定义处**（非调用点），否则 `[=]` 退化为 `[&]`
5. **无符号整数打印/拼接必须走无符号路径**（%llu），禁止 Cast i64 走 %lld
6. **i128 双槽约定全覆盖**：maxRegIdIn/数组 stride/指针步进/Cast 矩阵/LoadPtr/StorePtr 都要按 16 字节
7. **emitCast 分支按"更具体类型优先"排序**（i128 目标先于通用 !toFloat）；同类型与 u64↔i64 转换矩阵必须覆盖
8. **十六进制生成统一走 `uint64HexText`**（含前导0逻辑），禁止各自 `%llX`（负浮点 A2006）
9. **Win x64 C 变参**：浮点参数必须同时设 xmmN 并 movq 到同参数位 GPR（位模式），否则 va_arg 读垃圾
10. **无符号字面量需值域自适应位宽提升**（正32→正64→正128），无后缀字面量同理（超 int32 提升整64）

## 六、下一步计划

1. Task 2.11（或阶段3 OOP 准备，见 `plans/002-阶段2-核心语言.md` 与 `plans/002-阶段3-OOP与错误处理.md`）
2. 后续每次代码修改后必须：编译零警告 + 单测全过 + E2E 全过 + 优化一致 + 更新 lessons.md + HANDOFF.md + 更新日志.md
3. 验证命令参考：`cmake --build target/build --config Debug`、`target\Debug\cn_unit_tests.exe`、`python tests\e2e\run_e2e.py`、`python target\verify_opt_consistency.py`
