# HANDOFF - CN语言编译器项目交接文档

> **交接原因**：2026-08-15 会话结束（**Debug 全面审查子任务已完成 ✅**——4 项前序记录缺陷全部根治：大栈帧 __chkstk / 泛型+函数指针 / 构造函数重载 / 泛型类方法循环遍历泛型字段。新增 E2E 39_chkstk/40_generic_funptr/41_ctor_overload/42_generic_field_loop + 单测 BigFrameEmitsChkstk/SmallFrameNoChkstk。单测 **1050/1050**、E2E **44/44** 全通过，构建 0 错误 0 警告）。
>
> **⚠️ gitcode 推送状态（2026-08-15 收尾子任务）**：`git push gitcode develop` 3 次重试均被服务端 403 资源限制拒绝（`No available resources at the moment`；`git ls-remote gitcode develop` 探活正常=读可用、写受限）。本地 develop 领先 origin/develop **9 个提交**（d61badb 方案C / d61badb~045853c 阶段6 8 个提交），**本地已安全保存**，待服务端恢复后执行 `git push gitcode develop` 即可。
>
> **前序里程碑**：阶段3「OOP 与错误处理」（901/901 单测、28/28 E2E）；阶段A「Linux ARM64」（918/918 单测）；阶段B 优化补全（949/949 单测）；阶段C 寄存器分配/调试信息（964/964 单测）；reg_alloc -O2 崩溃修复（965/965）；字符串转换函数命名优化（965/965）；打印函数族命名统一方案C（965/965）；**阶段6第1步：条件编译+数学库（987/987 单测、30/30 E2E）**；**第2步：核心库+容器库（987/987、32/32）**；**第3步：映射集合+算法库（987/987、34/34）**；**第4步：IO输入+文件库（1005/1005、36/36）**；**第5步：字符串扩展+时间+系统库（1048/1048、39/39）**；**第6步：全链路集成串联（1048/1048、40/40，本交接）**。

## 一、我们在做什么任务

正在开发CN语言编译器——一门全中文语法的系统级编程语言，参考C++编程范式，用C++17从零编写编译器，直接生成汇编代码。当前处于**阶段6「标准库」**（plans/002-阶段6-标准库.md）：第1~6步（条件编译/数学库、核心库+容器库、映射集合+算法库、IO输入+文件库、字符串扩展+时间+系统库、全链路集成串联）已完成。剩余 Task 6.5 剩余部分（stdlib/字符串.cn 常规包装）与 Debug 子任务（编译器缺陷修复，含本轮新发现的大栈帧缺陷）。

**本次任务**：阶段6第6步子任务——全链路集成串联（Task 6.11）：编写学生成绩统计工具综合命令行工具，使 9 个标准库模块全部串联可用，E2E 38_tool 验证全链路。

## 二、已经完成了什么

### 2.0 本轮核心任务：字符串扩展 + 时间 + 系统库（Task 6.5补充 + Task 6.9 + Task 6.10）（已完成 ✅，2026-08-15）

**字符串扩展（stdlib/字符串扩展.cn + string_api.cpp 扩展）**：

| 项 | 内容 |
|----|------|
| 运行时 | `src/runtime/string_api.cpp` 扩展：`__cn_str_to_int`（strtoll 整串解析 + 整32* 成功标志，拒绝 "123abc" 部分解析，仅空白判定非法）/ `__cn_str_to_double`（strtod 整串解析）/ `__cn_str_to_bool`（"真"/"假"/"true"/"false"，英文大小写不敏感） |
| stdlib | `stdlib/字符串扩展.cn`：`字符串转整数/转浮点/转布尔`（结果<T,整32> 包装，失败 错误(6)）+ `替换`（循环 字符串查找+子串+连接，目标不存在原样副本，安全计数 100 防无限循环）+ `填充左/右`（字符转字符串+循环连接）+ `分割`（字符串* 数组 + 数组长度，C 风格因 CN 数组长度须编译期常量；返回数量，数组不足 错误(6)） |
| E2E | `tests/e2e/35_string_ext/`（主.cn + 字符串扩展.cn 模块副本 + 主.expected）：内置直调解析 + 模块导入验证 替换/填充/分割 组合函数 |
| 单测 | `tests/unit/runtime/test_string_parse_api.cpp`（24 用例：合法/非法/前导空格/空串/超范围/空指针/中文布尔/英文大小写） |

**时间库（stdlib/时间.cn + time_api.cpp）**：

| 项 | 内容 |
|----|------|
| 运行时 | `src/runtime/time_api.cpp`：`__cn_time`（time(nullptr) 秒级）/ `__cn_clock_ms`（Windows QueryPerformanceCounter / POSIX clock_gettime(CLOCK_MONOTONIC)，#ifdef _WIN32）/ `__cn_time_format`（localtime_s/localtime_r + strftime，失败 nullptr，动态分配调用方释放） |
| stdlib | `stdlib/时间.cn`：`当前时间戳() -> 整64`、`单调时钟毫秒() -> 整64`、`格式化时间(时间戳, 格式) -> 结果<字符串,整32>`（失败 错误(6)） |
| E2E | `tests/e2e/36_time/`（主.cn + 主.expected）：**E2E 固定文本比对 → 程序内部断言只输出 真/假**（时间戳递增、格式化时间长度=10、分隔符 '-'、年份 4 位数字） |
| 单测 | `tests/unit/runtime/test_time_api.cpp`（9 用例：时间戳>0/递增、时钟毫秒非负/递增、格式断言、非法时间戳 nullptr） |

**系统库（stdlib/系统.cn + system_api.cpp）**：

| 项 | 内容 |
|----|------|
| 运行时 | `src/runtime/system_api.cpp`：`__cn_argc`/`__cn_argv`（全局缓存；Windows `GetCommandLineW` + `CommandLineToArgvW` 从进程原始命令行解析 + `WideCharToMultiByte(CP_UTF8)` 转 UTF-8 缓存；Linux 直接缓存 entry argv） |
| stdlib | `stdlib/系统.cn`：`参数个数() -> 整32`、`参数(整32 索引) -> 结果<字符串,整32>`（越界 错误(6)，字符串 CRT 持有不得释放） |
| E2E | `tests/e2e/37_system/`（主.cn + **主.args** + 主.expected）：run_e2e.py 扩展 `.args` 文件注入命令行参数（每行一个参数），验证 参数个数=3 + 中文参数 "CN语言" UTF-8 无乱码 |
| 单测 | `tests/unit/runtime/test_system_api.cpp`（10 用例：argc 计数、argv 索引、越界/负索引/未初始化、setter 往返） |

**验证结果**：构建 0 错误 0 警告（MSVC /W4 /WX）；单元测试 **1048/1048**（1005 + 43 新增）；E2E **39/39**（36 前序 + 35_string_ext + 36_time + 37_system）。

### 2.1 本轮新增内置函数五处同步清单（⚠️ 后续新增内置必须照做）

| 同步点 | 文件 | 内容 |
|--------|------|------|
| ① 语义注册 | `src/cn_compiler/semantic/semantic.cpp` registerBuiltins | `解析.转整数/转浮点/转布尔` + `时间.当前时间戳/单调时钟毫秒/格式化时间` + `系统.参数个数/参数`（带前缀限定名，数学库同模式） |
| ② IR 名称映射 | `src/cn_compiler/ir/ir.cpp` visitCallExpr | `解析.xxx`→`__cn_str_to_*`、`时间.xxx`→`__cn_time/__cn_clock_ms/__cn_time_format`、`系统.xxx`→`__cn_argc/__cn_argv` |
| ③ IR 结果类型映射 | `src/cn_compiler/ir/ir.cpp` | 转整数/时间戳/时钟毫秒/参数个数→i64、转浮点→f64、转布尔→i64（CN 层 Cast 布尔）、格式化时间/参数→ptr |
| ④ 运行时声明与构建 | `src/runtime/runtime.hpp` + `CMakeLists.txt` cn_runtime + `src/cn_main.cpp` compileRuntime 源文件数组 + linkExe .obj/.o 列表 | time_api.cpp + system_api.cpp 四处（x64/arm64）+ **shell32.lib（Windows CommandLineToArgvW）** |
| ⑤ 测试/文档 | E2E + 单测 + plans + 更新日志 | 35_string_ext/36_time/37_system E2E + test_string_parse_api/test_time_api/test_system_api 单测 |

### 2.2 本轮命名冲突与平台差异解决方案（⚠️ 重要，后续标准库/测试注意）

| # | 问题 | 解决 |
|---|------|------|
| 1 | **`字符串.转整数` 前缀编译失败** | `字符串` 是类型关键字（Kw_String），词法器将 `字符串.转整数` 拆为 关键字+标识符 报"预期表达式"（lessons 变量名前缀同类问题）——内置改为 `解析.` 前缀（`解析.转整数` 等），stdlib 模块公开纯名 `字符串转整数` 不冲突 |
| 2 | **`结果` 关键字不可作变量名** | 字符串扩展.cn 局部变量 `字符串 结果` 报"预期变量名，实际是 '结果'"（Kw_Result）——改 `缓冲` |
| 3 | **Windows WinMain 下 argc 恒 0** | `/ENTRY:WinMainCRTStartup` 的 WinMain 形参无 argc/argv（第4参是 nCmdShow），原 `entry(0, nullptr)` 导致 系统.参数个数 恒 0——改转发 MSVC CRT 全局 `__argc`/`__argv` |
| 4 | **Windows 中文参数乱码（GBK）** | `__argv` 是 ANSI（GBK 代码页）中文乱码；`__wargv`（CRT 宽参数）仅 wmain 入口初始化（WinMain 下 nullptr）——改 `GetCommandLineW` + `CommandLineToArgvW` 从进程原始命令行（Unicode）解析 + `WideCharToMultiByte(CP_UTF8)` 转 UTF-8 缓存（依赖 shell32.lib） |
| 5 | **E2E 固定文本 vs 时间动态值** | 时间戳/日期内容随运行时刻变化——E2E 程序内部断言（递增/长度/分隔符），只输出 真/假（1/0） |
| 6 | **字符串 == 运算符未定义** | 36_time 分隔符比较用 `字符串比较` 内置（规格书未定义字符串 == 运算符） |

### 2.3 前序里程碑（已全部完成 ✅）

- 阶段0~3 全部（901/901 单测、28/28 E2E、5 项缺陷全修复）
- 阶段4 优化与 Win x64 完善（949/949 → 964/964 单测）
- 阶段5「Linux ARM64」阶段A（ARM64 后端 + 本机全链路，918/918 单测）
- reg_alloc -O2 崩溃缺陷修复（965/965 单测、28/28 E2E）
- 字符串转换函数命名优化（965/965 单测、28/28 E2E）
- 打印函数族命名统一方案C（965/965 单测、28/28 E2E）
- 阶段6第1步：条件编译+数学库（987/987 单测、30/30 E2E）
- 阶段6第2步：核心库+容器库（987/987 单测、32/32 E2E）
- 阶段6第3步：映射集合+算法库（987/987 单测、34/34 E2E）
- 阶段6第4步：IO输入+文件库（1005/1005 单测、36/36 E2E）

## 三、当前测试基线

| 指标 | 数值 |
|------|------|
| 单元测试 | **1050/1050**（Win x64；1048 + 2 新增 __chkstk） |
| E2E | **44/44**（含 39_chkstk、40_generic_funptr、41_ctor_overload、42_generic_field_loop；Win x64） |
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
13. **结构体赋值（含结构体返回调用）必须 CopyStruct**：`读取结果 = 数据.读取(99)` 右值为 CallExpr 时 srcAddr=调用返回地址，按 size 字节 rep movsb
14. **构造函数重载 ✅ 已支持（Debug 子任务修复）**：构造/析构 methods 表 key=sigKey（名#参数串）多版本共存，语义/IR 按实参匹配；标准库类现可用构造重载
15. **打印函数族语义**：`打印(...)` = 结尾换行；`打印行(...)` = 不换行；`格式化(...)` = 格式化字符串。仅支持标量+字符串+字符变参
16. **条件编译指令**：`#定义`/`#如果定义(宏)`/`#否则`/`#结束如果`，行级裁剪保留行号，命令行 `-D 宏名` 注入
17. **数学库内置函数**：注册为带 `数学.` 前缀限定名，用户模块公开函数优先；`stdlib/数学.cn` 仅供导入使用
18. **运行时 .obj 清单**：cn_main.cpp compileRuntime 源文件数组 + linkExe .obj 列表必须同步（x64/arm64）
19. **标准库设计模式**：错误码枚举（`错误` 是内置构造器不可重定义）；指针解引用赋值用 `p[0]` 下标（行首 `*` 歧义）；元素按 8 字节槽存储；常量成员函数语法 `常量 函数 名() -> 类型`
20. **泛型+函数指针回调 ✅ 已支持（Debug 子任务修复）**：substTypeParam/substGenericType 保留函数指针参数列表后缀并递归替换内嵌 T；泛型排序/二分现可用回调
21. **泛型类方法内循环遍历第一个泛型字段 ✅ 已修复（Debug 子任务）**：IR emitClassMethod 设 genericTypeParams_（T->实参），语义 resolveGenericTypeName 补裸参数替换；类方法内循环/打印变参字段下标均正常
22. **打印变参内嵌泛型类字段下标 ✅ 已修复**：同泛型字段循环根因（IR genericTypeParams_ 缺失），E2E 42 验证打印盒 元素0/1/2 正确
23. **IO/文件内置函数**：注册为带 `IO.`/`文件.` 前缀限定名（数学库同模式）；E2E 直接调内置限定名（不导入 stdlib 模块）；`读取整数/读取浮点` 成功标志经整32* 输出
24. **中文路径文件读写**：`MultiByteToWideChar(CP_UTF8)` + `_wfopen_s`（std::filesystem::u8path 对中文路径挂起，弃用）
25. **E2E stdin 注入**：run_e2e.py 检测同名 `.input` 文件，subprocess input 参数注入
26. **字符串与 无 比较合法**：`字符串 != 无`（EOF 判定）语义层放行（字符串本质 char*，指针间比较按地址）
27. **内置函数限定名前缀不可用类型关键字**（本轮新增）：`字符串`/`整数`/`浮点` 等类型关键字不能作限定名前缀（`字符串.转整数` 词法拆分报错）——字符串解析内置用 `解析.` 前缀
28. **关键字不可作变量名**（本轮新增）：`结果`（Kw_Result）/`错误`/`正常` 等内置构造器关键字不可作变量名——标准库局部变量用 `缓冲` 等
29. **系统库命令行参数**（本轮新增）：Windows `GetCommandLineW` + `CommandLineToArgvW`（Unicode）+ `WideCharToMultiByte(CP_UTF8)` 转 UTF-8（`__argv` GBK 乱码、`__wargv` WinMain 下 nullptr）；依赖 shell32.lib
30. **E2E 命令行参数注入**（本轮新增）：run_e2e.py 检测同名 `.args` 文件（每行一个参数，首行=argv[1]，argv[0]=exe 名），subprocess 命令追加
31. **时间库 E2E 固定输出**（本轮新增）：时间戳/日期动态值 → 程序内部断言（递增/长度/分隔符）只输出 真/假；字符串分隔符比较用 `字符串比较` 内置

## 五、踩过的坑（绝对不要再踩）

1. **打印函数族命名"行"字语义已反转**（权重15.0，✅已修复）：`打印行`=不换行，勿按直觉理解为换行
2. **结构体返回调用赋值必须 CopyStruct**（✅已修复）：`变量 = 结构体返回调用()` 若漏 CopyStruct 只存 8 字节地址
3. **构造函数重载会互相覆盖**（✅已修复 2026-08-15）：构造/析构 methods 表 sigKey key 多版本共存，语义/IR 按实参匹配；标准库类现可用构造重载
4. **嵌套泛型字段 + 结构体返回方法崩溃**（⚠️规避）：组合类用独立基础字段实现（嵌套泛型字段 + 结构体返回方法组合仍建议规避，留待阶段7根治）
5. **物理寄存器宽度 A2022**（✅已修复）：codegen 中 `mov 32位寄存器, 物理寄存器` 必须转 32 位名（r12→r12d）
6. **CRLF 宏名带 `\r`**（✅已修复）：终止字符集合必须含 `\r`/`\n`
7. **中文字节长度比较**（✅已修复）：`compare(0,N,"中文")` 的 N 必须是字节数
8. **内置函数与模块函数冲突**（✅已修复）：用户模块公开函数优先；stdlib 模块体内不能直调同名内置（自递归）
9. **运行时新增源文件两处同步**（✅已修复）：compileRuntime 源文件数组 + linkExe .obj 列表（x64/arm64）
10. **泛型函数 + 函数指针参数不可用**（✅已修复 2026-08-15）：substTypeParam/substGenericType 保留函数指针后缀并递归替换内嵌 T；泛型排序/二分现可用回调（具体类型重载保留作性能选择）
11. **泛型类方法内循环遍历第一个泛型字段损坏**（✅已修复 2026-08-15）：IR emitClassMethod 设 genericTypeParams_（T->实参）+ 语义 resolveGenericTypeName 裸参数替换；类方法内循环/打印字段下标正常
12. **打印变参内嵌泛型类字段下标损坏**（✅已修复 2026-08-15）：同泛型字段循环根因，E2E 42 验证 打印盒 元素0/1/2 正确
13. **数组声明语法**：`整32[6] 数组 = { ... }`（类型前置 + 长度在类型括号内）
14. **变量名避免以类型关键字开头**：`整数成功` 词法拆分为 关键字+标识符 报"预期标识符"
15. **MSVC 单测 fopen/freopen 触发 C4996**（本轮新增）：用 `fopen_s`/`freopen_s` 或 `_dup`/`_dup2`
16. **strto* 仅空白输入误判合法**（本轮新增）：`"   "` strtoll 的 end 指向开头，须在跳过尾部空白**前**判定 `end != str`（先跳空白会误判为合法 0）
17. **内置限定名前缀不可用类型关键字**（本轮新增）：`字符串.转整数` 词法拆分报错（`字符串` 是 Kw_String）——用 `解析.` 前缀
18. **`结果`/`错误`/`正常` 等关键字不可作变量名**（本轮新增）：`字符串 结果` 报"预期变量名"——用 `缓冲`
19. **Windows WinMain 无 argc/argv**（本轮新增）：`/ENTRY:WinMainCRTStartup` 下 WinMain 形参无 argc/argv——用 `GetCommandLineW` + `CommandLineToArgvW`（依赖 shell32.lib）
20. **Windows __argv GBK 乱码 / __wargv 仅 wmain 初始化**（本轮新增）：中文命令行参数用 `WideCharToMultiByte(CP_UTF8)` 转 UTF-8 缓存
21. **大栈帧无 __chkstk 栈探测崩溃**（✅已修复 2026-08-15 Debug 子任务）：codegen 栈帧（寄存器槽区+变量槽区，含打印变参展开临时寄存器）超 Windows 栈 guard 页（约 8KB）时 `sub rsp, N` 越过未提交栈页 → 0xC0000005（首个打印前即崩，无任何输出）。根治：栈帧 >4KB 发射 MSVC 三段式 `mov rax,N / call __chkstk / sub rsp,rax`（__chkstk 按 4KB 页探测提交，libcmt.lib 提供符号）；emitParamSetup 提前到 chkstk 前（chkstk 破坏 rcx）。回归 E2E 39_chkstk（1200 局部变量 >9KB 栈帧正常）。原规避（拆函数 <8KB）可取消
22. **跨模块重复声明 `错误码` 枚举冲突**（本轮新增）：容器.cn + 映射集合.cn 同目录同时导入 → 用例目录副本改 `导入 容器.错误码` 共享（删除自身枚举）
23. **模块级变量/泛型类参数不支持**（本轮新增）：数据跨函数传递用 数组指针参数（`整64*`/`字符串*`）；结构体按值返回/传参正常（40 字节 统计结果 已验证）

## 六、下一步计划

1. **阶段6「标准库」后续任务**（plans/002-阶段6-标准库.md）：
   - Task 6.5 剩余：stdlib/字符串.cn 常规包装（长度/比较/连接/复制/查找/格式化）——字符串 API 已全部内置（Task 2.5/2.8/2.9），此处仅提供 stdlib 模块包装
   - 数学库 P1：对数/反三角/随机数/复数（未做）
2. **Debug 子任务（编译器缺陷修复）✅ 已完成（2026-08-15）**：
   - **大栈帧无 __chkstk 栈探测 ✅ 已修复**：栈帧 >4KB 发射 `mov rax,N / call __chkstk / sub rsp,rax`（MSVC 三段式），E2E 39_chkstk 验证
   - **构造函数重载 ✅ 已修复**：构造/析构 methods 表 sigKey key + 实参匹配，E2E 41_ctor_overload 验证
   - **嵌套泛型字段 this 传递**：栈/队列已改独立字段规避（现有实现稳定，嵌套泛型字段 + 结构体返回方法组合仍建议规避，留待阶段7根治）
   - **泛型函数 + 函数指针参数 ✅ 已修复**：substTypeParam/substGenericType 保留函数指针后缀，E2E 40_generic_funptr 验证
   - **泛型类方法内循环遍历第一个泛型字段 ✅ 已修复**：IR emitClassMethod 设 genericTypeParams_，E2E 42_generic_field_loop 验证
3. **阶段5 阶段B/C**（plans/002-阶段5-LinuxARM64.md）：交叉编译/QEMU 验证、ARM64 优化对齐
4. **遗留风险**：
   - `stdlib/数学.cn`/`IO.cn`/`文件.cn`/`字符串扩展.cn`/`时间.cn`/`系统.cn` 模块体内 `数学.xxx`/`IO.xxx`/`文件.xxx`/`解析.xxx`/`时间.xxx`/`系统.xxx` 直调冲突（仅供外部导入使用；E2E 直接验证内置）
   - 接口附加 vtable 预留（多接口场景，未实现）
   - ARM64 E2E 在 x86 主机需交叉工具链/QEMU
   - **x64_instructions.cpp 存量超行数**：单文件超 1000 行约束，待后续拆分
   - system_api.cpp 转换后 argv 字符串 malloc 持有（进程生命周期，退出由 OS 回收——可接受；若需严格释放可在 entry 返回后清理，当前无机制）
   - **gitcode 远程推送受阻（2026-08-15 收尾子任务）**：服务端 403 资源限制（写资源不可用，ls-remote 读正常），本地 ahead 9 提交未推送，已安全保存，待服务端恢复后重推

## 七、关键文件索引

| 模块 | 文件 |
|------|------|
| 字符串解析运行时（扩展） | [`src/runtime/string_api.cpp`](src/runtime/string_api.cpp)（__cn_str_to_int/to_double/to_bool，strto* 整串解析 + 整32* 成功标志） |
| 时间运行时（新增） | [`src/runtime/time_api.cpp`](src/runtime/time_api.cpp)（__cn_time/__cn_clock_ms/__cn_time_format，平台 #ifdef _WIN32） |
| 系统运行时（新增） | [`src/runtime/system_api.cpp`](src/runtime/system_api.cpp)（__cn_argc/__cn_argv，Windows CommandLineToArgvW UTF-8 缓存） |
| 字符串扩展库（新增） | [`stdlib/字符串扩展.cn`](stdlib/字符串扩展.cn)（字符串转整数/转浮点/转布尔/替换/填充左/填充右/分割） |
| 时间库（新增） | [`stdlib/时间.cn`](stdlib/时间.cn)（当前时间戳/单调时钟毫秒/格式化时间） |
| 系统库（新增） | [`stdlib/系统.cn`](stdlib/系统.cn)（参数个数/参数） |
| 语义注册（修改） | [`src/cn_compiler/semantic/semantic.cpp`](src/cn_compiler/semantic/semantic.cpp)（registerBuiltins：解析./时间./系统. 内置注册） |
| IR 映射（修改） | [`src/cn_compiler/ir/ir.cpp`](src/cn_compiler/ir/ir.cpp)（解析./时间./系统. 名称映射 + 结果类型映射） |
| 运行时入口（修改） | [`src/runtime/runtime.cpp`](src/runtime/runtime.cpp)（entry 缓存 argc/argv；WinMain 转发） |
| E2E 运行器（修改） | [`tests/e2e/run_e2e.py`](tests/e2e/run_e2e.py)（.args 文件命令行参数注入） |
| 字符串扩展 E2E（新增） | tests/e2e/35_string_ext/（主.cn + 字符串扩展.cn + 主.expected） |
| 时间 E2E（新增） | tests/e2e/36_time/（主.cn + 主.expected，内部断言输出 真/假） |
| 系统 E2E（新增） | tests/e2e/37_system/（主.cn + 主.args + 主.expected，中文参数 UTF-8） |
| 字符串解析单测（新增） | tests/unit/runtime/test_string_parse_api.cpp（24 用例） |
| 时间单测（新增） | tests/unit/runtime/test_time_api.cpp（9 用例） |
| 系统单测（新增） | tests/unit/runtime/test_system_api.cpp（10 用例） |
| 计划文档 | plans/002-阶段6-标准库.md（Task 6.5 字符串处理库 ✅、Task 6.9 时间库 ✅、Task 6.10 系统库 ✅） |
