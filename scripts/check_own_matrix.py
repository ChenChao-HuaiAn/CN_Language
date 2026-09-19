#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""020 拥有权流转矩阵解析器（426-a 立·AGENTS.md §6.7「020=纯状态层」的机械自检）。

用途（矩阵 markdown 表格本身=唯一数据源·人机同读·不建第二份 JSON 防双源漂移）：
  ① 结构校验：§一 A/B 两表列数一致性（A=7 列·B=5 列）——表格被手改坏即报错；
  ② 靶子统计：待办列含「靶子」的格子清单（空格=下一轮工作面·排班输入）；
  ③ 两侧缺口：A 表 v2 验证列为「—」的格子（宿主有验证而 v2 侧未验的面）；
  ④ 已销项统计：含「已销项」的格子数（收敛度量）。

用法：python3 scripts/check_own_matrix.py
退出码：0 = 结构正常（统计为信息输出）；1 = 表格结构损坏（禁止提交）。
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent
矩阵路径 = 仓库根 / "plans" / "020-拥有权流转矩阵与内存安全防线.md"


def 解析() -> int:
    if not 矩阵路径.exists():
        print("[×] plans/020 缺失")
        return 1
    行们 = 矩阵路径.read_text(encoding="utf-8").splitlines()
    问题: list[str] = []
    靶子行: list[str] = []
    v2缺口: list[str] = []
    已销项 = 0
    当前表 = None
    a列数 = b列数 = None

    for 行 in 行们:
        if 行.startswith("### A. "):
            当前表 = "A"
        elif 行.startswith("### B. "):
            当前表 = "B"
        elif 行.startswith("## "):
            当前表 = None
        if not 行.startswith("|") or not 当前表:
            continue
        单元 = [c.strip() for c in 行.strip().strip("|").split("|")]
        if all(re.fullmatch(r":?-+:?", c) for c in 单元):  # 分隔行=列数权威
            if 当前表 == "A":
                a列数 = len(单元)
            else:
                b列数 = len(单元)
            continue
        if 行.startswith("| #") or 单元[0] == "#":
            continue  # 表头行
        期望 = a列数 if 当前表 == "A" else b列数
        if 期望 and len(单元) != 期望:
            问题.append(f"{当前表} 表行 #{单元[0] if 单元 else '?'}：{len(单元)} 列 ≠ 表头 {期望} 列")
            continue
        全行 = 行
        if "靶子" in 全行 and "已销项" not in 全行:
            靶子行.append(f"[{当前表}#{单元[0]}] {单元[1][:36]}")
        if 当前表 == "A" and len(单元) >= 6 and 单元[5] in ("—", "-", ""):
            v2缺口.append(f"[A#{单元[0]}] {单元[1][:36]}")
        if "已销项" in 全行:
            已销项 += 1

    print("=== 020 拥有权矩阵解析（结构校验+收敛统计）===")
    print(f"  表结构：A 表 {a列数} 列｜B 表 {b列数} 列")
    print(f"  在册靶子（待办列含「靶子」）：{len(靶子行)} 格")
    for s in 靶子行:
        print("    " + s)
    print(f"  v2 验证缺口（A 表 v2 验证列=—）：{len(v2缺口)} 格")
    for s in v2缺口[:12]:
        print("    " + s)
    if len(v2缺口) > 12:
        print(f"    …等共 {len(v2缺口)} 格")
    print(f"  已销项格子：{已销项} 格（收敛度量·对照上轮看进度）")
    if 问题:
        print(f"\n结论：表格结构缺陷 {len(问题)} 项，禁止提交 ✗")
        for s in 问题[:10]:
            print("[×] " + s)
        return 1
    print("\n结论：矩阵结构正常 ✓（统计为排班输入·靶子清单=plans/021 §三 候选源）")
    return 0


if __name__ == "__main__":
    sys.exit(解析())
