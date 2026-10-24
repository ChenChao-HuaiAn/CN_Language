#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""云端轮结果组装与直跑收口（#1221 复审 P2 自 ci_daemon 外迁·守 1000 行红线）。

三个纯组装职责（原 ci_daemon 内联，逐行等价搬移 + M6 字段）：
  轮早退结果   —— 构建后 develop 头已前进：绿闭环防重派、不落盘防 latest 污染；
                  M6 未执行原因显式（新头轮承担）。
  轮结果构造   —— 日志尾部/执行机归因/e2e 明细/M6 计划与执行记录/提交历史。
  直跑过期因 + 直跑收口 —— #1221 P1-10：直跑（非池）路径早退/HEAD≠任务 sha
                  一律不落盘（防 latest 被非任务 sha 污染）；落盘函数由调用方注入。
  预验分支sha10 —— ci/预验-<sha10>[-m6<attempt>] 解析（#1221 P0-1 非幂等后缀）。

依赖方向：ci_daemon → 本模块 → {ci_report, ci_shard}（无环·本模块禁 import ci_daemon）。
"""
from __future__ import annotations

import subprocess
import time
from datetime import datetime
from pathlib import Path

import ci_report
import ci_shard


def 预验分支sha10(轮分支: str) -> str:
    """ci/预验-<sha10>[-m6<attempt>] → sha10（旧分支恰 10 位；M6 触发轮前 10 位不变）。"""
    尾 = 轮分支[len("ci/预验-"):] if 轮分支.startswith("ci/预验-") else ""
    return 尾[:10] if len(尾) >= 10 else ""


def 轮早退结果(sha: str, 轮分支: str, 步骤们: dict, 开始: float,
              轮日志路径: Path, 现头: str, m6计划: dict, *, 平台: str) -> dict:
    """过期早退结果：绿闭环防重派、不落盘防 latest 污染；M6 未执行原因显式。"""
    return {"sha": sha, "分支": 轮分支, "平台": 平台, "步骤": 步骤们,
            "绿": True, "总秒": round(time.time() - 开始),
            "时刻": datetime.now().isoformat(timespec="seconds"),
            "日志": 轮日志路径.name, "日志尾部": [], "早退新头": 现头,
            "M6": dict(m6计划, 处置="过期早退",
                       说明="develop 已前进——本轮未执行 M6（新头轮承担）")}


def 轮结果构造(sha: str, 轮分支: str, 步骤们: dict, 开始: float,
              轮日志路径: Path, m6计划: dict, *, 平台: str, 日志目录: Path,
              runner标识: str, 构建并行: int, 是win: bool, 聚合绿) -> dict:
    """跑一轮 尾部结果组装（日志尾部/归因/e2e 明细/M6 计划/提交历史）。"""
    with 轮日志路径.open("r", encoding="utf-8", errors="replace") as f:
        尾部 = f.readlines()[-40:]
    return {"sha": sha, "分支": 轮分支, "平台": 平台, "步骤": 步骤们,
            # 1299：执行机归因字段（win 不写不适用项）
            **ci_report.执行机字段(runner标识, 构建并行, 是win),
            "绿": 聚合绿(步骤们),
            "总秒": round(time.time() - 开始),
            "时刻": datetime.now().isoformat(timespec="seconds"),
            "日志": 轮日志路径.name, "日志尾部": "".join(尾部).splitlines()[-20:],
            # 842：失败详情与归因数据（e2e 未跑/旧版=null 降级）
            "e2e明细": ci_shard.读e2e明细(日志目录 / ("e2e明细_%s.json" % sha[:10])),
            # #1221：M6 计划+执行记录（md5/首个差异字节）——触发/未跑原因显式
            "M6": m6计划,
            "提交历史": ci_report.取提交历史(),
            # 1070：提交时刻（门禁排序键）
            "提交时刻": ci_report.提交时刻(sha)}


def ls远程失败红结果(分支: str, 平台: str, sha10: str) -> dict:
    """#1221 复审 P2-1：点名预验 ls-remote 失败时的合成红结果（按分支内嵌 sha10）。

    落盘后文件名=预验_<sha10>.json，ssh 轮询 60s 内收口（否则守满 240 分钟总超时）。
    真实完整 sha 不可知——用 sha10+补零占位（仅作文件名/审计键，不参与任何验证结论）。"""
    合成sha = (sha10 + "0" * 40)[:40]
    return {"sha": 合成sha, "分支": 分支, "平台": 平台, "绿": False,
            "步骤": {"同步": {"rc": -1, "说明": "ls-remote 失败（无法解析分支头）"}},
            "总秒": 0, "时刻": datetime.now().isoformat(timespec="seconds"),
            "日志": "", "日志尾部": ["[ls-remote 失败] 按分支内嵌 sha10 合成红结果"
                                 "供 ssh 轮询快速收口"]}


def 轮M6不可验结果(sha: str, 轮分支: str, 步骤们: dict, 开始: float,
                  轮日志路径: Path, m6计划: dict, *, 平台: str) -> dict:
    """#1221 复审 P1-4①：M6 触发但平台不可原生验——**构建前**早退判红。

    交叉模拟 arm64 等平台无法原生跑 79_v2：原先要跑完整 E2E 才转红（白烧 60~80%
    算力）。本结果不落 latest（红轮规则），步骤标「平台不可验」与代码红分流
    （红治理按类别归因，不与编译器缺陷混为一谈）。"""
    步骤们["M6平台保障"] = {
        "rc": -1, "类别": "平台不可验",
        "说明": "平台不可验：" + str(m6计划.get("说明") or "M6 无法原生运行")}
    尾部 = ["[M6·平台不可验] %s——构建前早退（不跑 E2E；原生平台轮承担）"
            % m6计划.get("说明", "")]
    return {"sha": sha, "分支": 轮分支, "平台": 平台, "步骤": 步骤们,
            "绿": False, "总秒": round(time.time() - 开始),
            "时刻": datetime.now().isoformat(timespec="seconds"),
            "日志": 轮日志路径.name, "日志尾部": 尾部,
            "M6": dict(m6计划, 处置="平台不可验")}


def 直跑过期因(sha: str, 结果: dict, 头查=None, 仓库根: Path | None = None,
              轮分支: str = "develop") -> str:
    """直跑轮过期判定；空串=未过期。

    #1221 复审 P1-2：
      · develop 轮：早退新头 或 跑后 HEAD≠任务 sha → 过期（不落盘防 latest 污染）；
      · 预验/点名轮：**不按 HEAD 判**——同步失败（分支删/网络抖）时 HEAD 是旧树头，
        若按过期不落盘，ssh 轮询要等满 240 分钟；预验轮只认「早退新头」标记，同步
        失败的高绿=False 结果照常落盘→ ssh 5 分钟内拿到红结论。
      · git rev-parse 空输出（失败）=不可判 → 按过期收口（不落盘+显式提示），
        不得当「未过期」fail-open。
    头查=可注入（selftest 零 git）；仓库根=git 工作目录。"""
    if 结果.get("早退新头"):
        return "构建后早退（develop 已前进 %s）" % str(结果["早退新头"])[:10]
    if (轮分支 or "develop") != "develop":
        return ""
    if 头查 is not None:
        头 = 头查()
    else:
        头 = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True,
                          text=True, encoding="utf-8", errors="replace",
                          timeout=30, cwd=仓库根).stdout.strip()
    if not 头:
        return "HEAD 不可判（git rev-parse 无输出）——按过期收口（不落盘·防 latest 污染）"
    if 头 != sha:
        return "跑后复查（HEAD 已前进 %s）" % 头[:10]
    return ""


def 直跑收口(sha: str, 结果: dict, 落盘函数, 仓库根: Path | None = None,
            头查=None, 轮分支: str = "develop") -> None:
    """直跑轮收口：过期不落盘（新头轮承担），否则落盘并打印完成行。
    #1221 复审 P1-2：轮分支=预验/点名分支时不做 HEAD≠sha 判定（同步失败也要落盘
    红结果，ssh 轮询快速失败）。"""
    过期因 = 直跑过期因(sha, 结果, 头查=头查, 仓库根=仓库根, 轮分支=轮分支)
    if 过期因:
        print("[过期] %s——本轮不落盘（防 latest 污染；新头轮承担）" % 过期因, flush=True)
        return
    落盘函数(结果)
    print("[完成] 绿=%s 总秒=%s 详情=%s"
          % (结果["绿"], 结果["总秒"], 结果["日志"]), flush=True)
