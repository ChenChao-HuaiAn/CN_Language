# HANDOFF 交接文档

**交接时间**: 2026-08-24 16:43 CST（arena分配器方案进行中，换电脑暂停）

## 我们在做什么任务

CN 语言编译器的**运行时内存管理重构**--使用 tracked 注册表方案实现 `内存::释放全部()`，解决 78_chain_build 编译 5 大组件时内存累积 OOM。

## 已经完成了什么

### 1. arena 统一分配方案（已废弃）
- 尝试让 `cn_alloc`/`cn_realloc`/`cn_alloc_tracked`/`__cn_object_new` 全部使用 arena bump 分配
- `cn_free`/`cn_free_tracked`/`__cn_object_delete` 变为空操作
- **结果**：78_chain_build 内存峰值从 12.3GB 降到 193MB，但 `内存::释放全部()` 后段错误（arena 释放了向量数据数组）
- **教训**：arena 模式下 `cn_free` 是空操作，破坏了 C++/Rust 式的正常 RAII 内存管理

### 2. tracked 注册表方案（当前方案，已实现但有 bug）
- `cn_alloc`/`cn_realloc`/`cn_free` 恢复使用 `std::malloc`/`std::realloc`/`std::free`（正常 RAII）
- `cn_alloc_tracked` 分配时注册到全局链表（`TrackedNode`）
- `cn_free_tracked` 从链表移除并 `std::free`
- `__cn_alloc_reset()` 遍历链表逐个 `std::free`，然后清空链表
- `__cn_object_new`/`__cn_object_delete` 恢复 `std::malloc`/`std::free`

### 3. 当前测试结果
- **单元测试**：60个内存/运行时/对象/字符串测试全部通过
- **78_chain_build**：
  - 内存峰值 193MB（之前 12.3GB），内存问题已解决
  - 运行时间 3秒（之前 2分34秒），速度大幅提升
  - **段错误**：前两次 `内存::释放全部()` 成功（释放 944K + 2.96M 个 tracked 分配），第三次段错误

### 4. 段错误根因分析（未解决）
- `内存::释放全部()` 释放了所有 tracked 内存（字符串），但 `78_chain_build` 中可能有 tracked 字符串在 `内存::释放全部()` 后仍被访问
- 或者 `cn_realloc_tracked` 的链表更新有 bug（`std::realloc` 内存复用导致重复节点）
- `__cn_read_line` 中使用了 `cn_realloc_tracked` 扩容（第31行），这可能导致链表中的指针失效

## 修改的文件清单

1. [`src/runtime/io_api.cpp`](src/runtime/io_api.cpp) - 核心修改：
   - 添加 `TrackedNode` 链表结构
   - `cn_alloc`/`cn_realloc`/`cn_free` 恢复 `std::malloc`/`std::realloc`/`std::free`
   - `cn_alloc_tracked` 注册到链表
   - `cn_free_tracked` 从链表移除
   - `cn_realloc_tracked` 更新链表（可能有问题）
   - `__cn_object_new`/`__cn_object_delete` 恢复 `std::malloc`/`std::free`
   - 实现 `__cn_alloc_reset()` 遍历链表释放
2. [`src/runtime/time_api.cpp`](src/runtime/time_api.cpp) - `std::malloc` 改为 `cn_alloc_tracked`

## 当前卡在哪

**段错误未解决**。78_chain_build 运行时第三次 `内存::释放全部()` 段错误。

### 下一步调试方向
1. **检查 `cn_realloc_tracked` 的链表更新**：`std::realloc` 可能返回新地址，`trackedUnregister(ptr)` + `trackedRegister(new_p)` 可能导致重复节点（内存复用）
2. **检查 `__cn_read_line` 和 `__cn_file_read_line`**：它们使用 `cn_realloc_tracked` 扩容，可能导致链表中的指针失效
3. **考虑简化方案**：`cn_realloc_tracked` 不更新链表，而是 `cn_alloc_tracked` 新块 + `memcpy` + `cn_free_tracked` 旧块

## 下一步计划

### 近期（在 Win10 x86 上继续）
1. **修复 78_chain_build 段错误**：
   - 调试 `cn_realloc_tracked` 的链表更新逻辑
   - 或者改用 `cn_alloc_tracked` + `memcpy` + `cn_free_tracked` 替代 `cn_realloc_tracked`
2. **全量 E2E 和单元测试验证**
3. **Git 提交并推送 gitcode develop 分支**

### 中长期
1. **C++侧平台抽象**（plans/009子任务3）：创建 platform.hpp + windows.cpp + linux.cpp + 重构 cn_main.cpp/module.cpp/cargo_parser.cpp
2. **79_bootstrap_closed_loop**：Win10 x86 有 MSVC 工具链，可以运行此用例
3. **自举IR生成/代码生成通用化**（P5-31待办）

## 换电脑后如何继续

### 环境差异
- **当前**：麒麟ARM64，g++ 9.3.0，无MSVC工具链
- **目标**：Win10 x86，有MSVC 2022（ml64/link/cl）

### 代码兼容性
- 项目代码完全跨平台（C++ + CMake + #ifdef _WIN32 分支）
- `cn build` 命令在 Win10 上用 MSVC 工具链（cl/ml64/link）
- 在 ARM64 上跳过的用例（62_ffi、79_bootstrap_closed_loop）在 Win10 上可以运行

### 平台抽象层方案
- **自举侧（CN版）**：已完成，5组件已创建（后端接口/后端工厂/IR遍历/x64后端/arm64后端）
- **C++侧**：待实施，30处 #ifdef 待重构为 platform.hpp 体系
- 详见 `plans/009 CN语言编译器 平台抽象层重构方案.md`

## 踩过的坑（绝对不要再踩）

1. **CN语言语法必须查规范** - 编写CN语言代码前必须调用 cn-language-spec skill 查询 spec 06 OOP规范
2. **ARM64后端emitNewObject必须填充接口分派区** - 权重16.8
3. **ARM64窄类型加载必须区分有符号/无符号** - 有符号用xN目标（ldrsb xN），无符号用wN目标（ldrb wN）。权重16.8
4. **ARM64窄类型存储用64位str xN** - 避免 strb/strh 只写部分字节导致高字节残留垃圾
5. **麒麟ARM64缺少g++符号链接** - 检查 `which g++`，若不存在但 `which g++-9` 存在则创建符号链接
6. **GCC 9 system()返回值必须处理** - `-Werror=unused-result` 对 system() 报错
7. **arena模式破坏RAII** - arena模式下cn_free是空操作，向量扩容的旧数据不释放，内存累积。学习C++/Rust：保持std::malloc/realloc/free的正常RAII语义
8. **平台限制用例通过PLATFORM_SKIP跳过** - 在run_e2e.py中添加PLATFORM_SKIP字典
9. **代码生成.cn拆分为门面+后端抽象层** - 新增平台应通过抽象层而非直接修改现有代码
10. **__cn_alloc_reset之前未实现** - 头文件声明了但运行时.cpp中没有实现，导致78_chain_build OOM

## Git状态

- **分支**: develop
- **未提交工作**: tracked注册表方案实现（io_api.cpp + time_api.cpp），78_chain_build段错误未解决
- **验证基线**: 单元测试60个内存相关测试通过，78_chain_build内存峰值193MB但段错误
---

## 2026-08-24/25 会话：缺陷修复记录（E2E 107/109 + 单测 1196/1196）

### 已完成修复
1. **tracked 注册表结构性加固（src/runtime/io_api.cpp）**：手写链表 → unordered_set + size map（天然去重、O(1) 增删、reset 按值收集释放）
   - cn_free_tracked 仅当 ptr 确实在册才释放（杜绝 reset 后遗留释放 double-free）
   - cn_realloc_tracked 恢复 std::realloc（避免 malloc+memcpy 堆碎片），换址原子更新注册表，原地扩展仅更新 size
2. **run_e2e.py 内存防护 + 管道防死锁**：78/79 等重负载用例轮询工作集，超 4096MB（--max-mem-mb 可调）立即 taskkill 判失败；轮询期间后台线程持续排空 stdout/stderr 管道（此前 64KB 管道缓冲写满导致子进程挂死"卡电脑"）
3. **CMakeLists.txt**：新增 cn_runtime_79_objs 目标，每次构建自动重编 /MT 静态运行时 obj 到 target/（消除 79 链接的陈旧 obj）
4. **MSVC 可移植性修复（单测）**：test_input_api.cpp 的 dup/dup2/close/fileno → _dup/_dup2/_close/_fileno；test_bootstrap_lexer.cpp 的 realpath/PATH_MAX → _fullpath + 复用 module::readSourceFile（UTF-8 中文路径）——MSVC 14.44/SDK 26100 下全套构建+1196/1196 全绿
5. **79 主.cn 内存纪律**（已回退基线，见遗留）

### 遗留问题（组件级，非运行时）
- **78/79 组件链内存峰值 4GB+**：自举组件（IR生成/代码生成 处理 x64后端 656 行 + arm64后端 788 行合并源码）存在大字节字符串处理（O(n^2) 级），segment 78 修复后从"段错误"变为"内存超限被防护拦停"；79 修复版（加 reset）触发 reset-UAF → 已恢复 8/17 基线版并以防护兜底。根治需组件内部流式处理（IR生成 逐行而非整体拼接），列 P 级后续
- **79 真实闭环待组件性能修复后复验**（ml64/link 闭环本身完好）

---

## 2026-08-30 会话：v2 控制流（如果/当）完成（P6b）

### 已完成
1. **v2 词法（多字符运算符）**：== != <= >= && || ++ += -> :: 最长匹配；-> 单 token。
2. **v2 语法（控制流节点）**：如果/否则/否则如果、当、赋值、自增（前/后缀）、一元 !/-、中断/继续；
   二元优先级 5 层；一元前缀仅表达式入口（修复 n-1 被解析为负号）。
3. **v2 语义**：检查如果/检查当（条件+块+否则链）；赋值/自增/一元/中断/继续 检查。
4. **v2 IR 生成（跳转/标签）**：函数IR 块计数/循环上下文；新块/发射跳转/条件跳转/标签；
   生成如果（真/假/汇合块）、生成当（条件/体/出口块）；预扫描跳过控制流体块区间（防重复生成）。
5. **v2 代码生成**：bbN 标签、jmp、条件跳转（mov eax/test/jnz/jmp）、一元；帧大小 +8（修 [rbp] 越界段错误）；
   扫描函数排除常量操作数（Symbol ID 非寄存器）。
6. **验证（真实链路）**：主(){如果==3→+1} 返回 4；阶乘(5) 当循环 返回 120。
   - 链接注意：v2 asm 入口须用 /ENTRY:mainCRTStartup（main 直连不初始化 CRT 会段错误）。

### 关键坑（已根治）
- **一元 - 与二元 - 歧义**：- 前缀只能在表达式入口解析，否则 n-1 被解析为 负号(1)。
- **体语句节点物理位置**：控制流语句的体语句节点在池中位于控制流语句节点之前，
  外层块区间误包含 → IR 生成需预扫描收集体块索引并跳过。
- **帧大小**：变量槽偏移 = 8*(最大寄存器+槽)+8，帧大小须 +8 覆盖（否则 [rbp] 越界段错误）。
- **条件区间**：如果/当 节点须存 条件终点（子起点），否则条件区间误包含真块体。

### 下一步
- 中断/继续 循环验证（已实现未实测）、指针/字段访问 → v2 编译自身。


---

## 2026-08-30 会话（续）：v2 指针/字段访问 完成（P6c）

### 已完成
1. **语法**：指针类型 `类型*`、`&` 取地址/`*` 解引用 前缀、结构体字段偏移表（字段名→偏移 8 字节槽）、
   `解析后缀链` 提取（`a + b.c` 二元操作数支持后缀）、成员/解引用左值赋值（`位置.x = 7`、`*p = 10`）、
   可选分号 `;` 消费、变量声明支持 `类型* 名`。
2. **语义**：支持 取地址/解引用/成员/赋值(文本=0) 节点检查；成员偏移由语法层算好存节点附加（语义层不写 AST）。
3. **IR**：IR_取地址(lea)/指针加载/指针存储/字段地址 发射；`生成左值地址` 辅助（顺序遍历左值区间）；
   成员对象索引存子起点；`&对象` 子区间预扫描跳过；赋值后清表达式栈。
4. **代码生成**：lea/ptrload/ptrstore/fieldaddr（add 偏移）。
5. **验证**：`点{位置.x=7; 位置.y=9; 指针=&位置.x; *指针=10; 返回 位置.x+位置.y}` = **19**；控制流回归 阶乘(5)=120。

### 关键坑（已根治）
- **成员节点基址崩溃**：成员节点需存对象索引（子起点），否则基址取 Load 值（如 7）当地址解引用 → 段错误。
- **`&位置.x` 重复生成**：取地址子区间（位置、x）被线性遍历生成 Load/字段/加载（其中加载解引用垃圾值崩溃）
  → 预扫描跳过子区间。
- **`*指针` 左值根识别**：`生成左值地址` 若区间末尾是解引用根，其操作数即地址（跳过前面标识符的取地址）。
- **宿主编译器嵌套泛型 mangle 超长**（ml64 identifier too long）：函数参数含 `映射<整64,整64>& 字段表` 时
  MSVC 名字修饰超 247 字符 → 字段表改由语法层扁平映射（字段名→偏移），IR/语义层不传表。
- **`7 位置.y` 误解析**：`解析二元链` 无条件解析左操作数原子，导致后续标识符被误生成 → 仅当当前位置是
  二元运算符才进二元链。

### 下一步
- 中断/继续 循环实测（IR 已支持未验证）-> v2 编译自身。
