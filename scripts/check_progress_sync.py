#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""plans/021 进度总表同步自检（防漂移；只读，不改任何文件）。

背景：`plans/021-任务进度观察表.md` 是全项目任务进度唯一总入口（AGENTS.md §2.2 开工必读、
§6.3 收尾必更）。本脚本检查「总表文首『最后更新』戳」是否落后于 git 提交信息里的最新轮次——
落后即提示先更新总表（AGENTS.md §6.3），避免过期总表误导后续会话；同时检查 HANDOFF.md 是否
仍含总表指针（AGENTS.md §8.3）。

说明：本检查是**启发式自检**（靠提交信息中的轮次字样），不是硬门禁；报警时请人工核对总表。

用法：python3 scripts/check_progress_sync.py
退出码：0 = 同步（含仅有警告的情形）；1 = 漂移或总表缺失。
"""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent
总表路径 = 仓库根 / "plans" / "021-任务进度观察表.md"
交接路径 = 仓库根 / "HANDOFF.md"

中文位 = {"零": 0, "〇": 0, "一": 1, "二": 2, "三": 3, "四": 4,
          "五": 5, "六": 6, "七": 7, "八": 8, "九": 9}


def 中文转数(文本: str):
    """中文数字 → 整数（0~999，覆盖本项目轮次号）；纯阿拉伯数字直接转换。"""
    if not 文本:
        return None
    if 文本.isdigit():
        return int(文本)
    if "百" in 文本:
        左, _, 右 = 文本.partition("百")
        值 = (中文位.get(左, 1) if 左 else 1) * 100
        if 右:
            if "十" in 右:
                十位, _, 个位 = 右.partition("十")
                值 += (中文位.get(十位, 1) if 十位 else 1) * 10
                值 += 中文位.get(个位, 0) if 个位 else 0
            else:
                值 += 中文位.get(右, 0)
        return 值
    if "十" in 文本:
        十位, _, 个位 = 文本.partition("十")
        值 = (中文位.get(十位, 1) if 十位 else 1) * 10
        值 += 中文位.get(个位, 0) if 个位 else 0
        return 值
    return 中文位.get(文本)


def 提取轮次(文本: str) -> set:
    """提取文本中的轮次号：`第N轮`（阿拉伯/中文数字）与轮次写法 `NN-a`（如 89-a）。"""
    结果 = set()
    for 匹配 in re.finditer(r"第\s*([0-9零〇一二三四五六七八九十百]+)\s*轮", 文本):
        值 = 中文转数(匹配.group(1))
        if 值 is not None:
            结果.add(值)
    for 匹配 in re.finditer(r"(?<![0-9])(\d{1,3})\s*-\s*[a-z](?![a-z])", 文本):
        结果.add(int(匹配.group(1)))
    return 结果


def 主流程() -> int:
    print("=== plans/021 进度总表同步自检（防漂移，只读）===")

    if not 总表路径.exists():
        print("[×] 总表缺失：plans/021-任务进度观察表.md")
        print("    修复：恢复该总表并保持每轮更新（AGENTS.md §2.2 / §6.3）")
        return 1
    print(f"[√] 总表存在：{总表路径.relative_to(仓库根)}")

    # ① 解析文首「最后更新」戳（扫描全部含该词的行，取首个可解析出轮次的——正文其它处可能
    #    出现「刷新『最后更新』戳」这类描述句，无轮次号，天然被跳过）
    戳值 = None
    戳行 = ""
    for 行 in 总表路径.read_text(encoding="utf-8").splitlines():
        if "最后更新" not in 行:
            continue
        行文本 = 行.strip().lstrip(">").strip()
        轮次们 = 提取轮次(行文本)
        if 轮次们:
            戳值, 戳行 = max(轮次们), 行文本
            break
    if 戳值 is None:
        print("[×] 未找到可解析的「最后更新」戳")
        print("    期望形如：> **最后更新：第 90 轮**（YYYY-MM-DD，<机器>）")
        return 1
    print(f"[√] 总表戳：第 {戳值} 轮   ← {戳行[:96]}")

    # ② git 历史最新轮次
    日志 = subprocess.run(["git", "log", "--format=%h|%s", "-300"],
                         cwd=仓库根, capture_output=True, text=True, check=False)
    if 日志.returncode != 0:
        print("[!] 无法读取 git 历史（跳过轮次比对）")
        return 0
    git值, git来源 = None, ""
    for 行 in 日志.stdout.splitlines():
        if "|" not in 行:
            continue
        短哈希, 标题 = 行.split("|", 1)
        for 值 in 提取轮次(标题):
            if git值 is None or 值 > git值:
                git值, git来源 = 值, f"{短哈希} {标题[:64]}"
    if git值 is None:
        print("[!] git 历史未出现轮次字样（跳过轮次比对）")
    else:
        print(f"[√] git 历史最新轮次：第 {git值} 轮   ← {git来源}")

    # ③ 漂移判定
    if git值 is not None and 戳值 < git值:
        print(f"[×] 漂移：总表戳（第 {戳值} 轮）落后于 git 最新轮次（第 {git值} 轮）")
        print("    修复：更新 plans/021（§2 进度数字 / §3 排班队列 / §5 轮次索引）"
              "并刷新文首「最后更新」戳（AGENTS.md §6.3）")
        return 1

    # ④ HANDOFF 指针（仅警告）
    if 交接路径.exists() and "plans/021" in 交接路径.read_text(encoding="utf-8"):
        print("[√] HANDOFF.md 含总表指针")
    else:
        print("[!] 警告：HANDOFF.md 未见指向 plans/021 的指针（建议按 AGENTS.md §8.3 恢复）")

    print("结论：总表与 git 最新轮次同步 ✓")
    return 0


if __name__ == "__main__":
    sys.exit(主流程())
