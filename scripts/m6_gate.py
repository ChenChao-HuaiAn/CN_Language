#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""M6 自举固定点机械触发器（任务 #1221·2026-10-23）。

背景（#1221 覆盖对账结论）：M6=79_v2 自举闭环（fix_p ≡ fix_s 逐字节），此前只有
「云端全量每轮都跑、本地/CI 调用面靠自觉」：纯文档/前端轮白付 ~20 分钟重负载是
浪费面；反向更危险——写集动了编译器产物面（codegen/opt/ir/model/协议常量/stdlib）时，若本轮落在不能原生跑 M6 的平台
（交叉模拟 arm64 的 78/79 登记跳过）或调用面未开重负载（integrate 本地全量经
ci.ps1/run_e2e 默认跳过 78/79·GitHub Actions 工作流同样未带 --heavy-ci），79_v2
被计入「跳过」而轮次照绿=流程自觉而非机械保证。本模块把「动编译器产物面（codegen/opt/ir/model/协议常量/stdlib）⇒
本轮必须跑 M6」做成单点判定；执行面复用 run_e2e 79_v2（--require-m6 跳过即红 +
md5/首个差异字节审计记录）。

判定口径（结构性·非启发式）：写集路径前缀命中下列任一面即触发——
  宿主：src/cn_compiler/codegen/、src/cn_compiler/opt/、src/cn_compiler/ir/
  v2  ：CN语言编译器v2/代码生成/、CN语言编译器v2/IR/
纯前端（lexer/parser/semantic 不含 IR）、文档、脚本、tests 一律豁免（M6 是产物
逐字节面，前端改动由既有双编译/闭环用例承担；语义改动的 IR 产物变化经 ir/ 面命中）。

纯函数面（--selftest 正反例·零 git 零构建）：
  判定写集(文件们)             → {"触发", "命中", "说明"}
  平台可跑M6(平台, 交叉模拟)    → 本平台能否原生跑 M6（交叉模拟 arm64=否）
  判定轮(文件们, 平台, 交叉模拟) → 追加 {"平台可跑", "处置"}；处置∈未触发|必跑|未跑-转派
  误放行检查(点名清单)          → --allow-known-red/--allow-pool-red 点名含 79_v2=拒
  摘要(计划)                   → 摘要行文本（M6 未跑时必须显式原因）

纪律（miss 不静默）：写集不可判（git 异常/空清单歧义）由调用方按 fail-safe 触发
（多付一次 M6 成本优于漏验）；平台不可跑时 run_e2e 摘要与 ci_daemon 轮记录必须显式
「M6 未跑（原因）」，预验轮直接判红（fail-closed），develop 轮由平台轮转承担并在
结果里留「转派」原因。
"""
from __future__ import annotations

import hashlib
import subprocess
import sys

# M6 用例（tests/e2e/79_v2_自举闭环·run_e2e 特殊编排执行v2锚定链）
M6用例名 = "79_v2_自举闭环"
M6用例关键字 = "79_v2"

# 写集触发前缀（守卫=产物语义：代码生成/优化/IR 变化才可能改变 fix_p/fix_s 逐字节面）
# #1221 复审补：协议常量（v2 IR 节点/类型码契约）、stdlib（自举编译的源码级输入）、
#   model（AST/类型系统→IR/代码生成的直接上游）一并纳入——按 include/导入依赖的
#   超集口径（文件级闭包另立任务；超集只会多验不会漏验）。
M6写集前缀 = (
    "src/cn_compiler/codegen/",
    "src/cn_compiler/opt/",
    "src/cn_compiler/ir/",
    "src/cn_compiler/model/",
    "CN语言编译器v2/代码生成/",
    "CN语言编译器v2/IR/",
    "CN语言编译器v2/协议常量/",
    "stdlib/",
)

# 预验认领限制的原生 M6 平台（#1221 复审 P1-4④：真机 arm64 **可以**原生跑 M6，
# 但服务端平台字段无法区分「真机 arm64」与「交叉模拟 arm64」（同报 linux-arm64），
# 故预验派发缺省限 win-x64/linux-x86_64；arm64 覆盖由 develop 轮转的真机轮承担。
M6预验原生平台 = ("win-x64", "linux-x86_64")

# 可原生跑 M6 的平台（三平台后端均有 as/g++ 或 ml64/link 闭环编排）；arm64 需真机
M6原生平台 = ("win-x64", "linux-arm64", "linux-x86_64")


def _归一(路径: str) -> str:
    """写集路径归一：反斜杠→斜杠、去前导 ./——git 输出跨平台一致消费（586-a 同族：
    路径形态差异不得令前缀匹配静默失效）。只动前缀，不 strip 文件名自身字符。"""
    文本 = (路径 or "").replace("\\", "/")
    while 文本.startswith("./"):
        文本 = 文本[2:]
    return 文本


def 判定写集(文件们) -> dict:
    """纯函数：改动文件清单 → M6 触发判定（前缀精确匹配·含目录本身形态）。

    返回 {"触发": bool, "命中": [路径...], "说明": str}。空清单/None=不触发（调用方
    对「写集不可判」另行 fail-safe 触发，见模块头纪律段——本函数不把空当未知）。
    """
    命中 = set()
    for 原路径 in 文件们 or []:
        路径 = _归一(原路径)
        if not 路径:
            continue
        for 前缀 in M6写集前缀:
            if 路径 == 前缀.rstrip("/") or 路径.startswith(前缀):
                命中.add(路径)
                break
    命中们 = sorted(命中)
    if 命中们:
        说明 = ("写集命中 %d 文件（编译器产物面 codegen/opt/ir/model/协议常量/stdlib）"
                % len(命中们))
    else:
        说明 = "写集不含编译器产物面（codegen/opt/ir/model/协议常量/stdlib）——M6 豁免"
    return {"触发": bool(命中们), "命中": 命中们, "说明": 说明}


# 自举链写集触发面（#1796·2026-10-29 用户批 CI 提速方案3）：78_v2/79_v2 自举双用例
#   是全量 e2e 最重的两面（79≈24 分钟+78≈10 分钟·占 e2e 总时长约八成），其验证对象
#   是「编译器能编译编译器且产物自洽」——只随编译器链/运行时/stdlib/用例自身变化。
#   触发面取 M6写集前缀的**超集**（宿主 src/cn_compiler 全部+v2 全部+stdlib+runtime
#   +78/79 用例目录自身），M6 触发 ⇒ 自举触发为机械包含关系（--require-m6 与
#   --skip-heavy 永不并发）。纯文档/官网/脚本/工具/普通用例改动豁免——漏验风险由
#   每日 --force 全量兜底轮承担。
自举写集前缀 = (
    "src/cn_compiler/",
    "CN语言编译器v2/",
    "stdlib/",
    "runtime/",
    "tests/e2e/78_v2_自举链构建/",
    "tests/e2e/79_v2_自举闭环/",
)


def 判定自举写集(文件们) -> dict:
    """纯函数：改动文件清单 → 自举链（78/79）触发判定（#1796·同构 判定写集）。

    返回 {"触发": bool, "命中": [路径...], "说明": str}。空清单/None=不触发（调用方
    对「写集不可判」另行 fail-safe 触发——本函数不把空当未知）。"""
    命中 = set()
    for 原路径 in 文件们 or []:
        路径 = _归一(原路径)
        if not 路径:
            continue
        for 前缀 in 自举写集前缀:
            if 路径 == 前缀.rstrip("/") or 路径.startswith(前缀):
                命中.add(路径)
                break
    命中们 = sorted(命中)
    if 命中们:
        说明 = ("自举链写集命中 %d 文件（src/cn_compiler|CN语言编译器v2|stdlib|runtime"
                "|78/79 用例）——78/79 照跑" % len(命中们))
    else:
        说明 = ("自举链写集未触发（写集不含编译器链/运行时/stdlib/78/79 用例）"
                "——78/79 跳过（每日 --force 全量兜底·#1796）")
    return {"触发": bool(命中们), "命中": 命中们, "说明": 说明}


# ── M6 输入面摘要（#1599）────────────────────────────────────────────────
# 复用判据的**机械证据**：把「M6 的输入」压成一个可比较标识——面相同 ⇒ fix_p/fix_s
# 的输入逐字节相同 ⇒ 逐字节结论必然保持。这是「甲案」的安全形态（判据=内容相等，
# 不是「develop 增量碰了哪些文件」的推断）。
# 为什么不用 patch-id：git patch-id 会把空白差异归并（源码缩进/字符串内空格不入哈希），
# 用它判「补丁相同」会出现假阳性复用（空白差异确实能改变 v2 产物 → M6 假绿）。
# 为什么不用「增量 ∩ 写集 = ∅」：本仓实测 develop 增量 31/31 命中编译器面（#1599
# 量化），该判据现实命中率 0；面摘要相等是它的严格超集（同时覆盖 rebase 与文档轮）。
自举面路径 = (
    # 宿主编译器（fix_p/fix_s 的产出者）：源码+构建入口
    "CMakeLists.txt",
    "build.ps1",
    "src",
    # v2 编译器自身源码（自举输入）
    "CN语言编译器v2",
    # 标准库源码（自举编译输入）
    "stdlib",
    # M6 用例自身 + 编排/判定实现（改判定=改「通过」的含义，必须一并纳入）
    "tests/e2e/78_v2_自举链构建",
    "tests/e2e/79_v2_自举闭环",
    "tests/e2e/run_e2e.py",
    "scripts/ci_build.py",
    "scripts/ci_m6.py",
    "scripts/ci_daemon.py",
    "scripts/ci_round.py",
    "scripts/ci_shard.py",
    "scripts/m6_gate.py",
)

# 面摘要宽度（hex）：sha256 前 128 位。预验分支名 `-m6<面摘要>` 协议宽度——
# 旧实现的 time_ns 后缀恒 ≤16 hex，宽度不同故两形态可机械区分（见 摘要自分支名）。
面摘要宽度 = 32


def _提交可读(提交: str) -> bool:
    """提交对象在本仓库可解析（`<提交>^{commit}`）——面摘要的前置：先确认提交在，
    才能把「路径解析失败」读成「该面路径在此提交不存在」而非仓库/参数异常。"""
    return subprocess.run(["git", "cat-file", "-e", str(提交) + "^{commit}"],
                          capture_output=True).returncode == 0


def _路径对象(提交: str, 路径: str) -> str:
    """`git rev-parse <提交>:<路径>` → tree/blob 对象 id；路径在该提交不存在=空串。

    目录得到 tree id（递归覆盖全部子项的内容+路径+模式），文件得到 blob id——
    两者都是 git 的内容寻址标识，天然逐字节精确，不需要自行比对文件内容。"""
    取 = subprocess.run(["git", "rev-parse", "--verify", "--quiet",
                         "%s:%s" % (提交, 路径)],
                        capture_output=True, text=True, encoding="utf-8", errors="replace")
    return (取.stdout or "").strip() if 取.returncode == 0 else ""


def 面摘要(提交: str, 提交可查=None, 对象查=None) -> str | None:
    """M6 输入面摘要（hex·宽度 面摘要宽度）；不可判=None。

    面清单与面路径**一并**进哈希预像——增删面路径即自动改变全部摘要（无需人工
    「版本号 +1」这类可遗忘手续），杜绝「面的定义变了但旧摘要仍被复用」的漏验面。
    不可判返回 None 的有两类（都要重跑·miss 不静默）：
      ①提交对象不在/仓库异常；
      ②面路径**全数解析不出对象**（正常提交至少有 src 与 v2 源码；全空=喂了别的树/
        裸 SHA 无树，此时「两边都全空」会被误判成同一面——宁判不可判）。
    单条面路径缺失（旧提交尚无该路径）记哨兵「-」参与哈希：两侧同缺=同值，正确。
    提交可查/对象查=注入点（selftest 零 git）。"""
    提交 = str(提交 or "")
    if not 提交:
        return None
    if not (提交可查 or _提交可读)(提交):
        return None
    取 = 对象查 or _路径对象
    行们 = ["面v1"]
    命中数 = 0
    for 路径 in 自举面路径:
        对象 = 取(提交, 路径) or ""
        if 对象:
            命中数 += 1
        行们.append("%s\t%s" % (路径, 对象 or "-"))
    if not 命中数:
        return None
    return hashlib.sha256(("\n".join(行们) + "\n").encode("utf-8")).hexdigest()[:面摘要宽度]


def 摘要自分支名(名: str) -> str:
    """预验分支名 → 面摘要（`ci/预验-<sha10>-m6<面摘要>`）；不合该形=空串。

    形态契约：`-m6` 之后**恰为 面摘要宽度 位 hex** 才是面摘要。旧实现的
    time_ns 后缀恒 ≤16 hex、一次性绕行名=面摘要+8 位 nonce（40 hex）——宽度
    不同，故「恰 32 位」既是解析判据也是形态判别（不是按语义猜：凡宽度不符一律
    当无摘要，不复用·宁多跑不漏放）。"""
    名 = str(名 or "")
    标记 = "-m6"
    位 = 名.rfind(标记)
    if 位 < 0:
        return ""
    尾 = 名[位 + len(标记):]
    if len(尾) != 面摘要宽度 or any(c not in "0123456789abcdef" for c in 尾):
        return ""
    return 尾


def 同面绿行(行们, 源分支: str, 面: str) -> list[dict]:
    """纯函数：预验任务行 → 「同源分支 + 完成·绿 + 分支名内嵌面摘要 == 本轮面」的候选。

    只做粗筛（服务端快照可见的事实）；M6 通过证据须由调用方按分支名取详情核
    （证据在结果 JSON 里，快照没有——不在此处假定）。面为空=不复用（无证据）。"""
    if not 面:
        return []
    return [行 for 行 in (行们 or [])
            if str(行.get("状态") or "") == "完成" and 行.get("绿")
            and str(行.get("源分支") or "") == 源分支
            and 摘要自分支名(str(行.get("分支") or "")) == 面]


def 面复用(源分支: str, 面: str, 状态查, 证据查) -> bool:
    """#1599 M6 跨基复用（甲案的**安全形态**）：M6 输入面逐字节相同的绿记录 → 可免重跑。

    判据链（全机械，无一处「推断 develop 增量碰没碰写集」）：
      ①记录自描述：预验分支名内嵌 `-m6<面摘要>`（摘要自分支名·无需历史提交对象在本机）；
      ②面摘要相等：自举面 = 宿主/v2/stdlib/M6 用例与编排/构建入口的 tree/blob 对象 id
        哈希（面摘要）——面相等 ⇒ fix_p/fix_s 输入逐字节相同 ⇒ 逐字节结论必然保持，
        这是复用成立的**唯一**理由；
      ③记录完成·绿且带 M6 通过证据（证据查·#1221「无证据旧绿不可信」同口径）。
    面不同（develop 改了编译器）= 必须重跑——**绝不「只补验受影响面」**：M6 是全局
    逐字节性质，不可按文件分解（#1599 论证·甲案后半不成立故未实现）。
    面之外的部分沿用 830 用户裁决口径（2026-10-15「内容未变·绿照用·组合正确性由
    develop 平台轮转全量权威守护」）——本判据比 830 更严（830 零条件即复用）。
    源分支须相同：面相等只证明 M6 输入相同，不证明补丁相同。
    查询失败/无候选/证据缺失=False（宁多跑不漏放）。
    状态查(路径)->dict|None 与 证据查(分支, 链顶)->bool 由调用方注入（本模块零网络）。"""
    if not 源分支 or not 面:
        return False
    态 = 状态查("/api/state")
    if not 态:
        return False
    for 行 in 同面绿行(态.get("预验任务") or [], 源分支, 面):
        分支 = str(行.get("分支") or "")
        if 分支 and 证据查(分支, str(行.get("sha") or "")):
            return True
    return False


def 绕行原因(名: str, 链顶: str, 面: str, 状态查, 证据查) -> str:
    """#1599 确定性命名下的绕行判定：返回空串=直接用 名 入队；非空=须改用一次性名。

    背景（#1599 量化·近 8 小时 50 行/31 完成）：M6 轮原用 `-m6<纳秒hex>` 非幂等名，
    同一链顶被重复入队 6 次、6 轮全量白跑（重复轮 13/31=42%），把本就 2-3 槽的池
    压得更深（等待中位 121 分钟）。定名改为面摘要确定性键后：同一链顶恒同名 ⇒ 服务端
    「排队/执行中→不动」把重复入队收成一行，「完成绿」幂等直取旧结论。

    两类绕行（都显式披露，不静默）：
      · 面摘要不可判（git 提交对象/仓库异常）——不去重（绝不以默认摘要冒充「同一面」）；
      · 同名既有行是「完成·绿但无 M6 证据」——幂等直取会把无证据绿当结论=#1221 P0-1
        死锁原形；绕开后新行结论仍由 查同链顶M6绿记录（按 sha）复用到，不丢证据。
    今日 `ci/预验-*` 轮无「证据缺失绿」来路（轮早退仅 develop 轮·平台不可验判红），
    第二条是防回归的机械兜底而非常态路径。
    名由调用方按协议拼装（本模块不重复分支名协议）；状态查/证据查=注入（零网络）。"""
    if not 面:
        return ("面摘要不可判（git 提交对象/仓库异常）——改用一次性分支名"
                "（不去重、不跨基复用：宁多跑不漏放）")
    态 = 状态查("/api/state") or {}
    已有 = next((行 for 行 in (态.get("预验任务") or [])
                 if str(行.get("分支") or "") == 名), None)
    if (已有 is not None and str(已有.get("状态") or "") == "完成"
            and 已有.get("绿") and not 证据查(名, 链顶)):
        return ("同名行 %s 完成·绿但无 M6 通过证据——追加一次性 nonce 绕开"
                "（防服务端幂等直取无证据旧绿=#1221 P0-1 死锁）" % 名)
    return ""


def 平台可跑M6(平台: str, 交叉模拟: bool = False) -> bool:
    """本平台能否**原生**跑 M6：win-x64/linux-x86_64 恒可；linux-arm64 仅真机可
    （交叉模拟=x86_64 宿主 qemu 跑 arm64 ELF，78/79 登记跳过=数小时性能深坑）。"""
    if 平台 not in M6原生平台:
        return False
    if 平台 == "linux-arm64":
        return not 交叉模拟
    return True


def 判定轮(文件们, 平台: str, 交叉模拟: bool = False) -> dict:
    """纯函数：一轮（develop/预验/兜底）的 M6 计划。

    处置三态：
      未触发   = 写集不含编译器产物面（codegen/opt/ir/model/协议常量/stdlib）（纯文档/脚本/前端轮零成本）
      必跑     = 触发且本平台可原生跑（执行面须 --require-m6：跳过即红）
      未跑-转派 = 触发但本平台不可原生跑（预验轮判红；develop 轮由平台轮转承担）
    """
    写集 = 判定写集(文件们)
    可跑 = 平台可跑M6(平台, 交叉模拟)
    计划 = dict(写集)
    计划["平台"] = 平台
    计划["交叉模拟"] = bool(交叉模拟)
    计划["平台可跑"] = 可跑
    if not 写集["触发"]:
        计划["处置"] = "未触发"
    elif 可跑:
        计划["处置"] = "必跑"
    else:
        计划["处置"] = "未跑-转派"
    计划["说明"] = 摘要(计划)
    return 计划


def 判定未知写集(原因: str, 平台: str, 交叉模拟: bool = False) -> dict:
    """写集不可判（git 异常/基线缺失）=fail-safe 触发——多付一次 M6 成本优于漏验。

    处置仍按平台能力分流（必跑/未跑-转派），并显式携带「写集不可判」+原因。"""
    计划 = 判定轮([M6写集前缀[0] + "<写集不可判>"], 平台, 交叉模拟)
    计划["写集不可判"] = True
    计划["写集原因"] = 原因
    计划["说明"] = "M6 fail-safe 触发（写集不可判：%s）" % 原因
    return 计划


def 摘要(计划: dict) -> str:
    """计划→单行摘要（M6 未跑必须带原因——静默豁免即缺陷）。"""
    处置 = (计划 or {}).get("处置")
    if 处置 == "未触发":
        return "M6 未触发（写集不含编译器产物面（codegen/opt/ir/model/协议常量/stdlib））"
    if 处置 == "必跑":
        return ("M6 必跑（写集命中 %d 文件：%s）"
                % (len(计划.get("命中") or []),
                   "、".join((计划.get("命中") or [])[:3])
                   + ("…" if len(计划.get("命中") or []) > 3 else "")))
    if 处置 == "未跑-转派":
        return ("M6 未跑（%s 交叉模拟无法原生跑 79_v2——由 win/linux-x64/arm64 真机轮承担）"
                % 计划.get("平台", "?"))
    return "M6 计划缺失（处置=%r）——按未跑处置" % (处置,)


def _是M6名(名) -> bool:
    """精确识别 M6 用例名（#1221 复审 P1-1）：只认 `79_v2_自举闭环` 或 `79_v2_` 前缀。

    禁子串匹配——`179_v2_…/279_v2_…/479_v2_…` 含 "79_v2" 子串，旧口径会把它们
    误判成 M6（误拦点名清单/误伤失败清单）。失败清单条目先按 `:` 截名再判。"""
    文本 = str(名 or "").split(":", 1)[0].strip()
    return 文本 == M6用例名 or 文本.startswith(M6用例关键字 + "_")


def 误放行检查(点名清单) -> str | None:
    """M6 红禁放行：--allow-known-red/--allow-pool-red 点名含 79_v2 即拒。

    返回 None=点名清单不含 M6；非 None=拒绝原因。调用方在参数解析后与放行消费点
    双挂（单一事实源本函数）。"""
    违规 = [n for n in (点名清单 or []) if _是M6名(n)]
    if 违规:
        return ("M6（%s·自举固定点）红不得以 --allow-known-red/--allow-pool-red 放行"
                "——点名 %s；请修复编译器产物面（codegen/opt/ir/model/协议常量/stdlib）"
                "后重跑（#1221 机械保证）。"
                % (M6用例名, "、".join(str(n) for n in 违规)))
    return None


def 红例含M6(红例们) -> bool:
    """失败/红用例清单是否含 M6（79_v2）——放行消费点硬拦判据（点名说辞不可绕过）。"""
    return any(_是M6名(n) for n in (红例们 or []))


def 等价免验冲突(触发: bool, 开关: bool) -> str | None:
    """#1221 复审 P1-4：--verified-same-content（等价免验）与 M6 触发互斥。

    等价免验=整体跳过全量+云预验；M6 触发轮必须真跑固定点——两者不可并存。
    返回 None=允许；非 None=拒绝原因（批流程消费）。"""
    if 触发 and 开关:
        return ("--verified-same-content 与 M6 写集触发互斥（#1221）："
                "编译器产物面（codegen/opt/ir/model/协议常量/stdlib）改动轮不得以"
                "等价免验跳过全量/云预验——去掉该开关后重跑集成。")
    return None


def 需重新云端预验(首批已绿: bool, 触发: bool) -> bool:
    """#1221 复审 P1-5：重组轮是否仍需**进入**云端预验门禁。

    M6 触发轮恒 True：链顶已变，旧绿默认不适用——是否真重跑由门禁内的两级复用判据
    决定（#1599：同链顶证据在案 或 同 M6 输入面证据在案 才免重跑；见 面复用/绕行原因）。
    普通轮沿用「首批已绿=免重跑」复用语义。"""
    return (not 首批已绿) or bool(触发)


def 预验认领限制(须M6: bool, 用户限制) -> tuple[list[str], str]:
    """预验任务认领平台白名单决策（#1221 P1-7·纯函数）。

    须M6 且用户未显式限制 → 限原生 M6 平台 win-x64/linux-x86_64（交叉模拟 arm64
    无法原生跑固定点）；用户显式限制优先（含 arm64 且为真机仍可跑，交叉模拟实例
    会由 ci_daemon fail-closed 判红）。返回 (限制列表, 提示文本·空=无提示)。"""
    限制 = [str(x).strip() for x in (用户限制 or []) if str(x).strip()]
    if not 须M6:
        return 限制, ""
    if not 限制:
        return list(M6预验原生平台), (
            "M6 触发——预验认领限 win-x64/linux-x86_64（真机 arm64 可跑，但平台字段"
            "无法区分交叉模拟；arm64 覆盖由 develop 轮转真机轮承担）")
    if "linux-arm64" in 限制:
        return 限制, ("注意：--pool-limit 含 linux-arm64——若认领者为交叉模拟实例，"
                     "预验轮会因 M6 无法原生运行而判红（真机 arm64 可跑）")
    return 限制, ""


def _自测() -> int:
    """--selftest：纯函数正反例（零 git 零构建·CI 式）。"""
    断言们: list[tuple[str, bool]] = []

    def 断言(说明: str, 条件: bool) -> None:
        断言们.append((说明, bool(条件)))

    # ── 判定写集：五面前缀正例 ──
    正例 = [
        "src/cn_compiler/codegen/x64/指令发射.cpp",
        "src/cn_compiler/codegen/arm64/后端.cpp",
        "src/cn_compiler/opt/窥孔.cpp",
        "src/cn_compiler/ir/ir.cpp",
        "src/cn_compiler/model/类型.h",
        "CN语言编译器v2/代码生成/代码生成ARM64.cn",
        "CN语言编译器v2/IR/优化.cn",
        "CN语言编译器v2/协议常量/协议常量.cn",
        "stdlib/字符串.cn",
    ]
    for 路径 in 正例:
        断 = 判定写集([路径])
        断言("正例触发：%s" % 路径, 断["触发"] and 断["命中"] == [路径])

    # ── 判定写集：纯前端/文档/脚本/测试/相似前缀反例 ──
    反例 = [
        "src/cn_compiler/lexer/lexer.cpp",
        "src/cn_compiler/parser/parser.cpp",
        "src/cn_compiler/semantic/checker.cpp",
        "src/cn_compiler/driver/main.cpp",
        "src/cn_compiler/irx/不是IR目录.cpp",      # 前缀边界：ir/ ≠ irx/
        "src/cn_compiler/codegenx/a.cpp",          # 前缀边界：codegen/ ≠ codegenx/
        "CN语言编译器v2/词法/词法.cn",
        "CN语言编译器v2/语法/语法.cn",
        "CN语言编译器v2/语义/语义.cn",
        "CN语言编译器v2/主.cn",
        "CN语言编译器v2/驱动/驱动.cn",
        "stdlibx/不是标准库.cpp",                 # 前缀边界：stdlib/ ≠ stdlibx/
        "docs/README.md",
        "scripts/m6_gate.py",
        "tests/e2e/79_v2_自举闭环/v2锚定链.txt",
    ]
    for 路径 in 反例:
        断 = 判定写集([路径])
        断言("反例豁免：%s" % 路径, not 断["触发"] and 断["命中"] == [])

    # ── 混合写集：任一命中即触发；命中清单去重排序 ──
    混合 = 判定写集(["docs/x.md",
                    "src/cn_compiler/opt/窥孔.cpp",
                    "src/cn_compiler/opt/窥孔.cpp",
                    "src/cn_compiler/lexer/lexer.cpp"])
    断言("混合写集：任一命中即触发且去重", 混合["触发"] and 混合["命中"] == ["src/cn_compiler/opt/窥孔.cpp"])

    # ── 路径归一：反斜杠/前导 ./ 形态仍命中（586-a 同族）──
    断言("路径归一：反斜杠写集命中",
         判定写集(["src\\cn_compiler\\ir\\ir.cpp"])["触发"])
    断言("路径归一：前导 ./ 写集命中",
         判定写集(["./src/cn_compiler/codegen/x64/a.cpp"])["触发"])
    断言("空写集=不触发（未知由调用方 fail-safe 另判）", not 判定写集([])["触发"])
    断言("None 写集=不触发不炸", not 判定写集(None)["触发"])

    # ── 判定自举写集（#1796）：编译器链正例（含 M6 豁免的前端面）/纯外围反例 ──
    自举正例 = [
        "src/cn_compiler/lexer/lexer.cpp",         # M6 豁免但自举触发（前端面）
        "src/cn_compiler/semantic/checker.cpp",
        "src/cn_compiler/driver/main.cpp",
        "CN语言编译器v2/主.cn",
        "CN语言编译器v2/语义/语义.cn",
        "runtime/运行时.c",
        "tests/e2e/78_v2_自举链构建/主.cn",
        "tests/e2e/79_v2_自举闭环/v2锚定链.txt",   # M6 反例但自举触发（用例自身面）
    ]
    for 路径 in 自举正例:
        断 = 判定自举写集([路径])
        断言("自举正例触发：%s" % 路径, 断["触发"] and 断["命中"] == [路径])
    自举反例 = [
        "docs/README.md",
        "website/tools/docs_data.py",
        "scripts/m6_gate.py",
        "scripts/queue_service.py",
        "tests/e2e/113_某用例/主.cn",
        "tests/unit/某测试.cpp",
        "tests/e2e/789_v2_相似前缀/主.cn",         # 前缀边界：78_v2_ ≠ 789_v2_
        "tests/e2e/780_v2_另一前缀/主.cn",         # 前缀边界：79_v2_ ≠ 780_v2_
        "plans/001.md",
    ]
    for 路径 in 自举反例:
        断 = 判定自举写集([路径])
        断言("自举反例豁免：%s" % 路径, not 断["触发"] and 断["命中"] == [])
    # M6 面 ⊂ 自举面（--require-m6 与 --skip-heavy 永不并发的机械保证·#1796）
    断言("包含关系：M6 触发 ⇒ 自举触发",
         all(判定自举写集([p])["触发"] for p in 正例))
    断言("自举空写集=不触发（未知由调用方 fail-safe 另判）",
         not 判定自举写集([])["触发"] and not 判定自举写集(None)["触发"])
    断言("自举路径归一：反斜杠写集命中",
         判定自举写集(["src\\cn_compiler\\lexer\\a.cpp"])["触发"])

    # ── 平台能力 ──
    断言("win-x64 原生可跑", 平台可跑M6("win-x64"))
    断言("linux-x86_64 原生可跑", 平台可跑M6("linux-x86_64"))
    断言("linux-arm64 真机可跑", 平台可跑M6("linux-arm64", 交叉模拟=False))
    断言("linux-arm64 交叉模拟不可跑", not 平台可跑M6("linux-arm64", 交叉模拟=True))
    断言("未知平台不可跑（fail-closed）", not 平台可跑M6("darwin-arm64"))

    # ── 判定轮三态 ──
    码 = ["src/cn_compiler/codegen/x64/a.cpp"]
    未触发 = 判定轮(["docs/a.md"], "win-x64")
    必跑 = 判定轮(码, "win-x64")
    转派 = 判定轮(码, "linux-arm64", 交叉模拟=True)
    断言("轮三态：未触发", 未触发["处置"] == "未触发" and "M6 未触发" in 未触发["说明"])
    断言("轮三态：必跑", 必跑["处置"] == "必跑" and 必跑["平台可跑"]
         and "M6 必跑" in 必跑["说明"] and "codegen" in 必跑["说明"])
    断言("轮三态：交叉模拟转派", 转派["处置"] == "未跑-转派" and not 转派["平台可跑"]
         and "M6 未跑" in 转派["说明"] and "交叉模拟" in 转派["说明"])
    未知 = 判定未知写集("git diff 失败", "win-x64")
    断言("未知写集=fail-safe 触发（不可判不漏验）",
         未知["触发"] and 未知["处置"] == "必跑" and 未知.get("写集不可判")
         and "fail-safe" in 未知["说明"])
    未知转派 = 判定未知写集("基线缺失", "linux-arm64", 交叉模拟=True)
    断言("未知写集+交叉模拟=转派（处置按平台能力分流）",
         未知转派["触发"] and 未知转派["处置"] == "未跑-转派")

    # ── M6 红禁放行（点名清单）──
    断言("点名清单含 79_v2=拒", 误放行检查(["79_v2_自举闭环"]) is not None)
    断言("点名清单含 79_v2 变体=拒", 误放行检查(["79_v2_其他后缀"]) is not None)
    断言("点名清单不含 M6=放行", 误放行检查(["113_某用例", "127_v2_某闭环"]) is None)
    断言("179/279/479_v2 子串不误拒（P1-1 精确名）",
         误放行检查(["179_v2_构造字面量穷举负测", "279_v2_空指针常量传播",
                     "479_v2_主返回非整错误值拒绝"]) is None)
    断言("空点名清单=放行", 误放行检查([]) is None and 误放行检查(None) is None)
    断言("红例含 M6=硬拦", 红例含M6(["79_v2_自举闭环"]) and 红例含M6(["79_v2_自举闭环: 固定点不一致"]))
    断言("红例不含 M6=不拦", not 红例含M6(["78_v2_自举链构建"]) and not 红例含M6([]))
    断言("179/279/479_v2 失败不误判为 M6",
         not 红例含M6(["179_v2_构造字面量穷举负测: x", "279_v2_空指针常量传播: y",
                      "479_v2_主返回非整错误值拒绝: z"]))

    # ── 预验认领限制（P1-7 纯函数）──
    限0, 提0 = 预验认领限制(False, [])
    限1, 提1 = 预验认领限制(True, [])
    限2, 提2 = 预验认领限制(True, ["linux-arm64"])
    断言("认领限制：非 M6=用户限制原样", 限0 == [] and 提0 == "")
    断言("认领限制：M6 触发缺省=原生平台白名单",
         限1 == ["win-x64", "linux-x86_64"] and "M6 触发" in 提1)
    断言("认领限制：用户显式含 arm64=保留+告警",
         限2 == ["linux-arm64"] and "注意" in 提2)

    # ── 等价免验/云端预验复用（P1-4/P1-5 纯函数）──
    断言("等价免验：触发+开关=拒/触发+无开关=允许/未触发+开关=允许",
         等价免验冲突(True, True) is not None
         and 等价免验冲突(True, False) is None
         and 等价免验冲突(False, True) is None)
    断言("云端预验复用：触发轮恒进门禁（真跑与否由门禁两级复用判据定）/普通轮沿用首批已绿",
         需重新云端预验(True, True) and 需重新云端预验(False, True)
         and not 需重新云端预验(True, False) and 需重新云端预验(False, False))

    # ── #1599 M6 输入面摘要与复用判据（注入 git 面·零仓库副作用）──
    def 造对象查(映射):
        return lambda 提交, 路径: 映射.get(路径, "")

    面基 = {"src": "t-src", "CN语言编译器v2": "t-v2", "stdlib": "t-std"}
    摘基 = 面摘要("c1", 提交可查=lambda c: True, 对象查=造对象查(面基))
    断言("面摘要：同对象面=同摘要·改任一面对象即变·提交不可读/面全空=None",
         len(摘基 or "") == 面摘要宽度
         and 摘基 == 面摘要("c1", 提交可查=lambda c: True, 对象查=造对象查(dict(面基)))
         and 摘基 != 面摘要("c2", 提交可查=lambda c: True,
                            对象查=造对象查({**面基, "src": "t-src2"}))
         and 面摘要("c9", 提交可查=lambda c: False, 对象查=造对象查(面基)) is None
         and 面摘要("c9", 提交可查=lambda c: True, 对象查=造对象查({})) is None)
    面路径账: list[str] = []

    def 记账对象查(提交, 路径):
        面路径账.append(路径)
        return 面基.get(路径, "")

    面摘要("c1", 提交可查=lambda c: True, 对象查=记账对象查)
    断言("面摘要：面清单逐条进预像（查询面=自举面路径全量·增删面路径即改摘要）",
         len(面路径账) == len(自举面路径) and set(面路径账) == set(自举面路径))
    名面 = "ci/预验-abcdef0123-m6" + "a" * 32
    断言("分支名面摘要解析：恰 32hex 才算面摘要（legacy 纳秒/一次性 nonce/无后缀=空）",
         摘要自分支名(名面) == "a" * 32
         and 摘要自分支名("ci/预验-abcdef0123-m618e3b7f864ba5e30") == ""
         and 摘要自分支名("ci/预验-abcdef0123-m6" + "a" * 40) == ""
         and 摘要自分支名("ci/预验-abcdef0123") == "")
    候选表 = [{"状态": "完成", "绿": 1, "源分支": "任务/1", "分支": 名面},
              {"状态": "完成", "绿": 0, "源分支": "任务/1", "分支": 名面},
              {"状态": "完成", "绿": 1, "源分支": "任务/2", "分支": 名面}]
    断言("同面绿行：只留 完成·绿+同源分支+面相等（面空=不复用）",
         len(同面绿行(候选表, "任务/1", "a" * 32)) == 1
         and 同面绿行(候选表, "任务/1", "b" * 32) == []
         and 同面绿行(候选表, "任务/1", "") == [])
    态面 = {"预验任务": [{"分支": 名面, "sha": "f" * 40, "源分支": "任务/1",
                        "状态": "完成", "绿": 1}]}
    真证 = lambda 分支, 顶: True
    假证 = lambda 分支, 顶: False
    断言("面复用：同面绿+证据=True·无证据/面变/源分支异/查询不可达=False",
         面复用("任务/1", "a" * 32, lambda p: 态面, 真证) is True
         and 面复用("任务/1", "a" * 32, lambda p: 态面, 假证) is False
         and 面复用("任务/1", "b" * 32, lambda p: 态面, 真证) is False
         and 面复用("任务/9", "a" * 32, lambda p: 态面, 真证) is False
         and 面复用("任务/1", "a" * 32, lambda p: None, 真证) is False
         and 面复用("任务/1", "", lambda p: 态面, 真证) is False)
    断言("绕行原因：面可用+无同名行=空（确定性名直接用）·面空=须一次性名",
         绕行原因(名面, "f" * 40, "a" * 32, lambda p: {"预验任务": []}, 真证) == ""
         and 绕行原因("", "f" * 40, "", lambda p: {"预验任务": []}, 真证) != "")
    断言("绕行原因：同名行完成绿无证据=须一次性名（防 P0-1 幂等直取死锁）·有证据=不绕行",
         绕行原因(名面, "f" * 40, "a" * 32, lambda p: 态面, 假证) != ""
         and 绕行原因(名面, "f" * 40, "a" * 32, lambda p: 态面, 真证) == "")

    失败 = 0
    for 说明, 通过 in 断言们:
        print("  [%s] %s" % ("✓" if 通过 else "✗", 说明))
        if not 通过:
            失败 += 1
    print("\n[m6_gate selftest] %d 例·失败 %d" % (len(断言们), 失败))
    return 1 if 失败 else 0


def _命令行(argv: list[str]) -> int:
    """非纯函数胶水面：--changed-since <rev>（GitHub Actions 等不可改命令行调用面）。

    git diff --name-only <rev>..HEAD → 判定写集 → 触发时输出 CN_M6_REQUIRE=1
    （配 --gh-env 写入 $GITHUB_ENV 形态）。git 异常=fail-safe 触发（不漏验）；
    退出码恒 0（触发是信号而非失败——由 E2E 的 --require-m6 语义判红）。"""
    if "--changed-since" in argv:
        i = argv.index("--changed-since")
        基线 = argv[i + 1] if i + 1 < len(argv) else ""
        gh_env = "--gh-env" in argv
        if not 基线:
            print("错误: --changed-since 需要基线 rev", file=sys.stderr)
            return 2
        try:
            结果 = subprocess.run(
                ["git", "-c", "core.quotepath=false", "diff", "--no-renames",
                 "--name-only", "%s..HEAD" % 基线],
                capture_output=True, text=True, encoding="utf-8", errors="replace",
                timeout=120)
            文件们 = ([行.strip().strip('"') for 行 in (结果.stdout or "").splitlines()
                      if 行.strip()] if 结果.returncode == 0 else None)
            原因 = "" if 结果.returncode == 0 else (结果.stderr or "").strip()[:160]
        except Exception as 异常:                 # 超时/OSError 同样不可判
            文件们, 原因 = None, repr(异常)
        if 文件们 is None:
            摘要 = "M6 fail-safe 触发（写集不可判：%s）" % 原因
            触发 = True
        else:
            判定 = 判定写集(文件们)
            触发 = 判定["触发"]
            摘要 = 判定["说明"]
        if gh_env:
            # #1221 P1-2：$GITHUB_ENV 只接受 KEY=VALUE——摘要走 stderr，stdout 只出环境行
            print(摘要, file=sys.stderr)
            if 触发:
                print("CN_M6_REQUIRE=1")
        else:
            print(摘要)
        return 0
    print("用法: python scripts/m6_gate.py --selftest | "
          "--changed-since <rev> [--gh-env]")
    return 0


if __name__ == "__main__":
    if "--selftest" in sys.argv:
        sys.exit(_自测())
    sys.exit(_命令行(sys.argv[1:]))
