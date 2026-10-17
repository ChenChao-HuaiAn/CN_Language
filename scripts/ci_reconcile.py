#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ci_reconcile.py —— 对账周检段（973）：两对账工具的周期化挂载（治「想起来才对账」）。

被挂载工具（均已入库·报告式默认 rc=0·--strict 才判红）：
  ① scripts/neg_expect_audit.py --strict      孤儿负例>0 → rc=1（题库→001 法条/诊断源）
  ② scripts/check_diag_negtest.py --diff <基线> --strict   增量诊断锚缺口 → rc=1

挂载方式（对齐 ci_daemon 静态门禁面既有模式·不自创机制）：
  · 运行面=ci_daemon 兜底轮/develop 面轮（轮分支 == "develop"——池兜底任务与 --force
    每日 cron 直跑均落 develop）静态段之后；预验轮（ci/预验-*）与点名分支不跑。
  · 周期=默认 7 天（CN_AUDIT_PERIOD_DAYS 环境变量可覆盖·运维调节面）；到期才跑。
    状态文件 ci-logs/对账周检.json 记录上次对账时刻/sha——周检=银行对账节拍。
  · 增量基线（check_diag_negtest --diff）=状态记录的上次对账 sha；首跑（无状态）取
    「周期天前」提交（git rev-list --before）为窗（首跑即覆盖最近一周并入面）。
  · 红播报=步骤 rc 走既有 步骤()/结果 JSON/上报 链路显形（结果页「红步骤：负测期望
    对账(rc=1)」）+工具报告全文入轮日志（含孤儿/缺口清单与处置指引）；日志另打
    `[973 对账周检][红]` 行便于 grep。**不拖轮绿**——对齐静态门禁面既有口径
    （check_spec_coverage 等红步骤同样不拦兜底轮·243 存量红先例）：对账红多为存量/
    历史面（无「本 sha 引入者」），拖轮红会污染 develop 红灯归因「自上次绿以来」
    语义（842）与红治理「立新号返工」流程。
  · 检查点推进=两工具**跑完**（rc∈{0,1}——红=有结论）即推进（时刻+sha+结果留档）；
    超时（-99）/用法错（2）/基线不可得（-1）不推进=下轮 develop 面重试。红结论经
    步骤/日志播报后由处置流程承担（工具 docstring 口径：孤儿→补法条或修期望文案；
    缺口→补负测锚或现场补证据·人裁决留痕·提及即立项），不做「未清账轮轮重跑」
    ——neg 全库口径孤儿在库即每周再红（天然持续），diag 增量口径=本周期并入面结论。

诚实边界（728n/841 同规·不假装全覆盖）：文本层对账 ≠ 语义证明；孤儿/缺口=机械筛选
面（低误报取向）；确属实现级拒绝/内部错误探针的处置由人裁决留痕；v2 IO::打印到错误
通道混进度日志不作文本语义分类——细则随各工具 docstring。

成本：两工具合计 ~5s 热缓存（neg_expect_audit 全库 ~4s〔294 负例×万字级 001 文本〕+
check_diag_negtest 4-gram 倒排 ~1s；首跑含覆盖矩阵拉取 ~20s——本树实测）；相对整轮
（构建+单测+E2E 30~90 分钟）可忽略；绿时每周期一次。

自测：ci_daemon_selftest.py 973 断言段（python3 scripts/ci_daemon.py --selftest）。
"""
from __future__ import annotations

import json
import os
import subprocess
import sys
import time
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent
日志目录 = 仓库根 / "ci-logs"

周期天默认 = 7.0                       # CN_AUDIT_PERIOD_DAYS 覆盖（仅调度节拍·不承载判据）
状态文件名 = "对账周检.json"
步骤超时秒 = 900                       # 与 daemon 静态门禁面同款（实测两工具 <30s·余量充足）


def 周期天() -> float:
    """周检周期（天）：CN_AUDIT_PERIOD_DAYS 覆盖·缺省 7；非法/非正回缺省（显式声明：
    该值只决定「多久对一次账」的节拍，不承载对账结论——判红与否由工具 rc 定）。"""
    try:
        值 = float(os.environ.get("CN_AUDIT_PERIOD_DAYS") or 周期天默认)
    except ValueError:
        return 周期天默认
    return 值 if 值 > 0 else 周期天默认


def 读状态(状态文件: Path) -> dict | None:
    """读对账状态；缺/损坏 → None（调用方按「到期」保守处置——fail-safe 方向=多对账
    不少对账；不携带静默放行面）。"""
    try:
        状 = json.loads(状态文件.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None
    return 状 if isinstance(状, dict) else None


def 对账周期到期(状态文件: Path, 周期天数: float, 现在: float | None = None) -> tuple[bool, str]:
    """周期到期判定（纯读·路径/周期/时刻全可注入——自测直调）：
    无状态/损坏/缺时刻字段 → 到期（冷启动保守跑）；距上次对账 ≥ 周期天数 → 到期。
    返回 (是否到期, 缘由)。"""
    现在 = time.time() if 现在 is None else 现在
    状 = 读状态(状态文件)
    if 状 is None:
        return True, "无对账检查点（冷启动/损坏）→到期"
    try:
        上次 = float(状["上次对账时刻"])
    except (KeyError, TypeError, ValueError):
        return True, "检查点缺「上次对账时刻」字段→到期（保守）"
    已过天 = (现在 - 上次) / 86400.0
    if 已过天 >= 周期天数:
        return True, "距上次对账 %.1f 天 ≥ %.1f 天→到期" % (已过天, 周期天数)
    return False, "距上次对账 %.1f 天 < %.1f 天→本轮不对账" % (已过天, 周期天数)


def 取对账基线(状态: dict | None, 头sha: str, 周期天数: float) -> tuple[str, str]:
    """增量对账（check_diag_negtest --diff）基线解析 → (基线 sha, 缘由)；""=基线不可得
    （调用方显式留档跳过——非静默）。① 状态记录的上次对账 sha（先 rev-parse 验可见
    ——防历史裁剪/他树镜像）；② 首跑（无状态）取「周期天前」提交（周窗语义）。"""
    sha = str((状态 or {}).get("上次对账sha") or "").strip()
    if sha:
        验 = subprocess.run(["git", "rev-parse", "--verify", "--quiet", sha + "^{commit}"],
                            cwd=str(仓库根), capture_output=True, text=True)
        if 验.returncode == 0 and 验.stdout.strip():
            return 验.stdout.strip(), "上次对账检查点 %s（本周期并入面）" % sha[:10]
    周窗 = subprocess.run(["git", "rev-list", "-1", "--before=%s days ago" % 周期天数, 头sha],
                         cwd=str(仓库根), capture_output=True, text=True)
    if 周窗.returncode == 0 and 周窗.stdout.strip():
        return 周窗.stdout.strip(), "首跑周窗：%s 天前提交" % 周期天数
    return "", "基线不可得（git 异常/历史不足）"


def 写状态(状态文件: Path, sha: str, 对账结果: dict) -> None:
    """推进检查点（时刻+sha+上轮结果·审计可见）。普通整写即可：单 runner 单写者无
    并发面；撕裂写下次即判「无状态/损坏」→到期重跑（fail-safe 方向安全）。"""
    状态文件.parent.mkdir(parents=True, exist_ok=True)
    状态文件.write_text(json.dumps(
        {"上次对账时刻": time.time(), "上次对账sha": sha, "上次结果": 对账结果,
         "工具": "scripts/ci_reconcile.py（973 对账周检段）"},
        ensure_ascii=False, indent=1), encoding="utf-8")


def 对账周检段(步骤们: dict, 步骤, 日志, 轮分支: str, sha: str,
               状态文件: Path | None = None, 周期天数: float | None = None) -> None:
    """兜底轮周期对账段（ci_daemon.跑一轮 在静态门禁面之后调用·本函数自判分支/周期）。

    对齐既有模式：与静态门禁面同款薄段——就地跑两工具、步骤名进 结果 JSON（红步骤
    =播报通道）、报告全文入轮日志；不拖轮绿与检查点口径见模块头 docstring。"""
    if 轮分支 != "develop":
        return                       # 设计面：仅 develop 面轮（兜底/直跑）——预验/点名不跑
    状态文件 = 状态文件 or (日志目录 / 状态文件名)
    周期天数 = 周期天() if 周期天数 is None else 周期天数
    到期, 缘由 = 对账周期到期(状态文件, 周期天数)
    if not 到期:
        print("  [973 对账周检] %s" % 缘由, file=日志, flush=True)
        return
    print("  [973 对账周检] %s——跑两对账工具 --strict（报告全文随本步骤入轮日志）" % 缘由,
          file=日志, flush=True)
    基线, 基线缘由 = 取对账基线(读状态(状态文件), sha, 周期天数)
    print("  [973 对账周检] 增量基线：%s" % 基线缘由, file=日志, flush=True)
    # ① 题库→法条/诊断源（全库·无基线）；② 诊断→负测锚（增量·基线=检查点/首跑周窗）。
    #    两工具相互独立：第一红不中断第二（两账都要结论）。
    步骤("负测期望对账", [sys.executable, "scripts/neg_expect_audit.py", "--strict"], 步骤超时秒)
    if 基线:
        步骤("诊断锚对账", [sys.executable, "scripts/check_diag_negtest.py",
                          "--diff", 基线, "--strict"], 步骤超时秒)
    else:
        # 显式留档跳过（rc=-1 进结果=显形·非静默）——不推进检查点，下轮重试
        步骤们["诊断锚对账"] = {"rc": -1, "说明": "跳过：%s" % 基线缘由}
    对账结果 = {名: 步骤们.get(名, {}).get("rc") for 名 in ("负测期望对账", "诊断锚对账")}
    全跑完 = all(rc in (0, 1) for rc in 对账结果.values())
    if 全跑完:
        写状态(状态文件, sha, 对账结果)
    else:
        异常账 = ["%s=%s" % (名, rc) for 名, rc in 对账结果.items() if rc not in (0, 1)]
        print("  [973 对账周检][异常] 工具未跑完（%s）——检查点不推进·下轮 develop 面重试"
              % "、".join(异常账), file=日志, flush=True)
    红账 = [名 for 名, rc in 对账结果.items() if rc == 1]
    if 红账:
        print("  [973 对账周检][红] %s——处置：补 001 法条/负测锚或人裁决留痕（提及即立项；"
              "边界随工具 docstring）" % "、".join(红账), file=日志, flush=True)
    elif 全跑完:
        print("  [973 对账周检] 清账：两账全绿（检查点推进 sha=%s）" % sha[:10], file=日志, flush=True)
