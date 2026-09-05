# HANDOFF 交接文档

**交接时间**: 2026-09-05 第十六轮（深度系统 x86_64 本机）——**plans/016 linux-x86_64 后端专项落地**（用户裁决方案A）。

## 本轮已完成（全部有实证）

### 一、里程碑：CN 程序首次在 x86_64 Linux 原生全链闭环
`cn build x.cn --target linux-x86_64` → 本机可执行直接运行（本项目第三个后端、首个在本机可完整验证的后端）。

### 二、新增后端（src/cn_compiler/codegen/linux_x64/ 7 文件）
- GAS `.intel_syntax noprefix` + System V AMD64 ABI；全栈槽映射；对齐 ARM64 六文件结构。
- SysV：整型 rdi,rsi,rdx,rcx,r8,r9（第7起栈）、**浮点 xmm0~7 独立计数**、隐藏返回指针 rdi+rax、16 字节对齐（needHiddenRet 分配 +8 偶数化）。
- i128 双槽（add/adc、sub/sbb + __cn_*_i128 辅助）；**i128 常量走 .data 全局常量池 L128cN**（不复制 ARM64 [sp,#0..31] 临时区隐患）。
- **retbuf 帧内固定区**（retbufFrameOffset_，对齐 win x64 2026-08 修复）。
- **形态 A 契约锚定**：结构体返回调用 IR 层预插 retbuf=operands[0]（`调用 %v62 10 2 (void)` 实证），后端原样传递自然落 rdi——本后端三路 hasBigRet（i128/u128/struct* result.type），对齐 ARM64；**与 win 第4路 calleeReturnsStruct 的差异见「呈报」**。

### 三、实施中根治六枚缺陷（汇编器/运行时/gdb 三层实证，已入 lessons）
1. x86 无 `imul r64,r64,r64` 三寄存器形式 → 双操作数 `imul r10, r9`。
2. 32 位类型比较位模式不一致（i32 变量清高32 vs 64 位常量全1，负数枚举 -1==-1 误判）→ 非宽类型 `cmp r10d, r9d`。
3. 32 位类型除法同因（-8/2 商 2147483644）→ `cdq+idiv r9d`（含途中补「提前 return 丢 store」回归）。
4. push/pop 配对：push rbx 先 rbp 后、pop rbp 先 rbx 后（颠倒 ret 读垃圾，gdb rip=0 实证）。
5. movss/movsd 宽度前缀按类型（两处装载点）。
6. 空块条件跳转防御（先 mov r10,0 再 test）。

### 四、门禁（全绿，本机深度系统 x86_64）
- 构建零警告（GCC 12.3 -Wall -Wextra -Werror）+ 单测 **1238/1238**（原 1223+15 新增）。
- E2E linux-x86_64 全量 **154 用例：127 过 / 0 败 / 27 平台跳过**。
- 新增 E2E **152_linuxx64_系统V调用约定**（SysV 分歧面 7 断言）+ 单测 test_linux_x64_codegen.cpp（15 项）。
- 接入：backend_factory/cn_main/run_e2e（平台跳过+自动推断）/CMakeLists——**全部纯增量，win/arm64 代码路径零变化**。

## 下一步
1. **v2 自举编译器的 linux-x86_64 代码生成**（代码生成X64Linux.cn）——v2 闭环 23 用例解锁，v2 三后端对齐。
2. **家机复核 win x64 第4路判定**（calleeReturnsStruct）与形态 A retbuf 契约的交互——本机无 MSVC 无法回归 win；若 win 侧存在形态 A 错位（argOffset=1 推 retbuf 实参到 rdx），24 错误处理用例应在家机复测锚定。
3. linux-arm64 侧复验：152 用例在单位机跑一轮（用例三平台语义一致，预期绿）。
4. B1a 契约分叉收口、v2 组件拆分（既有裁决不变）。

## 灰色点披露
- 78/79 v1 链内存失控——已知遗留勿翻案。
- 寄存器分配器（regalloc）linux-x86_64 未接入（沿 arm64 阶段C 决策默认关，正确性优先）。
- 递归同函数多次大返回调用共享同一 16 字节 retbuf 帧区——与 win x64 同款已知限制（顺序 IR 下安全）。

## 踩过的坑（本轮，全入 lessons）
- 终端肉眼验证输出会被 echo 拼接误导（换行缺失不可见）——E2E 逐行比对是唯一可信验收。
- x86_64 汇编器的 movss/movsd 宽度前缀、imul 三寄存器形式缺失——MASM 直译不安全，逐形态过 as。
- ARM64 的 [sp,#临时区] 模式照抄前须审栈布局（与最深变量槽理论重叠）。
- 32 位类型的 64 位运算「看似可行」暗藏位模式坑（比较/除法双双实证）——宽度语义跟随源类型。

## 门禁状态（第十六轮最终，深度系统 x86_64）
构建零警告 + 单测 1238/1238 + E2E linux-x86_64 全量 154 用例 127 过/0 败/27 跳过 + -O0 抽查绿。
