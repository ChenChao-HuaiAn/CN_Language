#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""miss_registry.json 三路并集合并驱动（#1259 高冲突面治理·方案③）。

背景（1259 立法）：registry 为全任务共写单点 JSON，每任务补条即撞同一文件
（1144/1142/1157/1156 连环实录；10-27 深度机 1187 轮五度冲突实证）——普通
git 合并对 JSON 数组整段冲突，人工合并疲劳且曾产出损坏 JSON 进 develop。

协议：git merge driver（.gitattributes: scripts/miss_registry.json merge=miss-registry）。
调用形如 `python scripts/merge_miss_registry.py %O %A %B`：
  %O=共同基（base） %A=当前侧（ours·就地改写为合并结果） %B=对方侧（theirs）。
语义：条目数组按 (文件, 函数, 锚文本) 键三路并集；同键字段分歧=ours 优先
  （本侧已甄别的登记不丢）；顶层 版本/说明=ours 为准。任一侧 JSON 不可解析
  或结构不符 → 退出非零回退普通冲突（fail-safe·禁静默丢条目）。

每机一次性安装（与 hooksPath 同级的一次性配置）：
  git config merge.miss-registry.name  "miss_registry 三路并集合并"
  git config merge.miss-registry.driver "python3 scripts/merge_miss_registry.py %O %A %B"
未安装时 git 回退普通冲突（无害·本脚本不被调用）。
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

# 条目身份键（登记制口径：锚文本=代码行文本·归一化后双向子串匹配——此处按
# 原文精确三元组并集；同锚不同函数/文件为不同条目，与门禁对账口径一致）
def 条目键(条: dict) -> tuple:
    return (条.get("文件", ""), 条.get("函数", ""), 条.get("锚文本", ""))


def 必填字段齐(条: dict) -> bool:
    必填 = ("文件", "锚文本", "来源", "类别", "批准")
    return all(条.get(k) for k in 必填)


def 读侧(路径: str) -> dict:
    文本 = Path(路径).read_text(encoding="utf-8")
    # 冲突标记残留（1156 实录：工具误提交半成品）——fail-safe 拒合并
    for 标记 in ("<<<<<<<", "=======", ">>>>>>>"):
        if 标记 in 文本:
            raise ValueError(f"{路径} 含冲突标记 {标记}")
    d = json.loads(文本)
    if not isinstance(d, dict) or not isinstance(d.get("条目"), list):
        raise ValueError(f"{路径} 结构不符（顶层须含 条目 数组）")
    return d


def 三路并集(base: dict, ours: dict, theirs: dict) -> dict:
    出 = dict(ours)                      # 顶层 版本/说明=ours 为准
    并: dict[tuple, dict] = {}
    for 条 in base.get("条目", []):
        if 必填字段齐(条):
            并[条目键(条)] = 条
    分歧 = 0
    for 条 in ours.get("条目", []):
        if not 必填字段齐(条):
            continue
        k = 条目键(条)
        if k in 并 and 并[k] != 条:
            分歧 += 1
        并[k] = 条                          # ours 优先
    for 条 in theirs.get("条目", []):
        if not 必填字段齐(条):
            continue
        k = 条目键(条)
        if k not in 并:                     # theirs 独有补入；同键已由 ours 定
            并[k] = 条
    # 稳定序：保持 ours 原序为骨架，theirs 新增按原序追加在后
    ours序 = [条目键(t) for t in ours.get("条目", []) if 必填字段齐(t)]
    theirs新增 = [条目键(t) for t in theirs.get("条目", [])
                 if 必填字段齐(t) and 条目键(t) not in set(ours序)]
    出["条目"] = [并[k] for k in ours序 + theirs新增]
    if 分歧:
        print(f"[miss-registry-merge] 同键字段分歧 {分歧} 条=ours 优先", file=sys.stderr)
    return 出


def 主() -> int:
    if len(sys.argv) != 4:
        print("用法: merge_miss_registry.py <base> <ours> <theirs>", file=sys.stderr)
        return 1
    try:
        base = 读侧(sys.argv[1])
        ours = 读侧(sys.argv[2])
        theirs = 读侧(sys.argv[3])
        出 = 三路并集(base, ours, theirs)
    except (ValueError, json.JSONDecodeError, OSError) as e:
        print(f"[miss-registry-merge] fail-safe 回退普通冲突: {e}", file=sys.stderr)
        return 1
    Path(sys.argv[2]).write_text(
        json.dumps(出, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"[miss-registry-merge] 三路并集完成：{len(出['条目'])} 条")
    return 0


if __name__ == "__main__":
    sys.exit(主())
