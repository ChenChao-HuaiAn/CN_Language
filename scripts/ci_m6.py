#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""云端轮 M6 写集触发与 E2E 段编排（任务 #1221·2026-10-23）。

两个职责（均自 ci_daemon.py 外迁/新增，守既有 900 行警戒线——「先拆再加」）：
  ① 轮写集 → M6 计划：预验轮=三点 diff <origin/develop>...<sha>（链顶=合并结果，
     develop 只前进不回退，merge-base 即链基底；远端 ref 陈旧只会多算不漏算）；
     develop/兜底轮=上次已验绿 sha（ci-logs/latest.json）..本轮 sha，缺失退
     sha~1。git 异常/写集不可判=fail-safe 触发（多付一次 M6 成本优于漏验）。
  ② E2E 段编排：原 ci_daemon.静态门禁与e2e段 的 ④ 段原样搬移（902 红即停/
     1297 分片/842 明细/566 重负载通道语义逐行保持），只加 M6 门禁三态——
       触发+本平台可跑 → e2e 带 --require-m6（跳过即红）
       触发+交叉模拟 arm64 → 预验轮加「M6平台保障」rc=-1（fail-closed·不得静默进
         develop）；develop/兜底轮记「转派」（显式原因·由平台轮转的原生轮承担）
       未触发 → 零成本（纯文档/脚本/前端轮不付 M6）
     执行记录（md5/首个差异字节）从 run_e2e --json 明细「M6」键回填进轮结果。

依赖方向：ci_daemon → 本模块（单向·禁反引）；ci_m6 → m6_gate（判定单点）+
ci_shard（E2E 命令/分片）+ ci_fallback（基线）+ ci_report（物理内存）。
"""
from __future__ import annotations

import json
import os
import subprocess
from pathlib import Path

import ci_fallback
import ci_report
import ci_shard
import m6_gate

# 预验分支前缀（与 integrate.预验远程分支前缀 同口径·本模块只读分支名做基线选择）
预验分支前缀 = "ci/预验-"


def 原生平台(是win: bool, 机架: str) -> str:
    """本机原生平台名（win-x64 / linux-arm64 / linux-x86_64）。"""
    if 是win:
        return "win-x64"
    return "linux-arm64" if (机架 or "").lower() in ("aarch64", "arm64") else "linux-x86_64"


def 交叉模拟态(平台: str, 机架: str) -> bool:
    """交叉模拟判定（与 tests/e2e/e2e交叉模拟.判定交叉模拟 同构的本地判据）：
    报 linux-arm64 而宿主非 arm64=x86_64 宿主 qemu 面；78/79 在该面登记跳过。"""
    return 平台 == "linux-arm64" and (机架 or "").lower() not in ("aarch64", "arm64")


def M6原生可跑(平台: str, 机架: str) -> bool:
    """实例「M6 原生可跑」能力位（池协议上报·#1221 复审 P1-7）：
    真机 win/linux-x64/arm64=True，交叉模拟 arm64=False。"""
    return m6_gate.平台可跑M6(平台, 交叉模拟态(平台, 机架))


def 轮写集(轮分支: str, sha: str, 仓库根: Path,
           远端: str = "origin", 基线查=None,
           运行=None) -> tuple[list[str] | None, str]:
    """本轮写集（改动文件清单）；None=不可判（调用方 fail-safe 触发）。

    基线选择：
      预验轮（ci/预验-*）：<远端>/develop...<sha>——链顶=在最新 develop 上叠的
        任务提交集；中文路径用 core.quotepath=false（586-a 同族：路径转义会令
        前缀判定静默失效）。
      develop/兜底轮：上次已验绿 sha..本轮 sha（latest.json）；基线缺失=不可判
        （fail-safe 触发·#1221 复审 P2——不再拿 sha~1 猜）；基线==本轮 sha 才退
        sha~1。
    diff 前先 `git fetch <远端> develop`（带超时；失败不阻断，沿用已有远端 ref——
    陈旧 ref 只会多算不漏算）。`--no-renames`：重命名拆成删除+新增，前缀判定不漏。
    返回 (文件们|None, 原因)。运行=可注入 subprocess.run（selftest 零 git）。
    """
    跑 = 运行 or (lambda 命令: subprocess.run(
        命令, capture_output=True, text=True, encoding="utf-8", errors="replace",
        cwd=仓库根, timeout=120))
    # #1221 复审 P2：刷新 develop ref（fetch 专用 30s 超时·失败不阻断——陈旧 ref
    #   只会多算不漏算；分片从片不计算计划=零 fetch）
    try:
        if 运行 is None:
            subprocess.run(["git", "fetch", 远端, "develop"], capture_output=True,
                           text=True, encoding="utf-8", errors="replace",
                           cwd=仓库根, timeout=30)
        else:
            跑(["git", "fetch", 远端, "develop"])
    except Exception:
        pass   # 网络/超时——沿用已有远端 ref（下面 diff 失败仍会 fail-safe 触发）
    基线 = ""
    原因 = ""
    if 轮分支.startswith(预验分支前缀):
        基线 = "%s/develop" % 远端
    else:
        基线 = (基线查() if 基线查 is not None else ci_fallback.上次已验SHA()) or ""
        if not 基线:
            return None, "基线缺失（latest.json 无上次已验绿 sha）——写集不可判"
        if 基线 == sha:
            基线 = "%s~1" % sha
    命令 = ["git", "-c", "core.quotepath=false", "diff", "--no-renames",
            "--name-only",
            ("%s...%s" if 轮分支.startswith(预验分支前缀) else "%s..%s") % (基线, sha)]
    try:
        结果 = 跑(命令)
    except Exception as 异常:          # 超时/OSError 同样不可判
        return None, "git diff 异常：%r" % (异常,)
    if getattr(结果, "returncode", 1) != 0:
        return None, "git diff 失败（基线 %s）：%s" % (
            基线, (getattr(结果, "stderr", "") or "").strip()[:160])
    文件们 = [行.strip().strip('"') for 行 in (结果.stdout or "").splitlines() if 行.strip()]
    return 文件们, "基线 %s..%s（%d 文件）" % (基线, sha[:10], len(文件们))


def 轮计划(轮分支: str, sha: str, 平台: str, 是win: bool, 仓库根: Path,
           远端: str, 机架: str, 基线查=None, 运行=None) -> dict:
    """云端轮 M6 计划（m6_gate 判定 + git 写集胶水）——结果进轮记录「M6」键。"""
    交叉 = 交叉模拟态(平台, 机架)
    写集, 原因 = 轮写集(轮分支, sha, 仓库根, 远端, 基线查, 运行)
    if 写集 is None:
        计划 = m6_gate.判定未知写集(原因, 平台, 交叉)
    else:
        计划 = m6_gate.判定轮(写集, 平台, 交叉)
        计划["写集原因"] = 原因
    return 计划


def 不可验处置(轮分支: str, m6计划: dict, 日志) -> bool:
    """#1570：M6 触发且本平台不可原生跑时的处置。True=早退判红（预验轮）；
    False=develop/兜底备份轮记转派并继续 E2E（M6 由原生平台/预验承担）。"""
    if 轮分支 == "develop" or 轮分支.startswith("ci/兜底-"):
        print("  [M6·转派] %s——develop 备份轮继续 E2E（不早退）"
              % m6计划.get("说明", ""), file=日志, flush=True)
        return False
    print("  [M6·平台不可验] %s——构建前早退（不跑 E2E）"
          % m6计划.get("说明", ""), file=日志, flush=True)
    return True


保内存失败说明 = ("物理内存探测失败——无法确定 --max-mem-mb 保险丝"
                  "（win=GetPhysicallyInstalledSystemMemory / linux=sysconf·/proc/meminfo"
                  " 均不可用；拒绝默认值继续·#1608）")


def 内存保险丝或判红(步骤们: dict, 日志) -> int | None:
    """#1608：物理内存→max-mem 保险丝；探测失败写 步骤们["e2e"] rc=-1（聚合绿
    白名单键=本轮必红）并返回 None——调用点必须跳过 e2e（禁止默认值继续）。
    主 e2e 段与分片轮共用（失败文案单点·不漂移）。"""
    保内存 = ci_report.保内存保险丝MB(ci_report.物理内存MB())
    if 保内存 is None:
        步骤们["e2e"] = {"rc": -1, "说明": 保内存失败说明}
        print("  [e2e·红] " + 保内存失败说明, file=日志, flush=True)
    return 保内存


def e2e段(步骤们: dict, 步骤, cn: Path, 日志, sha: str, 轮分支: str, 分片: int,
          分片召唤时刻, 计划: dict, *, 是win: bool, 平台: str, 日志目录: Path,
          e2e并行: int, e2e超时秒: int, runner标识: str, 启动心跳,
          e2e明细路径: Path) -> None:
    """④ E2E 段（原 ci_daemon 内联逐行搬移 + M6 门禁）。

    M6 门禁（#1221 + 复审 P1-7/8/计划缺失）：
      必跑  → e2e 命令带 --require-m6（选集含 79_v2 而未跑/未通过=exit 1）；
              跑完强制核 `--json` 明细 M6.状态=="通过"（缺失/非通过=rc -1 拖红）。
      不可跑（交叉模拟 arm64 等）→ 一律「M6平台保障」rc=-1（fail-closed·预验与
              develop/兜底同规——该轮不落 latest，下一轮写集仍含改动→原生平台重验）。
      计划缺失/形态坏 → 按不可判 fail-closed 处理（不静默当未触发）。
      未触发 → 不写 M6 步骤（零成本）。
    #1608：物理内存探测失败（None）=e2e 不跑、e2e 步骤 rc=-1 显式红——不再按
      1528 的「探测失败=4096 兜底」拿假数字继续（是win 仅决定命令形态，探测按实际平台）。
    分片：片 0 恒含 79_v2（#1420 重者留主）——主实例 e2e基 带 require，从片不带。
    """
    计划 = 计划 if isinstance(计划, dict) and 计划.get("处置") else \
        m6_gate.判定未知写集("M6 计划缺失/形态坏", 平台, 交叉模拟态(平台, ""))
    触发 = bool(计划.get("触发"))
    可跑 = bool(计划.get("平台可跑"))
    须M6 = 触发 and 可跑
    if 须M6 and not ci_shard.e2e支持m6():
        # 跨版本窗口：旧版 run_e2e 无 --require-m6（传参=argparse 假红·不传=假绿）——
        # fail-closed 判红并给明确原因（升级 runner 树/部署后重跑）。
        步骤们["M6平台保障"] = {"rc": -1, "类别": "版本不支持",
                               "说明": "run_e2e 版本过旧无 --require-m6——M6 无法机械验证"
                                       "（fail-closed；更新 runner 树后重跑）"}
        print("  [M6·版本不支持] 本树 run_e2e 无 --require-m6——轮判红（fail-closed）",
              file=日志, flush=True)
        return
    if 触发:
        if 可跑:
            步骤们["M6平台保障"] = {"rc": 0, "说明": "M6 必跑（--require-m6·#1221）"}
            print("  [M6] %s——e2e 带 --require-m6（79_v2 跳过/失败=本轮红）"
                  % m6_gate.摘要(计划), file=日志, flush=True)
        elif 轮分支 == "develop" or 轮分支.startswith("ci/兜底-"):
            # #1570：develop/兜底轮是**合入后**的备份验证——合入前的预验轮已原生
            #   跑过 M6（integrate 门禁），本平台不可原生时记「转派」并继续 E2E，
            #   不再让平台能力差异造红行；M6 义务由原生平台轮/预验承担。
            步骤们["M6平台保障"] = {"rc": 0, "类别": "转派",
                                   "说明": "转派原生平台：" + m6_gate.摘要(计划)}
            计划["执行"] = {"状态": "转派", "说明": "平台不可原生验——转派原生轮"}
            print("  [M6·转派] %s——develop 备份轮记转派（M6 由原生平台/预验承担）"
                  % m6_gate.摘要(计划), file=日志, flush=True)
        else:
            # 预验轮 fail-closed（唯一合法入 develop 通道）：不可原生平台不得给绿。
            步骤们["M6平台保障"] = {"rc": -1, "类别": "平台不可验",
                                   "说明": "平台不可验：" + m6_gate.摘要(计划) + "（fail-closed）"}
            print("  [M6·红] %s——fail-closed（预验轮不可原生平台不得给绿）"
                  % m6_gate.摘要(计划), file=日志, flush=True)
    保内存 = 内存保险丝或判红(步骤们, 日志)
    if 保内存 is None:
        # #1608：探测失败=E2E 不跑·明确红因（禁默认值继续）；M6 必跑轮另记证据缺失。
        if 须M6:
            步骤们["M6平台保障"] = {"rc": -1, "类别": "物理内存探测失败",
                                   "说明": "E2E 未跑——M6 执行证据缺失（#1608）"}
        return
    e2e基 = ci_shard.e2e命令基(cn, 保内存, 平台, 是win, 须M6=须M6)
    # #1221 复审 P2：命中清单随环境进 run_e2e 审计记录（子进程继承；轮后恢复）
    原命中 = os.environ.get("CN_M6_HITS")
    if 须M6:
        os.environ["CN_M6_HITS"] = ",".join(计划.get("命中") or [])
    try:
        if 分片 > 1:
            # ── 1297/1369 主实例分片形态：片 0=本机（重型独立面·run_e2e 分片语义）──
            #   1369：宽限窗内无任何从片认领→立即退化单实例序列（已完成从片照收·缺失
            #   片本机补跑·不空等）；收片窗独立，未齐片只报缺失=诚实红（门禁不放宽）。
            聚合rc, 聚合秒, 合并明细, _分片行们, _形态 = ci_shard.主实例分片段(
                轮分支=轮分支, 片数=分片, 召唤时刻=分片召唤时刻, e2e基=e2e基,
                明细参=ci_shard.e2e明细参, 步骤=步骤, 步骤们=步骤们, 日志=日志, sha=sha,
                日志目录=日志目录, e2e并行=e2e并行, e2e超时秒=e2e超时秒,
                runner=runner标识, 读明细=ci_shard.读e2e明细,
                片心跳=lambda k: 启动心跳(轮分支, k))
            步骤们["e2e并行"]["秒"] = 聚合秒
            # 1385 P1-1：分片形态透传（分布式=并行取 max；退化单实例=串行总和不混算）
            步骤们["e2e并行"]["分片形态"] = _形态
            步骤们["e2e"] = {"rc": 聚合rc}
            if 合并明细 is not None:
                try:
                    e2e明细路径.write_text(
                        json.dumps(合并明细, ensure_ascii=False, indent=1),
                        encoding="utf-8")
                except OSError as e:
                    print("  [警告] 聚合明细写回失败（%s）——门禁详情降级单片" % e,
                          file=日志, flush=True)
        elif 步骤("e2e并行", e2e基 + ci_shard.e2e明细参(e2e明细路径)
                  + ["--jobs", str(e2e并行)], e2e超时秒):
            步骤们["e2e"] = {"rc": 0}
        else:
            # 2026-10-16 用户令「红即停」（#902）：并行主轮一旦出现失败用例→立即中止该轮，
            #   不再跑串行复验（复验整轮对确定性缺陷必红=纯浪费时间）；447-a 口径废止。
            print("  [红即停] 并行主轮发现失败用例——中止该轮（用户令 2026-10-16·"
                  "#902·不再串行复验；红例见 e2e明细/日志）", file=日志, flush=True)
            步骤们["e2e"] = {"rc": 步骤们["e2e并行"]["rc"]}
    finally:
        if 原命中 is None:
            os.environ.pop("CN_M6_HITS", None)
        else:
            os.environ["CN_M6_HITS"] = 原命中
    # #1221：M6 执行记录（run_e2e --json 的「M6」键）回填进轮计划——md5/首个差异
    #   字节随轮结果上 8300；须M6 时**强制核验** `M6.状态=="通过"`（复审 P1-8）：
    #   缺失/非通过=「M6平台保障」rc=-1 拖红（旧版 run_e2e 无该键同样红·不放行）。
    明细 = ci_shard.读e2e明细(e2e明细路径)
    if isinstance(明细, dict) and isinstance(明细.get("M6"), dict):
        计划["执行"] = 明细["M6"]
    if 须M6:
        执行记录 = 计划.get("执行") or {}
        状态 = 执行记录.get("状态")
        if 状态 != "通过":
            md5段 = (" fix_p_md5=%s fix_s_md5=%s 首个差异字节=%s"
                     % (执行记录.get("fix_p_md5"), 执行记录.get("fix_s_md5"),
                        执行记录.get("首个差异字节"))) if 执行记录 else ""
            步骤们["M6平台保障"] = {
                "rc": -1,
                "说明": ("M6 执行证据缺失/非通过（状态=%r）——本轮红（#1221 复审 P1-8）%s"
                         % (状态, md5段))}
            print("  [M6·红] 明细无有效 M6 通过记录（状态=%r）——轮判红%s"
                  % (状态, md5段), file=日志, flush=True)
