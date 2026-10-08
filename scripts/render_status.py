#!/usr/bin/env python3
# 收工文档渲染器（924 治理二期·「单一事实源+自动渲染」首版）：
#   源=plans/025 本机节最新小节（六要素+收工回填——收工时唯一手写处）+ git log 本分支提交；
#   渲染=①更新日志.md 本机节条目 ②交接.md 本机节草稿——默认打印供核对，--write 直接写入。
#   治「同一事实写 5 处」的收工文档税（920 诊断）：手写 5 处→1 处（025）+2 处渲染+2 处维持手写
#   （看板窄通道行=即改即推语义；021 台账=结构复杂高风险·首版不动·后续演进）。
# 用法：python scripts/render_status.py [--machine 家机] [--write] [--dry-git]
# 渲染区豁免 check_handoff 的深度校验前提：写入后必跑 check_handoff 复核（本脚本不自检）。

# 兼容 python3.8：注解泛型下标/联合字符串化（287·同 gate_quick 修复缘由）。
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent
# （025 节头〔带编号〕， 交接/更新日志节头〔无编号〕， 小节前缀）
机节映射 = {"家机": ("## 一、家机", "## 家机 win-x64 节", "### 1."),
             "深度机": ("## 二、深度机", "## 深度机 linux-x86_64 节", "### 2."),
             "单位机": ("## 三、单位机", "## 单位机 ARM64 节", "### 3.")}


def 取分支提交们() -> list[str]:
    基准 = subprocess.run(["git", "merge-base", "HEAD", "gitcode/develop"],
                          capture_output=True, text=True, cwd=仓库根).stdout.strip()
    if not 基准:
        return []
    出 = subprocess.run(["git", "log", "--oneline", "--reverse", f"{基准}..HEAD"],
                        capture_output=True, text=True, cwd=仓库根).stdout
    return [l for l in 出.split("\n") if l.strip()]


def 取025小节(机: str) -> tuple[str, list[str]]:
    """本机节最新小节（### N.xx 「…」）标题+正文行们。"""
    节头, _, 小节前缀 = 机节映射[机]
    行 = (仓库根 / "plans/025-三机任务统筹与实施计划.md").read_text(encoding="utf-8").split("\n")
    i节 = next(k for k, l in enumerate(行) if l.startswith(节头))
    小节们 = [k for k, l in enumerate(行) if k > i节 and l.startswith(小节前缀)]
    if not 小节们:
        raise SystemExit(f"[渲染] 025 {机}节无小节")
    i = 小节们[-1]
    j = next((k for k in range(i + 1, len(行)) if 行[k].startswith("## ")), len(行))
    return 行[i], [l for l in 行[i + 1:j] if l.strip()]


def 提炼(标题: str, 正文: list[str], 键: str) -> str:
    """从六要素行提取字段值（键如「交付」「实测数字」「写集」）——行内 **键**： 后的文本。"""
    for l in 正文:
        m = re.match(r"\d+\.\s+\*\*(.+?)[:：](.*?)\*\*(.+)", l)
        if m and 键 in m.group(1):
            return (m.group(2) + m.group(3)).strip()
    return ""


def 渲染(机: str) -> tuple[str, str]:
    标题, 正文 = 取025小节(机)
    m = (re.search(r"[「【](.+?)[」】]", 标题) or re.match(r"### \d+\.\d+ (.+?)（分支", 标题))
    轮名 = (m.group(1) if m else 标题[10:60]) .strip("「」")
    分支 = (re.search(r"分支 `([^`]+)`", 标题).group(1) if "分支 `" in 标题 else "?")
    提交们 = 取分支提交们()
    交付 = 提炼(标题, 正文, "交付")
    实测 = 提炼(标题, 正文, "实测")
    边界 = 提炼(标题, 正文, "边界")
    下一棒 = 提炼(标题, 正文, "下一棒") or "（见 025 收工回填）"
    日志条 = (f"## 功能完善总结（{轮名}·{分支}）\n\n"
             + (f"- **交付**：{交付}\n" if 交付 else "")
             + (f"- **实测**：{实测}\n" if 实测 else "")
             + (f"- **诚实边界**：{边界}\n" if 边界 else "")
             + f"- 提交：{len(提交们)} 个（{提交们[0][:8] if 提交们 else '?'}…）\n")
    交接节 = (f"{机节映射[机][1]}\n\n**交接时间**：{轮名}（分支 `{分支}`·最新 {提交们[-1][:8] if 提交们 else '?'}）。"
             f"**接手先 `git fetch`，读远端看板/分支（别信本地文件）。**\n\n"
             f"### 一、本轮做了什么\n\n1. {交付 or '见 plans/025 本机节收工回填'}\n"
             + (f"2. 实测：{实测}\n" if 实测 else "")
             + f"\n### 二、下一棒\n\n1. {下一棒}\n"
             + (f"\n### 三、坑与教训\n\n1. {边界}\n" if 边界 else "")
             + f"\n（完整六要素与收工回填=plans/025 本机节；提交清单=git log。\n）\n")
    return 日志条, 交接节


def 写入(机: str, 日志条: str, 交接节: str) -> None:
    for 文件, 节头, 新体 in (("更新日志.md", 机节映射[机][1], 日志条),
                              ("交接.md", 机节映射[机][1], 交接节)):
        p = 仓库根 / 文件
        t = p.read_text(encoding="utf-8")
        i = t.index(节头)
        m2 = re.compile(r"^## ", re.M).search(t, i + len(节头))
        j = m2.start() if m2 else len(t)
        体 = 新体.rstrip()
        if 体.startswith(节头):        # 交接草稿自带节头——剥掉统一由此拼
            体 = 体[len(节头):].lstrip("\n")
        with p.open("w", encoding="utf-8", newline="") as f:   # 295：同 integrate 收口——write_text(newline=) py3.10+ 本机 3.8 崩
            f.write(t[:i] + 节头 + "\n\n" + 体 + "\n\n" + t[j:].lstrip("\n"))
        print(f"[写入] {文件} {节头}（整节替换）")


if __name__ == "__main__":
    机 = "家机"
    args = sys.argv[1:]
    if "--machine" in args:
        机 = args[args.index("--machine") + 1]
    日志条, 交接节 = 渲染(机)
    if "--write" in args:
        写入(机, 日志条, 交接节)
        print("[提示] 已写入——必跑 check_handoff 复核后提交。")
    else:
        print("=" * 20, "更新日志草稿", "=" * 20)
        print(日志条)
        print("=" * 20, "交接草稿", "=" * 20)
        print(交接节)
