# HANDOFF - CN语言编译器项目交接文档

> **交接原因**：2026-08-13 会话结束（基础语言缺陷完善A完成：i128 完整支持 + 结构体按值传参/返回 + 结构体整体赋值，单测 549/549、E2E 11/11，已提交推送 gitcode develop 提交 `59c3944`）。本文档写给完全没有上下文的新会话看，帮助快速恢复开发。

## 一、我们在做什么任务

正在开发CN语言编译器——一门全中文语法的系统级编程语言，参考C++编程范式，用C++17从零编写编译器，直接生成汇编代码（Win x64 MASM，ml64 汇编）。当前完成阶段2全部 + 基础语言缺陷完善A。

## 二、已经完成了什么

### 2.1 设计规格书（已批准）
- 文件：`plans/001 CN语言编译器设计规格书.md`（984行，14章）
- 关键字计数 54（新增"联合体"）

### 2.2 cn-language-spec 技能（已创建）
- 目录：`.ai-coder/skills/cn-language-spec/`（16个文件）
- **编写任何 CN 语言相关代码前必须调用此技能查规范**

### 2.3 阶段零/一/二（全部完成 ✅）
- Task 0.1~2.7 全部完成（CLI/诊断/词法/语法/语义/IR/X64代码生成/运行时/driver + 控制流/函数/类型/数组指针/字符串/优化器/结构体枚举联合体）

### 2.4 基础语言缺陷完善A（本次完成 ✅）
- **i128 完整支持（规格书10.5）**：
  - `src/runtime/i128_api.cpp`：全部 128 位运算辅助函数（`__cn_add/sub/mul/div/mod_i128`、`__cn_div/mod_u128`、`__cn_cmp_i128/u128`、转换、打印），MSVC `_umul128` 4次乘加 + 128次迭代二进制长除法，指针式 API（`a[0]=低64位/a[1]=高64位`）
  - IR/codegen：双寄存器模型（id=高64位/id+1=低64位）、add/adc、sub/sbb 进位/借位链、`emitInt128MulDivMod` 调辅助函数、`emitInt128Compare`、打印行展开走 `__cn_print_i128/u128`、i128→i64 截断 Cast
  - 语义：`isI128/textExceedsInt64/splitI128Text`；字面量越界检查（整128 上限 2^127-1、正128 上限 2^128-1，含无后缀超 int64 自动提升场景）
- **结构体按值传参/返回（规格书7.4，Win x64 ABI）**：
  - 传参：`structParamIndexes` + `emitParamSetup` rep movsb 从指针拷贝到参数槽（精确字节数）
  - 返回：Win x64 隐藏返回指针方案——`IRFunction.structReturn/structReturnSize`；调用方分配 `__retbufN` 返回缓冲区（64字节）作为隐藏参数（rcx）；被调方 prologue `mov r12, rcx` 保存（非易失寄存器）；epilogue 按 `structReturnSize` **精确字节数**拷贝到缓冲区
- **结构体整体赋值**：`CopyStruct` IR 指令（extra=字节数）+ codegen `rep movsb` 内联拷贝；含数组字段结构体整体拷贝

### 2.5 本次修复的 4 个 BUG
1. **epilogue 漏 ret**：structReturn 分支直接 return 漏掉尾部 `mov rsp,rbp/pop rbp/ret` → 执行流落入下一函数无限递归 → 0xC00000FD
2. **64 字节硬编码拷贝越界**：16 字节班级结构体被写 64 字节越界 48 字节破坏相邻栈变量 → `structReturnSize` 精确拷贝
3. **StorePtr 目标类型 i64 覆盖**：`出.分数[1] = ...`（IndexExpr 对象为结构体数组字段 MemberExpr）targetType 推导遗漏 → 8 字节写入覆盖相邻数组元素 → 补 MemberExpr 分支
4. **i128 字面量越界未报错**：语义层加 整128/正128 上限检查

### 2.6 测试状态
- **单元测试 549/549**（原 508 + 新增 41：`test_i128_api`/`test_i128_semantic`/`test_ir_i128`/`test_ir_struct_value`/`test_x64_struct_value`/`test_x64_i128`）
- **E2E 11/11**（01~09 无回归 + 新增 `10_struct_value` 41 行、`11_i128` 34 行）
- **-O0/-O1 输出一致**（10/11 验证）
- **编译零警告**（MSVC /W4 /WX）
- Git 提交 `59c3944` 已推送 gitcode develop

## 三、环境与构建命令

- **cmake 不在 PATH**：需用 VS 自带环境：`cmd /c ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1 && "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build target/build --config Debug --target cn"`
- **重新配置 CMake**（新增测试文件后）：`cmake -S . -B target/build` 再 build
- **构建目标**：`cn`（主程序）、`cn_unit_tests`（单元测试）
- **单元测试运行**：`target\Debug\cn_unit_tests.exe --gtest_brief=1`
- **E2E 运行**：`python tests\e2e\run_e2e.py --cn target\Debug\cn.exe`（10/11 已自动发现）
- **编译 CN 源码**：`target\Debug\cn.exe build tests\e2e\10_struct_value\结构体值.cn -O1 --output target\10.exe`
- **查看 IR**：`target\Debug\cn.exe ir <文件.cn>`
- **注意**：cmd 重定向中文输出会乱码（代码页），用 Python `subprocess` + `encoding='utf-8'` 读取；findstr 匹配中文不可靠；cmd 无 tail/head，用 PowerShell `Select-Object -Last`

## 四、当前卡在哪

无卡点。基础语言缺陷完善A 全部完成并通过验证（549/549 单测 + E2E 11/11 + 已提交推送 gitcode develop `59c3944`）。

## 五、下一步计划

- **Task 2.8 字符串系统完善**（阶段2/3 后续）
- 全局值传播（需 SSA）、CSE、复写传播、跨块 DCE 增强、代数简化——优化器增强
- -O2/-O3 映射为 -O1 同级别（预留循环优化/内联/向量化）
- 结构体作为函数参数/返回（按值拷贝）已支持；**结构体数组元素按值传参已支持**；剩余：嵌套结构体按值返回（含数组字段）已支持（10_struct_value 验证）
- i128 乘/除/取余与全范围128位已支持；剩余：**i128 辅助函数入 stdlib/数学.cn**（阶段6）
- 大结构体（>64 字节）按值返回：当前返回缓冲区 64 字节上限，>64 字节结构体需扩容（后续 Task）

## 六、踩过的坑（绝对不要再踩，详见 lessons.md 权重）

1. **结构体组合场景步进/槽数必须按 typeSizeOf**（权重22.75）：结构体数组初始化列表、栈槽数（长度×元素槽数）、指针算术/`p[i]`/`p++` 步进、数组元素间距、结构体数组字段——全部按结构体总大小/字段元素大小，禁止 8 字节假设
2. **函数调用结果类型必须查语义层真实返回类型**（权重22.75）：funcReturnTypeOf
3. **ptr 常量恒64位**（权重22.75）：`ConstInt (ptr) [0]` 必须 mov rax（mov eax 只写低32位）
4. **长字符串 db 拆行**（权重22.75）：ml64 单行 db 过长 A2042；行尾不得加逗号 A2008；每行 ≤24 字节
5. **自定义类型作参数**（权重22.75）：parseParamDecl/parseFuncPtrType 参数类型必须用 parseTypeNameEx
6. **数组名退化**（权重22.75）：语义层二元运算/比较前数组类型必须退化为指针
7. **IR 层结构体信息推导三种形态**（权重20.8）：IdentifierExpr/MemberExpr/IndexExpr/BinaryExpr 都要能推出结构体类型
8. **epilogue 提前 return 分支必须补 ret**（权重22.75）：structReturn 分支漏 `mov rsp,rbp/pop rbp/ret` → 执行流落入下一函数 → 0xC00000FD 栈溢出
9. **结构体返回拷贝必须精确字节数**（权重22.75）：禁止 64 字节硬编码——16 字节结构体越界写 48 字节破坏相邻栈变量；用 IRFunction.structReturnSize
10. **StorePtr 目标类型推导必须覆盖 MemberExpr 数组字段对象**（权重22.75）：`出.分数[1] = v` targetType 若遗漏变 i64 → 8 字节写入覆盖相邻数组元素
11. **i128 字面量越界检查放语义层**（权重22.75）：含无后缀超 int64 自动提升场景（lastType_ 非整128 也检查）；整128 上限 2^127-1、正128 上限 2^128-1
12. **Win x64 影子空间**（权重31.2）：所有 call 前预留 32 字节
13. **MASM 内存操作数必须显式大小前缀**（权重31.2）：qword/dword ptr
14. **UTF-8 中文前缀字节偏移**（权重20.8）：compare/substr 偏移用字节数
15. **x86-64 写32位寄存器清零高32位**（权重20.8）：地址在 rax 时值加载用 rcx 系列
16. **数组布局方向必须全局一致**（权重26）：C 语义 = Add 方向 + codegen 逆序登记
17. **运行时**（后续补充）
