#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""plans/021 进度总表同步自检（防漂移；只读，不改任何文件）。

背景：`plans/021-任务进度观察表.md` 是全项目任务进度唯一总入口（AGENTS.md §2.2 开工必读、
§6.3 收尾必更）。本脚本检查「总表文首『最后更新』戳」是否落后于 git 提交信息里的最新轮次——
落后即提示先更新总表（AGENTS.md §6.3），避免过期总表误导后续会话；同时检查交接.md（原 HANDOFF.md）是否
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
交接路径 = 仓库根 / "交接.md"

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


def 提取最新轮列(文本: str):
    """提取 §三 平表行「最新轮」列（580-a 六列平表末列=最近实质推进轮次·老化审计机械锚）：
    匹配行尾 `| NNN-a |`（允许 -b/-c 轮内后缀）。未命中返回 None（调用方回退全文提取）。
    注：子任务号 `NNN-NNN` 为数字-数字形态，与本函数及 提取轮次 的 `NNN-a`（数字-字母）
    正则天然无碰撞（580-a 编号体系设计约束）。"""
    匹配 = re.search(r"\|\s*(\d{1,3}-[a-z](?:-[a-z])?)\s*\|\s*$", 文本)
    if 匹配:
        return int(匹配.group(1).split("-")[0])
    return None


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
    # 455-b 语义修正（机制级·全机集成阻塞根治）：窄通道直推提交（信息含
    #   「（窄通道）」——看板行/回签/通告·协议 v2 §8.5）不计入漂移基准——
    #   021 戳追踪开发轮集成进度，窄通道不改变 021 状态；他机未集成认领号
    #   （看板先行直推）计入基准会使任何开发轮集成被结构性拦截（448/455
    #   实证：455 戳 < 家机 456/457 看板先行号=非本轮欠账）。
    for 行 in 日志.stdout.splitlines():
        if "|" not in 行:
            continue
        短哈希, 标题 = 行.split("|", 1)
        if "（窄通道）" in 标题 or "(窄通道)" in 标题:
            continue
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

    # ④ 交接（原 HANDOFF）指针（仅警告）
    if 交接路径.exists() and "plans/021" in 交接路径.read_text(encoding="utf-8"):
        print("[√] 交接.md 含总表指针")
    else:
        print("[!] 警告：交接.md 未见指向 plans/021 的指针（建议按 AGENTS.md §8.3 恢复）")

    # ⑤ 挂账老化审计（2026-09-21 用户批方案甲④·仅警告；580-a 六态化+「最新轮」列锚定）：
    #    §三 带 🔄/⏸/📣/🔬 的行，行「最新轮」列（缺失则回退行内全文提取）落后 git 最新
    #    （非窄通道）轮次 >40 → 提醒当轮处置（排班/销项/显式降级说明）；
    #    无任何轮次号可引用的行不判（⏸ 挂起态以重启触发为准）。
    try:
        在队列区 = False
        for 序, 行 in enumerate(总表路径.read_text(encoding="utf-8").splitlines(), 1):
            if re.match(r"^## ", 行):
                在队列区 = 行.startswith("## 三、排班队列")
                continue
            if not 在队列区 or git值 is None:
                continue
            if not any(符 in 行 for 符 in ("🔄", "⏸", "📣", "🔬")):
                continue
            最新列 = 提取最新轮列(行)
            引用 = {最新列} if 最新列 is not None else 提取轮次(行)
            if 引用:
                最新引用 = max(引用)
                if git值 - 最新引用 > 40:
                    来源 = "最新轮列" if 最新列 is not None else "行内引用"
                    print(f"[!] 老化：L{序}（{行.strip()[:44]}…）{来源}=第{最新引用}轮 vs "
                          f"git 第{git值}轮（差 {git值 - 最新引用}>40）——当轮须处置"
                          "（排班/销项/显式降级说明·队列纪律④）")
    except Exception:
        pass  # 老化审计为增量警告，任何异常不阻断主检

    print("结论：总表与 git 最新轮次同步 ✓")
    return 0


if __name__ == "__main__":
    sys.exit(主流程())
