#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""共享文档结构门禁（AGENTS.md §8.3「共享文档编辑六纪律」的机械自检；只读，不改任何文件）。

背景：HANDOFF.md 曾因「锚点局部插入＋旧文保留」的编辑方式逐轮翻倍（642 处重复标题、9268 行），
本脚本在提交前拦截同类结构缺陷——健康态 HANDOFF.md 约 140 行（三机节各 ~40 行）。

检查面：
  HANDOFF.md  — ① 三机节齐全 ② 每机节「### 一、」恰好 1 次（一分区一记录）
                ③ 节内标题无重复 ④ 无行中粘连标题（「正文。### 标题」= 事故签名）
                ⑤ 全文 ≤400 行、每机节 ≤120 行
  更新日志.md — ① 三机节齐全 ② 无行中粘连标题 ③ 全文 ≤400 行
                （注：机节内含 `## 功能完善总结` 子标题，故不做 `^## ` 切分与节内去重）

用法：python3 scripts/check_handoff.py
退出码：0 = 结构正常；1 = 存在结构缺陷（禁止提交，见 AGENTS.md §6.6）。
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent

HANDOFF_总行上限 = 400   # 健康态约 140 行
HANDOFF_单节上限 = 120   # 一分区一记录（六纪律 2）
日志_总行上限 = 400
机器关键词 = ("家机", "深度机", "单位机")
粘连标题模式 = re.compile(r"#{2,6}\s*[\u4e00-\u9fffA-Za-z0-9]")


def 粘连行清单(行们: list[str]) -> list[str]:
    """找出「行中粘连标题」行：非行首处出现 ##~###### 且其后直接跟文字（六纪律 3 的事故签名）。"""
    命中: list[str] = []
    for 序, 行 in enumerate(行们, 1):
        if re.match(r"\s*#{1,6}\s", 行):
            continue  # 行首标题本就合法
        检查文本 = 行.split("](#", 1)[0]  # 排除链接锚点 ](#xxx)
        检查文本 = re.sub(r"`[^`]*`", "", 检查文本)  # 排除行内代码对标题字样的提及（如「`## 功能完善总结`」）
        if 粘连标题模式.search(检查文本):
            命中.append(f"      L{序}: {行.strip()[:72]}")
    return 命中


def 按二号标题分节(行们: list[str]) -> dict[str, list[str]]:
    """按 `^## ` 切分为 {节标题: 节内容行}；首个 `## ` 之前归入「(序章)」。"""
    节s: dict[str, list[str]] = {"(序章)": []}
    当前 = "(序章)"
    for 行 in 行们:
        if re.match(r"^## \S", 行):
            当前 = 行.strip()
            节s[当前] = []
        else:
            节s[当前].append(行)
    return 节s


def 查交接() -> list[str]:
    """HANDOFF.md 结构检查，返回问题清单（空=通过）。"""
    问题: list[str] = []
    路径 = 仓库根 / "HANDOFF.md"
    if not 路径.exists():
        return ["HANDOFF.md 缺失"]
    行们 = 路径.read_text(encoding="utf-8").splitlines()
    节s = 按二号标题分节(行们)
    机器节 = {(名, 标题): 内容
              for 标题, 内容 in 节s.items()
              for 名 in 机器关键词 if 名 in 标题}

    # ① 三机节齐全
    for 名 in 机器关键词:
        if not any(名 == 短名 for (短名, _标题) in 机器节):
            问题.append(f"① HANDOFF.md 缺「{名}」分节")

    # ②③⑤ 逐机节检查
    for (名, 标题), 内容 in 机器节.items():
        一们 = [行 for 行 in 内容 if re.match(r"^### 一、", 行)]
        if len(一们) != 1:
            问题.append(f"② 「{名}」节『### 一、』出现 {len(一们)} 次（应恰 1 次——一分区一记录，"
                        f"上轮记录须随整节替换消失）")
        标题们 = [行.strip() for 行 in 内容 if re.match(r"#{1,6}\s", 行)]
        重复 = sorted({标题 for 标题 in 标题们 if 标题们.count(标题) > 1})
        if 重复:
            样例 = "；".join(重复[:3])
            问题.append(f"③ 「{名}」节内标题重复 {len(重复)} 种（应每轮整节替换后仅 1 份）：{样例}")
        if len(内容) > HANDOFF_单节上限:
            问题.append(f"⑤ 「{名}」节 {len(内容)} 行 > 上限 {HANDOFF_单节上限}（一分区一记录）")

    # ④ 全文粘连标题
    粘连 = 粘连行清单(行们)
    if 粘连:
        问题.append("④ HANDOFF.md 存在行中粘连标题（六纪律 3 事故签名，编辑锚点必选错）：\n"
                    + "\n".join(粘连[:6]) + (f"\n      …等共 {len(粘连)} 行" if len(粘连) > 6 else ""))

    # ⑤ 全文行数
    if len(行们) > HANDOFF_总行上限:
        问题.append(f"⑤ HANDOFF.md 全文 {len(行们)} 行 > 上限 {HANDOFF_总行上限}")
    return 问题


def 查日志() -> list[str]:
    """更新日志.md 结构检查，返回问题清单（空=通过）。"""
    问题: list[str] = []
    路径 = 仓库根 / "更新日志.md"
    if not 路径.exists():
        return ["更新日志.md 缺失"]
    文本 = 路径.read_text(encoding="utf-8")
    行们 = 文本.splitlines()
    for 名 in 机器关键词:
        if not re.search(rf"^## .*{名}", 文本, re.M):
            问题.append(f"更新日志.md 缺「{名}」节")
    粘连 = 粘连行清单(行们)
    if 粘连:
        问题.append("更新日志.md 存在行中粘连标题：\n" + "\n".join(粘连[:6]))
    if len(行们) > 日志_总行上限:
        问题.append(f"更新日志.md 全文 {len(行们)} 行 > 上限 {日志_总行上限}")
    return 问题


def 主流程() -> int:
    print("=== 共享文档结构门禁（六纪律机械自检，只读）===")
    交接路径 = 仓库根 / "HANDOFF.md"
    if 交接路径.exists():
        print("  HANDOFF.md 各节行数：")
        for 标题, 内容 in 按二号标题分节(交接路径.read_text(encoding="utf-8").splitlines()).items():
            print(f"    {标题[:44]}：{len(内容)} 行")

    全部问题 = 查交接() + 查日志()
    if 全部问题:
        print(f"\n结论：结构缺陷 {len(全部问题)} 项，禁止提交 ✗")
        for 问题 in 全部问题:
            print("[×] " + 问题)
        return 1
    print("\n结论：HANDOFF.md / 更新日志.md 结构正常 ✓")
    return 0


if __name__ == "__main__":
    sys.exit(主流程())
