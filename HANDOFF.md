# HANDOFF - CN语言编译器项目交接文档

> **交接原因**：2026-08-13 会话结束（**Task 2.10 函数特性增强实现完成**——函数重载（mangling 附录C）、默认参数（右向左连续 + 调用补全）、强制类型转换（emitCast 矩阵补全）、lambda 表达式（匿名函数+闭包捕获，新增 `自动` 关键字））。本文档写给完全没有上下文的新会话看，帮助快速恢复开发。
>
> **前序里程碑**：基础语言缺陷完善A（i128 + 结构体按值 + CopyStruct）、Task 2.8 字符串系统完善B（13 API）、优化器增强完善C（5 Pass + -O2/-O3）、集成验证（14_integration2 + 6 BUG）、Debug 全面审查（9 用例 + 5 BUG）、Task 2.9 语言表达力增强（三元/拼接/格式化/打印打印行）、**Task 2.10 函数特性增强**，单测 **726/726**、E2E **16/16**、64 组合 -O0/-O1/-O2/-O3 输出一致、编译零警告（/W4 /WX）。

## 一、我们在做什么任务

正在开发CN语言编译器——一门全中文语法的系统级编程语言，参考C++编程范式，用C++17从零编写编译器，直接生成汇编代码（Win x64 MASM，ml64 汇编）。当前完成阶段2全部 + 缺陷完善 A/B/C + Debug 审查 + Task 2.9 表达力 + **Task 2.10 函数特性增强**。下一步：Task 2.11（或阶段3 OOP 准备，见 plans 阶段2 文档）。

## 二、已经完成了什么

### 2.1 设计规格书（已批准）
- 文件：`plans/001 CN语言编译器设计规格书.md`（984行，14章）
- 关键字计数 55（新增"自动"）

### 2.2 cn-language-spec 技能（已创建）
- 目录：`.ai-coder/skills/cn-language-spec/`（16个文件）
- **编写任何 CN 语言相关代码前必须调用此技能查规范**

### 2.3 阶段零/一/二（全部完成 ✅）
- Task 0.1~2.10 全部完成（CLI/诊断/词法/语法/语义/IR/X64代码生成/运行时/driver + 控制流/函数/类型/数组指针/字符串/优化器/结构体枚举联合体 + 缺陷完善A/B/C + 表达力增强 + 函数特性增强）

### 2.4 Task 2.10 函数特性增强（本次完成 ✅ 2026-08-13）
- **函数重载（规格书04-一B，附录C mangling）**：
  - 语义层 `signatureKey = 名#参数类型串`（`加#整32,整32`）作函数表 key；同名不同参数/个数共存，仅返回类型不同不构成重载（报错）
  - `resolveOverload` 按 conversionLevel（0=精确/1=宽化/2=隐式/-1=不可转）总等级选最优；调用回填 `CallExpr.resolvedSignature`
  - codegen `nameMangle` 解析 sigKey 生成 `?UTF8HEX@@Y<参数编码>@Z`；mangleTypeCode：整32=H/浮64=N/字符串=PAX/空类型*=PEX/指针=PE<所指>
  - `IRFunction.name` = 源码名（测试契约）、`mangledName` = sigKey；函数头/ENDP/EXTERN 全用 mangledName；FuncAddr 用 `funcFirstSigKey`（LNK1120 修复）
- **默认参数（规格书04-一C）**：`参数 = 默认值`；**从右向左连续声明**（反向扫描 noDefaultSeen 后遇有默认报错）；`funcDefaultArgs_[mangledName]` 记录尾部默认值常量，调用补全 `missing = 总数 - 实参个数` 取末尾 missing 个；默认值须编译期常量（evalDefaultExpr）
- **强制类型转换（规格书04-一E）**：`类型名(表达式)`；parser 歧义判定（`(` 前 token 是类型关键字/类型名 → CastExpr）；语义检查（数值族/指针↔整数/指针→指针显式合法）；emitCast 补全：ptr↔int（64位 mov）、u64→浮（新增 `__cn_u64_to_f64`，cvtsi2sd 有符号语义错）、浮→i128（新增 `__cn_f64_to_i128`）、u32→浮（mov eax 零扩展）
- **lambda 表达式（规格书04-一D）**：`[捕获]`（[]/[=]/[&]/[变量]）；参数表可选、返回类型推导（lambdaInferMode_）；赋值目标用新增 `自动` 关键字（Kw_Auto）；降级为匿名函数+闭包捕获环境——匿名函数签名=[捕获参数..., 显式参数...]，调用点展开捕获实参；`collectLambdaCaptures` 递归扫描收集外层变量；visitLambdaExpr 保存/恢复 outerFunction/outerBlock/blockCounter_/varStack_（0xC0000005 修复）
- **测试**：新增 `test_overload.cpp`（8）/`test_default_param.cpp`（6）/`test_cast_expr.cpp`（8）/`test_lambda.cpp`（8）共 30 个；`tests/e2e/16_function_extra/函数增强.cn`（21 行期望）；单测 **726/726**、E2E **16/16**、64 组合 -O0~-O3 一致、编译零警告

### 2.5 修复的 BUG（Task 2.10 全部已修复 ✅）
1. **A1010**：ENDP 用纯名 → 改 mangledName
2. **0xC0000005 IR 崩溃**：visitLambdaExpr 覆盖外层函数状态 → 保存/恢复
3. **0xC0000374 堆损坏**：lambda 返回字符串字面量被调用方 `字符串释放` → E2E 改返回拼接字符串（**教训：常量池字符串不可释放**）
4. **A2022**：`mov rcx, @str0`（ConstString 标签不能 mov 入 64 位寄存器）→ emitCall ptr 常量参数用 lea
5. **LNK1120**：FuncAddr 用纯名 → funcFirstSigKey
6. **默认参数规则多次修正**：最终反向扫描（`求和(a, b=10, c=20)` 合法不误报）
7. **`结果`/`原始` 关键字冲突**：Kw_Result/Kw_Raw → 测试变量改 `值`/`通用指针`
8. **lambda 解析**：`[=]` 未消费 `]`；`[] {...}` 无参（peekLambdaCapture 放宽为 `(` 或 `{`）
9. **`浮64(3)` 表达式上下文未识别**：parsePrimary 加类型关键字 Cast 探测
10. **字符转换失败**：dstNumeric 补 `字符`

### 2.6 前序里程碑（背景）
- 缺陷完善A：i128 完整支持（双寄存器模型 + 运行时辅助函数）+ 结构体按值传参/返回（Win x64 隐藏返回指针）+ CopyStruct
- Task 2.8：字符串 13 API（子串/字典序/大写/修剪/反转/从整数等，内存语义：动态结果调用方释放）
- 完善C：优化器 5 Pass（代数简化/const_fold/copy_prop/cse/跨块DCE）+ -O2/-O3
- Task 2.9：三元表达式（惰性求值 CFG）、字符串+数值隐式拼接（`__cn_str_from_bool`）、`格式化()`（`__cn_format` 变参，movq 位模式 ABI）、`打印`=println/`打印行`=print

## 三、总结发现的问题

- **常量池字符串不可释放**：lambda 返回 `.data` 常量池字符串字面量，调用方 `字符串释放` → 0xC0000374 堆损坏。**规律：任何返回字符串字面量地址的场景（lambda/三元/直接返回字面量）调用方不能释放**
- **MSVC x64 变参 ABI**：浮点按参数位 xmmN + 必须 movq 位模式到同参数位整型寄存器（va_arg 从保存槽读）
- **关键字冲突**：`结果`（Kw_Result）/`原始`（Kw_Raw）/`自动`（Kw_Auto）等关键字不能作变量名——测试用例编写注意
- **cn.exe 与 cn_unit_tests.exe 分开构建**：改 semantic/parser 等库后必须重建 cn 目标（只建 cn_unit_tests 会导致 CLI 用旧库）

## 四、当前卡在哪

- **无卡点**：Task 2.10 全部完成，所有验证通过（单测 726/726、E2E 16/16、64 组合一致、零警告）
- 下一步：Task 2.11（见 `plans/002-阶段2-核心语言.md` 阶段二文档后续条目）或阶段3 OOP 与错误处理准备

## 五、下一步计划

1. 若继续阶段2：按 `plans/002-阶段2-核心语言.md` 后续 Task 逐项实施（每项 E2E 先行 + 单测 + 文档打勾 + 提交）
2. 阶段3（OOP 与错误处理）：`plans/002-阶段3-OOP与错误处理.md`（类/继承/接口/错误码传播）
3. 每轮提交前：更新 `更新日志.md`（覆盖写）+ plans 打勾 + HANDOFF.md + git 提交（中文标题，推 gitcode develop）

## 六、踩过的坑绝对不要再踩（经验教训已同步 lessons.md）

1. **常量池字符串不可释放**（0xC0000374）——返回字面量地址的调用方禁用 `字符串释放`
2. **ENDP 符号必须与函数头一致**（mangledName）——A1010
3. **visitLambdaExpr 必须保存/恢复外层生成状态**——0xC0000005
4. **ConstString 标签不能 mov 入寄存器**（A2022）——ptr 常量参数用 lea
5. **重载符号三处一致**（定义/调用/FuncAddr）——LNK1120
6. **默认参数反向扫描规则**：noDefaultSeen 后遇有默认参数报错
7. **测试变量名避开关键字**：`结果`/`原始`/`自动` 等
8. **cn.exe 与单元测试分开重建**：改库后两个目标都要 build
9. **Python subprocess 捕获中文输出用 text=True 遇 GBK 解码错**：脚本写文件再读取，避免内联打印被吞
