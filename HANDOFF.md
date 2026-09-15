# HANDOFF 交接文档

> **分机分区（2026-09-12 起，AGENTS.md §8.3）**：本文件按机器分节，每机**只整体替换自己节**，
> 他机节不动（两机并行改动落在不同区域，git 三方合并自动成功）。
> 新会话必读顺序：⓪根目录 `plans/021-任务进度观察表.md`（**全项目任务进度唯一总入口**：总目标/
> 当前进度/排班队列/里程碑——开工先读、收尾必更，见 AGENTS.md §2.2/§6.3）→ ①本机节（自己机器
> 最近交接）→ ②深度机节（开发主线最新进展）→ ③根目录 `三机任务看板.md`（三机并行唯一看板，
> 开工前先认领/查看）。

## 家机 win-x64 节

**最近交接**：2026-09-15——**第二百二十九轮 229-a：F1-30-ASan 三项之①（构建接入 + 单测面检出）**（基线 d423a07）。

### 一、本轮做了什么（写给无上下文的新会话）

1. **CMake ASan 可选构建 ✅**：`CMakeLists.txt` 新增 `CN_ENABLE_ASAN`（默认 OFF）——MSVC `/fsanitize=address`
   （显式移除 `/RTC1` 冲突）／GCC 同款；**位置必须在所有 target 定义之前**（否则 gtest 注解不匹配 LNK2038）。
   用法：`cmake -S . -B target/build-asan -DCN_ENABLE_ASAN=ON && cmake --build target/build-asan --config Debug`。
2. **ASan 版单测 ✅**：**1327/1327 全过·无越界/UAF 报告**（唯一输出=预期超大分配告警；运行加
   `ASAN_OPTIONS=allocator_may_return_null=1`〔`AllocFailureReturnsNull` 测试语义〕）。
3. **默认构建零变化 ✅**：锚定链逐字节不变（426813 行·`f67e8f4f48f5a3914769`）。
4. **用法纪律（重要）**：ASan 与普通构建**共享 `target/Debug` 输出** → ASan 构建会覆盖普通产物
   （实测混用致 `LNK1319` 237 项不匹配）→ **ASan 验证后必须重建主构建**（`rm -f target/Debug/*.{exe,lib,exp}` + `build.ps1`）。

### 二、验证链

ASan 构建（零警告）+ ASan 单测（1327/1327·无内存错误报告）+ 主构建重建（零警告 + 1327/1327）+ 锚定链逐字节不变。

### 三、下一步（新会话按序）

1. **F1-30 余项**：ASan 版 E2E 跑一轮（命令已在上方）；DWARF 完整 / 增量编译（各自出轮）。
2. **F1-26 mem2reg**（单位机 226-a 调研建议方向；**开工前核对谁在做**——避免撞车）。
3. **A7**（深度机 227-a 在飞·避让）；**D9 ③ i128 折叠**（挂 F1-26）；**C3 波 4 206-d/e**。
4. D1 续波 / C2 / 波 7 NLL / 波 8+ / F2-33/34/35/36b/37。

### 四、诚实边界

- **ASan 版 E2E 未跑**（检出面=单测 1327 用例）；**MSVC ASan 无 LeakSanitizer**（泄漏检出面零·Linux/valgrind 补）。
- 两构建共享输出目录（**用后须重建主构建**）——本轮已实测恢复流程。

## 深度机 linux-x86_64 节

**交接时间**: 2026-09-15 第两百三十轮（**深度机 linux-x86_64**）——**230-a：文档指针化轮**
（用户裁决 A 方案；基线 eeebb02，认领 9e69221）。上一实质轮 225-a（A7 部分收口）见 git 60c1d13。

### 一、本轮做了什么

1. **看板**：三行收窄（≤300 字符）+删 4 段过期通告（188-a/187-a/163-a/144-a）；95→26 行。
2. **plans/021**：戳 8→2 行；§二标题名实相符+门禁大表指针化；§五 156 行收紧为四列一句话表；
   §三/§四 20 行超宽台账行指针化；§六补导航。493→479 行、单行全 ≤400 字符。
3. **门禁**：check_handoff.py 新增 plans/021 戳块 ≤5 行/单行 ≤400 字符、看板行宽 ≤300；
   反例（HEAD 版）exit=1 全数拦截、正例 exit=0。
4. **AGENTS**：§8.2 看板行格式、§8.3 plans/021 指针化格式（标注裁决来源）。

### 二、验证

check_handoff.py（扩展后）反例拦截/正例通过；check_progress_sync.py ✓；源码零改动。

### 三、下一轮任务（按序）

1. **227-a 已被家机占用（F1-29）；本机下一轮=231-a 候选**：A7 本体根治收口（导入泛型体
   `复制(下标链)` resolvedType 推导/分派④守卫→stdlib 六处切换重新应用→285→锚定链→valgrind 反证；
   全套定位数据在 plans/020 第七十四节 §三/§四+记忆 cn-a7-copystuct-family）。
2. 「读取文件行」uninit 独立案（回退态 v2p valgrind 剩余 6 errors——根治验收=归零）。
3. D1 续波等队列照旧（plans/021 §三）。

### 四、验证链（本机复现口径）

```
> 全量门禁：rm -rf target/build && cmake -S . -B target/build -DCMAKE_BUILD_TYPE=Debug &&
> cmake --build target/build -j $(nproc) → ./target/cn_unit_tests →
> python3 tests/e2e/run_e2e.py --cn target/cn --jobs 8（313 用例：311 过/0 败/2 跳）。
> 共享文档自检：python3 scripts/check_handoff.py（四文件含行宽/戳块上限）＋
> python3 scripts/check_progress_sync.py——提交前必跑。
> stdlib 态变更后必 rm target/audit2/v2p_*（指纹盲区）；valgrind 已装（apt）。
```

### 五、诚实边界

- §五 索引保留 156 轮一句话行（表体 ~160 行）——未砍除：按轮号快速定位有导航价值；行宽已锁死。
- §三/§四 部分台账行的压缩以「指针」替代原文细节——细节权威源=plans/019/020 各节与 git 提交信息，
  若发现某指针失准请以 git log --grep=轮次号 为准修正。
- github 镜像仍缺仓库（待用户建仓）。

## 单位机 ARM64 节

**最近交接**：2026-09-15——**第两百二十六轮（226-a）：D8 寄存器感知发射尝试 → 按纪律回退（源码零净变更）+ F1-26 调研**
（基线 8bfcc67）。上轮 222-a（F1-28 arm64 寄存器分配接线落地 + D7 归因修正，提交 8bfcc67）见下「一」。

### 一、本会话已交付（222-a，可靠状态=当前 HEAD）

**F1-28 arm64 寄存器分配接线落地 + D7 归因修正**（提交 `8bfcc67`，已推送 gitcode）：
- **D7 撤销**：区间覆盖校验器在 v2 全树 + 全量 E2E 语料实测 **0 违例** → 原归因「活跃区间过短」不成立；
  真因＝196-a 接线实现的消费面遗漏（值流分叉）→ `reg_alloc.{cpp,hpp}` **零改动**，不变量固化为单测。
- **接线**：结果落位唯一通道 `storeVirtualResult{,Fp}`（57 处）+ 读侧统一 `loadOperandToX/V` + `regSlotOffset`/
  `regSlotMem` 对已分配寄存器返回 0（弃用槽旁路退化为无害空操作）+ 序言/尾声被调用者保存对（三处返回路径全覆盖）
  + `stackParamBase()` 补偿 + 返回位直传物理寄存器 + `backend_factory` arm64 `-O2` 联动。
- **门禁**：零警告 + 单测 1327/1327 + 全量 E2E **312 用例 310 过/0 失败/2 跳** + **锚定链重锚 561364→558209 行**
  （`534c3dd8…`·fix_p≡fix_s）+ **性能锚 栈访存 1077 vs 2379（-54.7%）** + 反证（`--no-regalloc` 两态输出逐字节一致）。

### 二、本轮（226-a）做了什么（尝试轮·源码零净变更）

1. **D8 尝试**：6 处发射路径（常量装载 4 类 / 整数二元运算 / Load / LoadPtr / AddrOf / FieldAddr）改为
   「结果与操作数直接以分配寄存器为目标」，消除「算到固定临时寄存器（x9/x10）再搬运」。
2. **收益实测（生效时，245 用例口径）**：**行数 21204→19996（-5.7%）**、**冗余 `mov xN,临时` 1121→699（-37.6%）**。
3. **★全量 E2E → 7 例失败**：`78_v2`/`79_v2`（v2p 二进制 `malloc` 堆断言＝堆损坏）+ `88`/`94`/`97`/`99`/`89`（段错误 -11）
   ——**这些用例单独跑全部 rc=0**＝典型 **UB 非确定性**。
4. **处置**：`git checkout` 回退（**源码零净变更**）→ 重建零警告 → 单测 **1327/1327** + 全部失败用例 rc=0 复核。

### 三、下一轮任务（按序，均已在 plans/025 §三.2b 落盘）

1. **F1-26 mem2reg（推荐·性能第一）**：`ssa.cpp` 的 Phi 现为**纯装饰**（复用 Load 结果 id、不删 Load、不改引用，
   codegen 输出预留注释），实测 `-O2≡-O3` 一致＝对值流零影响。**规格 §7.1/§7.4 要求 IR 本身为 SSA**（实现与规格偏离）→
   方案：① 地址逃逸分析（未被 `取地址`/引用实参/成员链逃逸的 Alloca 槽）② 迭代支配边界插 Phi
   ③ 变量重命名（Load→到达定义值）+ 删 Load/Store ④ SSA destruction（Phi→前驱块尾并行复制，或 codegen 消费 Phi）。
   **独立成轮 + 分阶段门禁**（影响全部后端与优化 pass）。
2. **D8 重做（别名/宽度契约定位）**：**定位法已定**＝① 全量 E2E 锁定失败用例 → ② 同用例 `-O2`（启用）vs
   `--no-regalloc` 产物**逐指令语义对比** → ③ 必要时最小 IR 探针（`cn ir` dump + 手改）。**收益基线已留档**
   （-5.7% 行数 / -37.6% mov），已修两处陷阱（常量分支 dst 既源又目标 / 32 位路径 x9-vs-w9）记入 lessons 223。
3. **F2-33 纯 stdlib 三项**（随机数/迭代器/正则——零语言变更，可独立出轮）。
4. **linux-x86_64 后端 regalloc**（222-a 的 arm64 接线可复用；当前仍写死关闭）。
5. 其余：F1-29（pass 边界放开）/ F1-30-ASan / D1 续波 / D3 / F2-36b（arm64 性能基线）。
6. **文档缺陷清理**：`plans/021` §一~§五 存在历史重复段（第二份为旧副本）→ 持锁时整体去重。

### 四、验证链（本机复现口径）

```
# 全量门禁（arm64）
rm -rf target/build && cmake -S . -B target/build -DCMAKE_BUILD_TYPE=Debug &&
  cmake --build target/build -j 8            # 零警告
./target/cn_unit_tests                       # 1327/1327
python3 tests/e2e/run_e2e.py --cn target/cn --target linux-arm64 --jobs 4   # 312/310/0/2
# 锚定链（arm64）：79_v2 单跑（v2p 缓存命中后约 3 分钟）
python3 tests/e2e/run_e2e.py --cn target/cn --target linux-arm64 --filter 79_v2 --verbose
#   → target/audit2/selfwork79/fix_p.asm ≡ fix_s.asm（558209 行 / 534c3dd8…）
# 性能锚：cn compile <用例> --target linux-arm64 -O2 [--no-regalloc] → grep -c '\[x29, #-'
# ASCII 门禁（C++ 改动后必跑）：python3 scripts/check_ascii_idents.py
> 注意①：v2 锚定链用例首建 v2p 时勿用 --jobs 6（并行内存压力曾致 78_v2 偶发失败；--jobs 4 全绿）。
> 注意②：**改代码生成层（发射方法的寄存器契约）时，必须直接跑全量 E2E**——逐组抽样单跑会假绿（lessons 223）。
> 远程推送现状（2026-09-15 实测）：gitcode 正常；github 本机无凭据（沿既有口径待他机代推）。
```

### 五、诚实边界

- **226-a 未交付功能**（回退）：D8 的 UB 根因**未定位**（只定位到"全量才暴露"这一事实）；收益数据是"生效时"实测，
  下一轮重做时须重新验证。
- **D7 结论**：分配器本身无此缺陷（同语料 0 违例）→ **win-x64 无需为分配器区间做任何改动**；家机复跑全量 E2E 即可复核。
- **win-x64 同族动态核查本机做不到**（无 MSVC/无 x86 执行环境）。
- **linux-x86_64 后端仍写死关闭** regalloc。
- 探针/中间产物：`/tmp/d7work/`（含 `d8_full.cpp`＝D8 改造完整版，可复用）；`target/audit2/selfwork79/` 为锚定链正式产物。
- github 镜像本机无凭据未推（170-a 起累积）。
