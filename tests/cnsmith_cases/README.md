# CN-Smith 命中样本库（D32·258-a 立案）

258-a 轮 CN-Smith 500 采样（seed=258）击中的 **RA/寄存器分配面缺陷族**样本：

- `s266.cn`：**-O3 语义分歧**（变8 应=91513L〔经 变6 拷贝〕，-O3 实跑被写成 -37945〔=变10 的值〕
  → 变13 输出 -91471(O0) vs 37903(O3)）——**260-a 已根治**：真根因=**SSA Pass 占位 Phi 化**
  （非 RA）——汇合块 Load 的槽 Store 在其后顺序段时前驱注入 Load 读未初始化槽（UB），
  Phi 化前提收紧后消除（详见 260-a 提交信息+plans/021 §4.2 D32 销项）；转正 E2E 387/388。
- `s268.cn`：**-O3 汇编 A2022**（mov eax, r14——Cast src 物理寄存器 64 位名未收缩；
  258-a 已修 r8~r15 收缩，s268 若仍失败=另有漏点）——**260-a 复验通过**（构建+运行正常）。

复现：`cn build tests/cnsmith_cases/s26N.cn -O3`；O0/O3 输出 diff。
根因域=256-a 启用的寄存器分配（F1-28）+SSA/Phi 降级（256-a）在 E2E 语料未覆盖
组合下的宽度适配/槽混淆。**归 D32 排班根治**（Rust 参照=register allocation 完备测试+
llvm-习得 validation pass：RA 后对每 mov 校验操作数宽度一致性）。**260-a 已收官**：
根因反转（非 RA 而是 SSA 占位 Phi 化——见上）；配套防线=check_asm_width 三后端
（arm64/linux_x64/win-x64 宽度混配）+387/388 双级别锚定用例。
