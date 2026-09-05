# plans/016 — linux-x86_64 后端专项（System V AMD64 ABI + GAS）

> 立项依据：用户裁决方案A（2026-09-05，深度系统 x86_64 本机呈报后）。
> 目标：宿主编译器新增 `linux-x86_64` 目标平台后端，使 CN 程序可在 x86_64 Linux 上
> **原生编译→汇编→链接→运行**（本项目第三个后端，也是首个在本机可全链闭环验证的后端）。
> 关联：plans/002 阶段5（linux-arm64 后端同构参照）、plans/014 状态表、AGENTS §1 双目标（性能/安全）。

## 一、架构决策（关键裁决记录）

| 决策点 | 裁决 | 依据 |
|---|---|---|
| 代码位置 | **独立目录** `src/cn_compiler/codegen/linux_x64/`，7 个新文件 | 本机无 MSVC，win-x64 门禁无法在本机回归——**禁止改动 x64/arm64 现有实现**（含头文件），`backend_factory.cpp` 分发处为唯一触碰点（纯增量分支，win/arm64 代码路径零变化） |
| 汇编语法 | GAS **`.intel_syntax noprefix`** | 与 win-x64 MASM 指令文本最大同构（mov/add/call 直接复用语义），移植出错面最小；GAS 原生支持，`as`/`g++` 链接无感 |
| ABI | **System V AMD64**（SVG ABI 1.0） | 平台标准；与 Win x64 ABI 差异见下表 |
| 寄存器策略 | **全栈槽**（%vN → `[rbp-8N-8]`），无寄存器分配 | 与 x64/arm64 双后端一致的正确性优先策略 |
| i128 | 双槽模型 + `__cn_*_i128` 运行时辅助（指针式 API） | IR 层平台无关，直接对齐 arm64_codegen_i128.cpp；x86_64 有 adc/sbb |
| 优化/调试信息 | regAlloc 恒关（对齐 arm64 阶段C 决策）、debugInfo 支持 | 正确性最高优先 |

## 二、Win x64 vs SysV x86_64 ABI 差异清单（实现对照）

| 维度 | Win x64（现有） | SysV x86_64（本专项） |
|---|---|---|
| 整型/指针参数 | rcx, rdx, r8, r9（4 个） | **rdi, rsi, rdx, rcx, r8, r9（6 个）** |
| 浮点参数 | xmm0–xmm3（与整型**共享槽位号**） | **xmm0–xmm7，整型/浮点独立计数**（第 N 个浮点参数 → xmm[浮点序号]） |
| 隐藏返回指针 | rcx（第 1 参数位） | **rdi（第 1 参数位）**，返回值 rax = retbuf 地址 |
| 结构体按值参数 | 指针传入（>8 字节） | 同（IR 层已统一指针传递，无差异） |
| 栈参数 | 第 5 参起 `[rbp+16+8k]` | **第 7 参起** `[rbp+16+8k]`（返回地址8+saved rbp8 锚定相同） |
| caller 预留 shadow space | 前 4 参 32 字节 | **无** |
| 栈对齐 | 8 字节即可 | **call 前 rsp % 16 == 0**（frameSize 16 对齐 + 调用点栈参数区 16 对齐垫片） |
| 被调用者保存 | rbx, rdi, rsi, rbp, r12–r15 | 同（rbx 存 retbuf 须 push/pop，与 saved rbp 合计 16 字节保持对齐） |
| 链接 | ml64 + link（/ENTRY:WinMainCRTStartup） | as + g++ **-no-pie**（同 linux-arm64；符号绝对寻址经 `lea reg,[rip+sym]` 可 PIE 兼容） |

## 三、后端寄存器约定（linux_x64 后端内部）

- 帧指针 rbp；栈槽 `[rbp-8N-8]`（regSlotOffset 同 arm64）；变量槽紧随寄存器槽区；`[rbp+16...]` 入参栈区。
- **栈槽寻址无 8/255 偏移限制**（disp32 全范围一条指令）——arm64 的 x13 大偏移间接寻址机制**整体不需要**，`stackMemText` 恒返回 `[rbp+off]`。
- 临时寄存器（全 caller-saved，指令边界虚拟值一律落栈槽，不跨指令存活）：r10（主临时）/ r11（次临时 + 间接调用函数指针 + i128 常量临时区寻址）/ r9（第三临时）。
- 除法专用：rax（被除数）+ rdx（cqo/xor edx,edx）；商 rax、余 rdx → 栈槽。移位量：cl（`shl r10, cl`）。
- 浮点临时：xmm0/xmm1；常量池加载 xmm7（地址经 rax `lea rax,[rip+label]`）。
- 符号地址装载：`lea r10, [rip+sym]`（-no-pie 与 PIE 均合法，比 mov offset 更鲁棒）。

## 四、任务拆分（对齐 arm64 六文件结构 + 平台接入）

- [x] T1 `linux_x64_codegen.hpp`：类 `LinuxX64CodeGenerator : public Backend` + `LinuxX64AsmWriter`（`#`→`//` 注释差异：GAS Intel 语法 `#` 是行注释起始，统一 `//`）
- [x] T2 `linux_x64_codegen.cpp`：符号修饰（nameMangle/labelMangle/mangleTypeCode/symbolName 照抄 arm64 版语义）、栈槽、模块装配（.data/.rodata/.text/.intel_syntax 头）、函数框架（prologue：push rbp/mov rbp,rsp/[push rbx]/sub rsp,frame；paramSetup：SysV 双计数参数装载；epilogue）
- [x] T3 `linux_x64_instructions.cpp`：常量加载/整型二元/除余（cqo+idiv、xor+div、除零检查码1）/浮点/移位（cl）/Cast 全矩阵/比较（ucomisd+setcc，与 win x64 同映射）/Load/Store/AddrOf/FieldAddr（空指针码3）/LoadPtr/StorePtr/CopyStruct/Call（16 对齐栈参数区+隐藏 rdi）
- [x] T4 `linux_x64_codegen_dispatch.cpp`：emitInstruction 分派 + emitTerminator（test r10,r10; jz 假块; jmp 真块）
- [x] T5 `linux_x64_codegen_i128.cpp`：add/sub（add/adc、sub/sbb）+ 乘除余/比较（`__cn_*_i128` 辅助，r10 临时区）
- [x] T6 `linux_x64_codegen_oop.cpp`：NewObject/DeleteObject/VirtualCall（call [r11+槽*8]）/VtableAddr/接口分派区
- [x] T7 `linux_x64_codegen_vtable.cpp`：虚表 .section .rodata + 静态字段 .data（_cn_vtable_/_cn_static_ 符号同 arm64）
- [x] T8 接入：`backend_factory.cpp`（"linux-x86_64" 分支）、`CMakeLists.txt`（cn_compiler_codegen 加 7 文件）、`cn_main.cpp`（--target 校验/工具链守卫/帮助文案——as/g++ 路径 else 分支已天然覆盖）、`run_e2e.py`（平台跳过 linux-x86_64 + --target 校验 + v2 闭环守卫）
- [x] T9 单测 `tests/unit/codegen/test_linux_x64_codegen.cpp`（对齐 test_arm64_codegen.cpp 模式：最小模块/字符串常量/中文修饰/平台标识/框架/算术/比较分支/i128/OOP/栈槽映射断言）
- [x] T10 E2E：新增 `152_linuxx64_系统V调用约定`（SysV 分歧面专项：7 参数函数/浮点整型混合参数/结构体返回/i128/间接调用）；全量 `run_e2e.py --target linux-x86_64` 红→绿闭环
- [x] T11 门禁：构建零警告（GCC -Wall -Wextra -Werror）+ 单测全过 + E2E linux-x86_64 全量 + win-x64 单测不回归（本机可跑 1223 单测佐证）
- [x] T12 文档：更新日志/HANDOFF/plans/014 状态表/lessons

## 五、平台边界（本轮不做，防蔓延）

- **v2 自举编译器**（CN语言编译器v2/）的 x86_64 代码生成（代码生成X64Linux.cn）＝后续专项：本轮 v2 闭环 E2E 用例（119–150）在 linux-x86_64 目标下按平台跳过处理（v2p 仅支持 win-x64/linux-arm64 两目标）。
- 62_ffi（Windows API）、69_memory_management（运行时行为差异）、78_chain_build（v1 链）、79_bootstrap_closed_loop（依赖 ml64）按平台跳过——与 linux-arm64 同清单。
- 寄存器分配器接入（regalloc）：沿 arm64 决策默认关闭，留作后续优化轮。

## 六、验证路径（本机原生闭环，首次实现）

1. `cmake --build target/build` 零警告 → `cn_unit_tests` 全过。
2. `cn build tests/e2e/01_hello/hello.cn --target linux-x86_64` → 产出本机可执行 → 运行输出「你好，世界」。
3. `python3 tests/e2e/run_e2e.py --target linux-x86_64` 全量（预期：除平台跳过外全绿）。
4. `-O0/-O2` 双级别抽查（优化管线对后端透明）。

## 七、实施结果（2026-09-05 首轮完成，深度系统 x86_64 本机）

**门禁（全绿）**：构建零警告（GCC 12.3 `-Wall -Wextra -Werror`）+ 单测 **1238/1238**（原 1223 + 新增 15）
+ E2E linux-x86_64 全量 **154 用例：127 过 / 0 败 / 27 平台跳过**（v2 闭环 23 + 62_ffi/69_memory/78_chain/79_bootstrap 4——与 linux-arm64 同清单）。

**里程碑**：CN 程序首次在 x86_64 Linux 上**原生编译→汇编→链接→运行**全链闭环
（`cn build x.cn --target linux-x86_64` → 本机可执行直接运行）。

**实施中发现并根治的缺陷/差异（详见 lessons）**：
1. x86_64 **无 `imul r64,r64,r64` 三寄存器形式**（仅立即数三操作数）——乘法改双操作数 `imul r10, r9`。
2. **32 位类型比较**：i32 变量装载（清高32）与 64 位常量装载（全1）位模式不一致，负数枚举 `-1 == -1` 误判——非宽类型一律 `cmp r10d, r9d`（对齐 ARM64 w 寄存器策略的本质）。
3. **32 位除法**：同因，`cqo+idiv r64` 对清高32 的负数按正数除（-8/2 商 2147483644）——32 位类型 `cdq+idiv r9d`。
4. **prologue/epilogue 配对**：push rbp+rbx 时 `mov rsp,rbp` 后 rsp 在 saved rbp 处——push 顺序必须 rbx 先 rbp 后，epilogue `pop rbp` 先 `pop rbx` 后（顺序颠倒 ret 读垃圾，gdb 实证 rip=0）。
5. **movss/movsd 宽度前缀**：movss 配 qword ptr 汇编报错——常量池装载两处按类型分派。
6. **i128 常量走 .data 全局常量池**（L128cN）——不复制 ARM64 的 [sp,#0..31] 栈临时区（与最深变量槽理论重叠）。
7. **retbuf 用帧内固定区**（`retbufFrameOffset_`，对齐 win x64）——rsp 临时区 `add rsp` 后悬垂。
8. **形态 A retbuf 契约锚定**：用户结构体返回调用由 IR 层预插 retbuf 地址为 operands[0]（`调用 %v62 10 2 (void)`），后端原样传递自然落 rdi（SysV 隐藏指针位）——对齐 ARM64；win x64 第4路 calleeReturnsStruct 与本实现的差异（本后端三路判定）已在 ARM64 单位机全量 E2E + 本机全量 E2E 双平台锚定。

**遗留/后续**：v2 自举编译器的 linux-x86_64 代码生成（代码生成X64Linux.cn）=后续专项；win x64 第4路判定与形态 A 的交互建议家机复核一轮（本机无 MSVC 无法回归 win）。
