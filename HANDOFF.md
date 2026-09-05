# HANDOFF 交接文档

**交接时间**: 2026-09-05 第十七轮（家机 win-x64）——**plans/016 呈报复核轮：win 侧无形态A错位（四重实证）+ win 第4路 calleeReturnsStruct 死代码归真（方案A 用户裁决「立刻修复」）**。

## 本轮已完成（全部有实证）

### 一、拉取 f9fe211（深度系统机 plans/016 linux-x86_64 后端专项）+ 本机全量门禁
- 对方机器无 MSVC，win 侧声称「纯增量零变化」未实测——本机补测：构建零警告（MSVC /W4 /WX）+ 单测 1238/1238（=原 1223+15，与对方 GCC 端一致）+ E2E 全量 154 用例 152 过/2 败（78/79 已知 OOM 家族零回归；新增 152_linuxx64 用例 win 侧 PASS）——**win 侧零回归实证完成**。

### 二、呈报复核结论：win 侧不存在形态 A 实参错位（四重实证，呈报锚定完成）
- 呈报原文担忧「若 win 侧存在形态 A 错位（argOffset=1 推 retbuf 实参到 rdx），24 错误处理用例应家机复测锚定」。实测：**不存在错位**。
- 四重实证：①探针（结果+结构体双形态带参直调）IR dump `调用 %v25 3 4 (void)`=形态A契约实锤（retbuf 预插 operands[0]、result.type=void）；②调用方 asm `mov rcx, r12; mov rdx, 3; mov r8, 4`=operands[0] 原样落 rcx（Win ABI 隐藏指针位）、实参无后移；③被调方 asm structReturn=true 协议（prologue 收 rcx 入 retbuf 槽/参数从 edx 起/epilogue 按字节拷回 retbuf）；④运行输出 7/30/40 EXIT=0。
- E2E 全量含 24_error_handling 等结构体返回密集用例全绿。

### 三、复核连带揪出新缺陷（方案A 当轮根治）：win 第4路 `calleeReturnsStruct` 从未生效的死代码
- **缺陷**：`activeModule_` 成员（x64_codegen.hpp）**全仓无赋值恒 nullptr**；`calleeReturnsStruct` 首行短路**恒返 false**——x64_instructions.cpp 2026-08「自举检查修复」注释声称的「按被调函数 structReturn 标志判定」**从未运行（假修复）**。当年 E2E 转绿真因=IR 层形态A预插（ir_call.cpp hiddenArgs 首插 retbuf）——「假修复」与「真机制」同期上线，双巧合互洽掩盖死代码。
- **危险反事实**：若按注释激活第4路（补赋值），形态A调用 hasBigRet 变 true → argOffset=1 → 预插 retbuf 被当普通实参推到 rdx → 实参全错位。
- **linux_x64 同族死代码**：新后端按「对齐 win x64」移植同款函数（这次有赋值=活的）但**零调用点**——复制叙事未核实被移植侧行为。
- **根治（行为零变化）**：①win 删第4路调用+函数+`activeModule_` 成员，hasBigRet 收敛三路（i128/u128/struct* result.type）——与 linux-x86_64/arm64 判定完全同构（x64_codegen.hpp/x64_instructions.cpp）；②linux_x64 删同族死函数（linux_x64_codegen.cpp 原 617-625）与 generateAssembly 赋值行（原 1050）+ hpp 声明与成员；③ir_decl.cpp/linux_x64_instructions.cpp 注释归真（形态A契约如实描述：IR 层预插 operands[0]、后端原样传递自然落隐藏指针位 win=rcx/SysV=rdi/arm64=x0；structReturn 标志仅供被调方 paramOffset 协议；勿复辙警示）。

### 四、文档同步
- lessons 高权重条目（死代码+假修复：回滚测试是「修复生效」唯一可信验收/移植复制行为不是注释叙事/死代码当缺陷对待——权重 9.5）。
- plans/016 第八节复核闭环；plans/014 状态表补第十六轮行+本轮第十七轮行；更新日志覆盖写入；本 HANDOFF。

## 门禁状态（第十七轮最终，家机 win-x64）
- 构建零警告（MSVC /W4 /WX）+ 单测 **1238/1238** + E2E 全量 **154 用例 152 过/2 败**（78=4105MB、79=4132MB 已知 OOM 家族与基线一致零回归）。
- 修复后复跑（build.ps1 -Test）同门禁通过——行为零变化实证（死代码删除无任何产物漂移）。

## 下一步
1. **v2 自举编译器的 linux-x86_64 代码生成**（代码生成X64Linux.cn）——v2 闭环 23 用例在 linux-x86_64 解锁，v2 三后端对齐（HANDOFF 第十六轮下一步①不变；需专项立项呈报待批）。
2. **v2@16 冒烟推进**：代码生成.cn/代码生成共用.cn/代码生成ARM64.cn/主.cn（第十五轮六链全绿后 v2 自举最后四组件；「v2 编译 v2 自身」对拍是终点）。
3. **linux-arm64 复验**（单位机）：152 用例 + 本轮 win 侧变更（纯删除性，双平台同码）在 ARM64 跑一轮。
4. B1a 契约分叉收口、v2 组件拆分（既有裁决不变）。

## 灰色点披露
- 78/79 v1 链内存失控——已知遗留勿翻案。
- 虚调用 emitVirtualCall 无 retbuf 处理——虚方法返回结构体场景无既有用例覆盖（本拉取前即如此，非本轮引入）；若未来虚方法返回结构体立项时须同轮设计 retbuf 协议。
- 递归同函数多次大返回调用共享同一 16 字节 retbuf 帧区——与 linux-x86_64 同款已知限制（顺序 IR 下安全）。

## 踩过的坑（本轮，全入 lessons）
- **「修复生效」的验收是可观测行为变化或直接实证**——注释声称的修复若被同期上线的另一机制掩盖（本例第4路死代码 vs 形态A预插），测试转绿不等于修复起效；回滚测试（删掉看是否回归）是唯一可信验收。
- **「对齐/移植」复制行为不是注释叙事**——移植前核实目标在被移植侧是否真的活着（有赋值/有调用点）。
- **死代码当缺陷对待**——恒短路分支/恒 null 成员与「从不触发的检查」同族，本例误导跨机跨后端两轮。
