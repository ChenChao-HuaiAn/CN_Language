#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""高冲突单点 JSON 写后机械校验闸（#1259 方案④·任一治理方案落地前的正交防线）。

背景（1259 立法+1156 实录）：冲突不止于并发合并，「工具误提交半成品」（带冲突
标记/损坏 JSON）同样致损且曾进 develop。本闸对两高冲突单点文件做写后校验：
  scripts/miss_registry.json     —— JSON 可解析+无冲突标记+条目必填字段+批准枚举
  website/tools/doc_coverage.json —— JSON 可解析+无冲突标记+顶层结构
红=exit 1（可行动诊断·L1 静态段挂点）。selftest=test_merge_drivers.py 同批。
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

冲突标记 = ("<<<<<<<", ">>>>>>>")
批准枚举 = {"已批", "存量过渡", "呈报中", "已毙"}
# 硬必填窄集（与 merge_miss_registry.py 必填字段齐 同口径）——登记人/登记日/函数/
#   静默风险/原因 为历史演进字段（1238 前存量条不全·存量兼容不卡）
条目必填 = ("文件", "锚文本", "来源", "类别", "批准")


def 校验_registry(路径: str, 红: list) -> None:
    p = Path(路径)
    if not p.exists():
        红.append(f"{路径} 不存在")
        return
    文本 = p.read_text(encoding="utf-8")
    for 标记 in 冲突标记:
        if 标记 in 文本:
            行 = 文本[:文本.index(标记)].count("\n") + 1
            红.append(f"{路径}:{行} 含冲突标记 {标记}（半成品禁提交·1156 实录）")
    try:
        d = json.loads(文本)
    except json.JSONDecodeError as e:
        红.append(f"{路径} JSON 不可解析: {e}")
        return
    条目 = d.get("条目") if isinstance(d, dict) else None
    if not isinstance(条目, list):
        红.append(f"{路径} 顶层须含 条目 数组")
        return
    for i, 条 in enumerate(条目):
        for 字段 in 条目必填:
            if not 条.get(字段):
                红.append(f"{路径} 条目[{i}] 缺必填字段 {字段}")
        if 条.get("批准") not in 批准枚举:
            红.append(f"{路径} 条目[{i}] 批准字段非法: {条.get('批准')!r}（须属 {sorted(批准枚举)}）")


def 校验_doc_coverage(路径: str, 红: list) -> None:
    p = Path(路径)
    if not p.exists():
        红.append(f"{路径} 不存在")
        return
    文本 = p.read_text(encoding="utf-8")
    for 标记 in 冲突标记:
        if 标记 in 文本:
            行 = 文本[:文本.index(标记)].count("\n") + 1
            红.append(f"{路径}:{行} 含冲突标记 {标记}")
    try:
        d = json.loads(文本)
    except json.JSONDecodeError as e:
        红.append(f"{路径} JSON 不可解析: {e}")
        return
    if not isinstance(d, dict) or not isinstance(d.get("特性清单"), list):
        红.append(f"{路径} 顶层须含 特性清单 数组")


def 主() -> int:
    红: list = []
    校验_registry("scripts/miss_registry.json", 红)
    校验_doc_coverage("website/tools/doc_coverage.json", 红)
    if 红:
        print(f"[registry-sanity] ✗ {len(红)} 红（写后机械校验·1259 方案④）")
        for r in 红:
            print(f"  [×] {r}")
        return 1
    print("[registry-sanity] ✓ 两高冲突单点 JSON 可解析/无冲突标记/schema 齐")
    return 0


if __name__ == "__main__":
    sys.exit(主())
