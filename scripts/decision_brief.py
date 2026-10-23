# -*- coding: utf-8 -*-
"""decision_brief.py——裁决一页纸生成器（239·2026-10-07·诊断轮问题③「用户=唯一决策瓶颈」落地）。

扫任务台账全部挂起/待裁决面（⏸ 任务+备注含 待裁决/待批/待用户/呈报 的活任务），
按优先级排序输出结构化清单：stdout=人类可读一页纸；--json=AI 会话机读
（{"来源": 服务端台账/本地缓存原因, "件们": [...]}——消费后生成大白话呈报：
每件=问题+选项+推荐+可照抄裁决语）。

384：数据源从 plans/021 文件改为看板服务端任务表（382 起台账唯一权威在服务端，
021 已冻结退位——本脚本当时漏切仍读冻结旧账=数据过期，本批收口）。服务不可达
降级读 board_cache.json 最后快照（黄字提示），不静默装新。

用法：
  python scripts/decision_brief.py            # 一页纸（每周或用户令「裁决会」时跑）
  python scripts/decision_brief.py --json     # 机读

设计纪律（v5）：只读不写；零依赖纯标准库；不判断「该不该裁决」——那是 AI/用户的事。
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sys
import urllib.error
import urllib.request
from pathlib import Path

import atomic_json

本树根 = Path(__file__).resolve().parent.parent
挂因词 = ("待裁决", "待批", "待用户", "呈报", "选择题", "观察", "挂起")
优先级序 = {"P0": 0, "P1": 1, "P2": 2, "P3": 3}
# 556：认 CN_BOARD_URL（与 standup/board_cli/task_board/wt 同覆盖口·原硬编码与其不一致）
看板地址 = os.environ.get("CN_BOARD_URL", "http://124.222.106.84:8301").rstrip("/")


def 拉任务们() -> tuple[list[dict], str]:
    """服务端任务全量 → (任务们, 来源描述)。降级读缓存。
    556：缓存读写统一走 atomic_json（原子写·读容错——原裸 write_text/read_text
    与 standup/board_cli 三写者并发可半写损坏·坏缓存裸 json.loads 炸读侧）。"""
    缓 = 本树根 / "scripts" / "board_cache.json"

    def 缓存任务们() -> list:
        旧 = atomic_json.读(缓)
        return 旧.get("tasks", []) if isinstance(旧, dict) else []

    try:
        请求 = urllib.request.Request(f"{看板地址}/api/tasks")
        try:
            import intent
            intent.加鉴权头(请求)   # 1425/P2-A：Bearer 不可重定向
        except Exception as e:
            print(f"[黄] intent 鉴权助手不可用，本次裸请求（诊断见上）：{e}", file=sys.stderr)
        with urllib.request.urlopen(请求, timeout=8) as r:
            数据 = json.loads(r.read().decode("utf-8"))
        if not isinstance(数据, dict):
            raise ValueError(f"看板任务响应非对象（{type(数据).__name__}）")
        if "任务们" not in 数据:
            raise ValueError("看板任务响应缺「任务们」字段（服务端版本不符或响应损坏）")
        任务们 = 数据["任务们"]
        if not isinstance(任务们, list):
            raise ValueError(f"看板任务「任务们」类型异常（{type(任务们).__name__}）")
        atomic_json.合并写(缓, {"tasks": 任务们})
        return 任务们, "服务端台账"
    except urllib.error.HTTPError as e:
        # 1425：401/403=鉴权失败（令牌/登录墙），不得冒充「服务不可达」。
        鉴权失败 = e.code in (401, 403)   # P3-D：本地兜底
        try:
            import intent
            鉴权失败 = intent.鉴权失败码(e.code)
            原因 = (intent.鉴权失败文案("看板服务", e.code) if 鉴权失败
                    else f"看板服务返回 HTTP {e.code}")
        except Exception as ex:
            print(f"[黄] intent 鉴权助手不可用，按裸码兜底（诊断见上）：{ex}", file=sys.stderr)
            原因 = (f"看板服务鉴权失败（{e.code}）——令牌校验失败（详见上方诊断）"
                    if 鉴权失败 else f"看板服务返回 HTTP {e.code}")
        任务们 = 缓存任务们()
        if 任务们:
            return 任务们, f"本地缓存（{原因}·可能是旧快照）"
        print(f"[失败] {原因}且无缓存——裁决面暂不可知（恢复后重跑）", file=sys.stderr)
    except OSError as e:
        任务们 = 缓存任务们()
        if 任务们:
            return 任务们, f"本地缓存（看板服务不可达：{e}·可能是旧快照）"
        print(f"[失败] 看板服务不可达且无缓存（{e}）——裁决面暂不可知（恢复后重跑）",
              file=sys.stderr)
    except ValueError as e:
        # P3-B/P3-C：缺字段/非 dict/坏 JSON=响应异常，不得冒充连接层不可达。
        原因 = f"看板任务响应异常（{e}）"
        任务们 = 缓存任务们()
        if 任务们:
            return 任务们, f"本地缓存（{原因}·可能是旧快照）"
        print(f"[失败] {原因}且无缓存——裁决面暂不可知（修复服务后重跑）",
              file=sys.stderr)
    return [], ""


def 收集待裁决(任务们: list[dict]) -> list[dict]:
    """⏸ 任务 ∪ 备注含挂因词的活任务（⬜/🏃）——✅ 已收口不入。"""
    件们: list[dict] = []
    for t in 任务们:
        件 = {"号": t["号"], "标题": t["标题"], "状态": t["状态"],
              "优先级": t["优先级"], "备注": t.get("备注", "")}
        if 件["状态"] == "⏸":
            件["挂因"] = 件["备注"] or "挂起（备注未写因）"
            件们.append(件)
        elif 件["状态"] in ("⬜", "🏃") and any(w in 件["备注"] for w in 挂因词):
            句 = next((句 for 句 in re.split(r"[·;；]", 件["备注"]) if any(w in 句 for w in 挂因词)), "")
            件["挂因"] = 句.strip() or 件["备注"][:80]
            件们.append(件)
    件们.sort(key=lambda x: (优先级序.get(x["优先级"], 9), x["号"]))
    return 件们


def 一页纸(件们: list[dict], 来源: str) -> str:
    行们 = [f"# 裁决一页纸（{len(件们)} 件待拍板·源={来源}）", ""]
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
    参数 = 解析.parse_args()
    任务们, 来源 = 拉任务们()
    if not 来源:
        return 1
    件们 = 收集待裁决(任务们)
    if 参数.json:
        # 1485：增「来源」字段——机读消费方可分辨服务端台账/本地缓存（鉴权失败真因随源）。
        print(json.dumps({"来源": 来源, "件们": 件们}, ensure_ascii=False, indent=2))
    else:
        print(一页纸(件们, 来源))
    return 0


if __name__ == "__main__":
    sys.exit(主流程())
