# -*- coding: utf-8 -*-
"""standup.py——开机一条命令（241·2026-10-07·诊断轮问题⑤「AI 开机成本高」落地）。

一键开工简报：fetch+落后提示 → 看板就绪摘要 → 交接本机节最近条目 → 教训高权重标题
→ 待裁决件数。全部子进程/文本切片复用现成设施，零重写。

用法：python scripts/standup.py [--机 深度机]   # --机 缺省按 hostname 推测
"""
from __future__ import annotations

import argparse
import json
import socket
import subprocess
import sys
from pathlib import Path

本树根 = Path(__file__).resolve().parent.parent
主机别名 = {"deepin": "深度机", "CHENCHAO-W": "家机", "arm64": "单位机", "user-pc": "单位机"}  # hostname 前缀→交接节关键词（296：user-pc=单位机实机·原兜底误标深度机）


def 跑(命令: list[str], cwd: Path | None = None) -> str:
    r = subprocess.run(命令, capture_output=True, text=True, encoding="utf-8",
                       errors="replace", cwd=cwd or 本树根)
    return r.stdout + (r.stderr if r.returncode else "")


def 落后提示() -> list[str]:
    行们: list[str] = []
    r = subprocess.run(["git", "fetch", "gitcode"], capture_output=True, text=True, cwd=本树根)
    if r.returncode != 0:
        return ["⚠️ git fetch 失败（网络/远端异常）——先解决再开工"]
    本地 = 跑(["git", "rev-parse", "refs/heads/develop"]).strip()
    远端 = 跑(["git", "rev-parse", "gitcode/develop"]).strip()
    脏 = 跑(["git", "status", "--porcelain"]).strip()
    if 本地 != 远端:
        数 = len(跑(["git", "log", "--oneline", f"HEAD..gitcode/develop"]).strip().splitlines())
        行们.append(f"📌 develop 落后远端 {数} 提交——先 git merge --ff-only gitcode/develop 对齐（fetch≠对齐）")
    if 脏:
        行们.append(f"📌 工作树不干净（{len(脏.splitlines())} 项）——涉共享文档的写集请走 worktree")
    if not 行们:
        行们.append("✓ develop 已对齐·工作树干净")
    return 行们


def 本机节(机: str) -> list[str]:
    文 = (本树根 / "交接.md").read_text(encoding="utf-8")
    节们: dict[str, list[str]] = {}
    当前 = None
    for 行 in 文.splitlines():
        if 行.startswith("## "):
            当前 = 行[3:].strip()
            节们[当前] = []
        elif 当前:
            节们[当前].append(行)
    目标 = next((k for k in 节们 if 机 in k), None)
    if not 目标:
        return [f"（交接.md 无「{机}」节）"]
    条 = [l for l in 节们[目标] if l.strip().lstrip("- ").startswith("**")]
    行们 = [f"## 交接·{目标}（最近 {min(2, len(条))} 条）"]
    行们 += [l.strip() for l in 条[-2:]]
    return 行们


def 教训高权重(上限: int = 10) -> list[str]:
    文 = (本树根 / "项目记忆" / "教训.md").read_text(encoding="utf-8")
    起始 = 文.find("高权重全文区")
    段 = 文[起始:] if 起始 >= 0 else 文
    标题们 = [l.lstrip("# ").strip() for l in 段.splitlines()
              if l.startswith("## ") and "高权重全文区" not in l]
    行们 = [f"## 教训·高权重标题（前 {min(上限, len(标题们))} 条·全文=项目记忆/教训.md §一）"]
    行们 += [f"- {t[:88]}{'…' if len(t) > 88 else ''}" for t in 标题们[:上限]]
    return 行们


def 看板与裁决() -> list[str]:
    行们 = ["## 看板（task_board --ready 摘要）"]
    就绪 = 跑([sys.executable, "scripts/task_board.py", "--ready"])
    行们 += 就绪.strip().splitlines()[:14]
    try:
        件 = json.loads(跑([sys.executable, "scripts/decision_brief.py", "--json"]))
        行们.append(f"## 待裁决 {len(件)} 件（python scripts/decision_brief.py 看一页纸）")
    except Exception:
        行们.append("## 待裁决：decision_brief 输出解析失败（脚本异常？）")
    return 行们


def 主流程() -> int:
    解析 = argparse.ArgumentParser(description="开机一条命令（241）")
    解析.add_argument("--机", default=None, help="本机名（缺省按 hostname 推测）")
    参数 = 解析.parse_args()
    机 = 参数.机 or next((v for k, v in 主机别名.items()
                          if socket.gethostname().lower().startswith(k.lower())), "深度机")
    输出 = ["═" * 46, f" 开工简报·{机}·{socket.gethostname()}", "═" * 46]
    输出 += 落后提示()
    输出 += 看板与裁决()
    输出 += 本机节(机)
    输出 += 教训高权重()
    print("\n".join(输出))
    return 0


if __name__ == "__main__":
    sys.exit(主流程())
