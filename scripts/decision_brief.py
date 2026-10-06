# -*- coding: utf-8 -*-
"""decision_brief.py——裁决一页纸生成器（239·2026-10-07·诊断轮问题③「用户=唯一决策瓶颈」落地）。

扫 plans/021 全部挂起/待裁决面（⏸ 行+备注含 待裁决/待批/待用户/呈报 的活行），
按优先级排序输出结构化清单：stdout=人类可读一页纸；--json=AI 会话机读（消费后
生成大白话呈报：每件=问题+选项+推荐+可照抄裁决语）。

用法：
  python scripts/decision_brief.py            # 一页纸（每周或用户令「裁决会」时跑）
  python scripts/decision_brief.py --json     # 机读
  python scripts/decision_brief.py --账本 <路径>  # 指定总账文件（测试用·默认 plans/021*）

设计纪律（v5）：只读不写；零依赖纯标准库；不判断「该不该裁决」——那是 AI/用户的事。
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

本树根 = Path(__file__).resolve().parent.parent
挂因词 = ("待裁决", "待批", "待用户", "呈报", "选择题", "观察", "挂起")
优先级序 = {"P0": 0, "P1": 1, "P2": 2, "P3": 3}


def 解析行(行: str) -> dict | None:
    """021 任务行 → {号,标题,状态,前置,优先级,备注}；非任务行返回 None。"""
    m = re.match(r"^\|\s*(\d+[a-z]?)\s*\|\s*(.*?)\s*\|\s*(⬜|🏃|⏸|✅)\s*\|"
                 r"\s*(.*?)\s*\|\s*(P[0-3]?)\s*\|\s*(.*?)\s*\|\s*$", 行)
    if not m:
        return None
    return {"号": m.group(1), "标题": m.group(2), "状态": m.group(3),
            "前置": m.group(4), "优先级": m.group(5) or "P1", "备注": m.group(6)}


def 收集待裁决(账本: Path) -> list[dict]:
    """⏸ 行 ∪ 备注含挂因词的活行（⬜/🏃）——✅ 已收口不入。"""
    件们: list[dict] = []
    for 行 in 账本.read_text(encoding="utf-8").splitlines():
        件 = 解析行(行)
        if 件 is None:
            continue
        if 件["状态"] == "⏸":
            件["挂因"] = 件["备注"] or "挂起（备注未写因）"
            件们.append(件)
        elif 件["状态"] in ("⬜", "🏃") and any(w in 件["备注"] for w in 挂因词):
            句 = next((句 for 句 in re.split(r"[·;；]", 件["备注"]) if any(w in 句 for w in 挂因词)), "")
            件["挂因"] = 句.strip() or 件["备注"][:80]
            件们.append(件)
    件们.sort(key=lambda x: (优先级序.get(x["优先级"], 9), x["号"]))
    return 件们


def 一页纸(件们: list[dict], 账本名: str) -> str:
    行们 = [f"# 裁决一页纸（{len(件们)} 件待拍板·源={账本名}）", ""]
    if not 件们:
        行们 += ["（无挂起/待裁决事项——全线在跑）", ""]
        return "\n".join(行们)
    行们.append("| # | 优先级 | 状态 | 事项 | 挂因/待决点 |")
    行们.append("|---|--------|------|------|------------|")
    for 件 in 件们:
        标题 = 件["标题"][:60] + ("…" if len(件["标题"]) > 60 else "")
        挂因 = 件["挂因"][:100] + ("…" if len(件["挂因"]) > 100 else "")
        行们.append(f"| #{件['号']} | {件['优先级']} | {件['状态']} | {标题} | {挂因} |")
    行们 += ["",
             "> 裁决方式：逐件批「按推荐/选甲/选乙/挂起」即可；P3 与机制级小事按 AGENTS §3",
             "> 分级授权 AI 自行裁决留痕，无需占用本页。改 001 规范/安全语义必呈用户。"]
    return "\n".join(行们)


def 主流程() -> int:
    解析 = argparse.ArgumentParser(description="裁决一页纸生成器（239）")
    解析.add_argument("--json", action="store_true", help="机读输出（AI 会话消费）")
    解析.add_argument("--账本", default=None, help="指定总账路径（测试用·默认 plans/021*）")
    参数 = 解析.parse_args()
    if 参数.账本:
        账本 = Path(参数.账本)
    else:
        们 = sorted((本树根 / "plans").glob("021*.md"))
        if not 们:
            print("[失败] 找不到 plans/021 总账", file=sys.stderr)
            return 1
        账本 = 们[0]
    if not 账本.exists():
        print(f"[失败] 总账不存在：{账本}", file=sys.stderr)
        return 2
    件们 = 收集待裁决(账本)
    if 参数.json:
        print(json.dumps(件们, ensure_ascii=False, indent=2))
    else:
        print(一页纸(件们, 账本.name))
    return 0


if __name__ == "__main__":
    sys.exit(主流程())
