# HANDOFF - CN语言编译器项目交接文档

> **交接原因**：2026-08-13 会话结束（阶段2收尾完成：7个Task + 09_integration 综合用例 + Debug 全面审查修复 10 个 BUG，单元测试 508/508、E2E 9/9，已提交推送 gitcode develop）。本文档写给完全没有上下文的新会话看，帮助快速恢复开发。

## 一、我们在做什么任务

正在开发CN语言编译器——一门全中文语法的系统级编程语言，参考C++编程范式，用C++从零编写编译器，直接生成汇编代码（Win x64 MASM，ml64 汇编）。

## 二、已经完成了什么

### 2.1 设计规格书（已批准）
- 文件：`plans/001 CN语言编译器设计规格书.md`（984行，14章）
- 关键字计数已从 53 更新为 54（新增"联合体"）

### 2.2 cn-language-spec 技能（已创建）
- 目录：`.ai-coder/skills/cn-language-spec/`（16个文件）
- **编写任何 CN 语言相关代码前必须调用此技能查规范**

### 2.3 阶段零/一（已完成）
- Task 0.1/0.3/1.1~1.10 全部完成（CLI框架/诊断/词法/语法/语义/IR/X64代码生成/运行时/driver集成）

### 2.4 阶段二：Task 2.1~2.7（已完成 ✅）
- **Task 2.1**：控制流完整；**Task 2.2**：函数完整（原型/递归/函数指针）
- **Task 2.3**：类型系统完整；**Task 2.4**：数组与指针完整
- **Task 2.5**：字符串系统完整；**Task 2.6**：优化器基础（常量折叠+DCE）
- **Task 2.7**：结构体/枚举/联合体完整

### 2.5 阶段2全链路集成验证（本次完成 ✅）
- **综合用例** `tests/e2e/09_integration/综合集成.cn`：一个"学生成绩管理系统"程序串联阶段2全部功能
  - 结构体（字符串/浮点/枚举字段）+ 结构体数组初始化列表 + 结构体指针算术遍历 + `->` 成员访问 + 函数指针回调排序（索引数组）+ 递归求平均分 + 枚举负数/选择case + 联合体共享内存 + 位宽整数/浮点/位运算/后缀/隐式转换 + 字符串API + 原始/多行字符串 + 空指针判空 + 常量折叠/DCE
- **发现并修复 9 个跨组件集成 bug**（详见 更新日志.md / lessons.md）：
  1. 结构体数组初始化列表只写占位0 → `emitStructInitTo` 逐字段写入
  2. 结构体数组栈槽数不足（长度×元素槽数）
  3. 结构体指针算术/`p[i]`/`p++` 固定8字节 → 新增 `ptrElemStride` 按 typeSizeOf
  4. 结构体/枚举作函数参数解析失败 → parseParamDecl 用 parseTypeNameEx
  5. 函数指针参数含结构体指针解析失败 → parseFuncPtrType 用 parseTypeNameEx
  6. 数组名退化缺失（名单 + 人数）→ 语义层 visitBinaryExpr 数组退化
  7. 长字符串 db 单行超长 A2042 + 尾逗号 A2008 → 按24字节/行拆分
  8. 用户函数返回 f64 误标 i32（eax 读 xmm0 除零崩溃）→ funcReturnTypeOf
  9. 空指针常量0存32位判空失败 → ConstInt ptr 强制64位
- **单元测试** `tests/unit/parser/test_parser_struct_param.cpp`（+4：自定义类型作参数/函数指针复合参数）
- **测试状态**：单元测试 **492/492**、E2E **9/9**（01~09 全过）、CLI 全命令验证通过、-O0/-O1 输出一致

### 2.6 阶段2 Debug 全面审查（本次完成 ✅）
- **模拟生产环境**：10 个审查用例（div_zero/bounds/edge_values/float_param/recursion_combo/semantic_errors/opt_correctness/string_edge/combo_hard/combo_arrow）+ 全部 E2E 01~09 + 单测 492/492，发现并修复 **10 个 BUG**：
  1. 除零未插桩（错误码1）→ 0xC0000094 崩溃 → `emitDivMod` 除零检查
  2. idiv 立即数 A2001 → 常量除数先 mov ecx
  3. 无符号除法用 idiv+有符号扩展（正32 4000000000/2 错算）→ 无符号 div + xor edx,edx
  4. 无符号比较用 setg/setl → seta/setb 系列
  5. u32 LoadPtr movsxd 符号扩展 → mov eax 零扩展
  6. 浮点参数经 rcx 读（caller 用 xmm0-3）→ `emitParamSetup` 从 xmmN 读
  7. 浮点返回只 mov rax 未设 xmm0 → epilogue `currentReturnType_` movsd xmm0
  8. const_fold 尾部实参传播浮点常量 A2050 → 尾部传播排除浮点/i128
  9. const_fold 无符号除/比较用有符号语义（-O1 不一致）→ 无符号分支
  10. 结构体数组字段当值 LoadPtr + 越界不插桩 → `memberObjStructType` 数组字段退化 + 步进按元素大小 + 越界插桩
- **测试状态**：单元测试 **508/508**、E2E **9/9**、-O0/-O1 输出一致、编译零警告（MSVC /W4 /WX）
- **收尾**：文档一致性验证（plans 打勾/关键字54/更新日志/HANDOFF/lessons）、临时产物清理（check_out.txt + target 审查用例）、Git 提交推送 gitcode develop

## 三、环境与构建命令

- **cmake 不在 PATH**：需用 VS 自带环境：`cmd /c ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1 && "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build target/build --config Debug --target cn"`
- **重新配置 CMake**（新增测试文件后）：`cmake -S . -B target/build` 再 build
- **构建目标**：`cn`（主程序）、`cn_unit_tests`（单元测试）
- **单元测试运行**：`target\Debug\cn_unit_tests.exe --gtest_brief=1`
- **E2E 运行**：`python tests\e2e\run_e2e.py --cn target\Debug\cn.exe`（09_integration 已含）
- **编译 CN 源码（-O0/-O1）**：`target\Debug\cn.exe build tests\e2e\09_integration\综合集成.cn -O1 --output target\09_integration_O1.exe`
- **查看 IR**：`target\Debug\cn.exe ir <文件.cn>`
- **注意**：cmd 重定向中文输出会乱码（代码页），用 Python `subprocess` + `encoding='utf-8'` 读取；findstr 匹配中文不可靠，用 read_file 直接读文件

## 四、当前卡在哪

无卡点。阶段2全部完成并通过收尾（508/508 单测 + E2E 9/9 + Debug 审查 10 bug 修复 + 已提交推送 gitcode develop）。

## 五、下一步计划

- **Task 2.8 字符串系统完善**（阶段2/3 后续）
- 全局值传播（需 SSA）、CSE、复写传播、跨块 DCE 增强、代数简化（x+0/x*1）——优化器增强
- -O2/-O3 映射为 -O1 同级别（预留循环优化/内联/向量化）
- 结构体作为函数参数/返回值（按值拷贝）：规格书 001 7.4，留后续 Task
- 结构体整体赋值/拷贝未实现（09 用例用索引数组规避）——后续 Task 可补
- i128 乘/除/取余与全范围128位（>2^63）需运行时辅助函数（__cn_mul_i128 等，规格书10.5）

## 六、踩过的坑（绝对不要再踩，详见 lessons.md 权重）

1. **结构体组合场景步进/槽数必须按 typeSizeOf**（权重22.75）：结构体数组初始化列表、栈槽数（长度×元素槽数）、指针算术/`p[i]`/`p++` 步进、数组元素间距、结构体数组字段——全部按结构体总大小/字段元素大小，禁止 8 字节假设
2. **函数调用结果类型必须查语义层真实返回类型**（权重22.75）：用户函数返回 f64 时 IR 误标 i32 → codegen 用 eax 读 xmm0 → 除零崩溃（funcReturnTypeOf）
3. **ptr 常量恒64位**（权重22.75）：`ConstInt (ptr) [0]` 必须 mov rax（mov eax 只写低32位，64位比较读到高位残留判空失败）
4. **长字符串 db 拆行**（权重22.75）：ml64 单行 db 过长 A2042；行尾不得加逗号 A2008；每行 ≤24 字节
5. **自定义类型作参数**（权重22.75）：parseParamDecl/parseFuncPtrType 参数类型必须用 parseTypeNameEx（支持 学生* / 学生[5] / 学生* 函数指针参数）
6. **数组名退化**（权重22.75）：语义层二元运算/比较前数组类型（学生[5]）必须退化为指针（学生*）
7. **IR 层结构体信息推导三种形态**（权重20.8）：IdentifierExpr/MemberExpr/IndexExpr/BinaryExpr（指针算术结果）都要能推出结构体类型，否则 decl==nullptr 静默返回0
8. **非 SSA IR 折叠引用点同步替换**（权重20.8）：常量折叠后必须同步替换所有引用
9. **DCE 必须函数级引用**（权重16.8）：跨块引用
10. **Win x64 影子空间**（权重31.2）：所有 call 前预留 32 字节
11. **MASM 内存操作数必须显式大小前缀**（权重31.2）
12. **UTF-8 中文前缀字节偏移**（权重20.8）：compare/substr 偏移用字节数
13. **x86-64 写32位寄存器清零高32位**（权重20.8）：地址在 rax 时值加载用 rcx 系列
14. **数组布局方向必须全局一致**（权重26）：C 语义 = Add 方向 + codegen 逆序登记
15. **运行时 obj 缓存**：改 io_api.cpp/string_api.cpp 后删 target 下旧 obj
16. **可选分号歧义**：`*p = v` 前必须加分号；裸表达式语句后加分号避免下一行 `(` 被吸收
17. **cmd 中文输出乱码**：重定向用 Python UTF-8 读取，findstr 匹配中文不可靠
