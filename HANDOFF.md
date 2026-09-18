# HANDOFF 交接文档

> **分机分区（2026-09-12 起，AGENTS.md §8.3）**：本文件按机器分节，每机**只整体替换自己节**，
> 他机节不动（两机并行改动落在不同区域，git 三方合并自动成功）。
> 新会话必读顺序：⓪根目录 `plans/021-任务进度观察表.md`（**全项目任务进度唯一总入口**：总目标/
> 当前进度/排班队列/里程碑——开工先读、收尾必更，见 AGENTS.md §2.2/§6.3）→ ①本机节（自己机器
> 最近交接）→ ②深度机节（开发主线最新进展）→ ③根目录 `三机任务看板.md`（三机并行唯一看板，
> 开工前先认领/查看）。

## 家机 win-x64 节

### 一、本轮交付（🏃 324-c 开发轮：C24=T44/T27 修复补 linux_x64 后端·基线 0a9aef8·2026-09-18）

用户 2026-09-18 裁决「家机下一轮认领即」（324-a x64l 漂移呈报）。**四点同步**（win 316-a/319-a 蓝本映射 linux_x64）：

1. **槽补登记**（linux_x64_codegen.cpp）：Store.extra 存储目标名 + $s1 高半槽两段（win 316-a 第二轮映射·须在全部 Alloca 完成后执行）——根治 -O3 SSA 使用点重写汇合临时（__ternary$N）无 Alloca → varSlotOf 回 0 拼出裸 `[rbp]` 偏移 0（x64l 双级 SIGSEGV·m44_01 与 win rbp0 同源）。
2. **emitCopy i128 双半**（linux_x64_instructions.cpp）：判据扩源类型（copySrcType）+ 变量形态（id<0）双槽寻址（基名低半+$s1 高半）——原仅判 inst.type 且只用 id（id=-1 时 regSlotOffset(0) 落错槽）。
3. **emitDivMod 特判宽度对齐**：`cmp r9d, -1`（32 位除数经 mov r9d 零扩展——原恒 64 位 cmp 永不命中 → INT_MIN/-1 落 idiv 溢出 SIGFPE·m27_01/s2609179004 x64l -O0）。
4. **emitCast 窄→128 宽化 movsxd**：i32 源槽零扩展装载（mov r10d）符号丢失——-1 变 +4294967295+高半 sar 63 得 0（m27_02~05 O0 错值·m27_02 O0=(-2^63)/4294967295 数学反验证吻合）；i8/i16 经 emitStackLoad movsx 已 64 位符号扩展·无需补。

**验证（本机静态面）**：零警告构建+单测 1353/1353+check_asm_width 四维 PASS（271×3 后端）+win 全量 E2E 444=443/1（唯一败 406=T43 待用户裁决）+**x64l .s 四点指纹**（c24_probe2 双级别实测：i128 双半成对 mov 全在场/无裸 `[rbp]` 偏移 0/`cmp r9d, -1`/`movsxd r10, r10d`）。

### 二、下一轮（按序）

1. **运行级复验归深度机**（C24 验收原文）：p314a t44_ternary/nest/u128+p314b x01~x04 矩阵全绿+回归轨归零。
2. 波次7=v2 欠账族；T43/T46/T47/T48 待用户裁决。

### 三、坑与边界

- 本机无 x64l 运行能力——**静态指纹（.s）是本机唯一可验证面**；运行级归深度机（C24 明示）。
- 窄→128 的 i8/i16 经 emitStackLoad movsx 已 64 位符号扩展——**补前先读装载路径再定补点**（避免双扩展/漏扩展）。
- emitCopy 混合形态（寄存器↔变量）防御兜底（IR 层 Load/Store 已拆双半·该形态不可达）。
- 探针设计教训：对比形态探针必须用**运行时值**（输入 API）——字面量会被常量折叠致「零命中」假象（首版 c24_probe 全折叠）。

## 深度机 linux-x86_64 节

**交接时间**: 2026-09-18 🧪 **324-a M3 采样验证轮收工**（1000 差分+400 负向+回归轨对账+T44/T45/T46 x64l 运行级复验）。基线 0eaff2b4（target/cn 07:21 重建·src 零改动）。daemon 收工前已停、push 后按 313-b 新版重启。

### 一、本轮做了什么（324-a）

1. **采样双轨全绿**：正向 1000 新样本（seed=1789687508）O0/O3 差分 2000/2000 全一致零命中；负向 400（seed=1789687530）400/400 拒绝·零崩溃·诊断含行号（连续第七轮）。
2. **回归轨全库对账 129 件**（带类别前缀防跨类重名覆盖）：一致 120·9 命中全旧类零新类别——**★两处漂移（修复未兑现·x64l 运行级口径）**：
   - **T44 漂移**：i128 三元族 x64l 仍 O0/O3 双级 SIGSEGV（实弹 m44_01 未消+扩面 7 形态：p314a t44_ternary/nest/u128·p314b x01~x04）——根因=316-a 修复仅落 `codegen/x64/`（win 专属：x64_codegen.cpp 槽补登记 + x64_instructions.cpp emitCopy i128 双半），linux_x64 零改动 → asm 仍 `mov qword ptr [rbp], r10`（高半槽偏移 0；gdb 崩溃点 cn_main+202 rbp=0 实证）；非三元 5 形态健康（if2/chain3/outside/arr/arg/ret）。「平台无关同修」预判失效。
   - **T27 漂移**：x64l -O0 两处未兑现——①整32 INT_MIN/-1 SIGFPE（m27_01/s2609179004）：x64l emitDivMod 特判 `cmp r9, -1`（64 位）vs 除数 `mov r9d` 装载（零扩展）→32 位负值永不等于 -1→特判永不命中→idiv 溢出陷阱（win 侧宽度感知 ecx/rcx 正确=家机验证口径）；②窄整负值→128 宽化零扩展（m27_02~05 O0 错值）：低半 `mov r10d` 装载未 movsxd→-1 变 +4294967295·高半 sar 63 得 0——数学反验证吻合（m27_02 O0=-2147483648=(-2^63)/4294967295）。
3. **T44/T45/T46 实弹对账**：**T45 全转正 ✓**（m45_01~03 值正确+p314a t45_* 9 形态全健康）；**T44 未消**（x64l 双级 SIGSEGV·见上）；**T46 未修复**（m46_01 仍读 0·O0=O3 一致=差分轨盲区·待用户裁决）。
4. 预期命中：m37_01/m42_01（320-a 波次6 在飞）。

### 二、验证链（当轮实测）

```
python3 scripts/cnsmith_gen.py --seed 1789687508 --count 1000 --out target/cnsmith_auto
python3 scripts/cnsmith_diff.py --dir target/cnsmith_auto --cn target/cn --jobs 8      # 2000/2000 全一致
python3 scripts/cnsmith_gen.py --seed 1789687530 --count 400 --negative --out target/cnsmith_negative
python3 /tmp/negative_check.py target/cnsmith_negative 8                                # 400/400 拒绝·0 崩溃
python3 scripts/cnsmith_diff.py --dir target/cnsmith_regress --cn target/cn --jobs 8   # 129 件·9 旧类命中
python3 /tmp/verify_hits.py target/p314a/*.cn target/p314b/*.cn                         # T44 三元族 7 挂·T45 全健康
gdb -batch -ex run -ex "x/3i $pc" --args <m27_01_O0>                                    # idiv %r9d SIGFPE 定位
```

### 三、下一轮任务（按序）

1. **T44/T27 修复补 linux_x64 后端**（家机修复轮·四点：emitCopy i128 双半/槽补登记/`cmp r9d,-1`/宽化 movsxd）；修复后本机复跑 p314a/p314b+回归轨销账；
2. daemon 常态化采样续跑（新种子域起·x64l 快）；
3. T46 待用户裁决；T43（win A2070）阻塞 win 全量转绿。

### 四、诚实边界

- 本机无 win/arm64 产物运行能力——win 侧口径只能由家机复核（本轮 x64l 漂移即「家机单侧验收」盲区的实证）；
- m27_02~05 的 O3 输出「域内数学正确」沿用 319-a 定性；O0 错值根因已锁定（窄→128 宽化零扩展），i128 helper 内部是否另有残余未在本轮展开；
- daemon 采样轮次在 07:23 后暂停至 push 完成（重启后自动续）。

## 单位机 ARM64 节

**交接时间**：2026-09-17 🏃 297-a 全修轮收工（用户裁「按照你的方式，全部修复」）——**T25/T24/B10 三案当日根治**+406/407/408 转正+全量门禁绿。接手前先 `git fetch`。

### 一、本轮做了什么

1. **T25 根治（浮64 汇合赋值 -O3 归零）**：病灶=arm64 `emitCopy` 按整型 is64 分派，f64 落 32 位分支传 w9 → `str w9` 32 位截断写（高 32 位丢）。修复=浮点分支 `loadOperandToV`+`storeVirtualResultFp`（f32→s0/f64→d0）；asm 实证 `str d0` 64 位完整写；27 形态回归矩阵全绿（三元 9+汇合 9+最小 2+浮32 3+数组 2）。
2. **T24 根治（后缀自增值语义）**：`visitUnaryExpr` Inc/Dec 三路径+`handleClassFieldIncDec(+postfix)` 按后缀返回**旧值**（原忽略 postfix 一律新值）；16 形态全绿（赋值/实参/比较/打印/成员/窄型/语句位不回归/前缀对照）。
3. **B10 根治（方案甲·同层 `:` 前瞻消歧）**：parser `ternaryColonAhead()` 替换 C-1 白名单判据（括号/嵌套 ? 深度感知；新判据⊇旧判据=存量成功程序零语义变化）——`a ? -b : c` 不再要求括号；后缀传播 `r?+1`=42 不回归。
4. **T26 同族判定**：arm64 短路四形态 O0/O2/O3 全对=免疫（x86_64 单侧·精确验证归深度机 B11）；x64/x64l emitCopy=mov 文本拼接结构（无同型截断）静态注记。
5. **规范条文**：plans/001 §4.5 优先级表（`++`/`--` 后缀 13/前缀 12 级+值语义）+三元消歧+错误传播节；spec 01b 五处同步。
6. **新 E2E 三件**：406_浮点汇合矩阵O3（编译选项.txt=-O3·13 行）/407_后缀自增矩阵（16 行）/408_三元负号矩阵（6 行）+coverage_map 4.4 登记。
7. **看板通告段广播**：v2 消歧同口径（家机 296-a v2 ? 链对齐）+T24 v2 对齐登记。

### 二、验证链（全绿）

```
cmake --build target/build -j 8          # 零警告（grep error|warning=0）
./target/cn_unit_tests                    # 1353/1353
python3 tests/e2e/run_e2e.py --target linux-arm64 --cn target/cn --jobs 8   # 426=424/0/2（跳=62_ffi/69 平台败面）
python3 tests/e2e/run_e2e.py --filter 60_error ...  # 传播回归 PASS
> 78_v2_自举链构建 PASS ＋ 79_v2_自举闭环 PASS（锚定链承载）
> 406/407/408 单跑 PASS；双门禁独立验退出码=0
```

### 三、发现的问题

- T26=x86_64 单侧（arm64 免疫实证）——精确形态归深度机 B11。
- T24/T25 的 **v2 树同型对齐**待家机 296-a 收工后独立轮（自举编译器也有这两缺陷·通告段已广播）。
- i1 Copy 走整数路径 32 位写=安全（布尔矩阵全绿实证·注记）。

### 四、下一轮候选

1. **M3 采样轮⑥**（用户令不停）：行 8/10/11 算术位运算比较面。
2. **T24/T25 v2 对齐轮**（家机 296-a 收工后）。
3. A12 标签锚点搭车。

### 五、诚实边界与坑

- win/x64l Copy 浮点路径运行级复验归各机（静态审查无同型截断·运行级未验）。
- 406~408 未配单测（E2E 为强门禁·单测面留后续）。
- 修复期间 E2E 全量 426=424/0/2 的 2 跳=62_ffi/69（既有平台败面·非本轮引入）。