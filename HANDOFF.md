# HANDOFF 交接文档

> **分机分区（2026-09-12 起，AGENTS.md §8.3）**：本文件按机器分节，每机**只整体替换自己节**，
> 他机节不动（两机并行改动落在不同区域，git 三方合并自动成功）。
> 新会话必读顺序：⓪根目录 `plans/021-任务进度观察表.md`（**全项目任务进度唯一总入口**：总目标/
> 当前进度/排班队列/里程碑——开工先读、收尾必更，见 AGENTS.md §2.2/§6.3）→ ①本机节（自己机器
> 最近交接）→ ②深度机节（开发主线最新进展）→ ③根目录 `三机任务看板.md`（三机并行唯一看板，
> 开工前先认领/查看）。

## 家机 win-x64 节

**最近交接**：2026-09-14——**第一百六十九轮（169-a）：D1 函数级拆分第四波**（`visitCallExpr` 658 → 239 行，9 方法提取；
纯重构，产物对拍 21 样本一致 17/不一致 0）。本会话累计八轮：161-a（A8）+ 162-a（A5+A3）+ 164-a（A4 联合体）+ 165-a（C16）+
166-a（`visitAssignmentExpr` 1034→25 子方法）+ 167-a（`visitCallExpr` 3 族）+ 168-a（同函数 2 子族）+ 169-a（同函数 9 方法）。
**下一步=跨机轮（深度机/单位机 161~169-a 契约面）→ D1 续波（`visitCallExpr` 族A struct 化 137 行收尾 → v2 `生成语句.cn` 635
→ `handleClassCallExpr` 362）→ 波 3 剩余（149-a IR diff）**。

### 一、169-a 做了什么

1. **拆分面**：`visitCallExpr` 剩余 3 大块 → **9 个方法**：族B 类构造（`resolveGenericCtorName`/`checkCtorCall`）+ 族C 成员方法
   （`checkInterfaceMethodCall`/`checkMemberCallCore`〔三输出引用参数〕/`checkInstanceMethodCall`/`checkStaticMethodCall`）+
   族D 直接调用（`checkVariadicBuiltinCall`/`checkDirectCallFallback`/`checkDirectCall`）；658 → 239 行；`semantic.hpp` +9 声明。
2. **剩余**：仅族A 模块限定调用块（137 行）——含 12 个跨段共享局部变量，需 **struct 化**（QualifiedCallInfo）才能拆；留下轮。
3. **★三次构建失败 → 三处结构性修正（教训）**：①输出形参名与段内局部声明**重定义**（须改赋值）②被单行早退替换的
   `if (...) {` 其**闭合括号**必须一并去除（否则结构崩坏 → C2059/C2653 连锁）③未使用形参触发 C4100（/WX）→ 去参数并同步
   三处（定义/声明/调用）。**通用**：脚本断言的「行多重集零缺失」保证不丢代码，但**结构正确性须靠零警告构建兜底**。
4. **等价性**：行多重集缺失 16 行全为预期改写；产物对拍（基线=166-a 编译器，覆盖三轮）21 样本 一致 17/不一致 0/跳过 4。

### 二、门禁（169-a 实测）

零警告构建 + 单测 **1317/1317** + 全量 E2E **306 用例 306 过 / 0 败 / 0 跳过** + 锚定链 **fix_p ≡ fix_s 逐字节自洽** +
组件对拍 **44/44** + 运行级 **3/3** + 等价性对拍 21 样本（一致 17/不一致 0/跳过 4）+ 行多重集（16 项全预期改写）。

### 三、下一步（新会话按序）

1. **跨机轮（首要）**：深度机/单位机复跑 161~169-a 契约面（关键字 46 词 / A4 用例 275~278 / 78_v2·79_v2 linux 首验 /
   规范文本核对）；四轮宿主改动均已经产物对拍证明对 v2 产物零影响。
2. **D1 续波**：`visitCallExpr` 族A struct 化（137 行 → `QualifiedCallInfo` + 收集/重写两方法，**使该函数 ≤100 行=D1 计数 77→76**）
   → v2 `生成语句.cn` 生成赋值语句 635（须双编译对照 + 锚定链重锚）→ `handleClassCallExpr` 362 →
   `semantic_expr_op.cpp` visitAssignmentExpr 349（清单 `python scripts/check_fn_length.py`）。
3. **波 3 剩余（C3）**：149-a IR diff → 泛化 → 元素级深拷 → H7/H11 收口。
4. 其余：D6 / C2 / 波 4 / 波 7。

### 四、验证链（同前三波；本轮新增「结构正确性靠构建兜底」）

```
> 全量门禁：build.ps1 → cn_unit_tests.exe → python tests/e2e/run_e2e.py --cn target/Debug/cn.exe
> 等价性对拍：python scripts/refactor_parity.py <旧cn.exe> <新cn.exe>（串行）
> 逐族提取循环：脚本枚举块 → 机械搬运 → 行多重集（**预期改写须逐项说明**）→ 双断言 → **零警告构建（结构正确性唯一兜底）**
>   → 产物对拍 → 组件对拍/运行级（与其它 v2asm.asm 使用者互斥串行）
> 大块拆分的三坑（169-a 实证）：输出形参 vs 段内声明重定义 / 被替换 if 的闭合括号 / 未使用形参（C4100）
> D1 度量：python scripts/check_fn_length.py
```

### 五、诚实边界

- `visitCallExpr` 仍 239 行（>100，仅余族A 137 行待 struct 化）；D1 计数 77（累计最大者 1034→885→757→658→**239**）。
- v2 树 50 个超百行函数未动（须双编译对照 + 锚定链重锚）；跨机轮未做（家机无 linux 工具链）。
- 本会话四轮（166~169-a）提交均已双推（gitcode + github）。

## 深度机 linux-x86_64 节

**交接时间**: 2026-09-15 第一百七十轮（**深度机 linux-x86_64**）——**170-a D1 函数级拆分第五波：`visitCallExpr` 族A struct 化收尾**（用户特批接手家机认领后关机中断的任务；基线 a1dda26）。本会话 1 个开发轮。

### 一、本轮做了什么（写给无上下文的新会话）

1. **拆分面（纯重构零行为变更）**：`semantic_call.cpp` 的 `visitCallExpr` 剩余族A 模块限定调用块（原 132~268，137 行，
   12 个跨段共享变量）→ **`QualifiedCallInfo`**（`semantic.hpp` 类内嵌套 struct，11 跨段共享字段：pathPrefix/moduleName/
   funcName/qualified/qualifiedDot/subModule + isTypeName/moduleLoaded/userFuncExists/builtinQualified/qualifiedClass）
   + 两方法：`collectQualifiedCallInfo`（展平嵌套 MemberExpr 路径 + 重写销毁旧 MemberExpr 前值拷贝全部名称 + 算齐判定
   布尔；返回「首段是否标识符」早退反转）与 `rewriteQualifiedCall`（五分支重写/未加载诊断，只消费 info）；原 116~131
   语义注释随方法搬移。**`visitCallExpr` 239 → 96 行（≤100 达标）**。
2. **★169-a 残留缺陷根治（跨平台构建缺陷·GCC 侧）**：`checkDirectCall` 残留 `auto builtinIt = functions_.find(calleeName);`
   死行（169-a 族D 提取边界残留；MSVC 不报 unused-but-set → 家机门禁未拦截；**169-a 提交在 GCC `-Werror` 下不可构建**）
   ——按缺陷零容忍纪律三方案呈报（删残留/假消费/回退拆分），用户未答复按默认纪律自主裁决**方案甲删残留**（纯死代码；
   子方法内同名变量有完整消费=功能零丢失，git 对比 169-a^ 原版确证）；教训入 lessons 170 段（预防 154：行多重集
   「重复」盲区 + MSVC/GCC 警告面互补）。
3. **等价性三重证据**：①行多重集缺失 52 项全预期改写（struct 字段赋值/const 化/早退反转/fullPath+qualified 冗余中转
   消除/折行）；②新旧编译器全量产物对拍 **63 样本（一致 59/不一致 0/跳过 4=负测 rc 一致）**；③**v2 全树 asm（15.3MB）
   md5 逐字节一致**（`124bc194…`）。基线=a1dda26 + builtinIt 死行删除（唯一内容差异，已论证零行为影响）。

### 二、本轮验证（linux-x86_64 口径）

- GCC 零警告构建 + 单测 **1317/1317**
- 全量 E2E **306 用例 304 过 / 0 失败 / 2 跳**（跳=62_ffi/69_memory_management 平台守卫）
- **78_v2/79_v2 linux 侧首次动态验证双 PASS**（163-a 家机通告的首验点闭环；绑定自检含于用例）
- 锚定链 fix_p ≡ fix_s **394251 行逐字节一致**（md5 `2760ce6d…`；**linux-x86_64 口径新锚**；产物在
  `target/audit2/selfwork79/fix_{p,s}.asm`；不跨后端比较——win 410416 / arm64 555442 各自口径）
- D1 计数 **77→76**（`python scripts/check_fn_length.py`；新首列=`CN语言编译器v2/IR/生成语句.cn` 生成赋值语句 635）

### 三、下一轮任务（按序）

1. **D1 续波（长期滚动，76 个）**：首列=v2 侧 `生成语句.cn` 生成赋值语句 635（**须双编译对照 + 锚定链重锚**）→
   宿主 `handleClassCallExpr` 362 → `semantic_expr_op.cpp` visitAssignmentExpr 349 → v2 `生成返回语句` 335。
2. **C3 波 3 剩余**：149-a IR diff → 泛化 → 元素级深拷〔前置=波 4〕→ H7/H11 收口。
3. **联合体条件释放设施专项（登记）**：结果/可选 全释放面（108-a 定位路径）。
4. 其余登记：D6 / C2（维持排程）/ 波 4 `复制(x)` / 波 7 NLL / D3 / D4。
5. **跨机轮**：arm64 侧 161~170-a 契约面复验（单位机；linux-x86_64 已由本轮 170-a 覆盖 E2E 全量+锚定链）。

### 四、验证链（本机复现口径）

```
> 验证链命令一律串行；**对拍/锚定链必须独立串行且不得相互交错**（102-a 假 DIFF 教训）。
> **全量门禁**：`rm -rf target/build && cmake -S . -B target/build -DCMAKE_BUILD_TYPE=Debug &&
> cmake --build target/build -j $(nproc)`（查 warning 计数=0）→ `./target/cn_unit_tests` →
> `python3 tests/e2e/run_e2e.py --cn target/cn --jobs 8`（306 用例：304 过/0 败/2 跳；含 78_v2/79_v2
> linux 真跑=锚定链自洽 + 组件对拍 + 运行级）。
> **等价性对拍（纯重构轮）**：`python3 scripts/refactor_parity.py target/cn_p170base target/cn_p170new`
> ——**对拍二进制必须放 target/ 下**（stdlib 兜底=从可执行文件目录上溯 ≤3 层；/tmp 下探测失败→假 SKIP，
> lessons 预防 156）；基线构建须在**编辑冻结**状态（lessons 预防 155）。
> **D1 度量**：`python scripts/check_fn_length.py`。
> **手工 v2p 重建（改 v2 源码后）**：`target/cn build CN语言编译器v2/主.cn --target linux-x86_64
> --output target/audit2/v2p_linuxx64`（E2E runner 指纹变更自动重建；入口必须绝对路径 + stdlib 兜底靠
> target/cn 部署位置）。
> **远程推送现状（2026-09-15 实测）**：本机已补配 `github` remote（CN_Language_C.git）；本轮 github
> 直连可达性待验证（推送失败属正常，下轮补推）。
```

### 五、诚实边界

- **基线差异声明**：本轮等价性基线 = a1dda26（=169-a 产物）+ builtinIt 死行删除——该删除已论证零行为影响
  （未消费的只读查找；GCC 构建失败下不可能有「169-a linux 基线编译器」，此为唯一可行基线）。
- 169-a 家机产物对拍为 MSVC 口径；本轮 GCC 口径下 builtinIt 已修，两平台警告面互补的残余风险=低
  （行多重集「重复」检查已在本轮人工核对中覆盖；脚本化待 D1 续波顺手固化）。
- v2 树 50→49 个超百行函数未动（76 个 D1 残余中 v2 侧约 20 个；**改 v2 须双编译对照 + 锚定链重锚**）。
- 探针/对拍临时件：`/tmp/p170base/`、`/tmp/p170new/`、`target/parity_cmp/`（约 400MB，可删）——不入库。



## 单位机 ARM64 节

**最近交接**：2026-09-14——**160-a 文档轮（基线 ebf99ab；源码零改动）**：三项用户批准落盘
（A5 扩围 +4 词 / A8 冒号后置移除 / C16 追加 spec 03 细化）。**实施均由家机认领执行**，
本节即家机实施入口；上轮 159-a（plans/023 漂移修正恢复）见 git ebf99ab。

### 一、本轮做了什么（写给无上下文的新会话）

1. **A5 扩围 +4 词（userselect 批准）**：`公开`/`私有`/`静态`/`常量` 随 A5 同轮上下文化
   （6 词 → 10 词；目标态保留字 **50 → 46**）。落盘四处：`plans/024` §10.5（批准批注 +
   `常量` 判据勘误=**前瞻 1 token**：后随标识符/`:` → 声明、后随运算符 → 表达式；声明位清单
   =顶层＋局部＋for-init；宿主局部常量=真实 isConst 语义 vs v2 防御分支按 整64——实施轮须
   盘点对齐）+ `plans/001` §2.1（总数/子类目〔声明 6→3、常量 4→3〕/上下文清单/变更记录）+
   `plans/021` §3-A A5 行扩围。
2. **新排班 §3-A A8（userselect 批准方案A）**：**冒号后置类型标注移除**。缺陷定性：宿主 3 处
   接受 `名: 类型`（参数 parser.cpp:361 / 变量·常量声明通道 parser.cpp:598-599 / 顶层常量
   parser.cpp:830-833；实测 `函数 加倍(x: 整32)`、`变量 局部: 整32 = …`、`常量 MAX: 整32 = 100`
   全通过），而 spec 全集与 plans/001 均无此语法且 spec 03 明确否决；v2 语法层零支持（双侧分叉）、
   仓库存量零使用（移除零迁移）。实施=移除 3 处 Colon 分支 + 负测锚定（冒号形态必须报错，宿主/v2
   双侧）+ 全链门禁（锚定链预期不动：负测不进既有产物面）。
3. **C16 追加（userselect 批准）**：spec 03 循环 EBNF 细化（init 位声明三形态：`变量`/`常量`/
   类型前缀）随 C16 规范落盘轮一并补写。附实测锚：循环 init 位 `常量` 语义自洽——
   `循环(常量 i = 0; i < 3; i += 1)` 报「不能给常量 'i' 赋值」（更新位拦截在位）。

### 二、家机实施入口（按序）

1. **A8 先行**（冒号移除，独立轮：看板认领 → 移除 3 处 → 双侧负测 → 全链门禁 → 六件套）；
2. **A5 随后**（10 词上下文化；判据不建在待移除形态上；契约广播 §8.4——`89_关键字穷举`
   用例串变更，三机复跑）；
3. **C16**（前置=波 6 终验：plans/023 §九 → plans/001 §5.3 + spec 同步 + 循环 EBNF 细化 +
   契约广播）；
4. 排班余项：A3（关键字清理实施）/A4（联合体收口）/C3 波次残余。

### 三、诚实边界

- 本轮纯文档零源码改动，无门禁面；探针实测（常量三形态/冒号形态/循环常量语义）均以
  `./target/cn check` 临时文件验证后清理，未入库；
- plans/001 §2.1 子类目计数（OOP5 含 `实现` 待移除的历史口径）未在本轮重排——A3/A5 实施轮
  落地时一并清算；
- github 镜像本机无凭据未推（沿 158-a/159-a 口径，下次提交补推）。
