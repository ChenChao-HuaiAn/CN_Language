# HANDOFF - CN语言编译器项目交接文档

> **交接原因**：2026-08-13 会话结束（基础语言缺陷完善A完成：i128 完整支持 + 结构体按值传参/返回 + 结构体整体赋值；Task 2.8 字符串系统完善B完成：13个运行时字符串API；**优化器增强完善C完成：代数简化 + CSE + 复写传播 + 跨块DCE + 全局值传播 + -O2/-O3 级别区分；完善功能集成验证完成：14_integration2 综合用例 + 修复6个跨组件BUG，单测 660/660、E2E 14/14、56 组合 -O0/-O1/-O2/-O3 输出一致，提交 `8989012`**）。本文档写给完全没有上下文的新会话看，帮助快速恢复开发。

## 一、我们在做什么任务

正在开发CN语言编译器——一门全中文语法的系统级编程语言，参考C++编程范式，用C++17从零编写编译器，直接生成汇编代码（Win x64 MASM，ml64 汇编）。当前完成阶段2全部 + 基础语言缺陷完善A + 字符串系统完善B + 优化器增强完善C。

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

### 2.6 完善功能集成验证（本次完成 ✅ 2026-08-13，提交 `8989012`）
- **新增综合用例** `tests/e2e/14_integration2/员工档案.cn`：员工档案管理系统——48B 大结构体（i128 年薪 + 字符串姓名 + 整32[3] 数组）按值传参/返回、整体赋值（含修改隔离）、结构体指针数组冒泡排序（CopyStruct 交换 48B 元素）、i128 大数运算（乘/除/取余/负数）、字符串 13 API 组合、小结构体寄存器传参、优化 Pass 验证
- **修复 6 个跨组件 BUG**（集成组合场景暴露）：
  1. **i128 返回 + 结构体按值参数 ABI 错位**：`paramOffset = structReturn?1:0` 未涵盖 i128 返回（结构体参数从 rdx 传但被调方从 rcx 读）；i128 返回 prologue 未保存 rcx→r12（函数体内调用破坏 rcx → epilogue 崩溃 0xC0000005）→ paramOffset 涵盖 i128/u128 + prologue r12 + epilogue r12
  2. **LoadPtr/StorePtr 漏 i128 双槽分支**：结构体 i128 字段读写只 8 字节 → 加双槽分支（低64↔regSlot(id+1)、高64↔regSlot(id)）
  3. **emitCast 无"整数→i128"分支**：i32→i128 落默认 32 位 mov → 加符号扩展分支
  4. **i128 参数只存 8 字节**：emitParamSetup 对 i128 参数（寄存器/栈）从指针 rep movsb 16 字节
  5. **指针下标元素成员推导失败**：`名单[j].年薪`（`员工档案* 名单`）objSrcType 只认数组变量 → 补 `types::isPointer`（pointeeOf）
  6. **结构体下标赋值走 StorePtr**：`名单[j] = 名单[j+1]` 只存 8 字节 → 加结构体检测生成 CopyStruct
- **其他**：`semantic.cpp` 补 `funcParamTypesOf` 实现（声明未定义 → 链接错误）；`ir.cpp` i128 实参仅当目标参数非 i128 时才截断为 i64
- **新增单元测试** `test_x64_i128_struct.cpp`（+4）
- **验证**：单测 **660/660**、E2E **14/14**、01~14 × -O0/-O1/-O2/-O3 = **56 组合 FAILURES=0**、全 CLI（token/ast/check/ir/run）可用、编译零警告

### 2.7 Task 2.8 字符串系统完善B（本次完成 ✅ 2026-08-13）
- **规格书核对**：10.1 仅定义 5 个字符串 API（长度/比较/连接/复制/查找）；比较运算符仅整型与浮点变体，未定义字符串变体 → 不实现；按"常见字符串库补充并在规格书标注"新增 13 个 API
- **新增 13 个运行时字符串 API**（`src/runtime/string_api.cpp`）：
  - 子串 `__cn_str_sub`(字符串, 整64, 整64)→字符串（字节偏移，UTF-8 按字节）
  - 字典序 `__cn_str_cmp`(字符串, 字符串)→整64（<0/0/>0）
  - 大写 `__cn_str_upper`/小写 `__cn_str_lower`（ASCII，非ASCII原样）
  - 前缀 `__cn_str_starts_with`/后缀 `__cn_str_ends_with`/包含 `__cn_str_contains`→布尔
  - 修剪 `__cn_str_trim`（空格/制表/换行/回车）
  - 反转 `__cn_str_reverse`（UTF-8 安全：按字符逆序不拆字节）
  - 从整数 `__cn_str_from_int`/从浮点 `__cn_str_from_float`/从字符 `__cn_str_from_char`
  - 释放 `__cn_str_free`（封装 cn_free，nullptr 安全）
  - 内存语义：动态分配结果（子串/大写/小写/修剪/反转/从整数/从浮点/从字符）调用方负责释放
- **值语义决策**：字符串变量为指针，`字符串 t = s` 共享同一指针（与 C 一致，不深拷贝）；显式深拷贝用 `字符串复制`
- **格式化函数**：sprintf 风格变参 ABI 复杂暂缓（打印行多参数已覆盖格式化输出需求），后续标准库阶段
- **修改**：`semantic.cpp`（registerBuiltins 13 新API）、`ir.cpp`（中文名→符号映射 + resultType 映射；字符串释放 空类型用 emit）、`runtime.hpp`、`tests/unit/CMakeLists.txt`、规格书 10.1/10.3 标注、阶段2文档 Task 2.8 条目
- **新增测试**：`test_string_api_extra.cpp`（runtime 29）/`test_string_extra.cpp`（semantic 10）/`test_ir_string_extra.cpp`（ir 9）共 48+3 个；`tests/e2e/12_string_extra/字符串完善.cn`（27行期望）

### 2.7 Task 完善C 优化器增强（本次完成 ✅ 2026-08-13）
- **新增 5 个优化 Pass**（`src/cn_compiler/opt/`）：
  - **代数简化** `algebraic_simplify.hpp/.cpp`：整型恒等变换（x+0/0+x/x-0/x*1/1*x/x*0/x/1/x<<0/x>>0/x|0/x^0/x&-1→x 或 0；x-x/x^x→0；x&&假→假；x||真→真）；浮点/i128 保守跳过；常量结果原地替换、寄存器结果函数级替换链
  - **复写传播** `copy_propagation.hpp/.cpp`：块内 Store→Load 转发（唯一内部名精确匹配）；多槽/StorePtr/Call 保守清空
  - **块内 CSE** `cse.hpp/.cpp`：纯运算"操作码+类型+操作数身份"哈希复用；Load 复用（无写内存时）；浮点 CSE 仅 -O2 以上
  - **跨块 DCE 增强** `cross_block_dce.hpp/.cpp`：常量条件跳转折叠（真/假→无条件跳转）+ 入口可达性分析 + 不可达块整块删除（label 字符串互引无需修正索引）
  - **全局值传播** `global_value.hpp/.cpp`：函数级线性扫描常量 Store→Load 安全子集
- **共享工具** `opt_common.hpp`：isPureArith/constCanPropagate/normalizeIntText/replaceUses（寄存器无条件替换 + 常量白名单）
- **-O1/-O2/-O3 级别区分**（`runOptLevel`，driver 接入）：-O1=折叠+DCE+代数简化+复写传播；-O2=+CSE（含浮点）+跨块DCE；-O3=+全局值传播；`cn_main` -O2/-O3 不再映射 -O1，`--opt 0/1/2/3`
- **新增测试**：6 个单元测试（`test_algebraic` 20/`test_cse` 11/`test_copy_prop` 8/`test_cross_block_dce` 7/`test_global_value` 8/`test_opt_levels` 6，+56）；`tests/e2e/13_opt2/优化增强.cn` + `.expected`
- **验证**：单元测试 **656/656**（原600+新56）；E2E **13/13**；**01~13 全部用例 × -O0/-O1/-O2/-O3 = 52 组合输出完全一致**（`target/opt_verify/verify_all.py`，FAILURES=0，规格书12.4）；编译零警告（MSVC /W4 /WX）

### 2.8 测试状态
- **单元测试 656/656**（原 600 + 新增 56）
- **E2E 13/13**（01~12 无回归 + 新增 `13_opt2`）
- **-O0 vs -O1/-O2/-O3 输出一致**（52 组合全量验证）
- **编译零警告**（MSVC /W4 /WX）
- Git 提交 `59c3944` 已推送 gitcode develop（Task 2.8 + 完善C 尚未提交）

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

无卡点。基础语言缺陷完善A + Task 2.8 字符串系统完善B + 优化器增强完善C 全部完成并通过验证（656/656 单测 + E2E 13/13 + 52 组合 -O0/-O1/-O2/-O3 输出一致；Task 2.8 + 完善C 改动尚未提交推送）。

## 五、下一步计划

- **优化器增强完善C 已完成**：代数简化 + 复写传播（块内）+ 块内 CSE + 跨块 DCE（不可达块删除）+ 全局值传播（安全子集）+ -O2/-O3 级别区分
- **格式化函数**（sprintf 风格 `字符串格式化`）：变参 ABI 复杂暂缓，后续标准库阶段实现
- **优化器剩余**（需 SSA 或后续 Task）：SSA 构造与 Phi 节点、真正的全局数据流（前驱/多前驱交集）、跨块 CSE（支配关系）、循环优化（LICM/强度削减/不变量外提）、内联、跳转表、空块合并、-O3 向量化预留
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
17. **空类型返回的调用必须用 emit（result.id<0），不能用 emitResult**（Task 2.8 新增）：`字符串释放` 返回空类型，若用 emitResult 分配结果寄存器，codegen 会对 void 类型写返回值（widthFor 返回原寄存器生成多余 mov）→ 用 `emit` 直接发射，与打印行 void 展开一致
18. **运行时 API 扩展必须同步三处**（Task 2.8 新增）：runtime.hpp 声明 + string_api.cpp 实现 + semantic registerBuiltins 注册 + IR 名称映射与 resultType 映射（缺一不可，否则链接 undefined symbol / 类型错误）
19. **改 runtime 后必须删 target 下旧 obj 缓存**（权重15）：cn_main compileRuntime 带缓存（obj 新于 cpp 跳过），改 string_api.cpp 后必须删 target\string_api.obj 否则链接旧符号
20. **测试构造 IR 块必须有终止信息**（完善C 新增）：真实 IR 契约——每块 terminated=true + termKind（跳转/条件跳转/返回）；无终止的块会被 DCE 误删结果（%v9 无引用级联删除）且跨块 DCE 可达性分析从入口沿跳转目标遍历时"断链" → 误删可达块
21. **Pass 登记替换不得无条件报 changed**（完善C 新增）：CopyPropagation/GlobalValue 的 Load 命中登记替换时若无条件 changed=true，fixpoint 永不收敛（第二次运行 Load 结果已无引用仍报告修改）→ 登记不报修改，仅 replaceUses 实际替换引用点才报
22. **apply_diff 修改头文件时核对类声明完整性**（完善C 新增）：include 块附近插入易破坏类定义起始行（C2059 public 语法错误链）→ 修改后立即 read_file 复核结构
23. **静态成员函数不能访问非静态成员**（完善C 新增）：CSEPass::patternOf 原 static 访问 allowFloat_ → C2597；改成员函数
24. **优化器验证脚本产物放 target/**（完善C 新增）：target/opt_verify/verify_all.py（13 用例 × -O0/-O1/-O2/-O3 = 52 组合输出一致性，规格书12.4）
