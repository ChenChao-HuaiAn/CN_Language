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


def 轮写集(轮分支: str, sha: str, 仓库根: Path,
           远端: str = "origin", 基线查=None,
           运行=None) -> tuple[list[str] | None, str]:
    """本轮写集（改动文件清单）；None=不可判（调用方 fail-safe 触发）。

    基线选择：
      预验轮（ci/预验-*）：<远端>/develop...<sha>——链顶=在最新 develop 上叠的
        任务提交集；中文路径用 core.quotepath=false（586-a 同族：路径转义会令
        前缀判定静默失效）。
      develop/兜底轮：上次已验绿 sha..本轮 sha（latest.json）；缺失/等于本轮
        → sha~1；仍失败 → 不可判。
    返回 (文件们|None, 原因)。运行=可注入 subprocess.run（selftest 零 git）。
    """
    跑 = 运行 or (lambda 命令: subprocess.run(
        命令, capture_output=True, text=True, cwd=仓库根, timeout=120))
    基线 = ""
    原因 = ""
    if 轮分支.startswith(预验分支前缀):
        基线 = "%s/develop" % 远端
    else:
        基线 = (基线查() if 基线查 is not None else ci_fallback.上次已验SHA()) or ""
        if not 基线 or 基线 == sha:
            基线 = "%s~1" % sha
    if not 基线:
        return None, "基线缺失（无法判定本轮改动面）"
    命令 = ["git", "-c", "core.quotepath=false", "diff", "--name-only",
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


def e2e段(步骤们: dict, 步骤, cn: Path, 日志, sha: str, 轮分支: str, 分片: int,
          分片召唤时刻, 计划: dict, *, 是win: bool, 平台: str, 日志目录: Path,
          e2e并行: int, e2e超时秒: int, runner标识: str, 启动心跳,
          e2e明细路径: Path) -> None:
    """④ E2E 段（原 ci_daemon 内联逐行搬移 + M6 门禁三态）。

    M6 三态（计划来自 轮计划）：
      必跑  → e2e 命令带 --require-m6（选集含 79_v2 而未跑/未通过=exit 1）
      转派  → 预验轮置「M6平台保障」rc=-1（该轮红·不得静默进 develop）；
              develop/兜底轮 rc=0 且原因显式（由平台轮转的原生轮承担）
      未触发→ 不写 M6 步骤（零成本）
    分片：片 0 恒含 79_v2（#1420 重者留主）——主实例 e2e基 带 require，从片不带。
    """
    触发 = bool((计划 or {}).get("触发"))
    可跑 = bool((计划 or {}).get("平台可跑"))
    须M6 = 触发 and 可跑
    if 触发:
        if 可跑:
            步骤们["M6平台保障"] = {"rc": 0, "说明": "M6 必跑（--require-m6·#1221）"}
            print("  [M6] %s——e2e 带 --require-m6（79_v2 跳过/失败=本轮红）"
                  % m6_gate.摘要(计划), file=日志, flush=True)
        elif 轮分支.startswith(预验分支前缀):
            步骤们["M6平台保障"] = {"rc": -1, "说明": m6_gate.摘要(计划) + "（预验判红）"}
            print("  [M6·红] %s——预验轮 fail-closed（该平台无法原生验证固定点，"
                  "改由 win/linux-x64/arm64 真机实例重验）" % m6_gate.摘要(计划),
                  file=日志, flush=True)
        else:
            步骤们["M6平台保障"] = {"rc": 0, "说明": m6_gate.摘要(计划)}
            print("  [M6·转派] %s——develop 轮显式记录，由平台轮转的原生轮承担"
                  % m6_gate.摘要(计划), file=日志, flush=True)
    保内存 = min(4096, int(ci_report.物理内存MB(是win) * 0.8))
    e2e基 = ci_shard.e2e命令基(cn, 保内存, 平台, 是win, 须M6=须M6)
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
    # #1221：M6 执行记录（run_e2e --json 的「M6」键）回填进轮计划——md5/首个差异
    #   字节随轮结果上 8300；缺键=旧版 run_e2e/未跑（计划里的处置与原因已显式）。
    明细 = ci_shard.读e2e明细(e2e明细路径)
    if isinstance(明细, dict) and isinstance(明细.get("M6"), dict):
        计划["执行"] = 明细["M6"]
